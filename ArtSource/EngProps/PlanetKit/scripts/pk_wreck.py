# SPDX-License-Identifier: MIT
"""
pk_wreck.py - round 3 of the Verdance wreck: the hull section and the engine pod rebuilt after the first review
("reads as a quonset hut / a barrel"). The hull now has a chined ship section (flat dorsal deck, shoulder chines,
keel), tapers to a stern block, dips nose-down and rolls into the ground, has a dorsal spine stub, a torn wing
root, a full-length livery stripe on the chine, a hatch with hazard edging; the torn end is irregular (jittered).
Moss is grown ON the skin (up-facing faces copied, lifted, lumpy, thick) instead of flat discs, and vines hang
down the flanks with glowing pods. The engine pod gets a real nacelle profile (rounded intake lip, bulge,
petalled nozzle with an inner cone and a thin residual glow ring), a jagged breach on top showing the drum and
internal rings, and lies rolled and sunk into the ground.
"""

import math
import random

import bmesh
from mathutils import Matrix, Vector, noise
from mathutils.bvhtree import BVHTree

import pk_lib as L
from pk_geo import T, R, bx, prism, ring_loft
from pk_verdance import wreck_mats, _cut_all_below


def ring_x(coll, x0, x1, r0, r1, segs=40):
    """Closed ring (tube wall) around the X axis between radii r0..r1, x0..x1."""
    o = L.revolve_closed(coll, [(r0, x0), (r1, x0), (r1, x1), (r0, x1)], (0.0, 360.0), segs, axis="Z")
    return o


def place_x(m=None):
    """Matrix turning an object built around Z (axial = z) into one around X."""
    return (m or Matrix.Identity(4)) @ R(90, "Y")


SECTION = [(0.0, 1.0), (0.42, 1.0), (0.82, 0.62), (1.0, 0.18), (0.97, -0.22), (0.72, -0.72), (0.3, -1.0), (0.0, -1.0)]


def _section_ring(hw, hh, zc, x, n_per_side=14):
    """Chined ship section (mirror of SECTION), resampled to an even number of points, as an xyz ring."""
    half = SECTION
    # resample the half outline (top centre -> bottom centre) by arc length
    seg = []
    tot = 0.0
    for a, b in zip(half, half[1:]):
        d = math.hypot(b[0] - a[0], b[1] - a[1])
        seg.append((a, b, d))
        tot += d
    pts = []
    for k in range(n_per_side + 1):
        s = tot * k / n_per_side
        acc = 0.0
        for a, b, d in seg:
            if acc + d >= s - 1e-9:
                t = (s - acc) / max(d, 1e-9)
                pts.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
                break
            acc += d
    right = pts                                   # y >= 0, top -> bottom
    left = [(-y, z) for y, z in reversed(pts[1:-1])]   # bottom -> top on the other side
    ring = right + left
    return [(x, y * hw, zc + z * hh) for y, z in ring]


def _grow_moss(P, mat, skin, rng, cover=0.45, lift=0.02, thick=0.07, min_nz=0.45, scale=1.6, seed=0):
    """Copy the skin's up-facing faces where a noise mask says so, lift them, make them lumpy and give them
    thickness: moss that hugs the hull."""
    me = skin.data
    mw = skin.matrix_world
    bm = bmesh.new()
    vmap = {}
    for p in me.polygons:
        nw = (mw.to_3x3() @ p.normal).normalized()
        c = mw @ p.center
        if nw.z < min_nz:
            continue
        nval = noise.noise(c * (1.0 / scale) + Vector((seed, 0, 0)))
        if nval < 1.0 - 2.0 * cover:
            continue
        vs = []
        for vi in p.vertices:
            if vi not in vmap:
                co = mw @ me.vertices[vi].co
                vmap[vi] = bm.verts.new(co)
            vs.append(vmap[vi])
        try:
            bm.faces.new(vs)
        except ValueError:
            pass
    if not bm.faces:
        bm.free()
        return None
    bmesh.ops.subdivide_edges(bm, edges=list(bm.edges), cuts=2, use_grid_fill=True)
    bm.normal_update()
    # organic outline: jitter the patch boundary in the surface plane
    for v in bm.verts:
        if v.is_boundary:
            j = Vector((noise.noise(v.co * 3.1 + Vector((seed, 1, 0))), noise.noise(v.co * 3.1 + Vector((0, seed, 2))), 0.0))
            n = v.normal if v.normal.length > 0 else Vector((0, 0, 1))
            j -= n * j.dot(n)
            v.co += j * 0.35
    for v in bm.verts:
        n = v.normal if v.normal.length > 0 else Vector((0, 0, 1))
        bump = (noise.noise(v.co * 1.7 + Vector((0, seed, 0))) * 0.5 + 0.5) * 0.09
        v.co += n * (lift + (0.0 if v.is_boundary else bump))
    # boundary verts tucked down onto the hull so the patch edges blend
    for v in bm.verts:
        if v.is_boundary:
            v.co -= (v.normal if v.normal.length > 0 else Vector((0, 0, 1))) * (lift + 0.01)
    bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=thick)
    o = L.obj_from_bm(bm, P.coll)
    return P.add(o, mat, smooth=60)


