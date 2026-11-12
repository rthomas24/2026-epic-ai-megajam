# SPDX-License-Identifier: MIT
"""
build_cinder.py - regenerates the whole Cinder ground prop kit (9 static meshes).

    blender --background --factory-startup --python ArtSource/EngProps/Cinder/scripts/build_cinder.py
    ... -- --stage clay      geometry + a clay contact sheet only (fast, for shape iteration)
    ... -- --stage bake      + shared atlases, bake, collision, checks, contact sheets
    ... -- --stage review    + the per-asset review sheets
    ... -- --stage full      everything (default): + per-asset .blend, FBX, manifest, FBX round trip
    ... -- --fast            quicker, looser UV packing (iteration only; never for the final run)
    ... -- --only Crystal    limit a clay / bake / review run to the assets whose name contains a word

Cinder = volcanic world: vertical, crystalline, sharp shapes; black basalt, magma glow, cyan and amber
crystal spires. Everything is flat shaded on purpose. Original, code-built geometry (see ../README.md).
"""

import json
import math
import os
import random
import sys

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import propkit as pk  # noqa: E402

KIT = "Cinder"
KIT_DIR = pk.KIT_DIR
TEX_DIR = os.path.join(KIT_DIR, "textures")
REVIEW_DIR = os.path.join(KIT_DIR, "review")

# ---------------------------------------------------------------- palette (sRGB hex -> linear)
BASALT = pk.hexc("#1A1718")
BASALT_LIGHT = pk.hexc("#2E292C")
BASALT_BLUE = pk.hexc("#20222B")
ASH = pk.hexc("#4A4446")
CYAN = pk.hexc("#2EE6FF")
CYAN_DEEP = pk.hexc("#0B5F8C")
AMBER = pk.hexc("#FFB347")
AMBER_DEEP = pk.hexc("#A8480E")
MAGMA = pk.hexc("#FF5A1F")
MAGMA_HOT = pk.hexc("#FFB23C")

TAU = math.tau
EMISSION_STRENGTH = 1.5
CRYSTAL = {"cyan": (CYAN, CYAN_DEEP), "amber": (AMBER, AMBER_DEEP)}


def xform(at, rot=(0, 0, 0)):
    m = Matrix.Translation(Vector(at))
    for ang, axis in zip(rot, "XYZ"):
        m = m @ Matrix.Rotation(math.radians(ang), 4, axis)
    return m


# ============================================================================ crystals

def crystal(b, base, direction, length, radius, kind, seed, sides=6, chamfer=0.34, taper=0.86, tip=0.22,
            flat=1.0, steps=(), embed=0.1, tag=1, gain=1.0, face_deg=None, chisel=0.0):
    """One faceted prism: wide faces alternate with narrow edge facets; an off-centre two-stage point;
    optional growth steps (t, scale) cut ledges into the shaft. Every facet gets its own brightness and
    the glow runs hot at the root and dim at the tip, so the facets still read in the dark."""
    rng = random.Random(seed)
    glow_c, deep_c = CRYSTAL[kind]
    corners = []
    for k in range(sides):
        a = TAU * (k + rng.uniform(-0.14, 0.14)) / sides
        r = radius * rng.uniform(0.84, 1.12)
        corners.append(Vector((r * math.cos(a), r * math.sin(a) * flat)))
    poly = []
    for k in range(sides):
        c, prev, nxt = corners[k], corners[k - 1], corners[(k + 1) % sides]
        poly.append(c + (prev - c) * chamfer * 0.5)
        poly.append(c + (nxt - c) * chamfer * 0.5)
    m = len(poly)
    d = Vector(direction).normalized()
    base = Vector(base)
    side = d.orthogonal().normalized()
    if face_deg is not None:  # the wide axis of a flat blade points this way (azimuth, degrees)
        f = Vector((math.cos(math.radians(face_deg)), math.sin(math.radians(face_deg)), 0.0))
        f = f - d * f.dot(d)
        if f.length > 1e-4:
            side = f.normalized()
    tip_off = (side * rng.uniform(-1, 1) + d.cross(side) * rng.uniform(-1, 1)) * radius * 0.45
    if chisel:  # one oblique cut: the point sits out near one edge of the blade
        tip_off = side * radius * chisel
    # stations along the shaft: (distance along the axis, cross-section scale, lateral shift fraction)
    stations = [(-embed * length, 0.9, 0.0), (0.0, 0.96, 0.0)]
    for t, sc in steps:
        stations.append((length * (t - 0.012), sc[0], 0.0))
        stations.append((length * (t + 0.012), sc[1], 0.0))
    stations.append((length * (1.0 - tip), taper if not steps else min(taper, steps[-1][1][1]), 0.0))
    stations.append((length * (1.0 - tip * 0.42), taper * 0.52, 0.55))
    pts = [base + d * z + tip_off * sh for z, _s, sh in stations] + [base + d * length + tip_off]
    scales = [s for _z, s, _sh in stations] + [0.0]
    faces = b.sweep(pts, lambda i, t: None if scales[i] <= 0 else [(p.x * scales[i], p.y * scales[i]) for p in poly],
                    side=side, smooth=False, tag=tag)
    n_st = len(pts)
    facet = [rng.uniform(0.55, 1.0) for _ in range(m)]
    tone = [rng.uniform(0.8, 1.15) for _ in range(m)]

    def fn(v, f):
        h = max(0.0, (v.co - base).dot(d)) / length  # 0 at the root, 1 at the point
        j, k = f[b.fj], f[b.fk]
        edge = j >= 0 and j % 2 == 0
        if j < 0:  # buried root cap
            return {"col": pk.mulc(deep_c, 0.5), "rough": 0.3, "glow": (0, 0, 0), "noise": 0.2, "bump": 0.2}
        heat = pk.lerp(1.0, 0.07, pk.smooth01(h, 0.0, 0.7))
        if edge:
            g = 0.95 * heat
            col = pk.mixc(deep_c, glow_c, 0.75)
        else:
            g = 0.5 * facet[j] * heat
            col = pk.mulc(pk.mixc(deep_c, glow_c, 0.3 + 0.25 * facet[j]), tone[j])
        if k >= n_st - 3:  # the point: deeper, more saturated
            col = pk.mixc(col, deep_c, 0.35)
        return {"col": col, "rough": 0.3 if not edge else 0.24, "glow": pk.mulc(glow_c, g * gain),
                "noise": 0.15, "bump": 0.0}
    b.paint(faces, fn=fn)
    return faces


