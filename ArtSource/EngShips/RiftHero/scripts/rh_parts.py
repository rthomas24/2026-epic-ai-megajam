# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero hybrid: the designed hard-surface parts that replace the weak areas of the Rodin
# mesh: the portal ring (segmented housing on a yoke), the engine bulkhead and three stepped nacelles, and
# the separate portal disc. Frame: metres, nose +X, up +Z, Blender Y = -Unreal Y.
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_geo as G

PI = math.pi
BLACK, ACCENT, GUN, STEEL, GLOW, WHITE, HOT, DIM = range(8)
RING_C = (-1.70, 0.0, 0.55)
RING_RM, RING_A, RING_B, RING_RC = 0.497, 0.15, 0.075, 0.03       # mid radius, half depth, half thickness, corner
SEGS = 10


def materials(accent=None):
    """accent: an entry of rh_ships.ACCENTS (None = the Rift's magenta, the values this kit was designed with)."""
    a = accent or dict(paint=(0.80, 0.03, 0.46), glow=(0.55, 0.05, 0.42), glow_e=(1.0, 0.10, 0.78),
                       hot=(0.8, 0.5, 0.75), hot_e=(1.0, 0.72, 0.95), dim=(0.2, 0.02, 0.15), dim_e=(0.13, 0.008, 0.095))
    return [
        G.mat("MP_Black", (0.012, 0.012, 0.016), metallic=0.6, roughness=0.62),
        # accent_e: a little self-light, where the Rodin hull's own accent paint is in its emissive map (Lancer, Bastion)
        G.mat("MP_Accent", a["paint"], metallic=0.25, roughness=0.32, emission=a.get("accent_e"), strength=3.0 if a.get("accent_e") else 0.0),
        G.mat("MP_Gunmetal", (0.085, 0.09, 0.105), metallic=0.92, roughness=0.36),
        G.mat("MP_Steel", (0.30, 0.31, 0.34), metallic=0.95, roughness=0.30),
        G.mat("MP_Glow", a["glow"], roughness=0.5, emission=a["glow_e"], strength=3.0),
        G.mat("MP_White", (0.72, 0.73, 0.76), metallic=0.0, roughness=0.45),
        G.mat("MP_GlowHot", a["hot"], roughness=0.5, emission=a["hot_e"], strength=3.0),
        G.mat("MP_GlowDim", a["dim"], roughness=0.6, emission=a["dim_e"], strength=3.0),
    ]


# ------------------------------------------------------------------ rounded-rectangle section
def rrect(u, a, b, rc):
    """Point (dx, dr) on a rounded rectangle (half depth a along X, half thickness b radially, corner rc),
    parametrised by arc length mapped to u in [0, 2 pi); u = 0 is the middle of the outer face (dr = +b),
    u = pi/2 the front face (+X), pi the inner face, 3 pi/2 the rear face."""
    fa, fb, ca = 2 * (a - rc), 2 * (b - rc), PI * rc / 2
    per = 2 * fa + 2 * fb + 4 * ca
    s = (u % (2 * PI)) / (2 * PI) * per
    s = (s + fa / 2) % per                    # start at the rear end of the outer face
    if s < fa:
        return (-(a - rc) + s, b)
    s -= fa
    if s < ca:
        t = s / rc
        return ((a - rc) + rc * math.sin(t), (b - rc) + rc * math.cos(t))
    s -= ca
    if s < fb:
        return (a, (b - rc) - s)
    s -= fb
    if s < ca:
        t = s / rc
        return ((a - rc) + rc * math.cos(t), -(b - rc) - rc * math.sin(t))
    s -= ca
    if s < fa:
        return ((a - rc) - s, -b)
    s -= fa
    if s < ca:
        t = s / rc
        return (-(a - rc) - rc * math.sin(t), -(b - rc) - rc * math.cos(t))
    s -= ca
    if s < fb:
        return (-a, -(b - rc) + s)
    s -= fb
    t = s / rc
    return (-(a - rc) - rc * math.cos(t), (b - rc) + rc * math.sin(t))


def ring_f(u, ph):
    dx, dr = rrect(u, RING_A, RING_B, RING_RC)
    r = RING_RM + dr
    return (RING_C[0] + dx, RING_C[1] + r * math.cos(ph), RING_C[2] + r * math.sin(ph))


