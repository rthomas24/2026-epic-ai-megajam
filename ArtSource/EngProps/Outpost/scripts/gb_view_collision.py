# Triad Drift - review renders for a <Name>_r1.blend made by the *_build.py scripts: the mesh, and its UCX_ hulls
# alone from the same cameras.   blender --background <blend> --python gb_view_collision.py -- <out_prefix> <size_m>
import bpy, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import op_common as C
a = sys.argv[sys.argv.index("--") + 1:]
prefix, S = a[0], float(a[1])
ucx = [o for o in bpy.data.objects if o.name.startswith("UCX_")]
mesh = [o for o in bpy.data.objects if o.type == "MESH" and not o.name.startswith("UCX_")]
m = bpy.data.materials.new("ucx_orange")
m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (1.0, 0.45, 0.1, 1)
for o in ucx:
    o.data.materials.clear(); o.data.materials.append(m)
bpy.ops.mesh.primitive_cylinder_add(radius=0.34, depth=1.8, location=(float(a[2]), float(a[3]), float(a[4]) + 0.9) if len(a) > 4 else (S * 0.6, 0, 0.9), vertices=20)
pil = bpy.context.active_object
pm = bpy.data.materials.new("pilot"); pm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.2, 0.9, 0.3, 1)
pil.data.materials.append(pm)
views = [("hero", (S * 1.3, -S * 1.0, S * 0.7), (0, 0, S * 0.3), 35), ("top", (0, 0, 60), (0, 0, 0), ("ortho", S * 1.25)),
         ("side", (0, -60, S * 0.35), (0, 0, S * 0.35), ("ortho", S * 1.25))]
for o in ucx:
    o.hide_render = True
C.render_views(prefix + "_mesh", views)
for o in ucx:
    o.hide_render = False
C.render_views(prefix + "_ucx", views, hide=mesh)
print("__RESULT__ ok")
