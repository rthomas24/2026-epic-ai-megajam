import bpy,bmesh,math,numpy as np,sys
bpy.ops.wm.open_mainfile(filepath='/tmp/rwork/asm.blend')
sc=bpy.context.scene
body=bpy.data.objects['SM_Rift']
log=[]
# ---- body cleanup: drop small post-cut fragments, fill pinholes
bm=bmesh.new(); bm.from_mesh(body.data)
bm.faces.ensure_lookup_table()
seen=set(); comps=[]
for f in bm.faces:
    if f.index in seen: continue
    st=[f]; seen.add(f.index); c=[]
    while st:
        g=st.pop(); c.append(g)
        for e in g.edges:
            for h in e.link_faces:
                if h.index not in seen: seen.add(h.index); st.append(h)
    comps.append(c)
comps.sort(key=len,reverse=True)
dl=[f for c in comps[1:] for f in c]
log.append(f'post-cut fragments removed: {len(comps)-1} islands / {len(dl)} tris')
bmesh.ops.delete(bm,geom=dl,context='FACES')
uvl=bm.loops.layers.uv.active
be=[e for e in bm.edges if e.is_boundary and e.verts[0].co.x>-1.05 and e.verts[1].co.x>-1.05]
res=bmesh.ops.holes_fill(bm,edges=be,sides=8)
nf=res['faces']
for f in nf:
    for l in f.loops:
        # borrow UV from an adjacent original loop of the same vertex
        for l2 in l.vert.link_loops:
            if l2.face not in nf: l[uvl].uv=l2[uvl].uv.copy(); break
bmesh.ops.triangulate(bm,faces=nf)
log.append(f'pinholes filled (<=8 sides, away from rear): {len(nf)} faces')
nb=sum(1 for e in bm.edges if e.is_boundary)
log.append(f'body boundary edges now: {nb} (all inside new rear solids)')
bm.to_mesh(body.data); bm.free()
# ---- normals on kept body
bpy.ops.object.select_all(action='DESELECT')
body.select_set(True); bpy.context.view_layer.objects.active=body
try: bpy.ops.mesh.customdata_custom_splitnormals_clear()
except Exception as e: print('noclear',e)
bpy.ops.object.shade_smooth_by_angle(angle=math.radians(40), keep_sharp_edges=False)
wn=body.modifiers.new('WN','WEIGHTED_NORMAL'); wn.keep_sharp=True; wn.weight=50; wn.mode='FACE_AREA'
bpy.ops.object.modifier_apply(modifier='WN')
# ---- new parts: join, smooth by angle, weighted normals, smart UV
news=[o for o in sc.objects if o.name.startswith('NEW_')]
bpy.ops.object.select_all(action='DESELECT')
for o in news: o.select_set(True)
bpy.context.view_layer.objects.active=news[0]
bpy.ops.object.join()
nw=bpy.context.view_layer.objects.active; nw.name='NEW_Parts'
me=nw.data
# store zone as int attribute
za=me.attributes.new('zone','INT','FACE')
for p in me.polygons: za.data[p.index].value=p.material_index
bm=bmesh.new(); bm.from_mesh(me)
bmesh.ops.remove_doubles(bm,verts=bm.verts,dist=0.00001)
# mark seams where zone changes so every UV island is a single zone
zl=bm.faces.layers.int.get('zone')
for e in bm.edges:
    lf=e.link_faces
    if len(lf)==2 and lf[0][zl]!=lf[1][zl]: e.seam=True
bm.to_mesh(me); bm.free()
bpy.ops.object.shade_smooth_by_angle(angle=math.radians(32), keep_sharp_edges=True)
# also make zone borders sharp
bm=bmesh.new(); bm.from_mesh(me)
for e in bm.edges:
    if e.seam: e.smooth=False
bm.to_mesh(me); bm.free()
wn=nw.modifiers.new('WN','WEIGHTED_NORMAL'); wn.keep_sharp=True; wn.weight=50; wn.mode='FACE_AREA'
bpy.ops.object.modifier_apply(modifier='WN')
if not me.uv_layers: me.uv_layers.new(name='UVMap')
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.0, area_weight=0.0, correct_aspect=True, scale_to_bounds=False)
bpy.ops.object.mode_set(mode='OBJECT')
# triangulate new parts (keep UV/normals) -> count
bm=bmesh.new(); bm.from_mesh(me); bmesh.ops.triangulate(bm,faces=bm.faces); bm.to_mesh(me); bm.free()
log.append(f'new parts tris: {len(me.polygons)}')
log.append(f'body tris: {len(body.data.polygons)}')
# dump UV data
def dump(o):
    m=o.data; uv=m.uv_layers.active.data
    tri=np.array([[uv[li].uv[:] for li in p.loop_indices] for p in m.polygons])
    return tri
bt=dump(body); nt=dump(nw)
# island ids for new parts (UV-connected faces)
m=nw.data; uv=m.uv_layers.active.data
from collections import defaultdict
key=defaultdict(list)
for p in m.polygons:
    for li,vi in zip(p.loop_indices,p.vertices):
        u=uv[li].uv; key[(vi,round(u[0],6),round(u[1],6))].append(p.index)
par=list(range(len(m.polygons)))
def find(a):
    while par[a]!=a: par[a]=par[par[a]]; a=par[a]
    return a
for k,fs in key.items():
    for f in fs[1:]:
        ra,rb=find(fs[0]),find(f)
        if ra!=rb: par[ra]=rb
isl=np.array([find(i) for i in range(len(m.polygons))])
zone=np.array([za.data[p.index].value for p in m.polygons]) if False else np.array([m.attributes['zone'].data[p.index].value for p in m.polygons])
area3=np.array([p.area for p in m.polygons])
np.savez('/tmp/rwork/uvdump.npz',body=bt,new=nt,isl=isl,zone=zone,area3=area3)
print('\n'.join(log))
bpy.ops.wm.save_as_mainfile(filepath='/tmp/rwork/b1.blend')
