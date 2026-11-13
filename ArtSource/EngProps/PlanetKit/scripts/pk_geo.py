# SPDX-License-Identifier: MIT
"""pk_geo.py - small geometry helpers shared by the PlanetKit builders (metres, Blender +X = Unreal forward)."""

import math
import random

import bmesh
from mathutils import Matrix, Vector

import pk_lib as L


def T(x, y, z):
    return Matrix.Translation((x, y, z))


def R(deg, axis):
    return Matrix.Rotation(math.radians(deg), 4, axis)


def bx(P, mat, lo, hi, bevel=0.02, seg=2, matrix=None, angle=30.0, **kw):
    """Axis-aligned box from min / max corners (metres)."""
    lo, hi = Vector(lo), Vector(hi)
    o = L.box(P.coll, tuple(hi - lo), tuple((lo + hi) * 0.5))
    return P.add(o, mat, bevel=bevel, segments=seg, matrix=matrix, angle=angle, **kw)


def prism(P, mat, profile, axis, a0, a1, bevel=0.02, seg=2, matrix=None, segments=1, angle=30.0, **kw):
    """Extrude a closed 2D profile along `axis` from a0 to a1. Profile coordinates: axis Y -> (x, z),
    axis X -> (y, z), axis Z -> (x, y)."""
    o = L.extrude_profile(P.coll, profile, abs(a1 - a0), segments=segments, axis=axis)
    mid = (a0 + a1) * 0.5
    off = {"X": (mid, 0, 0), "Y": (0, mid, 0), "Z": (0, 0, mid)}[axis]
    m = T(*off)
    if matrix is not None:
        m = matrix @ m
    return P.add(o, mat, bevel=bevel, segments=seg, matrix=m, angle=angle, **kw)


def ngon(r, n, rot_deg=0.0, sx=1.0, sy=1.0):
    return [(r * sx * math.cos(math.radians(rot_deg) + math.tau * i / n),
             r * sy * math.sin(math.radians(rot_deg) + math.tau * i / n)) for i in range(n)]


def cham_rect(x0, x1, y0, y1, c):
    """Rectangle with 45 degree chamfered corners (2D)."""
    return [(x0 + c, y0), (x1 - c, y0), (x1, y0 + c), (x1, y1 - c), (x1 - c, y1), (x0 + c, y1), (x0, y1 - c),
            (x0, y0 + c)]


def blade_rect(x0, x1, y0, y1, c):
    """Null Armada plate outline: two opposite corners cut hard (blade-like), the others square."""
    return [(x0 + c, y0), (x1, y0), (x1, y1 - c), (x1 - c, y1), (x0, y1), (x0, y0 + c)]


def hex_column(P, mat, cx, cy, r, z0, z1, rot=0.0, top_tilt=(0.0, 0.0), bevel=0.03, seed=0, sides=6,
               jitter=0.08):
    """Basalt column: irregular hexagonal prism, top face optionally tilted (broken top)."""
    rng = random.Random(seed)
    prof = [(x * (1.0 + rng.uniform(-jitter, jitter)), y * (1.0 + rng.uniform(-jitter, jitter)))
            for x, y in ngon(r, sides, rot)]
    bm = bmesh.new()
    bot = [bm.verts.new((cx + x, cy + y, z0)) for x, y in prof]
    top = []
    for x, y in prof:
        z = z1 + top_tilt[0] * x + top_tilt[1] * y
        top.append(bm.verts.new((cx + x, cy + y, z)))
    n = len(prof)
    for i in range(n):
        bm.faces.new((bot[i], bot[(i + 1) % n], top[(i + 1) % n], top[i]))
    bm.faces.new(list(reversed(bot)))
    bm.faces.new(top)
    o = L.obj_from_bm(bm, P.coll)
    return P.add(o, mat, bevel=bevel, segments=2, angle=25.0)


def shard(P, mat, base, direction, r, length, sides=6, tip=0.28, seed=0, bevel=None):
    """Crystal: hexagonal prism with a pointed tip, from `base` along `direction` (the base is buried a little)."""
    rng = random.Random(seed)
    d = Vector(direction).normalized()
    t1 = d.orthogonal().normalized()
    t2 = d.cross(t1).normalized()
    bm = bmesh.new()
    body = length * (1.0 - tip)
    rot = rng.uniform(0, math.tau)
    ring0, ring1 = [], []
    for i in range(sides):
        a = rot + math.tau * i / sides
        off = (t1 * math.cos(a) + t2 * math.sin(a)) * r
        ring0.append(bm.verts.new(Vector(base) + off * 0.92))
        ring1.append(bm.verts.new(Vector(base) + d * body + off))
    apex = bm.verts.new(Vector(base) + d * length)
    for i in range(sides):
        j = (i + 1) % sides
        bm.faces.new((ring0[i], ring0[j], ring1[j], ring1[i]))
        bm.faces.new((ring1[i], ring1[j], apex))
    bm.faces.new(list(reversed(ring0)))
    o = L.obj_from_bm(bm, P.coll)
    return P.add(o, mat, bevel=bevel)


def superellipse_ring(hw, hh, n, e=2.6, zc=0.0, x=0.0):
    pts = []
    for i in range(n):
        a = math.tau * i / n
        c, s = math.cos(a), math.sin(a)
        y = hw * math.copysign(abs(c) ** (2.0 / e), c)
        z = hh * math.copysign(abs(s) ** (2.0 / e), s)
        pts.append((x, y, zc + z))
    return pts


def ring_loft(coll, rings, cap0=True, cap1=True):
    """Loft equal-length vertex rings (lists of xyz) into a tube; optional end caps."""
    bm = bmesh.new()
    vr = [[bm.verts.new(p) for p in ring] for ring in rings]
    n = len(rings[0])
    for a, b in zip(vr, vr[1:]):
        for i in range(n):
            bm.faces.new((a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]))
    if cap0:
        bm.faces.new(list(reversed(vr[0])))
    if cap1:
        bm.faces.new(vr[-1])
    return bm


def cut_below(obj, z=0.0, fill=True):
    """Bisect the mesh at height z, delete everything below, cap the hole (buried parts)."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    geom = list(bm.verts) + list(bm.edges) + list(bm.faces)
    res = bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0, 0, z), plane_no=(0, 0, 1), clear_inner=True)
    if fill:
        cut_edges = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
        if cut_edges:
            try:
                bmesh.ops.holes_fill(bm, edges=cut_edges, sides=0)
            except Exception:
                pass
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(me)
    bm.free()
    me.update()


def solidify_bm(bm, thickness):
    faces = list(bm.faces)
    bmesh.ops.solidify(bm, geom=faces, thickness=thickness)
    return bm
