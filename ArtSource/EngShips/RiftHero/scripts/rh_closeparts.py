# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero close-range parts (ship builder, Nov 13 night). Ryan, in the Dominion hangar at 3-4 m: "not
# sharp". The fresh-eyes review of the hybrid named what the player sees up close and what reads as melted AI output:
# the canopy (a melted chrome blob where the pilot should be), the guns (crumpled foil), the nose and the upper spine,
# and the livery's jagged black "crack" lines and streaky magenta. This module builds those as designed hard-surface
# parts on the Rodin hull's measured shape (the silhouette stays), cuts the Rodin faces they replace, and cleans the
# livery texture. Used by rh_hybrid.py (--close).
#   build(mats, body) -> (parts to join into SM_Rift_Parts, canopy glass object SM_Rift_Canopy)
#   clean_livery(bc_png) -> stats (in place)
# Frame: metres, nose +X, up +Z, Blender Y = -Unreal Y (the ship is symmetric).
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_geo as G
import rh_ships

PI = math.pi
BLACK, ACCENT, GUN, STEEL, GLOW, WHITE, HOT, DIM = range(8)      # rh_parts.materials() order
COCKPIT = (0.60, 0.0, 0.0)                                         # socket Cockpit (Unreal 60, 0, 0)
CANOPY_X = (0.30, 1.50)              # the Rodin canopy's dark glass, measured from above (y up to +-0.33)
GUN_Y, GUN_Z = 0.37, -0.565
NOSE_X = (1.45, 2.44)
SPINE_X = (-0.92, 0.27)
K = rh_ships.SHIPS["Rift"]                                         # the ship being built: configure(cfg)


def configure(cfg):
    """Build for another ship (rh_ships.py). The constants above are the Rift's and stay its defaults."""
    global K, COCKPIT, CANOPY_X, GUN_Y, GUN_Z, NOSE_X, SPINE_X
    K = cfg
    if cfg.get("canopy"):
        COCKPIT, CANOPY_X = cfg["canopy"]["cockpit"], cfg["canopy"]["x"]
    if cfg.get("guns"):
        GUN_Y, GUN_Z = cfg["guns"]["y"], cfg["guns"]["z"]
    if cfg.get("nose") and "x" in cfg["nose"]:
        NOSE_X = cfg["nose"]["x"]
    if cfg.get("spine"):
        SPINE_X = cfg["spine"]["x"]


def _bvh(ob):
    dg = bpy.context.evaluated_depsgraph_get()
    return BVHTree.FromObject(ob, dg)


def _ray(bvh, o, d):
    hit = bvh.ray_cast(Vector(o), Vector(d).normalized(), 20.0)
    return hit[0]


def _top(bvh, x, y=0.0):
    h = _ray(bvh, (x, y, 5.0), (0, 0, -1))
    return h.z if h else None


def _smooth(vals, k=1):
    v = np.array(vals, float)
    out = v.copy()
    for i in range(len(v)):
        a, b = max(0, i - k), min(len(v), i + k + 1)
        out[i] = v[a:b].mean()
    return out


def _interp(xs, ys):
    xs, ys = np.array(xs, float), np.array(ys, float)
    return lambda x: float(np.interp(x, xs, ys))


