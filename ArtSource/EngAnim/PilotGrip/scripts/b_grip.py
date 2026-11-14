# Triad Drift - solve the pilot's right-hand grip round the blade hilt on SK_Pilot's own skin, in Blender 5.2.
# blender --background --factory-startup --python b_grip.py -- <SK_Pilot.fbx> <SM_TD_BladeHilt.blend> <out_dir> [params.json]
#
# 1. Places the hilt in the palm: its axis diagonal across the palm (pommel at the heel of the hand, blade out past the
#    thumb and index), its surface on the palm skin, from landmarks of the rest hand (knuckles, palm skin).
# 2. Closes each finger like a real grasp: all three joints curl together (ratio 1 : 1.15 : 0.75); a phalanx whose skin
#    reaches the hilt's surface stops, with every joint before it; the rest keep curling.
# 3. Brings the thumb round over the index finger (a search over the thumb's base rotation and curls, scored by the tip
#    on the index's middle phalanx, no skin inside the hilt or the index).
# 4. Writes grip_blender.json (per-bone rotation deltas in armature space, the hilt frame, contact report) and review
#    renders of the posed hand holding the real hilt mesh.
# Armature space here is Blender's, in centimetres (the FBX's data); ue_grip_convert.py maps it to Unreal.
# Animation builder (eng/anim), Nov 13.
import bpy, sys, json, math, os
from mathutils import Vector, Matrix, Quaternion

argv = sys.argv[sys.argv.index("--") + 1:]
FBX, HILT_BLEND, OUT = argv[0], argv[1], argv[2]
P = {
    "tilt_deg": 28.0,        # hilt axis from straight across the knuckles toward the fingers (palm-crease diagonal)
    "back_cm": 2.6,          # hilt centre behind the knuckle line (toward the wrist)
    "skin_gap_cm": 0.15,     # hilt surface above the palm skin
    "shift_cm": 0.0,         # hilt centre along its own axis (+ = toward the blade)
    "ratios": [1.0, 1.15, 0.75],
    "limits": [90.0, 100.0, 70.0],
    "contact_gap_cm": 0.12,  # a phalanx stops when its skin is this close to the hilt surface
    "render": True,
}
if len(argv) > 3:
    P.update(json.load(open(argv[3])))
os.makedirs(OUT, exist_ok=True)

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=FBX)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
mesh = next(o for o in bpy.data.objects if o.type == 'MESH' and any(m.type == 'ARMATURE' for m in o.modifiers))
B = arm.data.bones
head = {b.name: b.head_local.copy() for b in B}
rest = {b.name: b.matrix_local.copy() for b in B}

FINGERS = ["index", "middle", "ring", "pinky"]
CHAIN = {f: ["%s_0%d_r" % (f, i) for i in (1, 2, 3)] for f in FINGERS + ["thumb"]}
HAND = "hand_r"

# ---- hand landmarks (rest)
H = head[HAND]
K = {f: head[CHAIN[f][0]] for f in FINGERS}
knuckles = sum(K.values(), Vector()) / 4.0
along = (knuckles - H).normalized()
across = (K["index"] - K["pinky"]).normalized()           # toward the thumb side
N = along.cross(across).normalized()
# palm side: where the thumb sits (radial-palmar of the hand), checked against the medial direction (+X here)
if N.dot(head["thumb_02_r"] - H) < 0:
    N = -N
across = N.cross(along).normalized() * (1 if N.cross(along).dot(across) > 0 else -1)

# ---- skin: vertices weighted to the hand, fingers and thumb (armature space = mesh data space here)
gname = {g.index: g.name for g in mesh.vertex_groups}
HANDSET = set([HAND] + [b for f in CHAIN for b in CHAIN[f]])
verts = []   # (co, [(bone, w)], dominant)
for v in mesh.data.vertices:
    ws = [(gname[g.group], g.weight) for g in v.groups if g.weight > 0.0 and g.group in gname]
    tot = sum(w for _, w in ws)
    if tot <= 0:
        continue
    hw = sum(w for n, w in ws if n in HANDSET)
    if hw / tot < 0.5:
        continue
    ws = [(n, w / tot) for n, w in ws]
    dom = max(ws, key=lambda t: t[1])[0]
    verts.append((v.co.copy(), ws, dom))

