# SPDX-License-Identifier: MIT
# Triad Drift - small generated, tileable textures for the ship materials (no third-party data, seeded):
#   textures/T_TD_PortalNoise.png   256, greyscale cloud noise (two octave groups) for M_TD_RiftPortal
#   textures/T_TD_ShipDetail_N.png  512, DirectX tangent-space normal: fine brushed / hammered metal grain with
#                                   sparse pits, isotropic (the Rodin UV islands point every way, so no lines)
#   blender --background --factory-startup --python rs_textures.py && python rs_textures_png.py
import os

import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), "textures")


def fft_noise(n, beta, rng, lo=1.0, hi=None):
    """Tileable noise with a 1/f^beta spectrum, band-limited to [lo, hi] cycles, normalised to 0..1."""
    fx = np.fft.fftfreq(n) * n
    f = np.sqrt(fx[None, :] ** 2 + fx[:, None] ** 2)
    amp = np.where(f > 0, 1.0 / np.maximum(f, 1e-6) ** beta, 0.0)
    amp[f < lo] = 0.0
    if hi:
        amp[f > hi] = 0.0
    spec = amp * np.exp(2j * np.pi * rng.random((n, n)))
    img = np.real(np.fft.ifft2(spec))
    img -= img.min()
    return img / max(img.max(), 1e-9)


def save(name, rgb, colorspace):
    """Blender's image writer produced empty files headless, so the pixels go out as raw RGB8 and
    rs_textures_png.py (system Python + Pillow) wraps them as PNG."""
    h, w = rgb.shape[:2]
    path = os.path.join(OUT, "%s_%dx%d.rgb8" % (name, w, h))
    (np.clip(rgb, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8).tofile(path)
    return path


def main():
    os.makedirs(OUT, exist_ok=True)
    rng = np.random.default_rng(20261113)
    # portal noise: soft clouds with wispy detail, contrast-stretched
    n = 256
    clouds = 0.7 * fft_noise(n, 1.7, rng, 2, 24) + 0.3 * fft_noise(n, 1.0, rng, 12, 64)
    clouds = np.clip((clouds - 0.25) / 0.55, 0.0, 1.0) ** 1.3
    p1 = save("T_TD_PortalNoise", np.repeat(clouds[..., None], 3, 2).astype(np.float32), "Non-Color")
    # ship detail normal: height = fine grain + medium mottling + a few pits
    n = 512
    height = 0.55 * fft_noise(n, 0.6, rng, 24, 200) + 0.35 * fft_noise(n, 1.4, rng, 4, 40)
    pits = fft_noise(n, 0.2, rng, 60, 220)
    height -= 0.6 * np.clip((pits - 0.82) / 0.18, 0.0, 1.0)
    gx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 0.5
    gy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 0.5
    strength = 6.0
    nx, ny, nz = -gx * strength, gy * strength, np.ones_like(gx)      # DirectX: green down
    length = np.sqrt(nx * nx + ny * ny + nz * nz)
    normal = np.stack([nx / length, ny / length, nz / length], -1) * 0.5 + 0.5
    p2 = save("T_TD_ShipDetail_N", normal.astype(np.float32), "Non-Color")
    print("__RESULT__", p1, os.path.getsize(p1), p2, os.path.getsize(p2))


main()
