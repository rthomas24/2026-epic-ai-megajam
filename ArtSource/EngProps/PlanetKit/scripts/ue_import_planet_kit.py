# SPDX-License-Identifier: MIT
# Triad Drift - import the PlanetKit set pieces (ArtSource/EngProps/PlanetKit/SM_*/) into Unreal.
# Run headless from the repo root:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\PlanetKit\scripts\ue_import_planet_kit.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Props/Planet (never touches SM_LandingPad / MI_LandingPad /
# T_LandingPad_*, which live there too):
#   Materials/M_TD_PlanetKit       lit master: BaseColorTex, ORMTex (AO/rough/metal, Masks), NormalTex,
#                                  EmissiveTex x EmissiveStrength, plus a tiling DetailNormalTex on texture
#                                  coordinate 1 (UVDetail, 4 m per tile) x DetailTiling, mixed in by DetailStrength
#                                  (0 = off). Usage flags for Nanite and instanced static meshes.
#   Pipelines/PL_PK_StaticMesh     Interchange pipeline (no collision, no lightmap UVs, no materials, Nanite)
#   Textures/T_<short>_BC|N|ORM|E   per-asset baked sets; Textures/T_PK_DetailStone_N the shared detail normal
#   SM_<short>, MI_<short>          one Nanite static mesh + one material instance per asset
# NO COLLISION on the meshes: gameplay owns collision (its own boxes). Simple shapes are removed and the body
# setup is set to "use simple as complex", which with no simple shapes means the mesh never collides.
# Idempotent: re-running replaces meshes, textures and instances. The master is created once; to change its
# graph delete M_TD_PlanetKit.uasset first (editing a master with live instances headless crashes the editor).

