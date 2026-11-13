# SPDX-License-Identifier: MIT
"""
sd_hard.py - code-built hard-surface dressing for the SpaceDressing kit: derelict satellites,
cargo pods, the navigation buoy, broken station truss / hull sections, debris clusters and the
portal gate ring frame. Units metres, front -Y, up +Z. Each builder fills a sd_lib.Part with parts
that carry role materials; sd_lib.finish_hard_surface joins, unwraps, bakes and exports.

Palette (neutral dressing, never the squad's magenta / cyan / amber or the enemy's red-orange on
black): warm ivory, gunmetal, gold trim and foil, faded teal and violet paint, teal and warm-white glow.
"""

import math
import random

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import sd_lib as L

C = {
    "ivory": "#E3DCCB", "ivory2": "#CFC6B2", "gunmetal": "#3A3F48", "gunmetal2": "#2C3038",
    "steel": "#959CA4", "gold": "#D2A13F", "teal": "#1E9C93", "teal2": "#15736D", "violet": "#5E43A0",
    "violet2": "#46327A", "blue_grey": "#3B4656", "hazard_gold": "#E2AE34", "hazard_teal": "#14968E",
    "solar": "#1B2452", "solar_line": "#AEB6C0", "glow_teal": "#2AF5C8", "glow_warm": "#FFE9C4",
    "yellow_paint": "#C9A23E",
}
GLOW_TEAL = C["glow_teal"]


def rot(deg, axis):
    return Matrix.Rotation(math.radians(deg), 4, axis)


def T(x, y, z):
    return Matrix.Translation((x, y, z))


def mats_common(prefix, scale, body=("ivory", "ivory2"), accent="teal", panel=(1.0, 0.015), fade=0.3, wear=0.5):
    m = {}
    m["body"] = L.role_paint(prefix + "_Body", C[body[0]], C[body[1]], metallic=0.25, rough=0.46, wear=wear,
                             fade=fade, fade_color="#B9B4A8", grime=0.3, panel=panel, scale=scale, seed=1)
    m["accent"] = L.role_paint(prefix + "_Accent", C[accent], C[accent + "2"] if accent + "2" in C else None,
                               metallic=0.3, rough=0.42, wear=wear, fade=fade + 0.15, fade_color="#8FA9A6",
                               grime=0.25, scale=scale, seed=2)
    m["dark"] = L.role_paint(prefix + "_Dark", C["gunmetal"], C["gunmetal2"], metallic=0.55, rough=0.5, wear=0.6,
                             wear_color="#9EA6AE", grime=0.35, scale=scale, seed=3)
    m["steel"] = L.role_metal(prefix + "_Steel", C["steel"], rough=0.36, metallic=0.9, scale=scale, seed=4)
    m["gold"] = L.role_paint(prefix + "_Gold", C["gold"], "#B8862E", metallic=0.85, rough=0.32, wear=0.3,
                             wear_color="#F0D48A", grime=0.2, scale=scale, seed=5)
    m["glow"] = L.role_glow(prefix + "_Glow", GLOW_TEAL, level=1.0)
    m["glow_warm"] = L.role_glow(prefix + "_GlowWarm", C["glow_warm"], level=1.0)
    return m


def greeble_cluster(P, mat, center, normal, count, size, height, spread, seed, matrix=Matrix.Identity(4)):
    """Hand-placed cluster of small boxes on a surface patch (clustered, never sprinkled)."""
    rng = random.Random(seed)
    n = Vector(normal).normalized()
    t1 = n.orthogonal().normalized()
    t2 = n.cross(t1).normalized()
    rotm = Matrix((t1, t2, n)).transposed().to_4x4()
    for _ in range(count):
        sx, sy, sz = rng.uniform(*size), rng.uniform(*size), rng.uniform(*height)
        off = t1 * rng.uniform(-spread, spread) + t2 * rng.uniform(-spread, spread)
        o = L.box(P.coll, (sx, sy, sz))
        m = matrix @ T(*(Vector(center) + off + n * (sz * 0.5 - 0.01))) @ rotm @ rot(rng.choice([0, 90]), "Z")
        P.add(o, mat, bevel=min(sx, sy, sz) * 0.12, segments=1, matrix=m)


# =========================================================================== satellites

def solar_mat(prefix, scale):
    return L.role_paint(prefix + "_Solar", C["solar"], metallic=0.5, rough=0.25, wear=0.25, wear_color="#C9CED4",
                        fade=0.2, fade_color="#4C5376", grime=0.15,
                        cells=(C["solar"], C["solar_line"], (0.34, 0.34), ("X", "Z"), 0.02), scale=scale, seed=7)


def nozzle(P, mat, matrix, r=0.18, length=0.32):
    prof = [(0.0, 0.0), (0.0, r * 0.4), (length * 0.3, r * 0.55), (length, r), (length + 0.02, r * 1.08),
            (length + 0.02, r * 0.85), (length * 0.35, r * 0.4), (length * 0.35, 0.0)]
    o = L.lathe(P.coll, prof, segments=16, axis="Z")
    P.add(o, mat, smooth=40.0, matrix=matrix)


def dish(P, mat, mat_dark, matrix, D=2.6, f=1.0, thick=0.05, struts=3):
    """Parabolic reflector (closed shell) + feed horn on struts + back ribs, axis +Z (concave side up)."""
    R = D * 0.5
    prof = []
    n = 8
    for i in range(n + 1):
        r = R * i / n
        prof.append((r * r / (4 * f), r))
    depth = R * R / (4 * f)
    back = [(prof[i][0] - thick, prof[i][1]) for i in range(n, -1, -1)]
    profile = [(-thick, 0.0)] + [(p - thick, r) for p, r in prof[1:]] + [(depth + 0.03, R + 0.04)] + \
              [(p, r) for p, r in reversed(prof[1:])] + [(0.0, 0.0)]
    # lathe expects (pos, r) going around the closed outline
    o = L.lathe(P.coll, profile, segments=24, axis="Z")
    P.add(o, mat, smooth=50.0, matrix=matrix)
    # feed horn at the focus on struts
    horn = L.cyl(P.coll, 0.09, 0.32, segments=12, axis="Z", center=(0, 0, f - 0.05), r_top=0.06)
    P.add(horn, mat_dark, bevel=0.01, segments=1, matrix=matrix)
    for k in range(struts):
        a = math.tau * k / struts
        p0 = (math.cos(a) * R * 0.92, math.sin(a) * R * 0.92, depth * 0.85)
        p1 = (math.cos(a) * 0.05, math.sin(a) * 0.05, f - 0.1)
        b, m = L.bar(P.coll, p0, p1, 0.035, 0.035)
        P.add(b, mat_dark, smooth=40.0, matrix=matrix @ m)
    for k in range(4):
        a = math.tau * k / 4 + 0.4
        p0 = (math.cos(a) * 0.15, math.sin(a) * 0.15, -thick - 0.06)
        p1 = (math.cos(a) * R * 0.8, math.sin(a) * R * 0.8, (R * 0.8) ** 2 / (4 * f) - thick - 0.04)
        b, m = L.bar(P.coll, p0, p1, 0.06, 0.1, up=(0, 0, 1))
        P.add(b, mat_dark, bevel=0.01, segments=1, matrix=matrix @ m)
    hub = L.cyl(P.coll, 0.22, 0.25, segments=12, axis="Z", center=(0, 0, -thick - 0.1))
    P.add(hub, mat_dark, bevel=0.015, segments=1, matrix=matrix)


def solar_panel(P, mat, frame_mat, size=(2.0, 1.7), thick=0.05, matrix=Matrix.Identity(4), torn=None):
    """Panel in the XZ plane (cells face +-Y), local origin at its inner (-X) edge centre.
    torn=(jag, holes, seed) makes it a jagged broken grid plate instead."""
    sx, sz = size
    if torn:
        jag, holes, seed = torn
        o = L.grid_plate(P.coll, sx, sz, 8, 6, thick, jag=jag, jag_sides=(0, 1, 1, 1), seed=seed, holes=holes)
        o.data.transform(T(sx * 0.5, 0, 0) @ rot(90, "X"))
    else:
        o = L.box(P.coll, (sx, thick, sz), center=(sx * 0.5, 0, 0))
    P.add(o, mat, bevel=0.008, segments=1, matrix=matrix)
    # frame rail along the inner edge + hinge blocks
    rail = L.box(P.coll, (0.07, thick * 1.6, sz), center=(0.025, 0, 0))
    P.add(rail, frame_mat, bevel=0.01, segments=1, matrix=matrix)


