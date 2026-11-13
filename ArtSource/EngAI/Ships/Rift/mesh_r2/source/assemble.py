import bpy,bmesh,sys; sys.path.insert(0,'/tmp/rwork')
import importlib, parts; importlib.reload(parts)
bpy.ops.wm.open_mainfile(filepath='/tmp/rwork/cut.blend')
sc=bpy.context.scene
objs=parts.build(sc.collection)
for o in objs:
    bpy.context.view_layer.objects.active=o; o.select_set(True)
bpy.ops.object.transform_apply(scale=True)
t=0
for o in objs:
    n=sum(len(p.vertices)-2 for p in o.data.polygons); t+=n; print(o.name,n)
print('new tris',t)
bpy.ops.wm.save_as_mainfile(filepath='/tmp/rwork/asm.blend')
