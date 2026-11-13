# Triad Drift - Ancient Relay: read-only measurements of Grok Bot's mesh for collision authoring.
#   blender --background --factory-startup --python relay_probe.py -- <fbx>
import bpy, bmesh, sys, math
from mathutils import Vector
from mathutils.bvhtree import BVHTree
a = sys.argv[sys.argv.index("--") + 1:]
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=a[0])
o = [x for x in bpy.data.objects if x.type == "MESH"][0]
bm = bmesh.new(); bm.from_mesh(o.data); bm.transform(o.matrix_world)
bvh = BVHTree.FromBMesh(bm)


def top(x, y):
    h = bvh.ray_cast(Vector((x / 100, y / 100, 11)), Vector((0, 0, -1)))
    return round(h[0].z * 100) if h[0] else 0


CX, CY = -75, 25
print("radial top-height profiles from (%d, %d), r = 200..640 step 20" % (CX, CY))
for deg in range(0, 360, 30):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    print("%4d " % deg + " ".join("%3d" % top(CX + c * r, CY + s * r) for r in range(200, 641, 20)))
print("stairs: top height along x (y = 0), x 250..565 step 10")
print(" ".join("%3d" % top(x, 0) for x in range(250, 566, 10)))
for x in (380, 450, 520):
    print("stairs: across y at x=%d, y -200..200 step 10" % x)
    print(" ".join("%3d" % top(x, y) for y in range(-200, 201, 10)))
hi = [v.co for v in bm.verts if v.co.z > 4.0]
print("verts above 400: x %.0f..%.0f  y %.0f..%.0f  z..%.0f" % (min(v.x for v in hi) * 100, max(v.x for v in hi) * 100, min(v.y for v in hi) * 100, max(v.y for v in hi) * 100, max(v.z for v in hi) * 100))
mid = [v.co for v in bm.verts if 1.6 < v.co.z <= 4.0]
print("verts 160..400: x %.0f..%.0f  y %.0f..%.0f" % (min(v.x for v in mid) * 100, max(v.x for v in mid) * 100, min(v.y for v in mid) * 100, max(v.y for v in mid) * 100))
# x histogram of verts above 250 (ring slab thickness)
hist = {}
for v in bm.verts:
    if v.co.z > 2.5:
        k = int(v.co.x * 100 // 20) * 20
        hist[k] = hist.get(k, 0) + 1
print("x histogram of verts above 250 cm:", sorted(hist.items()))
