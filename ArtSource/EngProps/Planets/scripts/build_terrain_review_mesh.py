# SPDX-License-Identifier: MIT
# Review terrain for M_TD_Terrain (numpy; no Unreal): an 8 x 8 km displaced grid with ridged-noise mountains 0-2,000 m
# around a flat basin, carrying exactly the vertex channels ATDPlanetTerrain writes (TDPlanetTerrain.cpp):
#   UV0 = world XY / 1000, UV1 = (height m, slope), colour = (height01, slope, broad mask, fine mask), slope = 1 - N.z
# A coarse 20 m grid plus one fine 2.5 m patch on the steepest face (cliff close-up / triplanar check).
# Cinder also carves river valleys along the 0.5 contour of the broad mask (see the note in ue_terrain_material.py).
# Writes Saved/TerrainMat/terrain_<Planet>.bin (+ .json with camera views) for ue_render_terrain_review.py.
#   "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" ArtSource/EngProps/Planets/scripts/build_terrain_review_mesh.py
import json
import os
import struct

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
OUT = os.path.join(REPO, "Saved", "TerrainMat")
HALF = 400000.0          # cm
STEP = 2000.0
BASIN = 60000.0          # half extent of the flat basin (cm)
BLEND = 90000.0
PEAK = 200000.0
FLOOR = -60.0


def hash_cell(x, y, seed):
    h = (x.astype(np.uint32) * np.uint32(0x8da6b343)) ^ (y.astype(np.uint32) * np.uint32(0xd8163841)) ^ np.uint32((seed * 0xcb1ab31f) & 0xFFFFFFFF)
    h ^= h >> np.uint32(13)
    h *= np.uint32(0x5bd1e995)
    h ^= h >> np.uint32(15)
    return (h & np.uint32(0xffffff)).astype(np.float64) / float(0xffffff) * 2.0 - 1.0


def noise(x, y, seed):
    x0 = np.floor(x)
    y0 = np.floor(y)
    tx = x - x0
    ty = y - y0
    tx = tx * tx * tx * (tx * (tx * 6 - 15) + 10)
    ty = ty * ty * ty * (ty * (ty * 6 - 15) + 10)
    xi = x0.astype(np.int64)
    yi = y0.astype(np.int64)
    a = hash_cell(xi, yi, seed)
    b = hash_cell(xi + 1, yi, seed)
    c = hash_cell(xi, yi + 1, seed)
    d = hash_cell(xi + 1, yi + 1, seed)
    return (a + (b - a) * tx) * (1 - ty) + (c + (d - c) * tx) * ty


def fbm(x, y, octaves, seed):
    s = 0.0
    amp, freq, nrm = 0.5, 1.0, 0.0
    for i in range(octaves):
        s = s + amp * noise(x * freq, y * freq, seed + i * 101)
        nrm += amp
        amp *= 0.5
        freq *= 2.03
    return s / nrm


def ridged(x, y, octaves, seed, sharp):
    s = 0.0
    amp, freq, nrm = 0.5, 1.0, 0.0
    weight = 1.0
    power = 1.4 + 1.2 * sharp
    for i in range(octaves):
        ca, sa = np.cos(i * 0.9 + 0.4), np.sin(i * 0.9 + 0.4)       # rotate each octave: no lattice-aligned ridges
        n = 1.0 - np.abs(noise((x * ca - y * sa) * freq, (x * sa + y * ca) * freq, seed + 7 + i * 131))
        n = np.maximum(n, 0.0) ** power * weight
        weight = np.clip(n * 1.6, 0.0, 1.0)
        s = s + n * amp
        nrm += amp
        amp *= 0.5
        freq *= 2.1
    return s / nrm


