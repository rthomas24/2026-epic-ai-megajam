# Triad Drift - pilot clip fixes and new clips, written as COPIES (originals kept). Run headless:
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotFixes\scripts\ue_pilot_fixes.py
# Reads ArtSource/EngAnim/PilotFixes/fixes.json:
#   "wrist":   [{"src", "dst", "bone", "axis", "deg"}]   turn a bone about an axis of its PARENT (every key), e.g. hand_r
#              about the forearm (lowerarm_r's +X): a forearm twist that leaves the hand where it is
#   "drop":    [{"src", "dst", "floor_cm", "bones"}]     lower the pelvis so the lying end pose rests on the floor
#                                                         (lowest of the given bones at floor_cm), ramped in as the body falls
#   "compose": [{"dst", "segments": [[clip, t0, t1], ...], "blend": s}]  join clip ranges with cross-fades, in place
# Every new clip is evaluated from the source's own data (AnimPose), so it works on retargeted clips.
# Animation builder (eng/anim), Nov 13.
import json
import math
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
CFG = json.load(open(os.environ.get("TD_FIXES") or os.path.join(HERE, "..", "fixes.json")))
ANIMS = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
PILOT = "/Game/TriadDrift/Gameplay/Characters/Pilot"
SKEL = unreal.load_asset(PILOT + "/SKEL_Pilot")
OPTS = unreal.AnimPoseEvaluationOptions()
LOCAL = unreal.AnimPoseSpaces.LOCAL
WORLD = unreal.AnimPoseSpaces.WORLD
FPS = 30
if CFG.get("folder"):  # crew / Warden work: clips live in another folder, on another skeleton
    ANIMS = PILOT = CFG["folder"]
    SKEL = unreal.load_asset(CFG.get("skeleton", "/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew"))


def log(m):
    unreal.log("TDPY: " + str(m))


def find(name):
    for folder in (ANIMS, PILOT):
        p = "%s/%s" % (folder, name)
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return unreal.load_asset(p)
    raise RuntimeError("no clip " + name)


def bones():
    ref = unreal.AnimPoseExtensions.get_reference_pose(SKEL)
    return [str(n) for n in unreal.AnimPoseExtensions.get_bone_names(ref)]


ALL = bones()


def sample(seq, t):
    """Local transforms of every bone at time t: {bone: (loc Vector, rot Quat)}."""
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
    out = {}
    for b in ALL:
        tr = unreal.AnimPoseExtensions.get_bone_pose(pose, b, LOCAL)
        out[b] = (tr.translation, tr.rotation)
    return out, pose


def qmul(a, b):
    return unreal.Quat(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                       a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                       a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                       a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z)


def qaxis(axis, deg):
    n = math.sqrt(sum(c * c for c in axis))
    h = math.radians(deg) * 0.5
    s = math.sin(h) / n
    return unreal.Quat(axis[0] * s, axis[1] * s, axis[2] * s, math.cos(h))


def nlerp(a, b, w):
    d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w
    s = -1.0 if d < 0 else 1.0
    q = [a.x * (1 - w) + b.x * w * s, a.y * (1 - w) + b.y * w * s, a.z * (1 - w) + b.z * w * s, a.w * (1 - w) + b.w * w * s]
    n = math.sqrt(sum(c * c for c in q))
    return unreal.Quat(*(c / n for c in q))


def lerpv(a, b, w):
    return unreal.Vector(a.x + (b.x - a.x) * w, a.y + (b.y - a.y) * w, a.z + (b.z - a.z) * w)


