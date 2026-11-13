#!/usr/bin/env python3
"""Triad Drift ENG-37: original procedural VFX textures (no third-party sources).
T_TD_VFX_Noise.png  512x512 RGBA, tileable, linear:
  R = fbm gradient noise (soft clouds)            G = ridged / wispy fbm (flame tongues)
  B = worley cells (inverted distance, crackle)   A = low-frequency fbm (large-scale breakup)
usage: "<Blender 5.2>/5.2/python/bin/python.exe" gen_textures.py <out_dir>     (needs numpy only)"""
import sys, os, zlib, struct
import numpy as np


def save_png(path, img):
    """img: HxW (grey) or HxWx4 uint8"""
    h, w = img.shape[:2]
    ct = 0 if img.ndim == 2 else 6
    raw = b''.join(b'\x00' + img[y].tobytes() for y in range(h))

    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, ct, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def perlin(n, period, seed):
    rng = np.random.default_rng(seed)
    ang = rng.uniform(0, 2*np.pi, (period, period))
    gx, gy = np.cos(ang), np.sin(ang)
    lin = np.arange(n) * period / n
    x, y = np.meshgrid(lin, lin)
    x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
    fx, fy = x - x0, y - y0

    def g(ix, iy, dx, dy):
        ix = ix % period
        iy = iy % period
        return gx[iy, ix]*dx + gy[iy, ix]*dy
    u = fx*fx*fx*(fx*(fx*6-15)+10)
    v = fy*fy*fy*(fy*(fy*6-15)+10)
    a = g(x0, y0, fx, fy)
    b = g(x0+1, y0, fx-1, fy)
    c = g(x0, y0+1, fx, fy-1)
    d = g(x0+1, y0+1, fx-1, fy-1)
    top = a + u*(b-a)
    bot = c + u*(d-c)
    return top + v*(bot-top)


def fbm(n, base, octaves, seed, ridged=False):
    out = np.zeros((n, n))
    amp = 1.0
    tot = 0
    for o in range(octaves):
        p = perlin(n, base * 2**o, seed + o*17)
        if ridged:
            p = 1 - np.abs(p)*2.2
        out += p*amp
        tot += amp
        amp *= 0.5
    return out/tot


def worley(n, cells, seed):
    rng = np.random.default_rng(seed)
    grid = np.stack(np.meshgrid(np.arange(cells), np.arange(cells)), -1)
    pts = (rng.uniform(0, 1, (cells, cells, 2)) + grid) / cells
    lin = (np.arange(n)+0.5)/n
    x, y = np.meshgrid(lin, lin)
    best = np.full((n, n), 9.0)
    cx, cy = np.floor(x*cells).astype(int), np.floor(y*cells).astype(int)
    for oy in (-1, 0, 1):
        for ox in (-1, 0, 1):
            ix, iy = (cx+ox) % cells, (cy+oy) % cells
            px = pts[iy, ix, 0] + ((cx+ox)//cells)
            py = pts[iy, ix, 1] + ((cy+oy)//cells)
            best = np.minimum(best, np.hypot(x-px, y-py))
    return best*cells


def norm(a, lo=1, hi=99):
    l, h = np.percentile(a, lo), np.percentile(a, hi)
    return np.clip((a-l)/(h-l), 0, 1)


def main(out):
    n = 512
    r = norm(fbm(n, 4, 6, 11))
    g = norm(fbm(n, 4, 5, 53, ridged=True))**1.6
    b = norm(1 - worley(n, 9, 7))**1.4 * 0.75 + norm(1 - worley(n, 23, 9))*0.25
    a = norm(fbm(n, 2, 3, 91))
    img = (np.stack([r, g, b, a], -1)*255 + 0.5).astype(np.uint8)
    os.makedirs(out, exist_ok=True)
    save_png(os.path.join(out, 'T_TD_VFX_Noise.png'), np.ascontiguousarray(img))
    strip = np.concatenate([img[..., i] for i in range(4)], 1)
    save_png(os.path.join(out, 'preview_noise_channels.png'), np.ascontiguousarray(strip))


if __name__ == '__main__':
    main(sys.argv[1])
