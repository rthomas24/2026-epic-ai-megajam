# Re-import every final FBX into a clean Blender scene and record what an importer will see.
import bpy, sys, json, glob, os, struct
from mathutils import Vector
W = '<repo>/ArtSource/EngCharacters'
REF = {'Grunt': W+'/Crew/export/NullGrunt/SK_NullGrunt.fbx', 'Heavy': W+'/Crew/export/NullHeavy/SK_NullHeavy.fbx', 'Warden': W+'/Warden/export/SK_Warden.fbx'}
refbones = {}
for ch, f in REF.items():
    bpy.ops.wm.read_factory_settings(use_empty=True); bpy.ops.import_scene.fbx(filepath=f)
    a = bpy.data.objects['root']
    refbones[ch] = {b.name: (a.matrix_world @ b.head_local).copy() for b in a.data.bones}
    refbones[ch+'_parent'] = {b.name: (b.parent.name if b.parent else None) for b in a.data.bones}
out = {}
for f in sorted(glob.glob('<work>/final/A_*.fbx')):
    ch = os.path.basename(f).split('_')[1]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    raw = open(f,'rb').read(); i = raw.find(b'UnitScaleFactor'); j = raw.find(b'D', i+40); usf = struct.unpack('<d', raw[j+1:j+9])[0]
    i2 = raw.find(b'CustomFrameRate'); j2 = raw.find(b'D', i2+40); cfr = struct.unpack('<d', raw[j2+1:j2+9])[0]
    bpy.ops.import_scene.fbx(filepath=f)
    sc = bpy.context.scene
    a = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
    act = a.animation_data.action; f0, f1 = [int(round(x)) for x in act.frame_range]
    names = set(b.name for b in a.data.bones); ref = refbones[ch]
    # armature-only FBX has no bind pose, so compare pose-independent bone offsets (head distance to the parent's head)
    def offs(H, par): return {n: ((H[n]-H[par[n]]).length if par[n] else 0.0) for n in H}
    H = {b.name: (a.matrix_world @ b.head_local).copy() for b in a.data.bones}
    o1 = offs(H, {b.name: (b.parent.name if b.parent else None) for b in a.data.bones}); o0 = offs(ref, refbones[ch+'_parent'])
    SKIP = lambda n: n.startswith('ik_') or n in ('pelvis','root','interaction','center_of_mass')
    rest_err = max(abs(o1[n]-o0[n]) for n in o1 if n in o0 and not SKIP(n))
    sc.frame_set(f0); posed = {b.name: (a.matrix_world @ a.pose.bones[b.name].head) for b in a.data.bones}
    o2 = offs(posed, {b.name: (b.parent.name if b.parent else None) for b in a.data.bones})
    par_ok = all((b.parent.name if b.parent else None) == refbones[ch+'_parent'][b.name] for b in a.data.bones if b.name in ref)
    P = lambda n: a.matrix_world @ a.pose.bones[n].head
    rec = []
    for fr in range(f0, f1+1):
        sc.frame_set(fr)
        rec.append({n: P(n).copy() for n in ('pelvis','foot_l','foot_r','ball_l','ball_r','head','hand_l','hand_r')} | {'root': a.matrix_world.to_translation().copy()})
    pz0 = rec[0]['pelvis'].z
    # foot slide while still standing (pelvis above 70 % of its first-frame height) and the foot on the floor
    slide = {}
    for s in 'lr':
        t = 0.0
        for x, y in zip(rec, rec[1:]):
            if x['pelvis'].z > 0.7*pz0 and min(x['ball_'+s].z, y['ball_'+s].z) < 0.05*pz0/0.96 and abs(y['foot_'+s].z - x['foot_'+s].z) < 0.01:
                t += (Vector((y['foot_'+s].x-x['foot_'+s].x, y['foot_'+s].y-x['foot_'+s].y))).length
        slide[s] = round(t*100, 2)
    # still from: first frame after which no tracked joint moves faster than 3 cm/s
    sp = [max((y[n]-x[n]).length for n in x)*30*100 for x, y in zip(rec, rec[1:])]
    still = len(rec)
    for k in range(len(sp)-1, -1, -1):
        if sp[k] > 3.0: break
        still = k+1
    trav = max((Vector((r['root'].x, r['root'].y))).length for r in rec)
    out[os.path.basename(f)] = dict(UnitScaleFactor=usf, fps_scene=sc.render.fps, fbx_CustomFrameRate=cfr, bones=len(names), ref_bones=len(ref),
        missing=sorted(set(ref)-names), extra=sorted(names-set(ref)), bone_offset_max_err_cm=round(rest_err*100,3), posed_bone_offset_max_err_cm=round(max(abs(o2[n]-o0[n]) for n in o2 if n in o0 and not SKIP(n))*100,3), hierarchy_matches=par_ok,
        armature_obj=a.name, obj_scale=round(a.scale[0],4), frames=f1-f0+1, first=f0, last=f1, seconds=round((f1-f0)/30,3),
        pelvis_z_cm=[round(min(r['pelvis'].z for r in rec)*100,1), round(max(r['pelvis'].z for r in rec)*100,1)],
        pelvis_end_z_cm=round(rec[-1]['pelvis'].z*100,1), head_end_z_cm=round(rec[-1]['head'].z*100,1),
        root_end_cm=[round(rec[-1]['root'].x*100,1), round(rec[-1]['root'].y*100,1), round(rec[-1]['root'].z*100,1)],
        root_end_dist_cm=round(Vector((rec[-1]['root'].x, rec[-1]['root'].y)).length*100,1), root_max_dist_cm=round(trav*100,1),
        foot_slide_standing_cm=slide, still_from_frame=still+f0)
for k, v in out.items(): print('V', k, json.dumps(v))
json.dump(out, open('<work>/final/verify.json','w'), indent=1)
