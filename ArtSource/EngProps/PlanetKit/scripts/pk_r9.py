# SPDX-License-Identifier: MIT
"""
pk_r9.py - round 9 rebuild of the weakest PlanetKit pieces after the r5 review (kit average ~2.8):

  SM_Verd_Wreck_Hull   no longer a tube: a crashed squad-style dropship fuselage with a lifting-body section
                       (hard chines), an ogive nose ploughed into the soil, a cockpit canopy a third of the way
                       back (the focal point), a dorsal spine and torn tail-fin root, one intact swept wing stub
                       rolled up into the air and one torn-off stub with spars sticking out, the stern torn open
                       where the engine pod ripped away (ribs, bulkhead, hanging conduits). Moss is a dense sheet
                       whose lift field crosses the skin smoothly, so its outline is a soft organic contour.
  SM_Verd_Wreck_Wing / _Fin   real airfoil sections (not single plates): skins with thickness, ribs and I-beam
                       spars exposed at a torn root whose edge follows a smooth tear curve (no stair-steps),
                       a deflected control surface on hinge brackets, a dark leading edge, livery bands that
                       conform to the skin, a nav-light pod embedded in the tip (lens seated in the pod).
  SM_Verd_Log          gnarled trunk with bark fissures, buttress root flare and roots into the ground, a hollow
                       splintered broken end with a glowing mushroom cluster inside, tiered shelf fungus,
                       soft moss sheet on top.
  SM_Cind_CollapseSlab columnar basalt: the slab is the flat tops of hexagonal columns (clipped flush to the box
                       sides), split into four pieces along the column joints; magma glows down the cracks.
  SM_Cind_DebrisChunk  same rock, scaled so its largest dimension is 150 cm.

Footprints are unchanged (finish_pk fits each mesh to the same box; footprint_check.py verifies).
"""

import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise
from mathutils.bvhtree import BVHTree

import pk_lib as L
from pk_ext import role_bark, role_basalt
from pk_geo import T, R, bx, prism, ring_loft, shard
from pk_verdance import wreck_mats, _moss_blob, _cut_all_below
from pk_wreck import _hang_vines


# =========================================================================== helpers

def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3.0 - 2.0 * t)


def smooth_boundary(bm, pred, iters=4, amount=0.5):
    """Laplacian smoothing of open boundary loops (only boundary neighbours count): a torn edge cut from a grid
    loses its stair-steps and follows a smooth curve."""
    for _ in range(iters):
        newpos = {}
        for v in bm.verts:
            if not v.is_boundary or not pred(v):
                continue
            nb = [e.other_vert(v) for e in v.link_edges if e.is_boundary]
            if len(nb) != 2:
                continue
            newpos[v] = v.co.lerp((nb[0].co + nb[1].co) * 0.5, amount)
        for v, p in newpos.items():
            v.co = p


def solidify_in(bm, thickness, inside):
    """Solidify an open skin INWARD (bmesh solidify extrudes along -normal, and lofted skins come out with either
    winding): the parametric surface stays the outer surface, so bands and panels laid on it sit on top."""
    bm.normal_update()
    score = 0.0
    for f in bm.faces:
        c = f.calc_center_median()
        score += f.normal.dot(c - inside(c)) * f.calc_area()
    bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=thickness if score >= 0 else -thickness)


def _bvh(objs):
    vs, fs = [], []
    for o in objs:
        mw = o.matrix_world
        base = len(vs)
        vs.extend(mw @ v.co for v in o.data.vertices)
        fs.extend([i + base for i in p.vertices] for p in o.data.polygons)
    return BVHTree.FromPolygons(vs, fs)


def shell_patch(P, mat, S, u0, u1, nu, v0, v1, nv, hint, off=0.004, thk=0.022, matrix=None, smooth=40):
    """Thin closed shell laid ON a parametric surface S(u, v) -> point: livery bands, panels, hatches that
    conform to curved skin. hint(u) gives a point inside the body (to orient the normals outward)."""
    pts, nrm = [], []
    eu, ev = (u1 - u0) * 1e-3, (v1 - v0) * 1e-3
    for i in range(nu + 1):
        u = u0 + (u1 - u0) * i / nu
        row_p, row_n = [], []
        for j in range(nv + 1):
            v = v0 + (v1 - v0) * j / nv
            p = S(u, v)
            du = S(u + eu, v) - S(u - eu, v)
            dv = S(u, v + ev) - S(u, v - ev)
            n = du.cross(dv)
            n = n.normalized() if n.length > 1e-12 else Vector((0, 0, 1))
            if n.dot(p - hint(u)) < 0:
                n = -n
            row_p.append(p)
            row_n.append(n)
        pts.append(row_p)
        nrm.append(row_n)
    bm = bmesh.new()
    outer = [[bm.verts.new(pts[i][j] + nrm[i][j] * (off + thk)) for j in range(nv + 1)] for i in range(nu + 1)]
    inner = [[bm.verts.new(pts[i][j] - nrm[i][j] * 0.012) for j in range(nv + 1)] for i in range(nu + 1)]
    for i in range(nu):
        for j in range(nv):
            bm.faces.new((outer[i][j], outer[i + 1][j], outer[i + 1][j + 1], outer[i][j + 1]))
            bm.faces.new((inner[i][j + 1], inner[i + 1][j + 1], inner[i + 1][j], inner[i][j]))
    for i in range(nu):
        bm.faces.new((outer[i][0], inner[i][0], inner[i + 1][0], outer[i + 1][0]))
        bm.faces.new((outer[i + 1][nv], inner[i + 1][nv], inner[i][nv], outer[i][nv]))
    for j in range(nv):
        bm.faces.new((outer[0][j + 1], inner[0][j + 1], inner[0][j], outer[0][j]))
        bm.faces.new((outer[nu][j], inner[nu][j], inner[nu][j + 1], outer[nu][j + 1]))
    o = L.obj_from_bm(bm, P.coll)
    return P.add(o, mat, smooth=smooth, matrix=matrix)


def moss_sheet(P, mat, objs, cover=0.45, thick=0.08, cell=0.25, scale=1.2, seed=0, min_nz=0.45, bump=0.05,
               occluders=None, creep=0.0, creep_nz=-0.2):
    """Moss grown on the up-facing, sky-visible skin of `objs` (already placed). The skin is copied, subdivided
    to `cell`, and every vertex is lifted by a smooth field: positive inside the noise mask, slightly negative
    (under the skin) outside it. The visible outline is where the field crosses zero, so it is a soft organic
    contour instead of the old face-cut polygons. Returns (object, list of thick points for mushrooms)."""
    off1 = Vector((seed * 1.7, seed * 0.3, seed * 2.9))
    off2 = Vector((seed * 0.4, seed * 3.1, 7.0))

    def mask(c):
        return (0.62 * noise.noise(c / scale + off1) + 0.3 * noise.noise(c * 2.7 / scale + off2)
                + 0.14 * noise.noise(c * 9.0 / scale + off1 * 0.5))       # frayed fringe
    thr = (0.5 - cover) * 0.6
    occ = _bvh(occluders or objs)
    bm = bmesh.new()
    for o in objs:
        mw = o.matrix_world
        rot = mw.to_3x3()
        me = o.data
        vmap = {}
        for p in me.polygons:
            nw = (rot @ p.normal).normalized()
            c = mw @ p.center
            if nw.z < min_nz - 0.2 and not (c.z < creep and nw.z > creep_nz):
                continue
            if mask(c) < thr - 0.25:
                continue
            h = occ.ray_cast(c + nw * 0.03 + Vector((0, 0, 0.02)), Vector((0, 0, 1)), 30.0)
            if h[0] is not None:
                continue
            vs = []
            for vi in p.vertices:
                if vi not in vmap:
                    vmap[vi] = bm.verts.new(mw @ me.vertices[vi].co)
                vs.append(vmap[vi])
            try:
                bm.faces.new(vs)
            except ValueError:
                pass
    if not bm.faces:
        bm.free()
        return None, []
    for _ in range(7):
        long_e = [e for e in bm.edges if e.calc_length() > cell]
        if not long_e:
            break
        bmesh.ops.subdivide_edges(bm, edges=long_e, cuts=1, use_grid_fill=True)
    bm.normal_update()
    lift = {}
    for v in bm.verts:
        n = v.normal if v.normal.length > 0 else Vector((0, 0, 1))
        up = smoothstep(min_nz - 0.15, min_nz + 0.1, n.z)
        val = mask(v.co) - thr
        if creep > 0.0 and n.z > creep_nz:
            g = smoothstep(creep, creep * 0.25, v.co.z)
            up = max(up, g)
            val += 0.35 * g
        lf = thick * smoothstep(-0.02, 0.32, val) * 1.1 - 0.012 - (1.0 - up) * thick * 1.6
        if lf > 0:
            lf += bump * (noise.noise(v.co * 3.0 / max(scale, 0.3) + off2) * 0.5 + 0.5) * smoothstep(0.05, 0.35, val)
            lf += bump * 0.5 * (noise.noise(v.co * 11.0 + off1) * 0.5 + 0.5) * smoothstep(0.0, 0.2, val)
        lift[v] = max(-0.025, lf)
    dead = [f for f in bm.faces if all(lift[v] <= 0.0 for v in f.verts)]
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    thickpts = []
    for v in bm.verts:
        n = v.normal if v.normal.length > 0 else Vector((0, 0, 1))
        v.co += n * lift[v]
        if lift[v] > thick * 0.8 and n.z > 0.6:
            thickpts.append(v.co.copy())
    if not bm.faces:
        bm.free()
        return None, []
    o = L.obj_from_bm(bm, P.coll)
    P.add(o, mat, smooth=70)
    return o, thickpts


def glow_mushrooms(P, m, pts, rng, clusters, stem_mat=None, size=1.0):
    """Clusters of small bioluminescent mushrooms (cyan / violet caps) on thick moss points."""
    if not pts:
        return
    for k in range(clusters):
        c = pts[rng.randrange(len(pts))]
        for j in range(rng.randint(3, 6)):
            off = Vector((rng.uniform(-0.16, 0.16), rng.uniform(-0.16, 0.16), 0.0)) * size
            hgt = rng.uniform(0.06, 0.2) * size
            base = c + off - Vector((0, 0, 0.06))
            stem = L.cyl(P.coll, 0.016 * size, hgt + 0.06, segments=6,
                         center=tuple(base + Vector((0, 0, (hgt + 0.06) * 0.5))))
            P.add(stem, stem_mat or m["vine"], smooth=50)
            r = rng.uniform(0.04, 0.085) * size
            cap = L.lathe(P.coll, [(0.0, r), (0.025, r * 0.78), (0.05, 0.0)], segments=10,
                          center=tuple(base + Vector((0, 0, hgt + 0.04))))
            P.add(cap, m["glow"] if (k + j) % 3 else m["vglow"], smooth=50)