def _hang_vines(P, m, trees, rng, x_range, y_half, z_top, count, pod_every=3):
    def surf(x, y):
        best = None
        for t in trees:
            h = t.ray_cast(Vector((x, y, z_top + 5.0)), Vector((0, 0, -1)))
            if h[0] is not None and (best is None or h[0].z > best.z):
                best = h[0]
        return best

    def side_hit(x, y, z, s):
        best = None
        for t in trees:
            h = t.ray_cast(Vector((x, s * (y_half + 3.0), z)), Vector((0, -s, 0)))
            if h[0] is not None and (best is None or abs(h[0].y) > abs(best.y)):
                best = h[0]
        return best
    for k in range(count):
        x0 = rng.uniform(*x_range)
        pts = []
        # over the top
        for j in range(11):
            t = j / 10.0
            y = (-0.75 + 1.5 * t) * y_half
            x = x0 + (2.4 if k % 2 else -2.4) * (t - 0.5) + 0.4 * math.sin(t * 5.0 + k)
            h = surf(x, y)
            if h is not None:
                pts.append(Vector((h.x, h.y, min(h.z + 0.05, z_top - 0.08))))
        if len(pts) < 4:
            continue
        # hang down one flank, hugging the side
        s = 1 if k % 2 else -1
        tail = []
        z = (pts[-1] if s > 0 else pts[0]).z
        x = (pts[-1] if s > 0 else pts[0]).x
        for j in range(8):
            z -= rng.uniform(0.35, 0.6)
            if z < 0.05:
                break
            h = side_hit(x, 0.0, z, s)
            if h is None:
                break
            tail.append(Vector((h.x, h.y + s * 0.05, h.z)))
        path = pts + tail if s > 0 else list(reversed(tail)) + pts
        r = rng.uniform(0.09, 0.14)
        P.add(L.tube_path(P.coll, path, r, sides=7, radius_fn=lambda t: 0.8 + 0.4 * abs(math.sin(t * 9.0))),
              m["vine"], smooth=50)
        for j in range(1, len(path) - 1, pod_every):
            pod = L.lathe(P.coll, [(0.0, 0.0), (0.05, 0.08), (0.14, 0.075), (0.2, 0.0)], segments=8,
                          center=tuple(path[j] + Vector((0, 0, -0.03))))
            P.add(pod, m["glow"] if (j + k) % 2 else m["vglow"], smooth=50)
        # leaf / frond clusters (small flattened shards) along the vine
        for j in range(2, len(path) - 1, 4):
            for q in range(3):
                d = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.2, 0.8))).normalized()
                leaf = L.lathe(P.coll, [(0.0, 0.0), (0.06, 0.07), (0.22, 0.05), (0.32, 0.0)], segments=6)
                rot = d.to_track_quat("Z", "Y").to_matrix().to_4x4()
                P.add(leaf, m["moss"], smooth=50, matrix=T(*path[j]) @ rot @ Matrix.Diagonal((1.0, 0.35, 1.0, 1.0)))


