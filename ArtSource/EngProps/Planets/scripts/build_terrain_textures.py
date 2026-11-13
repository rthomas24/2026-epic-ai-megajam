# SPDX-License-Identifier: MIT
# Tileable texture set for M_TD_Terrain (ENG-34 planet terrain), generated procedurally (numpy only, no downloads):
#   T_Terrain_Rock_M    linear masks: R albedo luminance (strata, mottling, cracks), G roughness, B cavity / AO
#   T_Terrain_Rock_N    DirectX normal map of the same rock (horizontal strata: made for side projection, V = world Z)
#   T_Terrain_Detail_M  linear masks: R pebbly soil / scree luminance, G granular drift (frost, ash) luminance,
#                       B sparse crystal facets (glints)
#   T_Terrain_Detail_N  DirectX normal map of the soil / drift mix
# Every layer is periodic (FFT-shaped noise, torus Voronoi, periodic warps), so the tiles are seamless.
#   "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" ArtSource/EngProps/Planets/scripts/build_terrain_textures.py
import os
import struct
import zlib

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), "textures")
N = 1024
PREVIEW =os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", "Saved", "TerrainMat", "tex_preview"))


def save_png(path, rgb8):
    h, w, _ = rgb8.shape
    raw = b"".join(b"\x00" + rgb8[y].tobytes() for y in range(h))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def to8(x):
    return np.clip(x * 255.0 + 0.5, 0, 255).astype(np.uint8)


def norm01(x):
    return (x - x.min()) / max(1e-9, float(x.max() - x.min()))


def fbm(rng, base, slope=1.9, aniso=(1.0, 1.0), equalise=False):
    """Spectrally shaped periodic noise. aniso scales the frequency axes (x, y): (0.25, 1) gives features 4x longer
    along x."""
    f = np.fft.fftfreq(N) * N
    fx, fy = np.meshgrid(f, f)
    r = np.sqrt((fx / aniso[0]) ** 2 + (fy / aniso[1]) ** 2)
    amp = np.where(r < 0.5, 0.0, 1.0 / np.maximum(r, 1e-6) ** slope)
    amp *= np.exp(-(np.maximum(base - r, 0.0) / (0.5 * base)) ** 2)
    spec = (rng.standard_normal((N, N)) + 1j * rng.standard_normal((N, N))) * amp
    x = np.real(np.fft.ifft2(spec))
    if equalise:
        return x.ravel().argsort().argsort().reshape(N, N) / float(N * N - 1)
    return norm01(x)


def voronoi(rng, count):
    """Torus Voronoi: returns (F1, F2, id of nearest) with distances in tile units."""
    pts = rng.random((count, 2))
    ys, xs = np.meshgrid(np.arange(N) / N, np.arange(N) / N, indexing="ij")
    f1 = np.full((N, N), 9.0)
    f2 = np.full((N, N), 9.0)
    ids = np.zeros((N, N), np.int32)
    for i, (px, py) in enumerate(pts):
        dx = np.abs(xs - px)
        dx = np.minimum(dx, 1.0 - dx)
        dy = np.abs(ys - py)
        dy = np.minimum(dy, 1.0 - dy)
        d = np.sqrt(dx * dx + dy * dy)
        closer = d < f1
        f2 = np.where(closer, f1, np.minimum(f2, d))
        ids = np.where(closer, i, ids)
        f1 = np.where(closer, d, f1)
    return f1, f2, ids


def warp(img, wx, wy):
    """Periodic domain warp (wx, wy in pixels), bilinear."""
    ys, xs = np.meshgrid(np.arange(N), np.arange(N), indexing="ij")
    x = (xs + wx) % N
    y = (ys + wy) % N
    x0 = np.floor(x).astype(int)
    y0 = np.floor(y).astype(int)
    tx = x - x0
    ty = y - y0
    x1 = (x0 + 1) % N
    y1 = (y0 + 1) % N
    x0 %= N
    y0 %= N
    return (img[y0, x0] * (1 - tx) + img[y0, x1] * tx) * (1 - ty) + (img[y1, x0] * (1 - tx) + img[y1, x1] * tx) * ty


def normal_from_height(h, strength):
    """DirectX (Unreal) tangent normal: +R along +U (columns), +G along +V (rows, downwards in the image)."""
    dx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * 0.5 * N
    dy = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * 0.5 * N
    nx, ny, nz = -dx * strength, -dy * strength, np.ones_like(h)
    ln = np.sqrt(nx * nx + ny * ny + nz * nz)
    return np.stack([nx / ln, ny / ln, nz / ln], -1) * 0.5 + 0.5


def cavity(h, radius_px):
    """Height minus its blur: > 0 on bumps, < 0 in creases (FFT gaussian, periodic)."""
    f = np.fft.fftfreq(N)
    fx, fy = np.meshgrid(f, f)
    g = np.exp(-2.0 * (np.pi * radius_px) ** 2 * (fx * fx + fy * fy))
    return h - np.real(np.fft.ifft2(np.fft.fft2(h) * g))


