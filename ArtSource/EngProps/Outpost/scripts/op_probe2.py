# Triad Drift - Outpost: horizontal ray probes of the source outpost (read only) for collision authoring.
import bpy, bmesh, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import op_common as C
from mathutils import Vector
from mathutils.bvhtree import BVHTree
C.clear_scene()
o = C.import_fbx("SM_VerdanceOutpost")
bm = bmesh.new(); bm.from_mesh(o.data); bm.transform(o.matrix_world)
bvh = BVHTree.FromBMesh(bm)
def hit(p, d):
    h = bvh.ray_cast(Vector([c / 100 for c in p]), Vector(d))
    return None if h[0] is None else [round(c * 100) for c in h[0]]
print("== exterior plan: per Y, x hit from +X / from -X at z=200,260,330")
for y in range(-380, 381, 20):
    row = []
    for z in (200, 260, 330):
        a = hit((900, y, z), (-1, 0, 0)); b = hit((-900, y, z), (1, 0, 0))
        row.append("%s/%s" % (a[0] if a else ".", b[0] if b else "."))
    print("y %5d  " % y + "   ".join(row))
print("== exterior plan: per X, y hit from +Y / from -Y at z=200,260,330")
for x in range(-400, 261, 20):
    row = []
    for z in (200, 260, 330):
        a = hit((x, 900, z), (0, -1, 0)); b = hit((x, -900, z), (0, 1, 0))
        row.append("%s/%s" % (a[1] if a else ".", b[1] if b else "."))
    print("x %5d  " % x + "   ".join(row))
print("== bay interior from (x,-80,z): +Y wall, -Y wall ; from (20,-80,z) -X wall")
for z in (158, 170, 190, 220, 260, 300, 340, 380, 395):
    print("z %d  " % z + "  ".join("x%d: %s/%s" % (x, (hit((x, -80, z), (0, 1, 0)) or [0, "."])[1], (hit((x, -80, z), (0, -1, 0)) or [0, "."])[1]) for x in (-60, -30, 0, 40, 80, 95, 105, 115)),
          " back:", [(hit((20, y, z), (-1, 0, 0)) or ["."])[0] for y in (-220, -150, -80, 0, 60)])
print("== bay floor / ceiling fine grid (x -90..130 step 10) at y=-200,-80,40")
for y in (-230, -200, -80, 40, 70):
    print("y %d floor " % y, [(hit((x, y, 250), (0, 0, -1)) or [0, 0, "."])[2] for x in range(-90, 131, 10)])
    print("y %d ceil  " % y, [(hit((x, y, 250), (0, 0, 1)) or [0, 0, "."])[2] for x in range(-90, 241, 10)])
print("== deck edge: floor from z=145.. x 180..260 at y=-300,-80,100,300; y edges")
for y in (-300, -80, 100, 300):
    print("y %d" % y, [(hit((x, y, 150), (0, 0, -1)) or [0, 0, "."])[2] for x in range(180, 261, 5)])
print("deck -x edge", [(hit((x, 0, 150), (0, 0, -1)) or [0, 0, "."])[2] for x in range(-410, -360, 5)])
print("deck +y edge", [(hit((0, y, 150), (0, 0, -1)) or [0, 0, "."])[2] for y in range(360, 395, 5)])
# islands with hi.x > 195
bw = bm.copy(); bmesh.ops.remove_doubles(bw, verts=bw.verts, dist=1e-5)
seen = set()
print("== islands reaching x > 195 or the bay pad")
for v in bw.verts:
    if v.index in seen: continue
    st = [v]; seen.add(v.index); vs = []
    while st:
        a = st.pop(); vs.append(a)
        for e in a.link_edges:
            b = e.other_vert(a)
            if b.index not in seen: seen.add(b.index); st.append(b)
    lo = [round(min(q.co[i] for q in vs) * 100, 1) for i in range(3)]; hi = [round(max(q.co[i] for q in vs) * 100, 1) for i in range(3)]
    if hi[0] > 195 or (lo[2] > 145 and hi[2] < 170 and len(vs) < 40):
        print(len(vs), lo, hi)
