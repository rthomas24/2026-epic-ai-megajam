# SPDX-License-Identifier: MIT
"""
pk_cinder.py - Cinder set pieces: the Null Armada enemy outpost (wall, doorway, sliding door, armoured crates, the
signal jammer tower) and the basalt route pieces (choke rock walls, stepping stone, collapse slab, debris chunk).

Null Armada language (aaa-quality-bar.md): blade-like angular plates, layered matte-black armour, dark crimson
secondary plates, red-orange emissive seams, white "eye" lights, forward-raked spikes. Cinder rock: black basalt
columns, ash on top faces, cyan / amber crystal veins, magma glow only near the lava line.
Units metres; Unreal forward = Blender +X; pivot bottom centre unless noted. Footprints are fitted exactly by
pk_ext.fit_box afterwards; the builders already build to size so the correction stays under ~1 %.
"""

import math
import random

from mathutils import Vector

import pk_lib as L
from pk_ext import role_basalt
from pk_geo import T, R, bx, prism, blade_rect, cham_rect, hex_column, shard, ngon, ring_loft

ARMOUR = "#26262E"
ARMOUR2 = "#1A1A20"
CRIMSON = "#5A0C16"
GUNMETAL = "#262A31"
SEAM = "#FF4A1E"           # red-orange
EYE = "#F4F7FF"


def null_mats(prefix, scale=1.5):
    m = {}
    m["armour"] = L.role_paint(prefix + "_Armour", ARMOUR, ARMOUR2, metallic=0.45, rough=0.36, wear=0.35,
                               wear_color="#6A6E78", grime=0.3, panel=(1.25 * scale, 0.012), scale=scale,
                               bump=0.35, seed=11, scorch=0.35)
    m["crimson"] = L.role_paint(prefix + "_Crimson", CRIMSON, "#3A0810", metallic=0.35, rough=0.4, wear=0.4,
                                wear_color="#8A6A6E", grime=0.3, scale=scale, bump=0.35, seed=12)
    m["trim"] = L.role_paint(prefix + "_Trim", GUNMETAL, "#1E2127", metallic=0.7, rough=0.42, wear=0.55,
                             wear_color="#9AA0A8", grime=0.35, scale=scale, seed=13)
    m["metal"] = L.role_metal(prefix + "_Metal", "#5C626B", rough=0.4, metallic=0.9, scale=scale, seed=14)
    m["seam"] = L.role_glow(prefix + "_Seam", SEAM, level=1.0)
    m["eye"] = L.role_glow(prefix + "_Eye", EYE, level=0.9)
    m["hazard"] = L.role_paint(prefix + "_Hazard", "#C3122E", metallic=0.3, rough=0.45, wear=0.5,
                               stripes=("#C3122E", "#141418", 0.18, (1.0, 0.0, 1.0)), scale=scale, seed=15)
    return m


# =========================================================================== outpost wall

WALL_T0, WALL_T1 = -0.75, 0.75          # thickness (X); inner face -X flush, outer face +X carries the buttresses
CORE_IN, CORE_OUT = -0.73, 0.25
WALL_H = 6.0
BUTTRESS_Y = (-5.625, -1.875, 1.875, 5.625)   # 3.75 m rhythm, continues across tiles


def _body_profile():
    return [(-0.75, 0.0), (0.75, 0.0), (0.75, 0.16), (0.6, 0.34), (CORE_OUT, 0.42), (CORE_OUT, 5.3),
            (CORE_IN, 5.3), (CORE_IN, 0.42), (-0.75, 0.34)]


CAP_TOP = 5.6                          # merlons rise from the cap to 6.0


def _cap_profile():
    return [(CORE_IN, 5.2), (CORE_OUT, 5.2), (0.75, 5.46), (0.75, 5.62), (0.6, CAP_TOP), (-0.6, CAP_TOP),
            (-0.75, 5.62), (-0.75, 5.46)]


def _merlon(P, m, y, size=1.0):
    """Forward-raked blade merlon on the cap (Null silhouette), 1.2 m wide across the wall, top at 6.0."""
    # blade crest: a plate across the wall (Y thickness 0.24) whose profile in X-Z rakes forward over the outer
    # (+X) face and ends in a point at 6.0 m
    prof = [(-0.6, CAP_TOP - 0.06), (0.62, CAP_TOP - 0.06), (0.74, 6.0), (0.5, 6.0), (-0.1, CAP_TOP + 0.22),
            (-0.6, CAP_TOP + 0.12)]
    for dy in (-0.32 * size, 0.32 * size):
        prism(P, m["armour"], prof, "Y", y + dy - 0.12, y + dy + 0.12, bevel=0.015, seg=2)
    prism(P, m["crimson"], [(-0.45, CAP_TOP - 0.04), (0.5, CAP_TOP - 0.04), (0.55, CAP_TOP + 0.2), (-0.2, CAP_TOP + 0.12)],
          "Y", y - 0.2 * size, y + 0.2 * size, bevel=0.01, seg=1)


def _chunks(y0, y1, step=2.5, from_low=True):
    """Split [y0, y1] into panels of `step` measured from the tile end, remainder at the far end."""
    out = []
    if from_low:
        a = y0
        while a < y1 - 1e-6:
            b = min(a + step, y1)
            out.append((a, b))
            a = b
    else:
        b = y1
        while b > y0 + 1e-6:
            a = max(b - step, y0)
            out.insert(0, (a, b))
            b = a
    return out


