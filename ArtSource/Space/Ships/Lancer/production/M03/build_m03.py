"""Reference-led Lancer M03, original geometry with the repository MIT kit."""
from pathlib import Path
import hashlib,json,math,runpy,time
import bpy,bmesh
from mathutils import Vector,Matrix
P=Path(__file__).resolve().parent;ROOT=P.parents[1];REPO=ROOT.parents[3];OUT=ROOT/'review/M03/Review01'
HS=runpy.run_path(str(REPO/'.claude/skills/blender-game-asset/scripts/hardsurface.py'))
MATS=[];GROUPS={}
PARTS={'Hull':(0,0,0),'Wing_L':(0,-70,0),'Wing_R':(0,70,0),'Canopy':(-112,0,63),
       'Primary_L':(30,-100,-42),'Primary_R':(30,100,-42),'Rotor_L':(65,-100,-57),'Rotor_R':(65,100,-57),
       'Secondary_L':(-90,-220,-44),'Secondary_R':(-90,220,-44),'Engine_L':(-402,-111,-3),'Engine_R':(-402,111,-3)}
HULL=[(-385,42,21,0),(-300,62,29,1),(-180,74,33,3),(-60,76,34,5),(60,66,30,7),
      (160,53,26,6),(280,36,21,3),(370,24,17,0),(400,18,12,0)]
WING=[(70,100,-260,54,-4),(145,70,-250,40,-8),(270,-70,-245,25,-15),(360,-155,-228,12,-19)]
AIR=[(0,0),(.06,.34),(.2,.5),(.8,.5),(.96,.24),(1,0),(.96,-.24),(.75,-.44),(.2,-.44),(.05,-.27)]

def write(name,data,folder=OUT):
    folder.mkdir(parents=True,exist_ok=True);(folder/name).write_text(json.dumps(data,indent=2)+'\n')
def co(v):x,y,z=v;return Vector((y/100,-x/100,z/100))
def rgb(h):return HS['hex_color'](h)

def materials():
    specs=[('Graphite','#303A44',0,.46,.08,0),('Gunmetal','#7B8792',1,.46,0,0),
           ('Titanium','#B2BDC8',1,.35,0,0),('Composite','#1B242D',0,.75,0,0),
           ('Canopy','#123448',0,.19,.45,0),('Blue','#12C8FF',0,.33,0,2.1)]
    params=[]
    for role,h,metal,rough,coat,em in specs:
        mat=bpy.data.materials.new('M_LancerM03_'+role);mat.use_nodes=True;mat.diffuse_color=(*rgb(h),1)
        ns=mat.node_tree.nodes;lk=mat.node_tree.links;bs=next(n for n in ns if n.type=='BSDF_PRINCIPLED')
        bs.inputs['Metallic'].default_value=metal;bs.inputs['Coat Weight'].default_value=coat;bs.inputs['Coat Roughness'].default_value=.26
        attr=ns.new('ShaderNodeVertexColor');attr.layer_name='PanelTint'
        mix=ns.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1;mix.inputs[1].default_value=(*rgb(h),1)
        lk.new(attr.outputs['Color'],mix.inputs[2]);lk.new(mix.outputs[0],bs.inputs['Base Color'])
        roughmul=ns.new('ShaderNodeMath');roughmul.operation='MULTIPLY_ADD';roughmul.inputs[1].default_value=.20;roughmul.inputs[2].default_value=rough-.10
        lk.new(attr.outputs['Alpha'],roughmul.inputs[0])
        if role in ['Graphite','Gunmetal','Titanium']:
            tex=ns.new('ShaderNodeTexCoord');mapping=ns.new('ShaderNodeVectorMath');mapping.operation='MULTIPLY';mapping.inputs[1].default_value=(20,500,500)
            lk.new(tex.outputs['Object'],mapping.inputs[0]);noise=ns.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=1;noise.inputs['Detail'].default_value=2;noise.inputs['Roughness'].default_value=.65
            lk.new(mapping.outputs[0],noise.inputs['Vector'])
            grain=ns.new('ShaderNodeMath');grain.operation='MULTIPLY_ADD';grain.inputs[1].default_value=.065;grain.inputs[2].default_value=-.0325;lk.new(noise.outputs['Fac'],grain.inputs[0])
            add=ns.new('ShaderNodeMath');add.operation='ADD';lk.new(roughmul.outputs[0],add.inputs[0]);lk.new(grain.outputs[0],add.inputs[1]);lk.new(add.outputs[0],bs.inputs['Roughness'])
            bump=ns.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.12;bump.inputs['Distance'].default_value=.00018;lk.new(noise.outputs['Fac'],bump.inputs['Height']);lk.new(bump.outputs['Normal'],bs.inputs['Normal'])
        else:lk.new(roughmul.outputs[0],bs.inputs['Roughness'])
        if em:bs.inputs['Emission Color'].default_value=(*rgb(h),1);bs.inputs['Emission Strength'].default_value=em
        MATS.append(mat);params.append(dict(name=mat.name,base_color_srgb=h,metallic=metal,roughness=rough,coat_weight=coat,emissive_strength=em,
            vertex_color='PanelTint',roughness_rule='Roughness + 0.2*(VertexColor.A-0.5) + microfinish(plus/minus0.0325)',
            microfinish=role in ['Graphite','Gunmetal','Titanium'],unreal_handoff='Reconstruct exposed parameters and bake/replicate procedural microfinish before native adoption',textures_required_for_this_blend=0))
    write('material_parameters.json',params,P)

