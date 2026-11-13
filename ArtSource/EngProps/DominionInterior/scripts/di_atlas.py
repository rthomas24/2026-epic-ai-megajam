# SPDX-License-Identifier: MIT
# DominionInterior - paints the kit's one shared 2K atlas / trim sheet procedurally (numpy, seeded, original work):
#   textures/T_DI_Atlas_BC.png  (sRGB)   base colour with cavity dirt and edge wear baked in (the level is lit flat)
#   textures/T_DI_Atlas_N.png   (DirectX normal, from the painted height)
#   textures/T_DI_Atlas_ORM.png (R = AO, G = roughness, B = metallic)
# Run:  blender --background --factory-startup --python scripts/di_atlas.py
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from di_regions import ATLAS, REGIONS  # noqa: E402

KIT = os.path.dirname(HERE)
rng = np.random.default_rng(7)

BLACK = (0.085, 0.088, 0.105)
GUN = (0.20, 0.215, 0.24)
GUN_D = (0.13, 0.14, 0.16)
FLOOR = (0.27, 0.28, 0.295)
FLOOR_D = (0.19, 0.195, 0.21)
CRIM = (0.30, 0.045, 0.07)
HAZ = (0.75, 0.33, 0.10)
STEN = (0.62, 0.63, 0.60)
HOLE = (0.025, 0.026, 0.03)


def pnoise(h, w, fmax=6, n=40):
    """Periodic smooth noise in [-1, 1] (sum of integer-frequency waves)."""
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
        """Worn paint: the mask minus noise-shaped chips."""
        n = pnoise(self.h, self.w, 14, 60)
        self.paint(m & (n > -keep), col, 0.7, 0.0, 0.85)


def chevrons(r, cx, top, width, n=3, pitch=50, thick=22, slope=0.8):
    m = np.zeros((r.h, r.w), bool)
    for k in range(n):
        d = (r.yy - (top + k * pitch)) - np.abs(r.xx - cx) * slope
        m |= (d >= 0) & (d < thick) & (np.abs(r.xx - cx) < width / 2)
    return m


def r_armour(name):
    r = Reg(name, BLACK, 0.24, 0.35)
    for i in range(2):
        x = i * 512
        r.plate(x + 4, 5, x + 508, 379, 4, 5)
        g = r.rect(x + 30, 248, x + 482, 251)
        r.H[g] -= 2.5
        for bx, by in ((22, 22), (490, 22), (22, 362), (490, 362), (256, 22), (256, 362)):
            r.bolt(x + bx, by, 5)
        r.paint(r.rect(x + 30, 290, x + 482, 322), CRIM, 0.4, 0.3)
        g = r.rect(x + 30, 288, x + 482, 290) | r.rect(x + 30, 322, x + 482, 324)
        r.H[g] -= 1.5
    if name == "armourB":
        m = np.zeros((r.h, r.w), bool)
        x = 70
        for wdt in (14, 14, 30, 14, 44):
            m |= r.rect(x, 80, x + wdt, 170)
            x += wdt + 12
        m |= r.rect(70, 184, x - 12, 194)
        m |= np.hypot(r.xx - (x + 22), r.yy - 100) < 11
        m |= np.hypot(r.xx - (x + 22), r.yy - 150) < 11
        m |= r.rect(512 + 380, 60, 512 + 450, 72) | r.rect(512 + 438, 60, 512 + 450, 130)
        r.stencil(m)
    return r


def r_gunmetal():
    r = Reg("gunmetal", GUN, 0.5, 0.9)
    r.hband(0, 20, 2.5)
    r.hband(236, 256, 2.5)
    for k in range(8):
        x = 64 + 128 * k
        r.plate(x - 10, 26, x + 10, 230, 3, 4)
    for k in range(32):
        r.bolt(16 + 32 * k, 10, 3)
        r.bolt(16 + 32 * k, 246, 3)
    return r


