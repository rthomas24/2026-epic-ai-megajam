# PilotBlade step 2: put the accepted Grok Bot clips where the pilot's C++ reads them. ATDFirstPersonCharacter loads its
# action clips BY NAME from /Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_<Move> at fixed rates (no data-asset
# slot), so "wiring" a replacement means: keep the old clip as <name>_v1 (a copy, never deleted), then put a re-timed
# copy of the new clip at the name the code reads. Re-timing only trims frames so the hit lands inside the code's window
# at the code's rate; no C++ value is touched. Also makes A_Pilot_Blade_HeavyWindup (new, not wired: no code slot).
# Idempotent: a job whose _v1 copy exists is taken as done unless PB_REDO=1 (then the name is rebuilt from the Blade clip;
# the _v1 copy is never rewritten).
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotBlade\scripts\ue_pb20_wire.py
import json, os
import unreal

EAL = unreal.EditorAssetLibrary
AL = unreal.AnimationLibrary
APE = unreal.AnimPoseExtensions
LOCAL = unreal.AnimPoseSpaces.LOCAL; WORLD = unreal.AnimPoseSpaces.WORLD
P = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/"
FPS = 30.0
REDO = os.environ.get("PB_REDO") == "1"
problems = []; report = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


# (name the code reads or new name, Blade source, first frame, last frame, keep old as _v1, note)
JOBS = [
    ("A_Pilot_Light1", "A_Pilot_Blade_Light1", 2, 35, True, "code rate 1.6, lockout 0.28 s: hand peak at frame 5 = 0.10 s real"),
    ("A_Pilot_Light2", "A_Pilot_Blade_Light2", 4, 35, True, "code rate 1.6, lockout 0.28 s: hand peak at frame 6 = 0.13 s real"),
    ("A_Pilot_Light3", "A_Pilot_Blade_Light3", 3, 35, True, "code rate 1.5, lockout 0.36 s: hand peak at frame 6 = 0.13 s real. Pelvis and legs from A_Pilot_Blade_Light1 "
     "(same frame numbers): the Grok chop drops the head to 98 cm and the close-combat camera loses the pilot (in-game capture r1)"),
    ("A_Pilot_Heavy", "A_Pilot_Blade_HeavyLunge", 17, 59, True, "code rate 1.4 from the START of the 0.2 s lunge: thrust at frame 6 = 0.14 s real; the wind-up is cut (see HeavyWindup)"),
    ("A_Pilot_Blade_HeavyWindup", "A_Pilot_Blade_HeavyLunge", 0, 17, False, "0.57 s wind-up for the 0.35 s attack hold (play at 1.6): NOT wired, no code slot"),
    ("A_Pilot_Dash", "A_Pilot_Blade_Dash", 2, 20, True, "code rate 1.8 (dash 0.18 s) / 0.8 (slide 0.7 s): 0.6 s of the low sprint start, 0.33 s real at 1.8"),
    # A_Pilot_Block: NOT replaced. In game the old authored guard (blade upright in front of the body) reads better from the
    # chase camera than the Grok high guard (blade level behind the helmet). A_Pilot_Blade_BlockHold stays as an unwired alternative.
    ("A_Pilot_HitReact_Front", "A_Pilot_Blade_HitFront", 0, 35, True, "full pose (the old clip is additive and PlayOneShot refuses it: never played); code rate 1.2 upper body, and 1.0 for GuardBroken"),
    ("A_Pilot_HitReact_Back", "A_Pilot_Blade_HitBack", 0, 35, True, "as HitReact_Front"),
]


LOWER_FROM = {"A_Pilot_Light3": "A_Pilot_Blade_Light1"}
LOWER = ("root", "pelvis", "thigh_l", "calf_l", "foot_l", "ball_l", "ball_l_end", "thigh_r", "calf_r", "foot_r", "ball_r", "ball_r_end")
# The Grok hit reacts throw both arms wide; the code plays one on EVERY hull hit (a 3-bolt burst = 3 flinches), so the
# wired copies are the motion at 55 % (in-game capture r1: the full one reads as flailing). Full versions: A_Pilot_Blade_Hit*.
AMP = {"A_Pilot_HitReact_Front": 0.55, "A_Pilot_HitReact_Back": 0.55}
ONLY = set(x for x in os.environ.get("PB_ONLY", "").split(",") if x)


