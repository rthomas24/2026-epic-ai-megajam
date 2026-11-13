# Probe an FBX: armature object transform, root/pelvis/head rest positions (local + world), unit settings.
# Run: blender --background --factory-startup --python b_probe_fbx.py -- <in.fbx>
import bpy, sys
argv = sys.argv[sys.argv.index("--") + 1:]
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=argv[0], use_anim=False, automatic_bone_orientation=False)
for o in bpy.context.scene.objects:
    print("OBJ", o.name, o.type, "parent", o.parent.name if o.parent else None, "loc", tuple(round(v, 4) for v in o.location),
          "rot", tuple(round(v, 4) for v in o.rotation_euler), "scale", tuple(round(v, 4) for v in o.scale))
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
bones = arm.data.bones
print("NBONES", len(bones), "roots", [b.name for b in bones if b.parent is None])
for n in ("root", "pelvis", "spine_01", "head", "foot_l", "hand_r"):
    b = bones.get(n)
    if b:
        print("BONE", n, "head_local", tuple(round(v, 4) for v in b.head_local), "world", tuple(round(v, 4) for v in (arm.matrix_world @ b.head_local)))
