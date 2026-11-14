# Triad Drift - author body moves on top of a base clip, in engine (no FBX round trip), for the crew skeleton.
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotFixes\scripts\ue_crew_author.py     (TD_AUTHOR = config json)
# Per move: a base clip (legs, idle motion, length), and keys in seconds for
#   yaw    chest facing in degrees (absolute; + = left shoulder forward)
#   pitch  upper body lean in degrees (+ = forward), about spine_01; the head counters half of it
#   pelvis [forward, up] in leg lengths; the feet stay planted (leg IK) unless a foot key moves them
#   feet   {"l" | "r": [forward, right, up]} in leg lengths: where that foot goes from its base position (a step)
#   arms   {"l" | "r": {"hand": [forward, right, up] from the shoulder in arm lengths, chest frame,
#                        "pole": elbow direction (optional), "face": direction for the arm's "axis_rest" (optional)}}
# "axis_rest" (per move, per arm) is a mesh-space direction in the REFERENCE pose that is rigid on the forearm, e.g.
# the normal of the Heavy's forearm shield; the forearm rolls so it points along "face".
# Mesh space: forward +Y, the character's right -X, up +Z. In place. Animation builder (eng/anim), Nov 13.
import json
import math
import os
import unreal

CFG = json.load(open(os.environ["TD_AUTHOR"]))
ANIMS = CFG.get("folder", "/Game/TriadDrift/Gameplay/Characters/Crew/Anims")
SKEL = unreal.load_asset(CFG.get("skeleton", "/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew"))
X = unreal.AnimPoseExtensions
OPTS = unreal.AnimPoseEvaluationOptions()
LOCAL, WORLD = unreal.AnimPoseSpaces.LOCAL, unreal.AnimPoseSpaces.WORLD
FPS = 30
REF = X.get_reference_pose(SKEL)
ALL = [str(n) for n in X.get_bone_names(REF)]
LEG = ("thigh", "calf", "foot", "ball")


def log(m): unreal.log("TDPY: " + str(m))
def tq(q): return (q.x, q.y, q.z, q.w)
def tv(v): return (v.x, v.y, v.z)
def add(a, b): return tuple(i + j for i, j in zip(a, b))
def sub(a, b): return tuple(i - j for i, j in zip(a, b))
def mul(a, s): return tuple(i * s for i in a)
def dot(a, b): return sum(i * j for i, j in zip(a, b))
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def length(a): return math.sqrt(dot(a, a))
def qinv(q): return (-q[0], -q[1], -q[2], q[3])


def norm(a):
    n = length(a)
    return mul(a, 1.0 / n) if n > 1e-9 else a


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def qaxis(axis, deg):
    a = norm(axis)
    h = math.radians(deg) * 0.5
    s = math.sin(h)
    return (a[0] * s, a[1] * s, a[2] * s, math.cos(h))


def rot(q, v):
    x, y, z, w = q
    t = (2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0]))
    return (v[0] + w * t[0] + y * t[2] - z * t[1], v[1] + w * t[1] + z * t[0] - x * t[2], v[2] + w * t[2] + x * t[1] - y * t[0])


def from_to(a, b):
    a, b = norm(a), norm(b)
    c = cross(a, b)
    s = length(c)
    if s < 1e-7:
        return (0.0, 0.0, 0.0, 1.0)
    return qaxis(c, math.degrees(math.atan2(s, dot(a, b))))


def find(name):
    for folder in (ANIMS,):
        p = "%s/%s" % (folder, name)
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return unreal.load_asset(p)
    raise RuntimeError("no clip " + name)


def spline(keys, t):
    """keys [(time, tuple)] sorted; Catmull-Rom, clamped at the ends."""
    ts = [k[0] for k in keys]
    if t <= ts[0]:
        return keys[0][1]
    if t >= ts[-1]:
        return keys[-1][1]
    i = max(j for j in range(len(ts) - 1) if ts[j] <= t)
    p0, p1, p2, p3 = keys[max(0, i - 1)][1], keys[i][1], keys[i + 1][1], keys[min(len(keys) - 1, i + 2)][1]
    u = (t - ts[i]) / (ts[i + 1] - ts[i])
    return tuple(0.5 * ((2 * b) + (-a + c) * u + (2 * a - 5 * b + 4 * c - d) * u * u + (-a + 3 * b - 3 * c + d) * u ** 3)
                 for a, b, c, d in zip(p0, p1, p2, p3))