def build(dst, src, f0, f1):
    s = EAL.load_asset(src)
    low = EAL.load_asset(P + LOWER_FROM[dst.split("/")[-1]]) if dst.split("/")[-1] in LOWER_FROM else None
    opts = unreal.AnimPoseEvaluationOptions()
    n0 = AL.get_num_frames(s)
    f1 = min(f1, n0)
    n = f1 - f0
    names = [str(x) for x in APE.get_bone_names(APE.get_anim_pose_at_frame(s, 0, opts))]
    keys = {b: ([], [], []) for b in names}
    amp = AMP.get(dst.split("/")[-1], 1.0)
    pose0 = APE.get_anim_pose_at_frame(s, f0, opts)
    for f in range(f0, f1 + 1):
        pose = APE.get_anim_pose_at_frame(s, f, opts)
        lpose = APE.get_anim_pose_at_frame(low, min(f, AL.get_num_frames(low)), opts) if low else None
        for b in names:
            tr = APE.get_bone_pose(lpose if (lpose and b in LOWER) else pose, b, LOCAL)
            if amp < 1.0:
                # a smaller version of the motion: every bone part-way from the clip's first pose to this pose
                t0 = APE.get_bone_pose(pose0, b, LOCAL)
                q0, q1 = t0.rotation, tr.rotation
                sgn = 1.0 if (q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w) >= 0 else -1.0
                q = [(1 - amp) * a_ + amp * sgn * b_ for a_, b_ in ((q0.x, q1.x), (q0.y, q1.y), (q0.z, q1.z), (q0.w, q1.w))]
                ln = sum(v * v for v in q) ** 0.5
                tr = unreal.Transform()
                tr.rotation = unreal.Quat(q[0] / ln, q[1] / ln, q[2] / ln, q[3] / ln)
                p0, p1 = t0.translation, APE.get_bone_pose(pose, b, LOCAL).translation
                tr.translation = unreal.Vector(p0.x + (p1.x - p0.x) * amp, p0.y + (p1.y - p0.y) * amp, p0.z + (p1.z - p0.z) * amp)
                tr.scale3d = t0.scale3d
            keys[b][0].append(tr.translation); keys[b][1].append(tr.rotation); keys[b][2].append(tr.scale3d)
    d = EAL.duplicate_asset(src, dst)
    if d is None:
        problems.append("duplicate failed " + dst); return None
    c = d.controller
    c.open_bracket("trim", False)
    if n < n0:
        c.resize_in_frames(unreal.FrameNumber(n), unreal.FrameNumber(0), unreal.FrameNumber(n0 - n), False)
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
    worst = 0.0
    for k in (0, n // 2, n):
        a = APE.get_bone_pose(APE.get_anim_pose_at_frame(d, k, opts), "hand_r", WORLD).translation
        b = APE.get_bone_pose(APE.get_anim_pose_at_frame(s, f0 + k, opts), "hand_r", WORLD).translation
        worst = max(worst, (a - b).length())
    if (worst > 0.5 and not low and amp >= 1.0) or bad or AL.get_num_frames(d) != n:
        problems.append("%s verify %.2f cm, bad tracks %s, frames %d wanted %d" % (dst, worst, bad, AL.get_num_frames(d), n))
    # right-hand speed peak in the new clip
    P_ = [APE.get_bone_pose(APE.get_anim_pose_at_frame(d, f, opts), "hand_r", WORLD).translation for f in range(n + 1)]
    sp = [(P_[f] - P_[f - 1]).length() * FPS for f in range(1, n + 1)]
    k = max(range(len(sp)), key=lambda i: sp[i]) + 1
    EAL.save_asset(dst, only_if_is_dirty=False)
    return dict(len=round(d.get_play_length(), 4), frames=n, hand_peak_frame=k, hand_peak_s=round(k / FPS, 3))


def main():
    for name, src, f0, f1, keep, note in JOBS:
        dst = P + name; srcp = P + src; v1 = dst + "_v1"
        if ONLY and name not in ONLY:
            continue
        if not EAL.does_asset_exist(srcp):
            problems.append("no source " + srcp); continue
        if keep:
            if EAL.does_asset_exist(v1) and not REDO:
                log("SKIP done " + name); continue
            if not EAL.does_asset_exist(v1):
                if not EAL.does_asset_exist(dst):
                    problems.append("nothing to keep at " + dst); continue
                old = EAL.load_asset(dst)
                o = EAL.duplicate_asset(dst, v1)
                if o is None or not EAL.save_asset(v1, only_if_is_dirty=False):
                    problems.append("could not keep " + v1); continue
                log("KEPT %s as %s (len %.3f additive %s)" % (name, name + "_v1", old.get_play_length(), old.get_editor_property("additive_anim_type")))
            refs = [str(r) for r in EAL.find_package_referencers_for_asset(dst, False)]
            if refs:
                log("referencers of %s: %s" % (name, refs))
            if EAL.does_asset_exist(dst) and not EAL.delete_asset(dst):
                problems.append("could not replace " + dst); continue
        else:
            if EAL.does_asset_exist(dst):
                if not REDO:
                    log("SKIP exists " + name); continue
                EAL.delete_asset(dst)
        r = build(dst, srcp, f0, f1)
        if r:
            r.update(name=name, path=dst, source=src, first_frame=f0, last_frame=f1, kept_old_as=(name + "_v1") if keep else None, note=note)
            report.append(r)
            log("NEW %s" % json.dumps(r))
    outdir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim")
    json.dump(report, open(os.path.join(outdir, "pilotblade_wire_report.json"), "w"), indent=1)
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


main()
