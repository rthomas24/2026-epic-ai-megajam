# SPDX-License-Identifier: MIT
# Triad Drift - cooked-size budget for the Fab intake folders (Props/Fab/**, Characters/Shared/Anims/Fab), headless.
#   1. Maximum Texture Size per folder (Lead, Nov 14: the Shipping zip is at 0.90 GB of 1 GB): Mechanic Base 1024,
#      PipeBeam 2048 with its normal and ORM at 1024, the tiling sets 2048 with normal and ORM at 1024 (the same rule, to
#      land near the 40 MB budget), the first batch (crates, core, barrel: 1 m props) 1024.
#   2. Mechanic Base: deletes every texture and material in its folder that the 22 kept meshes do not reach (through
#      their materials, instance parents and masters); the _orig instances are kept (Ryan's keep-replaced-art rule) and
#      counted.
#   3. Estimates the cooked size of each asset from the capped dimensions and the format (DXT1 0.5 B/px, DXT5 / BC5 /
#      BC7 1 B/px, full mip chain x 4/3; meshes from their triangle count; animations from the package size) and prints
#      it per folder. TD_FAB_DRY=1 reports without changing anything.
import json
import os

import unreal

eas = unreal.EditorAssetLibrary
reg = unreal.AssetRegistryHelpers.get_asset_registry()
DRY = os.environ.get("TD_FAB_DRY", "") == "1"
FAB = "/Game/TriadDrift/Gameplay/Props/Fab"
ANIMS = "/Game/TriadDrift/Gameplay/Characters/Shared/Anims/Fab"
MB = FAB + "/MechanicBase"
CAPS = [  # (folder prefix, default cap, {name suffix: cap})
    (MB, 1024, {}),
    (FAB + "/PipeBeam", 2048, {"_N": 1024, "_ORM": 1024}),
    (FAB + "/Textures", 2048, {"_N": 1024, "_ORM": 1024}),
    (FAB, 1024, {}),
]
TCS = unreal.TextureCompressionSettings


def log(m):
    unreal.log("TDPY: " + m)


def cap_for(path):
    for prefix, default, special in CAPS:
        if path.startswith(prefix + "/"):
            for suffix, c in special.items():
                if path.split(".")[0].endswith(suffix):
                    return c
            return default
    return None


def deps(pkg):
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    return [str(d) for d in (reg.get_dependencies(pkg, opts) or [])]


# --- Mechanic Base: what the 22 meshes reach
mb_all = [p.split(".")[0] for p in eas.list_assets(MB, recursive=True, include_folder=False)]
meshes = [p for p in mb_all if isinstance(eas.find_asset_data(p).get_asset(), unreal.StaticMesh)]
reach = set(meshes)
todo = list(meshes)
while todo:
    p = todo.pop()
    for d in deps(p):
        if d.startswith(MB) and d not in reach:
            reach.add(d)
            todo.append(d)
origs = [p for p in mb_all if p.endswith("_orig")]
unused = [p for p in mb_all if p not in reach and p not in origs]
log("MechanicBase: %d packages, %d meshes, %d reached, %d _orig kept, %d unused: %s" % (
    len(mb_all), len(meshes), len(reach), len(origs), len(unused), [u.rsplit("/", 1)[1] for u in unused]))
if not DRY:
    for u in unused:
        eas.delete_asset(u)

# --- caps and estimate
rows = {}


def add(group, key, mb):
    rows.setdefault(group, {}).setdefault(key, 0.0)
    rows[group][key] += mb


def group_of(path):
    if path.startswith(ANIMS):
        return "Anims"
    rest = path[len(FAB) + 1:].split("/")
    return rest[0] if rest[0] != "Textures" else "Textures/" + rest[1]


paths = [p.split(".")[0] for p in eas.list_assets(FAB, recursive=True, include_folder=False)] + \
        [p.split(".")[0] for p in eas.list_assets(ANIMS, recursive=True, include_folder=False)]
tex_rows = []
for p in paths:
    a = eas.load_asset(p)
    g = group_of(p)
    if isinstance(a, unreal.Texture2D):
        c = cap_for(p)
        if c and not DRY:
            a.set_editor_property("max_texture_size", c)
            eas.save_loaded_asset(a, False)
        c = a.get_editor_property("max_texture_size") or 16384
        w, h = a.blueprint_get_size_x(), a.blueprint_get_size_y()
        src = max(w, h)
        s = min(src, c) / float(src) if src else 1.0
        cw, ch = max(1, int(w * s)), max(1, int(h * s))
        cs = a.get_editor_property("compression_settings")
        alpha = not a.get_editor_property("compression_no_alpha") and a.get_editor_property("srgb") and cs == TCS.TC_DEFAULT and False
        if cs in (TCS.TC_NORMALMAP, TCS.TC_BC7):
            bpp, fmt = 1.0, "BC5" if cs == TCS.TC_NORMALMAP else "BC7"
        elif cs == TCS.TC_HDR:
            bpp, fmt = 8.0, "RGBA16F"
        else:
            bpp, fmt = 0.5, "DXT1"
        mbytes = cw * ch * bpp * 4.0 / 3.0 / (1024 * 1024)
        add(g, "textures", mbytes)
        tex_rows.append((g, a.get_name(), "%dx%d" % (w, h), "%dx%d" % (cw, ch), fmt, round(mbytes, 2)))
    elif isinstance(a, unreal.StaticMesh):
        tris = int(a.get_num_triangles(0))
        nanite = a.get_editor_property("nanite_settings").enabled
        # Nanite: ~40 B per source triangle plus the fallback (counted in tris above when Nanite is on); else ~60 B/tri
        add(g, "meshes", tris * (40.0 if nanite else 60.0) / (1024 * 1024))
    elif isinstance(a, unreal.AnimSequence):
        f = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir() + p[len("/Game/"):] + ".uasset")
        add(g, "anims", os.path.getsize(f) / (1024 * 1024) if os.path.isfile(f) else 0.3)
    else:
        add(g, "other", 0.01)
total = 0.0
for g, r in sorted(rows.items()):
    t = sum(r.values())
    total += t
    log("BUDGET %-28s %6.1f MB  %s" % (g, t, json.dumps({k: round(v, 1) for k, v in r.items()})))
log("BUDGET TOTAL %.1f MB (uncompressed estimate, before the pak's Oodle compression)" % total)
for row in sorted(tex_rows):
    log("TEX %s %s src %s cooked %s %s %.2f MB" % row)
log("OK")