def rock(rng):
    # strata: a terraced 1D profile along V, bent by a periodic warp, plus long horizontal streaks
    prof_rng = np.fft.irfft(np.fft.rfft(rng.standard_normal(N)) * np.concatenate([[0], 1.0 / np.arange(1, N // 2 + 1) ** 0.9]))
    prof = norm01(prof_rng)
    terr = np.floor(prof * 5.0) / 5.0 + 0.5 * ((prof * 5.0) % 1.0) / 5.0                    # steps with sloped treads
    terr = norm01(terr)
    bend = (fbm(rng, 2.0, 2.2) - 0.5) * 420.0 + (fbm(rng, 9.0, 2.0) - 0.5) * 60.0
    rows = (np.arange(N)[:, None] + bend) % N
    r0 = np.floor(rows).astype(int) % N
    r1 = (r0 + 1) % N
    t = rows - np.floor(rows)
    strata = terr[r0] * (1 - t) + terr[r1] * t
    streaks = fbm(rng, 6.0, 2.0, aniso=(0.4, 1.0))
    lumps = fbm(rng, 5.0, 2.1)
    grain = fbm(rng, 60.0, 1.3)
    # fractures: Voronoi edges at two sizes, warped so they are not straight
    wx = (fbm(rng, 5.0, 2.0) - 0.5) * 60.0
    wy = (fbm(rng, 5.0, 2.0) - 0.5) * 60.0
    f1, f2, _ = voronoi(rng, 28)
    e_big = warp(f2 - f1, wx, wy)
    f1, f2, _ = voronoi(rng, 150)
    e_small = warp(f2 - f1, wx * 0.5, wy * 0.5)
    crack_big = 1.0 - np.clip(e_big / 0.012, 0, 1)
    crack_small = 1.0 - np.clip(e_small / 0.006, 0, 1)
    cracks = np.maximum(crack_big, crack_small * 0.6)
    height = norm01(0.30 * strata + 0.15 * streaks + 0.40 * lumps + 0.07 * grain) - 0.20 * cracks
    height = norm01(height)
    cav = cavity(height, 10.0)
    ao = np.clip(0.80 + cav * 3.2, 0.0, 1.0) * (1.0 - 0.55 * cracks)
    lum = np.clip(0.50 + (strata - 0.5) * 0.28 + (streaks - 0.5) * 0.22 + (lumps - 0.5) * 0.40 + (grain - 0.5) * 0.16
                  + cav * 1.4, 0, 1) * (1.0 - 0.6 * cracks)
    rough = np.clip(0.80 + (grain - 0.5) * 0.25 - (strata - 0.5) * 0.15 + cracks * 0.15, 0, 1)
    return np.stack([lum, rough, ao], -1), normal_from_height(height, 0.055), height


def detail(rng):
    # R: pebbly soil / scree: small rounded Voronoi stones over fine soil
    f1, f2, ids = voronoi(rng, 520)
    stone_shape = np.clip((f2 - f1) / 0.022, 0, 1) ** 0.6
    rnd = rng.random(520)
    present = (rnd[ids] > 0.35).astype(float)                      # two thirds of the cells hold a stone
    stones = stone_shape * present
    tone = 0.75 + 0.5 * rng.random(520)[ids]
    soil = fbm(rng, 40.0, 1.5)
    clods = fbm(rng, 8.0, 2.0)
    soil_h = norm01(0.55 * stones + 0.25 * soil + 0.30 * clods)
    soil_l = np.clip((0.34 + 0.22 * soil + 0.20 * clods) * (1 - present * stone_shape) + stones * 0.62 * tone
                     + cavity(soil_h, 6.0) * 1.2, 0, 1)
    # G: granular drift (frost / ash): wind ripples plus fine grain
    rows = np.arange(N)[:, None] / N                       # wind ripples: 22 waves down the tile, bent by a periodic warp
    ripples = 0.5 + 0.5 * np.sin(2 * np.pi * (rows * 22.0 + (fbm(rng, 3.0, 2.2) - 0.5) * 3.0 + (fbm(rng, 12.0, 1.8) - 0.5) * 0.8))
    grain = fbm(rng, 90.0, 1.1)
    mounds = fbm(rng, 4.0, 2.2)
    drift_h = norm01(0.35 * ripples + 0.45 * mounds + 0.12 * grain)
    drift_l = np.clip(0.70 + (mounds - 0.5) * 0.30 + (ripples - 0.5) * 0.14 + (grain - 0.5) * 0.22
                      + cavity(drift_h, 8.0) * 1.0, 0, 1)
    # B: crystal facets: a few lit Voronoi cells (sharp angular shapes), brightest at the centre
    f1, f2, ids = voronoi(rng, 1400)
    lit = (rng.random(1400)[ids] > 0.955).astype(float)
    facets = lit * np.clip((f2 - f1) / 0.010, 0, 1) * (0.5 + 0.5 * rng.random(1400)[ids])
    facets = np.tile(facets[::2, ::2], (2, 2))            # half-size facets (the 2 x 2 repeat is invisible on sparse points)
    height = norm01(0.6 * soil_h + 0.4 * drift_h)
    return np.stack([soil_l, drift_l, np.clip(facets, 0, 1)], -1), normal_from_height(height, 0.035)


def seam(img):
    a = float(np.abs(img[:, 0].astype(float) - img[:, -1].astype(float)).mean())
    b = float(np.abs(img[:, 0].astype(float) - img[:, 1].astype(float)).mean())
    return "seam %.4f interior %.4f" % (a, b)


def main():
    rng = np.random.default_rng(20261134)
    rm, rn, _ = rock(rng)
    dm, dn = detail(rng)
    for name, img in (("T_Terrain_Rock_M", rm), ("T_Terrain_Rock_N", rn), ("T_Terrain_Detail_M", dm), ("T_Terrain_Detail_N", dn)):
        path = os.path.join(OUT, name + ".png")
        save_png(path, to8(img))
        if name.endswith("_M"):                              # greyscale R | G | B strip for review
            prev = np.concatenate([np.repeat(img[::2, ::2, c:c + 1], 3, -1) for c in range(3)], 1)
            os.makedirs(PREVIEW, exist_ok=True)
            save_png(os.path.join(PREVIEW, name + "_channels.png"), to8(prev))
        print(name, "mean", np.round(img.reshape(-1, 3).mean(0), 3), seam(img), "%.1f MB" % (os.path.getsize(path) / 1e6))


main()
