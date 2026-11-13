# Triad Drift - the additive glow for the placeholder explosions and flashes (Teammate 2, asked by the Lead, Nov 13).
# Run headless:  powershell -File Tools\td-python.ps1 -Script Source\unrealGameJam\TriadDrift\Combat\Scripts\td_make_flash_material.py
#
# M_TD_FlashGlow: unlit, ADDITIVE (a fading flash goes transparent, never a dark disc). Emissive =
#   Color (vector) x Opacity (scalar) x soft-sphere falloff (1 - Fresnel)^EdgePower,
# so a sphere reads as a glowing ball brightest at its centre with a soft edge instead of a flat disc. The C++ falls back
# to the opaque M_TD_PlaceholderGlow when this asset is missing. /Game/TriadDrift/Gameplay/Materials is always cooked.
#
# Idempotent: an existing material is rebuilt in place.

import unreal

MAT_DIR = "/Game/TriadDrift/Gameplay/Materials"
NAME = "M_TD_FlashGlow"


def log(message):
    unreal.log("TDPY: " + message)


def main():
    assets = unreal.EditorAssetLibrary
    lib = unreal.MaterialEditingLibrary
    path = "%s/%s" % (MAT_DIR, NAME)
    if not assets.does_directory_exist(MAT_DIR):
        assets.make_directory(MAT_DIR)
    if assets.does_asset_exist(path):
        material = assets.load_asset(path)
        lib.delete_all_material_expressions(material)
        log("rebuilding %s" % path)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = tools.create_asset(NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
        log("created %s" % path)
    if material is None:
        log("FAILED: could not create or load %s" % path)
        return

    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("two_sided", False)

    color = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -900, -200)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    opacity = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 0)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 1.0)
    edge = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 200)
    edge.set_editor_property("parameter_name", "EdgePower")
    edge.set_editor_property("default_value", 1.5)

    fresnel = lib.create_material_expression(material, unreal.MaterialExpressionFresnel, -900, 400)
    fresnel.set_editor_property("exponent", 1.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.0)
    inv = lib.create_material_expression(material, unreal.MaterialExpressionOneMinus, -650, 400)
    lib.connect_material_expressions(fresnel, "", inv, "")
    clamp = lib.create_material_expression(material, unreal.MaterialExpressionSaturate, -500, 400)
    lib.connect_material_expressions(inv, "", clamp, "")
    power = lib.create_material_expression(material, unreal.MaterialExpressionPower, -350, 350)
    lib.connect_material_expressions(clamp, "", power, "Base")
    lib.connect_material_expressions(edge, "", power, "Exponent")

    mul_a = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -500, -100)
    lib.connect_material_expressions(color, "", mul_a, "A")
    lib.connect_material_expressions(opacity, "", mul_a, "B")
    mul_b = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 0)
    lib.connect_material_expressions(mul_a, "", mul_b, "A")
    lib.connect_material_expressions(power, "", mul_b, "B")
    if not lib.connect_material_property(mul_b, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        log("FAILED: could not connect emissive")
        return

    try:
        lib.recompile_material(material)
    except RuntimeError:
        log("note: no renderer here, shaders will compile on first load")
    if not assets.save_asset(path, only_if_is_dirty=False):
        log("FAILED: could not save %s" % path)
        return

    check = assets.load_asset(path)
    additive = check.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_ADDITIVE
    vectors = [str(n) for n in lib.get_vector_parameter_names(check)]
    scalars = [str(n) for n in lib.get_scalar_parameter_names(check)]
    log("additive=%s vectors=%s scalars=%s" % (additive, vectors, scalars))
    if additive and "Color" in vectors and "Opacity" in scalars and "EdgePower" in scalars:
        log("OK")
    else:
        log("FAILED: material does not have the expected setup")


main()
