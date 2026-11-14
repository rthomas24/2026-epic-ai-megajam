import bpy, bmesh, math, sys
from mathutils import Vector, Matrix
WHITE,CHAR,GUN,MAG,EMI,DARK,EDIM=range(7)
ZN=['Z_White','Z_Charcoal','Z_Gunmetal','Z_Magenta','Z_Emissive','Z_DarkInterior','Z_EmissiveDim']
def new_bm(): return bmesh.new()
def revolve(bm, prof, zones, cy, cz, N=96, a0=0.0, a1=None, rmod=None, cap_zone=None):
    """prof: closed list of (x,r) cm; zones per edge; revolve about X axis through (cy,cz)."""
    full = a1 is None
    if full: angs=[a0+2*math.pi*i/N for i in range(N)]
    else: angs=[a0+(a1-a0)*i/N for i in range(N+1)]
    rings=[]
    for si,t in enumerate(angs):
        ring=[]
        for pi,(x,r) in enumerate(prof):
            rr = r + (rmod(si,pi) if rmod else 0)
            ring.append(bm.verts.new((x, cy+rr*math.cos(t), cz+rr*math.sin(t))))
        rings.append(ring)
    P=len(prof); faces=[]
    nseg = N if full else N
    for si in range(nseg):
        A=rings[si]; B=rings[(si+1)%len(rings)] if full else rings[si+1]
        for pi in range(P):
            q=[A[pi],A[(pi+1)%P],B[(pi+1)%P],B[pi]]
            try:
                f=bm.faces.new(q); f.material_index=zones[pi]; faces.append(f)
            except ValueError: pass
    if not full:
        for ring in (rings[0],rings[-1]):
            f=bm.faces.new(ring); f.material_index=cap_zone if cap_zone is not None else zones[0]
    return faces
def prism(bm, poly, axis, a, b, zone_side, zone_caps, scale_b=1.0, offset_b=(0,0)):
    """poly: list of 2D pts in the plane orthogonal to axis ('x','y','z'); extrude from a to b."""
    def P(u,v,w):
        if axis=='x': return (w,u,v)
        if axis=='y': return (u,w,v)
        return (u,v,w)
    cu=sum(p[0] for p in poly)/len(poly); cv=sum(p[1] for p in poly)/len(poly)
    A=[bm.verts.new(P(u,v,a)) for u,v in poly]
    B=[bm.verts.new(P(cu+(u-cu)*scale_b+offset_b[0], cv+(v-cv)*scale_b+offset_b[1], b)) for u,v in poly]
    n=len(poly); fs=[]
    for i in range(n):
        f=bm.faces.new([A[i],A[(i+1)%n],B[(i+1)%n],B[i]]); f.material_index=zone_side if isinstance(zone_side,int) else zone_side[i]; fs.append(f)
    fa=bm.faces.new(A[::-1]); fa.material_index=zone_caps
    fb=bm.faces.new(B); fb.material_index=zone_caps
    return fs,fa,fb
def chamf_rect(u0,u1,v0,v1,c, ctop=None):
    ct=c if ctop is None else ctop
    return [(u0+c,v0),(u1-c,v0),(u1,v0+c),(u1,v1-ct),(u1-ct,v1),(u0+ct,v1),(u0,v1-ct),(u0,v0+c)]
def finish(bm, name, coll, bevel_angle=None, bevel_off=0.0, segs=1):
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.001)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if bevel_angle:
        es=[e for e in bm.edges if e.is_manifold and len(e.link_faces)==2 and e.calc_face_angle(0)>math.radians(bevel_angle)]
        if es: bmesh.ops.bevel(bm, geom=es, offset=bevel_off, segments=segs, profile=0.5, affect='EDGES', clamp_overlap=True)
    me=bpy.data.meshes.new(name)
    for zn in ZN:
        mm=bpy.data.materials.get(zn) or bpy.data.materials.new(zn); me.materials.append(mm)
    bm.to_mesh(me); bm.free()
    o=bpy.data.objects.new(name, me); coll.objects.link(o); return o

