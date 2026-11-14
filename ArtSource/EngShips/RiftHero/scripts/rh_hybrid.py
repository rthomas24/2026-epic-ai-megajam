# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero HYBRID: the Rodin fuselage, canopy, wings, guns and fins (cleaned, same UVs) with the
# ring, engines and belly tail replaced by designed hard-surface parts (rh_parts.py).
#   blender --background --factory-startup --python rh_hybrid.py -- [--ship Rift] [--tag v01] [--close] [--no-bake]
#                                                                    [--no-export] [--no-sheets] [--out-root <dir>]
# Writes Rift_hybrid_<tag>.blend, export/T_Rift_* (corrected Rodin maps), export/T_Rift_Parts_* (baked),
# review sheets, and (unless --no-export) export/SM_Rift.fbx, SM_Rift_PortalDisc.fbx and manifests.
# --ship Lancer | Bastion (rh_ships.py): the same pass for another squad ship, into ArtSource/EngShips/<Ship>Hero/:
# no ring or engine parts, the close-range parts of its config, its nozzle glow cores (rs_extras.py) kept.
# --out-root <dir>: write everything there instead (a check run that leaves the repo alone).
import json
import math
import os
import shutil
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C
import rh_geo as G
import rh_parts as P
import rh_shots as S
import rh_textures as T
import rh_closeparts as CP
import rh_ships

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
TAG = ARGS[ARGS.index("--tag") + 1] if "--tag" in ARGS else "v01"
SHIP = ARGS[ARGS.index("--ship") + 1] if "--ship" in ARGS else "Rift"
CFG = rh_ships.get(SHIP)
C.use(CFG, ARGS[ARGS.index("--out-root") + 1] if "--out-root" in ARGS else None)
CP.configure(CFG)
HEAT = CFG["heat"]
SOCKETS_UE = CFG["sockets"]            # Unreal cm
SM = "SM_" + SHIP


def islands(bm):
    bm.faces.ensure_lookup_table()
    seen = set()
    out = []
    for f in bm.faces:
        if f.index in seen:
            continue
        stack, comp = [f], []
        seen.add(f.index)
        while stack:
            g = stack.pop()
            comp.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in seen:
                        seen.add(h.index)
                        stack.append(h)
        out.append(comp)
    return out


def clean_body(ob):
    st = {}
    me = ob.data
    st["tris_in"] = len(me.polygons)
    ring = CFG["ring"]                 # the Rift: its Rodin ring and engine block go (rh_parts.py replaces them)
    st["cut_faces"] = P.cut_rodin(ob) if ring else 0
    bm = bmesh.new()
    bm.from_mesh(me)
    # leftovers of the Rodin ring supports between the new ring and the fins: purple glassy shards. They are
    # told from the (magenta) fins by their texture colour.
    bc = T.load(os.path.join(C.RODIN, "T_%s_BC.png" % SHIP)) if ring else None
    hh, ww = bc.shape[:2] if ring else (0, 0)
    uvl = bm.loops.layers.uv.active
    shards = []
    for f in (bm.faces if ring else ()):
        c = f.calc_center_median()
        if -2.15 <= c.x <= -1.20 and 0.34 <= abs(c.y) <= 0.80 and c.z > 0.02 and                 math.hypot(c.y, c.z - P.RING_C[2]) < 0.92:
            u = sum(l[uvl].uv.x for l in f.loops) / len(f.loops)
            v = sum(l[uvl].uv.y for l in f.loops) / len(f.loops)
            r, g, b = bc[min(hh - 1, max(0, int(v * hh))), min(ww - 1, max(0, int(u * ww)))]
            if b > r * 1.05 or max(r, g, b) < 0.22:
                shards.append(f)
    st["ring_support_shards_removed"] = len(shards)
    bmesh.ops.delete(bm, geom=shards, context="FACES")
    v0 = len(bm.verts)
    if CFG.get("weld", True):          # (the Null fleet's low-poly hulls stay as Rodin made them: welding the fighter
        bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0002)      # left faces wound against their neighbours)
        bmesh.ops.dissolve_degenerate(bm, dist=1e-6, edges=bm.edges)
    st["welded_verts"] = v0 - len(bm.verts)
    bm.faces.index_update()
    isl = islands(bm)
    st["islands_after_weld"] = len(isl)
    kill = []
    n_tiny = n_zone = 0
    for comp in isl:
        vs = {v for f in comp for v in f.verts}
        lo = Vector((min(v.co.x for v in vs), min(v.co.y for v in vs), min(v.co.z for v in vs)))
        hi = Vector((max(v.co.x for v in vs), max(v.co.y for v in vs), max(v.co.z for v in vs)))
        diag = (hi - lo).length
        in_ring_zone = ring and lo.x > -2.15 and hi.x < -1.15 and max(abs(lo.y), abs(hi.y)) < 0.74 and lo.z > 0.02 \
            and diag < 0.9
        if CFG.get("keep_islands"):        # low-poly Rodin hulls (the Null fleet) are built of many small separate faces
            continue
        if (len(comp) < 16 and diag < 0.06) or len(comp) < 3:
            kill += comp
            n_tiny += 1
        elif in_ring_zone:
            kill += comp
            n_zone += 1
    st["tiny_islands_removed"], st["ring_zone_islands_removed"] = n_tiny, n_zone
    bmesh.ops.delete(bm, geom=list(set(kill)), context="FACES")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(me)
    bm.free()
    me.update()
    return st


