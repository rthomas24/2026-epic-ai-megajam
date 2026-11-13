# Re-export a crew FBX (bones in metres -> Unreal root bone at scale 100, which collapses every animation to 1/100)
# with the 100x baked into the data and a 0.01 unit scale, exactly as the pilot (p10_build.py): root bone at scale 1.
# Drops the SOCKET_ empties (the Unreal import script re-adds Muzzle / HeadGlow with the measured offsets).
# Run: blender --background --factory-startup --python b_cm_export.py -- <in.fbx> <out.fbx>
import bpy, sys, json
from mathutils import Matrix, Vector

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = argv[0], argv[1]
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=SRC, use_anim=False, automatic_bone_orientation=False, ignore_leaf_bones=False)
scene = bpy.context.scene
for o in list(scene.objects):
    if o.name.startswith("SOCKET_"):
        bpy.data.objects.remove(o)
arm = next(o for o in scene.objects if o.type == 'ARMATURE')
meshes = [o for o in scene.objects if o.type == 'MESH']
before = {b.name: b.head_local.copy() for b in arm.data.bones}
print("INFO arm", arm.name, "scale", tuple(arm.scale), "bones", len(arm.data.bones), "meshes", [(m.name, m.parent.name if m.parent else None, m.parent_type) for m in meshes])
for pb in arm.pose.bones:
    pb.matrix_basis.identity()
scene.unit_settings.scale_length = 0.01
arm.scale = (100.0, 100.0, 100.0)
for o in bpy.context.selected_objects:
    o.select_set(False)
arm.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
for m in meshes:
    m.matrix_parent_inverse = Matrix.Identity(4)
    m.location = (0, 0, 0); m.rotation_euler = (0, 0, 0); m.rotation_quaternion = (1, 0, 0, 0); m.scale = (1, 1, 1)
    m.data.transform(Matrix.Scale(100.0, 4))
bpy.context.view_layer.update()
worst = max((arm.data.bones[n].head_local - v * 100.0).length for n, v in before.items())
for m in meshes:
    zs = [(m.matrix_world @ v.co).z for v in m.data.vertices]
    print("INFO mesh", m.name, "height units", round(max(zs) - min(zs), 2), "verts", len(m.data.vertices), "mats", [x.name for x in m.data.materials])
print("INFO bone rest x100 max error", round(worst, 5), "head", tuple(round(v, 2) for v in arm.data.bones["head"].head_local))
# sockets, as NullTrooper/scripts/s40_export.py places them, now in centimetres (no extra x100 on the offset)
hb_ = arm.data.bones
hand_ = hb_["hand_r"].head_local.copy(); mid_ = hb_["middle_01_r"].head_local.copy()
fwd_ = (mid_ - hand_).normalized()
SOCKETS = [("Muzzle", "hand_r", hand_ + fwd_ * 20.0, fwd_), ("HeadGlow", "head", Vector((0, -15.0, 170.5)), Vector((0, -1, 0)))]
empties = []
for name, bone, pt, dr in SOCKETS:
    e = bpy.data.objects.new("SOCKET_" + name, None); scene.collection.objects.link(e)
    x = Vector(dr).normalized(); z = Vector((0, 0, 1)); y = z.cross(x).normalized(); z = x.cross(y)
    target = Matrix.Translation(pt) @ Matrix((x, y, z)).transposed().to_4x4()
    bm_ = hb_[bone].matrix_local
    local = bm_.inverted() @ target
    e.parent = arm; e.parent_type = 'BONE'; e.parent_bone = bone
    bpy.context.view_layer.update()
    e.matrix_world = arm.matrix_world @ bm_ @ local
    empties.append(e)
    print("INFO socket", name, bone, "bone-local t", tuple(round(v, 4) for v in local.translation))
bpy.context.view_layer.update()
for o in bpy.context.selected_objects:
    o.select_set(False)
arm.select_set(True)
for m in meshes:
    m.select_set(True)
for e in empties:
    e.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True, object_types={'ARMATURE', 'MESH', 'EMPTY'},
                         add_leaf_bones=False, bake_anim=False, mesh_smooth_type='FACE', use_tspace=True,
                         use_armature_deform_only=False, primary_bone_axis='Y', secondary_bone_axis='X',
                         axis_forward='-Z', axis_up='Y', apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
                         global_scale=1.0, use_mesh_modifiers=True, use_triangles=True)
print("EXPORT_OK", OUT)
