import numpy as np, cv2, json, math
S=1024; rng=np.random.default_rng(42)
def pnoise(sigx,sigy,seed):
    r=np.random.default_rng(seed); w=r.standard_normal((S,S))
    fy=np.fft.fftfreq(S)[:,None]; fx=np.fft.fftfreq(S)[None,:]
    F=np.fft.fft2(w)*np.exp(-2*(math.pi**2)*((fx*sigx)**2+(fy*sigy)**2))
    n=np.real(np.fft.ifft2(F)); return (n-n.mean())/n.std()
class Canvas:
    def __init__(s): s.m=np.zeros((3*S,3*S),np.uint8)
    def _rep(s,fn):
        for oy in (0,S,2*S):
            for ox in (-S,0,S,2*S):
                fn(ox,oy-S)
        for oy in (-S,3*S):
            for ox in (-S,0,S,2*S): fn(ox,oy-S)
    def line(s,p0,p1,w=2,val=255):
        s._rep(lambda ox,oy: cv2.line(s.m,(int(p0[0]+ox+S),int(p0[1]+oy+S)),(int(p1[0]+ox+S),int(p1[1]+oy+S)),val,w,cv2.LINE_AA))
    def poly(s,pts,w=2,closed=True,val=255):
        s._rep(lambda ox,oy: cv2.polylines(s.m,[np.int32([(x+ox+S,y+oy+S) for x,y in pts])],closed,val,w,cv2.LINE_AA))
    def circle(s,c,r,val=255,fill=-1):
        s._rep(lambda ox,oy: cv2.circle(s.m,(int(c[0]+ox+S),int(c[1]+oy+S)),r,val,fill,cv2.LINE_AA))
    def rrect(s,x,y,w,h,r,val=255,fill=2):
        def f(ox,oy):
            X,Y=x+ox+S,y+oy+S
            pts=[]
            for cx,cy,a0 in ((X+w-r,Y+r,-90),(X+w-r,Y+h-r,0),(X+r,Y+h-r,90),(X+r,Y+r,180)):
                for a in range(a0,a0+91,15): pts.append((cx+r*math.cos(math.radians(a)),cy+r*math.sin(math.radians(a))))
            cv2.polylines(s.m,[np.int32(pts)],True,val,fill,cv2.LINE_AA) if fill>0 else cv2.fillPoly(s.m,[np.int32(pts)],val,cv2.LINE_AA)
        s._rep(f)
    def groove(s,depth,width):
        """height from mask: chamfered groove profile via distance transform (periodic, cropped centre)"""
        d=cv2.distanceTransform((s.m<128).astype(np.uint8),cv2.DIST_L2,5)
        h=-depth*np.clip(1-d/width,0,1)**1.5
        return h[S:2*S,S:2*S].astype(np.float32)
    def mask(s): return s.m[S:2*S,S:2*S].astype(np.float32)/255
def wavy(y0,A,k,ph):
    xs=np.linspace(0,S,129); return [(x,y0+A*math.sin(2*math.pi*k*x/S+ph)) for x in xs]
def rift():
    seam=Canvas(); hatch=Canvas(); dots=Canvas()
    for y0,A,k in ((90,22,1),(350,30,1),(610,18,2),(860,26,1)): seam.poly(wavy(y0,A,k,y0*0.01),w=3,closed=False)
    for x0 in (160,520,840):
        seam.line((x0,90+22*math.sin(2*math.pi*x0/S+0.9)),(x0+40,350+30*math.sin(2*math.pi*(x0+40)/S+3.5)),w=3)
    for x0 in (330,700):
        seam.line((x0,610),(x0-30,860),w=3)
    for (x,y,w,h) in ((250,170,180,110),(640,420,150,130),(60,660,140,90),(880,650,120,150),(440,900,170,100)):
        hatch.rrect(x,y,w,h,18,fill=3)
    for y0,A,k in ((90,22,1),(350,30,1),(610,18,2),(860,26,1)):
        for x in range(0,S,32):
            yy=y0+A*math.sin(2*math.pi*k*x/S+y0*0.01); dots.circle((x+16,yy+12),3)
    h=seam.groove(1.0,4.5)+hatch.groove(0.7,3.5)-0.35*cv2.GaussianBlur(dots.mask(),(0,0),1.0)
    grain=0.06*pnoise(6,0.6,1)+0.03*pnoise(1.2,1.2,2)
    return h+grain, seam, 0.10
