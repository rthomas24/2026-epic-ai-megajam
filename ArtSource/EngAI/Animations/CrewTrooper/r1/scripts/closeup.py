import bpy, sys, os
os.environ['NORENDER']='1'
exec(open('/workspace/crewclips/render.py').read().replace("args = sys.argv","args = ['/tmp/x','2'] or sys.argv"))
from mathutils import Vector
ii=len(sys.argv)-1-sys.argv[::-1].index('--'); a=sys.argv[ii+1:]; OUT=a[0]; F=int(a[1])
sc=bpy.context.scene; tgt=bpy.data.objects['root']
sc.render.engine='BLENDER_WORKBENCH'; sc.display.shading.light='STUDIO'; sc.display.shading.show_cavity=True
sc.render.resolution_x=500; sc.render.resolution_y=500
cam_d=bpy.data.cameras.new('c'); cam=bpy.data.objects.new('c',cam_d); sc.collection.objects.link(cam); sc.camera=cam
cam_d.type='ORTHO'; cam_d.ortho_scale=float(a[2]) if len(a)>2 else 0.9
sc.frame_set(F)
c=(tgt.matrix_world@tgt.pose.bones[os.environ.get('CUB','hand_r')].head + tgt.matrix_world@tgt.pose.bones[os.environ.get('CUB2','hand_l')].head)/2
os.makedirs(OUT,exist_ok=True)
for vn,off in {'front':Vector((0,-3,0.3)),'side':Vector((3,0,0.2)),'top':Vector((0.3,-0.6,3)),'left':Vector((-3,-0.5,0.2))}.items():
    cam.location=c+off; cam.rotation_euler=(-off).to_track_quat('-Z','Y').to_euler()
    sc.render.filepath=os.path.join(OUT,vn+'.png'); bpy.ops.render.render(write_still=True)