def _cut(body, pred):
    bm = bmesh.new()
    bm.from_mesh(body.data)
    kill = [f for f in bm.faces if pred(f.calc_center_median())]
    n = len(kill)
    bmesh.ops.delete(bm, geom=kill, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bm.to_mesh(body.data)
    bm.free()
    body.data.update()
    return n


def _slice(body, pred, x):
    """Cut the body with the plane X = x and drop what lies ahead of it, among the faces whose centre passes pred
    (pred must also take the faces the plane crosses). Unlike _cut (whole faces by their centre: a ragged edge) the
    hull ends in a straight section, which a new part's collar hides."""
    bm = bmesh.new()
    bm.from_mesh(body.data)
    faces = [f for f in bm.faces if pred(f.calc_center_median())]
    n0 = len(bm.faces)
    geom = set(faces)
    for f in faces:
        geom.update(f.edges)
        geom.update(f.verts)
    bmesh.ops.bisect_plane(bm, geom=list(geom), dist=1e-5, plane_co=(x, 0.0, 0.0), plane_no=(1.0, 0.0, 0.0),
                           clear_outer=True, clear_inner=False)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    n = n0 - len(bm.faces)
    bm.to_mesh(body.data)
    bm.free()
    body.data.update()
    return n


def _face(ob, direction):
    """Open strips: make the faces look along `direction` on average (Unreal draws one side only)."""
    d = Vector(direction)
    me = ob.data
    if sum(p.normal.dot(d) * p.area for p in me.polygons) < 0:
        me.flip_normals()
    return ob


def sweep_rect(name, pts, w, h, mats, mat_idx, up=(0, 0, 1), parent=None):
    """A rectangular bar (w across, h along `up`-ish) swept along a polyline: canopy frame members."""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    P = [Vector(p) for p in pts]
    rings = []
    for i, p in enumerate(P):
        t = (P[min(i + 1, len(P) - 1)] - P[max(i - 1, 0)]).normalized()
        u = Vector(up)
        side = t.cross(u)
        if side.length < 1e-6:
            side = t.cross(Vector((0, 1, 0)))
        side.normalize()
        nup = side.cross(t).normalized()
        rings.append([bm.verts.new(p + side * sx * w * 0.5 + nup * sz * h * 0.5) for sx, sz in ((-1, -1), (1, -1), (1, 1), (-1, 1))])
    for i in range(len(rings) - 1):
        for k in range(4):
            f = bm.faces.new((rings[i][k], rings[i][(k + 1) % 4], rings[i + 1][(k + 1) % 4], rings[i + 1][k]))
            f.material_index = mat_idx
            for j, l in enumerate(f.loops):
                l[uvl].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[j]
    for ring in (rings[0], rings[-1]):
        f = bm.faces.new(ring)
        f.material_index = mat_idx
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    ob = G._finish(name, bm, mats, 30.0, parent)
    G.bevel(ob, width=min(w, h) * 0.18, segments=2)
    return ob


# ---------------------------------------------------------------------------------------------------- canopy
def build_canopy(mats, body, bvh):
    """The glass follows the Rodin dome (rays from above, an even polynomial fitted per station: one clean, smooth
    pane), 8 mm above it; the Rodin dome under it is cut and a dark cockpit deck, seat, pilot and dash go inside."""
    L = CANOPY_X[1] - CANOPY_X[0]
    ca = K["canopy"]
    ox, ky = ca["ox"], ca["ky"]          # where the seat, pilot and dash sit (designed on the Rift: 0, 1)

    def plan_w(x):                                              # teardrop plan: wide behind, pointed at the front
        s = min(1.0, max(0.0, (x - CANOPY_X[0]) / L))
        r0, r1 = ca.get("plan", (0.08, 0.92))                   # a larger r0 = a blunter, wider rear end
        return ca["wmax"] * (math.sin(PI * (r0 + r1 * s)) ** 0.5) * (1.0 - 0.25 * s)

    xs = np.linspace(CANOPY_X[0], CANOPY_X[1], 25)
    coefs = []
    for x in xs:
        w = max(plan_w(x), 0.04)
        ys = np.linspace(-w, w, 15)
        zs = []
        for y in ys:
            hz = _ray(bvh, (x, y, 5.0), (0, 0, -1))
            zs.append(hz.z if hz else np.nan)
        ys, zs = np.array(ys), np.array(zs)
        ok = ~np.isnan(zs)
        A = np.stack([np.ones(ok.sum()), ys[ok] ** 2, ys[ok] ** 4], -1)
        coefs.append(np.linalg.lstsq(A, zs[ok], rcond=None)[0] if ok.sum() >= 3 else np.array([0.1, -1.0, 0.0]))
    coefs = np.array([_smooth(np.array(coefs)[:, k], 2) for k in range(3)]).T
    cf = [_interp(xs, coefs[:, k]) for k in range(3)]

    def zdome(x, y, grow=0.008):
        return cf[0](x) + cf[1](x) * y * y + cf[2](x) * y ** 4 + grow

    if ca.get("dome") == "super":
        # a designed bubble instead of the fitted one (the Lancer's Rodin canopy is boxy, with thick frame bars: the
        # fit wobbled): crown = the measured top on the centre line, base = the hull just outside the canopy, a
        # superellipse between them. The glass edge and the sills then sit on the hull.
        crown, base = [], []
        for x in xs:
            w = plan_w(x)
            tops = [h.z for h in (_ray(bvh, (x, y, 5.0), (0, 0, -1)) for y in (-0.03, 0.0, 0.03)) if h]
            sides = [h.z for h in (_ray(bvh, (x, sy * (w + 0.04), 5.0), (0, 0, -1)) for sy in (1, -1)) if h]
            crown.append(max(tops) if tops else 0.0)
            base.append(sum(sides) / len(sides) if sides else (crown[-1] - 0.2))
        base = _smooth(base, 3)
        crown = np.maximum(_smooth(crown, 2), base + 0.02)
        cr, ba = _interp(xs, crown), _interp(xs, base)
        ne = ca.get("dome_n", 2.3)

        def zdome(x, y, grow=0.008):
            t = min(1.0, abs(y) / max(plan_w(x), 0.04))
            return ba(x) + (cr(x) - ba(x)) * (1.0 - t ** ne) ** (1.0 / ne) + grow

    gm = G.mat("M_%s_Glass" % K["ship"], rh_ships.ACCENTS[K["accent"]]["glass"], roughness=0.05)

    def gf(u, v):
        y = (u - 0.5) * 2 * plan_w(v)
        return (v - COCKPIT[0], y, zdome(v, y) - COCKPIT[2])
    glass = G.grid_surface("SM_%s_Canopy" % K["ship"], gf, list(np.linspace(0, 1, 25)), list(np.linspace(CANOPY_X[0], CANOPY_X[1], 40)),
                           [gm], closed_u=False, cap0="none", cap1="none", smooth_deg=80.0)
    glass.location = COCKPIT

    def deck_z(x):
        return zdome(x, 0.0, 0.0) - ca["deck"]

    cut = _cut(body, lambda c: CANOPY_X[0] + 0.02 < c.x < CANOPY_X[1] - 0.03 and abs(c.y) < plan_w(c.x) * ca.get("cutk", 1.08)
               and c.z > deck_z(c.x) + 0.01)                 # the Rodin chrome canopy frame goes too: the sills cover the seam
    parts = []

    def deck_f(u, v):
        return (v, (u - 0.5) * 2 * plan_w(v) * 1.02, deck_z(v))
    parts.append(G.grid_surface("CP_Deck", deck_f, list(np.linspace(0, 1, 7)), list(np.linspace(CANOPY_X[0], CANOPY_X[1], 18)),
                                mats, closed_u=False, cap0="none", cap1="none", mat_fn=lambda u, v: BLACK))
    # inner tub walls from the deck up to the glass edge (hides the cut)
    for sy in (1, -1):
        def wall_f(u, v, sy=sy):
            y = sy * plan_w(v) * 0.99
            return (v, y, deck_z(v) + u * (zdome(v, y, 0.0) - deck_z(v)))
        parts.append(G.grid_surface("CP_Wall_%d" % sy, wall_f, list(np.linspace(0, 1, 4)), list(np.linspace(CANOPY_X[0], CANOPY_X[1], 18)),
                                    mats, closed_u=False, cap0="none", cap1="none", mat_fn=lambda u, v: BLACK))
    # frame: sills along the glass edges, three bows over it
    for sy in (1, -1):
        pts = [(x, sy * plan_w(x), zdome(x, sy * plan_w(x), 0.014)) for x in np.linspace(CANOPY_X[0] + 0.03, CANOPY_X[1] - 0.03, 30)]
        parts.append(sweep_rect("CP_Sill_%d" % sy, pts, ca.get("sill_w", 0.052), 0.024, mats, WHITE))
    if ca.get("skirt"):
        # a coaming around the canopy: from the sill out and down into the hull (covers the ragged edge of the cut),
        # and a turtle-deck behind the glass that closes its open rear arch and fades into the hull
        out, drop = ca["skirt"]
        for sy in (1, -1):
            def skirt_f(u, v, sy=sy):
                w = plan_w(min(max(v, CANOPY_X[0]), CANOPY_X[1]))
                inner = 0.015 * min(1.0, max(0.0, (CANOPY_X[1] - v) / 0.03))   # ahead of the glass tip the two sides meet
                dive = min(1.0, max(0.0, (v - CANOPY_X[1] - 0.03) / 0.09)) ** 1.5      # the nose pad sinks into the hull
                return (v, sy * (w + inner + u * out),
                        zdome(min(max(v, CANOPY_X[0]), CANOPY_X[1]), w, 0.018) - (u ** 1.5) * drop - dive * (drop + 0.02))
            sk = G.grid_surface("CP_Skirt_%d" % sy, skirt_f, list(np.linspace(0, 1, 5)),
                                list(np.linspace(CANOPY_X[0] - 0.02, CANOPY_X[1] + 0.12, 38)), mats, closed_u=False,
                                cap0="none", cap1="none", mat_fn=lambda u, v: WHITE, smooth_deg=50.0)
            parts.append(_face(sk, (0, sy * 0.5, 1)))
        # the Rodin frame's front tip and the ragged edge around it go: the coaming's nose pad covers the hole
        cut += _cut(body, lambda c: CANOPY_X[1] - 0.10 < c.x < CANOPY_X[1] + 0.04 and abs(c.y) < 0.06
                    and c.z > zdome(CANOPY_X[1], 0.0, 0.0) - drop + 0.01)
        xr = CANOPY_X[0]
        w0, hl = plan_w(xr), ca.get("hump", 0.18)
        zb = zdome(xr, w0, 0.0) - drop

        def hump_f(u, t):
            y0 = (u - 0.5) * 2 * w0
            k = (1.0 - t) ** 1.4
            return (xr + 0.012 - t * hl, y0 * (1.0 + 0.14 * (1 - t)) * (1.0 - 0.3 * t), zb + (zdome(xr, y0, 0.020) - zb) * k)
        hump = G.grid_surface("CP_Hump", hump_f, list(np.linspace(0, 1, 17)), list(np.linspace(0, 1, 9)), mats, closed_u=False,
                              cap0="none", cap1="none", mat_fn=lambda u, v: GUN if v < 0.12 else WHITE, smooth_deg=50.0)
        parts.append(_face(hump, (-0.3, 0, 1)))
    for xb, wbar in ((CANOPY_X[0] + 0.10, 0.034), (ca["bows"][0], 0.026), (ca["bows"][1], 0.024)):
        w = plan_w(xb)
        pts = [(xb, y, zdome(xb, y, 0.016)) for y in np.linspace(-w, w, 21)]
        parts.append(sweep_rect("CP_Bow_%.2f" % xb, pts, wbar, 0.016, mats, GUN, up=(1, 0, 0)))
    # seat, pilot and dash, seen through the tinted glass
    zd = deck_z(0.60 + ox)
    parts.append(G.box("CP_SeatBack", (0.40 + ox, -0.09 * ky, zd), (0.47 + ox, 0.09 * ky, zd + 0.21), mats, BLACK))
    parts.append(G.box("CP_SeatHead", (0.41 + ox, -0.06 * ky, zd + 0.21), (0.46 + ox, 0.06 * ky, zd + 0.27), mats, ACCENT))
    parts.append(G.box("CP_Shoulders", (0.47 + ox, -0.105 * ky, zd + 0.06), (0.60 + ox, 0.105 * ky, zd + 0.15), mats, WHITE))
    parts.append(G.box("CP_Dash", (0.98 + ox, -0.16 * ky, deck_z(1.0 + ox)), (1.12 + ox, 0.16 * ky, deck_z(1.0 + ox) + 0.10), mats, GUN,
                       taper=(1.0, 0.4)))
    hk = 0.78 * ca.get("hk", 1.0)                               # v08: smaller, lower (review: large, sits high)
    parts.append(G.lathe_x("CP_Helmet", [(0.0, 0.0), (0.012 * hk, 0.042 * hk), (0.045 * hk, 0.066 * hk), (0.095 * hk, 0.068 * hk),
                                         (0.14 * hk, 0.055 * hk), (0.16 * hk, 0.0)],
                           (0.49 + ox, 0.0, zd + 0.18), mats, segments=24, mat_idx=[WHITE] * 6))
    parts.append(G.box("CP_Visor", (0.49 + ox + 0.115, -0.035, zd + 0.17), (0.49 + ox + 0.13, 0.035, zd + 0.20), mats, GLOW))
    return parts, glass, cut


# ---------------------------------------------------------------------------------------------------- guns
def build_guns(mats, body):
    return {"rift": _guns_rift, "lancer": _guns_lancer}[K["guns"]["kind"]](mats, body)


def _guns_lancer(mats, body):
    """A clean symmetric pair of rapid cannons (Lead, Nov 13 night: the Rodin mesh had a long twin barrel on one side
    and a short pod on the other). Each: a collar pod set into the front of the Rodin gun housing, the main barrel on
    the muzzle socket's axis, a shorter upper barrel and a clamp."""
    g = K["guns"]
    gy, gz, x0, tip = g["y"], g["z"], g["x0"], g["tip"]
    cut = _slice(body, lambda c: c.x > g["cut_x"] - 0.15 and 0.385 < abs(c.y) < 0.62 and c.z < -0.40, g["cut_x"])
    parts = []
    for sy in (1, -1):
        y = sy * gy
        pod = [(0.0, 0.0), (0.0, 0.060), (0.03, 0.088), (0.10, 0.088), (0.10, 0.082), (0.112, 0.082), (0.112, 0.088),
               (0.20, 0.088), (0.20, 0.093), (0.235, 0.093), (0.235, 0.088), (0.27, 0.088), (0.30, 0.078), (0.33, 0.060),
               (0.345, 0.040), (0.345, 0.0)]
        pidx = [GUN, GUN, GUN, BLACK, BLACK, BLACK, GUN, STEEL, ACCENT, STEEL, GUN, GUN, GUN, GUN, GUN, GUN]
        parts.append(G.lathe_x("GN_Pod_%d" % sy, pod, (x0, y, gz), mats, segments=32, mat_idx=pidx))
        bx = x0 + 0.32
        ln = tip - bx
        barrel = [(0.0, 0.0), (0.0, 0.030), (ln - 0.24, 0.030), (ln - 0.24, 0.037), (ln - 0.21, 0.037), (ln - 0.21, 0.030),
                  (ln - 0.09, 0.030), (ln - 0.09, 0.040), (ln, 0.040), (ln, 0.022), (ln - 0.03, 0.022), (ln - 0.03, 0.0)]
        bidx = [GUN, STEEL, STEEL, GUN, STEEL, STEEL, GUN, GUN, BLACK, GLOW, DIM, BLACK]
        parts.append(G.lathe_x("GN_Barrel_%d" % sy, barrel, (bx, y, gz), mats, segments=24, mat_idx=bidx))
        up = [(0.0, 0.0), (0.0, 0.019), (0.20, 0.019), (0.20, 0.025), (0.25, 0.025), (0.25, 0.013), (0.23, 0.013), (0.23, 0.0)]
        uidx = [GUN, STEEL, GUN, GUN, BLACK, DIM, BLACK, BLACK]
        parts.append(G.lathe_x("GN_Upper_%d" % sy, up, (bx - 0.01, y, gz + 0.058), mats, segments=20, mat_idx=uidx))
        parts.append(G.box("GN_Clamp_%d" % sy, (bx + 0.09, y - 0.042, gz - 0.042), (bx + 0.125, y + 0.042, gz + 0.088), mats, GUN))
        for k in range(2):                                       # cooling slots on the collar's top
            vx = x0 + 0.125 + k * 0.038
            parts.append(G.box("GN_Vent_%d_%d" % (sy, k), (vx, y - 0.030, gz + 0.066), (vx + 0.024, y + 0.030, gz + 0.092), mats, BLACK))
    for p in parts:
        G.bevel(p, width=0.004, segments=2, angle_deg=30.0)
    return parts, cut


def _guns_rift(mats, body):
    parts = []
    cut = _cut(body, lambda c: 1.40 < c.x < 2.62 and math.hypot(abs(c.y) - GUN_Y, c.z - GUN_Z) < 0.15)
    for sy in (1, -1):
        y = sy * GUN_Y
        # v08: slimmer (review: oversized), with seam grooves and a vented front collar
        pod = [(1.42, 0.0), (1.42, 0.068), (1.48, 0.095), (1.585, 0.095), (1.585, 0.089), (1.597, 0.089), (1.597, 0.095),
               (1.70, 0.095), (1.70, 0.100), (1.745, 0.100), (1.745, 0.095), (1.86, 0.095), (1.86, 0.089), (1.872, 0.089),
               (1.872, 0.095), (1.93, 0.095), (1.97, 0.086), (2.04, 0.070), (2.06, 0.045), (2.06, 0.0)]
        pidx = [GUN, WHITE, WHITE, BLACK, BLACK, BLACK, WHITE, STEEL, ACCENT, STEEL, WHITE, BLACK, BLACK, BLACK, WHITE, GUN,
                GUN, GUN, GUN, GUN]
        parts.append(G.lathe_x("GN_Pod_%d" % sy, [(x - 1.42, r) for x, r in pod], (1.42, y, GUN_Z), mats, segments=32, mat_idx=pidx))
        barrel = [(2.04, 0.0), (2.04, 0.030), (2.22, 0.030), (2.22, 0.038), (2.26, 0.038), (2.26, 0.030), (2.42, 0.030),
                  (2.42, 0.041), (2.50, 0.041), (2.50, 0.022), (2.47, 0.022), (2.47, 0.0)]
        bidx = [GUN, STEEL, STEEL, GUN, STEEL, STEEL, GUN, GUN, BLACK, BLACK, BLACK, BLACK]
        parts.append(G.lathe_x("GN_Barrel_%d" % sy, [(x - 2.04, r) for x, r in barrel], (2.04, y, GUN_Z), mats, segments=24, mat_idx=bidx))
        # cooling vents: a row of dark slots on the pod's upper flank
        for k in range(4):
            vx = 1.62 + k * 0.055
            parts.append(G.box("GN_Vent_%d_%d" % (sy, k), (vx, y - 0.035, GUN_Z + 0.072), (vx + 0.03, y + 0.035, GUN_Z + 0.098), mats, BLACK))
        # the mount that ties the pod to the hull
        parts.append(G.box("GN_Mount_%d" % sy, (1.50, y - sy * 0.10 - 0.025, GUN_Z + 0.02), (1.90, y - sy * 0.10 + 0.025, GUN_Z + 0.11),
                           mats, GUN))
    for p in parts:
        G.bevel(p, width=0.004, segments=2, angle_deg=30.0)
    return parts, cut


# ---------------------------------------------------------------------------------------------------- nose
def build_nose(mats, body, bvh):
    return {"rift": _nose_rift, "needle": _nose_needle}[K["nose"]["kind"]](mats, body, bvh)


def _nose_needle(mats, body, bvh):
    """The Lancer's ram prow: a clean shell on the measured stations (top, bottom, half width), roof-like above the
    chine and shallow below it, closing to a point at the Rodin tip; the Rodin nose inside it is cut whole."""
    n = K["nose"]
    T = np.array(n["table"], float)
    xs = T[:, 0]
    tf, bf, hf = _interp(xs, T[:, 1]), _interp(xs, T[:, 2]), _interp(xs, T[:, 3])
    x0, tip = n["x0"], float(xs[-1])

    def sec(x, u):
        grow = 0.006 + 0.006 * max(0.0, 1.0 - (x - x0) / 0.04)   # a raised collar where the shell meets the hull
        zt, zb, w = tf(x) + grow, bf(x) - grow, hf(x) + grow
        zch = zb + 0.30 * (zt - zb)                               # the chine
        c, s = math.cos(u), math.sin(u)
        if s >= 0:
            return (x, math.copysign(abs(c) ** (2 / 1.7), c) * w, zch + abs(s) ** (2 / 1.7) * (zt - zch))
        return (x, math.copysign(abs(c) ** (2 / 2.6), c) * w, zch - abs(s) ** (2 / 2.6) * (zch - zb))

    seams = ((1.47, 1.476), (1.80, 1.806), (2.10, 2.105))
    pockets = [{"u0": a, "u1": b, "v0": 1.52, "v1": 1.76, "depth": 0.005, "eu": 0.015, "ev": 0.012, "mat": GLOW, "wall_mat": BLACK}
               for a, b in ((0.16, 0.26), (PI - 0.26, PI - 0.16))]      # a thin glow strip low on each flank
    pockets.append({"u0": PI / 2 - 0.20, "u1": PI / 2 + 0.20, "v0": x0 + 0.09, "v1": 2.20, "depth": 0.006, "eu": 0.04, "ev": 0.02,
                    "mat": GUN, "wall_mat": BLACK})                 # the dark band along the top, as on the Rodin nose, recessed

    def mat_fn(u, v):
        c, s = math.cos(u), math.sin(u)
        if v < x0 + 0.035:
            return GUN                                           # the collar
        if any(a <= v <= b for a, b in seams):
            return BLACK
        if v > 2.24:
            return STEEL                                         # the ram spike
        if s < -0.03:
            return GUN                                           # dark underside
        if 0.48 < abs(c) < 0.80 and x0 + 0.07 < v < 2.10:
            return ACCENT                                        # a long accent flash down each flank; it tapers with the nose
        return WHITE
    vs = sorted(set(list(np.linspace(x0, tip, 40)) + [v for ab in seams for v in ab] + [x0 + 0.035, x0 + 0.07, 2.24]))
    nose = G.grid_surface("NS_Nose", lambda u, v: sec(v, u), list(np.linspace(0, 2 * PI, 49)[:-1]), vs, mats, pockets=pockets,
                          closed_u=True, cap0="ngon", cap1="point", mat_fn=mat_fn, smooth_deg=34.0)
    G.bevel(nose, width=0.003, segments=2, angle_deg=34.0)
    gy, gz = n["gun_y"], n["gun_z"]
    cut = _slice(body, lambda c: c.x > x0 - 0.12 and abs(c.y) < 0.30 and math.hypot(abs(c.y) - gy, c.z - gz) > 0.12, x0 + 0.012)
    return [nose], cut


def _nose_rift(mats, body, bvh):
    xs = np.linspace(NOSE_X[0], NOSE_X[1] - 0.04, 14)
    top, bot, half = [], [], []
    for x in xs:
        t, b = _ray(bvh, (x, 0, 5), (0, 0, -1)), _ray(bvh, (x, 0, -5), (0, 0, 1))
        zt = t.z if t else (top[-1] if top else -0.3)
        zb = b.z if (b and b.z < zt - 0.01) else zt - 0.02          # past the Rodin tip top and bottom meet: a point, not a block
        s = _ray(bvh, (x, 0.6, zt - 0.06), (0, -1, 0))          # from inside the gun gap, above the pods
        top.append(zt); bot.append(zb); half.append(min(abs(s.y) * 1.04, 0.24) if s else 0.15)
    top, bot, half = _smooth(top, 1), _smooth(bot, 1), _smooth(half, 1)
    tf, bf, hf = _interp(xs, top), _interp(xs, bot), _interp(xs, half)
    tip = NOSE_X[1]

    def sec(x, u, grow=0.005):
        x = min(x, tip)
        # v08: the last 25 cm taper to the Rodin's low, pointed tip (v07 ended in a blunt block)
        k = 1.0 if x <= tip - 0.25 else max(0.0, (tip - x) / 0.25) ** 0.85
        zt, zb = tf(min(x, 2.30)) + grow, bf(min(x, 2.30)) - grow
        if x > 2.30:                                             # v09: past the Rodin tip, close to a solid point
            zm = (zt + zb) / 2
            zt, zb = zm + (zt - zm) * k, zm - (zm - zb) * k
        w = min(hf(x) * 1.32, 0.27) * k + grow                   # v09: wide enough to cover the cheek shards
        zc = (zt + zb) / 2
        hz = max((zt - zb) / 2, 0.01) * (0.25 + 0.75 * k)
        c, s = math.cos(u), math.sin(u)
        n = 2.6                                                  # rounder than v07's 3.2: sleek, still with chines
        return (x, math.copysign(abs(c) ** (2 / n), c) * w, zc + math.copysign(abs(s) ** (2 / n), s) * hz)
    # panel lines as recessed pockets: a seam ring at two stations, the glowing nose slit on the top
    v_lo, v_hi = NOSE_X[0], tip
    pockets = [{"u0": PI / 2 - 0.10, "u1": PI / 2 + 0.10, "v0": 1.95, "v1": 2.24, "depth": 0.008, "eu": 0.025, "ev": 0.02,
                "mat": HOT}]                                      # v09: one long, narrow glowing slit along the top
    seams = ((1.72, 1.728), (1.90, 1.908), (2.20, 2.208))        # panel seams: clean dark bands

    def mat_fn(u, v):
        s = math.sin(u)
        if any(a <= v <= b for a, b in seams) or abs(s + 0.22) < 0.035:   # station seams and the chine seam
            return BLACK
        if s < -0.25:
            return GUN                                           # dark chin
        if abs(math.cos(u)) < 0.10 and s > 0 and v < 1.90:
            return ACCENT                                        # magenta centre stripe, ending before the slit
        return WHITE
    nose = G.grid_surface("NS_Nose", lambda u, v: sec(v, u), list(np.linspace(0, 2 * PI, 73)[:-1]),
                          sorted(set(list(np.linspace(v_lo, v_hi, 34)) + [v for ab in seams for v in ab])), mats, pockets=pockets, closed_u=True, cap0="none",
                          cap1="point", mat_fn=mat_fn, smooth_deg=34.0)
    G.bevel(nose, width=0.003, segments=2, angle_deg=34.0)
    # the Rodin nose under the shell goes (remove_hidden takes the rest)
    cut = _cut(body, lambda c: (NOSE_X[0] + 0.05 < c.x and abs(c.y) < min(hf(min(c.x, xs[-1])) * 1.32, 0.27) * 0.97 and
               bf(min(c.x, xs[-1])) - 0.01 < c.z < tf(min(c.x, xs[-1])) + 0.01 and math.hypot(abs(c.y) - GUN_Y, c.z - GUN_Z) > 0.13)
               or (c.x > 2.22 and abs(c.y) < 0.30 and c.z > -0.85 and math.hypot(abs(c.y) - GUN_Y, c.z - GUN_Z) > 0.10))
    # v11: whatever of the Rodin nose still pokes out of the shell goes: spikes above it (canopy front, top) and the
    # shards under its lip (the chin between the gun pods), the guns kept
    def poke(c):
        if not (NOSE_X[0] - 0.03 < c.x < NOSE_X[1] + 0.06) or math.hypot(abs(c.y) - GUN_Y, c.z - GUN_Z) < 0.12:
            return False
        xx = min(c.x, xs[-1])
        w = min(hf(xx) * 1.32, 0.27)
        above = abs(c.y) < w * 1.05 and c.z > tf(xx) + 0.004
        below = c.x > 1.95 and abs(c.y) < 0.30 and c.z < bf(xx) + 0.02
        tip = c.x > 2.28 and abs(c.y) < 0.32
        return above or below or tip
    cut += _cut(body, poke)
    return [nose], cut


# ---------------------------------------------------------------------------------------------------- spine
def build_spine(mats, body, bvh):
    xs = np.linspace(SPINE_X[0], SPINE_X[1], 16)
    sp = K["spine"]
    tops = _smooth([_top(bvh, x) or sp["tdef"] for x in xs], 2)
    halfs = []
    for x, t in zip(xs, tops):
        s = _ray(bvh, (x, 3.0, t - 0.03), (0, -1, 0))
        halfs.append(min(abs(s.y), sp["hcap"]) if s else sp["hdef"])
    halfs = _smooth(halfs, 2)
    tf, hf = _interp(xs, tops), _interp(xs, halfs)
    parts = []
    edges = sp["edges"]
    for i in range(len(edges) - 1):
        x0, x1 = edges[i] + 0.006, edges[i + 1] - 0.006

        def f(u, v):
            w = 0.82 * hf(v)
            y = (u - 0.5) * 2 * w
            ridge = 0.022 * (1 - abs(u - 0.5) * 2) ** 0.5
            return (v, y, tf(v) + 0.010 + ridge - 0.03 * (abs(u - 0.5) * 2) ** 3)
        plate = G.grid_surface("SP_Plate_%d" % i, f, list(np.linspace(0, 1, 13)), list(np.linspace(x0, x1, 10)), mats,
                               closed_u=False, cap0="none", cap1="none",
                               mat_fn=lambda u, v: ACCENT if abs(u - 0.5) < 0.07 else WHITE, smooth_deg=30.0)
        sol = plate.modifiers.new("Thick", "SOLIDIFY")
        sol.thickness = 0.014
        sol.offset = -1.0
        G.bevel(plate, width=0.003, segments=2, angle_deg=40.0)
        parts.append(plate)
    return parts


# ---------------------------------------------------------------------------------------------------- side plates
# v10 (Lead GO, Nov 13 night): the fuselage sides are the last thing that reads "melted" at 3 m. Denoise and paint
# passes could not fix them (the lumps are the Rodin shape itself), so clean armour plates go over them, built on
# the measured side surface (rays from inside the body outwards: the skin, not the wings or pods), smoothed and laid a
# few millimetres outside, with real gaps over a dark backing (inset seams), varied sizes, access panels, vents and
# bolts. Bands: UPPER (under the canopy sill and spine, the whole length) and MID (the side pods ahead of the wing).
UPPER = dict(x=[-0.60, -0.31, -0.03, 0.29, 0.56, 0.87, 1.08], z=(-0.29, -0.02), split={2: -0.12, 4: -0.10})
MID = dict(x=[0.22, 0.51, 0.79, 1.05, 1.29, 1.52], z=(-0.62, -0.29), split={1: -0.47, 3: -0.45})
GAP = 0.006


def _side_field(bvh, xs, zs, sy):
    Y = np.full((len(xs), len(zs)), np.nan)
    for i, x in enumerate(xs):
        for j, z in enumerate(zs):
            h = bvh.ray_cast(Vector((x, 0.0, z)), Vector((0, sy, 0)), 1.2)
            if h[0] is not None and abs(h[0].y) < 0.62:
                Y[i, j] = abs(h[0].y)
    for i in range(len(xs)):                                    # per station: outliers (holes, pods poking in) -> median
        col = Y[i]
        med = np.nanmedian(col) if np.isfinite(col).any() else 0.35
        bad = ~np.isfinite(col) | (np.abs(col - med) > 0.10)
        col[bad] = med
    lift = np.maximum.reduce([np.roll(Y, k, a) for a in (0, 1) for k in (-1, 0, 1)])   # cover the lumps
    sm = lift.copy()
    for _ in range(6):                                          # then smooth: clean, continuous panels
        p = np.pad(sm, 1, mode="edge")
        sm = (p[1:-1, 1:-1] * 4 + p[:-2, 1:-1] + p[2:, 1:-1] + p[1:-1, :-2] + p[1:-1, 2:]) / 8.0
    return sm


def _plates_band(mats, bvh, band, sy, name, mat_of, extras):
    xs = np.linspace(band["x"][0] - 0.02, band["x"][-1] + 0.02, 48)
    zs = np.linspace(band["z"][0] - 0.02, band["z"][1] + 0.02, 18)
    field = _side_field(bvh, xs, zs, sy)

    def yat(x, z, off):
        fi = float(np.interp(x, xs, np.arange(len(xs))))
        fj = float(np.interp(z, zs, np.arange(len(zs))))
        i0, j0 = int(min(fi, len(xs) - 2)), int(min(fj, len(zs) - 2))
        a, b = fi - i0, fj - j0
        v = (field[i0, j0] * (1 - a) * (1 - b) + field[i0 + 1, j0] * a * (1 - b) + field[i0, j0 + 1] * (1 - a) * b
             + field[i0 + 1, j0 + 1] * a * b)
        return sy * (v + off)

    def box_out(nm, cx, cz, w, h, depth, mat):
        y = yat(cx, cz, 0.027)
        return G.box(nm, (cx - w / 2, min(y, y + sy * depth), cz - h / 2), (cx + w / 2, max(y, y + sy * depth), cz + h / 2), mats, mat)

    out = []

    def back_f(u, v):                                           # dark backing: the gaps read as inset seams
        z = band["z"][0] + u * (band["z"][1] - band["z"][0])
        return (v, yat(v, z, 0.004), z)
    out.append(G.grid_surface("%s_Back" % name, back_f, list(np.linspace(0, 1, 8)), list(np.linspace(band["x"][0], band["x"][-1], 30)),
                              mats, closed_u=False, cap0="none", cap1="none", mat_fn=lambda u, v: GUN, flip=(sy < 0)))
    for k in range(len(band["x"]) - 1):
        x0, x1 = band["x"][k] + GAP / 2, band["x"][k + 1] - GAP / 2
        rows = [(band["z"][0], band["split"][k]), (band["split"][k], band["z"][1])] if k in band["split"] else [band["z"]]
        for r, (z0, z1) in enumerate(rows):
            z0, z1 = z0 + GAP / 2, z1 - GAP / 2
            m = mat_of(k, r)

            def f(u, v, z0=z0, z1=z1):
                z = z0 + u * (z1 - z0)
                return (v, yat(v, z, 0.016), z)
            pl = G.grid_surface("%s_P%d%d" % (name, k, r), f, list(np.linspace(0, 1, 7)), list(np.linspace(x0, x1, 9)), mats,
                                closed_u=False, cap0="none", cap1="none", mat_fn=lambda u, v, m=m: m, flip=(sy < 0), smooth_deg=40.0)
            sol = pl.modifiers.new("Thick", "SOLIDIFY")
            sol.thickness = 0.011
            sol.offset = -1.0
            G.bevel(pl, width=0.0025, segments=2, angle_deg=40.0)
            out.append(pl)
            for kind, fx, fz, w, h in extras.get((k, r), ()):     # details on this plate, in plate-local fractions
                cx, cz = x0 + fx * (x1 - x0), z0 + fz * (z1 - z0)
                if kind == "access":                             # raised access panel
                    out.append(box_out("%s_A%d%d" % (name, k, r), cx, cz, w, h, -0.010, GUN))
                elif kind == "vent":                             # louvre: dark slots
                    for q in range(5):
                        qz = cz - h / 2 + (q + 0.5) * h / 5
                        out.append(box_out("%s_V%d%d_%d" % (name, k, r, q), cx, qz, w, h / 7, -0.009, BLACK))
                elif kind == "bolts":
                    for bx in (-0.5, 0.5):
                        for bz in (-0.5, 0.5):
                            out.append(box_out("%s_B%d%d" % (name, k, r), cx + bx * w, cz + bz * h, 0.014, 0.014, -0.007, STEEL))
    return out


def build_side_plates(mats, body, bvh):
    parts = []
    for sy in (1, -1):
        up_mat = lambda k, r: WHITE                                  # v10b: white armour; colour stays in the hull livery
        mid_mat = lambda k, r: WHITE
        up_x = {(1, 0): (("access", 0.45, 0.5, 0.12, 0.08),), (3, 0): (("vent", 0.3, 0.5, 0.07, 0.12), ("bolts", 0.5, 0.5, 0.20, 0.14)),
                (5, 0): (("access", 0.6, 0.45, 0.10, 0.06),)}
        mid_x = {(0, 0): (("vent", 0.5, 0.5, 0.12, 0.10),), (1, 1): (("access", 0.5, 0.5, 0.14, 0.07),),
                 (3, 0): (("vent", 0.4, 0.5, 0.09, 0.09),), (4, 0): (("bolts", 0.5, 0.5, 0.14, 0.18),)}
        parts += _plates_band(mats, bvh, UPPER, sy, "SPU%+d" % sy, up_mat, up_x)
        parts += _plates_band(mats, bvh, MID, sy, "SPM%+d" % sy, mid_mat, mid_x)
    return parts


def denoise_sides(body):
    """v08: the cracks left on the fuselage sides are carved into the Rodin geometry. A mild, masked feature-preserving
    denoise (rh_denoise.py) on the side panels only (the full-hull v06 pass smeared steps and showed facets)."""
    import rh_denoise as D
    sd = K["sides"]
    vg = body.vertex_groups.get("CP_Sides") or body.vertex_groups.new(name="CP_Sides")
    idx = []
    for v in body.data.vertices:
        c = v.co
        if sd["x"][0] < c.x < sd["x"][1] and sd["z"][0] < c.z < sd["z"][1] and abs(c.y) > sd["ymin"]:
            idx.append(v.index)
    vg.add(idx, 1.0, "REPLACE")
    co, tris, pairs, me = D._arrays(body)
    # v09 (Lead: "a strong side-panel pass"): flatter panels, wider spatial reach; the cap on vertex travel keeps the form
    return D.denoise(body, sigma_s=sd["sigma_k"] * me, sigma_r=sd["sigma_r"], normal_iters=sd["iters"], vertex_iters=sd["iters"],
                     mask_group="CP_Sides", max_move=sd["max_move"])


def build(mats, body):
    bvh = _bvh(body)                 # measured once, on the untouched hull
    res = {}
    canopy, guns, nose, spine, glass = [], [], [], [], None
    if K.get("canopy"):
        canopy, glass, res["cut_canopy"] = build_canopy(mats, body, bvh)
    if K.get("guns"):
        guns, res["cut_guns"] = build_guns(mats, body)
    if K.get("nose"):
        nose, res["cut_nose"] = build_nose(mats, body, bvh)
    if K.get("spine"):
        spine = build_spine(mats, body, bvh)
    # v10's side plates failed the toy-like gate (Lead, decision A): built only with TD_RIFT_PLATES=1; v11 = v09 sides
    if os.environ.get("TD_RIFT_PLATES"):
        plates = build_side_plates(mats, body, bvh)
    else:
        plates = []
        if K.get("sides"):
            res["side_denoise"] = denoise_sides(body)
    res["side_plates"] = len(plates)
    res["parts"] = len(canopy) + len(guns) + len(nose) + len(spine) + len(plates)
    return canopy + guns + nose + spine + plates, glass, res


# ---------------------------------------------------------------------------------------------------- livery
REPAINT = [  # (x0, x1, |y| max, |y| min, z0, z1): leftover Rodin chrome under the canopy sill and beside the nose
    (0.95, 1.75, 0.42, 0.0, -0.42, 0.30),
    (1.40, 1.95, 0.36, 0.12, -0.48, -0.08),
]


def _paint_mask(body, shape, test):
    """Texels (h, w) covered by the UV triangles of the body faces whose centre passes test(c)."""
    h, w = shape
    mask = np.zeros((h, w), bool)
    me = body.data
    uvl = me.uv_layers.active.data
    for poly in me.polygons:
        c = poly.center
        if not test(c):
            continue
        uv = [uvl[li].uv for li in poly.loop_indices]
        for k in range(1, len(uv) - 1):
            tri = np.array([[uv[0].x, uv[0].y], [uv[k].x, uv[k].y], [uv[k + 1].x, uv[k + 1].y]]) * [w, h]
            x0, y0 = np.floor(tri.min(0)).astype(int)
            x1, y1 = np.ceil(tri.max(0)).astype(int)
            x0, y0, x1, y1 = max(x0, 0), max(y0, 0), min(x1, w - 1), min(y1, h - 1)
            if x1 < x0 or y1 < y0:
                continue
            gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
            a, b, cc = tri
            d = (b[1] - cc[1]) * (a[0] - cc[0]) + (cc[0] - b[0]) * (a[1] - cc[1])
            if abs(d) < 1e-12:
                continue
            l1 = ((b[1] - cc[1]) * (gx - cc[0]) + (cc[0] - b[0]) * (gy - cc[1])) / d
            l2 = ((cc[1] - a[1]) * (gx - cc[0]) + (a[0] - cc[0]) * (gy - cc[1])) / d
            inside = (l1 >= -0.02) & (l2 >= -0.02) & (1 - l1 - l2 >= -0.02)
            mask[y0:y1 + 1, x0:x1 + 1] |= inside
    return mask


def clean_livery(path, crack_px=6, smooth_px=10, body=None):
    """In place: fill the AI's jagged dark 'crack' lines inside white / magenta paint with the surrounding paint, and
    calm the streaks in the paint (within each paint class, blend towards its local smoothed colour)."""
    import rh_textures as T
    a = T.load(path)                                            # linear RGB (h, w, 3)
    lum = a @ np.array([0.2126, 0.7152, 0.0722])
    mx, mn = a.max(-1), a.min(-1)
    sat = (mx - mn) / np.maximum(mx, 1e-4)
    white = (lum > 0.32) & (sat < 0.22)
    if K["accent"] == "cyan":         # the accent paint class; called magenta below (the Rift came first)
        magenta = (sat > 0.45) & (a[..., 2] > a[..., 0] * 2.0) & (a[..., 1] > a[..., 0] * 1.5) & (lum > 0.05)
    elif K["accent"] == "amber":
        magenta = (sat > 0.45) & (a[..., 0] > a[..., 2] * 2.0) & (a[..., 1] > a[..., 2] * 1.3) & (lum > 0.05)
    else:
        magenta = (sat > 0.45) & (a[..., 0] > a[..., 1] * 2.0) & (a[..., 2] > a[..., 1] * 1.3) & (lum > 0.05)
    dark = lum < 0.20

    def box(m, r):
        k = 2 * r + 1
        c = np.cumsum(np.cumsum(np.pad(m.astype(np.float64), ((r + 1, r), (r + 1, r))), 0), 1)
        return (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)

    fw, fm = box(white, crack_px), box(magenta, crack_px)
    crack_w = dark & (fw > 0.38)
    crack_m = dark & (fm > 0.25) & ~crack_w           # v09: blade-edge smears
    out = a.copy()
    for cls, crack in ((white, crack_w), (magenta, crack_m)):
        m = cls.astype(np.float64)[..., None]
        local = np.stack([box(a[..., c] * cls, smooth_px) for c in range(3)], -1) / np.maximum(box(cls, smooth_px)[..., None], 1e-4)
        out = np.where(crack[..., None], local, out)
        keep = cls[..., None]
        k_local = 0.85 if cls is magenta else 0.55               # v08: the magenta blades were still streaky
        out = np.where(keep, a * (1 - k_local) + local * k_local, out)   # streaks calmed, some real shading kept
    if body is not None:
        # leftover chrome around the new canopy and nose: dark texels there become white paint (and non-metal below)
        def test(c):
            return any(x0 < c.x < x1 and ymin <= abs(c.y) < ymax and z0 < c.z < z1 for x0, x1, ymax, ymin, z0, z1 in K["repaint"])
        region = _paint_mask(body, lum.shape, test)
        region = region[::-1] if False else region
        paint_white = np.median(a[white], axis=0) if white.any() else np.array([0.8, 0.8, 0.82])
        rp = region & ((lum < 0.30) | (magenta & K["repaint_accent"]))   # v09: the ragged magenta sill stripe goes white too
        out[rp] = paint_white
        repaint_px = int(rp.sum())
    T.save(out, path, True)
    res = {"white_px": int(white.sum()), "magenta_px": int(magenta.sum()), "cracks_filled": int(crack_w.sum() + crack_m.sum())}
    # the cracks are carved into the normal map too: inside the paint, the normal map's fine band is mostly removed
    # (the tiling detail layer in Unreal brings clean micro detail back); outside the paint (greebles, vents) it stays
    npath = path.replace("_BC.png", "_N.png")
    if os.path.isfile(npath):
        nm = T.load_linear(npath) if hasattr(T, "load_linear") else None
        if nm is None:
            im = bpy.data.images.load(npath, check_existing=False)
            im.colorspace_settings.name = "Non-Color"
            w, h = im.size
            buf = np.empty(w * h * 4, np.float32)
            im.pixels.foreach_get(buf)
            bpy.data.images.remove(im)
            nm = buf.reshape(h, w, 4)[..., :3].astype(np.float64)
        paint = (white | magenta | crack_w | crack_m)
        pm = box(paint, 2)[..., None]
        v = nm * 2.0 - 1.0
        soft = np.stack([box(v[..., c] * paint, 6) for c in range(2)], -1) / np.maximum(box(paint, 6)[..., None], 1e-4)
        xy = v[..., :2] * (1.0 - 0.95 * pm) + soft * 0.95 * pm   # v09: cracks carved into the normal map go
        z = np.sqrt(np.clip(1.0 - (xy ** 2).sum(-1), 0.0, 1.0))
        outn = np.concatenate([xy, z[..., None]], -1) * 0.5 + 0.5
        h, w = outn.shape[:2]
        im = bpy.data.images.new("__n__", w, h, alpha=False)
        im.colorspace_settings.name = "Non-Color"
        px = np.ones((h, w, 4), np.float32)
        px[..., :3] = np.clip(outn, 0.0, 1.0)
        im.pixels.foreach_set(px.ravel())
        im.filepath_raw = npath
        im.file_format = "PNG"
        im.save()
        bpy.data.images.remove(im)
        res["normal_paint_softened_frac"] = round(float(pm.mean()), 3)
    if body is not None:
        res["repainted_px"] = repaint_px
        opath = path.replace("_BC.png", "_ORM.png")
        if os.path.isfile(opath):
            im = bpy.data.images.load(opath, check_existing=False)
            im.colorspace_settings.name = "Non-Color"
            w, h = im.size
            buf = np.empty(w * h * 4, np.float32)
            im.pixels.foreach_get(buf)
            o = buf.reshape(h, w, 4)
            o[rp, 1] = 0.45
            o[rp, 2] = 0.0
            im.pixels.foreach_set(o.ravel())
            im.filepath_raw = opath
            im.file_format = "PNG"
            im.save()
            bpy.data.images.remove(im)
    return res
