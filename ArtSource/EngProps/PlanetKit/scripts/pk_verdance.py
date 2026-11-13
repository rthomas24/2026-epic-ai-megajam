# SPDX-License-Identifier: MIT
"""
pk_verdance.py - Verdance set pieces: the ancient relay (stone platforms S / M / L, the plateau, ruin pillar, node
stub), a fallen bioluminescent log, and the crashed ship wreck (hull section, engine pod, wing and fin pieces).

Verdance language: organic, rounded, flowing; violet-grey carved stone with teal moss and faint cyan glyph lines;
the wreck is an original ivory / teal hull, torn and half-buried, overgrown with glowing moss and vines.
Walkable tops are flat at exactly the footprint height (flagstone joints are 2-3 cm chamfers below it); sides stay
within ~2 cm of the gameplay box (masonry relief is 0-2 cm).
"""

import math
import random

import bmesh
from mathutils import Matrix, Vector

import pk_lib as L
from pk_ext import role_stone, role_bark
from pk_geo import T, R, bx, prism, cham_rect, ngon, ring_loft, cut_below, shard, superellipse_ring

STONE_D, STONE_L, STONE_TINT = "#3A3D55", "#787A94", "#6A5A9E"
GLYPH = "#3CFFE0"


def stone_mats(prefix, top_z, scale=1.0, rings=None, moss=0.85):
    return {"stone": role_stone(prefix + "_Stone", STONE_D, STONE_L, scale=scale, moss=moss, top_z=top_z,
                                glyph_rings=rings, seed=3, block=1.2 * scale, tint=STONE_TINT),
            "band": role_stone(prefix + "_Band", "#2E3046", "#5E6080", scale=scale, moss=0.3, top_z=None, seed=5,
                               block=0.6, tint=STONE_TINT),
            "glyph": L.role_glow(prefix + "_Glyph", GLYPH, level=0.55, base="#123A3A")}


# =========================================================================== masonry

def _course_blocks(P, mat, length, z0, z1, side_frame, rng, block=(1.1, 1.9), relief=0.018, bevel=0.03,
                   offset=0.0, seg=2):
    """One course of blocks along a side. side_frame(u, depth) -> matrix placing a block centred at u (along the
    side) with its outer face on the box plane; blocks are 0.35 m deep."""
    u = -length * 0.5 - offset
    while u < length * 0.5 - 1e-6:
        w = rng.uniform(*block)
        a, b = max(u, -length * 0.5), min(u + w, length * 0.5)
        if b - a > 0.15:
            rel = rng.uniform(0.0, relief)
            o = L.box(P.coll, (b - a, 0.35, z1 - z0), ((a + b) * 0.5, -0.175 - rel, (z0 + z1) * 0.5))
            P.add(o, mat, bevel=bevel * rng.uniform(0.8, 1.3), segments=seg, matrix=side_frame)
        u += w


def _side_frames(X, Y):
    """Matrices that map local (u along side, y outward = -y local) to each of the 4 sides of an X x Y box.
    Local block: u along X, outer face at local y = 0, inner toward -y... we build blocks at local y in [-0.35, 0]
    then rotate so local -y points inward."""
    hx, hy = X * 0.5, Y * 0.5
    # local +y = outward normal
    return [(T(0, hy, 0) @ R(0, "Z"), X), (T(0, -hy, 0) @ R(180, "Z"), X),
            (T(hx, 0, 0) @ R(-90, "Z"), Y), (T(-hx, 0, 0) @ R(90, "Z"), Y)]


def _glyph_band(P, m, X, Y, z0, z1, rng, cell=0.5):
    """Recessed carved band (1.5 cm in) with glowing glyph strokes standing 5 mm proud of the band (so still
    1 cm inside the box): random vertical / horizontal / L / dot strokes per 0.5 m cell, edge lines top and bottom."""
    hx, hy = X * 0.5, Y * 0.5
    for mtx, length in _side_frames(X, Y):
        o = L.box(P.coll, (length - 0.06, 0.35, z1 - z0), (0, -0.175 - 0.015, (z0 + z1) * 0.5))
        P.add(o, m["band"], bevel=0.012, segments=1, matrix=mtx)
        h = z1 - z0
        w = 0.035
        for zz in (z0 + 0.06, z1 - 0.06 - w):
            o = L.box(P.coll, (length - 0.5, 0.014, w), (0, -0.011, zz + w * 0.5))
            P.add(o, m["glyph"], matrix=mtx)
        n = int((length - 0.6) / cell)
        for k in range(n):
            u = -length * 0.5 + 0.3 + (k + 0.5) * cell
            kind = rng.random()
            zc = (z0 + z1) * 0.5
            hh = h * 0.42
            strokes = []
            if kind < 0.3:
                strokes.append((u, zc - hh * 0.5, w, hh))                           # vertical
            elif kind < 0.55:
                strokes.append((u, zc - hh * 0.5, w, hh))
                strokes.append((u + cell * 0.15, zc + hh * 0.5 - w, cell * 0.3, w))  # L
            elif kind < 0.75:
                strokes.append((u, zc - w * 0.5, cell * 0.6, w))                     # horizontal
                strokes.append((u, zc - hh * 0.5, w * 1.6, w * 1.6))               # dot
            elif kind < 0.9:
                strokes.append((u - cell * 0.15, zc - hh * 0.5, w, hh * 0.6))
                strokes.append((u + cell * 0.15, zc - hh * 0.1, w, hh * 0.6))
            for (uc, zb, sw, sh) in strokes:
                o = L.box(P.coll, (sw, 0.014, sh), (uc, -0.011, zb + sh * 0.5))
                P.add(o, m["glyph"], matrix=mtx)