def berm(P, mat, pts, width, height, seed=0, n_across=10):
    """A continuous mound of soil along a ground path (pts: Vectors, z ignored): noisy width / height that taper
    at both ends, edges 3 cm under the ground. Replaces rows of identical blobs."""
    pts = [Vector((p.x, p.y, 0.0)) for p in pts]
    off = Vector((seed * 1.3, seed * 0.7, 5.0))
    rings = []
    for i, p in enumerate(pts):
        a = pts[max(0, i - 1)]
        b = pts[min(len(pts) - 1, i + 1)]
        t = (b - a).normalized()
        nrm = Vector((-t.y, t.x, 0.0))
        s = i / (len(pts) - 1)
        taper = max(0.08, math.sin(math.pi * s)) ** 0.5
        w = width * (0.8 + 0.4 * (noise.noise(p * 0.6 + off) * 0.5 + 0.5)) * (0.6 + 0.4 * taper)
        h = height * (0.7 + 0.6 * (noise.noise(p * 0.45 + off * 2.0) * 0.5 + 0.5)) * taper
        ring = []
        for j in range(n_across + 1):
            u = -1.0 + 2.0 * j / n_across
            bump = 0.12 * h * noise.noise(p * 2.3 + nrm * u * 2.0 + off)
            z = h * max(0.0, 1.0 - u * u) ** 1.1 + (bump if abs(u) < 0.85 else 0.0) - 0.03
            ring.append(tuple(p + nrm * (u * w) + Vector((0.0, 0.0, z))))
        rings.append(ring)
    bm = ring_loft(P.coll, rings, cap0=True, cap1=True)
    return P.add(L.obj_from_bm(bm, P.coll), mat, smooth=60)


def vines(P, m, tree, rng, x_range, y_half, z_top, count, radius=0.06, pod_every=3):
    """Vines draped over the top and hanging down one flank, hugging the surface (ray-cast), wavy, with glowing
    pods and leaf clusters."""
    def surf(x, y):
        h = tree.ray_cast(Vector((x, y, z_top + 5.0)), Vector((0, 0, -1)))
        return h[0]

    def side_hit(x, z, s):
        h = tree.ray_cast(Vector((x, s * (y_half + 3.0), z)), Vector((0, -s, 0)))
        return h[0]
    for k in range(count):
        x0 = rng.uniform(*x_range)
        pts = []
        for j in range(15):
            t = j / 14.0
            y = (-0.8 + 1.6 * t) * y_half
            x = x0 + (1.6 if k % 2 else -1.6) * (t - 0.5) + 0.35 * math.sin(t * 9.0 + k)
            h = surf(x, y)
            if h is not None:
                pts.append(Vector((h.x, h.y, h.z + radius * 0.6)))
        if len(pts) < 4:
            continue
        s = 1 if k % 2 else -1
        end = pts[-1] if s > 0 else pts[0]
        tail = []
        z, x = end.z, end.x
        for j in range(9):
            z -= rng.uniform(0.3, 0.5)
            x += rng.uniform(-0.15, 0.15)
            if z < 0.05:
                break
            h = side_hit(x, z, s)
            if h is None:
                break
            tail.append(Vector((h.x, h.y + s * radius * 0.6, h.z)))
        path = pts + tail if s > 0 else list(reversed(tail)) + pts
        r = radius * rng.uniform(0.8, 1.2)
        P.add(L.tube_path(P.coll, path, r, sides=6, radius_fn=lambda t: 0.75 + 0.35 * abs(math.sin(t * 13.0))),
              m["vine"], smooth=50)
        for j in range(1, len(path) - 1, pod_every):
            pod = L.lathe(P.coll, [(0.0, 0.0), (0.04, 0.06), (0.11, 0.055), (0.16, 0.0)], segments=8,
                          center=tuple(path[j] + Vector((0, 0, -0.02))))
            P.add(pod, m["glow"] if (j + k) % 2 else m["vglow"], smooth=50)
        for j in range(2, len(path) - 1, 3):
            for q in range(3):
                d = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.2, 0.8))).normalized()
                leaf = L.lathe(P.coll, [(0.0, 0.0), (0.05, 0.06), (0.18, 0.045), (0.27, 0.0)], segments=6)
                rot = d.to_track_quat("Z", "Y").to_matrix().to_4x4()
                P.add(leaf, m["moss"], smooth=50, matrix=T(*path[j]) @ rot @ Matrix.Diagonal((1.0, 0.3, 1.0, 1.0)))


def yt(c, th):
    c = max(0.0, min(1.0, c))
    return 5.0 * th * (0.2969 * math.sqrt(c) - 0.126 * c - 0.3516 * c * c + 0.2843 * c ** 3 - 0.1015 * c ** 4)


def airfoil_ring(n, c0=0.0, c1=1.0):
    """(chord fraction, side) pairs around a closed airfoil outline: upper c0 -> c1, lower c1 -> c0."""
    cs = [c0 + (c1 - c0) * (0.5 - 0.5 * math.cos(math.pi * i / n)) for i in range(n + 1)]
    ring = [(c, 1) for c in cs]
    low = list(reversed(cs))
    if c0 <= 1e-9:
        low = low[:-1]
    ring += [(c, -1) for c in low]
    return ring


# =========================================================================== hull

HALF = [(0.0, 1.0), (0.3, 0.97), (0.6, 0.85), (0.84, 0.56), (1.0, 0.14), (0.95, -0.12), (0.76, -0.47),
        (0.48, -0.8), (0.2, -0.97), (0.0, -1.0)]
_HL = [0.0]
for _a, _b in zip(HALF, HALF[1:]):
    _HL.append(_HL[-1] + math.hypot(_b[0] - _a[0], _b[1] - _a[1]))
S_CHINE = _HL[4] / _HL[-1]


def half_pt(s):
    d = max(0.0, min(1.0, s)) * _HL[-1]
    for i in range(len(HALF) - 1):
        if d <= _HL[i + 1] + 1e-9:
            t = (d - _HL[i]) / max(_HL[i + 1] - _HL[i], 1e-9)
            a, b = HALF[i], HALF[i + 1]
            return a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
    return HALF[-1]


def hull_prof(x):
    """half width, half height, centre z of the fuselage at station x (stern -13 .. nose +13)."""
    w, h, zc = 3.95, 2.55, 0.0
    if x > 2.0:
        u = min(1.0, (x - 2.0) / 11.0)
        w = max(0.12, 3.95 * (1.0 - u ** 2.0) ** 0.55)
        h = max(0.12, 2.55 * (1.0 - u ** 1.7) ** 0.55)
        zc = -0.6 * u * u
    elif x < -7.0:
        v = min(1.0, (-7.0 - x) / 6.0)
        w = 3.95 - 0.5 * v
        h = 2.55 - 0.3 * v
        zc = 0.25 * v
    return w, h, zc


def hull_S(x, v):
    w, h, zc = hull_prof(x)
    side = 1.0 if v >= 0 else -1.0
    y, z = half_pt(abs(v))
    return Vector((x, side * y * w, zc + z * h))


def hull_top(x):
    w, h, zc = hull_prof(x)
    return zc + h


def _stations(spec):
    xs = []
    for a, b, step in spec:
        n = max(1, int(round((b - a) / step)))
        for i in range(n):
            xs.append(a + (b - a) * i / n)
    xs.append(spec[-1][1])
    return xs