def chunk(b, size, seed, at, rot=(0, 0, 0), cuts=12, bevel=0.012, floor=None, tint=BASALT, cut_range=(0.52, 0.86),
          taper=0.0, subdiv=2, lump=0.2, ash=1.0, cracks=0.0, rough=0.5, segs=1, clips=()):
    """One angular, fractured basalt block: flat shaded, chamfered edges."""
    fl = floor if floor is not None else -size[2] * 0.5
    faces = b.rock(size, seed=seed, subdiv=subdiv, lump=lump, freq=1.3, cuts=cuts, cut_range=cut_range,
                   crisp=True, bevel=bevel, bevel_segments=segs, bevel_angle=24.0, floor=fl, xf=xform(at, rot),
                   smooth=False, taper=taper, down_bias=0.3, clips=clips)
    b.paint(faces, col=tint, rough=rough, noise=0.9, bump=0.45, mask=ash, mask2=0.0)
    if cracks:  # faint, and only in the lowest 35 cm: heat from the ground
        b.paint(faces, fn=lambda v, f: {"mask2": cracks * 0.44 * (1.0 - pk.smooth01(v.co.z, 0.1, 0.35))})
    return faces


def attach(b, main, faces, embed=0.05):
    c = sum((v.co for f in faces for v in f.verts), Vector()) / sum(len(f.verts) for f in faces)
    m = sum((v.co for f in main for v in f.verts), Vector()) / sum(len(f.verts) for f in main)
    d = Vector((m.x - c.x, m.y - c.y, 0.0))
    b.push_until_touch(faces, main, d, embed=embed + 0.04 * d.length)
    return faces


def root_light(b, faces, roots, color, reach=0.5, level=0.1):
    """Paint a little of the crystals' light onto the rock they grow from, so the base is grounded in the dark."""
    def fn(v, f):
        dist = min((v.co - Vector(r)).length for r in roots)
        k = (1.0 - pk.smooth01(dist, 0.02, reach)) ** 2 * pk.smooth01(v.co.z, -0.02, 0.06)
        return {"glow": pk.mulc(color, level * k)} if k > 0.001 else None
    b.paint(faces, fn=fn)


