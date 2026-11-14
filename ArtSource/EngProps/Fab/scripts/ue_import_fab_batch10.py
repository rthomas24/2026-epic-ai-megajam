# SPDX-License-Identifier: MIT
# Triad Drift - Quixel priority batch (#207, Fab Standard), headless (Tools\td-python.ps1). Per group: shared textures
# (colour 1024, normal 512, ORM / masks 512), one instance (two for foliage: teal, violet) and the meshes from the
# group's export folder. Scans on M_TD_PropTextured with Nanite; foliage on M_TD_FabFoliageCard without Nanite or
# collision; the leak decal on M_TD_FabDecal. Nothing is placed.
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
FAB = os.path.dirname(HERE)
ROOT = "/Game/TriadDrift/Gameplay/Props/Fab"
PIPELINE = "/Game/TriadDrift/Gameplay/Ships/Pipelines/PL_Ship_StaticMesh"
PROP = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
CARD = ROOT + "/VerdancePlants/M_TD_FabFoliageCard"
DECAL = ROOT + "/ScorchMark/M_TD_FabDecal"
GROUPS = [  # (source group, Unreal folder, kind, collision kept)
    ("CinderCliff", "CinderCliff", "scan", True),
    ("CinderRockScatter", "CinderRockScatter", "scan", False),
    ("VerdanceLog", "VerdanceLog", "scan", True),
    ("VerdanceFerns", "VerdanceFerns", "foliage", False),
    ("VerdanceLeatherFern", "VerdanceFerns", "foliage", False),
    ("VerdanceClover", "VerdanceClover", "foliage", False),
]
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
TCS = unreal.TextureCompressionSettings
fails, rows = [], []


def log(m):
    unreal.log("TDPY: " + m)


def tex(path, dest, suffix):
    t = unreal.AssetImportTask()
    for k, v in (("filename", path), ("destination_path", dest), ("automated", True), ("replace_existing", True), ("save", False)):
        t.set_editor_property(k, v)
    tools.import_asset_tasks([t])
    a = eas.load_asset("%s/%s" % (dest, os.path.splitext(os.path.basename(path))[0]))
    if a is None:
        fails.append(path)
        return None
    if suffix == "N":
        a.set_editor_property("compression_settings", TCS.TC_NORMALMAP)
        a.set_editor_property("srgb", False)
        a.set_editor_property("flip_green_channel", False)
        cap = 512
    elif suffix in ("ORM", "Mask"):
        a.set_editor_property("compression_settings", TCS.TC_GRAYSCALE if suffix == "Mask" else TCS.TC_MASKS)
        a.set_editor_property("srgb", False)
        cap = 512
    else:
        a.set_editor_property("compression_settings", TCS.TC_DEFAULT)
        a.set_editor_property("srgb", True)
        cap = 1024
    a.set_editor_property("max_texture_size", cap)
    eas.save_loaded_asset(a, False)
    return a


def mi(dest, name, parent, texs, scalars, vectors):
    path = "%s/%s" % (dest, name)
    m = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(m, eas.load_asset(parent))
    for k, v in texs.items():
        if v:
            mel.set_material_instance_texture_parameter_value(m, k, v)
    for k, v in scalars.items():
        mel.set_material_instance_scalar_parameter_value(m, k, float(v))
    for k, v in vectors.items():
        mel.set_material_instance_vector_parameter_value(m, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    eas.save_loaded_asset(m, False)
    return m


for group, folder, kind, keep_coll in GROUPS:
    src = os.path.join(FAB, group, "export")
    dest = "%s/%s" % (ROOT, folder)
    if not eas.does_directory_exist(dest):
        eas.make_directory(dest)
    t = {s: tex(os.path.join(src, "T_Fab_%s_%s.png" % (group, s)), dest, s)
         for s in ("BC", "N", "ORM") if os.path.isfile(os.path.join(src, "T_Fab_%s_%s.png" % (group, s)))}
    if kind == "scan":
        main = mi(dest, "MI_Fab_" + group, PROP, {"BaseColorTex": t.get("BC"), "NormalTex": t.get("N"), "ORMTex": t.get("ORM")},
                  {"EmissiveStrength": 0.0}, {})
    else:
        main = mi(dest, "MI_Fab_%s_Teal" % group, CARD, {"BaseColorTex": t.get("BC"), "NormalTex": t.get("N")},
                  {"SelfIllum": 0.15, "Roughness": 0.6}, {"Tint": (1, 1, 1)})
        mi(dest, "MI_Fab_%s_Violet" % group, CARD, {"BaseColorTex": t.get("BC"), "NormalTex": t.get("N")},
           {"SelfIllum": 0.2, "Roughness": 0.6}, {"Tint": (1.15, 0.65, 1.35)})
    for f in sorted(os.listdir(src)):
        if not f.endswith(".fbx"):
            continue
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
        mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
        mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(os.path.join(src, f)), params)
        mesh = eas.load_asset("%s/%s" % (dest, f[:-4]))
        if mesh is None:
            fails.append("mesh " + f)
            continue
        ns = mesh.get_editor_property("nanite_settings")
        ns.enabled = kind == "scan"
        mesh.set_editor_property("nanite_settings", ns)
        sme.set_generate_lightmap_uv(mesh, False)
        if not keep_coll:
            sme.remove_collisions(mesh)
            body = mesh.get_editor_property("body_setup")
            if body:
                body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
        for i in range(len(mesh.static_materials)):
            mesh.set_material(i, main)
        eas.save_loaded_asset(mesh, False)
        b = mesh.get_bounding_box()
        rows.append({"mesh": "%s/%s" % (folder, mesh.get_name()), "tris": int(mesh.get_num_triangles(0)),
                     "hulls": sme.get_convex_collision_count(mesh), "nanite": ns.enabled,
                     "size_cm": [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)]})
# leak decal
look = json.load(open(os.path.join(FAB, "DominionLeak", "prep_report.json"), encoding="utf-8"))["look"]
dest = ROOT + "/DominionLeak"
if not eas.does_directory_exist(dest):
    eas.make_directory(dest)
mask = tex(os.path.join(FAB, "DominionLeak", "export", "T_Fab_DominionLeak_Mask.png"), dest, "Mask")
if mask:
    mask.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    mask.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    eas.save_loaded_asset(mask, False)
    mi(dest, "MI_Fab_DominionLeak", DECAL, {"MaskTex": mask}, {k: v for k, v in look.items() if not isinstance(v, list)},
       {k: v for k, v in look.items() if isinstance(v, list)})
for r in rows:
    log("MESH " + json.dumps(r))
log("OK" if not fails else "FAILED %s" % fails)
