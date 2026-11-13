# SPDX-License-Identifier: MIT
"""
sq_silhouette.py - silhouette sheet of the three squad ships (black on white), headless:

    blender --background --factory-startup --python sq_silhouette.py -- --tag r3

Reads the built ships from SM_Squad_<Ship>/SM_Squad_<Ship>.blend. Columns: Rift, Lancer, Bastion.
Rows (all at TRUE game pixel density: 1920 px wide frame, the chase camera's 90 deg FOV):
  1  chase view: from behind-above at 14 m (TDShip arm 14 m, height 2.8 m)
  2  behind-above at 200 m, enlarged 4x (nearest) so the real pixel count can be judged
  3  top view, orthographic, all three at one scale (longest ship ~128 px)
Writes review/Squad_<tag>_silhouettes.png.
"""

import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
SHIPS = ("SM_Squad_Rift", "SM_Squad_Lancer", "SM_Squad_Bastion")
CELL = 448


def load(asset):
    path = os.path.join(KIT, asset, asset + ".blend")
    with bpy.data.libraries.load(path, link=False) as (src, dst):
        dst.objects = [asset]
    return dst.objects[0]


def px(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    tag = argv[argv.index("--tag") + 1] if "--tag" in argv else "r1"
    scene = bpy.context.scene
    for o in list(scene.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    black = bpy.data.materials.new("SIL_Black")
    bsdf = black.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (0, 0, 0, 1)
    bsdf.inputs["Roughness"].default_value = 1.0
    bsdf.inputs["Specular IOR Level"].default_value = 0.0
    world = bpy.data.worlds.new("SIL_World")
    world.color = (1, 1, 1)
    nt = world.node_tree
    bg = next(n for n in nt.nodes if n.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (1, 1, 1, 1)
    bg.inputs["Strength"].default_value = 1.0
    scene.world = world
    r = scene.render
    r.engine = "BLENDER_EEVEE"
    scene.view_settings.view_transform = "Standard"
    r.image_settings.file_format = "PNG"
    cam_d = bpy.data.cameras.new("SIL_Cam")
    cam = bpy.data.objects.new("SIL_Cam", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    game_lens = 18.0 / (math.tan(math.radians(45.0)) * CELL / 1920.0)
    rows = []
    tmp = os.path.join(KIT, "review", "_sil_tmp.png")
    objs = []
    for a in SHIPS:
        o = load(a)
        scene.collection.objects.link(o)
        o.data.materials.clear()
        o.data.materials.append(black)
        o.hide_render = True
        objs.append(o)
    views = [("chase14", 14.0, (180.0, 11.0), 1, False), ("m200x4", 200.0, (180.0, 11.0), 4, False),
             ("top", None, None, 1, True)]
    for tagv, dist, ang, zoom, ortho in views:
        row = []
        for o in objs:
            o.hide_render = False
            for q in objs:
                if q is not o:
                    q.hide_render = True
            if ortho:
                cam_d.type = "ORTHO"
                cam_d.ortho_scale = 5.3 * CELL / 128.0
                cam.location = (0, 0, 30)
                cam.rotation_euler = (0, 0, 0)
                cam_d.clip_end = 100
            else:
                cam_d.type = "PERSP"
                cam_d.lens = game_lens * zoom
                az, el = (math.radians(v) for v in ang)
                d = Vector((math.sin(az) * math.cos(el), -math.cos(az) * math.cos(el), math.sin(el)))
                cam.location = d * dist
                cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
                cam_d.clip_start = 0.05
                cam_d.clip_end = dist * 4
            r.resolution_x = r.resolution_y = CELL // zoom
            r.filepath = tmp
            bpy.ops.render.render(write_still=True)
            im = px(tmp)
            if zoom > 1:
                im = np.repeat(np.repeat(im, zoom, axis=0), zoom, axis=1)
            row.append(im)
        rows.append(row)
    os.remove(tmp)
    gap = 8
    H = len(rows) * CELL + (len(rows) + 1) * gap
    W = 3 * CELL + 4 * gap
    sheet = np.full((H, W, 4), 0.5, dtype=np.float32)
    sheet[:, :, 3] = 1
    for ri, row in enumerate(reversed(rows)):          # image rows run bottom-to-top
        for ci, im in enumerate(row):
            y0 = gap + ri * (CELL + gap)
            x0 = gap + ci * (CELL + gap)
            sheet[y0:y0 + CELL, x0:x0 + CELL] = im
    out = os.path.join(KIT, "review", "Squad_%s_silhouettes.png" % tag)
    img = bpy.data.images.new("SIL_Sheet", width=W, height=H, alpha=False)
    img.pixels.foreach_set(sheet.ravel())
    img.filepath_raw = out
    img.file_format = "PNG"
    img.save()
    print("__RESULT__" + out)


main()
