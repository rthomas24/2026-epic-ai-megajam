# Triad Drift - Verdance wreck: read-only measurements of Grok Bot's mesh (opening, deck, ceiling, hull sections).
import bpy, bmesh, sys
from mathutils import Vector
from mathutils.bvhtree import BVHTree
a = sys.argv[sys.argv.index("--") + 1:]
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=a[0])
o = [x for x in bpy.data.objects if x.type == "MESH"][0]
bm = bmesh.new(); bm.from_mesh(o.data); bm.transform(o.matrix_world)
bvh = BVHTree.FromBMesh(bm)
def ray(p, d):
    h = bvh.ray_cast(Vector([c / 100 for c in p]), Vector(d))
    return [round(c * 100) for c in h[0]] if h[0] else None
print("== floor (down from z=400) and ceiling (up from z=400) inside the opening; rows y +700..-700 step 50, cols x -150..400 step 50")
for y in range(700, -701, -50):
    print("%5d  " % y + " ".join("%3s/%-3s" % ((ray((x, y, 400), (0, 0, -1)) or [0, 0, "."])[2], (ray((x, y, 400), (0, 0, 1)) or [0, 0, "."])[2]) for x in range(-150, 401, 50)))
print("== from the +Y side at z=400 looking -Y: first hit y per x (-300..500 step 25)")
print(" ".join("%s" % (ray((x, 900, 400), (0, -1, 0)) or [0, "."])[1] for x in range(-300, 501, 25)))
print("== from the -Y side at z=400 looking +Y")
print(" ".join("%s" % (ray((x, -900, 400), (0, 1, 0)) or [0, "."])[1] for x in range(-300, 501, 25)))
print("== ground-level skirt: top height (down from 1300) along y at x=124, y 900..-900 step 50")
print(" ".join("%s" % (ray((124, y, 1300), (0, 0, -1)) or [0, 0, "."])[2] for y in range(900, -901, -50)))
print("== lowest surface from below (up from z=-10) along y at x=124")
print(" ".join("%s" % (ray((124, y, -10), (0, 0, 1)) or [0, 0, "."])[2] for y in range(900, -901, -50)))
print("== hull sections: per x (step 200): y min/max, z min/max of verts in the slab")
for x in range(-1600, 1600, 200):
    vs = [v.co for v in bm.verts if x <= v.co.x * 100 < x + 200]
    if vs:
        print("%6d  y %5.0f..%5.0f  z %5.0f..%5.0f  n %d" % (x, min(v.y for v in vs) * 100, max(v.y for v in vs) * 100, min(v.z for v in vs) * 100, max(v.z for v in vs) * 100, len(vs)))