def cluster(name, coll, kind, shards, rocks, sharp=None, reach=0.5):
    """shards: [(base xy, lean_deg, lean_azimuth_deg, length, radius, kwargs)], rocks: [chunk kwargs]."""
    b = pk.Builder()
    rock_faces = []
    main = None
    for i, r in enumerate(rocks):
        fs = chunk(b, **r)
        if main is None:
            main = fs
        else:
            attach(b, main, fs)
        rock_faces.extend(fs)
    roots = []
    seated = list(rock_faces)
    for i, (xy, lean, az, length, radius, kw) in enumerate(shards):
        la, aa = math.radians(lean), math.radians(az)
        d = Vector((math.sin(la) * math.cos(aa), math.sin(la) * math.sin(aa), math.cos(la)))
        base = Vector((xy[0], xy[1], kw.pop("z", 0.02)))
        fs = crystal(b, base, d, length, radius, kind, seed=sum(ord(c) for c in name) * 31 + i, **kw)
        if i > 0 and base.length > 1e-3:  # slide outlying shards inward until they bite into the rock or a neighbour
            inward = Vector((-base.x, -base.y, 0.0)).normalized()
            base = base + inward * b.push_until_touch(fs, seated, inward, embed=0.03)
        seated = seated + fs
        roots.append(base)
    root_light(b, rock_faces, roots, CRYSTAL[kind][0], reach=reach)
    return b.finish(name, coll, recentre=True)


def crystal_01(coll):
    """~80 cm, cyan. Urchin: a low fan of seven shards splayed from one point."""
    S = [((0.0, 0.0), 8, 20, 0.8, 0.075, {}),
         ((0.07, 0.03), 34, 15, 0.62, 0.06, {"sides": 5}),
         ((-0.06, 0.06), 38, 140, 0.56, 0.055, {"sides": 5}),
         ((-0.02, -0.08), 42, 255, 0.5, 0.05, {"sides": 5}),
         ((0.1, -0.07), 56, 320, 0.36, 0.04, {"sides": 5, "tip": 0.3}),
         ((-0.11, -0.02), 60, 195, 0.32, 0.038, {"sides": 5, "tip": 0.3}),
         ((0.03, 0.11), 58, 80, 0.3, 0.036, {"sides": 5, "tip": 0.3})]
    R = [dict(size=(0.5, 0.44, 0.2), seed=3, at=(0.0, 0.0, 0.02), cuts=10, floor=-0.1, bevel=0.01),
         dict(size=(0.24, 0.2, 0.14), seed=5, at=(0.3, 0.1, 0.02), cuts=7, floor=-0.07, bevel=0.008,
              tint=BASALT_LIGHT)]
    return cluster("SM_Cind_Crystal_01", coll, "cyan", S, R, reach=0.3)


def crystal_02(coll):
    """~180 cm, amber. Windswept: five flat blades all leaning the same way like a hand of cards, each
    ending in one oblique chisel cut."""
    blade = {"flat": 0.26, "face_deg": 100, "sides": 6, "tip": 0.2, "chamfer": 0.3}
    S = [((0.0, 0.0), 20, 10, 1.92, 0.3, dict(blade, chisel=0.8, steps=[(0.52, (0.95, 0.82))])),
         ((0.16, -0.24), 26, 4, 1.4, 0.19, dict(blade, chisel=-0.8, face_deg=90)),
         ((-0.14, 0.26), 15, 18, 1.1, 0.185, dict(blade, chisel=0.75, face_deg=112)),
         ((0.34, 0.12), 33, 0, 0.8, 0.11, dict(blade, chisel=0.7, face_deg=80)),
         ((-0.3, -0.16), 29, 26, 0.5, 0.1, dict(blade, chisel=-0.7, tip=0.3))]
    R = [dict(size=(1.0, 0.8, 0.32), seed=11, at=(0.05, 0.0, 0.03), cuts=11, floor=-0.16, bevel=0.014),
         dict(size=(0.46, 0.3, 0.3), seed=13, at=(-0.6, 0.25, 0.03), cuts=12, floor=-0.14, tint=BASALT_LIGHT,
              cut_range=(0.44, 0.8), taper=0.4),
         dict(size=(0.34, 0.22, 0.2), seed=17, at=(0.62, -0.4, 0.02), cuts=11, floor=-0.09, cut_range=(0.44, 0.8),
              taper=0.4)]
    return cluster("SM_Cind_Crystal_02", coll, "amber", S, R, reach=0.45)