def _glow_caps(P, m, moss, rng, count):
    """Clusters of small bioluminescent mushrooms on top of the moss patches (cyan / violet caps)."""
    if moss is None:
        return
    mw = moss.matrix_world
    tops = [mw @ v.co for v in moss.data.vertices]
    tops = [p for p in tops]
    tops.sort(key=lambda p: -p.z)
    pool = tops[: max(10, len(tops) // 2)]
    for k in range(count):
        c = pool[rng.randrange(len(pool))]
        for j in range(rng.randint(2, 4)):
            off = Vector((rng.uniform(-0.18, 0.18), rng.uniform(-0.18, 0.18), 0.0))
            hgt = rng.uniform(0.08, 0.2)
            base = c + off - Vector((0, 0, 0.05))
            stem = L.cyl(P.coll, 0.018, hgt + 0.05, segments=6, center=tuple(base + Vector((0, 0, (hgt + 0.05) * 0.5))))
            P.add(stem, m["vine"], smooth=50)
            r = rng.uniform(0.05, 0.09)
            cap = L.lathe(P.coll, [(0.0, r), (0.03, r * 0.75), (0.055, 0.0)], segments=10,
                          center=tuple(base + Vector((0, 0, hgt + 0.03))))
            P.add(cap, m["glow"] if (k + j) % 3 else m["vglow"], smooth=50)


def _soil_skirt(P, m, Lx, half_w, rng):
    """Mounds of soil and roots piled along both flanks at the ground line (the hull sank into the jungle floor)."""
    from pk_verdance import _moss_blob
    soil = m.get("soil")
    for side in (-1, 1):
        x = -Lx * 0.5 + 0.8
        while x < Lx * 0.5 - 1.5:
            rad = rng.uniform(0.9, 1.6)
            _moss_blob(P, soil, (x, side * (half_w - rad * 0.5), 0.12), rad, rng.uniform(0.5, 0.75), int(x * 10) + side,
                       ry=rad * 0.5)
            x += rad * rng.uniform(1.1, 1.6)


def build_wreck_hull(P):
    rng = random.Random(31)
    m = wreck_mats("Hull")
    Lx, hw, hh = 25.0, 4.3, 4.0
    zc = 1.15
    stations = 50
    rings = []
    tear_at = {}
    for i in range(stations + 1):
        t = i / float(stations)
        x = -Lx * 0.5 + Lx * t
        # stern taper (t < 0.18) into a block, full section after; slight crumple waviness
        s = 0.72 + 0.28 * min(1.0, t / 0.18) ** 0.6
        wob = 1.0 + 0.012 * math.sin(t * 23.0)
        rings.append(_section_ring(hw * s * wob, hh * s, zc - (1.0 - s) * 0.8, x))
    bm = ring_loft(P.coll, rings, cap0=True, cap1=False)
    n = len(rings[0])
    # irregular tear at +X: continuous tear line per angle + jitter; delete faces past it
    for k in range(n):
        tear_at[k] = Lx * 0.5 - 0.6 - 2.6 * (0.5 + 0.5 * math.sin(k * 0.9 + 0.7)) * (0.55 + 0.45 * math.sin(k * 2.3))
    dead = []
    for f in bm.faces:
        c = f.calc_center_median()
        a = (math.atan2(c.y, c.z - zc) / math.tau) % 1.0
        k = int(a * n) % n
        if c.x > tear_at[k] + rng.uniform(-0.25, 0.25):
            dead.append(f)
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    for v in bm.verts:
        if v.is_boundary and v.co.x > 0:
            v.co.x += rng.uniform(-0.18, 0.18)
            v.co.z += rng.uniform(-0.08, 0.08)
    bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=0.16)
    skin = L.obj_from_bm(bm, P.coll)
    # crash pose: nose (+X) dipped 4 deg into the ground, rolled 9 deg
    pose = T(0, 0, 0.35) @ R(4.0, "Y") @ R(9.0, "X")
    P.add(skin, m["hull"], bevel=0.03, segments=1, angle=25, matrix=pose)
    mw0 = skin.matrix_world
    skin_tree = BVHTree.FromPolygons([mw0 @ v.co for v in skin.data.vertices], [p.vertices[:] for p in skin.data.polygons])

    def stick(mat, size, x, y_dir, z, lift=0.0, bevel=0.015, from_top=False, twist=0.0):
        """Ray-cast onto the posed skin and lay a thin box (size: along hull x, across, thickness) on it."""
        if from_top:
            o_, d_ = pose @ Vector((x, y_dir, 12.0)), Vector((0, 0, -1))
        else:
            o_, d_ = pose @ Vector((x, y_dir * 12.0, z)), (pose.to_3x3() @ Vector((0, -y_dir, 0))).normalized()
        h = skin_tree.ray_cast(o_, d_)
        if h[0] is None:
            return None
        nrm = h[1].normalized()
        if nrm.dot(-d_) < 0:
            nrm = -nrm
        xa = (pose.to_3x3() @ Vector((1, 0, 0))).normalized()
        xa = (xa - nrm * xa.dot(nrm)).normalized()
        ya = nrm.cross(xa).normalized()
        rot = Matrix((xa, ya, nrm)).transposed().to_4x4()
        bo = L.box(P.coll, size, (0, 0, size[2] * 0.5 - 0.03))
        return P.add(bo, mat, bevel=bevel, segments=1, matrix=T(*(h[0] + nrm * lift)) @ rot @ R(twist, "Z"))

    # livery: a teal stripe along both shoulder chines (segments laid on the skin), keel band below it
    for side in (-1, 1):
        for i in range(9):
            x = -9.5 + i * 2.1
            stick(m["livery"], (2.12, 0.55, 0.07), x, side, zc + hh * 0.55)
            if i % 2 == 0:
                stick(m["livery"], (2.05, 0.16, 0.07), x, side, zc + hh * 0.22)
    # window row (dim glass) on the starboard flank, hatch with hazard edging and handles on the port flank
    for i in range(6):
        stick(m["glass"], (0.95, 0.4, 0.08), -6.0 + i * 1.9, 1, zc + hh * 0.3)
    stick(m["dark"], (2.2, 1.8, 0.08), -3.5, -1, zc + 1.1, bevel=0.03)
    for dz in (-0.95, 0.95):
        stick(m["livery"], (2.4, 0.13, 0.1), -3.5, -1, zc + 1.1 + dz)
    stick(m["metal"], (0.12, 0.5, 0.14), -2.6, -1, zc + 1.1)
    # registry block and armour plates on the dorsal deck (laid from above)
    for i, (x, y) in enumerate(((-7.5, 1.4), (-4.8, -1.2), (-1.5, 1.6), (2.4, -1.5), (5.0, 1.2))):
        stick(m["hull"] if i % 2 else m["dark"], (2.0, 1.4, 0.07), x, y, 0.0, from_top=True)
    # dorsal sensor fin stub, broken (raked back), lit tip lens
    fin = [(-3.0, -0.3), (2.0, -0.3), (1.2, 0.45), (-1.0, 1.3), (-1.8, 1.35), (-1.6, 0.8)]
    h = skin_tree.ray_cast(pose @ Vector((-6.5, 0.0, 12.0)), Vector((0, 0, -1)))
    if h[0] is not None:
        base = h[0] - Vector((0, 0, 0.08))
        prism(P, m["dark"], fin, "Y", -0.18, 0.18, bevel=0.03, seg=2, matrix=T(*base) @ R(4.0, "Y") @ R(9.0, "X"))
        lens = L.box(P.coll, (0.3, 0.42, 0.12), (0, 0, 0))
        P.add(lens, m["glow"], bevel=0.02, segments=1, matrix=T(*base) @ R(4.0, "Y") @ R(9.0, "X") @ T(-1.0, 0, 1.0))
    # torn wing root on the port flank: a thick stub plate jutting out, jagged outer edge
    wr = L.grid_plate(P.coll, 5.0, 0.8, 10, 3, 0.38, jag=0.7, jag_sides=(0, 0, 1, 0), seed=7)
    h = skin_tree.ray_cast(pose @ Vector((-2.0, -12.0, zc - 0.6)), (pose.to_3x3() @ Vector((0, 1, 0))).normalized())
    if h[0] is not None:
        P.add(wr, m["hull"], bevel=0.03, segments=1, matrix=T(*(h[0] + Vector((0, -0.2, 0)))) @ R(9.0, "X") @ R(-10, "X"))

    # stern block: engine mount plate and two torn pylon stubs on top
    for side in (-1, 1):
        prism(P, m["dark"], [(-1.0, 0.0), (1.2, 0.0), (0.8, 0.8), (-0.6, 0.8)], "X", -11.8, -8.4, bevel=0.04, seg=2,
              matrix=pose @ T(0, side * 2.0, zc + hh * 0.86) @ R(side * 22, "X"))
    # interior visible through the tear: deck, bulkhead racks, ribs that stick past the torn edge
    bx(P, m["dark"], (-11.0, -3.4, 0.2), (11.0, 3.4, 0.95), bevel=0.03, seg=1, matrix=pose)
    for k in range(5):
        y = -2.4 + k * 1.2
        bx(P, m["dark"], (8.0, y - 0.35, 0.9), (10.4, y + 0.35, 2.3 + rng.uniform(0, 0.7)), bevel=0.03, seg=1, matrix=pose)
    for xr in (7.2, 9.6, 12.0):
        pts = []
        for j in range(15):
            a = j / 14.0
            ring = _section_ring(hw - 0.22, hh - 0.2, zc, xr)
            idx = int(a * (len(ring) // 2))
            pts.append(Vector(ring[idx]))
        if xr > 11.0:
            pts = pts[:9]
        for side in (1, -1):
            pp = [Vector((p.x, side * p.y, p.z)) for p in pts]
            P.add(L.tube_path(P.coll, pp, 0.11, sides=6), m["metal"], smooth=40, matrix=pose)
    # peeled plates at the tear
    for k, (y, z, ang) in enumerate(((3.5, 3.6, 30), (-3.2, 4.3, -35), (0.8, 5.0, 15))):
        pl = L.grid_plate(P.coll, 2.0, 1.5, 6, 4, 0.06, jag=0.6, jag_sides=(0, 1, 1, 1), seed=k)
        L.bend(pl, along="X", radius=2.2, start=0.0, plane_axis="Z")
        nrm = Vector((0, y, z - zc)).normalized()
        mt = pose @ T(Lx * 0.5 - 3.2, y * 0.95, z - 0.1) @ R(-math.degrees(math.atan2(nrm.y, nrm.z)), "X") @ R(ang, "Z")
        P.add(pl, m["hull"], smooth=30, matrix=mt)
    # moss that grows on the skin, vines hanging down the flanks
    moss = _grow_moss(P, m["moss"], skin, rng, cover=0.34, lift=0.02, thick=0.1, scale=1.1, seed=3)
    _glow_caps(P, m, moss, rng, 26)
    _soil_skirt(P, m, Lx, hw * 0.98, rng)
    mw = skin.matrix_world
    trees = [BVHTree.FromPolygons([mw @ v.co for v in skin.data.vertices], [p.vertices[:] for p in skin.data.polygons])]
    _hang_vines(P, m, trees, rng, (-10.5, 8.0), hw, 7.6, count=8)


def build_wreck_engine(P):
    rng = random.Random(41)
    m = wreck_mats("Engine", scale=0.8)
    zc = 3.5
    prof = [(-4.2, 2.75), (-4.15, 3.05), (-3.9, 3.28), (-3.3, 3.42), (-1.0, 3.5), (1.6, 3.42), (2.8, 3.05),
            (3.6, 2.75), (3.95, 2.62)]
    segs = 48
    rings = [[(x, r * math.cos(math.tau * k / segs), zc + r * math.sin(math.tau * k / segs)) for k in range(segs)]
             for x, r in prof]
    # densify along x for the breach outline
    dense = []
    for (xa, ra), (xb, rb) in zip(prof, prof[1:]):
        steps = max(1, int(abs(xb - xa) / 0.35))
        for s_ in range(steps):
            t = s_ / float(steps)
            x = xa + (xb - xa) * t
            r = ra + (rb - ra) * t
            dense.append([(x, r * math.cos(math.tau * k / segs), zc + r * math.sin(math.tau * k / segs)) for k in range(segs)])
    dense.append(rings[-1])
    bm = ring_loft(P.coll, dense, cap0=False, cap1=False)
    dead = []
    for f in bm.faces:
        c = f.calc_center_median()
        a = math.degrees(math.atan2(c.z - zc, c.y))
        edge = 2.2 + 0.9 * math.sin(a * 0.11) + rng.uniform(-0.3, 0.3)
        if 30 < a < 125 and -1.4 + 0.5 * math.sin(a * 0.2) < c.x < edge:
            dead.append(f)
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    for v in bm.verts:
        if v.is_boundary and -1.6 < v.co.x < 3.2:
            v.co += Vector((rng.uniform(-0.12, 0.12), 0, 0))
            # bend the torn lips inward (dent)
            d = Vector((0, v.co.y, v.co.z - zc)).normalized()
            v.co -= d * rng.uniform(0.0, 0.18)
    bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=0.16)
    shell = L.obj_from_bm(bm, P.coll)
    pose = T(0, 0, -0.35) @ R(14.0, "X") @ R(-3.0, "Y")
    P.add(shell, m["hull"], bevel=0.025, segments=1, angle=25, matrix=pose)
    # intake lip ring (dark), livery bands, nozzle petals
    P.add(ring_x(P.coll, -0.25, 0.25, 2.62, 3.12), m["dark"], bevel=0.03, segments=2, matrix=pose @ T(-4.15, 0, zc) @ place_x())
    for x, r in ((-2.9, 3.46), (2.3, 3.32)):
        P.add(ring_x(P.coll, -0.22, 0.22, r - 0.06, r + 0.03), m["livery"], bevel=0.01, segments=1,
              matrix=pose @ T(x, 0, zc) @ place_x())
    skirt = L.revolve_closed(P.coll, [(2.6, 0.0), (2.74, 0.0), (2.5, 0.62), (2.38, 0.62)], (0.0, 360.0), 48, axis="Z")
    P.add(skirt, m["dark"], bevel=0.015, segments=1, matrix=pose @ T(3.5, 0, zc) @ place_x())
    for k in range(12):
        o = L.box(P.coll, (0.6, 0.05, 0.06), (0, 0, 0))
        P.add(o, m["metal"], bevel=0.008, segments=1, matrix=pose @ T(3.8, 0, zc) @ R(30 * k, "X") @ T(0, 0, 2.62) @ R(-21, "Y"))
    # inner drum, intake fan with broken blades, nozzle cone, thin residual glow ring inside the nozzle
    P.add(L.lathe(P.coll, [(-3.6, 1.3), (-2.0, 1.75), (2.2, 1.65), (3.4, 1.1), (3.9, 0.0)], segments=28, axis="X"),
          m["dark"], smooth=35, matrix=pose @ T(0, 0, zc))
    P.add(L.lathe(P.coll, [(-4.1, 0.0), (-3.95, 0.5), (-3.65, 0.85)], segments=24, axis="X"), m["metal"], smooth=40,
          matrix=pose @ T(0, 0, zc))
    for k in range(16):
        a = 360.0 * k / 16
        if 50 < a < 140 and k % 3:
            continue
        ln = 2.1 if k % 5 else 1.2
        o = L.box(P.coll, (0.07, 0.34, ln), (0, 0, 0))
        P.add(o, m["metal"], bevel=0.01, segments=1, matrix=pose @ T(-3.65, 0, zc) @ R(a, "X") @ T(0, 0, 0.85 + ln * 0.5) @ R(28, "Z"))
    P.add(ring_x(P.coll, -0.06, 0.06, 2.2, 2.4), m["glow"], matrix=pose @ T(3.45, 0, zc) @ place_x())
    for k in range(10):
        o = L.box(P.coll, (0.5, 0.07, 0.95), (0, 0, 0))
        P.add(o, m["dark"], bevel=0.01, segments=1, matrix=pose @ T(3.3, 0, zc) @ R(36 * k, "X") @ T(0, 0, 1.75))
    # internal rings and conduits visible through the breach
    for x in (-0.9, 0.3, 1.5):
        P.add(ring_x(P.coll, -0.09, 0.09, 3.05, 3.3), m["metal"], bevel=0.01, segments=1, matrix=pose @ T(x, 0, zc) @ place_x())
    for k in range(3):
        a = math.radians(70 + 20 * k)
        pts = [Vector((x, math.cos(a) * 2.0, zc + math.sin(a) * 2.0)) for x in (-1.6, -0.5, 0.6, 1.8, 2.6)]
        P.add(L.tube_path(P.coll, pts, 0.09, sides=8), m["metal"], smooth=40, matrix=pose)
    # torn pylon stub
    prism(P, m["dark"], [(-0.8, 0.0), (0.8, 0.0), (0.5, 1.0), (-0.4, 1.0)], "X", -2.6, 1.6, bevel=0.03, seg=2,
          matrix=pose @ T(0, -2.5, zc + 2.15) @ R(-42, "X"))
    moss = _grow_moss(P, m["moss"], shell, rng, cover=0.32, lift=0.02, thick=0.06, scale=1.6, seed=5)
    _glow_caps(P, m, moss, rng, 12)
    mw = shell.matrix_world
    trees = [BVHTree.FromPolygons([mw @ v.co for v in shell.data.vertices], [p.vertices[:] for p in shell.data.polygons])]
    _hang_vines(P, m, trees, rng, (-3.6, 3.6), 3.4, 7.2, count=5)


def _with_cut(fn):
    def run(P):
        fn(P)
        _cut_all_below(P)
    return run


BUILDERS = {
    "SM_Verd_Wreck_Hull": dict(fn=_with_cut(build_wreck_hull), family="verdance", dims=(26.0, 9.0, 7.0), tex=2048,
                               budget=(2000, 40000), ao=1.5, snap=True),
    "SM_Verd_Wreck_Engine": dict(fn=_with_cut(build_wreck_engine), family="verdance", dims=(9.0, 7.0, 7.0),
                                 tex=2048, budget=(2000, 40000), ao=1.0, snap=True),
}
