# SPDX-License-Identifier: MIT
# Triad Drift - Quixel Megascans "Ash" (smshcoh, Fab Standard) -> an ash-drift decal mask for Cinder, headless:
#   blender --background --factory-startup --python prep_fab_ash.py -- <4K Opacity jpg> <repo>
# T_Fab_AshDrift_Mask (1024, grayscale): the supplied opacity with a soft border (so the decal box never shows an edge),
# and review/Fab_AshDrift_on_ground.png: the look composited on T_Fab_TundraVolcanic_BC with the M_TD_FabDecal formula.
import json
import os
import sys

import bpy
import numpy as np

SRC, REPO = sys.argv[sys.argv.index("--") + 1:][:2]
FAB = os.path.join(REPO, "ArtSource", "EngProps", "Fab")
D = os.path.join(FAB, "AshDrift")
for sub in ("export", "review"):
    os.makedirs(os.path.join(D, sub), exist_ok=True)
LOOK = {"CharColor": (0.16, 0.15, 0.14), "CharRoughness": 0.95, "Opacity": 0.55, "Bias": 0.1, "Contrast": 0.9,
        "EmberColor": (0.0, 0.0, 0.0), "EmberStrength": 0.0, "EmberPower": 10.0}


def load(path, size=1024):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"
    im.scale(size, size)
    x = np.empty(size * size * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(size, size, 4)


def save(arr, path):
    h, w = arr.shape[:2]
    im = bpy.data.images.new(os.path.basename(path), w, h)
    im.colorspace_settings.name = "Non-Color"
    o = arr.copy()
    o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = path
    im.file_format = "PNG"
    im.save()


m = load(SRC)[..., 0]
t = np.linspace(0, 1, 1024)
ramp = np.clip(np.minimum(t, 1 - t) / 0.18, 0, 1)
ramp = ramp * ramp * (3 - 2 * ramp)
m = m * np.outer(ramp, ramp)
out = np.zeros((1024, 1024, 4), np.float32)
out[..., 0] = out[..., 1] = out[..., 2] = m
save(out, os.path.join(D, "export", "T_Fab_AshDrift_Mask.png"))

g = load(os.path.join(FAB, "Textures", "TundraVolcanic", "export", "T_Fab_TundraVolcanic_BC.png"))[..., :3]
lin = np.where(g <= 0.04045, g / 12.92, ((g + 0.055) / 1.055) ** 2.4) * 0.9
a = np.clip((m - LOOK["Bias"]) * LOOK["Contrast"], 0, 1)[..., None] * LOOK["Opacity"]
res = lin * (1 - a) + np.array(LOOK["CharColor"]) * 0.9 * a


def srgb(x):
    x = np.clip(x, 0, 1)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(x, 1 / 2.4) - 0.055)


sheet = np.concatenate([srgb(lin), np.full((1024, 16, 3), 0.08), srgb(res)], 1)
s = np.ones(sheet.shape[:2] + (4,), np.float32)
s[..., :3] = sheet
save(s, os.path.join(D, "review", "Fab_AshDrift_on_ground.png"))
json.dump({"looks": {"Cinder": LOOK}, "mask_mean": round(float(m.mean()), 3)}, open(os.path.join(D, "prep_report.json"), "w"), indent=1)
print("__RESULT__ ok mask_mean %.3f" % m.mean())