def finish(ob,g,mat=0,tint=.9,bevel=.3,rough=.5,wear=False):
    ob.data.materials.append(MATS[mat]);a=ob.data.color_attributes.new(name='PanelTint',type='FLOAT_COLOR',domain='CORNER')
    for d in a.data:d.color=(tint,tint,tint,rough)
    HS['smooth_by_angle'](ob,angle_deg=38)
    if bevel:
        HS['add_bevel'](ob,width=bevel/100,segments=2,angle_deg=38)
        if wear:
            ob.data.materials.append(MATS[2])
            next(m for m in ob.modifiers if m.type=='BEVEL').material=1
        HS['add_weighted_normal'](ob)
    GROUPS.setdefault(g,[]).append(ob);return ob

def solid(g,name,vv,ff,mat=0,tint=.9,bevel=.3,rough=.5,wear=False):
    bm=bmesh.new();v=[bm.verts.new(co(p)) for p in vv]
    for f in ff:bm.faces.new([v[i] for i in f])
    return finish(HS['_finish']('SRC_M03_'+name,bm,'Construction'),g,mat,tint,bevel,rough,wear)

def cross(w,h,z=0,y=0):
    return [(y+w,z+.5*h),(y+.66*w,z+h),(y-.66*w,z+h),(y-w,z+.5*h),
            (y-w,z-.55*h),(y-.66*w,z-h),(y+.66*w,z-h),(y+w,z-.55*h)]

def loft(g,name,sections,mat=0,tint=.9,bevel=.4,rough=.5,wear=False):
    vv=[];ff=[]
    for x,w,h,*extra in sections:
        y,z=(extra+[0,0])[:2];vv.extend((x,a,b) for a,b in cross(w,h,z,y))
    for j in range(len(sections)-1):
        for i in range(8):ff.append((j*8+i,j*8+(i+1)%8,(j+1)*8+(i+1)%8,(j+1)*8+i))
    ff.extend([tuple(reversed(range(8))),tuple(range(len(vv)-8,len(vv)))])
    return solid(g,name,vv,ff,mat,tint,bevel,rough,wear)

def tube(g,name,profile,mat=2,y=0,z=0,n=32,tint=.9,rough=.5):
    vv=[];ff=[]
    for x,r in profile:vv.extend((x,y+r*math.cos(i*math.tau/n),z+r*math.sin(i*math.tau/n)) for i in range(n))
    for j in range(len(profile)):
        for i in range(n):ff.append((j*n+i,j*n+(i+1)%n,((j+1)%len(profile))*n+(i+1)%n,((j+1)%len(profile))*n+i))
    return solid(g,name,vv,ff,mat,tint,0,rough)

