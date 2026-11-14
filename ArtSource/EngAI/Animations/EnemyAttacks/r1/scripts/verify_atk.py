# Reimport verification for the enemy attack clips (frames, fps, units, skeleton, per-frame foot contact, slide, floor).
import bpy,sys,json,math
from mathutils import Vector
import numpy as np
sys.path.append('/home/box/.local/blender-4.5.3-linux-x64/4.5/scripts/addons_core')
from io_scene_fbx import parse_fbx
a=json.loads(sys.argv[sys.argv.index('--')+1])
def gs(p):
    root,ver=parse_fbx.parse(p); out={}
    for e in root.elems:
        if e.id==b'GlobalSettings':
            for c in e.elems:
                if c.id==b'Properties70':
                    for pp in c.elems:
                        n=pp.props[0].decode()
                        if n in ('UnitScaleFactor','TimeMode','CustomFrameRate','UpAxis','FrontAxis'): out[n]=pp.props[-1]
    return out
def load(p):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.scale_length=0.01
    bpy.ops.import_scene.fbx(filepath=p, automatic_bone_orientation=False, ignore_leaf_bones=False)
    arm=[o for o in bpy.data.objects if o.type=='ARMATURE'][0]; body=[o for o in bpy.data.objects if o.type=='MESH'][0]
    return arm, body