def _medallion(P, m, mtx, u, zc, rad):
    """Carved round medallion on a side face (outer face on the box plane, recessed centre, glowing ring and
    four ticks): the focal point of each side."""
    o = L.cyl(P.coll, rad, 0.06, segments=32, axis="Y", center=(u, -0.03, zc))
    P.add(o, m["band"], bevel=0.015, segments=2, matrix=mtx)
    o = L.cyl(P.coll, rad * 0.62, 0.06, segments=32, axis="Y", center=(u, -0.045, zc))
    P.add(o, m["band"], bevel=0.01, segments=1, matrix=mtx)
    ring = L.revolve_closed(P.coll, [(rad * 0.78, -0.004), (rad * 0.84, -0.004), (rad * 0.84, 0.0), (rad * 0.78, 0.0)],
                            (0.0, 360.0), 40, axis="Y")
    P.add(ring, m["glyph"], matrix=mtx @ T(u, 0.0, zc))
    for k in range(4):
        a = math.radians(45 + 90 * k)
        t = L.box(P.coll, (0.03, 0.006, rad * 0.25), (0, 0, 0))
        P.add(t, m["glyph"], matrix=mtx @ T(u + math.cos(a) * rad * 0.42, -0.015, zc + math.sin(a) * rad * 0.42) @ R(-math.degrees(a) + 90, "Y"))
    d = L.cyl(P.coll, rad * 0.12, 0.02, segments=12, axis="Y", center=(u, -0.012, zc))
    P.add(d, m["glyph"], matrix=mtx)


def _flagstones(P, mat, X, Y, H, n, thick, rng):
    """Top cap made of n x n flagstones, top faces exactly at H, chamfered joints, slightly varied thickness
    underneath only."""
    gx, gy = X / n, Y / n
    for i in range(n):
        for j in range(n):
            x0, x1 = -X * 0.5 + i * gx, -X * 0.5 + (i + 1) * gx
            y0, y1 = -Y * 0.5 + j * gy, -Y * 0.5 + (j + 1) * gy
            bx(P, mat, (x0 + 0.004, y0 + 0.004, H - thick), (x1 - 0.004, y1 - 0.004, H), bevel=0.028, seg=2)


def build_platform(P, X, Y, H, courses, band, flag_n, seed, prefix="Relay", rings_r=None, medallion=0.0,
                   block=(1.9, 3.2), seg=2, side_medallions=(), pilasters=()):
    rng = random.Random(seed)
    rings = [(0.0, 0.0, r) for r in (rings_r or [min(X, Y) * 0.3])]
    m = stone_mats(prefix, H, scale=max(0.6, min(X, Y) / 8.0), rings=rings)
    cap = 0.3 if H > 0.8 else 0.22
    # inner core (fills the box behind the facing blocks)
    bx(P, m["stone"], (-X * 0.5 + 0.3, -Y * 0.5 + 0.3, 0.0), (X * 0.5 - 0.3, Y * 0.5 - 0.3, H - cap + 0.01), bevel=None)
    # facing courses
    z = 0.0
    for ci, ch in enumerate(courses):
        if ci == band:
            _glyph_band(P, m, X, Y, z, z + ch, rng)
        else:
            for mtx, length in _side_frames(X, Y):
                _course_blocks(P, m["stone"], length, z, z + ch, mtx, rng, offset=rng.uniform(0.0, 0.8), block=block,
                               seg=seg, bevel=0.03 if seg > 1 else 0.04)
        z += ch
    # corner quoins: tall blocks at the corners (flush), alternate courses
    for sx in (-1, 1):
        for sy in (-1, 1):
            bx(P, m["stone"], (sx * X * 0.5 - (0.55 if sx > 0 else 0.0), sy * Y * 0.5 - (0.55 if sy > 0 else 0.0), 0.0),
               (sx * X * 0.5 + (0.0 if sx > 0 else 0.55), sy * Y * 0.5 + (0.0 if sy > 0 else 0.55), H - cap),
               bevel=0.04, seg=2)
    if medallion:
        zc = sum(courses[:band]) * 0.5 if band > 0 else courses[0] * 0.5
        rad = min(zc * 0.85, medallion)
        for mtx, length in _side_frames(X, Y):
            _medallion(P, m, mtx, 0.0, zc, rad)
            for u in side_medallions:
                _medallion(P, m, mtx, u, zc, rad * 0.62)
            # flush pilasters (darker stone) breaking up long faces, from the ground to the glyph band
            for u in pilasters:
                o = L.box(P.coll, (0.9, 0.4, sum(courses[:band])), (u, -0.2, sum(courses[:band]) * 0.5))
                P.add(o, m["band"], bevel=0.04, segments=1, matrix=mtx)
    _flagstones(P, m["stone"], X, Y, H, flag_n, cap, rng)


