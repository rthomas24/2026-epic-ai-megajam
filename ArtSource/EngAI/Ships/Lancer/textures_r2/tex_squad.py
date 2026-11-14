import sys,json; sys.path.insert(0,'/tmp/rwork'); from texlib import *
import numpy as np
res={}
for s,hue,tol in (('Lancer',192,35),('Bastion',28,40)):
    D=f'/tmp/rift-r2-repo/ArtSource/EngAI/Ships/{s}/mesh_r1/'; O=f'/tmp/rwork/tex/{s}/'
    K=raster(np.load(f'uv_{s}.npy'),2048)
    bc=rd(D+f'T_{s}_BC.png'); n=rd(D+f'T_{s}_N.png'); orm=rd(D+f'T_{s}_ORM.png'); e=rd(D+f'T_{s}_E.png')
    BC=clean_bc(nearest_fill(bc,K)); N=clean_n_dx(nearest_fill(n,K),0.5,3.0); ORM=clean_orm(nearest_fill(orm,K))
    E=clean_e2(e,hue,tol,0.24 if s=='Bastion' else 0.28,0.16 if s=='Bastion' else 0.35,lilac_v=2.0,sat_floor=0.7,boost=1.35); E[K==0]=0
    Kg=cv2.dilate(K,np.ones((9,9),np.uint8))
    out={}
    for k,a,b in (('BC',bc,BC),('N',n,N),('ORM',orm,ORM)):
        c=np.where(Kg[...,None]>0,b,a)  # keep r1 pixels outside the islands+gutter
        wr(O+f'T_{s}_{k}.png',c); out[k]=c
    wr(O+f'T_{s}_E.png',E)
    grey=lambda x: int(((x.max(2)>8)&((x.max(2)-x.min(2))<0.3*x.max(2).clip(1))).sum())
    dim=lambda x: int(((x.max(2)>8)&(x.max(2)<70)).sum())
    def mottle(x,m):
        L=cv2.cvtColor(x,cv2.COLOR_RGB2LAB)[...,0].astype(np.float32); mu=cv2.blur(L,(7,7)); sd=np.sqrt(np.clip(cv2.blur(L*L,(7,7))-mu*mu,0,None)); return round(float(np.median(sd[m>0])),2)
    res[s]={'E_grey_px':[grey(e),grey(E)],'E_dim_muddy_px(<70)':[dim(e),dim(E)],'E_lit_px':[int((e.max(2)>10).sum()),int((E.max(2)>10).sum())],
            'E_mean_lit_value':[round(float(e.max(2)[e.max(2)>10].mean()),1),round(float(E.max(2)[E.max(2)>10].mean()),1)],
            'BC_mottle':[mottle(bc,K),mottle(out['BC'],K)]}
print(json.dumps(res,indent=1)); json.dump(res,open('/tmp/rwork/tex/squad_metrics.json','w'),indent=1)
