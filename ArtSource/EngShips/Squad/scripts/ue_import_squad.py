# SPDX-License-Identifier: MIT
# Triad Drift - import the Squad player ships (ArtSource/EngShips/Squad/SM_Squad_*/) into Unreal and make the
# ship visuals data assets. Run headless from the repo root:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Squad\scripts\ue_import_squad.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Characters/Squad:
#   Materials/M_TD_Squad            lit master: BaseColorTex, ORMTex (AO/rough/metal, Masks), NormalTex,
#                                   EmissiveTex x EmissiveStrength x blink(BlinkRate, BlinkAmount); Nanite usage
#   Pipelines/PL_SQ_StaticMesh      Interchange pipeline (UCX_ collision by name, no lightmap UVs, no materials)
#   Textures/T_Squad_<ship>_BC|N|ORM|E
#   SM_Squad_<ship>, MI_Squad_<ship> one Nanite static mesh + one material instance per ship, sockets checked
#                                   (Muzzle_0..N, Engine, Engine_N, ability points)
#   DA_<Ship>_Visuals               UTDShipVisuals: Mesh + SquadColor (= UTDStatics::ShipColor)
# Gameplay finds DA_<Ship>_Visuals here when /Game/TriadDrift/Presentation has none of that name
# (UTDStatics::LoadPresentationAsset). Presentation always wins.
# Idempotent: re-running replaces meshes, textures and instances and updates the data assets. The master is
# created once; to change its graph delete M_TD_Squad.uasset (and its instances) and run again.

import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Characters/Squad"
TEX_DEST = DEST + "/Textures"
MASTER = DEST + "/Materials/M_TD_Squad"
PIPELINE = DEST + "/Pipelines/PL_SQ_StaticMesh"
# Blender bakes the emissive at strength 2; the placeholder engine glow runs at x8, and a white hull sits next
# to the glow, so the squad runs at 2 x 4 = 8.
EMISSIVE_SCALE = 4.0
VISUALS = {  # data asset -> (hull mesh, squad colour = UTDStatics::ShipColor)
    "DA_Rift_Visuals": ("SM_Squad_Rift", (1.0, 0.17, 0.84)),
    "DA_Lancer_Visuals": ("SM_Squad_Lancer", (0.25, 0.91, 1.0)),
    "DA_Bastion_Visuals": ("SM_Squad_Bastion", (1.0, 0.7, 0.12)),
}

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


def import_textures(paths, notes):
    tasks = []
    for p in paths:
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    if tasks:
        tools.import_asset_tasks(tasks)
    out = {}
    for p in paths:
        name = os.path.splitext(os.path.basename(p))[0]
        tex = eas.load_asset("%s/%s" % (TEX_DEST, name))
        if tex is None:
            notes.append("texture not imported: " + name)
            continue
        suffix = name.rsplit("_", 1)[-1]
        if suffix == "N":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, notes)
            _set(tex, "srgb", False, notes)
            _set(tex, "flip_green_channel", False, notes)      # already DirectX
        elif suffix == "ORM":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, notes)
            _set(tex, "srgb", False, notes)
        else:
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT, notes)
            _set(tex, "srgb", True, notes)
        eas.save_loaded_asset(tex)
        out[suffix] = tex
    return out


def ensure_pipeline(notes):
    ensure_dir(DEST + "/Pipelines")
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


