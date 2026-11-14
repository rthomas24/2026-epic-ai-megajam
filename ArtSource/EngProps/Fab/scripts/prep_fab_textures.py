# SPDX-License-Identifier: MIT
# Triad Drift - three Fab surface sets -> project texture sets at 4096 (T_Fab_<Set>_BC / _N DirectX / _ORM), headless:
#   blender --background --factory-startup --python prep_fab_textures.py -- <intake3 root> <scratch tga dir> <out root>
# ForestNordicMoss (Quixel Megascans "Forest Nordic Rock Mossy Rough 05"): moss shifted from green toward Verdance teal.
# ConcreteRubble ("Concrete (Rubble) Vol. 08", ehsan vaezi, CC BY 4.0): darkened and cooled toward Cinder ash.
# StoneTileFloor ("Stone Tile Floor 1", Yi-X): colours as supplied (Verdance relay plaza only).
# The recoloured base colours keep the seller's version beside them as T_Fab_<Set>_BC_orig.jpg. The normal convention is
# measured against each set's height map (OpenGL is flipped to DirectX).
import json
import os
import sys

import bpy
import numpy as np

INTAKE, TGA, OUT = sys.argv[sys.argv.index("--") + 1:][:3]
SIZE = 4096
rep = {}


def load(path):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"
    if tuple(im.size) != (SIZE, SIZE):
        im.scale(SIZE, SIZE)
    x = np.empty(SIZE * SIZE * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(SIZE, SIZE, 4)


def save(arr, path, fmt="PNG"):
    im = bpy.data.images.new(os.path.basename(path), SIZE, SIZE)
    im.colorspace_settings.name = "Non-Color"
    o = arr.copy()
    o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = path
    im.file_format = fmt
    if fmt == "JPEG":
        bpy.context.scene.render.image_settings.quality = 92
    im.save()
    bpy.data.images.remove(im)


def opengl(n, h):
    dv = np.zeros_like(h)
    dv[1:-1] = h[2:] - h[:-2]
    c = float(np.corrcoef((n[8:-8:4, 8:-8:4, 1] - 0.5).ravel(), dv[8:-8:4, 8:-8:4].ravel())[0, 1])
    return c < 0, c


def hsv_to_rgb(h, s, v):
    i = np.floor(h * 6).astype(int) % 6
    f = h * 6 - np.floor(h * 6)
    p, q, t = v * (1 - s), v * (1 - f * s), v * (1 - (1 - f) * s)
    r = np.choose(i, [v, q, p, p, t, v])
    g = np.choose(i, [t, v, v, q, p, p])
    b = np.choose(i, [p, p, t, v, v, q])
    return np.stack([r, g, b], -1)


def rgb_to_hsv(rgb):
    mx, mn = rgb.max(-1), rgb.min(-1)
    d = mx - mn
    s = np.where(mx > 1e-5, d / np.maximum(mx, 1e-5), 0)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    dd = np.maximum(d, 1e-5)
    h = np.where(mx == r, ((g - b) / dd) % 6, np.where(mx == g, (b - r) / dd + 2, (r - g) / dd + 4)) / 6.0
    return np.where(d > 1e-5, h, 0), s, mx


def write_set(name, bc, n, h, ao, rough, metal, bc_orig=None):
    d = os.path.join(OUT, name, "export")
    os.makedirs(d, exist_ok=True)
    gl, c = opengl(n, h)
    if gl:
        n[..., 1] = 1.0 - n[..., 1]
    orm = np.ones((SIZE, SIZE, 4), np.float32)
    orm[..., 0], orm[..., 1], orm[..., 2] = ao, rough, metal
    save(bc, os.path.join(d, "T_Fab_%s_BC.png" % name))
    save(n, os.path.join(d, "T_Fab_%s_N.png" % name))
    save(orm, os.path.join(d, "T_Fab_%s_ORM.png" % name))
    if bc_orig is not None:
        save(bc_orig, os.path.join(d, "T_Fab_%s_BC_orig.jpg" % name), "JPEG")
    rep[name] = {"normal": "%s (%.3f)" % ("OpenGL, flipped" if gl else "DirectX", c),
                 "bc_mean": [round(float(bc[..., i].mean()), 3) for i in range(3)],
                 "orm_mean": [round(float(orm[..., i].mean()), 3) for i in range(3)]}


# Forest Nordic Rock Mossy Rough 05 (Megascans): moss hue 0.17-0.40 rotated toward teal (~0.47), a little more saturated
m = os.path.join(INTAKE, "ForestNordicMoss05", "src", "source", "ForestNordicMoss05", "xiembdz_4K_")
bc = load(m + "Basecolor.jpg")
orig = bc.copy()
h, s, v = rgb_to_hsv(bc[..., :3])
moss = ((h > 0.15) & (h < 0.42) & (s > 0.12)).astype(np.float32)
h2 = np.where(moss > 0, 0.47 - (0.42 - np.clip(h, 0.15, 0.42)) * 0.25, h)
s2 = np.where(moss > 0, np.clip(s * 1.25, 0, 1), s)
bc[..., :3] = hsv_to_rgb(h2, s2, v)
write_set("ForestNordicMoss", bc, load(m + "Normal.jpg"), load(m + "Displacement.jpg")[..., 0],
          load(m + "AO.jpg")[..., 0], load(m + "Roughness.jpg")[..., 0], 0.0, orig)
rep["ForestNordicMoss"]["moss_share"] = round(float(moss.mean()), 3)

# Concrete (Rubble) Vol. 08: darker and cooler (ash)
r = os.path.join(TGA, "ConcreteRubble08", "Concrete Vol 08_")
bc = load(r + "basecolor.tga")
orig = bc.copy()
lum = bc[..., :3] @ np.array([0.2126, 0.7152, 0.0722])
bc[..., :3] = (bc[..., :3] * 0.35 + lum[..., None] * 0.65) * 0.55 * np.array([0.95, 0.95, 1.0])
write_set("ConcreteRubble", bc, load(r + "normal.tga"), load(r + "height.tga")[..., 0],
          load(r + "ambientOcclusion.tga")[..., 0], load(r + "roughness.tga")[..., 0], load(r + "metallic.tga")[..., 0], orig)

# Stone Tile Floor 1: ORMH = occlusion, roughness, metallic, height
t = os.path.join(INTAKE, "StoneTileFloor1", "src", "source", "StoneTileFloor1", "StoneTileFloor1_01_")
ormh = load(t + "ORMH.png")
write_set("StoneTileFloor", load(t + "BaseColor.png"), load(t + "Normal.png"), ormh[..., 3],
          ormh[..., 0], ormh[..., 1], ormh[..., 2])
json.dump(rep, open(os.path.join(OUT, "prep_textures_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))
