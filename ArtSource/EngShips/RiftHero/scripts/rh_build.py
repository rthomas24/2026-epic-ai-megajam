# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero: clean rebuild of the Rift over Grok Bot's Rodin mesh (the 3D reference, read-only).
#   blender --background --factory-startup --python rh_build.py -- --stage 1|2|3 [--tag v01] [--no-review]
# Stage 1 primary masses, 2 secondary forms, 3 tertiary detail (panel lines, vents, hatches).
# Frame: metres, nose +X, up +Z, Blender Y = -Unreal Y. Origin = bounding-box centre of the Rodin mesh.
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C
import rh_geo as G

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(flag, default=None):
    return ARGS[ARGS.index(flag) + 1] if flag in ARGS else default


STAGE = int(arg("--stage", "3"))
TAG = arg("--tag", "v%02d" % STAGE)
PI = math.pi
HULL, ACCENT, MECH, GLASS, GLOW = range(5)


def materials():
    return [
        G.mat("M_Rift_Hull", (0.80, 0.81, 0.84), metallic=0.10, roughness=0.36),
        G.mat("M_Rift_Accent", (0.85, 0.04, 0.50), metallic=0.15, roughness=0.30),
        G.mat("M_Rift_Mech", (0.045, 0.048, 0.06), metallic=0.85, roughness=0.42),
        G.mat("M_Rift_Glass", (0.02, 0.015, 0.05), metallic=0.0, roughness=0.06,
              emission=(0.6, 0.1, 0.9), strength=0.15),
        G.mat("M_Rift_Glow", (0.9, 0.1, 0.7), roughness=0.5, emission=(1.0, 0.12, 0.78), strength=3.0),
    ]


# =============================================================================== hull
# x, z_top, z_bottom, half width, z of the widest point, top exponent, bottom exponent (cm; fitted to the
# Rodin centre-line and half-width measurements in analysis/, smoothed)
HULL_ST = np.array([
    (233.0, -45.8, -47.2, 0.6, -46.5, 2.0, 2.0),
    (231.0, -43.5, -50.0, 3.6, -46.6, 2.0, 2.0),
    (226.0, -41.0, -55.0, 7.5, -47.2, 2.0, 2.0),
    (212.0, -37.0, -66.0, 15.0, -50.0, 2.4, 2.3),
    (192.0, -31.0, -71.0, 26.0, -50.0, 2.7, 2.4),
    (168.0, -25.0, -73.0, 38.0, -48.0, 2.9, 2.4),
    (150.0, -19.0, -75.0, 45.0, -46.0, 3.0, 2.4),
    (120.0, -8.0, -77.0, 51.0, -44.0, 3.0, 2.4),
    (96.0, 0.0, -79.0, 53.0, -43.0, 3.0, 2.4),
    (72.0, 6.0, -80.0, 54.0, -42.0, 3.0, 2.4),
    (48.0, 10.0, -83.0, 55.0, -42.0, 3.0, 2.4),
    (24.0, 13.0, -88.0, 56.0, -42.0, 3.0, 2.3),
    (0.0, 14.0, -91.0, 57.0, -42.0, 3.0, 2.3),
    (-48.0, 15.0, -93.0, 58.0, -42.0, 3.0, 2.3),
    (-96.0, 15.0, -93.0, 60.0, -40.0, 3.0, 2.3),
    (-130.0, 10.0, -89.0, 66.0, -38.0, 3.0, 2.5),
    (-155.0, 4.0, -84.0, 76.0, -37.0, 3.2, 2.8),
    (-180.0, -2.0, -78.0, 84.0, -36.0, 3.8, 3.4),
    (-205.0, -6.0, -73.0, 86.0, -36.0, 4.2, 4.0),
    (-228.0, -8.0, -70.0, 85.0, -36.0, 4.5, 4.5),
])[::-1]
HX = HULL_ST[:, 0] / 100.0


CHINE = 0.03


def hull_params(x):
    p = G.pchip(HX, HULL_ST[:, 1:], x)
    return p[0] / 100, p[1] / 100, p[2] / 100, p[3] / 100, p[4], p[5]


def hull_f(th, x):
    zt, zb, hw, zw, nt, nb = hull_params(x)
    c, s = math.cos(th), math.sin(th)
    if s > 1e-9:                     # upper shell sits CHINE inside the lower one: a hard chine line down each side
        ch = CHINE * min(1.0, hw / 0.30)
        return (x, (hw - ch) * G.spow(c, nt), zw + ch * 0.6 + (zt - zw - ch * 0.6) * G.spow(s, nt))
    return (x, hw * G.spow(c, nb), zw + (zw - zb) * G.spow(s, nb))


