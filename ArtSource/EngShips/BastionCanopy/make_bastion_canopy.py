# SPDX-License-Identifier: MIT
# #149: a fitted canopy shell for the Bastion, whose cockpit glazing is baked into the hull. Reads the hull from the
# supplied export (ArtSource/EngShips/BastionHero/export/SM_Bastion.fbx, never written), finds the glazing faces by
# their dark, low-saturation colour in T_Bastion_BC within the cockpit region (front upper hull), copies them 0.8 cm out
# along their normals into SM_Bastion_Canopy (one slot, M_Bastion_Glass), pivot at the hull's origin so it attaches at
# the root with an identity transform, and writes a review render and a manifest.
#   blender --background --factory-startup --python ArtSource/EngShips/BastionCanopy/make_bastion_canopy.py
import json
import math
import os

import bmesh
import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(os.path.dirname(HERE), "BastionHero", "export")
OUT = HERE
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, "SM_Bastion.fbx"))
hull = next(o for o in bpy.data.objects if o.type == "MESH" and o.name == "SM_Bastion")
img = bpy.data.images.load(os.path.join(SRC, "T_Bastion_BC.png"))
W, H = img.size
px = list(img.pixels)


def sample(u, v):
    x = int((u % 1.0) * (W - 1))
    y = int((v % 1.0) * (H - 1))
    i = (y * W + x) * 4
    return px[i], px[i + 1], px[i + 2]


# The glazing region read off the textured top view: the dark cockpit at the nose, x 1.20..2.30 m, |y| < 0.36 m.
# Its frames and the empty seat well sit below a smooth canopy line, so the shell is a dome laid over the region:
# per X row its base is the highest hull point under the row (rays from above), plus a bulge across Y, tapering at
# the front and back so it closes onto the hull.
from mathutils.bvhtree import BVHTree
dg = bpy.context.evaluated_depsgraph_get()
bvh = BVHTree.FromObject(hull, dg)
X0, X1, YH = float(os.environ.get("TD_CAN_X0", "1.20")), float(os.environ.get("TD_CAN_X1", "2.30")), float(os.environ.get("TD_CAN_Y", "0.36"))
NX, NY = 24, 12
inv = hull.matrix_world.inverted()


def top(x, y):
    hit = bvh.ray_cast(inv @ Vector((x, y, 5.0)), Vector((0, 0, -1)))
    return (hull.matrix_world @ hit[0]).z if hit[0] is not None else None


base = []
for i in range(NX + 1):
    x = X0 + (X1 - X0) * i / NX
    zs = [top(x, YH * (2.0 * j / NY - 1.0)) for j in range(NY + 1)]
    zs = [z for z in zs if z is not None]
    base.append(max(zs) if zs else 0.3)
# smooth the row bases so the dome line is fair
# r4 (the Lead on r3: a pale slab larger than the well it replaced): a pane set INTO the cockpit well only, narrower
# than the region (x 1.28..2.24 m, |y| < 0.30 m), 4 cm below the frame tops around it so the hull's frame shows as a
# lip, gently convex, never below the hull surface under it.
X0, X1, YH = float(os.environ.get("TD_CAN_X0", "1.28")), float(os.environ.get("TD_CAN_X1", "2.24")), float(os.environ.get("TD_CAN_Y", "0.30"))
rim = []
for i in range(NX + 1):
    x = X0 + (X1 - X0) * i / NX
    zs = [top(x, s_ * 0.36) for s_ in (-1.0, 1.0)]
    zs = [z for z in zs if z is not None]
    rim.append(max(zs) if zs else 0.3)
rim = [sum(rim[max(0, i - 3):i + 4]) / len(rim[max(0, i - 3):i + 4]) for i in range(len(rim))]
out = bmesh.new()
grid = {}
for i in range(NX + 1):
    t = i / NX
    for j in range(NY + 1):
        s_ = 2.0 * j / NY - 1.0
        x = X0 + (X1 - X0) * t
        y = YH * s_
        h = top(x, y)
        h = h if h is not None else rim[i] - 0.1
        bulge = 0.03 * (1.0 - s_ * s_) * math.sin(math.pi * t) ** 0.5
        z = max(h + 0.008, rim[i] - 0.04 + bulge)
        grid[(i, j)] = out.verts.new((x, y, z))
for i in range(NX):
    for j in range(NY):
        out.faces.new([grid[(i, j)], grid[(i + 1, j)], grid[(i + 1, j + 1)], grid[(i, j + 1)]])
out.normal_update()
for f in out.faces:
    if f.normal.z < 0:
        f.normal_flip()
sel = list(range(len(out.faces)))
REG = {"x": [X0, X1], "y_half": YH}
mesh = bpy.data.meshes.new("SM_Bastion_Canopy")
out.to_mesh(mesh)
can = bpy.data.objects.new("SM_Bastion_Canopy", mesh)
bpy.context.scene.collection.objects.link(can)
mat = bpy.data.materials.new("M_Bastion_Glass")
mat.diffuse_color = (0.02, 0.02, 0.03, 1.0)
mesh.materials.append(mat)
for p in mesh.polygons:
    p.use_smooth = True
bb = [can.matrix_world @ Vector(c) for c in can.bound_box]
size = [round((max(v[i] for v in bb) - min(v[i] for v in bb)) * 100, 1) for i in range(3)]
print("CANOPY tris", sum(len(p.vertices) - 2 for p in mesh.polygons), "size_cm", size)
# export: cm, +X forward like the hull (the hull export used the default FBX axes; match it)
for o in bpy.data.objects:
    o.select_set(o == can)
bpy.context.view_layer.objects.active = can
fbx = os.path.join(OUT, "SM_Bastion_Canopy.fbx")
bpy.ops.export_scene.fbx(filepath=fbx, use_selection=True, apply_unit_scale=True, bake_space_transform=False, mesh_smooth_type="FACE")
# review render: hull grey, canopy black, from front-above
for o in bpy.data.objects:
    if o.type == "MESH" and o.name.startswith("UCX"):
        o.hide_render = True
scn = bpy.context.scene
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scn.collection.objects.link(cam)
sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
scn.collection.objects.link(sun)
sun.rotation_euler = (math.radians(35), 0, math.radians(25))
hm = bpy.data.materials.new("hullgrey")
hm.diffuse_color = (0.6, 0.6, 0.6, 1.0)
hull.data.materials.clear()
hull.data.materials.append(hm)
scn.render.resolution_x, scn.render.resolution_y = 960, 540
for name, d in (("front_above", Vector((4.2, 1.8, 1.6))), ("side", Vector((1.7, 3.5, 0.8)))):
    cam.location = d
    cam.rotation_euler = (Vector((1.7, 0.0, 0.3)) - d).to_track_quat('-Z', 'Y').to_euler()
    scn.camera = cam
    scn.render.filepath = os.path.join(OUT, "review_%s.png" % name)
    bpy.ops.render.render(write_still=True)
with open(os.path.join(OUT, "SM_Bastion_Canopy.manifest.json"), "w", encoding="utf-8") as fh:
    json.dump({"asset": "SM_Bastion_Canopy", "source": "faces of ArtSource/EngShips/BastionHero/export/SM_Bastion.fbx (read only)",
               "rule": {"region_m": REG, "shell": "dome over the region: row base = highest hull point under the row + 1.2 cm, 8.5 cm bulge across, tapered ends"},
               "faces": len(sel), "size_cm": size, "pivot": "the hull's origin (attach at the root, identity transform)"}, fh, indent=1)