def write(dst, frames, skeleton=None):
    """frames: list of {bone: (loc, rot)}; writes a new sequence (or rewrites dst) at 30 fps."""
    path = "%s/%s" % (ANIMS, dst)
    seq = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if seq and skeleton and seq.get_editor_property("skeleton") != skeleton:
        raise RuntimeError("%s exists on another skeleton" % dst)
    if not seq:
        f = unreal.AnimSequenceFactory()
        f.set_editor_property("target_skeleton", skeleton or SKEL)
        seq = unreal.AssetToolsHelpers.get_asset_tools().create_asset(dst, ANIMS, unreal.AnimSequence, f)
    c = seq.get_editor_property("controller")
    c.open_bracket(unreal.Text("Pilot fix"), False)
    c.set_frame_rate(unreal.FrameRate(FPS, 1), False)
    c.set_number_of_frames(unreal.FrameNumber(len(frames) - 1), False)
    one = unreal.Vector(1, 1, 1)
    for b in ALL:
        try:
            c.add_bone_curve(b, False)
        except Exception:
            pass
        c.set_bone_track_keys(b, [fr[b][0] for fr in frames], [fr[b][1] for fr in frames], [one] * len(frames), False)
    c.close_bracket(False)
    seq.set_editor_property("enable_root_motion", False)
    if not unreal.EditorAssetLibrary.save_loaded_asset(seq, False):
        raise RuntimeError("could not save %s (is a running game holding the file?)" % dst)
    log("wrote %s: %d frames (%.2f s)" % (dst, len(frames), (len(frames) - 1) / float(FPS)))
    return seq


def frames_of(seq):
    n = max(2, int(round(seq.get_play_length() * FPS)) + 1)
    return [sample(seq, min(seq.get_play_length(), i / float(FPS)))[0] for i in range(n)]


for w in CFG.get("wrist", []):
    src = find(w["src"])
    frs = frames_of(src)
    off = qaxis(w["axis"], w["deg"])
    for fr in frs:
        loc, rot = fr[w["bone"]]
        fr[w["bone"]] = (loc, qmul(off, rot))
    write(w["dst"], frs)

def tq(q):
    return (q.x, q.y, q.z, q.w)


def tv(v):
    return (v.x, v.y, v.z)


def pmul(a, b):
    return tq(qmul(unreal.Quat(*a), unreal.Quat(*b)))


def prot(q, v):
    """rotate v (tuple) by q (tuple)."""
    x, y, z, w = q
    t = (2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0]))
    return (v[0] + w * t[0] + y * t[2] - z * t[1], v[1] + w * t[1] + z * t[0] - x * t[2], v[2] + w * t[2] + x * t[1] - y * t[0])


