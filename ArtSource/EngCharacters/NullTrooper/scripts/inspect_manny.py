import bpy, sys, json
from mathutils import Vector
fbx = sys.argv[sys.argv.index("--")+1]
for o in list(bpy.data.objects): bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=fbx, automatic_bone_orientation=False, ignore_leaf_bones=False, use_anim=False)
print("unit scale", bpy.context.scene.unit_settings.scale_length)
for o in bpy.data.objects:
    print("OBJ", o.name, o.type, "loc", tuple(round(v,4) for v in o.location), "rot", tuple(round(v,4) for v in o.rotation_euler), "scale", tuple(round(v,4) for v in o.scale), "parent", o.parent.name if o.parent else None)
    if o.type == 'MESH':
        me=o.data
        ws=[o.matrix_world@v.co for v in me.vertices]
        lo=Vector((min(v.x for v in ws),min(v.y for v in ws),min(v.z for v in ws)))
        hi=Vector((max(v.x for v in ws),max(v.y for v in ws),max(v.z for v in ws)))
        print(" verts",len(me.vertices),"polys",len(me.polygons),"lo",tuple(round(v,3) for v in lo),"hi",tuple(round(v,3) for v in hi))
        print(" mats",[m.name for m in me.materials],"uv",[u.name for u in me.uv_layers],"vgroups",len(o.vertex_groups))
        print(" mods",[(m.type) for m in o.modifiers])
    if o.type=='ARMATURE':
        print(" bones",len(o.data.bones))
        for n in ["root","pelvis","spine_01","spine_02","spine_03","spine_04","spine_05","neck_01","neck_02","head","clavicle_l","upperarm_l","lowerarm_l","hand_l","middle_01_l","middle_03_l","thigh_l","calf_l","foot_l","ball_l","thigh_r","upperarm_r","hand_r"]:
            b=o.data.bones.get(n)
            if b:
                h=o.matrix_world@b.head_local; t=o.matrix_world@b.tail_local
                print("  %-12s head %s tail %s" % (n, tuple(round(v,3) for v in h), tuple(round(v,3) for v in t)))
        print(" all:", [b.name for b in o.data.bones])
