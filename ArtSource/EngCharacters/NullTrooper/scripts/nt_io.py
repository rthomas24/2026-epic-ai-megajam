# Null Trooper: FBX import / export helpers shared by the stage scripts (Blender 5.2).
import bpy
from mathutils import Matrix


def import_ue_fbx(path):
    """Import a UE-exported skeletal FBX. Returns (armature, mesh) in metres with object scale 1.
    The armature object keeps the name 'root': Unreal turns that node back into the 'root' bone."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path, automatic_bone_orientation=False, ignore_leaf_bones=False, use_anim=False)
    new = [o for o in bpy.data.objects if o not in before]
    arm = next(o for o in new if o.type == 'ARMATURE')
    meshes = [o for o in new if o.type == 'MESH']
    empties = [o for o in new if o.type == 'EMPTY']
    bpy.context.view_layer.update()
    mw = arm.matrix_world.copy()
    s = mw.to_scale()[0]
    arm.parent = None
    arm.matrix_world = mw
    bpy.context.view_layer.update()
    for o in bpy.context.selected_objects: o.select_set(False)
    arm.select_set(True); bpy.context.view_layer.objects.active = arm
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for m in meshes:
        m.data.transform(Matrix.Scale(s, 4))
        m.matrix_parent_inverse = Matrix.Identity(4)
        m.location = (0, 0, 0); m.rotation_euler = (0, 0, 0); m.scale = (1, 1, 1)
    for e in empties: bpy.data.objects.remove(e)
    arm.name = "root"
    bpy.context.view_layer.update()
    return arm, (meshes[0] if meshes else None)


def export_skeletal_fbx(arm, meshes, path, scale_opt='FBX_SCALE_ALL'):
    for o in bpy.context.selected_objects: o.select_set(False)
    arm.select_set(True)
    for m in meshes: m.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'ARMATURE', 'MESH'},
                             add_leaf_bones=False, bake_anim=False, mesh_smooth_type='FACE', use_tspace=True,
                             use_armature_deform_only=False, primary_bone_axis='Y', secondary_bone_axis='X',
                             axis_forward='-Z', axis_up='Y', apply_unit_scale=True, apply_scale_options=scale_opt,
                             global_scale=1.0, use_mesh_modifiers=True, use_triangles=True)