def _inner_panels(P, m, y0, y1, from_low=True, glow_ends=(True, True)):
    """Inner face (-X): blade-cut armour plates 2.5 m wide in two rows, a vent band at the base, red-orange seams
    between plates and a lit line under the cap."""
    xf = WALL_T0                                 # plate face exactly on the box
    gap = 0.06
    chunks = _chunks(y0, y1, 2.5, from_low)
    for i, (a, b) in enumerate(chunks):
        ya, yb = a + gap * 0.5, b - gap * 0.5
        if yb - ya < 0.5:
            continue
        upper = blade_rect(ya, yb, 3.0, 5.15, 0.35) if i % 2 == 0 else blade_rect(-yb, -ya, 3.0, 5.15, 0.35)
        if i % 2:
            upper = [(-u, v) for u, v in upper]
        prism(P, m["crimson"] if i % 2 else m["armour"], upper, "X", xf, xf + 0.04, bevel=0.012, seg=2)
        # horizontal trim rail between the rows (gunmetal), clamps where it crosses the seams
        bx(P, m["trim"], (xf + 0.005, ya, 2.93), (xf + 0.05, yb, 3.0), bevel=0.01, seg=1)
        lower = cham_rect(ya, yb, 1.18, 2.92, 0.12)
        prism(P, m["crimson"] if i % 3 == 1 else m["armour"], lower, "X", xf, xf + 0.04, bevel=0.012, seg=2)
        # vent band: slats behind a frame (recessed 1 cm), base functional detail
        bx(P, m["trim"], (xf + 0.01, ya, 0.5), (xf + 0.05, yb, 1.1), bevel=0.01, seg=1)
        n = max(3, int((yb - ya) / 0.16))
        for k in range(5):
            z = 0.58 + k * 0.105
            bx(P, m["metal"], (xf + 0.004, ya + 0.08, z), (xf + 0.03, yb - 0.08, z + 0.045), bevel=0.006, seg=1)
        # bolts / clamp blocks at the plate corners (tertiary)
        for yy in (ya + 0.12, yb - 0.12):
            bx(P, m["trim"], (xf + 0.0, yy - 0.05, 2.97), (xf + 0.03, yy + 0.05, 3.03 + 0.0), bevel=0.008, seg=1)
    # seams: vertical red-orange strips in the plate gaps (half strips at tile ends complete across tiles)
    edges = sorted(set([c[0] for c in chunks] + [chunks[-1][1]]))
    for e in edges:
        at_end = abs(e - y0) < 1e-6 or abs(e - y1) < 1e-6
        if at_end:
            lo_end = abs(e - y0) < 1e-6
            if not glow_ends[0 if lo_end else 1]:
                continue
            ya, yb = (e, e + 0.022) if lo_end else (e - 0.022, e)
        else:
            ya, yb = e - 0.022, e + 0.022
        bx(P, m["seam"], (xf + 0.012, ya, 1.2), (xf + 0.045, yb, 5.1), bevel=None)
    bx(P, m["seam"], (xf + 0.004, y0, 5.24), (xf + 0.03, y1, 5.3), bevel=None)


def _outer_panels(P, m, y0, y1, buttress_y):
    """Outer face (+X): plates in the bays between buttresses, a lit line under the cap."""
    xf = CORE_OUT
    cuts = [y0] + sorted(y for b in buttress_y for y in (b - 0.42, b + 0.42) if y0 < y < y1) + [y1]
    bays = [(cuts[i], cuts[i + 1]) for i in range(0, len(cuts) - 1, 2)] if cuts else []
    for a, b in bays:
        ya, yb = a + 0.05, b - 0.05
        if yb - ya < 0.6:
            continue
        prism(P, m["armour"], cham_rect(ya, yb, 0.6, 2.75, 0.18), "X", xf - 0.02, xf + 0.03, bevel=0.012, seg=2)
        prism(P, m["armour"], cham_rect(ya, yb, 2.85, 5.05, 0.18), "X", xf - 0.02, xf + 0.03, bevel=0.012, seg=2)
        # hazard band low on the bay (Null markings)
        bx(P, m["hazard"], (xf - 0.01, ya + 0.2, 0.48), (xf + 0.045, yb - 0.2, 0.56), bevel=0.005, seg=1)
    bx(P, m["seam"], (xf - 0.01, y0, 5.12), (xf + 0.035, y1, 5.18), bevel=None)


def _buttress(P, m, y):
    """Outer buttress: vertical front to 2.4 m then raked back into the cap; crimson inlay, red-orange slit,
    paired white eye lights. Front at x 0.745 so the slit sits 5 mm proud, inside the box."""
    w = 0.36
    prof = [(CORE_OUT - 0.05, 0.3), (0.745, 0.3), (0.745, 2.4), (0.6, 4.6), (0.52, 5.5), (CORE_OUT - 0.05, 5.5)]
    prism(P, m["armour"], prof, "Y", y - w, y + w, bevel=0.025, seg=2)
    # spur plates on both sides (layered armour)
    for s in (-1, 1):
        sp = [(CORE_OUT - 0.02, 0.4), (0.7, 0.4), (0.7, 1.5), (0.55, 3.2), (CORE_OUT - 0.02, 3.6)]
        a, b = (y + w, y + w + 0.07) if s > 0 else (y - w - 0.07, y - w)
        prism(P, m["crimson"], sp, "Y", a, b, bevel=0.012, seg=1)
    # front inlay + slit + eyes
    bx(P, m["trim"], (0.73, y - 0.16, 0.55), (0.75, y + 0.16, 2.25), bevel=0.01, seg=1)
    bx(P, m["seam"], (0.745, y - 0.03, 0.7), (0.75, y + 0.03, 2.1), bevel=None)
    for s in (-1, 1):
        bx(P, m["eye"], (0.735, y + s * 0.09 - 0.045, 2.28), (0.75, y + s * 0.09 + 0.045, 2.33), bevel=None)
    # foot block anchoring it into the plinth
    bx(P, m["trim"], (0.55, y - w - 0.05, 0.0), (0.75, y + w + 0.05, 0.34), bevel=0.02, seg=2)


def _cap_details(P, m, y0, y1):
    """Cap: armour segments with blade notches every 2.5 m on top (stay under 6.0 m)."""
    for a, b in _chunks(y0, y1, 2.5):
        bx(P, m["trim"], (-0.5, a + 0.04, CAP_TOP - 0.03), (0.5, b - 0.04, CAP_TOP), bevel=0.008, seg=1)