def build(coll):
    objs=[]
    # ---------- PORTAL RING -------------------------------------------------
    RX,RY,RZ=-170.0,0.0,55.0
    prof=[(-8,57.9956),(8,57.9956),(11,55),(11,53.5),(10,53.5),(10,51.5),(11,51.5),(11,49),(9,47),(3,47),(2.2,49.6),(-2.2,49.6),(-3,47),(-9,47),(-11,49),(-11,51.5),(-10,51.5),(-10,53.5),(-11,53.5),(-11,55)]
    zones=[WHITE,WHITE,CHAR,EMI,EMI,EMI,CHAR,CHAR,CHAR,EMI,EMI,EMI,CHAR,CHAR,CHAR,EMI,EMI,EMI,CHAR,WHITE]
    bm=new_bm(); revolve(bm,[(RX+x,r) for x,r in prof],zones,RY,RZ,N=112)
    o=finish(bm,'NEW_PortalRing',coll); objs.append(o)
    # armour clamps on the collar
    bm=new_bm()
    cp=[(-9,56.5),(9,56.5),(9,59.2),(7.5,60.6),(-7.5,60.6),(-9,59.2)]
    cz=[MAG,MAG,MAG,MAG,MAG,MAG]
    for deg in (20,160,58,122,-22,-158):
        t=math.radians(deg); w=math.radians(6.5)
        revolve(bm,[(RX+x,r) for x,r in cp],cz,RY,RZ,N=6,a0=t-w,a1=t+w,cap_zone=CHAR)
    o=finish(bm,'NEW_RingClamps',coll,bevel_angle=40,bevel_off=0.6); objs.append(o)
    # yoke cradle under the ring
    bm=new_bm()
    yp=[(-15,55.5),(15,55.5),(15,62),(12,65.5),(-12,65.5),(-15,62)]
    revolve(bm,[(RX+x,r) for x,r in yp],[GUN,GUN,GUN,CHAR,GUN,GUN],RY,RZ,N=20,a0=math.radians(-132),a1=math.radians(-48),cap_zone=CHAR)
    o=finish(bm,'NEW_RingYoke',coll,bevel_angle=40,bevel_off=0.8); objs.append(o)
    # ---------- DORSAL SPINE FAIRING + DECK ---------------------------------
    bm=new_bm()
    sp=[(-202,-14),(-100,-14),(-100,32),(-129,32),(-150,10),(-156,4),(-190,4),(-202,-2)]
    # prism along Y: poly in (x,z)
    def Pxz(bm,poly,y0,y1,zs,zc):
        A=[bm.verts.new((u,y0,v)) for u,v in poly]; B=[bm.verts.new((u,y1,v)) for u,v in poly]
        n=len(poly)
        for i in range(n):
            f=bm.faces.new([A[i],A[(i+1)%n],B[(i+1)%n],B[i]]); f.material_index=zs
        bm.faces.new(A[::-1]).material_index=zc; bm.faces.new(B).material_index=zc
    Pxz(bm,sp,-24,24,WHITE,CHAR)
    for f in bm.faces:
        if abs(f.normal.y)>0.7 or f.calc_center_median().z<-8: f.material_index=CHAR
    o=finish(bm,'NEW_Spine',coll,bevel_angle=30,bevel_off=2.5,segs=2); objs.append(o)
    bm=new_bm()
    prism(bm,chamf_rect(-57,57,-26,2,4,ctop=7),'x',-105,-228,[CHAR,CHAR,CHAR,WHITE,WHITE,WHITE,CHAR,CHAR],CHAR,scale_b=0.97)
    o=finish(bm,'NEW_Deck',coll,bevel_angle=30,bevel_off=1.2,segs=2); objs.append(o)
    # ---------- REAR NACELLES / KEEL / STRAKES ------------------------------
    def nacelle(cy,cz,R0,R1,name):
        bm=new_bm()
        prof=[(-162,0.0),(-162,R0-2),(-160.5,R0-0.5),(-198,R0-0.5),(-201,R0-2.5),(-201,R0-4),(-204,R0-4),(-204,R1),(-229,R1),(-231,R1-1.5),(-233.5,R1-1.5),(-233.5,0.0)]
        zones=[CHAR,GUN,CHAR,GUN,CHAR,MAG,CHAR,WHITE,GUN,CHAR,CHAR,CHAR]
        revolve(bm,prof,zones,cy,cz,N=56)
        return finish(bm,name,coll)
    objs.append(nacelle(64,-34,34,31,'NEW_Nacelle_1'))
    objs.append(nacelle(-64,-34,34,31,'NEW_Nacelle_0'))
    objs.append(nacelle(0,-34,34,32,'NEW_Nacelle_C'))
    bm=new_bm()
    prism(bm,chamf_rect(-47,47,-95,-38,9,ctop=4),'x',-162,-226,[CHAR,GUN,CHAR,CHAR,CHAR,CHAR,CHAR,CHAR],CHAR,scale_b=0.82,offset_b=(0,4))
    o=finish(bm,'NEW_Keel',coll,bevel_angle=30,bevel_off=1.2,segs=2); objs.append(o)
    bm=new_bm()
    prism(bm,chamf_rect(-108,108,-76,-47,7),'x',-158,-210,[CHAR,CHAR,GUN,WHITE,WHITE,WHITE,GUN,CHAR],MAG,scale_b=0.88,offset_b=(0,3))
    o=finish(bm,'NEW_Strakes',coll,bevel_angle=30,bevel_off=1.0,segs=2); objs.append(o)
    # ---------- ENGINES -----------------------------------------------------
    def nozzle(cy,cz,R,lip_x,name,petals=True):
        bm=new_bm()
        Rl=R-3.5; Ri=R-6.5
        prof=[(-233,R),(-241,R),(-242.5,R-1.5),(lip_x+5,Rl+0.6),(lip_x,Rl),(lip_x,Rl-1.6),(lip_x+0.8,Rl-2.3),(-241,Ri),(-236.8,Ri),(-233,Ri)]
        zones=[CHAR,CHAR,GUN,GUN,GUN,GUN,DARK,DARK,EDIM,DARK]
        def rmod(si,pi):
            return (0.9 if (si%2==0 and pi in (3,4)) else 0.0) if petals else 0.0
        revolve(bm,prof,zones,cy,cz,N=48,rmod=rmod)
        # collar band on housing
        band=[(-238,R),(-236,R),(-236,R+1.6),(-238,R+1.6)]
        revolve(bm,band,[CHAR,MAG,MAG,MAG],cy,cz,N=48)
        # back turbine plate (emissive glow behind blades)
        plate=[(-236.0,0.0),(-236.0,Ri+0.5),(-235.0,Ri+0.5),(-235.0,0.0)]
        revolve(bm,plate,[DARK,DARK,DARK,DARK],cy,cz,N=32)
        # emissive core cone
        cone=[(-236.5,0.0),(-236.5,Ri*0.62),(-237.6,Ri*0.62),(-243.5,2.0),(-244.0,0.0)]
        revolve(bm,cone,[EMI,EMI,EMI,EMI,EMI],cy,cz,N=32)
        # turbine blades
        nb=12
        for k in range(nb):
            t=2*math.pi*k/nb
            bl=new_bm()
            vs=[]
            for (x,yy,zz) in [(-236.6,Ri*0.55,-0.5),(-236.6,Ri+0.3,-0.5),(-236.6,Ri+0.3,0.5),(-236.6,Ri*0.55,0.5),(-239.0,Ri*0.55,-0.5),(-239.0,Ri+0.3,-0.5),(-239.0,Ri+0.3,0.5),(-239.0,Ri*0.55,0.5)]:
                vs.append((x,yy,zz))
            # twist: rotate about blade radial axis by 28deg, then around X by t
            M=Matrix.Rotation(t,3,'X')@Matrix.Rotation(math.radians(28),3,'Y')
            vv=[]
            for (x,yy,zz) in vs:
                p=Vector((x+237.8,yy,zz)); p=Matrix.Rotation(math.radians(30),3,'Y')@Vector((p.x,0,p.z))+Vector((0,p.y,0))
                p=Vector((p.x-237.8,p.y,p.z)); p=Matrix.Rotation(t,3,'X')@p
                vv.append(bm.verts.new((p.x,cy+p.y,cz+p.z)))
            for q in [(0,1,2,3),(7,6,5,4),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)]:
                f=bm.faces.new([vv[i] for i in q]); f.material_index=GUN
        return finish(bm,name,coll)
    objs.append(nozzle(64,-34,28,-250,'NEW_Engine_1'))
    objs.append(nozzle(-64,-34,28,-250,'NEW_Engine_0'))
    # central closed turbine intake nacelle
    bm=new_bm(); R=30
    prof=[(-233,R),(-243,R),(-245,R-2),(-246,R-3.5),(-246,R-5),(-244.5,R-6.5),(-238,R-7),(-232,R-7)]
    revolve(bm,prof,[CHAR,GUN,GUN,GUN,GUN,DARK,DARK,DARK],0,-34,N=56)
    band=[(-240,R),(-237,R),(-237,R+1.6),(-240,R+1.6)]
    revolve(bm,band,[CHAR,MAG,MAG,MAG],0,-34,N=56)
    plate=[(-232.5,0.0),(-232.5,R-6.5),(-231.5,R-6.5),(-231.5,0.0)]
    revolve(bm,plate,[DARK,DARK,DARK,DARK],0,-34,N=40)
    cone=[(-233.0,0.0),(-233.0,15.5),(-234.5,15.5),(-241.5,5.5),(-242.6,0.0)]
    revolve(bm,cone,[EMI,EMI,EMI,EMI,EMI],0,-34,N=40)
    ring2=[(-233,15.5),(-233,R-6.8),(-234.2,R-6.8),(-234.2,15.5)]
    revolve(bm,ring2,[EDIM,EDIM,EDIM,EDIM],0,-34,N=40)
    nv=10
    for k in range(nv):
        t=2*math.pi*k/nv
        vs=[(-233.2,15.0,-0.7),(-233.2,R-6.6,-0.7),(-233.2,R-6.6,0.7),(-233.2,15.0,0.7),(-238.5,13.0,-0.7),(-238.5,R-6.6,-0.7),(-238.5,R-6.6,0.7),(-238.5,13.0,0.7)]
        vv=[]
        for (x,yy,zz) in vs:
            p=Matrix.Rotation(t,3,'X')@Vector((x,yy,zz)); vv.append(bm.verts.new((p.x,p.y,-34+p.z)))
        for q in [(0,1,2,3),(7,6,5,4),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)]:
            bm.faces.new([vv[i] for i in q]).material_index=GUN
    objs.append(finish(bm,'NEW_Engine',coll))
    for o in objs:
        o.scale=(0.01,0.01,0.01)
    return objs
