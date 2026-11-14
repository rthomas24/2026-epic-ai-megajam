# SPDX-License-Identifier: MIT
# Kit (planet outposts) - paints the kit's one shared 2K trim sheet procedurally (numpy, seeded, original work).
# Forked from the DominionInterior atlas painter (same finish pass: cavity AO, worn bright edges, scuffs, DirectX
# normal from the painted height); new regions and the planet-outpost palette: light composite panels, dark
# gunmetal frames, hazard yellow, rubberised treads. Emissive is NOT in the sheet: light strips use the separate
# Glow slot (teal on Verdance, ember on Cinder), so one sheet serves both planets.
#   textures/T_Kit_Trim_BC.png (sRGB), T_Kit_Trim_N.png (DirectX), T_Kit_Trim_ORM.png (R AO, G rough, B metal)
# Run:  blender --background --factory-startup --python scripts/kit_atlas.py
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from kit_regions import ATLAS, REGIONS  # noqa: E402

KIT = os.path.dirname(HERE)
rng = np.random.default_rng(11)

WHITE = (0.55, 0.585, 0.60)      # composite panel (light warm grey, not pure white: keeps the bloom honest)
WHITE_D = (0.46, 0.475, 0.49)
GUN = (0.16, 0.17, 0.19)
GUN_D = (0.095, 0.10, 0.115)
FLOOR = (0.30, 0.315, 0.33)
FLOOR_D = (0.20, 0.21, 0.225)
YEL = (0.78, 0.52, 0.08)
TEAL_P = (0.10, 0.36, 0.38)      # painted accent band (not emissive)
STEN = (0.20, 0.21, 0.22)
HOLE = (0.025, 0.026, 0.03)
RUB = (0.07, 0.072, 0.078)


def pnoise(h, w, fmax=6, n=40):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    out = np.zeros((h, w), np.float32)
    for _ in range(n):
        fx, fy = rng.integers(-fmax, fmax + 1), rng.integers(0, fmax + 1)
        if fx == 0 and fy == 0:
            continue
        amp = 1.0 / (1.0 + (fx * fx + fy * fy) ** 0.6)
        out += amp * np.cos(2 * np.pi * (fx * xx / w + fy * yy / h) + rng.uniform(0, 6.283))
    return out / (np.abs(out).max() + 1e-6)


def blur(a, r):
    out = np.zeros_like(a)
    for d in range(-r, r + 1):
        out += np.roll(a, d, 1)
    out /= (2 * r + 1)
    o2 = np.zeros_like(a)
    for d in range(-r, r + 1):
        o2 += np.roll(out, d, 0)
    return o2 / (2 * r + 1)


class Reg:
    def __init__(self, name, col, rough, metal):
        _, _, w, h = REGIONS[name]
        self.name, self.w, self.h = name, w, h
        self.H = np.zeros((h, w), np.float32)
        self.C = np.empty((h, w, 3), np.float32)
        self.C[:] = col
        self.R = np.full((h, w), rough, np.float32)
        self.M = np.full((h, w), metal, np.float32)
        self.yy, self.xx = np.mgrid[0:h, 0:w].astype(np.float32)
        self.wear = 1.0

    def paint(self, m, col=None, rough=None, metal=None, alpha=1.0):
        a = (m.astype(np.float32) * alpha)
        if col is not None:
            self.C = self.C * (1 - a[..., None]) + np.array(col, np.float32) * a[..., None]
        if rough is not None:
            self.R = self.R * (1 - a) + rough * a
        if metal is not None:
            self.M = self.M * (1 - a) + metal * a

    def rect(self, x0, y0, x1, y1):
        return (self.xx >= x0) & (self.xx < x1) & (self.yy >= y0) & (self.yy < y1)

    def plate(self, x0, y0, x1, y1, height, bevel=5.0):
        d = np.minimum(np.minimum(self.xx - x0, x1 - 1 - self.xx), np.minimum(self.yy - y0, y1 - 1 - self.yy))
        m = d >= 0
        v = height * np.clip((d + 1) / bevel, 0, 1)
        self.H = np.where(m, self.H + v, self.H)
        return m

    def groove(self, m, depth=3.0):
        self.H[m] -= depth
        self.paint(m, HOLE, 0.85, 0.0, 0.55)

    def hband(self, y0, y1, height, bevel=4.0):
        d = np.minimum(self.yy - y0, y1 - 1 - self.yy)
        m = d >= 0
        self.H = np.where(m, self.H + height * np.clip((d + 1) / bevel, 0, 1), self.H)
        return m

    def disc(self, cx, cy, r, height):
        d = r - np.hypot(self.xx - cx, self.yy - cy)
        m = d >= 0
        self.H = np.where(m, self.H + height * np.clip(d / 2.0, 0, 1), self.H)
        return m

    def bolt(self, cx, cy, r=5):
        m = self.disc(cx, cy, r, 2.5)
        self.paint(m, GUN, 0.35, 1.0)

    def stencil(self, m, col=STEN, keep=0.25):
        n = pnoise(self.h, self.w, 14, 60)
        self.paint(m & (n > -keep), col, 0.7, 0.0, 0.85)


