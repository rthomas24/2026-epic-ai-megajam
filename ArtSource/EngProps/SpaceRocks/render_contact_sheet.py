# SPDX-License-Identifier: MIT
"""
render_contact_sheet.py - one image of the whole SpaceRocks kit, every asset at the SAME scale.

    blender --background --factory-startup --python render_contact_sheet.py
    blender --background --factory-startup --python render_contact_sheet.py -- --assets SM_Asteroid_04 --out <png>

Appends the game mesh (with its baked preview material) from every <Asset>/<Asset>.blend and renders
each one alone with an identical orthographic camera (frame = 128 cm, the 100 cm bounding sphere plus
margin) under the lighting the kit is made for: one strong sun, black space, a faint cool fill.
Rows: front three-quarter view, back three-quarter view, flat silhouette (thumbnail readability).
The label under each asset is rendered text (name, triangles, texture size).
Uses the cell/sheet helpers of the repo's blender-mcp/review_renders.py.
"""

import json
import math
import os
import runpy
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = ["SM_Asteroid_01", "SM_Asteroid_02", "SM_Asteroid_03", "SM_Asteroid_04",
          "SM_SpaceDebris_01", "SM_SpaceDebris_02", "SM_SpaceDebris_03"]
FRAME_M = 1.28
CELL = 320


def _repo_root():
    d = HERE
    while not os.path.isdir(os.path.join(d, ".claude", "skills")):
        parent = os.path.dirname(d)
        if parent == d:
            raise RuntimeError("repo root not found")
        d = parent
    return d


rv = runpy.run_path(os.path.join(_repo_root(), ".claude", "skills", "blender-mcp", "scripts", "review_renders.py"))


def append_asset(name):
    path = os.path.join(HERE, name, name + ".blend")
    with bpy.data.libraries.load(path, link=False) as (src, dst):
        dst.objects = [n for n in src.objects if n == name]
    obj = dst.objects[0]
    bpy.context.scene.collection.objects.link(obj)
    return obj


def main(assets=ASSETS, out=None):
    out = out or os.path.join(HERE, "review", "SpaceRocks_contact_sheet.png")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    scene = bpy.context.scene
    for o in list(scene.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    objs = [append_asset(a) for a in assets]

    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = scene.render.resolution_y = CELL
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.view_transform = "Standard"
    try:
        scene.eevee.taa_render_samples = 32
    except Exception:
        pass
    world = bpy.data.worlds.new("Space")
    scene.world = world
    bg = world.node_tree.nodes.get("Background")
    if bg is not None:
        bg.inputs["Color"].default_value = (0.012, 0.014, 0.03, 1.0)   # near-black space, a hint of blue
        bg.inputs["Strength"].default_value = 1.0

    cam_data = bpy.data.cameras.new("Cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = FRAME_M
    cam = bpy.data.objects.new("Cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    sun_data = bpy.data.lights.new("Sun", "SUN")
    sun_data.energy = 5.0
    sun_data.angle = math.radians(1.0)
    sun = bpy.data.objects.new("Sun", sun_data)
    fill_data = bpy.data.lights.new("Fill", "SUN")
    fill_data.energy = 0.22
    fill_data.color = (0.45, 0.6, 1.0)
    fill = bpy.data.objects.new("Fill", fill_data)
    for o in (sun, fill):
        scene.collection.objects.link(o)

    label_mat = bpy.data.materials.new("Label")
    nt = label_mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = (0, 0, 0, 1)
    bsdf.inputs["Emission Color"].default_value = (0.85, 0.87, 0.9, 1)
    bsdf.inputs["Emission Strength"].default_value = 1.0

    tmp = os.path.join(bpy.app.tempdir, "spacerocks_cell.png")
    look = rv["_look_matrix"]
    view_dir = rv["_view_dir"]
    rows = []
    for az, el, with_label in ((35.0, 22.0, True), (215.0, 12.0, False)):
        d = view_dir(az, el)
        rot = look(-d)
        cam.matrix_world = Matrix.Translation(d * 4.0) @ rot.to_4x4()
        # One strong sun from the camera's upper left; faint cool fill from the opposite side.
        sun.matrix_world = look(-d + rot.col[1] * -0.55 + rot.col[0] * 0.75).to_4x4()
        fill.matrix_world = look(d * 0.3 + rot.col[1] * 0.4 + rot.col[0] * -1.0).to_4x4()
        cells = []
        for obj in objs:
            for o in objs:
                o.hide_render = o is not obj
            text = None
            if with_label:
                me = obj.data
                tris = sum(len(p.vertices) - 2 for p in me.polygons)
                man_path = os.path.join(HERE, obj.name, obj.name + ".manifest.json")
                tex = ""
                if os.path.exists(man_path):
                    with open(man_path, encoding="utf-8") as fh:
                        tex = "  {}px".format(json.load(fh).get("texture_size", ""))
                curve = bpy.data.curves.new("Label", "FONT")
                curve.body = "{}   {} tris{}".format(obj.name, tris, tex)
                curve.size = 0.046
                curve.align_x = "CENTER"
                curve.materials.append(label_mat)
                text = bpy.data.objects.new("Label", curve)
                scene.collection.objects.link(text)
                text.matrix_world = Matrix.Translation(d * 2.0 + rot.col[1] * -0.585) @ rot.to_4x4()
            scene.render.filepath = tmp
            bpy.ops.render.render(write_still=True)
            cells.append(rv["_png_to_array"](tmp))
            if text is not None:
                bpy.data.objects.remove(text, do_unlink=True)
        rows.append(cells)
    # Silhouette row from the alpha of the first row (label strip excluded).
    sil = []
    for cell in rows[0]:
        a = cell[:, :, 3:4].copy()
        a[: int(CELL * 0.11)] = 0.0      # image rows run bottom-to-top: drop the label strip
        s = np.zeros_like(cell)
        s[:, :, :3] = 0.82
        s[:, :, 3:4] = (a > 0.5).astype(np.float32)
        sil.append(s)
    cells = rows[0] + rows[1] + sil
    rv["_write_sheet"](cells, out, (0.0, 0.0, 0.0), len(objs))
    if os.path.exists(tmp):
        os.remove(tmp)
    return {"status": "ok", "sheet": out, "assets": [o.name for o in objs], "frame_cm": FRAME_M * 100.0,
            "cm_per_px": round(FRAME_M * 100.0 / CELL, 4), "rows": ["front 3/4 (sun)", "back 3/4 (sun)", "silhouette"]}


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--assets", nargs="*")
    p.add_argument("--out")
    a = p.parse_args(argv)
    print("__RESULT__" + json.dumps(main(a.assets or ASSETS, a.out)))


if __name__ == "__main__":
    _cli()