def crystal_03(coll):
    """~300 cm, cyan. Twin spires: two tall prisms in a narrow V with a ring of small shards at their feet."""
    S = [((-0.1, 0.0), 9, 170, 3.05, 0.2, {"steps": [(0.36, (0.95, 0.82)), (0.7, (0.8, 0.7))]}),
         ((0.22, 0.05), 13, 5, 2.3, 0.165, {"steps": [(0.5, (0.94, 0.8))]}),
         ((0.02, -0.26), 30, 265, 1.05, 0.11, {}),
         ((0.36, -0.2), 44, 320, 0.7, 0.085, {"sides": 5}),
         ((-0.4, 0.14), 47, 150, 0.62, 0.08, {"sides": 5}),
         ((0.05, 0.3), 38, 85, 0.82, 0.09, {"sides": 5}),
         ((-0.3, -0.24), 55, 215, 0.42, 0.06, {"sides": 5, "tip": 0.3}),
         ((0.44, 0.22), 58, 30, 0.38, 0.055, {"sides": 5, "tip": 0.3})]
    R = [dict(size=(1.3, 1.1, 0.42), seed=21, at=(0.02, 0.0, 0.04), cuts=12, floor=-0.2, bevel=0.016),
         dict(size=(0.6, 0.46, 0.34), seed=23, at=(0.8, 0.3, 0.04), cuts=9, floor=-0.16, tint=BASALT_LIGHT),
         dict(size=(0.5, 0.4, 0.26), seed=27, at=(-0.78, -0.3, 0.03), cuts=8, floor=-0.12)]
    return cluster("SM_Cind_Crystal_03", coll, "cyan", S, R, reach=0.6)


def crystal_04(coll):
    """~500 cm, amber. Monolith: one massive stepped spire with parallel buttress crystals fused to its sides."""
    S = [((0.0, 0.0), 6, 40, 5.05, 0.4, {"steps": [(0.27, (0.97, 0.86)), (0.5, (0.84, 0.74)), (0.76, (0.72, 0.6))],
                                         "tip": 0.16, "chisel": 0.7, "gain": 0.9}),
         ((0.42, -0.1), 8, 20, 3.3, 0.24, {"steps": [(0.58, (0.94, 0.8))], "gain": 0.62}),
         ((-0.36, 0.24), 5, 120, 2.5, 0.21, {"steps": [(0.44, (0.94, 0.8))], "gain": 0.62}),
         ((-0.2, -0.42), 10, 250, 1.75, 0.17, {"gain": 0.62}),
         ((0.26, 0.42), 12, 70, 1.2, 0.14, {"gain": 0.62}),
         ((0.72, 0.2), 40, 15, 0.95, 0.1, {"sides": 5}),
         ((-0.7, -0.2), 46, 200, 0.8, 0.09, {"sides": 5}),
         ((0.3, -0.7), 50, 290, 0.6, 0.075, {"sides": 5, "tip": 0.3}),
         ((-0.34, 0.72), 52, 115, 0.55, 0.07, {"sides": 5, "tip": 0.3})]
    R = [dict(size=(1.9, 1.7, 0.6), seed=31, at=(0.0, 0.0, 0.05), cuts=13, floor=-0.28, bevel=0.02),
         dict(size=(0.9, 0.7, 0.5), seed=33, at=(1.1, -0.5, 0.05), cuts=10, floor=-0.22, tint=BASALT_LIGHT),
         dict(size=(0.8, 0.6, 0.4), seed=37, at=(-1.0, 0.6, 0.04), cuts=9, floor=-0.18),
         dict(size=(0.5, 0.4, 0.3), seed=39, at=(-0.5, -1.0, 0.03), cuts=8, floor=-0.14, tint=BASALT_LIGHT)]
    return cluster("SM_Cind_Crystal_04", coll, "amber", S, R, reach=0.8)


# ============================================================================ basalt rocks

SHARD = dict(cuts=13, cut_range=(0.42, 0.8), lump=0.1)  # small rubble: all planes, no round residue


def basalt_01(coll):
    """~80 cm across. One block split in two along a slanting fracture: the halves have matching faces and
    lean apart."""
    b = pk.Builder()
    size, plane = (0.74, 0.54, 0.7), ((0.02, 0.0, 0.0), (1.0, 0.12, 0.3))
    main = chunk(b, size, 41, (-0.05, 0.0, 0.17), rot=(0, -13, 12), cuts=11, floor=-0.38, clips=[plane], segs=2,
                 bevel=0.014)
    other = chunk(b, size, 41, (0.08, 0.0, 0.14), rot=(0, 15, 12), cuts=11, floor=-0.38, tint=BASALT_BLUE,
                  clips=[(plane[0], tuple(-c for c in plane[1]))], segs=2, bevel=0.014)
    b.push_until_touch(other, main, (-1, 0, 0), embed=0.0)
    return b.finish("SM_Cind_Basalt_01", coll, recentre=True)


