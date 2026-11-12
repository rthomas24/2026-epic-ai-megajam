# Triad Drift - create the gameplay-owned placeholder materials. Lead-owned.
# Run headless:  powershell -File Tools\td-python.ps1 -Script Tools\td-make-materials.py
#
# M_TD_PlaceholderGlow: unlit, emissive = the vector parameter "Color" (the same parameter name the
# engine's BasicShapeMaterial and EmissiveMeshMaterial use, so UTDStatics::TintPlaceholder works with
# it unchanged). Unlike the engine's EmissiveMeshMaterial it carries the usage flags a cooked game
# needs: instanced static meshes (stars, asteroid markers), skeletal meshes and Niagara mesh
# particles. In a cooked build a material without the right usage flag is silently replaced by the
# grey default material.
#
# Idempotent: an existing material is rebuilt in place. Claim the asset in CLAIMS.md before running.

import unreal

MAT_DIR = "/Game/TriadDrift/Gameplay/Materials"
NAME = "M_TD_PlaceholderGlow"


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
    material.set_editor_property("two_sided", False)
    for flag in ("used_with_instanced_static_meshes", "used_with_skeletal_mesh", "used_with_niagara_mesh_particles",
                 "used_with_static_lighting"):
        try:
            material.set_editor_property(flag, True)
        except Exception as error:  # a flag renamed in this engine build must not stop the others
            log("note: could not set %s (%s)" % (flag, error))

    color = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -420, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    if not lib.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        log("FAILED: could not connect Color to emissive")
        return

    try:
        lib.recompile_material(material)
    except RuntimeError:
        # The headless editor has no renderer (-nullrhi). The graph and flags are saved as they are;
        # shaders are compiled by the first editor or cook that loads the asset.
        log("note: no renderer here, shaders will compile on first load")
    if not assets.save_asset(path, only_if_is_dirty=False):
        log("FAILED: could not save %s" % path)
        return

    # Read it back: the checks a cooked game depends on.
    check = assets.load_asset(path)
    unlit = check.get_editor_property("shading_model") == unreal.MaterialShadingModel.MSM_UNLIT
    instanced = check.get_editor_property("used_with_instanced_static_meshes")
    names = [str(n) for n in lib.get_vector_parameter_names(check)]
    log("unlit=%s instanced=%s vector parameters=%s" % (unlit, instanced, names))
    if unlit and instanced and "Color" in names:
        log("OK")
    else:
        log("FAILED: material does not have the expected setup")


main()
