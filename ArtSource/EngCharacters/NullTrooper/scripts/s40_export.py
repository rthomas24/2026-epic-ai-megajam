# Stage 40 (step F, Blender half): export SK_NullTrooper + armature 'root' to export/SK_NullTrooper.fbx and write a manifest.
import bpy, os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE); import nt_io
ASSET = sys.argv[sys.argv.index("--") + 1] if len(sys.argv) > sys.argv.index("--") + 1 else "NullTrooper"
EXPORT = os.path.join(ROOT, "export") if ASSET == "NullTrooper" else os.path.join(ROOT, "export", ASSET)
arm = bpy.data.objects["root"]; sk = bpy.data.objects["SK_" + ASSET]
for pb in arm.pose.bones: pb.matrix_basis.identity()
bpy.context.view_layer.update()
out = os.path.join(EXPORT, "SK_%s.fbx" % ASSET)
import math
from mathutils import Vector, Matrix
hb_ = arm.data.bones
hand_ = hb_["hand_r"].head_local.copy(); mid_ = hb_["middle_01_r"].head_local.copy()
fwd_ = (mid_ - hand_).normalized()
SOCKETS = [("Muzzle", "hand_r", hand_ + fwd_ * 0.2, fwd_), ("HeadGlow", "head", Vector((0, -0.15, 1.705)), Vector((0, -1, 0)))]
empties = []
for o in list(bpy.data.objects):
    if o.name.startswith("SOCKET_"): bpy.data.objects.remove(o)
for name, bone, pt, dr in SOCKETS:
    e = bpy.data.objects.new("SOCKET_" + name, None); bpy.context.scene.collection.objects.link(e)
    x = Vector(dr).normalized(); z = Vector((0, 0, 1)); y = z.cross(x).normalized(); z = x.cross(y)
    target = Matrix.Translation(pt) @ Matrix((x, y, z)).transposed().to_4x4()
    bm_ = hb_[bone].matrix_local
    local = bm_.inverted() @ target
    local.translation = local.translation * 100.0     # Unreal reads the socket offset in cm (measured: 0.01x otherwise)
    e.parent = arm; e.parent_type = 'BONE'; e.parent_bone = bone
    bpy.context.view_layer.update()
    e.matrix_world = bm_ @ local
    empties.append(e)
bpy.context.view_layer.update()
for o in bpy.context.selected_objects: o.select_set(False)
arm.select_set(True); sk.select_set(True)
for e in empties: e.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.fbx(filepath=out, use_selection=True, object_types={'ARMATURE', 'MESH', 'EMPTY'}, add_leaf_bones=False, bake_anim=False,
                         mesh_smooth_type='FACE', use_tspace=True, use_armature_deform_only=False, primary_bone_axis='Y', secondary_bone_axis='X',
                         axis_forward='-Z', axis_up='Y', apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL', global_scale=1.0,
                         use_mesh_modifiers=True, use_triangles=True)
for e in empties: bpy.data.objects.remove(e)
sk.data.calc_loop_triangles()
co = [v.co for v in sk.data.vertices]
man = {"asset": "SK_" + ASSET, "skeleton": "/Game/Characters/Mannequins/Meshes/SK_Mannequin",
       "physics_asset": "/Game/Characters/Mannequins/Rigs/PA_Mannequin",
       "tris": len(sk.data.loop_triangles), "verts": len(co), "bones_in_fbx": len(arm.data.bones) + 1,
       "size_cm": [round((max(c[i] for c in co) - min(c[i] for c in co)) * 100, 1) for i in range(3)],
       "material_slots": [m.name for m in sk.data.materials],
       "textures": ["T_%s_%s.png" % (ASSET, x) for x in ("BC", "N", "ORM", "E")],
       "emission_strength_blender": 3.0, "normal_map": "DirectX (no green flip in Unreal)"}
man["sockets"] = [{"name": n_, "bone": b_, "how": "SOCKET_ empty under the bone in the FBX"} for n_, b_, _p, _d in SOCKETS]
json.dump(man, open(os.path.join(EXPORT, "SK_%s.manifest.json" % ASSET), "w"), indent=1)
print("EXPORT", json.dumps(man))
