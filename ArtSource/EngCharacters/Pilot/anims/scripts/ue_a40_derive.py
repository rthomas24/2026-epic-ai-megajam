# Pilot anims step 4: make the light combo right-handed.
#  - the straight retargets of MM_Attack_02 (left hook) and MM_Attack_03 (spinning kick) are kept as
#    A_Pilot_Alt_LeftHook and A_Pilot_Alt_SpinKick;
#  - A_Pilot_Light2 = A_Pilot_Alt_LeftHook mirrored left/right (a right-hand hook), baked in place;
#  - A_Pilot_Light3 = the release of A_Pilot_Heavy (keys 22..55: wound-up right hook), so the combo ends on a big swing.
# Only assets in /Game/TriadDrift/Gameplay/Characters/Pilot/Anims are created, changed or saved.
import json, os
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
WORLD = unreal.AnimPoseSpaces.WORLD
LOCAL = unreal.AnimPoseSpaces.LOCAL
HERE = os.path.dirname(os.path.abspath(__file__))
PARENTS = json.load(open(os.path.join(os.path.dirname(HERE), "pilot_parents.json")))
TRIM_FROM = 22


def log(m):
    unreal.log("TDPY: " + str(m))


def P(n):
    return OUT + "/" + n


def vdist(a, b):
    return ((a.x - b.x) ** 2 + (a.y - b.y) ** 2 + (a.z - b.z) ** 2) ** 0.5


def keep_as(old, new):
    if EAL.does_asset_exist(P(new)):
        log("%s already kept" % new)
        return
    if not EAL.does_asset_exist(P(old)):
        raise RuntimeError("missing " + old)
    EAL.duplicate_asset(P(old), P(new))
    EAL.save_asset(P(new), only_if_is_dirty=False)
    log("kept %s as %s" % (old, new))


def fresh_copy(src, dst):
    if EAL.does_asset_exist(P(dst)):
        EAL.delete_asset(P(dst))
    obj = EAL.duplicate_asset(P(src), P(dst))
    return obj


def set_keys(seq, bone, pos, rot, scl):
    c = seq.controller
    ok = c.set_bone_track_keys(bone, pos, rot, scl, False)
    if not ok:
        c.add_bone_track(bone, False)
        ok = c.set_bone_track_keys(bone, pos, rot, scl, False)
    return ok


