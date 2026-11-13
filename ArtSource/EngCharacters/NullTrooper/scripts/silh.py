# silh.py -- <out prefix>: flat black silhouette on grey, front and side, fixed orthographic framing
import bpy, sys, os
from mathutils import Vector
out = sys.argv[sys.argv.index("--") + 1]
sc = bpy.context.scene
ob = next(o for o in bpy.data.objects if o.name.startswith("SK_Null"))
for o in sc.objects:
    if o != ob: o.hide_render = True
m = bpy.data.materials.new("black"); nt = m.node_tree
for n in list(nt.nodes): nt.nodes.remove(n)
e = nt.nodes.new("ShaderNodeEmission"); e.inputs[0].default_value = (0, 0, 0, 1)
o_ = nt.nodes.new("ShaderNodeOutputMaterial"); nt.links.new(e.outputs[0], o_.inputs[0])
ob.data.materials.clear(); ob.data.materials.append(m)
w = bpy.data.worlds.new("grey"); sc.world = w
w.node_tree.nodes["Background"].inputs[0].default_value = (0.35, 0.35, 0.35, 1)
cd = bpy.data.cameras.new("c"); cd.type = 'ORTHO'; cd.ortho_scale = 2.15
cam = bpy.data.objects.new("c", cd); sc.collection.objects.link(cam); sc.camera = cam
sc.render.engine = 'BLENDER_EEVEE'; sc.render.resolution_x = 520; sc.render.resolution_y = 860
sc.view_settings.view_transform = 'Standard'
for name, loc in (("front", (0, -6, 0.95)), ("side", (-6, 0, 0.95))):
    cam.location = loc
    cam.rotation_euler = (Vector((0, 0, 0.95)) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = out + "_" + name + ".png"
    bpy.ops.render.render(write_still=True)
print("SILH", out)
