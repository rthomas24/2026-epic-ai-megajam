# SPDX-License-Identifier: MIT
# Triad Drift - the approved recolours of Nicholas's three Fab props (issues #175-#177, Nicholas: "Approve all three
# adaptations", Nov 14 2026). Texture edits only; the mesh, normal and ORM maps are unchanged.
#   blender --background --factory-startup --python recolour_fab.py -- <repo>/ArtSource/EngProps/Fab
# Reads export/T_Fab_<Asset>_*_orig.png (the unmodified seller colours, kept) and writes export/T_Fab_<Asset>_BC.png
# (and _E for the core):
#   CrateStack: Null black armour with crimson accents on the bright rims and frames; small lettering removed by
#               painting the body from a blurred base colour (panel lines stay in the normal map).
#   PowerCore:  Null black (already); the cyan symbol, display and cables shifted to the Null red-orange; emissive too.
#   OilBarrel:  hazard amber; the seller's gear logo painted out and replaced by a plain black-and-amber hazard band;
#               thin rings black; grime, drips and rust kept from the original luminance.
import os
import sys

import bpy
import numpy as np

ROOT = sys.argv[sys.argv.index("--") + 1]
CRIMSON = np.array([0.50, 0.025, 0.04])     # sRGB 0-1
ARMOUR = np.array([0.045, 0.043, 0.048])
GLOW = np.array([1.0, 0.30, 0.05])           # Null red-orange
AMBER = np.array([0.96, 0.56, 0.04])
HAZARD_BLACK = np.array([0.035, 0.03, 0.03])


def load(path):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"        # work on the stored sRGB values directly
    w, h = im.size
    a = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(a)
    bpy.data.images.remove(im)
    return a.reshape(h, w, 4)[::-1].copy()            # top-down rows


def save(arr, path):
    h, w = arr.shape[:2]
    im = bpy.data.images.new(os.path.basename(path), w, h, alpha=False)
    im.colorspace_settings.name = "Non-Color"
    out = arr[::-1].copy()
    out[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(out, 0, 1).ravel())
    im.filepath_raw = path
    im.file_format = "PNG"
    im.save()
    bpy.data.images.remove(im)


def hsv(rgb):
    mx, mn = rgb.max(-1), rgb.min(-1)
    d = mx - mn
    s = np.where(mx > 1e-5, d / np.maximum(mx, 1e-5), 0)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    dd = np.maximum(d, 1e-5)
    hh = np.where(mx == r, ((g - b) / dd) % 6, np.where(mx == g, (b - r) / dd + 2, (r - g) / dd + 4)) / 6.0
    return np.where(d > 1e-5, hh, 0), s, mx


def blur(x, r):
    """Box blur of a 2-D array (radius r) with integral images."""
    p = np.pad(x, r + 1, mode="edge")
    c = p.cumsum(0).cumsum(1)
    k = 2 * r + 1
    s = c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]
    return s[:x.shape[0], :x.shape[1]] / (k * k)


def crates(d):
    a = load(f"{d}/T_Fab_CrateStack_BC_orig.png")
    rgb = a[..., :3]
    h, s, v = hsv(rgb)
    lum = rgb @ np.array([0.2126, 0.7152, 0.0722])
    soft = blur(lum, 6)                                 # strokes of the garbled lettering are 2-5 px: gone at r 6
    detail = np.clip(soft / max(np.percentile(soft, 90), 1e-3), 0, 1.3)
    accent = (h > 0.06) & (h < 0.17) & (s > 0.5) & (v > 0.62)           # bright orange-yellow rims and frames
    accent = blur(accent.astype(np.float32), 7) > 0.8                    # opening: drops scratches and specks
    accent = blur((blur(accent.astype(np.float32), 7) > 0.05).astype(np.float32), 1)          # grow back to the rim, soft edge
    metal = (s < 0.22) & (v > 0.3)
    out = ARMOUR[None, None] * (0.7 + 0.6 * detail[..., None])
    out = out * (1 - accent[..., None]) + CRIMSON[None, None] * (0.55 + 0.5 * detail[..., None]) * accent[..., None]
    gun = np.repeat((0.12 + 0.25 * soft)[..., None], 3, -1)
    out = np.where(metal[..., None], gun, out)
    a[..., :3] = out
    save(a, f"{d}/T_Fab_CrateStack_BC.png")
    return {"accent_share": round(float(accent.mean()), 3), "metal_share": round(float(metal.mean()), 3)}


