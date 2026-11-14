# SPDX-License-Identifier: MIT
# Cloud tops (environment builder, Nov 14): the Aftermath opened against Verdance's cloud layer as a flat white-and-yellow
# sheet. Rebuilds the M_TD_Planet graph from ue_planet_surface.py (close-range cloud breakup and relief) WITHOUT touching
# the planet instances' tuned values (ue_rebuild_planet_surface.py would reset them), then takes the yellow out of
# Verdance's close-range detail tint (1.35/1.15/0.55 -> 1.05/1.2/0.8).
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_planet_cloudtops.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.abspath(os.path.join(HERE, "..", "..", "EngProps", "Planets", "scripts")))
import ue_mat_builder  # noqa: E402
import ue_planet_surface as surf  # noqa: E402

D = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
problems = []
mat = eal.load_asset(D + "/M_TD_Planet")
t = {n: eal.load_asset("%s/%s" % (D, n)) for n in
     ("T_Planet_Verdance_BC", "T_Planet_Verdance_N", "T_Planet_Verdance_ORM", "T_Planet_Verdance_E", "T_Planet_Verdance_Clouds", "T_Ground_MacroNoise")}
missing = [k for k, v in t.items() if v is None]
if mat is None or missing:
    unreal.log("TDPY: FAILED missing %s" % missing)
else:
    mel.delete_all_material_expressions(mat)
    b = ue_mat_builder.MB(mat, problems)
    surf.build_surface_graph(b, t)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    n = mel.get_statistics(mat).get_editor_property("num_pixel_shader_instructions")
    unreal.log("TDPY: M_TD_Planet %s pixel instructions, problems %s" % (n, problems))
    if n and not problems:
        eal.save_loaded_asset(mat, False)
        mi = eal.load_asset(D + "/MI_Planet_Verdance")
        mel.set_material_instance_vector_parameter_value(mi, "DetailTint", unreal.LinearColor(1.05, 1.2, 0.8, 1.0))
        # T4 (Nov 14): close up, a teal wash swamped the baked ground: the #148 night values (glow 11, ambient 0.3) back
        # toward c0ea8682 (7 / 0.2); the eclipse gets its lift from the shot's exposure instead (td.Intro.EclipseEV, T3)
        mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", 7.0)
        mel.set_material_instance_scalar_parameter_value(mi, "NightAmbient", 0.2)
        mel.update_material_instance(mi)
        eal.save_loaded_asset(mi, False)
        # ArtBaseTex (the painted globe shown over the landing cap from afar) defaults to Verdance's: Cinder gets its own
        cin = eal.load_asset(D + "/MI_Planet_Cinder")
        ctex = eal.load_asset(D + "/T_Planet_Cinder_BC")
        if cin and ctex:
            mel.set_material_instance_texture_parameter_value(cin, "ArtBaseTex", ctex)
            mel.update_material_instance(cin)
            eal.save_loaded_asset(cin, False)
        unreal.log("TDPY: Cinder ArtBaseTex %s" % ("set" if (cin and ctex) else "MISSING"))
        unreal.log("TDPY: OK")
    else:
        unreal.log("TDPY: FAILED (not saved)")