def fib_dirs(n):
    out = []
    g = math.pi * (3 - math.sqrt(5))
    for i in range(n):
        z = 1 - 2 * (i + 0.5) / n
        r = math.sqrt(1 - z * z)
        out.append(Vector((r * math.cos(g * i), r * math.sin(g * i), z)))
    return out


def remove_hidden(ob, others):
    """Delete body faces that cannot be seen from anywhere outside (interior junk, and geometry now buried in
    the new parts). A face stays if any of 64 rays from its centre escapes."""
    dg = bpy.context.evaluated_depsgraph_get()
    verts, polys = [], []
    for o in [ob] + others:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        base = len(verts)
        verts += [o.matrix_world @ v.co for v in me.vertices]
        polys += [[base + i for i in p.vertices] for p in me.polygons]
        ev.to_mesh_clear()
    bvh = BVHTree.FromPolygons(verts, polys)
    dirs = fib_dirs(64)
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    kill = []
    for f in bm.faces:
        c = f.calc_center_median()
        n = f.normal
        seen = False
        for d in [n, -n] + dirs:
            if d.length < 1e-6:
                continue
            hit = bvh.ray_cast(c + d * 0.003, d, 8.0)
            if hit[0] is None:
                seen = True
                break
        if not seen:
            kill.append(f)
    n = len(kill)
    bmesh.ops.delete(bm, geom=kill, context="FACES")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()
    return n


def image_material(name, stem_path, emission_strength, heat=None):
    """Preview material = what Unreal's M_TD_Ship shows: BC, ORM, DirectX normal (green flipped back), E."""
    old = bpy.data.materials.get(name)                 # e.g. the material the Rodin FBX brought in
    if old is not None:
        old.name = "SRC_" + name
    mat = bpy.data.materials.new(name)
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")

    def img(suffix, cs):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(stem_path + suffix + ".png", check_existing=True)
        n.image.colorspace_settings.name = cs
        return n
    nt.links.new(img("BC", "sRGB").outputs["Color"], bsdf.inputs["Base Color"])
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(img("ORM", "Non-Color").outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    if os.path.isfile(stem_path + "E.png"):
        nt.links.new(img("E", "sRGB").outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = emission_strength
    nsep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(img("N", "Non-Color").outputs["Color"], nsep.inputs["Color"])
    inv = nt.nodes.new("ShaderNodeMath")
    inv.operation = "SUBTRACT"
    inv.inputs[0].default_value = 1.0
    nt.links.new(nsep.outputs["Green"], inv.inputs[1])
    comb = nt.nodes.new("ShaderNodeCombineColor")
    nt.links.new(nsep.outputs["Red"], comb.inputs["Red"])
    nt.links.new(inv.outputs["Value"], comb.inputs["Green"])
    nt.links.new(nsep.outputs["Blue"], comb.inputs["Blue"])
    nm = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(comb.outputs["Color"], nm.inputs["Color"])
    nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def join(objs, name):
    dg = bpy.context.evaluated_depsgraph_get()
    for o in objs:                                   # apply modifiers (bevel, weighted normals)
        if o.modifiers:
            ev = o.evaluated_get(dg)
            me = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=dg)
            o.modifiers.clear()
            o.data = me
    target = objs[0]
    with bpy.context.temp_override(active_object=target, object=target, selected_objects=objs,
                                   selected_editable_objects=objs):
        bpy.ops.object.join()
    target.name = name
    target.data.name = name
    return target


def smart_uv(ob):
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    ob.select_set(True)
    vl.objects.active = ob
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.004, area_weight=0.0)
    bpy.ops.uv.pack_islands(margin=0.003)
    bpy.ops.object.mode_set(mode="OBJECT")