def strip(g,name,path,width,depth,mat=0,tint=.9):
    vv=[];ff=[]
    for i,p in enumerate(path):
        d=Vector(path[min(i+1,len(path)-1)])-Vector(path[max(0,i-1)]);side=Vector((-d.y,d.x,0)).normalized()*width*.5
        for s,z in [(1,-1),(1,1),(-1,1),(-1,-1)]:vv.append(tuple(Vector(p)+side*s+Vector((0,0,z*depth*.5))))
    for j in range(len(path)-1):
        for i in range(4):ff.append((j*4+i,j*4+(i+1)%4,(j+1)*4+(i+1)%4,(j+1)*4+i))
    ff.extend([(3,2,1,0),tuple(range(len(vv)-4,len(vv)))])
    return solid(g,name,vv,ff,mat,tint,.12)

def hull_profile(x):
    for a,b in zip(HULL,HULL[1:]):
        if a[0]<=x<=b[0]:
            t=(x-a[0])/(b[0]-a[0]);return [a[i]+(b[i]-a[i])*t for i in (1,2,3)]
    raise ValueError(x)
def upper(x,u=0):
    w,h,z=hull_profile(x);return z+h-max(0,abs(u)-.66)/.34*h*.5

def primary_structure():
    loft('Hull','StructuralKeel',[(x,w,h,0,z) for x,w,h,z in HULL],3,.92,.55)
    # Chamfered front muzzle-like prow opening, recessed rather than capped flush.
    tube('Hull','ProwRim',[(395,19),(402,19),(402,14),(395,14)],1,0,0,n=8)
    loft('Hull','NoseRecess',[(389,14,9),(394,14,9)],3,bevel=.15)
    loft('Hull','CanopySeat',[(-132,40,8,0,40),(-80,59,12,0,40),(55,54,11,0,39),(150,24,8,0,39)],0,.85,.4)
    for side,label in [(-1,'L'),(1,'R')]:
        g='Wing_'+label;vv=[];ff=[]
        for y,lead,trail,h,z in WING:
            vv.extend((trail+(lead-trail)*u,side*y,z+v*h) for u,v in AIR)
        n=len(AIR)
        for j in range(len(WING)-1):
            for i in range(n):ff.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
        ff.extend([tuple(reversed(range(n))),tuple(range(len(vv)-n,len(vv)))])
        solid(g,'WingLoadShell'+label,vv,ff,0,.8,.6,.65)
        shoulder=loft(g,'Shoulder'+label,[(-158,38,24,side*110,31),(-80,42,27,side*110,36),(16,43,27,side*110,39),(72,36,27,side*110,39)],0,.91,.5,.6)
        cutter=loft('TEMP','IntakeCut'+label,[(43,30,18,side*110,39),(79,30,18,side*110,39)],3,bevel=0)
        HS['apply_modifiers'](shoulder);HS['boolean'](shoulder,cutter,operation='DIFFERENCE',solver='EXACT');HS['apply_modifiers'](shoulder)
        GROUPS['TEMP'].remove(cutter);bpy.data.objects.remove(cutter,do_unlink=True)
        # Cannons are held by a shaped saddle, not floating below the wing.
        loft('Hull','PrimarySaddle'+label,[(-16,20,21,side*100,-30),(28,26,25,side*100,-31),(66,21,21,side*100,-32)],1,.8,.4)
    # Armored cockpit glass with distinct framing, low roof and sloped windshield.
    secs=[(-112,40,47,84),(-50,53,49,104),(65,45,45,95),(143,23,40,54)]
    vv=[];ff=[]
    for x,w,b,t in secs:
        vv.extend([(x,w,b),(x,.82*w,t-10),(x,.5*w,t),(x,-.5*w,t),(x,-.82*w,t-10),(x,-w,b),(x,-.6*w,b-3),(x,.6*w,b-3)])
    for j in range(len(secs)-1):
        for i in range(8):ff.append((j*8+i,j*8+(i+1)%8,(j+1)*8+(i+1)%8,(j+1)*8+i))
    ff.extend([tuple(reversed(range(8))),tuple(range(len(vv)-8,len(vv)))])
    solid('Canopy','Glazing',vv,ff,4,1,.2,.5)
    for j,(x,w,b,t) in enumerate(secs):
        ring=vv[j*8:j*8+8];verts=[]
        centre=Vector((x,0,(b+t)*.5))
        for dx,scale in [(-1.4,1.05),(1.4,1.05),(-1.4,.975),(1.4,.975)]:
            for p in ring:
                v=centre+(Vector(p)-centre)*scale;v.x=x+dx;verts.append(tuple(v))
        faces=[]
        for i in range(8):
            k=(i+1)%8;faces.extend([(i,k,8+k,8+i),(16+i,24+i,24+k,16+k),(i,16+i,16+k,k),(8+i,8+k,24+k,24+i)])
        solid('Canopy','Frame'+str(j),verts,faces,2,.87,.18,.6)
    for side in [-1,1]:
        strip('Canopy','SideSill'+str(side),[(x,side*w,b) for x,w,b,t in secs],5,4,1,.85)
        strip('Canopy','RoofRail'+str(side),[(x,side*.5*w,t+.7) for x,w,b,t in secs],2.5,2.8,2,.9)

