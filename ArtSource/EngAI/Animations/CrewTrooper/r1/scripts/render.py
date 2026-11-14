import bpy, sys, math, os
from mathutils import Vector
args = sys.argv[sys.argv.index('--')+1:]; args = args[:args.index('--python')] if '--python' in args else args
BLEND_OUT_DIR = args[0]; NF = int(args[1]) if len(args) > 1 else 8
sc = bpy.context.scene
tgt = bpy.data.objects['root']
N = sc.frame_end
sc.render.engine = 'BLENDER_WORKBENCH'
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'SINGLE'
sc.display.shading.single_color = (0.55, 0.55, 0.6)
sc.display.shading.show_shadows = True; sc.display.shading.show_cavity = True
sc.render.resolution_x = 360; sc.render.resolution_y = 480
sc.render.image_settings.file_format = 'PNG'
sc.world = bpy.data.worlds.new('w') if not sc.world else sc.world
# ground
bpy.ops.mesh.primitive_plane_add(size=8, location=(0,0,0))
g = bpy.context.object; mat = bpy.data.materials.new('g'); g.data.materials.append(mat)
g.color = (0.3,0.3,0.32,1)
# grid lines every 50 cm
for i in range(-8, 9):
    for ax in (0,1):
        bpy.ops.mesh.primitive_cube_add(size=1, location=((i*0.5,0,0.001) if ax==0 else (0,i*0.5,0.001)))
        c = bpy.context.object; c.scale = ((0.004,8,0.001) if ax==0 else (8,0.004,0.001)); c.color=(0.18,0.18,0.2,1)
# 180 cm figure (capsule-ish) beside
bpy.ops.mesh.primitive_cylinder_add(radius=0.12, depth=1.8, location=(1.1, 0.4, 0.9))
fig = bpy.context.object; fig.color = (0.9, 0.55, 0.1, 1)
sc.display.shading.color_type = 'OBJECT'
for o in bpy.data.objects:
    if o.type == 'MESH' and o.name == 'SK_NullTrooper': o.color = (0.42,0.44,0.5,1)
import bmesh
em = bpy.data.objects.get('RiflePreview')
if em:
    # crude rifle: stock+receiver+barrel along local X (u), up local Y
    parts = [((0.30,0,0),(0.62,0.11,0.05)),((0.70,0.01,0),(0.30,0.03,0.03)),((0.17,-0.09,0),(0.05,0.12,0.035)),((0.36,-0.07,0),(0.06,0.10,0.04))]
    for loc, sz in parts:
        bpy.ops.mesh.primitive_cube_add(size=1); c = bpy.context.object
        c.scale = sz; c.location = loc; c.parent = em; c.color = (0.85,0.15,0.1,1)
bpy.ops.mesh.primitive_cube_add(size=1)
rf = bpy.context.object; rf.color = (0.85, 0.15, 0.1, 1)
rf.data.transform(__import__('mathutils').Matrix.Translation((0,0.5,0)))
rf.scale = (0.035, 0.75, 0.06)
c1 = rf.constraints.new('COPY_LOCATION'); c1.target = tgt; c1.subtarget = 'hand_r'
c2 = rf.constraints.new('DAMPED_TRACK'); c2.target = tgt; c2.subtarget = 'hand_l'; c2.track_axis = 'TRACK_Y'
if em: rf.hide_render = True
cam_d = bpy.data.cameras.new('c'); cam = bpy.data.objects.new('c', cam_d); sc.collection.objects.link(cam); sc.camera = cam
cam_d.type = 'ORTHO'; cam_d.ortho_scale = 2.6
views = {'front': (Vector((-2.2, -4.5, 1.15)),), 'side': (Vector((5, -0.0, 1.15)),)}
os.makedirs(BLEND_OUT_DIR, exist_ok=True)
picks = [1 + round(i*(N-1)/(NF-1)) for i in range(NF)]
for vn, (pos,) in ({} if os.environ.get('NORENDER') else views).items():
    for f in picks:
        sc.frame_set(f)
        # follow pelvis horizontally (root motion visible via grid)
        pv = tgt.matrix_world @ tgt.pose.bones['pelvis'].head
        center = Vector((pv.x*0.0, pv.y*0.0, 0.95))
        ctr = Vector((pv.x, pv.y, 0.0))
        cam.location = pos + ctr
        d = (Vector((0,0,0.95)) - pos); d.z = 0
        cam.rotation_euler = d.to_track_quat('-Z','Y').to_euler()
        cam.location.z = 0.95
        fig.location = ctr + Vector((1.1, 0.4, 0.9))
        sc.render.filepath = os.path.join(BLEND_OUT_DIR, f'{vn}_{f:03d}.png')
        bpy.ops.render.render(write_still=True)
print('PICKS', picks)
