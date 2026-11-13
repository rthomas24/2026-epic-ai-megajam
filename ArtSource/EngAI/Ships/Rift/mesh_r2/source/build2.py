import bpy,bmesh,json,numpy as np
bpy.ops.wm.open_mainfile(filepath='/tmp/rwork/b1.blend')
P=json.load(open('/tmp/rwork/pack.json')); s=P['scale']; G=P['G']; M=P['M']; place=P['place']
d=np.load('/tmp/rwork/uvdump.npz'); isl=d['isl']
nw=bpy.data.objects['NEW_Parts']; m=nw.data; uv=m.uv_layers.active.data
for p in m.polygons:
    x,y,w,h,rot,mn,mx=place[str(int(isl[p.index]))]
    for li in p.loop_indices:
        u,v=uv[li].uv
        if rot==0: px=x+M+(u-mn[0])*s*G; py=y+M+(mx[1]-v)*s*G
        else:      px=x+M+(v-mn[1])*s*G; py=y+M+(u-mn[0])*s*G
        uv[li].uv=(px/G, 1-py/G)
body=bpy.data.objects['SM_Rift']
# move zone attr: body faces get zone -1
za=body.data.attributes.new('zone','INT','FACE')
for i in range(len(body.data.polygons)): za.data[i].value=-1
# unify material slots: one M_Rift
mat=bpy.data.materials.get('M_Rift') or bpy.data.materials.new('M_Rift')
for o in (body,nw):
    o.data.materials.clear(); o.data.materials.append(mat)
    for p in o.data.polygons: p.material_index=0
bpy.ops.object.select_all(action='DESELECT')
nw.select_set(True); body.select_set(True); bpy.context.view_layer.objects.active=body
bpy.ops.object.join()
o=bpy.context.view_layer.objects.active; o.name='SM_Rift'; o.data.name='SM_Rift'
me=o.data
V=np.array([v.co[:] for v in me.vertices])
print('tris',len(me.polygons),'bbox min',(V.min(0)*100).round(3),'max',(V.max(0)*100).round(3))
uvd=me.uv_layers.active.data
tri=np.array([[uvd[li].uv[:] for li in p.loop_indices] for p in me.polygons])
pos=np.array([[me.vertices[vi].co[:] for vi in p.vertices] for p in me.polygons])
zone=np.array([me.attributes['zone'].data[i].value for i in range(len(me.polygons))])
src=np.array([me.attributes['src_face'].data[i].value for i in range(len(me.polygons))])
np.savez('/tmp/rwork/final_uv.npz',tri=tri,pos=pos,zone=zone,src=src)
bpy.ops.wm.save_as_mainfile(filepath='/tmp/rwork/b2.blend')