def basalt_02(coll):
    """~200 cm across. A big wedge slab tilted up on a blocky footing, with broken pieces at its foot."""
    b = pk.Builder()
    main = chunk(b, (2.2, 1.65, 1.0), 51, (0.0, 0.0, 0.25), rot=(0, 0, 15), cuts=13, floor=-0.45, bevel=0.02)
    slab = chunk(b, (2.5, 1.45, 0.62), 53, (-0.2, 0.05, 1.1), rot=(8, -36, 35), cuts=12, floor=-0.3,
                 bevel=0.022, tint=BASALT_BLUE, taper=0.2, cut_range=(0.6, 0.9))
    b.push_until_touch(slab, main, (0, 0, -1), embed=0.2)
    attach(b, main, chunk(b, (0.76, 0.62, 0.62), 57, (-1.4, -0.5, 0.12), rot=(0, 10, 60), cuts=10, floor=-0.3,
                          tint=BASALT_LIGHT, taper=0.3))
    attach(b, main, chunk(b, (0.62, 0.34, 0.36), 59, (0.9, -1.2, 0.08), rot=(15, 0, 20), floor=-0.17,
                          taper=0.45, bevel=0.012, **SHARD))
    return b.finish("SM_Cind_Basalt_02", coll, recentre=True)


def basalt_03(coll):
    """~350 cm across. A jagged ridge of three leaning fins on broken blocks. The only basalt rock with
    faint ember-orange cracks."""
    b = pk.Builder()
    main = chunk(b, (3.4, 2.1, 1.2), 61, (0.0, 0.0, 0.3), rot=(0, 0, 8), cuts=14, floor=-0.55, bevel=0.03,
                 cracks=1.0, subdiv=3)
    fins = [((1.5, 1.0, 3.3), 63, (-0.8, 0.05, 1.3), (6, -16, 15), BASALT_BLUE),
            ((1.3, 0.9, 2.5), 67, (0.5, -0.05, 1.1), (-5, 12, -10), BASALT),
            ((1.1, 0.75, 1.75), 69, (1.55, 0.1, 0.85), (4, 24, 30), BASALT_BLUE)]
    for size, seed, at, rot, tint in fins:
        fs = chunk(b, size, seed, at, rot=rot, cuts=12, floor=-size[2] * 0.42, bevel=0.026, tint=tint, taper=0.62,
                   cut_range=(0.56, 0.88))
        b.push_until_touch(fs, main, (0, 0, -1), embed=0.14)
    attach(b, main, chunk(b, (1.2, 0.95, 0.8), 71, (-2.5, -0.4, 0.16), rot=(0, 8, 50), cuts=10, floor=-0.38,
                          tint=BASALT_LIGHT, taper=0.3, cracks=1.0))
    attach(b, main, chunk(b, (0.95, 0.5, 0.5), 73, (2.1, -1.1, 0.1), rot=(10, 0, 20), floor=-0.23,
                          taper=0.45, bevel=0.014, **SHARD))
    return b.finish("SM_Cind_Basalt_03", coll, recentre=True)


# ============================================================================ basalt columns

