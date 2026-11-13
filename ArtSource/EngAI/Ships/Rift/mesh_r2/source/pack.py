import numpy as np, cv2, json
d=np.load('uvdump.npz'); B=d['body']; Nt=d['new']; isl=d['isl']; zone=d['zone']
G=1024
occ=np.zeros((G,G),np.uint8)
for t in B:
    pts=np.round(np.c_[t[:,0]*G, (1-t[:,1])*G]*8).astype(np.int32)
    cv2.fillPoly(occ,[pts],1,lineType=cv2.LINE_8,shift=3)
print('kept UV coverage %.1f%%'%(occ.mean()*100))
M=3  # margin px at 1024 (=6px at 2K)
occd=cv2.dilate(occ,np.ones((2*M+1,2*M+1),np.uint8))
print('free after margin %.1f%%'%((1-occd.mean())*100))
cv2.imwrite('/tmp/occ.png',(1-occd)*255)
ids=np.unique(isl)
boxes=[]
for i in ids:
    T=Nt[isl==i].reshape(-1,2); mn=T.min(0); mx=T.max(0); boxes.append((i,mn,mx))
print('islands',len(ids))
area_new=sum((mx-mn).prod() for _,mn,mx in boxes)
print('sum bbox area (smart-uv units)',area_new)
def try_pack(s):
    o=occd.copy(); place={}
    order=sorted(boxes,key=lambda b:-(b[2]-b[1]).max())
    for i,mn,mx in order:
        sz=(mx-mn)*s*G
        best=None
        for rot in (0,1):
            w=int(np.ceil(sz[rot]))+2*M; h=int(np.ceil(sz[1-rot]))+2*M
            w=max(w,1+2*M); h=max(h,1+2*M)
            if w>=G or h>=G: continue
            ii=cv2.integral(o)  # (G+1,G+1)
            S=ii[h:,w:]-ii[:-h,w:]-ii[h:,:-w]+ii[:-h,:-w]
            ys,xs=np.nonzero(S==0)
            if len(ys)==0: continue
            k=np.argmin(ys*G+xs)  # top-left first
            cand=(ys[k],xs[k],w,h,rot)
            best=cand; break
        if best is None: return None
        y,x,w,h,rot=best
        o[y:y+h,x:x+w]=1
        place[int(i)]=(int(x),int(y),int(w),int(h),rot,mn.tolist(),mx.tolist())
    return place
lo,hi=0.05,2.0; bestp=None
for it in range(9):
    s=(lo+hi)/2; p=try_pack(s)
    print('scale',round(s,4),'ok' if p else 'fail')
    if p: lo=s; bestp=(s,p)
    else: hi=s
s,p=bestp
json.dump({'scale':s,'G':G,'M':M,'place':p},open('pack.json','w'))
print('final scale',s)