# ---- hilt radius profile along its axis (cm, pivot = grip centre, + = toward the blade)
def hilt_r(x):
    if x < -8.5 or x > 19.5:
        return 0.0
    if x < -6.3: return 1.8    # pommel
    if x < 7.6: return 1.70    # grip (ribs 3.3-3.4 cm, ferrules 3.7)
    if x < 11.0: return 3.3    # collar and fins
    return 1.75                # neck and emitter


def axial(p, C, A):
    d = p - C
    x = d.dot(A)
    return x, (d - A * x).length


def palm_depth(p):
    """How far a point is inside the hand's palm block (fingers must not pass through the palm), cm."""
    d = p - H
    u, sv, v = d.dot(along), d.dot(across), d.dot(N)
    if not (1.0 < u < HL - 0.6 and abs(sv) < 4.0 and -2.5 < v < PALM_V):
        return 0.0
    return min(PALM_V - v, v + 2.5, u - 1.0, HL - 0.6 - u)


HL = (knuckles - H).length
PALM_V = P.get("palm_v_cm", 1.6)


# ---- forward kinematics with rotation deltas in armature space about rest heads
def rot_about(q, pivot):
    return Matrix.Translation(pivot) @ q.to_matrix().to_4x4() @ Matrix.Translation(-pivot)


PARENT = {b.name: (b.parent.name if b.parent else None) for b in B}


def deltas(angles):
    """angles: {bone: Quaternion in armature space at rest}; returns {bone: 4x4 delta} for the hand's children."""
    D = {HAND: Matrix.Identity(4)}
    order = [HAND] + [b for f in FINGERS + ["thumb"] for b in CHAIN[f]]
    for b in order[1:]:
        q = angles.get(b, Quaternion())
        D[b] = D[PARENT[b]] @ rot_about(q, head[b])
    return D


def deform(co, ws, D):
    out = Vector((0.0, 0.0, 0.0))
    for n, w in ws:
        out += (D[n] @ co if n in D else co) * w
    return out


def seg_verts(bone):
    return [(co, ws) for co, ws, dom in verts if dom == bone]


def penetration(points, C, A, gap):
    worst = 0.0
    for p in points:
        x, r = axial(p, C, A)
        R = hilt_r(x)
        if R > 0 and r < R + gap:
            worst = max(worst, R + gap - r)
    return worst


hl = (knuckles - H).length
HANDV = [co for co, ws, dom in verts if dom == HAND]


def place(tilt, back, lift, drop=0.0):
    """Hilt axis diagonal across the palm (tilt toward the fingers, lift off the palm plane at the blade end), lowered
    onto the palm skin until it just touches (no hand-bone skin inside it)."""
    t = math.radians(tilt)
    A = (across * math.cos(t) + along * math.sin(t)).normalized()
    A = (A * math.cos(math.radians(lift)) + N * math.sin(math.radians(lift))).normalized()
    C0 = knuckles - along * back
    lo, hi = -2.0, 12.0
    for _ in range(30):
        h = (lo + hi) * 0.5
        if penetration(HANDV, C0 + N * h, A, P["skin_gap_cm"]) > 0:
            lo = h
        else:
            hi = h
    return C0 + N * (hi + drop) + A * P["shift_cm"], A