def chevrons(r, y0, y1, pitch=64, width=30):
    return (((r.xx + (r.yy - y0)) % pitch) < width) & (r.yy >= y0) & (r.yy < y1)


# ---------------------------------------------------------------------------------------------- regions
def r_wall(name):
    """400 x 300 cm (1024 x 768 px). y = 0 is the TOP of the wall (V = 1)."""
    r = Reg(name, WHITE, 0.42, 0.05)
    s = 2.56
    # top frame band 0..20 cm, kick band 270..300 cm (dark), light-slot band at 55..65 cm from the top
    top = r.hband(0, int(20 * s), 3)
    r.paint(top, GUN, 0.45, 0.85)
    kick = r.hband(int(270 * s), 768, 3)
    r.paint(kick, GUN_D, 0.55, 0.6)
    for x in range(0, 1024, 32):
        r.bolt(x + 16, int(10 * s), 3)
    slot = r.rect(0, int(56 * s), 1024, int(64 * s))
    r.H[slot] -= 4
    r.paint(slot, GUN_D, 0.5, 0.6)
    for i in range(2):                     # two 200 cm panels, a seam at mid height (150 cm from the floor)
        x = i * 512
        for (y0, y1) in ((int(20 * s) + 4, int(56 * s) - 2), (int(64 * s) + 2, int(150 * s) - 2), (int(150 * s) + 2, int(270 * s) - 4)):
            r.plate(x + 6, y0, x + 506, y1, 3, 6)
        g = r.rect(x, int(150 * s) - 2, x + 512, int(150 * s) + 2) | r.rect(x, 0, x + 3, 768) | r.rect(x + 509, 0, x + 512, 768)
        r.groove(g, 3)
        for bx in (18, 494):
            for by in (int(72 * s), int(140 * s), int(160 * s), int(260 * s)):
                r.bolt(x + bx, by, 4)
        # a painted teal band and a small id stencil on the upper panel
        band = r.rect(x + 40, int(96 * s), x + 472, int(110 * s))
        r.paint(band, TEAL_P, 0.4, 0.1)
    if name == "wallB":
        v = r.plate(560, int(180 * s), 760, int(250 * s), -4, 4)      # vent on the second panel
        r.paint(v, GUN_D, 0.5, 0.7)
        sl = v & (((r.yy - int(180 * s)) % 14) < 7) & r.rect(572, int(184 * s), 748, int(246 * s))
        r.H[sl] -= 4
        r.paint(sl, HOLE, 0.9, 0.0)
        m = np.zeros((r.h, r.w), bool)
        x = 80
        for wdt in (14, 30, 14, 14, 44):                                # stencilled bay number
            m |= r.rect(x, int(118 * s), x + wdt, int(140 * s))
            x += wdt + 12
        r.stencil(m, STEN, 0.2)
        hz = chevrons(r, int(240 * s), int(268 * s), 56, 26) & r.rect(40, 0, 470, 768)
        r.stencil(hz, YEL, 0.35)
    return r


