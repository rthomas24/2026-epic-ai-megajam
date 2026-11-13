# SPDX-License-Identifier: MIT
"""
dm_tiles.py - the Dominion's tiling hull-panel textures (Triad Drift), Blender 5.2 headless, numpy only.

    blender --background --factory-startup --python dm_tiles.py -- [--size 2048] [--seed 7]

Writes ArtSource/EngShips/Dominion/textures/:
  T_Dominion_PanelMask.png  RGBA, linear data:
      R  plate tone (0..1, per plate)            G  cavity: 1 open surface, 0 in seams / grooves / rivet rings
      B  height (0 seam floor .. 1 raised plate)  A  light strips (red-orange emissive mask, sparse)
  T_Dominion_PanelN.png     RGB, linear: R = 0.5 + 0.5 * n_u, G = 0.5 + 0.5 * n_v, B = n_w, where u, v are the
                            texture's UV axes in UNREAL's convention (u right, v DOWN the image). The Unreal
                            material samples it in its own triplanar code (no tangent basis), so these are
                            exactly the slopes along the projection's two axes.

The tile is seamless: plates come from a recursive split of a 4 x 4 grid of cells that align with the tile edges.
One tile covers 32 m on the hull (1.56 cm per texel at 2048).
"""

import math
import os
import random
import sys

import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), "textures")


def smooth(x):
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3.0 - 2.0 * x)


def split_cells(size, rng):
    """Leaves (x0, y0, x1, y1) in texels (top-down rows), seamless (cells align with the tile)."""
    base = size // 4
    leaves = []

    def rec(x0, y0, x1, y1, depth):
        w, h = x1 - x0, y1 - y0
        stop = depth >= 5 or max(w, h) < size // 28 or (depth >= 2 and rng.random() < 0.18 + 0.1 * depth)
        if stop:
            leaves.append((x0, y0, x1, y1))
            return
        horiz = w >= h if rng.random() < 0.8 else w < h
        q = size // 128
        if horiz:
            c = x0 + int(round(w * rng.uniform(0.28, 0.72) / q)) * q
            c = min(max(c, x0 + q), x1 - q)
            rec(x0, y0, c, y1, depth + 1)
            rec(c, y0, x1, y1, depth + 1)
        else:
            c = y0 + int(round(h * rng.uniform(0.28, 0.72) / q)) * q
            c = min(max(c, y0 + q), y1 - q)
            rec(x0, y0, x1, c, depth + 1)
            rec(x0, c, x1, y1, depth + 1)
    # a few 2-cell plates (long armour slabs) so the grid of cells doesn't show
    used = set()
    for cy in range(4):
        for cx in range(4):
            if (cx, cy) in used:
                continue
            if cx < 3 and (cx + 1, cy) not in used and rng.random() < 0.22:
                used |= {(cx, cy), (cx + 1, cy)}
                rec(cx * base, cy * base, (cx + 2) * base, (cy + 1) * base, 1)
            else:
                used.add((cx, cy))
                rec(cx * base, cy * base, (cx + 1) * base, (cy + 1) * base, 0)
    return leaves