def build_wreck_hull_r9(P):
    rng = random.Random(931)
    m = wreck_mats("Hull")
    m["hull"] = L.role_paint("Hull_Hull", "#D9D2C1", "#BDB5A2", metallic=0.3, rough=0.45, wear=0.6,
                             wear_color="#9AA2AA", fade=0.35, fade_color="#A9B0A0", grime=0.55,
                             panel=(2.1, 0.02), scale=1.5, seed=21, scorch=0.6)
    m["livery"] = L.role_paint("Hull_Livery", "#19C3B1", "#12938A", metallic=0.3, rough=0.4, wear=0.6,
                               fade=0.3, fade_color="#7FB3AA", grime=0.35, scale=1.5, seed=22, scorch=0.25)
    m["glass"] = L.role_glow("Hull_Canopy", "#2A8FA8", level=0.55, base="#081820")
    NS = 16
    xs = _stations([(-13.0, -9.4, 0.12), (-9.4, 9.0, 0.42), (9.0, 13.0, 0.3)])
    rings = []
    for x in xs:
        w, h, zc = hull_prof(x)
        right = [half_pt(k / NS) for k in range(NS + 1)]
        left = [(-y, z) for y, z in reversed(right[1:-1])]
        rings.append([(x, y * w, zc + z * h) for y, z in right + left])
    bm = ring_loft(P.coll, rings, cap0=False, cap1=True)

    def tear_x(a):
        return (-13.0 + 0.45 + 1.15 * (0.5 + 0.5 * math.sin(2.0 * a + 0.6)) + 0.55 * (0.5 + 0.5 * math.sin(5.0 * a + 1.3))
                + 0.18 * math.sin(11.0 * a + 0.2))

    def ang(p):
        _, _, zc = hull_prof(p.x)
        return math.atan2(p.y, p.z - zc)
    dead = [f for f in bm.faces if f.calc_center_median().x < tear_x(ang(f.calc_center_median()))]
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    smooth_boundary(bm, lambda v: v.co.x < -9.0, iters=5, amount=0.55)
    for v in bm.verts:
        if v.is_boundary and v.co.x < -9.0:
            v.co.x += rng.uniform(-0.04, 0.04)
    solidify_in(bm, 0.14, lambda c: Vector((c.x, 0.0, hull_prof(c.x)[2])))
    skin = L.obj_from_bm(bm, P.coll)
    # crash pose: nose (+X) pitched 5 deg into the ground, rolled 12 deg so the intact starboard wing rises
    pose = T(0, 0, 1.15) @ R(6.0, "Y") @ R(8.0, "X")
    P.add(skin, m["hull"], smooth=32, matrix=pose)
    hint = lambda u: Vector((u, 0.0, hull_prof(u)[2]))
    # --- livery: chine stripe both sides, pinstripe above it, nose band, dark spine deck panels
    for side in (1, -1):
        a0, a1 = side * (S_CHINE - 0.085), side * (S_CHINE - 0.035)
        shell_patch(P, m["livery"], hull_S, -9.0, 10.6, 90, min(a0, a1), max(a0, a1), 2, hint, matrix=pose)
        a0, a1 = side * (S_CHINE - 0.14), side * (S_CHINE - 0.12)
        shell_patch(P, m["livery"], hull_S, -5.5, 8.2, 70, min(a0, a1), max(a0, a1), 1, hint, matrix=pose)
    shell_patch(P, m["livery"], hull_S, 9.4, 10.0, 6, -0.55, 0.55, 40, hint, matrix=pose)
    shell_patch(P, m["dark"], hull_S, 8.2, 8.5, 3, -0.45, 0.45, 30, hint, matrix=pose)
    # port boarding hatch (1.2 x 1.8 m) with hazard-teal edging
    lo_, hi_ = -(S_CHINE - 0.09), -(S_CHINE - 0.27)
    shell_patch(P, m["dark"], hull_S, -3.6, -2.4, 8, lo_, hi_, 8, hint, matrix=pose, off=0.006, thk=0.02)
    for (u0, u1, v0, v1) in ((-3.75, -3.6, lo_ - 0.02, hi_ + 0.02), (-2.4, -2.25, lo_ - 0.02, hi_ + 0.02),
                             (-3.75, -2.25, lo_ - 0.02, lo_), (-3.75, -2.25, hi_, hi_ + 0.02)):
        shell_patch(P, m["livery"], hull_S, u0, u1, 3, v0, v1, 3, hint, matrix=pose, off=0.006, thk=0.03)
    # --- dorsal spine ridge from the canopy back to the tail
    srings = []
    sx = _stations([(-11.4, 3.6, 0.5)])
    for i, x in enumerate(sx):
        t = i / (len(sx) - 1)
        ht = 0.45 * min(1.0, t / 0.06, (1.0 - t) / 0.12 + 0.15)
        z = hull_top(x) - 0.12
        srings.append([(x, -0.5, z), (x, 0.5, z), (x, 0.26, z + 0.12 + ht), (x, -0.26, z + 0.12 + ht)])
    P.add(L.obj_from_bm(ring_loft(P.coll, srings), P.coll), m["dark"], bevel=0.03, segments=2, angle=30, matrix=pose)
    # torn tail-fin root on the spine (the fin itself is SM_Verd_Wreck_Fin)
    zt = hull_top(-9.3) + 0.35
    fin = [(-11.2, 0.0), (-6.9, 0.0), (-7.5, 0.45), (-8.0, 0.78), (-8.35, 0.95), (-8.6, 1.32), (-9.0, 1.5),
           (-9.45, 1.62), (-9.8, 1.9), (-10.3, 1.98), (-10.75, 1.7), (-11.05, 1.85)]
    prism(P, m["hull"], [(x, z + zt - 0.15) for x, z in fin], "Y", -0.2, 0.2, bevel=0.03, seg=2, matrix=pose)
    prism(P, m["livery"], [(-10.95, zt + 0.05), (-7.45, zt + 0.05), (-7.7, zt + 0.3), (-10.95, zt + 0.3)], "Y",
          -0.215, 0.215, bevel=0.01, seg=1, matrix=pose)
    # --- cockpit canopy, a third of the way back from the nose: glass bubble, dark coaming, frame bows
    cx, ca, cb, cc = 5.4, 2.0, 0.95, 0.72
    zc0 = hull_top(cx) - 0.22
    prof = [(ca * (-1.0 + 2.0 * i / 16.0), cb * max(0.0, 1.0 - (-1.0 + 2.0 * i / 16.0) ** 2) ** 0.45) for i in range(17)]
    glass = L.lathe(P.coll, prof, segments=28, axis="X")
    P.add(glass, m["glass"], smooth=50, matrix=pose @ T(cx, 0, zc0) @ Matrix.Diagonal((1.0, 1.0, cc / cb, 1.0)))
    prof2 = [((ca + 0.25) * (-1.0 + 2.0 * i / 16.0), (cb + 0.14) * max(0.0, 1.0 - (-1.0 + 2.0 * i / 16.0) ** 2) ** 0.45)
             for i in range(17)]
    coam = L.lathe(P.coll, prof2, segments=28, axis="X")
    P.add(coam, m["dark"], smooth=40, matrix=pose @ T(cx, 0, zc0 - 0.1) @ Matrix.Diagonal((1.0, 1.0, 0.36, 1.0)))
    for dxb in (-0.75, 0.45):
        s = max(0.0, 1.0 - (dxb / ca) ** 2) ** 0.45
        pts = []
        for j in range(13):
            a = math.radians(8 + 164 * j / 12.0)
            pts.append(Vector((cx + dxb, math.cos(a) * (cb * s + 0.02), zc0 + math.sin(a) * (cc * s + 0.02))))
        P.add(L.tube_path(P.coll, pts, 0.045, sides=6), m["dark"], smooth=40, matrix=pose)
    spine_pts = [Vector((cx + ca * t, 0.0, zc0 + cc * max(0.0, 1.0 - t * t) ** 0.45 + 0.02)) for t in
                 [-0.85 + 1.6 * k / 10.0 for k in range(11)]]
    P.add(L.tube_path(P.coll, spine_pts, 0.04, sides=6), m["dark"], smooth=40, matrix=pose)
    # --- wings: intact swept starboard stub (torn tip), torn-off port stub with spars sticking out
    def wgeo(y):
        t = abs(y) - 3.5
        return 1.3 - 0.8 * t, 7.4 - 0.75 * t, 0.6 - 0.07 * t, 0.3 + 0.2 * t      # x LE, chord, thickness, z (dihedral)

    def wing(side, y_end, tear_amp):
        ys = _stations([(3.5, y_end - 0.7, 0.3), (y_end - 0.7, y_end + 0.05, 0.08)])
        rings_w = []
        for y in ys:
            xle, ch, th, zw = wgeo(y)
            rings_w.append([(xle - c * ch, side * y, zw + sg * yt(c, th)) for c, sg in airfoil_ring(14)])
        bmw = ring_loft(P.coll, rings_w, cap0=True, cap1=False)

        def ty(xc, up):
            return y_end - tear_amp * (0.5 + 0.5 * math.sin(xc * 1.25 + (0.4 if up else 2.1))) \
                - 0.25 * (0.5 + 0.5 * math.sin(xc * 3.3 + (1.0 if up else 0.2)))
        dead_w = []
        for f in bmw.faces:
            c = f.calc_center_median()
            if abs(c.y) > ty(c.x, c.z > wgeo(c.y)[3]):
                dead_w.append(f)
        bmesh.ops.delete(bmw, geom=dead_w, context="FACES")
        bmesh.ops.delete(bmw, geom=[v for v in bmw.verts if not v.link_faces], context="VERTS")
        smooth_boundary(bmw, lambda v: abs(v.co.y) > y_end - tear_amp - 0.6, iters=5, amount=0.55)
        solidify_in(bmw, 0.05, lambda c: Vector((c.x, c.y, wgeo(c.y)[3])))
        wo = L.obj_from_bm(bmw, P.coll)
        P.add(wo, m["hull"], smooth=30, matrix=pose)
        # spars (dark I-beams) and ribs, poking out of the torn tip
        for cfr in (0.22, 0.6):
            ga, gb = wgeo(3.6), wgeo(y_end)
            hgt = 2 * yt(cfr, 0.6) * 0.85
            prof_i = L.ibeam_profile(hgt, 0.16, 0.03, 0.03)
            yb = y_end + (0.1 if cfr < 0.5 else 0.05)
            f = (yb - 3.6) / (y_end - 3.6)
            pa = Vector((ga[0] - cfr * ga[1], side * 3.6, ga[3]))
            pb = Vector((gb[0] - cfr * gb[1], side * y_end, gb[3]))
            pe = pa + (pb - pa) * f - Vector((0, 0, 0.15 if side < 0 else 0.0))
            o, mm = L.bar(P.coll, pa, pe, 0.16, hgt, up=(0, 0, 1), profile=prof_i)
            P.add(o, m["metal"], bevel=0.008, segments=1, matrix=pose @ mm)
        for k, yr in enumerate((y_end - 0.55, y_end - 0.2)):
            xle, ch, th, zw = wgeo(yr)
            pts_r = [(xle - c * ch, zw + sg * yt(c, th) * 0.9) for c, sg in airfoil_ring(10, 0.04, 0.75)]
            prism(P, m["dark"], pts_r, "Y", side * yr - 0.025, side * yr + 0.025, bevel=None, matrix=pose)
        return wo
    wing_s = wing(1, 4.55, 0.3)
    wing_p = wing(-1, 4.35, 0.35)
    # wing livery band near the starboard leading edge (upper skin)
    def wS(y, c):
        xle, ch, th, zw = wgeo(y)
        return Vector((xle - c * ch, y, zw + yt(c, th)))
    shell_patch(P, m["livery"], wS, 3.95, 4.2, 4, 0.1, 0.17, 3, lambda u: Vector((-2.0, u, wgeo(u)[3])), matrix=pose)
    shell_patch(P, m["livery"], wS, 3.95, 4.2, 4, 0.2, 0.22, 1, lambda u: Vector((-2.0, u, wgeo(u)[3])), matrix=pose)
    # --- stern torn open: bulkhead with a door frame, broken ribs, conduits spilling out
    bw, bh, bz = hull_prof(-9.9)
    ring_b = [(y * bw * 0.96, bz + z * bh * 0.96) for y, z in
              [half_pt(k / 12.0) for k in range(13)] + [(-y, z) for y, z in reversed([half_pt(k / 12.0) for k in range(1, 12)])]]
    prism(P, m["dark"], ring_b, "X", -10.05, -9.85, bevel=0.02, seg=1, matrix=pose)
    bx(P, m["metal"], (-10.12, -0.75, bz - 1.0), (-10.0, 0.75, bz + 1.2), bevel=0.02, seg=1, matrix=pose)
    bx(P, m["dark"], (-10.16, -0.55, bz - 0.95), (-10.05, 0.55, bz + 1.05), bevel=0.015, seg=1, matrix=pose)
    bx(P, m["glow"], (-10.18, 0.62, bz + 0.6), (-10.1, 0.7, bz + 0.85), bevel=None, matrix=pose)
    for xr, a0, a1 in ((-12.35, 40, 150), (-11.55, -20, 205), (-10.75, -60, 250)):
        w, h, zc = hull_prof(xr)
        pts = []
        for j in range(16):
            s = (a0 + (a1 - a0) * j / 15.0) / 360.0
            sv = (s * 2.0) % 2.0 - 1.0 if False else None
            a = math.radians(a0 + (a1 - a0) * j / 15.0)
            # walk the section by angle: map angle to the half outline
            side = 1.0 if math.cos(a) >= 0 else -1.0
            sh = min(1.0, max(0.0, (math.pi * 0.5 - math.asin(max(-1.0, min(1.0, math.sin(a))))) / math.pi))
            y, z = half_pt(sh)
            pts.append(Vector((xr, side * y * w * 0.93, zc + z * h * 0.93)))
        P.add(L.tube_path(P.coll, pts, 0.08, sides=6), m["metal"], smooth=40, matrix=pose)
    for k, (y0, z0) in enumerate(((-0.9, 0.2), (0.4, 0.5), (1.2, -0.3))):
        p0 = Vector((-10.0, y0, bz + z0))
        p3 = Vector((-12.9, y0 * 1.3 + 0.3, bz - 2.6))
        pts = L.bezier_points(p0, p0 + Vector((-1.4, 0, 0.1)), p3 + Vector((0.4, 0, 1.4)), p3, 10)
        P.add(L.tube_path(P.coll, pts, 0.07 + 0.02 * k, sides=7), m["dark"], smooth=45, matrix=pose)
    # peeled skin plates bent outward at the torn stern rim
    for k, v in enumerate((0.12, -0.38, 0.62, -0.7, 0.36)):
        pv = hull_S(-11.0, v)
        xt = tear_x(ang(pv))
        u1 = xt + 0.3
        u0 = max(-12.95, u1 - rng.uniform(0.9, 1.4))
        dv = 0.08 + 0.03 * k
        hinge = hull_S(u1, v)
        tv = (hull_S(u1, v + 0.01) - hull_S(u1, v - 0.01)).normalized()
        peel = T(*hinge) @ Matrix.Rotation(math.radians(rng.choice((-1, 1)) * rng.uniform(25, 40)), 4, tv) @ T(*(-hinge))
        shell_patch(P, m["hull"], hull_S, u0, u1, 6, v - dv, v + dv, 4, hint, matrix=pose @ peel, off=-0.12, thk=0.13)
    # --- stern pylon flanges where the engine pod tore off
    for side in (-1, 1):
        prism(P, m["dark"], [(-0.9, 0.0), (1.1, 0.0), (0.7, 0.75), (-0.5, 0.75)], "X", -12.2, -9.0, bevel=0.04,
              seg=2, matrix=pose @ T(0, side * 2.0, hull_top(-10.5) - 0.55) @ R(side * 30, "X"))
    # --- ground: continuous soil berms along both flanks (higher toward the nose) and a ploughed nose mound
    world = [skin, wing_s, wing_p]
    tree = _bvh(world)
    soil = m["soil"]
    for side in (-1, 1):
        pts = []
        x = -12.6
        while x <= 12.4:
            h = tree.ray_cast(Vector((x, side * 8.0, 0.25)), Vector((0, -side, 0)), 10.0)
            if h[0] is not None:
                pts.append(Vector((x, h[0].y - side * 0.3, 0.0)))
            x += 0.45
        for a, b, wd, ht, sd in ((0, len(pts) // 2 + 2, 0.95, 0.9, 11), (len(pts) // 2 - 2, len(pts), 1.05, 1.3, 12)):
            seg = pts[a:b]
            if len(seg) >= 4:
                berm(P, soil, seg, wd, ht, seed=sd + side * 5)
    arc = [Vector((10.4 + 1.1 * math.cos(t), 2.6 * math.sin(t), 0.0)) for t in
           [math.radians(-80 + 160 * k / 14.0) for k in range(15)]]
    berm(P, soil, arc, 1.4, 1.5, seed=31)
    # --- overgrowth: soft moss sheet on hull, wing and spine; mushrooms; vines over the top
    moss, thick = moss_sheet(P, m["moss"], world, cover=0.42, thick=0.15, cell=0.31, scale=3.6, seed=3,
                             min_nz=0.68, bump=0.08, occluders=world, creep=1.1)
    glow_mushrooms(P, m, thick, rng, 16)
    vines(P, m, _bvh([skin]), rng, (-8.5, 7.0), 2.6, 7.6, count=6, radius=0.065)


# =========================================================================== wing / fin pieces

def build_panel_r9(P, fin=False):
    """A torn wing (or tail fin) lying on its leading edge, partly buried, leaning. Local build frame: span along X
    (torn root at -X, tip at +X), chord up along Z (leading edge at z=0), thickness along Y. The skin is a real
    airfoil with thickness; ribs and I-beam spars show at the torn root (spars bent out of it) and, on the wing, at
    a breach in the upper skin; one control-surface section has torn off its outer hinge and hangs deflected."""
    seed = 61 if fin else 51
    rng = random.Random(seed)
    m = wreck_mats("Fin" if fin else "Wing", scale=0.6)
    m["hull"] = L.role_paint(("Fin" if fin else "Wing") + "_Hull2", "#D9D2C1", "#BDB5A2", metallic=0.3, rough=0.45,
                             wear=0.65, wear_color="#9AA2AA", fade=0.35, fade_color="#A9B0A0", grime=0.6,
                             panel=(1.1, 0.016), scale=0.6, seed=21 + (3 if fin else 0), scorch=0.8)
    m["livery"] = L.role_paint(("Fin" if fin else "Wing") + "_Livery2", "#19C3B1", "#12938A", metallic=0.3,
                               rough=0.4, wear=0.6, fade=0.3, fade_color="#7FB3AA", grime=0.35, scale=0.6, seed=22,
                               scorch=0.25)
    span = 4.1 if fin else 5.45
    c_root, c_tip = (2.75, 0.75) if fin else (3.0, 2.15)
    sweep = 1.95 if fin else 0.8
    th_root, th_tip = (0.3, 0.14) if fin else (0.36, 0.2)
    lean = 27.0 if fin else 25.0
    x0, x1 = -span * 0.5, span * 0.5
    CM = 0.78                                    # hinge line (rear spar)
    fx0, fx1 = (0.1, 1.5) if fin else (0.55, 2.2)  # the torn-off control-surface bay

    def geo(x):
        t = (x - x0) / span
        return sweep * t, c_root + (c_tip - c_root) * t, th_root + (th_tip - th_root) * t

    def S(x, c, side):
        zle, ch, th = geo(x)
        return Vector((x, side * yt(c, th), zle + c * ch))

    def cmax(x):
        return CM + 0.005 if fx0 + 0.05 < x < fx1 - 0.05 else 1.0
    xs = _stations([(x0, x0 + 1.3, 0.09), (x0 + 1.3, x1, 0.2)])
    for xf in (fx0, fx0 + 0.05, fx1 - 0.05, fx1):
        if all(abs(xf - q) > 0.03 for q in xs):
            xs.append(xf)
    xs.sort()
    rings = [[tuple(S(x, c, sg)) for c, sg in airfoil_ring(12, 0.0, cmax(x))] for x in xs]
    zle, ch, th = geo(x1)
    for dx_, k in ((0.05, 0.72), (0.08, 0.35)):
        rings.append([(x1 + dx_, sg * yt(c, th) * k, zle + c * ch) for c, sg in airfoil_ring(12, 0.0, 1.0)])
    bm = ring_loft(P.coll, rings, cap0=False, cap1=True)

    def tear(z, side):
        cz = z / c_root
        ph = 0.0 if side > 0 else 1.9
        return (x0 + 0.1 + 0.75 * (0.5 + 0.5 * math.sin(cz * 5.5 + seed + ph))
                + 0.3 * (0.5 + 0.5 * math.sin(cz * 13.0 + 2.0 * seed + ph)) + 0.35 * max(0.0, 1.0 - abs(cz - 0.55) / 0.12))
    dead = []
    for f in bm.faces:
        c = f.calc_center_median()
        if c.x < tear(c.z, 1 if c.y >= 0 else -1):
            dead.append(f)
    if not fin:
        for f in bm.faces:
            c = f.calc_center_median()
            if c.y > 0 and f not in dead:
                ex = ((c.x + 0.6) / 0.7) ** 2 + ((c.z - 1.05) / 0.42) ** 2
                if ex < 1.0 + 0.35 * noise.noise(Vector((c.x * 2.0, c.z * 2.0, 3.0))):
                    dead.append(f)
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    smooth_boundary(bm, lambda v: True, iters=6, amount=0.5)
    for v in bm.verts:
        if v.is_boundary and v.co.x < x0 + 1.4:
            v.co.x += 0.07 * noise.noise(Vector((v.co.z * 7.0, v.co.y * 3.0, seed)))
    solidify_in(bm, 0.035, lambda c: Vector((c.x, 0.0, c.z)))
    up = T(0, 0, -0.32) @ R(lean, "X")
    skin = L.obj_from_bm(bm, P.coll)
    P.add(skin, m["hull"], smooth=30, matrix=up)
    # internal structure: ribs and two I-beam spars that stick out of the torn root, bent
    ribs = [x0 + 0.3, x0 + 0.85, x0 + 1.4, -0.85, -0.35] if not fin else [x0 + 0.3, x0 + 0.85, x0 + 1.5]
    for xr in ribs:
        zle_r, ch_r, th_r = geo(xr)
        prof_r = [(yt(c, th_r) * 0.8 * sg, zle_r + c * ch_r) for c, sg in airfoil_ring(8, 0.04, CM - 0.02)]
        prism(P, m["dark"], prof_r, "X", xr - 0.018, xr + 0.018, bevel=None, matrix=up)
    for cfr, stick in ((0.25, 0.42), (CM - 0.03, 0.22)):
        zle_a, ch_a, th_a = geo(x0 + 0.2)
        zle_b, ch_b, th_b = geo(x1 - 0.25)
        hgt = 2 * yt(cfr, th_a) * 0.82
        prof_i = L.ibeam_profile(hgt, 0.1, 0.02, 0.02)
        pa = Vector((x0 + 0.2, 0.0, zle_a + cfr * ch_a))
        pb = Vector((x1 - 0.25, 0.0, zle_b + cfr * ch_b))
        o, mm = L.bar(P.coll, pa, pb, 0.1, hgt, up=(0, 1, 0), profile=prof_i)
        P.add(o, m["metal"], bevel=0.006, segments=1, matrix=up @ mm)
        pc = pa + Vector((-stick, 0.05, -0.08 - stick * 0.3))
        o, mm = L.bar(P.coll, pa + Vector((0.05, 0, 0)), pc, 0.1, hgt * 0.9, up=(0, 1, 0), profile=prof_i)
        P.add(o, m["metal"], bevel=0.006, segments=1, matrix=up @ mm)
    # hinge line groove strips on both faces, and the torn control surface hanging out of its bay
    for side in (1, -1):
        shell_patch(P, m["dark"], lambda x, c, side=side: S(x, c, side), x0 + 1.0, x1 - 0.1, 26, CM - 0.012, CM,
                    1, lambda u: Vector((u, 0.0, geo(u)[0] + 0.4 * geo(u)[1])), matrix=up, thk=0.003, off=0.0)
    cs_r = []
    for i in range(8):
        x = fx0 + 0.06 + (fx1 - fx0 - 0.12) * i / 7.0
        cs_r.append([tuple(S(x, c, sg)) for c, sg in airfoil_ring(5, CM + 0.02, 1.0)])
    bmc = ring_loft(P.coll, cs_r, cap0=True, cap1=True)
    h0 = Vector((fx0, 0.0, geo(fx0)[0] + CM * geo(fx0)[1]))
    h1 = Vector((fx1, 0.0, geo(fx1)[0] + CM * geo(fx1)[1]))
    ax = (h1 - h0).normalized()
    hang = T(*h0) @ Matrix.Rotation(math.radians(-24.0), 4, ax) @ Matrix.Rotation(math.radians(7.0), 4, Vector((0, 1, 0))) @ T(*(-h0))
    P.add(L.obj_from_bm(bmc, P.coll), m["hull"], smooth=30, matrix=up @ hang)
    zh = h0.z + 0.05
    bx(P, m["metal"], (fx0 + 0.02, -0.05, zh - 0.12), (fx0 + 0.14, 0.05, zh + 0.12), bevel=0.01, seg=1, matrix=up)
    bx(P, m["metal"], (fx0 + 0.02, -0.04, zh - 0.05), (fx0 + 0.14, 0.04, zh + 0.16), bevel=0.01, seg=1,
       matrix=up @ hang)
    # dark leading edge (wraps the nose of the airfoil) and livery on both faces
    def LE(x, v):
        return S(x, abs(v), 1 if v >= 0 else -1)
    hint = lambda u: Vector((u, 0.0, geo(u)[0] + 0.35 * geo(u)[1]))
    shell_patch(P, m["dark"], LE, x0 + 1.15, x1 - 0.05, 24, -0.06, 0.06, 8, hint, matrix=up, thk=0.015)
    for side in (1, -1):
        if fin:
            # one broad raked band and a dark unit-number block near the tip
            shell_patch(P, m["livery"], lambda x, c, side=side: S(x, min(0.76, c + 0.25 * (x - x0) / span), side),
                        x0 + 1.15, x1 - 0.5, 20, 0.22, 0.34, 2, hint, matrix=up)
            shell_patch(P, m["dark"], lambda x, c, side=side: S(x, c, side), x1 - 1.1, x1 - 0.55, 4, 0.42, 0.66, 4,
                        hint, matrix=up)
        else:
            shell_patch(P, m["livery"], lambda x, c, side=side: S(x, c, side), x0 + 1.15, x1 - 0.4, 22, 0.52, 0.6, 2,
                        hint, matrix=up)
            shell_patch(P, m["livery"], lambda x, c, side=side: S(x, c, side), 0.1, x1 - 0.4, 12, 0.64, 0.66, 1,
                        hint, matrix=up)
    # nav-light pod embedded in the tip, lens seated in it
    zle, ch, th = geo(x1)
    zp = zle + 0.14 * ch
    ps = 1.35 if fin else 1.0
    pod = L.lathe(P.coll, [(-0.3, 0.0), (-0.2, 0.065), (0.05, 0.08), (0.16, 0.055), (0.2, 0.0)], segments=12, axis="X")
    P.add(pod, m["dark"], smooth=40, matrix=up @ T(x1 + 0.03, 0, zp) @ Matrix.Diagonal((ps, ps, ps, 1.0)))
    lens = L.lathe(P.coll, [(0.1, 0.0), (0.12, 0.058), (0.2, 0.048), (0.25, 0.0)], segments=12, axis="X")
    P.add(lens, m["glow"], smooth=40, matrix=up @ T(x1 + 0.03, 0, zp) @ Matrix.Diagonal((ps, ps, ps, 1.0)))
    if fin:
        # fin-tip antenna blade, seated in the tip cap
        prism(P, m["dark"], [(x1 - 0.7, 0.0), (x1 - 0.1, 0.0), (x1 - 0.25, 0.32), (x1 - 0.55, 0.3)], "Y", -0.025, 0.025,
              bevel=0.008, seg=1, matrix=up @ T(0, 0, zle + ch - 0.06))
    # ground line: soil heaped against the buried leading edge on both sides, moss sheet, mushrooms
    yg = -0.32 * math.tan(math.radians(lean))
    for sd in (-1, 1):
        pts = [Vector((x0 + 0.05 + (span - 0.1) * k / 16.0, yg + sd * 0.1, 0.0)) for k in range(17)]
        berm(P, m["soil"], pts, 0.32 if sd < 0 else 0.26, 0.42, seed=seed + sd)
    moss, thick = moss_sheet(P, m["moss"], [skin], cover=0.36, thick=0.06, cell=0.12, scale=1.3, seed=seed,
                             min_nz=0.35, bump=0.035, occluders=[skin], creep=0.75, creep_nz=-0.6)
    glow_mushrooms(P, m, [q for q in thick if q.z < 1.5], rng, 3, size=0.8)


# =========================================================================== log

def build_log_r9(P):
    rng = random.Random(912)
    m = {"bark": role_bark("Log_Bark", scale=0.6),
         "wood": L.role_paint("Log_Wood", "#6E4A5C", "#55384A", metallic=0.0, rough=0.78, wear=0.0, grime=0.5,
                              scale=0.4, seed=2),
         "rot": L.role_paint("Log_Rot", "#2A1C26", "#1C141A", metallic=0.0, rough=0.9, wear=0.0, grime=0.3,
                             scale=0.3, seed=5),
         "fungus": L.role_paint("Log_Fungus", "#7B3FE4", "#5A2DB0", metallic=0.0, rough=0.55, wear=0.0, grime=0.15,
                                scale=0.3, seed=4),
         "glow": L.role_glow("Log_FungusGlow", "#3CFFE0", level=0.85, base="#103A36"),
         "vglow": L.role_glow("Log_VioletGlow", "#B07CFF", level=0.75, base="#2A1650")}
    m["moss"] = wreck_mats("LogM", scale=0.4)["moss"]
    m["vine"] = m["bark"]
    NA = 26
    LZ = 0.58                                  # trunk centre height (top ~1.15 m, fills the 1.4 m box with the flare)

    def prof(x, a):
        f = smoothstep(-1.5, -2.5, x)                     # root flare toward -X
        ry, rz = 0.43 * (1.0 + 0.08 * f), 0.55 * (1.0 + 0.12 * f)
        lobes = 1.0 + f * 0.16 * max(0.0, math.cos(5.0 * a + 0.4)) ** 2
        bark = 1.0 + 0.04 * noise.noise(Vector((math.cos(a) * 2.6, math.sin(a) * 2.6, x * 0.5))) \
            + 0.025 * math.sin(a * 13.0 + x * 0.6 + 0.9 * math.sin(x * 2.0))
        yc = 0.05 * math.sin(x * 0.7)
        zc = LZ + 0.04 * math.sin(x * 0.9) + 0.05 * f
        r = lobes * bark
        return Vector((x, yc + math.cos(a) * ry * r, zc + math.sin(a) * rz * r))
    xs = _stations([(-2.5, 1.5, 0.2), (1.5, 2.5, 0.07)])
    rings = [[tuple(prof(x, math.tau * k / NA)) for k in range(NA)] for x in xs]
    # rounded root end: two shrinking rings
    base_ring = rings[0]
    for dx_, k in ((0.025, 0.85), (0.045, 0.6), (0.055, 0.3)):
        rings.insert(0, [(-2.5 - dx_, p[1] * k, LZ + 0.1 + (p[2] - LZ - 0.1) * k) for p in base_ring])
    bm = ring_loft(P.coll, rings, cap0=True, cap1=False)

    def tear(a):
        return 2.5 - 0.1 - 0.3 * (0.5 + 0.5 * math.sin(3.0 * a + 1.0)) - 0.32 * abs(math.sin(4.5 * a + 0.3)) ** 4

    def angle(p):
        return math.atan2(p.z - LZ, p.y)
    dead = [f for f in bm.faces if f.calc_center_median().x > tear(angle(f.calc_center_median()))]
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    smooth_boundary(bm, lambda v: v.co.x > 1.4, iters=3, amount=0.4)
    solidify_in(bm, 0.08, lambda c: Vector((c.x, 0.05 * math.sin(c.x * 0.7), LZ)))
    trunk = L.obj_from_bm(bm, P.coll)
    P.add(trunk, m["bark"], smooth=45)
    # hollow: pale wood lining, rotten dark back wall, splinters around the rim, glowing mushrooms inside
    lin = []
    for x in _stations([(1.55, 2.2, 0.1)]):
        lin.append([(x, prof(x, math.tau * k / NA).y * 0.8, LZ + (prof(x, math.tau * k / NA).z - LZ) * 0.8)
                    for k in range(NA)])
    bml = ring_loft(P.coll, lin, cap0=True, cap1=False)
    solidify_in(bml, 0.03, lambda c: Vector((c.x, 0.0, LZ)))
    P.add(L.obj_from_bm(bml, P.coll), m["wood"], smooth=45)
    P.add(L.lathe(P.coll, [(1.55, 0.0), (1.6, 0.25), (1.62, 0.3)], segments=18, axis="X"), m["rot"], smooth=45,
          matrix=T(0, 0, LZ) @ Matrix.Diagonal((1.0, 1.0, 1.2, 1.0)))
    from pk_geo import shard
    for k in range(13):
        a = math.tau * k / 13 + rng.uniform(-0.15, 0.15)
        xt = tear(a)
        p = prof(xt - 0.12, a)
        base = Vector((xt - 0.2, 0.0, LZ)) + (p - Vector((p.x, 0.0, LZ))) * 0.95
        d = Vector((1.0, math.cos(a) * 0.18, math.sin(a) * 0.18))
        shard(P, m["wood"], base, d, rng.uniform(0.03, 0.06), rng.uniform(0.18, 0.3), sides=4, tip=0.65, seed=k)
    inner = [Vector((rng.uniform(1.7, 2.1), rng.uniform(-0.1, 0.1), LZ - 0.42)) for _ in range(4)]
    glow_mushrooms(P, m, inner, rng, 4, stem_mat=m["wood"], size=1.15)
    # roots: tapered tendrils from the flare down into the ground
    for k, ad in enumerate((-50, -25, -5, 185, 205, 230)):
        a = math.radians(ad + rng.uniform(-6, 6))
        xr = -2.25 + rng.uniform(-0.1, 0.25)
        p0 = prof(xr, a)
        out = Vector((0.0, math.cos(a), 0.0)).normalized()
        p1 = p0 + out * 0.08 + Vector((-0.08, 0, -0.1))
        p1.y = max(-0.47, min(0.47, p1.y))
        p2 = Vector((xr - 0.15, max(-0.48, min(0.48, p1.y)), -0.12))
        P.add(L.tube_path(P.coll, [p0 - out * 0.08, p1, p2], 0.065, sides=7,
                          radius_fn=lambda t: 1.0 - 0.6 * t), m["bark"], smooth=50)
    # broken branch stubs
    for (xb, ab, ln) in ((-0.4, 60, 0.32), (0.9, 120, 0.24)):
        p0 = prof(xb, math.radians(ab))
        d = Vector((0.35, math.cos(math.radians(ab)) * 0.5, 0.8)).normalized()
        bend_ = Vector((0.12, 0.0, -0.04))
        pts = [p0 - d * 0.08, p0 + d * ln * 0.5, p0 + d * ln + bend_]
        P.add(L.tube_path(P.coll, pts, 0.09, sides=9, radius_fn=lambda t: 1.0 - 0.35 * t), m["bark"], smooth=50)
        shard(P, m["wood"], p0 + d * (ln - 0.02), d, 0.05, 0.14, sides=4, tip=0.7, seed=int(xb * 10) + 40)
    # tiered shelf fungus clusters on the flanks (the focal points: x -0.9 starboard, x 0.7 port, x 1.4 starboard)
    tree = _bvh([trunk])
    for ci, (xf, side, zf) in enumerate(((-0.9, 1, 0.62), (0.65, -1, 0.58), (1.35, 1, 0.5))):
        for j, (rr, dz) in enumerate(((0.13, 0.0), (0.1, 0.11), (0.075, 0.2))):
            xx = xf + rng.uniform(-0.08, 0.08)
            h = tree.ray_cast(Vector((xx, side * 2.0, zf + dz)), Vector((0, -side, 0)), 3.0)
            if h[0] is None:
                continue
            cap = L.lathe(P.coll, [(0.0, rr), (0.035, rr * 0.92), (0.06, rr * 0.55), (0.07, 0.0)], segments=16,
                          arc=(-95.0, 95.0))
            mt = T(h[0].x, h[0].y - side * 0.06, h[0].z - 0.03) @ R(90 * side, "Z") @ R(-6 * side, "X")
            P.add(cap, m["fungus"], smooth=50, matrix=mt)
            gill = L.lathe(P.coll, [(-0.004, rr * 0.9), (0.0, 0.0)], segments=16, arc=(-92.0, 92.0))
            P.add(gill, m["glow"] if (ci + j) % 2 == 0 else m["vglow"], smooth=50, matrix=mt)
    # moss sheet on top, mushrooms in it
    moss, thick = moss_sheet(P, m["moss"], [trunk], cover=0.48, thick=0.045, cell=0.12, scale=0.9, seed=7,
                             min_nz=0.45, bump=0.03, occluders=[trunk], creep=0.25)
    glow_mushrooms(P, m, thick, rng, 6, stem_mat=m["wood"], size=1.3)
    # low ground skirt of leaf litter / soil hugging the base on both sides
    for sd in (-1, 1):
        pts = [Vector((-2.2 + 4.3 * k / 12.0, sd * 0.3 + 0.05 * math.sin(k * 1.3), 0.0)) for k in range(13)]
        berm(P, m["rot"], pts, 0.17, 0.15, seed=70 + sd, n_across=6)


# =========================================================================== collapse slab (columnar basalt)

def _clip(bm, planes):
    for co, no in planes:
        geom = list(bm.verts) + list(bm.edges) + list(bm.faces)
        res = bmesh.ops.bisect_plane(bm, geom=geom, plane_co=co, plane_no=no, clear_outer=True)
        ce = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
        if ce:
            try:
                bmesh.ops.holes_fill(bm, edges=ce, sides=0)
            except Exception:
                pass


def build_collapse_slab_r9(P):
    """Columnar basalt bridge slab: Voronoi cells of a jittered hex lattice (5-7 sided columns, 0.4-0.7 m) whose
    tops are the walkable surface at 60 cm (settled 0-1.8 cm per piece), sides clipped flush to the box. Four
    pieces split along the column joints by deep dark cracks; amber / cyan crystals grow down in the cracks below
    the walking surface; magma glows only at the lava line (bottom 14 cm of the sides, cracked basalt)."""
    m = {"basalt": role_basalt("Slab_Basalt", scale=0.6, seed=5, crack=1.0, crack_zmax=0.14),
         "cyan": L.role_glow("Slab_CrystalC", "#2EE6FF", level=0.85, base="#0B2A33"),
         "amber": L.role_glow("Slab_CrystalA", "#FFB347", level=0.85, base="#3A2408")}
    rng = random.Random(277)
    X, Y, Z = 6.1, 3.2, 0.6
    r = 0.42
    dx, dy = r * 1.5, r * math.sqrt(3.0)
    cs = []
    i = 0
    x = -X * 0.5 - r * 0.2
    while x < X * 0.5 + r:
        y = -Y * 0.5 - r * 0.3 + (dy * 0.5 if i % 2 else 0.0)
        while y < Y * 0.5 + r:
            cs.append(Vector((x + rng.uniform(-0.13, 0.13), y + rng.uniform(-0.13, 0.13), 0.0)))
            y += dy
        x += dx
        i += 1
    bounds = [-1.55, 0.05, 1.62]

    def piece(c):
        return sum(1 for b in bounds if c.x > b + 0.35 * math.sin(c.y * 1.7 + b * 3.0))
    pid = [piece(c) for c in cs]
    sink = [0.0, 0.012, 0.005, 0.017]
    crack_pts = []
    box_planes = [((X * 0.5, 0, 0), (1, 0, 0)), ((-X * 0.5, 0, 0), (-1, 0, 0)), ((0, Y * 0.5, 0), (0, 1, 0)),
                  ((0, -Y * 0.5, 0), (0, -1, 0))]
    for k, c in enumerate(cs):
        top = Z - sink[pid[k]] - rng.uniform(0.0, 0.004)
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation((c.x, c.y, top * 0.5)) @
                              Matrix.Diagonal((2.0, 2.0, top, 1.0)))
        planes = []
        for j, n in enumerate(cs):
            if j == k or (n - c).length > 1.5:
                continue
            d = (n - c).normalized()
            g = 0.022 if pid[j] != pid[k] else 0.004
            mid = (c + n) * 0.5
            planes.append((tuple(mid - d * g), tuple(d)))
            if pid[j] != pid[k] and j > k:
                crack_pts.append(mid)
        _clip(bm, planes + box_planes)
        if len(bm.faces) < 4 or len(bm.verts) < 6:
            bm.free()
            continue
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        o = L.obj_from_bm(bm, P.coll)
        chip = rng.random() < 0.2
        P.add(o, m["basalt"], bevel=0.022 if chip else 0.014, segments=2 if chip else 1, angle=25)
    # filler under the joints (cracks stay dark and deep: its top is 18 cm below the walking surface)
    bx(P, m["basalt"], (-X * 0.5 + 0.01, -Y * 0.5 + 0.01, 0.0), (X * 0.5 - 0.01, Y * 0.5 - 0.01, Z - 0.18), bevel=None)
    # crystals growing up in the cracks, tips 3+ cm under the walking surface
    rng.shuffle(crack_pts)
    placed = 0
    for cpt in crack_pts:
        if placed >= 5 or not (-X * 0.5 + 0.4 < cpt.x < X * 0.5 - 0.4 and -Y * 0.5 + 0.4 < cpt.y < Y * 0.5 - 0.4):
            continue
        mat = m["amber"] if placed % 2 else m["cyan"]
        for j in range(3):
            d = Vector((rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25), 1.0)).normalized()
            base = Vector((cpt.x, cpt.y, Z - 0.22)) + Vector((rng.uniform(-0.02, 0.02), rng.uniform(-0.02, 0.02), 0))
            shard(P, mat, base, d, rng.uniform(0.012, 0.02), rng.uniform(0.12, 0.17), sides=5, tip=0.35,
                  seed=900 + placed * 7 + j)
        placed += 1


