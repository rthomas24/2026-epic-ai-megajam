import bpy,sys,math,json
from mathutils import Vector
files=sys.argv[sys.argv.index('--')+1:]
out={}
for p in files:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=p)
    arm=[o for o in bpy.data.objects if o.type=='ARMATURE'][0]
    act=arm.animation_data.action
    f0,f1=[int(x) for x in act.frame_range]
    sc=bpy.context.scene
    def P(n): return arm.matrix_world@arm.pose.bones[n].head
    rec=[]
    for f in range(f0,f1+1):
        sc.frame_set(f)
        rec.append({n:P(n).copy() for n in ['Pelvis','L_Foot','R_Foot','L_Ankle','R_Ankle','L_Hip','R_Hip','L_Wrist','R_Wrist','Head']})
    # ground from leg mesh at frame f0
    sc.frame_set(f0)
    dg=bpy.context.evaluated_depsgraph_get()
    gz=min((o.evaluated_get(dg).matrix_world@v.co).z for o in bpy.data.objects if o.type=='MESH' and o.name=='leg' for v in o.evaluated_get(dg).data.vertices)
    hip=rec[0]['R_Hip']-rec[0]['L_Hip']; hipyaw=math.degrees(math.atan2(hip.y,hip.x))  # 180 = facing -Y (R at -X)
    hand=rec[0]['L_Wrist']-rec[0]['R_Wrist']; handyaw=math.degrees(math.atan2(hand.y,hand.x))
    tr=(rec[-1]['Pelvis']-rec[0]['Pelvis']); 
    def slide(n):
        s=0
        for a,b in zip(rec,rec[1:]):
            if a[n].z-gz<0.06 and b[n].z-gz<0.06: s+=(Vector((b[n].x,b[n].y))-Vector((a[n].x,a[n].y))).length
        return s
    lp=rec[-1]['Pelvis']-gz*Vector((0,0,1))
    out[p.split('/')[-1]]=dict(frames=f1-f0+1,ground=round(gz,3),pelvis0=round(rec[0]['Pelvis'].z-gz,3),pelvisEnd=round(rec[-1]['Pelvis'].z-gz,3),travel=[round(tr.x,2),round(tr.y,2)],hipyaw=round(hipyaw),rhand_to_lhand_yaw=round(handyaw),slideL=round(slide('L_Foot'),3),slideR=round(slide('R_Foot'),3),loopgap=round((rec[-1]['Pelvis']-rec[0]['Pelvis']).length,3))
for k,v in out.items(): print("RES",k,json.dumps(v))
