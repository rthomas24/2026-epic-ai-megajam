"""Analyse raw HY-Motion SMPL-H FBX: travel, foot flight, hand speed, facing, loop seam."""
import bpy, sys, json, math, os
from mathutils import Vector

path = sys.argv[sys.argv.index('--')+1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=path, automatic_bone_orientation=False, ignore_leaf_bones=False)
arms = [o for o in bpy.data.objects if o.type=='ARMATURE']
assert arms, 'no armature in '+path
arm = arms[0]
# find bones
names = [b.name for b in arm.data.bones]
# SMPL-H typically Pelvis, L_Ankle/R_Ankle or left_ankle, etc.
def find(*cands):
    for c in cands:
        if c in arm.pose.bones: return c
        for n in arm.pose.bones.keys():
            if n.lower()==c.lower(): return n
    # fuzzy
    for c in cands:
        cl=c.lower().replace('_','')
        for n in arm.pose.bones.keys():
            if cl in n.lower().replace('_',''): return n
    return None

pel = find('Pelvis','pelvis','Hips','hips','root')
lank = find('L_Ankle','left_ankle','ankle_l','LeftFoot','LeftAnkle','L_Foot')
rank = find('R_Ankle','right_ankle','ankle_r','RightFoot','RightAnkle','R_Foot')
lwri = find('L_Wrist','left_wrist','hand_l','LeftHand','L_Hand','L_Wrist')
rwri = find('R_Wrist','right_wrist','hand_r','RightHand','R_Hand','R_Wrist')
head = find('Head','head')
# scene frames
sc=bpy.context.scene
# get action range
act=None
if arm.animation_data and arm.animation_data.action:
    act=arm.animation_data.action
frames=[]
if act:
    for fc in act.fcurves:
        for kp in fc.keyframe_points:
            frames.append(int(round(kp.co[0])))
f0,f1 = (min(frames), max(frames)) if frames else (sc.frame_start, sc.frame_end)
N=f1-f0+1

def world(bone, f):
    sc.frame_set(f)
    bpy.context.view_layer.update()
    return (arm.matrix_world @ arm.pose.bones[bone].matrix).to_translation()

def unit_scale():
    # guess cm vs m from pelvis height at first frame
    p=world(pel, f0)
    return 'cm' if p.z>10 else 'm'

us=unit_scale()
scl=1.0 if us=='cm' else 100.0  # to cm

series=[]
for f in range(f0,f1+1):
    rec={'f':f}
    rec['pelvis']=list(world(pel,f)*scl)
    if lank: rec['l_ankle']=list(world(lank,f)*scl)
    if rank: rec['r_ankle']=list(world(rank,f)*scl)
    if lwri: rec['l_wrist']=list(world(lwri,f)*scl)
    if rwri: rec['r_wrist']=list(world(rwri,f)*scl)
    if head: rec['head']=list(world(head,f)*scl)
    series.append(rec)

def v(a): return Vector(a)
# travel
p0=v(series[0]['pelvis']); pN=v(series[-1]['pelvis'])
horiz=Vector((pN.x-p0.x, pN.y-p0.y, 0))
# per-frame horizontal speed
speeds=[]
for i in range(1,len(series)):
    a=v(series[i-1]['pelvis']); b=v(series[i]['pelvis'])
    speeds.append((Vector((b.x-a.x,b.y-a.y,0)).length)*30)  # cm/s at 30fps
# foot flight: ankle z relative to min
def flight(key, thresh=4.0):
    if key not in series[0]: return None
    zs=[r[key][2] for r in series]
    zmin=min(zs)
    down=['1' if z<=zmin+thresh else '0' for z in zs]
    # contact line l/r later
    return dict(zmin=round(zmin,2), zmax=round(max(zs),2), down_frames=sum(int(x) for x in down), flight_frames=len(down)-sum(int(x) for x in down), down=''.join(down))

# hand speed peaks (combo impacts)
def hand_peaks(key, k=3):
    if key not in series[0]: return []
    sp=[]
    for i in range(1,len(series)):
        a=v(series[i-1][key]); b=v(series[i][key])
        sp.append((i, (b-a).length*30))
    # local maxima
    peaks=[]
    for i in range(1,len(sp)-1):
        if sp[i][1]>=sp[i-1][1] and sp[i][1]>=sp[i+1][1] and sp[i][1]>80:
            peaks.append({'frame': f0+sp[i][0], 'cm_s': round(sp[i][1],1)})
    peaks.sort(key=lambda x:-x['cm_s'])
    return peaks[:k]

# facing: pelvis +Y or -Y? use head-pelvis and maybe a forward bone. Approximate by first-to-mean travel? 
# Use first frame head-pelvis is up; left/right via ankles midpoint vs pelvis.
# loop seam: first vs last pose of listed bones
def seam():
    d=0; n=0
    for k in ('pelvis','l_ankle','r_ankle','l_wrist','r_wrist','head'):
        if k in series[0] and k in series[-1]:
            # horizontal for pelvis, full for others relative to pelvis
            a=v(series[0][k]); b=v(series[-1][k])
            if k=='pelvis':
                d += Vector((a.x-b.x,a.y-b.y,a.z-b.z)).length
            else:
                a=a-v(series[0]['pelvis']); b=b-v(series[-1]['pelvis'])
                d += (a-b).length
            n+=1
    return round(d,2)

# net direction in XY
# SMPL-H in these files typically faces -Y
# classify travel
dir_cm = {'x': round(horiz.x,1), 'y': round(horiz.y,1), 'horiz': round(horiz.length,1)}
mean_spd = sum(speeds)/len(speeds) if speeds else 0
# pelvis height
pz=[r['pelvis'][2] for r in series]

out={
    'file': os.path.basename(path),
    'armature': arm.name,
    'bones_sample': names[:8],
    'bone_map': {'pelvis':pel,'l_ankle':lank,'r_ankle':rank,'l_wrist':lwri,'r_wrist':rwri,'head':head},
    'units_guess': us,
    'frame_start': f0, 'frame_end': f1, 'frames': N, 'seconds': round((N-1)/30,3),
    'travel_cm': dir_cm,
    'mean_horiz_speed_cm_s': round(mean_spd,1),
    'max_horiz_speed_cm_s': round(max(speeds) if speeds else 0,1),
    'pelvis_z_cm': {'min':round(min(pz),1),'max':round(max(pz),1),'start':round(pz[0],1),'end':round(pz[-1],1)},
    'l_foot': flight('l_ankle'),
    'r_foot': flight('r_ankle'),
    'l_hand_peaks': hand_peaks('l_wrist'),
    'r_hand_peaks': hand_peaks('r_wrist'),
    'loop_seam_cm': seam(),
}
# also start-end pelvis z difference and whether still standing (pelvis z > 50)
out['standing_end'] = pz[-1] > 50
out['standing_start'] = pz[0] > 50
print(json.dumps(out))
open('/tmp/analyse_one.json','w').write(json.dumps(out, indent=2))
