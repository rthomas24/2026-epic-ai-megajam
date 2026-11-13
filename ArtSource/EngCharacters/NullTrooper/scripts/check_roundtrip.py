import bpy, os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE); import nt_io
from mathutils import Matrix
path = sys.argv[sys.argv.index("--")+1]
for o in list(bpy.data.objects): bpy.data.objects.remove(o)
arm, mesh = nt_io.import_ue_fbx(path)
ref = json.load(open(os.path.join(ROOT, "work", "manny_bones_ref.json")))
worst = 0.0; worstn = None; missing = [n for n in ref if n not in arm.data.bones]
for b in arm.data.bones:
    if b.name not in ref: print("EXTRA bone", b.name); continue
    m = arm.matrix_world @ b.matrix_local; r = Matrix(ref[b.name])
    d = max(abs(m[i][j]-r[i][j]) for i in range(4) for j in range(4))
    if d > worst: worst, worstn = d, b.name
ws = [mesh.matrix_world @ v.co for v in mesh.data.vertices]
print("ROUNDTRIP bones", len(arm.data.bones), "missing", missing, "max matrix abs diff", worst, worstn,
      "height", max(v.z for v in ws), "minz", min(v.z for v in ws), "front(min y)", min(v.y for v in ws), "armobj", arm.name)
mesh.data.calc_loop_triangles()
print("ROUNDTRIP mesh tris", len(mesh.data.loop_triangles), "verts", len(mesh.data.vertices), "vertex groups", len(mesh.vertex_groups),
      "materials", [m.name for m in mesh.data.materials], "x", min(v.x for v in ws), max(v.x for v in ws))
