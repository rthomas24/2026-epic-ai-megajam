import sys, numpy as np
from PIL import Image
def s2l(a): return np.where(a<=0.04045,a/12.92,((a+0.055)/1.055)**2.4)
def l2s(l): return np.where(l<=0.0031308,l*12.92,1.055*np.clip(l,0,None)**(1/2.4)-0.055)
def pushpull(img, mask):
    # img HxWxC float, mask HxW bool valid
    levels=[(img*mask[...,None], mask.astype(np.float32))]
    while levels[-1][1].shape[0]>1:
        c,w=levels[-1]; H,W=w.shape
        c2=c.reshape(H//2,2,W//2,2,-1).sum((1,3)); w2=w.reshape(H//2,2,W//2,2).sum((1,3))
        levels.append((c2,w2))
    # resolve from coarse
    c,w=levels[-1]; cur=c/np.maximum(w,1e-6)[...,None]
    for c,w in reversed(levels[:-1]):
        up=np.repeat(np.repeat(cur,2,0),2,1)
        avg=c/np.maximum(w,1e-6)[...,None]
        t=np.clip(w,0,1)[...,None]
        cur=avg*t+up*(1-t)
    return np.where(mask[...,None], img, cur)
bc_in, e_in, out, tgt = sys.argv[1], sys.argv[2], sys.argv[3], float(sys.argv[4])
a=np.asarray(Image.open(bc_in).convert('RGB')).astype(np.float32)/255
valid=a.sum(-1)>0
e=np.asarray(Image.open(e_in).convert('L')).astype(np.float32)/255
emask=np.clip(e*6,0,1)  # protect glow texels
lin=s2l(a); lum=0.2126*lin[...,0]+0.7152*lin[...,1]+0.0722*lin[...,2]
med=np.median(lum[valid]); t=((tgt/255+0.055)/1.055)**2.4; k=min(4.0,max(1.0,t/max(med,1e-4)))
lin2=lin*k; lin2=lin2/(1+np.maximum(lin2-0.5,0)*1.5); lin2=np.clip(lin2,0,0.82)
lin2=lin2*(1-emask[...,None])+lin*emask[...,None]
lin2=pushpull(lin2, valid)
sr=l2s(lin2)
Image.fromarray((np.clip(sr,0,1)*255+0.5).astype(np.uint8)).save(out)
L=lambda x:0.2126*x[...,0]+0.7152*x[...,1]+0.0722*x[...,2]
print(out,'gain %.2f'%k,'covered median before %.1f after %.1f; whole-image median after %.1f'%(np.median(L(a*255)[valid]),np.median(L(sr*255)[valid]),np.median(L(sr*255))))