def build_debris_r9(P):
    """Physics debris: four fused basalt column shards with flat fracture planes, ash on top faces, magma cracks,
    one small amber crystal cluster; largest dimension 150 cm, pivot at the centre."""
    rng = random.Random(119)
    m = role_basalt("Debris_Basalt2", scale=0.3, crack=0.8, seed=9, crack_color="#FF5A1F")
    amb = L.role_glow("Debris_Crystal", "#FFB347", level=0.85, base="#3A2408")
    specs = [((0.0, 0.0), 0.3, 1.35, (8, 4)), ((0.42, 0.18), 0.25, 1.1, (-12, 9)), ((-0.36, 0.26), 0.27, 1.0, (10, -14)),
             ((0.08, -0.42), 0.23, 0.9, (-6, -10))]
    for k, ((cx, cy), rr, ln, (tx, ty)) in enumerate(specs):
        bm = bmesh.new()
        prof = [(math.cos(math.radians(60 * i + 13 * k)) * rr * rng.uniform(0.9, 1.08),
                 math.sin(math.radians(60 * i + 13 * k)) * rr * rng.uniform(0.9, 1.08)) for i in range(6)]
        bot = [bm.verts.new((u, v, -ln * 0.5 - 0.3)) for u, v in prof]
        top = [bm.verts.new((u, v, ln * 0.5 + 0.3)) for u, v in prof]
        for i in range(6):
            bm.faces.new((bot[i], bot[(i + 1) % 6], top[(i + 1) % 6], top[i]))
        bm.faces.new(list(reversed(bot)))
        bm.faces.new(top)
        n1 = Vector((rng.uniform(-0.5, 0.5), rng.uniform(-0.5, 0.5), 1.0)).normalized()
        n2 = Vector((rng.uniform(-0.5, 0.5), rng.uniform(-0.5, 0.5), -1.0)).normalized()
        _clip(bm, [((0, 0, ln * 0.5), tuple(n1)), ((0, 0, -ln * 0.5), tuple(n2))])
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        o = L.obj_from_bm(bm, P.coll)
        P.add(o, m, bevel=0.02, segments=2, angle=25, matrix=T(cx, cy, 0.0) @ R(tx, "X") @ R(ty, "Y") @ R(35, "X"))
    for j in range(3):
        d = Vector((rng.uniform(-0.4, 0.4), rng.uniform(0.3, 0.8), rng.uniform(0.2, 0.7))).normalized()
        shard(P, amb, Vector((0.0, 0.0, 0.0)), d, rng.uniform(0.03, 0.05), rng.uniform(0.45, 0.56), seed=60 + j)
    # uniform scale so the largest dimension is 1.5 m
    lo = Vector((1e9, 1e9, 1e9))
    hi = Vector((-1e9, -1e9, -1e9))
    for o in P.parts:
        for v in o.data.vertices:
            w = o.matrix_world @ v.co
            lo = Vector((min(lo.x, w.x), min(lo.y, w.y), min(lo.z, w.z)))
            hi = Vector((max(hi.x, w.x), max(hi.y, w.y), max(hi.z, w.z)))
    s = 1.5 / max(hi - lo)
    for o in P.parts:
        o.matrix_world = Matrix.Diagonal((s, s, s, 1.0)) @ o.matrix_world