def build_wall(P, doorway=False):
    m = null_mats("Wall")
    y0, y1 = -7.5, 7.5
    if not doorway:
        prism(P, m["armour"], _body_profile(), "Y", y0, y1, bevel=0.03, seg=2)
        prism(P, m["armour"], _cap_profile(), "Y", y0, y1, bevel=0.03, seg=2)
        _inner_panels(P, m, y0, y1)
        _outer_panels(P, m, y0, y1, BUTTRESS_Y)
        for y in BUTTRESS_Y:
            _buttress(P, m, y)
            _merlon(P, m, y)
        _cap_details(P, m, y0, y1)
        return
    # ---- doorway FRAME (lead correction): fills the 6 m gap gameplay leaves between two plain wall segments.
    # Envelope 1.5 (X) x 6.0 (Y) x 6.0 (Z); opening 4.4 m wide x 5.0 m tall; heavy posts 0.8 m wide whose outer
    # faces (y = +-3.0) meet the neighbouring walls flush; lintel built in above the opening, top at 6.0.
    ow, oh, gy = 2.2, 5.0, 3.0
    for s in (-1, 1):
        a, b = (ow, gy) if s > 0 else (-gy, -ow)
        # post: full-thickness block, blade chamfer on the opening edges only
        prof = [(-0.71, 0.0), (0.71, 0.0), (0.71, oh + 0.2), (0.58, 6.0), (-0.58, 6.0), (-0.71, oh + 0.2)]
        prism(P, m["armour"], prof, "Y", a, b, bevel=0.03, seg=2)
        # pilaster plates on both faces (layered armour, crimson inlay), inside the box
        for xs in (-1, 1):
            xa, xb = (0.71, 0.745) if xs > 0 else (-0.745, -0.71)
            pa, pb = (a + 0.08, b - 0.1) if s > 0 else (a + 0.1, b - 0.08)
            plate = blade_rect(pa, pb, 0.35, 4.7, 0.25)
            if s < 0:
                plate = [(a + b - u, v) for u, v in plate]
            prism(P, m["crimson"], plate, "X", xa, xb, bevel=0.01, seg=1)
            # framing light on the opening edge of each face, eye pair above it
            yl = s * (ow + 0.09)
            xf = 0.745 if xs > 0 else -0.75
            bx(P, m["seam"], (0.69 if xs > 0 else -0.75, yl - 0.03, 0.25), (0.75 if xs > 0 else -0.69, yl + 0.03, oh - 0.1), bevel=None)
            for e in (-1, 1):
                bx(P, m["eye"], (0.69 if xs > 0 else -0.75, s * (ow + 0.45) + e * 0.1 - 0.04, 4.86),
                   (0.75 if xs > 0 else -0.69, s * (ow + 0.45) + e * 0.1 + 0.04, 4.92), bevel=None)
            # heavy base block on each face
            bx(P, m["trim"], (0.62 if xs > 0 else -0.75, a, 0.0), (0.75 if xs > 0 else -0.62, b, 0.32), bevel=0.02, seg=2)
        # reveal: trim liner on the opening side of the post, flush at y = +-2.2 (the opening stays clear)
        ra, rb = (ow, ow + 0.06) if s > 0 else (-ow - 0.06, -ow)
        bx(P, m["trim"], (-0.6, ra, 0.0), (0.6, rb, oh), bevel=0.01, seg=1)
        # vertical door-track slot lights on the reveal, set 5 mm inside the post
        yr = s * (ow + 0.005)
        for xx in (-0.3, 0.3):
            bx(P, m["seam"], (xx - 0.025, yr - 0.004, 0.3), (xx + 0.025, yr + 0.004 if s > 0 else yr + 0.004, oh - 0.3), bevel=None)
    # lintel above the opening: bottom face exactly at 5.0, full thickness, cap profile on top
    bx(P, m["trim"], (-0.75, -ow, oh), (0.75, ow, oh + 0.45), bevel=0.025, seg=2)
    prism(P, m["armour"], _cap_profile(), "Y", -ow - 0.05, ow + 0.05, bevel=0.03, seg=2)
    for xs in (-1, 1):
        xf = 0.745 if xs > 0 else -0.75
        for s in (-1, 1):
            p0 = Vector((xf + 0.0025, s * 0.7, oh + 0.4))
            p1 = Vector((xf + 0.0025, 0.0, oh + 0.1))
            o, mm = L.bar(P.coll, p0, p1, 0.07, 0.005, up=(1.0, 0.0, 0.0))
            P.add(o, m["seam"], matrix=mm)
        bx(P, m["seam"], (xf, -ow + 0.15, oh + 0.04), (xf + 0.005, -0.9, oh + 0.09), bevel=None)
        bx(P, m["seam"], (xf, 0.9, oh + 0.04), (xf + 0.005, ow - 0.15, oh + 0.09), bevel=None)
    _merlon(P, m, 0.0, size=1.6)
    # floor threshold plate (door track), flush with the ground
    bx(P, m["metal"], (-0.75, -ow, 0.0), (0.75, ow, 0.03), bevel=0.008, seg=1)
    for xx in (-0.3, 0.3):
        bx(P, m["trim"], (xx - 0.05, -ow, 0.0), (xx + 0.05, ow, 0.045), bevel=0.01, seg=1)
    _cap_details(P, m, -gy, gy)


# =========================================================================== jammer tower

def _oct(w, c):
    h = w * 0.5
    return cham_rect(-h, h, -h, h, c)