def machinery():
    loft('Primary','Receiver',[(-47,17,16,0,-15),(-25,24,22,0,-15),(22,25,23,0,-15),(37,20,20,0,-15)],1,.8,.45)
    loft('Primary','ReceiverHood',[(-34,16,6,0,5),(17,22,7,0,5),(31,17,6,0,5)],0,.95,.4)
    # Rotor remains the old component/pivot name; new geometry is a single clear bore.
    tube('Rotor','Barrel',[(0,17),(94,17),(119,18),(135,16.5),(137,16),(137,13),(0,13)],2,n=48,rough=.65)
    tube('Rotor','BarrelBase',[(1,22),(11,23),(25,21),(25,16),(1,16)],1,n=40,rough=.7)
    tube('Rotor','MuzzleBlue',[(121,18.3),(126,18.3),(126,17),(121,17)],5,n=48,tint=1)
    for i,x in enumerate([49,83]):tube('Rotor','BarrelCollar'+str(i),[(x,18.5),(x+4,18.5),(x+4,16),(x,16)],1,n=40,rough=.7)
    # Four discreet launch ports per side preserve every legacy rocket frame.
    loft('Secondary','FlushBay',[(-66,29,23,0,-25),(-44,34,27,0,-25),(83,34,27,0,-25),(100,29,23,0,-25)],3,.9,.4)
    loft('Secondary','BayMount',[(-56,23,13,0,10),(23,23,14,0,10),(60,18,12,0,9)],1,.8,.35)
    for i,(y,z) in enumerate([(-16,-9),(-16,-39),(16,-9),(16,-39)]):
        tube('Secondary','LaunchLiner'+str(i),[(86,12),(105,12),(109,11),(109,9),(86,9)],1,y,z,n=24,rough=.8)
    # Both drives share a local mesh but remain independently swappable assemblies.
    block=loft('Engine','EngineBlock',[(3,38,37,0,0),(31,46,46,0,1),(118,46,46,0,3),(191,38,37,0,6),(215,27,26,0,8)],1,.82,.6,rough=.7)
    cutter=HS['cylinder']('CUT_Exhaust',radius=.325,depth=.70,segments=64,axis='Y',location=co((22,0,0)),collection='Construction')
    cutter.data.materials.append(MATS[1]);HS['apply_modifiers'](block);HS['boolean'](block,cutter,operation='DIFFERENCE',solver='EXACT');HS['apply_modifiers'](block)
    bpy.data.objects.remove(cutter,do_unlink=True)
    # A dark hollow cap and recessed blue annulus produce an actual nozzle cavity.
    tube('Engine','ExhaustRim',[(0,32),(4,36),(24,36),(31,30),(31,25),(2,25),(0,28)],2,n=64,rough=.7)
    tube('Engine','BlueCore',[(22,25.5),(25,25.5),(25,12),(22,12)],5,n=64,tint=1)
    tube('Engine','CenterHub',[(23,12.3),(30,12.3),(30,1),(23,1)],3,n=32)

