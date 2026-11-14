import bpy,bmesh,numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree
bpy.ops.wm.open_mainfile(filepath='/tmp/rwork/asm.blend')
body=bpy.data.objects['SM_Rift']
bm=bmesh.new(); bm.from_mesh(body.data)
P=[v.co.copy() for e in bm.edges if len(e.link_faces)==1 for v in e.verts]
P=[p for p in P if p.x<-1.05]
trees=[]
for o in bpy.data.objects:
    if o.name.startswith('NEW_'):
        b=bmesh.new(); b.from_mesh(o.data); trees.append((o.name,BVHTree.FromBMesh(b)))
def inside(p):
    for n,t in trees:
        hits=0; o=p.copy(); d=Vector((0.0123,0.0071,1.0)).normalized()
        while True:
            loc,nor,idx,dist=t.ray_cast(o,d)
            if loc is None: break
            hits+=1; o=loc+d*1e-5
        if hits%2==1: return n
    return None
out=[]
for p in P:
    if not inside(p): out.append(p)
print('rear cut boundary verts',len(P),'outside new solids',len(out))
for p in out[:60]: print([round(x*100,1) for x in p])