def r_ceiling():
    r = Reg("ceiling", (0.105, 0.11, 0.125), 0.55, 0.5)
    for i in range(2):
        x = i * 512
        r.plate(x + 4, 5, x + 508, 379, 3, 5)
        my = (r.yy - 110) % 22
        slot = r.rect(x + 70, 110, x + 442, 110 + 22 * 8) & (my < 9)
        r.H[slot] -= 4
        r.paint(slot, HOLE, 0.9, 0.0)
        for bx, by in ((22, 22), (490, 22), (22, 362), (490, 362)):
            r.bolt(x + bx, by, 4)
    return r


def r_cable():
    r = Reg("cable", GUN_D, 0.6, 0.8)
    r.hband(0, 12, 4)
    r.hband(116, 128, 4)
    for cy, col in ((34, (0.05, 0.05, 0.055)), (64, (0.22, 0.04, 0.05)), (94, (0.12, 0.125, 0.13))):
        t = (r.yy - cy) / 12.0
        m = np.abs(t) < 1
        r.H = np.where(m, np.maximum(r.H, 9 * np.sqrt(np.clip(1 - t * t, 0, 1))), r.H)
        r.paint(m, col, 0.75, 0.0)
    for k in range(4):
        x = 128 + 256 * k
        m = r.plate(x - 12, 14, x + 12, 114, 11, 4)
        r.paint(m, GUN, 0.45, 0.9)
        r.bolt(x, 22, 4)
        r.bolt(x, 106, 4)
    return r


def r_hazard():
    r = Reg("hazard", (0.06, 0.06, 0.065), 0.55, 0.2)
    m = r.hband(0, 12, 3) | r.hband(116, 128, 3)
    r.paint(m, GUN, 0.5, 0.9)
    stripe = (((r.xx + np.abs(r.yy - 64)) % 128) < 60) & (r.yy >= 14) & (r.yy < 114)
    r.stencil(stripe, HAZ, 0.45)
    return r


def r_kick():
    r = Reg("kick", FLOOR_D, 0.7, 0.4)
    r.hband(0, 14, 3)
    r.hband(114, 128, 3)
    bar = (((r.xx + r.yy) % 32) < 9) & (r.yy >= 22) & (r.yy < 106)
    r.H[bar] += 2.0
    return r


def r_triad():
    r = Reg("triad", GUN_D, 0.5, 0.8)
    r.plate(6, 6, 250, 250, 3, 5)
    c = chevrons(r, 112, 50, 170, 3, 52, 24, 0.75)
    r.H[c] += 3.0
    r.stencil(c, (0.42, 0.43, 0.42), 0.05)
    m = r.plate(142, 16, 244, 240, 6, 5)       # the armour plate bolted over half of it
    r.paint(m, BLACK, 0.24, 0.35)
    for by in (30, 128, 226):
        r.bolt(156, by, 5)
        r.bolt(230, by, 5)
    r.wear = 2.2
    return r


def r_vent():
    r = Reg("vent", GUN, 0.5, 0.9)
    r.plate(6, 6, 250, 250, 3, 5)
    slot = r.rect(34, 40, 222, 216) & (((r.yy - 40) % 22) < 11)
    r.H[slot] -= 5
    r.paint(slot, HOLE, 0.9, 0.0)
    for bx, by in ((18, 18), (238, 18), (18, 238), (238, 238)):
        r.bolt(bx, by, 4)
    return r


def r_hatch():
    r = Reg("hatch", GUN_D, 0.5, 0.8)
    r.plate(6, 6, 250, 250, 3, 5)
    m = r.plate(30, 30, 226, 226, -3, 4)
    r.paint(m, BLACK, 0.3, 0.35)
    r.plate(40, 40, 216, 216, 3, 4)
    h = r.plate(150, 112, 200, 144, -4, 3)
    r.paint(h, HOLE, 0.8, 0.0)
    for bx, by in ((18, 18), (238, 18), (18, 238), (238, 238)):
        r.bolt(bx, by, 4)
    tri = (r.xx - 40 + r.yy - 40 < 60) & r.rect(40, 40, 216, 216) & (((r.xx - r.yy) % 24) < 12)
    r.stencil(tri, HAZ, 0.4)
    return r


def r_swatch(name, col, rough, metal):
    return Reg(name, col, rough, metal)


