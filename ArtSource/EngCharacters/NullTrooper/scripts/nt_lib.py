# Null Trooper: modelling library (Blender 5.2, headless). Fitted armour shells on a body mesh.
# Conventions: metres, character faces -Y, +X is the character's LEFT, +Z up. Left-side parts are built and
# then mirrored to the right (bone names _l -> _r).
import bpy, bmesh, json, math
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

COL = "NullTrooper"
MATS = {}
PARTS = []
HERO2 = {"NT_Chest", "NT_Pec_L", "NT_Back", "NT_ThighFront_L", "NT_Knee_L", "NT_Shin_L", "NT_HelmDome", "NT_HelmFace"}  # every finished part object


def V(*a):
    return Vector(a)


def lerp(a, b, t):
    return a + (b - a) * t


def pw(points):
    """Piecewise-linear function through [(x, y), ...] (x ascending)."""
    pts = sorted(points)

    def f(x):
        if x <= pts[0][0]: return pts[0][1]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            if x <= x1:
                return y0 + (y1 - y0) * ((x - x0) / (x1 - x0) if x1 > x0 else 0.0)
        return pts[-1][1]
    return f


def const(v):
    return lambda x: v


def fn(v):
    if callable(v): return v
    if isinstance(v, (list, tuple)): return pw(v)
    return const(v)


def collection():
    c = bpy.data.collections.get(COL)
    if not c:
        c = bpy.data.collections.new(COL)
        bpy.context.scene.collection.children.link(c)
    return c


def material(key):
    if key in MATS: return MATS[key]
    spec = {"armor": ("M_NT_Armor", (0.030, 0.032, 0.036, 1), 0.9, 0.5, None),
            "armor2": ("M_NT_ArmorDark", (0.014, 0.015, 0.017, 1), 0.8, 0.42, None),
            "suit": ("M_NT_Suit", (0.006, 0.006, 0.007, 1), 0.0, 0.8, None),
            "emis": ("M_NT_Emissive", (0.02, 0.004, 0.002, 1), 0.0, 0.5, (1.0, 0.068, 0.013, 1)),
            "emisdim": ("M_NT_EmissiveDim", (0.02, 0.004, 0.002, 1), 0.0, 0.5, (0.45, 0.03, 0.006, 1)),
            "emisglow": ("M_NT_EmissiveGlow", (0.02, 0.004, 0.002, 1), 0.0, 0.5, (0.2, 0.014, 0.003, 1)),
            "emiscore": ("M_NT_EmissiveCore", (0.02, 0.004, 0.002, 1), 0.0, 0.5, (0.6, 0.04, 0.008, 1)),
            "emisline": ("M_NT_EmissiveLine", (0.02, 0.004, 0.002, 1), 0.0, 0.5, (0.1, 0.007, 0.002, 1)),
            "rubber": ("M_NT_Rubber", (0.009, 0.009, 0.01, 1), 0.0, 0.7, None),
            "emishex": ("M_NT_EmissiveHex", (0.02, 0.004, 0.002, 1), 0.0, 0.5, (1.0, 0.068, 0.013, 1)),
            "stripe": ("M_NT_Stripe", (0.55, 0.03, 0.01, 1), 0.0, 0.5, None)}[key]
    m = bpy.data.materials.get(spec[0]) or bpy.data.materials.new(spec[0])
    b = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    if b:
        b.inputs["Base Color"].default_value = spec[1]
        b.inputs["Metallic"].default_value = spec[2]
        b.inputs["Roughness"].default_value = spec[3]
        if spec[4]:
            b.inputs["Emission Color"].default_value = spec[4]
            b.inputs["Emission Strength"].default_value = 2.5
    m.diffuse_color = spec[1] if not spec[4] else spec[4]
    MATS[key] = m
    return m


