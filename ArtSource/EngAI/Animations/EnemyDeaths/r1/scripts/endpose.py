import bpy, glob, os, math
from mathutils import Vector
for f in sorted(glob.glob('<work>/final/A_*.blend')):
    bpy.ops.wm.open_mainfile(filepath=f)
    sc=bpy.context.scene; t=bpy.data.objects['root']
    def chest(fr):
        sc.frame_set(fr)
        pb=t.pose.bones['spine_05']; M=t.matrix_world@pb.matrix; R=t.matrix_world@pb.bone.matrix_local
        q=M.to_quaternion()@R.to_quaternion().inverted()
        return q@Vector((0,-1,0))
    v=chest(sc.frame_end)
    up=v.z
    print('P',os.path.basename(f)[:-6],'chest-forward z=%.2f'%up, 'faces', 'UP (on back)' if up>0.5 else 'DOWN (face down)' if up<-0.5 else 'SIDE')
