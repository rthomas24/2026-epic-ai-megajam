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
VALUES = {
    "MI_Planet_Verdance": {"NightAmbient": 0.17, "EmissiveStrength": 7.0, "SurfaceRimStrength": 0.8},   # was 0.10 / 4.5 / 0.5
    "MI_PlanetAtmo_Verdance": {"SunMaskAmount": 0.62, "AtmoIntensity": 3.4},                        # was 0.92 / 3.0
    "MI_PlanetRing_Verdance": {"Brightness": 1.55},                                                   # was 1.2
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