# ---------------------------------------------------------------- body
class Body:
    def __init__(self, obj, arm):
        self.obj, self.arm = obj, arm
        me = obj.data
        self.vg = {g.index: g.name for g in obj.vertex_groups}
        self.dom = []
        for v in me.vertices:
            g = max(v.groups, key=lambda g: g.weight) if v.groups else None
            self.dom.append(self.vg[g.group] if g else "")
        self.co = [v.co.copy() for v in me.vertices]
        me.calc_loop_triangles()
        self.tris = [tuple(t.vertices) for t in me.loop_triangles]
        self._bvh = {}

    def bone(self, name):
        return (self.arm.matrix_world @ self.arm.data.bones[name].head_local).copy()

    def bvh(self, region):
        """region: tuple of bone-name prefixes, or None for the whole body."""
        key = tuple(region) if region else None
        if key not in self._bvh:
            if key is None:
                tris = self.tris
            else:
                ok = [any(d.startswith(p) for p in key) for d in self.dom]
                tris = [t for t in self.tris if ok[t[0]] or ok[t[1]] or ok[t[2]]]
            self._bvh[key] = BVHTree.FromPolygons(self.co, tris)
        return self._bvh[key]


R_TORSO = ("pelvis", "spine", "neck", "clavicle", "head")
R_TORSO_ARMS = R_TORSO + ("upperarm",)
R_ARM_L = ("upperarm_l", "upperarm_twist_01_l", "upperarm_twist_02_l", "lowerarm_l", "lowerarm_twist_01_l", "lowerarm_twist_02_l",
           "hand_l", "index_", "middle_", "ring_", "pinky_", "thumb_")
R_SHOULDER_L = ("clavicle_l", "upperarm_l", "upperarm_twist_01_l", "upperarm_twist_02_l", "spine_05", "spine_04")
R_LEG_L = ("thigh_l", "thigh_twist_01_l", "thigh_twist_02_l", "calf_l", "calf_twist_01_l", "calf_twist_02_l", "foot_l", "ball_l")
R_HIP_L = R_LEG_L + ("pelvis",)


def raycast_r(bvh, origin, d, far=0.6):
    """Distance from `origin` along `d` to the outermost body surface (ray shot inward from far away)."""
    if bvh is None: return None
    hit, n, idx, dist = bvh.ray_cast(origin + d * far, -d, far + 0.3)
    if hit is None: return None
    if n.dot(d) < -0.2:  # back face: we looked through a hole
        return None
    return far - dist


# ---------------------------------------------------------------- frames / samplers
def axis_frame(a, b, front, out):
    ax = (b - a)
    e1 = (front - ax * (front.dot(ax) / ax.dot(ax))).normalized()
    e2 = out - ax * (out.dot(ax) / ax.dot(ax))
    e2 = (e2 - e1 * e2.dot(e1)).normalized()
    return a, ax, e1, e2


def sweep(a, b, front, out, th0, th1, u0, u1):
    """Sampler around the axis a->b. theta (deg): 0 = front, +90 = out. s runs th0->th1, t runs u0->u1.
    th0/th1 are functions of t; u0/u1 functions of s (u is the fraction along a->b)."""
    a, ax, e1, e2 = axis_frame(a, b, front, out)
    th0, th1, u0, u1 = fn(th0), fn(th1), fn(u0), fn(u1)

    def f(s, t):
        th = math.radians(lerp(th0(t), th1(t), s))
        u = lerp(u0(s), u1(s), t)
        return a + ax * u, e1 * math.cos(th) + e2 * math.sin(th)
    return f


def sphere(center, pole, zero, ph0, ph1, la0, la1):
    """Sampler on a sphere around `center`. phi (deg) turns around `pole` starting at `zero` (toward pole x zero);
    lambda is the latitude toward `pole`. s runs la0->la1 (functions of t), t runs ph0->ph1 (functions of s)."""
    pole = pole.normalized()
    e0 = (zero - pole * zero.dot(pole)).normalized()
    e1 = pole.cross(e0)
    ph0, ph1, la0, la1 = fn(ph0), fn(ph1), fn(la0), fn(la1)

    def f(s, t):
        la = math.radians(lerp(la0(t), la1(t), s))
        ph = math.radians(lerp(ph0(s), ph1(s), t))
        d = (e0 * math.cos(ph) + e1 * math.sin(ph)) * math.cos(la) + pole * math.sin(la)
        return center, d
    return f