class Field:
    def __init__(self, planet):
        self.cinder = planet == "Cinder"
        self.seed = 7331 if self.cinder else 4242
        self.sharp = 0.9 if self.cinder else 0.35

    def broad(self, x, y):
        return fbm(x / 250000.0, y / 250000.0, 3, self.seed + 71) * 0.5 + 0.5

    def height(self, x, y):
        s = self.seed
        wx = x + 60000.0 * noise(x / 300000.0, y / 300000.0, s + 11)
        wy = y + 60000.0 * noise(x / 300000.0 + 31.7, y / 300000.0 - 12.3, s + 13)
        hills = fbm(wx / 90000.0, wy / 90000.0, 5, s + 1) * 0.5 + 0.5
        mount = ridged(wx / 260000.0, wy / 260000.0, 6, s + 5, self.sharp)
        rng_mask = 0.25 + 0.75 * np.clip((fbm(wx / 420000.0, wy / 420000.0, 3, s + 3) + 0.3) / 0.65, 0.0, 1.0)
        h = hills * 16000.0 + mount ** 1.7 * rng_mask * (PEAK - 16000.0) * 1.6
        if self.cinder:
            rn = (self.broad(x, y) - 0.5) * 2.0
            h = h - np.exp(-(rn * 9.0) ** 2) * 9000.0
        dx = np.maximum(0.0, np.abs(x) - BASIN)
        dy = np.maximum(0.0, np.abs(y) - BASIN)
        t = np.clip(np.sqrt(dx * dx + dy * dy) / BLEND, 0.0, 1.0)
        w = t * t * t * (t * (t * 6 - 15) + 10)
        return FLOOR + (np.minimum(h, PEAK) - FLOOR) * w

    def sample(self, x, y, step):
        h = self.height(x, y)
        hx = self.height(x + step, y) - self.height(x - step, y)
        hy = self.height(x, y + step) - self.height(x, y - step)
        n = np.stack([-hx, -hy, np.full_like(h, 2.0 * step)], -1)
        n /= np.linalg.norm(n, axis=-1, keepdims=True)
        slope = np.clip(1.0 - n[..., 2], 0.0, 1.0)
        h01 = np.clip((h - FLOOR) / PEAK, 0.0, 1.0)
        fine = noise(x / 6000.0, y / 6000.0, self.seed + 73) * 0.5 + 0.5
        return h, n, slope, h01, self.broad(x, y), fine


def grid(field, x0, y0, x1, y1, step, hole=None):
    xs = np.arange(x0, x1 + step * 0.5, step)
    ys = np.arange(y0, y1 + step * 0.5, step)
    gx, gy = np.meshgrid(xs, ys)
    h, n, slope, h01, broad, fine = field.sample(gx, gy, step * 0.5)
    nx, ny = len(xs), len(ys)
    idx = np.arange(nx * ny).reshape(ny, nx)
    a, b_, c, d = idx[:-1, :-1], idx[:-1, 1:], idx[1:, :-1], idx[1:, 1:]
    keep = np.ones(a.shape, bool)
    if hole is not None:
        cx, cy = gx[:-1, :-1] + step * 0.5, gy[:-1, :-1] + step * 0.5
        keep = ~((cx > hole[0]) & (cx < hole[2]) & (cy > hole[1]) & (cy < hole[3]))
    # counter-clockwise seen from above in a right-handed frame = front-facing upwards for a ProceduralMesh in Unreal
    tris = np.concatenate([np.stack([a[keep], c[keep], b_[keep]], -1), np.stack([b_[keep], c[keep], d[keep]], -1)])
    pos = np.stack([gx, gy, h], -1).reshape(-1, 3)
    uv1 = np.stack([h / 100.0, slope], -1).reshape(-1, 2)
    col = np.stack([h01, slope, broad, fine], -1).reshape(-1, 4)
    return pos, n.reshape(-1, 3), uv1, col, tris.astype(np.int32), (gx, gy, h, slope, broad)