def hull_top(x, y=0.0):
    zt, zb, hw, zw, nt, nb = hull_params(x)
    t = min(abs(y) / hw, 0.999)
    return zw + (zt - zw) * (1 - t ** nt) ** (1.0 / nt)


def hull_side(x, z):
    """Half width of the hull at height z."""
    zt, zb, hw, zw, nt, nb = hull_params(x)
    if z >= zw:
        t, n = min((z - zw) / (zt - zw), 0.999), nt
    else:
        t, n = min((zw - z) / (zw - zb), 0.999), nb
    return hw * (1 - t ** n) ** (1.0 / n)


def sym(pockets):
    """Mirror hull pockets given on the +Y side (theta in (-pi/2, pi/2)) to the other side."""
    out = []
    for p in pockets:
        out.append(p)
        if not p.get("centre"):
            q = dict(p)
            q["u0"], q["u1"] = PI - p["u1"], PI - p["u0"]
            out.append(q)
    return out


def pk(u0, u1, v0, v1, depth=0.008, mat=None, wall=0.004, r=0.5, centre=False, wall_mat=None):
    """Pocket on a surface whose u is an angle on a ~r metre radius and v is metres."""
    d = {"u0": u0, "u1": u1, "v0": v0, "v1": v1, "depth": depth, "eu": wall / r, "ev": wall, "centre": centre}
    if mat is not None:
        d["mat"] = mat
    if wall_mat is not None:
        d["wall_mat"] = wall_mat
    return d


def line_v(x, u0, u1, w=0.012, depth=0.007, r=0.5, centre=False):
    """Circumferential panel line at station x between angles u0..u1."""
    return pk(u0, u1, x - w / 2, x + w / 2, depth, MECH, wall=0.003, r=r, centre=centre)


def line_u(u, x0, x1, w=0.012, depth=0.007, r=0.5, centre=False):
    """Longitudinal panel line at angle u between stations x0..x1."""
    return pk(u - w / 2 / r, u + w / 2 / r, x0, x1, depth, MECH, wall=0.003, r=r, centre=centre)


def build_hull(mats, detail):
    us = sorted([-PI / 2 + 2 * PI * k / 56 for k in range(56)] + [0.02, PI - 0.02])
    vs = list(np.linspace(-2.28, 2.33, 78))
    pockets = []
    if detail:
        d = math.radians
        # circumferential seams (upper hull, wing to wing over the back)
        for x, a0 in ((1.86, -60), (1.52, -20), (0.30, 12), (-0.42, 12), (-1.12, 14), (-1.47, -8), (-2.02, -62)):
            pockets.append(line_v(x, d(a0), PI - d(a0), centre=True))
        # belly seams
        for x in (1.20, 0.20, -0.80):
            pockets.append(line_v(x, d(-80), d(-38)))
        pockets.append(line_v(-2.02, d(-90) + 0.001, d(-62) - 0.03))
        # longitudinal seams: shoulder line and chine line on each side
        pockets.append(line_u(d(50), -1.12, 0.30))
        pockets.append(line_u(d(33), 0.30, 1.52))
        pockets.append(line_u(d(-50), -0.80, 1.20))
        pockets.append(line_u(d(24), -2.02, -1.47))
        # magenta dorsal accent panels (recessed 4 mm, real edges)
        pockets.append(pk(d(56), d(82), -1.06, -0.48, 0.004, ACCENT, wall=0.006))
        pockets.append(pk(d(56), d(82), -0.36, 0.24, 0.004, ACCENT, wall=0.006))
        pockets.append(pk(d(40), d(62), 1.58, 1.80, 0.004, ACCENT, wall=0.006))
        # engine block: magenta shoulder band and side heat vents (dark, deep)
        pockets.append(pk(d(30), d(58), -1.96, -1.53, 0.004, ACCENT, wall=0.006))
        for k in range(5):
            x0 = -2.22 + k * 0.085
            pockets.append(pk(d(-22), d(10), x0, x0 + 0.05, 0.03, MECH, wall=0.008, wall_mat=MECH))
        # service hatches low on the flanks, behind the cockpit
        pockets.append(pk(d(-30), d(-12), -0.62, -0.28, 0.005, HULL, wall=0.005, wall_mat=MECH))
        pockets.append(pk(d(-30), d(-14), 0.70, 0.98, 0.005, HULL, wall=0.005, wall_mat=MECH))
        # belly: dark keel plate with cooling slots
        pockets.append(pk(d(-90) + 0.002, d(-66), -1.90, -0.95, 0.006, MECH, wall=0.006))
        pockets.append(pk(d(-90) + 0.002, d(-72), -0.60, 0.05, 0.006, MECH, wall=0.006))
        pockets = sym(pockets)

    hull = G.grid_surface("SM_Rift", hull_f, us, vs, mats, pockets=pockets, cap0="ngon", cap1="point")
    return hull


