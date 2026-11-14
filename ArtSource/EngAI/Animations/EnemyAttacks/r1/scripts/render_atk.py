import bpy, sys, json, math
from mathutils import Vector
args = json.loads(sys.argv[sys.argv.index('--')+1])
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene; sc.unit_settings.scale_length = 0.01; sc.render.fps = 30
bpy.ops.import_scene.fbx(filepath=args['fbx'], automatic_bone_orientation=False, ignore_leaf_bones=False)
arm = [o for o in bpy.data.objects if o.type=='ARMATURE'][0]
body = [o for o in bpy.data.objects if o.type=='MESH'][0]
act = arm.animation_data.action; f0, f1 = [int(x) for x in act.frame_range]
def colr(n):
    if 'Shield' in n: return (0.75,0.12,0.1,1)
    if 'Gun' in n: return (0.85,0.38,0.1,1)
    return (0.42,0.42,0.46,1)
for m in body.data.materials: m.diffuse_color = colr(m.name)
sc.render.engine = 'BLENDER_WORKBENCH'; sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'MATERIAL'
sc.display.shading.show_shadows = True
sc.render.resolution_x = args.get('w', 360); sc.render.resolution_y = args.get('h', 480)
sc.render.film_transparent = False
w = bpy.data.worlds.new('w'); sc.world = w; w.color = (0.08,0.085,0.1)
# floor grid (50 cm tiles via checker material not avail in workbench: use plane + 180cm figure)
bpy.ops.mesh.primitive_plane_add(size=1200, location=(0,0,0)); fl = bpy.context.object
mf = bpy.data.materials.new('floor'); mf.diffuse_color = (0.18,0.19,0.2,1); fl.data.materials.append(mf)
# foot marks: small discs at frame-1 foot positions to show sliding
fig = None
if args.get('figure', True):
    bpy.ops.mesh.primitive_cylinder_add(radius=18, depth=180, location=(args.get('figx',150), args.get('figy',40), 90)); fig = bpy.context.object
    mfig = bpy.data.materials.new('fig'); mfig.diffuse_color = (0.2,0.55,0.9,1); fig.data.materials.append(mfig)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam')); sc.collection.objects.link(cam); sc.camera = cam
cam.data.clip_start = 1; cam.data.clip_end = 10000; cam.data.type = 'ORTHO'; cam.data.ortho_scale = args.get('ortho', 380)
view = args.get('view', 'q34')
tgt = Vector((0, args.get('ty',0), args.get('tz',135)))
dirs = {'q34': Vector((-0.55,-1,0.12)), 'front': Vector((0,-1,0.05)), 'side': Vector((-1,0,0.05)), 'q34l': Vector((0.6,-1,0.12))}
cam.location = tgt + dirs[view].normalized() * 1500
cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z','Y').to_euler()
frames = args.get('frames') or list(range(f0, f1+1))
for f in frames:
    sc.frame_set(f)
    if args.get('focus'):
        tgt = arm.matrix_world @ arm.pose.bones[args['focus']].head
        cam.location = tgt + dirs[view].normalized() * 1500
        cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z','Y').to_euler()
    sc.render.filepath = f"{args['outdir']}/{args['tag']}_{view}_{f:03d}.png"
    bpy.ops.render.render(write_still=True)
print('RANGE', f0, f1)
