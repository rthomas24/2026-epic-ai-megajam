# EngAnim batch 2 (Nov 13): clips DERIVED from existing ones by re-timing (trim / stretch) so that the motion matches
# the gameplay timing in code, plus non-additive copies of the pilot hit reacts. Every result is a NEW asset (a copy);
# no existing asset is changed. Job table: JOBS below (dst, src, list of source times in seconds per output key at 30 fps).
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngAnim\Retarget\scripts\ue_r20_derive.py
import json, os
import unreal

EAL = unreal.EditorAssetLibrary
AL = unreal.AnimationLibrary
APE = unreal.AnimPoseExtensions
LOCAL = unreal.AnimPoseSpaces.LOCAL
WORLD = unreal.AnimPoseSpaces.WORLD
P = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/"
C = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/"
FPS = 30.0
problems = []
report = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def span(f0, f1):
    """Source frames f0..f1 one to one."""
    return [f / FPS for f in range(f0, f1 + 1)]


def hold(f, n):
    return [f / FPS] * (n + 1)


def stretch_until(src_len, src_hit, dst_hit):
    """The part before src_hit is stretched so it lands at dst_hit; the rest plays at its own speed."""
    out = []
    n_before = int(round(dst_hit * FPS))
    for k in range(n_before):
        out.append(src_hit * k / n_before)
    t = src_hit
    while t <= src_len + 1e-4:
        out.append(min(t, src_len))
        t += 1.0 / FPS
    return out


# name, source, times, what changed / why
JOBS = [
    (P + "A_Pilot_Light1_v2", P + "A_Pilot_Light1", span(8, 24),
     "source frames 8-24: the wind-up before the swing is cut. Hand at full speed 0.12 s after the press and at full reach at 0.23 s at rate 1.0 (was 0.24 / 0.31 s at rate 1.6, i.e. after the 0.28 s swing was over)."),
    (P + "A_Pilot_Light2_v2", P + "A_Pilot_Light2", span(8, 24), "as Light1_v2 (peak 0.12 s, reach 0.20 s at rate 1.0)."),
    (P + "A_Pilot_Light3_v2", P + "A_Pilot_Light3", span(7, 27), "source frames 7-27: peak 0.12 s, reach 0.23 s inside the 0.36 s swing at rate 1.0."),
    (P + "A_Pilot_Heavy_v2", P + "A_Pilot_Heavy", span(28, 50),
     "source frames 28-50: the release only. Peak 0.15 s, reach 0.27 s after the lunge starts (lunge lasts 0.2 s). The original hits 0.77 s after the lunge starts at rate 1.4."),
    (P + "A_Pilot_Heavy_Windup", P + "A_Pilot_Heavy", span(14, 28),
     "source frames 14-28 (0.467 s): the wind-up alone, for the 0.35 s the attack button is held before the lunge fires (play at 1.33)."),
    (P + "A_Pilot_AirSlam_Fall", P + "A_Pilot_Heavy", hold(26, 15), "the wound-up pose of the heavy held for 0.5 s (a loop): the pose to fall in during an air slam."),
    (P + "A_Pilot_AirSlam_Land", P + "A_Pilot_Heavy", span(28, 55), "source frames 28-55: release on landing with the full recovery (0.9 s)."),
    (P + "A_Pilot_Dash_v2", P + "A_Pilot_Dash", span(5, 12),
     "source frames 5-12 (0.233 s): only the forward dive of the dash clip, so at rate 1.0 it covers the 0.18 s dash and fades out (0.1 s) into the run. The original at rate 1.8 lasts 0.54 s: the leap and landing play after the dash is over."),
    (C + "A_Crew_Grunt_Attack_01", C + "MM_Attack_01", stretch_until(1.0, 0.4667, 0.6),
     "MM_Attack_01 with the wind-up stretched so the fist is at full reach at 0.60 s = the grunt MeleeWindup (was 0.47 s: the punch landed 0.13 s before the damage)."),
    (C + "A_Crew_Heavy_Bash", C + "MM_Attack_01", stretch_until(1.0, 0.4667, 0.7),
     "MM_Attack_01 with the wind-up stretched so the hit is at 0.70 s = the heavy MeleeWindup."),
]
UNADD = [
    (P + "A_Pilot_HitReact_Front_v2", P + "A_Pilot_HitReact_Front"),
    (P + "A_Pilot_HitReact_Back_v2", P + "A_Pilot_HitReact_Back"),
]