# =============================================================================== engine shroud
def shroud_f(th, t):
    xr = -2.31
    xf = -1.50 + 0.20 * max(0.0, math.sin(th)) ** 1.5 - 0.10 * max(0.0, -math.sin(th))     # raked front edge
    x = xr + t * (xf - xr)
    zt, zb, hw, zw, nt, nb = hull_params(max(x, -2.28))
    off = 0.055 * min(1.0, (1.0 - t) / 0.06) ** 0.5 - 0.015          # chamfered front lip, ends inside the hull
    nt, nb = max(nt, 4.2), max(nb, 4.2)
    c, s = math.cos(th), math.sin(th)
    hw2, zt2, zb2 = hw + off, zt + off, zb - off * 0.8
    if s >= 0:
        return (x, hw2 * G.spow(c, nt), zw + (zt2 - zw) * G.spow(s, nt))
    return (x, hw2 * G.spow(c, nb), zw + (zw - zb2) * G.spow(s, nb))


def build_shroud(mats, detail):
    us = [-PI / 2 + 2 * PI * k / 56 for k in range(56)]
    vs = sorted(list(np.linspace(0.0, 1.0, 22)) + [0.955, 0.985])
    pockets = []
    if detail:
        d = math.radians
        ps = []
        ps.append({"u0": d(34), "u1": d(62), "v0": 0.30, "v1": 0.80, "depth": 0.004, "eu": 0.012, "ev": 0.012,
                   "mat": ACCENT})
        for k in range(5):                                          # side heat vents
            t0 = 0.10 + k * 0.105
            ps.append({"u0": d(-24), "u1": d(8), "v0": t0, "v1": t0 + 0.06, "depth": 0.03, "eu": 0.016,
                       "ev": 0.012, "mat": MECH})
        ps.append({"u0": d(12), "u1": d(168), "v0": 0.86, "v1": 0.875, "depth": 0.007, "eu": 0.006, "ev": 0.004,
                   "mat": MECH, "centre": True})
        ps.append({"u0": d(72), "u1": d(108), "v0": 0.06, "v1": 0.40, "depth": 0.006, "eu": 0.012, "ev": 0.012,
                   "mat": MECH, "centre": True})
        pockets = sym(ps)
    sh = G.grid_surface("SM_Rift_Shroud", shroud_f, us, vs, mats, pockets=pockets, cap0="ngon", cap1="ngon",
                        mat_fn=lambda u, v: HULL)
    bm = bmesh.new()
    bm.from_mesh(sh.data)
    rear = [f for f in bm.faces if len(f.verts) > 8 and f.normal.x < -0.9]
    if rear:
        bmesh.ops.inset_region(bm, faces=rear, thickness=0.04, depth=0.0, use_even_offset=True)
        r2 = bmesh.ops.inset_region(bm, faces=rear, thickness=0.012, depth=-0.04, use_even_offset=True)
        for f in rear + r2["faces"]:
            f.material_index = MECH
    bm.to_mesh(sh.data)
    bm.free()
    G.bevel(sh, width=0.006, segments=2, angle_deg=35)
    return sh


# =============================================================================== wings
# plan-view rails (cm): outer edge O and inner (concave, forward) edge I, mid-height z and thickness
WING_ST = np.array([
    # Ox,   Oy,   Ix,   Iy,    zc,   th
    (-127, 46, 26, 46, -44.0, 34.0),
    (-127, 75, 22, 75, -45.0, 32.0),
    (-124, 105, 20, 101, -47.5, 28.0),
    (-105, 132, 23, 122, -52.0, 25.0),
    (-72, 158, 33, 138, -57.0, 22.5),
    (-30, 180, 50, 150, -62.5, 20.5),
    (22, 198, 72, 159, -69.0, 18.5),
    (80, 207, 101, 167, -77.0, 16.0),
    (135, 206.5, 135, 173, -88.0, 13.0),
    (182, 197, 172, 176, -98.5, 9.5),
    (217, 186, 208, 177, -105.5, 6.0),
    (240, 176.6, 239, 175.4, -109.5, 1.6),
], float) / 100.0
WS = np.linspace(0.0, 1.0, len(WING_ST))


