# SPDX-License-Identifier: MIT
# Triad Drift - Quixel Megascans "Tundra Icelandic Ground Volcanic Rough 01" (yickdbe, Fab Standard) -> T_Fab_TundraVolcanic_*:
#   blender --background --factory-startup --python prep_fab_tundra.py -- <folder with the 2K EXRs> <out root>
# From the supplied 2K EXR maps (linear float): base colour converted to sRGB and darkened / cooled toward Cinder's basalt
# at 2048 (the seller's colour kept as T_Fab_TundraVolcanic_BC_orig.jpg); normal (convention measured against the
# displacement map, OpenGL flipped to DirectX) and ORM (AO, roughness, metallic 0) at 1024.
import json
import os
import sys

import bpy
import numpy as np

SRC, OUT = sys.argv[sys.argv.index("--") + 1:][:2]
P = os.path.join(SRC, "Tundra_Icelandic_Ground_Volcanic_Rough_01_yickdbe_2K_")
D = os.path.join(OUT, "TundraVolcanic", "export")
os.makedirs(D, exist_ok=True)


def load(name, size):
    im = bpy.data.images.load(P + name + ".exr")
    im.colorspace_settings.name = "Non-Color"          # raw stored values (linear for colour, encoded for normal)
    if tuple(im.size) != (size, size):
        im.scale(size, size)
    x = np.empty(size * size * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(size, size, 4)


def save(arr, name, fmt="PNG"):
    s = arr.shape[0]
    im = bpy.data.images.new(name, s, s)
    im.colorspace_settings.name = "Non-Color"
    o = arr.copy()
    o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = os.path.join(D, name)
    im.file_format = fmt
    im.save()
    bpy.data.images.remove(im)


def to_srgb(x):
    x = np.clip(x, 0, 1)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(x, 1 / 2.4) - 0.055)


bc = load("BaseColor", 2048)
bc[..., :3] = to_srgb(bc[..., :3])
save(bc, "T_Fab_TundraVolcanic_BC_orig.jpg", "JPEG")
lum = bc[..., :3] @ np.array([0.2126, 0.7152, 0.0722])
# the source is already basalt-dark (mean about 0.20 sRGB): desaturate the brown, cool it, dim 10% (a stronger cut crushed it to black)
bc[..., :3] = (bc[..., :3] * 0.4 + lum[..., None] * 0.6) * 0.9 * np.array([0.95, 0.97, 1.03])
save(bc, "T_Fab_TundraVolcanic_BC.png")
n = load("Normal", 1024)
h = load("Displacement", 1024)[..., 0]
dv = np.zeros_like(h)
dv[1:-1] = h[2:] - h[:-2]
c = float(np.corrcoef((n[8:-8:2, 8:-8:2, 1] - 0.5).ravel(), dv[8:-8:2, 8:-8:2].ravel())[0, 1])
if c < 0:
    n[..., 1] = 1.0 - n[..., 1]
save(n, "T_Fab_TundraVolcanic_N.png")
orm = np.ones((1024, 1024, 4), np.float32)
orm[..., 0] = load("AO", 1024)[..., 0]
orm[..., 1] = load("Roughness", 1024)[..., 0]
orm[..., 2] = 0.0
save(orm, "T_Fab_TundraVolcanic_ORM.png")
rep = {"normal": "%s (%.3f)" % ("OpenGL, flipped" if c < 0 else "DirectX", c),
       "bc_mean": [round(float(bc[..., i].mean()), 3) for i in range(3)], "orm_mean": [round(float(orm[..., i].mean()), 3) for i in range(3)],
       "n_range": [round(float(n[..., 1].min()), 3), round(float(n[..., 1].max()), 3)]}
json.dump(rep, open(os.path.join(OUT, "TundraVolcanic", "prep_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))