def column_group(name, coll, cells, R, seed, lean=(0, 0, 0), rubble=()):
    """Fused hexagonal columns of uneven height. cells = [(q, r, height_m, joints)] on a hex grid of
    circumradius R; neighbours share their (jittered) corners, each column is grown 1.5 cm past them so
    the shells interlock, and every column is a stack of segments with a chamfered groove at each joint."""
    b = pk.Builder()
    rng = random.Random(seed)
    corner_cache = {}

    def corner(x, y):
        key = (round(x / (R * 0.2)), round(y / (R * 0.2)))
        if key not in corner_cache:
            corner_cache[key] = Vector((x + rng.uniform(-0.13, 0.13) * R, y + rng.uniform(-0.13, 0.13) * R))
        return corner_cache[key]
    xf = xform((0, 0, 0), lean)
    all_faces = []
    for q, r, height, joints in cells:
        cx, cy = R * math.sqrt(3.0) * (q + r * 0.5), R * 1.5 * r
        poly = [corner(cx + R * math.cos(math.radians(30 + 60 * k)), cy + R * math.sin(math.radians(30 + 60 * k)))
                for k in range(6)]
        cen = sum(poly, Vector((0.0, 0.0))) / 6.0
        poly = [cen + (p - cen) * (1.0 + 0.015 / R) for p in poly]
        cham = R * 0.06
        inset = 0.95
        st = [(-0.3, 1.0)]
        zs = sorted(height * j + rng.uniform(-0.05, 0.05) for j in joints)
        for zj in zs:
            st += [(zj - cham, 1.0), (zj, inset), (zj + cham * 0.6, 1.0)]
        st += [(height - cham, 1.0), (height, inset)]
        ta = rng.uniform(0, TAU)
        tilt = Vector((math.cos(ta), math.sin(ta))) * rng.uniform(0.16, 0.36)
        chip = rng.randrange(6)
        n = len(st)
        faces = b.sweep([Vector((0, 0, z)) for z, _s in st],
                        lambda i, t: [((cen.x + (p.x - cen.x) * st[i][1]), (cen.y + (p.y - cen.y) * st[i][1]))
                                      for p in poly],
                        side=(1, 0, 0), smooth=False, tag=0)
        verts = {v for f in faces for v in f.verts}
        for v in verts:  # tilt the top face (and its chamfer ring)
            if b.t(v) >= (n - 2) / float(n - 1) - 1e-6:
                v.co.z += tilt.x * (v.co.x - cen.x) + tilt.y * (v.co.y - cen.y)
                if int(round(b.u(v) * 6)) % 6 == chip:  # one broken corner
                    v.co.z -= R * 0.32
        for v in verts:
            v.co = xf @ v.co
        tone = rng.uniform(0.78, 1.2)
        seg_tone = [rng.uniform(0.95, 1.05) for _ in range(n)]
        face_tone = [(0.72, 1.0, 1.32, 0.86, 1.18, 0.94)[(k + chip) % 6] for k in range(6)]
        groove = {i for i, (_z, s) in enumerate(st) if s < 1.0 and i < n - 1}

        def fn(v, f):
            i = int(round(b.t(v) * (n - 1)))
            if f[b.fk] == -2 or (f[b.fk] == n - 2):  # top face and its chamfer: dusted with ash
                return {"col": pk.mixc(BASALT_LIGHT, ASH, 0.85), "rough": 0.75, "noise": 0.9, "bump": 0.5,
                        "mask": 1.0}
            col = pk.mulc(pk.mixc(BASALT, BASALT_BLUE, 0.5), tone * seg_tone[min(f[b.fk] if f[b.fk] >= 0 else 0, n - 1)]
                          * (face_tone[f[b.fj]] if 0 <= f[b.fj] < 6 else 1.0))
            if i in groove:
                col = pk.mulc(col, 0.55)
            return {"col": col, "rough": 0.46, "noise": 0.8, "bump": 0.3, "mask": 0.6}
        b.paint(faces, fn=fn)
        all_faces.extend(faces)
    for size, sd, at, rot in rubble:
        fs = chunk(b, size, sd, at, rot=rot, floor=-size[2] * 0.45, bevel=0.012, tint=BASALT_LIGHT, taper=0.45,
                   **SHARD)
        attach(b, all_faces, fs)
    return b.finish(name, coll, recentre=True)


def columns_01(coll):
    """~250 cm. Seven fused columns stepping down from an off-centre peak, with two broken stumps."""
    cells = [(0, 0, 2.5, (0.42,)), (1, 0, 2.05, (0.68,)), (0, 1, 1.6, (0.55,)), (-1, 1, 1.05, ()),
             (-1, 0, 1.9, (0.36,)), (0, -1, 0.62, ()), (1, -1, 1.3, (0.6,)), (2, -1, 0.34, ()),
             (-1, 2, 0.4, ())]
    rubble = [((0.4, 0.32, 0.3), 81, (-0.75, -0.6, 0.05), (10, 0, 30)),
              ((0.3, 0.26, 0.22), 83, (0.95, 0.55, 0.04), (0, 15, 70))]
    return column_group("SM_Cind_BasaltColumn_01", coll, cells, 0.3, 7, lean=(0, 3, 0), rubble=rubble[:0])


def columns_02(coll):
    """~500 cm. A tall organ of fourteen columns: a 5 m peak, a second summit and a staircase of stumps."""
    cells = [(0, 0, 5.0, (0.3, 0.62)), (1, 0, 4.3, (0.44, 0.8)), (0, 1, 3.7, (0.52,)),
             (-1, 1, 4.6, (0.36, 0.7)), (-1, 0, 3.1, (0.58,)), (0, -1, 2.5, (0.45,)),
             (1, -1, 3.4, (0.33, 0.7)), (2, -1, 1.7, (0.5,)), (2, 0, 2.2, (0.62,)), (1, 1, 1.35, ()),
             (-2, 1, 1.9, (0.4,)), (-1, -1, 1.1, ()), (-2, 2, 0.7, ()), (0, -2, 0.5, ()), (3, -2, 0.42, ())]
    rubble = [((0.6, 0.5, 0.4), 91, (-1.3, -1.0, 0.07), (0, 10, 40)),
              ((0.45, 0.36, 0.3), 93, (1.6, 1.0, 0.05), (12, 0, 100)),
              ((0.34, 0.28, 0.22), 97, (0.4, -1.6, 0.04), (0, 0, 20))]
    return column_group("SM_Cind_BasaltColumn_02", coll, cells, 0.36, 19, lean=(-2, 4, 0), rubble=rubble[:0])


