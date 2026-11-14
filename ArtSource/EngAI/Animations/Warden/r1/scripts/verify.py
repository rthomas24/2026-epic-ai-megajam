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
arm,body=load(a['ref']); ref={b.name:(arm.matrix_world@b.head_local) for b in arm.data.bones}; refname=arm.name
res={}
for p in a['clips']:
    arm,body=load(p); name=p.split('/')[-1][:-4]
    r={'fbx_settings':gs(p),'armature_object':arm.name,'bones':len(arm.data.bones)}
    names={b.name for b in arm.data.bones}
    r['bone_names_match_SK_Warden']= names==set(ref) and arm.name==refname
    r['rest_max_diff_cm']=round(max((arm.matrix_world@b.head_local-ref[b.name]).length for b in arm.data.bones),4)
    act=arm.animation_data.action; f0,f1=[int(x) for x in act.frame_range]
    r['frames']=f1-f0+1; r['first_frame_after_reimport']=f0; r['fps']=bpy.context.scene.render.fps
    r['object_animated']= any(fc.data_path in ('location','rotation_euler','rotation_quaternion','scale') for fc in act.fcurves)
    sc=bpy.context.scene; dg=bpy.context.evaluated_depsgraph_get()
    W=lambda n: arm.matrix_world@arm.pose.bones[n].head
    tr={k:[] for k in ('foot_l','foot_r','ball_l','ball_r','pelvis')}; low=[]; poses=[]
    for f in range(f0,f1+1):
        sc.frame_set(f)
        for k in tr: tr[k].append(W(k).copy())
        ev=body.evaluated_get(dg); me=ev.to_mesh()
        zs=np.array([ (body.matrix_world@v.co).z for v in me.vertices]); low.append(float(zs.min())); ev.to_mesh_clear()
        poses.append({b.name:W(b.name).copy() for b in arm.pose.bones})
    def planted_travel(P, tol=3.0):
        zmin=min(p.z for p in P); t=0.0
        for i in range(1,len(P)):
            if P[i].z<zmin+tol and P[i-1].z<zmin+tol:
                d=P[i]-P[i-1]; t+=math.hypot(d.x,d.y)
        return round(t,1)
    r['planted_travel_cm']={k:planted_travel(tr[k]) for k in ('foot_l','foot_r','ball_l','ball_r')}
    r['foot_total_travel_cm']={k:round(sum(math.hypot(*(tr[k][i]-tr[k][i-1]).xy) for i in range(1,len(tr[k]))),1) for k in ('foot_l','foot_r')}
    px=[p.x for p in tr['pelvis']]; py=[p.y for p in tr['pelvis']]
    r['pelvis_xy_range_cm']=[round(max(px)-min(px),1), round(max(py)-min(py),1)]
    r['pelvis_net_xy_cm']=round(math.hypot(px[-1]-px[0],py[-1]-py[0]),1)
    r['lowest_per_frame']=[round(x,1) for x in low]
    r['lowest_vertex_z_cm']={'min':round(min(low),1),'max':round(max(low),1)}
    r['start_end_max_bone_delta_cm']=round(max((poses[0][n]-poses[-1][n]).length for n in poses[0]),1)
    res[name]=r
    print(name, json.dumps(r))
json.dump(res,open(a['out'],'w'),indent=1)