def build_plateau(P):
    X = Y = 26.0
    H = 6.0
    build_platform(P, X, Y, H, courses=[1.25, 1.2, 1.15, 1.15, 0.95], band=4, flag_n=6, seed=9, prefix="Plateau",
                   rings_r=[3.5, 7.0], medallion=2.1, block=(3.0, 5.2), seg=1, side_medallions=(-8.5, 8.5),
                   pilasters=(-11.6, -4.3, 4.3, 11.6))


def build_pillar(P):
    m = stone_mats("Pillar", 6.0, scale=0.5, moss=0.7)
    bx(P, m["stone"], (-0.6, -0.6, 0.0), (0.6, 0.6, 0.55), bevel=0.04, seg=2)
    bx(P, m["stone"], (-0.52, -0.52, 0.55), (0.52, 0.52, 0.85), bevel=0.035, seg=2)
    # fluted octagonal shaft
    prof = []
    for i in range(16):
        a = math.tau * i / 16
        r = 0.42 if i % 2 == 0 else 0.37
        prof.append((r * math.cos(a), r * math.sin(a)))
    prism(P, m["stone"], prof, "Z", 0.8, 5.35, bevel=0.015, seg=1)
    # glowing band with stone collars
    for z0, z1 in ((3.85, 4.0), (4.32, 4.47)):
        prism(P, m["stone"], ngon(0.48, 8, 22.5), "Z", z0, z1, bevel=0.02, seg=2)
    prism(P, m["glyph"], ngon(0.43, 16), "Z", 4.06, 4.26, bevel=None)
    prism(P, m["band"], ngon(0.44, 16), "Z", 4.0, 4.06, bevel=None)
    prism(P, m["band"], ngon(0.44, 16), "Z", 4.26, 4.32, bevel=None)
    # glyph strokes on the base faces
    rng = random.Random(4)
    for mtx in (T(0, -0.6, 0), T(0, 0.6, 0) @ R(180, "Z"), T(0.6, 0, 0) @ R(90, "Z"), T(-0.6, 0, 0) @ R(-90, "Z")):
        for k in range(3):
            u = -0.3 + 0.3 * k
            hgt = rng.uniform(0.15, 0.3)
            o = L.box(P.coll, (0.03, 0.008, hgt), (u, -0.002, 0.27))
            P.add(o, m["glyph"], matrix=mtx)
    # capital
    bx(P, m["stone"], (-0.5, -0.5, 5.3), (0.5, 0.5, 5.6), bevel=0.035, seg=2)
    bx(P, m["stone"], (-0.6, -0.6, 5.6), (0.6, 0.6, 6.0), bevel=0.045, seg=2)


def build_nodestub(P):
    H = 1.8
    m = stone_mats("Node", H, scale=0.5, rings=[(0, 0, 0.95)], moss=0.7)
    m["core"] = L.role_glow("Node_Core", GLYPH, level=1.0, base="#0E3A3A")
    prism(P, m["stone"], ngon(1.6 / math.cos(math.radians(22.5)), 8, 22.5), "Z", 0.0, 0.45, bevel=0.04, seg=2)
    prism(P, m["stone"], ngon(1.35 / math.cos(math.radians(22.5)), 8, 22.5), "Z", 0.45, 1.4, bevel=0.035, seg=2)
    # glyph channels down the 8 faces of the drum
    for k in range(8):
        a = math.radians(45 * k)
        mtx = R(45 * k, "Z")
        o = L.box(P.coll, (0.012, 0.05, 0.75), (1.352, 0.0, 0.92))
        P.add(o, m["glyph"], matrix=mtx)
    # top cap: lathe ring with a bowl socket (walkable rim at 1.8)
    # cap as an octagonal ring prism with an inner round hole: build as lathe of a closed profile
    capp = [(0.48, H - 0.06), (0.62, H), (1.5, H), (1.5, H - 0.4), (0.6, H - 0.4), (0.4, H - 0.3)]
    o = L.revolve_closed(P.coll, [(r, z) for r, z in capp], (0.0, 360.0), 32, axis="Z")
    P.add(o, m["stone"], bevel=0.02, segments=2, angle=35)
    # bowl and core
    bowl = L.lathe(P.coll, [(H - 0.7, 0.0), (H - 0.62, 0.42), (H - 0.32, 0.5)], segments=24)
    P.add(bowl, m["band"], smooth=50)
    core = L.lathe(P.coll, [(H - 0.62, 0.0), (H - 0.55, 0.22), (H - 0.4, 0.34), (H - 0.22, 0.3), (H - 0.1, 0.16), (H - 0.04, 0.0)], segments=20)
    P.add(core, m["core"], smooth=60)
    # four claw prongs holding the core (below the rim)
    for k in range(4):
        prism(P, m["stone"], [(0.25, H - 0.6), (0.48, H - 0.4), (0.46, H - 0.12), (0.3, H - 0.25)], "Y", -0.05, 0.05,
              bevel=0.012, seg=1, matrix=R(45 + 90 * k, "Z"))


# =========================================================================== log