def main():
    t0 = time.time()
    res = {"tag": TAG}
    bpy.ops.wm.read_factory_settings(use_empty=True)
    os.makedirs(C.EXPORT, exist_ok=True)

    # ---------------- body
    res["ship"] = SHIP
    body = C.import_rodin(SM)
    body.data.materials.clear()
    res["body"] = clean_body(body)
    if CFG["ring"]:
        mats, parts, disc = P.build_all()
    else:
        mats, parts, disc = P.materials(rh_ships.ACCENTS[CFG["accent"]]), [], None
    glass = None
    if "--close" in ARGS:                       # close-range parts: canopy, guns, nose, spine (rh_closeparts.py)
        extra, glass, res["close"] = CP.build(mats, body)
        parts += extra
    bpy.context.view_layer.update()
    res["body"]["hidden_faces_removed"] = remove_hidden(body, parts)
    if CFG.get("weld", True):          # unwelded hulls keep Rodin's own normals
        body.data.shade_smooth()
        body.data.set_sharp_from_angle(angle=math.radians(CFG.get("sharp_deg", 42)))
        wn = body.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
        wn.keep_sharp = True
        wn.weight = 50
    res["body"]["tris_out"] = G.tri_count([body])

    # ---------------- textures of the kept Rodin set
    raw = CFG.get("textures") == "raw"     # the Rodin maps as they are (Bastion: grey armour, not white paint to flatten)
    if raw:
        for suffix in ("BC", "N", "ORM", "E"):
            shutil.copyfile(os.path.join(C.RODIN, "T_%s_%s.png" % (SHIP, suffix)), os.path.join(C.EXPORT, "T_%s_%s.png" % (SHIP, suffix)))
        res["textures"] = "source maps, unchanged"
    else:
        res["textures"] = T.clean(C.RODIN, C.EXPORT, SHIP, CFG["accent"])
    if HEAT:
        shutil.copyfile(HEAT, os.path.join(C.EXPORT, "T_%s_HeatMask.png" % SHIP))
    if "--close" in ARGS and not raw:
        res["livery"] = CP.clean_livery(os.path.join(C.EXPORT, "T_%s_BC.png" % SHIP), body=body)
    body.data.materials.append(image_material("M_" + SHIP, os.path.join(C.EXPORT, "T_%s_" % SHIP), 2.0))

    # ---------------- new parts: one mesh, one texture set
    res["parts_tris"] = {p.name: G.tri_count([p]) for p in parts}
    part = join(parts, SM + "_Parts")
    smart_uv(part)
    if "--no-bake" not in ARGS:
        bk = C.skill("blender-texturing/scripts/bake_texture_set.py")
        b = bk["main"](target=part.name, out_dir=C.EXPORT, asset_name=SHIP + "_Parts", size=2048, samples=8,
                       ao_samples=48, margin=8, build_preview_material=False)
        res["bake"] = {k: b.get(k) for k in ("emission_strength", "seconds", "normal_convention")}
        stem = os.path.join(C.EXPORT, "T_%s_Parts_" % SHIP)
        bc, orm = T.load(stem + "BC.png"), T.load(stem + "ORM.png")
        ao = orm[..., 0:1]
        T.save(bc * (0.50 + 0.50 * ao ** 1.2), stem + "BC.png", True)          # grime in the recesses
        if HEAT:
            heat = np.zeros((64, 64, 3), np.float32)                             # leeward parts: no entry heating
            T.save(heat, stem + "HeatMask.png", False)
        part.data.materials.clear()
        part.data.materials.append(image_material("M_%s_Parts" % SHIP, stem, float(b.get("emission_strength") or 3.0)))
        for poly in part.data.polygons:
            poly.material_index = 0
    part.parent = body
    glow = None
    if CFG.get("nozzle_glow"):         # the glow cores in the nozzles of the plain Rodin ship (slot M_<Ship>_Glow)
        sys.path.insert(0, os.path.join(rh_ships.KIT, "Rodin", "scripts"))
        import rs_extras
        glow = rs_extras.nozzle_glow(SHIP, body)
    for name, p in SOCKETS_UE.items():
        e = bpy.data.objects.new("SOCKET_" + name, None)
        e.empty_display_size = 0.1
        e.location = (p[0] / 100.0, -p[1] / 100.0, p[2] / 100.0)
        e.parent = body
        bpy.context.scene.collection.objects.link(e)
    bpy.context.view_layer.update()

    # ---------------- numbers
    names = [body.name, part.name] + ([glow.name] if glow else [])
    rep = C.skill("blender-mcp/scripts/asset_report.py")["main"](targets=names, tri_budget=CFG.get("tri_budget", 65000))
    asm = C.skill("blender-mcp/scripts/assembly_check.py")["main"](targets=names, min_island_tris=1)
    res["report"] = {"totals": rep["totals"], "issues": [i for i in rep["issues"] if i["severity"] != "info"][:12]}
    res["assembly"] = {"ok": asm.get("ok"), "islands": asm.get("islands"), "main_body_tris": asm.get("main_body_tris"),
                       "floating_count": len(asm.get("floating", [])),
                       "floating_tris": sum(f.get("tris", 0) for f in asm.get("floating", [])),
                       "floating_top": sorted(asm.get("floating", []), key=lambda f: -f.get("tris", 0))[:6]}
    dg = bpy.context.evaluated_depsgraph_get()
    allv = []
    for n in names:
        ev = bpy.data.objects[n].evaluated_get(dg)
        allv += [ev.matrix_world @ v.co for v in ev.data.vertices]
    res["bbox_cm"] = [[round(min(v[i] for v in allv) * 100, 1) for i in range(3)],
                      [round(max(v[i] for v in allv) * 100, 1) for i in range(3)]]

    # ---------------- sheets: original | hybrid
    if "--no-sheets" not in ARGS:
        src = C.import_rodin("SRC_%sRodin" % SHIP, textured=True)
        cams = ["chase", "chase_3q", "foot_rear_8m", "foot_3q_8m", "rear_close"]
        res["sheets"] = {}
        for mode in ("material", "clay"):
            a = S.render([src.name], C.REVIEW, "tmp_orig_" + TAG, mode, cams)
            b = S.render(names, C.REVIEW, "tmp_hyb_" + TAG, mode, cams)      # ring shown open (no portal disc)
            out = os.path.join(C.REVIEW, "06_hybrid_%s_ORIGINAL_vs_HYBRID_%s.png" % (TAG, mode))
            res["sheets"][mode] = S.grid([[a[c], b[c]] for c in cams], out)
            for p in list(a.values()) + list(b.values()):
                os.remove(p)
        rr = C.skill("blender-mcp/scripts/review_renders.py")
        res["sheets"]["turnaround"] = rr["main"](targets=names, out_dir=C.REVIEW, label="06_hybrid_%s" % TAG,
                                                 mode="material", views=C.VIEWS, resolution=800).get("sheet")
        bpy.data.objects.remove(src)

    blend = os.path.join(C.ROOT, "%s_hybrid_%s.blend" % (SHIP, TAG))
    bpy.ops.wm.save_as_mainfile(filepath=blend, compress=True)
    res["blend"] = blend

    # ---------------- export
    if "--no-export" not in ARGS:
        ex = C.skill("blender-unreal-export/scripts/export_for_unreal.py")
        for root, coll in ((body.name, "auto"),) + (((disc.name, "none"),) if disc else ()) + (((glass.name, "none"),) if glass else ()):
            for stale in (root + ".fbx", root + ".manifest.json"):
                if os.path.isfile(os.path.join(C.EXPORT, stale)):
                    os.remove(os.path.join(C.EXPORT, stale))
            try:
                ex["main"](root=root, out_dir=C.EXPORT, front_axis="+X", collision=coll)
            except AttributeError as error:
                res.setdefault("export_notes", []).append("%s: exporter cleanup: %s" % (root, error))
        man_path = os.path.join(C.EXPORT, SM + ".manifest.json")
        if os.path.isfile(man_path):
            with open(man_path, encoding="utf-8") as fh:
                man = json.load(fh)
            man["ue_sockets"] = [{"name": n, "ue_cm": list(p)} for n, p in SOCKETS_UE.items()]
            if CFG.get("tex"):
                man["max_texture_size"] = CFG["tex"]       # the Null fleet's maps are capped (1K) in Unreal
            man["slots"] = {"M_%s" % SHIP: "kept Rodin hull, corrected set T_%s_BC/N/ORM/E + T_%s_HeatMask" % (SHIP, SHIP),
                            "M_%s_Parts" % SHIP: "new %s, baked set T_%s_Parts_BC/N/ORM/E + T_%s_Parts_HeatMask"
                            % ("ring / engines / tail" if disc else "canopy interior and frame / guns / nose / spine", SHIP, SHIP)}
            if glass:
                man["canopy"] = {"fbx": "SM_%s_Canopy.fbx" % SHIP, "attach_socket": "Cockpit", "slot": "M_%s_Glass" % SHIP,
                                 "note": "tinted canopy glass (translucent, not Nanite); seat, pilot, frame and deck are in SM_%s_Parts" % SHIP}
            if disc:
                man["portal_disc"] = {"fbx": "SM_Rift_PortalDisc.fbx", "attach_socket": "Portal", "slot": "M_Rift_Portal",
                                      "radius_cm": 44.0, "uv": "radial: centre (0.5, 0.5), rim at radius 0.5"}
            with open(man_path, "w", encoding="utf-8") as fh:
                json.dump(man, fh, indent=2)
            res["export"] = {k: man.get(k) for k in ("tris", "size_cm", "materials", "collision", "sockets",
                                                     "errors", "warnings")}
    res["seconds"] = round(time.time() - t0, 1)
    C.out(res)


main()