def build_sat01(asset, tag):
    P = L.Part(asset, "SpaceDressing")
    sc = 0.6
    m = mats_common("Sat1", sc, panel=(0.6, 0.012), fade=0.35)
    m["foil"] = L.role_foil("Sat1_Foil", "#D9A63C", scale=0.4)
    m["solar"] = solar_mat("Sat1", sc)
    m["body"] = L.role_paint("Sat1_Body", C["ivory"], C["ivory2"], metallic=0.25, rough=0.46, wear=0.55, fade=0.35,
                             fade_color="#B9B4A8", grime=0.35, panel=(0.6, 0.012), scale=sc, seed=1, scorch=0.55,
                             label=(C["teal"], (-0.3, 0.3), (0.95, 1.3), ("X", "Z")))
    m["dishp"] = L.role_paint("Sat1_Dish", C["ivory"], C["ivory2"], metallic=0.25, rough=0.4, wear=0.4, fade=0.3,
                              grime=0.25, scale=sc, seed=12)
    # --- bus: painted upper section, flange, gold-foil lower section, top deck
    oct8 = lambda r, h, c: L.cyl(P.coll, r, h, segments=8, axis="Z", center=c)
    upper = oct8(1.25, 1.7, (0, 0, 0.55))
    upper.data.transform(rot(22.5, "Z"))
    for nrm in [(math.cos(math.radians(a)), math.sin(math.radians(a)), 0) for a in range(0, 360, 45)]:
        L.hs["inset_panels"](upper, normal=nrm, min_dot=0.95, fraction=0.7, thickness=0.12, depth=-0.035, seed=int(nrm[0] * 7 + 9))
    # micrometeoroid puncture on the front face
    rng = random.Random(4)
    cbm = bmesh.new()
    bmesh.ops.create_icosphere(cbm, subdivisions=2, radius=0.34)
    for v in cbm.verts:
        v.co *= rng.uniform(0.75, 1.25)
    cut = L.bm_to_object("CUT_hole", cbm, P.coll)
    cut.location = (0.35, -1.12, 0.85)
    L.hs["boolean"](upper, cut, operation="DIFFERENCE", solver="EXACT")
    L.hs["apply_modifiers"](upper)
    L.remove_object(cut)
    P.add(upper, m["body"], bevel=0.025, segments=2, hull=False)
    flange = oct8(1.36, 0.14, (0, 0, -0.33))
    flange.data.transform(rot(22.5, "Z"))
    P.add(flange, m["dark"], bevel=0.02, segments=1)
    lower = oct8(1.29, 1.15, (0, 0, -0.95))
    lower.data.transform(rot(22.5, "Z"))
    P.add(lower, m["foil"], bevel=0.04, segments=2)
    bottom = oct8(1.05, 0.2, (0, 0, -1.6))
    bottom.data.transform(rot(22.5, "Z"))
    P.add(bottom, m["dark"], bevel=0.02, segments=1)
    P.hull_from([upper, lower, flange, bottom])
    deck = L.cyl(P.coll, 0.78, 0.45, segments=16, axis="Z", center=(0, 0, 1.6))
    P.add(deck, m["dark"], bevel=0.02, segments=1)
    ring = L.cyl(P.coll, 0.85, 0.08, segments=16, axis="Z", center=(0, 0, 1.42))
    P.add(ring, m["accent"], bevel=0.015, segments=1)
    greeble_cluster(P, m["dark"], (0.0, 0.0, 1.4), (0, 0, 1), 9, (0.12, 0.3), (0.06, 0.18), 0.85, seed=3)
    # --- dish on a mast with gimbal, tilted toward the front (-Y) and up
    mast = L.cyl(P.coll, 0.11, 1.1, segments=10, axis="Z", center=(0, 0, 2.35))
    P.add(mast, m["steel"], smooth=40)
    gimbal = L.box(P.coll, (0.42, 0.34, 0.34), center=(0, 0, 2.95))
    P.add(gimbal, m["dark"], bevel=0.03, segments=1)
    dm = T(0, -0.15, 3.05) @ rot(62, "X")
    dish(P, m["dishp"], m["dark"], dm, D=2.7, f=1.05)
    # --- whip antenna and omni antenna
    whip = L.cyl(P.coll, 0.03, 3.2, segments=6, axis="Z", center=(0, 0, 1.6), r_top=0.012)
    P.add(whip, m["steel"], smooth=50, matrix=T(0.55, 0.45, 1.75) @ rot(-9, "X") @ rot(6, "Y"))
    omni = L.cyl(P.coll, 0.04, 1.1, segments=6, axis="Z", center=(0, 0, -0.55))
    P.add(omni, m["steel"], smooth=50, matrix=T(-0.5, 0.4, -1.65))
    disc = L.cyl(P.coll, 0.16, 0.04, segments=12, axis="Z", center=(0, 0, -1.05))
    P.add(disc, m["dark"], smooth=40, matrix=T(-0.5, 0.4, -1.65))
    # --- thrusters on the bottom corners
    for k in range(4):
        a = math.radians(45 + 90 * k)
        nozzle(P, m["dark"], T(math.cos(a) * 0.72, math.sin(a) * 0.72, -1.68) @ rot(180, "X"))
    # --- blinking light on the deck rim
    house = L.cyl(P.coll, 0.16, 0.12, segments=12, axis="Z", center=(0, 0, 0))
    P.add(house, m["dark"], bevel=0.015, segments=1, matrix=T(0.55, -0.5, 1.86))
    lamp = L.lathe(P.coll, [(0.0, 0.12), (0.06, 0.11), (0.11, 0.07), (0.14, 0.0)], segments=12, axis="Z")
    P.add(lamp, m["glow_warm"], smooth=60, matrix=T(0.55, -0.5, 1.9))
    # --- solar wings: booms + yokes; right wing intact, left wing bent and snapped
    for side in (1, -1):
        boom = L.cyl(P.coll, 0.075, 1.75, segments=10, axis="X", center=(side * 1.78, 0, 0.55))
        P.add(boom, m["steel"], smooth=40)
        yoke = L.box(P.coll, (0.3, 0.32, 0.36), center=(side * 2.62, 0, 0.55))
        P.add(yoke, m["dark"], bevel=0.025, segments=1)
        drive = L.cyl(P.coll, 0.2, 0.36, segments=12, axis="X", center=(side * 1.18, 0, 0.55))
        P.add(drive, m["dark"], bevel=0.02, segments=1)
    # right (+X): three panels, tilted 14 deg to track the sun
    PW = 1.72
    wing = T(2.75, 0, 0.55) @ rot(14, "X")
    x = 0.0
    for i in range(3):
        solar_panel(P, m["solar"], m["gold"], size=(PW, 1.7), matrix=wing @ T(x, 0, 0))
        if i < 2:
            for zz in (-0.55, 0.55):
                hinge = L.box(P.coll, (0.16, 0.09, 0.22), center=(x + PW + 0.04, 0, zz))
                P.add(hinge, m["dark"], bevel=0.01, segments=1, matrix=wing)
        x += PW + 0.08
    P.hulls.append([wing @ Vector((xx, yy, zz)) for xx in (0.0, x) for yy in (-0.06, 0.06) for zz in (-0.86, 0.86)])
    # left (-X): panel 1 bent down at the yoke, panel 2 folded at a broken hinge and torn, panel 3 gone
    lw = T(-2.75, 0, 0.55) @ rot(180, "Z") @ rot(-24, "X") @ rot(9, "Z")
    solar_panel(P, m["solar"], m["gold"], size=(PW, 1.7), matrix=lw)
    hinge = L.box(P.coll, (0.2, 0.12, 0.26), center=(PW + 0.04, 0, -0.62))
    P.add(hinge, m["dark"], bevel=0.01, segments=1, matrix=lw)
    p2 = lw @ T(PW + 0.08, 0, -0.62) @ rot(58, "Y") @ rot(-18, "Z") @ T(-0.04, 0, 0.62)
    solar_panel(P, m["solar"], m["gold"], size=(PW, 1.7), matrix=p2, torn=(0.55, 0.2, 21))
    spar, sm = L.bar(P.coll, (0.0, 0.0, 0.0), (1.4, 0.08, 0.3), 0.05, 0.05)
    P.add(spar, m["steel"], smooth=40, matrix=p2 @ T(1.2, 0, 0.45) @ sm, deform=lambda o: L.jag_end(o, "Y", +1, 0.05, 3))
    P.hulls.append([lw @ Vector((xx, yy, zz)) for xx in (0.0, PW) for yy in (-0.06, 0.06) for zz in (-0.86, 0.86)])
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=1.0,
                                 extra_manifest={"family": "derelict_satellite"})


def build_sat02(asset, tag):
    P = L.Part(asset, "SpaceDressing")
    sc = 0.5
    m = mats_common("Sat2", sc, body=("violet", "violet2"), accent="teal", panel=(0.55, 0.012), fade=0.45)
    m["body"] = L.role_paint("Sat2_Body", C["violet"], C["violet2"], metallic=0.3, rough=0.44, wear=0.55, fade=0.45,
                             fade_color="#9C93AE", grime=0.35, panel=(0.55, 0.012), scale=sc, seed=1, scorch=0.5)
    m["ivory"] = L.role_paint("Sat2_Ivory", C["ivory"], C["ivory2"], metallic=0.25, rough=0.45, wear=0.5, fade=0.3,
                              grime=0.35, panel=(0.5, 0.01), scale=sc, seed=11,
                              label=(C["violet"], (-0.3, 0.3), (-0.3, 0.3), ("X", "Z")))
    m["foil"] = L.role_foil("Sat2_Foil", "#D2A03A", scale=0.35)
    m["dishp"] = L.role_paint("Sat2_Dish", C["ivory"], C["ivory2"], metallic=0.25, rough=0.4, wear=0.4, fade=0.3,
                              grime=0.25, scale=sc, seed=13)
    m["solar"] = solar_mat("Sat2", sc)
    # --- cylindrical bus along Y (front -Y): violet paint, foil waist, ivory end caps
    body = L.lathe(P.coll, [(-1.2, 0.0), (-1.2, 0.55), (-1.12, 0.82), (-1.0, 0.9), (1.0, 0.9), (1.12, 0.82),
                            (1.2, 0.55), (1.2, 0.0)], segments=20, axis="Y")
    P.add(body, m["body"], smooth=35)
    waist = L.lathe(P.coll, [(-0.5, 0.0), (-0.5, 0.95), (0.35, 0.95), (0.35, 0.0)], segments=20, axis="Y")
    P.add(waist, m["foil"], smooth=35)
    for y0 in (-0.62, 0.47):
        band = L.lathe(P.coll, [(y0, 0.0), (y0, 0.97), (y0 + 0.12, 0.97), (y0 + 0.12, 0.0)], segments=20, axis="Y")
        P.add(band, m["gold"], smooth=35)
    capf = L.lathe(P.coll, [(-1.38, 0.0), (-1.38, 0.5), (-1.3, 0.7), (-1.15, 0.72), (-1.15, 0.0)], segments=20, axis="Y")
    P.add(capf, m["ivory"], smooth=35)
    capr = L.lathe(P.coll, [(1.15, 0.0), (1.15, 0.75), (1.3, 0.72), (1.42, 0.5), (1.42, 0.0)], segments=20, axis="Y")
    P.add(capr, m["ivory"], smooth=35)
    P.hull_from([body, waist, capf, capr])
    # --- front dish facing -Y on a short tripod
    for k in range(3):
        a = math.tau * k / 3 + 0.5
        b, bm_ = L.bar(P.coll, (math.cos(a) * 0.45, -1.3, math.sin(a) * 0.45), (math.cos(a) * 0.12, -1.85, math.sin(a) * 0.12), 0.07, 0.07)
        P.add(b, m["dark"], smooth=40, matrix=bm_)
    dish(P, m["dishp"], m["dark"], T(0, -1.9, 0.05) @ rot(90, "X") @ rot(8, "Y"), D=2.3, f=0.85)
    # --- rear engine bell and RCS quads
    bell = L.lathe(P.coll, [(0.0, 0.0), (0.0, 0.2), (0.25, 0.3), (0.75, 0.48), (0.78, 0.52), (0.78, 0.44),
                            (0.3, 0.24), (0.3, 0.0)], segments=18, axis="Y")
    P.add(bell, m["dark"], smooth=40, matrix=T(0, 1.4, 0))
    for k in range(4):
        a = math.radians(45 + 90 * k)
        q = L.box(P.coll, (0.22, 0.26, 0.22), center=(math.cos(a) * 0.8, 0.95, math.sin(a) * 0.8))
        P.add(q, m["dark"], bevel=0.02, segments=1)
        nozzle(P, m["steel"], T(math.cos(a) * 0.92, 0.95, math.sin(a) * 0.92) @ Matrix.Rotation(math.pi / 2 - a, 4, "Y"), r=0.06, length=0.12)
    # --- sensor turret on top with the blinking light
    turret = L.box(P.coll, (0.5, 0.55, 0.3), center=(0, 0.2, 1.0))
    P.add(turret, m["ivory"], bevel=0.03, segments=1)
    lens = L.cyl(P.coll, 0.11, 0.12, segments=12, axis="Y", center=(0.1, -0.12, 1.02))
    P.add(lens, m["dark"], bevel=0.01, segments=1)
    stalk = L.cyl(P.coll, 0.035, 0.5, segments=6, axis="Z", center=(-0.15, 0.35, 1.35))
    P.add(stalk, m["steel"], smooth=40)
    lamp = L.lathe(P.coll, [(0.0, 0.1), (0.05, 0.095), (0.1, 0.06), (0.12, 0.0)], segments=12, axis="Z")
    P.add(lamp, m["glow"], smooth=60, matrix=T(-0.15, 0.35, 1.6))
    greeble_cluster(P, m["dark"], (0.0, 0.6, 0.86), (0, 0, 1), 6, (0.1, 0.24), (0.05, 0.14), 0.3, seed=8)
    # --- boom to +X with two panels: outer one snapped at a hinge and hanging
    boom, bmm = L.bar(P.coll, (0.85, 0.2, 0.0), (3.2, 0.2, 0.0), 0.14, 0.14)
    P.add(boom, m["dark"], bevel=0.015, segments=1, matrix=bmm)
    yoke = L.box(P.coll, (0.24, 0.3, 0.3), center=(3.25, 0.2, 0.0))
    P.add(yoke, m["dark"], bevel=0.02, segments=1)
    wing = T(3.35, 0.2, 0.0) @ rot(-10, "X")
    solar_panel(P, m["solar"], m["gold"], size=(1.75, 1.75), matrix=wing)
    hinge = L.box(P.coll, (0.16, 0.1, 0.22), center=(1.8, 0, -0.75))
    P.add(hinge, m["dark"], bevel=0.01, segments=1, matrix=wing)
    p2 = wing @ T(1.82, 0, -0.75) @ rot(-68, "Y") @ rot(22, "X") @ T(0, 0, 0.75)
    solar_panel(P, m["solar"], m["gold"], size=(1.75, 1.75), matrix=p2, torn=(0.45, 0.15, 5))
    P.hulls.append([wing @ Vector((xx, yy, zz)) for xx in (0.0, 1.8) for yy in (-0.06, 0.06) for zz in (-0.9, 0.9)])
    # --- snapped boom stub on -X
    stub, stm = L.bar(P.coll, (-0.85, 0.2, 0.0), (-1.9, 0.25, 0.12), 0.14, 0.14, segments=3)
    P.add(stub, m["dark"], bevel=0.015, segments=1, matrix=stm, deform=lambda o: L.jag_end(o, "Y", +1, 0.12, 7))
    wire = L.tube_path(P.coll, L.bezier_points((-1.7, 0.2, 0.02), (-2.2, 0.0, -0.3), (-2.4, 0.5, -0.6), (-2.15, 0.9, -0.9), 12), 0.025, sides=6)
    P.add(wire, m["accent"], smooth=50)
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=0.9,
                                 extra_manifest={"family": "derelict_satellite"})


