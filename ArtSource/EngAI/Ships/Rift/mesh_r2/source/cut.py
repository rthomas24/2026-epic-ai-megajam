import bpy, bmesh, sys, numpy as np
sys.path.insert(0,'/tmp/rwork'); from rlib import *
sc=reset()
bpy.ops.import_scene.fbx(filepath='/tmp/rift-r2-repo/ArtSource/EngAI/Ships/Rift/mesh_r1/SM_Rift.fbx')
o=bpy.data.objects['SM_Rift']; m=o.data
bm=bmesh.new(); bm.from_mesh(m)
sl=bm.faces.layers.int.new('src_face')
bm.faces.ensure_lookup_table()
for f in bm.faces: f[sl]=f.index
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0003)
def comps(bm):
    bm.faces.ensure_lookup_table(); seen=set(); out=[]
    for f in bm.faces:
        if f.index in seen: continue
        st=[f]; seen.add(f.index); c=[]
        while st:
            g=st.pop(); c.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in seen: seen.add(h.index); st.append(h)
        out.append(c)
    return sorted(out,key=len,reverse=True)
cs=comps(bm)
log=[]
dele=[f for c in cs[1:] for f in c]
log.append(f'islands after weld: {len(cs)}; deleted {len(cs)-1} detached islands ({len(dele)} tris) incl. ragged ring remnant')
bmesh.ops.delete(bm, geom=dele, context='FACES')
X0=float(sys.argv[-3]); YW=float(sys.argv[-2]); X1=float(sys.argv[-1])
cut=[]
for f in bm.faces:
    c=f.calc_center_median()*100
    if (c.x< X0 and abs(c.y)<YW and -97<c.z<2) or (c.x<X0 and abs(c.y)<55 and c.z>=2) or (X0<=c.x< X1 and abs(c.y)<50 and c.z>-22):
        cut.append(f)
log.append(f'region cut: {len(cut)} tris')
bmesh.ops.delete(bm, geom=cut, context='FACES')
cs=comps(bm); small=[]
for c in cs[1:]:
    vs=np.array([v.co[:] for f in c for v in f.verts])*100; ctr=(vs.min(0)+vs.max(0))/2; r=np.linalg.norm(vs-ctr,axis=1).max()
    small.append((len(c),ctr.round(1),round(r,2)))
log.append(f'post-cut islands: {len(cs)} -> {small}')
bm.to_mesh(m); bm.free()
print('\n'.join(log))
bpy.ops.wm.save_as_mainfile(filepath='/tmp/rwork/cut.blend')
