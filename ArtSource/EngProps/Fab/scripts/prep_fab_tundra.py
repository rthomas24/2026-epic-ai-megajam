# SPDX-License-Identifier: MIT
# Triad Drift - Quixel Megascans "Tundra Icelandic Ground ... Rough 01" sets (Fab Standard) -> T_Fab_<Name>_*:
#   blender --background --factory-startup --python prep_fab_tundra.py -- <folder with the 2K EXRs> <out root> [Name]
#   Name = TundraVolcanic (default; Volcanic, yickdbe) or TundraSulfur (Sulfur, yifsfbr)
# From the supplied 2K EXR maps (linear float): base colour converted to sRGB at 2048 (the seller's colour kept as
# T_Fab_<Name>_BC_orig.jpg); normal (convention measured against the displacement map, OpenGL flipped to DirectX) and
# ORM (AO, roughness, metallic 0) at 1024.
#   TundraVolcanic: desaturated, cooled and dimmed 10% toward Cinder basalt (the source is already dark).
#   TundraSulfur: hue and saturation kept (sulfur yellow round vents and lava cracks); only scaled so its mean brightness
#                 matches the volcanic set's base colour (export/T_Fab_TundraVolcanic_BC.png).
import json
import os
import sys

import bpy
import numpy as np

ARGS = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = ARGS[:2]
NAME = ARGS[2] if len(ARGS) > 2 else "TundraVolcanic"
FILES = {"TundraVolcanic": "Tundra_Icelandic_Ground_Volcanic_Rough_01_yickdbe_2K_",
         "TundraSulfur": "Tundra_Icelandic_Ground_Sulfur_Rough_01_yifsfbr_2K_"}
P = os.path.join(SRC, FILES[NAME])
D = os.path.join(OUT, NAME, "export")
os.makedirs(D, exist_ok=True)
LUMA = np.array([0.2126, 0.7152, 0.0722])


def load(path, size, exr=True):
    im = bpy.data.images.load(P + path + ".exr" if exr else path)
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


rep = {}
bc = load("BaseColor", 2048)
bc[..., :3] = to_srgb(bc[..., :3])
save(bc, "T_Fab_%s_BC_orig.jpg" % NAME, "JPEG")
lum = bc[..., :3] @ LUMA
rep["source_mean_luma"] = round(float(lum.mean()), 3)
if NAME == "TundraVolcanic":
    # the source is already basalt-dark (mean about 0.20 sRGB): desaturate the brown, cool it, dim 10% (a stronger cut crushed it to black)
    bc[..., :3] = (bc[..., :3] * 0.4 + lum[..., None] * 0.6) * 0.9 * np.array([0.95, 0.97, 1.03])
else:
    ref = load(os.path.join(OUT, "TundraVolcanic", "export", "T_Fab_TundraVolcanic_BC.png"), 2048, exr=False)
    target = float((ref[..., :3] @ LUMA).mean())
    # An exact match (x0.44) turned the sulfur brown: yellow at basalt brightness reads as brown. Brightness is brought
    # part of the way (mean luma ~1.6x the volcanic set) and saturation lifted 15% so it still reads as sulfur.
    goal = target * 1.6
    k = goal / max(float(lum.mean()), 1e-4)
    rgb = bc[..., :3] * k
    l2 = rgb @ LUMA
    bc[..., :3] = l2[..., None] + (rgb - l2[..., None]) * 1.15
    rep["brightness_match"] = {"volcanic_mean_luma": round(target, 3), "goal_mean_luma": round(goal, 3), "scale": round(k, 3),
                               "saturation": 1.15}
save(bc, "T_Fab_%s_BC.png" % NAME)
n = load("Normal", 1024)
h = load("Displacement", 1024)[..., 0]
dv = np.zeros_like(h)
dv[1:-1] = h[2:] - h[:-2]
c = float(np.corrcoef((n[8:-8:2, 8:-8:2, 1] - 0.5).ravel(), dv[8:-8:2, 8:-8:2].ravel())[0, 1])
if c < 0:
    n[..., 1] = 1.0 - n[..., 1]
save(n, "T_Fab_%s_N.png" % NAME)
orm = np.ones((1024, 1024, 4), np.float32)
orm[..., 0] = load("AO", 1024)[..., 0]
orm[..., 1] = load("Roughness", 1024)[..., 0]
orm[..., 2] = 0.0
save(orm, "T_Fab_%s_ORM.png" % NAME)
rep.update({"normal": "%s (%.3f)" % ("OpenGL, flipped" if c < 0 else "DirectX", c),
            "bc_mean": [round(float(bc[..., i].mean()), 3) for i in range(3)], "orm_mean": [round(float(orm[..., i].mean()), 3) for i in range(3)]})
json.dump(rep, open(os.path.join(OUT, NAME, "prep_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))