def wing_f(u, s):
    ox, oy, ix, iy, zc, th = G.pchip(WS, WING_ST, s)
    c, sn = math.cos(u), math.sin(u)
    mx, my = (ox + ix) / 2, (oy + iy) / 2
    hx, hy = (ox - ix) / 2, (oy - iy) / 2
    z = zc + th / 2 * math.copysign(abs(sn) ** 1.25, sn)
    return (mx + hx * c, my + hy * c, z)


def build_wing(mats, detail):
    us = [2 * PI * k / 32 for k in range(32)]
    vs = list(np.linspace(0.0, 1.0, 50))
    pockets = []
    if detail:
        e = 0.012
        def wp(u0, u1, s0, s1, depth, m, wall=0.012):
            return {"u0": u0, "u1": u1, "v0": s0, "v1": s1, "depth": depth, "eu": wall * 1.6, "ev": wall * 0.42,
                    "mat": m}
        # top surface (u 0..pi): white root shoulder, magenta blade, dark seam between; u = 0 is the outer edge
        pockets.append(wp(0.30, PI - 0.30, 0.215, 0.228, 0.007, MECH, wall=0.004))          # root seam
        pockets.append(wp(0.30, PI - 0.30, 0.56, 0.570, 0.006, MECH, wall=0.004))           # mid-span seam
        pockets.append(wp(0.42, PI - 0.42, 0.25, 0.545, 0.004, ACCENT))                     # inner blade panel
        pockets.append(wp(0.42, PI - 0.42, 0.585, 0.90, 0.004, ACCENT))                     # outer blade panel
        # underside
        pockets.append(wp(PI + 0.30, 2 * PI - 0.30, 0.215, 0.228, 0.007, MECH, wall=0.004))
        pockets.append(wp(PI + 0.42, 2 * PI - 0.42, 0.25, 0.90, 0.004, ACCENT))
        # trailing-edge heat vents near the root (outer / rear edge), top surface
        for k in range(4):
            s0 = 0.085 + k * 0.03
            pockets.append(wp(0.42, 0.95, s0, s0 + 0.016, 0.022, MECH, wall=0.006))
    wing = G.grid_surface("SM_Rift_Wings", wing_f, us, vs, mats, pockets=pockets, cap0="ngon", cap1="point",
                          mat_fn=lambda u, v: HULL)
    return G.mirror_y(wing)


# =============================================================================== ring (portal hoop)
RING_C = (-1.70, 0.0, 0.55)


def build_ring(mats, detail):
    prof = [(-0.12, 0.455), (-0.12, 0.515), (-0.106, 0.515), (-0.106, 0.537), (-0.12, 0.537), (-0.12, 0.58),
            (-0.022, 0.58), (-0.022, 0.560), (0.022, 0.560), (0.022, 0.58), (0.12, 0.58), (0.12, 0.537),
            (0.106, 0.537), (0.106, 0.515), (0.12, 0.515), (0.12, 0.455), (0.04, 0.455), (0.04, 0.474),
            (-0.04, 0.474), (-0.04, 0.455)]
    #        0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15
    idx = [HULL, MECH, GLOW, MECH, HULL, HULL, MECH, GLOW, MECH, HULL, HULL, MECH, GLOW, MECH, HULL,
           HULL, MECH, GLOW, MECH, HULL]
    ring = G.lathe_x("SM_Rift_Ring", prof, RING_C, mats, segments=96, mat_idx=idx)
    G.bevel(ring, width=0.006, segments=2)
    return ring