def r_floor():
    """400 x 400 cm: four 200 cm plates, tread dots, a darker worn lane through the middle."""
    r = Reg("floor", FLOOR, 0.72, 0.35)
    for i in range(2):
        for j in range(2):
            x, y = i * 512, j * 512
            r.plate(x + 4, y + 4, x + 508, y + 508, 3, 5)
            d = 5 - np.hypot(((r.xx - x) % 32) - 16, ((r.yy - y) % 32) - 16)
            dots = (d >= 0) & r.rect(x + 40, y + 40, x + 472, y + 472)
            r.H = np.where(dots, r.H + 1.4 * np.clip(d / 2, 0, 1), r.H)
            for bx, by in ((18, 18), (494, 18), (18, 494), (494, 494)):
                r.bolt(x + bx, y + by, 5)
    g = r.rect(0, 0, 1024, 3) | r.rect(0, 1021, 1024, 1024) | r.rect(0, 0, 3, 1024) | r.rect(1021, 0, 1024, 1024) \
        | r.rect(510, 0, 514, 1024) | r.rect(0, 510, 1024, 514)
    r.groove(g, 3)
    lane = np.clip(1.0 - np.abs(r.yy - 512) / 360.0, 0, 1) * (0.6 + 0.4 * pnoise(1024, 1024, 4, 30))
    r.paint(np.ones_like(lane, bool), FLOOR_D, 0.8, 0.25, np.clip(lane, 0, 1) * 0.35)
    # one painted walk-lane edge line per plate (not a hazard border on every tile: rooms read striped otherwise)
    for y in (96, 928):
        r.stencil(r.rect(64, y, 960, y + 6), YEL, 0.5)
    r.wear = 1.4
    return r


def r_grate():
    r = Reg("grate", FLOOR_D, 0.65, 0.7)
    m = r.hband(0, 20, 3) | r.hband(492, 512, 3)
    r.paint(m, GUN, 0.5, 0.85)
    mx, my = r.xx % 32, (r.yy - 24) % 24
    d = np.minimum(np.minimum(mx - 4, 28 - mx), np.minimum(my - 4, 20 - my))
    hole = (d >= 0) & (r.yy >= 24) & (r.yy < 488)
    r.H = np.where(hole, -7 * np.clip((d + 1) / 2.0, 0, 1), r.H)
    r.paint(hole & (d >= 1), HOLE, 0.9, 0.0)
    for k in range(16):
        r.bolt(16 + 32 * k, 10, 3)
        r.bolt(16 + 32 * k, 502, 3)
    return r


def r_screen():
    r = Reg("screen", GUN_D, 0.45, 0.85)
    r.plate(4, 4, 508, 252, 4, 5)
    g = r.plate(18, 18, 494, 238, -4, 4)
    r.paint(g, (0.02, 0.03, 0.04), 0.08, 0.0)
    grid = g & (((r.xx % 34) < 1) | ((r.yy % 34) < 1))
    r.paint(grid, (0.04, 0.07, 0.08))
    return r


def r_vent():
    r = Reg("vent", GUN, 0.5, 0.85)
    r.plate(6, 6, 250, 250, 3, 5)
    slot = r.rect(30, 36, 226, 220) & (((r.yy - 36) % 20) < 10)
    r.H[slot] -= 5
    r.paint(slot, HOLE, 0.9, 0.0)
    for bx, by in ((18, 18), (238, 18), (18, 238), (238, 238)):
        r.bolt(bx, by, 4)
    return r


def r_hatch():
    r = Reg("hatch", WHITE_D, 0.45, 0.1)
    r.plate(6, 6, 250, 250, 3, 5)
    m = r.plate(30, 30, 226, 226, -3, 4)
    r.paint(m, GUN, 0.45, 0.8)
    r.plate(40, 40, 216, 216, 3, 4)
    h = r.plate(150, 112, 200, 144, -4, 3)
    r.paint(h, HOLE, 0.8, 0.0)
    for bx, by in ((18, 18), (238, 18), (18, 238), (238, 238)):
        r.bolt(bx, by, 4)
    tri = (r.xx - 40 + r.yy - 40 < 60) & r.rect(40, 40, 216, 216) & (((r.xx - r.yy) % 24) < 12)
    r.stencil(tri, YEL, 0.4)
    return r