def finger_eval(ch, k, segs, th, C, A):
    ang = {ch[j]: Quaternion(k, math.radians(th[j])) for j in range(3)}
    D = deltas(ang)
    gaps = []
    pen = 0.0
    for s in range(3):
        g = []
        for co, ws in segs[s]:
            p = deform(co, ws, D)
            x, r = axial(p, C, A)
            R = hilt_r(x)
            if R > 0:
                g.append(r - R)
        m = min(g) if g else 9.0
        gaps.append(m)
        pen = max(pen, -m)
        if s > 0:
            for co, ws in segs[s][::2]:
                pen = max(pen, palm_depth(deform(co, ws, D)))
    w = (0.6, 1.0, 1.0)
    cost = sum(wi * min(4.0, max(0.0, gi - P["contact_gap_cm"])) ** 2 for wi, gi in zip(w, gaps)) + 300.0 * pen * pen
    cost += 0.0004 * ((th[1] - 1.1 * th[0]) ** 2 + (th[2] - 0.7 * th[1]) ** 2)
    cost += P.get("curl_w", 0.002) * max(0.0, P.get("min_mcp", 55.0) - th[0]) ** 2   # a hand that holds, not points
    return cost, gaps


def solve_fingers(C, A, sub=1):
    angles = {}
    out = {}
    for f in FINGERS:
        ch = CHAIN[f]
        d01 = (head[ch[1]] - head[ch[0]]).normalized()
        k = d01.cross(N).normalized()            # positive turn takes the finger toward the palm
        segs = [seg_verts(b)[::sub] for b in ch]
        start_pen = max(-min([axial(co, C, A)[1] - hilt_r(axial(co, C, A)[0]) for co, ws in segs[s] if hilt_r(axial(co, C, A)[0]) > 0] or [9.0]) for s in range(3))
        best = None
        for t1 in range(0, int(P["limits"][0]) + 1, 15):
            for t2 in range(0, int(P["limits"][1]) + 1, 15):
                for t3 in range(0, int(P["limits"][2]) + 1, 14):
                    c, g = finger_eval(ch, k, segs, [t1, t2, t3], C, A)
                    if best is None or c < best[0]:
                        best = (c, [float(t1), float(t2), float(t3)])
        th = best[1]
        cur = best[0]
        step = 8.0
        while step > 0.7:
            improved = False
            for j in range(3):
                for sg in (-1, 1):
                    tr = list(th)
                    tr[j] = max(0.0, min(P["limits"][j], tr[j] + sg * step))
                    c, g = finger_eval(ch, k, segs, tr, C, A)
                    if c < cur:
                        cur, th, improved = c, tr, True
            if not improved:
                step *= 0.5
        c, gaps = finger_eval(ch, k, segs, th, C, A)
        for j in range(3):
            angles[ch[j]] = Quaternion(k, math.radians(th[j]))
        out[f] = {"deg": th, "axis": k, "gaps": gaps, "start_pen": max(0.0, start_pen), "cost": c}
    return angles, out


def score(fs):
    return sum(r["cost"] for r in fs.values())
    sc = 0.0
    for f, r in fs.items():
        if r["start_pen"] > 0.05:
            sc += 50 * r["start_pen"]
        w = (0.5, 1.0, 1.0)
        sc += sum(wi * min(4.0, max(0.0, g - 0.15)) for wi, g in zip(w, r["gaps"]))
    return sc


if P.get("curl_sweep"):
    pass
