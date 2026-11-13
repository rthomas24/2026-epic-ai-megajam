import numpy as np, cv2, json
r=np.load('r1raw.npz'); f=np.load('final_uv.npz')
kept=f['zone']<0
src=f['src'][kept]; ft=f['tri'][kept]; fp=f['pos'][kept]
miss=0; maxd=0.0; maxp=0.0
for sidx,t,p in zip(src,ft,fp):
    t0=r['tri'][sidx]; p0=r['pos'][sidx]
    # loops keep their order through the weld; check rotation alignment
    best=min(range(3),key=lambda k: np.abs(np.roll(p0,-k,0)-p).sum())
    t0=np.roll(t0,-best,0); p0=np.roll(p0,-best,0)
    maxd=max(maxd,float(np.abs(t0-t).max())); maxp=max(maxp,float(np.abs(p0-p).max()))
miss=len(src)-len(set(src.tolist()))
print(f'kept faces {kept.sum()}, duplicate src ids {miss}, max UV delta vs r1 {maxd:.3e}, max position delta {maxp*100:.4f} cm (weld)')
res={}
for G in (2048,4096):
    A=np.zeros((G,G),np.uint8); Bm=np.zeros((G,G),np.uint8)
    def fill(img,T):
        for t in T:
            pts=np.round(np.c_[t[:,0]*G,(1-t[:,1])*G]*8).astype(np.int32); cv2.fillPoly(img,[pts],1,lineType=cv2.LINE_8,shift=3)
            cv2.polylines(img,[pts],True,1,1,lineType=cv2.LINE_8,shift=3)
    fill(A,f['tri'][kept]); fill(Bm,f['tri'][~kept])
    ov=int((A&Bm).sum()); res[G]=ov
    # min gap
    dist=cv2.distanceTransform((1-A).astype(np.uint8),cv2.DIST_L2,5)
    gap=dist[Bm>0].min()
    print(f'{G}: kept px {A.sum()}, new px {Bm.sum()}, overlap px {ov}, min gap new->kept {gap:.1f}px, new out of 0-1: {int(((f["tri"][~kept]<0)|(f["tri"][~kept]>1)).sum())}')
# heatmask: fraction of heatmask non-zero pixels that fall on new UV
hm=cv2.imread('/tmp/rift-r2-repo/ArtSource/EngAI/Ships/Rift/heatmask_r1/T_Rift_HeatMask.png',cv2.IMREAD_UNCHANGED)
print('heatmask', hm.shape, hm.dtype)
G=hm.shape[0]; Bm=np.zeros((G,G),np.uint8)
for t in f['tri'][~kept]:
    pts=np.round(np.c_[t[:,0]*G,(1-t[:,1])*G]*8).astype(np.int32); cv2.fillPoly(Bm,[pts],1,shift=3)
h=hm if hm.ndim==2 else hm[...,:3].max(2)
print('heatmask lit px under new-part UVs:', int(((h>8)&(Bm>0)).sum()), 'of', int((h>8).sum()))
# texel density
def dens(T,Pp):
    e1=T[:,1]-T[:,0]; e2=T[:,2]-T[:,0]; a2=0.5*np.abs(e1[:,0]*e2[:,1]-e1[:,1]*e2[:,0]); a3=0.5*np.linalg.norm(np.cross(Pp[:,1]-Pp[:,0],Pp[:,2]-Pp[:,0]),axis=1)
    return np.sqrt(a2.sum()/a3.sum())*2048/100  # px per cm
print('texel density px/cm: kept %.2f new %.2f'%(dens(f['tri'][kept],f['pos'][kept]),dens(f['tri'][~kept],f['pos'][~kept])))
