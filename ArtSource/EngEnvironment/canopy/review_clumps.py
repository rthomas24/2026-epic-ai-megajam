# SPDX-License-Identifier: MIT
# Review render of the tree clumps (workbench, vertex colour), one image per clump, side and 30-degree-down views.
#   blender --background --factory-startup --python ArtSource/EngEnvironment/canopy/review_clumps.py
import math
import os

import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "review")
os.makedirs(OUT, exist_ok=True)
for i in range(1, 5):
    name = "SM_Scatter_Verd_TreeClump_%02d" % i
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(HERE, "export", name + ".fbx"))
    obj = [o for o in bpy.data.objects if o.type == "MESH"][0]
    sc = bpy.context.scene
    sc.render.engine = "BLENDER_WORKBENCH"
    sc.display.shading.color_type = "VERTEX"
    sc.display.shading.light = "STUDIO"
    sc.render.resolution_x, sc.render.resolution_y = 900, 500
    world = bpy.data.worlds.new("w")
    world.color = (0.3, 0.45, 0.45)
    sc.world = world
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    sc.collection.objects.link(cam)
    sc.camera = cam
    bb = [obj.matrix_world @ Vector(b) for b in obj.bound_box]
    c = sum(bb, Vector()) / 8
    dist = max(obj.dimensions) * 3.2
    cam.location = c + Vector((dist * math.cos(0.5), -dist * math.sin(0.5), dist * 0.6))
    cam.rotation_euler = (c - cam.location).to_track_quat("-Z", "Y").to_euler()
    sc.render.filepath = os.path.join(OUT, name + ".png")
    bpy.ops.render.render(write_still=True)
    print("REVIEW:", sc.render.filepath)
