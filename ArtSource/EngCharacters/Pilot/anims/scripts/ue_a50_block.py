# Pilot anims step 5: author A_Pilot_Block, a held guard (1 s loop, 30 fps, in place).
# Base: the fighting stance at key 0 of A_Pilot_Light1 (knees bent, staggered feet). Then:
#  - right arm: upper arm aimed at an elbow out front-right, forearm aimed across the chest so the right forearm is
#    raised across the body at chest height (hand in front of the left chest);
#  - left arm: hand down on the left hip, elbow out;
#  - a little lower (pelvis -3 cm) and a 0.6 cm breathing bob so the loop is not frozen.
# Mesh space of SKEL_Pilot: +X = character's left, +Y = forward, +Z = up (cm).
import json, math, os
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
HERE = os.path.dirname(os.path.abspath(__file__))
PARENTS = json.load(open(os.path.join(os.path.dirname(HERE), "pilot_parents.json")))

# targets (cm, mesh space)
R_ELBOW = unreal.Vector(-28.0, 28.0, 112.0)
R_HAND = unreal.Vector(14.0, 42.0, 128.0)
L_ELBOW = unreal.Vector(34.0, -8.0, 108.0)
L_HAND = unreal.Vector(19.0, 2.0, 94.0)
CROUCH = 3.0
BOB = 0.6
NKEYS = 31


def log(m):
    unreal.log("TDPY: " + str(m))


def then(a, b):  # apply a, then b (verified in ue_a40_derive.py: A*B means A then B)
    return a * b


def V(a, b):
    return unreal.Vector(b.x - a.x, b.y - a.y, b.z - a.z)


def norm(v):
    l = math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
    return unreal.Vector(v.x / l, v.y / l, v.z / l)


def rot_between(a, b):
    a, b = norm(a), norm(b)
    cx = unreal.Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
    d = max(-1.0, min(1.0, a.x * b.x + a.y * b.y + a.z * b.z))
    ang = math.acos(d)
    s = math.sqrt(cx.x ** 2 + cx.y ** 2 + cx.z ** 2)
    if s < 1e-8:
        return unreal.Quat(0, 0, 0, 1), 0.0
    k = math.sin(ang / 2) / s
    return unreal.Quat(cx.x * k, cx.y * k, cx.z * k, math.cos(ang / 2)), math.degrees(ang)


def tf_rot(q):
    return unreal.Transform(unreal.Vector(0, 0, 0), q.rotator(), unreal.Vector(1, 1, 1))


def tf_move(v):
    return unreal.Transform(v, unreal.Rotator(0, 0, 0), unreal.Vector(1, 1, 1))


def main():
    base = EAL.load_asset(OUT + "/A_Pilot_Light1")
    skel = base.get_editor_property("skeleton")
    names = [str(n) for n in APE.get_bone_names(APE.get_reference_pose(skel))]
    loc = unreal.AnimationLibrary.get_bone_poses_for_frame(base, names, 0, False)
    L = {n: loc[i] for i, n in enumerate(names)}

    def worlds():
        W = {}
        for n in names:  # parents come first in skeleton order
            p = PARENTS.get(n)
            W[n] = L[n] if (p is None or p not in W) else then(L[n], W[p])
        return W

    # quaternion rotation self-test (rotation of +X by 90 deg about Z should give +Y)
    q, _ = rot_between(unreal.Vector(1, 0, 0), unreal.Vector(0, 1, 0))
    t = then(tf_move(unreal.Vector(1, 0, 0)), tf_rot(q)).translation
    log("rotation test (expect 0,1,0): %.3f %.3f %.3f" % (t.x, t.y, t.z))
    sign = 1.0 if t.y > 0.9 else -1.0

    def aim(bone, child, target):
        W = worlds()
        piv = W[bone].translation
        cur = V(piv, W[child].translation)
        q, deg = rot_between(cur, V(piv, target))
        if sign < 0:
            q = unreal.Quat(-q.x, -q.y, -q.z, q.w)
        Wn = then(then(then(W[bone], tf_move(unreal.Vector(-piv.x, -piv.y, -piv.z))), tf_rot(q)), tf_move(piv))
        p = PARENTS[bone]
        L[bone] = then(Wn, W[p].inverse())
        log("aim %s -> %.1f deg" % (bone, deg))

    W0 = worlds()
    log("base pelvis %s hand_r %s hand_l %s" % (W0["pelvis"].translation, W0["hand_r"].translation, W0["hand_l"].translation))
    for _ in range(2):  # second pass removes the small error from the elbow moving
        aim("upperarm_r", "lowerarm_r", R_ELBOW)
        aim("lowerarm_r", "hand_r", R_HAND)
        aim("upperarm_l", "lowerarm_l", L_ELBOW)
        aim("lowerarm_l", "hand_l", L_HAND)
    # hands: keep the forearm-relative orientation they had; straighten the right wrist a little toward the forearm
    W1 = worlds()
    log("result elbow_r %s hand_r %s elbow_l %s hand_l %s" % (W1["lowerarm_r"].translation, W1["hand_r"].translation,
        W1["lowerarm_l"].translation, W1["hand_l"].translation))

    pel = L["pelvis"]
    dst_path = OUT + "/A_Pilot_Block"
    if EAL.does_asset_exist(dst_path):
        EAL.delete_asset(dst_path)
    dst = EAL.duplicate_asset(OUT + "/A_Pilot_Light1", dst_path)
    c = dst.controller
    c.open_bracket("block", False)
    try:
        c.remove_all_curves_of_type(unreal.RawCurveTrackTypes.RCT_FLOAT, False)
    except Exception as e:
        log("note curves: %s" % e)
    n_keys = unreal.AnimationLibrary.get_num_frames(dst) + 1
    if n_keys != NKEYS:
        log("note: base has %d keys" % n_keys)
    bad = []
    for n in names:
        tr, rot, sc = [], [], []
        for k in range(n_keys):
            t = L[n].translation
            if n == "pelvis":
                ph = 2 * math.pi * k / (n_keys - 1)
                t = unreal.Vector(t.x, t.y, t.z - CROUCH - BOB * 0.5 * (1 - math.cos(ph)))
            tr.append(t); rot.append(L[n].rotation); sc.append(L[n].scale3d)
        if not c.set_bone_track_keys(n, tr, rot, sc, False):
            c.add_bone_track(n, False)
            if not c.set_bone_track_keys(n, tr, rot, sc, False):
                bad.append(n)
    c.close_bracket(False)
    dst.set_editor_property("enable_root_motion", False)
    try:
        dst.set_editor_property("loop", True)
    except Exception:
        pass
    log("Block: len=%.3f frames=%d failed %s" % (dst.get_play_length(), unreal.AnimationLibrary.get_num_frames(dst), bad))
    log("save %s" % EAL.save_asset(dst_path, only_if_is_dirty=False))
    d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    log("dirty packages: %s" % [x.get_name() for x in d])
    log("OK")


main()
