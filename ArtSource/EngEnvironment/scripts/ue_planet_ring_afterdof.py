# SPDX-License-Identifier: MIT
# Issue #90 item 5 (Nicholas, menu): a blurry teal patch where Verdance's rings cross the planet. The ring
# (M_TD_PlanetRing, translucent) and the atmosphere shell (M_TD_PlanetAtmosphere, additive) do not write depth, so the
# menu and intro cameras' depth of field blurred them with the planet's depth behind them. Render both after depth of
# field: they stay sharp where they cross the planet. Material property only (no graph change). Environment builder.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_planet_ring_afterdof.py
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
bad = []
for path in ("/Game/TriadDrift/Gameplay/Props/Planets/M_TD_PlanetRing", "/Game/TriadDrift/Gameplay/Props/Planets/M_TD_PlanetAtmosphere"):
    m = eal.load_asset(path)
    if m is None:
        bad.append("missing " + path)
        continue
    try:
        before = m.get_editor_property("translucency_pass")
        m.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_AFTER_DOF)
        mel.recompile_material(m)
        eal.save_asset(path, only_if_is_dirty=False)
        unreal.log("TDPY: %s translucency pass %s -> after DOF" % (path, before))
    except Exception as e:  # noqa: BLE001
        bad.append("%s: %s" % (path, e))
unreal.log("TDPY: " + ("FAILED %s" % bad if bad else "OK"))