def build_log(P):
    rng = random.Random(12)
    m = {"bark": role_bark("Log_Bark", scale=0.6),
         "wood": L.role_paint("Log_Wood", "#8A6A7E", "#6B4E64", metallic=0.0, rough=0.75, wear=0.0, grime=0.4, scale=0.4, seed=2),
         "fungus": L.role_paint("Log_Fungus", "#7B3FE4", "#5A2DB0", metallic=0.0, rough=0.55, wear=0.0, grime=0.1, scale=0.3, seed=4),
         "fglow": L.role_glow("Log_FungusGlow", "#3CFFE0", level=0.8, base="#103A36"),
         "vglow": L.role_glow("Log_VioletGlow", "#B07CFF", level=0.7, base="#2A1650")}
    # trunk: rings along X with bark ridges, elliptic section, slight bend
    n = 28
    rings = []
    stations = 22
    for i in range(stations + 1):
        t = i / float(stations)
        x = -2.5 + 5.0 * t
        taper = 1.0 - 0.12 * t
        ring = []
        for k in range(n):
            a = math.tau * k / n
            ridge = 1.0 + 0.045 * math.sin(a * 9 + t * 2.0) + 0.02 * math.sin(a * 23 + 1.3)
            r = 0.5 * taper * ridge
            y = math.cos(a) * r * 0.95
            z = 0.6 + math.sin(a) * r * 1.16 + 0.04 * math.sin(t * 3.1)
            ring.append((x, y, z))
        rings.append(ring)
    bm = ring_loft(P.coll, rings, cap0=True, cap1=False)
    # broken end at +X: splintered spikes (jag the last ring forward / back)
    obj = L.obj_from_bm(bm, P.coll)
    P.add(obj, m["bark"], smooth=40)
    # inner wood cap at +X, recessed, and splinters
    last = rings[-1]
    cx = last[0][0]
    face = L.lathe(P.coll, [(cx - 0.12, 0.0), (cx - 0.12, 0.4)], segments=24, axis="X", center=(0, 0, 0.0))
    P.add(face, m["wood"], matrix=T(0, 0, 0.6 + 0.04 * math.sin(3.1)) @ Matrix.Diagonal((1.0, 0.95, 1.16, 1.0)))
    for k in range(9):
        a = math.tau * k / 9 + rng.uniform(-0.2, 0.2)
        r = 0.34
        base = Vector((cx - 0.3, math.cos(a) * r * 0.95, 0.6 + math.sin(a) * r * 1.16))
        d = Vector((1.0, math.cos(a) * 0.15, math.sin(a) * 0.15))
        shard(P, m["wood"], base, d, rng.uniform(0.04, 0.08), rng.uniform(0.25, 0.45), sides=4, tip=0.6, seed=k)
    # root flare stubs at -X (inside the box)
    for k in range(5):
        a = math.radians(-40 + 70 * k)
        p0 = Vector((-2.3, math.cos(a) * 0.25, 0.6 + math.sin(a) * 0.3))
        p1 = Vector((-2.48, math.cos(a) * 0.38, 0.6 + math.sin(a) * 0.55))
        P.add(L.tube_path(P.coll, [p0, (p0 + p1) * 0.5, p1], 0.09, sides=8, radius_fn=lambda t: 1.0 - 0.6 * t), m["bark"], smooth=50)
    # branch stub
    P.add(L.tube_path(P.coll, [(0.6, 0.0, 1.05), (0.8, -0.1, 1.2), (0.95, -0.14, 1.3)], 0.1, sides=10,
                      radius_fn=lambda t: 1.0 - 0.5 * t), m["bark"], smooth=50)
    # shelf fungus brackets (violet caps, glowing teal gills) on top / sides, kept inside 1.1 x 1.4
    # shelf brackets on the flanks: placed on the trunk surface (embedded 6 cm), horizontal, pointing outward
    spots = [(-1.6, 1, 35), (-1.15, -1, 20), (-0.25, 1, 5), (0.25, 1, 40), (1.35, -1, 30), (1.8, 1, 15), (-0.6, -1, 45)]
    for i, (x, side, ang) in enumerate(spots):
        t = (x + 2.5) / 5.0
        r = 0.5 * (1.0 - 0.12 * t)
        a = math.radians(ang)
        py = side * math.cos(a) * r * 0.95
        pz = 0.6 + math.sin(a) * r * 1.16 + 0.04 * math.sin(t * 3.1)
        rr = rng.uniform(0.11, 0.15)
        cap = L.lathe(P.coll, [(0.0, rr), (0.04, rr * 0.92), (0.07, rr * 0.6), (0.08, 0.0)], segments=16,
                      arc=(-100.0, 100.0))
        mt = T(x, py - side * 0.05, pz - 0.03) @ R(90 * side, "Z")
        P.add(cap, m["fungus"], smooth=50, matrix=mt)
        gill = L.lathe(P.coll, [(-0.004, rr * 0.88), (0.0, 0.0)], segments=16, arc=(-95.0, 95.0))
        P.add(gill, m["fglow"] if i % 2 == 0 else m["vglow"], smooth=50, matrix=mt)
    # small glowing mushrooms on top
    for i in range(6):
        x = rng.uniform(-2.0, 1.9)
        y = rng.uniform(-0.12, 0.12)
        ztop = 0.6 + 0.5 * (1.0 - 0.12 * (x + 2.5) / 5.0) * 1.16
        stem = L.cyl(P.coll, 0.015, 0.16, segments=6, center=(x, y, ztop + 0.02))
        P.add(stem, m["wood"], smooth=50)
        capg = L.lathe(P.coll, [(0.0, 0.05), (0.03, 0.035), (0.045, 0.0)], segments=10)
        P.add(capg, m["fglow"], smooth=50, matrix=T(x, y, ztop + 0.09))