def build_ring(mats):
    u_start = PI + 0.56 - 2 * PI
    nu = 20
    us = [u_start + 2 * PI * k / nu for k in range(nu)]
    v0 = -PI / 2
    vs = list(np.linspace(v0, v0 + 2 * PI, SEGS * 4 + 1))
    eu, ev = 0.03, 0.008
    pockets = []
    dph = 2 * PI / SEGS
    for k in range(SEGS):
        a = v0 + k * dph
        b = a + dph
        if k > 0:
            pockets.append({"u0": -2.30, "u1": 2.30, "v0": a - 0.016, "v1": a + 0.016, "depth": 0.012, "eu": eu,
                            "ev": 0.005, "mat": BLACK})
        s0, s1 = a + 0.060, b - 0.060
        pockets.append({"u0": -0.14, "u1": 0.14, "v0": s0, "v1": s1, "depth": 0.013, "eu": eu, "ev": ev,
                        "mat": GLOW, "wall_mat": BLACK})                                   # outer seam channel
        pockets.append({"u0": 2.66, "u1": 3.60, "v0": a + 0.03, "v1": b - 0.03, "depth": 0.020, "eu": eu,
                        "ev": ev, "mat": GLOW, "wall_mat": BLACK})                          # inner emitter groove
        for c in (PI / 2, -PI / 2):                                                         # side face panels
            pockets.append({"u0": c - 0.25, "u1": c + 0.25, "v0": s0, "v1": s1, "depth": 0.006, "eu": eu,
                            "ev": ev, "mat": ACCENT, "wall_mat": BLACK})
        for c in (0.52, -0.52):                                                             # raised outer armour
            pockets.append({"u0": c - 0.26, "u1": c + 0.26, "v0": s0 - 0.02, "v1": s1 + 0.02, "depth": -0.007,
                            "eu": eu, "ev": ev, "mat": WHITE, "wall_mat": WHITE})
    ring = G.grid_surface("SM_Rift_Ring", ring_f, us, vs, mats, pockets=pockets, cap0="none", cap1="none",
                          mat_fn=lambda u, v: GUN, simplify_deg=0.3)
    return ring


def yoke_f(u, t):
    """Clamp around the lower arc of the ring. t in [-1, 1] maps to +-58 degrees either side of the bottom."""
    ph = -PI / 2 + t * math.radians(58)
    k = 1.0 - 0.30 * abs(t) ** 3
    dx, dr = rrect(u, 0.195 * k, 0.118 * k, 0.04)
    r = RING_RM + dr
    return (RING_C[0] + dx, RING_C[1] + r * math.cos(ph), RING_C[2] + r * math.sin(ph))


def build_yoke(mats):
    u_start = PI + 0.56 - 2 * PI
    us = [u_start + 2 * PI * k / 20 for k in range(20)]
    vs = list(np.linspace(-1.0, 1.0, 21))
    pockets = []
    for sgn in (1, -1):
        lo, hi = (0.20, 0.86) if sgn > 0 else (-0.86, -0.20)
        for c in (PI / 2, -PI / 2):
            pockets.append({"u0": c - 0.42, "u1": c + 0.42, "v0": lo, "v1": hi, "depth": 0.008, "eu": 0.03,
                            "ev": 0.012, "mat": WHITE, "wall_mat": BLACK})
        pockets.append({"u0": -0.10, "u1": 0.10, "v0": lo + 0.1, "v1": hi - 0.06, "depth": 0.012, "eu": 0.03,
                        "ev": 0.012, "mat": GLOW, "wall_mat": BLACK})
        pockets.append({"u0": -1.9, "u1": 1.9, "v0": lo - 0.07 if sgn > 0 else hi + 0.045,
                        "v1": lo - 0.045 if sgn > 0 else hi + 0.07, "depth": 0.008, "eu": 0.03, "ev": 0.006,
                        "mat": BLACK})
    yoke = G.grid_surface("SM_Rift_RingYoke", yoke_f, us, vs, mats, pockets=pockets, cap0="ngon", cap1="ngon",
                          mat_fn=lambda u, v: GUN, simplify_deg=0.3)
    G.bevel(yoke, width=0.006, segments=1, angle_deg=50)
    return yoke


