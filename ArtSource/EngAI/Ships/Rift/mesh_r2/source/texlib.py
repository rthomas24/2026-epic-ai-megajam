import numpy as np, cv2
def rd(p): 
    im=cv2.imread(p,cv2.IMREAD_UNCHANGED)
    if im.ndim==3 and im.shape[2]==4: im=im[...,:3]
    return cv2.cvtColor(im,cv2.COLOR_BGR2RGB)
def wr(p,rgb): cv2.imwrite(p,cv2.cvtColor(np.ascontiguousarray(rgb),cv2.COLOR_RGB2BGR),[cv2.IMWRITE_PNG_COMPRESSION,9])
def raster(tris,G,dil=0):
    m=np.zeros((G,G),np.uint8)
    for t in tris:
        pts=np.round(np.c_[t[:,0]*G,(1-t[:,1])*G]*8).astype(np.int32); cv2.fillPoly(m,[pts],1,shift=3)
        cv2.polylines(m,[pts],True,1,1,shift=3)
    if dil: m=cv2.dilate(m,np.ones((2*dil+1,2*dil+1),np.uint8))
    return m
def nearest_fill(img,mask):
    """fill pixels where mask==0 with the nearest masked pixel's value"""
    inv=(mask==0).astype(np.uint8)
    if inv.sum()==0: return img.copy()
    src=(mask>0).astype(np.uint8)
    d,lab=cv2.distanceTransformWithLabels(1-src,cv2.DIST_L2,5,labelType=cv2.DIST_LABEL_PIXEL)
    ys,xs=np.nonzero(src)
    # labels index zero-pixels of (1-src) i.e. src pixels, in scan order
    lut_y=np.zeros(lab.max()+1,np.int64); lut_x=np.zeros(lab.max()+1,np.int64)
    order=lab[ys,xs]; lut_y[order]=ys; lut_x[order]=xs
    out=img[lut_y[lab],lut_x[lab]]
    out[src>0]=img[src>0]
    return out
def clean_bc(bc,strength=1.0):
    f=bc.copy()
    for (d,sc,ss) in ((9,22,4),(7,14,3)):
        f=cv2.bilateralFilter(f,d,sc*strength,ss)
    # keep crisp panel lines: re-inject high-frequency luminance edges where strong
    lab0=cv2.cvtColor(bc,cv2.COLOR_RGB2LAB).astype(np.float32); lab1=cv2.cvtColor(f,cv2.COLOR_RGB2LAB).astype(np.float32)
    hp=lab0[...,0]-cv2.GaussianBlur(lab0[...,0],(0,0),1.2)
    edge=np.clip((np.abs(hp)-6)/10,0,1)
    lab1[...,0]+=hp*edge*0.8
    # flatten mottling on clean white ceramic & magenta paint: pull chroma toward local smooth chroma
    big=cv2.bilateralFilter(f,15,30*strength,8).astype(np.float32)
    lb=cv2.cvtColor(big.astype(np.uint8),cv2.COLOR_RGB2LAB).astype(np.float32)
    L=lab1[...,0]; C=np.hypot(lab1[...,1]-128,lab1[...,2]-128)
    white=np.clip((L-150)/40,0,1)*np.clip((18-C)/10,0,1)
    mag=np.clip((C-40)/20,0,1)*np.clip(((lab1[...,1]-128)-25)/15,0,1)
    w=np.maximum(white,mag)[...,None]*0.6
    lab1=lab1*(1-w)+lb*w
    lab1[...,0]+=hp*edge*0.5*w[...,0]
    out=cv2.cvtColor(np.clip(lab1,0,255).astype(np.uint8),cv2.COLOR_LAB2RGB)
    return out
def clean_orm(orm):
    o=orm.copy()
    o[...,0]=255
    g=cv2.medianBlur(orm[...,1],5); g=cv2.bilateralFilter(g,9,25,4)
    o[...,1]=g
    b=cv2.medianBlur(orm[...,2],5).astype(np.float32)/255
    b=np.clip((b-0.25)/0.5,0,1); b=b*b*(3-2*b)
    o[...,2]=(b*255).round().astype(np.uint8)
    return o
def clean_n_dx(n,low_keep=0.45,sig=3.0):
    v=n.astype(np.float32)/127.5-1
    xy=v[...,:2]
    lo=cv2.GaussianBlur(xy,(0,0),sig); hi=xy-lo
    xy2=hi*1.0+lo*low_keep
    # soften high-freq speckle (triangle soup) slightly but keep lines
    t3=np.dstack([xy2,np.zeros_like(xy2[...,0])]).astype(np.float32)
    xy2=cv2.bilateralFilter(t3,5,0.25,2)[...,:2]
    z=np.sqrt(np.clip(1-(xy2**2).sum(-1),0,1))
    out=np.dstack([xy2,z]); return np.clip((out+1)*127.5,0,255).round().astype(np.uint8)
def clean_e(e,hue_target,hue_tol=40,vmin=0.30,smin=0.45,boost=1.25):
    """keep only bright, saturated emissive of the squad hue; zero grey/muddy smudges"""
    hsv=cv2.cvtColor(e,cv2.COLOR_RGB2HSV_FULL).astype(np.float32)
    h=hsv[...,0]*360/255; s=hsv[...,1]/255; v=hsv[...,2]/255
    dh=np.abs(((h-hue_target)+180)%360-180)
    keep=np.clip((v-vmin)/0.12,0,1)*np.clip((s-smin)/0.15,0,1)*np.clip((hue_tol-dh)/12,0,1)
    keep=cv2.morphologyEx((keep>0.02).astype(np.uint8),cv2.MORPH_OPEN,np.ones((2,2),np.uint8)).astype(np.float32)*keep
    out=e.astype(np.float32)*keep[...,None]*boost
    # crisp: unsharp
    bl=cv2.GaussianBlur(out,(0,0),1.0); out=out+0.6*(out-bl)
    out[keep<0.02]=0
    return np.clip(out,0,255).round().astype(np.uint8)
def clean_e2(e,hue_target,hue_tol,vmin,smin,lilac_v=0.42,lilac_s=0.15,lilac_tol=55,sat_floor=0.62,boost=1.25):
    hsv=cv2.cvtColor(e,cv2.COLOR_RGB2HSV_FULL).astype(np.float32)
    h=hsv[...,0]*360/255; s=hsv[...,1]/255; v=hsv[...,2]/255
    dh=np.abs(((h-hue_target)+180)%360-180)
    kA=np.clip((v-vmin)/0.12,0,1)*np.clip((s-smin)/0.15,0,1)*np.clip((hue_tol-dh)/12,0,1)
    kB=np.clip((v-lilac_v)/0.1,0,1)*np.clip((s-lilac_s)/0.08,0,1)*np.clip((lilac_tol-dh)/12,0,1)
    keep=np.maximum(kA,kB)
    keep=cv2.morphologyEx((keep>0.02).astype(np.uint8),cv2.MORPH_OPEN,np.ones((2,2),np.uint8)).astype(np.float32)*keep
    s2=np.maximum(s,sat_floor); v2=np.clip(v*boost,0,1)
    hsv2=np.dstack([hsv[...,0],s2*255,v2*255]).astype(np.uint8)
    rgb=cv2.cvtColor(hsv2,cv2.COLOR_HSV2RGB_FULL).astype(np.float32)*keep[...,None]
    bl=cv2.GaussianBlur(rgb,(0,0),1.0); rgb=rgb+0.6*(rgb-bl)
    rgb[keep<0.02]=0
    return np.clip(rgb,0,255).round().astype(np.uint8)