if P.get("curls"):
    # Mode 2 (an animator's grip): the fingers take set curls; the hilt is fitted into the fist they make.
    angles = {}
    fs = {}
    for f in FINGERS:
        ch = CHAIN[f]
        kk = (head[ch[1]] - head[ch[0]]).normalized().cross(N).normalized()
        for jn in range(3):
            angles[ch[jn]] = Quaternion(kk, math.radians(P["curls"][f][jn]))
    D0 = deltas(angles)
    FV = {f: [[deform(co, ws, D0) for co, ws in seg_verts(b)] for b in CHAIN[f]] for f in FINGERS}
    PV = HANDV[::2]

    def frame(x):
        tilt, lift, du, dv, ds = x
        t = math.radians(tilt)
        A = (across * math.cos(t) + along * math.sin(t)).normalized()
        A = (A * math.cos(math.radians(lift)) + N * math.sin(math.radians(lift))).normalized()
        return knuckles + along * du + N * dv + across * ds, A

    def fit_cost(x, detail=False):
        C, A = frame(x)
        pen = 0.0
        gaps = {}
        segcost = 0.0
        for f in FINGERS:
            mins = []
            for s_ in (0, 1, 2):
                g = []
                for p in FV[f][s_]:
                    xx, r = axial(p, C, A)
                    R = hilt_r(xx)
                    if R > 0:
                        g.append(r - R)
                mins.append(min(g) if g else 6.0)
            m = min(mins)
            gaps[f] = m
            pen = max(pen, -m)
            # every phalanx on the hilt: the middle and the tip carry it, the first one follows
            segcost += 0.3 * max(0.0, mins[0]) ** 2 + max(0.0, mins[1]) ** 2 + max(0.0, mins[2]) ** 2
        gp = [axial(p, C, A)[1] - hilt_r(axial(p, C, A)[0]) for p in PV if hilt_r(axial(p, C, A)[0]) > 0]
        palm = min(gp) if gp else 6.0
        pen = max(pen, -palm - P.get("palm_tol_cm", 0.0))   # the armoured palm plate may sink into the hilt (hidden)
        cost = 400 * pen * pen + segcost + 0.5 * max(0.0, palm) ** 2 + P.get("deep_w", 0.0) * x[3]
        cost += 0.002 * (x[0] - P.get("tilt_pref", 15)) ** 2
        return (cost, gaps, palm) if detail else cost

    FVs = {f: [seg[::3] for seg in FV[f]] for f in FINGERS}
    FVfull, PVfull = FV, PV
    FV, PV = FVs, PVfull[::3]
    best = None
    for tl in P.get("fit_tilts", (0.0, 10.0, 20.0, 30.0)):
        for du in [i * 0.5 for i in range(-8, 3)]:
            for dv in [i * 0.5 for i in range(0, 14)]:
                for ds in (-2.0, -1.0, 0.0, 1.0, 2.0, 3.0):
                    c_ = fit_cost([tl, 0.0, du, dv, ds])
                    if best is None or c_ < best[0]:
                        best = (c_, [tl, 0.0, du, dv, ds])
    FV, PV = FVfull, PVfull
    x = best[1]
    cur = fit_cost(x)
    steps = [5.0, 5.0, 0.5, 0.5, 0.5]
    for rnd in range(40):
        improved = False
        for ii in range(5):
            for sg in (-1, 1):
                tr = list(x)
                tr[ii] += sg * steps[ii]
                c2 = fit_cost(tr)
                if c2 < cur:
                    cur, x, improved = c2, tr, True
        if not improved:
            steps = [st * 0.5 for st in steps]
            if steps[2] < 0.05:
                break
    C, A = frame(x)
    c, gaps, palm = fit_cost(x, True)
    for f in FINGERS:
        fs[f] = {"deg": P["curls"][f], "gaps": [gaps[f]] * 3}
    tilt, back, lift, drop, sc = x[0], -x[2], x[1], x[3], c
    report = {"fingers": {f: {"deg": P["curls"][f], "gap": round(gaps[f], 2)} for f in FINGERS}, "params": P,
              "placement": {"fit": x, "cost": c, "palm_gap": palm}}
    print("GRIP fit tilt %.1f lift %.1f du %.2f dv %.2f ds %.2f cost %.3f gaps %s palm %.2f" % (x[0], x[1], x[2], x[3], x[4], c, {f: round(g, 2) for f, g in gaps.items()}, palm))