def pedestal_f(th, x):
    """Base that plants the yoke into the spine: wide under the ring, running forward into a keel."""
    k = np.array([(-2.02, 0.10, -0.06), (-1.94, 0.30, 0.03), (-1.70, 0.36, 0.055), (-1.48, 0.32, 0.05),
                  (-1.30, 0.20, 0.10), (-1.12, 0.13, 0.17), (-0.98, 0.05, 0.20)])
    hw, top = G.pchip(k[:, 0], k[:, 1:], x)
    bot = -0.20
    c, s = math.cos(th), math.sin(th)
    zc = (top + bot) / 2
    return (x, hw * G.spow(c, 5.0), zc + (top - bot) / 2 * G.spow(s, 5.0))


def build_pedestal(mats):
    us = [-PI / 2 + 2 * PI * k / 28 for k in range(28)]
    vs = list(np.linspace(-2.02, -0.98, 24))
    pockets = []
    for sgn in (1, -1):
        a, b = (0.30, 0.95) if sgn > 0 else (PI - 0.95, PI - 0.30)
        for i in range(4):
            x0 = -1.43 + i * 0.075
            pockets.append({"u0": a, "u1": b, "v0": x0, "v1": x0 + 0.04, "depth": 0.02, "eu": 0.05, "ev": 0.006,
                            "mat": BLACK})
    pockets.append({"u0": 1.2, "u1": PI - 1.2, "v0": -1.46, "v1": -1.08, "depth": 0.005, "eu": 0.04, "ev": 0.008,
                    "mat": ACCENT, "wall_mat": BLACK})
    ped = G.grid_surface("SM_Rift_RingBase", pedestal_f, us, vs, mats, pockets=pockets, cap0="ngon", cap1="ngon",
                         mat_fn=lambda u, v: WHITE if 0.12 < u < PI - 0.12 else GUN, simplify_deg=0.3)
    G.bevel(ped, width=0.005, segments=1, angle_deg=50)
    return ped


def build_portal_disc():
    m = G.mat("M_Rift_Portal", (0.25, 0.02, 0.35), roughness=0.3, emission=(0.5, 0.05, 0.8), strength=0.6)
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    R, seg, rings = RING_RM - RING_B - 0.012 + 0.03, 64, 6        # reaches into the emitter groove
    centre = bm.verts.new((0.0, 0.0, 0.0))
    vr = [[bm.verts.new((0.0, R * (k / rings) * math.cos(2 * PI * s / seg), R * (k / rings) * math.sin(2 * PI * s / seg)))
           for s in range(seg)] for k in range(1, rings + 1)]

    def uv(v):
        return (0.5 + 0.5 * v.co.y / R, 0.5 + 0.5 * v.co.z / R)
    for s in range(seg):
        s1 = (s + 1) % seg
        f = bm.faces.new((centre, vr[0][s], vr[0][s1]))
        for l in f.loops:
            l[uvl].uv = uv(l.vert)
        for k in range(rings - 1):
            f = bm.faces.new((vr[k][s], vr[k + 1][s], vr[k + 1][s1], vr[k][s1]))
            for l in f.loops:
                l[uvl].uv = uv(l.vert)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.faces.ensure_lookup_table()
    bm.faces[0].normal_update()
    if bm.faces[0].normal.x > 0:              # faces the chase camera (-X); the material should be two-sided
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    disc = G._finish("SM_Rift_PortalDisc", bm, [m], 30.0)
    disc.location = RING_C
    return disc, R


# ------------------------------------------------------------------ engines
X_EXIT = -2.50
NOZ = {"C": ((0.0, -0.34), 0.275), "O": ((0.64, -0.34), 0.19)}


