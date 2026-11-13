# Which hand / foot does the striking: per frame distance of each hand and foot from neck_01 / pelvis, plus pelvis yaw.
# Run: blender --background --factory-startup --python b_hands.py -- <a.fbx> [<b.fbx> ...]
import bpy, sys, math
argv = sys.argv[sys.argv.index("--") + 1:]
for src in argv:
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)
    bpy.ops.import_scene.fbx(filepath=src, use_anim=True)
    sc = bpy.context.scene
    arm = next(o for o in sc.objects if o.type == 'ARMATURE')
    act = arm.animation_data.action
    f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
    W = lambda n: arm.matrix_world @ arm.pose.bones[n].head
    best = {}
    line = []
    for f in range(f0, f1 + 1):
        sc.frame_set(f)
        nk = W("neck_01"); pv = W("pelvis")
        d = {"hr": (W("hand_r") - nk).length, "hl": (W("hand_l") - nk).length,
             "fr": W("foot_r").z, "fl": W("foot_l").z}
        L = W("upperarm_l") - W("upperarm_r")
        yaw = math.degrees(math.atan2(L.y, L.x))
        for k, v in d.items():
            if k not in best or v > best[k][0]:
                best[k] = (round(v, 3), f)
        if (f - f0) % 3 == 0:
            line.append("f%d hr%.2f hl%.2f yaw%.0f fr%.2f fl%.2f" % (f, d["hr"], d["hl"], yaw, d["fr"], d["fl"]))
    print("HANDS", src.split("\\")[-1], "max(dist,frame)", best)
    print("HANDS_TRACE", " | ".join(line))
