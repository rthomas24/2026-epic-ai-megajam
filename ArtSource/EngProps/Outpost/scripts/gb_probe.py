# Triad Drift - read-only ray probe of any FBX: top-surface height map and optional horizontal slices.
#   blender --background --factory-startup --python gb_probe.py -- <fbx> <step_cm> [x0 x1 y0 y1] [zslice ...]
import bpy, bmesh, sys
from mathutils import Vector
from mathutils.bvhtree import BVHTree
a = sys.argv[sys.argv.index("--") + 1:]
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=a[0])
step = int(a[1])
o = [x for x in bpy.data.objects if x.type == "MESH"][0]
bm = bmesh.new(); bm.from_mesh(o.data); bm.transform(o.matrix_world)
co = [v.co for v in bm.verts]
lo = [min(c[i] for c in co) * 100 for i in range(3)]; hi = [max(c[i] for c in co) * 100 for i in range(3)]
print("bounds", [round(c) for c in lo], [round(c) for c in hi], "tris", sum(len(f.verts) - 2 for f in bm.faces))
x0, x1, y0, y1 = [int(v) for v in a[2:6]] if len(a) >= 6 else (int(lo[0]), int(hi[0]), int(lo[1]), int(hi[1]))
bvh = BVHTree.FromBMesh(bm)
xs = list(range(x0 - x0 % step, x1 + 1, step)); ys = list(range(y0 - y0 % step, y1 + 1, step))
print("TOP height (rows y high to low; cols x %d..%d step %d)" % (xs[0], xs[-1], step))
print("      " + "".join("%5d" % x for x in xs))
for y in reversed(ys):
    print("%5d " % y + "".join(("%5d" % round(h[0].z * 100)) if (h := bvh.ray_cast(Vector((x / 100, y / 100, hi[2] / 100 + 1)), Vector((0, 0, -1))))[0] else "    ." for x in xs))
for z in [int(v) for v in a[6:]]:
    print("SLICE z=%d: per y, first hit x from +X / from -X" % z)
    for y in reversed(ys):
        p = bvh.ray_cast(Vector((hi[0] / 100 + 1, y / 100, z / 100)), Vector((-1, 0, 0))); q = bvh.ray_cast(Vector((lo[0] / 100 - 1, y / 100, z / 100)), Vector((1, 0, 0)))
        print("%5d  %s / %s" % (y, round(p[0].x * 100) if p[0] else ".", round(q[0].x * 100) if q[0] else "."))