import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Props/Planet"
TEX_DEST = DEST + "/Textures"
MASTER = DEST + "/Materials/M_TD_PlanetKit"
PIPELINE = DEST + "/Pipelines/PL_PK_StaticMesh"
DETAIL_PNG = os.path.join(KIT, "textures", "T_PK_DetailStone_N.png")
# Blender bakes emission at strength 2.0; Unreal needs more at the game's exposure (see SpaceDressing / SpaceRocks).
EMISSIVE_SCALE = 4.0
DETAIL_STRENGTH = 0.55
PROTECTED = ("SM_LandingPad", "MI_LandingPad", "T_LandingPad_")

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
        name = os.path.splitext(os.path.basename(p))[0]
        if name.startswith(PROTECTED):
            notes.append("skipped protected " + name)
            continue
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
                 "collision": False, "import_collision_according_to_mesh_name": False,
                 "generate_lightmap_u_vs": False, "build_nanite": True}.items():
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
    for k, v in {"combine_meshes": True, "auto_generate_collision": False, "generate_lightmap_u_vs": False,
                 "build_nanite": True}.items():
        _set(smd, k, v, notes)
    _set(smd, "normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS, notes)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    return "legacy"


def ensure_master(default_orm, default_e, default_detail):
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
    E = unreal

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def tex_param(pname, default, sampler, x, y):
        n = node(E.MaterialExpressionTextureSampleParameter2D, x, y)
        n.set_editor_property("parameter_name", pname)
        if default is not None:
            n.set_editor_property("texture", default)
        n.set_editor_property("sampler_type", sampler)
        return n

    def scalar(pname, value, x, y):
        n = node(E.MaterialExpressionScalarParameter, x, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("default_value", value)
        return n

    base = tex_param("BaseColorTex", eas.load_asset("/Engine/EngineResources/DefaultTexture"), st.SAMPLERTYPE_COLOR, -1100, -500)
    orm = tex_param("ORMTex", default_orm, st.SAMPLERTYPE_MASKS, -1100, -200)
    normal = tex_param("NormalTex", eas.load_asset("/Engine/EngineMaterials/DefaultNormal"), st.SAMPLERTYPE_NORMAL, -1100, 100)
    emis = tex_param("EmissiveTex", default_e, st.SAMPLERTYPE_COLOR, -1100, 700)
    strength = scalar("EmissiveStrength", 0.0, -1100, 980)
    e_mul = node(E.MaterialExpressionMultiply, -700, 800)
    # detail normal on UV1
    uv1 = node(E.MaterialExpressionTextureCoordinate, -1700, 380)
    uv1.set_editor_property("coordinate_index", 1)
    tiling = scalar("DetailTiling", 1.0, -1700, 480)
    uv_mul = node(E.MaterialExpressionMultiply, -1500, 400)
    detail = tex_param("DetailNormalTex", default_detail, st.SAMPLERTYPE_NORMAL, -1300, 400)
    dstr = scalar("DetailStrength", 0.0, -1100, 560)
    mask = node(E.MaterialExpressionComponentMask, -900, 420)
    mask.set_editor_property("r", True)
    mask.set_editor_property("g", True)
    mask.set_editor_property("b", False)
    mask.set_editor_property("a", False)
    d_mul = node(E.MaterialExpressionMultiply, -750, 440)
    zero = node(E.MaterialExpressionConstant, -750, 540)
    zero.set_editor_property("r", 0.0)
    app = node(E.MaterialExpressionAppendVector, -600, 460)
    add = node(E.MaterialExpressionAdd, -450, 200)
    nrm = node(E.MaterialExpressionNormalize, -300, 200)
    mp = unreal.MaterialProperty
    ok = True
    ok &= mel.connect_material_property(base, "RGB", mp.MP_BASE_COLOR)
    ok &= mel.connect_material_property(orm, "R", mp.MP_AMBIENT_OCCLUSION)
    ok &= mel.connect_material_property(orm, "G", mp.MP_ROUGHNESS)
    ok &= mel.connect_material_property(orm, "B", mp.MP_METALLIC)
    ok &= mel.connect_material_expressions(uv1, "", uv_mul, "A")
    ok &= mel.connect_material_expressions(tiling, "", uv_mul, "B")
    ok &= mel.connect_material_expressions(uv_mul, "", detail, "UVs")
    ok &= mel.connect_material_expressions(detail, "RGB", mask, "")
    ok &= mel.connect_material_expressions(mask, "", d_mul, "A")
    ok &= mel.connect_material_expressions(dstr, "", d_mul, "B")
    ok &= mel.connect_material_expressions(d_mul, "", app, "A")
    ok &= mel.connect_material_expressions(zero, "", app, "B")
    ok &= mel.connect_material_expressions(normal, "RGB", add, "A")
    ok &= mel.connect_material_expressions(app, "", add, "B")
    ok &= mel.connect_material_expressions(add, "", nrm, "")
    ok &= mel.connect_material_property(nrm, "", mp.MP_NORMAL)
    ok &= mel.connect_material_expressions(emis, "RGB", e_mul, "A")
    ok &= mel.connect_material_expressions(strength, "", e_mul, "B")
    ok &= mel.connect_material_property(e_mul, "", mp.MP_EMISSIVE_COLOR)
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


def strip_collision(mesh, notes):
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    try:
        sme.remove_collisions(mesh)
    except Exception as error:
        notes.append("remove_collisions: %s" % error)
    try:
        bs = mesh.get_editor_property("body_setup")
        if bs is not None:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    except Exception as error:
        notes.append("collision trace flag: %s" % error)
    try:
        return int(sme.get_simple_collision_count(mesh))
    except Exception:
        try:
            return int(sme.get_convex_collision_count(mesh))
        except Exception:
            return -1


def mesh_triangles(mesh, notes):
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
    # assets with their own importer (SM_Cind_MagmaSurface: ue_import_magma_surface.py, master M_TD_Magma, no Nanite)
    # are never re-imported here, so a full kit rebuild cannot re-parent them onto M_TD_PlanetKit
    def _custom(path):
        with open(path, encoding="utf-8") as fh:
            return bool(json.load(fh).get("custom_importer"))
    manifests = [m for m in manifests if not _custom(m)]
    # optional subset: PK_ONLY=SM_A,SM_B (env var, or a pk_only.txt next to this script) re-imports only those
    only = os.environ.get("PK_ONLY", "")
    only_file = os.path.join(HERE, "pk_only.txt")
    if not only and os.path.isfile(only_file):
        with open(only_file, encoding="utf-8") as fh:
            only = fh.read()
    only = [n.strip() for n in only.replace("\n", ",").split(",") if n.strip()]
    if only:
        manifests = [m for m in manifests if os.path.basename(m).split(".")[0] in only]
        log("subset: " + ", ".join(only))
    if not manifests:
        log("FAILED: no manifests under " + KIT)
        return
    ensure_dir(DEST)
    ensure_dir(TEX_DEST)
    folders = [os.path.dirname(m) for m in manifests]
    first_orm = sorted(t for f in folders for t in glob.glob(os.path.join(f, "T_*_ORM.png")))[:1]
    first_e = sorted(t for f in folders for t in glob.glob(os.path.join(f, "T_*_E.png")))[:1]
    detail_path = "%s/T_PK_DetailStone_N" % TEX_DEST
    keep_detail = bool(only) and eas.does_asset_exist(detail_path)     # subset run: don't re-save the shared map
    d = import_textures(first_orm + first_e + ([DETAIL_PNG] if os.path.isfile(DETAIL_PNG) and not keep_detail else []), [])
    if keep_detail:
        d["N"] = eas.load_asset(detail_path)
    master = ensure_master(d.get("ORM"), d.get("E"), d.get("N"))
    if master is None:
        return
    detail_tex = d.get("N")
    rows, good, bad = [], 0, 0
    for man_path in manifests:
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        asset = man["asset"]
        short = asset[3:]
        if asset.startswith(PROTECTED):
            log("skipped protected " + asset)
            continue
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
        simple = strip_collision(mesh, notes)
        mi_path = "%s/MI_%s" % (DEST, short)
        mi = eas.load_asset(mi_path) if eas.does_asset_exist(mi_path) else tools.create_asset(
            "MI_" + short, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        for pname, suffix in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
            if suffix in tex:
                mel.set_material_instance_texture_parameter_value(mi, pname, tex[suffix])
        strength = (man.get("emission_strength") or 0.0) * EMISSIVE_SCALE if "E" in tex else 0.0
        mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", strength)
        dstrength = DETAIL_STRENGTH if (man.get("detail_uv") and detail_tex is not None) else 0.0
        mel.set_material_instance_scalar_parameter_value(mi, "DetailStrength", dstrength)
        mel.set_material_instance_scalar_parameter_value(mi, "DetailTiling", 1.0)
        eas.save_loaded_asset(mi)
        for i in range(len(mesh.static_materials)):
            mesh.set_material(i, mi)
        eas.save_loaded_asset(mesh)
        tris, tri_src = mesh_triangles(mesh, notes)
        b = mesh.get_bounds()
        bb = mesh.get_bounding_box()
        size = [round(bb.max.x - bb.min.x, 1), round(bb.max.y - bb.min.y, 1), round(bb.max.z - bb.min.z, 1)]
        nanite = False
        try:
            nanite = bool(mesh.get_editor_property("nanite_settings").enabled)
        except Exception:
            pass
        uv_channels = -1
        try:
            uv_channels = int(sme.get_num_uv_channels(mesh, 0))
        except Exception:
            pass
        row = {"asset": asset, "importer": how, "tris": tris, "tri_source": tri_src, "size_cm": size,
               "min_cm": [round(bb.min.x, 1), round(bb.min.y, 1), round(bb.min.z, 1)],
               "max_cm": [round(bb.max.x, 1), round(bb.max.y, 1), round(bb.max.z, 1)],
               "simple_collision_shapes": simple, "nanite": nanite, "uv_channels": uv_channels,
               "material_slots": len(mesh.static_materials), "textures": sorted(tex), "mi": "MI_" + short,
               "emissive_strength": strength, "detail_strength": dstrength, "notes": notes}
        rows.append(row)
        log("%s: tris %s (%s), size %s cm, min %s, simple collision %s, nanite %s, uv %s, slots %d, textures %s, "
            "EmissiveStrength %.1f, Detail %.2f%s" % (
                asset, tris, tri_src, size, row["min_cm"], simple, nanite, uv_channels, row["material_slots"],
                row["textures"], strength, dstrength, ("  NOTES: " + "; ".join(notes)) if notes else ""))
        good += 1
    out = os.path.join(REPO, "Saved", "PlanetKit_import_report.json")
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