def build(size=2048, seed=7):
    rng = random.Random(seed)
    s = size / 2048.0
    H = np.zeros((size, size), np.float32)
    tone = np.zeros((size, size), np.float32)
    cav = np.ones((size, size), np.float32)
    light = np.zeros((size, size), np.float32)
    gap = max(2, int(round(5 * s)))          # seam gap (texels): ~8 cm
    for (x0, y0, x1, y1) in split_cells(size, rng):
        w, h = x1 - x0, y1 - y0
        ix0, iy0, ix1, iy1 = x0 + gap // 2 + 1, y0 + gap // 2 + 1, x1 - (gap - gap // 2), y1 - (gap - gap // 2)
        if ix1 - ix0 < 4 or iy1 - iy0 < 4:
            continue
        yy, xx = np.mgrid[iy0:iy1, ix0:ix1].astype(np.float32)
        d = np.minimum(np.minimum(xx - ix0, ix1 - 1 - xx), np.minimum(yy - iy0, iy1 - 1 - yy))
        bevel = max(3.0, 7.0 * s)
        kind = rng.random()
        area = w * h / float(size * size)
        level = 1.0 if kind < 0.72 else 0.55          # inset plates sit lower
        plate = level * smooth(d / bevel)
        t = rng.random()
        H[iy0:iy1, ix0:ix1] = plate
        tone[iy0:iy1, ix0:ix1] = t
        # cavity darkening hugging the seam
        cav[iy0:iy1, ix0:ix1] = np.minimum(cav[iy0:iy1, ix0:ix1], 0.55 + 0.45 * smooth(d / (bevel * 2.5)))
        pw, ph = ix1 - ix0, iy1 - iy0
        sub = (slice(iy0, iy1), slice(ix0, ix1))
        if 0.72 <= kind < 0.84 and min(pw, ph) > 60 * s:
            # vent: parallel grooves along the long axis inside an inset frame
            m = int(14 * s)
            along_x = pw >= ph
            inner = (d > m)
            coord = (yy - iy0) if along_x else (xx - ix0)
            period = max(6.0, 12.0 * s)
            groove = 0.5 + 0.5 * np.cos(coord / period * 2.0 * math.pi)
            vent = 0.55 - 0.35 * smooth((groove - 0.35) / 0.4)
            Hs = H[sub]
            Hs[inner] = np.minimum(Hs[inner], vent[inner] * level)
            cs = cav[sub]
            cs[inner] = np.minimum(cs[inner], 0.45 + 0.4 * groove[inner])
        elif 0.86 <= kind < 0.88 and min(pw, ph) > 40 * s:
            # light strip: a narrow glowing slot along the long side
            along_x = pw >= ph
            m = int(10 * s)
            if along_x:
                c = iy0 + int(ph * rng.choice((0.25, 0.75)))
                sy = slice(max(iy0, c - int(3 * s)), c + int(3 * s))
                sx = slice(ix0 + m, ix1 - m)
            else:
                c = ix0 + int(pw * rng.choice((0.25, 0.75)))
                sx = slice(max(ix0, c - int(3 * s)), c + int(3 * s))
                sy = slice(iy0 + m, iy1 - m)
            light[sy, sx] = 1.0
            H[sy, sx] = 0.35
            cav[sy, sx] = 0.5
        elif kind >= 0.90 and min(pw, ph) > 50 * s:
            # hatch: an inner chamfered rectangle outline + rivets
            m = int(min(pw, ph) * 0.18)
            ring = (np.abs(d - m) < 2.0 * s)
            Hs = H[sub]
            Hs[ring] = 0.6 * level
            cav[sub][ring] = 0.5
        # rivet rows along the edges of large plates
        if area > 0.004 and rng.random() < 0.6:
            step = int(36 * s)
            inset = int(9 * s)
            r = 2.2 * s
            pts = []
            for x in range(ix0 + inset, ix1 - inset, step):
                pts += [(x, iy0 + inset), (x, iy1 - 1 - inset)]
            for y in range(iy0 + inset + step, iy1 - inset - step // 2, step):
                pts += [(ix0 + inset, y), (ix1 - 1 - inset, y)]
            for (px, py) in pts:
                ys, xs = slice(max(0, py - 4), py + 5), slice(max(0, px - 4), px + 5)
                gy, gx = np.mgrid[ys, xs].astype(np.float32)
                rr = np.sqrt((gx - px) ** 2 + (gy - py) ** 2)
                bump = np.clip(1.0 - rr / r, 0.0, 1.0) * 0.18
                H[ys, xs] += bump
                cav[ys, xs] = np.minimum(cav[ys, xs], np.where(np.abs(rr - r) < 1.0, 0.7, 1.0))
    # fine grain so plates aren't perfectly flat (tileable: sum of integer-frequency sines)
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32) / size * 2.0 * math.pi
    grain = np.zeros_like(H)
    for k in range(12):
        fx, fy = rng.randint(3, 40), rng.randint(3, 40)
        grain += np.sin(fx * xx + fy * yy + rng.random() * 6.28) / (1.0 + 0.15 * (fx + fy))
    grain /= np.abs(grain).max() + 1e-6
    H = np.clip(H + 0.02 * grain * (H > 0.2), 0.0, 1.2)
    # normal from height (wrap-around central differences); slope scale: 1.0 height = 3.5 cm, 1 texel = 1.56 cm
    k = 3.5 / (32.0 * 100.0 / size)
    dhu = (np.roll(H, -1, axis=1) - np.roll(H, 1, axis=1)) * 0.5 * k
    dhv = (np.roll(H, -1, axis=0) - np.roll(H, 1, axis=0)) * 0.5 * k      # rows go DOWN = +v in Unreal UVs
    nu, nv, nw = -dhu, -dhv, np.ones_like(H)
    ln = np.sqrt(nu * nu + nv * nv + nw * nw)
    nu, nv, nw = nu / ln, nv / ln, nw / ln
    mask = np.stack([tone, cav, np.clip(H / 1.2, 0, 1), light], axis=-1)
    nrm = np.stack([0.5 + 0.5 * nu, 0.5 + 0.5 * nv, nw, np.ones_like(nw)], axis=-1)
    return mask, nrm


def save(arr, path, alpha=True):
    h, w = arr.shape[:2]
    img = bpy.data.images.new(os.path.basename(path)[:-4], width=w, height=h, alpha=alpha, float_buffer=False)
    img.colorspace_settings.name = "Non-Color"
    flat = np.flipud(arr).astype(np.float32)          # Blender rows run bottom-up
    if arr.shape[2] == 3:
        flat = np.concatenate([flat, np.ones(flat.shape[:2] + (1,), np.float32)], axis=-1)
    img.pixels.foreach_set(flat.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    size, seed = 2048, 7
    if "--size" in argv:
        size = int(argv[argv.index("--size") + 1])
    if "--seed" in argv:
        seed = int(argv[argv.index("--seed") + 1])
    os.makedirs(OUT, exist_ok=True)
    mask, nrm = build(size, seed)
    save(mask, os.path.join(OUT, "T_Dominion_PanelMask.png"), alpha=True)
    save(nrm, os.path.join(OUT, "T_Dominion_PanelN.png"), alpha=False)
    print("__RESULT__" + str({"size": size, "light_share": float(mask[..., 3].mean()),
                              "seam_share": float((mask[..., 2] < 0.05).mean())}))


if __name__ == "__main__":
    main()