def ellipsoid(center, rx, ry, rz, rot=None):
    """Radius function (world axes, optional rotation matrix) for use as shape=(sampler-aware)."""
    inv = rot.inverted() if rot else None

    def r(o, d):
        q = inv @ d if inv else d
        return 1.0 / math.sqrt((q.x / rx) ** 2 + (q.y / ry) ** 2 + (q.z / rz) ** 2)
    return r


def planes(pl, p=10.0, base=None):
    """Rounded-polyhedron radius: soft-min over planes [(normal, distance)], optionally with a base radius fn."""
    pl = [(Vector(n).normalized(), d) for n, d in pl]

    def r(o, d):
        acc = 0.0
        for n, dist in pl:
            c = n.dot(d)
            if c > 1e-4:
                acc += (c / dist) ** p
        if base:
            acc += (1.0 / base(o, d)) ** p
        return acc ** (-1.0 / p) if acc > 0 else 1.0
    return r


# ---------------------------------------------------------------- parts
def _new_obj(name, bm, mats, bone, bevel=None, bevel_mode='WEIGHT', angle=30.0, segs=2):
    me = bpy.data.meshes.new(name)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me); bm.free()
    for k in mats: me.materials.append(material(k))
    ob = bpy.data.objects.new(name, me)
    collection().objects.link(ob)
    ob["nt_bone"] = json.dumps(bone) if not isinstance(bone, str) else bone
    ob["nt_bevel"] = json.dumps([bevel or 0.0, bevel_mode, angle, segs])
    return ob


def _params(n, grooves, axis):
    vals = [i / n for i in range(n + 1)]
    depth = {}
    for g in grooves:
        if g[0] != axis: continue
        pos, w, d = g[1], g[2], g[3]
        vals = [v for v in vals if abs(v - pos) > w * 1.6 or v in (0.0, 1.0)]
        vals += [pos - w, pos, pos + w]
        depth[round(pos, 6)] = (d, g[4] if len(g) > 4 else None)
    vals = sorted(set(round(v, 6) for v in vals))
    return vals, depth