def r_grating():
    r = Reg("grating", FLOOR, 0.72, 0.5)
    m = r.hband(0, 26, 3) | r.hband(486, 512, 3)
    r.paint(m, FLOOR_D, 0.6, 0.7)
    mx = r.xx % 64
    my = (r.yy - 32) % 32
    d = np.minimum(np.minimum(mx - 8, 56 - mx), np.minimum(my - 6, 26 - my))
    hole = (d >= 0) & (r.yy >= 32) & (r.yy < 480)
    r.H = np.where(hole, -7 * np.clip((d + 1) / 3.0, 0, 1), r.H)
    r.paint(hole & (d >= 2), HOLE, 0.9, 0.0)
    for k in range(32):
        r.bolt(16 + 32 * k, 13, 3)
        r.bolt(16 + 32 * k, 499, 3)
    return r


def r_plate(name):
    r = Reg(name, FLOOR, 0.8, 0.3)
    r.plate(3, 3, 509, 509, 4, 5)
    d = 5 - np.hypot((r.xx % 32) - 16, (r.yy % 32) - 16)
    dots = (d >= 0) & r.rect(64, 64, 448, 448)
    r.H = np.where(dots, r.H + 1.6 * np.clip(d / 2, 0, 1), r.H)
    for bx, by in ((20, 20), (492, 20), (20, 492), (492, 492)):
        r.bolt(bx, by, 5)
    if name == "plateB":
        m = r.rect(36, 36, 150, 48) | r.rect(36, 36, 48, 150) | r.rect(362, 464, 476, 476) | r.rect(464, 362, 476, 476)
        x = 300
        for wdt in (12, 12, 26, 12):
            m |= r.rect(x, 40, x + wdt, 58)
            x += wdt + 8
        r.stencil(m, STEN, 0.2)
        hz = r.rect(36, 456, 330, 480) & (((r.xx + r.yy) % 40) < 20)
        r.stencil(hz, HAZ, 0.3)
    return r


def r_screen():
    r = Reg("screen", GUN_D, 0.45, 0.85)
    r.plate(4, 4, 508, 316, 4, 5)
    g = r.plate(22, 22, 490, 298, -4, 4)
    r.paint(g, (0.022, 0.028, 0.04), 0.08, 0.0)
    grid = g & (((r.xx % 39) < 1) | ((r.yy % 39) < 1))
    r.paint(grid, (0.045, 0.055, 0.075))
    return r


def r_keys():
    r = Reg("keys", GUN_D, 0.5, 0.8)
    r.plate(4, 4, 508, 188, 3, 5)
    mx, my = (r.xx - 24) % 34, (r.yy - 28) % 34
    d = np.minimum(np.minimum(mx - 3, 30 - mx), np.minimum(my - 3, 30 - my))
    key = (d >= 0) & r.rect(24, 28, 24 + 34 * 10, 28 + 34 * 4)
    r.H = np.where(key, r.H + 3 * np.clip((d + 1) / 3.0, 0, 1), r.H)
    r.paint(key, (0.07, 0.072, 0.08), 0.6, 0.0)
    r.paint(key & r.rect(24 + 34 * 8, 28, 24 + 34 * 10, 28 + 34), CRIM, 0.5, 0.0)
    for cx in (410, 466):
        m = r.disc(cx, 62, 20, 5)
        r.paint(m, GUN, 0.35, 1.0)
    s = r.plate(388, 120, 488, 150, -4, 3)
    r.paint(s, HOLE, 0.8, 0.0)
    return r


def r_petal():
    r = Reg("petal", BLACK, 0.22, 0.4)
    r.plate(5, 5, 507, 507, 5, 8)
    for x in (128, 384):
        g = r.rect(x - 2, 30, x + 2, 482)
        r.H[g] -= 3
    m = r.rect(150, 60, 362, 190)
    r.paint(m, CRIM, 0.38, 0.3)
    g = r.rect(148, 58, 364, 60) | r.rect(148, 190, 364, 192)
    r.H[g] -= 2
    c = chevrons(r, 256, 300, 150, 3, 44, 18, 0.7)
    r.H[c] += 2.0
    r.stencil(c, (0.2, 0.2, 0.21), 0.0)
    for bx, by in ((26, 26), (486, 26), (26, 486), (486, 486), (256, 26), (256, 486)):
        r.bolt(bx, by, 6)
    return r