def build_portal_disc(mats_portal):
    """Flat disc inside the hoop, its own asset / material slot, radial UVs (centre 0.5, 0.5; rim on the unit
    circle of radius 0.5)."""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    R, seg, rings = 0.462, 64, 6
    cx, cy, cz = RING_C
    centre = bm.verts.new((cx, cy, cz))
    vr = [[bm.verts.new((cx, cy + R * (k / rings) * math.cos(2 * PI * s / seg),
                         cz + R * (k / rings) * math.sin(2 * PI * s / seg))) for s in range(seg)]
          for k in range(1, rings + 1)]

    def uv(v):
        return (0.5 + 0.5 * (v.co.y - cy) / R, 0.5 + 0.5 * (v.co.z - cz) / R)
    for s in range(seg):
        s1 = (s + 1) % seg
        f = bm.faces.new((centre, vr[0][s], vr[0][s1]))
        for l in f.loops:
            l[uvl].uv = uv(l.vert)
        for k in range(rings - 1):
            f = bm.faces.new((vr[k][s], vr[k + 1][s], vr[k + 1][s1], vr[k][s1]))
            for l in f.loops:
                l[uvl].uv = uv(l.vert)
    for f in bm.faces:
        f.normal_update()
    bm.faces.ensure_lookup_table()
    if bm.faces[0].normal.x > 0:          # face the chase camera (-X); the material is two-sided anyway
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    return G._finish("SM_Rift_PortalDisc", bm, mats_portal, 30.0)


# =============================================================================== engines
def nozzle_profile(R, x_exit=-2.50, deep=True):
    X = x_exit
    p = [(X + 0.30, 0.0), (X + 0.30, R * 1.00), (X + 0.215, R * 1.08), (X + 0.185, R * 1.08), (X + 0.185, R * 0.97),
         (X + 0.17, R * 0.97), (X + 0.08, R * 1.02), (X + 0.02, R * 0.985), (X, R * 0.95),
         (X, R * 0.86), (X + 0.10, R * 0.80), (X + 0.17, R * 0.66), (X + 0.20, R * 0.66),
         (X + 0.20, R * 0.50), (X + 0.245, R * 0.50), (X + 0.245, R * 0.22), (X + 0.14, 0.0)]
    m = [MECH, HULL, ACCENT, MECH, MECH, MECH, MECH, MECH,
         MECH, MECH, MECH, GLOW, MECH, GLOW, GLOW, MECH, MECH]
    return p, m


def build_engines(mats, detail):
    objs = []
    p, m = nozzle_profile(0.265)
    c = G.lathe_x("SM_Rift_NozzleC", p, (0.0, 0.0, -0.34), mats, segments=48, mat_idx=m)
    p, m = nozzle_profile(0.195)
    o = G.lathe_x("SM_Rift_NozzleO", p, (0.0, 0.64, -0.34), mats, segments=40, mat_idx=m)
    G.mirror_y(o)
    for ob in (c, o):
        G.bevel(ob, width=0.005, segments=2)
        objs.append(ob)
    return objs


# =============================================================================== fins
def fin_f(u, t):
    root_a, root_b = Vector((-1.99, 0.60, -0.16)), Vector((-1.36, 0.60, -0.16))
    tip_a, tip_b = Vector((-2.06, 0.985, 0.77)), Vector((-1.80, 0.985, 0.77))
    tt = t ** 0.9
    a, b = root_a.lerp(tip_a, tt), root_b.lerp(tip_b, tt)
    n = Vector((0.0, 0.925, -0.38)).normalized()
    th = 0.15 * (1 - t) + 0.06 * t
    c, s = math.cos(u), math.sin(u)
    p = (a + b) / 2 + (b - a) / 2 * c + n * (th / 2) * math.copysign(abs(s) ** 1.3, s)
    return tuple(p)


def build_fins(mats, detail):
    us = [2 * PI * k / 20 for k in range(20)]
    vs = list(np.linspace(0.0, 1.0, 14))
    pockets = []
    if detail:
        for a, b in ((0.5, PI - 0.5), (PI + 0.5, 2 * PI - 0.5)):
            pockets.append({"u0": a, "u1": b, "v0": 0.30, "v1": 0.90, "depth": 0.004, "eu": 0.03, "ev": 0.012,
                            "mat": ACCENT})
            pockets.append({"u0": a - 0.15, "u1": b + 0.15, "v0": 0.235, "v1": 0.25, "depth": 0.006, "eu": 0.012,
                            "ev": 0.004, "mat": MECH})
    fin = G.grid_surface("SM_Rift_Fins", fin_f, us, vs, mats, pockets=pockets, cap0="ngon", cap1="ngon",
                         mat_fn=lambda u, v: HULL)
    G.mirror_y(fin)
    G.bevel(fin, width=0.004, segments=2, angle_deg=40)
    return fin


