# SPDX-License-Identifier: MIT
# Cinder magma must read as lethal at the pilot's eye height (Lead, Nov 14: a playtest pilot walked into it): retune
# MI_Cind_MagmaSurface (M_TD_Magma, the props builder's graph is not touched): wider, hotter molten channels between
# fewer crust plates, brighter cracks that stay lit out to 30-90 m, a yellow-white core. Environment builder, pass 5.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_magma_lethal.py
import unreal

PATH = "/Game/TriadDrift/Gameplay/Props/Planet/MI_Cind_MagmaSurface"
SCALARS = {
    # s11 (T7: 21 deaths at the conduit route, "thin cracks on foot"): mostly molten with drifting crust islands,
    # a dark crusted bank at the edge, bright enough to read from 20 m on foot
    "ChannelMin": 0.62, "ChannelMax": 0.95, "ChannelPower": 1.1,     # s6 0.34 / 0.82 / 1.4; original 0.18 / 0.65 / 2.0
    "EmissiveStrength": 8.0,                                         # s6 6.0; original 2.5
    "CrackGlow": 0.7, "CrackWidth": 0.14,
    "CrackFadeStart": 3000.0, "CrackFadeRange": 6000.0,
    "FlickerSpeed": 3.5, "RimWidth": 70.0, "RimSoft": 90.0,          # a crusted bank you can see the edge by
}
VECTORS = {
    "HotCore": (1.0, 0.55, 0.1),
    "HotEdge": (1.0, 0.16, 0.015),
    "CrackColor": (1.0, 0.22, 0.02),
    "MoltenBase": (0.25, 0.06, 0.01),
    "RimAsh": (0.012, 0.009, 0.008),                                 # near-black crust on the banks
}
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
mi = eal.load_asset(PATH)
bad = []
if mi is None:
    bad.append("missing " + PATH)
else:
    for k, v in SCALARS.items():
        if not mel.set_material_instance_scalar_parameter_value(mi, k, float(v)):
            bad.append(k)
    for k, v in VECTORS.items():
        if not mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0)):
            bad.append(k)
    mel.update_material_instance(mi)
    eal.save_asset(PATH, only_if_is_dirty=False)
unreal.log("TDPY: " + ("FAILED %s" % bad if bad else "OK"))
