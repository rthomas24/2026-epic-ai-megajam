import bpy, sys, math, os, json
from mathutils import Vector
# args: blend_path outdir [extra_frames_csv]
args=sys.argv[sys.argv.index('--')+1:]
blend, outdir = args[0], args[1]
extra=[]
if len(args)>2:
    extra=[int(x) for x in args[2].split(',') if x]
os.makedirs(outdir, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=blend)
sc=bpy.context.scene
sc.render.engine='BLENDER_WORKBENCH'
sc.display.shading.light='STUDIO'
sc.display.shading.color_type='OBJECT'
sc.display.shading.show_shadows=True
sc.display.shading.show_cavity=True
sc.render.resolution_x=360
sc.render.resolution_y=480
sc.render.image_settings.file_format='PNG'
sc.render.fps=30
if not sc.world:
    sc.world=bpy.data.worlds.new('w')
tgt=bpy.data.objects['root']
# color meshes
for o in bpy.data.objects:
    if o.type=='MESH':
        if 'SK_' in o.name or o.name.startswith('SK'):
            o.hide_render=False; o.hide_viewport=False
            o.color=(0.42,0.44,0.52,1)
        else:
            # hide source smpl if present
            if 'Reference' in o.name or o.name.lower().startswith('mesh'):
                o.hide_render=True; o.hide_viewport=True
# ground in metres? After usf1/cm scene, check tgt scale / pelvis
sc.frame_set(int(sc.frame_start) if sc.frame_start else 1)
bpy.context.view_layer.update()
pb=tgt.pose.bones['pelvis']
p0=(tgt.matrix_world @ pb.matrix).to_translation()
# If pelvis z ~90, units are cm; we want to work in metres for camera convenience.
# Easier: detect unit. If p0.z>10, scene is cm.
cm = p0.z>10
u = 0.01 if cm else 1.0  # multiplier to metres
# ground
size=12 if not cm else 1200
bpy.ops.mesh.primitive_plane_add(size=size, location=(0,0,0))
g=bpy.context.object; g.color=(0.28,0.28,0.30,1)
step=0.5 if not cm else 50
ext=8
for i in range(-ext,ext+1):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(i*step,0, (0.001 if not cm else 0.1)))
    c=bpy.context.object; c.scale=((0.004,8,0.001) if not cm else (0.4,800,0.1)); c.color=(0.16,0.16,0.18,1)
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0,i*step, (0.001 if not cm else 0.1)))
    c=bpy.context.object; c.scale=((8,0.004,0.001) if not cm else (800,0.4,0.1)); c.color=(0.16,0.16,0.18,1)
# 180 cm figure
fig_r=0.12 if not cm else 12
fig_d=1.8 if not cm else 180
fig_z=0.9 if not cm else 90
bpy.ops.mesh.primitive_cylinder_add(radius=fig_r, depth=fig_d, location=(1.2 if not cm else 120, 0.4 if not cm else 40, fig_z))
fig=bpy.context.object; fig.color=(0.95,0.55,0.12,1)
# preview rifle for trooper aim clips (not exported; contact-sheet only)
if 'hand_r' in tgt.pose.bones and 'hand_l' in tgt.pose.bones and 'Trooper' in blend:
    bpy.ops.mesh.primitive_cube_add(size=1)
    rf=bpy.context.object
    rf.color=(0.85,0.15,0.1,1)
    import mathutils
    rf.data.transform(mathutils.Matrix.Translation((0,0.5,0)))
    # scene may be metres
    rf.scale = (0.035, 0.78, 0.055)
    c1=rf.constraints.new('COPY_LOCATION'); c1.target=tgt; c1.subtarget='hand_r'
    c2=rf.constraints.new('DAMPED_TRACK'); c2.target=tgt; c2.subtarget='hand_l'; c2.track_axis='TRACK_Y'
# frame range from action
act=tgt.animation_data.action if tgt.animation_data else None
if act:
    f0,f1=[int(round(x)) for x in act.frame_range]
else:
    f0,f1=int(sc.frame_start), int(sc.frame_end)
N=f1-f0+1
NF=8
picks=[f0+round(i*(N-1)/(NF-1)) for i in range(NF)]
for f in extra:
    if f not in picks:
        picks.append(f)
picks=sorted(set(picks))
# camera
# character height
heads=[]
def head_z(f):
    sc.frame_set(f); bpy.context.view_layer.update()
    h=tgt.pose.bones.get('head') or tgt.pose.bones['pelvis']
    return (tgt.matrix_world @ h.matrix).to_translation().z
ht=max(head_z(f0), (1.9 if not cm else 190))
# ortho scale covers height + margin
cam_d=bpy.data.cameras.new('c'); cam=bpy.data.objects.new('c',cam_d); sc.collection.objects.link(cam); sc.camera=cam
cam_d.type='ORTHO'
# scale in scene units
cam_d.ortho_scale = (ht*1.7) 
views={
    'front': Vector((0, -4.8 if not cm else -480, ht*0.55)),
    'side': Vector((4.8 if not cm else 480, 0, ht*0.55)),
}
def pel(f):
    sc.frame_set(f); bpy.context.view_layer.update()
    return tgt.matrix_world @ tgt.pose.bones['pelvis'].matrix.to_translation()
open(os.path.join(outdir,'meta.txt'),'w').write(f'cm={cm} f0={f0} f1={f1} picks={picks} ht={ht}\n')
for vn,off0 in views.items():
    for f in picks:
        p=pel(f)
        cam.location=Vector((p.x+off0.x, p.y+off0.y, off0.z))
        target=Vector((p.x,p.y, p.z*0.7 if p.z>0 else ht*0.4))
        direction=target-cam.location
        cam.rotation_euler=direction.to_track_quat('-Z','Y').to_euler()
        # figure beside, 180cm
        fig.location=Vector((p.x+(1.25 if not cm else 125), p.y+(0.4 if not cm else 40), fig_z))
        sc.render.filepath=os.path.join(outdir,f'{vn}_{f:04d}.png')
        bpy.ops.render.render(write_still=True)
print('OK', os.path.basename(blend), 'frames', f0, f1, 'picks', picks, 'cm', cm)
