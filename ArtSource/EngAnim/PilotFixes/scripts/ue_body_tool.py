# Triad Drift - world-space touch-ups on an existing crew / Warden clip, feet kept where they are by leg IK:
#   shift      [[t, dx, dy, dz], ...]  the body (everything but the root) moves by this, cm, mesh space (forward +Y, right -X)
#   feet       {"l" | "r": [[t, dx, dy, dz], ...]}  where that foot goes from its own position in the source clip
#   ik         [t0, t1]  when the legs are solved to the foot targets (default: the whole clip)
#   chest_yaw  [[t, deg], ...]  the upper body turns about spine_01 (+ = left shoulder forward), on top of the clip
#   arm_drop   {"l" | "r": [[t, deg], ...]}  that arm swings down from the shoulder by deg
# Keys are eased (smoothstep) between neighbours. TD_BODY = config json. Animation builder (eng/anim), Nov 14.
import json
import math
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
os.environ["TD_AUTHOR"] = os.environ["TD_BODY"]
_src = open(os.path.join(HERE, "ue_crew_author.py"), encoding="utf-8-sig").read()
exec(_src[:_src.index("for mv in CFG[")])   # helpers, ALL, REF, write() of the authoring tool


def keyed(keys, t, width):
    if not keys:
        return (0.0,) * width
    if t <= keys[0][0]:
        return tuple(keys[0][1:])
    for a, b in zip(keys, keys[1:]):
        if t <= b[0]:
            w = (t - a[0]) / max(1e-6, b[0] - a[0])
            w = w * w * (3 - 2 * w)
            return tuple(x + (y - x) * w for x, y in zip(a[1:], b[1:]))
    return tuple(keys[-1][1:])


ARM = ("upperarm", "lowerarm", "hand", "index", "middle", "ring", "pinky", "thumb", "weapon")
for cl in CFG["clips"]:
    src = find(cl["src"])
    rate = cl.get("rate", 1.0)
    L = src.get_play_length() / rate
    n = int(round(L * FPS)) + 1
    ik0, ik1 = cl.get("ik", [0.0, L])
    frames = []
    track = {}
    for i in range(n):
        t = i / float(FPS)
        pose = X.get_anim_pose_at_time(src, min(src.get_play_length(), t * rate), OPTS)
        fr, W = {}, {}
        for b in ALL:
            lt, wt = X.get_bone_pose(pose, b, LOCAL), X.get_bone_pose(pose, b, WORLD)
            fr[b] = [tv(lt.translation), tq(lt.rotation)]
            W[b] = [tv(wt.translation), tq(wt.rotation)]
        upper = [b for b in ALL if b not in ("root", "pelvis") and not b.startswith(LEG) and not b.startswith("ik_")]
        foot0 = {s: W["foot_" + s][0] for s in "lr"}
        foot_rot0 = {s: W["foot_" + s][1] for s in "lr"}
        shift = keyed(cl.get("shift"), t, 3)
        for b in ALL:
            if b != "root" and not b.startswith("ik_"):
                W[b][0] = add(W[b][0], shift)
        yaw = keyed(cl.get("chest_yaw"), t, 1)[0]
        if abs(yaw) > 1e-4:
            piv, q = W["spine_01"][0], qaxis((0.0, 0.0, 1.0), yaw)
            for b in upper:
                W[b][0] = add(piv, rot(q, sub(W[b][0], piv)))
                W[b][1] = qmul(q, W[b][1])
        for s, keys in cl.get("arm_drop", {}).items():
            deg = keyed(keys, t, 1)[0]
            if abs(deg) > 1e-4:
                piv = W["upperarm_" + s][0]
                d = norm(sub(W["lowerarm_" + s][0], piv))
                ax = cross(d, (0.0, 0.0, -1.0))
                if length(ax) > 1e-4:
                    q = qaxis(ax, deg)
                    for b in ALL:
                        if b.endswith("_" + s) and b.startswith(ARM):
                            W[b][0] = add(piv, rot(q, sub(W[b][0], piv)))
                            W[b][1] = qmul(q, W[b][1])
        solve = ik0 - 1e-6 <= t <= ik1 + 1e-6
        for s in "lr":
            th, ca, fo = "thigh_" + s, "calf_" + s, "foot_" + s
            if solve:
                Hp = W[th][0]
                L1, L2 = length(sub(W[ca][0], W[th][0])), length(sub(W[fo][0], W[ca][0]))
                tgt = add(foot0[s], keyed(cl.get("feet", {}).get(s), t, 3))
                pole = add(sub(W[ca][0], Hp), (0.0, 0.3 * (L1 + L2), 0.0))
                K, Tg = two_bone(Hp, tgt, L1, L2, pole)
                qt = from_to(sub(W[ca][0], Hp), sub(K, Hp))
                tree = [b for b in ALL if b.endswith("_" + s) and b.startswith(LEG)]
                for b in tree:
                    W[b][0] = add(Hp, rot(qt, sub(W[b][0], Hp)))
                    W[b][1] = qmul(qt, W[b][1])
                qc = from_to(sub(W[fo][0], W[ca][0]), sub(Tg, K))
                for b in tree:
                    if not b.startswith("thigh"):
                        W[b][0] = add(K, rot(qc, sub(W[b][0], K)))
                        W[b][1] = qmul(qc, W[b][1])
                d = qmul(foot_rot0[s], qinv(W[fo][1]))   # the foot keeps the source clip's orientation
                for b in tree:
                    if b.startswith(("foot", "ball")):
                        W[b][0] = add(W[fo][0], rot(d, sub(W[b][0], W[fo][0])))
                        W[b][1] = qmul(d, W[b][1])
            track.setdefault(s, []).append(W[fo][0])
        par = {"spine_01": "pelvis"}
        for sd in "lr":
            par.update({"upperarm_" + sd: "clavicle_" + sd, "thigh_" + sd: "pelvis", "calf_" + sd: "thigh_" + sd, "foot_" + sd: "calf_" + sd})
        out = {}
        for b in ALL:
            p_ = par.get(b)
            out[b] = (tuple(fr[b][0]), qmul(qinv(W[p_][1]), W[b][1]) if p_ else tuple(fr[b][1]))
        out["pelvis"] = (add(fr["pelvis"][0], shift), out["pelvis"][1])
        if cl.get("in_place", True):
            out["root"] = ((0.0, 0.0, fr["root"][0][2]), out["root"][1])
        frames.append(out)
    write(cl["dst"], frames)
    log("%s pelvis height every 0.2 s: %s" % (cl["dst"], " ".join("%.0f" % frames[j][0 if False else "pelvis"][0][2] for j in range(0, n, 6))))
    slide = {}
    for s_ in "lr":   # planted = within 2 cm of that foot's lowest height on both frames (the measure used on the earlier versions)
        pts = track[s_]
        zmin = min(p[2] for p in pts)
        slide[s_] = sum(math.hypot(pts[j + 1][0] - pts[j][0], pts[j + 1][1] - pts[j][1]) for j in range(len(pts) - 1) if pts[j][2] < zmin + 2.0 and pts[j + 1][2] < zmin + 2.0)
    log("%s -> %s: %.2f s; planted foot slide in total: left %.1f cm, right %.1f cm" % (cl["src"], cl["dst"], L, slide["l"], slide["r"]))
log("OK")
