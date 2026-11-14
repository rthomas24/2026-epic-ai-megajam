# Triad Drift - PA_Warden: report its bodies, then (TD_PA_FIX=1) apply the ragdoll fix and save.
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotFixes\scripts\ue_pa_warden.py
# The work is done by the game module's editor command TD.Ground.PAReport (TDPilotGripDebug.cpp).
# Animation builder (eng/anim), Nov 13.
import os
import unreal
PATH = "/Game/TriadDrift/Gameplay/Characters/Warden/PA_Warden.PA_Warden"
pa = unreal.load_asset(PATH.split(".")[0])
fix = os.environ.get("TD_PA_FIX") == "1"
unreal.SystemLibrary.execute_console_command(None, "TD.Ground.PAReport %s%s" % (PATH, " fix" if fix else ""))
if fix:
    if not unreal.EditorAssetLibrary.save_loaded_asset(pa, False):
        raise RuntimeError("could not save PA_Warden")
    unreal.SystemLibrary.execute_console_command(None, "TD.Ground.PAReport %s" % PATH)
unreal.log("TDPY: OK")
