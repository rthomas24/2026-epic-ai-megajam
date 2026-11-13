# SPDX-License-Identifier: MIT
# Triad Drift - import the Rodin ships (ArtSource/EngShips/<Ship>/SM_<Ship>.fbx, made by rs_prep.py from Grok
# Bot's Hyper3D Rodin meshes) into Unreal and point the engineering fallback visuals at them. Headless:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Rodin\scripts\ue_import_rodin_ships.py
#
# Writes under /Game/TriadDrift/Gameplay/Ships:
#   Materials/M_TD_Ship           lit master: BaseColorTex x BaseTint, ORMTex (AO / rough x RoughnessScale / metal),
#                                 NormalTex, EmissiveTex x EmissiveTint x EmissiveStrength
#                                 + HeatMaskTex x HeatColor x HeatStrength x HeatGlow (HeatGlow 0..1, default 0:
#                                 descent / afterburner code can drive it on a dynamic instance later)
#                                 + BaseColor x SelfLight (a little self-light so the squad colours still read on
#                                 the unlit side: the chase camera mostly sees the side facing away from the sun)
#   Pipelines/PL_Ship_StaticMesh  Interchange pipeline (UCX_ collision by name, no lightmap UVs, no materials)
#   <Ship>/SM_<Ship>, MI_<Ship>, T_<Ship>_BC|N|ORM|E[|HeatMask]     Nanite static mesh, one material slot
# and edits ONLY the Mesh / MaterialOverrides fields of the existing engineering fallbacks
#   /Game/TriadDrift/Gameplay/Characters/Squad/DA_<Ship>_Visuals
#   /Game/TriadDrift/Gameplay/Characters/NullFleet/DA_Enemy_<Type>_Visuals
# (/Game/TriadDrift/Presentation is never touched and still wins). The old meshes (SM_Squad_*, SM_Null_*) stay
# where they are; to switch a ship back run this script with TD_RODIN_REVERT=<Ship>[,<Ship>...] in the
# environment (add OldFleet to also drop the material override from the interceptor, bomber and carrier
# visuals), or re-run the old ue_import_squad.py / ue_import_null_fleet.py.
# TD_RODIN_MI_ONLY=1 keeps meshes and textures and only rewrites the material instances and visuals (tuning).
# Idempotent. The master is created once; to change its graph delete M_TD_Ship.uasset and run again.

import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(HERE))))
KIT = os.path.join(REPO, "ArtSource", "EngShips")
ROOT = "/Game/TriadDrift/Gameplay/Ships"
MASTER = ROOT + "/Materials/M_TD_Ship"
PIPELINE = ROOT + "/Pipelines/PL_Ship_StaticMesh"
SQUAD = "/Game/TriadDrift/Gameplay/Characters/Squad"
FLEET = "/Game/TriadDrift/Gameplay/Characters/NullFleet"

# ship -> data asset folder, data asset, visuals class, the old mesh (for revert), material instance values
SHIPS = {
    "Rift": dict(da=(SQUAD, "DA_Rift_Visuals", "TDShipVisuals"), old=SQUAD + "/SM_Squad_Rift",
                 emissive=6.0, base_tint=(1.0, 1.0, 1.0), rough=1.0, self_light=0.12),
    "Lancer": dict(da=(SQUAD, "DA_Lancer_Visuals", "TDShipVisuals"), old=SQUAD + "/SM_Squad_Lancer",
                   emissive=6.0, base_tint=(1.0, 1.0, 1.0), rough=1.0, self_light=0.12),
    "Bastion": dict(da=(SQUAD, "DA_Bastion_Visuals", "TDShipVisuals"), old=SQUAD + "/SM_Squad_Bastion",
                    emissive=6.0, base_tint=(1.5, 1.5, 1.5), rough=1.0, self_light=0.18),
    # Null Armada: the Rodin albedo came out light silver; the brief is near-black armour, red-orange glow,
    # white eyes. BaseTint multiplies the albedo down; the emissive map keeps its own colours.
    "NullFighter": dict(da=(FLEET, "DA_Enemy_Fighter_Visuals", "TDEnemyVisuals"), old=FLEET + "/SM_Null_Fighter",
                        emissive=10.0, base_tint=(0.35, 0.33, 0.34), rough=1.0,
                        emissive_tint=(1.0, 0.55, 0.35)),
    "NullGunship": dict(da=(FLEET, "DA_Enemy_Gunship_Visuals", "TDEnemyVisuals"), old=FLEET + "/SM_Null_Gunship",
                        emissive=8.0, base_tint=(0.6, 0.57, 0.57), rough=1.0,
                        emissive_tint=(1.0, 0.55, 0.35)),
}
# The other Null Armada ships keep their engineering meshes (the Rodin interceptor arrived without textures; the
# bomber and carrier have no Rodin mesh yet). Their visuals get the same material-override fix, so the whole
# fleet draws its real materials instead of the placeholder's flat glow.
OLD_FLEET = {
    "DA_Enemy_Interceptor_Visuals": "MI_Null_Interceptor",
    "DA_Enemy_Bomber_Visuals": "MI_Null_Bomber",
    "DA_Enemy_Carrier_Visuals": "MI_Null_Carrier",
}
SUFFIX_PARAM = (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E"),
                ("HeatMaskTex", "HeatMask"))

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("TDPY: " + msg)


