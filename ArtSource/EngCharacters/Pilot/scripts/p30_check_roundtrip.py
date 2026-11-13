# Pilot step 3: read back the FBX that Unreal exported from SK_Pilot (Saved/Pilot/SK_Pilot_back.fbx) and count it.
# Run: blender --background --factory-startup --python scripts/p30_check_roundtrip.py
import bpy, os, numpy as np
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
for o in list(bpy.data.objects): bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=os.path.join(REPO, "Saved", "Pilot", "SK_Pilot_back.fbx"), automatic_bone_orientation=False, ignore_leaf_bones=False, use_anim=False)
bpy.context.view_layer.update()
for o in bpy.data.objects:
    print("BACK object", o.name, o.type, "world scale", tuple(round(v, 4) for v in o.matrix_world.to_scale()))
    if o.type == 'MESH':
        o.data.calc_loop_triangles()
        co = np.array([o.matrix_world @ v.co for v in o.data.vertices]) * 100
        print("BACK mesh triangles %d verts %d materials %s size cm %s z %.1f..%.1f" % (len(o.data.loop_triangles), len(co), [m.name for m in o.data.materials],
              (co.max(0) - co.min(0)).round(1).tolist(), co[:, 2].min(), co[:, 2].max()))
    if o.type == 'ARMATURE':
        print("BACK armature bones %d roots %s" % (len(o.data.bones), [b.name for b in o.data.bones if b.parent is None]))