else:
    cands = []
    for tilt in P.get("tilts", [18, 28, 38]):
        for back in P.get("backs", [1.5, 2.5, 3.5, 4.5]):
            for lift in P.get("lifts", [-10, 0, 10, 20]):
              for drop in P.get("drops", [0.0]):
                C, A = place(tilt, back, lift, drop)
                _, fs = solve_fingers(C, A, sub=4)
                sc = score(fs)
                cands.append((sc, tilt, back, lift, drop))
                print("GRIP try tilt %3d back %.1f lift %3d drop %.1f -> score %.2f curl %s gaps %s" % (tilt, back, lift, drop, sc, {f: [round(a) for a in r["deg"]] for f, r in fs.items()}, {f: [round(g, 1) for g in r["gaps"]] for f, r in fs.items()}))
    cands.sort()
    sc, tilt, back, lift, drop = cands[0]
    if P.get("force"):
        tilt, back, lift, drop = P["force"]
    C, A = place(tilt, back, lift, drop)
    angles, fs = solve_fingers(C, A, sub=P.get("final_sub", 4))
    report = {"fingers": {}, "params": P, "placement": {"tilt": tilt, "back": back, "lift": lift, "drop": drop, "score": sc}}
    for f, r in fs.items():
        report["fingers"][f] = {"deg": [round(a, 1) for a in r["deg"]], "gaps": [round(g, 2) for g in r["gaps"]]}
        print("GRIP %s curl %s gaps %s" % (f, [round(a, 1) for a in r["deg"]], [round(g, 2) for g in r["gaps"]]))
    print("GRIP chosen tilt %s back %s lift %s score %.2f centre %s axis %s" % (tilt, back, lift, sc, tuple(round(c, 2) for c in C), tuple(round(c, 3) for c in A)))

# ---- thumb: base rotation search + curls, tip onto the index's middle phalanx
D = deltas(angles)
ich = CHAIN["index"]
i2, i3 = D[ich[1]] @ head[ich[1]], D[ich[2]] @ head[ich[2]]
iseg = (i3 - i2)
mid_i = (i2 + i3) * 0.5
x_i, _ = axial(mid_i, C, A)
radial_out = ((mid_i - C) - A * x_i).normalized()
target = mid_i + radial_out * 1.1 - A * 0.0
tch = CHAIN["thumb"]
t1, t2, t3 = head[tch[0]], head[tch[1]], head[tch[2]]
tdir3 = (t3 - t2).normalized()
tip_rest = t3 + tdir3 * 2.4
tverts = [seg_verts(b) for b in tch]
_reach = max([(co - t1).length for seg in tverts for co, ws in seg] or [0.0])
print("GRIP thumb reach (skin, from thumb_01 head) %.2f cm; target %.2f cm away; index P2 middle %.2f cm away" % (_reach, (target - t1).length, (mid_i - t1).length))
ivert = [(co, ws) for co, ws, dom in verts if dom in ich]
ipts = [deform(co, ws, D) for co, ws in ivert]
best = None
axes = [A, N, along] if P.get("thumb_hand_axes", True) else [Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))]


