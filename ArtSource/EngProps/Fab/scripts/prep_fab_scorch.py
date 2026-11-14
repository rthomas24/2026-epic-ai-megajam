# SPDX-License-Identifier: MIT
# Triad Drift - Quixel Megascans "Scorch Mark" (wlbkbdhc, Fab Standard): the supplied brush mask -> T_Fab_ScorchMark_Mask
# (1024, grayscale, noise floor cut) and a preview of the two decal looks composited top-down on our own ground textures:
#   blender --background --factory-startup --python prep_fab_scorch.py -- <2K brush jpg> <repo>
# The composite uses the same formula as the M_TD_FabDecal master: opacity = saturate((mask - Bias) * Contrast) * Opacity,
# colour = CharColor, emissive = EmberColor * EmberStrength * mask^EmberPower (Cinder only).
import json
import os
import sys

import bpy
import numpy as np

SRC, REPO = sys.argv[sys.argv.index("--") + 1:][:2]
FAB = os.path.join(REPO, "ArtSource", "EngProps", "Fab")
D = os.path.join(FAB, "ScorchMark")
os.makedirs(os.path.join(D, "export"), exist_ok=True)
os.makedirs(os.path.join(D, "review"), exist_ok=True)
# the look parameters (written into the two material instances by ue_import_fab_decal.py)
LOOKS = {
    # Cinder's ground is near-black basalt, so a dark char does not read: pale ash with a dim ember only in the hottest core
    "Cinder": {"CharColor": (0.055, 0.05, 0.046), "CharRoughness": 0.95, "Opacity": 0.8, "Bias": 0.05, "Contrast": 1.2,
               "EmberColor": (1.0, 0.28, 0.05), "EmberStrength": 3.0, "EmberPower": 10.0},
    # Verdance: cool near-black char, soft falloff so the blast streaks show
    "Verdance": {"CharColor": (0.012, 0.013, 0.015), "CharRoughness": 0.9, "Opacity": 0.85, "Bias": 0.05, "Contrast": 1.2,
                 "EmberColor": (0.0, 0.0, 0.0), "EmberStrength": 0.0, "EmberPower": 10.0},
}


def load(path, size):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"
    if tuple(im.size) != (size, size):
        im.scale(size, size)
    x = np.empty(size * size * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(size, size, 4)


def save(arr, path):
    s0, s1 = arr.shape[:2]
    im = bpy.data.images.new(os.path.basename(path), s1, s0)
    im.colorspace_settings.name = "Non-Color"
    o = arr.copy()
    o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = path
    im.file_format = "PNG"
    im.save()
    bpy.data.images.remove(im)


def srgb_to_lin(x):
    return np.where(x <= 0.04045, x / 12.92, ((x + 0.055) / 1.055) ** 2.4)


def lin_to_srgb(x):
    x = np.clip(x, 0, 1)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(x, 1 / 2.4) - 0.055)


m = load(SRC, 1024)[..., 0]
floor = float(np.percentile(m, 40))                      # the black field carries faint grain: cut it
m = np.clip((m - floor) / max(1.0 - floor, 1e-3), 0, 1)
mask = np.zeros((1024, 1024, 4), np.float32)
mask[..., 0] = mask[..., 1] = mask[..., 2] = m
save(mask, os.path.join(D, "export", "T_Fab_ScorchMark_Mask.png"))

rep = {"noise_floor_cut": round(floor, 3), "mask_mean": round(float(m.mean()), 3)}
grounds = {"Cinder": os.path.join(FAB, "Textures", "TundraVolcanic", "export", "T_Fab_TundraVolcanic_BC.png"),
           "Verdance": os.path.join(FAB, "Textures", "ForestNordicMoss", "export", "T_Fab_ForestNordicMoss_BC.png")}
row = []
for name, look in LOOKS.items():
    g = load(grounds[name], 1024)[..., :3]
    lin = srgb_to_lin(g)
    lit = lin * 0.9                                      # flat sun stand-in
    a = np.clip((m - look["Bias"]) * look["Contrast"], 0, 1)[..., None] * look["Opacity"]
    out = lit * (1 - a) + np.array(look["CharColor"]) * 0.9 * a
    out += np.array(look["EmberColor"]) * look["EmberStrength"] * (m ** look["EmberPower"])[..., None] * 0.08   # bloom-less glow
    before = lin_to_srgb(lit)
    after = lin_to_srgb(out)
    rep[name] = {"ground_mean": round(float(before.mean()), 3), "core_contrast": round(float(before[m > 0.6].mean() - after[m > 0.6].mean()), 3)
                 if name == "Verdance" else round(float(after[m > 0.6].mean() - before[m > 0.6].mean()), 3)}
    row.append(np.concatenate([before, np.full((1024, 16, 3), 0.08), after], 1))
sheet = np.concatenate([row[0], np.full((16, row[0].shape[1], 3), 0.08), row[1]], 0)
s = np.ones(sheet.shape[:2] + (4,), np.float32)
s[..., :3] = sheet
save(s, os.path.join(D, "review", "Fab_ScorchMark_on_ground.png"))
json.dump({"looks": LOOKS, "report": rep}, open(os.path.join(D, "prep_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))
