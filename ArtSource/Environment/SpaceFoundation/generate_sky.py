"""Original deterministic Triad Drift sky texture authoring. Python stdlib only.

Does not run on import. Requires --write-assets to create PNGs. Source-only
review can call sample_nebula()/validate() without creating any binary files.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import random
import struct
import zlib

TAU = math.tau
ROOT = Path(__file__).resolve().parent


def clamp(x, low=0.0, high=1.0):
    return max(low, min(high, x))


def noise(x, y, z):
    """Continuous value noise in sphere space; longitude seam is not special."""
    ix, iy, iz = math.floor(x), math.floor(y), math.floor(z)
    fx, fy, fz = x - ix, y - iy, z - iz
    fx, fy, fz = (v * v * (3 - 2 * v) for v in (fx, fy, fz))

    def h(a, b, c):
        n = (a * 73856093 ^ b * 19349663 ^ c * 83492791 ^ 90210) & 0xFFFFFFFF
        n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
        return ((n ^ (n >> 16)) & 0xFFFF) / 65535.0

    def lerp(a, b, t):
        return a + (b - a) * t

    planes = []
    for k in (0, 1):
        a = lerp(h(ix, iy, iz + k), h(ix + 1, iy, iz + k), fx)
        b = lerp(h(ix, iy + 1, iz + k), h(ix + 1, iy + 1, iz + k), fx)
        planes.append(lerp(a, b, fy))
    return lerp(*planes, fz)


def sphere(u, v):
    longitude = u * TAU
    latitude = (0.5 - v) * math.pi
    c = math.cos(latitude)
    return c * math.cos(longitude), c * math.sin(longitude), math.sin(latitude)


def sample_nebula(u, v):
    """Returns display-referred RGB in [0,1], authored for ~EV 0 fixed exposure.

    Two curved cloud banks frame a broad dark combat corridor. No camera-space
    cutouts: composition rotates naturally with the sphere rather than the HUD.
    """
    x, y, z = sphere(u, v)
    broad = noise(x * 2.0 + 12, y * 2.0 - 3, z * 2.0 + 8)
    detail = noise(x * 6.3 + 3, y * 6.3 + 19, z * 6.3 - 6)
    fine = noise(x * 16.5 - 2, y * 16.5 + 8, z * 16.5 + 11)
    warp = (broad - 0.5) * 0.44
    band = z + 0.28 * x - 0.16 * math.sin(y * 4.0) + warp
    cloud = math.exp(-((band - 0.21) / 0.22) ** 2)
    cloud *= clamp((0.20 + 0.54 * broad + 0.25 * detail + 0.11 * fine - 0.35) * 2.4)
    wisps = math.exp(-((band + 0.25) / 0.13) ** 2) * (0.3 + detail * 0.7)
    dust = 1.0 - 0.78 * math.exp(-((band - 0.02 - (detail - 0.5) * 0.1) / 0.065) ** 2)
    teal = clamp(0.5 + x * 0.45 + y * 0.3)
    violet = 1.0 - teal
    density = cloud * dust
    # Saturated nebulae stay below white; stars remain the brightest elements.
    return (
        clamp(0.006 + density * (0.045 * teal + 0.38 * violet) + wisps * 0.07),
        clamp(0.011 + density * (0.34 * teal + 0.045 * violet) + wisps * 0.025),
        clamp(0.032 + density * (0.39 * teal + 0.37 * violet) + wisps * 0.14),
    )


def png(path, width, height, pixels):
    """Write RGB PNG with no third-party image library."""
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xFFFFFFFF)
    rows = b''.join(b'\0' + bytes(pixels[y * width * 3:(y + 1) * width * 3]) for y in range(height))
    data = b'\x89PNG\r\n\x1a\n'
    data += chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    data += chunk(b'sRGB', b'\0')
    data += chunk(b'IDAT', zlib.compress(rows, 6)) + chunk(b'IEND', b'')
    path.write_bytes(data)


def make_stars(width, height, count=1600, seed=117):
    pixels = bytearray(width * height * 3)
    rng = random.Random(seed)
    for i in range(count):
        # Uniform solid angle, not uniform latitude: avoids pole pin-cushions.
        u = rng.random()
        v = math.acos(rng.uniform(-1, 1)) / math.pi
        px, py = int(u * width) % width, min(height - 1, int(v * height))
        bright = 0.24 + 0.76 * rng.random() ** 4
        radius = 2 if i < 24 else 1
        warm = rng.random() > 0.79
        color = (1.0, 0.79, 0.62) if warm else (0.67, 0.86, 1.0)
        for oy in range(-radius, radius + 1):
            yy = py + oy
            if not 0 <= yy < height:
                continue
            for ox in range(-radius, radius + 1):
                value = bright * math.exp(-(ox * ox + oy * oy) / (0.38 if radius == 1 else 1.0))
                index = (yy * width + (px + ox) % width) * 3
                for c in range(3):
                    pixels[index + c] = min(255, pixels[index + c] + round(value * color[c] * 255))
    return pixels


def validate():
    for u in (0, 0.17, 0.41, 0.79, 1):
        for v in (0, 0.1, 0.5, 0.9, 1):
            assert all(math.isfinite(c) and 0 <= c <= 1 for c in sample_nebula(u, v))
    assert max(abs(a - b) for v in (0.1, 0.4, 0.8) for a, b in zip(sample_nebula(0, v), sample_nebula(1, v))) < 1e-10
    for v in (0, 1):
        assert max(abs(a - b) for a, b in zip(sample_nebula(0, v), sample_nebula(0.37, v))) < 1e-10
    assert len(make_stars(64, 32, 12)) == 64 * 32 * 3
    assert make_stars(64, 32, 12) == make_stars(64, 32, 12)
    return 'range, seam, poles, deterministic star buffer: PASS'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-assets', action='store_true', help='Required opt-in after claims are published')
    parser.add_argument('--width', type=int, choices=(512, 1024, 2048), default=1024)
    args = parser.parse_args()
    print(validate())
    if not args.write_assets:
        print('Source-only validation. No binary files written.')
        return
    out = ROOT / 'export'
    for name in ('T_TD_Nebula_BC.png', 'T_TD_Stars_BC.png'):
        if (out / name).exists():
            raise RuntimeError('Refusing to overwrite existing source texture: ' + str(out / name))
    out.mkdir(exist_ok=True)
    width, height = args.width, args.width // 2
    pixels = bytearray()
    for y in range(height):
        for x in range(width):
            pixels.extend(round(c * 255) for c in sample_nebula(x / width, y / (height - 1)))
    png(out / 'T_TD_Nebula_BC.png', width, height, pixels)
    png(out / 'T_TD_Stars_BC.png', 2048, 1024, make_stars(2048, 1024))
    print(json.dumps({'nebula': [width, height], 'stars': [2048, 1024], 'destination': str(out)}))


if __name__ == '__main__':
    main()