# =========================================================================== wreck

def wreck_mats(prefix, scale=1.5):
    return {"hull": L.role_paint(prefix + "_Hull", "#D9D2C1", "#BDB5A2", metallic=0.3, rough=0.45, wear=0.6,
                                 wear_color="#9AA2AA", fade=0.35, fade_color="#A9B0A0", grime=0.55,
                                 panel=(1.2, 0.018), scale=scale, seed=21, scorch=0.55),
            "livery": L.role_paint(prefix + "_Livery", "#19C3B1", "#12938A", metallic=0.3, rough=0.42, wear=0.6,
                                   fade=0.4, fade_color="#7FB3AA", grime=0.4, scale=scale, seed=22, scorch=0.3),
            "dark": L.role_paint(prefix + "_Dark", "#2B3038", "#22262C", metallic=0.6, rough=0.5, wear=0.5,
                                 wear_color="#8E969E", grime=0.4, scale=scale, seed=23),
            "metal": L.role_metal(prefix + "_Metal", "#7E868F", rough=0.42, scale=scale, seed=24),
            "moss": role_stone(prefix + "_Moss", "#16584C", "#1F7A66", scale=0.5, moss=1.0, seed=8, rough=0.9,
                               moss_color="#1E7A66"),
            "vine": role_bark(prefix + "_Vine", scale=0.3, seed=3),
            "glow": L.role_glow(prefix + "_Glow", "#3CFFE0", level=0.85, base="#0F3A36"),
            "vglow": L.role_glow(prefix + "_VGlow", "#B07CFF", level=0.75, base="#2A1650"),
            "glass": L.role_glow(prefix + "_Glass", "#1A5A70", level=0.35, base="#0A1A22"),
            "soil": role_stone(prefix + "_Soil", "#231F1A", "#3B3529", scale=0.5, moss=0.7, seed=12, rough=0.95,
                               moss_color="#1B5A4C")}


def _moss_blob(P, mat, centre, radius, flat, seed, ry=None):
    rng = random.Random(seed)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
    from mathutils import noise
    for v in bm.verts:
        n = noise.noise(v.co * 1.7 + Vector((seed, 0, 0)))
        v.co = Vector((v.co.x * radius * (1 + 0.25 * n), v.co.y * (ry or radius) * (1 + 0.25 * n), v.co.z * flat))
    o = L.obj_from_bm(bm, P.coll)
    return P.add(o, mat, smooth=60, matrix=T(*centre))


def _surface_z(objs_bvh, x, y, ztop=20.0):
    hit = None
    for bvh in objs_bvh:
        r = bvh.ray_cast(Vector((x, y, ztop)), Vector((0, 0, -1)))
        if r[0] is not None and (hit is None or r[0].z > hit.z):
            hit = r[0]
    return hit


def _vines_and_moss(P, m, bvh_parts, x_range, y_half, z_cap, rng, count=6, moss_count=10):
    from mathutils.bvhtree import BVHTree
    trees = []
    for o in bvh_parts:
        mw = o.matrix_world
        trees.append(BVHTree.FromPolygons([mw @ v.co for v in o.data.vertices], [p.vertices[:] for p in o.data.polygons]))
    # moss blobs on upward surfaces
    for k in range(moss_count):
        x = rng.uniform(*x_range)
        y = rng.uniform(-y_half * 0.55, y_half * 0.55)
        hit = _surface_z(trees, x, y)
        if hit is None:
            continue
        rad = rng.uniform(0.5, 1.2)
        _moss_blob(P, m["moss"], (hit.x, hit.y, min(hit.z, z_cap - 0.12)), rad, 0.12, seed=k + 11)
    # vines draped across the top, following the surface, with glowing pods
    for k in range(count):
        x0 = rng.uniform(*x_range)
        pts = []
        for j in range(15):
            t = j / 14.0
            y = (-0.92 + 1.84 * t) * y_half
            x = x0 + 0.6 * math.sin(t * 3.0 + k)
            hit = _surface_z(trees, x, y)
            if hit is None:
                continue
            pts.append(Vector((hit.x, hit.y, min(hit.z + 0.04, z_cap - 0.06))))
        if len(pts) >= 4:
            P.add(L.tube_path(P.coll, pts, rng.uniform(0.045, 0.07), sides=6), m["vine"], smooth=50)
            for j in range(2, len(pts) - 1, 3):
                pod = L.lathe(P.coll, [(0.0, 0.0), (0.04, 0.07), (0.12, 0.06), (0.17, 0.0)], segments=8,
                              center=tuple(pts[j] + Vector((0, 0, -0.02))))
                P.add(pod, m["glow"] if (j + k) % 2 else m["vglow"], smooth=50)