def build_jammer(P):
    m = null_mats("Jammer", scale=3.0)
    # foundation: octagonal plinth 5 x 5, upper base tapering
    prism(P, m["trim"], _oct(5.0, 1.0), "Z", 0.0, 0.6, bevel=0.05, seg=2)
    rings = []
    for z, w in ((0.6, 4.2), (2.4, 3.4)):
        rings.append([(x, y, z) for x, y in _oct(w, w * 0.22)])
    bm = ring_loft(P.coll, rings)
    P.add(L.obj_from_bm(bm, P.coll), m["armour"], bevel=0.04, segments=2)
    # four heavy raked legs on the diagonals, reaching the 5 x 5 corners, armour + crimson, lit edge seams
    for k in range(4):
        a = 45 + 90 * k
        prof = [(1.2, 0.0), (3.45, 0.0), (3.45, 0.7), (2.35, 4.2), (1.75, 8.6), (1.35, 9.2), (1.1, 4.0)]
        prism(P, m["armour"], prof, "Y", -0.28, 0.28, bevel=0.04, seg=2, matrix=R(a, "Z"))
        side = [(1.4, 0.75), (3.1, 0.75), (2.2, 4.0), (1.7, 7.6), (1.3, 7.0)]
        for s_ in (-1, 1):
            prism(P, m["crimson"], side, "Y", s_ * 0.28 - (0.04 if s_ < 0 else 0.0), s_ * 0.28 + (0.04 if s_ > 0 else 0.0),
                  bevel=0.012, seg=1, matrix=R(a, "Z"))
        o, mm = L.bar(P.coll, (3.43, 0.0, 0.85), (2.4, 0.0, 4.1), 0.08, 0.06, up=(1, 0, 0))
        P.add(o, m["seam"], matrix=R(a, "Z") @ mm)
        o, mm = L.bar(P.coll, (2.33, 0.0, 4.3), (1.77, 0.0, 8.45), 0.08, 0.06, up=(1, 0, 0))
        P.add(o, m["seam"], matrix=R(a, "Z") @ mm)
        bx(P, m["eye"], (3.0, -0.12, 0.35), (3.46, 0.12, 0.45), bevel=None, matrix=R(a, "Z"))
    # shaft: tapering chamfered square, 8 segments with collars
    z = 2.4
    w0, w1 = 3.6, 2.0
    top = 34.0

    def width(zz):
        return w0 + (w1 - w0) * (zz - 2.4) / (top - 2.4)
    seg_z = [2.4] + [6.0 + 4.0 * i for i in range(7)] + [top]
    for i in range(len(seg_z) - 1):
        za, zb = seg_z[i], seg_z[i + 1]
        rings = [[(x, y, za) for x, y in _oct(width(za), width(za) * 0.2)],
                 [(x, y, zb) for x, y in _oct(width(zb), width(zb) * 0.2)]]
        bm = ring_loft(P.coll, rings)
        P.add(L.obj_from_bm(bm, P.coll), m["armour"], bevel=0.03, segments=2)
        # collar at the top of each segment
        if i < len(seg_z) - 2:
            cw = width(zb) + 0.3
            prism(P, m["trim"], _oct(cw, cw * 0.24), "Z", zb - 0.3, zb + 0.18, bevel=0.03, seg=2)
            # red eye lights on two faces of the collar
            for s in (-1, 1):
                bx(P, m["eye" if i % 2 else "seam"], (s * cw * 0.5 - (0.02 if s > 0 else -0.0) - (0.0), -0.12, zb - 0.12),
                   (s * cw * 0.5 + (0.0 if s > 0 else 0.02), 0.12, zb - 0.04), bevel=None)
        # vertical seam strips on +-Y faces, following the taper
        for s in (-1, 1):
            pa = Vector((0.0, s * (width(za) * 0.5 + 0.004), za + 0.35))
            pb = Vector((0.0, s * (width(zb) * 0.5 + 0.004), zb - 0.35))
            o, mm = L.bar(P.coll, pa, pb, 0.08, 0.02, up=(0, s, 0))
            P.add(o, m["seam"], matrix=mm)
        # armour plates on +-X faces (layered), blade-cut alternating
        for s in (-1, 1):
            hz = (zb - za)
            wa, wb = width(za), width(zb)
            plate = blade_rect(-wa * 0.32, wa * 0.32, za + 0.4, zb - 0.45, 0.35)
            if i % 2:
                plate = [(-u, v) for u, v in plate]
            xm = s * (((wa + wb) * 0.25) - 0.01)
            prism(P, m["crimson"] if i % 3 == 0 else m["armour"], plate, "X", xm - 0.03, xm + 0.03, bevel=0.012, seg=1)
    # cable conduits up the +X face with clamps
    for dy in (-0.45, 0.45):
        pts = []
        for zz in [2.6 + 0.5 * k for k in range(int((top - 3.0) / 0.5))]:
            pts.append((width(zz) * 0.5 + 0.12, dy, zz))
        P.add(L.tube_path(P.coll, pts, 0.07, sides=8), m["metal"], smooth=40)
    # crown hub
    prism(P, m["trim"], _oct(2.6, 0.6), "Z", top - 0.3, top + 1.4, bevel=0.04, seg=2)
    bx(P, m["seam"], (-1.31, -0.6, top + 0.5), (1.31, 0.6, top + 0.6), bevel=None)
    # crossbar 1 (16 m along Y) and 2 (14 m along X): armoured arms raked UP and out from the hub into blade
    # spikes (review r5: "flat T-bars, a telephone pole"), emitter nodes two thirds out, lit top seams
    import bmesh
    for (axis, length, zc, thick, rake) in (("Y", 16.0, top + 0.95, 1.0, 2.2), ("X", 14.0, top + 3.3, 0.8, 1.7)):
        half = length * 0.5
        for s in (-1, 1):
            rings = []
            for t in (0.0, 0.3, 0.6, 0.85, 1.0):
                hw = 0.5 * (1.0 - 0.75 * t) + 0.06
                hh = thick * 0.5 * (1.0 - 0.55 * t) + (0.25 * t if t > 0.8 else 0.0)
                prof = blade_rect(-hw, hw, -hh, hh, min(hw, hh) * 0.6)
                d = s * t * half
                z = zc + rake * t ** 1.6
                if axis == "Y":
                    rings.append([(u, d, z + v) for u, v in prof])
                else:
                    rings.append([(d, u, z + v) for u, v in prof])
            bm = ring_loft(P.coll, rings)
            P.add(L.obj_from_bm(bm, P.coll), m["armour"], bevel=0.02, segments=2)
            # crimson under-blade along the arm (layered armour, forward-raked tooth)
            g = [(0.3, 0.0), (half * 0.62, rake * 0.32), (half * 0.45, -0.9), (0.3, -1.6)]
            if axis == "Y":
                prism(P, m["crimson"], [(s * u, v) for u, v in g], "X", -0.1, 0.1, bevel=0.015, seg=1, matrix=T(0, 0, zc - 0.2))
            else:
                prism(P, m["crimson"], [(s * u, v) for u, v in g], "Y", -0.1, 0.1, bevel=0.015, seg=1, matrix=T(0, 0, zc - 0.2))
            # emitter node two thirds out, glowing ring and a paired white eye
            t = 0.66
            p = Vector((0, s * t * half, zc + rake * t ** 1.6)) if axis == "Y" else Vector((s * t * half, 0, zc + rake * t ** 1.6))
            node = L.cyl(P.coll, 0.42, 0.7, segments=16, axis="Z", center=tuple(p + Vector((0, 0, -0.55))))
            P.add(node, m["trim"], bevel=0.03, segments=2)
            ring = L.revolve_closed(P.coll, [(0.36, -0.06), (0.44, -0.06), (0.44, 0.06), (0.36, 0.06)], (0.0, 360.0), 20, axis="Z")
            P.add(ring, m["seam"], matrix=T(*(p + Vector((0, 0, -0.62)))))
            o = L.lathe(P.coll, [(0.0, 0.16), (-1.6, 0.0)], segments=8, center=tuple(p + Vector((0, 0, -0.85))))
            P.add(o, m["metal"], smooth=40)
            # top seam along the arm
            q0 = Vector((0, s * 0.8, zc + thick * 0.5 - 0.02)) if axis == "Y" else Vector((s * 0.8, 0, zc + thick * 0.5 - 0.02))
            q1 = p + Vector((0, 0, thick * 0.5 * (1 - 0.55 * t) - 0.03))
            o, mm = L.bar(P.coll, q0, q1, 0.08, 0.04)
            P.add(o, m["seam"], matrix=mm)
    # upper mast, the 8 m crown: four forward-raked blade plates caging a 2.8 m emitter core, paired eyes
    prism(P, m["armour"], _oct(1.5, 0.35), "Z", top + 1.3, top + 4.6, bevel=0.03, seg=2)
    prism(P, m["trim"], _oct(2.4, 0.55), "Z", top + 4.2, top + 4.75, bevel=0.03, seg=2)
    for k in range(4):
        a = 90 * k
        bx(P, m["eye"], (1.2, -0.25, top + 4.4), (1.205, -0.1, top + 4.48), bevel=None, matrix=R(a, "Z"))
        bx(P, m["eye"], (1.2, 0.1, top + 4.4), (1.205, 0.25, top + 4.48), bevel=None, matrix=R(a, "Z"))
    core_c = Vector((0.0, 0.0, top + 6.0))
    bm = bmesh.new()
    vs = [bm.verts.new(core_c + Vector(v)) for v in ((1.15, 0, 0), (-1.15, 0, 0), (0, 1.15, 0), (0, -1.15, 0), (0, 0, 1.55), (0, 0, -1.35))]
    for a_, b_ in ((0, 2), (2, 1), (1, 3), (3, 0)):
        bm.faces.new((vs[a_], vs[b_], vs[4]))
        bm.faces.new((vs[b_], vs[a_], vs[5]))
    P.add(L.obj_from_bm(bm, P.coll), m["seam"])
    halo = L.revolve_closed(P.coll, [(1.45, -0.07), (1.6, -0.07), (1.6, 0.07), (1.45, 0.07)], (0.0, 360.0), 32, axis="Z")
    P.add(halo, m["seam"], matrix=T(*core_c))
    for k in range(4):
        a = 45 + 90 * k
        prof = [(0.6, top + 4.6), (1.5, top + 4.7), (2.2, top + 5.6), (2.0, top + 7.0), (0.9, top + 7.95), (1.15, top + 6.9),
                (1.45, top + 5.9), (1.0, top + 5.2)]
        prism(P, m["armour"], prof, "Y", -0.11, 0.11, bevel=0.02, seg=1, matrix=R(a, "Z"))
        prism(P, m["crimson"], [(1.55, top + 5.0), (1.95, top + 5.65), (1.8, top + 6.6), (1.45, top + 5.9)], "Y", -0.13, 0.13,
              bevel=0.01, seg=1, matrix=R(a, "Z"))
        # halo struts joining the blades to the halo ring
        o, mm = L.bar(P.coll, (1.5, 0.0, top + 6.0), (1.75, 0.0, top + 6.0), 0.14, 0.14)
        P.add(o, m["trim"], matrix=R(a, "Z") @ mm)
    # spire to 42 m
    o = L.lathe(P.coll, [(top + 7.4, 0.2), (top + 8.0, 0.0)], segments=8)
    P.add(o, m["metal"], smooth=40)
    prism(P, m["trim"], _oct(0.7, 0.16), "Z", top + 7.1, top + 7.5, bevel=0.02, seg=1)
    # heavy mid-shaft collar (secondary mass) at a third of the height, with eyes and seams
    zm = 13.6
    cw = width(zm) + 0.9
    prism(P, m["trim"], _oct(cw, cw * 0.26), "Z", zm - 0.7, zm + 0.5, bevel=0.04, seg=2)
    prism(P, m["crimson"], _oct(cw - 0.2, cw * 0.26), "Z", zm + 0.5, zm + 0.75, bevel=0.03, seg=1)
    for k in range(4):
        bx(P, m["seam"], (cw * 0.5 - 0.01, -0.5, zm - 0.25), (cw * 0.5 + 0.004, 0.5, zm - 0.17), bevel=None, matrix=R(90 * k, "Z"))
        bx(P, m["eye"], (cw * 0.5 - 0.01, -0.3, zm + 0.1), (cw * 0.5 + 0.004, -0.12, zm + 0.18), bevel=None, matrix=R(90 * k, "Z"))
        bx(P, m["eye"], (cw * 0.5 - 0.01, 0.12, zm + 0.1), (cw * 0.5 + 0.004, 0.3, zm + 0.18), bevel=None, matrix=R(90 * k, "Z"))
    # base detail the player sees on foot: access hatch on the +X face of the upper base, hazard sill, conduits
    hx0 = 1.95
    bx(P, m["trim"], (hx0 - 0.05, -0.55, 0.62), (hx0 + 0.05, 0.55, 2.3), bevel=0.02, seg=2, matrix=R(0, "Z"))
    bx(P, m["hazard"], (hx0 - 0.04, -0.6, 0.6), (hx0 + 0.07, 0.6, 0.72), bevel=0.01, seg=1)
    bx(P, m["seam"], (hx0 + 0.04, -0.4, 2.12), (hx0 + 0.06, 0.4, 2.18), bevel=None)
    for dy in (-1.2, 1.2):
        pts = [(2.4, dy, 0.62), (2.1, dy, 1.4), (1.85, dy, 2.6), (1.75, dy, 3.2)]
        P.add(L.tube_path(P.coll, pts, 0.09, sides=8), m["metal"], smooth=40)


