# Rebuild M_TD_Planet IN PLACE with the night-side graph (ue_planet_surface.py), keeping the asset, its path and the
# instances (MI_Planet_Verdance / MI_Planet_Cinder keep their texture and look overrides), then set the night-side
# instance values and the atmosphere instances' SunDirection default / SunMaskAmount.
# Must run with a renderer (the headless -nullrhi editor crashes editing a master that has instances):
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_rebuild_planet_surface.py
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ue_mat_builder  # noqa: E402
import ue_planet_surface as surf  # noqa: E402

DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
problems = []


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    mat = eal.load_asset(DEST + "/M_TD_Planet")
    if mat is None:
        log("FAILED: M_TD_Planet missing")
        return
    t = {n: eal.load_asset("%s/%s" % (DEST, n)) for n in
         ("T_Planet_Verdance_BC", "T_Planet_Verdance_N", "T_Planet_Verdance_ORM", "T_Planet_Verdance_E", "T_Planet_Verdance_Clouds")}
    if any(v is None for v in t.values()):
        log("FAILED: textures missing %s" % [k for k, v in t.items() if v is None])
        return
    mel.delete_all_material_expressions(mat)
    b = ue_mat_builder.MB(mat, problems)
    surf.build_surface_graph(b, t)
    if problems:
        for p in problems:
            log("PROBLEM: " + p)
        log("FAILED: graph wiring, not saved")
        return
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    st = mel.get_statistics(mat)
    n = st.get_editor_property("num_pixel_shader_instructions")
    log("M_TD_Planet compiled: %s pixel instructions, %s samplers, shading %s" % (
        n, st.get_editor_property("num_samplers"), mat.get_editor_property("shading_model")))
    if not n:
        log("FAILED: M_TD_Planet did not compile, not saved")
        return
    eal.save_loaded_asset(mat, False)
    for p, vals in surf.INSTANCE_VALUES.items():
        inst = eal.load_asset("%s/MI_Planet_%s" % (DEST, p))
        for k, v in vals["scalars"].items():
            mel.set_material_instance_scalar_parameter_value(inst, k, float(v))
        for k, v in vals["vectors"].items():
            mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        mel.set_material_instance_vector_parameter_value(inst, "SunDirection", unreal.LinearColor(*surf.SUN_DIR, 1.0))
        mel.update_material_instance(inst)
        st = mel.get_statistics(inst)
        log("MI_Planet_%s parent %s: %s pixel instructions, NightGlowMask %.2f, EmissiveStrength %.1f" % (
            p, inst.get_editor_property("parent").get_name(), st.get_editor_property("num_pixel_shader_instructions"),
            mel.get_material_instance_scalar_parameter_value(inst, "NightGlowMask"),
            mel.get_material_instance_scalar_parameter_value(inst, "EmissiveStrength")))
        eal.save_loaded_asset(inst, False)
        atmo = eal.load_asset("%s/MI_PlanetAtmo_%s" % (DEST, p))
        for k, v in surf.ATMO_VALUES.items():
            mel.set_material_instance_scalar_parameter_value(atmo, k, float(v))
        mel.set_material_instance_vector_parameter_value(atmo, "SunDirection", unreal.LinearColor(*surf.SUN_DIR, 1.0))
        mel.update_material_instance(atmo)
        eal.save_loaded_asset(atmo, False)
        log("MI_PlanetAtmo_%s SunDirection %s SunMaskAmount %s" % (p, surf.SUN_DIR, surf.ATMO_VALUES["SunMaskAmount"]))
    dirty = [str(x.get_name()) for x in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    log("still dirty (not saved): %s" % dirty)
    log("OK")


main()