def shell(name, sample, ns, nt, body=None, region=None, offset=0.012, shape=None, smooth=2, clear=0.004, bulge=None,
          thick=0.012, sink=0.004, maxskirt=0.035, grooves=(), wrap=False, mat="armor", emis=None, mats2=None,
          bone="transfer", bevel=0.003, inner=False, edge_drop=0.0, far=0.6, segs=1, emis_key="emis", cross=False):
    """Fitted plate. Outer surface r = max(shape, smooth(body)+offset) + bulge along the sampler's rays; a skirt
    drops from the rim into the body; the back is capped. grooves: ('s'|'t', pos, halfwidth, depth[, (lo, hi)]).
    emis: list of (s0, s1, t0, t1) rectangles that get the emissive material; mats2: [(key, s0, s1, t0, t1)]."""
    if name in HERO2: segs = 2
    bvh = body.bvh(region) if body else None
    S, gs = _params(ns, grooves, 's')
    T, gt = _params(nt, grooves, 't')
    if wrap: S = S[:-1]
    NS, NT = len(S), len(T)
    offset_f = offset if callable(offset) else (lambda s, t: offset)
    O = [[None] * NT for _ in range(NS)]; D = [[None] * NT for _ in range(NS)]
    RB = [[None] * NT for _ in range(NS)]
    for i, s in enumerate(S):
        for j, t in enumerate(T):
            o, d = sample(s, t); d = d.normalized()
            O[i][j], D[i][j] = o, d
            RB[i][j] = raycast_r(bvh, o, d, far)
    # fill holes with neighbours, then smooth
    if bvh:
        for _ in range(NS + NT):
            miss = [(i, j) for i in range(NS) for j in range(NT) if RB[i][j] is None]
            if not miss: break
            for i, j in miss:
                nb = []
                for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    a, b = i + di, j + dj
                    if wrap: a %= NS
                    if 0 <= a < NS and 0 <= b < NT and RB[a][b] is not None: nb.append(RB[a][b])
                if nb: RB[i][j] = sum(nb) / len(nb)
        for i in range(NS):
            for j in range(NT):
                if RB[i][j] is None: RB[i][j] = 0.0
        RS = [row[:] for row in RB]
        for _ in range(smooth):
            N = [row[:] for row in RS]
            for i in range(NS):
                for j in range(NT):
                    acc, w = RS[i][j] * 2.0, 2.0
                    for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        a, b = i + di, j + dj
                        if wrap: a %= NS
                        if 0 <= a < NS and 0 <= b < NT: acc += RS[a][b]; w += 1.0
                    N[i][j] = acc / w
            RS = N
    bm = bmesh.new()
    outer = [[None] * NT for _ in range(NS)]
    rin = [[None] * NT for _ in range(NS)]
    skl = [[0.0] * NT for _ in range(NS)]
    for i, s in enumerate(S):
        for j, t in enumerate(T):
            o, d = O[i][j], D[i][j]
            if bvh:
                r = RS[i][j] + offset_f(s, t)
                if shape: r = max(shape(o, d), RS[i][j] + clear)
                r = max(r, RB[i][j] + clear)
            else:
                r = shape(o, d)
            if bulge: r += bulge(s, t)
            if edge_drop:
                e = min(s, 1 - s) if not wrap else 1.0
                e = min(e, t, 1 - t)
                r -= edge_drop * max(0.0, 1.0 - e / 0.12) ** 2
            g = gs.get(round(s, 6))
            if g and (g[1] is None or g[1][0] <= t <= g[1][1]): r -= g[0]
            g = gt.get(round(t, 6))
            if g and (g[1] is None or g[1][0] <= s <= g[1][1]): r -= g[0]
            if bvh:
                ri = max(RB[i][j] - sink, r - maxskirt)
                ri = min(ri, r - 0.004)
            else:
                ri = r - thick
            outer[i][j] = bm.verts.new(o + d * r)
            rin[i][j] = ri
            skl[i][j] = max(0.002, r - ri)
    keys = [mat]
    if emis: keys.append(emis_key)
    for m2 in (mats2 or []):
        if m2[0] not in keys: keys.append(m2[0])
    for g in grooves:
        if len(g) > 5 and g[5] not in keys: keys.append(g[5])
    bw = bm.edges.layers.float.new("bevel_weight_edge")
    ni = NS if wrap else NS - 1

    def face_mat(i, j):
        sc = (S[i] + (S[i + 1] if i + 1 < NS else 1.0)) / 2
        tc = (T[j] + T[j + 1]) / 2
        for g in grooves:
            if len(g) > 5:
                a, c = (sc, tc) if g[0] == 's' else (tc, sc)
                rng = g[4] or (0.0, 1.0)
                if abs(a - g[1]) < g[2] and rng[0] <= c <= rng[1]: return keys.index(g[5])
        for e in (emis or []):
            if e[0] <= sc <= e[1] and e[2] <= tc <= e[3]: return keys.index(emis_key)
        for m2 in (mats2 or []):
            if m2[1] <= sc <= m2[2] and m2[3] <= tc <= m2[4]: return keys.index(m2[0])
        return 0
    uvl = bm.loops.layers.uv.new("UVMap")

    def plen(pts):
        return sum((q - p_).length for p_, q in zip(pts, pts[1:]))
    W = sum(plen([outer[i][j].co for i in range(NS)]) for j in range(NT)) / NT
    if wrap: W *= NS / max(1.0, NS - 1.0)
    Hh = sum(plen([outer[i][j].co for j in range(NT)]) for i in range(NS)) / NS
    Sx = lambda i: (S[i] if i < NS else 1.0) * W

    cl = bm.loops.layers.float_color.new("PlateCoord")

    def setuv(face, uvs, plate=False):
        # PlateCoord: R / G = distance (m) to the plate's s / t edges, B = signed distance from the plate's middle, A = cross-break flag
        for l_, u_ in zip(face.loops, uvs):
            l_[uvl].uv = u_
            if plate:
                l_[cl] = (1.0 if wrap else min(u_[0], W - u_[0]), min(u_[1], Hh - u_[1]), u_[1] - Hh * 0.5, 1.0 if cross else 0.0)
            else:
                l_[cl] = (1.0, 1.0, 1.0, 0.0)
    for i in range(ni):
        for j in range(NT - 1):
            a, b = i, (i + 1) % NS
            f = bm.faces.new((outer[a][j], outer[b][j], outer[b][j + 1], outer[a][j + 1]))
            f.material_index = face_mat(i, j); f.smooth = True
            setuv(f, [(Sx(i), T[j] * Hh), (Sx(i + 1), T[j] * Hh), (Sx(i + 1), T[j + 1] * Hh), (Sx(i), T[j + 1] * Hh)], plate=True)
    # boundary loops (index pairs, in order)
    if wrap:
        loops = [[(i, 0) for i in range(NS)], [(i, NT - 1) for i in range(NS)]]
    else:
        loop = [(i, 0) for i in range(NS)] + [(NS - 1, j) for j in range(1, NT)] + \
               [(i, NT - 1) for i in range(NS - 2, -1, -1)] + [(0, j) for j in range(NT - 2, 0, -1)]
        loops = [loop]
    corners = set() if wrap else {(0, 0), (NS - 1, 0), (NS - 1, NT - 1), (0, NT - 1)}
    if inner:
        innerv = [[bm.verts.new(O[i][j] + D[i][j] * rin[i][j]) for j in range(NT)] for i in range(NS)]
        for i in range(ni):
            for j in range(NT - 1):
                a, b = i, (i + 1) % NS
                f = bm.faces.new((innerv[a][j + 1], innerv[b][j + 1], innerv[b][j], innerv[a][j])); f.smooth = True
                X0 = W * 2.3 + 0.2
                setuv(f, [(X0 - Sx(i), T[j + 1] * Hh), (X0 - Sx(i + 1), T[j + 1] * Hh), (X0 - Sx(i + 1), T[j] * Hh), (X0 - Sx(i), T[j] * Hh)])
        getin = lambda ij: innerv[ij[0]][ij[1]]
    else:
        cache = {}

        def getin(ij):
            if ij not in cache:
                i, j = ij
                cache[ij] = bm.verts.new(O[i][j] + D[i][j] * rin[i][j])
            return cache[ij]
    inner_loops = []
    for loop in loops:
        n = len(loop)
        il = []
        for k in range(n):
            p, q = loop[k], loop[(k + 1) % n]
            vo0, vo1 = outer[p[0]][p[1]], outer[q[0]][q[1]]
            vi0, vi1 = getin(p), getin(q)
            f = bm.faces.new((vo0, vo1, vi1, vi0)); f.smooth = True

            def bd(ij):
                if wrap: return (0.0, -1.0 if ij[1] == 0 else 1.0)
                return (-1.0 if ij[0] == 0 else (1.0 if ij[0] == NS - 1 else 0.0), -1.0 if ij[1] == 0 else (1.0 if ij[1] == NT - 1 else 0.0))
            sp = S[p[0]] * W
            sq = (1.0 if (wrap and q[0] == 0 and p[0] == NS - 1) else S[q[0]]) * W
            if wrap and p[0] == 0 and q[0] == NS - 1: sp = W
            up_, uq_ = (sp, T[p[1]] * Hh), (sq, T[q[1]] * Hh)
            dp, dq = bd(p), bd(q)
            setuv(f, [up_, uq_, (uq_[0] + dq[0] * skl[q[0]][q[1]], uq_[1] + dq[1] * skl[q[0]][q[1]]),
                      (up_[0] + dp[0] * skl[p[0]][p[1]], up_[1] + dp[1] * skl[p[0]][p[1]])])
            e = bm.edges.get((vo0, vo1))
            if e: e[bw] = 1.0
            if not inner:
                pass
            il.append(vi0)
            if p in corners:
                e = bm.edges.get((vo0, vi0))
                if e: e[bw] = 1.0
        inner_loops.append(il)
    # no back caps: the open backs sit inside the body and are never visible
    # groove edges bevel-free; done
    ob = _new_obj(name, bm, keys, bone, bevel=bevel, bevel_mode='WEIGHT', segs=segs)
    return ob


