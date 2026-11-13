# SPDX-License-Identifier: MIT
# Triad Drift - import the SpaceDressing kit (ArtSource/EngProps/SpaceDressing/SM_*/) into Unreal.
# Run headless from the repo root:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\SpaceDressing\scripts\ue_import_space_dressing.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Props/SpaceDressing:
#   Materials/M_TD_SpaceDressing   lit master: BaseColorTex, ORMTex (AO/rough/metal, Masks), NormalTex,
#                                  EmissiveTex x EmissiveStrength x blink(BlinkRate Hz, BlinkAmount 0..1);
#                                  usage flags for instanced static meshes and Nanite
#   Pipelines/PL_SD_StaticMesh     Interchange pipeline (UCX_ collision by name, no lightmap UVs, no materials)
#   Textures/T_<short>_BC|N|ORM|E   textures with the right compression / sRGB
#   SM_<short>, MI_<short>          one Nanite static mesh + one material instance per asset
# Same approach as Tools/td-import-props.py (which is not modified): a texture parameter's DEFAULT texture
# must match its sampler type, so the ORM and E defaults are textures of this kit, imported first.
# Idempotent: re-running replaces meshes, textures and instances. The master is created once; to change its
# graph, delete M_TD_SpaceDressing.uasset and run again (editing a master with live instances headless crashed
# the editor before; see td-import-props.py).

import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Props/SpaceDressing"
TEX_DEST = DEST + "/Textures"
MASTER = DEST + "/Materials/M_TD_SpaceDressing"
PIPELINE = DEST + "/Pipelines/PL_SD_StaticMesh"
# The Blender bake strength (2.0) is far too weak at the game's exposure (the SpaceRocks kit glowed dark until it
# was multiplied); EmissiveStrength = manifest emission_strength x this.
EMISSIVE_SCALE = 5.0
# (BlinkRate Hz, BlinkAmount): the whole emissive of these meshes is a beacon light, so it may blink.
BLINK = {"SM_Derelict_Satellite_01": (0.7, 1.0), "SM_Derelict_Satellite_02": (0.55, 1.0),
         "SM_BeaconBuoy_01": (0.4, 0.6)}

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
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False,
                 "collision": True, "import_collision_according_to_mesh_name": True,
                 "one_convex_hull_per_ucx": True, "generate_lightmap_u_vs": False, "build_nanite": True}.items():
        _set(mesh, k, v, notes)
    for sub, props in (("material_pipeline", {"import_materials": False}),):
        try:
            sp = pipe.get_editor_property(sub)
            for k, v in props.items():
                _set(sp, k, v, notes)
            try:
                tp = sp.get_editor_property("texture_pipeline")
                _set(tp, "import_textures", False, notes)
            except Exception:
                pass
        except Exception as error:
            notes.append("pipeline %s: %s" % (sub, error))
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
    for flag in ("used_with_instanced_static_meshes", "used_with_nanite"):
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
    """Source triangles (mesh description) and, for reference, the Nanite fallback mesh's LOD0 count."""
    fallback = -1
    try:
        fallback = int(mesh.get_num_triangles(0))
    except Exception:
        pass
    try:
        return int(mesh.get_static_mesh_description(0).get_triangle_count()), "source; Nanite fallback %d" % fallback
    except Exception as error:
        notes.append("source triangle count unavailable (%s)" % error)
    return fallback, "Nanite fallback LOD0"


def main():
    manifests = sorted(glob.glob(os.path.join(KIT, "SM_*", "SM_*.manifest.json")))
    if not manifests:
        log("FAILED: no manifests under " + KIT)
        return
    ensure_dir(DEST)
    ensure_dir(TEX_DEST)
    # defaults for the master's texture parameters: one ORM (Masks) and one E (Color) texture of this kit
    first_orm = sorted(glob.glob(os.path.join(KIT, "SM_*", "T_*_ORM.png")))[:1]
    first_e = sorted(glob.glob(os.path.join(KIT, "SM_*", "T_*_E.png")))[:1]
    d = import_textures(first_orm + first_e, [])
    master = ensure_master(d.get("ORM"), d.get("E"))
    if master is None:
        return
    rows, good, bad = [], 0, 0
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
        sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
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
            notes.append("no UCX hulls imported: added a 4-hull convex decomposition as fallback")
            try:
                sme.set_convex_decomposition_collisions(mesh, 4, 16, 100000)
            except Exception as error:
                notes.append("convex decomposition failed: %s" % error)
            hulls = sme.get_convex_collision_count(mesh)
        # material instance
        mi_path = "%s/MI_%s" % (DEST, short)
        mi = eas.load_asset(mi_path) if eas.does_asset_exist(mi_path) else tools.create_asset(
            "MI_" + short, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        for pname, suffix in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
            if suffix in tex:
                mel.set_material_instance_texture_parameter_value(mi, pname, tex[suffix])
        strength = (man.get("emission_strength") or 0.0) * EMISSIVE_SCALE if "E" in tex else 0.0
        mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", strength)
        rate, amount = BLINK.get(asset, (0.5, 0.0))
        mel.set_material_instance_scalar_parameter_value(mi, "BlinkRate", rate)
        mel.set_material_instance_scalar_parameter_value(mi, "BlinkAmount", amount)
        eas.save_loaded_asset(mi)
        for i in range(len(mesh.static_materials)):
            mesh.set_material(i, mi)
        eas.save_loaded_asset(mesh)
        tris, tri_src = mesh_triangles(mesh, notes)
        b = mesh.get_bounds()
        size = [round(b.box_extent.x * 2.0, 1), round(b.box_extent.y * 2.0, 1), round(b.box_extent.z * 2.0, 1)]
        nanite = False
        try:
            nanite = bool(mesh.get_editor_property("nanite_settings").enabled)
        except Exception:
            pass
        row = {"asset": asset, "importer": how, "tris": tris, "tri_source": tri_src, "size_cm": size,
               "bounds_origin_cm": [round(b.origin.x, 1), round(b.origin.y, 1), round(b.origin.z, 1)],
               "sphere_radius_cm": round(b.sphere_radius, 1), "collision_hulls": hulls, "nanite": nanite,
               "material_slots": len(mesh.static_materials), "textures": sorted(tex), "mi": "MI_" + short,
               "emissive_strength": strength, "blink": [rate, amount], "notes": notes}
        rows.append(row)
        log("%s: tris %s (%s), size %s cm, origin %s, hulls %d, nanite %s, slots %d, textures %s, EmissiveStrength %.1f%s" % (
            asset, tris, tri_src, size, row["bounds_origin_cm"], hulls, nanite, row["material_slots"], row["textures"],
            strength, ("  NOTES: " + "; ".join(notes)) if notes else ""))
        good += 1
    out = os.path.join(REPO, "Saved", "SpaceDressing_import_report.json")
    try:
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as fh:
            json.dump(rows, fh, indent=2)
        log("report: " + out)
    except Exception as error:
        log("note: could not write report (%s)" % error)
    log("imported %d asset(s), %d failed" % (good, bad))
    if good and not bad:
        log("OK")
    else:
        log("FAILED: see the lines above")


main()