def nacelle(name, centre_yz, R, mats, segments):
    X = X_EXIT
    p = [(X + 0.66, 0.0), (X + 0.64, 0.50 * R), (X + 0.56, 0.88 * R), (X + 0.44, 1.00 * R), (X + 0.36, 1.00 * R),
         (X + 0.25, 1.15 * R), (X + 0.185, 1.15 * R), (X + 0.185, 1.02 * R), (X + 0.115, 1.02 * R),
         (X + 0.115, 1.09 * R), (X + 0.085, 1.09 * R), (X + 0.085, 1.00 * R), (X + 0.012, 0.945 * R),
         (X, 0.92 * R), (X, 0.85 * R), (X + 0.10, 0.80 * R), (X + 0.13, 0.80 * R), (X + 0.13, 0.71 * R),
         (X + 0.165, 0.71 * R), (X + 0.165, 0.63 * R), (X + 0.31, 0.36 * R), (X + 0.31, 0.0)]
    # lip dark -> one bright inner ring -> dim magenta throat wall -> white-hot core
    idx = [GUN, WHITE, WHITE, WHITE, STEEL, ACCENT, GUN, GUN, GUN, STEEL, GUN, STEEL, STEEL, STEEL,
           GUN, GUN, BLACK, GLOW, BLACK, DIM, HOT, HOT]
    ob = G.lathe_x(name, p, (0.0, centre_yz[0], centre_yz[1]), mats, segments=segments, mat_idx=idx)
    # actuator struts around the sleeve, between the two collars
    n = 10 if R > 0.25 else 8
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    uvl = bm.loops.layers.uv.verify()
    for k in range(n):
        ang = 2 * PI * (k + 0.5) / n
        r0, r1 = 0.99 * R, 1.115 * R
        w = 0.018 + 0.03 * R
        x0, x1 = X + 0.118, X + 0.183
        c, s = math.cos(ang), math.sin(ang)
        pts = []
        for x in (x0, x1):
            for r, ww in ((r0, w), (r1, w * 0.7)):
                for side in (-1, 1):
                    y = r * c - side * ww * s
                    z = r * s + side * ww * c
                    pts.append(bm.verts.new((x, centre_yz[0] + y, centre_yz[1] + z)))
        for q in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
            f = bm.faces.new([pts[i] for i in q])
            f.material_index = STEEL
            for j, l in enumerate(f.loops):
                l[uvl].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[j]
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.shade_smooth()
    ob.data.set_sharp_from_angle(angle=math.radians(30))
    return ob


BULK = {"x0": -2.19, "x1": -1.98, "hw": 0.76, "zt": -0.10, "zb": -0.66, "n": 3.6}


def bulkhead_f(th, x):
    hw, zt, zb, n = BULK["hw"], BULK["zt"], BULK["zb"], BULK["n"]
    t = (x - BULK["x0"]) / (BULK["x1"] - BULK["x0"])
    k = 1.0 - 0.10 * max(0.0, (t - 0.35) / 0.65) ** 1.5                   # shroud narrows forward into the hull
    zc = (zt + zb) / 2
    return (x, hw * k * G.spow(math.cos(th), n), zc + (zt - zb) / 2 * k * G.spow(math.sin(th), n))


def build_engines(mats):
    objs = []
    us = [-PI / 2 + 2 * PI * k / 48 for k in range(48)]
    vs = list(np.linspace(BULK["x0"], BULK["x1"], 9))
    pockets = []
    d = math.radians
    bulk = G.grid_surface("SM_Rift_EngineBulkhead", bulkhead_f, us, vs, mats, pockets=pockets, cap0="ngon",
                          cap1="ngon", mat_fn=lambda u, v: GUN)
    bm = bmesh.new()
    bm.from_mesh(bulk.data)
    rear = [f for f in bm.faces if len(f.verts) > 8 and f.normal.x < -0.9]
    bmesh.ops.inset_region(bm, faces=rear, thickness=0.035, depth=0.0, use_even_offset=True)
    r2 = bmesh.ops.inset_region(bm, faces=rear, thickness=0.010, depth=-0.03, use_even_offset=True)
    for f in rear + r2["faces"]:
        f.material_index = BLACK
    bm.to_mesh(bulk.data)
    bm.free()
    G.bevel(bulk, width=0.008, segments=2, angle_deg=35)
    objs.append(bulk)

    c = nacelle("SM_Rift_NozzleC", NOZ["C"][0], NOZ["C"][1], mats, 36)
    o = nacelle("SM_Rift_NozzleO", NOZ["O"][0], NOZ["O"][1], mats, 28)
    G.mirror_y(o)
    for ob in (c, o):
        G.bevel(ob, width=0.004, segments=1, angle_deg=32)
        objs.append(ob)

    # back-plate structure: louvre packs in the four gaps between the nacelles, two tie bars, bolt bosses
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")

    def add_box(lo, hi, mi):
        lo, hi = Vector(lo), Vector(hi)
        vs_ = [bm.verts.new((x, y, z)) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]
        for q in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
            f = bm.faces.new([vs_[i] for i in q])
            f.material_index = mi
            for j, l in enumerate(f.loops):
                l[uvl].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[j]

    xr = BULK["x0"] + 0.03                    # recessed plate surface
    for sy in (1, -1):
        for zc, hz in ((-0.175, 0.04), (-0.525, 0.04)):
            yc = 0.40 * sy
            add_box((xr - 0.035, yc - 0.06, zc - hz), (xr + 0.01, yc + 0.06, zc + hz), STEEL)      # frame
            for i in range(4):
                z = zc - hz + 0.012 + i * (2 * hz - 0.024) / 3.0
                add_box((xr - 0.045, yc - 0.048, z - 0.005), (xr - 0.03, yc + 0.048, z + 0.005), BLACK)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    det = G._finish("SM_Rift_EngineDetail", bm, mats, 30.0)
    G.bevel(det, width=0.003, segments=1, angle_deg=30)
    objs.append(det)
    return objs