def retime(dst, src, times, why):
    if EAL.does_asset_exist(dst):
        log("SKIP exists " + dst); return
    s = EAL.load_asset(src)
    if s is None:
        problems.append("no source " + src); return
    opts = unreal.AnimPoseEvaluationOptions()
    n0 = AL.get_num_frames(s)
    n = len(times) - 1
    pose0 = APE.get_anim_pose_at_time(s, 0.0, opts)
    names = [str(x) for x in APE.get_bone_names(pose0)]
    keys = {b: ([], [], []) for b in names}
    for t in times:
        pose = APE.get_anim_pose_at_time(s, min(max(t, 0.0), s.get_play_length()), opts)
        for b in names:
            tr = APE.get_bone_pose(pose, b, LOCAL)
            keys[b][0].append(tr.translation); keys[b][1].append(tr.rotation); keys[b][2].append(tr.scale3d)
    d = EAL.duplicate_asset(src, dst)
    c = d.controller
    c.open_bracket("retime", False)
    if n < n0:
        c.resize_in_frames(unreal.FrameNumber(n), unreal.FrameNumber(0), unreal.FrameNumber(n0 - n), False)
    elif n > n0:
        c.resize_in_frames(unreal.FrameNumber(n), unreal.FrameNumber(0), unreal.FrameNumber(n - n0), False)
    bad = []
    for b in names:
        ok = c.set_bone_track_keys(b, keys[b][0], keys[b][1], keys[b][2], False)
        if not ok:
            c.add_bone_track(b, False)
            ok = c.set_bone_track_keys(b, keys[b][0], keys[b][1], keys[b][2], False)
        if not ok:
            bad.append(b)
    c.close_bracket(False)
    d.set_editor_property("enable_root_motion", False)
    d.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
    AL.set_bone_compression_settings(d, EAL.load_asset("/ACLPlugin/ACLAnimBoneCompressionSettings"))
    # verify: the new clip at key k equals the source at times[k] (hand_r, component space)
    worst = 0.0
    for k in (0, n // 2, n):
        a = APE.get_bone_pose(APE.get_anim_pose_at_frame(d, k, opts), "hand_r", WORLD).translation
        b = APE.get_bone_pose(APE.get_anim_pose_at_time(s, times[k], opts), "hand_r", WORLD).translation
        worst = max(worst, (a - b).length())
    L = d.get_play_length()
    if AL.get_num_frames(d) != n or abs(L - n / FPS) > 0.002:
        problems.append("%s length %s frames %s, wanted %d" % (dst, L, AL.get_num_frames(d), n))
    if worst > 0.5 or bad:
        problems.append("%s verify error %.2f cm, failed tracks %s" % (dst, worst, bad))
    ok = EAL.save_asset(dst, only_if_is_dirty=False)
    log("NEW %s len=%.3f frames=%d from %s (%.3f..%.3f s) verify %.3f cm saved=%s" % (dst.split("/")[-1], L, AL.get_num_frames(d), src.split("/")[-1], times[0], times[-1], worst, ok))
    report.append(dict(name=dst.split("/")[-1], path=dst, source=src, len=round(L, 4), frames=AL.get_num_frames(d), why=why, status="new"))


def main():
    for dst, src, times, why in JOBS:
        retime(dst, src, times, why)
    for dst, src in UNADD:
        if EAL.does_asset_exist(dst):
            log("SKIP exists " + dst); continue
        d = EAL.duplicate_asset(src, dst)
        d.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
        d.set_editor_property("ref_pose_type", unreal.AdditiveBasePoseType.ABPT_NONE)
        d.set_editor_property("ref_pose_seq", None)
        AL.set_bone_compression_settings(d, EAL.load_asset("/ACLPlugin/ACLAnimBoneCompressionSettings"))
        ok = EAL.save_asset(dst, only_if_is_dirty=False)
        log("NEW %s (full pose copy, additive=%s) saved=%s" % (dst.split("/")[-1], d.get_editor_property("additive_anim_type"), ok))
        report.append(dict(name=dst.split("/")[-1], path=dst, source=src, len=round(d.get_play_length(), 4), frames=AL.get_num_frames(d), status="new",
                           why="full-pose (non-additive) copy: the original is flagged additive, and UTDLoopBlendAnimInstance::PlayOneShot refuses additive sequences, so the pilot flinch never plays."))
    outdir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim")
    os.makedirs(outdir, exist_ok=True)
    rp = os.path.join(outdir, "derive_report.json")
    old = json.load(open(rp)) if os.path.isfile(rp) else []
    json.dump([r for r in old if r["name"] not in set(x["name"] for x in report)] + report, open(rp, "w"), indent=1)
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


main()
