# SPDX-License-Identifier: MIT
# Issue #90 item 8 (Nicholas): polish of the opening shot (the intro's Verdance eclipse) and the menu planet. Instance
# values only (M_TD_Planet / M_TD_PlanetAtmosphere / M_TD_PlanetRing graphs untouched), so the galaxy view gains too:
#   - the night side was a featureless black disc: a little more night ambient and Verdance's bioluminescent night glow
#   - backlit, the atmosphere rim vanished on the night side (sun mask 0.92): a thin scattering ring all round the limb
#   - the rings read dim brown against the sun: a touch brighter.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_planet_polish.py
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
D = "/Game/TriadDrift/Gameplay/Props/Planets/"
VALUES = {   # #148 (second pass, Lead: "still a hole"): the planet must read as a world in the eclipse shot
    "MI_Planet_Verdance": {"NightAmbient": 0.2, "EmissiveStrength": 7.0, "SurfaceRimStrength": 1.4, "NightGlowStart": 0.0},
    # ART-13 a13 (third pass): 4.5 / 3.2 washed the globe in flat teal haze close up (2.2 and 1.6 radii in the galaxy);
    # back to the c0ea8682 strength and the default rim falloff, keeping the rim round the night side (0.3)
    "MI_PlanetAtmo_Verdance": {"SunMaskAmount": 0.3, "AtmoIntensity": 3.4, "RimPower": 4.0},
    "MI_PlanetRing_Verdance": {"Brightness": 2.0},
}
bad = []
for name, vals in VALUES.items():
    mi = eal.load_asset(D + name)
    if mi is None:
        bad.append(name)
        continue
    for k, v in vals.items():
        if not mel.set_material_instance_scalar_parameter_value(mi, k, float(v)):
            bad.append("%s.%s" % (name, k))
    mel.update_material_instance(mi)
    eal.save_asset(D + name, only_if_is_dirty=False)
unreal.log("TDPY: " + ("FAILED %s" % bad if bad else "OK"))
