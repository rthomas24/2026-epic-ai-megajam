import sys; sys.path.insert(0,'/tmp/rwork'); from texlib import *
import numpy as np, os
D='/tmp/rift-r2-repo/ArtSource/EngAI/Ships/Rift/mesh_r1/'; O='/tmp/rwork/tex/rift/'; os.makedirs(O,exist_ok=True)
G=2048
f=np.load('/tmp/rwork/final_uv.npz'); kept=f['zone']<0
K=raster(f['tri'][kept],G)
Kp=cv2.dilate(K,np.ones((9,9),np.uint8))  # kept + 4px gutter
ZP={0:((200,199,205),(255,107,0),(0,0,0)),1:((40,40,45),(255,102,204),(0,0,0)),2:((88,88,96),(255,77,255),(0,0,0)),
    3:((203,8,131),(255,90,26),(0,0,0)),4:((240,110,230),(255,77,0),(255,70,235)),5:((16,16,19),(255,140,153),(0,0,0)),6:((110,20,95),(255,110,0),(150,30,140))}
Zm=np.full((G,G),-1,np.int16)
for z in range(7):
    m=raster(f['tri'][f['zone']==z],G); Zm[m>0]=z
NW=(Zm>=0).astype(np.uint8)
bc=rd(D+'T_Rift_BC.png'); n=rd(D+'T_Rift_N.png'); orm=rd(D+'T_Rift_ORM.png'); e=rd(D+'T_Rift_E.png')
# pre-pad so filters near island borders do not pull in junk
bcp=nearest_fill(bc,K); np_=nearest_fill(n,K); op=nearest_fill(orm,K)
bc2=clean_bc(bcp); n2=clean_n_dx(np_,low_keep=0.45,sig=3.0); o2=clean_orm(op); e2=clean_e2(e,300,52,0.30,0.30)
e2[K==0]=0
def comp(clean,zi):
    out=clean.copy()
    for z,(b,o,em) in ZP.items():
        out[Zm==z]=(b,o,em)[zi]
    return out
used=np.maximum(K,NW)
BC=comp(bc2,0); ORM=comp(o2,1); E=comp(e2,2)
ao8=cv2.medianBlur(cv2.imread('/tmp/rwork/tex/ao_bake.png',cv2.IMREAD_GRAYSCALE),5)
ao=cv2.bilateralFilter(ao8.astype(np.float32)/255,9,0.15,3)
ao=np.clip(ao,0,1)**1.3
rng=np.random.default_rng(7)
noise=cv2.GaussianBlur(rng.standard_normal((G,G)).astype(np.float32),(0,0),2.5); noise/=noise.std()
fine=cv2.GaussianBlur(rng.standard_normal((G,G)).astype(np.float32),(0,0),0.8); fine/=fine.std()
m=NW>0
aof=(0.35+0.65*ao)
bcf=BC.astype(np.float32)
bcf[m]=bcf[m]*aof[m][:,None]*(1+0.025*noise[m][:,None]+0.01*fine[m][:,None])
emis=(Zm==4)|(Zm==6)
bcf[emis]=BC[emis]
BC=np.clip(bcf,0,255).round().astype(np.uint8)
g=ORM[...,1].astype(np.float32); g[m]+= 7*noise[m]+3*fine[m]+ (1-ao[m])*25
ORM[...,1]=np.clip(g,0,255).round().astype(np.uint8)
N=n2.copy(); N[NW>0]=(128,128,255)
# gutter / padding: nearest fill everything outside used UVs (E only within 8px gutter)
BC=nearest_fill(BC,used); ORM=nearest_fill(ORM,used); N=nearest_fill(N,used)
Ef=nearest_fill(E,used); g=cv2.dilate(used,np.ones((17,17),np.uint8)); E=np.where(g[...,None]>0,Ef,0).astype(np.uint8)
ORM[...,0]=255
for k,im in (('BC',BC),('N',N),('ORM',ORM),('E',E)): wr(O+f'T_Rift_{k}.png',im)
# GL normal for the GLB
NG=N.copy(); NG[...,1]=255-NG[...,1]; wr('/tmp/rwork/tex/T_Rift_N_GL.png',NG)
# heat mask compat: zero spill under new parts
hm=cv2.imread('/tmp/rift-r2-repo/ArtSource/EngAI/Ships/Rift/heatmask_r1/T_Rift_HeatMask.png',cv2.IMREAD_UNCHANGED)
hm2=hm.copy(); newonly=(cv2.dilate(NW,np.ones((9,9),np.uint8))>0)&(K==0); hm2[newonly]=0
cv2.imwrite(O+'T_Rift_HeatMask_r2.png',hm2,[cv2.IMWRITE_PNG_COMPRESSION,9])
print('heat: kept-pixel identical:',bool((hm2[K>0]==hm[K>0]).all()),'zeroed px',int(newonly.sum()))
# stats
def stat(a,b,m): return float(np.abs(a.astype(int)-b.astype(int))[m>0].mean())
print('mean |delta| on kept: BC %.2f N %.2f ORM %.2f'%(stat(bc,BC,K),stat(n,N,K),stat(orm,ORM,K)))
grey=lambda x: int(((x.max(2)>8)&((x.max(2)-x.min(2))<0.3*x.max(2).clip(1))).sum())
print('E grey/muddy px before',grey(e),'after',grey(E),'E lit px before',int((e.max(2)>10).sum()),'after',int((E.max(2)>10).sum()))
# local noise metric on kept white/magenta (std of luminance in 7px window)
def mottle(x,m):
    L=cv2.cvtColor(x,cv2.COLOR_RGB2LAB)[...,0].astype(np.float32)
    mu=cv2.blur(L,(7,7)); sd=np.sqrt(np.clip(cv2.blur(L*L,(7,7))-mu*mu,0,None)); return float(np.median(sd[m>0]))
hsv=cv2.cvtColor(bc,cv2.COLOR_RGB2HSV_FULL)
paint=((hsv[...,1]<40)&(hsv[...,2]>140)|(hsv[...,1]>150)&(hsv[...,2]>150)).astype(np.uint8)&K
print('BC mottle (median local L std on white/magenta paint) before %.2f after %.2f'%(mottle(bc,paint),mottle(BC,paint)))
print('ORM rough noise before %.2f after %.2f'%(float(np.median(np.abs(orm[...,1].astype(int)-cv2.blur(orm[...,1],(7,7)).astype(int))[K>0])),float(np.median(np.abs(ORM[...,1].astype(int)-cv2.blur(ORM[...,1],(7,7)).astype(int))[K>0]))))