def hull(name, pts, mat="armor", bone="transfer", bevel=0.003, angle=20.0, mirror_x=False, segs=1):
    bm = bmesh.new()
    P = [Vector(p) for p in pts]
    if mirror_x: P += [Vector((-p.x, p.y, p.z)) for p in P if abs(p.x) > 1e-6]
    vs = [bm.verts.new(p) for p in P]
    r = bmesh.ops.convex_hull(bm, input=vs)
    junk = list({g for g in r.get("geom_interior", []) + r.get("geom_unused", []) if isinstance(g, bmesh.types.BMVert)})
    if junk: bmesh.ops.delete(bm, geom=junk, context='VERTS')
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(1.5), verts=bm.verts, edges=bm.edges)
    return _new_obj(name, bm, [mat], bone, bevel=bevel, bevel_mode='ANGLE', angle=angle, segs=segs)


def prism(name, outline, origin, ex, ey, ez, depth, taper=1.0, mat="armor", cap_mat=None, bone="transfer", bevel=0.003,
          angle=25.0, back=0.0, inset=None, segs=1):
    """Extrude a 2D outline (in ex/ey) along ez from -back to depth. inset=(margin, sink): an inset top face that
    gets cap_mat (e.g. an emissive panel in a frame)."""
    ex, ey, ez = ex.normalized(), ey.normalized(), ez.normalized()
    bm = bmesh.new()
    c2 = Vector((sum(p[0] for p in outline) / len(outline), sum(p[1] for p in outline) / len(outline)))
    bot = [bm.verts.new(origin + ex * p[0] + ey * p[1] - ez * back) for p in outline]
    top = [bm.verts.new(origin + ex * lerp(c2.x, p[0], taper) + ey * lerp(c2.y, p[1], taper) + ez * depth) for p in outline]
    n = len(outline)
    for k in range(n):
        bm.faces.new((bot[k], bot[(k + 1) % n], top[(k + 1) % n], top[k]))
    bm.faces.new(bot[::-1])
    keys = [mat]
    if inset:
        if cap_mat and cap_mat not in keys: keys.append(cap_mat)
        m, sk = inset
        ins = []
        for k in range(n):
            p = Vector((lerp(c2.x, outline[k][0], taper), lerp(c2.y, outline[k][1], taper)))
            dirc = (c2 - p); L = dirc.length
            q = p + dirc * min(0.9, m / L)
            ins.append(bm.verts.new(origin + ex * q.x + ey * q.y + ez * (depth - sk)))
        for k in range(n):
            bm.faces.new((top[k], top[(k + 1) % n], ins[(k + 1) % n], ins[k]))
        f = bm.faces.new(ins)
        f.material_index = keys.index(cap_mat) if cap_mat else 0
    else:
        f = bm.faces.new(top)
        if cap_mat:
            keys.append(cap_mat); f.material_index = 1
    return _new_obj(name, bm, keys, bone, bevel=bevel, bevel_mode='ANGLE', angle=angle, segs=segs)


