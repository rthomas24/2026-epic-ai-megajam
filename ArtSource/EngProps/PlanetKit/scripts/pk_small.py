# SPDX-License-Identifier: MIT
"""
pk_small.py - small interactables shared by both planets: a neutral alien terminal (SM_Console), the energy
core canister the player carries (SM_ExtractionCore, pivot at the centre), the slim route marker post
(SM_RoutePost) and the objective trailhead plinth + marker column (SM_Trailhead).
Neutral palette (works on both worlds): gunmetal, warm ivory, ancient violet-grey stone; cyan glow.
"""

import math
import random

from mathutils import Vector

import pk_lib as L
from pk_ext import role_stone
from pk_geo import T, R, bx, prism, cham_rect, ngon

GLOW = "#3FE9FF"


def neutral_mats(prefix, scale=0.3):
    return {"body": L.role_paint(prefix + "_Body", "#3A3F48", "#2C3038", metallic=0.55, rough=0.42, wear=0.5,
                                 wear_color="#A3AAB2", grime=0.3, panel=(0.3, 0.006), scale=scale, seed=31),
            "ivory": L.role_paint(prefix + "_Ivory", "#D8D2C4", "#C2BBAB", metallic=0.25, rough=0.45, wear=0.4,
                                  grime=0.25, scale=scale, seed=32),
            "metal": L.role_metal(prefix + "_Metal", "#8A9199", rough=0.35, scale=scale, seed=33),
            "glow": L.role_glow(prefix + "_Glow", GLOW, level=1.0, base="#0C2A33"),
            "screen": L.role_glow(prefix + "_Screen", GLOW, level=0.9, base="#06161C", pattern="bars", scale=0.15)}


def build_console(P):
    m = neutral_mats("Console")
    # foot: chamfered plinth, waisted column, angled reading head
    prism(P, m["body"], cham_rect(-0.3, 0.3, -0.3, 0.3, 0.08), "Z", 0.0, 0.1, bevel=0.012, seg=2)
    prism(P, m["ivory"], cham_rect(-0.22, 0.22, -0.22, 0.22, 0.06), "Z", 0.1, 0.18, bevel=0.01, seg=2)
    stem = L.lathe(P.coll, [(0.18, 0.15), (0.45, 0.11), (0.8, 0.13), (0.9, 0.16)], segments=16)
    P.add(stem, m["body"], smooth=30)
    # two side fins up the stem
    for s in (-1, 1):
        prism(P, m["ivory"], [(-0.12, 0.18), (0.12, 0.18), (0.06, 0.85), (-0.1, 0.92)], "Y", s * 0.12 - 0.015, s * 0.12 + 0.015,
              bevel=0.006, seg=1, matrix=R(0, "Z"))
    # head: wedge tilted toward +X (the user stands on +X), screen inset on the slope
    head = [(-0.24, 0.85), (0.26, 0.85), (0.3, 0.95), (-0.08, 1.2), (-0.26, 1.2), (-0.3, 1.0)]
    prism(P, m["body"], head, "Y", -0.27, 0.27, bevel=0.015, seg=2)
    # screen on the sloped face (normal ~ (0.55, 0, 0.83))
    a = math.degrees(math.atan2(0.38, 0.25))
    scr = L.box(P.coll, (0.36, 0.42, 0.012), (0, 0, 0))
    P.add(scr, m["screen"], matrix=T(0.11, 0.0, 1.075) @ R(90 - a, "Y"))
    bez = L.box(P.coll, (0.4, 0.47, 0.01), (0, 0, 0))
    P.add(bez, m["metal"], bevel=0.003, segments=1, matrix=T(0.105, 0.0, 1.071) @ R(90 - a, "Y"))
    # status light strip and a rim
    bx(P, m["glow"], (0.265, -0.18, 0.88), (0.3, 0.18, 0.9), bevel=None)
    bx(P, m["metal"], (-0.3, -0.27, 1.17), (-0.06, 0.27, 1.2), bevel=0.006, seg=1)


