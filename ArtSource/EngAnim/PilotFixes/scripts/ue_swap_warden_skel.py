# Triad Drift - replace the Warden clips that were on SKEL_NullCrew by their SKEL_Warden copies (<name>_W, made by
# ue_pilot_fixes.py with fixes_warden_onto_own_skeleton.json): delete the old asset, rename the copy to its name.
import unreal
EAL = unreal.EditorAssetLibrary
F = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/"
bad = 0
for n in ("Guard", "Walk", "Stagger", "GuardRaise", "Slam"):
    old, new = F + "A_Crew_Warden_" + n, F + "A_Crew_Warden_" + n + "_W"
    a = EAL.load_asset(new)
    if a is None or a.get_editor_property("skeleton").get_name() != "SKEL_Warden":
        unreal.log("TDPY: FAILED no SKEL_Warden copy of " + n); bad += 1; continue
    a = None
    if EAL.does_asset_exist(old) and not EAL.delete_asset(old):
        unreal.log("TDPY: FAILED could not delete " + old); bad += 1; continue
    ok = EAL.rename_asset(new, old)
    EAL.save_asset(old, only_if_is_dirty=False)
    b = EAL.load_asset(old)
    unreal.log("TDPY: %s: renamed %s, now on %s, %.2f s" % (n, ok, b.get_editor_property("skeleton").get_name() if b else None, b.get_play_length() if b else 0))
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for d in reg.get_assets_by_path(F[:-1], recursive=False):
    if str(d.asset_class_path.asset_name) == "ObjectRedirector":
        unreal.log("TDPY: removing redirector %s: %s" % (d.package_name, EAL.delete_asset(str(d.package_name))))
unreal.log("TDPY: OK" if not bad else "TDPY: %d failed" % bad)
