# Triad Drift - probe SK_Pilot.fbx's right hand in Blender: bone heads / axes in armature space, how many skin vertices
# each finger bone moves, and a close-up render of the rest hand. Animation builder, Nov 13.
# blender --background --factory-startup --python b_probe_hand.py -- <SK_Pilot.fbx> <out_dir>
import bpy, sys, json, os, math
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
src, out = argv[0], argv[1]
os.makedirs(out, exist_ok=True)
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=src)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
mesh = next(o for o in bpy.data.objects if o.type == 'MESH' and any(m.type == 'ARMATURE' for m in o.modifiers))
res = {"arm_matrix": [list(r) for r in arm.matrix_world], "mesh_matrix": [list(r) for r in mesh.matrix_world], "bones": {}}
for b in arm.data.bones:
    n = b.name
    if not (n.endswith("_r") or n in ("root", "pelvis")):
        continue
    if not any(k in n for k in ("hand", "index", "middle", "ring", "pinky", "thumb", "lowerarm", "root", "pelvis")):
        continue
    m = b.matrix_local
    res["bones"][n] = {"head": list(b.head_local), "tail": list(b.tail_local), "parent": b.parent.name if b.parent else "",
                       "x": list(m.col[0].xyz), "y": list(m.col[1].xyz), "z": list(m.col[2].xyz)}
# weights per finger bone
vg = {g.index: g.name for g in mesh.vertex_groups}
counts = {}
for v in mesh.data.vertices:
    for g in v.groups:
        n = vg.get(g.group)
        if n in res["bones"] and g.weight > 0.2:
            counts[n] = counts.get(n, 0) + 1
res["weighted_verts"] = counts
json.dump(res, open(os.path.join(out, "b_hand_probe.json"), "w"), indent=1)
print("PROBE arm", arm.name, "scale", tuple(arm.scale), "mesh", mesh.name, "verts", len(mesh.data.vertices))
for n in sorted(counts):
    print("PROBE w", n, counts[n])
h = arm.matrix_world @ arm.data.bones["hand_r"].head_local
print("PROBE hand world", tuple(round(c, 4) for c in h))