# =========================================================================== crates, door

def build_crate(P, size):
    m = null_mats("Crate", scale=0.5)
    sx, sy, sz = size
    hx, hy = sx * 0.5, sy * 0.5
    # body inset 5 cm, chamfered
    bx(P, m["armour"], (-hx + 0.05, -hy + 0.05, 0.03), (hx - 0.05, hy - 0.05, sz - 0.04), bevel=0.05, seg=3)
    # corner caps (reach the box) and edge rails
    c = 0.26
    for sxn in (-1, 1):
        for syn in (-1, 1):
            for z0, z1 in ((0.0, c), (sz - c, sz)):
                lo = (hx - c if sxn > 0 else -hx, hy - c if syn > 0 else -hy, z0)
                hi = (hx if sxn > 0 else -hx + c, hy if syn > 0 else -hy + c, z1)
                bx(P, m["trim"], lo, hi, bevel=0.03, seg=2)
            # vertical edge rail
            lo = (hx - 0.12 if sxn > 0 else -hx + 0.0, hy - 0.12 if syn > 0 else -hy + 0.0, c)
            hi = (hx - 0.0 if sxn > 0 else -hx + 0.12, hy - 0.0 if syn > 0 else -hy + 0.12, sz - c)
            bx(P, m["trim"], (lo[0] + (0.0 if sxn > 0 else 0.0), lo[1], lo[2]), hi, bevel=0.02, seg=2)
    # horizontal edge rails along X and Y at top and bottom
    for z0, z1 in ((0.0, 0.1), (sz - 0.1, sz)):
        for syn in (-1, 1):
            bx(P, m["trim"], (-hx + c, (hy - 0.1) if syn > 0 else -hy, z0), (hx - c, hy if syn > 0 else -hy + 0.1, z1), bevel=0.02, seg=2)
        for sxn in (-1, 1):
            bx(P, m["trim"], ((hx - 0.1) if sxn > 0 else -hx, -hy + c, z0), (hx if sxn > 0 else -hx + 0.1, hy - c, z1), bevel=0.02, seg=2)
    # side armour plates (crimson on the long sides, blade cut), lid seam glow, latches and handles
    for syn in (-1, 1):
        yf = hy - 0.05 if syn > 0 else -hy + 0.05
        plate = blade_rect(-hx + 0.4, hx - 0.4, 0.2, sz - 0.42, 0.18)
        a, b = (yf, yf + 0.035) if syn > 0 else (yf - 0.035, yf)
        o = L.extrude_profile(P.coll, [(u, v) for u, v in plate], 0.035, axis="Y")
        P.add(o, m["crimson"], bevel=0.01, segments=1, matrix=T(0, (a + b) * 0.5, 0))
        # stencil block (Null unit marking) on the plate
        bx(P, m["hazard"], (-hx + 0.55, a - 0.003 if syn < 0 else b - 0.004, 0.32), (-hx + 0.95, a + 0.004 if syn < 0 else b + 0.003, 0.5), bevel=None)
        for xx in (-hx * 0.45, hx * 0.45):
            bx(P, m["metal"], (xx - 0.09, (hy - 0.07) if syn > 0 else -hy, sz - 0.36), (xx + 0.09, hy if syn > 0 else -hy + 0.07, sz - 0.2), bevel=0.01, seg=1)
    for sxn in (-1, 1):
        xf = hx if sxn > 0 else -hx
        # handle bar on the ends
        p0 = (xf - sxn * 0.02, -hy * 0.4, sz * 0.55)
        p1 = (xf - sxn * 0.02, hy * 0.4, sz * 0.55)
        o, mm = L.bar(P.coll, p0, p1, 0.04, 0.06)
        P.add(o, m["metal"], bevel=0.008, segments=1, matrix=mm)
        for yy in (-hy * 0.4, hy * 0.4):
            bx(P, m["trim"], (hx - 0.08 if sxn > 0 else -hx, yy - 0.05, sz * 0.5), (hx if sxn > 0 else -hx + 0.08, yy + 0.05, sz * 0.6), bevel=0.01, seg=1)
    # end armour plates flush with the box on the short ends (gameplay collides with them)
    for sxn in (-1, 1):
        x0, x1 = (hx - 0.06, hx) if sxn > 0 else (-hx, -hx + 0.06)
        bx(P, m["armour"], (x0, -hy + 0.13, 0.12), (x1, hy - 0.13, sz - 0.42), bevel=0.012, seg=1)
    # lid seam: red-orange line all round at 0.75 * height
    zs = sz - 0.36
    for syn in (-1, 1):
        y = hy - 0.05 if syn > 0 else -hy + 0.05
        bx(P, m["seam"], (-hx + c, y - 0.004, zs - 0.02), (hx - c, y + 0.004, zs + 0.02), bevel=None)
    for sxn in (-1, 1):
        x = hx - 0.05 if sxn > 0 else -hx + 0.05
        bx(P, m["seam"], (x - 0.004, -hy + c, zs - 0.02), (x + 0.004, hy - c, zs + 0.02), bevel=None)
    # lid: raised armour panel on top (top face stays at the box)
    bx(P, m["armour"], (-hx + 0.3, -hy + 0.3, sz - 0.06), (hx - 0.3, hy - 0.3, sz - 0.0), bevel=0.02, seg=2)
    bx(P, m["eye"], (-0.15, -hy * 0.5 - 0.02, sz - 0.075), (0.15, -hy * 0.5 + 0.02, sz - 0.002), bevel=None)


