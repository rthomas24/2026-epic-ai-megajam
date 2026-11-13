# SPDX-License-Identifier: MIT
"""
make_detail_normal.py - writes the tileable stone detail normal used by M_TD_PlanetKit on UV channel 1
(textures/T_PK_DetailStone_N.png, 1024 x 1024, DirectX green, seamless because the height field is built in the
frequency domain on a periodic grid).

    blender --background --factory-startup --python make_detail_normal.py
"""

import os

import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), "textures", "T_PK_DetailStone_N.png")
N = 1024


def periodic_noise(seed, n, falloff, band=None):
    rng = np.random.default_rng(seed)
    white = rng.standard_normal((n, n))
    f = np.fft.fft2(white)
    kx = np.fft.fftfreq(n)[:, None]
    ky = np.fft.fftfreq(n)[None, :]
    k = np.sqrt(kx * kx + ky * ky) + 1e-6
    amp = k ** (-falloff)
    if band is not None:
        amp *= np.exp(-((k - band[0]) / band[1]) ** 2)
    h = np.real(np.fft.ifft2(f * amp))
    h -= h.min()
    return h / max(h.max(), 1e-9)


def main():
    h = (periodic_noise(3, N, 1.6) * 0.6 + periodic_noise(7, N, 0.9, band=(0.12, 0.06)) * 0.4)
    # pits (vesicles / weathering): threshold of a mid-frequency field
    pits = periodic_noise(11, N, 1.2, band=(0.05, 0.02))
    h -= np.clip((pits - 0.72) * 3.0, 0.0, 1.0) * 0.35
    strength = 6.0
    dx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) * 0.5 * strength
    dy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) * 0.5 * strength
    nx, ny, nz = -dx, -dy, np.ones_like(h)
    ln = np.sqrt(nx * nx + ny * ny + nz * nz)
    nx, ny, nz = nx / ln, ny / ln, nz / ln
    rgba = np.zeros((N, N, 4), dtype=np.float32)
    rgba[:, :, 0] = nx * 0.5 + 0.5
    rgba[:, :, 1] = -ny * 0.5 + 0.5          # DirectX: green flipped (rows run bottom-to-top in Blender images)
    rgba[:, :, 2] = nz * 0.5 + 0.5
    rgba[:, :, 3] = 1.0
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    img = bpy.data.images.new("T_PK_DetailStone_N", N, N, alpha=False, float_buffer=False)
    img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = OUT
    img.file_format = "PNG"
    img.save()
    print("__RESULT__", OUT, float(rgba[:, :, 0].std()), float(rgba[:, :, 1].std()))


main()
