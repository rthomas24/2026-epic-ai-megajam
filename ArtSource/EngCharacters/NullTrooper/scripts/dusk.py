# dusk.py -- <label>: one EEVEE render under a dim warm key + weak cool fill (the Verdance dusk case)
import bpy, sys, os, math
from mathutils import Vector
a = sys.argv[sys.argv.index("--") + 1:]
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sc = bpy.context.scene
for o in sc.objects:
    if not o.name.startswith("SK_Null"): o.hide_render = True
w = bpy.data.worlds.new("dusk"); sc.world = w
w.node_tree.nodes["Background"].inputs[0].default_value = (0.02, 0.025, 0.04, 1); w.node_tree.nodes["Background"].inputs[1].default_value = 0.6
def light(kind, loc, col, energy, size=2.0):
    d = bpy.data.lights.new(kind, kind); d.color = col; d.energy = energy
    if kind == 'AREA': d.size = size
    o = bpy.data.objects.new(kind, d); sc.collection.objects.link(o); o.location = loc
    o.rotation_euler = (Vector((0, 0, 1.0)) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    return o
light('AREA', (-2.5, -3.0, 2.2), (1.0, 0.62, 0.38), 260, 2.5)   # dim warm key
light('AREA', (3.0, -1.0, 1.5), (0.5, 0.6, 1.0), 40, 3.0)       # weak cool fill
cd = bpy.data.cameras.new("cam"); cd.lens = 70
cam = bpy.data.objects.new("cam", cd); sc.collection.objects.link(cam)
cam.location = (2.6, -5.2, 1.5); cam.rotation_euler = (Vector((0, 0, 0.95)) - Vector(cam.location)).to_track_quat('-Z', 'Y').to_euler()
sc.camera = cam
sc.render.engine = 'BLENDER_EEVEE'; sc.render.resolution_x = 700; sc.render.resolution_y = 1100
sc.render.filepath = os.path.join(ROOT, "review", a[0] + "_dusk.png")
bpy.ops.render.render(write_still=True)
print("DUSK", sc.render.filepath)