def write(planet):
    f = Field(planet)
    _, _, _, _, _, (gx, gy, h, slope, broad) = grid(f, -HALF, -HALF, HALF, HALF, STEP)
    r = np.sqrt(gx * gx + gy * gy)
    # fine patch on the steepest face 1.6-3 km out
    box = sum(np.roll(np.roll(slope, i, 0), k, 1) for i in range(-3, 4) for k in range(-3, 4)) / 49.0   # a broad steep face
    cand = np.where((r > 160000) & (r < 300000), box, -1.0)
    iy, ix = np.unravel_index(np.argmax(cand), cand.shape)
    px, py = gx[iy, ix], gy[iy, ix]
    hole = (px - 24000, py - 24000, px + 24000, py + 24000)
    sections = [grid(f, -HALF, -HALF, HALF, HALF, STEP, hole)[:5], grid(f, hole[0], hole[1], hole[2], hole[3], 250.0)[:5]]
    path = os.path.join(OUT, "terrain_%s.bin" % planet)
    with open(path, "wb") as fh:
        fh.write(struct.pack("<I", len(sections)))
        for pos, nrm, uv1, col, tris in sections:
            fh.write(struct.pack("<II", len(pos), len(tris)))
            for arr in (pos, nrm, uv1, col):
                fh.write(arr.astype("<f4").tobytes())
            fh.write(tris.astype("<i4").tobytes())

    def ground(x, y):
        return float(f.height(np.array([x]), np.array([y]))[0])

    def at(x, y, up):
        return [float(x), float(y), ground(x, y) + up]
    # cliff: stand 30 m from the steepest point on its downhill side, look at the face
    e = 500.0
    gxn = ground(px + e, py) - ground(px - e, py)
    gyn = ground(px, py + e) - ground(px, py - e)
    gl = max(1e-6, (gxn * gxn + gyn * gyn) ** 0.5)
    dxn, dyn = -gxn / gl, -gyn / gl
    cx, cy = px + dxn * 7000.0, py + dyn * 7000.0
    # peak: the highest point; air view 300 m above ground, 1.8 km before the peak on the origin side
    iy2, ix2 = np.unravel_index(np.argmax(h), h.shape)
    kx, ky, kz = gx[iy2, ix2], gy[iy2, ix2], h[iy2, ix2]
    kl = max(1.0, (kx * kx + ky * ky) ** 0.5)
    ax, ay = kx - kx / kl * 200000.0, ky - ky / kl * 200000.0
    # valley: lowest flat point 1.7-3 km out (Cinder: on a river); fly 35 m above it, look towards the peak
    score = np.where((r > 170000) & (r < 300000) & (slope < 0.05), -h - (np.abs(broad - 0.5) * 4e5 if f.cinder else 0.0), -1e12)
    iy3, ix3 = np.unravel_index(np.argmax(score), score.shape)
    vx, vy = gx[iy3, ix3], gy[iy3, ix3]
    views = {
        "air300": dict(loc=at(ax, ay, 30000.0), look=[float(kx), float(ky), float(kz) * 0.55]),
        "valley": dict(loc=at(vx, vy, 3500.0), look=[float(vx + (kx - vx) * 0.5), float(vy + (ky - vy) * 0.5), ground(vx, vy) + 2000.0]),
        "eye_basin": dict(loc=[BASIN - 9000.0, 4000.0, 170.0], look=[BASIN + 150000.0, 30000.0, 9000.0]),     # on the slab, 40 m before its edge
        "seam_air": dict(loc=[BASIN - 30000.0, -20000.0, 10000.0], look=[BASIN + 20000.0, 10000.0, 0.0]),            # arrival altitude over the slab edge
        "cliff": dict(loc=[float(cx), float(cy), ground(px, py) + 500.0], look=[float(px), float(py), ground(px, py)]),   # level with the face, 70 m out
        "far5km": dict(loc=[-330000.0, -60000.0, 500000.0], look=[60000.0, 20000.0, 0.0]),
    }
    with open(os.path.join(OUT, "terrain_%s.json" % planet), "w") as fh:
        json.dump(dict(views=views, basin=BASIN, patch=[float(px), float(py)]), fh, indent=1)
    print(planet, "verts", sum(len(s[0]) for s in sections), "tris", sum(len(s[4]) for s in sections),
          "h %.0f..%.0f m" % (h.min() / 100, h.max() / 100), "max slope %.2f" % slope.max(),
          "slope>0.3: %.1f%%" % (100 * (slope > 0.3).mean()), "%.1f MB" % (os.path.getsize(path) / 1e6))


os.makedirs(OUT, exist_ok=True)
for p in ("Verdance", "Cinder"):
    write(p)