def box(name, center, ex, ey, ez, sx, sy, sz, **kw):
    o = [(-sx / 2, -sy / 2), (sx / 2, -sy / 2), (sx / 2, sy / 2), (-sx / 2, sy / 2)]
    return prism(name, o, center, ex, ey, ez, sz, **kw)


def lathe(name, center, axis, profile, segs=20, mat="armor", ring_mats=None, bone="transfer", bevel=0.0, angle=30.0, ref=None):
    """profile: [(height along axis, radius)]; ends are capped. ring_mats: {profile segment index: material key}."""
    axis = axis.normalized()
    ref = ref or (Vector((0, 0, 1)) if abs(axis.z) < 0.9 else Vector((1, 0, 0)))
    e1 = (ref - axis * ref.dot(axis)).normalized(); e2 = axis.cross(e1)
    bm = bmesh.new()
    rings = []
    for h, r in profile:
        if r <= 1e-6:
            rings.append([bm.verts.new(center + axis * h)])
        else:
            rings.append([bm.verts.new(center + axis * h + (e1 * math.cos(2 * math.pi * k / segs) + e2 * math.sin(2 * math.pi * k / segs)) * r)
                          for k in range(segs)])
    keys = [mat]
    for k in (ring_mats or {}).values():
        if k not in keys: keys.append(k)
    for idx, (a, b) in enumerate(zip(rings, rings[1:])):
        mi = keys.index(ring_mats[idx]) if ring_mats and idx in ring_mats else 0
        for k in range(segs):
            k2 = (k + 1) % segs
            if len(a) == 1 and len(b) == 1: continue
            if len(a) == 1: f = bm.faces.new((a[0], b[k], b[k2]))
            elif len(b) == 1: f = bm.faces.new((a[k], a[k2], b[0]))
            else: f = bm.faces.new((a[k], a[k2], b[k2], b[k]))
            f.material_index = mi; f.smooth = True
    if len(rings[0]) > 1: bm.faces.new(rings[0][::-1])
    if len(rings[-1]) > 1: bm.faces.new(rings[-1])
    return _new_obj(name, bm, keys, bone, bevel=bevel, bevel_mode='ANGLE', angle=angle)