# =========================================================================== cargo pods

def hazard_mat(name, scale, width=0.32):
    return L.role_paint(name, C["hazard_gold"], metallic=0.25, rough=0.45, wear=0.6, grime=0.35, scale=scale,
                        stripes=(C["hazard_gold"], C["hazard_teal"], width, (0.7071, 0.0, 0.7071)), seed=9)


def build_pod01(asset, tag):
    """ISO-style container 8.2 m: corrugated sides, gold end frames, corner castings, door locking
    bars, gold/teal hazard bands, an ID block on the roof."""
    P = L.Part(asset, "SpaceDressing")
    sc = 0.7
    m = mats_common("Pod1", sc)
    m["body"] = L.role_paint("Pod1_Body", C["blue_grey"], "#323C4A", metallic=0.35, rough=0.48, wear=0.6,
                             wear_color="#A9B0B6", fade=0.25, fade_color="#59626E", grime=0.35, scale=sc, seed=1,
                             label=(C["ivory"], (-0.75, 0.75), (-2.6, -1.1), ("X", "Y")))
    m["frame"] = L.role_paint("Pod1_Frame", C["yellow_paint"], "#A9862F", metallic=0.35, rough=0.45, wear=0.7,
                              grime=0.35, scale=sc, seed=4)
    m["hazard"] = hazard_mat("Pod1_Hazard", sc)
    W, Hh, Lg = 2.9, 2.9, 7.6
    body = L.box(P.coll, (W, Lg, Hh))
    P.add(body, m["body"], bevel=0.04, segments=2)
    P.hull_from([body])
    # corrugation on +-X
    n = 15
    for side in (1, -1):
        for i in range(n):
            y = -2.9 + 5.8 * i / (n - 1)
            rib = L.box(P.coll, (0.1, 0.16, 2.45), center=(side * (W * 0.5 + 0.03), y, 0))
            P.add(rib, m["body"], bevel=0.02, segments=1)
    # roof/floor panels (raised plates with seams)
    for zs in (1, -1):
        for j, (y0, y1) in enumerate(((-3.1, -0.15), (0.15, 3.1))):
            pl = L.box(P.coll, (2.4, y1 - y0, 0.06), center=(0, (y0 + y1) * 0.5, zs * (Hh * 0.5 + 0.02)))
            P.add(pl, m["body"], bevel=0.015, segments=1)
    # hazard bands near both ends
    for ys in (1, -1):
        band = L.box(P.coll, (W + 0.08, 0.5, Hh + 0.08), center=(0, ys * 3.4, 0))
        P.add(band, m["hazard"], bevel=0.02, segments=1)
    # end frames + corner castings
    for ys in (1, -1):
        y = ys * 3.95
        for zz in (1, -1):
            b = L.box(P.coll, (3.2, 0.32, 0.3), center=(0, y, zz * 1.45))
            P.add(b, m["frame"], bevel=0.03, segments=1)
        for xx in (1, -1):
            b = L.box(P.coll, (0.3, 0.32, 2.6), center=(xx * 1.45, y, 0))
            P.add(b, m["frame"], bevel=0.03, segments=1)
        for xx in (1, -1):
            for zz in (1, -1):
                c = L.box(P.coll, (0.44, 0.44, 0.44), center=(xx * 1.45, ys * 3.98, zz * 1.45))
                L.hs["inset_panels"](c, normal=None, min_dot=0.0, fraction=1.0, thickness=0.11, depth=-0.06, min_area=0.05)
                P.add(c, m["dark"], bevel=0.02, segments=1)
    # door end (-Y): two leaves, locking bars with cam keepers and handles; rear end (+Y): plate
    for xx in (-0.7, 0.7):
        leaf = L.box(P.coll, (1.32, 0.06, 2.5), center=(xx, -3.82, 0))
        P.add(leaf, m["body"], bevel=0.015, segments=1)
    for xx in (-1.05, -0.35, 0.35, 1.05):
        bar = L.cyl(P.coll, 0.045, 2.55, segments=8, axis="Z", center=(xx, -3.92, 0))
        P.add(bar, m["steel"], smooth=40)
        for zz in (1.2, -1.2):
            k = L.box(P.coll, (0.16, 0.12, 0.12), center=(xx, -3.9, zz))
            P.add(k, m["dark"], bevel=0.015, segments=1)
        h = L.box(P.coll, (0.32, 0.06, 0.05), center=(xx + 0.13, -3.98, -0.3))
        P.add(h, m["steel"], bevel=0.01, segments=1)
    rear = L.box(P.coll, (2.6, 0.08, 2.6), center=(0, 3.84, 0))
    L.hs["inset_panels"](rear, normal=(0, 1, 0), min_dot=0.9, fraction=1.0, thickness=0.25, depth=-0.04)
    P.add(rear, m["body"], bevel=0.015, segments=1)
    # status light strip by the door + tie-down rings on the roof
    s = L.box(P.coll, (0.06, 0.06, 0.55), center=(1.57, -3.76, 0.9))
    P.add(s, m["glow"], bevel=0.008, segments=1)
    for yy in (-2.2, 2.2):
        for xx in (-1.0, 1.0):
            ring = L.tube_path(P.coll, [(xx - 0.15, yy, 1.5), (xx - 0.12, yy, 1.66), (xx, yy, 1.72), (xx + 0.12, yy, 1.66), (xx + 0.15, yy, 1.5)], 0.035, sides=6)
            P.add(ring, m["steel"], smooth=50)
    return L.finish_hard_surface(P, asset, 1024, tag, (2000, 25000), ao_distance=0.8,
                                 extra_manifest={"family": "cargo_pod"})