def build_door(P):
    m = null_mats("Door", scale=0.8)
    hx = 0.15
    split = [(0.0, 0.0), (0.0, 2.3), (-0.45, 2.85), (-0.45, 3.45), (0.0, 4.0), (0.0, 6.0)]
    gap = 0.015
    left = [(-3.1, 0.0)] + [(y - gap, z) for y, z in split] + [(-3.1, 6.0)]
    right = [(y + gap, z) for y, z in reversed(split)] + [(3.1, 6.0), (3.1, 0.0)]
    for poly in (left, right):
        prism(P, m["armour"], poly, "X", -0.12, 0.12, bevel=0.02, seg=2)
    # layered face plates on both faces, inside the box (x up to +-0.145)
    for xs in (-1, 1):
        xa, xb = (0.115, 0.145) if xs > 0 else (-0.145, -0.115)
        for s in (-1, 1):
            # big blade plates per half
            for (z0, z1) in ((0.25, 2.1), (4.2, 5.75)):
                p = blade_rect(0.25, 2.85, z0, z1, 0.4)
                p = [(u * s, v) for u, v in p]
                prism(P, m["armour"], p, "X", xa, xb, bevel=0.012, seg=1)
            p = cham_rect(0.85, 2.85, 2.3, 4.0, 0.2)
            p = [(u * s, v) for u, v in p]
            prism(P, m["crimson"], p, "X", xa, xb, bevel=0.012, seg=1)
            # horizontal red-orange band
            bx(P, m["seam"], (xa, s * 0.95 if s > 0 else -2.85, 2.16), (xb + (0.005 if xs > 0 else 0.0) - (0.0 if xs > 0 else 0.005), 2.85 if s > 0 else -0.95, 2.22), bevel=None)
        # glow following the split line
        pts = split
        for (y0, z0), (y1, z1) in zip(pts, pts[1:]):
            o, mm = L.bar(P.coll, (xs * 0.122, y0, z0 + (0.16 if z0 < 0.1 else 0.0)), (xs * 0.122, y1, z1 - (0.16 if z1 > 5.9 else 0.0)), 0.07, 0.008, up=(xs, 0, 0))
            P.add(o, m["seam"], matrix=mm)
        # white eye pair
        for s in (-1, 1):
            bx(P, m["eye"], (xa, s * 0.5 - 0.12, 4.55), (xb + (0.005 if xs > 0 else 0.0), s * 0.5 + 0.12, 4.62), bevel=None)
    # bottom sweep and top guide rails (full thickness)
    bx(P, m["trim"], (-0.15, -3.1, 0.0), (0.15, 3.1, 0.16), bevel=0.02, seg=2)
    bx(P, m["trim"], (-0.15, -3.1, 5.84), (0.15, 3.1, 6.0), bevel=0.02, seg=2)
    for s in (-1, 1):
        bx(P, m["hazard"], (-0.15, s * 3.1 - (0.25 if s > 0 else 0.0) if s > 0 else -3.1, 0.16), (0.15, 3.1 if s > 0 else -2.85, 5.84), bevel=0.012, seg=1)


