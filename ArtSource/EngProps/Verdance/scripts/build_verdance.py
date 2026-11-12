# SPDX-License-Identifier: MIT
"""
build_verdance.py - regenerates the whole Verdance ground prop kit (10 static meshes).

    blender --background --factory-startup --python ArtSource/EngProps/Verdance/scripts/build_verdance.py
    ... -- --stage clay      geometry + a clay contact sheet only (fast, for shape iteration)
    ... -- --stage bake      + shared atlases, bake, collision, checks, contact sheets
    ... -- --stage review    + the per-asset review sheets
    ... -- --stage full      everything (default): + per-asset .blend, FBX, manifest, FBX round trip
    ... -- --fast            quicker, looser UV packing (iteration only; never for the final run)
    ... -- --only Fern,Rock  limit a clay / bake / review run to the assets whose name contains a word

Verdance = bioluminescent jungle: organic, rounded, flowing shapes; teal + violet with cyan, magenta
and lime glow. Everything here is original, code-built geometry (see ../README.md).
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

KIT = "Verdance"
KIT_DIR = pk.KIT_DIR
TEX_DIR = os.path.join(KIT_DIR, "textures")
REVIEW_DIR = os.path.join(KIT_DIR, "review")

# ---------------------------------------------------------------- palette (sRGB hex -> linear)
TEAL = pk.hexc("#19C3B1")
TEAL_DEEP = pk.hexc("#0F3B3A")
TEAL_MID = pk.hexc("#0E7C78")
VIOLET = pk.hexc("#7B3FE4")
INDIGO = pk.hexc("#2A1A6E")
PLUM = pk.hexc("#4A1F8C")
STEM_PALE = pk.hexc("#8ADCD0")
STEM_SHADE = pk.hexc("#234E86")
BIO = pk.hexc("#3CFFE0")
MAGENTA = pk.hexc("#FF2BD6")
LIME = pk.hexc("#B8FF3A")
GILL_DARK = pk.hexc("#0A2A33")
STONE = pk.hexc("#565C78")
STONE_WARM = pk.hexc("#6A5F86")
MOSS_DARK = pk.hexc("#0B4A45")
MOSS_LIGHT = pk.hexc("#1FB59A")

TAU = math.tau
EMISSION_STRENGTH = 2.0


def bez(p0, p1, p2, p3, n):
    out = []
    for i in range(n):
        t = i / float(n - 1)
        u = 1.0 - t
        out.append(Vector(p0) * (u ** 3) + Vector(p1) * (3 * u * u * t) + Vector(p2) * (3 * u * t * t)
                   + Vector(p3) * (t ** 3))
    return out


def arc_path(base, azimuth, length, a0, a1, n, curl=None, ease=1.0):
    """Planar spine: starts at `base`, leaves at angle a0 from vertical and bends to a1 (degrees).
    curl=(start_t, extra_deg, shrink) winds the tip into a spiral."""
    rad = Vector((math.cos(azimuth), math.sin(azimuth), 0.0))
    up = Vector((0.0, 0.0, 1.0))
    pts = [Vector(base)]
    ds = length / float(n - 1)
    for i in range(1, n):
        t = (i - 0.5) / float(n - 1)
        a = pk.lerp(a0, a1, t ** ease)
        step = ds
        if curl is not None and t > curl[0]:
            k = (t - curl[0]) / (1.0 - curl[0])
            a += curl[1] * k ** 1.25
            step = ds * pk.lerp(1.0, curl[2], k)
        ar = math.radians(a)
        pts.append(pts[-1] + (rad * math.sin(ar) + up * math.cos(ar)) * step)
    return pts, Vector((-math.sin(azimuth), math.cos(azimuth), 0.0))


# ============================================================================ mushroom parts

class Cap:
    """Lathed mushroom cap with a scalloped rim and ridged gills underneath. Only the gill ridges and
    the rim glow; the membrane between the ridges is dark."""

    def __init__(self, R, bot, top, sides, scallop=6, amp_r=0.05, amp_z=0.03, phase=0.0, gill=0.02):
        self.R, self.bot, self.top, self.sides = R, bot, top, sides
        self.n, self.amp_r, self.amp_z, self.ph, self.gill = scallop, amp_r, amp_z, phase, gill
        self.nb = len(bot)

    def profile(self):
        return [(r * self.R, z) for r, z in self.bot] + [(r * self.R, z) for r, z in self.top]

    def rmod(self, a, k, r, z):
        w = (r / self.R) ** 2
        return 1.0 + self.amp_r * w * (math.sin(self.n * a + self.ph) + 0.35 * math.sin((2 * self.n + 1) * a + 1.7 * self.ph))

    def zmod(self, a, k, r, z):
        w = (r / self.R) ** 2
        dz = -self.amp_z * w * (0.5 + 0.5 * math.sin(self.n * a + self.ph + 1.0))
        if 0 < k < self.nb:
            j = int(round(a / TAU * self.sides))
            if j % 2:
                dz -= self.gill * min(1.0, r / self.R * 1.3)
        return dz

    def ztop(self, rho):
        pts = self.top
        for (r0, z0), (r1, z1) in zip(pts[:-1], pts[1:]):
            lo, hi = min(r0, r1), max(r0, r1)
            if lo <= rho <= hi and hi > lo:
                return z0 + (z1 - z0) * (rho - r0) / (r1 - r0)
        return pts[-1][1]

    def surf(self, rho, th):
        r = rho * self.R
        rr = r * self.rmod(th, self.nb, r, 0.0)
        return Vector((rr * math.cos(th), rr * math.sin(th), self.ztop(rho) + self.zmod(th, self.nb, r, 0.0)))

    def normal(self, rho, th):
        e = 0.02
        a = self.surf(min(rho + e, 0.999), th) - self.surf(max(rho - e, 0.0), th)
        b = self.surf(rho, th + e) - self.surf(rho, th - e)
        n = a.cross(b)
        return n.normalized() if n.length > 1e-9 else Vector((0, 0, 1))

    def build(self, b, xf, top_inner, top_outer, gill_col, rim_glow=0.8, top_rough=0.55, tag=1, gill_gain=0.6):
        prof = self.profile()
        faces = b.lathe(prof, sides=self.sides, xf=xf, rmod=self.rmod, zmod=self.zmod, tag=tag)
        n = len(prof)
        nb, R = self.nb, self.R

        def fn(v, f):
            k = int(round(b.t(v) * (n - 1)))
            rho = prof[k][0] / R
            fk = f[b.fk]
            if 0 <= fk < nb - 1:  # underside: glow only along the gill ridges (Par.b), narrowed in the material
                ridge = int(round(b.u(v) * self.sides)) % 2 == 1 and k > 0
                g = pk.mulc(gill_col, gill_gain * pk.smooth01(rho, 0.36, 0.97))
                return {"col": pk.mixc(GILL_DARK, TEAL_DEEP, 0.5 if ridge else 0.15), "rough": 0.7, "glow": g,
                        "noise": 0.25, "bump": 0.2, "mask": 1.0 if ridge else 0.0, "mask2": 1.0}
            col = pk.mixc(top_inner, top_outer, pk.smooth01(rho, 0.15, 0.95))
            glow = pk.mulc(gill_col, rim_glow) if k == nb else (0, 0, 0)
            return {"col": col, "rough": top_rough, "glow": glow, "noise": 0.6, "bump": 0.22}
        b.paint(faces, fn=fn)
        return faces

    def spots(self, b, xf, count, color, rho_range=(0.2, 0.8), size=(0.05, 0.1), seed=1, sides=8, tag=1):
        rng = random.Random(seed)
        for i in range(count):
            rho = pk.lerp(rho_range[0], rho_range[1], math.sqrt((i + 0.5) / count))
            th = i * 2.39996 + rng.uniform(-0.25, 0.25)
            r = self.R * rng.uniform(*size) * (1.0 - 0.35 * rho)
            wp = xf @ self.surf(rho, th)
            wn = (xf.to_3x3() @ self.normal(rho, th)).normalized()
            fs = b.dome(wp, wn, r, r * 0.45, sides=sides, tag=tag)
            n_ring = 3.0  # dome profile has 4 points: bright crown, dimmer skirt

            def fn(v, f, _c=color):
                k = b.t(v)
                return {"col": pk.mulc(pk.mixc(_c, (1, 1, 1), 0.3), 0.55), "rough": 0.3,
                        "glow": pk.mulc(_c, 0.25 + 0.65 * pk.smooth01(k, 1.0 / n_ring, 1.0)), "noise": 0.1,
                        "bump": 0.05}
            b.paint(fs, fn=fn)


def parasol(dome, droop, thick, lip=0.035, p=2.2):
    """(bottom, top) profile lists in (rho, z): a shallow umbrella. droop < 0 turns the rim up."""
    def zt(r):
        return dome * (1.0 - r ** p) - droop * r ** 3

    def zb(r):
        return zt(r) - (lip + (thick - lip) * (1.0 - r) ** 1.2)
    bot = [(0.0, zb(0.0)), (0.3, zb(0.3)), (0.62, zb(0.62)), (0.86, zb(0.86)), (0.97, zb(0.97))]
    top = [(1.0, (zt(1.0) + zb(1.0)) * 0.5), (0.97, zt(0.97)), (0.84, zt(0.84)), (0.64, zt(0.64)),
           (0.42, zt(0.42)), (0.2, zt(0.2)), (0.0, zt(0.0))]
    return bot, top


def bell(H):
    bot = [(0.0, 0.76 * H), (0.3, 0.68 * H), (0.56, 0.44 * H), (0.79, 0.17 * H), (0.94, 0.03 * H)]
    top = [(1.0, 0.0), (1.05, 0.05 * H), (0.98, 0.15 * H), (0.87, 0.35 * H), (0.71, 0.58 * H),
           (0.5, 0.8 * H), (0.28, 0.94 * H), (0.11, 1.0 * H), (0.0, 1.04 * H)]
    return bot, top


def stem(b, pts, r_base, r_mid, r_top, sides=9, buttress=0.0, lobes=5, phase=0.0, tag=0,
         pale=STEM_PALE, shade=STEM_SHADE):
    n = len(pts)

    def rad(i):
        t = i / float(n - 1)
        return r_mid + (r_base - r_mid) * (1.0 - pk.smooth01(t, 0.0, 0.3)) + (r_top - r_mid) * pk.smooth01(t, 0.55, 1.0)

    def rmod(i, j):
        t = i / float(n - 1)
        a = TAU * j / sides
        return 1.0 + buttress * max(0.0, math.cos(lobes * a + phase)) * (1.0 - pk.smooth01(t, 0.0, 0.28))
    faces = b.tube(pts, [rad(i) for i in range(n)], sides=sides, rmod=rmod, tag=tag)

    def fn(v, f):
        t = b.t(v)
        col = pk.mixc(shade, pale, pk.smooth01(t, 0.02, 0.5))
        ring = int(round(t * (n - 1)))
        if ring % 2 == 1 and 0.12 < t < 0.9:  # soft growth bands
            col = pk.mulc(pk.mixc(col, VIOLET, 0.18), 0.8)
        return {"col": col, "rough": 0.62, "noise": 0.55, "bump": 0.3}
    b.paint(faces, fn=fn)
    return faces


def mound(b, R, h, seed=0, col=None, depth=0.1, sides=16, tag=0):
    """Low mossy swell the stems grow from. Its rim is below the ground plane."""
    rng = random.Random(seed)
    ph = [rng.uniform(0, TAU) for _ in range(3)]

    def rmod(a, k, r, z):
        return 1.0 + 0.13 * math.sin(3 * a + ph[0]) + 0.07 * math.sin(5 * a + ph[1])

    def zmod(a, k, r, z):
        return 0.0 if z <= -0.04 else 0.3 * h * math.sin(2 * a + ph[2]) * (r / R)
    prof = [(0.0, -depth), (R * 0.95, -depth), (R, -0.045), (R * 0.72, h * 0.5), (R * 0.36, h * 0.95), (0.0, h)]
    faces = b.lathe(prof, sides=sides, rmod=rmod, zmod=zmod, tag=tag)
    base = col or pk.mixc(MOSS_DARK, MOSS_LIGHT, 0.3)
    b.paint(faces, fn=lambda v, f: {"col": pk.mixc(pk.mulc(base, 0.6), base, pk.smooth01(v.co.z, -0.03, h)),
                                    "rough": 0.92, "noise": 0.9, "bump": 0.8})
    return faces


def cap_xf(origin, tangent):
    return Matrix.Translation(Vector(origin)) @ pk.rot_to(tangent)


def teardrop(b, at, direction, scale, color, sides=8, tag=0, peak=0.5, body=PLUM):
    """Glowing bulb hanging from `at` along `direction`: bright belly, dim neck and tip, so it keeps its form."""
    prof = [(0.0, -0.012), (0.026, 0.012), (0.048, 0.05), (0.056, 0.095), (0.046, 0.14), (0.023, 0.172),
            (0.0, 0.19)]
    if sides < 8:  # the small lures keep the lighter profile
        prof = [(0.0, -0.012), (0.03, 0.02), (0.055, 0.075), (0.047, 0.125), (0.02, 0.165), (0.0, 0.185)]
    fs = b.lathe([(r * scale, z * scale) for r, z in prof], sides=sides, xf=cap_xf(at, direction), tag=tag)

    def fn(v, f):
        g = max(0.0, math.sin(math.pi * min(1.0, b.t(v) * 1.05))) ** 1.8
        rib = 1.0 if int(round(b.u(v) * sides)) % 2 == 0 else 0.3
        return {"col": pk.mixc(body, pk.mixc(color, (1, 1, 1), 0.15), (0.2 + 0.5 * g) * (0.6 + 0.4 * rib)),
                "rough": 0.28, "glow": pk.mulc(color, (0.03 + peak * g) * rib), "noise": 0.2, "bump": 0.08}
    b.paint(fs, fn=fn)
    return fs


def small_mushroom(b, base, height, R, lean, seed, top_inner, top_outer, gill_col, kind="parasol", tag=2, spots=0,
                   spot_col=None):
    """Companion mushroom: short curved stem + cap. `lean` = horizontal offset of the top."""
    base = Vector(base)
    top = base + Vector((lean[0], lean[1], height))
    tan = Vector((lean[0] * 0.9, lean[1] * 0.9, height * 0.55)).normalized()
    if kind == "bell":
        H = R * 1.25
        bot, tp = bell(H)
        cap = Cap(R, bot, tp, 14, scallop=5, amp_r=0.03, amp_z=0.02 * R, phase=seed, gill=0.012)
        origin = top - tan * H
    else:
        bot, tp = parasol(R * 0.42, R * 0.22, R * 0.4, lip=0.02)
        cap = Cap(R, bot, tp, 14, scallop=5, amp_r=0.05, amp_z=0.05 * R, phase=seed, gill=0.012)
        origin = top - tan * (R * 0.42)
    end = origin + tan * (bot[0][1] + 0.02)
    pts = bez(base + Vector((0, 0, -0.06)), base + Vector((0, 0, height * 0.35)),
              end - tan * height * 0.3, end, 6)
    stem(b, pts, R * 0.3, R * 0.17, R * 0.15, sides=7, tag=tag)
    xf = cap_xf(origin, tan)
    cap.build(b, xf, top_inner, top_outer, gill_col, tag=tag)
    if spots:
        cap.spots(b, xf, spots, spot_col or gill_col, seed=seed + 5, sides=6, tag=tag, size=(0.08, 0.13))


# ============================================================================ mushrooms

def mushroom_01(coll):
    """~120 cm. Flat shelf: a wide dished table cap on a stout stem, cyan spots, two small companions."""
    b = pk.Builder()
    mound(b, 0.5, 0.07, seed=1)
    tan = Vector((0.1, 0.04, 1.0)).normalized()
    R = 0.66
    bot, top = parasol(0.05, -0.1, 0.17, lip=0.045, p=2.6)
    cap = Cap(R, bot, top, 28, scallop=7, amp_r=0.055, amp_z=0.04, phase=0.6, gill=0.02)
    origin = Vector((0.1, 0.03, 1.0))
    end = origin + tan * (bot[0][1] + 0.03)
    pts = bez((0, 0, -0.1), (-0.05, 0.0, 0.35), end - tan * 0.35, end, 9)
    stem(b, pts, 0.24, 0.115, 0.15, sides=10, buttress=0.18, lobes=4, tag=0)
    xf = cap_xf(origin, tan)
    cap.build(b, xf, PLUM, VIOLET, BIO, rim_glow=0.9)
    cap.spots(b, xf, 12, BIO, seed=3, size=(0.045, 0.09))
    small_mushroom(b, (0.36, -0.2, 0.0), 0.44, 0.2, (0.16, -0.1), 4, PLUM, VIOLET, BIO, spots=4)
    small_mushroom(b, (-0.3, 0.26, 0.0), 0.27, 0.13, (-0.09, 0.08), 9, PLUM, VIOLET, BIO)
    return b.finish("SM_Verd_Mushroom_01", coll, sharp_angle=75)


def mushroom_02(coll):
    """~250 cm. Bell: a tall slender leaning stem with a deep bell cap, magenta gill ridges seen from below."""
    b = pk.Builder()
    b.hull_tags = {0}
    mound(b, 0.42, 0.08, seed=2)
    H, R = 0.62, 0.52
    bot, top = bell(H)
    cap = Cap(R, bot, top, 26, scallop=5, amp_r=0.035, amp_z=0.035, phase=1.3, gill=0.018)
    tan = Vector((0.2, -0.08, 1.0)).normalized()
    origin = Vector((0.3, -0.12, 1.9))
    end = origin + tan * (bot[0][1] + 0.03)
    pts = bez((0, 0, -0.1), (-0.14, 0.06, 0.8), end - tan * 0.7, end, 12)
    stem(b, pts, 0.17, 0.075, 0.062, sides=8, buttress=0.2, lobes=3, tag=0)
    xf = cap_xf(origin, tan)
    cap.build(b, xf, TEAL_MID, VIOLET, MAGENTA, rim_glow=0.9)
    cap.spots(b, xf, 10, MAGENTA, rho_range=(0.35, 0.86), seed=7, size=(0.05, 0.09))
    # drooping skirt two thirds of the way up the stem
    i = 8
    t = (pts[i + 1] - pts[i - 1]).normalized()
    sk = [(0.0, -0.07), (0.19, -0.13), (0.215, -0.11), (0.08, 0.035), (0.0, 0.05)]
    fs = b.lathe(sk, sides=14, xf=cap_xf(pts[i], t), tag=1,
                 rmod=lambda a, k, r, z: 1.0 + 0.1 * (r / 0.2) * math.sin(6 * a))

    def skirt(v, f):
        k = int(round(b.t(v) * 4))
        return {"col": pk.mixc(STEM_PALE, VIOLET, 0.4), "rough": 0.55,
                "glow": pk.mulc(MAGENTA, 0.35) if k == 2 else (0, 0, 0), "noise": 0.4, "bump": 0.3}
    b.paint(fs, fn=skirt)
    small_mushroom(b, (-0.3, -0.16, 0.0), 0.78, 0.2, (-0.16, -0.1), 3, TEAL_MID, VIOLET, MAGENTA, kind="bell")
    small_mushroom(b, (0.22, 0.26, 0.0), 0.36, 0.12, (0.08, 0.1), 6, TEAL_MID, VIOLET, MAGENTA, kind="bell")
    return b.finish("SM_Verd_Mushroom_02", coll, sharp_angle=75)


def mushroom_03(coll):
    """~450 cm. Layered: a thick buttressed trunk with three stacked umbrella tiers. The player walks under
    it, so the glow is where they look: gill ridges, rims, lures hanging from the rims, brackets on the trunk."""
    b = pk.Builder()
    b.hull_tags = {0}
    mound(b, 0.85, 0.1, seed=3, sides=12)
    pts = bez((0, 0, -0.12), (-0.18, -0.1, 1.5), (0.5, 0.2, 2.9), (0.34, 0.14, 4.36), 15)
    stem(b, pts, 0.44, 0.2, 0.1, sides=10, buttress=0.32, lobes=5, phase=0.7, tag=0)
    tiers = [(8, 1.5, 0.3, 0.2, 0.3, 24, 7, 0.0), (11, 1.02, 0.26, 0.13, 0.22, 20, 6, 1.1),
             (14, 0.62, 0.3, 0.08, 0.18, 16, 5, 2.3)]
    for n, (i, R, dome, droop, thick, sides, sc, ph) in enumerate(tiers):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[i - 1]).normalized()
        bot, top = parasol(dome, droop, thick, lip=0.04)
        cap = Cap(R, bot, top, sides, scallop=sc, amp_r=0.05, amp_z=0.07 * R, phase=ph, gill=0.03 * R)
        xf = cap_xf(pts[i] - t * (dome * 0.5), t)
        cap.build(b, xf, pk.mixc(TEAL_DEEP, INDIGO, 0.45 - 0.1 * n), pk.mixc(TEAL_MID, TEAL, 0.25 + 0.2 * n), BIO,
                  rim_glow=0.9)
        if n == 0:
            cap.spots(b, xf, 5, LIME, rho_range=(0.35, 0.8), seed=20, size=(0.04, 0.07), sides=6)
        rng = random.Random(50 + n)
        count = [5, 3, 0][n]
        for k in range(count):  # lures: curved threads of different lengths, each ending in a glowing bulb
            th = TAU * (k + 0.5) / count + rng.uniform(-0.25, 0.25)
            p0 = xf @ (cap.surf(0.9, th) - Vector((0, 0, 0.03)))
            L = rng.uniform(0.22, 0.85) * (1.0 if n == 0 else 0.7)
            sway = Vector((rng.uniform(-0.1, 0.1), rng.uniform(-0.1, 0.1), 0.0))
            sp = [p0 + Vector((0, 0, 0.03)), p0 - Vector((0, 0, L * 0.3)) + sway * 0.1,
                  p0 - Vector((0, 0, L * 0.68)) + sway * 0.75, p0 - Vector((0, 0, L)) + sway]
            fs = b.tube(sp, [0.014, 0.011, 0.009, 0.008], sides=4, tag=1)
            b.paint(fs, fn=lambda v, f: {"col": pk.mixc(INDIGO, STEM_PALE, 0.3 + 0.4 * b.t(v)), "rough": 0.5,
                                         "glow": pk.mulc(LIME, 0.12 * b.t(v)), "noise": 0.3, "bump": 0.15})
            teardrop(b, sp[-1] + Vector((0, 0, 0.005)), (sway.x * 0.5, sway.y * 0.5, -1.0),
                     rng.uniform(0.85, 1.25), LIME, sides=6, tag=1, peak=0.6, body=TEAL_MID)
    # bracket shelves on the trunk at eye level
    for i, az, R in ((3, 0.6, 0.26), (4, 3.4, 0.2)):
        t = (pts[i + 1] - pts[i - 1]).normalized()
        out = Vector((math.cos(az), math.sin(az), 0.0))
        out = (out - t * out.dot(t)).normalized()
        rad = 0.2 + (0.44 - 0.2) * (1.0 - pk.smooth01(i / 14.0, 0.0, 0.3))
        prof = [(0.0, -0.06), (R, -0.015), (R * 0.93, 0.03), (R * 0.45, 0.075), (0.0, 0.085)]
        fs = b.lathe(prof, sides=10, xf=cap_xf(pts[i] + out * (rad * 0.8), t * 0.9 + out * 0.35), tag=1,
                     rmod=lambda a, k, r, z: 1.0 + 0.08 * math.sin(5 * a))

        def bracket(v, f):
            k = int(round(b.t(v) * 4))
            return {"col": pk.mixc(TEAL_MID, VIOLET, 0.25 + 0.15 * k), "rough": 0.55,
                    "glow": pk.mulc(BIO, 0.75) if k == 1 else (0, 0, 0), "noise": 0.5, "bump": 0.3}
        b.paint(fs, fn=bracket)
    return b.finish("SM_Verd_Mushroom_03", coll, sharp_angle=75)


# ============================================================================ fronds and plants

def frond(b, base, az, length, a0, a1, width, n=10, wave=0.14, waves=3.0, tip_glow=BIO, tag=0, dark=TEAL_DEEP,
          mid=TEAL_MID, tip=TEAL, thick=0.012, ease=1.0, rib=VIOLET, glow_from=0.72, cup=0.5, bend=0.0, phase=0.0,
          rib_glow=0.14):
    """One frond: a lance blade folded into a V along its midrib (so it has depth edge-on), with a gently
    waved edge, a glowing midrib and a glowing tip. `bend` sweeps it sideways (metres at the tip)."""
    pts, side = arc_path(base, az, length, a0, a1, n + 1, ease=ease)
    if bend:
        pts = [p + side * (bend * (i / float(n)) ** 2) for i, p in enumerate(pts)]

    def width_at(i):
        t = i / float(n)
        if i == n:
            return 0.0
        env = (0.18 + 0.82 * max(0.0, math.sin(math.pi * t ** 0.75)) ** 0.9) * (1.0 - t ** 6)
        return width * env * (1.0 + wave * math.sin(TAU * waves * t + phase))
    faces = b.blade(pts, [width_at(i) for i in range(n + 1)], side, thick=lambda t: thick * (1.0 - 0.6 * t),
                    cup=lambda t: cup * (1.0 - 0.5 * t), tag=tag)

    def fn(v, f):
        t = b.t(v)
        u = b.u(v)
        col = pk.mixc(pk.mixc(dark, mid, pk.smooth01(t, 0.0, 0.5)), tip, pk.smooth01(t, 0.55, 1.0) * 0.7)
        glow = pk.mulc(tip_glow, 0.7 * pk.smooth01(t, glow_from, 1.0))
        if abs(u - 0.25) < 0.01:  # top ridge = midrib
            col = pk.mixc(col, rib, 0.6)
            glow = pk.mulc(tip_glow, rib_glow + (0.9 - rib_glow) * pk.smooth01(t, glow_from, 1.0))
        elif abs(u - 0.75) < 0.01:  # keel underneath
            col = pk.mulc(col, 0.55)
        return {"col": col, "rough": 0.62, "glow": glow, "noise": 0.45, "bump": 0.2}
    b.paint(faces, fn=fn)
    return faces


def crown(b, R, seed=0):
    """Small root flare the fronds leave from. Nearly all of it is below the ground plane."""
    return mound(b, R, 0.022, seed=seed, col=pk.mixc(TEAL_DEEP, TEAL_MID, 0.35), depth=0.05, sides=8)


def crozier(b, base, az, height, lean, r0, turns=1.6, width=0.05, glow=MAGENTA, n_stalk=5, n_curl=16, tag=0):
    """Fiddlehead: a round tapering stalk that winds into a spiral at the top, with a glowing bud at its heart."""
    rad = Vector((math.cos(az), math.sin(az), 0.0))
    up = Vector((0.0, 0.0, 1.0))
    side = Vector((-math.sin(az), math.cos(az), 0.0))
    base = Vector(base)
    top = base + up * (height - r0 * 1.9) + rad * lean
    pts = bez(base + up * -0.04, base + up * height * 0.4 - rad * lean * 0.5, top - up * height * 0.3, top, n_stalk)
    c = top + rad * r0
    phi_max = TAU * turns
    for i in range(1, n_curl + 1):
        k = i / float(n_curl)
        phi = phi_max * k ** 0.9
        pts.append(c + (-rad * math.cos(phi) + up * math.sin(phi)) * (r0 * (1.0 - 0.8 * k)))
    n = len(pts) - 1
    radii = [width * 0.62 * (1.0 - 0.68 * i / float(n)) for i in range(n + 1)]
    faces = b.tube(pts, radii, sides=5, side=side, tag=tag)

    def fn(v, f):
        t = b.t(v)
        col = pk.mixc(TEAL_DEEP, pk.mixc(TEAL_MID, VIOLET, 0.5), pk.smooth01(t, 0.0, 0.7))
        g = pk.smooth01(t, 0.4, 0.95)
        if f[b.fj] in (0, 4):  # the two faces on one flank of the coil carry the glowing line
            return {"col": pk.mixc(col, glow, 0.45), "rough": 0.5, "glow": pk.mulc(glow, 0.08 + 0.55 * g),
                    "noise": 0.4, "bump": 0.3}
        return {"col": col, "rough": 0.5, "glow": pk.mulc(glow, 0.06 * g), "noise": 0.4, "bump": 0.3}
    b.paint(faces, fn=fn)
    # the bud at the centre of the spiral: bright heart, dim rim
    prof = [(0.0, -r0 * 0.32), (r0 * 0.3, -r0 * 0.14), (r0 * 0.34, r0 * 0.1), (0.0, r0 * 0.34)]
    fs = b.lathe(prof, sides=8, xf=Matrix.Translation(pts[-1]) @ pk.rot_to(side), tag=tag)
    b.paint(fs, fn=lambda v, f: {"col": pk.mixc(PLUM, glow, 0.5), "rough": 0.3,
                                 "glow": pk.mulc(glow, 0.2 + 0.6 * abs(b.t(v) * 2.0 - 1.0) ** 2), "noise": 0.1,
                                 "bump": 0.1})
    return faces


def fern_01(coll):
    """~80 cm. An uneven fountain of ten arching fronds with scalloped edges; only the tips glow."""
    b = pk.Builder()
    rng = random.Random(11)
    crown(b, 0.1, seed=4)
    az = 0.4
    for i in range(7):  # outer: arch up and over, tips droop below the middle of the blade
        az += TAU / 7 * rng.uniform(0.7, 1.3)
        L = [1.42, 1.02, 1.28, 0.96, 1.36, 1.08, 1.2][i]
        base = Vector((math.cos(az) * 0.045, math.sin(az) * 0.045, -0.02))
        frond(b, base, az, L, rng.uniform(8, 16), rng.uniform(138, 156), 0.1 + 0.03 * L, n=14, ease=1.25,
              bend=rng.uniform(-0.16, 0.16), phase=rng.uniform(0, TAU), wave=0.24, waves=3.5, glow_from=0.84,
              rib_glow=0.03)
    for i in range(3):  # inner: short and upright, lime tips
        az = TAU * (i + 0.5) / 3 + rng.uniform(-0.3, 0.3)
        base = Vector((math.cos(az) * 0.02, math.sin(az) * 0.02, 0.0))
        frond(b, base, az, [1.0, 0.78, 0.88][i], rng.uniform(3, 9), rng.uniform(46, 64), 0.08, n=9,
              tip_glow=LIME, glow_from=0.8, ease=1.4, bend=rng.uniform(-0.06, 0.06), waves=2.0, rib_glow=0.03)
    return b.finish("SM_Verd_Fern_01", coll, sharp_angle=80)


def fern_02(coll):
    """~160 cm. Tall croziers: fiddleheads wound into glowing magenta spirals over a few open fronds."""
    b = pk.Builder()
    rng = random.Random(23)
    crown(b, 0.14, seed=7)
    for i, (az, L) in enumerate([(0.3, 1.5), (1.75, 1.15), (3.3, 1.38), (4.9, 1.0)]):
        az += rng.uniform(-0.15, 0.15)
        base = Vector((math.cos(az) * 0.08, math.sin(az) * 0.08, -0.02))
        frond(b, base, az, L, rng.uniform(16, 24), rng.uniform(102, 120), 0.1 + 0.045 * L, n=9, ease=1.5,
              bend=rng.uniform(-0.2, 0.2), phase=rng.uniform(0, TAU))
    specs = [(1.74, 0.18, 0.1, 16, 1.6), (1.4, 0.15, 0.17, 14, 1.5), (1.06, 0.12, 0.2, 12, 1.4),
             (0.74, 0.095, 0.2, 10, 1.3)]
    for i, (h, r0, lean, n_curl, turns) in enumerate(specs):
        az = TAU * i / 4 + rng.uniform(-0.3, 0.3) + 0.9
        base = Vector((math.cos(az) * 0.04, math.sin(az) * 0.04, 0.0))
        crozier(b, base, az, h, lean, r0, turns=turns, width=0.055 + 0.014 * (3 - i), n_curl=n_curl)
    return b.finish("SM_Verd_Fern_02", coll, sharp_angle=80)


def glowplant_01(coll):
    """~58 cm. Lantern bulbs: smooth arching stalks hung with magenta teardrops over a leaf rosette."""
    b = pk.Builder()
    rng = random.Random(31)
    crown(b, 0.07, seed=9)
    for i in range(5):
        az = TAU * i / 5 + rng.uniform(-0.3, 0.3)
        base = Vector((math.cos(az) * 0.03, math.sin(az) * 0.03, -0.01))
        frond(b, base, az, rng.uniform(0.24, 0.36), rng.uniform(45, 60), rng.uniform(96, 112),
              rng.uniform(0.055, 0.07), n=5, wave=0.0, tip_glow=BIO, glow_from=0.85, thick=0.01, cup=0.4)
    stalks = [(0.94, 0.0, 1.4), (0.78, 2.3, 1.22), (0.64, 4.3, 1.1), (0.5, 1.1, 0.98), (0.36, 3.4, 0.82)]
    for L, az, s in stalks:
        az += rng.uniform(-0.2, 0.2)
        base = Vector((math.cos(az) * 0.025, math.sin(az) * 0.025, 0.0))
        pts, _side = arc_path(base, az, L, rng.uniform(4, 10), 168.0, 12, ease=1.7)
        fs = b.tube(pts, [pk.lerp(0.015, 0.007, k / 11.0) for k in range(12)], sides=4, tag=0)
        b.paint(fs, fn=lambda v, f: {"col": pk.mixc(TEAL_DEEP, VIOLET, b.t(v) * 0.7), "rough": 0.5,
                                     "noise": 0.3, "bump": 0.3})
        d = (pts[-1] - pts[-2]).normalized()
        teardrop(b, pts[-1] - d * 0.004, d, s, MAGENTA, sides=8, peak=0.32)
    return b.finish("SM_Verd_GlowPlant_01", coll, sharp_angle=80)


def glowplant_02(coll):
    """~56 cm. Spiral pod: a round tapering corkscrew stem beaded with lime pods, curled tendrils at its foot."""
    b = pk.Builder()
    rng = random.Random(41)
    crown(b, 0.07, seed=13)
    n = 26
    pts = []
    for i in range(n):
        t = i / float(n - 1)
        ang = TAU * 2.4 * t ** 0.9
        rad = 0.14 * (1.0 - t) ** 0.75 + 0.012
        pts.append(Vector((math.cos(ang) * rad, math.sin(ang) * rad, 0.56 * t ** 0.85)))
    radii = [pk.lerp(0.04, 0.009, (i / float(n - 1)) ** 0.8) for i in range(n)]
    radii[-1] = 0.0
    pts[0] = Vector((pts[0].x * 0.6, pts[0].y * 0.6, -0.05))
    fs = b.sweep(pts, lambda i, t: None if radii[i] <= 0 else [
        (radii[i] * math.cos(TAU * (j + 0.5) / 8), radii[i] * math.sin(TAU * (j + 0.5) / 8)) for j in range(8)],
        side=(0, 0, 1), tag=0)

    def helix(v, f):
        t = b.t(v)
        line = f[b.fj] == 7  # the one narrow face along the upper side of the coil
        col = pk.mixc(TEAL_DEEP, pk.mixc(TEAL_MID, VIOLET, 0.5), pk.smooth01(t, 0.0, 0.8))
        return {"col": pk.mixc(col, LIME, 0.3) if line else col, "rough": 0.45,
                "glow": pk.mulc(LIME, (0.12 + 0.4 * t) if line else 0.0), "noise": 0.35, "bump": 0.25}
    b.paint(fs, fn=helix)
    for k in range(8):  # pods on the outside of the coil, smaller toward the tip
        t = 0.1 + 0.82 * k / 7.0
        i = int(t * (n - 1))
        p = pts[i]
        out = Vector((p.x, p.y, 0.0))
        out = (out.normalized() if out.length > 1e-4 else Vector((1, 0, 0))) * 0.8 + Vector((0, 0, 0.6))
        s = pk.lerp(1.0, 0.45, t)
        prof = [(0.0, -0.01), (0.03 * s, 0.012 * s), (0.042 * s, 0.05 * s), (0.024 * s, 0.09 * s), (0.0, 0.11 * s)]
        fs = b.lathe(prof, sides=7, xf=cap_xf(p + out.normalized() * radii[i] * 0.6, out), tag=0)
        b.paint(fs, fn=lambda v, f: {"col": pk.mixc(TEAL_MID, LIME, 0.25 + 0.4 * b.t(v)), "rough": 0.3,
                                     "glow": pk.mulc(LIME, 0.05 + 0.55 * max(0.0, math.sin(math.pi * b.t(v))) ** 1.5),
                                     "noise": 0.2, "bump": 0.1})
    for i in range(4):  # ground leaves
        az = TAU * i / 4 + rng.uniform(-0.3, 0.3) + 0.4
        base = Vector((math.cos(az) * 0.05, math.sin(az) * 0.05, -0.01))
        frond(b, base, az, rng.uniform(0.28, 0.4), rng.uniform(55, 68), rng.uniform(100, 112),
              rng.uniform(0.07, 0.085), n=5, wave=0.0, tip_glow=LIME, glow_from=0.82, thick=0.012, cup=0.4)
    for i in range(2):  # curled tendrils
        az = TAU * (i + 0.5) / 2 + rng.uniform(-0.4, 0.4)
        base = Vector((math.cos(az) * 0.06, math.sin(az) * 0.06, 0.0))
        tp, side = arc_path(base, az, rng.uniform(0.36, 0.46), 35.0, 70.0, 12, curl=(0.45, 420.0, 0.35))
        fs = b.tube(tp, [pk.lerp(0.011, 0.004, k / 11.0) for k in range(12)], sides=4, side=side, tag=0)
        b.paint(fs, fn=lambda v, f: {"col": pk.mixc(TEAL_DEEP, VIOLET, 0.3 + 0.5 * b.t(v)), "rough": 0.45,
                                     "glow": pk.mulc(LIME, pk.smooth01(b.t(v), 0.7, 1.0) * 0.6),
                                     "noise": 0.3, "bump": 0.2})
    return b.finish("SM_Verd_GlowPlant_02", coll, sharp_angle=80)


# ============================================================================ rocks

def _rock_paint(b, faces, tint, moss=1.0, lichen=None):
    """Moss amount is painted per vertex from the smoothed normal, so its border wanders across the
    facets instead of following their edges; lichen patches are painted where asked."""
    b.bm.normal_update()

    def fn(v, f):
        d = {"col": tint, "rough": 0.84, "noise": 0.85, "bump": 0.9,
             "mask": moss * pk.smooth01(v.normal.z, 0.05, 0.8), "mask2": 0.0}
        if lichen:
            m = 0.0
            for c, r in lichen:
                m = max(m, 1.0 - pk.smooth01((v.co - Vector(c)).length, r * 0.25, r))
            d["mask2"] = m
        return d
    b.paint(faces, fn=fn)


def _paint_lichen(b, faces, probes, radius):
    """Glowing lichen patches (Par.a). Each probe (x, z) is snapped to the nearest front-facing (-Y)
    surface point, so the patches always sit on the rock whatever shape the seed produced."""
    b.bm.normal_update()
    verts = {v for f in faces for v in f.verts}
    front = [v for v in verts if v.normal.y < -0.25 and v.co.z > 0.1] or list(verts)
    centres = []
    for (px, pz), r in zip(probes, radius):
        best = min(front, key=lambda v: (v.co.x - px) ** 2 + (v.co.z - pz) ** 2)
        centres.append((best.co.copy(), r))

    def fn(v, f):
        m = 0.0
        for c, r in centres:
            m = max(m, 1.0 - pk.smooth01((v.co - c).length, r * 0.3, r))
        return {"mask2": m}
    b.paint(faces, fn=fn)


def _boulder(b, size, seed, at, rot=(0, 0, 0), subdiv=2, cuts=12, bevel=0.035, floor=None, tint=STONE,
             lichen=None, lump=0.3, cut_range=(0.66, 0.9), moss=1.0, segs=2, taper=0.0):
    """One worn, chiselled boulder: sliced facets with soft chamfered edges."""
    xf = Matrix.Translation(Vector(at))
    for ang, axis in zip(rot, "XYZ"):
        xf = xf @ Matrix.Rotation(math.radians(ang), 4, axis)
    fl = floor if floor is not None else -size[2] * 0.5
    faces = b.rock(size, seed=seed, subdiv=subdiv, lump=lump, freq=1.15, cuts=cuts, cut_range=cut_range,
                   crisp=True, bevel=bevel, bevel_segments=segs, bevel_angle=26.0, floor=fl, xf=xf, taper=taper)
    _rock_paint(b, faces, tint, moss=moss, lichen=lichen)
    return faces


def _attach(b, main, faces):
    """Slide a secondary stone horizontally toward the main one until they interlock."""
    c = sum((v.co for f in faces for v in f.verts), Vector()) / sum(len(f.verts) for f in faces)
    m = sum((v.co for f in main for v in f.verts), Vector()) / sum(len(f.verts) for f in main)
    d = Vector((m.x - c.x, m.y - c.y, 0.0))
    b.push_until_touch(faces, main, d, embed=0.05 + 0.04 * d.length)
    return faces


def rock_01(coll):
    """~60 cm across. A low wide stone with a second stone leaning on it (a cleft between them) and a pebble."""
    b = pk.Builder()
    main = _boulder(b, (0.72, 0.5, 0.4), 5, (0.0, 0.0, 0.1), rot=(0, 7, 15), cuts=8, bevel=0.036, floor=-0.2,
                    cut_range=(0.7, 0.92))
    _attach(b, main, _boulder(b, (0.36, 0.3, 0.46), 8, (0.3, -0.2, 0.15), rot=(18, -24, 60), cuts=7, bevel=0.024,
                              floor=-0.25, tint=STONE_WARM, taper=0.3))
    _attach(b, main, _boulder(b, (0.2, 0.17, 0.15), 3, (-0.3, 0.3, 0.03), cuts=6, bevel=0.01, floor=-0.07,
                              segs=1))
    return b.finish("SM_Verd_Rock_01", coll, sharp_angle=48, recentre=True)


def rock_02(coll):
    """~150 cm across. Two flat stones stacked off-centre on a base boulder, with a pebble."""
    b = pk.Builder()
    main = _boulder(b, (1.6, 1.22, 0.78), 12, (-0.05, 0.0, 0.22), rot=(0, -6, 20), cuts=10, bevel=0.055, floor=-0.36)
    mid = _boulder(b, (1.14, 0.9, 0.38), 17, (0.15, 0.05, 0.7), rot=(7, 9, 70), cuts=9, bevel=0.034, floor=-0.18,
                   tint=STONE_WARM, segs=1)
    b.push_until_touch(mid, main, (0, 0, -1), embed=0.05)
    top = _boulder(b, (0.68, 0.55, 0.29), 19, (-0.04, -0.02, 1.1), rot=(-6, -8, 130), cuts=8, bevel=0.028,
                   floor=-0.14, segs=1)
    b.push_until_touch(top, mid, (0, 0, -1), embed=0.04)
    _attach(b, main, _boulder(b, (0.38, 0.3, 0.26), 21, (0.84, -0.56, 0.05), cuts=7, bevel=0.016, floor=-0.11,
                              tint=STONE_WARM, segs=1))
    return b.finish("SM_Verd_Rock_02", coll, sharp_angle=48, recentre=True)


def rock_03(coll):
    """~300 cm across. A leaning, tapering tooth of rock with two buttress stones and a pebble; glowing lichen
    climbs its front face (the only rock in the kit that glows)."""
    b = pk.Builder()
    main = _boulder(b, (2.05, 1.75, 2.9), 31, (-0.2, 0.15, 0.95), rot=(8, -20, 10), subdiv=2, cuts=9, bevel=0.1,
                    floor=-1.1, cut_range=(0.7, 0.92), taper=0.26)
    right = _attach(b, main, _boulder(b, (1.5, 1.15, 0.9), 37, (1.2, -0.35, 0.3), rot=(0, 8, 30), cuts=9,
                                      bevel=0.06, floor=-0.42, tint=STONE_WARM))
    _attach(b, main, _boulder(b, (1.0, 0.85, 0.6), 41, (-1.4, -0.5, 0.2), rot=(0, -6, -25), cuts=10, bevel=0.035,
                              floor=-0.28, segs=1))
    _attach(b, main, _boulder(b, (0.45, 0.36, 0.3), 43, (0.25, -1.5, 0.07), cuts=7, bevel=0.016, floor=-0.13,
                              tint=STONE_WARM, segs=1))
    _paint_lichen(b, main, [(-0.25, 1.75), (-0.05, 1.3), (0.1, 0.85), (-0.2, 0.45)], [0.5, 0.45, 0.42, 0.36])
    _paint_lichen(b, right, [(1.0, 0.55)], [0.42])
    return b.finish("SM_Verd_Rock_03", coll, sharp_angle=48, recentre=True)


# name, builder, texture set, triangle budget, collision
ASSETS = [
    ("SM_Verd_Mushroom_01", mushroom_01, "Verd_Mushrooms", (600, 2500), True),
    ("SM_Verd_Mushroom_02", mushroom_02, "Verd_Mushrooms", (600, 2500), True),
    ("SM_Verd_Mushroom_03", mushroom_03, "Verd_Mushrooms", (600, 2500), True),
    ("SM_Verd_Fern_01", fern_01, "Verd_Flora", (300, 1200), False),
    ("SM_Verd_Fern_02", fern_02, "Verd_Flora", (300, 1200), False),
    ("SM_Verd_GlowPlant_01", glowplant_01, "Verd_Flora", (300, 1200), False),
    ("SM_Verd_GlowPlant_02", glowplant_02, "Verd_Flora", (300, 1200), False),
    ("SM_Verd_Rock_01", rock_01, "Verd_Rocks", (500, 2000), False),
    ("SM_Verd_Rock_02", rock_02, "Verd_Rocks", (500, 2000), True),
    ("SM_Verd_Rock_03", rock_03, "Verd_Rocks", (500, 2000), True),
]
SETS = ("Verd_Mushrooms", "Verd_Flora", "Verd_Rocks")
COLLISION_NOTE = {
    "SM_Verd_Mushroom_01": "whole shape (the cap is below eye height, nothing to walk under)",
    "SM_Verd_Mushroom_02": "stem and root mound only, so the player can stand under the cap",
    "SM_Verd_Mushroom_03": "trunk and root mound only, so the player can walk under the tiers",
}


def materials():
    return {
        "Verd_Mushrooms": pk.flora_material("M_Verd_Mushrooms_Proc", EMISSION_STRENGTH),
        "Verd_Flora": pk.flora_material("M_Verd_Flora_Proc", EMISSION_STRENGTH),
        "Verd_Rocks": pk.mossy_rock_material("M_Verd_Rocks_Proc", MOSS_DARK, MOSS_LIGHT, BIO, LIME,
                                             EMISSION_STRENGTH),
    }


def fp_layout(o):
    return [
        (o["SM_Verd_Mushroom_03"], 1.4, 2.8, 20), (o["SM_Verd_Mushroom_02"], -3.3, 1.2, 0),
        (o["SM_Verd_Mushroom_01"], -1.9, -2.2, 30), (o["SM_Verd_Rock_03"], 5.0, 0.6, -25),
        (o["SM_Verd_Rock_02"], 2.9, -2.6, 40), (o["SM_Verd_Rock_01"], 0.3, -3.6, 0),
        (o["SM_Verd_Fern_02"], -4.4, -1.6, 0), (o["SM_Verd_Fern_01"], 1.2, -3.0, 60),
        (o["SM_Verd_GlowPlant_01"], -0.7, -4.0, 0), (o["SM_Verd_GlowPlant_02"], 1.9, -4.3, 0),
        (o["SM_Verd_Fern_01"], 4.4, -2.9, 200), (o["SM_Verd_GlowPlant_01"], -3.2, -3.6, 120),
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
    items = [(objs[n], "{}  {:.0f} cm".format(n.replace("SM_Verd_", ""), max(v.co.z for v in objs[n].data.vertices) * 100))
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
        set_info[set_name] = pk.atlas_bake(set_name, group, TEX_DIR, size=1024, weights=weights,
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
    tag = "_wip" if only else "Verdance"
    sheets["lit"] = pk.contact_sheet(items, figure, os.path.join(REVIEW_DIR, tag + "_contact_sheet.png"), mode="lit")
    sheets["dark"] = pk.contact_sheet(items, figure, os.path.join(REVIEW_DIR, tag + "_contact_sheet_dark.png"),
                                      mode="dark")
    sheets["clay"] = pk.contact_sheet(items, figure, os.path.join(REVIEW_DIR, tag + "_contact_sheet_clay.png"),
                                      mode="clay")
    if not only:
        sheets["first_person"] = pk.first_person_view(
            fp_layout(objs), figure, os.path.join(REVIEW_DIR, "Verdance_first_person_dusk.png"),
            ground=(0.012, 0.03, 0.028), sky=(0.03, 0.03, 0.09), sky_strength=0.6,
            sun=(0.5, (0.55, 0.6, 1.0)), fig_at=(-0.4, -1.2))
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
    extra = {n: {"collision_note": COLLISION_NOTE.get(n, "one convex hull of the whole piece")}
             for n, _f, _s, _b, c in ASSETS if c}
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
    with open(os.path.join(REVIEW_DIR, "Verdance_summary.json"), "w", encoding="utf-8") as fh:
        json.dump(out, fh, indent=1)
    for n in names:
        print("FINAL {:24s} tris {:5d} h {:6.1f} cm  ucx {}  roundtrip {}".format(
            n, pack[n]["tris"], summary[n]["height_cm"], pack[n]["collision"], rt[n]["ok"]))
    print("__RESULT__" + json.dumps({"ok": out["all_round_trips_ok"]}))


if __name__ == "__main__":
    main()