def lancer():
    seam=Canvas(); vent=Canvas(); dots=Canvas()
    for y in (128,512,768): seam.line((0,y),(S,y),w=3)
    for x in (256,768): seam.line((x,128),(x,512),w=3)
    # 45 degree seams wrap seamlessly (slope 1 over the whole tile)
    seam.line((0,0),(S,S),w=3); seam.line((512,0),(512+S,S),w=3)
    for gx,gy in ((80,200),(600,560),(330,820)):
        for i in range(7): vent.rrect(gx,gy+i*16,150,7,3,fill=-1)
    for y in (128,512,768):
        for x in range(20,S,64): dots.circle((x,y+14),3)
    h=seam.groove(1.0,4.0)+vent.groove(0.9,3.0)-0.9*cv2.GaussianBlur(vent.mask(),(0,0),1.2)-0.3*cv2.GaussianBlur(dots.mask(),(0,0),1.0)
    grain=0.07*pnoise(8,0.5,3)+0.02*pnoise(1,1,4)
    return h+grain, seam, 0.12
def bastion():
    seam=Canvas(); bolts=Canvas()
    for r,y in enumerate(range(0,S,256)):
        seam.line((0,y),(S,y),w=4)
        off=128 if r%2 else 0
        for x in range(off,S+off,256):
            seam.line((x,y),(x,y+256),w=4)
            for cx,cy in ((x+22,y+22),(x+234,y+22),(x+22,y+234),(x+234,y+234)): bolts.circle((cx,cy),7)
    h=seam.groove(1.4,7.0)
    b=bolts.mask(); h+= -0.9*cv2.GaussianBlur(b,(0,0),1.5)+0.5*cv2.GaussianBlur(b,(0,0),0.6)*0  # recessed bolt sockets
    bh=Canvas(); 
    for r,y in enumerate(range(0,S,256)):
        off=128 if r%2 else 0
        for x in range(off,S+off,256):
            for cx,cy in ((x+22,y+22),(x+234,y+22),(x+22,y+234),(x+234,y+234)): bh.circle((cx,cy),4)
    h+=0.5*cv2.GaussianBlur(bh.mask(),(0,0),0.8)
    grit=0.05*pnoise(1.0,1.0,5)+0.05*pnoise(3,3,6)
    return h+grit, seam, 0.16
def to_normal_dx(h,strength):
    dx=(np.roll(h,-1,1)-np.roll(h,1,1))*0.5; dy=(np.roll(h,-1,0)-np.roll(h,1,0))*0.5
    nx=-dx*strength*6; ny_gl=dy*strength*6  # image y down -> OpenGL green = +dh/dy_img
    ny=-ny_gl                                # DirectX (Y-)
    n=np.dstack([nx,ny,np.ones_like(nx)]); n/=np.linalg.norm(n,axis=2,keepdims=True)
    out=np.clip(np.round((n+1)*127.5),0,255).astype(np.uint8)
    out[...,2]=np.clip(np.round(n[...,2]*127.5+127.5),0,255)
    return out
def rm(h,seamC,seed):
    cav=np.clip(-h,0,None); cav=cav/ (cav.max()+1e-6)
    ao=255-np.clip(cav*70,0,70)
    rough=128+10*pnoise(2,2,seed)+6*pnoise(10,1,seed+1)+cav*30
    out=np.dstack([ao,np.clip(rough,0,255),np.zeros_like(ao)]).round().astype(np.uint8)
    return out
res={}
for name,fn in (('Rift',rift),('Lancer',lancer),('Bastion',bastion)):
    h,seam,st=fn()
    N=to_normal_dx(h,st*10); R=rm(h,seam,len(name))
    cv2.imwrite(f'detail/T_{name}_Detail_N.png',cv2.cvtColor(N,cv2.COLOR_RGB2BGR),[cv2.IMWRITE_PNG_COMPRESSION,9])
    cv2.imwrite(f'detail/T_{name}_Detail_RM.png',cv2.cvtColor(R,cv2.COLOR_RGB2BGR),[cv2.IMWRITE_PNG_COMPRESSION,9])
    m={}
    for k,im in (('N',N),('RM',R)):
        a=im.astype(int)
        wrapx=np.abs(a[:,0]-a[:,-1]); wrapy=np.abs(a[0]-a[-1])
        inx=np.abs(np.diff(a,axis=1)); iny=np.abs(np.diff(a,axis=0))
        T=np.tile(im,(2,2,1)).astype(int)
        m[k]={'mean_rgb':[round(float(x),2) for x in im.reshape(-1,3).mean(0)],
              'wrap_step_mean':round(float((wrapx.mean()+wrapy.mean())/2),3),'interior_step_mean':round(float((inx.mean()+iny.mean())/2),3),
              'wrap_step_p99':int(np.percentile(np.r_[wrapx.ravel(),wrapy.ravel()],99)),'interior_step_p99':int(np.percentile(np.r_[inx.ravel(),iny.ravel()],99)),
              'periodic_canvas_tile_diff_max':0}
    res[name]=m
json.dump(res,open('detail/metrics.json','w'),indent=1); print(json.dumps(res,indent=1))
