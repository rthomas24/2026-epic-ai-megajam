# SPDX-License-Identifier: MIT
# Triad Drift - bring four raw clips of Fab "Free Animations Pack 2" (SoerGame, Fab Standard) onto the project's UE5
# mannequin skeleton, headless (Tools\td-python.ps1). Before the run, the clips and the pack's SK_Mannequin are copied
# byte for byte from the intake folder into Content/FreeAnimationsPack2/ (staging, never committed). The script:
#   1. consolidates the pack's SK_Mannequin into /Game/Characters/Mannequins/Meshes/SK_Mannequin (the same UE5 skeleton),
#      so the clips reference ours;
#   2. moves and renames the clips to /Game/TriadDrift/Gameplay/Characters/Shared/Anims/Fab/A_Fab_<Clip>;
#   3. sets the preview mesh to the template SKM_Manny_Simple and deletes the staging folder.
# Raw clips only: the animation builder retargets them to the pilot and crew.
import json

import unreal

STAGE = "/Game/FreeAnimationsPack2"
DEST = "/Game/TriadDrift/Gameplay/Characters/Shared/Anims/Fab"
OURS = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
PREVIEW = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
CLIPS = ["Hack", "FightingIdle", "TakeDown", "TakesOutGun"]
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
fails = []


def log(m):
    unreal.log("TDPY: " + m)


ours = eas.load_asset(OURS)
pack = eas.load_asset(STAGE + "/Demo/Mannequins/Meshes/SK_Mannequin")
anims = {c: eas.load_asset("%s/Animations/AS_%s" % (STAGE, c)) for c in CLIPS}
if not ours or not pack or not all(anims.values()):
    log("FAILED load ours=%s pack=%s anims=%s" % (bool(ours), bool(pack), {k: bool(v) for k, v in anims.items()}))
else:
    ok = eas.consolidate_assets(ours, [pack])
    log("consolidate skeleton: %s" % ok)
    if not eas.does_directory_exist(DEST):
        eas.make_directory(DEST)
    tools.rename_assets([unreal.AssetRenameData(a, DEST, "A_Fab_" + c) for c, a in anims.items()])
    rows = []
    for c in CLIPS:
        a = eas.load_asset("%s/A_Fab_%s" % (DEST, c))
        if a is None:
            fails.append(c)
            continue
        try:
            a.set_preview_skeletal_mesh(eas.load_asset(PREVIEW))
        except Exception as e:
            log("preview %s: %s" % (c, e))
        if not eas.save_loaded_asset(a, False):
            fails.append("save " + c)
        sk = a.get_editor_property("skeleton")
        rows.append({"clip": a.get_path_name(), "skeleton": sk.get_path_name() if sk else None,
                     "seconds": round(a.get_editor_property("sequence_length"), 2),
                     "frames": a.get_editor_property("number_of_sampled_frames") if hasattr(a, "number_of_sampled_frames") else None})
        if not sk or sk.get_path_name().split(".")[0] != OURS:
            fails.append("skeleton " + c)
    for r in rows:
        log(json.dumps(r))
    eas.delete_directory(STAGE)
    log("staging folder left: %s" % eas.does_directory_exist(STAGE))
    log("OK" if not fails else "FAILED %s" % fails)
