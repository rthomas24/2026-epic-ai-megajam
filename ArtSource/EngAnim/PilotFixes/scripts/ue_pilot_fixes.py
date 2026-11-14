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
CFG = json.load(open(os.path.join(HERE, "..", "fixes.json")))
ANIMS = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
PILOT = "/Game/TriadDrift/Gameplay/Characters/Pilot"
SKEL = unreal.load_asset(PILOT + "/SKEL_Pilot")
OPTS = unreal.AnimPoseEvaluationOptions()
LOCAL = unreal.AnimPoseSpaces.LOCAL
WORLD = unreal.AnimPoseSpaces.WORLD
FPS = 30


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


def write(dst, frames):
    """frames: list of {bone: (loc, rot)}; writes a new sequence (or rewrites dst) at 30 fps."""
    path = "%s/%s" % (ANIMS, dst)
    seq = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not seq:
        f = unreal.AnimSequenceFactory()
        f.set_editor_property("target_skeleton", SKEL)
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
    unreal.EditorAssetLibrary.save_loaded_asset(seq, False)
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


SOCKET = unreal.load_asset(PILOT + "/SK_Pilot").find_socket("BladeSocket")
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
            want = (fa[0], fa[1], fa[2] - 0.15)
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

for v in CFG.get("visuals", []):
    da = unreal.load_asset(PILOT + "/DA_Pilot_Visuals")
    da.set_editor_property(v["slot"], find(v["clip"]))
    unreal.EditorAssetLibrary.save_loaded_asset(da, False)
    log("DA_Pilot_Visuals.%s = %s" % (v["slot"], v["clip"]))
log("OK")
