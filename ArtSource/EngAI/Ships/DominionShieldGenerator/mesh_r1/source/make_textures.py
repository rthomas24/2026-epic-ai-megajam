#!/usr/bin/env python3
"""2K (and 4K master) PBR atlas for SM_DominionShieldGenerator + SM_DominionShieldGeneratorCore.

Palette matches Dominion hull r2 (mesh_r2/source/make_textures.py):
  gunmetal armor #4c525c–#646b76 (sRGB 60–90 median target),
  crimson trim #9a1620–#d42834, amber #ff5a14, recess #141619,
  electric cyan core #4CCFFF / FLinearColor(0.3, 0.8, 1.0).

Normal is authored OpenGL (Y+), then flipped to DirectX (255-G) for the shipped T_*_N.png.
An OpenGL copy is also written for Blender preview / glTF.
"""
import os, sys, math
import numpy as np
from PIL import Image, ImageDraw

OUT2K = sys.argv[1] if len(sys.argv) > 1 else "/tmp/dsg_r1/tex2k"
OUT4K = sys.argv[2] if len(sys.argv) > 2 else "/tmp/dsg_r1/tex4k"
os.makedirs(OUT2K, exist_ok=True)
os.makedirs(OUT4K, exist_ok=True)
SIZE = 4096  # masters; downsampled to 2048

# name: (u0, v0, u1, v1) in UV space, v up. Row 0 of the image is v = 1.
# Pylon atlas
PYLON_ATLAS = {
    "ARMOR":      (0.0,   0.5,    0.5,    1.0),
    "ARMOR_DARK": (0.5,   0.5,    1.0,    1.0),
    "TRIM_RED":   (0.0,   0.25,   0.25,   0.5),
    "BANNER":     (0.25,  0.0,    0.5,    0.5),
    "METAL":      (0.0,   0.0,    0.25,   0.25),
    "RECESS":     (0.5,   0.25,   0.75,   0.5),
    "AMBER":      (0.5,   0.0,    0.625,  0.125),
    "WHITE":      (0.625, 0.0,    0.75,   0.125),
    "CYAN":       (0.5,   0.125,  0.75,   0.25),
    "PLATE":      (0.75,  0.0,    1.0,    0.5),
}

# Core atlas (separate 2K)
CORE_ATLAS = {
    "ENERGY":     (0.0,   0.5,    0.5,    1.0),
    "ENERGY_HOT": (0.5,   0.5,    1.0,    1.0),
    "RIB":        (0.0,   0.0,    0.5,    0.5),
    "RIB_DARK":   (0.5,   0.0,    0.75,   0.5),
    "CYAN":       (0.75,  0.0,    1.0,    0.25),
    "AMBER":      (0.75,  0.25,   1.0,    0.5),
}


def hexrgb(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)], np.float32) / 255.0


def lerp3(a, b, t):
    t = np.clip(t, 0, 1)
    if np.ndim(t) and t.shape != a.shape:
        t = t[..., None]
    return a * (1 - t) + b * t


def uv_rect(atlas, name, h, w):
    u0, v0, u1, v1 = atlas[name]
    # image row 0 = v=1
    x0, x1 = int(u0 * w), int(u1 * w)
    y0, y1 = int((1 - v1) * h), int((1 - v0) * h)
    return y0, x0, y1, x1


def fbm(h, w, rng, octaves=5, base=16.0):
    acc = np.zeros((h, w), np.float32)
    amp = 0.5
    total = 0.0
    for i in range(octaves):
        gh, gw = max(2, int(h / base) * (2 ** i)), max(2, int(w / base) * (2 ** i))
        grid = rng.standard_normal((gh, gw)).astype(np.float32)
        img = Image.fromarray(grid, mode='F').resize((w, h), Image.BILINEAR)
        acc += np.asarray(img, np.float32) * amp
        total += amp
        amp *= 0.5
    acc = acc / max(total, 1e-6)
    acc = (acc - acc.min()) / max(acc.max() - acc.min(), 1e-6)
    return acc


