# SPDX-License-Identifier: MIT
# Triad Drift - Quixel "Scorch Mark" as deferred decals, headless (Tools\td-python.ps1):
#   T_Fab_ScorchMark_Mask (1024, grayscale, linear), master M_TD_FabDecal (deferred decal, translucent), and
#   MI_Fab_ScorchMark_Cinder / MI_Fab_ScorchMark_Verdance with the looks from prep_fab_scorch.py (prep_report.json).
# M_TD_FabDecal: opacity = saturate((mask - Bias) * Contrast) * Opacity; base colour = CharColor; roughness = CharRoughness;
# emissive = EmberColor * EmberStrength * mask^EmberPower. Nothing is placed or assigned.
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
D = os.path.join(os.path.dirname(HERE), "ScorchMark")
DEST = "/Game/TriadDrift/Gameplay/Props/Fab/ScorchMark"
MASTER = DEST + "/M_TD_FabDecal"
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
fails = []


def log(m):
    unreal.log("TDPY: " + m)


if not eas.does_directory_exist(DEST):
    eas.make_directory(DEST)
t = unreal.AssetImportTask()
for k, v in (("filename", os.path.join(D, "export", "T_Fab_ScorchMark_Mask.png")), ("destination_path", DEST), ("automated", True),
             ("replace_existing", True), ("save", False)):
    t.set_editor_property(k, v)
tools.import_asset_tasks([t])
mask = eas.load_asset(DEST + "/T_Fab_ScorchMark_Mask")
mask.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
mask.set_editor_property("srgb", False)
mask.set_editor_property("max_texture_size", 1024)
mask.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
mask.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
eas.save_loaded_asset(mask, False)

if not eas.does_asset_exist(MASTER):
    m = tools.create_asset("M_TD_FabDecal", DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    def expr(cls, x, y):
        return mel.create_material_expression(m, cls, x, y)

    def scalar(name, value, x, y):
        e = expr(unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", value)
        return e

    def vector(name, value, x, y):
        e = expr(unreal.MaterialExpressionVectorParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", unreal.LinearColor(value[0], value[1], value[2], 1.0))
        return e

    tex = expr(unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
    tex.set_editor_property("parameter_name", "MaskTex")
    tex.set_editor_property("texture", mask)
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    sub = expr(unreal.MaterialExpressionSubtract, -600, 0)
    mul = expr(unreal.MaterialExpressionMultiply, -450, 0)
    sat = expr(unreal.MaterialExpressionSaturate, -300, 0)
    op = expr(unreal.MaterialExpressionMultiply, -150, 0)
    pw = expr(unreal.MaterialExpressionPower, -600, 300)
    em1 = expr(unreal.MaterialExpressionMultiply, -400, 300)
    em2 = expr(unreal.MaterialExpressionMultiply, -200, 300)
    ok = True
    ok &= mel.connect_material_expressions(tex, "R", sub, "A")
    ok &= mel.connect_material_expressions(scalar("Bias", 0.05, -900, 250), "", sub, "B")
    ok &= mel.connect_material_expressions(sub, "", mul, "A")
    ok &= mel.connect_material_expressions(scalar("Contrast", 1.2, -900, 330), "", mul, "B")
    ok &= mel.connect_material_expressions(mul, "", sat, "")
    ok &= mel.connect_material_expressions(sat, "", op, "A")
    ok &= mel.connect_material_expressions(scalar("Opacity", 0.85, -900, 410), "", op, "B")
    ok &= mel.connect_material_expressions(tex, "R", pw, "Base")
    ok &= mel.connect_material_expressions(scalar("EmberPower", 10.0, -900, 490), "", pw, "Exp")
    ok &= mel.connect_material_expressions(pw, "", em1, "A")
    ok &= mel.connect_material_expressions(scalar("EmberStrength", 0.0, -900, 570), "", em1, "B")
    ok &= mel.connect_material_expressions(em1, "", em2, "A")
    ok &= mel.connect_material_expressions(vector("EmberColor", (1.0, 0.28, 0.05), -900, 650), "RGB", em2, "B")
    mp = unreal.MaterialProperty
    ok &= mel.connect_material_property(op, "", mp.MP_OPACITY)
    ok &= mel.connect_material_property(vector("CharColor", (0.012, 0.013, 0.015), -400, -250), "RGB", mp.MP_BASE_COLOR)
    ok &= mel.connect_material_property(scalar("CharRoughness", 0.9, -400, -150), "", mp.MP_ROUGHNESS)
    ok &= mel.connect_material_property(em2, "", mp.MP_EMISSIVE_COLOR)
    if not ok:
        fails.append("wiring")
    try:
        mel.recompile_material(m)
    except RuntimeError:
        log("note: no renderer here, the master's shaders compile on first load")
    eas.save_asset(MASTER, only_if_is_dirty=False)
master = eas.load_asset(MASTER)
looks = json.load(open(os.path.join(D, "prep_report.json"), encoding="utf-8"))["looks"]
for name, look in looks.items():
    path = "%s/MI_Fab_ScorchMark_%s" % (DEST, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        "MI_Fab_ScorchMark_" + name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    mel.set_material_instance_texture_parameter_value(mi, "MaskTex", mask)
    for k, v in look.items():
        if isinstance(v, (list, tuple)):
            mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        else:
            mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    if not eas.save_loaded_asset(mi, False):
        fails.append("save " + name)
    log("%s: parent %s, %s" % (mi.get_name(), mi.get_editor_property("parent").get_name(), json.dumps(look)))
log("master domain %s blend %s; mask %dx%d" % (master.get_editor_property("material_domain"), master.get_editor_property("blend_mode"),
                                              mask.blueprint_get_size_x(), mask.blueprint_get_size_y()))
log("OK" if not fails else "FAILED %s" % fails)