def r_roof():
    r = Reg("roof", WHITE_D, 0.55, 0.1)
    r.plate(4, 4, 508, 508, 3, 6)
    for x in (170, 342):
        r.groove(r.rect(x - 2, 20, x + 2, 492), 3)
    slot = r.rect(40, 236, 472, 276)                 # the light strip channel along U (glow mesh sits in it)
    r.H[slot] -= 6
    r.paint(slot, GUN_D, 0.5, 0.6)
    for bx, by in ((18, 18), (494, 18), (18, 494), (494, 494)):
        r.bolt(bx, by, 4)
    return r


def r_frame():
    r = Reg("frame", GUN, 0.45, 0.9)
    r.hband(0, 16, 2.5)
    r.hband(112, 128, 2.5)
    for k in range(4):
        x = 64 + 128 * k
        r.plate(x - 26, 24, x + 26, 104, 3, 4)
    for k in range(16):
        r.bolt(16 + 32 * k, 8, 3)
        r.bolt(16 + 32 * k, 120, 3)
    return r


def r_hazard():
    r = Reg("hazard", (0.06, 0.06, 0.065), 0.55, 0.2)
    m = r.hband(0, 12, 3) | r.hband(116, 128, 3)
    r.paint(m, GUN, 0.5, 0.9)
    r.stencil(chevrons(r, 14, 114, 128, 60), YEL, 0.45)
    return r


def r_kick():
    r = Reg("kick", FLOOR_D, 0.7, 0.4)
    r.hband(0, 14, 3)
    r.hband(114, 128, 3)
    bar = (((r.xx + r.yy) % 32) < 9) & (r.yy >= 22) & (r.yy < 106)
    r.H[bar] += 2.0
    return r


def r_rail():
    r = Reg("rail", YEL, 0.38, 0.2)
    r.wear = 1.8
    n = pnoise(128, 512, 20, 60)
    r.paint(n > 0.55, GUN, 0.4, 0.9, 0.9)            # chipped paint down to metal
    return r


def r_tread():
    """200 x 100 cm (512 x 256 px): rubber tread with ribs, a yellow nosing strip along y = 0 (the front edge)."""
    r = Reg("tread", RUB, 0.8, 0.0)
    nose = r.hband(0, 26, 2)
    r.paint(nose, YEL, 0.45, 0.2)
    rib = ((r.yy % 18) < 7) & (r.yy > 34) & (r.yy < 250)
    r.H[rib] += 2.0
    r.paint(rib, (0.1, 0.1, 0.11))
    return r


def r_pillar():
    r = Reg("pillar", WHITE_D, 0.45, 0.1)
    r.plate(4, 4, 508, 252, 3, 5)
    for x in (128, 256, 384):
        r.groove(r.rect(x - 2, 10, x + 2, 246), 3)
    band = r.rect(0, 200, 512, 236)
    r.paint(band, GUN, 0.45, 0.85)
    r.stencil(chevrons(r, 204, 232, 48, 22), YEL, 0.4)
    return r


def r_console():
    r = Reg("console", GUN, 0.45, 0.8)
    r.plate(4, 4, 508, 252, 3, 5)
    p = r.plate(30, 30, 482, 226, -3, 4)
    r.paint(p, WHITE_D, 0.45, 0.1)
    for bx, by in ((16, 16), (496, 16), (16, 240), (496, 240)):
        r.bolt(bx, by, 4)
    g = r.rect(60, 120, 452, 124)
    r.groove(g, 2)
    return r


