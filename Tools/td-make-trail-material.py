# Triad Drift - create the translucent glow for the placeholder engine trails (T1, asked by the Lead, Nov 13).
# Run headless:  powershell -File Tools\td-python.ps1 -Script Tools\td-make-trail-material.py
#
# M_TD_TrailGlow: translucent, unlit. Emissive = vector parameter "Color" (as M_TD_PlaceholderGlow, so
# UTDStatics::TintPlaceholder works unchanged); opacity = scalar parameter "Opacity" x per-instance custom data 0,
# so each instanced segment of a ribbon fades on its own (UTDFlightFeedbackComponent writes 1 at the nozzle down to
# 0 at the tail). Usage flags for instanced static meshes so a cooked game keeps it. Nick's EngineTrail Niagara cue
# still replaces the whole placeholder when it is assigned.
#
# Idempotent: an existing material is rebuilt in place.

import unreal

MAT_DIR = "/Game/TriadDrift/Gameplay/Materials"
NAME = "M_TD_TrailGlow"


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
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", True)
    for flag in ("used_with_instanced_static_meshes", "used_with_niagara_mesh_particles"):
        try:
            material.set_editor_property(flag, True)
        except Exception as error:
            log("note: could not set %s (%s)" % (flag, error))

    color = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, -100)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    if not lib.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        log("FAILED: could not connect Color to emissive")
        return

    opacity = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 150)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.7)
    fade = lib.create_material_expression(material, unreal.MaterialExpressionPerInstanceCustomData, -600, 300)
    fade.set_editor_property("data_index", 0)
    mul = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 200)
    lib.connect_material_expressions(opacity, "", mul, "A")
    lib.connect_material_expressions(fade, "", mul, "B")
    if not lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_OPACITY):
        log("FAILED: could not connect opacity")
        return

    try:
        lib.recompile_material(material)
    except RuntimeError:
        log("note: no renderer here, shaders will compile on first load")
    if not assets.save_asset(path, only_if_is_dirty=False):
        log("FAILED: could not save %s" % path)
        return

    check = assets.load_asset(path)
    translucent = check.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_TRANSLUCENT
    instanced = check.get_editor_property("used_with_instanced_static_meshes")
    vectors = [str(n) for n in lib.get_vector_parameter_names(check)]
    scalars = [str(n) for n in lib.get_scalar_parameter_names(check)]
    log("translucent=%s instanced=%s vectors=%s scalars=%s" % (translucent, instanced, vectors, scalars))
    if translucent and instanced and "Color" in vectors and "Opacity" in scalars:
        log("OK")
    else:
        log("FAILED: material does not have the expected setup")


main()