def build_wreck_hull(P):
    rng = random.Random(31)
    m = wreck_mats("Hull")
    Lx, hw, hh = 26.0, 4.5, 4.25
    zc = 2.35                                  # buried: centre 2.35 m above ground, top at ~7
    n = 40
    stations = 34
    rings = []
    for i in range(stations + 1):
        t = i / float(stations)
        x = -Lx * 0.5 + Lx * t
        # tail taper at -X (stern), full section toward the torn front
        s = 0.86 + 0.14 * math.sin(min(1.0, t * 1.6) * math.pi * 0.5)
        rings.append(superellipse_ring(hw * s, hh * s, n, e=2.4, zc=zc, x=x))
    bm = ring_loft(P.coll, rings, cap0=True, cap1=False)
    # torn front end: delete skin faces past a ragged line
    tear = {}
    for k in range(n):
        tear[k] = Lx * 0.5 - 0.3 - 3.2 * abs(math.sin(k * 0.61 + 0.4)) * rng.uniform(0.4, 1.0)
    dead = []
    for f in bm.faces:
        c = f.calc_center_median()
        a = (math.atan2(c.z - zc, c.y) / math.tau) % 1.0
        k = int(a * n) % n
        if c.x > tear[k]:
            dead.append(f)
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=0.14)
    skin = L.obj_from_bm(bm, P.coll)
    cut_below(skin, 0.0)
    P.add(skin, m["hull"], smooth=35)
    # interior: deck and inner bulkheads (visible through the tear), ribs past the torn edge
    bx(P, m["dark"], (-12.0, -3.6, 0.0), (11.5, 3.6, 0.9), bevel=0.03, seg=1)
    for xr in (7.0, 9.4, 11.8):
        for half in (-1, 1):
            pts = []
            for j in range(12):
                a = math.radians(-12 + (j / 11.0) * 104)
                y = half * (hw - 0.25) * math.copysign(abs(math.cos(a)) ** 0.8, math.cos(a))
                z = zc + (hh - 0.25) * math.sin(a)
                pts.append(Vector((xr, y, max(z, 0.05))))
            if xr > 11:
                pts = pts[:8]
            o = L.tube_path(P.coll, pts, 0.12, sides=6)
            P.add(o, m["metal"], smooth=40)
    # interior racks / conduits glimpsed through the tear
    for k in range(5):
        y = -2.5 + k * 1.25
        bx(P, m["dark"], (8.5, y - 0.3, 0.9), (10.5, y + 0.3, 2.2 + rng.uniform(0, 0.6)), bevel=0.03, seg=1)
    # peeled plates near the tear (bent, attached along one edge)
    for k, (y, z, ang) in enumerate(((3.8, 4.0, 35), (-3.2, 5.6, -40), (1.2, 6.65, 20))):
        pl = L.grid_plate(P.coll, 2.2, 1.6, 6, 4, 0.06, jag=0.6, jag_sides=(0, 1, 1, 1), seed=k)
        L.bend(pl, along="X", radius=2.5, start=0.0, plane_axis="Z")
        nrm = Vector((0, y, z - zc)).normalized()
        mt = T(Lx * 0.5 - 4.0, y * 0.97, z) @ R(math.degrees(math.atan2(nrm.y, nrm.z)) * -1.0, "X") @ R(ang, "Z")
        P.add(pl, m["hull"], smooth=30, matrix=mt)
    # livery stripe band and armour plates on the upper flanks (layered over the skin)
    for side in (-1, 1):
        for i in range(6):
            x0 = -11.0 + i * 3.4
            a = math.radians(38)
            y = side * (hw * 0.985) * math.cos(a) ** (2.0 / 2.4)
            z = zc + hh * 0.985 * math.sin(a) ** (2.0 / 2.4)
            o = L.box(P.coll, (3.1, 0.06, 0.55), (0, 0, 0))
            tilt = math.degrees(math.atan2(math.cos(a), math.sin(a))) * 0.55
            mt = T(x0 + 1.55, y, z) @ R(side * tilt, "X")
            P.add(o, m["livery"] if i % 2 == 0 else m["hull"], bevel=0.015, segments=1, matrix=mt)
        # window row (dim glass), upper side
        for i in range(7):
            x = -9.5 + i * 2.2
            a = math.radians(55)
            y = side * (hw * 0.99) * math.cos(a) ** (2.0 / 2.4)
            z = zc + hh * 0.99 * math.sin(a) ** (2.0 / 2.4)
            o = L.box(P.coll, (0.9, 0.08, 0.42), (0, 0, 0))
            mt = T(x, y, z) @ R(side * 28, "X")
            P.add(o, m["glass"], bevel=0.01, segments=1, matrix=mt)
    # engine pylon stubs on top at the stern (where the pods tore off)
    for side in (-1, 1):
        prism(P, m["dark"], [(-1.2, 0.0), (1.4, 0.0), (0.9, 0.9), (-0.8, 0.9)], "X", -11.5, -8.0, bevel=0.04, seg=2,
              matrix=T(0, side * 2.2, zc + hh * 0.86) @ R(side * 25, "X"))
    # vines and moss over the top
    _vines_and_moss(P, m, [skin], (-11.0, 9.0), hw, 7.0, rng, count=7, moss_count=12)