def thumb_eval(a, b, c, f2, f3, fine=False):
    q1 = Quaternion(axes[0], math.radians(a)) @ Quaternion(axes[1], math.radians(b)) @ Quaternion(axes[2], math.radians(c))
    ang = dict(angles)
    ang[tch[0]] = q1
    d2 = (q1 @ (t2 - t1)).normalized()
    # curls about the axis that turns the thumb toward the hilt axis side (perpendicular to its own direction and A)
    if P.get("thumb_mode") == "wrap":
        # hand-posed wrap: the base swings the thumb round the hilt (about its axis A), then the two joints curl about
        # A as posed (their rest-space axes are A carried back through the turns above them)
        q1 = Quaternion(A, math.radians(a)) @ Quaternion(N, math.radians(b)) @ Quaternion(along, math.radians(c))
        ang[tch[0]] = q1
        q2 = Quaternion(q1.inverted() @ A, math.radians(f2))
        ang[tch[1]] = q2
        ang[tch[2]] = Quaternion((q1 @ q2).inverted() @ A, math.radians(f3))
    else:
        kt = d2.cross(-radial_out).normalized() if d2.cross(-radial_out).length > 1e-4 else A
        kt = q1.inverted() @ kt   # the curl axis in the joints' rest frame
        ang[tch[1]] = Quaternion(kt, math.radians(f2))
        ang[tch[2]] = Quaternion(kt, math.radians(f3))
    DD = deltas(ang)
    tip = DD[tch[2]] @ tip_rest
    pen = 0.0
    pts = []
    for s in range(3):
        pts += [deform(co, ws, DD) for co, ws in tverts[s]]
    pen = penetration(pts, C, A, 0.1)
    pen = max([pen] + [palm_depth(p) for p in pts[::2]])
    if max(abs(b), abs(c)) > P.get("thumb_max", 45) or not (P.get("thumb_a_min", -40) <= a <= P.get("thumb_a_max", 130)):
        pen = max(pen, 1.0)
    # into the index finger (its skin points within 0.4 cm of a thumb skin point)
    pen_i = 0.0
    if fine:
        for p in pts[::2]:
            for q in ipts[::3]:
                dd = (p - q).length
                if dd < 0.35:
                    pen_i = max(pen_i, 0.35 - dd)
    # the thumb lies on the hilt (its last two segments touch it) and its tip on the index finger
    tg = []
    for s_ in (1, 2):
        g = [axial(p, C, A)[1] - hilt_r(axial(p, C, A)[0]) for p in [deform(co, ws, DD) for co, ws in tverts[s_][::2]] if hilt_r(axial(p, C, A)[0]) > 0]
        tg.append(min(g) if g else 4.0)
    cost = (tip - target).length ** 2 + 400 * pen * pen + 400 * pen_i * pen_i + P.get("thumb_reg", 0.0004) * (a * a + b * b + c * c + f2 * f2 + f3 * f3)
    cost += P.get("thumb_contact_w", 2.0) * sum(max(0.0, g - 0.1) ** 2 for g in tg) + 0.002 * (min(0, f2) ** 2 + min(0, f3) ** 2) * 100
    return cost, ang, tip, pen, pen_i


