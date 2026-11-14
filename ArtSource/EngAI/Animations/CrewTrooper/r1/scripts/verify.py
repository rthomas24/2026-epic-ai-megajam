import bpy, sys, json, glob, os
from mathutils import Vector
files = sorted(glob.glob('/workspace/crewclips/final/A_*.fbx'))
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath='/tmp/wt-crew-trooper-clips/ArtSource/EngCharacters/Crew/export/NullTrooper/SK_NullTrooper.fbx')
ref = set(b.name for b in bpy.data.objects['root'].data.bones)
out = {}
for f in files:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    raw = open(f,'rb').read(); i = raw.find(b'UnitScaleFactor')
    import struct
    j = raw.find(b'D', i+40); usf = struct.unpack('<d', raw[j+1:j+9])[0]
    bpy.ops.import_scene.fbx(filepath=f)
    sc = bpy.context.scene
    arms = [o for o in bpy.data.objects if o.type == 'ARMATURE']; a = arms[0]
    act = a.animation_data.action; f0, f1 = [int(round(x)) for x in act.frame_range]
    names = set(b.name for b in a.data.bones)
    P = lambda n: a.matrix_world @ a.pose.bones[n].head
    rec = []
    for fr in range(f0, f1+1):
        sc.frame_set(fr)
        rec.append({n: P(n).copy() for n in ('pelvis','foot_l','foot_r','ball_l','ball_r','head','hand_r','hand_l')} | {'root': a.matrix_world.to_translation().copy()})
    slide = {}
    for s in 'lr':
        t = 0.0
        for x, y in zip(rec, rec[1:]):
            if x['pelvis'].z > 0.55 and x['ball_'+s].z < 0.04 and y['ball_'+s].z < 0.04 and x['foot_'+s].z < 0.13 and y['foot_'+s].z < 0.13:
                t += (Vector((y['foot_'+s].x-x['foot_'+s].x, y['foot_'+s].y-x['foot_'+s].y))).length
        slide[s] = round(t*100, 2)
    sc.frame_set(f0); A0 = {pb.name: (a.matrix_world @ pb.matrix).copy() for pb in a.pose.bones}
    sc.frame_set(f1); seam = max(((a.matrix_world @ pb.matrix).to_translation() - A0[pb.name].to_translation()).length for pb in a.pose.bones)
    hr = [ (r['hand_l']-r['hand_r']).length for r in rec ]
    out[os.path.basename(f)] = dict(UnitScaleFactor=usf, bones=len(names), missing_vs_SK=sorted(ref-names), extra=sorted(names-ref),
        armature_obj=a.name, obj_scale=round(a.scale[0],4), fps=sc.render.fps, frames=f1-f0+1, first=f0, last=f1, seconds=round((f1-f0)/30,3),
        pelvis_z_cm=[round(min(r['pelvis'].z for r in rec)*100,1), round(max(r['pelvis'].z for r in rec)*100,1)],
        head_z0_cm=round(rec[0]['head'].z*100,1), root_end_cm=[round(rec[-1]['root'].x*100,1), round(rec[-1]['root'].y*100,1), round(rec[-1]['root'].z*100,1)],
        ball_min_cm=round(min(min(r['ball_l'].z, r['ball_r'].z) for r in rec)*100,1),
        foot_slide_cm=slide, first_last_max_bone_delta_cm=round(seam*100,2), hand_gap_cm=[round(min(hr)*100,1), round(max(hr)*100,1)])
for k, v in out.items(): print('V', k, json.dumps(v))
json.dump(out, open('/workspace/crewclips/final/verify.json','w'), indent=1)