def wing_surface(y,u):
    for a,b in zip(WING,WING[1:]):
        if a[0]<=y<=b[0]:
            t=(y-a[0])/(b[0]-a[0]);lead,trail,h,z=[a[k]+(b[k]-a[k])*t for k in (1,2,3,4)]
            for p,q in zip(AIR[:6],AIR[1:6]):
                if p[0]<=u<=q[0]:return trail+(lead-trail)*u,z+h*(p[1]+(q[1]-p[1])*(u-p[0])/(q[0]-p[0]))
    raise ValueError((y,u))

def wing_panel(side,label,y0,y1,u0,u1,tint):
    ys=sorted(set([y0,y1]+[s[0] for s in WING if y0<s[0]<y1]));us=sorted(set([u0,u1]+[p[0] for p in AIR[:6] if u0<p[0]<u1]))
    vv=[];ff=[];rows=len(ys);cols=len(us);n=rows*cols
    for dz in [-1.4,1.05]:
        for y in ys:
            for u in us:
                x,z=wing_surface(y,u);vv.append((x,side*y,z+dz))
    for r in range(rows-1):
        for c in range(cols-1):
            a=r*cols+c;b=a+1;d=(r+1)*cols+c;e=d+1;ff.extend([(a,b,e,d),(a+n,d+n,e+n,b+n)])
    edge=list(range(cols))+[r*cols+cols-1 for r in range(1,rows)]+list(range(n-2,n-cols-1,-1))+[r*cols for r in range(rows-2,0,-1)]
    ff.extend((a,a+n,b+n,b) for a,b in zip(edge,edge[1:]+edge[:1]))
    solid('Wing_'+label,'Armor'+label+str(y0)+str(u0),vv,ff,0,tint,.18,.6,True)