res={}
for c in a['clips']:
    p=c['fbx']; arm,body=load(c['ref']); ref={b.name:(arm.matrix_world@b.head_local) for b in arm.data.bones}; refname=arm.name
    arm,body=load(p); name=p.split('/')[-1][:-4]
    r={'fbx_settings':gs(p),'armature_object':arm.name,'bones':len(arm.data.bones)}
    r['bone_names_match_ref']= {b.name for b in arm.data.bones}==set(ref) and arm.name==refname
    r['rest_max_diff_cm']=round(max((arm.matrix_world@b.head_local-ref[b.name]).length for b in arm.data.bones),4)
    act=arm.animation_data.action; f0,f1=[int(x) for x in act.frame_range]
    n=f1-f0+1; r['frames']=n; r['seconds']=round((n-1)/30,3); r['first_frame_after_reimport']=f0; r['fps']=bpy.context.scene.render.fps
    sc=bpy.context.scene; dg=bpy.context.evaluated_depsgraph_get()
    V=c.get('inplace_speed',0.0)
    # foot vertex sets
    gi={g.index:g.name for g in body.vertex_groups}
    fv={'l':[],'r':[]}
    for v in body.data.vertices:
        if not v.groups: continue
        g=max(v.groups,key=lambda g:g.weight); nm=gi[g.group]
        for s in 'lr':
            if nm in (f'foot_{s}',f'ball_{s}'): fv[s].append(v.index)
    W=lambda b: arm.matrix_world@arm.pose.bones[b].head
    rec=[]
    for i,f in enumerate(range(f0,f1+1)):
        sc.frame_set(f)
        ev=body.evaluated_get(dg); me=ev.to_mesh()
        co=np.array([(body.matrix_world@v.co)[:] for v in me.vertices]); ev.to_mesh_clear()
        shift=np.array([0,-V*i/30.0,0])    # in-place loop: world = local + travel
        d={'low':float(co[:,2].min())}
        for s in 'lr':
            fc=co[fv[s]]+shift; k=int(np.argmin(fc[:,2]))
            d['sole_'+s]=float(fc[:,2].min()); d['solept_'+s]=fc[k].tolist()
            d['ball_'+s]=(Vector(W(f'ball_{s}'))+Vector(shift)).to_tuple(); d['ank_'+s]=(Vector(W(f'foot_{s}'))+Vector(shift)).to_tuple()
        for b in c.get('track',[]): d[b]=(Vector(W(b))+Vector(shift)).to_tuple()
        rec.append(d)
    TOL=c.get('contact_tol',2.0)
    cont={}; slide={}
    for s in 'lr':
        cs=[]
        for i,d in enumerate(rec):
            ok=d['sole_'+s]<=TOL
            if ok:
                # the contact point must be still (to the previous OR the next frame: touchdown / lift-off frames count)
                def mv(j):
                    if j<0 or j>=n: return 1e9
                    b0=Vector(rec[j]['ball_'+s]); b1=Vector(d['ball_'+s]); a0=Vector(rec[j]['ank_'+s]); a1=Vector(d['ank_'+s])
                    return min(math.hypot(*(b1-b0).xy), math.hypot(*(a1-a0).xy))
                ok = min(mv(i-1), mv(i+1))<=1.5
            cs.append(ok)
        cont[s]=cs
        # slide within each contact run (pivot point = whichever of ball / ankle moves less over the run)
        runs=[]; i=0
        while i<n:
            if cs[i]:
                j=i
                while j+1<n and cs[j+1]: j+=1
                bb=max(math.hypot(*(Vector(rec[k]['ball_'+s])-Vector(rec[i]['ball_'+s])).xy) for k in range(i,j+1))
                aa=max(math.hypot(*(Vector(rec[k]['ank_'+s])-Vector(rec[i]['ank_'+s])).xy) for k in range(i,j+1))
                runs.append({'frames_1based':[i+1,j+1],'ball_slide_cm':round(bb,2),'ankle_slide_cm':round(aa,2),'pivot_slide_cm':round(min(bb,aa),2)})
                i=j+1
            else: i+=1
        slide[s]=runs
    r['contact_tolerance_cm']=TOL
    r['contact_per_frame']={s:''.join('L' if s=='l' and x else 'R' if x else '.' for x in cont[s]) for s in 'lr'}
    both=['B' if cont['l'][i] and cont['r'][i] else 'l' if cont['l'][i] else 'r' if cont['r'][i] else '.' for i in range(n)]
    r['contact_line']=''.join(both)
    r['frames_without_any_contact']=[i+1 for i,x in enumerate(both) if x=='.']
    r['contact_runs']=slide
    r['max_planted_slide_cm']=round(max([x['pivot_slide_cm'] for s in 'lr' for x in slide[s]] or [0]),2)
    r['sole_min_cm']={s:round(min(d['sole_'+s] for d in rec),2) for s in 'lr'}
    r['lowest_vertex_cm']=round(min(d['low'] for d in rec),2)
    r['lowest_vertex_per_frame']=[round(d['low'],1) for d in rec]
    # clip-specific
    if c.get('kind')=='run':
        lean=[];drop=[];fwd=[]
        for i,f in enumerate(range(f0,f1+1)):
            sc.frame_set(f)
            pv=W('pelvis'); nk=W('neck_01'); v=nk-pv
            lean.append(math.degrees(math.atan2(math.hypot(v.x,v.y),v.z)))
            drop.append(W('upperarm_l').z-W('upperarm_r').z); fwd.append(W('upperarm_l').y-W('upperarm_r').y)
        r['torso_lean_deg_mean']=round(sum(lean)/n,1); r['torso_lean_deg_range']=[round(min(lean),1),round(max(lean),1)]
        r['shoulder_drop_cm_mean (left minus right shoulder height)']=round(sum(drop)/n,1)
        r['right_shoulder_ahead_cm_mean']=round(sum(fwd)/n,1)
        r['loop_first_last_max_bone_delta_cm']=0.0
        sc.frame_set(f0); P0={b.name:W(b.name).copy() for b in arm.pose.bones}; sc.frame_set(f1); P1={b.name:W(b.name).copy() for b in arm.pose.bones}
        r['loop_first_last_max_bone_delta_cm']=round(max((P0[k]-P1[k]).length for k in P0),3)
    if c.get('kind')=='bash':
        tb=c['track'][0]; vel=[0.0]+[ -(Vector(rec[i][tb])-Vector(rec[i-1][tb])).y*30 for i in range(1,n)]
        r['shield_fwd_speed_cm_s_per_frame (forearm, +=forward)']=[round(x) for x in vel]
        pk=max(range(n),key=lambda i:vel[i]); r['shield_peak_speed_frame_1based']=pk+1
        r['shield_fwd_position_cm_per_frame']=[round(-rec[i][tb][1]) for i in range(n)]
    if c.get('kind')=='slam':
        hz=[min(rec[i]['hand_l'][2],rec[i]['hand_r'][2]) for i in range(n)]
        r['hands_low_z_cm_per_frame']=[round(x) for x in hz]
        r['hands_top_frame_1based']=max(range(n),key=lambda i:hz[i])+1
        r['first_frame_hands_within_15cm_of_floor_1based']=next((i+1 for i in range(n) if hz[i]<=15),None)
    res[name]=r
    print('VER',name,json.dumps({k:v for k,v in r.items() if 'per_frame' not in k and k!='contact_runs'}))
json.dump(res,open(a['out'],'w'),indent=1)