# name, builder, texture set, triangle budget, collision
ASSETS = [
    ("SM_Cind_Crystal_01", crystal_01, "Cind_Crystals", (600, 2500), False),
    ("SM_Cind_Crystal_02", crystal_02, "Cind_Crystals", (600, 2500), True),
    ("SM_Cind_Crystal_03", crystal_03, "Cind_Crystals", (600, 2500), True),
    ("SM_Cind_Crystal_04", crystal_04, "Cind_Crystals", (600, 2500), True),
    ("SM_Cind_Basalt_01", basalt_01, "Cind_Basalt", (500, 2000), False),
    ("SM_Cind_Basalt_02", basalt_02, "Cind_Basalt", (500, 2000), True),
    ("SM_Cind_Basalt_03", basalt_03, "Cind_Basalt", (500, 2000), True),
    ("SM_Cind_BasaltColumn_01", columns_01, "Cind_Columns", (1, 2500), True),
    ("SM_Cind_BasaltColumn_02", columns_02, "Cind_Columns", (1, 2500), True),
]
SETS = ("Cind_Crystals", "Cind_Basalt", "Cind_Columns")


def materials():
    return {
        "Cind_Crystals": pk.flora_material("M_Cind_Crystals_Proc", EMISSION_STRENGTH, ground_dark=0.8),
        "Cind_Basalt": pk.basalt_material("M_Cind_Basalt_Proc", ASH, MAGMA, MAGMA_HOT, EMISSION_STRENGTH),
        "Cind_Columns": pk.basalt_material("M_Cind_Columns_Proc", ASH, MAGMA, MAGMA_HOT, EMISSION_STRENGTH),
    }