def build_pod02(asset, tag):
    """Capsule pod 6.6 m: ivory hull, violet reinforcement rings, gold/teal hazard waist band,
    skid rails, side hatch with latches, docking ring at the nose, thrusters at the tail."""
    P = L.Part(asset, "SpaceDressing")
    sc = 0.6
    m = mats_common("Pod2", sc, panel=(0.9, 0.012))
    m["violet"] = L.role_paint("Pod2_Violet", C["violet"], C["violet2"], metallic=0.35, rough=0.4, wear=0.6,
                               fade=0.3, fade_color="#8C80A6", grime=0.3, scale=sc, seed=6)
    m["hazard"] = hazard_mat("Pod2_Hazard", sc, 0.28)
    R = 1.28
    prof = [(-3.1, 0.0), (-3.08, 0.35), (-2.95, 0.72), (-2.7, 1.02), (-2.4, 1.2), (-2.1, R), (2.1, R), (2.4, 1.2),
            (2.7, 1.02), (2.95, 0.72), (3.08, 0.35), (3.1, 0.0)]
    hull = L.lathe(P.coll, prof, segments=28, axis="Y")
    P.add(hull, m["body"], smooth=35)
    P.hull_from([hull])
    for y0 in (-2.05, -1.0, 0.9, 1.95):
        r = L.lathe(P.coll, [(y0, 0.0), (y0, R + 0.07), (y0 + 0.03, R + 0.1), (y0 + 0.13, R + 0.1), (y0 + 0.16, R + 0.07), (y0 + 0.16, 0.0)], segments=28, axis="Y")
        P.add(r, m["violet"], smooth=35)
    band = L.lathe(P.coll, [(-0.55, 0.0), (-0.55, R + 0.04), (0.55, R + 0.04), (0.55, 0.0)], segments=28, axis="Y")
    P.add(band, m["hazard"], smooth=35)
    for k in range(4):
        a = math.radians(45 + 90 * k)
        rail = L.box(P.coll, (0.16, 3.6, 0.2), center=(0, 0, 0))
        P.add(rail, m["dark"], bevel=0.02, segments=1, matrix=Matrix.Rotation(a, 4, "Y") @ T(0, 0, R + 0.04))
    # hatch on +X with latches and grab handle
    hatch = L.box(P.coll, (0.1, 1.1, 0.95), center=(R - 0.02, 0.0, 0.0))
    P.add(hatch, m["violet"], bevel=0.02, segments=1)
    for yy, zz in ((-0.45, 0.38), (0.45, 0.38), (-0.45, -0.38), (0.45, -0.38)):
        lt = L.box(P.coll, (0.08, 0.14, 0.1), center=(R + 0.06, yy, zz))
        P.add(lt, m["steel"], bevel=0.01, segments=1)
    hd = L.tube_path(P.coll, [(R + 0.02, -0.3, 0.6), (R + 0.16, -0.25, 0.6), (R + 0.18, 0.0, 0.6), (R + 0.16, 0.25, 0.6), (R + 0.02, 0.3, 0.6)], 0.03, sides=6)
    P.add(hd, m["steel"], smooth=50)
    # docking ring + probe at the nose, three thrusters at the tail
    dock = L.lathe(P.coll, [(-3.3, 0.3), (-3.3, 0.5), (-3.02, 0.55), (-3.02, 0.3), (-3.3, 0.3)], segments=20, axis="Y")
    P.add(dock, m["steel"], smooth=40)
    probe = L.cyl(P.coll, 0.08, 0.6, segments=10, axis="Y", center=(0, -3.35, 0))
    P.add(probe, m["dark"], bevel=0.01, segments=1)
    for k in range(3):
        a = math.tau * k / 3
        nozzle(P, m["dark"], T(math.cos(a) * 0.45, 3.0, math.sin(a) * 0.45) @ rot(-90, "X"), r=0.14, length=0.26)
    # running lights
    for y0 in (-1.0, 1.95):
        lamp = L.lathe(P.coll, [(0.0, 0.07), (0.04, 0.065), (0.07, 0.04), (0.085, 0.0)], segments=10, axis="Z")
        P.add(lamp, m["glow"], smooth=60, matrix=T(0, y0 + 0.08, R + 0.1))
    return L.finish_hard_surface(P, asset, 1024, tag, (2000, 25000), ao_distance=0.7,
                                 extra_manifest={"family": "cargo_pod"})


def build_pod03(asset, tag):
    """Hexagonal crate 4.8 m in a gunmetal frame cage, violet panels, gold/teal hazard end caps."""
    P = L.Part(asset, "SpaceDressing")
    sc = 0.5
    m = mats_common("Pod3", sc)
    m["violet"] = L.role_paint("Pod3_Violet", C["violet"], C["violet2"], metallic=0.35, rough=0.42, wear=0.6,
                               fade=0.3, fade_color="#8C80A6", grime=0.3, scale=sc, seed=6, panel=(0.8, 0.012),
                               label=(C["ivory"], (-0.5, 0.5), (-1.2, 0.2), ("X", "Y")))
    m["hazard"] = hazard_mat("Pod3_Hazard", sc, 0.26)
    R = 1.5
    core = L.cyl(P.coll, R, 4.0, segments=6, axis="Y")
    L.hs["inset_panels"](core, normal=None, min_dot=0.0, fraction=1.0, thickness=0.16, depth=-0.05, min_area=0.5)
    P.add(core, m["violet"], bevel=0.03, segments=2)
    P.hull_from([core])
    corners = [Vector((math.cos(math.radians(60 * k)) * (R + 0.02), 0, math.sin(math.radians(60 * k)) * (R + 0.02))) for k in range(6)]
    for c in corners:
        b, bm_ = L.bar(P.coll, (c.x, -2.3, c.z), (c.x, 2.3, c.z), 0.24, 0.24)
        P.add(b, m["dark"], bevel=0.025, segments=1, matrix=bm_)
    for y in (-2.15, 0.0, 2.15):
        for k in range(6):
            a, b2 = corners[k], corners[(k + 1) % 6]
            b, bm_ = L.bar(P.coll, (a.x, y, a.z), (b2.x, y, b2.z), 0.2, 0.22, up=(0, 1, 0))
            P.add(b, m["dark"] if y else m["gold"], bevel=0.02, segments=1, matrix=bm_)
    for ys in (1, -1):
        cap = L.cyl(P.coll, R * 0.93, 0.1, segments=6, axis="Y", center=(0, ys * 2.05, 0))
        P.add(cap, m["hazard"], bevel=0.015, segments=1)
        for k in range(4):
            a = math.radians(30 + 90 * k)
            lt = L.box(P.coll, (0.22, 0.12, 0.16), center=(math.cos(a) * 0.9, ys * 2.13, math.sin(a) * 0.9))
            P.add(lt, m["steel"], bevel=0.015, segments=1)
        hub = L.cyl(P.coll, 0.32, 0.12, segments=12, axis="Y", center=(0, ys * 2.13, 0))
        P.add(hub, m["dark"], bevel=0.015, segments=1)
    for xs in (1, -1):
        h = L.tube_path(P.coll, [(xs * 1.36, -0.6, 0.55), (xs * 1.58, -0.55, 0.62), (xs * 1.62, 0.0, 0.64), (xs * 1.58, 0.55, 0.62), (xs * 1.36, 0.6, 0.55)], 0.04, sides=6)
        P.add(h, m["steel"], smooth=50)
    lamp = L.lathe(P.coll, [(0.0, 0.08), (0.05, 0.075), (0.08, 0.045), (0.1, 0.0)], segments=10, axis="Z")
    P.add(lamp, m["glow"], smooth=60, matrix=T(corners[1].x, -2.15, corners[1].z + 0.12))
    return L.finish_hard_surface(P, asset, 1024, tag, (2000, 25000), ao_distance=0.6,
                                 extra_manifest={"family": "cargo_pod"})


# =========================================================================== beacon buoy

def build_buoy(asset, tag):
    """5 m navigation buoy: ivory spindle with gold band, armoured halo ring with a teal glow strip on
    three arms, solar petals, stabiliser fins, warm-white light tip in a cage."""
    P = L.Part(asset, "SpaceDressing")
    sc = 0.4
    m = mats_common("Buoy", sc, panel=(0.45, 0.01))
    m["solar"] = solar_mat("Buoy", sc)
    prof = [(-2.5, 0.0), (-2.42, 0.16), (-2.2, 0.36), (-1.95, 0.5), (-1.7, 0.52), (-1.45, 0.42), (-1.3, 0.24),
            (-1.1, 0.2), (-1.0, 0.32), (-0.92, 0.55), (-0.85, 0.6), (0.35, 0.6), (0.42, 0.55), (0.6, 0.38),
            (0.75, 0.2), (0.8, 0.0)]
    body = L.lathe(P.coll, prof, segments=24, axis="Z")
    P.add(body, m["body"], smooth=35)
    P.hull_from([body])
    band = L.lathe(P.coll, [(-0.25, 0.0), (-0.25, 0.63), (0.05, 0.63), (0.05, 0.0)], segments=24, axis="Z")
    P.add(band, m["gold"], smooth=35)
    collar = L.lathe(P.coll, [(-1.32, 0.0), (-1.32, 0.3), (-1.08, 0.3), (-1.08, 0.0)], segments=16, axis="Z")
    P.add(collar, m["dark"], smooth=35)
    mast = L.cyl(P.coll, 0.08, 1.6, segments=10, axis="Z", center=(0, 0, 1.5))
    P.add(mast, m["dark"], smooth=40)
    for z in (1.0, 1.6):
        rib = L.cyl(P.coll, 0.12, 0.06, segments=10, axis="Z", center=(0, 0, z))
        P.add(rib, m["steel"], smooth=40)
    tip = L.lathe(P.coll, [(0.0, 0.15), (0.12, 0.14), (0.22, 0.09), (0.27, 0.0)], segments=14, axis="Z")
    P.add(tip, m["glow_warm"], smooth=60, matrix=T(0, 0, 2.28))
    base = L.cyl(P.coll, 0.17, 0.06, segments=12, axis="Z", center=(0, 0, 2.28))
    P.add(base, m["dark"], smooth=40)
    cap = L.cyl(P.coll, 0.12, 0.04, segments=12, axis="Z", center=(0, 0, 2.62))
    P.add(cap, m["dark"], smooth=40)
    for k in range(3):
        a = math.tau * k / 3
        b, bm_ = L.bar(P.coll, (math.cos(a) * 0.15, math.sin(a) * 0.15, 2.28), (math.cos(a) * 0.13, math.sin(a) * 0.13, 2.62), 0.025, 0.025)
        P.add(b, m["dark"], smooth=40, matrix=bm_)
    # halo ring: armoured channel + inner glow strip, on three arms
    ring = L.revolve_closed(P.coll, [(1.36, -0.14), (1.52, -0.14), (1.57, -0.09), (1.57, 0.09), (1.52, 0.14), (1.36, 0.14)],
                            (0.0, 360.0), 48, axis="Z")
    P.add(ring, m["body"], smooth=30)
    strip = L.revolve_closed(P.coll, [(1.30, -0.04), (1.37, -0.04), (1.37, 0.04), (1.30, 0.04)], (0.0, 360.0), 48, axis="Z")
    P.add(strip, m["glow"], smooth=30)
    lips = []
    for zs in (1, -1):
        lip = L.revolve_closed(P.coll, [(1.3, zs * 0.075), (1.4, zs * 0.075), (1.4, zs * 0.15), (1.3, zs * 0.15)], (0.0, 360.0), 48, axis="Z")
        P.add(lip, m["gold"], smooth=30)
    P.hull_from([ring])
    for k in range(3):
        a = math.tau * k / 3 + math.pi / 6
        b, bm_ = L.bar(P.coll, (math.cos(a) * 0.5, math.sin(a) * 0.5, -0.35), (math.cos(a) * 1.42, math.sin(a) * 1.42, 0.0),
                       0.13, 0.2, segments=2)
        P.add(b, m["dark"], bevel=0.02, segments=1, matrix=bm_)
        clamp = L.box(P.coll, (0.22, 0.26, 0.36))
        P.add(clamp, m["dark"], bevel=0.02, segments=1, matrix=T(math.cos(a) * 1.46, math.sin(a) * 1.46, 0.0) @ Matrix.Rotation(a, 4, "Z"))
    # solar petals on the upper shoulder
    for k in range(6):
        a = math.tau * k / 6
        pm = Matrix.Rotation(a, 4, "Z") @ T(0.55, 0, 0.42) @ rot(-35, "Y")
        pet = L.box(P.coll, (0.5, 0.36, 0.035), center=(0.25, 0, 0))
        P.add(pet, m["solar"], bevel=0.008, segments=1, matrix=pm)
    # stabiliser fins
    for k in range(3):
        a = math.tau * k / 3 + math.pi / 3
        fin = L.extrude_profile(P.coll, [(0.3, -2.25), (0.7, -2.15), (1.05, -1.75), (0.95, -1.55), (0.42, -1.45)], 0.06, axis="Y")
        P.add(fin, m["accent"], bevel=0.012, segments=1, matrix=Matrix.Rotation(a, 4, "Z"))
    greeble_cluster(P, m["dark"], (0.6, 0.0, -0.4), (1, 0, 0), 5, (0.08, 0.18), (0.04, 0.1), 0.25, seed=4)
    greeble_cluster(P, m["dark"], (-0.3, 0.52, -0.6), (-0.5, 0.87, 0), 4, (0.08, 0.18), (0.04, 0.1), 0.22, seed=5)
    return L.finish_hard_surface(P, asset, 1024, tag, (2000, 25000), ao_distance=0.6,
                                 extra_manifest={"family": "beacon_buoy", "blink": True})