def _set(obj, prop, value, notes):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as error:
        notes.append("could not set %s.%s (%s)" % (type(obj).__name__, prop, error))
        return False


def ensure_dir(path):
    if not eas.does_directory_exist(path):
        eas.make_directory(path)


def import_textures(files, dest, max_size, notes):
    """files: {suffix: absolute png path}. Returns {suffix: texture}."""
    tasks = []
    for suffix, p in files.items():
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", dest)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    if tasks:
        tools.import_asset_tasks(tasks)
    out = {}
    tcs = unreal.TextureCompressionSettings
    for suffix, p in files.items():
        name = os.path.splitext(os.path.basename(p))[0]
        tex = eas.load_asset("%s/%s" % (dest, name))
        if tex is None:
            notes.append("texture not imported: " + name)
            continue
        if suffix == "N":
            _set(tex, "compression_settings", tcs.TC_NORMALMAP, notes)
            _set(tex, "srgb", False, notes)
            _set(tex, "flip_green_channel", False, notes)      # Grok Bot's T_*_N are already DirectX
        elif suffix == "ORM":
            _set(tex, "compression_settings", tcs.TC_MASKS, notes)
            _set(tex, "srgb", False, notes)
        elif suffix == "HeatMask":
            _set(tex, "compression_settings", tcs.TC_GRAYSCALE, notes)
            _set(tex, "srgb", False, notes)
        else:
            _set(tex, "compression_settings", tcs.TC_DEFAULT, notes)
            _set(tex, "srgb", True, notes)
        _set(tex, "max_texture_size", int(max_size), notes)
        eas.save_loaded_asset(tex)
        out[suffix] = tex
    return out


def ensure_pipeline(notes):
    ensure_dir(ROOT + "/Pipelines")
    if not eas.does_asset_exist(PIPELINE):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", PIPELINE)
    pipe = eas.load_asset(PIPELINE)
    _set(pipe, "asset_type_sub_folders", False, notes)
    mesh = pipe.get_editor_property("mesh_pipeline")
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False, "combine_static_meshes": True,
                 "collision": True, "import_collision_according_to_mesh_name": True,
                 "one_convex_hull_per_ucx": True, "generate_lightmap_u_vs": False, "build_nanite": True}.items():
        _set(mesh, k, v, notes)
    try:
        sp = pipe.get_editor_property("material_pipeline")
        _set(sp, "import_materials", False, notes)
        try:
            _set(sp.get_editor_property("texture_pipeline"), "import_textures", False, notes)
        except Exception:
            pass
    except Exception as error:
        notes.append("pipeline material_pipeline: %s" % error)
    eas.save_loaded_asset(pipe)
    return unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1]))


def import_mesh(fbx, dest, notes):
    try:
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(ensure_pipeline(notes))
        mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
        mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(fbx), params)
        return "interchange"
    except Exception as error:
        notes.append("Interchange import failed (%s)" % error)
        return "failed"


