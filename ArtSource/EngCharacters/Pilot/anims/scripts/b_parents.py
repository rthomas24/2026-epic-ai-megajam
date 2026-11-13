# Write the SKEL_Pilot bone hierarchy (bone -> parent) from an Unreal-exported FBX to a JSON file.
# Run: blender --background --factory-startup --python b_parents.py -- <in.fbx> <out.json>
import bpy, sys, json
argv = sys.argv[sys.argv.index("--") + 1:]
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=argv[0], use_anim=False)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
d = {b.name: (b.parent.name if b.parent else arm.name) for b in arm.data.bones}
json.dump(d, open(argv[1], "w"), indent=0)
print("PARENTS armature", arm.name, "bones", len(d))