def r_pad():
    r = Reg("pad", FLOOR, 0.78, 0.3)
    s = 512 / 600.0   # px per cm in this region; (0, 0) = pad centre
    for k in range(1, 6):
        g = (np.abs(r.xx - k * 100 * s) < 1.5) | (np.abs(r.yy - k * 100 * s) < 1.5)
        r.H[g] -= 2.5
    rad = np.hypot(r.xx, r.yy)
    m = (np.abs(rad - 300 * s) < 9) | (rad < 60 * s) & (rad > 44 * s)
    m |= ((r.xx < 7) | (r.yy < 7)) & (rad > 80 * s) & (rad < 260 * s)
    m |= (r.rect(380, 380, 440, 392) | r.rect(428, 380, 440, 440))
    r.stencil(m, STEN, 0.3)
    r.stencil(rad < 30 * s, HAZ, 0.4)
    edge = ((r.xx > 462) | (r.yy > 462))
    r.paint(edge, (0.06, 0.06, 0.065), 0.55, 0.2)
    r.stencil(edge & (((r.xx + r.yy) % 44) < 22), HAZ, 0.45)
    g = (np.abs(r.xx - 461) < 1.5) | (np.abs(r.yy - 461) < 1.5)
    r.H[g] -= 2.5
    return r


def r_torn():
    r = Reg("torn", GUN, 0.55, 0.9)
    n = pnoise(r.h, r.w, 9, 70)
    n2 = pnoise(r.h, r.w, 20, 80)
    r.paint(n > 0.15, (0.05, 0.048, 0.05), 0.85, 0.2, 0.8)          # scorch
    r.paint(np.abs(n2) < 0.035, (0.5, 0.5, 0.52), 0.25, 1.0, 0.9)   # bright scratches
    r.H += 3.0 * n + 1.0 * n2
    r.wear = 2.0
    return r


def finish(r):
    h, w = r.h, r.w
    wear = pnoise(h, w, 5, 40)
    fine = pnoise(h, w, 40, 80)
    grain = rng.normal(0, 1, (h, w)).astype(np.float32)
    bl = blur(r.H, 5)
    cav = np.clip(bl - r.H, 0, 3) / 3.0
    edge = np.clip(r.H - bl, 0, 2) / 2.0
    ao = 1.0 - 0.75 * np.clip(cav * 1.6, 0, 1)
    c = r.C * (1.0 + 0.10 * r.wear * wear[..., None] + 0.05 * fine[..., None] + 0.015 * grain[..., None])
    c = c * (0.45 + 0.55 * ao[..., None])
    c = c + (0.10 * r.wear) * (edge * np.clip(0.4 + fine, 0, 1))[..., None]       # worn bright edges
    scuff = np.clip(pnoise(h, w, 30, 60) - 0.55, 0, 1) * 2.0 * r.wear
    c = c + 0.05 * scuff[..., None]
    rough = np.clip(r.R + 0.07 * wear + 0.05 * fine + 0.25 * scuff + 0.15 * cav, 0.04, 1.0)
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


def build():
    regs = [r_armour("armourA"), r_armour("armourB"), r_gunmetal(), r_ceiling(), r_cable(), r_hazard(), r_kick(),
            r_triad(), r_vent(), r_hatch(),
            r_swatch("sw_black", BLACK, 0.24, 0.35), r_swatch("sw_gun", GUN, 0.5, 0.9),
            r_swatch("sw_crim", CRIM, 0.4, 0.3), r_swatch("sw_light", (0.55, 0.57, 0.6), 0.5, 0.2),
            r_grating(), r_screen(), r_keys(), r_petal(), r_plate("plateA"), r_plate("plateB"), r_pad(), r_torn()]
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
    save(bc, os.path.join(out, "T_DI_Atlas_BC.png"), True)
    save(nm, os.path.join(out, "T_DI_Atlas_N.png"), False)
    save(orm, os.path.join(out, "T_DI_Atlas_ORM.png"), False)
    print("__RESULT__ atlas written to", out)


if __name__ == "__main__":
    main()
