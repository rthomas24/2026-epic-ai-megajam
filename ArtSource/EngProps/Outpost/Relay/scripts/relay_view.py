# SPDX-License-Identifier: MIT
# Triad Drift - Ancient Relay: review renders of the edited stairs (textured, then the collision hulls in orange)
# with 180 cm pilot stand-ins standing on the collision.
#   blender --background Relay/Relay_r2.blend --python Relay/scripts/relay_view.py -- r2
import bpy, sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts"))
import op_common as C
tag = sys.argv[sys.argv.index("--") + 1]
KIT = os.path.dirname(HERE)
ucx = [o for o in bpy.data.objects if o.name.startswith("UCX_")]
mesh = [o for o in bpy.data.objects if o.type == "MESH" and not o.name.startswith("UCX_")]
m = bpy.data.materials.new("ucx_orange")
m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (1.0, 0.45, 0.1, 1)
for o in ucx:
    o.data.materials.clear(); o.data.materials.append(m)


def pilot(name, x, y, z):
    bpy.ops.mesh.primitive_cylinder_add(radius=0.34, depth=1.8, location=(x / 100, y / 100, z / 100 + 0.9), vertices=20)
    o = bpy.context.active_object; o.name = name
    pm = bpy.data.materials.new(name); pm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.2, 0.9, 0.3, 1)
    o.data.materials.append(pm)
    return o


pilot("P_ramp", 600, 95, 60)       # on the flight (collision Z at x 600 = 60)
pilot("P_top", 380, 95, 128)       # on the carved slope
pilot("P_court", 150, 95, 135)     # on the court
views = [("stairs34", (13.5, -7.5, 4.5), (5.0, 0.9, 0.8), 30),
         ("stairs_side", (5.0, -40, 1.0), (5.0, 0.9, 1.0), ("ortho", 7.5)),
         ("hero", (16, -12, 8), (0, 0, 3), 35),
         ("top", (2.0, 0, 60), (2.0, 0, 0), ("ortho", 15.5))]
for o in ucx:
    o.hide_render = True
p = os.path.join(KIT, "review", tag)
C.render_views(p + "_mesh", views)
for o in ucx:
    o.hide_render = False
C.render_views(p + "_ucx", views, hide=mesh)
print("__RESULT__ ok")