def two_bone(S, T, L1, L2, pole):
    d = sub(T, S)
    dl = max(1e-3, min(length(d), 0.985 * (L1 + L2)))
    dn = norm(d)
    T = add(S, mul(dn, dl))
    a = (L1 * L1 + dl * dl - L2 * L2) / (2 * dl)
    h = math.sqrt(max(0.0, L1 * L1 - a * a))
    perp = norm(sub(pole, mul(dn, dot(pole, dn))))
    return add(S, add(mul(dn, a), mul(perp, h))), T


def write(dst, frames):
    path = "%s/%s" % (ANIMS, dst)
    seq = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not seq:
        f = unreal.AnimSequenceFactory()
        f.set_editor_property("target_skeleton", SKEL)
        seq = unreal.AssetToolsHelpers.get_asset_tools().create_asset(dst, ANIMS, unreal.AnimSequence, f)
    c = seq.get_editor_property("controller")
    c.open_bracket(unreal.Text("Crew author"), False)
    c.set_frame_rate(unreal.FrameRate(FPS, 1), False)
    c.set_number_of_frames(unreal.FrameNumber(len(frames) - 1), False)
    one = unreal.Vector(1, 1, 1)
    for b in ALL:
        try:
            c.add_bone_curve(b, False)
        except Exception:
            pass
        c.set_bone_track_keys(b, [unreal.Vector(*fr[b][0]) for fr in frames], [unreal.Quat(*fr[b][1]) for fr in frames], [one] * len(frames), False)
    c.close_bracket(False)
    seq.set_editor_property("enable_root_motion", False)
    if not unreal.EditorAssetLibrary.save_loaded_asset(seq, False):
        raise RuntimeError("could not save %s (is a running game holding the file?)" % dst)
    log("wrote %s: %d frames (%.2f s)" % (dst, len(frames), (len(frames) - 1) / float(FPS)))