def build_core(P):
    m = neutral_mats("Core", scale=0.2)
    # canister along Z, pivot at the centre: end caps, guard bars, glowing core, carry handle
    for z, sgn in ((-0.55, 1), (0.55, -1)):
        cap = L.lathe(P.coll, [(z, 0.0), (z, 0.24), (z + sgn * 0.05, 0.275), (z + sgn * 0.17, 0.27),
                               (z + sgn * 0.2, 0.22), (z + sgn * 0.2, 0.0)], segments=24)
        P.add(cap, m["body"], bevel=0.008, segments=1, angle=35)
        ring = L.lathe(P.coll, [(z + sgn * 0.2, 0.2), (z + sgn * 0.24, 0.2)], segments=24)
        P.add(ring, m["metal"], smooth=40)
        bx(P, m["glow"], (-0.276, -0.03, z + sgn * 0.08 - 0.015), (-0.27, 0.03, z + sgn * 0.08 + 0.015), bevel=None)
    core = L.lathe(P.coll, [(-0.36, 0.0), (-0.34, 0.13), (-0.2, 0.17), (0.2, 0.17), (0.34, 0.13), (0.36, 0.0)], segments=32)
    P.add(core, m["glow"], smooth=50)
    inner = L.lathe(P.coll, [(-0.36, 0.08), (0.36, 0.08)], segments=8)
    P.add(inner, m["ivory"], smooth=40)
    for k in range(4):
        a = math.radians(45 + 90 * k)
        x, y = math.cos(a) * 0.215, math.sin(a) * 0.215
        o = L.cyl(P.coll, 0.032, 0.74, segments=8, center=(x, y, 0.0))
        P.add(o, m["metal"], bevel=0.006, segments=1)
    # carry handle on one side (inside 55 x 55)
    pts = [(0.2, 0.0, -0.4), (0.25, 0.0, -0.3), (0.255, 0.0, 0.0), (0.25, 0.0, 0.3), (0.2, 0.0, 0.4)]
    P.add(L.tube_path(P.coll, pts, 0.016, sides=8), m["ivory"], smooth=40, matrix=R(0, "Z"))


def build_routepost(P):
    m = neutral_mats("RoutePost", scale=0.15)
    m["bulb"] = L.role_glow("RoutePost_Bulb", GLOW, level=1.0, base="#0C2A33")
    # slender tapered three-vane post with a flared foot, collar, cradle prongs and a teardrop bulb
    foot = L.lathe(P.coll, [(0.0, 0.088), (0.03, 0.084), (0.06, 0.05), (0.1, 0.04)], segments=24)
    P.add(foot, m["body"], smooth=40)
    shaft = L.lathe(P.coll, [(0.08, 0.04), (0.6, 0.032), (1.12, 0.025), (1.18, 0.03)], segments=24)
    P.add(shaft, m["ivory"], smooth=40)
    for k in range(3):
        prism(P, m["body"], [(0.02, 0.06), (0.088, 0.1), (0.05, 0.75), (0.02, 1.0)], "Y", -0.006, 0.006,
              bevel=0.002, seg=1, matrix=R(120 * k, "Z"))
    collar = L.lathe(P.coll, [(1.14, 0.045), (1.2, 0.055), (1.23, 0.04)], segments=24)
    P.add(collar, m["metal"], smooth=40)
    bulb = L.lathe(P.coll, [(1.2, 0.0), (1.24, 0.09), (1.31, 0.155), (1.37, 0.17), (1.43, 0.15), (1.5, 0.09),
                            (1.54, 0.0)], segments=32)
    P.add(bulb, m["bulb"], smooth=60)
    for k in range(3):
        a = math.radians(60 + 120 * k)
        pts = [(math.cos(a) * 0.045, math.sin(a) * 0.045, 1.2)]
        for t in (0.25, 0.5, 0.75, 1.0):
            r = 0.05 + 0.115 * math.sin(t * math.pi * 0.85)
            pts.append((math.cos(a) * r, math.sin(a) * r, 1.2 + 0.26 * t))
        P.add(L.tube_path(P.coll, pts, 0.008, sides=8), m["metal"], smooth=40)