def refinement():
    # Segmented top armor has real seams, varied panel values and chamfered metal edges.
    for i,(a,b) in enumerate([(-371,-304),(-302,-222),(-220,-139),(-137,-58),(-56,42),(44,154),(156,267),(269,367),(369,395)]):
        sections=[]
        for x in [a,(a+b)/2,b]:
            w,h,z=hull_profile(x);sections.append((x,w*.64,1.8,0,z+h-.4))
        loft('Hull','DeckPlate'+str(i),sections,0,.73+.04*(i%5),.25,rough=.4+.06*(i%4),wear=True)
        if a>150:
            # A recessed center service tile breaks the long prow into functional bays.
            service=[]
            for x in [a+6,b-6]:
                w,h,z=hull_profile(x);service.append((x,w*.24,1.1,0,z+h+1.4))
            loft('Hull','NoseService'+str(i),service,1,.7,.18,rough=.8)
    for side,label in [(-1,'L'),(1,'R')]:
        path=[]
        for x in [398,370,280,180,90,-50]:
            w,h,z=hull_profile(x);path.append((x,side*w*.8,upper(x,.8)+.7))
        strip('Hull','ConduitChannel'+label,path,9,3.5,3,.9)
        strip('Hull','ContinuousBlueConduit'+label,[(x,y,z+1.5) for x,y,z in path],3,1,5,1)
        for j,(y0,y1,u0,u1) in enumerate([(92,177,.22,.53),(180,275,.22,.55),(278,347,.22,.55),(155,257,.59,.81),(260,342,.59,.81)]):
            wing_panel(side,label,y0,y1,u0,u1,.74+.055*(j%4))
        led=[]
        for y in [92,145,270,354]:
            x,z=wing_surface(y,.93);led.append((x,side*y,z+.7))
        strip('Wing_'+label,'WingEdgeChannel'+label,led,8,3,3,.9)
        strip('Wing_'+label,'WingEdgeBlue'+label,[(x,y,z+1.5) for x,y,z in led],2.8,1,5,1)
        # Recessed intake opening: inset glow backing and six attached mechanical louvers.
        loft('Wing_'+label,'IntakeBacking'+label,[(40,29,17.5,side*110,39),(47,29,17.5,side*110,39)],5,1,.12)
        for j in range(6):
            y=side*(87+j*9)
            loft('Wing_'+label,'IntakeLouver'+label+str(j),[(51,2.2,19,y,39),(67,2.2,19,y,39)],3,.9,.15)
        # Small longitudinal pipes and hardpoints are clustered below the wing root.
        for z in [-25,-34]:
            loft('Hull','RootRail'+label+str(z),[(-190,4,4,side*72,z),(-34,4,4,side*72,z),(28,3,3,side*72,z)],1,.8,.25)
        loft('Wing_'+label,'ShoulderPlate'+label,[(-143,28,3,side*110,58),(-62,32,3,side*110,64),(9,30,3,side*110,66)],0,.88,.22,wear=True)
    # Engine armor cassettes follow the angular case, leaving mechanical lower surfaces.
    for i,(a,b) in enumerate([(40,87),(90,137),(140,184)]):
        loft('Engine','DriveArmor'+str(i),[(a,35,7,0,44),(b,32,7,0,43)],0,.78+i*.05,.35,rough=.65,wear=True)
    for i in range(12):
        a=i*math.tau/12;y=35*math.cos(a);z=35*math.sin(a)
        loft('Engine','NozzlePetal'+str(i),[(3,2.8,2.8,y,z),(20,3.2,3.2,y,z),(27,2,2,y,z)],1,.8,.15,rough=.8)
    for side in [-1,1]:
        loft('Primary','ReceiverPanel'+str(side),[(-27,3,11,side*23,-15),(13,3,12,side*23,-15),(25,2,8,side*21,-15)],0,.8,.2)
    # Sealed belly access construction; no collision or landing-gear logic is added.
    loft('Hull','BellyPanel',[(-222,38,3,0,-28),(-170,42,3,0,-30),(-55,37,3,0,-29)],1,.65,.3,rough=.8)

