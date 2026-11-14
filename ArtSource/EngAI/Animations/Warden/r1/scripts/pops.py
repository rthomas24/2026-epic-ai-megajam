import bpy,sys,math
for p in sys.argv[sys.argv.index('--')+1:]:
    bpy.ops.wm.read_factory_settings(use_empty=True); bpy.context.scene.unit_settings.scale_length=0.01
    bpy.ops.import_scene.fbx(filepath=p, automatic_bone_orientation=False, ignore_leaf_bones=False)
    arm=[o for o in bpy.data.objects if o.type=='ARMATURE'][0]; act=arm.animation_data.action; f0,f1=[int(x) for x in act.frame_range]
    prev=None; worst=[]
    for f in range(f0,f1+1):
        bpy.context.scene.frame_set(f)
        cur={pb.name:(arm.matrix_world@pb.matrix).to_quaternion() for pb in arm.pose.bones if not pb.name.startswith(('ik_','interaction','center'))}
        if prev:
            for n,q in cur.items():
                ang=math.degrees(prev[n].rotation_difference(q).angle); ang=min(ang,360-ang)
                worst.append((ang,f,n))
        prev=cur
    worst.sort(reverse=True)
    print(p.split('/')[-1],'max deg/frame:',[(round(a),f,n) for a,f,n in worst[:4]]); w2=[x for x in worst if not any(k in x[2] for k in ('index','middle','ring','pinky','thumb','twist'))]; print('  non-finger:',[(round(a),f,n) for a,f,n in w2[:6]])
