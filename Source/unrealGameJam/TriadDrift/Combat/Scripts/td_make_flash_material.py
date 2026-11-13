# Triad Drift - the glow and smoke materials for the placeholder explosions and flashes (Teammate 2, asked by the Lead,
# Nov 13). Run headless:
#   powershell -File Tools\td-python.ps1 -Script Source\unrealGameJam\TriadDrift\Combat\Scripts\td_make_flash_material.py
#
# M_TD_FlashGlow: unlit, ADDITIVE (a fading flash goes transparent, never a dark disc). Emissive =
#   Color x Opacity x lerp((1 - Fresnel)^EdgePower, Fresnel^EdgePower, Rim)
# so a sphere is a glow brightest at its centre (Rim 0) or a ring brightest at its rim (Rim 1, the shockwave).
# M_TD_SmokePuff: unlit, TRANSLUCENT (dark smoke can't be additive). Emissive = Color; opacity =
#   Opacity x (1 - Fresnel)^EdgePower, a soft-edged puff.
# The C++ falls back to the opaque M_TD_PlaceholderGlow when these are missing. /Game/TriadDrift/Gameplay/Materials is
# always cooked. Idempotent: existing materials are rebuilt in place.

import unreal

MAT_DIR = "/Game/TriadDrift/Gameplay/Materials"


def log(message):
    unreal.log("TDPY: " + message)


def open_material(name):
    assets = unreal.EditorAssetLibrary
    lib = unreal.MaterialEditingLibrary
    path = "%s/%s" % (MAT_DIR, name)
    if not assets.does_directory_exist(MAT_DIR):
        assets.make_directory(MAT_DIR)
    if assets.does_asset_exist(path):
        material = assets.load_asset(path)
        lib.delete_all_material_expressions(material)
        log("rebuilding %s" % path)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
        log("created %s" % path)
    return material, path


def scalar(lib, material, name, default, x, y):
    node = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def falloff(lib, material, edge, y):
    """Returns (centre, rim): (1 - Fresnel)^Edge and Fresnel^Edge."""
    fresnel = lib.create_material_expression(material, unreal.MaterialExpressionFresnel, -1100, y)
    fresnel.set_editor_property("exponent", 1.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.0)
    sat = lib.create_material_expression(material, unreal.MaterialExpressionSaturate, -950, y)
    lib.connect_material_expressions(fresnel, "", sat, "")
    inv = lib.create_material_expression(material, unreal.MaterialExpressionOneMinus, -800, y)
    lib.connect_material_expressions(sat, "", inv, "")
    centre = lib.create_material_expression(material, unreal.MaterialExpressionPower, -650, y)
    lib.connect_material_expressions(inv, "", centre, "Base")
    lib.connect_material_expressions(edge, "", centre, "Exponent")
    rim = lib.create_material_expression(material, unreal.MaterialExpressionPower, -650, y + 150)
    lib.connect_material_expressions(sat, "", rim, "Base")
    lib.connect_material_expressions(edge, "", rim, "Exponent")
    return centre, rim


def save(material, path):
    assets = unreal.EditorAssetLibrary
    try:
        unreal.MaterialEditingLibrary.recompile_material(material)
    except RuntimeError:
        log("note: no renderer here, shaders will compile on first load")
    return assets.save_asset(path, only_if_is_dirty=False)


def build_glow():
    lib = unreal.MaterialEditingLibrary
    material, path = open_material("M_TD_FlashGlow")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("two_sided", False)
    color = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -1100, -300)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    opacity = scalar(lib, material, "Opacity", 1.0, -1100, -150)
    edge = scalar(lib, material, "EdgePower", 1.5, -1100, 0)
    rim_switch = scalar(lib, material, "Rim", 0.0, -1100, 150)
    centre, rim = falloff(lib, material, edge, 300)
    shape = lib.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -450, 300)
    lib.connect_material_expressions(centre, "", shape, "A")
    lib.connect_material_expressions(rim, "", shape, "B")
    lib.connect_material_expressions(rim_switch, "", shape, "Alpha")
    mul_a = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -450, -200)
    lib.connect_material_expressions(color, "", mul_a, "A")
    lib.connect_material_expressions(opacity, "", mul_a, "B")
    mul_b = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 0)
    lib.connect_material_expressions(mul_a, "", mul_b, "A")
    lib.connect_material_expressions(shape, "", mul_b, "B")
    if not lib.connect_material_property(mul_b, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        log("FAILED: could not connect glow emissive")
        return False
    if not save(material, path):
        log("FAILED: could not save %s" % path)
        return False
    check = unreal.EditorAssetLibrary.load_asset(path)
    scalars = [str(n) for n in lib.get_scalar_parameter_names(check)]
    ok = check.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_ADDITIVE and all(
        s in scalars for s in ("Opacity", "EdgePower", "Rim"))
    log("M_TD_FlashGlow scalars=%s ok=%s" % (scalars, ok))
    return ok


def build_smoke():
    lib = unreal.MaterialEditingLibrary
    material, path = open_material("M_TD_SmokePuff")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", False)
    color = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -1100, -300)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.05, 0.05, 0.055, 1.0))
    opacity = scalar(lib, material, "Opacity", 0.5, -1100, -150)
    edge = scalar(lib, material, "EdgePower", 1.2, -1100, 0)
    centre, _ = falloff(lib, material, edge, 300)
    if not lib.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        log("FAILED: could not connect smoke colour")
        return False
    mul = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 100)
    lib.connect_material_expressions(opacity, "", mul, "A")
    lib.connect_material_expressions(centre, "", mul, "B")
    if not lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_OPACITY):
        log("FAILED: could not connect smoke opacity")
        return False
    if not save(material, path):
        log("FAILED: could not save %s" % path)
        return False
    check = unreal.EditorAssetLibrary.load_asset(path)
    scalars = [str(n) for n in lib.get_scalar_parameter_names(check)]
    ok = check.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_TRANSLUCENT and "Opacity" in scalars
    log("M_TD_SmokePuff scalars=%s ok=%s" % (scalars, ok))
    return ok


def main():
    if build_glow() and build_smoke():
        log("OK")
    else:
        log("FAILED")


main()