def build_trailhead(P):
    rng = random.Random(5)
    m = {"stone": role_stone("Trail_Stone", "#3A3D55", "#787A94", scale=0.6, moss=0.5, top_z=0.4,
                             glyph_rings=[(0, 0, 1.6), (0, 0, 2.15)], seed=7, block=1.0, tint="#6A5A9E"),
         "col": role_stone("Trail_Column", "#34374E", "#6E7090", scale=0.4, moss=0.35, top_z=4.8, seed=8, block=0.8),
         "glow": L.role_glow("Trail_Glow", GLOW, level=1.0, base="#0C2A33"),
         "metal": L.role_metal("Trail_Metal", "#8A9199", rough=0.35, scale=0.3, seed=9)}
    # plinth 5 x 5 x 0.4: chamfered border blocks + four inner flagstones, top flat at 0.4
    for i in range(2):
        for j in range(2):
            bx(P, m["stone"], (-2.5 + i * 2.5 + 0.004, -2.5 + j * 2.5 + 0.004, 0.0),
               (-2.5 + (i + 1) * 2.5 - 0.004, -2.5 + (j + 1) * 2.5 - 0.004, 0.4), bevel=0.03, seg=2)
    # marker column 4.4 m above the plinth: tapered square obelisk with glyph strips, glowing crown
    prism(P, m["col"], cham_rect(-0.55, 0.55, -0.55, 0.55, 0.12), "Z", 0.4, 0.75, bevel=0.03, seg=2)
    rings = []
    for z, w in ((0.75, 0.82), (3.9, 0.56)):
        rings.append([(x, y, z) for x, y in cham_rect(-w * 0.5, w * 0.5, -w * 0.5, w * 0.5, w * 0.12)])
    from pk_geo import ring_loft
    P.add(L.obj_from_bm(ring_loft(P.coll, rings), P.coll), m["col"], bevel=0.02, segments=2)
    for k in range(4):
        mt = R(90 * k, "Z")
        for z0, z1 in ((1.0, 2.2), (2.45, 3.6)):
            wz = 0.41 - 0.13 * ((z0 + z1) * 0.5 - 0.75) / 3.15
            o = L.box(P.coll, (0.012, 0.045, z1 - z0), (wz + 0.003, 0.0, (z0 + z1) * 0.5))
            P.add(o, m["glow"], matrix=mt)
    # collar, crown prongs and the glowing crystal at the top (to 4.8)
    prism(P, m["col"], cham_rect(-0.36, 0.36, -0.36, 0.36, 0.08), "Z", 3.85, 4.05, bevel=0.02, seg=2)
    for k in range(4):
        prism(P, m["metal"], [(0.12, 4.0), (0.3, 4.05), (0.3, 4.35), (0.2, 4.7), (0.16, 4.45)], "Y", -0.03, 0.03,
              bevel=0.006, seg=1, matrix=R(45 + 90 * k, "Z"))
    gem = L.lathe(P.coll, [(4.05, 0.0), (4.15, 0.12), (4.4, 0.17), (4.62, 0.1), (4.8, 0.0)], segments=6)
    P.add(gem, m["glow"])


BUILDERS = {
    "SM_Console": dict(fn=build_console, family="verdance", dims=(0.6, 0.6, 1.2), tex=1024, budget=(1000, 10000), ao=0.2),
    "SM_ExtractionCore": dict(fn=build_core, family="verdance", dims=(0.55, 0.55, 1.1), tex=1024, budget=(1000, 10000),
                              ao=0.15, pivot="centre"),
    "SM_RoutePost": dict(fn=build_routepost, family="verdance", dims=(0.34, 0.34, 1.54), tex=1024, budget=(1000, 10000),
                         ao=0.1),
    "SM_Trailhead": dict(fn=build_trailhead, family="verdance", dims=(5.0, 5.0, 4.8), tex=1024, budget=(1000, 10000),
                         ao=0.5, detail=True),
}