for mv in CFG["moves"]:
    src = find(mv["src"])
    L = mv.get("length", src.get_play_length())
    n = int(round(L * FPS)) + 1
    keys = mv["keys"]
    for k in keys:
        if k["t"] == "end":
            k["t"] = L
    chan = lambda name, default: [(k["t"], tuple(k.get(name, default))) for k in keys]
    yaw_k = [(k["t"], (k.get("yaw", 0.0),)) for k in keys]
    pitch_k = [(k["t"], (k.get("pitch", 0.0),)) for k in keys]
    pel_k = chan("pelvis", [0.0, 0.0])
    arms = sorted({s for k in keys for s in k.get("arms", {})})
    feet = sorted({s for k in keys for s in k.get("feet", {})})
    axis_local = {}
    for s, n_rest in mv.get("axis_rest", {}).items():
        lw = tq(X.get_ref_bone_pose(REF, "lowerarm_" + s, WORLD).rotation)
        axis_local[s] = rot(qinv(lw), norm(tuple(n_rest)))
    frames, notes = [], []
    base_len = src.get_play_length()
    for i in range(n):
        t = i / float(FPS)
        pose = X.get_anim_pose_at_time(src, (t % base_len) if mv.get("base_loops", True) else min(t, base_len), OPTS)
        fr, W = {}, {}
        for b in ALL:
            lt = X.get_bone_pose(pose, b, LOCAL)
            wt = X.get_bone_pose(pose, b, WORLD)
            fr[b] = [tv(lt.translation), tq(lt.rotation)]
            W[b] = [tv(wt.translation), tq(wt.rotation)]
        root_xy = (W["root"][0][0], W["root"][0][1], 0.0)   # in place
        for b in ALL:
            W[b][0] = sub(W[b][0], root_xy)
        fr["root"][0] = (0.0, 0.0, fr["root"][0][2])
        upper = [b for b in ALL if b not in ("root", "pelvis") and not b.startswith(LEG) and not b.startswith("ik_")]
        legl = (length(sub(W["calf_r"][0], W["thigh_r"][0])) + length(sub(W["foot_r"][0], W["calf_r"][0])))
        foot0 = {s: W["foot_" + s][0] for s in "lr"}
        knee0 = {s: W["calf_" + s][0] for s in "lr"}
        # ---- pelvis shift (everything but the root and the planted feet follows)
        pf, pu = spline(pel_k, t)
        shift = (0.0, pf * legl, pu * legl)
        for b in ALL:
            if b != "root" and not b.startswith("ik_"):
                W[b][0] = add(W[b][0], shift)
        # ---- chest yaw (absolute) and pitch about spine_01
        right = norm(sub(W["upperarm_r"][0], W["upperarm_l"][0]))
        cur = math.degrees(math.atan2(-right[1], -right[0]))
        yaw = spline(yaw_k, t)[0]
        pitch = spline(pitch_k, t)[0]
        yr = math.radians(yaw)
        R = (-math.cos(yr), -math.sin(yr), 0.0)
        F = (-math.sin(yr), math.cos(yr), 0.0)
        U = (0.0, 0.0, 1.0)
        if not mv.get("chest", True):   # a lying body: leave the chest as the clip has it
            yaw, pitch = cur, 0.0
        q = qmul(qaxis(R, pitch), qaxis(U, yaw - cur))   # lean forward: up turns toward forward, about the right axis
        if dot(rot(qaxis(R, 10.0), U), F) < 0:
            q = qmul(qaxis(R, -pitch), qaxis(U, yaw - cur))
        piv = W["spine_01"][0]
        for b in upper:
            W[b][0] = add(piv, rot(q, sub(W[b][0], piv)))
            W[b][1] = qmul(q, W[b][1])
        Fp, Up = rot(qaxis(R, pitch if dot(rot(qaxis(R, 10.0), U), F) > 0 else -pitch), F), None
        # the head counters half of the lean (keeps looking at the target)
        hq = qaxis(R, (-0.5 * pitch) if dot(rot(qaxis(R, 10.0), U), F) > 0 else (0.5 * pitch))
        hp = W["neck_01"][0]
        for b in upper:
            if b.startswith(("neck", "head")):
                W[b][0] = add(hp, rot(hq, sub(W[b][0], hp)))
                W[b][1] = qmul(hq, W[b][1])
        # ---- arms: two-bone IK to keyed hand targets in the chest frame (yawed, not pitched)
        for s in arms:
            ak = [(k["t"], tuple(k["arms"][s]["hand"])) for k in keys if s in k.get("arms", {})]
            up, lo, ha = "upperarm_" + s, "lowerarm_" + s, "hand_" + s
            S, E0, H0 = W[up][0], W[lo][0], W[ha][0]
            L1, L2 = length(sub(E0, S)), length(sub(H0, E0))
            h = spline(ak, t)
            T = add(S, mul(add(add(mul(F, h[0]), mul(R, h[1])), mul(U, h[2])), L1 + L2))
            pk = [(k["t"], tuple(k["arms"][s].get("pole", [-0.1, 0.55 if s == "r" else -0.55, -0.8]))) for k in keys if s in k.get("arms", {})]
            p = spline(pk, t)
            E, T = two_bone(S, T, L1, L2, add(add(mul(F, p[0]), mul(R, p[1])), mul(U, p[2])))
            qu = from_to(sub(E0, S), sub(E, S))
            sub_tree = [b for b in upper if b.endswith("_" + s) and not b.startswith("clavicle")]
            for b in sub_tree:
                W[b][0] = add(S, rot(qu, sub(W[b][0], S)))
                W[b][1] = qmul(qu, W[b][1])
            ql = from_to(sub(W[ha][0], W[lo][0]), sub(T, E))
            for b in sub_tree:
                if b != up and "upperarm" not in b:
                    W[b][0] = add(E, rot(ql, sub(W[b][0], E)))
                    W[b][1] = qmul(ql, W[b][1])
            if s in axis_local and any("face" in k.get("arms", {}).get(s, {}) for k in keys):
                fk = [(k["t"], tuple(k["arms"][s]["face"])) for k in keys if "face" in k.get("arms", {}).get(s, {})]
                fd = spline(fk, t)
                want = add(add(mul(F, fd[0]), mul(R, fd[1])), mul(U, fd[2]))
                a = norm(sub(T, E))
                cur_n = rot(W[lo][1], axis_local[s])
                cn = norm(sub(cur_n, mul(a, dot(cur_n, a))))
                wn = norm(sub(want, mul(a, dot(want, a))))
                ang = math.degrees(math.atan2(dot(cross(cn, wn), a), dot(cn, wn)))
                qr = qaxis(a, ang)
                for b in sub_tree:
                    if b != up and "upperarm" not in b:
                        W[b][0] = add(E, rot(qr, sub(W[b][0], E)))
                        W[b][1] = qmul(qr, W[b][1])
                if i % 6 == 0:
                    notes.append("t%.2f face.fwd %.2f" % (t, dot(rot(W[lo][1], axis_local[s]), F)))
        # ---- legs: feet planted (or stepped by keys), knees keep their side
        for s in "lr":
            th, ca, fo = "thigh_" + s, "calf_" + s, "foot_" + s
            Hp = W[th][0]
            L1, L2 = length(sub(W[ca][0], W[th][0])), length(sub(W[fo][0], W[ca][0]))
            tgt = foot0[s]
            if s in feet:
                fk = [(k["t"], tuple(k.get("feet", {}).get(s, [0.0, 0.0, 0.0]))) for k in keys]
                o = spline(fk, t)
                tgt = add(tgt, (-o[1] * legl, o[0] * legl, o[2] * legl))
            pole = sub(add(knee0[s], shift), Hp)
            K, Tg = two_bone(Hp, tgt, L1, L2, add(pole, (0.0, 0.3 * legl, 0.0)))
            qt = from_to(sub(W[ca][0], Hp), sub(K, Hp))
            leg_tree = [b for b in ALL if b.endswith("_" + s) and b.startswith(LEG)]
            for b in leg_tree:
                W[b][0] = add(Hp, rot(qt, sub(W[b][0], Hp)))
                W[b][1] = qmul(qt, W[b][1])
            qc = from_to(sub(W[fo][0], W[ca][0]), sub(Tg, K))
            foot_rot = W[fo][1]
            for b in leg_tree:
                if not b.startswith("thigh"):
                    W[b][0] = add(K, rot(qc, sub(W[b][0], K)))
                    W[b][1] = qmul(qc, W[b][1])
            # the foot keeps the base clip's own orientation (flat on the ground)
            base_foot = tq(X.get_bone_pose(pose, fo, WORLD).rotation)
            d = qmul(base_foot, qinv(W[fo][1]))
            for b in leg_tree:
                if b.startswith(("foot", "ball")):
                    W[b][0] = add(W[fo][0], rot(d, sub(W[b][0], W[fo][0])))
                    W[b][1] = qmul(d, W[b][1])
        # ---- back to local transforms (rotations from the world rotations; translations: only the pelvis moved)
        par = {"spine_01": "pelvis", "neck_01": "spine_05"}
        for sd in "lr":
            par.update({"upperarm_" + sd: "clavicle_" + sd, "lowerarm_" + sd: "upperarm_" + sd, "hand_" + sd: "lowerarm_" + sd,
                        "thigh_" + sd: "pelvis", "calf_" + sd: "thigh_" + sd, "foot_" + sd: "calf_" + sd})
        out = {}
        for b in ALL:
            p_ = par.get(b)
            out[b] = (tuple(fr[b][0]), qmul(qinv(W[p_][1]), W[b][1]) if p_ else tuple(fr[b][1]))
        out["pelvis"] = (add(fr["pelvis"][0], shift), out["pelvis"][1])
        frames.append(out)
        if mv.get("probe") and i in (0, n // 2, n - 1):
            log("%s probe t%.2f: head %s hand_l %s hand_r %s lowerarm_r %s foot_r %s" % (mv["dst"], t, *[tuple(round(c) for c in W[b][0]) for b in ("head", "hand_l", "hand_r", "lowerarm_r", "foot_r")]) + " | pelvis %s calf_l %s foot_l %s calf_r %s" % tuple(tuple(round(c) for c in W[b][0]) for b in ("pelvis", "calf_l", "foot_l", "calf_r")))
    log("%s -> %s%s" % (mv["src"], mv["dst"], (": " + " | ".join(notes)) if notes else ""))
    write(mv["dst"], frames)
log("OK")
