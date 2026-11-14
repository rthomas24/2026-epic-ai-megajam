# Triad Drift - retarget the Fab mannequin clips (Content/TriadDrift/Gameplay/Characters/Shared/Anims/Fab, Fab Standard
# licence) onto the pilot and the crew, as new assets only, root motion removed, and probe them:
#   A_Fab_Hack -> A_Pilot_Fab_Hack (pilot); A_Fab_FightingIdle -> A_Crew_Grunt_FightingIdle_Fab;
#   A_Fab_TakesOutGun -> A_Crew_Trooper_TakesOutGun_Fab. Logs both hands' paths for trimming.
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\Pilot\scripts\ue_fab_retarget.py
# Animation builder (eng/anim), Nov 14.
import unreal

EAL = unreal.EditorAssetLibrary
AL = unreal.AnimationLibrary
X = unreal.AnimPoseExtensions
CH = "/Game/TriadDrift/Gameplay/Characters"
FAB = CH + "/Shared/Anims/Fab/"
JOBS = [("A_Fab_Hack", "A_Pilot_Fab_Hack", CH + "/Pilot/Anims", CH + "/Pilot/SK_Pilot", CH + "/Pilot/Anims/RTG_Manny_to_Pilot"),
        ("A_Fab_FightingIdle", "A_Crew_Grunt_FightingIdle_Fab", CH + "/Crew/Anims", CH + "/NullTrooper/SK_NullTrooper", CH + "/Crew/RTG_Manny_to_NullCrew"),
        ("A_Fab_TakesOutGun", "A_Crew_Trooper_TakesOutGun_Fab", CH + "/Crew/Anims", CH + "/NullTrooper/SK_NullTrooper", CH + "/Crew/RTG_Manny_to_NullCrew")]


def log(m): unreal.log("TDPY: " + str(m))


src_mesh = None
for p in ("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple", "/Game/Characters/Mannequins/Meshes/SKM_Manny"):
    if EAL.does_asset_exist(p):
        src_mesh = EAL.load_asset(p); break
for src, dst, out, mesh, rtg in JOBS:
    seq0 = EAL.load_asset(FAB + src)
    log("%s: %.2f s on %s" % (src, seq0.get_play_length(), seq0.get_editor_property("skeleton").get_name()))
    path = out + "/" + dst
    if not EAL.does_asset_exist(path):
        unreal.IKRetargetBatchOperation.duplicate_and_retarget([EAL.find_asset_data(FAB + src)], src_mesh, EAL.load_asset(mesh), EAL.load_asset(rtg),
                                                              src, dst, "", "", out, False, False, True)
    seq = EAL.load_asset(path)
    if not seq:
        log("FAILED %s" % dst); continue
    nk = AL.get_num_frames(seq) + 1
    poses = [AL.get_bone_poses_for_frame(seq, ["root"], f, False)[0] for f in range(nk)]
    seq.controller.set_bone_track_keys("root", [unreal.Vector(0, 0, 0)] * nk, [p.rotation for p in poses], [p.scale3d for p in poses], True)
    seq.set_editor_property("enable_root_motion", False)
    EAL.save_asset(path, only_if_is_dirty=False)
    L = seq.get_play_length()
    o = unreal.AnimPoseEvaluationOptions()
    for i in range(int(L * 10) + 1):
        t = min(L, i / 10.0)
        p = X.get_anim_pose_at_time(seq, t, o)
        g = lambda b: X.get_bone_pose(p, b, unreal.AnimPoseSpaces.WORLD).translation
        hl, hr, pe = g("hand_l"), g("hand_r"), g("pelvis")
        log("%s t%.1f hand_l %.0f %.0f %.0f hand_r %.0f %.0f %.0f pelvis %.0f %.0f %.0f" % (dst, t, hl.x, hl.y, hl.z, hr.x, hr.y, hr.z, pe.x, pe.y, pe.z))
log("OK")