# =========================================================================== basalt

def _crystal_mat(name, col):
    return L.role_glow(name, col, level=0.85, base="#0B2A33" if col == "#2EE6FF" else "#3A2408")


def build_choke(P, length, seed):
    rng = random.Random(seed)
    m = {"basalt": role_basalt("Choke_Basalt", scale=1.2, crack=0.6, crack_color="#FFB347", seed=seed),
         "cyan": _crystal_mat("Choke_CrystalC", "#2EE6FF"), "amber": _crystal_mat("Choke_CrystalA", "#FFB347")}
    W, Lg, Ht = 4.0, length, 6.0
    r = 0.55
    dx = r * 1.5
    dy = r * math.sqrt(3.0)
    cols = []
    i = 0
    x = -W * 0.5 + r * 0.95
    while x <= W * 0.5 - r * 0.6:
        y = -Lg * 0.5 + r * 0.95 + (dy * 0.5 if i % 2 else 0.0)
        while y <= Lg * 0.5 - r * 0.6:
            cols.append((x, y))
            y += dy
        x += dx
        i += 1
    for k, (cx, cy) in enumerate(cols):
        t = abs(cy) / (Lg * 0.5)
        hmax = Ht * (1.0 - 0.32 * t ** 1.6) - abs(cx) * 0.25
        h = hmax * rng.uniform(0.82, 1.0)
        if rng.random() < 0.05 and abs(cy) < Lg * 0.3:
            h = Ht                                   # guarantee the full height somewhere near the middle
        tilt = (rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25)) if rng.random() < 0.45 else (0.0, 0.0)
        rr = r * rng.uniform(0.92, 1.08)
        hex_column(P, m["basalt"], cx, cy, rr, 0.0, min(h, Ht - abs(tilt[0]) * rr - abs(tilt[1]) * rr),
                   rot=rng.uniform(0, 60), top_tilt=tilt, bevel=0.035, seed=seed * 1000 + k)
    # one column at full height in the middle (sets the 6 m)
    hex_column(P, m["basalt"], 0.0, 0.0, r * 1.05, 0.0, Ht, rot=10, bevel=0.035, seed=seed + 7)
    # crystal veins: clusters erupting from cracks on both long faces and from the tops, placed with ray casts on
    # the real column surfaces (bases embedded 12 cm), tips kept inside the 4 m box
    from mathutils.bvhtree import BVHTree
    trees = []
    for o in P.parts:
        mw = o.matrix_world
        trees.append(BVHTree.FromPolygons([mw @ v.co for v in o.data.vertices], [p.vertices[:] for p in o.data.polygons]))

    def cast(origin, d):
        best = None
        for t in trees:
            h = t.ray_cast(origin, d)
            if h[0] is not None and (best is None or h[3] < best[1]):
                best = (h[0], h[3], h[1])
        return best
    placed = 0
    tries = 0
    while placed < 9 and tries < 80:
        tries += 1
        side = 1 if placed % 2 else -1
        y = rng.uniform(-Lg * 0.42, Lg * 0.42)
        z = rng.uniform(1.0, 4.4)
        h = cast(Vector((side * 3.0, y, z)), Vector((-side, 0, 0)))
        if h is None or abs(h[0].x) < 1.0:
            continue
        base = h[0]
        col = m["cyan"] if placed % 3 else m["amber"]
        for j in range(rng.randint(4, 7)):
            d = Vector((side * rng.uniform(0.6, 1.0), rng.uniform(-0.5, 0.5), rng.uniform(0.1, 0.9))).normalized()
            room = (W * 0.5 - 0.03 - abs(base.x)) / max(abs(d.x), 1e-3)
            if room < 0.25:
                continue
            ln = min(rng.uniform(0.5, 1.1), room)
            shard(P, col, base - d * 0.15, d, rng.uniform(0.14, 0.24), ln + 0.15, seed=seed * 50 + placed * 7 + j)
        placed += 1
    placed = 0
    tries = 0
    while placed < 6 and tries < 80:
        tries += 1
        x, y = rng.uniform(-1.2, 1.2), rng.uniform(-Lg * 0.3, Lg * 0.3)
        h = cast(Vector((x, y, 20.0)), Vector((0, 0, -1)))
        if h is None or h[0].z > Ht - 1.25 or h[0].z < 3.0 or h[2].z < 0.8:
            continue
        col = m["amber"] if placed % 2 else m["cyan"]
        for j in range(6):
            d = Vector((rng.uniform(-0.35, 0.35), rng.uniform(-0.35, 0.35), 1.0)).normalized()
            shard(P, col, h[0] - d * 0.15, d, rng.uniform(0.15, 0.26), rng.uniform(0.7, 1.2),
                  seed=seed * 70 + placed * 9 + j)
        placed += 1


def build_stepping_stone(P):
    m = {"basalt": role_basalt("Step_Basalt", scale=0.5, crack=1.0, crack_zmax=0.55, seed=3)}
    rng = random.Random(41)
    r = 0.52
    centres = [(0.0, 0.0)] + [(math.cos(math.radians(30 + 60 * k)) * r * 1.68, math.sin(math.radians(30 + 60 * k)) * r * 1.68) for k in range(6)]
    for k, (cx, cy) in enumerate(centres):
        top = 1.6
        hex_column(P, m["basalt"], cx, cy, r * rng.uniform(0.96, 1.04), 0.0, top, rot=rng.uniform(0, 60),
                   bevel=0.03, seed=300 + k)
    # filler under the column joints so the walkable top has no holes (1 cm below the top)
    hex_column(P, m["basalt"], 0.0, 0.0, 1.05, 0.0, 1.59, rot=0, bevel=0.02, seed=399, jitter=0.0)
    # skirt of lower broken columns at the base (magma line), inside the 2.8 m box
    for k in range(6):
        a = math.radians(60 * k)
        cx, cy = math.cos(a) * 1.05, math.sin(a) * 1.05
        hex_column(P, m["basalt"], cx, cy, 0.3, 0.0, rng.uniform(0.35, 0.8), rot=rng.uniform(0, 60),
                   top_tilt=(rng.uniform(-0.3, 0.3), rng.uniform(-0.3, 0.3)), bevel=0.025, seed=400 + k)


