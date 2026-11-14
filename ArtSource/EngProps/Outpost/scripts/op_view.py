# Triad Drift - Outpost: review renders of Outpost_<tag>.blend (textured, EEVEE) with a 180 cm pilot stand-in.
#   blender --background Outpost_r1.blend --python scripts/op_view.py -- r1
import bpy, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import op_common as C
tag = sys.argv[sys.argv.index("--") + 1]
con = bpy.data.objects["SM_Outpost_Console"]
con.location = (-0.19, -0.815, 1.53)          # the assembled placement (Blender coords)
# pilot stand-ins: capsule-sized boxes 80 x 80 x 180 cm
def pilot(name, x, y, z):
    bpy.ops.mesh.primitive_cylinder_add(radius=0.4, depth=1.8, location=(x, y, z + 0.9), vertices=20)
    o = bpy.context.active_object; o.name = name
    m = bpy.data.materials.new(name); m.diffuse_color = (0.95, 0.55, 0.35, 1)
    m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.95, 0.55, 0.35, 1)
    o.data.materials.append(m)
    return o
pilot("PilotBay", 0.75, -0.815, 1.53)
pilot("PilotRamp", 4.3, -0.70, 0.705)
p = os.path.join(C.KIT, "review", tag)
C.render_views(p, [
    ("front34", (13, -9, 6.5), (1, 0, 2.4), 35),
    ("bay", (5.2, -0.8, 3.0), (0, -0.8, 2.6), 24),
    ("ramp_side", (4.0, -12, 1.4), (4.0, 0, 1.4), ("ortho", 6.5)),
    ("bay_low", (2.3, 0.3, 1.9), (-0.6, -1.6, 1.7), 20),
    ("under", (7, 7, 0.25), (0, 0, 1.2), 26),
    ("top", (1.1, 0, 30), (1.1, 0, 0), ("ortho", 11.0)),
])
print("__RESULT__ ok")
