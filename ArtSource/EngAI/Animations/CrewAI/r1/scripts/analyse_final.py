import bpy, sys, json, math, os
from mathutils import Vector
path = sys.argv[sys.argv.index('--')+1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=path, automatic_bone_orientation=False, ignore_leaf_bones=False)
arms=[o for o in bpy.data.objects if o.type=='ARMATURE']
arm=arms[0]
sc=bpy.context.scene
act=arm.animation_data.action if arm.animation_data else None
frames=[]
if act:
    for fc in act.fcurves:
        for kp in fc.keyframe_points:
            frames.append(int(round(kp.co[x] if False else kp.co[0])))
f0,f1=(min(frames),max(frames)) if frames else (int(sc.frame_start),int(sc.frame_end))
N=f1-f0+1
bones=['pelvis','hand_l','hand_r','foot_l','foot_r','spine_05','head','lowerarm_l']
present=[b for b in bones if b in arm.pose.bones]
def wpos(b,f):
    sc.frame_set(f); bpy.context.view_layer.update()
    return (arm.matrix_world @ arm.pose.bones[b].matrix).to_translation()
# units: if pelvis z > 10, already cm (usf1). if ~1, metres
p0=wpos('pelvis',f0)
scl=1.0 if p0.z>10 else 100.0
series={b:[] for b in present}
root=[]
for f in range(f0,f1+1):
    sc.frame_set(f); bpy.context.view_layer.update()
    root.append((arm.matrix_world @ arm.location)*scl if False else Vector(arm.matrix_world.to_translation())*scl)
    for b in present:
        series[b].append(wpos(b,f)*scl)

def peaks(key, min_speed=80, k=5):
    sp=[]
    pts=series[key]
    for i in range(1,len(pts)):
        sp.append((f0+i, (pts[i]-pts[i-1]).length*30))
    peaks=[]
    for i in range(1,len(sp)-1):
        if sp[i][1]>=sp[i-1][1] and sp[i][1]>=sp[i+1][1] and sp[i][1]>=min_speed:
            peaks.append({'frame':sp[i][0],'cm_s':round(sp[i][1],1)})
    peaks.sort(key=lambda x:-x['cm_s'])
    return peaks[:k]

# bone rotation snaps (deg/frame)
snaps=[]
for pb in arm.pose.bones:
    prev=None
    for f in range(f0,f1+1):
        sc.frame_set(f); bpy.context.view_layer.update()
        q=arm.pose.bones[pb.name].matrix.to_quaternion()
        if prev is not None:
            ang=math.degrees(prev.rotation_difference(q).angle)
            if ang>40:
                snaps.append({'bone':pb.name,'frame':f,'deg':round(ang,1)})
        prev=q
snaps.sort(key=lambda x:-x['deg'])

# travel of root and pelvis
pel=series['pelvis']
root_end=root[-1]-root[0]
pel_h=Vector((pel[-1].x-pel[0].x, pel[-1].y-pel[0].y, 0))
# loop seam relative to pelvis
seam=0
if 'head' in series:
    for b in present:
        a=series[b][0]-pel[0]; c=series[b][-1]-pel[-1]
        seam=max(seam, (a-c).length)

# foot min z
footz=[]
for b in ('foot_l','foot_r'):
    if b in series: footz += [p.z for p in series[b]]

out={
 'file':os.path.basename(path),
 'armature':arm.name,
 'n_bones':len(arm.data.bones),
 'bone_names_ok': all(n in arm.data.bones for n in ['pelvis','spine_05','hand_l','hand_r','foot_l','foot_r','ik_hand_gun']),
 'units_scl':scl,
 'pelvis_z_start_cm':round(pel[0].z,1),
 'pelvis_z_end_cm':round(pel[-1].z,1),
 'pelvis_z_min_cm':round(min(p.z for p in pel),1),
 'frame_start':f0,'frame_end':f1,'frames':N,'seconds':round((N-1)/30,3),
 'root_end_cm':[round(root_end.x,1),round(root_end.y,1),round(root_end.z,1)],
 'pelvis_horiz_end_cm':[round(pel_h.x,1),round(pel_h.y,1)],
 'loop_seam_rel_cm':round(seam,2),
 'l_hand_peaks':peaks('hand_l') if 'hand_l' in series else [],
 'r_hand_peaks':peaks('hand_r') if 'hand_r' in series else [],
 'snaps_over_40deg':snaps[:8],
 'max_snap_deg': snaps[0]['deg'] if snaps else 0,
 'foot_z_min_cm': round(min(footz),1) if footz else None,
}
print(json.dumps(out))