def split_rects(y0, x0, y1, x1, rng, min_px, max_px, out, depth=0):
    h, w = y1 - y0, x1 - x0
    if h < min_px or w < min_px or depth > 7:
        out.append((y0, x0, y1, x1)); return
    if h > max_px or w > max_px or (depth < 3 and rng.random() < 0.7):
        if w >= h and w > min_px * 2:
            c = int(x0 + w * rng.uniform(0.3, 0.7))
            split_rects(y0, x0, y1, c, rng, min_px, max_px, out, depth + 1)
            split_rects(y0, c, y1, x1, rng, min_px, max_px, out, depth + 1)
        elif h > min_px * 2:
            c = int(y0 + h * rng.uniform(0.3, 0.7))
            split_rects(y0, x0, c, x1, rng, min_px, max_px, out, depth + 1)
            split_rects(c, x0, y1, x1, rng, min_px, max_px, out, depth + 1)
        else:
            out.append((y0, x0, y1, x1))
    else:
        out.append((y0, x0, y1, x1))


def height_to_normal(H, strength=2.4):
    """H in 0..1. Returns OpenGL N as RGB 0..1 (DirectX copy flips green later).

    Image row 0 is v = 1 (UV-up), so +dH/drow = -dH/dv. OpenGL Ny = dH/dv = -dH/drow.
    """
    dHdx = np.gradient(H, axis=1)   # +U (columns increase)
    dHdy = np.gradient(H, axis=0)   # +row (down the image, -V)
    nx = -dHdx * strength
    ny_gl = -dHdy * strength        # OpenGL Ny = dH/dv = -dH/drow
    nz = np.ones_like(H)
    n = np.stack([nx, ny_gl, nz], -1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True).clip(1e-6)
    return n * 0.5 + 0.5


def trident_mask(h, w):
    """Dominion trident, same construction language as hull r2 banners."""
    img = Image.new('L', (w, h), 0)
    d = ImageDraw.Draw(img)

    def P(pts, scale=0.92):
        cx, cy = w * 0.5, h * 0.52
        sx, sy = w * 0.46 * scale, h * 0.46 * scale
        xy = [(cx + x * sx, cy + (y - 0.5) * 2 * sy) for x, y in pts]
        d.polygon(xy, fill=255)

    P([(0, 0.05), (0.075, 0.17), (0.03, 0.22), (0.03, 0.62), (0.0, 0.70),
       (-0.03, 0.62), (-0.03, 0.22), (-0.075, 0.17)])
    for s in (-1, 1):
        P([(s * 0.03, 0.36), (s * 0.20, 0.31), (s * 0.29, 0.21), (s * 0.33, 0.05),
           (s * 0.40, 0.22), (s * 0.37, 0.36), (s * 0.03, 0.47)])
    P([(-0.30, 0.44), (0.0, 0.53), (0.30, 0.44), (0.30, 0.49), (0.0, 0.59), (-0.30, 0.49)])
    P([(-0.12, 0.72), (0.12, 0.72), (0.08, 0.76), (-0.08, 0.76)])
    return np.asarray(img, np.float32) / 255.0


def armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, lo, hi, rng, rough0, metal0, slits=0, vents=True):
    h, w = y1 - y0, x1 - x0
    rects = []
    split_rects(0, 0, h, w, rng, 48, 280, rects)
    g = fbm(h, w, rng, 5, 20)
    T = fbm(h, w, rng, 3, 48)
    seam = np.zeros((h, w), np.float32)
    kind = np.zeros((h, w), np.int32)
    for i, (ry0, rx0, ry1, rx1) in enumerate(rects):
        seam[ry0:ry0 + 3, rx0:rx1] = 1
        seam[ry1 - 3:ry1, rx0:rx1] = 1
        seam[ry0:ry1, rx0:rx0 + 3] = 1
        seam[ry0:ry1, rx1 - 3:rx1] = 1
        kind[ry0:ry1, rx0:rx1] = i % 3
        if vents and rng.random() < 0.18 and (ry1 - ry0) > 70 and (rx1 - rx0) > 70:
            gy0 = ry0 + int((ry1 - ry0) * rng.uniform(0.2, 0.35))
            gy1 = ry1 - int((ry1 - ry0) * rng.uniform(0.2, 0.35))
            gx0 = rx0 + int((rx1 - rx0) * rng.uniform(0.2, 0.35))
            gx1 = rx1 - int((rx1 - rx0) * rng.uniform(0.2, 0.35))
            N_h[y0 + gy0:y0 + gy1, x0 + gx0:x0 + gx1] -= 0.35
            # vent slots
            for k in range(gx0 + 4, gx1 - 4, 10):
                N_h[y0 + gy0 + 4:y0 + gy1 - 4, x0 + k:x0 + k + 4] -= 0.4
                R[y0 + gy0 + 4:y0 + gy1 - 4, x0 + k:x0 + k + 4] = 0.85
                M[y0 + gy0 + 4:y0 + gy1 - 4, x0 + k:x0 + k + 4] = 0.7
        elif rng.random() < 0.22 and (ry1 - ry0) > 55 and (rx1 - rx0) > 55:
            gy0 = ry0 + int((ry1 - ry0) * rng.uniform(0.18, 0.32))
            gy1 = ry1 - int((ry1 - ry0) * rng.uniform(0.18, 0.32))
            gx0 = rx0 + int((rx1 - rx0) * rng.uniform(0.18, 0.32))
            gx1 = rx1 - int((rx1 - rx0) * rng.uniform(0.18, 0.32))
            N_h[y0 + gy0:y0 + gy1, x0 + gx0:x0 + gx1] += 0.22
            # inset border
            N_h[y0 + gy0:y0 + gy0 + 3, x0 + gx0:x0 + gx1] -= 0.15
            N_h[y0 + gy1 - 3:y0 + gy1, x0 + gx0:x0 + gx1] -= 0.15

    t = np.clip(0.5 + 0.35 * T + (g - 0.5) * 0.8, 0, 1)
    col = lerp3(hexrgb(lo), hexrgb(hi), t)
    col = col * (0.92 + 0.16 * g)[..., None]
    col = col + seam[..., None] * (hexrgb('#16181c') - col) * 0.85
    # worn bevel highlight near seams
    from numpy.lib.stride_tricks import sliding_window_view  # noqa: unused, keep simple
    edge = np.clip(seam * 0.0 + (1 - seam), 0, 1)
    # dilate-ish highlight: just lift near seam via a cheap blur
    k = 5
    pad = np.pad(seam, k, mode='edge')
    blur = np.zeros_like(seam)
    for dy in range(-k, k + 1):
        for dx in range(-k, k + 1):
            blur += pad[k + dy:k + dy + h, k + dx:k + dx + w]
    blur /= (2 * k + 1) ** 2
    rim = np.clip(blur * 3, 0, 1) * (1 - seam)
    col = col + rim[..., None] * 0.10
    BC[y0:y1, x0:x1] = np.clip(col, 0, 1)
    N_h[y0:y1, x0:x1] += 0.08 * g + 0.18 * (1 - seam) * (0.5 + 0.5 * T) - 0.28 * seam
    R[y0:y1, x0:x1] = np.clip(rough0 + (g - 0.5) * 0.22 + seam * 0.18, 0.22, 0.92)
    M[y0:y1, x0:x1] = np.clip(metal0 + (0.5 - g) * 0.08 - seam * 0.15, 0.15, 0.85)
    if slits > 0:
        for _ in range(slits):
            sy = int(rng.uniform(8, h - 12))
            sx = int(rng.uniform(8, w - 40))
            L = int(rng.uniform(18, 36))
            E[y0 + sy:y0 + sy + 4, x0 + sx:x0 + sx + L] = hexrgb('#ff5a14')
            BC[y0 + sy:y0 + sy + 4, x0 + sx:x0 + sx + L] = hexrgb('#ff8a40')
            R[y0 + sy:y0 + sy + 4, x0 + sx:x0 + sx + L] = 0.28
            M[y0 + sy:y0 + sy + 4, x0 + sx:x0 + sx + L] = 0.0


def solid_tile(BC, N_h, R, M, E, y0, x0, y1, x1, bc, e, rough, metal, hgt=0.8):
    BC[y0:y1, x0:x1] = hexrgb(bc)
    if e:
        E[y0:y1, x0:x1] = hexrgb(e)
    R[y0:y1, x0:x1] = rough
    M[y0:y1, x0:x1] = metal
    N_h[y0:y1, x0:x1] = hgt