def build_collapse_slab(P):
    m = {"basalt": role_basalt("Slab_Basalt", scale=0.6, seed=5),
         "magma": L.role_glow("Slab_Magma", "#FF5A1F", level=0.9, base="#2A0E06")}
    import bmesh
    rng = random.Random(77)
    X, Y, Z = 6.1, 3.2, 0.6
    # crack lines across the slab (x positions at y=-Y/2 and y=+Y/2)
    cuts = [(-1.6, -1.1), (0.25, 0.75), (1.75, 2.1)]
    xs_lo = [-X * 0.5] + [c[0] for c in cuts] + [X * 0.5]
    xs_hi = [-X * 0.5] + [c[1] for c in cuts] + [X * 0.5]
    gap = 0.035
    for i in range(len(xs_lo) - 1):
        a0, a1 = xs_lo[i] + (gap if i > 0 else 0.0), xs_lo[i + 1] - (gap if i < len(xs_lo) - 2 else 0.0)
        b0, b1 = xs_hi[i] + (gap if i > 0 else 0.0), xs_hi[i + 1] - (gap if i < len(xs_lo) - 2 else 0.0)
        # jagged crack edges: intermediate points
        ln = [(a0, -Y * 0.5)]
        n = 5
        for k in range(1, n):
            t = k / float(n)
            ln.append((b0 * t + a0 * (1 - t) + (rng.uniform(-0.06, 0.06) if i > 0 else 0.0), -Y * 0.5 + Y * t))
        ln.append((b0, Y * 0.5))
        rn = [(b1, Y * 0.5)]
        for k in range(n - 1, 0, -1):
            t = k / float(n)
            rn.append((b1 * t + a1 * (1 - t) + (rng.uniform(-0.06, 0.06) if i < len(xs_lo) - 2 else 0.0), -Y * 0.5 + Y * t))
        rn.append((a1, -Y * 0.5))
        poly = ln + rn
        bm = bmesh.new()
        top = [bm.verts.new((u, v, Z)) for u, v in poly]
        bot = [bm.verts.new((u, v, rng.uniform(0.0, 0.12))) for u, v in poly]
        nn = len(poly)
        for k in range(nn):
            bm.faces.new((bot[k], bot[(k + 1) % nn], top[(k + 1) % nn], top[k]))
        bm.faces.new(top)
        bm.faces.new(list(reversed(bot)))
        P.add(L.obj_from_bm(bm, P.coll), m["basalt"], bevel=0.035, segments=3, angle=25)
    # magma layer visible through the cracks
    for c in cuts:
        o, mm = L.bar(P.coll, (c[0], -Y * 0.5 + 0.05, 0.3), (c[1], Y * 0.5 - 0.05, 0.3), 0.12, 0.24)
        P.add(o, m["magma"], matrix=mm)


def build_debris_chunk(P):
    m = role_basalt("Debris_Basalt", scale=0.3, crack=0.8, seed=9, crack_color="#FF5A1F")
    import bpy
    r = L.rocks["make_asteroid"](name=L.uname("chunk"), radius=0.75, seed=19, subdivisions=5, elongation=0.25,
                                 roughness=0.5, ridge=0.7, craters=0, collection=P.coll.name, material=m,
                                 sharp_angle_deg=35)
    o = bpy.data.objects[r["object"]]
    if o.data.uv_layers:
        for uvl in list(o.data.uv_layers):
            o.data.uv_layers.remove(uvl)
    P.add(o, m, smooth=35)


BUILDERS = {
    "SM_Cind_OutpostWall": dict(fn=lambda P: build_wall(P, False), family="cinder", dims=(1.5, 15.0, 6.0), tex=2048,
                                budget=(2000, 40000), ao=0.8),
    "SM_Cind_OutpostWall_Doorway": dict(fn=lambda P: build_wall(P, True), family="cinder", dims=(1.5, 6.0, 6.0),
                                        tex=2048, budget=(1500, 40000), ao=0.8),
    "SM_Cind_JammerTower": dict(fn=build_jammer, family="cinder", dims=(None, None, 42.0), tex=2048,
                                budget=(2000, 40000), ao=2.0, fit_axes=(False, False, True)),
    "SM_Cind_Crate_A": dict(fn=lambda P: build_crate(P, (3.0, 2.2, 1.4)), family="cinder", dims=(3.0, 2.2, 1.4),
                            tex=1024, budget=(2000, 10000), ao=0.4),
    "SM_Cind_Crate_B": dict(fn=lambda P: build_crate(P, (3.6, 3.0, 1.4)), family="cinder", dims=(3.6, 3.0, 1.4),
                            tex=1024, budget=(2000, 10000), ao=0.4),
    "SM_Door": dict(fn=build_door, family="cinder", dims=(0.3, 6.2, 6.0), tex=1024, budget=(2000, 10000), ao=0.5),
    "SM_Cind_ChokeRock_A": dict(fn=lambda P: build_choke(P, 12.0, 1), family="cinder", dims=(4.0, 12.0, 6.0),
                                tex=2048, budget=(2000, 40000), ao=1.0, detail=True, snap=True),
    "SM_Cind_ChokeRock_B": dict(fn=lambda P: build_choke(P, 14.0, 2), family="cinder", dims=(4.0, 14.0, 6.0),
                                tex=2048, budget=(2000, 40000), ao=1.0, detail=True, snap=True),
    "SM_Cind_SteppingStone": dict(fn=build_stepping_stone, family="cinder", dims=(2.8, 2.8, 1.6), tex=1024,
                                  budget=(1000, 10000), ao=0.5, detail=True),
    "SM_Cind_CollapseSlab": dict(fn=build_collapse_slab, family="cinder", dims=(6.1, 3.2, 0.6), tex=1024,
                                 budget=(500, 10000), ao=0.5, detail=True),
    "SM_Cind_DebrisChunk": dict(fn=build_debris_chunk, family="cinder", dims=(None, None, None), tex=1024,
                                budget=(500, 10000), ao=0.4, pivot="centre", fit_axes=(False, False, False)),
}