# =============================================================================== canopy
CAN_ST = np.array([(0.34, 0.03, 0.012), (0.42, 0.17, 0.085), (0.60, 0.265, 0.165), (0.85, 0.30, 0.20),
                   (1.10, 0.29, 0.172), (1.32, 0.235, 0.112), (1.46, 0.14, 0.05), (1.53, 0.02, 0.008)])


def canopy_f(th, x):
    hw, h = G.pchip(CAN_ST[:, 0], CAN_ST[:, 1:], x)
    zd = hull_top(x) - 0.055
    c, s = math.cos(th), math.sin(th)
    if s >= 0:
        return (x, hw * G.spow(c, 2.3), zd + h * G.spow(s, 2.3))
    return (x, hw * c, zd + 0.05 * s)


def build_canopy(mats, detail):
    us = [2 * PI * k / 40 for k in range(40)]
    vs = list(np.linspace(0.34, 1.53, 34))
    pockets = []
    for a, b in ((0.16, PI / 2 - 0.085), (PI / 2 + 0.085, PI - 0.16)):
        for x0, x1 in ((0.50, 0.88), (0.945, 1.40)):
            pockets.append({"u0": a, "u1": b, "v0": x0, "v1": x1, "depth": 0.007, "eu": 0.022, "ev": 0.006,
                            "mat": GLASS, "wall_mat": MECH})
    return G.grid_surface("SM_Rift_Canopy", canopy_f, us, vs, mats, pockets=pockets, cap0="point", cap1="point",
                          mat_fn=lambda u, v: MECH, smooth_deg=40)


# =============================================================================== guns
GUN_C = (0.37, -0.60)
GUN_ST = np.array([(1.12, 0.012), (1.22, 0.062), (1.45, 0.102), (1.75, 0.104), (1.93, 0.088), (2.03, 0.070)])


def gun_f(th, x):
    r = float(G.pchip(GUN_ST[:, 0], GUN_ST[:, 1], x))
    return (x, GUN_C[0] + r * 0.92 * G.spow(math.cos(th), 2.6), GUN_C[1] + r * 1.08 * G.spow(math.sin(th), 2.6))


def build_guns(mats, detail):
    us = [2 * PI * k / 24 for k in range(24)]
    vs = list(np.linspace(1.12, 2.03, 20))
    pockets = []
    if detail:
        pockets.append({"u0": -0.9, "u1": 0.9, "v0": 1.50, "v1": 1.512, "depth": 0.006, "eu": 0.03, "ev": 0.003,
                        "mat": MECH})
        pockets.append({"u0": -0.7, "u1": 0.7, "v0": 1.56, "v1": 1.86, "depth": 0.004, "eu": 0.05, "ev": 0.006,
                        "mat": ACCENT})
        us = [-PI + 2 * PI * k / 24 for k in range(24)]
    pod = G.grid_surface("SM_Rift_GunPods", gun_f, us, vs, mats, pockets=pockets, cap0="point", cap1="ngon",
                         mat_fn=lambda u, v: HULL)
    prof = [(1.98, 0.0), (1.98, 0.052), (2.085, 0.052), (2.10, 0.040), (2.34, 0.040), (2.34, 0.058), (2.37, 0.058),
            (2.37, 0.047), (2.40, 0.047), (2.40, 0.058), (2.43, 0.058), (2.43, 0.047), (2.46, 0.047), (2.46, 0.058),
            (2.50, 0.058), (2.50, 0.030), (2.36, 0.030), (2.36, 0.0)]
    idx = [MECH] * len(prof)
    idx[15] = GLOW
    idx[16] = GLOW
    barrel = G.lathe_x("SM_Rift_GunBarrels", prof, (0.0, GUN_C[0], GUN_C[1]), mats, segments=20, mat_idx=idx)
    for ob in (pod, barrel):
        G.mirror_y(ob)
    G.bevel(barrel, width=0.003, segments=2)
    G.bevel(pod, width=0.004, segments=2)
    return [pod, barrel]


# =============================================================================== intakes, saddle, spine
def intake_f(th, x):
    t = (x - (-0.55)) / (0.66 - (-0.55))                    # 0 rear .. 1 mouth
    g = min(1.0, t / 0.55) ** 0.8
    hw, hh = 0.02 + 0.15 * g, 0.02 + 0.115 * g
    cy = 0.655 - 0.03 * (1 - g)
    cz = -0.40 - 0.02 * (1 - g)
    return (x, cy + hw * G.spow(math.cos(th), 4.0), cz + hh * G.spow(math.sin(th), 4.0))