def fp_layout(o):
    return [
        (o["SM_Cind_Crystal_04"], 1.8, 3.4, 0), (o["SM_Cind_BasaltColumn_02"], -3.6, 2.4, 20),
        (o["SM_Cind_Crystal_03"], -1.2, -0.6, 40), (o["SM_Cind_Basalt_03"], 5.2, 0.4, 150),
        (o["SM_Cind_BasaltColumn_01"], 3.4, -2.2, 0), (o["SM_Cind_Crystal_02"], -3.6, -2.4, 200),
        (o["SM_Cind_Basalt_02"], 2.2, -3.4, 60), (o["SM_Cind_Crystal_01"], -1.2, -4.0, 0),
        (o["SM_Cind_Basalt_01"], 0.4, -4.6, 0), (o["SM_Cind_Crystal_01"], 3.9, -4.6, 130),
    ]


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    stage = argv[argv.index("--stage") + 1] if "--stage" in argv else "full"
    only = argv[argv.index("--only") + 1].split(",") if "--only" in argv else None
    if only and stage == "full":
        raise SystemExit("--only cannot be combined with --stage full (the atlases are shared)")
    os.makedirs(REVIEW_DIR, exist_ok=True)
    os.makedirs(TEX_DIR, exist_ok=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    coll = bpy.data.collections.new(KIT)
    scene.collection.children.link(coll)
    mats = materials()
    objs, hulls, summary = {}, {}, {}
    for name, fn, set_name, budget, _col in ASSETS:
        if only and not any(s in name for s in only):
            continue
        obj, hull_pts = fn(coll)
        obj.data.materials.append(mats[set_name])
        objs[name], hulls[name] = obj, hull_pts
        zs = [v.co.z for v in obj.data.vertices]
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        print("BUILT {:24s} tris {:5d}  budget {}  height {:.0f} cm  dims {}".format(
            name, tris, budget, max(zs) * 100, [round(d * 100) for d in obj.dimensions]))
    ref_coll = bpy.data.collections.new("REF")
    scene.collection.children.link(ref_coll)
    figure = pk.make_figure(ref_coll)
    items = [(objs[n], "{}  {:.0f} cm".format(n.replace("SM_Cind_", ""), max(v.co.z for v in objs[n].data.vertices) * 100))
             for n, *_ in ASSETS if n in objs]

    if stage == "clay":
        ppm = float(argv[argv.index("--ppm") + 1]) if "--ppm" in argv else 120.0
        r = pk.contact_sheet(items, figure, os.path.join(REVIEW_DIR, "_wip_clay_sheet.png"), mode="clay", ppm=ppm)
        print("__RESULT__" + json.dumps(r))
        return

    # ---- shared atlases + bake
    set_info = {}
    for set_name in SETS:
        group = [objs[n] for n, _f, s, _b, _c in ASSETS if s == set_name and n in objs]
        if not group:
            continue
        weights = [min(2.0, max(0.75, (1.6 / max(o.dimensions)) ** 0.5)) for o in group]
        set_info[set_name] = pk.atlas_bake(set_name, group, TEX_DIR, size=1024, weights=weights, angle=40.0,
                                           shape="AABB" if "--fast" in argv else "CONCAVE")
        print("BAKED", json.dumps(set_info[set_name]))

    # ---- collision
    for name, _f, _s, _b, col in ASSETS:
        if col and name in objs:
            pk.make_ucx(objs[name], hulls[name])

    # ---- measured checks
    for name, _f, set_name, budget, col in ASSETS:
        if name not in objs:
            continue
        c = pk.check_asset(name, budget, REVIEW_DIR)
        c.pop("asset_report")
        c["texture_set"] = set_name
        c["collision"] = bool(col)
        c["px_per_m"] = set_info[set_name]["px_per_m"].get(name)
        summary[name] = c
        print("CHECK {:24s} tris {:5d} in_budget {} errors {} warnings {} assembly {} h {} cm".format(
            name, c["tris"], c["in_budget"], c["errors"], c["warnings"], c["assembly_ok"], c["height_cm"]))

    # ---- review renders
    if stage in ("full", "review"):
        for name in objs:
            summary[name]["review"] = pk.review_asset(name, REVIEW_DIR)
    sheets = {}
    tag = "_wip" if only else "Cinder"
    sheets["lit"] = pk.contact_sheet(items, figure, os.path.join(REVIEW_DIR, tag + "_contact_sheet.png"), mode="lit")
    sheets["dark"] = pk.contact_sheet(items, figure, os.path.join(REVIEW_DIR, tag + "_contact_sheet_dark.png"),
                                      mode="dark")
    sheets["clay"] = pk.contact_sheet(items, figure, os.path.join(REVIEW_DIR, tag + "_contact_sheet_clay.png"),
                                      mode="clay")
    if not only:
        sheets["first_person"] = pk.first_person_view(
            fp_layout(objs), figure, os.path.join(REVIEW_DIR, "Cinder_first_person_dusk.png"),
            ground=(0.02, 0.016, 0.016), sky=(0.07, 0.025, 0.015), sky_strength=0.5,
            sun=(0.45, (1.0, 0.6, 0.4)), fig_at=(0.6, -1.0))
    if stage in ("bake", "review"):
        print("__RESULT__" + json.dumps({"sheets": sheets}))
        return

    # ---- per-asset .blend + FBX + manifest (this reloads the file, so nothing after it may use `objs`)
    names = [n for n, *_ in ASSETS if n in objs]
    sets_by_asset = {n: s for n, _f, s, _b, _c in ASSETS}
    bpy.data.objects.remove(figure, do_unlink=True)
    for m in list(mats.values()):
        if m.users == 0:
            bpy.data.materials.remove(m)
    extra = {n: {"collision_note": "one convex hull of the whole piece"} for n, _f, _s, _b, c in ASSETS if c}
    pack = pk.package(KIT_DIR, names, sets_by_asset, set_info, extra_manifest=extra)
    expect = {n: {"tris": pack[n]["tris"], "height_cm": summary[n]["height_cm"], "collision": summary[n]["collision"]}
              for n in names}
    rt = pk.fbx_round_trip(KIT_DIR, names, expect)
    for n in names:
        summary[n]["export"] = pack[n]
        summary[n]["fbx_round_trip"] = rt[n]
    out = {"kit": KIT, "blender": bpy.app.version_string, "assets": summary, "texture_sets": set_info,
           "sheets": {k: {kk: (os.path.basename(vv) if kk == "path" else vv) for kk, vv in v.items()}
                      for k, v in sheets.items()},
           "emission_strength": EMISSION_STRENGTH, "all_round_trips_ok": all(rt[n]["ok"] for n in names)}
    with open(os.path.join(REVIEW_DIR, "Cinder_summary.json"), "w", encoding="utf-8") as fh:
        json.dump(out, fh, indent=1)
    for n in names:
        print("FINAL {:24s} tris {:5d} h {:6.1f} cm  ucx {}  roundtrip {}".format(
            n, pack[n]["tris"], summary[n]["height_cm"], pack[n]["collision"], rt[n]["ok"]))
    print("__RESULT__" + json.dumps({"ok": out["all_round_trips_ok"]}))


if __name__ == "__main__":
    main()