def build():
    regs = [r_wall("wall"), r_wall("wallB"), r_floor(), r_grate(), r_screen(), r_vent(), r_hatch(), r_roof(),
            r_frame(), r_hazard(), r_kick(), r_rail(), r_tread(), r_pillar(), r_console(),
            Reg("sw_dark", GUN_D, 0.5, 0.6), Reg("sw_gun", GUN, 0.45, 0.9), Reg("sw_white", WHITE, 0.42, 0.05),
            Reg("sw_yel", YEL, 0.4, 0.2), Reg("sw_glass", (0.10, 0.20, 0.24), 0.05, 0.0), Reg("sw_black", HOLE, 0.8, 0.0),
            Reg("sw_floor", FLOOR, 0.72, 0.35), Reg("sw_rub", RUB, 0.85, 0.0)]
    bc = np.zeros((ATLAS, ATLAS, 3), np.float32)
    nm = np.zeros((ATLAS, ATLAS, 3), np.float32)
    nm[:] = (0.5, 0.5, 1.0)
    orm = np.ones((ATLAS, ATLAS, 3), np.float32)
    for r in regs:
        x, y, w, h = REGIONS[r.name]
        c, n, o = finish(r)
        bc[y:y + h, x:x + w] = c
        nm[y:y + h, x:x + w] = n
        orm[y:y + h, x:x + w] = o
    return bc, nm, orm


def finish(r):
    h, w = r.h, r.w
    wear = pnoise(h, w, 5, 40)
    fine = pnoise(h, w, 40, 80)
    grain = rng.normal(0, 1, (h, w)).astype(np.float32)
    bl = blur(r.H, 5)
    cav = np.clip(bl - r.H, 0, 3) / 3.0
    edge = np.clip(r.H - bl, 0, 2) / 2.0
    ao = 1.0 - 0.7 * np.clip(cav * 1.6, 0, 1)
    c = r.C * (1.0 + 0.07 * r.wear * wear[..., None] + 0.04 * fine[..., None] + 0.012 * grain[..., None])
    c = c * (0.5 + 0.5 * ao[..., None])
    c = c + (0.08 * r.wear) * (edge * np.clip(0.4 + fine, 0, 1))[..., None]       # worn bright edges
    grime = np.clip(pnoise(h, w, 30, 60) - 0.5, 0, 1) * 1.6 * r.wear
    c = c * (1.0 - 0.18 * grime[..., None])                                          # grime streaks
    rough = np.clip(r.R + 0.06 * wear + 0.05 * fine + 0.2 * grime + 0.15 * cav, 0.04, 1.0)
    dx = (np.roll(r.H, -1, 1) - np.roll(r.H, 1, 1)) * 0.5
    dy = (np.roll(r.H, -1, 0) - np.roll(r.H, 1, 0)) * 0.5
    dx += 0.05 * (np.roll(fine, -1, 1) - np.roll(fine, 1, 1)) * 2.0 + 0.02 * np.roll(grain, 1, 1)
    dy += 0.05 * (np.roll(fine, -1, 0) - np.roll(fine, 1, 0)) * 2.0 + 0.02 * np.roll(grain, 1, 0)
    k = 0.9
    nx, ny, nz = -dx * k, -dy * k, np.ones_like(dx)      # DirectX: +G points down the image
    ln = np.sqrt(nx * nx + ny * ny + nz * nz)
    nrm = np.stack([nx / ln, ny / ln, nz / ln], -1) * 0.5 + 0.5
    orm = np.stack([ao, rough, np.clip(r.M, 0, 1)], -1)
    return np.clip(c, 0.0, 1.0), nrm, orm


def save(arr, path, srgb):
    import bpy
    h, w, _ = arr.shape
    img = bpy.data.images.new(os.path.basename(path), w, h, alpha=False, float_buffer=False)
    img.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
    rgba = np.ones((h, w, 4), np.float32)
    rgba[..., :3] = arr[::-1]
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def main():
    out = os.path.join(KIT, "textures")
    os.makedirs(out, exist_ok=True)
    bc, nm, orm = build()
    save(bc, os.path.join(out, "T_Kit_Trim_BC.png"), True)
    save(nm, os.path.join(out, "T_Kit_Trim_N.png"), False)
    save(orm, os.path.join(out, "T_Kit_Trim_ORM.png"), False)
    print("__RESULT__ atlas written to", out)


if __name__ == "__main__":
    main()
