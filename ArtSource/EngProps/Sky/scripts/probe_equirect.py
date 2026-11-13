# Probe: which pixel of a Cycles equirectangular camera render does a Blender world direction land on?
# blender --background --factory-startup --python probe_equirect.py -- <out.png>
import math
import sys

import bpy
import numpy as np

out = sys.argv[sys.argv.index("--") + 1]
sc = bpy.context.scene
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
sc.render.engine = "CYCLES"
sc.cycles.samples = 4
sc.render.resolution_x, sc.render.resolution_y = 512, 256
w = bpy.data.worlds.new("W")
sc.world = w
w.node_tree.nodes["Background"].inputs[1].default_value = 0.0
cam_d = bpy.data.cameras.new("C")
cam_d.type = "PANO"
cam_d.panorama_type = "EQUIRECTANGULAR"
cam = bpy.data.objects.new("C", cam_d)
sc.collection.objects.link(cam)
sc.camera = cam
cam.rotation_euler = (math.radians(90), 0.0, math.radians(-90))
marks = {"+X": ((1, 0, 0), (1, 0, 0)), "+Y": ((0, 1, 0), (0, 1, 0)), "+Z": ((0, 0, 1), (0, 0, 1)),
         "mid": ((0.7071, 0.0, 0.7071), (1, 1, 0))}
for k, (d, col) in marks.items():
    me = bpy.data.meshes.new(k)
    import bmesh
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=8, v_segments=6, radius=0.4)
    bm.to_mesh(me)
    o = bpy.data.objects.new(k, me)
    sc.collection.objects.link(o)
    o.location = [c * 10 for c in d]
    m = bpy.data.materials.new(k)
    nt = m.node_tree
    nt.nodes.clear()
    e = nt.nodes.new("ShaderNodeEmission")
    e.inputs[0].default_value = (*col, 1)
    e.inputs[1].default_value = 10
    oo = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(e.outputs[0], oo.inputs[0])
    me.materials.append(m)
sc.render.filepath = out
sc.render.image_settings.file_format = "PNG"
bpy.ops.render.render(write_still=True)
img = bpy.data.images.load(out)
px = np.array(img.pixels[:]).reshape(256, 512, 4)
res = {}
for ch, name in ((0, "R"), (1, "G"), (2, "B")):
    mask = px[:, :, ch] > 0.5
    ys, xs = np.nonzero(mask)
    res[name] = (float(xs.mean() / 512) if len(xs) else None, float(ys.mean() / 256) if len(ys) else None)
print("__RESULT__", res, "(u, v_bottom_up)")