def build_intakes(mats, detail):
    us = [2 * PI * k / 24 for k in range(24)]
    vs = list(np.linspace(-0.55, 0.66, 20))
    pockets = []
    if detail:
        pockets.append({"u0": 0.5, "u1": PI - 0.5, "v0": 0.50, "v1": 0.512, "depth": 0.006, "eu": 0.03, "ev": 0.003,
                        "mat": MECH})
        pockets.append({"u0": 0.6, "u1": PI - 0.6, "v0": -0.05, "v1": 0.44, "depth": 0.004, "eu": 0.05, "ev": 0.008,
                        "mat": ACCENT})
    ob = G.grid_surface("SM_Rift_Intakes", intake_f, us, vs, mats, pockets=pockets, cap0="point", cap1="ngon",
                        mat_fn=lambda u, v: HULL)
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    mouth = [f for f in bm.faces if len(f.verts) > 8]
    bmesh.ops.inset_region(bm, faces=mouth, thickness=0.018, depth=0.0, use_even_offset=True)
    r = bmesh.ops.inset_region(bm, faces=mouth, thickness=0.004, depth=-0.10, use_even_offset=True)
    for f in mouth + r["faces"]:
        f.material_index = MECH
    for f in mouth:
        f.material_index = GLOW if False else MECH
    bm.to_mesh(ob.data)
    bm.free()
    G.mirror_y(ob)
    G.bevel(ob, width=0.004, segments=2)
    objs = [ob]
    # three vanes in each mouth
    vanes = None
    for k in range(3):
        y = 0.655 - 0.075 + 0.075 * k
        b = G.box("SM_Rift_IntakeVanes" if k == 0 else "tmp_vane", (0.50, y - 0.006, -0.50), (0.635, y + 0.006, -0.30),
                  mats, MECH)
        if vanes is None:
            vanes = b
        else:
            join([vanes, b])
    G.mirror_y(vanes)
    G.bevel(vanes, width=0.002, segments=1)
    objs.append(vanes)
    return objs


def saddle_f(th, x):
    t = (x + 1.70) / 0.26
    g = max(0.0, 1 - abs(t) ** 2.6) ** 0.45
    hw, top, bot = 0.20 + 0.25 * g, -0.02 + 0.165 * g, -0.12
    c, s = math.cos(th), math.sin(th)
    zc = (top + bot) / 2
    return (x, hw * G.spow(c, 3.4), zc + (top - bot) / 2 * G.spow(s, 3.4))


def spine_f(th, x):
    k = np.array([(-1.62, 0.15, 0.215), (-1.40, 0.14, 0.262), (-1.05, 0.125, 0.268), (-0.70, 0.10, 0.225),
                  (-0.42, 0.07, 0.175), (-0.26, 0.02, 0.14)])
    hw, top = G.pchip(k[:, 0], k[:, 1:], x)
    bot = 0.02
    c, s = math.cos(th), math.sin(th)
    zc = (top + bot) / 2
    return (x, hw * G.spow(c, 2.8), zc + (top - bot) / 2 * G.spow(s, 2.8))


def build_saddle_spine(mats, detail):
    us = [-PI / 2 + 2 * PI * k / 32 for k in range(32)]
    sad = G.grid_surface("SM_Rift_Saddle", saddle_f, us, list(np.linspace(-1.955, -1.445, 22)), mats,
                         cap0="point", cap1="point", mat_fn=lambda u, v: MECH)
    pockets = []
    if detail:
        for k in range(6):                                    # dorsal louvres on the spine
            x0 = -1.30 + k * 0.07
            pockets.append({"u0": 0.55, "u1": 1.15, "v0": x0, "v1": x0 + 0.035, "depth": 0.018, "eu": 0.05,
                            "ev": 0.006, "mat": MECH})
            pockets.append({"u0": PI - 1.15, "u1": PI - 0.55, "v0": x0, "v1": x0 + 0.035, "depth": 0.018,
                            "eu": 0.05, "ev": 0.006, "mat": MECH})
        pockets.append({"u0": 1.2, "u1": PI - 1.2, "v0": -1.36, "v1": -0.50, "depth": 0.004, "eu": 0.04,
                        "ev": 0.008, "mat": ACCENT})
    spine = G.grid_surface("SM_Rift_Spine", spine_f, us, list(np.linspace(-1.62, -0.26, 30)), mats, pockets=pockets,
                           cap0="ngon", cap1="point", mat_fn=lambda u, v: HULL)
    return [sad, spine]