def main():
    keep_as("A_Pilot_Light2", "A_Pilot_Alt_LeftHook")
    keep_as("A_Pilot_Light3", "A_Pilot_Alt_SpinKick")
    src = EAL.load_asset(P("A_Pilot_Alt_LeftHook"))
    skel = src.get_editor_property("skeleton")
    ref = APE.get_reference_pose(skel)
    names = [str(n) for n in APE.get_bone_names(ref)]
    log("bones %d" % len(names))
    R = {n: APE.get_bone_pose(ref, n, WORLD) for n in names}
    RL = {n: APE.get_bone_pose(ref, n, LOCAL) for n in names}

    # composition order self-test: world(child) == local(child) (*) world(parent)
    c, p = "lowerarm_r", PARENTS["lowerarm_r"]
    t1 = (RL[c] * R[p]).translation
    t2 = (R[p] * RL[c]).translation
    a_then_b = vdist(t1, R[c].translation) < vdist(t2, R[c].translation)
    log("compose test: A*B means %s (errors %.4f / %.4f)" % ("A then B" if a_then_b else "B then A",
        vdist(t1, R[c].translation), vdist(t2, R[c].translation)))

    def then(a, b):  # apply a, then b
        return a * b if a_then_b else b * a

    # lateral axis check and mirror map (by name, else by mirrored ref position)
    log("ref upperarm_l %s upperarm_r %s" % (R["upperarm_l"].translation, R["upperarm_r"].translation))
    mirror = {}
    worst = 0.0
    for n in names:
        if n.endswith("_l") and n[:-2] + "_r" in R:
            m = n[:-2] + "_r"
        elif n.endswith("_r") and n[:-2] + "_l" in R:
            m = n[:-2] + "_l"
        else:
            t = R[n].translation
            mt = unreal.Vector(-t.x, t.y, t.z)
            m = min(names, key=lambda k: vdist(R[k].translation, mt))
        mirror[n] = m
        t = R[m].translation
        worst = max(worst, vdist(R[n].translation, unreal.Vector(-t.x, t.y, t.z)))
    log("rest symmetry: worst mirrored position error %.2f cm" % worst)
    log("non-name mirror pairs: %s" % {k: v for k, v in mirror.items() if not (k.endswith("_l") or k.endswith("_r"))})

    def mirror_tf(t):
        q = t.rotation
        tr = t.translation
        return unreal.Transform(unreal.Vector(-tr.x, tr.y, tr.z), unreal.Quat(q.x, -q.y, -q.z, q.w).rotator(), unreal.Vector(1, 1, 1))

    # ---- Light2: mirror ----
    dst = fresh_copy("A_Pilot_Alt_LeftHook", "A_Pilot_Light2")
    nkeys = unreal.AnimationLibrary.get_num_frames(src) + 1
    opts = unreal.AnimPoseEvaluationOptions()
    keys = {n: ([], [], []) for n in names}
    max_err = 0.0
    for f in range(nkeys):
        pose = APE.get_anim_pose_at_frame(src, f, opts)
        W = {n: APE.get_bone_pose(pose, n, WORLD) for n in names}
        if f == 0:
            # reconstruction check: R (then) D == W
            d = then(R["hand_r"].inverse(), W["hand_r"])
            max_err = vdist(then(R["hand_r"], d).translation, W["hand_r"].translation)
            log("delta reconstruction error %.4f" % max_err)
        Wm = {}
        for n in names:
            m = mirror[n]
            D = then(R[m].inverse(), W[m])     # world-space delta of the mirror bone
            Wm[n] = then(R[n], mirror_tf(D))
        for n in names:
            par = PARENTS.get(n)
            if par is None or par not in Wm:
                L = Wm[n]
            else:
                L = then(Wm[n], Wm[par].inverse())
            keys[n][0].append(L.translation)
            keys[n][1].append(L.rotation)
            keys[n][2].append(unreal.Vector(1, 1, 1))
    c = dst.controller
    c.open_bracket("mirror", False)
    bad = [n for n in names if not set_keys(dst, n, *keys[n])]
    c.close_bracket(False)
    dst.set_editor_property("enable_root_motion", False)
    log("Light2 mirrored: keys %d, failed tracks %s" % (nkeys, bad))
    # verify: mirrored hand_l world at frame k == mirror of source hand_r world
    for f in (0, nkeys // 2, nkeys - 1):
        a = APE.get_bone_pose(APE.get_anim_pose_at_frame(dst, f, opts), "hand_r", WORLD).translation
        b = APE.get_bone_pose(APE.get_anim_pose_at_frame(src, f, opts), "hand_l", WORLD).translation
        log("check f%d new hand_r %s vs mirrored src hand_l (%.1f %.1f %.1f) err %.2f" % (
            f, a, -b.x, b.y, b.z, vdist(a, unreal.Vector(-b.x, b.y, b.z))))
    log("save Light2 %s" % EAL.save_asset(P("A_Pilot_Light2"), only_if_is_dirty=False))

    # ---- Light3: trimmed Heavy ----
    heavy = EAL.load_asset(P("A_Pilot_Heavy"))
    hk = unreal.AnimationLibrary.get_num_frames(heavy) + 1
    frames = list(range(TRIM_FROM, hk))
    loc = [unreal.AnimationLibrary.get_bone_poses_for_frame(heavy, names, f, False) for f in frames]
    dst3 = fresh_copy("A_Pilot_Heavy", "A_Pilot_Light3")
    c3 = dst3.controller
    c3.open_bracket("trim", False)
    newlen = (hk - 1) - TRIM_FROM
    c3.resize_in_frames(unreal.FrameNumber(newlen), unreal.FrameNumber(0), unreal.FrameNumber(TRIM_FROM), False)
    bad = []
    for i, n in enumerate(names):
        if not set_keys(dst3, n, [p[i].translation for p in loc], [p[i].rotation for p in loc], [p[i].scale3d for p in loc]):
            bad.append(n)
    c3.close_bracket(False)
    dst3.set_editor_property("enable_root_motion", False)
    log("Light3 trimmed: len=%.3f frames=%d keys given %d failed %s" % (
        dst3.get_play_length(), unreal.AnimationLibrary.get_num_frames(dst3), len(frames), bad))
    a = APE.get_bone_pose(APE.get_anim_pose_at_frame(dst3, 0, opts), "hand_r", WORLD).translation
    b = APE.get_bone_pose(APE.get_anim_pose_at_frame(heavy, TRIM_FROM, opts), "hand_r", WORLD).translation
    log("check trim f0 vs heavy f%d hand_r err %.3f" % (TRIM_FROM, vdist(a, b)))
    log("save Light3 %s" % EAL.save_asset(P("A_Pilot_Light3"), only_if_is_dirty=False))
    for n in ("A_Pilot_Light2", "A_Pilot_Light3", "A_Pilot_Alt_LeftHook", "A_Pilot_Alt_SpinKick", "A_Pilot_Heavy"):
        s = EAL.load_asset(P(n))
        log("SEQ %s len=%.3f frames=%d" % (n, s.get_play_length(), unreal.AnimationLibrary.get_num_frames(s)))
    d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    log("dirty packages: %s" % [x.get_name() for x in d])
    log("OK")


main()