def energy_tile(BC, N_h, R, M, E, y0, x0, y1, x1, rng, hot=False):
    h, w = y1 - y0, x1 - x0
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    g = fbm(h, w, rng, 6, 12)
    # faceted crystalline field + radial pulse
    cx, cy = w * 0.5, h * 0.5
    rr = np.sqrt(((xx - cx) / (w * 0.5)) ** 2 + ((yy - cy) / (h * 0.5)) ** 2)
    ang = np.arctan2(yy - cy, xx - cx)
    facets = (np.abs(np.cos(ang * 6)) * 0.35 + np.abs(np.sin(ang * 3)) * 0.2)
    veins = np.clip(np.sin((xx * 0.04 + g * 18) ) * np.sin((yy * 0.05 - g * 14)), 0, 1) ** 3
    pulse = np.clip(1.0 - rr * 0.85, 0, 1) ** 1.4
    if hot:
        core = hexrgb('#e8fbff'); mid = hexrgb('#4CCFFF'); rim = hexrgb('#1a6a9a')
        e_core = hexrgb('#f5ffff'); e_mid = hexrgb('#4CCFFF'); e_rim = hexrgb('#0a6cff')
    else:
        core = hexrgb('#b8ecff'); mid = hexrgb('#4CCFFF'); rim = hexrgb('#163a5c')
        e_core = hexrgb('#dff8ff'); e_mid = hexrgb('#3ec4ff'); e_rim = hexrgb('#0b4ea8')
    t = np.clip(pulse * 0.75 + (1 - facets) * 0.2 + g * 0.15, 0, 1)
    col = lerp3(rim, mid, np.clip(t * 1.2, 0, 1))
    col = lerp3(col, core, np.clip((t - 0.55) * 2.2, 0, 1))
    col = col + veins[..., None] * (core - col) * 0.65
    emis = lerp3(e_rim, e_mid, t)
    emis = lerp3(emis, e_core, np.clip((t - 0.5) * 2.0, 0, 1))
    emis = emis * (0.55 + 0.75 * t + 0.35 * veins)[..., None]
    if hot:
        emis = emis * 1.25
    BC[y0:y1, x0:x1] = np.clip(col, 0, 1)
    E[y0:y1, x0:x1] = np.clip(emis, 0, 2)
    N_h[y0:y1, x0:x1] = 0.45 + 0.25 * facets + 0.15 * g - 0.2 * veins
    R[y0:y1, x0:x1] = np.clip(0.18 + (1 - t) * 0.25, 0.12, 0.55)
    M[y0:y1, x0:x1] = 0.05


