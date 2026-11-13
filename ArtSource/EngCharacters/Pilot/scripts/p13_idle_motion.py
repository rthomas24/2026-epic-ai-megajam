# Pilot: how much does each idle move? Run: blender --background work/Pilot_v01.blend --python scripts/p13_idle_motion.py
import bpy, numpy as np
arm = bpy.data.objects["root"]; sc = bpy.context.scene
for name in ("Idle_02", "Idle_3", "Idle_6"):
    a = bpy.data.actions[name]; arm.animation_data.action = a; arm.animation_data.action_slot = a.slots[0]
    f0, f1 = (int(round(v)) for v in a.frame_range); P = []
    for f in range(f0, f1 + 1):
        sc.frame_set(f); P.append([list(arm.pose.bones[b].head) for b in ("head", "hand_l", "hand_r", "pelvis", "foot_l", "foot_r")])
    P = np.array(P) * 100; dev = np.linalg.norm(P - P.mean(0), axis=2); spd = np.linalg.norm(np.diff(P, axis=0), axis=2) * 30
    print("IDLE %-8s max excursion from mean cm: head %.1f hand_l %.1f hand_r %.1f pelvis %.1f foot_l %.1f foot_r %.1f | mean speed cm/s head %.1f hands %.1f | hands height cm %.0f %.0f" % (
        (name,) + tuple(dev.max(0)) + (spd[:, 0].mean(), spd[:, 1:3].mean(), P[:, 1, 2].mean(), P[:, 2, 2].mean())))