# =========================================================================== engine pod

def ring_x(coll, x0, x1, r0, r1, segs=40):
    return L.revolve_closed(coll, [(r0, x0), (r1, x0), (r1, x1), (r0, x1)], (0.0, 360.0), segs, axis="Z")


def place_x(m=None):
    return (m or Matrix.Identity(4)) @ R(90, "Y")


def build_wreck_engine_r9(P):
    """The engine pod that tore off the hull: nacelle with a flared intake lip and broken fan, a jagged breach on
    top showing the drum and internal rings, a petalled nozzle. It lies nozzle-down, pitched 10 deg and rolled,
    1.2 m deep in the soil (the r7 pod sat on the ground like a barrel), with soil berms, a soft moss sheet that
    creeps up from the ground, thin vines and a torn pylon stub."""
    rng = random.Random(941)
    m = wreck_mats("Engine", scale=0.8)
    m["vine"] = role_bark("Engine_Vine2", scale=0.3, seed=3)
    sx = 0.88
    zc = 3.5
    prof = [(-4.2, 2.75), (-4.15, 3.05), (-3.9, 3.28), (-3.3, 3.42), (-1.0, 3.5), (1.6, 3.42), (2.8, 3.05),
            (3.6, 2.75), (3.95, 2.62)]
    prof = [(x * sx, r) for x, r in prof]
    segs = 48
    dense = []
    for (xa, ra), (xb, rb) in zip(prof, prof[1:]):
        steps = max(1, int(abs(xb - xa) / 0.3))
        for s_ in range(steps):
            t = s_ / float(steps)
            x = xa + (xb - xa) * t
            r = ra + (rb - ra) * t
            dense.append([(x, r * math.cos(math.tau * k / segs), zc + r * math.sin(math.tau * k / segs)) for k in range(segs)])
    x, r = prof[-1]
    dense.append([(x, r * math.cos(math.tau * k / segs), zc + r * math.sin(math.tau * k / segs)) for k in range(segs)])
    bm = ring_loft(P.coll, dense, cap0=False, cap1=False)

    def edge_at(a):
        return 2.0 + 0.8 * math.sin(a * 0.11) + 0.25 * math.sin(a * 0.37)

    x_end = prof[-1][0]

    def shear(a):          # exhaust end sheared off on a slant with a ragged rim
        return x_end - 0.1 - 1.15 * (0.5 + 0.5 * math.cos(math.radians(a) - 1.0)) - 0.12 * math.sin(math.radians(a) * 7.0)
    dead = []
    for f in bm.faces:
        c = f.calc_center_median()
        a = math.degrees(math.atan2(c.z - zc, c.y))
        if 28 < a < 128 and -1.3 + 0.45 * math.sin(a * 0.2) < c.x < edge_at(a):
            dead.append(f)
        elif c.x > shear(a):
            dead.append(f)
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    smooth_boundary(bm, lambda v: v.co.x > -2.0, iters=4, amount=0.5)
    for v in bm.verts:
        if v.is_boundary and -1.6 < v.co.x < 3.0:
            d = Vector((0, v.co.y, v.co.z - zc)).normalized()
            v.co -= d * (0.08 + 0.1 * (noise.noise(v.co * 2.5) * 0.5 + 0.5))      # torn lips bent inward
    solidify_in(bm, 0.16, lambda c: Vector((c.x, 0.0, zc)))
    shell = L.obj_from_bm(bm, P.coll)
    pose = R(10.0, "Y") @ R(14.0, "X")
    hard = []
    hard.append(P.add(shell, m["hull"], smooth=32, matrix=pose))
    x_in, x_out = prof[0][0], prof[-1][0]
    hard.append(P.add(ring_x(P.coll, -0.28, 0.28, 2.55, 3.18), m["dark"], bevel=0.03, segments=2,
                      matrix=pose @ T(x_in, 0, zc) @ place_x()))
    hard.append(P.add(ring_x(P.coll, -0.22, 0.22, 3.46 - 0.06, 3.46 + 0.035), m["livery"], bevel=0.01, segments=1,
                      matrix=pose @ T(-2.9 * sx, 0, zc) @ place_x()))
    band2 = L.revolve_closed(P.coll, [(3.3 - 0.06, -0.22), (3.3 + 0.035, -0.22), (3.3 + 0.035, 0.22), (3.3 - 0.06, 0.22)],
                             (-140.0, 95.0), 30, axis="Z")
    hard.append(P.add(band2, m["livery"], bevel=0.01, segments=1, matrix=pose @ T(0.9 * sx, 0, zc) @ place_x()))
    hard.append(P.add(L.lathe(P.coll, [(-3.6 * sx, 1.3), (-2.0 * sx, 1.75), (2.2 * sx, 1.65), (3.4 * sx, 1.1),
                                       (3.9 * sx, 0.0)], segments=28, axis="X"), m["dark"], smooth=35,
                      matrix=pose @ T(0, 0, zc)))
    hard.append(P.add(L.lathe(P.coll, [(x_in + 0.1, 0.0), (x_in + 0.25, 0.5), (x_in + 0.55, 0.85)], segments=24,
                              axis="X"), m["metal"], smooth=40, matrix=pose @ T(0, 0, zc)))
    for k in range(18):
        a = 360.0 * k / 18
        if 40 < a < 150 and k % 3:
            continue
        ln = 2.3 if k % 5 else (0.9 if k % 2 else 1.4)
        o = L.box(P.coll, (0.06, 0.36, ln), (0, 0, 0))
        hard.append(P.add(o, m["metal"], bevel=0.01, segments=1,
                          matrix=pose @ T(x_in + 0.5, 0, zc) @ R(a, "X") @ T(0, 0, 0.6 + ln * 0.5) @ R(28, "Z")))
    hard.append(P.add(ring_x(P.coll, -0.06, 0.06, 2.15, 2.35), m["glow"], matrix=pose @ T(x_out - 0.5, 0, zc) @ place_x()))
    for k in range(10):
        o = L.box(P.coll, (0.5, 0.07, 0.95), (0, 0, 0))
        hard.append(P.add(o, m["dark"], bevel=0.01, segments=1,
                          matrix=pose @ T(x_out - 0.65, 0, zc) @ R(36 * k, "X") @ T(0, 0, 1.72)))
    for x in (-0.9 * sx, 0.3 * sx, 1.5 * sx):
        hard.append(P.add(ring_x(P.coll, -0.09, 0.09, 3.05, 3.3), m["metal"], bevel=0.01, segments=1,
                          matrix=pose @ T(x, 0, zc) @ place_x()))
    for k in range(3):
        a = math.radians(70 + 20 * k)
        pts = [Vector((x, math.cos(a) * 2.0, zc + math.sin(a) * 2.0)) for x in (-1.5, -0.5, 0.5, 1.5, 2.3)]
        hard.append(P.add(L.tube_path(P.coll, pts, 0.09, sides=8), m["metal"], smooth=40, matrix=pose))
    # torn pylon stub: a swept blade with a ragged top and two spar ends sticking out
    pyl = [(-2.3, 0.0), (1.4, 0.0), (1.0, 0.55), (0.55, 0.75), (0.1, 0.62), (-0.4, 0.95), (-1.0, 0.8),
           (-1.5, 1.05), (-1.95, 0.7)]
    hard.append(prism(P, m["dark"], [(x * sx, z) for x, z in pyl], "Y", -0.22, 0.22, bevel=0.03, seg=2,
                      matrix=pose @ T(0, -2.5, zc + 2.2) @ R(-42, "X")))
    for xs_ in (-1.2, 0.3):
        o, mm = L.bar(P.coll, (xs_ * sx, 0.0, 0.5), (xs_ * sx + 0.15, 0.05, 1.45), 0.1, 0.16)
        hard.append(P.add(o, m["metal"], bevel=0.008, segments=1, matrix=pose @ T(0, -2.5, zc + 2.2) @ R(-42, "X") @ mm))
    # sink it: top at exactly 7.0 m, the rest goes into the soil (1.2 m deep)
    top = max((o.matrix_world @ v.co).z for o in hard for v in o.data.vertices)
    sh = T(0, 0, 7.0 - top)
    for o in P.parts:
        o.matrix_world = sh @ o.matrix_world
    # soil: a mound around the buried side and a ploughed berm in front of the nozzle
    tree = _bvh(hard)
    for side in (-1, 1):
        pts = []
        x = -4.3
        while x <= 4.3:
            h = tree.ray_cast(Vector((x, side * 6.0, 0.35)), Vector((0, -side, 0)), 8.0)
            if h[0] is not None:
                pts.append(Vector((x, h[0].y + side * 0.15, 0.0)))
            x += 0.4
        if len(pts) >= 4:
            berm(P, m["soil"], pts, 1.1, 1.05, seed=40 + side)
    arc = [Vector((3.4 + 0.9 * math.cos(t), 2.6 * math.sin(t), 0.0)) for t in
           [math.radians(-80 + 160 * k / 12.0) for k in range(13)]]
    berm(P, m["soil"], arc, 0.8, 0.9, seed=47)
    moss, thick = moss_sheet(P, m["moss"], [hard[0]], cover=0.38, thick=0.12, cell=0.22, scale=2.4, seed=5,
                             min_nz=0.5, bump=0.07, occluders=hard, creep=1.6)
    glow_mushrooms(P, m, thick, rng, 8)
    vines(P, m, _bvh([hard[0]]), rng, (-3.0, 2.6), 3.3, 7.4, count=4, radius=0.06)