# =========================================================================== station truss

def truss(P, m, length, w, bay, origin=Matrix.Identity(4), lon=0.3, frame=0.22, diag=0.14, damage=None,
          seed=0, flange=False, torn_start=None):
    """Square lattice girder along local +Y from 0 to `length`. damage: dict(lon_cut=[a,b,c,d] metres
    short of length, kinks={idx: (deg, axis)}, torn_from=y, hang=True). Returns hull points."""
    rng = random.Random(seed)
    hw = w * 0.5
    corners = [(-hw, -hw), (hw, -hw), (hw, hw), (-hw, hw)]
    d = damage or {}
    torn = d.get("torn_from", length + 1.0)
    cuts = d.get("lon_cut", [0, 0, 0, 0])
    kinks = d.get("kinks", {})
    y_start = 0.0 if torn_start is None else torn_start
    for i, (x, z) in enumerate(corners):
        y0 = 0.0 + (rng.uniform(0.0, 1.2) if torn_start is not None else 0.0)
        Li = length - cuts[i]
        b, bm_ = L.bar(P.coll, (x, y0, z), (x, Li, z), lon, lon, segments=max(2, int((Li - y0) / 2.0)))
        mid = (Li + y0) * 0.5

        def deform(o, i=i, mid=mid):
            if i in kinks:
                ang, ax = kinks[i]
                L.twist_rotate_tail(o, along="Y", start=torn - mid, angle_deg=ang, axis=ax)
            if d:
                L.jag_end(o, along="Y", end=+1, depth=0.35, seed=seed + i)
            if torn_start is not None:
                L.jag_end(o, along="Y", end=-1, depth=0.35, seed=seed + 10 + i)
        P.add(b, m["lon"], smooth=35.0, matrix=origin @ bm_, deform=deform)
    nb = int(round(length / bay))
    for k in range(nb + 1):
        y = k * bay
        if y > torn - 0.2 or y < y_start + 0.3:
            continue
        for j in range(4):
            (x0, z0), (x1, z1) = corners[j], corners[(j + 1) % 4]
            b, bm_ = L.bar(P.coll, (x0, y, z0), (x1, y, z1), frame, frame, up=(0, 1, 0))
            P.add(b, m["frame"], smooth=35.0, matrix=origin @ bm_)
    hanging_done = False
    for k in range(nb):
        y0, y1 = k * bay, (k + 1) * bay
        if y0 < y_start + 0.3:
            continue
        for j in range(4):
            (x0, z0), (x1, z1) = corners[j], corners[(j + 1) % 4]
            if (k + j) % 2:
                p0, p1 = (x0, y0, z0), (x1, y1, z1)
            else:
                p0, p1 = (x1, y0, z1), (x0, y1, z0)
            if y1 > torn:
                if d.get("hang") and not hanging_done:
                    hanging_done = True
                    b, bm_ = L.bar(P.coll, p0, (x0 * 1.6, y0 + bay * 0.7, z0 + 1.8), diag, diag)
                    P.add(b, m["diag"], smooth=35.0, matrix=origin @ bm_, deform=lambda o: L.jag_end(o, "Y", +1, 0.25, 5))
                continue
            if d and y1 > torn - bay and rng.random() < 0.4:
                continue
            b, bm_ = L.bar(P.coll, p0, p1, diag, diag)
            P.add(b, m["diag"], smooth=35.0, matrix=origin @ bm_)
    pts = [origin @ Vector((x, y, z)) for x in (-hw - 0.2, hw + 0.2) for y in (0.0, max(length - min(cuts), 0.5)) for z in (-hw - 0.2, hw + 0.2)]
    return pts


def truss_mats(prefix, sc, label_axes=("Y", "Z")):
    m = mats_common(prefix, sc, panel=(2.0, 0.03))
    m["lon"] = L.role_paint(prefix + "_Lon", C["ivory"], C["ivory2"], metallic=0.3, rough=0.48, wear=0.6, fade=0.3,
                            grime=0.45, scale=sc, seed=21, scorch=0.3)
    m["frame"] = L.role_paint(prefix + "_Frame", C["yellow_paint"], "#A88630", metallic=0.35, rough=0.46, wear=0.7,
                              grime=0.45, scale=sc, seed=22)
    m["diag"] = L.role_paint(prefix + "_Diag", C["gunmetal"], C["gunmetal2"], metallic=0.5, rough=0.5, wear=0.6,
                             grime=0.4, scale=sc, seed=23)
    m["hullp"] = L.role_paint(prefix + "_Hull", C["ivory"], C["ivory2"], metallic=0.25, rough=0.46, wear=0.5,
                              fade=0.35, grime=0.4, panel=(2.0, 0.03), scale=sc, seed=24, scorch=0.4,
                              label=(C["violet"], (-20, 20), (0.3, 1.1), label_axes))
    m["cable"] = L.role_paint(prefix + "_Cable", "#2A2D33", metallic=0.1, rough=0.7, wear=0.0, grime=0.2, scale=sc, seed=25)
    m["cable2"] = L.role_paint(prefix + "_CableTeal", C["teal2"], metallic=0.1, rough=0.6, wear=0.0, grime=0.2, scale=sc, seed=26)
    m["burnt"] = L.role_metal(prefix + "_Burnt", "#4A4440", rough=0.55, metallic=0.7, scale=sc, seed=27)
    return m


def spill_cables(P, m, start_pts, rng, reach=6.0, origin=Matrix.Identity(4), radius=(0.06, 0.12)):
    for i, p in enumerate(start_pts):
        p = Vector(p)
        a = Vector((rng.uniform(-1, 1), rng.uniform(0.6, 1.4), rng.uniform(-1, 1)))
        p1 = p + Vector((0, reach * 0.35, 0))
        p2 = p + a * reach * 0.7
        p3 = p + Vector((rng.uniform(-1, 1) * reach * 0.6, reach * rng.uniform(0.5, 1.0), rng.uniform(-1, 1) * reach * 0.6))
        pts = L.bezier_points(p, p1, p2, p3, 22)
        c = L.tube_path(P.coll, [origin @ q for q in pts], rng.uniform(*radius), sides=7)
        P.add(c, m["cable"] if i % 2 == 0 else m["cable2"], smooth=50)