TM = int(P.get("thumb_max", 45))
for a in range(int(P.get("thumb_a_min", -40)), int(P.get("thumb_a_max", 130)) + 1, 10):
    for b in range(-TM, TM + 1, max(9, TM // 6)):
        for c in range(-TM, TM + 1, max(9, TM // 6)):
            for f2 in (0, 25, 50, 75):
                for f3 in (0, 30, 60):
                    cost, ang, tip, pen, _ = thumb_eval(a, b, c, f2, f3)
                    if best is None or cost < best[0]:
                        best = (cost, (a, b, c, f2, f3))
# refine
params = list(best[1])
stepv = [6.0, 6.0, 6.0, 10.0, 10.0]
cur = thumb_eval(*params, fine=True)[0]
for rnd in range(6):
    for i in range(5):
        for sgn in (-1, 1):
            trial = list(params)
            trial[i] += sgn * stepv[i]
            c2 = thumb_eval(*trial, fine=True)[0]
            if c2 < cur:
                cur, params = c2, trial
    stepv = [s * 0.5 for s in stepv]
cost, ang, tip, pen, pen_i = thumb_eval(*params, fine=True)
angles = ang
report["thumb"] = {"params": params, "tip_to_target_cm": round((tip - target).length, 2), "pen_hilt": round(pen, 3), "pen_index": round(pen_i, 3)}
print("GRIP thumb %s tip->target %.2f cm pen hilt %.2f index %.2f" % ([round(p, 1) for p in params], (tip - target).length, pen, pen_i))

# ---- contact report on the final pose
D = deltas(angles)
# skin crumple: how much the thumb's triangles stretch or shrink (area ratio), worst and mean
_vd = {}
for v in mesh.data.vertices:
    ws = [(gname[g.group], g.weight) for g in v.groups if g.weight > 0 and g.group in gname]
    t = sum(w for _, w in ws)
    if t > 0 and any(n.startswith("thumb") for n, _ in ws):
        _vd[v.index] = deform(v.co, [(n, w / t) for n, w in ws], D)
_ratios = []
for poly in mesh.data.polygons:
    vs = list(poly.vertices)
    if len(vs) >= 3 and all(i in _vd for i in vs[:3]):
        a0, b0, c0 = (mesh.data.vertices[i].co for i in vs[:3])
        a1, b1, c1 = (_vd[i] for i in vs[:3])
        r0 = (b0 - a0).cross(c0 - a0).length
        if r0 > 1e-6:
            _ratios.append((b1 - a1).cross(c1 - a1).length / r0)
if _ratios:
    _dev = sorted(abs(r - 1.0) for r in _ratios)
    print("GRIP thumb skin: %d tris, area change mean %.2f, 90th pct %.2f, worst %.2f" % (len(_dev), sum(_dev) / len(_dev), _dev[int(0.9 * (len(_dev) - 1))], _dev[-1]))
print("GRIP joints from the hilt axis (cm): " + " ".join("%s %.2f" % (b, axial(D[PARENT[b]] @ head[b], C, A)[1]) for b in
      ["index_02_r", "index_03_r", "middle_03_r", "ring_03_r", "pinky_03_r", "thumb_02_r", "thumb_03_r"]))
for f in FINGERS + ["thumb"]:
    rs = []
    for b in CHAIN[f]:
        pts = [deform(co, ws, D) for co, ws in seg_verts(b)]
        if pts:
            m = min(axial(p, C, A)[1] - hilt_r(axial(p, C, A)[0]) for p in pts if hilt_r(axial(p, C, A)[0]) > 0) if any(hilt_r(axial(p, C, A)[0]) > 0 for p in pts) else 99
            rs.append(round(m, 2))
    report.setdefault("gaps_cm", {})[f] = rs
palm_pts = [deform(co, ws, D) for co, ws, dom in verts if dom == HAND]
report["palm_gap_cm"] = round(min((axial(p, C, A)[1] - hilt_r(axial(p, C, A)[0])) for p in palm_pts if hilt_r(axial(p, C, A)[0]) > 0), 2)
print("GRIP gaps (skin to hilt surface, per phalanx, cm) %s palm %.2f" % (report["gaps_cm"], report["palm_gap_cm"]))

top = (D[tch[2]] @ t3 - C)
top = (top - A * top.dot(A)).normalized()
out = {
    "space": "blender armature (cm)",
    "bones": {b: {"quat_wxyz": list(angles.get(b, Quaternion())), "pivot": list(head[b])} for b in [x for f in FINGERS + ["thumb"] for x in CHAIN[f]]},
    "delta_order": "D[b] = D[parent] * Rot(q at rest head)",
    "hilt": {"centre": list(C), "axis": list(A), "top": list(top)},
    "landmarks": {"hand_r": list(H), "knuckles": list(knuckles), "along": list(along), "across": list(across), "palm_normal": list(N)},
    "rest_heads": {b: list(head[b]) for b in [HAND, "lowerarm_r"] + [x for f in FINGERS + ["thumb"] for x in CHAIN[f]]},
    "report": report,
}
json.dump(out, open(os.path.join(OUT, "grip_blender.json"), "w"), indent=1)

if not P["render"]:
    print("GRIP done (no render)")
    sys.exit(0)

# ---- pose the armature for review renders, and put the real hilt in the hand
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='POSE')
for b in [HAND] + [x for f in FINGERS + ["thumb"] for x in CHAIN[f]]:
    pb = arm.pose.bones[b]
    pb.matrix = (Matrix.Identity(4) if P.get("rest_only") else D[b]) @ rest[b]
    bpy.context.view_layer.update()
bpy.ops.object.mode_set(mode='OBJECT')
# review only: keep the forearm and hand, drop the rest of the body (it hides the palm)
if P.get("hand_only", True):
    import bmesh
    keep = HANDSET | {"lowerarm_r"}
    bm = bmesh.new()
    bm.from_mesh(mesh.data)
    dl = bm.verts.layers.deform.active
    gidx = {g.name: g.index for g in mesh.vertex_groups}
    kidx = {gidx[n] for n in keep if n in gidx}
    drop = [v for v in bm.verts if sum(w for gi, w in v[dl].items() if gi in kidx) < 0.5 * max(1e-6, sum(v[dl].values()))]
    bmesh.ops.delete(bm, geom=drop, context='VERTS')
    # colour the digits for review: thumb red, index blue (dominant weight of the face's first vertex)
    if P.get("digit_colours", True):
        mesh.data.materials.clear()
        for nm, col in (("skin", (0.75, 0.75, 0.75, 1)), ("thumb", (0.85, 0.15, 0.1, 1)), ("index", (0.15, 0.3, 0.9, 1))):
            m = bpy.data.materials.new("rv_" + nm)
            m.diffuse_color = col
            mesh.data.materials.append(m)
        inv = {v: k for k, v in gidx.items()}
        for fc in bm.faces:
            ws = fc.verts[0][dl]
            dom = inv.get(max(ws.items(), key=lambda t: t[1])[0], "") if len(ws) else ""
            fc.material_index = 1 if dom.startswith("thumb") else 2 if dom.startswith("index") else 0
    bm.to_mesh(mesh.data)
    bm.free()
# landmark markers (small spheres): knuckles red, palm normal tip green, hilt centre blue
if P.get("markers"):
    for nm, pt, col in [("k_" + f, K[f], (1, 0, 0, 1)) for f in FINGERS] + [("H", H, (1, 1, 0, 1)), ("Npalm", H + N * 4.0, (0, 1, 0, 1)), ("C", C, (0, 0, 1, 1))]:
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.004, location=arm.matrix_world @ pt)
        s = bpy.context.active_object
        m = bpy.data.materials.new(nm)
        m.diffuse_color = col
        s.data.materials.append(m)
with bpy.data.libraries.load(HILT_BLEND) as (src, dst):
    dst.objects = [n for n in src.objects if n == "SM_TD_BladeHilt"]
hilt = dst.objects[0]
bpy.context.scene.collection.objects.link(hilt)
# the .blend hilt: blade along -Y, slide on +Z, metres; armature space is cm under the armature's 0.01 scale
Yo = -A
Zo = (top - Yo * top.dot(Yo)).normalized()
Xo = Yo.cross(Zo)
Mh = Matrix((Xo, Yo, Zo)).transposed().to_4x4()
Mh.translation = C
hilt.matrix_world = arm.matrix_world @ Mh @ Matrix.Scale(100.0, 4)
hilt.data.materials.clear()
mat = bpy.data.materials.new("hilt_review")
mat.diffuse_color = (0.95, 0.45, 0.1, 1.0)
hilt.data.materials.append(mat)
hilt.hide_render = bool(P.get("rest_only"))

sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'
sc.display.shading.light = 'STUDIO'
sc.display.shading.color_type = 'MATERIAL'
sc.display.shading.show_cavity = True
sc.render.resolution_x, sc.render.resolution_y = 900, 700
sc.render.film_transparent = False
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.lens = P.get("lens", 50)
Wc = arm.matrix_world @ ((knuckles + H) * 0.5 if P.get("rest_only") else C)
Ww = lambda v: (arm.matrix_world.to_3x3() @ v).normalized()
side = A.cross(N).normalized()
views = {"prof": across, "dorsal": -N, "palmar": N} if P.get("rest_only") else {"back": -N * 1.0 + side * 0.35, "palm": N + side * -0.35, "thumbside": A * 0.9 - N * 0.5, "pommel": -A * 0.9 - N * 0.4, "edge": side, "edge2": -side}
for name, d in views.items():
    dw = Ww(d)
    cam.location = Wc + dw * P.get("cam_m", 0.5)
    cam.rotation_euler = (-dw).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = os.path.join(OUT, "grip_%s.png" % name)
    bpy.ops.render.render(write_still=True)
print("GRIP renders in", OUT)