def import_mesh(fbx, notes):
    try:
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(ensure_pipeline(notes))
        mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
        mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(fbx), params)
        return "interchange"
    except Exception as error:
        notes.append("Interchange import failed (%s); legacy FBX importer" % error)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", fbx)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    ui = unreal.FbxImportUI()
    _set(ui, "import_mesh", True, notes)
    _set(ui, "import_as_skeletal", False, notes)
    _set(ui, "import_materials", False, notes)
    _set(ui, "import_textures", False, notes)
    smd = ui.get_editor_property("static_mesh_import_data")
    for k, v in {"combine_meshes": True, "auto_generate_collision": False, "one_convex_hull_per_ucx": True,
                 "generate_lightmap_u_vs": False, "build_nanite": True}.items():
        _set(smd, k, v, notes)
    _set(smd, "normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS, notes)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    return "legacy"


def ensure_master(default_orm, default_e):
    ensure_dir(DEST + "/Materials")
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

    def tex_param(pname, default, sampler, x, y):
        n = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        n.set_editor_property("parameter_name", pname)
        if default is not None:
            n.set_editor_property("texture", default)
        n.set_editor_property("sampler_type", sampler)
        return n

    def scalar(pname, value, x, y):
        n = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("default_value", value)
        return n

    base = tex_param("BaseColorTex", eas.load_asset("/Engine/EngineResources/DefaultTexture"), st.SAMPLERTYPE_COLOR, -900, -400)
    orm = tex_param("ORMTex", default_orm, st.SAMPLERTYPE_MASKS, -900, -100)
    normal = tex_param("NormalTex", eas.load_asset("/Engine/EngineMaterials/DefaultNormal"), st.SAMPLERTYPE_NORMAL, -900, 200)
    emis = tex_param("EmissiveTex", default_e, st.SAMPLERTYPE_COLOR, -900, 500)
    strength = scalar("EmissiveStrength", 0.0, -900, 780)
    rate = scalar("BlinkRate", 0.5, -1300, 900)
    amount = scalar("BlinkAmount", 0.0, -900, 1100)
    time = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1300, 1000)
    t_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1100, 950)
    sine = mel.create_material_expression(mat, unreal.MaterialExpressionSine, -950, 950)
    s_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -800, 950)
    s_mul.set_editor_property("const_b", 3.0)
    clamp = mel.create_material_expression(mat, unreal.MaterialExpressionClamp, -650, 950)
    lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -500, 1000)
    lerp.set_editor_property("const_a", 1.0)
    e_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 650)
    e_mul2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 750)
    mp = unreal.MaterialProperty
    ok = True
    ok &= mel.connect_material_property(base, "RGB", mp.MP_BASE_COLOR)
    ok &= mel.connect_material_property(orm, "R", mp.MP_AMBIENT_OCCLUSION)
    ok &= mel.connect_material_property(orm, "G", mp.MP_ROUGHNESS)
    ok &= mel.connect_material_property(orm, "B", mp.MP_METALLIC)
    ok &= mel.connect_material_property(normal, "RGB", mp.MP_NORMAL)
    ok &= mel.connect_material_expressions(time, "", t_mul, "A")
    ok &= mel.connect_material_expressions(rate, "", t_mul, "B")
    ok &= mel.connect_material_expressions(t_mul, "", sine, "")
    ok &= mel.connect_material_expressions(sine, "", s_mul, "A")
    ok &= mel.connect_material_expressions(s_mul, "", clamp, "")
    ok &= mel.connect_material_expressions(clamp, "", lerp, "B")
    ok &= mel.connect_material_expressions(amount, "", lerp, "Alpha")
    ok &= mel.connect_material_expressions(emis, "RGB", e_mul, "A")
    ok &= mel.connect_material_expressions(strength, "", e_mul, "B")
    ok &= mel.connect_material_expressions(e_mul, "", e_mul2, "A")
    ok &= mel.connect_material_expressions(lerp, "", e_mul2, "B")
    ok &= mel.connect_material_property(e_mul2, "", mp.MP_EMISSIVE_COLOR)
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


def mesh_triangles(mesh, notes):
    try:
        return int(mesh.get_num_triangles(0))
    except Exception:
        pass
    try:
        return int(mesh.get_static_mesh_description(0).get_triangle_count())
    except Exception as error:
        notes.append("triangle count unavailable (%s)" % error)
    return -1


def fix_sockets(mesh, wanted, notes):
    """Make sure every manifest socket exists on the mesh under its plain name (Muzzle_0 ...)."""
    names = []
    for s in wanted:
        name = s["name"]
        sock = mesh.find_socket(name)
        if sock is None:
            pref = mesh.find_socket("SOCKET_" + name)
            if pref is not None:
                pref.set_editor_property("socket_name", name)
                sock = pref
                notes.append("socket renamed SOCKET_%s -> %s" % (name, name))
        if sock is None:
            sock = unreal.StaticMeshSocket(mesh)
            sock.set_editor_property("socket_name", name)
            sock.set_editor_property("relative_location", unreal.Vector(*s["ue_cm"]))
            mesh.add_socket(sock)
            notes.append("socket %s added from the manifest" % name)
        # always (re)write the manifest position: a re-import keeps the previous mesh's sockets
        sock.set_editor_property("relative_location", unreal.Vector(*s["ue_cm"]))
        sock.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
        loc = sock.get_editor_property("relative_location")
        names.append("%s(%.0f,%.0f,%.0f)" % (name, loc.x, loc.y, loc.z))
    return names


def make_visuals(da_name, mesh_assets, notes):
    cls = unreal.load_class(None, "/Script/unrealGameJam.TDShipVisuals")
    if cls is None:
        log("FAILED: class /Script/unrealGameJam.TDShipVisuals not found")
        return None
    path = "%s/%s" % (DEST, da_name)
    da = eas.load_asset(path) if eas.does_asset_exist(path) else None
    if da is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", cls)
        da = tools.create_asset(da_name, DEST, cls, factory)
    if da is None:
        log("FAILED: could not create " + path)
        return None
    for prop, mesh in mesh_assets.items():
        if mesh is not None:
            _set(da, prop, mesh, notes)
    eas.save_loaded_asset(da)
    if eas.does_asset_exist("/Game/TriadDrift/Presentation/" + da_name):
        notes.append("NOTE: /Game/TriadDrift/Presentation/%s exists and wins over this one" % da_name)
    return da