def combine():
    meshes={}
    for group,parts in GROUPS.items():
        if not parts:continue
        if len(parts)==1:ob=parts[0];HS['apply_modifiers'](ob);ob.name='SM_LancerM03_'+group;ob.data.name=ob.name
        else:ob=HS['join'](parts,name='SM_LancerM03_'+group,apply_mods=True)
        bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-5)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-5);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(ob.data);bm.free();ob.data.update();HS['smooth_by_angle'](ob,angle_deg=38);HS['add_weighted_normal'](ob);HS['apply_modifiers'](ob)
        HS['smart_uv'](ob,angle_deg=60,island_margin=.012)
        uv=ob.data.uv_layers.active.data;tiny=[]
        for f in ob.data.polygons:
            pp=[uv[i].uv.copy() for i in f.loop_indices];area=abs(sum(a.x*b.y-b.x*a.y for a,b in zip(pp,pp[1:]+pp[:1])))*.5
            if area<1e-12:tiny.append(f)
        if tiny:
            for d in uv:d.uv.x*=.94
            for i,f in enumerate(tiny):
                x=.95+(i%3)*.015;y=.01+(i//3)*.015
                if y>.98:raise RuntimeError('Repair micro topology')
                for li,val in zip(f.loop_indices,[(x,y),(x+.01,y),(x,y+.01)]):uv[li].uv=val
        if group in ['Hull','Wing_L','Wing_R','Canopy']:
            pivot=co(PARTS[group])
            for v in ob.data.vertices:v.co-=pivot
        meshes[group]=ob
    return meshes

def assemble(meshes):
    coll=bpy.data.collections.new('ASSET_Lancer_M03');bpy.context.scene.collection.children.link(coll);obs={}
    for group,ob in meshes.items():ob.name='SRC_Canonical_'+group
    for name,pos in PARTS.items():
        role='Primary' if name.startswith('Primary_') else 'Rotor' if name.startswith('Rotor_') else 'Secondary' if name.startswith('Secondary_') else 'Engine' if name.startswith('Engine_') else name
        ob=bpy.data.objects.new('SM_LancerM03_'+name,meshes[role].data);coll.objects.link(ob);ob.location=co(pos);ob['part_name']=name;obs[name]=ob
    for ob in meshes.values():bpy.data.objects.remove(ob,do_unlink=True)
    old=json.loads((ROOT/'attachments.json').read_text());sockets=[]
    for s in old['sockets']:
        spec=dict(s);spec['root_position_cm']=[a+b for a,b in zip(PARTS[s['parent']],s['position_cm'])]
        spec['legacy']=True;spec['purpose']='muzzle' if s['name'].startswith('Muzzle_') else 'thruster' if s['name'].startswith('Thruster_') else 'reference'
        sockets.append(spec)
    for name,p,f in [('Engine',[-402,0,-3],[-1,0,0]),('CockpitCamera',[148,0,78],[1,0,0]),('Camera_Chase',[-560,0,180],[1,0,0])]:
        sockets.append(dict(parent='Hull',name=name,position_cm=p,root_position_cm=p,forward=f,up=[0,0,1],legacy=False,purpose='supplemental_proposed'))
    for s in sockets:
        ob=bpy.data.objects.new('SOCKET_'+s['parent']+'_'+s['name'],None);coll.objects.link(ob);ob.parent=obs[s['parent']];ob.location=co(s['position_cm'])
        f=co([x*100 for x in s['forward']]).normalized();u=Vector((0,0,1));r=u.cross(f)
        ob.rotation_euler=Matrix((r,-f,u)).transposed().to_euler();ob.empty_display_type='ARROWS';ob.empty_display_size=.1
    write('attachments.json',dict(units='cm',forward='+X',right='+Y',up='+Z',legacy_count=33,sockets=sockets),P)
    write('unreal_visual_parts.json',dict(primary_mesh='SM_LancerM03_Hull',native_imported=False,parts=[dict(PartName=name,MeshSource=obs[name].data.name,UnrealAssetPath=None,MaterialSlots=[m.name for m in obs[name].data.materials],ParentSocket=None,translation_cm=pos,rotation_degrees=[0,0,0],scale=[1,1,1]) for name,pos in PARTS.items() if name!='Hull'],rocket_visual_offset_cm=[54,0,0]),P)
    bpy.context.view_layer.update();return obs

def build(refined=True):
    if not bpy.app.background or bpy.data.filepath:raise RuntimeError('Fresh scheduled factory-startup process required')
    before=hashlib.sha256((ROOT/'attachments.json').read_bytes()).hexdigest();start=time.perf_counter()
    for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    materials();primary_structure();machinery()
    if refined:refinement()
    meshes=combine();obs=assemble(meshes)
    assert before==hashlib.sha256((ROOT/'attachments.json').read_bytes()).hexdigest()
    write('build_manifest.json',dict(blender=bpy.app.version_string,phase='refined' if refined else 'structural',assembly_triangles=sum(len(o.data.polygons) for o in obs.values()),parts=len(obs),mesh_roles=len(meshes),materials=len(MATS),legacy_sha256=before,legacy_unchanged=True,elapsed_seconds=time.perf_counter()-start,native_imported=False,visual_approved=False))
    bpy.ops.wm.save_as_mainfile(filepath=str(P/('Lancer_M03_work.blend' if refined else 'Lancer_M03_structure_v01.blend')))
    return obs

if __name__=='__main__':build()
