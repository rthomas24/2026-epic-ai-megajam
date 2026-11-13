# Stage 20 (step D): weights for every part, join into SK_NullTrooper under the armature 'root'.
# blender --background --factory-startup NullTrooper_v05.blend --python s20_skin.py -- --out NullTrooper_v06.blend
import bpy, bmesh, os, sys, json, math
from mathutils import Vector, kdtree
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[argv.index("--out") + 1]
NAME = argv[argv.index("--name") + 1] if "--name" in argv else "SK_NullTrooper"
arm = bpy.data.objects["root"]
hi = bpy.data.objects["FIT_BodyHi"]
bone_names = [b.name for b in arm.data.bones]
B = lambda n: arm.data.bones[n].head_local.copy()

# nearest-vertex weight source (dense inflated body)
hnames = {g.index: g.name for g in hi.vertex_groups}
kd = kdtree.KDTree(len(hi.data.vertices))
for v in hi.data.vertices: kd.insert(v.co, v.index)
kd.balance()
hw = [[(hnames[g.group], g.weight) for g in v.groups if g.weight > 0.01] for v in hi.data.vertices]


def set_weights(ob, per_vert):
    ob.vertex_groups.clear()
    groups = {}
    for vi, ws in enumerate(per_vert):
        ws = sorted(ws, key=lambda x: -x[1])[:4]
        tot = sum(w for _, w in ws) or 1.0
        for n, w in ws:
            if n not in groups: groups[n] = ob.vertex_groups.new(name=n)
            groups[n].add([vi], w / tot, 'REPLACE')


def boot_weights(ob, side):
    ank, ball = B("foot_" + side), B("ball_" + side)
    f = Vector((ball.x - ank.x, ball.y - ank.y, 0)).normalized()
    out = []
    for v in ob.data.vertices:
        y = (v.co - ball).dot(f)
        wb = min(1.0, max(0.0, (y + 0.02) / 0.03))
        wc = 0.0 if y > -0.03 else min(1.0, max(0.0, (v.co.z - 0.13) / 0.09))
        wf = max(0.0, 1.0 - wb - wc)
        out.append([("ball_" + side, wb), ("calf_" + side, wc), ("foot_" + side, wf)])
    return out


parts = [o for o in bpy.data.collections["NullTrooper"].objects if o.type == 'MESH']
stats = {"rigid": 0, "transfer": 0, "fn": 0, "keep": 0}
for o in parts:
    spec = o.get("nt_bone", "transfer")
    if spec == "keep":
        stats["keep"] += 1; continue
    if spec.startswith("fn:boot"):
        side = "r" if o.name.endswith("_R") else "l"
        set_weights(o, boot_weights(o, side)); stats["fn"] += 1
    elif spec == "transfer":
        pv = []
        for v in o.data.vertices:
            co, idx, dist = kd.find(v.co)
            pv.append(hw[idx] or [("pelvis", 1.0)])
        set_weights(o, pv); stats["transfer"] += 1
    else:
        d = json.loads(spec)
        for n in d: assert n in bone_names, (o.name, n)
        set_weights(o, [list(d.items())] * len(o.data.vertices)); stats["rigid"] += 1
print("SKIN parts", stats)

# join
for o in bpy.context.selected_objects: o.select_set(False)
bodyo = bpy.data.objects["NT_Body"]
for o in parts: o.select_set(True)
bpy.context.view_layer.objects.active = bodyo
with bpy.context.temp_override(active_object=bodyo, selected_objects=parts, selected_editable_objects=parts):
    bpy.ops.object.join()
sk = bodyo; sk.name = NAME; sk.data.name = NAME
# drop unused groups, make sure every vertex is weighted, normalise
used = set()
for v in sk.data.vertices:
    for g in v.groups:
        if g.weight > 0: used.add(g.group)
unw = sum(1 for v in sk.data.vertices if not any(g.weight > 0 for g in v.groups))
for g in list(sk.vertex_groups):
    if g.name not in bone_names: sk.vertex_groups.remove(g)
for v in sk.data.vertices:
    gs = sorted([g for g in v.groups], key=lambda g: -g.weight)
    for g in gs[4:]: sk.vertex_groups[g.group].remove([v.index])
bad = 0
for v in sk.data.vertices:
    tot = sum(g.weight for g in v.groups)
    if tot <= 0: bad += 1; continue
    if abs(tot - 1) > 1e-4:
        for g in v.groups: g.weight /= tot
mx = max(len([g for g in v.groups if g.weight > 0]) for v in sk.data.vertices)
sk.parent = arm
sk.modifiers.clear()
m = sk.modifiers.new("Armature", 'ARMATURE'); m.object = arm
sk.data.calc_loop_triangles()
print("SKIN joined tris %d verts %d unweighted %d zero-total %d max_influences %d slots %s" % (
    len(sk.data.loop_triangles), len(sk.data.vertices), unw, bad, mx, [mm.name for mm in sk.data.materials]))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, OUT))