def tube(name, path, radius, segs=8, mat="armor2", bone="transfer", ribs=0, rib_depth=0.003, sub=6, groove_mat=None, groove_every=1):
    """Ribbed hose along a Catmull-Rom path. Each rib is three rings (high, high, low); the two bands into the
    groove get groove_mat."""
    P = [Vector(p) for p in path]
    pts = []
    ext = [P[0] * 2 - P[1]] + P + [P[-1] * 2 - P[-2]]
    for k in range(1, len(ext) - 2):
        p0, p1, p2, p3 = ext[k - 1], ext[k], ext[k + 1], ext[k + 2]
        for q in range(sub):
            t = q / sub
            pts.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    pts.append(P[-1])
    if ribs:
        Ln = [0.0]
        for a_, b_ in zip(pts, pts[1:]): Ln.append(Ln[-1] + (b_ - a_).length)
        n = ribs * 3
        res = []
        for k in range(n + 1):
            d = Ln[-1] * k / n
            for i in range(len(Ln) - 1):
                if Ln[i + 1] >= d - 1e-9:
                    f = (d - Ln[i]) / max(1e-9, Ln[i + 1] - Ln[i]); res.append(pts[i].lerp(pts[i + 1], f)); break
        pts = res
    bm = bmesh.new()
    keys = [mat] + ([groove_mat] if groove_mat else [])
    rings = []
    up = Vector((0, 0, 1))
    e1 = None
    for k, p in enumerate(pts):
        tg = (pts[min(k + 1, len(pts) - 1)] - pts[max(k - 1, 0)]).normalized()
        if e1 is None:
            e1 = (up - tg * up.dot(tg))
            if e1.length < 1e-3: e1 = Vector((1, 0, 0)) - tg * tg.x
            e1.normalize()
        else:
            e1 = (e1 - tg * e1.dot(tg)).normalized()
        e2 = tg.cross(e1)
        r = radius - (rib_depth if (ribs and k % 3 == 2) else 0.0)
        rings.append([bm.verts.new(p + (e1 * math.cos(2 * math.pi * q / segs) + e2 * math.sin(2 * math.pi * q / segs)) * r) for q in range(segs)])
    for k, (a_, b_) in enumerate(zip(rings, rings[1:])):
        for q in range(segs):
            q2 = (q + 1) % segs
            f = bm.faces.new((a_[q], a_[q2], b_[q2], b_[q])); f.smooth = True
            if groove_mat and ribs and k % 3 != 0 and (k // 3) % groove_every == 0: f.material_index = 1
    bm.faces.new(rings[0][::-1]); bm.faces.new(rings[-1])
    return _new_obj(name, bm, keys, bone, bevel=0.0)


def loft(name, sections, origin, ex, ey, ez, ring=16, mat="armor", bone="transfer", bevel=0.0, angle=30.0, cap=True):
    """Superellipse sections along ey. section: dict(y, w, h, n=2.5, x=0, z=0 (centre), flat=0 (flatten bottom fraction))."""
    ex, ey, ez = ex.normalized(), ey.normalized(), ez.normalized()
    bm = bmesh.new()
    rings = []
    for sct in sections:
        n = sct.get("n", 2.5); w, h = sct["w"] / 2, sct["h"] / 2
        r = []
        for k in range(ring):
            a = 2 * math.pi * k / ring
            c, s_ = math.cos(a), math.sin(a)
            px = w * (abs(c) ** (2 / n)) * (1 if c >= 0 else -1)
            pz = h * (abs(s_) ** (2 / n)) * (1 if s_ >= 0 else -1)
            fl = sct.get("flat", 0.0)
            if fl and pz < -h * (1 - fl): pz = -h * (1 - fl)
            r.append(bm.verts.new(origin + ex * (px + sct.get("x", 0.0)) + ey * sct["y"] + ez * (pz + sct.get("z", 0.0))))
        rings.append(r)
    for a, b in zip(rings, rings[1:]):
        for k in range(ring):
            k2 = (k + 1) % ring
            bm.faces.new((a[k], a[k2], b[k2], b[k])).smooth = True
    if cap:
        bm.faces.new(rings[0][::-1]); bm.faces.new(rings[-1])
    return _new_obj(name, bm, [mat], bone, bevel=bevel, bevel_mode='ANGLE', angle=angle)


# ---------------------------------------------------------------- finishing
def _swap_lr(name):
    if name.endswith("_l"): return name[:-2] + "_r"
    if name.endswith("_r"): return name[:-2] + "_l"
    return name


def mirror(ob):
    """Mirrored copy across X (left -> right)."""
    me = ob.data.copy()
    me.transform(Matrix.Scale(-1, 4, (1, 0, 0)))
    me.flip_normals()
    name = ob.name[:-2] + "_R" if ob.name.endswith("_L") else ob.name + "_R"
    me.name = name
    o2 = bpy.data.objects.new(name, me)
    collection().objects.link(o2)
    b = ob["nt_bone"]
    if b != "transfer" and not b.startswith("fn:"):
        d = json.loads(b)
        b = json.dumps({_swap_lr(k): v for k, v in d.items()})
    o2["nt_bone"] = b
    o2["nt_bevel"] = ob["nt_bevel"]
    return o2


def finish(ob, mirrored=False):
    """Bevel + weighted normals (applied). Returns [ob] or [ob, mirrored twin]."""
    obs = [ob] + ([mirror(ob)] if mirrored else [])
    dg = bpy.context.evaluated_depsgraph_get()
    for o in obs:
        width, mode, angle, segs = json.loads(o["nt_bevel"])
        for p in o.data.polygons: p.use_smooth = True
        if width > 0:
            m = o.modifiers.new("Bevel", 'BEVEL')
            m.width = width; m.segments = int(segs); m.limit_method = mode
            if mode == 'ANGLE': m.angle_limit = math.radians(angle)
            m.harden_normals = False; m.miter_outer = 'MITER_ARC'
        wn = o.modifiers.new("WN", 'WEIGHTED_NORMAL'); wn.keep_sharp = True; wn.weight = 50
        if width <= 0:
            o.data.set_sharp_from_angle(angle=math.radians(40))
        bpy.context.view_layer.update()
        dg = bpy.context.evaluated_depsgraph_get()
        me = bpy.data.meshes.new_from_object(o.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
        old = o.data
        o.modifiers.clear()
        o.data = me; me.name = o.name
        bpy.data.meshes.remove(old)
        if not me.uv_layers:
            me.uv_layers.new(name="UVMap")
            smart_unwrap(o)
        PARTS.append(o)
    return obs


def smart_unwrap(o, angle=66.0):
    for x in bpy.context.selected_objects: x.select_set(False)
    o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=0.01)
    bpy.ops.object.mode_set(mode='OBJECT')
    o.select_set(False)


def obj_bvh(ob):
    me = ob.data
    me.calc_loop_triangles()
    return BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [tuple(t.vertices) for t in me.loop_triangles])


def surf(bvh, origin, d, far=0.6):
    """Outermost point of a part along a ray: (point, normal)."""
    hit, n, idx, dist = bvh.ray_cast(origin + d * far, -d, far + 0.3)
    if hit is None: return None, None
    return hit, n