def core(d):
    a = load(f"{d}/T_Fab_PowerCore_BC_orig.png")
    h, s, v = hsv(a[..., :3])
    cold = ((h > 0.42) & (h < 0.72) & (s > 0.3)).astype(np.float32)
    cold = blur(cold, 1)[..., None]
    a[..., :3] = a[..., :3] * (1 - cold) + GLOW[None, None] * v[..., None] * cold
    save(a, f"{d}/T_Fab_PowerCore_BC.png")
    e = load(f"{d}/T_Fab_PowerCore_E_orig.png")
    e[..., :3] = GLOW[None, None] * e[..., :3].max(-1)[..., None]
    save(e, f"{d}/T_Fab_PowerCore_E.png")
    return {"cold_share": round(float(cold.mean()), 4)}


def barrel(d):
    a = load(f"{d}/T_Fab_OilBarrel_BC_orig.png")
    H, W = a.shape[:2]
    rgb = a[..., :3]
    r, g = rgb[..., 0], rgb[..., 1]
    ratio = g / np.maximum(r, 1e-4)
    red = (r > 0.2) & (ratio < 0.3)
    yellow = (r > 0.3) & (ratio > 0.4) & (ratio < 0.85) & (rgb[..., 2] < 0.35)
    paint = red | yellow
    # the middle (yellow) band of the atlas, where the four logos are: rows 1309-1744, columns 64-2001 at 2048
    y0, y1, x0, x1 = (int(H * 0.639), int(H * 0.852), int(W * 0.031), int(W * 0.977))
    band = np.zeros((H, W), bool)
    band[y0:y1, x0:x1] = True
    logo = blur((red & band).astype(np.float32), 2) > 0.02
    rclean_y = np.percentile(r[yellow & band & ~logo], 90)
    rclean_r = np.percentile(r[red & ~band], 90)
    grime = np.where(yellow | band, r / rclean_y, r / rclean_r)
    grime = np.where(logo, np.median(grime[band & yellow & ~logo]), grime)   # the logo takes the band's clean paint
    grime = np.clip(grime, 0, 1.05)[..., None]
    colour = np.broadcast_to(AMBER, rgb.shape).copy()
    rings = np.zeros((H, W), bool)
    rings[int(H * 0.054):int(H * 0.118), :] = True    # the two thin red rings
    colour[rings] = HAZARD_BLACK
    # plain hazard band where the logos were: 45-degree black stripes, 96 px period, 140 px tall, centred on the logos
    yy, xx = np.mgrid[0:H, 0:W]
    cy = int(H * 0.750)
    stripe = (np.abs(yy - cy) < 70) & band & (((xx + yy) // 48) % 2 == 0)
    edge = (np.abs(np.abs(yy - cy) - 74) < 4) & band     # thin black rules above and below the stripes
    colour[stripe | edge] = HAZARD_BLACK
    new = colour * grime
    m = paint | logo | (band & (stripe | edge))
    rgb[m] = new[m]
    a[..., :3] = rgb
    save(a, f"{d}/T_Fab_OilBarrel_BC.png")
    return {"paint_share": round(float(paint.mean()), 3), "logo_px": int(logo.sum())}


rep = {"CrateStack": crates(os.path.join(ROOT, "CrateStack", "export")),
       "PowerCore": core(os.path.join(ROOT, "PowerCore", "export")),
       "OilBarrel": barrel(os.path.join(ROOT, "OilBarrel", "export"))}
print("__RESULT__", rep)
