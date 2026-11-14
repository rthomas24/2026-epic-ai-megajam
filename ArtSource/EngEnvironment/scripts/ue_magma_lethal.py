# SPDX-License-Identifier: MIT
# Cinder magma must read as lethal at the pilot's eye height (Lead, Nov 14: a playtest pilot walked into it): retune
# MI_Cind_MagmaSurface (M_TD_Magma, the props builder's graph is not touched): wider, hotter molten channels between
# fewer crust plates, brighter cracks that stay lit out to 30-90 m, a yellow-white core. Environment builder, pass 5.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_magma_lethal.py
import unreal

PATH = "/Game/TriadDrift/Gameplay/Props/Planet/MI_Cind_MagmaSurface"
SCALARS = {
    "ChannelMin": 0.34, "ChannelMax": 0.82, "ChannelPower": 1.4,     # was 0.18 / 0.65 / 2.0: thin dark-crust field
    "EmissiveStrength": 6.0,                                         # was 2.5
    "CrackGlow": 0.55, "CrackWidth": 0.11,                           # was 0.18 / 0.08
    "CrackFadeStart": 3000.0, "CrackFadeRange": 6000.0,              # was 600 / 1500: cracks went dark beyond ~20 m
    "FlickerSpeed": 3.5, "RimWidth": 30.0,
}
VECTORS = {
    "HotCore": (1.0, 0.5, 0.08),                                     # was (1.0, 0.32, 0.03)
    "HotEdge": (0.95, 0.12, 0.012),
    "CrackColor": (1.0, 0.2, 0.02),
    "MoltenBase": (0.2, 0.05, 0.008),
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
