# Stage 40 (step F, Blender half): export SK_NullTrooper + armature 'root' to export/SK_NullTrooper.fbx and write a manifest.
import bpy, os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE); import nt_io
arm = bpy.data.objects["root"]; sk = bpy.data.objects["SK_NullTrooper"]
for pb in arm.pose.bones: pb.matrix_basis.identity()
bpy.context.view_layer.update()
out = os.path.join(ROOT, "export", "SK_NullTrooper.fbx")
nt_io.export_skeletal_fbx(arm, [sk], out)
sk.data.calc_loop_triangles()
co = [v.co for v in sk.data.vertices]
man = {"asset": "SK_NullTrooper", "skeleton": "/Game/Characters/Mannequins/Meshes/SK_Mannequin",
       "physics_asset": "/Game/Characters/Mannequins/Rigs/PA_Mannequin",
       "tris": len(sk.data.loop_triangles), "verts": len(co), "bones_in_fbx": len(arm.data.bones) + 1,
       "size_cm": [round((max(c[i] for c in co) - min(c[i] for c in co)) * 100, 1) for i in range(3)],
       "material_slots": [m.name for m in sk.data.materials],
       "textures": ["T_NullTrooper_BC.png", "T_NullTrooper_N.png", "T_NullTrooper_ORM.png", "T_NullTrooper_E.png"],
       "emission_strength_blender": 3.0, "normal_map": "DirectX (no green flip in Unreal)"}
json.dump(man, open(os.path.join(ROOT, "export", "SK_NullTrooper.manifest.json"), "w"), indent=1)
print("EXPORT", json.dumps(man))