def build_truss01(asset, tag):
    """42 m square girder torn from a station: docking flange at the intact end, pipe run on clamps,
    a clad hull section, equipment boxes, and a torn end with kinked longerons and spilled cables."""
    P = L.Part(asset, "SpaceDressing")
    sc = 2.0
    m = truss_mats("Tr1", sc)
    rng = random.Random(11)
    Lg, w = 40.0, 3.2
    dmg = dict(lon_cut=[0.0, 2.4, 0.7, 3.6], kinks={0: (16, "X"), 2: (-22, "Z")}, torn_from=Lg - 5.0, hang=True)
    pts = truss(P, m, Lg, w, 4.0, damage=dmg, seed=3, lon=0.4, frame=0.3, diag=0.19)
    P.hulls.append(pts)
    # docking flange + collar at y = 0
    plate = L.box(P.coll, (4.2, 0.45, 4.2), center=(0, -0.2, 0))
    L.hs["inset_panels"](plate, normal=(0, -1, 0), min_dot=0.9, fraction=1.0, thickness=0.5, depth=-0.08)
    P.add(plate, m["dark"], bevel=0.04, segments=1)
    collar = L.lathe(P.coll, [(-1.4, 1.2), (-1.4, 1.75), (-1.0, 1.85), (-0.42, 1.85), (-0.42, 1.2), (-1.4, 1.2)], segments=24, axis="Y")
    P.add(collar, m["accent"], smooth=35)
    for k in range(8):
        a = math.tau * k / 8
        lug = L.box(P.coll, (0.3, 0.5, 0.3), center=(math.cos(a) * 1.9, -1.0, math.sin(a) * 1.9))
        P.add(lug, m["dark"], bevel=0.03, segments=1)
    # pipe run along +X face on clamps, bending away at the torn end
    for zz, r in ((0.55, 0.22), (-0.15, 0.16)):
        path = [(w * 0.5 + 0.55, y, zz) for y in [0.4 + 1.0 * i for i in range(30)]]
        path += [Vector(q) for q in L.bezier_points((w * 0.5 + 0.55, 30.4, zz), (w * 0.5 + 0.6, 32.5, zz),
                                                    (w * 0.5 + 1.6, 34.0, zz + 0.6), (w * 0.5 + 2.6, 34.6, zz + 1.4), 10)[1:]]
        pipe = L.tube_path(P.coll, path, r, sides=10)
        P.add(pipe, m["steel"], smooth=40)
    for y in range(2, 31, 4):
        cl = L.box(P.coll, (0.85, 0.22, 1.1), center=(w * 0.5 + 0.35, y, 0.2))
        P.add(cl, m["dark"], bevel=0.02, segments=1)
    # hull cladding on -X between y 8 and 24 (torn at its far end)
    clad = L.grid_plate(P.coll, 16.0, 3.6, 16, 4, 0.16, jag=0.5, jag_sides=(0, 1, 0, 0), seed=4, holes=0.15)
    clad.data.transform(Matrix(((0, 0, 1, 0), (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    P.add(clad, m["hullp"], bevel=0.02, segments=1, matrix=T(-w * 0.5 - 0.22, 16.0, 0))
    for y in (9.0, 15.0, 21.0):
        rib = L.box(P.coll, (0.3, 0.3, 3.4), center=(-w * 0.5 - 0.1, y, 0))
        P.add(rib, m["dark"], bevel=0.02, segments=1)
    # equipment boxes on the top face with dim status lights
    for i, (y, sx, sy, sz) in enumerate(((12.5, 1.4, 1.8, 0.9), (14.6, 1.0, 1.1, 0.6), (26.0, 1.5, 1.2, 0.8))):
        bx = L.box(P.coll, (sx, sy, sz), center=(0.2 * (-1) ** i, y, w * 0.5 + 0.1 + sz * 0.5))
        L.hs["inset_panels"](bx, normal=(1, 0, 0), min_dot=0.9, fraction=1.0, thickness=0.12, depth=-0.04)
        P.add(bx, m["body"], bevel=0.04, segments=1)
        li = L.box(P.coll, (0.05, 0.25, 0.08), center=(sx * 0.5 + 0.2 * (-1) ** i + 0.01, y - sy * 0.3, w * 0.5 + 0.1 + sz * 0.7))
        P.add(li, m["glow"], bevel=0.01, segments=1)
    # cables inside the girder, spilling out of the torn end
    for x, z in ((0.5, 0.6), (-0.4, -0.5), (0.1, -0.9)):
        run = L.tube_path(P.coll, [(x, y, z) for y in (1.0, 12.0, 24.0, 33.0)], 0.1, sides=7)
        P.add(run, m["cable"], smooth=50)
    spill_cables(P, m, [(0.5, 33.0, 0.6), (-0.4, 33.0, -0.5), (0.1, 33.0, -0.9)], rng, reach=7.0)
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=3.0,
                                 extra_manifest={"family": "station_truss"})


def build_truss02(asset, tag):
    """26 m curved hull section: outer skin with a lit window row, ribs and stringers on the inside,
    both torn edges jagged with ribs sticking past the skin, cables hanging out."""
    P = L.Part(asset, "SpaceDressing")
    sc = 1.6
    m = truss_mats("Tr2", sc, label_axes=("X", "Y"))
    m["window_dark"] = L.role_paint("Tr2_WinDark", "#141A22", metallic=0.6, rough=0.12, wear=0.0, grime=0.0, scale=sc)
    rng = random.Random(5)
    Rb = 34.0
    SX, SY = 24.0, 15.0

    def bend_x(o):
        L.bend(o, along="X", radius=Rb, start=-SX * 0.5 - 2.0, plane_axis="Z")
    skin = L.grid_plate(P.coll, SX, SY, 24, 15, 0.45, jag=0.9, jag_sides=(0, 1, 0, 1), seed=8, holes=0.22)
    zmin = min(v.co.z for v in skin.data.vertices)
    zmax = max(v.co.z for v in skin.data.vertices)
    P.add(skin, m["hullp"], bevel=0.04, segments=1, deform=bend_x)
    # raised outer armour plates (on the convex -Z side) in a staggered pattern
    for i, (x, y, sx, sy) in enumerate(((-9.0, -4.5, 5.5, 4.2), (-2.5, -4.8, 6.5, 3.6), (-8.0, 2.2, 4.5, 5.0),
                                        (-2.0, 1.5, 5.0, 4.4), (4.2, -3.8, 4.0, 4.8))):
        pl = L.grid_plate(P.coll, sx, sy, 5, 4, 0.24)
        pl.data.transform(T(x, y, zmin - (min(v.co.z for v in pl.data.vertices) + 0.12) - 0.08))
        P.add(pl, m["body"], bevel=0.03, segments=1, deform=bend_x)
    # window row (some lit)
    for i in range(8):
        x = -10.5 + i * 1.6
        win = L.box(P.coll, (0.9, 0.55, (zmax - zmin) + 0.2), center=(x, 5.6, (zmin + zmax) * 0.5 - 0.05))
        lit = i in (1, 2, 5)
        P.add(win, m["glow_warm"] if lit else m["window_dark"], bevel=0.03, segments=1, deform=bend_x)
        fr = L.box(P.coll, (1.1, 0.75, 0.3), center=(x, 5.6, zmin - 0.05))
        P.add(fr, m["dark"], bevel=0.03, segments=1, deform=bend_x)
    # ribs (I-beams across X) and stringers (along Y) on the concave inner side (+Z)
    for j, y in enumerate((-6.0, -2.5, 1.0, 4.5)):
        ext = rng.uniform(1.2, 3.0)
        rib = L.extrude_profile(P.coll, L.ibeam_profile(0.7, 0.4, 0.07, 0.09), SX + ext, segments=28, axis="X")
        rib.data.transform(T((ext) * 0.5, y, zmax + 0.3))
        P.add(rib, m["frame"], smooth=35, deform=lambda o, s=j: (L.jag_end(o, "X", +1, 0.5, s), bend_x(o)))
    for j, x in enumerate((-8.0, -1.5, 5.0)):
        st = L.extrude_profile(P.coll, L.ibeam_profile(0.45, 0.3, 0.06, 0.07), SY - 1.0, segments=10, axis="Y")
        st.data.transform(T(x, -0.5, zmax + 0.2))
        P.add(st, m["diag"], smooth=35, deform=lambda o, s=j: (L.jag_end(o, "Y", +1, 0.6, s + 4), bend_x(o)))
    pts = []
    for o in P.parts:
        pts.extend(o.matrix_world @ v.co for v in o.data.vertices)
    # two hulls: split along X so the curved section is not one fat box
    xs = sorted(p.x for p in pts)
    xm = xs[len(xs) // 2]
    P.hulls.append([p for p in pts if p.x <= xm + 0.5])
    P.hulls.append([p for p in pts if p.x >= xm - 0.5])
    # cables hanging out of the torn far edge
    ends = []
    for y in (-4.0, -0.5, 3.0):
        e = Vector((SX * 0.5 - 1.5, y, zmax + 0.3))      # the same bend as bend_x, applied to a point
        theta = (e.x + SX * 0.5 + 2.0) / Rb
        rr = Rb - e.z
        ends.append(Vector((-SX * 0.5 - 2.0 + rr * math.sin(theta), y, Rb - rr * math.cos(theta))))
    spill_cables(P, m, ends, rng, reach=5.0)
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=2.5,
                                 extra_manifest={"family": "station_truss"})


def build_truss03(asset, tag):
    """55 m junction: a hub module with three girders torn off at different lengths and a bent
    radiator panel."""
    P = L.Part(asset, "SpaceDressing")
    sc = 2.4
    m = truss_mats("Tr3", sc)
    m["rad"] = L.role_paint("Tr3_Radiator", "#D9DCDD", "#BFC4C6", metallic=0.4, rough=0.35, wear=0.4, grime=0.35,
                            scale=sc, seed=31, cells=("#D3D7DA", "#8E959C", (0.6, 3.4), ("X", "Z"), 0.03))
    rng = random.Random(9)
    hub = L.lathe(P.coll, [(-5.2, 0.0), (-5.2, 2.2), (-4.8, 3.6), (-4.2, 4.2), (4.2, 4.2), (4.8, 3.6), (5.2, 2.2), (5.2, 0.0)],
                  segments=24, axis="Z")
    P.add(hub, m["body"], smooth=35)
    P.hull_from([hub])
    for z0 in (-2.4, 1.8):
        band = L.lathe(P.coll, [(z0, 0.0), (z0, 4.3), (z0 + 0.6, 4.3), (z0 + 0.6, 0.0)], segments=24, axis="Z")
        P.add(band, m["accent"], smooth=35)
    for k in range(8):
        a = math.tau * k / 8 + 0.2
        port = L.cyl(P.coll, 0.4, 0.3, segments=12, axis="X", center=(4.15, 0, 0.6))
        P.add(port, m["glow_warm"] if k in (1, 4) else m["dark"], bevel=0.03, segments=1, matrix=Matrix.Rotation(a, 4, "Z"))
    cap = L.cyl(P.coll, 1.4, 0.8, segments=16, axis="Z", center=(0, 0, 5.4))
    P.add(cap, m["dark"], bevel=0.05, segments=1)
    greeble_cluster(P, m["dark"], (0.0, 0.0, 5.8), (0, 0, 1), 7, (0.3, 0.6), (0.2, 0.5), 0.6, seed=2)
    arms = [(0.0, 26.0, dict(lon_cut=[0.0, 1.8, 3.0, 0.6], kinks={1: (18, "X")}, torn_from=21.0, hang=True)),
            (125.0, 15.0, dict(lon_cut=[1.5, 0.0, 2.2, 0.8], kinks={3: (-20, "Z")}, torn_from=11.0, hang=False)),
            (235.0, 7.5, dict(lon_cut=[0.4, 1.4, 0.0, 2.0], kinks={}, torn_from=5.0, hang=False))]
    for i, (ang, length, dmg) in enumerate(arms):
        origin = Matrix.Rotation(math.radians(ang), 4, "Z") @ rot(-90, "Z") @ T(0, 3.6, 0)
        pts = truss(P, m, length, 3.0, 3.5, origin=origin, damage=dmg, seed=40 + i, lon=0.42, frame=0.3, diag=0.2)
        P.hulls.append(pts)
        # cables strapped along a longeron, spilling out where it is torn
        for cx, cz in ((1.38, 1.38), (-1.38, -1.38)):
            run = L.tube_path(P.coll, [origin @ Vector((cx, y, cz)) for y in (0.5, length * 0.5, length - 3.6)], 0.11, sides=7)
            P.add(run, m["cable"], smooth=50)
        spill_cables(P, m, [origin @ Vector((1.38, length - 3.6, 1.38)), origin @ Vector((-1.38, length - 3.6, -1.38))], rng,
                     reach=4.5 if i else 6.0)
        if i == 0:
            rad = L.grid_plate(P.coll, 9.0, 3.6, 12, 4, 0.12, jag=0.4, jag_sides=(0, 1, 0, 0), seed=12)
            rad.data.transform(T(4.5, 0, 0) @ rot(90, "X"))
            P.add(rad, m["rad"], bevel=0.015, segments=1, matrix=origin @ T(1.85, 13.0, 0.0) @ rot(-12, "Z"),
                  deform=lambda o: L.bend(o, along="X", radius=14.0, start=0.0, plane_axis="Y"))
            mount = L.box(P.coll, (0.7, 0.9, 1.8), center=(1.7, 13.0, 0.0))
            P.add(mount, m["dark"], bevel=0.03, segments=1, matrix=origin)
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=3.0,
                                 extra_manifest={"family": "station_truss"})


# =========================================================================== debris clusters

def build_debris01(asset, tag):
    """~15 m drifting tangle: two bent torn hull plates, a kinked I-beam, two pipes, clamps."""
    P = L.Part(asset, "SpaceDressing")
    sc = 1.2
    m = truss_mats("Deb1", sc)
    m["hullp"] = L.role_paint("Deb1_Hull", C["ivory"], C["ivory2"], metallic=0.25, rough=0.46, wear=0.6, fade=0.4,
                              grime=0.5, panel=(1.6, 0.025), scale=sc, seed=24, scorch=0.45,
                              label=(C["teal"], (-10, 10), (0.6, 1.4), ("X", "Y")))
    a = L.grid_plate(P.coll, 7.5, 4.6, 15, 9, 0.2, jag=0.7, seed=3, holes=0.18)
    P.add(a, m["hullp"], bevel=0.025, segments=1, matrix=T(0, 0, 0) @ rot(12, "Y"),
          deform=lambda o: L.bend(o, along="X", radius=6.5, start=-0.5, plane_axis="Z"))
    b = L.grid_plate(P.coll, 5.2, 3.6, 10, 7, 0.18, jag=0.6, seed=8, holes=0.15)
    P.add(b, m["accent"], bevel=0.025, segments=1, matrix=T(-2.2, 0.6, 1.0) @ rot(-55, "X") @ rot(30, "Z"),
          deform=lambda o: L.bend(o, along="Y", radius=4.0, start=0.0, plane_axis="Z"))
    beam = L.extrude_profile(P.coll, L.ibeam_profile(0.6, 0.4, 0.07, 0.09), 10.5, segments=21, axis="Y")
    P.add(beam, m["frame"], smooth=35, matrix=T(1.2, -0.5, 0.5) @ rot(25, "Z") @ rot(15, "X"),
          deform=lambda o: (L.twist_rotate_tail(o, "Y", 0.8, 38, "X"), L.jag_end(o, "Y", +1, 0.5, 2), L.jag_end(o, "Y", -1, 0.4, 3)))
    p1 = L.tube_path(P.coll, L.bezier_points((-3.5, -2.0, -0.4), (-1.0, -1.5, 0.6), (2.0, 1.0, 1.4), (4.5, 3.5, 3.0), 26), 0.2, sides=10)
    P.add(p1, m["steel"], smooth=40)
    p2 = L.tube_path(P.coll, L.bezier_points((-2.8, 1.8, -0.8), (-0.5, 0.5, -0.2), (1.5, -0.8, 1.6), (3.0, -3.5, 2.2), 24), 0.13, sides=8)
    P.add(p2, m["accent"], smooth=40)
    for i, (x, y, z) in enumerate(((-0.8, -1.3, 0.55), (1.6, 0.9, 1.25), (0.2, 0.3, 0.2))):
        cl = L.box(P.coll, (0.7, 0.45, 0.7), center=(x, y, z))
        P.add(cl, m["dark"], bevel=0.03, segments=1)
    rng = random.Random(4)
    spill_cables(P, m, [(1.2, -0.5, 0.5), (0.0, 0.0, 0.3)], rng, reach=3.5, radius=(0.05, 0.08))
    pts = []
    for o in P.parts:
        pts.extend(o.matrix_world @ v.co for v in o.data.vertices)
    P.hulls.append(pts)
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=2.4, post_scale=1.6,
                                 extra_manifest={"family": "debris_cluster"})


def build_debris02(asset, tag):
    """~28 m cluster: a two-bay girder chunk torn at both ends, a big bent plate, three crossing
    I-beams, a jammed tank and curling pipes."""
    P = L.Part(asset, "SpaceDressing")
    sc = 2.0
    m = truss_mats("Deb2", sc)
    m["tank"] = L.role_paint("Deb2_Tank", C["teal"], C["teal2"], metallic=0.35, rough=0.4, wear=0.6, fade=0.35,
                             grime=0.4, scale=sc, seed=41, stripes=None,
                             label=(C["ivory"], (-0.6, 0.6), (-0.5, 0.5), ("Y", "Z")))
    rng = random.Random(8)
    origin = T(-3.0, -6.0, 0.0) @ rot(20, "Z") @ rot(-12, "X")
    dmg = dict(lon_cut=[0.6, 2.2, 0.0, 1.4], kinks={1: (22, "X")}, torn_from=6.0, hang=True)
    pts = truss(P, m, 9.0, 3.0, 3.0, origin=origin, damage=dmg, seed=17, torn_start=True)
    plate = L.grid_plate(P.coll, 12.0, 6.5, 20, 11, 0.24, jag=0.8, seed=13, holes=0.2)
    P.add(plate, m["hullp"], bevel=0.03, segments=1, matrix=T(2.5, 2.0, 0.5) @ rot(-35, "Y") @ rot(10, "X"),
          deform=lambda o: L.bend(o, along="X", radius=9.0, start=-1.0, plane_axis="Z"))
    for i, (Lb, mtx, kink) in enumerate(((13.0, T(0.0, 1.0, 1.6) @ rot(70, "Z") @ rot(10, "Y"), 28),
                                         (9.5, T(4.0, -2.5, -0.8) @ rot(-20, "Z") @ rot(25, "X"), -34),
                                         (7.0, T(-4.5, 4.0, 1.0) @ rot(40, "Z") @ rot(-30, "Y"), 18))):
        beam = L.extrude_profile(P.coll, L.ibeam_profile(0.75, 0.5, 0.08, 0.11), Lb, segments=int(Lb * 2), axis="Y")
        P.add(beam, m["frame"] if i != 1 else m["diag"], smooth=35, matrix=mtx,
              deform=lambda o, k=kink, s=i: (L.twist_rotate_tail(o, "Y", 0.5, k, "X" if s % 2 else "Z"),
                                             L.jag_end(o, "Y", +1, 0.6, s), L.jag_end(o, "Y", -1, 0.5, s + 9)))
    tank = L.lathe(P.coll, [(-2.3, 0.0), (-2.25, 0.6), (-2.0, 1.05), (-1.6, 1.25), (1.6, 1.25), (2.0, 1.05), (2.25, 0.6), (2.3, 0.0)],
                   segments=20, axis="Y")
    P.add(tank, m["tank"], smooth=35, matrix=T(-1.5, -1.5, -0.4) @ rot(65, "Z") @ rot(20, "X"))
    for y0 in (-1.2, 1.0):
        band = L.lathe(P.coll, [(y0, 0.0), (y0, 1.3), (y0 + 0.25, 1.3), (y0 + 0.25, 0.0)], segments=20, axis="Y")
        P.add(band, m["dark"], smooth=35, matrix=T(-1.5, -1.5, -0.4) @ rot(65, "Z") @ rot(20, "X"))
    for i, ctrl in enumerate((((-6.0, -4.0, -1.0), (-2.0, -2.0, 2.0), (2.0, 3.0, -1.0), (7.0, 6.0, 2.5)),
                              ((-5.0, 3.0, 2.0), (-1.0, 0.0, -1.5), (3.0, -1.0, 3.0), (6.0, -5.0, 0.5)))):
        pipe = L.tube_path(P.coll, L.bezier_points(*ctrl, 30), 0.26 if i == 0 else 0.18, sides=10)
        P.add(pipe, m["steel"] if i == 0 else m["accent"], smooth=40)
    for (x, y, z) in ((-1.0, -1.0, 0.4), (1.8, 1.6, 0.4), (-3.0, 2.0, 1.2), (3.5, -2.6, 0.2)):
        cl = L.box(P.coll, (1.0, 0.7, 1.0), center=(x, y, z))
        P.add(cl, m["dark"], bevel=0.04, segments=1)
    spill_cables(P, m, [origin @ Vector((0.4, 6.0, 0.5)), origin @ Vector((-0.3, 5.5, -0.4))], rng, reach=5.0)
    allp = []
    for o in P.parts:
        allp.extend(o.matrix_world @ v.co for v in o.data.vertices)
    xs = sorted(p.x for p in allp)
    xm = xs[len(xs) // 2]
    P.hulls.append([p for p in allp if p.x <= xm + 0.5])
    P.hulls.append([p for p in allp if p.x >= xm - 0.5])
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=4.0, post_scale=1.85,
                                 extra_manifest={"family": "debris_cluster"})


# =========================================================================== portal gate ring

RI = 60.0     # inner radius of the frame (m): 120 m inner diameter


def build_gate(asset, tag):
    """Ornate gate frame, ring axis = Y (Unreal X after export): 8 segments, each with a gunmetal
    core spine, ivory armour plates (outer and front/back), gold trim rails, a node housing with a
    teal emitter lens facing the centre, front/back glow slots and an outward crown fin. One segment
    is modelled and UV-unwrapped, then repeated 8 times (the copies share UV space: 8x texel density)."""
    P = L.Part(asset, "SpaceDressing")
    sc = 5.0
    m = {}
    m["core"] = L.role_paint("Gate_Core", C["gunmetal"], C["gunmetal2"], metallic=0.55, rough=0.48, wear=0.5,
                             wear_color="#A3AAB2", grime=0.3, panel=(4.0, 0.06), scale=sc, seed=1)
    m["plate"] = L.role_paint("Gate_Plate", C["ivory"], C["ivory2"], metallic=0.3, rough=0.42, wear=0.45,
                              fade=0.2, grime=0.25, panel=(6.0, 0.07), scale=sc, seed=2)
    m["gold"] = L.role_paint("Gate_Gold", C["gold"], "#B8862E", metallic=0.9, rough=0.3, wear=0.25,
                             wear_color="#F2D896", grime=0.15, scale=sc, seed=3)
    m["dark"] = L.role_paint("Gate_Node", "#2A2E36", "#22252C", metallic=0.6, rough=0.5, wear=0.5, grime=0.3,
                             panel=(3.0, 0.05), scale=sc, seed=4)
    m["glow"] = L.role_glow("Gate_Glow", GLOW_TEAL, level=1.0)
    m["lip"] = L.role_glow("Gate_Lip", GLOW_TEAL, level=0.55)
    A0, A1 = 67.5, 112.5        # this segment's span, node at 90 deg (top, +Z)
    NH = 4.2                    # node housing half-span (deg)

    def rev(profile, a0, a1, steps, mat, bevel=None, seg=1):
        o = L.revolve_closed(P.coll, profile, (a0, a1), max(1, steps), axis="Y")
        P.add(o, mat, bevel=bevel, segments=seg, angle=30.0, smooth=None if bevel else 30.0)
        return o
    # core spine (overlaps its neighbours by 0.2 deg so the copies interlock without coincident caps)
    core = [(RI + 1.0, -2.0), (RI + 1.6, -2.8), (RI + 7.6, -2.8), (RI + 8.4, -2.0), (RI + 8.4, 2.0), (RI + 7.6, 2.8),
            (RI + 1.6, 2.8), (RI + 1.0, 2.0)]
    rev(core, A0 - 0.2, A1 + 0.2, 9, m["core"], bevel=0.12, seg=1)
    # inner glowing lip (framing the portal), set in a channel between two gold lips
    rev([(RI, -0.42), (RI + 1.1, -0.42), (RI + 1.1, 0.42), (RI, 0.42)], A0 - 0.2, A1 + 0.2, 9, m["lip"])
    for s in (1, -1):
        rev([(RI + 0.35, s * 0.42), (RI + 1.2, s * 0.42), (RI + 1.2, s * 1.0), (RI + 0.55, s * 1.0)], A0 - 0.2, A1 + 0.2, 9, m["gold"])
    # outer armour plates: small near the node, big toward the segment boundary (big/medium rhythm)
    outer = [(RI + 8.2, -3.2), (RI + 9.6, -3.2), (RI + 10.4, -2.4), (RI + 10.4, 2.4), (RI + 9.6, 3.2), (RI + 8.2, 3.2)]
    outer_hi = [(RI + 8.2, -3.0), (RI + 9.9, -3.0), (RI + 11.0, -2.0), (RI + 11.0, 2.0), (RI + 9.9, 3.0), (RI + 8.2, 3.0)]
    for lo_, hi_ in ((90 - NH - 6.2, 90 - NH - 0.4), (A0 + 0.3, 90 - NH - 6.6)):
        a, b = sorted((lo_, hi_))
        prof = outer_hi if (b - a) < 7 else outer
        rev(prof, a, b, max(2, int((b - a) / 3.0)), m["plate"], bevel=0.12, seg=1)
        a2, b2 = 180 - b, 180 - a
        rev(prof, a2, b2, max(2, int((b2 - a2) / 3.0)), m["plate"], bevel=0.12, seg=1)
    # gold trim rails along the outer face
    for s in (1, -1):
        rev([(RI + 10.2, s * 2.55), (RI + 10.75, s * 2.55), (RI + 10.75, s * 2.15), (RI + 10.2, s * 2.15)],
            A0 - 0.2, 90 - NH + 0.5, 6, m["gold"])
        rev([(RI + 10.2, s * 2.55), (RI + 10.75, s * 2.55), (RI + 10.75, s * 2.15), (RI + 10.2, s * 2.15)],
            90 + NH - 0.5, A1 + 0.2, 6, m["gold"])
    # front / back armour plates: trapezoids with slanted ends (angled seams)
    for s in (1, -1):
        side = [(RI + 1.8, s * 2.7), (RI + 7.4, s * 2.7), (RI + 6.9, s * 3.45), (RI + 2.3, s * 3.45)]
        for a, b in ((A0 + 0.4, 90 - NH - 0.6), (90 + NH + 0.6, A1 - 0.4)):
            rev(side, a, b, max(3, int((b - a) / 3.0)), m["plate"], bevel=0.08, seg=1)
    # node housing: a wedge block standing proud on all sides
    node = [(RI - 1.4, -3.4), (RI - 0.4, -4.6), (RI + 10.6, -4.6), (RI + 12.0, -3.4), (RI + 12.0, 3.4),
            (RI + 10.6, 4.6), (RI - 0.4, 4.6), (RI - 1.4, 3.4)]
    rev(node, 90 - NH, 90 + NH, 4, m["dark"], bevel=0.15, seg=1)
    for a, b in ((90 + NH - 0.2, 90 + NH + 1.6), (90 - NH - 1.6, 90 - NH + 0.2)):   # buttresses
        rev([(RI + 6.0, -3.8), (RI + 11.4, -3.2), (RI + 11.4, 3.2), (RI + 6.0, 3.8)], a, b, 2, m["dark"], bevel=0.1)
    # emitter lens facing the centre, in a gold bezel
    lens = L.lathe(P.coll, [(0.0, 2.3), (0.35, 2.2), (0.8, 1.75), (1.05, 1.0), (1.15, 0.0)], segments=14, axis="Z")
    P.add(lens, m["glow"], smooth=60, matrix=T(0, 0, RI - 1.25) @ rot(180, "X"))
    bez = L.lathe(P.coll, [(0.0, 2.3), (0.0, 3.0), (0.7, 3.0), (0.9, 2.6), (0.6, 2.3), (0.0, 2.3)], segments=14, axis="Z")
    P.add(bez, m["gold"], bevel=0.04, segments=1, matrix=T(0, 0, RI - 1.3) @ rot(180, "X"))
    # front/back glow slots (read as the 8 glowing nodes when the gate is approached head-on)
    for s in (1, -1):
        slot = L.box(P.coll, (0.9, 0.3, 7.0), center=(0, s * 4.6, RI + 5.0))
        P.add(slot, m["glow"], bevel=0.05, segments=1)
        frame = L.box(P.coll, (1.7, 0.36, 7.8), center=(0, s * 4.5, RI + 5.0))
        L.hs["inset_panels"](frame, normal=(0, s, 0), min_dot=0.9, fraction=1.0, thickness=0.35, depth=-0.25)
        P.add(frame, m["gold"], bevel=0.04, segments=1)
    # crown fin on the outer face
    blade = [(-3.8, RI + 11.5), (3.8, RI + 11.5), (2.2, RI + 15.5), (0.6, RI + 19.0), (-0.6, RI + 19.0), (-2.2, RI + 15.5)]
    fin = L.extrude_profile(P.coll, blade, 2.6, axis="X")
    P.add(fin, m["dark"], bevel=0.14, segments=1)
    finrim = L.extrude_profile(P.coll, [(-0.7, RI + 16.4), (0.7, RI + 16.4), (0.4, RI + 19.2), (-0.4, RI + 19.2)], 2.8, axis="X")
    P.add(finrim, m["gold"], bevel=0.06, segments=1)
    for sgn in (1, -1):
        side_blade = L.extrude_profile(P.coll, [(-2.4, RI + 10.7), (2.4, RI + 10.7), (0.4, RI + 15.2), (-0.4, RI + 15.2)], 1.2, axis="X")
        P.add(side_blade, m["gold"], bevel=0.08, segments=1,
              matrix=Matrix.Rotation(math.radians(sgn * 5.2), 4, "Y") @ T(0, 0, 0) @ rot(0, "Z"))
    # keystone at the seam between two segments (one per segment, the copies supply the rest)
    rev([(RI + 9.6, -3.0), (RI + 12.2, -2.4), (RI + 12.2, 2.4), (RI + 9.6, 3.0)], A0 - 1.1, A0 + 1.1, 2, m["dark"], bevel=0.1)
    rev([(RI + 12.0, -1.0), (RI + 13.4, -0.6), (RI + 13.4, 0.6), (RI + 12.0, 1.0)], A0 - 0.6, A0 + 0.6, 1, m["gold"], bevel=0.06)
    # conduits from node to plates (clustered detail around the node)
    for s in (1, -1):
        for yy in (-1.4,):
            pts = []
            for i in range(9):
                t = math.radians(90 + s * (NH - 0.8 + 6.4 * i / 8))
                pts.append((math.cos(t) * (RI + 11.25), yy, math.sin(t) * (RI + 11.25)))
            c = L.tube_path(P.coll, pts, 0.32, sides=6)
            P.add(c, m["gold"], smooth=45)
    # --- one segment -> UVs -> 8 copies
    seg = P.join(name=asset + "_seg")
    L.unwrap(seg, 2048)
    uv_info = {"unwrapped": "one segment, 8 rotated copies share it", "uv_segment": L.uv_stats(seg.data, 1024)}
    bm = bmesh.new()
    for k in range(8):
        me = seg.data.copy()
        me.transform(Matrix.Rotation(math.radians(45.0 * k), 4, "Y"))
        bm.from_mesh(me)
        bpy.data.meshes.remove(me)
    me = bpy.data.meshes.new(asset + "_ring")
    bm.to_mesh(me)
    bm.free()
    for mat in seg.data.materials:
        me.materials.append(mat)
    ring = bpy.data.objects.new(asset + "_ring", me)
    P.coll.objects.link(ring)
    L.remove_object(seg)
    # collision: 16 convex hulls, one per 22.5 deg arc of the frame
    pts = L.mesh_points(me)
    ang = (np.degrees(np.arctan2(pts[:, 2], pts[:, 0])) + 360.0) % 360.0
    for k in range(16):
        a0 = k * 22.5
        d = (ang - a0 + 360.0) % 360.0
        sel = pts[(d <= 23.0) | (d >= 359.5)]
        P.hulls.append([Vector(p) for p in sel])
    P.parts = [ring]
    return L.finish_hard_surface(P, asset, 2048, tag, (2000, 25000), ao_distance=5.0, overlap_uv=uv_info,
                                 extra_manifest={"family": "gate_ring", "inner_diameter_cm": RI * 200.0,
                                                 "ring_axis_unreal": "X (the gate is flown through along X)",
                                                 "scaling": "uniform scale s gives an inner diameter of 12000*s cm"})


BUILDERS = {
    "SM_Derelict_Satellite_01": build_sat01,
    "SM_Derelict_Satellite_02": build_sat02,
    "SM_CargoPod_01": build_pod01,
    "SM_CargoPod_02": build_pod02,
    "SM_CargoPod_03": build_pod03,
    "SM_BeaconBuoy_01": build_buoy,
    "SM_StationTruss_01": build_truss01,
    "SM_StationTruss_02": build_truss02,
    "SM_StationTruss_03": build_truss03,
    "SM_DebrisCluster_01": build_debris01,
    "SM_DebrisCluster_02": build_debris02,
    "SM_GateRing_01": build_gate,
}


def build(asset, tag):
    return BUILDERS[asset](asset, tag)
