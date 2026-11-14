# SPDX-License-Identifier: MIT
# Triad Drift - second Verdance / Cinder batch from Quixel (Fab Standard), headless (Tools\td-python.ps1):
#   Plants: T_Fab_VerdancePlants_BC (RGBA, opacity in alpha) / _N at 1024, a masked two-sided card master
#           M_TD_FabFoliageCard (colour x Tint, opacity mask = BC alpha, SelfIllum for Verdance's glow), instances
#           MI_Fab_VerdancePlants_Teal / _Violet, and SM_Fab_VerdancePlant_Fern / _Broad / _Long (no collision, no Nanite).
#   Ash:    T_Fab_AshDrift_Mask (1024 grayscale) and MI_Fab_AshDrift_Cinder on the existing M_TD_FabDecal.
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
FAB = os.path.dirname(HERE)
PL = "/Game/TriadDrift/Gameplay/Props/Fab/VerdancePlants"
ASH = "/Game/TriadDrift/Gameplay/Props/Fab/AshDrift"
DECAL = "/Game/TriadDrift/Gameplay/Props/Fab/ScorchMark/M_TD_FabDecal"
PIPELINE = "/Game/TriadDrift/Gameplay/Ships/Pipelines/PL_Ship_StaticMesh"
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
TCS = unreal.TextureCompressionSettings
fails = []


def log(m):
    unreal.log("TDPY: " + m)


def tex(path, dest, comp, srgb):
    if not eas.does_directory_exist(dest):
        eas.make_directory(dest)
    t = unreal.AssetImportTask()
    for k, v in (("filename", path), ("destination_path", dest), ("automated", True), ("replace_existing", True), ("save", False)):
        t.set_editor_property(k, v)
    tools.import_asset_tasks([t])
    a = eas.load_asset("%s/%s" % (dest, os.path.splitext(os.path.basename(path))[0]))
    if a is None:
        fails.append(path)
        return None
    a.set_editor_property("compression_settings", comp)
    a.set_editor_property("srgb", srgb)
    a.set_editor_property("max_texture_size", 1024)
    if comp == TCS.TC_NORMALMAP:
        a.set_editor_property("flip_green_channel", False)
    eas.save_loaded_asset(a, False)
    return a