# =============================================================================== helpers
def join(objs):
    target = objs[0]
    with bpy.context.temp_override(active_object=target, object=target, selected_objects=objs,
                                   selected_editable_objects=objs):
        bpy.ops.object.join()
    return target


SOCKETS_UE = {           # Unreal cm (+X forward, +Y right, +Z up); old = the ship builder's current export
    "Muzzle_0": ((252, -37, -60), (252, -37, -60)), "Muzzle_1": ((252, 37, -60), (252, 37, -60)),
    "Engine": ((-252, 0, -34), (-252, 0, -34)),
    "Engine_0": ((-252, -64, -34), (-252, -64, -34)), "Engine_1": ((-252, 64, -34), (-252, 64, -34)),
    "Portal": ((-170, 0, 55), (-170, 0, 55)), "Cockpit": ((60, 0, 0), (60, 0, 0)),
    "WingTip_L": ((238, -176, -109), (240, -176, -109.5)), "WingTip_R": ((238, 176, -109), (240, 176, -109.5)),
}


def add_sockets(root):
    for name, (old, new) in SOCKETS_UE.items():
        e = bpy.data.objects.new("SOCKET_" + name, None)
        e.empty_display_size = 0.1
        e.location = (new[0] / 100.0, -new[1] / 100.0, new[2] / 100.0)
        e.parent = root
        G.collection().objects.link(e)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mats = materials()
    detail = STAGE >= 3
    hull = build_hull(mats, detail)
    parts = [build_wing(mats, detail), build_ring(mats, detail), build_fins(mats, detail), build_shroud(mats, detail)]
    parts += build_engines(mats, detail)
    if STAGE >= 2:
        parts.append(build_canopy(mats, detail))
        parts += build_guns(mats, detail)
        parts += build_intakes(mats, detail)
        parts += build_saddle_spine(mats, detail)
    for p in parts:
        p.parent = hull
    add_sockets(hull)
    disc = None
    if STAGE >= 2:
        disc = build_portal_disc([G.mat("M_Rift_Portal", (0.25, 0.02, 0.35), roughness=0.3,
                                        emission=(0.8, 0.1, 1.0), strength=1.5)])
    bpy.context.view_layer.update()
    names = [hull.name] + [p.name for p in parts]
    blend = os.path.join(C.ROOT, "Rift_%s.blend" % TAG)
    bpy.ops.wm.save_as_mainfile(filepath=blend, compress=True)

    rep = C.skill("blender-mcp/scripts/asset_report.py")["main"](targets=names, tri_budget=60000)
    asm = C.skill("blender-mcp/scripts/assembly_check.py")["main"](targets=names)
    res = {"blend": blend, "tris": rep["totals"], "issues": [i for i in rep["issues"] if i["severity"] != "info"],
           "per_part": {o["name"]: o.get("tris") for o in rep["objects"]},
           "assembly": {k: v for k, v in asm.items() if k not in ("islands_detail", "groups")}}
    allv = []
    dg = bpy.context.evaluated_depsgraph_get()
    for n in names:
        ev = bpy.data.objects[n].evaluated_get(dg)
        allv += [ev.matrix_world @ v.co for v in ev.data.vertices]
    lo = [min(v[i] for v in allv) for i in range(3)]
    hi = [max(v[i] for v in allv) for i in range(3)]
    res["bbox_cm"] = [[round(a * 100, 1) for a in lo], [round(a * 100, 1) for a in hi]]
    if "--no-review" not in ARGS:
        rr = C.skill("blender-mcp/scripts/review_renders.py")
        sh = rr["main"](targets=names, out_dir=C.REVIEW, label="%02d_rebuild_%s" % (STAGE, TAG), mode="clay",
                        views=C.VIEWS, resolution=800)
        res["clay"] = sh.get("sheet")
        if STAGE >= 3 or "--material" in ARGS:
            sh = rr["main"](targets=names + ([disc.name] if disc else []), out_dir=C.REVIEW,
                            label="%02d_rebuild_%s" % (STAGE, TAG), mode="material", views=C.VIEWS, resolution=800)
            res["material"] = sh.get("sheet")
    C.out(res)


main()
