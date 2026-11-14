import bpy,sys,json,glob,os
from mathutils import Vector
out={}
for f in sorted(glob.glob('<work>/raw/*2-s*.fbx')):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    try: bpy.ops.import_scene.fbx(filepath=f)
    except Exception as e: out[os.path.basename(f)]={'err':str(e)}; continue
    a=[o for o in bpy.data.objects if o.type=='ARMATURE'][0]
    sc=bpy.context.scene; f0,f1=[int(round(x)) for x in a.animation_data.action.frame_range]
    P=lambda n:(a.matrix_world@a.pose.bones[n].head).copy()
    R=[]
    for fr in range(f0,f1+1):
        sc.frame_set(fr); R.append({n:P(n) for n in ('Pelvis','Head','L_Ankle','R_Ankle','L_Wrist','R_Wrist')})
    p0=R[0]['Pelvis']; N=len(R)
    head=R[-1]['Head']-p0
    trav=max((Vector((r['Pelvis'].x-p0.x,r['Pelvis'].y-p0.y))).length for r in R)
    endtrav=(Vector((R[-1]['Pelvis'].x-p0.x,R[-1]['Pelvis'].y-p0.y))).length
    # fall frame: pelvis below 50% start height
    ff=next((i for i,r in enumerate(R) if r['Pelvis'].z<p0.z*0.45),None)
    # end activity: max ankle/wrist speed in last 15 frames (cm/s)
    def spd(n,i): return (R[i][n]-R[i-1][n]).length*30*100
    tail=max(spd(n,i) for n in ('L_Ankle','R_Ankle') for i in range(N-15,N))
    tailw=max(spd(n,i) for n in ('L_Wrist','R_Wrist') for i in range(N-15,N))
    # settle frame: after which all tracked joints move < 20cm/s
    st=N-1
    while st>1 and max(spd(n,st) for n in R[0])<20: st-=1
    # pre-fall ankle travel (while pelvis > 70% height)
    pre={s:sum((Vector((R[i][s].x-R[i-1][s].x,R[i][s].y-R[i-1][s].y))).length for i in range(1,N) if R[i]['Pelvis'].z>0.7*p0.z) for s in ('L_Ankle','R_Ankle')}
    out[os.path.basename(f)[:-4]]=dict(frames=N,head_end_xy=[round(head.x*100),round(head.y*100)],pel_end_z=round(R[-1]['Pelvis'].z*100),pel_max_trav=round(trav*100),pel_end_trav=round(endtrav*100),fall_frame=ff,settle_frame=st+1,tail_ankle_cms=round(tail),tail_wrist_cms=round(tailw),pre_ankle_travel=[round(pre['L_Ankle']*100),round(pre['R_Ankle']*100)])
for k,v in out.items(): print('A',k,json.dumps(v))
json.dump(out,open('<work>/analyse2.json','w'),indent=1)