def make_pylon():
    rng = np.random.RandomState(41041)
    h = w = SIZE
    BC = np.zeros((h, w, 3), np.float32)
    N_h = np.full((h, w), 0.5, np.float32)
    R = np.full((h, w), 0.5, np.float32)
    M = np.full((h, w), 0.4, np.float32)
    E = np.zeros((h, w, 3), np.float32)
    A = PYLON_ATLAS

    def rec(name):
        return uv_rect(A, name, h, w)

    y0, x0, y1, x1 = rec("ARMOR")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#4c525c', '#646b76', rng, 0.48, 0.52, slits=10)
    y0, x0, y1, x1 = rec("ARMOR_DARK")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#3a3f48', '#4e545e', rng, 0.52, 0.55, slits=6)
    y0, x0, y1, x1 = rec("TRIM_RED")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#9a1620', '#d42834', rng, 0.38, 0.48, slits=0, vents=False)
    y0, x0, y1, x1 = rec("METAL")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#2a2d33', '#373a41', rng, 0.42, 0.62, slits=0, vents=False)
    y0, x0, y1, x1 = rec("RECESS")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#141619', '#1c1f23', rng, 0.68, 0.55, slits=0, vents=False)
    y0, x0, y1, x1 = rec("PLATE")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#444a54', '#5a616c', rng, 0.46, 0.50, slits=4)

    y0, x0, y1, x1 = rec("BANNER")
    hh, ww = y1 - y0, x1 - x0
    g = fbm(hh, ww, rng, 4, 24)
    col = lerp3(hexrgb('#9a1620'), hexrgb('#d42834'), np.clip(0.55 + (g - 0.5) * 1.1, 0, 1))
    # darker border
    border = np.zeros((hh, ww), bool)
    mrg = max(8, ww // 18)
    border[:mrg, :] = True; border[-mrg:, :] = True; border[:, :mrg] = True; border[:, -mrg:] = True
    col = np.where(border[..., None], hexrgb('#5a1014')[None, None, :], col)
    # pale inner band
    band = np.zeros((hh, ww), bool)
    band[mrg:mrg + 6, mrg:-mrg] = True
    band[-(mrg + 6):-mrg, mrg:-mrg] = True
    col = np.where(band[..., None], hexrgb('#d8d4cc')[None, None, :], col)
    tri = trident_mask(hh, ww)
    col = col * (1 - tri[..., None]) + hexrgb('#f2f0ea')[None, None, :] * tri[..., None]
    BC[y0:y1, x0:x1] = col
    N_h[y0:y1, x0:x1] = 0.55 + 0.2 * tri + 0.08 * g - 0.25 * border
    R[y0:y1, x0:x1] = np.clip(0.42 - 0.15 * tri + 0.1 * g, 0.2, 0.7)
    M[y0:y1, x0:x1] = 0.25 + 0.2 * tri

    solid_tile(BC, N_h, R, M, E, *rec("AMBER"), '#ff7a30', '#ff5a14', 0.28, 0.0, 0.85)
    solid_tile(BC, N_h, R, M, E, *rec("WHITE"), '#ffffff', '#fff4e6', 0.28, 0.0, 0.85)
    # cyan emitter tips on the pylon
    y0, x0, y1, x1 = rec("CYAN")
    energy_tile(BC, N_h, R, M, E, y0, x0, y1, x1, rng, hot=False)

    return finish_maps(BC, N_h, R, M, E, "T_DominionShieldGenerator")


def make_core():
    rng = np.random.RandomState(41042)
    h = w = SIZE
    BC = np.zeros((h, w, 3), np.float32)
    N_h = np.full((h, w), 0.5, np.float32)
    R = np.full((h, w), 0.4, np.float32)
    M = np.full((h, w), 0.3, np.float32)
    E = np.zeros((h, w, 3), np.float32)
    A = CORE_ATLAS

    def rec(name):
        return uv_rect(A, name, h, w)

    energy_tile(BC, N_h, R, M, E, *rec("ENERGY"), rng, hot=False)
    energy_tile(BC, N_h, R, M, E, *rec("ENERGY_HOT"), rng, hot=True)
    y0, x0, y1, x1 = rec("RIB")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#3a4048', '#5a6270', rng, 0.36, 0.7, slits=0, vents=False)
    y0, x0, y1, x1 = rec("RIB_DARK")
    armor_tile(BC, N_h, R, M, E, y0, x0, y1, x1, '#1c2026', '#2c323a', rng, 0.4, 0.72, slits=0, vents=False)
    y0, x0, y1, x1 = rec("CYAN")
    energy_tile(BC, N_h, R, M, E, y0, x0, y1, x1, rng, hot=True)
    solid_tile(BC, N_h, R, M, E, *rec("AMBER"), '#ff7a30', '#ff5a14', 0.28, 0.0, 0.85)
    return finish_maps(BC, N_h, R, M, E, "T_DominionShieldGeneratorCore")


def finish_maps(BC, N_h, R, M, E, prefix):
    # packed ORM: R=AO (1), G=Rough, B=Metallic  — standard UE
    AO = np.clip(0.92 + 0.08 * (N_h - N_h.min()) / max(N_h.max() - N_h.min(), 1e-6), 0.55, 1.0)
    # darken AO in recesses (low height)
    hn = (N_h - N_h.min()) / max(N_h.max() - N_h.min(), 1e-6)
    AO = np.clip(0.55 + 0.45 * hn, 0.45, 1.0)
    ORM = np.stack([AO, np.clip(R, 0, 1), np.clip(M, 0, 1)], -1)
    N_gl = height_to_normal(N_h, strength=1.8)
    N_dx = N_gl.copy()
    N_dx[..., 1] = 1.0 - N_dx[..., 1]  # DirectX 255-G

    def save_arr(arr, path, mode='RGB'):
        a = np.clip(arr * 255.0, 0, 255).astype(np.uint8)
        Image.fromarray(a, mode).save(path, optimize=False)
        print("wrote", path, a.shape, flush=True)

    def down(src4k, dst2k):
        im = Image.open(src4k).convert('RGB').resize((2048, 2048), Image.LANCZOS)
        im.save(dst2k, optimize=False)
        print("wrote", dst2k, im.size, flush=True)

    maps = {
        f"{prefix}_BC.png": BC,
        f"{prefix}_ORM.png": ORM,
        f"{prefix}_E.png": np.clip(E, 0, 1),
        f"{prefix}_N_OpenGL.png": N_gl,
        f"{prefix}_N.png": N_dx,
    }
    for name, arr in maps.items():
        p4 = os.path.join(OUT4K, name)
        save_arr(arr, p4)
        down(p4, os.path.join(OUT2K, name))
    # E can be >1 in linear construction; already clipped for PNG
    return maps


if __name__ == "__main__":
    make_pylon()
    make_core()
    print("done textures ->", OUT2K, OUT4K)