def tail_f(th, x):
    """Ventral boat-tail under the engines: closes the Rodin belly plate where it was cut."""
    k = np.array([(-2.27, 0.20, -0.745), (-2.19, 0.33, -0.86), (-2.05, 0.42, -0.955), (-1.86, 0.44, -0.97)])
    hw, bot = G.pchip(k[:, 0], k[:, 1:], x)
    top = -0.62
    zc = (top + bot) / 2
    return (x, hw * G.spow(math.cos(th), 4.0), zc + (top - bot) / 2 * G.spow(math.sin(th), 4.0))


def build_tail(mats):
    us = [-PI / 2 + 2 * PI * k / 24 for k in range(24)]
    vs = list(np.linspace(-2.27, -1.86, 10))
    pockets = []
    for i in range(3):
        x0 = -2.16 + i * 0.07
        pockets.append({"u0": -PI / 2 + 0.25, "u1": -PI / 2 + 1.0, "v0": x0, "v1": x0 + 0.035, "depth": 0.018,
                        "eu": 0.05, "ev": 0.006, "mat": BLACK})
        pockets.append({"u0": -PI / 2 - 1.0 + 2 * PI, "u1": -PI / 2 - 0.25 + 2 * PI, "v0": x0, "v1": x0 + 0.035,
                        "depth": 0.018, "eu": 0.05, "ev": 0.006, "mat": BLACK})
    for a, b in ((-0.35, 0.45), (PI - 0.45, PI + 0.35)):                 # white cheek plates
        pockets.append({"u0": a, "u1": b, "v0": -2.21, "v1": -1.91, "depth": -0.008, "eu": 0.04, "ev": 0.008,
                        "mat": WHITE, "wall_mat": WHITE})
    us = [-PI / 2 + 0.1 + 2 * PI * k / 24 for k in range(24)]
    tail = G.grid_surface("SM_Rift_VentralTail", tail_f, us, vs, mats, pockets=pockets, cap0="ngon", cap1="ngon",
                          mat_fn=lambda u, v: GUN, simplify_deg=0.3)
    G.bevel(tail, width=0.006, segments=1, angle_deg=45)
    return tail


def build_all():
    mats = materials()
    parts = [build_ring(mats), build_yoke(mats), build_pedestal(mats), build_tail(mats)] + build_engines(mats)
    disc, disc_r = build_portal_disc()
    return mats, parts, disc


def cut_rodin(ob):
    """Remove the Rodin ring and everything behind the engine bulkhead. Returns the number of faces removed."""
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    kill = []
    for f in bm.faces:
        c = f.calc_center_median()
        r = math.hypot(c.y, c.z - RING_C[2])
        near = -1.88 <= c.x <= -1.30 and ((0.38 <= r <= 0.70 and c.z > 0.12) or
                                          (0.70 < r <= 0.82 and c.z > 0.30 and abs(c.y) < 0.72))
        wide = -2.10 <= c.x <= -1.22 and 0.36 <= r <= 0.72 and c.z > 0.15 and abs(c.y) < 0.60
        ringish = near or wide
        if ringish or c.x < BULK["x0"] + 0.045 or (c.x < -1.95 and c.z < -0.71):
            kill.append(f)
    n = len(kill)
    bmesh.ops.delete(bm, geom=kill, context="FACES")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(ob.data)
    bm.free()
    return n