# =========================================================================== new: relay altar, prototype console

def build_relay_altar(P):
    """SM_Verd_RelayAltar (200 x 200 x 150 cm): the relay's awakening pedestal on the plateau. Two masonry steps,
    an octagonal drum with glyph channels and a carved glyph band, a 2 x 2 m top slab (walkable, flat at 150 cm)
    laid as four stones round a funnel socket, the glowing glyph core held by four stone claws below the top,
    glyph rings on the top face. Same stone, glyph and moss language as the relay platforms."""
    from pk_verdance import stone_mats, GLYPH
    from pk_geo import ngon
    rng = random.Random(1501)
    H = 1.5
    m = stone_mats("Altar", H, scale=0.5, rings=[(0.0, 0.0, 0.66), (0.0, 0.0, 0.84)], moss=0.6)
    m["core"] = L.role_glow("Altar_Core", GLYPH, level=1.0, base="#0E3A3A")
    bx(P, m["stone"], (-1.0, -1.0, 0.0), (1.0, 1.0, 0.26), bevel=0.04, seg=2)
    bx(P, m["stone"], (-0.88, -0.88, 0.26), (0.88, 0.88, 0.44), bevel=0.035, seg=2)
    oct_r = 0.74 / math.cos(math.radians(22.5))
    prism(P, m["stone"], ngon(oct_r, 8, 22.5), "Z", 0.44, 1.24, bevel=0.03, seg=2)
    # carved band with glyph ticks, and glowing channels down the eight drum faces
    prism(P, m["band"], ngon(0.765 / math.cos(math.radians(22.5)), 8, 22.5), "Z", 0.98, 1.1, bevel=0.012, seg=1)
    for k in range(8):
        mtx = R(45 * k, "Z")
        P.add(L.box(P.coll, (0.012, 0.05, 0.42), (0.742, 0.0, 0.7)), m["glyph"], matrix=mtx)
        for j in range(3):
            P.add(L.box(P.coll, (0.008, 0.035, 0.05 + 0.03 * ((j + k) % 2)), (0.768, -0.18 + 0.18 * j, 1.04)),
                  m["glyph"], matrix=mtx)
    # corner quoin blocks on the lower step (break the square silhouette)
    for sx in (-1, 1):
        for sy in (-1, 1):
            bx(P, m["band"], (sx * 0.98 - (0.22 if sx > 0 else 0.0), sy * 0.98 - (0.22 if sy > 0 else 0.0), 0.0),
               (sx * 0.98 + (0.0 if sx > 0 else 0.22), sy * 0.98 + (0.0 if sy > 0 else 0.22), 0.34), bevel=0.03, seg=2)
    # top slab: four stones round a 0.7 m square opening, top exactly at H
    for lo, hi in (((-1.0, -1.0), (1.0, -0.35)), ((-1.0, 0.35), (1.0, 1.0)), ((-1.0, -0.35), (-0.35, 0.35)),
                   ((0.35, -0.35), (1.0, 0.35))):
        bx(P, m["stone"], (lo[0] + 0.004, lo[1] + 0.004, H - 0.26), (hi[0] - 0.004, hi[1] - 0.004, H), bevel=0.028, seg=2)
    funnel = L.revolve_closed(P.coll, [(0.2, H - 0.15), (0.5, H - 0.03), (0.5, H - 0.28), (0.2, H - 0.28)],
                              (0.0, 360.0), 40, axis="Z")
    P.add(funnel, m["band"], bevel=0.01, segments=1, angle=35)
    core = L.lathe(P.coll, [(H - 0.42, 0.0), (H - 0.36, 0.14), (H - 0.25, 0.2), (H - 0.13, 0.17), (H - 0.05, 0.09),
                            (H - 0.03, 0.0)], segments=20)
    P.add(core, m["core"], smooth=60)
    for k in range(4):
        prism(P, m["stone"], [(0.17, H - 0.4), (0.27, H - 0.3), (0.25, H - 0.06), (0.19, H - 0.14)], "Y", -0.035, 0.035,
              bevel=0.01, seg=1, matrix=R(45 + 90 * k, "Z"))
    # a few moss clumps on the lower step corners (kept off the walkable top)
    moss = wreck_mats("AltarM", scale=0.4)["moss"]
    for k, (x, y) in enumerate(((0.8, -0.75), (-0.82, 0.7), (0.6, 0.86))):
        _moss_blob(P, moss, (x, y, 0.27), rng.uniform(0.14, 0.2), 0.06, 1600 + k)


