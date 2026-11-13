# Triad Drift - ray probe of the outpost FBX (read only): floor height / ceiling height maps on a 20 cm grid.
# blender --background --factory-startup --python op_probe.py -- <fbx> [zstart_cm]
import bpy, bmesh, sys
from mathutils import Vector
from mathutils.bvhtree import BVHTree
argv = sys.argv[sys.argv.index("--") + 1:]
for _o in list(bpy.data.objects): bpy.data.objects.remove(_o)
bpy.ops.import_scene.fbx(filepath=argv[0])
z0 = float(argv[1]) / 100 if len(argv) > 1 else 2.5
o = [x for x in bpy.data.objects if x.type == "MESH"][0]
bm = bmesh.new(); bm.from_mesh(o.data); bm.transform(o.matrix_world)
bvh = BVHTree.FromBMesh(bm)
xs = list(range(-400, 421, 20)); ys = list(range(-380, 381, 20))
print("FLOOR below z=%d (rows y from +380 to -380, cols x -400..420)" % (z0 * 100))
print("      " + "".join("%4d" % x for x in xs))
for y in reversed(ys):
    row = []
    for x in xs:
        h = bvh.ray_cast(Vector((x / 100, y / 100, z0)), Vector((0, 0, -1)))
        row.append("%4d" % round(h[0].z * 100) if h[0] else "   .")
    print("%5d " % y + "".join(row))
print("CEILING above z=%d" % (z0 * 100))
for y in reversed(ys):
    row = []
    for x in xs:
        h = bvh.ray_cast(Vector((x / 100, y / 100, z0)), Vector((0, 0, 1)))
        row.append("%4d" % round(h[0].z * 100) if h[0] else "   .")
    print("%5d " % y + "".join(row))