def main():
    manifests = sorted(glob.glob(os.path.join(KIT, "SM_Squad_*", "SM_Squad_*.manifest.json")))
    if not manifests:
        log("FAILED: no manifests under " + KIT)
        return
    ensure_dir(DEST)
    ensure_dir(TEX_DEST)
    first_orm = sorted(glob.glob(os.path.join(KIT, "SM_Squad_*", "T_*_ORM.png")))[:1]
    first_e = sorted(glob.glob(os.path.join(KIT, "SM_Squad_*", "T_*_E.png")))[:1]
    d = import_textures(first_orm + first_e, [])
    master = ensure_master(d.get("ORM"), d.get("E"))
    if master is None:
        return
    rows, meshes, good, bad = [], {}, 0, 0
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for man_path in manifests:
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        asset = man["asset"]
        short = asset[3:]
        folder = os.path.dirname(man_path)
        notes = []
        fbx = os.path.join(folder, asset + ".fbx")
        if not os.path.isfile(fbx):
            log("FAILED %s: no FBX" % asset)
            bad += 1
            continue
        tex = import_textures(sorted(glob.glob(os.path.join(folder, "T_%s_*.png" % short))), notes)
        how = import_mesh(fbx, notes)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
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
        if hulls == 0:
            notes.append("no UCX hulls imported: 4-hull convex decomposition as fallback")
            try:
                sme.set_convex_decomposition_collisions(mesh, 4, 16, 100000)
            except Exception as error:
                notes.append("convex decomposition failed: %s" % error)
            hulls = sme.get_convex_collision_count(mesh)
        sockets = fix_sockets(mesh, man.get("ue_sockets") or [], notes)
        mi_path = "%s/MI_%s" % (DEST, short)
        mi = eas.load_asset(mi_path) if eas.does_asset_exist(mi_path) else tools.create_asset(
            "MI_" + short, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        for pname, suffix in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
            if suffix in tex:
                mel.set_material_instance_texture_parameter_value(mi, pname, tex[suffix])
        strength = (man.get("emission_strength") or 0.0) * EMISSIVE_SCALE if "E" in tex else 0.0
        mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", strength)
        mel.set_material_instance_scalar_parameter_value(mi, "BlinkAmount", 0.0)
        eas.save_loaded_asset(mi)
        for i in range(len(mesh.static_materials)):
            mesh.set_material(i, mi)
        eas.save_loaded_asset(mesh)
        meshes[asset] = mesh
        tris = mesh_triangles(mesh, notes)
        b = mesh.get_bounds()
        size = [round(b.box_extent.x * 2.0, 1), round(b.box_extent.y * 2.0, 1), round(b.box_extent.z * 2.0, 1)]
        nanite = False
        try:
            nanite = bool(mesh.get_editor_property("nanite_settings").enabled)
        except Exception:
            pass
        row = {"asset": asset, "importer": how, "tris": tris, "size_cm": size,
               "bounds_origin_cm": [round(b.origin.x, 1), round(b.origin.y, 1), round(b.origin.z, 1)],
               "sphere_radius_cm": round(b.sphere_radius, 1), "collision_hulls": hulls, "nanite": nanite,
               "material_slots": len(mesh.static_materials), "textures": sorted(tex), "mi": "MI_" + short,
               "emissive_strength": strength, "sockets": sockets, "notes": notes}
        rows.append(row)
        log("%s: tris %s, size %s cm, bounds origin %s, hulls %d, nanite %s, slots %d, textures %s, "
            "EmissiveStrength %.1f, sockets %s%s" % (asset, tris, size, row["bounds_origin_cm"], hulls, nanite,
                                                     row["material_slots"], row["textures"], strength, sockets,
                                                     ("  NOTES: " + "; ".join(notes)) if notes else ""))
        good += 1
    # ship visuals data assets
    da_ok = 0
    for da_name, (hull, color) in VISUALS.items():
        notes = []
        assets = {"mesh": meshes.get(hull) or eas.load_asset("%s/%s" % (DEST, hull)),
                  "squad_color": unreal.LinearColor(color[0], color[1], color[2], 1.0)}
        if assets["mesh"] is None:
            log("FAILED %s: hull mesh %s missing" % (da_name, hull))
            bad += 1
            continue
        da = make_visuals(da_name, assets, notes)
        if da is None:
            bad += 1
            continue
        got = {}
        for prop in assets:
            try:
                got[prop] = str(da.get_editor_property(prop))
            except Exception as error:
                got[prop] = "unreadable (%s)" % error
        log("%s: %s%s" % (da_name, got, ("  NOTES: " + "; ".join(notes)) if notes else ""))
        rows.append({"data_asset": da_name, "properties": got, "notes": notes})
        da_ok += 1
    out = os.path.join(REPO, "Saved", "Squad", "Squad_import_report.json")
    try:
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as fh:
            json.dump(rows, fh, indent=2)
        log("report: " + out)
    except Exception as error:
        log("note: could not write report (%s)" % error)
    log("imported %d mesh(es), %d data asset(s), %d failed" % (good, da_ok, bad))
    if good and da_ok == len(VISUALS) and not bad:
        log("OK")
    else:
        log("FAILED: see the lines above")


main()