def mi(dest, name, parent, texs, scalars, vectors):
    path = "%s/%s" % (dest, name)
    m = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(m, parent)
    for k, v in texs.items():
        mel.set_material_instance_texture_parameter_value(m, k, v)
    for k, v in scalars.items():
        mel.set_material_instance_scalar_parameter_value(m, k, float(v))
    for k, v in vectors.items():
        mel.set_material_instance_vector_parameter_value(m, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    if not eas.save_loaded_asset(m, False):
        fails.append("save " + name)
    return m


# ---- plants
pe = os.path.join(FAB, "VerdancePlants", "export")
bc = tex(os.path.join(pe, "T_Fab_VerdancePlants_BC.png"), PL, TCS.TC_DEFAULT, True)
nm = tex(os.path.join(pe, "T_Fab_VerdancePlants_N.png"), PL, TCS.TC_NORMALMAP, False)
MASTER = PL + "/M_TD_FabFoliageCard"
if not eas.does_asset_exist(MASTER):
    m = tools.create_asset("M_TD_FabFoliageCard", PL, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    for flag in ("used_with_instanced_static_meshes",):
        m.set_editor_property(flag, True)
    ST = unreal.MaterialSamplerType

    def e(cls, x, y):
        return mel.create_material_expression(m, cls, x, y)

    def scalar(n, v, x, y):
        s = e(unreal.MaterialExpressionScalarParameter, x, y)
        s.set_editor_property("parameter_name", n)
        s.set_editor_property("default_value", v)
        return s

    b = e(unreal.MaterialExpressionTextureSampleParameter2D, -800, -100)
    b.set_editor_property("parameter_name", "BaseColorTex")
    b.set_editor_property("texture", bc)
    b.set_editor_property("sampler_type", ST.SAMPLERTYPE_COLOR)
    n = e(unreal.MaterialExpressionTextureSampleParameter2D, -800, 250)
    n.set_editor_property("parameter_name", "NormalTex")
    n.set_editor_property("texture", nm)
    n.set_editor_property("sampler_type", ST.SAMPLERTYPE_NORMAL)
    tint = e(unreal.MaterialExpressionVectorParameter, -800, -300)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    col = e(unreal.MaterialExpressionMultiply, -500, -150)
    glow = e(unreal.MaterialExpressionMultiply, -300, 0)
    ok = mel.connect_material_expressions(b, "RGB", col, "A")
    ok &= mel.connect_material_expressions(tint, "RGB", col, "B")
    ok &= mel.connect_material_expressions(col, "", glow, "A")
    ok &= mel.connect_material_expressions(scalar("SelfIllum", 0.15, -500, 50), "", glow, "B")
    mp = unreal.MaterialProperty
    ok &= mel.connect_material_property(col, "", mp.MP_BASE_COLOR)
    ok &= mel.connect_material_property(glow, "", mp.MP_EMISSIVE_COLOR)
    ok &= mel.connect_material_property(b, "A", mp.MP_OPACITY_MASK)
    ok &= mel.connect_material_property(n, "RGB", mp.MP_NORMAL)
    ok &= mel.connect_material_property(scalar("Roughness", 0.6, -500, 150), "", mp.MP_ROUGHNESS)
    if not ok:
        fails.append("foliage wiring")
    try:
        mel.recompile_material(m)
    except RuntimeError:
        log("note: no renderer, M_TD_FabFoliageCard compiles on first load")
    eas.save_asset(MASTER, only_if_is_dirty=False)
master = eas.load_asset(MASTER)
teal = mi(PL, "MI_Fab_VerdancePlants_Teal", master, {"BaseColorTex": bc, "NormalTex": nm}, {"SelfIllum": 0.15, "Roughness": 0.6}, {"Tint": (1, 1, 1)})
mi(PL, "MI_Fab_VerdancePlants_Violet", master, {"BaseColorTex": bc, "NormalTex": nm}, {"SelfIllum": 0.2, "Roughness": 0.6}, {"Tint": (1.15, 0.65, 1.35)})
rows = []
for name in ("Fern", "Broad", "Long"):
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    mgr.import_asset(PL, unreal.InterchangeManager.create_source_data(os.path.join(pe, "SM_Fab_VerdancePlant_%s.fbx" % name)), params)
    mesh = eas.load_asset("%s/SM_Fab_VerdancePlant_%s" % (PL, name))
    if mesh is None:
        fails.append("mesh " + name)
        continue
    ns = mesh.get_editor_property("nanite_settings")
    ns.enabled = False                                   # 24-32 masked triangles: Nanite buys nothing here
    mesh.set_editor_property("nanite_settings", ns)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    sme.set_generate_lightmap_uv(mesh, False)
    sme.remove_collisions(mesh)
    body = mesh.get_editor_property("body_setup")
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    for i in range(len(mesh.static_materials)):
        mesh.set_material(i, teal)
    eas.save_loaded_asset(mesh, False)
    rows.append({"mesh": mesh.get_name(), "tris": int(mesh.get_num_triangles(0)), "hulls": sme.get_convex_collision_count(mesh)})
log("PLANTS " + json.dumps(rows))

# ---- ash drift decal
look = json.load(open(os.path.join(FAB, "AshDrift", "prep_report.json"), encoding="utf-8"))["looks"]["Cinder"]
mask = tex(os.path.join(FAB, "AshDrift", "export", "T_Fab_AshDrift_Mask.png"), ASH, TCS.TC_GRAYSCALE, False)
if mask:
    mask.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    mask.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    eas.save_loaded_asset(mask, False)
    mi(ASH, "MI_Fab_AshDrift_Cinder", eas.load_asset(DECAL), {"MaskTex": mask},
       {k: v for k, v in look.items() if not isinstance(v, (list, tuple))},
       {k: v for k, v in look.items() if isinstance(v, (list, tuple))})
log("OK" if not fails else "FAILED %s" % fails)