def ensure_master(defaults):
    ensure_dir(ROOT + "/Materials")
    if eas.does_asset_exist(MASTER):
        return eas.load_asset(MASTER)
    folder, name = MASTER.rsplit("/", 1)
    mat = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    for flag in ("used_with_nanite", "used_with_instanced_static_meshes"):
        try:
            mat.set_editor_property(flag, True)
        except Exception as error:
            log("note: could not set %s (%s)" % (flag, error))
    st = unreal.MaterialSamplerType
    E = unreal

    def tex_param(pname, default, sampler, x, y):
        n = mel.create_material_expression(mat, E.MaterialExpressionTextureSampleParameter2D, x, y)
        n.set_editor_property("parameter_name", pname)
        # a parameter's default texture must match its sampler type or the master fails to compile
        n.set_editor_property("texture", default)
        n.set_editor_property("sampler_type", sampler)
        return n

    def scalar(pname, value, x, y):
        n = mel.create_material_expression(mat, E.MaterialExpressionScalarParameter, x, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("default_value", value)
        return n

    def vector(pname, rgb, x, y):
        n = mel.create_material_expression(mat, E.MaterialExpressionVectorParameter, x, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        return n

    def mul(a, a_out, b, b_out, x, y):
        n = mel.create_material_expression(mat, E.MaterialExpressionMultiply, x, y)
        ok1 = mel.connect_material_expressions(a, a_out, n, "A")
        ok2 = mel.connect_material_expressions(b, b_out, n, "B")
        return n, ok1 and ok2

    base = tex_param("BaseColorTex", defaults["BC"], st.SAMPLERTYPE_COLOR, -1200, -500)
    tint = vector("BaseTint", (1.0, 1.0, 1.0), -1200, -250)
    orm = tex_param("ORMTex", defaults["ORM"], st.SAMPLERTYPE_MASKS, -1200, -50)
    rough = scalar("RoughnessScale", 1.0, -1200, 200)
    normal = tex_param("NormalTex", eas.load_asset("/Engine/EngineMaterials/DefaultNormal"), st.SAMPLERTYPE_NORMAL, -1200, 320)
    emis = tex_param("EmissiveTex", defaults["E"], st.SAMPLERTYPE_COLOR, -1200, 600)
    etint = vector("EmissiveTint", (1.0, 1.0, 1.0), -1200, 850)
    strength = scalar("EmissiveStrength", 0.0, -1200, 1050)
    heat = tex_param("HeatMaskTex", defaults["HeatMask"], st.SAMPLERTYPE_LINEAR_GRAYSCALE, -1200, 1200)
    hcol = vector("HeatColor", (1.0, 0.32, 0.05), -1200, 1450)
    hstr = scalar("HeatStrength", 20.0, -1200, 1650)
    hglow = scalar("HeatGlow", 0.0, -1200, 1750)
    selfl = scalar("SelfLight", 0.0, -1200, 1900)

    ok = True
    bc, k = mul(base, "RGB", tint, "RGB", -800, -400); ok &= k
    rg, k = mul(orm, "G", rough, "", -800, 100); ok &= k
    e1, k = mul(emis, "RGB", etint, "RGB", -800, 700); ok &= k
    e2, k = mul(e1, "", strength, "", -600, 800); ok &= k
    h1, k = mul(heat, "R", hcol, "RGB", -800, 1300); ok &= k
    h2, k = mul(hstr, "", hglow, "", -800, 1700); ok &= k
    h3, k = mul(h1, "", h2, "", -600, 1400); ok &= k
    add0 = mel.create_material_expression(mat, E.MaterialExpressionAdd, -400, 1000)
    ok &= mel.connect_material_expressions(e2, "", add0, "A")
    ok &= mel.connect_material_expressions(h3, "", add0, "B")
    sl, k = mul(bc, "", selfl, "", -600, 1900); ok &= k
    add = mel.create_material_expression(mat, E.MaterialExpressionAdd, -200, 1100)
    ok &= mel.connect_material_expressions(add0, "", add, "A")
    ok &= mel.connect_material_expressions(sl, "", add, "B")
    mp = unreal.MaterialProperty
    ok &= mel.connect_material_property(bc, "", mp.MP_BASE_COLOR)
    ok &= mel.connect_material_property(orm, "R", mp.MP_AMBIENT_OCCLUSION)
    ok &= mel.connect_material_property(rg, "", mp.MP_ROUGHNESS)
    ok &= mel.connect_material_property(orm, "B", mp.MP_METALLIC)
    ok &= mel.connect_material_property(normal, "RGB", mp.MP_NORMAL)
    ok &= mel.connect_material_property(add, "", mp.MP_EMISSIVE_COLOR)
    if not ok:
        log("FAILED: could not wire " + MASTER)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        log("note: no renderer in this headless run; the master's shaders compile on first load in an editor")
    eas.save_asset(MASTER, only_if_is_dirty=False)
    log("built " + MASTER)
    return mat


def fix_sockets(mesh, wanted, notes):
    names = []
    for s in wanted:
        name = s["name"]
        sock = mesh.find_socket(name)
        if sock is None:
            pref = mesh.find_socket("SOCKET_" + name)
            if pref is not None:
                pref.set_editor_property("socket_name", name)
                sock = pref
        if sock is None:
            sock = unreal.StaticMeshSocket(mesh)
            sock.set_editor_property("socket_name", name)
            mesh.add_socket(sock)
            notes.append("socket %s added from the manifest" % name)
        sock.set_editor_property("relative_location", unreal.Vector(*s["ue_cm"]))
        sock.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
        loc = sock.get_editor_property("relative_location")
        names.append("%s(%.0f,%.0f,%.0f)" % (name, loc.x, loc.y, loc.z))
    return names


def set_visuals_mesh(cfg, mesh, notes, mi=None):
    folder, da_name, cls_name = cfg["da"]
    path = "%s/%s" % (folder, da_name)
    if not eas.does_asset_exist(path):
        notes.append("data asset missing: " + path)
        return None
    da = eas.load_asset(path)
    _set(da, "mesh", mesh, notes)
    # Gameplay tints the placeholder hull with a dynamic glow material in slot 0 before the art arrives, and
    # SetStaticMesh keeps component overrides: the visuals name the MI so the real mesh never draws flat glow.
    _set(da, "material_overrides", [mi] if mi else [], notes)
    eas.save_loaded_asset(da)
    if eas.does_asset_exist("/Game/TriadDrift/Presentation/" + da_name):
        notes.append("NOTE: /Game/TriadDrift/Presentation/%s exists and wins field by field" % da_name)
    return str(da.get_editor_property("mesh"))


def main():
    revert = [s.strip() for s in os.environ.get("TD_RODIN_REVERT", "").split(",") if s.strip()]
    only = [s.strip() for s in os.environ.get("TD_RODIN_ONLY", "").split(",") if s.strip()]
    mi_only = bool(os.environ.get("TD_RODIN_MI_ONLY"))      # retune material instances and visuals only
    ensure_dir(ROOT)
    rows, good, bad = [], 0, 0
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    master = None
    order = [s for s in SHIPS if not only or s in only]
    for ship in order:
        cfg = SHIPS[ship]
        notes = []
        if ship in revert:
            old = eas.load_asset(cfg["old"])
            got = set_visuals_mesh(cfg, old, notes) if old else None
            log("%s: REVERTED %s -> %s %s" % (ship, cfg["da"][1], got, "; ".join(notes)))
            good += 1 if got else 0
            bad += 0 if got else 1
            continue
        asset = "SM_" + ship
        folder = os.path.join(KIT, ship)
        man_path = os.path.join(folder, asset + ".manifest.json")
        fbx = os.path.join(folder, asset + ".fbx")
        if not (os.path.isfile(man_path) and os.path.isfile(fbx)):
            log("FAILED %s: no FBX / manifest in %s (run rs_prep.py)" % (ship, folder))
            bad += 1
            continue
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        dest = "%s/%s" % (ROOT, ship)
        ensure_dir(dest)
        tex_dir = os.path.join(REPO, man["texture_dir"])
        files = {}
        for suffix in ("BC", "N", "ORM", "E"):
            p = os.path.join(tex_dir, "T_%s_%s.png" % (ship, suffix))
            if os.path.isfile(p):
                files[suffix] = p
        if man.get("heat_mask"):
            files["HeatMask"] = os.path.join(REPO, man["heat_mask"])
        if mi_only:
            tex = {}
            for suffix, p in files.items():
                t = eas.load_asset("%s/%s" % (dest, os.path.splitext(os.path.basename(p))[0]))
                if t is not None:
                    tex[suffix] = t
        else:
            tex = import_textures(files, dest, man.get("max_texture_size", 2048), notes)
        if "HeatMask" in tex:       # a smooth mask: 1K is plenty and saves about 2 MB cooked per ship
            _set(tex["HeatMask"], "max_texture_size", 1024, notes)
            eas.save_loaded_asset(tex["HeatMask"])
        if master is None:
            if not eas.does_asset_exist(MASTER) and not all(k in tex for k in ("BC", "ORM", "E", "HeatMask")):
                log("FAILED: the master needs a ship with all five maps first (Rift)")
                return
            master = ensure_master(tex)
            if master is None:
                return
        how = "kept (TD_RODIN_MI_ONLY)" if mi_only else import_mesh(fbx, dest, notes)
        mesh = eas.load_asset("%s/%s" % (dest, asset))
        if mesh is None:
            log("FAILED %s: mesh not found after import (%s) %s" % (asset, how, "; ".join(notes)))
            bad += 1
            continue
        try:
            sme.set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            notes.append("lightmap toggle: %s" % error)
        try:
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = True
            mesh.set_editor_property("nanite_settings", ns)
        except Exception as error:
            notes.append("nanite: %s" % error)
        hulls = sme.get_convex_collision_count(mesh)
        sockets = fix_sockets(mesh, man.get("ue_sockets") or [], notes)
        mi_path = "%s/MI_%s" % (dest, ship)
        mi = eas.load_asset(mi_path) if eas.does_asset_exist(mi_path) else tools.create_asset(
            "MI_" + ship, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        for pname, suffix in SUFFIX_PARAM:
            if suffix in tex:
                mel.set_material_instance_texture_parameter_value(mi, pname, tex[suffix])
        mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", cfg["emissive"] if "E" in tex else 0.0)
        mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", cfg["rough"])
        mel.set_material_instance_scalar_parameter_value(mi, "HeatGlow", 0.0)
        mel.set_material_instance_scalar_parameter_value(mi, "SelfLight", cfg.get("self_light", 0.0))
        et = cfg.get("emissive_tint", (1.0, 1.0, 1.0))
        mel.set_material_instance_vector_parameter_value(mi, "EmissiveTint", unreal.LinearColor(et[0], et[1], et[2], 1.0))
        bt = cfg["base_tint"]
        mel.set_material_instance_vector_parameter_value(mi, "BaseTint", unreal.LinearColor(bt[0], bt[1], bt[2], 1.0))
        eas.save_loaded_asset(mi)
        for i in range(len(mesh.static_materials)):
            mesh.set_material(i, mi)
        eas.save_loaded_asset(mesh)
        tris = -1
        try:
            tris = int(mesh.get_num_triangles(0))
        except Exception:
            pass
        b = mesh.get_bounds()
        size = [round(b.box_extent.x * 2.0, 1), round(b.box_extent.y * 2.0, 1), round(b.box_extent.z * 2.0, 1)]
        got = set_visuals_mesh(cfg, mesh, notes, mi)
        row = {"asset": asset, "importer": how, "tris": tris, "size_cm": size,
               "bounds_origin_cm": [round(b.origin.x, 1), round(b.origin.y, 1), round(b.origin.z, 1)],
               "collision_hulls": hulls, "material_slots": len(mesh.static_materials), "textures": sorted(tex),
               "sockets": sockets, "visuals": cfg["da"][1], "visuals_mesh": got, "notes": notes}
        rows.append(row)
        log("%s: tris %s, size %s cm, bounds origin %s, hulls %d, slots %d, textures %s, sockets %s, %s.Mesh = %s%s"
            % (asset, tris, size, row["bounds_origin_cm"], hulls, row["material_slots"], row["textures"], sockets,
               cfg["da"][1], got, ("  NOTES: " + "; ".join(notes)) if notes else ""))
        if got:
            good += 1
        else:
            bad += 1
    for da_name, mi_name in OLD_FLEET.items():
        path = "%s/%s" % (FLEET, da_name)
        mi = eas.load_asset("%s/%s" % (FLEET, mi_name))
        if not eas.does_asset_exist(path) or mi is None:
            log("note: %s or %s missing, left alone" % (da_name, mi_name))
            continue
        da = eas.load_asset(path)
        notes = []
        _set(da, "material_overrides", [] if "OldFleet" in revert else [mi], notes)
        eas.save_loaded_asset(da)
        log("%s.MaterialOverrides = %s %s" % (da_name, "[] (reverted)" if "OldFleet" in revert else mi_name, "; ".join(notes)))
    out = os.path.join(REPO, "Saved", "RodinShips", "import_report.json")
    try:
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as fh:
            json.dump(rows, fh, indent=2)
    except Exception as error:
        log("note: could not write report (%s)" % error)
    log("done: %d ok, %d failed" % (good, bad))
    log("OK" if good and not bad else "FAILED: see the lines above")


main()
