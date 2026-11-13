# SPDX-License-Identifier: MIT
"""
render_range.py - range-readability strip for one Null Fleet ship, as the game shows it at 300 m.

    blender --background --factory-startup SM_Null_Fighter.blend --python render_range.py -- \
        --asset SM_Null_Fighter --out review/SM_Null_Fighter_e1_range300.png --emission 2.0 --scale 2.5

Gameplay draws enemy art up to 2.5x larger with range (ATDEnemyShip RangeScaleMax, reached at 250 m), so the
mesh is scaled by --scale. The camera has the pixel density of a 1920 px wide frame at the game's 90 deg FOV,
300 m away, against the bright nebula; each view is rendered at its true pixel count and enlarged 4x
(nearest), so a dot that would be 1 px in game is a 4x4 block here. Views: chase (from behind, above), side,
front 3/4. --emission sets the baked preview material's emission strength (the Unreal MI's EmissiveStrength / 6,
so the relative brightness between versions is honest; EEVEE has no bloom, Unreal will glow more).
"""

import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Vector  # noqa: E402

import nf_lib as L  # noqa: E402


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:]
    p = argparse.ArgumentParser()
    p.add_argument("--asset", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--emission", type=float, default=2.0)
    p.add_argument("--scale", type=float, default=2.5)
    p.add_argument("--dist", type=float, default=300.0)
    a = p.parse_args(argv)
    obj = bpy.data.objects[a.asset]
    for slot in obj.material_slots:
        if slot.material and slot.material.node_tree:
            for n in slot.material.node_tree.nodes:
                if n.type == "BSDF_PRINCIPLED":
                    n.inputs["Emission Strength"].default_value = a.emission
    obj.scale = (a.scale,) * 3
    bpy.context.view_layer.update()
    pts = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    centre = sum(pts, Vector()) / 8.0
    scene = bpy.data.scenes.new("__range")
    scene.collection.objects.link(obj)
    world = bpy.data.worlds.new("__range_world")
    scene.world = world
    L._nebula_world(world)
    cam_d = bpy.data.cameras.new("__range_cam")
    cam = bpy.data.objects.new("__range_cam", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    sun_d = bpy.data.lights.new("__range_sun", "SUN")
    sun = bpy.data.objects.new("__range_sun", sun_d)
    scene.collection.objects.link(sun)
    sun_d.energy = 4.0
    sun.rotation_euler = Vector((0.4, -1.0, -0.3)).to_track_quat("-Z", "Y").to_euler()
    r = scene.render
    r.engine = "BLENDER_EEVEE"
    try:
        scene.eevee.taa_render_samples = 32
    except Exception:
        pass
    zoom, cell = 4, 448
    px_cell = cell // zoom
    r.resolution_x = r.resolution_y = px_cell
    r.image_settings.file_format = "PNG"
    cam_d.lens = 18.0 / (math.tan(math.radians(45.0)) * px_cell / 1920.0)
    cam_d.clip_start = 1.0
    cam_d.clip_end = a.dist * 4.0
    cells = []
    for az, el in ((165.0, 14.0), (90.0, 6.0), (35.0, 14.0)):
        aa, ee = math.radians(az), math.radians(el)
        d = Vector((math.sin(aa) * math.cos(ee), -math.cos(aa) * math.cos(ee), math.sin(ee)))
        cam.location = centre + d * a.dist
        cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
        tmp = a.out[:-4] + "_tmp.png"
        r.filepath = tmp
        bpy.ops.render.render(write_still=True, scene=scene.name)
        img = L._load_rgba(tmp)
        os.remove(tmp)
        cells.append(np.repeat(np.repeat(img, zoom, axis=0), zoom, axis=1))
    gap = 6
    strip = np.full((cell + 2 * gap, 3 * cell + 4 * gap, 4), 0.06, dtype=np.float32)
    strip[:, :, 3] = 1.0
    for i, c in enumerate(cells):
        x0 = gap + i * (cell + gap)
        strip[gap:gap + cell, x0:x0 + cell] = c
    L._save_rgba(strip, a.out)
    print("__RESULT__", a.out)


main()