def build_prototype_console(P):
    """SM_Cind_PrototypeConsole (160 x 160 x 140 cm): the outpost's prototype console in Null Armada language: a
    blade-cut armoured base with forward-raked corner spikes, crimson side plates, a raked console head whose
    sloped face carries a glowing red-orange screen (scan-line bars), two white eye lights, and the prototype: a
    caged glowing cell standing behind the head (the focal point). Operator side +X."""
    from pk_cinder import null_mats
    from pk_geo import cham_rect, blade_rect
    m = null_mats("Proto", scale=0.5)
    m["screen"] = L.role_glow("Proto_Screen", "#FF6A3A", level=0.6, base="#2A0A06", pattern="bars", scale=0.15)
    m["cell"] = L.role_glow("Proto_Cell", "#FFC9A0", level=0.7, base="#3A2A20")
    m["matte"] = L.role_paint("Proto_Matte", "#26262E", "#1A1A20", metallic=0.25, rough=0.62, wear=0.3,
                              wear_color="#6A6E78", grime=0.3, panel=(0.6, 0.01), scale=0.5, bump=0.3, seed=11)
    # base plinth (blade-cut octagon) and inner column
    prism(P, m["trim"], cham_rect(-0.8, 0.8, -0.8, 0.8, 0.26), "Z", 0.0, 0.16, bevel=0.02, seg=2)
    prism(P, m["armour"], cham_rect(-0.62, 0.62, -0.62, 0.62, 0.2), "Z", 0.16, 0.3, bevel=0.02, seg=2)
    prism(P, m["armour"], cham_rect(-0.42, 0.42, -0.5, 0.5, 0.12), "Z", 0.3, 0.95, bevel=0.02, seg=2)
    for s in (-1, 1):
        bx(P, m["seam"], (-0.4, s * 0.62 - 0.012, 0.2), (0.4, s * 0.62 + 0.012, 0.23), bevel=None)
    # crimson blade plates on the column sides and front
    for s in (-1, 1):
        pl = blade_rect(-0.34, 0.34, 0.38, 0.88, 0.14)
        prism(P, m["crimson"], [(u, v) for u, v in pl], "Y", s * 0.5 - 0.025, s * 0.5 + 0.025, bevel=0.008, seg=1)
    prism(P, m["crimson"], [(-0.4, 0.4), (0.4, 0.4), (0.4, 0.82), (0.0, 0.9), (-0.4, 0.82)], "X", 0.4, 0.45,
          bevel=0.008, seg=1)
    for s in (-1, 1):
        bx(P, m["eye"], (0.445, s * 0.17 - 0.07, 0.72), (0.455, s * 0.17 + 0.07, 0.75), bevel=None)
    # forward-raked spikes at the four base corners
    for sx in (-1, 1):
        for sy in (-1, 1):
            prof = [(-0.12, 0.0), (0.12, 0.0), (0.22, 0.62), (0.05, 0.3)]
            prism(P, m["armour"], prof, "Y", -0.035, 0.035, bevel=0.008, seg=1,
                  matrix=T(sx * 0.58, sy * 0.58, 0.14) @ R(math.degrees(math.atan2(sy, sx)), "Z"))
    # console head: raked wedge, sloped face toward +X carries the screen
    head = [(-0.62, 0.9), (0.72, 0.92), (0.8, 1.04), (-0.3, 1.27), (-0.62, 1.22)]
    prism(P, m["matte"], head, "Y", -0.72, 0.72, bevel=0.02, seg=2)
    for s in (-1, 1):
        prism(P, m["crimson"], [(-0.55, 0.95), (0.6, 0.96), (0.66, 1.03), (-0.25, 1.21), (-0.55, 1.17)], "Y",
              s * 0.72 - 0.02, s * 0.72 + 0.02, bevel=0.006, seg=1)
    a, b = Vector((0.8, 1.04)), Vector((-0.3, 1.27))
    d = (b - a).normalized()
    n = Vector((-d.y, d.x))
    if n.y < 0:
        n = -n
    p0, p1 = a + d * 0.22, a + d * ((b - a).length - 0.3)
    scr = [tuple(p0 - n * 0.01), tuple(p1 - n * 0.01), tuple(p1 + n * 0.012), tuple(p0 + n * 0.012)]
    prism(P, m["screen"], scr, "Y", -0.4, 0.4, bevel=None)
    frame = [tuple(a + d * 0.08 - n * 0.01), tuple(a + d * ((b - a).length - 0.08) - n * 0.01),
             tuple(a + d * ((b - a).length - 0.08) + n * 0.022), tuple(a + d * 0.08 + n * 0.022)]
    for s in (-1, 1):
        prism(P, m["trim"], frame, "Y", s * 0.6 - 0.04, s * 0.6 + 0.04, bevel=0.005, seg=1)
    # the prototype: a caged glowing cell standing behind the head
    cx = -0.5
    P.add(L.cyl(P.coll, 0.2, 0.08, segments=8, center=(cx, 0.0, 1.24)), m["trim"], bevel=0.01, segments=1)
    P.add(L.cyl(P.coll, 0.12, 0.3, segments=16, center=(cx, 0.0, 1.43 - 0.15 - 0.04)), m["cell"], smooth=40)
    for k in range(6):
        ang = math.radians(60 * k + 30)
        o, mm = L.bar(P.coll, (cx + math.cos(ang) * 0.17, math.sin(ang) * 0.17, 1.26),
                      (cx + math.cos(ang) * 0.15, math.sin(ang) * 0.15, 1.4), 0.03, 0.03)
        P.add(o, m["crimson"], bevel=0.005, segments=1, matrix=mm)
    P.add(L.cyl(P.coll, 0.19, 0.03, segments=8, center=(cx, 0.0, 1.385)), m["trim"], bevel=0.008, segments=1)
    # cables from the column back into the head
    for s in (-1, 1):
        pts = L.bezier_points(Vector((-0.42, s * 0.3, 0.45)), Vector((-0.7, s * 0.34, 0.55)),
                              Vector((-0.72, s * 0.3, 0.85)), Vector((-0.55, s * 0.28, 0.98)), 8)
        P.add(L.tube_path(P.coll, pts, 0.035, sides=7), m["trim"], smooth=40)