def build_wreck_engine(P):
    rng = random.Random(41)
    m = wreck_mats("Engine", scale=0.8)
    zc = 3.5
    prof = [(-4.5, 3.0), (-4.3, 3.35), (-3.6, 3.5), (2.2, 3.45), (3.4, 3.1), (4.5, 2.7)]
    bm = bmesh.new()
    segs = 40
    rings = []
    for x, r in prof:
        rings.append([(x, r * math.cos(math.tau * k / segs), zc + r * math.sin(math.tau * k / segs)) for k in range(segs)])
    bm = ring_loft(P.coll, rings, cap0=False, cap1=False)
    # break a chunk out of the upper housing
    dead = []
    for f in bm.faces:
        c = f.calc_center_median()
        a = math.degrees(math.atan2(c.z - zc, c.y))
        if 35 < a < 115 and -1.0 < c.x < 2.6 + 0.8 * math.sin(a * 0.2):
            dead.append(f)
    bmesh.ops.delete(bm, geom=dead, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bmesh.ops.solidify(bm, geom=list(bm.faces), thickness=0.16)
    shell = L.obj_from_bm(bm, P.coll)
    P.add(shell, m["hull"], smooth=35)
    # livery rings
    for x in (-3.3, 2.9):
        o = L.lathe(P.coll, [(x - 0.25, 3.5 if x < 0 else 3.25), (x + 0.25, 3.5 if x < 0 else 3.2)], segments=40, axis="X")
        P.add(o, m["livery"], smooth=35, matrix=T(0, 0, zc))
    # inner core: compressor drum, fan at the intake, nozzle cone at the exhaust
    core = L.lathe(P.coll, [(-3.9, 1.2), (-2.0, 1.6), (2.5, 1.5), (4.2, 0.9)], segments=24, axis="X")
    P.add(core, m["dark"], smooth=35, matrix=T(0, 0, zc))
    hub = L.lathe(P.coll, [(-4.35, 0.0), (-4.2, 0.45), (-3.9, 0.8)], segments=20, axis="X")
    P.add(hub, m["metal"], smooth=40, matrix=T(0, 0, zc))
    for k in range(14):
        a = 360.0 * k / 14
        if 60 < a < 120 and k % 2:
            continue                                     # broken blades
        o = L.box(P.coll, (0.08, 0.32, 2.2), (0, 0, 0))
        P.add(o, m["metal"], bevel=0.01, segments=1, matrix=T(-3.95, 0, zc) @ R(a, "X") @ T(0, 0, 1.75) @ R(25, "Z"))
    # exhaust glow ring and stator vanes (dim residual glow)
    ring = L.lathe(P.coll, [(4.1, 2.45), (4.25, 2.5)], segments=40, axis="X")
    P.add(ring, m["glow"], smooth=35, matrix=T(0, 0, zc))
    for k in range(10):
        o = L.box(P.coll, (0.5, 0.06, 1.2), (0, 0, 0))
        P.add(o, m["dark"], bevel=0.01, segments=1, matrix=T(3.9, 0, zc) @ R(36 * k, "X") @ T(0, 0, 1.85))
    # internal rings visible through the break
    for x in (-0.4, 0.8, 2.0):
        o = L.lathe(P.coll, [(x - 0.1, 3.3), (x + 0.1, 3.3)], segments=32, axis="X")
        P.add(o, m["metal"], smooth=35, matrix=T(0, 0, zc), deform=None)
    # pylon stub torn off on one side
    prism(P, m["dark"], [(-0.6, 0.0), (0.6, 0.0), (0.4, 0.8), (-0.3, 0.8)], "X", -2.5, 1.5, bevel=0.03, seg=2,
          matrix=T(0, -2.6, zc + 1.9) @ R(-40, "X"))
    _vines_and_moss(P, m, [shell], (-3.8, 3.8), 3.4, 7.0, rng, count=4, moss_count=6)


def _panel_piece(P, m, length, chord, thick, tilt, seed, fin=False):
    """Torn wing / fin panel standing on edge, partly buried: a plate (span along X, chord up), jagged torn top and
    root edges (boundary vertices pushed in at random, not whole cells), crumpled toward the root, tapered and swept
    for the fin, leaning by `tilt` degrees; livery stripe through both faces, ribs sticking out of the torn root,
    a dead nav light and moss at the ground line."""
    rng = random.Random(seed)
    pl = L.grid_plate(P.coll, length, chord, 44, 22, thick, jag=0.95, jag_sides=(0, 1, 0, 1), seed=seed)

    def shape(o, squash=True):
        me = o.data
        for vtx in me.vertices:
            t = min(1.0, max(0.0, (vtx.co.y + chord * 0.5) / chord))   # 0 at the ground edge, 1 at the top
            if fin:
                vtx.co.x = vtx.co.x * (1.0 - 0.38 * t) - 0.9 * t
            if squash:
                vtx.co.z *= 1.0 - 0.4 * t                            # thinner toward the top edge
            vtx.co.z += 0.05 * math.sin(vtx.co.x * 1.7 + seed) * math.sin(vtx.co.y * 2.1)   # crumple
        me.update()
    shape(pl)
    up = T(0, 0, chord * 0.5 - 0.45) @ R(tilt, "X") @ R(90, "X")
    P.add(pl, m["hull"], bevel=0.02, segments=1, matrix=up)
    # livery stripe (through both faces) and a dark leading-edge strip low on the panel
    st = L.grid_plate(P.coll, length * 0.7, 0.3, 14, 1, thick + 0.03)
    st.data.transform(T(-length * 0.1, chord * 0.05, 0))
    shape(st)
    P.add(st, m["livery"], bevel=0.01, segments=1, matrix=up)
    st2 = L.grid_plate(P.coll, length * 0.45, 0.1, 9, 1, thick + 0.03)
    st2.data.transform(T(-length * 0.2, chord * 0.05 + 0.3, 0))
    shape(st2)
    P.add(st2, m["livery"], bevel=0.008, segments=1, matrix=up)
    le = L.grid_plate(P.coll, length * 0.92, 0.22, 18, 1, thick * 1.15)
    le.data.transform(T(0.0, -chord * 0.5 + 0.35, 0))
    shape(le)
    P.add(le, m["dark"], bevel=0.02, segments=1, matrix=up)
    lens = L.box(P.coll, (0.25, 0.12, thick + 0.04), (-length * (0.25 if fin else 0.4), chord * 0.05, 0.0))
    shape(lens)
    P.add(lens, m["glow"], bevel=0.01, segments=1, matrix=up)
    # moss where the panel meets the ground (the ground line of the leaning panel)
    for xm, rad in ((-length * 0.25, 0.7), (length * 0.2, 0.5)):
        g = up @ Vector((xm, -chord * 0.5 + 0.45, 0.0))
        _moss_blob(P, m["moss"], (g.x, g.y, 0.05), rad, 0.2, seed + int(xm * 10), ry=0.16)
    return pl


def build_wreck_wing(P):
    m = wreck_mats("Wing", scale=0.6)
    _panel_piece(P, m, 6.0, 3.2, 0.32, 33.0, 51)


def build_wreck_fin(P):
    m = wreck_mats("Fin", scale=0.6)
    _panel_piece(P, m, 5.25, 3.0, 0.3, 35.0, 61, fin=True)


def _cut_all_below(P):
    for o in P.parts:
        mw = o.matrix_world.copy()
        o.data.transform(mw)
        o.matrix_world = Matrix.Identity(4)
        cut_below(o, 0.0)


def _with_cut(fn):
    def run(P):
        fn(P)
        _cut_all_below(P)
    return run


BUILDERS = {
    "SM_Verd_RelayPlatform_S": dict(fn=lambda P: build_platform(P, 6.0, 6.0, 1.1, [0.44, 0.44], 1, 2, 1),
                                    family="verdance", dims=(6.0, 6.0, 1.1), tex=2048, budget=(1000, 40000), ao=0.6, detail=True),
    "SM_Verd_RelayPlatform_M": dict(fn=lambda P: build_platform(P, 6.0, 6.0, 2.9, [1.0, 0.95, 0.65], 2, 2, 2, medallion=0.8),
                                    family="verdance", dims=(6.0, 6.0, 2.9), tex=2048, budget=(1000, 40000), ao=0.6, detail=True),
    "SM_Verd_RelayPlatform_L": dict(fn=lambda P: build_platform(P, 10.0, 10.0, 3.3, [1.2, 1.15, 0.65], 2, 3, 3, rings_r=[1.6, 3.2], medallion=0.9,
                                                   side_medallions=(-3.0, 3.0)),
                                    family="verdance", dims=(10.0, 10.0, 3.3), tex=2048, budget=(1000, 40000), ao=0.8, detail=True),
    "SM_Verd_Plateau": dict(fn=build_plateau, family="verdance", dims=(26.0, 26.0, 6.0), tex=2048,
                            budget=(1000, 40000), ao=1.2, detail=True),
    "SM_Verd_RuinPillar": dict(fn=build_pillar, family="verdance", dims=(1.2, 1.2, 6.0), tex=1024,
                               budget=(1000, 10000), ao=0.4, detail=True),
    "SM_Verd_NodeStub": dict(fn=build_nodestub, family="verdance", dims=(3.2, 3.2, 1.8), tex=1024,
                             budget=(1000, 10000), ao=0.4, detail=True),
    "SM_Verd_Log": dict(fn=build_log, family="verdance", dims=(5.0, 1.1, 1.4), tex=1024, budget=(2000, 10000), ao=0.3, snap=True),
    "SM_Verd_Wreck_Hull": dict(fn=_with_cut(build_wreck_hull), family="verdance", dims=(26.0, 9.0, 7.0), tex=2048,
                               budget=(2000, 40000), ao=1.5, snap=True),
    "SM_Verd_Wreck_Engine": dict(fn=_with_cut(build_wreck_engine), family="verdance", dims=(9.0, 7.0, 7.0),
                                 tex=2048, budget=(2000, 40000), ao=1.0, snap=True),
    "SM_Verd_Wreck_Wing": dict(fn=_with_cut(build_wreck_wing), family="verdance", dims=(6.0, 1.6, 2.4), tex=1024,
                               budget=(1000, 10000), ao=0.5, snap=False),
    "SM_Verd_Wreck_Fin": dict(fn=_with_cut(build_wreck_fin), family="verdance", dims=(5.0, 1.6, 2.2), tex=1024,
                              budget=(1000, 10000), ao=0.5, snap=False),
}