def from_to(a, b, max_deg):
    na = math.sqrt(sum(c * c for c in a)); nb = math.sqrt(sum(c * c for c in b))
    a = [c / na for c in a]; b = [c / nb for c in b]
    cx = (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
    s = math.sqrt(sum(c * c for c in cx))
    ang = math.degrees(math.atan2(s, sum(i * j for i, j in zip(a, b))))
    if s < 1e-6:
        return (0.0, 0.0, 0.0, 1.0), 0.0
    ang = min(ang, max_deg)
    q = qaxis(cx, ang)
    return tq(q), ang


_PILOT_MESH = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot")
SOCKET = _PILOT_MESH.find_socket("BladeSocket") if _PILOT_MESH else None
SOCK_Q = tq(SOCKET.relative_rotation.quaternion()) if SOCKET else (0, 0, 0, 1)

for a in CFG.get("aim", []):
    # Turn the hand at the wrist, frame by frame, so the blade (BladeSocket +X) points where it should, by at most
    # max_deg. "dir" is a mesh-space direction (the pilot faces +Y, right is -X, up +Z); "thrust" lines the blade up
    # with the forearm wherever the forearm points forward (weight ramps in from forearm.forward 0.6 to 0.85).
    src = find(a["src"])
    n = max(2, int(round(src.get_play_length() * FPS)) + 1)
    frs, turned = [], []
    for i in range(n):
        fr, pose = sample(src, min(src.get_play_length(), i / float(FPS)))
        hand_w = unreal.AnimPoseExtensions.get_bone_pose(pose, "hand_r", WORLD)
        low_w = unreal.AnimPoseExtensions.get_bone_pose(pose, "lowerarm_r", WORLD)
        hq, lq = tq(hand_w.rotation), tq(low_w.rotation)
        blade = prot(pmul(hq, SOCK_Q), (1.0, 0.0, 0.0))
        weight = 1.0
        if a.get("mode") == "thrust":
            fa = [h - l for h, l in zip(tv(hand_w.translation), tv(low_w.translation))]
            nf = math.sqrt(sum(c * c for c in fa)); fa = [c / nf for c in fa]
            want = (fa[0], fa[1], fa[2] - a.get("down", 0.15))
            weight = min(1.0, max(0.0, (fa[1] - 0.6) / 0.25))
        else:
            want = tuple(a["dir"])
        qw, ang = from_to(blade, want, a.get("max_deg", 60.0) * weight)
        # a turn in component space about the wrist = the same turn in the forearm's frame, applied before the local
        qpar = pmul(pmul((-lq[0], -lq[1], -lq[2], lq[3]), qw), lq)
        loc, rot = fr["hand_r"]
        fr["hand_r"] = (loc, qmul(unreal.Quat(*qpar), rot))
        frs.append(fr)
        turned.append(ang)
    log("%s -> %s: wrist turned %.0f..%.0f deg (mean %.0f)" % (a["src"], a["dst"], min(turned), max(turned), sum(turned) / len(turned)))
    write(a["dst"], frs)

def vadd(a, b): return tuple(i + j for i, j in zip(a, b))
def vsub(a, b): return tuple(i - j for i, j in zip(a, b))
def vmul(a, s): return tuple(i * s for i in a)
def vdot(a, b): return sum(i * j for i, j in zip(a, b))
def vlen(a): return math.sqrt(vdot(a, a))
def vnorm(a):
    n = vlen(a)
    return vmul(a, 1.0 / n) if n > 1e-9 else a
def qinv(q): return (-q[0], -q[1], -q[2], q[3])


def catmull(keys, t, index):
    """keys: [(time, tuple)] sorted; Catmull-Rom through the values (component `index` of each key)."""
    ts = [k[0] for k in keys]
    if t <= ts[0]:
        return keys[0][index]
    if t >= ts[-1]:
        return keys[-1][index]
    i = max(j for j in range(len(ts) - 1) if ts[j] <= t)
    p0 = keys[max(0, i - 1)][index]; p1 = keys[i][index]; p2 = keys[i + 1][index]; p3 = keys[min(len(keys) - 1, i + 2)][index]
    u = (t - ts[i]) / (ts[i + 1] - ts[i])
    u = u * u * (3 - 2 * u) if index == 3 else u  # ease the torso yaw, spline the hand path
    return tuple(0.5 * ((2 * b) + (-a + c) * u + (2 * a - 5 * b + 4 * c - d) * u * u + (-a + 3 * b - 3 * c + d) * u ** 3)
                 for a, b, c, d in zip(p0, p1, p2, p3))


UPPER_SKIP = ("root", "pelvis", "thigh", "calf", "foot", "ball")

for sw in CFG.get("swing", []):
    # Re-author the right arm of a blade swing. Lower body kept; the upper body yaws with the swing (spine_01 about
    # component Z); the hand follows a keyed path in the chest's frame (2-bone IK, elbow down and out) and the wrist
    # turns the blade (BladeSocket +X) onto a keyed direction. Keys are in GAME seconds at the code's play rate.
    src = find(sw["src"])
    rate = sw.get("rate", 1.0)
    L = src.get_play_length()
    n = max(2, int(round(L * FPS)) + 1)
    keys = []
    for k in sw["keys"]:
        t = L if k["t"] == "end" else k["t"] * rate
        keys.append((t, tuple(k["hand"]), tuple(k["blade"]), (k.get("yaw", 0.0),)))
    frs, log_lines = [], []
    for i in range(n):
        t = min(L, i / float(FPS))
        fr, pose = sample(src, t)
        W = {}
        for b in ALL:
            tr = unreal.AnimPoseExtensions.get_bone_pose(pose, b, WORLD)
            W[b] = [tv(tr.translation), tq(tr.rotation)]
        # torso yaw: turn spine_01 so the shoulders' line faces the keyed yaw (component +Y forward, -X right)
        right = vnorm(vsub(W["upperarm_r"][0], W["upperarm_l"][0]))
        # a turn of +yaw about +Z takes forward (+Y) toward -X (the pilot's right): right = (-cos, -sin)
        cur_yaw = math.degrees(math.atan2(-right[1], -right[0]))
        want_yaw = catmull(keys, t, 3)[0]
        dyaw = max(-sw.get("max_yaw", 45.0), min(sw.get("max_yaw", 45.0), want_yaw - cur_yaw))
        dq = tq(qaxis((0.0, 0.0, 1.0), dyaw))
        pivot = W["spine_01"][0]
        for b in ALL:
            if b.startswith(UPPER_SKIP):  # pelvis and legs stay as the clip has them
                continue
            W[b][0] = vadd(pivot, prot(dq, vsub(W[b][0], pivot)))
            W[b][1] = pmul(dq, W[b][1])
        # chest frame after the yaw
        yaw = math.radians(cur_yaw + dyaw)
        R = (-math.cos(yaw), -math.sin(yaw), 0.0)
        F = (-math.sin(yaw), math.cos(yaw), 0.0)
        U = (0.0, 0.0, 1.0)
        S = W["upperarm_r"][0]
        E0, H0 = W["lowerarm_r"][0], W["hand_r"][0]
        L1, L2 = vlen(vsub(E0, S)), vlen(vsub(H0, E0))
        hp = catmull(keys, t, 1)
        T = vadd(S, vadd(vadd(vmul(F, hp[0]), vmul(R, hp[1])), vmul(U, hp[2])))
        d = vsub(T, S)
        dl = min(vlen(d), 0.97 * (L1 + L2))
        dn = vnorm(d)
        T = vadd(S, vmul(dn, dl))
        a_ = (L1 * L1 + dl * dl - L2 * L2) / (2 * dl)
        h_ = math.sqrt(max(0.0, L1 * L1 - a_ * a_))
        pole = vadd(vmul(R, 0.55), vmul(U, -0.8))
        perp = vnorm(vsub(pole, vmul(dn, vdot(pole, dn))))
        E = vadd(S, vadd(vmul(dn, a_), vmul(perp, h_)))
        qu, _ = from_to(vsub(E0, S), vsub(E, S), 180.0)
        W["upperarm_r"][1] = pmul(qu, W["upperarm_r"][1])
        lower_rot = pmul(qu, W["lowerarm_r"][1])
        hand_dir = prot(qu, vsub(H0, E0))
        ql, _ = from_to(hand_dir, vsub(T, E), 180.0)
        W["lowerarm_r"][1] = pmul(ql, lower_rot)
        hand_rot = pmul(ql, pmul(qu, W["hand_r"][1]))
        blade = prot(pmul(hand_rot, SOCK_Q), (1.0, 0.0, 0.0))
        bk = catmull(keys, t, 2)
        want = vnorm(vadd(vadd(vmul(F, bk[0]), vmul(R, bk[1])), vmul(U, bk[2])))
        qh, ang = from_to(blade, want, 180.0)
        W["hand_r"][1] = pmul(qh, hand_rot)
        W["lowerarm_r"][0], W["hand_r"][0] = E, T
        # back to locals for the bones that changed (translations are the clip's own)
        def local(child, parent):
            return pmul(qinv(W[parent][1]), W[child][1])
        for child, parent in (("spine_01", "pelvis"), ("upperarm_r", "clavicle_r"), ("lowerarm_r", "upperarm_r"), ("hand_r", "lowerarm_r")):
            loc, _ = fr[child]
            fr[child] = (loc, unreal.Quat(*local(child, parent)))
        frs.append(fr)
        if i % 3 == 0:
            log_lines.append("t%.2f yaw%+.0f hand(F%.0f R%.0f U%.0f) wrist%.0f" % (t, cur_yaw + dyaw, hp[0], hp[1], hp[2], ang))
    log("%s -> %s (%.2f s, x%.1f): %s" % (sw["src"], sw["dst"], L, rate, " | ".join(log_lines)))
    write(sw["dst"], frs)

for d in CFG.get("drop", []):
    src = find(d["src"])
    n = max(2, int(round(src.get_play_length() * FPS)) + 1)
    frs, lows = [], []
    for i in range(n):
        fr, pose = sample(src, min(src.get_play_length(), i / float(FPS)))
        frs.append(fr)
        lows.append(min(unreal.AnimPoseExtensions.get_bone_pose(pose, b, WORLD).translation.z for b in d["bones"]))
    target = lows[-1] - d["floor_cm"]  # how far the end pose floats
    stand = lows[0]
    # ramp in as the body comes down: 0 while the lowest tracked bone is above half its standing height, full at the end
    for i, fr in enumerate(frs):
        k = min(1.0, max(0.0, (stand - lows[i]) / max(1e-3, stand - lows[-1])))
        k = k * k * (3 - 2 * k)
        loc, rot = fr["pelvis"]
        # the pelvis is a child of root, whose frame is identity here: its local Z is component Z
        fr["pelvis"] = (unreal.Vector(loc.x, loc.y, loc.z - target * k), rot)
    log("%s: end pose lowest of %s at %.1f cm, lowered %.1f cm (floor %.1f)" % (d["src"], d["bones"], lows[-1], target, d["floor_cm"]))
    write(d["dst"], frs)

for cmp_ in CFG.get("compose", []):
    out = []
    blend = int(round(cmp_.get("blend", 0.15) * FPS))
    for si, (clip, t0, t1) in enumerate(cmp_["segments"]):
        seq = find(clip)
        t1 = min(t1, seq.get_play_length())
        seg = [sample(seq, t0 + i / float(FPS))[0] for i in range(int(round((t1 - t0) * FPS)) + 1)]
        if out and blend > 0:
            k = min(blend, len(seg), len(out))
            tail = out[-k:]
            for j in range(k):
                w = (j + 1) / float(k + 1)
                a, b = tail[j], seg[j]
                tail[j] = {bn: (lerpv(a[bn][0], b[bn][0], w), nlerp(a[bn][1], b[bn][1], w)) for bn in ALL}
            out = out[:-k] + tail + seg[k:]
        else:
            out += seg
    for fr in out:  # in place: the capsule does the moving
        loc, rot = fr["root"]
        fr["root"] = (unreal.Vector(0, 0, loc.z), rot)
    write(cmp_["dst"], out)

# ---- crew / Warden clips: clean, named copies on SKEL_NullCrew (the Warden's skeleton lists it as compatible)
if CFG.get("clips"):
    ANIMS = CFG.get("folder", "/Game/TriadDrift/Gameplay/Characters/Crew/Anims")
    PILOT = ANIMS
    SKEL = unreal.load_asset(CFG.get("skeleton", "/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew"))
    ALL = bones()
for cl in CFG.get("clips", []):
    # in place (root keeps only its height), optional loop closing (the last loop_s seconds ease into frame 0),
    # optional trim [t0, t1] and play-rate bake; logs the root travel removed and the loop seam left
    src = find(cl["src"])
    L = src.get_play_length()
    t0, t1 = cl.get("trim", [0.0, L])
    t1 = min(t1, L)
    rate = cl.get("rate", 1.0)
    n = max(2, int(round((t1 - t0) / rate * FPS)) + 1)
    sp = cl.get("splice")   # this clip first, then another: {"src", "offset" (other clip's time = t + offset), "fade": [t0, t1]}
    if sp:
        other = find(sp["src"])
        n = max(2, int(round((other.get_play_length() - sp["offset"]) * FPS)) + 1)
    frs = [sample(src, min(t1, t0 + i * rate / float(FPS)))[0] for i in range(n)]
    if sp:
        for i in range(n):
            tt = i / float(FPS)
            a_, b_ = sp["fade"]
            w = min(1.0, max(0.0, (tt - a_) / (b_ - a_)))
            w = w * w * (3 - 2 * w)
            if w > 0.0:
                o = sample(other, min(other.get_play_length(), max(0.0, tt + sp["offset"])))[0]
                frs[i] = {b: (lerpv(frs[i][b][0], o[b][0], w), nlerp(frs[i][b][1], o[b][1], w)) for b in ALL}
        log("%s: spliced into %s from %.2f-%.2f s (offset %.2f); %.2f s long" % (cl["src"], sp["src"], sp["fade"][0], sp["fade"][1], sp["offset"], (n - 1) / float(FPS)))
    be = cl.get("blend_ends")   # start from / return to another clip's first frame: {"src", "in": seconds, "out": seconds}
    if be:
        g = sample(find(be["src"]), be.get("t", 0.0))[0]
        for i in range(n):
            tt = i / float(FPS)
            end = (n - 1) / float(FPS)
            w = 1.0
            if be.get("in", 0) > 0:
                w = min(w, tt / be["in"])
            if be.get("out", 0) > 0:
                w = min(w, (end - tt) / be["out"])
            w = max(0.0, min(1.0, w)); w = w * w * (3 - 2 * w)
            if w < 1.0:
                # "upper": only the upper body takes the other pose (pelvis and legs keep their own steps: no foot slide)
                up = lambda b: not be.get("upper") or (b not in ("root", "pelvis") and not b.startswith(("thigh", "calf", "foot", "ball", "ik_")))
                frs[i] = {b: ((lerpv(g[b][0], frs[i][b][0], w), nlerp(g[b][1], frs[i][b][1], w)) if up(b) else frs[i][b]) for b in ALL}
        log("%s: starts from / returns to %s frame 0 (in %.2f s, out %.2f s)" % (cl["dst"], be["src"], be.get("in", 0), be.get("out", 0)))
    travel = 0.0
    root0 = frs[0]["root"][0]
    for fr in frs:
        loc, rot = fr["root"]
        travel = max(travel, math.hypot(loc.x - root0.x, loc.y - root0.y))
        fr["root"] = (unreal.Vector(0, 0, loc.z), rot) if cl.get("in_place", True) else (loc, rot)
    # root and pelvis facing over the clip (degrees about Z), and the optional fix: the root keeps its first rotation
    def yaw_of(q):
        fwd = prot(tq(q), (0.0, 1.0, 0.0))
        return math.degrees(math.atan2(-fwd[0], fwd[1]))
    ry = [yaw_of(fr["root"][1]) for fr in frs]
    py = [yaw_of(fr["pelvis"][1]) for fr in frs]
    log("%s: root yaw %.1f..%.1f deg, pelvis local yaw %.1f..%.1f deg" % (cl["src"], min(ry), max(ry), min(py), max(py)))
    if cl.get("zero_root_yaw"):
        r0 = frs[0]["root"][1]
        for fr in frs:
            fr["root"] = (fr["root"][0], r0)
    if cl.get("lock_pelvis_yaw"):
        # the body's facing drifts in the pelvis: turn the pelvis back about Z so it faces as in frame 0
        for fr, y in zip(frs, py):
            loc, rot = fr["pelvis"]
            fr["pelvis"] = (loc, qmul(qaxis((0.0, 0.0, 1.0), -(y - py[0])), rot))
    # component-space twists (e.g. the Warden's chest, which carries its shield), in order; each later bone is taken
    # to be a descendant of the earlier ones unless "independent"
    for ti, fr in enumerate(frs):
        if not cl.get("twist"):
            break
        t = min(t1, t0 + ti * rate / float(FPS))
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(src, t, OPTS)
        acc = (0.0, 0.0, 0.0, 1.0)
        for tw in cl["twist"]:
            a, b = tw.get("ramp", [0.0, 0.0])
            frac = ti / float(max(1, n - 1))
            w = 1.0 if b <= a else min(1.0, max(0.0, (frac - a) / (b - a)))
            w = w * w * (3 - 2 * w)
            q = tq(qaxis(tuple(tw.get("axis", [0.0, 0.0, 1.0])), tw["deg"] * w))
            pw = tq(unreal.AnimPoseExtensions.get_bone_pose(pose, tw["parent"], WORLD).rotation)
            pw2 = pmul(acc, pw)
            loc, rot = fr[tw["bone"]]
            fr[tw["bone"]] = (loc, qmul(unreal.Quat(*pmul(pmul(qinv(pw2), q), pw2)), rot))
            acc = pmul(q, acc)
    # borrow part of the pose from another clip's frame (e.g. the arm that holds the Warden's chest-mounted shield)
    if cl.get("copy_pose"):
        cp = cl["copy_pose"]
        ref, _ = sample(find(cp["src"]), cp.get("t", 0.0))
        names = [b for b in ALL if any(b.startswith(k) or k in b for k in cp["bones"]) and b.endswith(cp.get("side", "_l"))]
        a, b_ = cp.get("ramp", [0.0, 0.0])
        for ti, fr in enumerate(frs):
            frac = ti / float(max(1, n - 1))
            w = 1.0 if b_ <= a else min(1.0, max(0.0, (frac - a) / (b_ - a)))
            if cp.get("hold"):   # [t0, t1] seconds: the borrowed pose is held to t0 and let go by t1
                h0, h1 = cp["hold"]
                u = min(1.0, max(0.0, (ti / float(FPS) - h0) / (h1 - h0)))
                w *= 1.0 - u * u * (3 - 2 * u)
            for bn in names:
                fr[bn] = (fr[bn][0], nlerp(fr[bn][1], ref[bn][1], w))
        log("%s: %d bones posed from %s: %s" % (cl["dst"], len(names), cp["src"], names[:6]))
    if cl.get("arc_scale") is not None:   # flatten a jump: the pelvis keeps only part of its rise above the first frame
        z0 = frs[0]["pelvis"][0].z
        for fr in frs:
            loc, rot_ = fr["pelvis"]
            fr["pelvis"] = (unreal.Vector(loc.x, loc.y, z0 + (loc.z - z0) * (cl["arc_scale"] if loc.z > z0 else 1.0)), rot_)
    if cl.get("settle"):
        # a lying body whose hips and legs hang in the air: tip the whole body about a point that is already on the
        # ground (a bone, e.g. the head), about the horizontal axis across head -> pelvis, so the hips come down
        st = cl["settle"]
        for ti, fr in enumerate(frs):
            tt = ti / float(FPS)
            a_, b_ = st["ramp"]
            w = min(1.0, max(0.0, (tt - a_) / (b_ - a_)))
            w = w * w * (3 - 2 * w)
            if w <= 0.0:
                continue
            pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(src, min(t1, t0 + ti * rate / float(FPS)), OPTS)
            hp = tv(unreal.AnimPoseExtensions.get_bone_pose(pose, st["pivot_bone"], WORLD).translation)
            rp = tv(unreal.AnimPoseExtensions.get_bone_pose(pose, "root", WORLD).translation)
            pl, pr = fr["pelvis"]
            pc = (pl.x, pl.y, pl.z)                       # pelvis in the root's frame (root has no rotation)
            piv = (hp[0] - rp[0], hp[1] - rp[1], hp[2])
            d = (pc[0] - piv[0], pc[1] - piv[1], 0.0)
            axis = (-d[1], d[0], 0.0)
            q = tq(qaxis(axis, st["deg"] * w))
            if prot(q, d)[2] > 0:                          # the turn must lower the pelvis
                q = tq(qaxis(axis, -st["deg"] * w))
            rel = (pc[0] - piv[0], pc[1] - piv[1], pc[2] - piv[2])
            nl = vadd(piv, prot(q, rel))
            fr["pelvis"] = (unreal.Vector(*nl), qmul(unreal.Quat(*q), pr))
        log("%s: settled %.0f deg about %s; pelvis ends at %.0f cm" % (cl["dst"], st["deg"], st["pivot_bone"], frs[-1]["pelvis"][0].z))
    if cl.get("drop"):
        # a lying body that rests above the floor: lower the pelvis by cm, eased in over ramp [t0, t1] seconds
        dr = cl["drop"]
        for ti, fr in enumerate(frs):
            a_, b_ = dr["ramp"]
            w = min(1.0, max(0.0, (ti / float(FPS) - a_) / (b_ - a_)))
            w = w * w * (3 - 2 * w)
            loc, rot_ = fr["pelvis"]
            fr["pelvis"] = (unreal.Vector(loc.x, loc.y, loc.z - dr["cm"] * w), rot_)
        log("%s: dropped %.0f cm; pelvis ends at %.0f cm" % (cl["dst"], dr["cm"], frs[-1]["pelvis"][0].z))
    seam = 0.0
    if cl.get("loop"):
        k = max(1, int(round(cl.get("loop_s", 0.25) * FPS)))
        first = frs[0]
        for j in range(k):
            w = (j + 1) / float(k)
            w = w * w * (3 - 2 * w)
            fr = frs[n - k + j - 1] if n - k + j - 1 >= 0 else frs[-1]
            frs[n - k + j - 1] = {b: (lerpv(fr[b][0], first[b][0], w), nlerp(fr[b][1], first[b][1], w)) for b in ALL}
        frs[-1] = dict(first)
        seam = max(math.degrees(2 * math.acos(min(1.0, abs(frs[-2][b][1].x * first[b][1].x + frs[-2][b][1].y * first[b][1].y
                                                          + frs[-2][b][1].z * first[b][1].z + frs[-2][b][1].w * first[b][1].w))))
                   for b in ALL)
    log("%s -> %s: %.2f s, root travel removed %.0f cm, loop %s (last step %.1f deg)" % (cl["src"], cl["dst"], (n - 1) / float(FPS), travel, bool(cl.get("loop")), seam))
    write(cl["dst"], frs, unreal.load_asset(cl["dst_skeleton"]) if cl.get("dst_skeleton") else None)
    if cl.get("report_feet"):
        made = find(cl["dst"])
        for foot in ("foot_l", "foot_r"):
            pts = []
            for i in range(n):
                pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(made, i / float(FPS), OPTS)
                v = unreal.AnimPoseExtensions.get_bone_pose(pose, foot, WORLD).translation
                pts.append((v.x, v.y, v.z))
            zmin = min(p[2] for p in pts)
            slide = sum(math.hypot(pts[i + 1][0] - pts[i][0], pts[i + 1][1] - pts[i][1]) for i in range(n - 1) if pts[i][2] < zmin + 2.0 and pts[i + 1][2] < zmin + 2.0)
            log("%s %s: planted slide %.1f cm in total; XY range %.0f x %.0f cm; height %.1f..%.1f cm" % (cl["dst"], foot, slide,
                max(p[0] for p in pts) - min(p[0] for p in pts), max(p[1] for p in pts) - min(p[1] for p in pts), zmin, max(p[2] for p in pts)))

for v in CFG.get("visuals", []):
    da = unreal.load_asset(PILOT + "/DA_Pilot_Visuals")
    da.set_editor_property(v["slot"], find(v["clip"]))
    unreal.EditorAssetLibrary.save_loaded_asset(da, False)
    log("DA_Pilot_Visuals.%s = %s" % (v["slot"], v["clip"]))
log("OK")