def _with_cut(fn):
    def run(P):
        fn(P)
        _cut_all_below(P)
    return run


BUILDERS = {
    "SM_Verd_Wreck_Hull": dict(fn=_with_cut(build_wreck_hull_r9), family="verdance", dims=(26.0, 9.0, 7.0), tex=2048,
                               budget=(2000, 40000), ao=1.5, snap=True),
    "SM_Verd_Wreck_Wing": dict(fn=_with_cut(lambda P: build_panel_r9(P, False)), family="verdance",
                               dims=(6.0, 1.6, 2.4), tex=1024, budget=(1000, 10000), ao=0.5, snap=True),
    "SM_Verd_Wreck_Fin": dict(fn=_with_cut(lambda P: build_panel_r9(P, True)), family="verdance",
                              dims=(5.0, 1.6, 2.2), tex=1024, budget=(1000, 10000), ao=0.5, snap=True),
    "SM_Verd_Wreck_Engine": dict(fn=_with_cut(build_wreck_engine_r9), family="verdance", dims=(9.0, 7.0, 7.0),
                                 tex=2048, budget=(2000, 40000), ao=1.0, snap=True),
    "SM_Verd_RelayAltar": dict(fn=build_relay_altar, family="verdance", dims=(2.0, 2.0, 1.5), tex=1024,
                               budget=(1000, 10000), ao=0.4, detail=True),
    "SM_Cind_PrototypeConsole": dict(fn=build_prototype_console, family="cinder", dims=(1.6, 1.6, 1.4), tex=1024,
                                     budget=(1000, 10000), ao=0.4),
    "SM_Verd_Log": dict(fn=_with_cut(build_log_r9), family="verdance", dims=(5.0, 1.1, 1.4), tex=1024,
                        budget=(2000, 10000), ao=0.3, snap=True),
    "SM_Cind_CollapseSlab": dict(fn=build_collapse_slab_r9, family="cinder", dims=(6.1, 3.2, 0.6), tex=1024,
                                 budget=(500, 10000), ao=0.5, detail=True),
    "SM_Cind_DebrisChunk": dict(fn=build_debris_r9, family="cinder", dims=(None, None, None), tex=1024,
                                budget=(500, 10000), ao=0.4, pivot="centre", fit_axes=(False, False, False)),
}
