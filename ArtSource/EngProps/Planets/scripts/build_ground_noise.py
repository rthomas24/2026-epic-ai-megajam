# Tileable macro-noise texture for M_TD_Ground's anti-tiling layer: T_Ground_MacroNoise (512, linear, RGB).
#   R: broad fbm (macro value / tint / roughness), base ~3 cycles per tile
#   G: finer fbm (selects between the two scales of the ground texture), base ~9 cycles per tile
#   B: smooth fbm whose 0.5 iso-line M_TD_Ground turns into lava rivers (ridge = 1 - |2B - 1|), base ~4 cycles
# Each channel is spectrally shaped white noise (FFT, so it tiles exactly) then rank-equalised to a uniform 0..1,
# so the thresholds in the material mean the same coverage on every channel.
# Needs numpy only (Blender's bundled Python has it):
#   "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" ArtSource/EngProps/Planets/scripts/build_ground_noise.py
import os
import struct
import zlib

import numpy as np


def save_png(path, rgb8):
    h, w, _ = rgb8.shape
    raw = b"".join(b"\x00" + rgb8[y].tobytes() for y in range(h))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), "textures", "T_Ground_MacroNoise.png")
N = 512


def fbm(rng, base, slope=1.9, lo_cut=1.0):
    f = np.fft.fftfreq(N) * N
    fx, fy = np.meshgrid(f, f)
    r = np.sqrt(fx * fx + fy * fy)
    amp = np.where(r < lo_cut, 0.0, 1.0 / np.maximum(r, 1e-6) ** slope)
    amp *= np.exp(-(np.maximum(base - r, 0.0) / (0.5 * base)) ** 2)      # weaken frequencies under the base
    spec = (rng.standard_normal((N, N)) + 1j * rng.standard_normal((N, N))) * amp
    x = np.real(np.fft.ifft2(spec))
    rank = x.ravel().argsort().argsort().reshape(N, N)
    return rank / float(N * N - 1)


def main():
    rng = np.random.default_rng(20261113)
    r = fbm(rng, 3.0, 2.0)
    g = fbm(rng, 9.0, 1.7)
    b = fbm(rng, 4.0, 2.2)
    img = np.stack([r, g, b], -1)
    save_png(OUT, np.clip(img * 255.0 + 0.5, 0, 255).astype(np.uint8))
    seam = float(np.abs(img[:, 0] - img[:, -1]).mean())
    inner = float(np.abs(img[:, 0] - img[:, 1]).mean())
    print("wrote", OUT, "seam diff %.4f vs interior %.4f" % (seam, inner))


main()
