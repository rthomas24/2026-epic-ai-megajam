"""Rift refined foundation, repository MIT kit. Scheduled headless process only."""
from pathlib import Path
import hashlib, json, math, runpy, time
import bpy, bmesh
from mathutils import Matrix, Vector

PROD=Path(__file__).resolve().parent
ROOT=PROD.parent
REPO=ROOT.parents[3]
OUT=ROOT/'review/M01'
HS=runpy.run_path(str(REPO/'.claude/skills/blender-game-asset/scripts/hardsurface.py'))
GROUPS={};MATS=[]
VIOLET='#6B2BD9';PLUM='#2A0F4F';LIGHT='#CCD6DF'
PARTS={'Hull':(0,0,0),'Pulse_L':(20,-235,-32),'Pulse_R':(20,235,-32),
       'Splitter':(70,0,-34),'PortalGuard':(292,0,5),'PortalRotor':(292,0,5),
       'Engine':(-354,0,0)}

def write(name,data,folder=OUT):
    folder.mkdir(parents=True,exist_ok=True)
    (folder/name).write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')

def co(p):
    x,y,z=p
    return Vector((y/100,-x/100,z/100))

def rgb(h):return HS['hex_color'](h)

def materials():
    specs=[('Paint','#FFFFFF',0,.52,.10,0),('Composite','#35313D',0,.73,.02,0),
           ('Titanium','#ADB7C2',1,.39,.05,0),('Canopy','#253049',0,.28,.25,0),
           ('Magenta','#FF2BD6',0,.40,0,1.7),('Ceramic','#C6CDD3',0,.51,.08,0)]
    params=[]
    for name,h,metal,rough,coat,em in specs:
        mat=bpy.data.materials.new('M_Rift_'+name);mat.use_nodes=True
        mat.diffuse_color=(*rgb(VIOLET if name=='Paint' else h),1)
        nodes=mat.node_tree.nodes;links=mat.node_tree.links
        bsdf=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
        bsdf.inputs['Metallic'].default_value=metal
        bsdf.inputs['Coat Weight'].default_value=coat
        bsdf.inputs['Coat Roughness'].default_value=.30
        attr=nodes.new('ShaderNodeVertexColor');attr.layer_name='PanelTint'
        mix=nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY'
        mix.inputs[0].default_value=1;mix.inputs[1].default_value=(*rgb(h),1)
        links.new(attr.outputs['Color'],mix.inputs[2]);links.new(mix.outputs[0],bsdf.inputs['Base Color'])
        mul=nodes.new('ShaderNodeMath');mul.operation='MULTIPLY_ADD'
        mul.inputs[1].default_value=.4;mul.inputs[2].default_value=.8
        links.new(attr.outputs['Alpha'],mul.inputs[0])
        roughnode=nodes.new('ShaderNodeMath');roughnode.operation='MULTIPLY'
        roughnode.inputs[1].default_value=rough
        links.new(mul.outputs[0],roughnode.inputs[0]);links.new(roughnode.outputs[0],bsdf.inputs['Roughness'])
        if em:
            bsdf.inputs['Emission Color'].default_value=(*rgb(h),1)
            bsdf.inputs['Emission Strength'].default_value=em
        MATS.append(mat)
        params.append(dict(name=mat.name,base_color_srgb=h,metallic=metal,roughness=rough,
                           coat_weight=coat,coat_roughness=.30,emissive_strength=em,
                           vertex_color='PanelTint',base_color_rule='BaseColor * VertexColor.RGB',
                           roughness_rule='Roughness * (0.8 + 0.4 * VertexColor.A)',textures=0))
    write('material_parameters.json',params,PROD)

def finish(ob,group,mat=0,color=VIOLET,bevel=.35,rough=.5,smooth=45):
    ob.data.materials.append(MATS[mat])
    attr=ob.data.color_attributes.new(name='PanelTint',type='FLOAT_COLOR',domain='CORNER')
    tint=rgb(color) if mat==0 else (1,1,1)
    for face in ob.data.polygons:
        for li in face.loop_indices:
            v=ob.data.vertices[ob.data.loops[li].vertex_index].co
            variation=.09*math.sin(v.x*4.2+v.y*2.5)+.04*math.cos(v.z*10+v.x*7)
            attr.data[li].color=(*tint,max(.05,min(.95,rough+variation)))
    HS['smooth_by_angle'](ob,angle_deg=smooth)
    if bevel:
        HS['add_bevel'](ob,width=bevel/100,segments=2,angle_deg=42)
        HS['add_weighted_normal'](ob)
    GROUPS.setdefault(group,[]).append(ob)
    return ob

def solid(group,name,verts,faces,mat=0,color=VIOLET,bevel=.35,rough=.5,smooth=45):
    bm=bmesh.new();vv=[bm.verts.new(co(p)) for p in verts]
    for f in faces:bm.faces.new([vv[i] for i in f])
    ob=HS['_finish']('SRC_Rift_'+name,bm,'Construction')
    return finish(ob,group,mat,color,bevel,rough,smooth)

def loft(group,name,sections,mat=0,color=VIOLET,bevel=.4,n=3,segments=24,rough=.5):
    specs=[]
    for x,w,h,*rest in sections:
        y,z=(rest+[0,0])[:2]
        specs.append(dict(y=-x/100,w=w*2/100,h=h*2/100,x=y/100,z=z/100,n=n))
    ob=HS['loft']('SRC_Rift_'+name,specs,ring_verts=segments,collection='Construction')
    return finish(ob,group,mat,color,bevel,rough)

def tube(group,name,profile,mat=2,y=0,z=0,segments=32,color=VIOLET,rough=.5):
    vv=[];ff=[]
    for x,r in profile:
        vv.extend((x,y+r*math.cos(a*math.tau/segments),z+r*math.sin(a*math.tau/segments)) for a in range(segments))
    for j in range(len(profile)):
        for i in range(segments):
            k=(i+1)%segments;l=(j+1)%len(profile)
            ff.append((j*segments+i,j*segments+k,l*segments+k,l*segments+i))
    return solid(group,name,vv,ff,mat,color,0,rough,50)

def bolt(group,name,p,axis='Z'):
    ob=HS['cylinder']('SRC_Rift_'+name,radius=.018,depth=.025,segments=6,
                      axis=axis,location=co(p),collection='Construction')
    HS['apply_transform'](ob,location=True,rotation=True,scale=True)
    return finish(ob,group,2,bevel=.12,rough=.7)

def wing_profile(y):
    t=min(abs(y)/425,1)
    return (-165+490*t*t, 330*(1-t)**.48+6, 80*(1-t)**.65+3, -7+12*t*t)

def wing_surface(y,u,upper=True):
    centre,chord,height,z=wing_profile(y)
    return centre+(u-.5)*chord,z+(1 if upper else -1)*height*.5*math.sqrt(max(0,1-(2*u-1)**2))

def wing_patch(side,name,y0,y1,u0,u1,mat=0,color=VIOLET,rough=.5):
    # Dense supporting samples and buried back face avoid flat patches cutting a curved wing.
    ys=[y0+(y1-y0)*i/8 for i in range(9)]
    us=[u0+(u1-u0)*i/5 for i in range(6)]
    vv=[];ff=[];rows=len(ys);cols=len(us);n=rows*cols
    for dz in [-1.1,1.05]:
        for y in ys:
            for u in us:
                x,z=wing_surface(y,u);vv.append((x,side*y,z+dz))
    for r in range(rows-1):
        for c in range(cols-1):
            a=r*cols+c;b=a+1;d=(r+1)*cols+c;e=d+1
            ff.extend([(a,b,e,d),(a+n,d+n,e+n,b+n)])
    edge=list(range(cols))+[r*cols+cols-1 for r in range(1,rows)]+list(range(n-2,n-cols-1,-1))+[r*cols for r in range(rows-2,0,-1)]
    ff.extend((a,a+n,b+n,b) for a,b in zip(edge,edge[1:]+edge[:1]))
    return solid('Hull',name+str(side),vv,ff,mat,color,.15,rough,60)

def hull():
    vv=[];ff=[];steps=64;ring=32
    for j in range(steps+1):
        y=-425+850*j/steps;centre,chord,height,z=wing_profile(y)
        for i in range(ring):
            a=i*math.tau/ring
            vv.append((centre+chord*.5*math.cos(a),y,z+height*.5*math.sin(a)))
    for j in range(steps):
        for i in range(ring):ff.append((j*ring+i,j*ring+(i+1)%ring,(j+1)*ring+(i+1)%ring,(j+1)*ring+i))
    ff.extend([tuple(reversed(range(ring))),tuple(range(steps*ring,(steps+1)*ring))])
    wing=solid('Hull','CrescentMonocoque',vv,ff,0,VIOLET,.25,.60,65)
    wing.data.materials.append(MATS[2])
    attr=wing.data.color_attributes['PanelTint']
    for f in wing.data.polygons:
        c=sum((wing.data.vertices[i].co for i in f.vertices),Vector())/len(f.vertices)
        y=c.x*100;centre,chord,height,z=wing_profile(y);u=(-c.y*100-centre)/chord+.5
        if u>.987:
            f.material_index=1
            for li in f.loop_indices:attr.data[li].color=(1,1,1,.6)
        elif f.normal.z<-.1:
            for li in f.loop_indices:attr.data[li].color=(*rgb('#49306B'),.8)
    # Compact keel integrated through the continuous wing root, rather than Lancer's needle.
    loft('Hull','Keel',[(-345,26,15,0,10),(-318,57,32,0,13),(-240,78,44,0,18),
         (-130,81,47,0,20),(-20,69,45,0,25),(75,54,37,0,24),(145,37,27,0,18),(195,17,14,0,12)],
         0,PLUM,.55,n=2.8,segments=32,rough=.7)
    # Three separated upper armor panels reveal a dark supporting keel at their seams.
    for i,ss in enumerate([
        [(-306,49,20,0,35),(-257,66,29,0,38),(-175,68,30,0,42)],
        [(-173,68,30,0,42),(-92,66,28,0,46),(-35,59,23,0,49)],
        [(-33,59,23,0,49),(65,47,20,0,44),(137,31,14,0,40),(177,18,8,0,35)] ]):
        loft('Hull','DorsalArmor'+str(i),ss,0,VIOLET,.45,n=3.1,segments=24,rough=.4+i*.12)
    # Canopy sits on a sealed, load-bearing rim. Its gloss is deliberately restrained.
    loft('Hull','CanopyGasket',[(-82,36,5,0,68),(-51,55,10,0,70),(80,48,10,0,66),(132,18,3,0,53)],1,bevel=.5,n=3.8)
    loft('Hull','CanopyRim',[(-77,33,4,0,72),(-48,52,8,0,74),(78,45,8,0,70),(126,16,3,0,56)],5,bevel=.45,n=3.6)
    loft('Hull','Canopy',[(-70,28,4,0,77),(-40,48,23,0,87),(34,46,26,0,86),(84,36,17,0,78),(120,12,3,0,61)],3,bevel=.4,n=2.7,segments=32,rough=.65)
    for side in [-1,1]:
        loft('Hull','CanopyHinge'+str(side),[(-62,5,5,side*47,78),(-40,5,5,side*49,78)],2,bevel=.25,n=4,segments=12)
        # Form-following panel breaks and small fleet-ceramic blade near the inboard root.
        wing_patch(side,'SweepArmor',170,345,.22,.48,0,'#803BE0',.7)
        wing_patch(side,'MagentaLivery',220,396,.71,.755,0,'#D549C7',.55)
        wing_patch(side,'RootCeramic',103,165,.30,.46,5,rough=.7)
        for y in [205,312]:
            x,z=wing_surface(y,.34);bolt('Hull','WingLatch'+str(side)+str(y),(x,side*y,z+.6))
        # Visible weapon saddle intersects the wing underside by several centimetres.
        loft('Hull','PulseFairing'+str(side),[(-67,26,20,side*235,-17),(-20,28,21,side*235,-18),
             (75,23,17,side*235,-18),(112,13,10,side*235,-19)],0,PLUM,.5,n=3.4,segments=24)
        # Forks support the open portal bearing at two lower-quarter attachment sectors.
        loft('Hull','PortalFork'+str(side),[(112,12,18,side*37,-3),(160,14,17,side*44,-7),
             (235,12,13,side*60,-18),(295,11,10,side*68,-27)],0,VIOLET,.6,n=3.4,segments=24)
        loft('Hull','ForkRail'+str(side),[(152,3.5,6,side*51,-9),(228,4,6,side*67,-20),
             (290,3,5,side*75,-27)],2,bevel=.2,n=3,segments=12)
        # Nacelle construction is partially embedded in the aft crescent and keel.
        loft('Hull','DriveFairing'+str(side),[(-332,28,24,side*112,0),(-302,43,38,side*112,3),
             (-230,47,42,side*110,6),(-160,34,29,side*106,12),(-112,14,13,side*94,18)],
             0,VIOLET,.55,n=2.8,segments=32,rough=.65)
        # Framed radiator: a true seating bed under connected fins.
        loft('Hull','RadiatorBed'+str(side),[(-249,18,6,side*105,42),(-222,21,7,side*105,45),(-171,15,5,side*105,41)],1,bevel=.3,n=4,segments=12)
        for j in range(6):
            x=-233+j*9
            loft('Hull','RadiatorFin'+str(side)+str(j),[(x,16,4,side*105,50),(x+3,16,4,side*105,50)],2,bevel=.12,n=4,segments=8,rough=.75)
    loft('Hull','SplitterSaddle',[(62,20,15,0,-18),(131,21,17,0,-20),(165,15,13,0,-23)],1,bevel=.5,n=4)
    # Gear-bay covers are sealed visual panels; no landing animation is implemented.
    loft('Hull','BellyBay',[(-204,29,4,0,-28),(-165,32,5,0,-30),(-60,27,4,0,-27)],5,bevel=.3,n=4,rough=.8)

def weapons():
    # Canonical module origin: aft mounting plane, forward +X; reusable on either hardpoint.
    g='Pulse'
    loft(g,'PulseReceiver',[(-44,12,10),(0,18,16),(68,17,15),(100,12,12)],1,bevel=.45,n=3.5)
    loft(g,'PulseHood',[(-28,16,6,0,12),(16,21,7,0,12),(74,17,6,0,11),(111,9,4,0,9)],0,LIGHT,.35,n=3.5,rough=.65)
    tube(g,'PulseBarrel',[(72,11.5),(114,11.5),(137,10),(152,9),(155,7.5),(155,5),(72,5)],2,segments=32,rough=.7)
    tube(g,'PulseLens',[(148,5.2),(152,5.2),(152,1),(148,1)],4,segments=24)
    for i,x in enumerate([82,105,126]):
        tube(g,'PulseCoil'+str(i),[(x,13),(x+4,13),(x+4,10.8),(x,10.8)],2,segments=24,rough=.7)
    for side in [-1,1]:
        loft(g,'PulseBus'+str(side),[(-10,3,4,side*17,1),(62,3,4,side*16,1),(89,2,3,side*13,1)],4,bevel=.15,n=4,segments=8)
    g='Splitter'
    loft(g,'SplitterReceiver',[(-23,15,13),(16,23,19),(84,22,18),(130,14,14)],1,bevel=.5,n=3.4)
    loft(g,'SplitterShroud',[(-12,15,7,0,-12),(34,26,9,0,-13),(94,23,8,0,-12),(137,13,5,0,-10)],0,PLUM,.4,n=3.6,rough=.7)
    tube(g,'SplitterBarrel',[(105,14),(134,17),(208,16),(238,13),(250,11),(250,7),(105,7)],2,segments=40,rough=.75)
    tube(g,'SplitterLens',[(241,7.3),(246,7.3),(246,1.5),(241,1.5)],4,segments=32)
    for i,x in enumerate([144,169,194]):
        tube(g,'ChargeCoil'+str(i),[(x,18.5),(x+7,18.5),(x+7,15),(x,15)],4,segments=32)
    for side in [-1,1]:
        loft(g,'SplitterRail'+str(side),[(11,3,6,side*23,1),(94,4,6,side*22,1),(130,2,4,side*15,1)],2,bevel=.2,n=4,segments=12)

def portal_and_engines():
    tube('PortalGuard','PortalBearing',[(-14,77),(-9,82),(9,82),(17,77),(17,70.5),(-14,70.5)],1,segments=96)
    tube('PortalGuard','PortalArmor',[(-9,83),(-5,85),(7,85),(12,81),(12,78),(-9,78)],0,segments=96,color=VIOLET,rough=.6)
    # Front insert band is one closed radial surface, not stacked coplanar emissive rings.
    rotor=tube('PortalRotor','PortalRotor',[(-1,71.2),(15,71.2),(21,69),(21,63),(16,61),(-1,61)],2,segments=96,rough=.65)
    rotor.data.materials.append(MATS[4])
    for f in rotor.data.polygons:
        c=sum((rotor.data.vertices[i].co for i in f.vertices),Vector())/len(f.vertices)
        if -c.y*100>17 and math.hypot(c.x*100,c.z*100)<68.5:
            f.material_index=1
    for i in range(12):
        a=i*math.tau/12;y,z=81*math.cos(a),81*math.sin(a)
        # Housing/ring overlaps radially; case lugs are integrated machining, not floaters.
        loft('PortalGuard','BearingLug'+str(i),[(-6,5,5,y,z),(10,6,6,y,z),(19,4,4,y,z)],2,bevel=.25,n=3.2,segments=12,rough=.8)
    for side in [-1,1]:
        y=side*112
        # Engine canonical origin x=-354; all coordinates below are local to that pivot.
        tube('Engine','Nozzle'+str(side),[(1,31),(6,35),(28,35),(43,31),(43,24),(7,24),(1,26)],2,y,0,48,rough=.8)
        tube('Engine','DriveEmitter'+str(side),[(27,24.4),(31,24.4),(31,17),(27,17)],4,y,0,48)
        loft('Engine','DriveCore'+str(side),[(29,17.3,17.3,y,0),(42,16,16,y,0)],1,bevel=.2,n=2,segments=32)
        for j in range(12):
            a=j*math.tau/12;cy=y+33*math.cos(a);cz=33*math.sin(a)
            loft('Engine','NozzlePetal'+str(side)+str(j),[(6,3,3,cy,cz),(25,3.5,3.5,cy,cz),(32,2,2,cy,cz)],2,bevel=.15,n=3,segments=8,rough=.8)

def combine():
    meshes={}
    for group,parts in GROUPS.items():
        if len(parts)==1:
            ob=parts[0];HS['apply_modifiers'](ob)
            ob.name='SM_Rift_'+group;ob.data.name=ob.name
        else:
            ob=HS['join'](parts,name='SM_Rift_'+group,apply_mods=True)
        bm=bmesh.new();bm.from_mesh(ob.data)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-5)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-5)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(ob.data);bm.free();ob.data.update()
        HS['smooth_by_angle'](ob,angle_deg=55 if group=='Hull' else 45)
        HS['add_weighted_normal'](ob);HS['apply_modifiers'](ob)
        HS['smart_uv'](ob,angle_deg=62,island_margin=.012)
        uv=ob.data.uv_layers.active.data;tiny=[]
        for f in ob.data.polygons:
            pp=[uv[i].uv.copy() for i in f.loop_indices]
            area=abs(sum(a.x*b.y-b.x*a.y for a,b in zip(pp,pp[1:]+pp[:1])))*.5
            if area<1e-12:tiny.append(f)
        if tiny:
            for d in uv:d.uv.x*=.94
            for i,f in enumerate(tiny):
                x=.95+(i%3)*.015;y=.01+(i//3)*.015
                if y>.98:raise RuntimeError('Too many micro UV islands')
                for li,val in zip(f.loop_indices,[(x,y),(x+.01,y),(x,y+.01)]):uv[li].uv=val
        ob['micro_uv_islands_repaired']=len(tiny);meshes[group]=ob
    return meshes

def attachments():
    sockets=[]
    def add(parent,name,p,forward=(1,0,0),purpose='attachment'):
        sockets.append(dict(parent=parent,name=name,position_cm=p,forward=forward,up=(0,0,1),purpose=purpose))
    for side,label in [(-1,'L'),(1,'R')]:
        add('Hull','Mount_Primary_'+label,PARTS['Pulse_'+label])
        add('Pulse_'+label,'Muzzle_Primary_'+label,(177,side*235,-32),purpose='muzzle')
        add('Engine','Thruster_'+label,(-357,side*112,0),(-1,0,0),'thruster')
    add('Hull','Mount_Secondary',PARTS['Splitter'])
    add('Hull','Mount_Portal',PARTS['PortalGuard'])
    add('Hull','Engine',PARTS['Engine'],(-1,0,0),'engine_alias')
    add('Hull','CockpitCamera',(70,0,108),purpose='camera')
    add('Hull','Ability_Cloak',(0,0,20),purpose='ability')
    add('Hull','Shield_Origin',(0,0,0),purpose='ability')
    add('Hull','Camera_Chase',(-480,0,175),purpose='camera_reference_only')
    add('Splitter','Muzzle_Secondary',(323,0,-34),purpose='muzzle')
    add('Splitter','Charge_Focus',(322,0,-34),purpose='ability')
    add('PortalRotor','Portal_Pivot',PARTS['PortalRotor'],purpose='pivot')
    add('PortalRotor','Portal_Exit',(323,0,5),purpose='ability')
    # Runtime currently discovers Muzzle_0/Muzzle_1 on the first static mesh.
    # These hull aliases intentionally resolve the two primary guns only.
    add('Hull','Muzzle_0',(177,-235,-32),purpose='primary_runtime_alias')
    add('Hull','Muzzle_1',(177,235,-32),purpose='primary_runtime_alias')
    for s in sockets:
        s['root_position_cm']=list(s['position_cm'])
        origin=PARTS[s['parent']]
        s['position_cm']=[v-o for v,o in zip(s['position_cm'],origin)]
    return dict(units='cm',forward='+X',right='+Y',up='+Z',status='proposed_art_contract_engineering_review_pending',sockets=sockets)

def assemble(meshes):
    coll=bpy.data.collections.new('ASSET_Rift_M01');bpy.context.scene.collection.children.link(coll)
    obs={}
    for group,ob in meshes.items():ob.name='SRC_Canonical_'+group
    for name,position in PARTS.items():
        source=meshes['Pulse' if name.startswith('Pulse_') else name]
        ob=bpy.data.objects.new('SM_Rift_'+name,source.data);coll.objects.link(ob)
        ob.location=co(position);ob['part_name']=name;obs[name]=ob
    for ob in meshes.values():bpy.data.objects.remove(ob,do_unlink=True)
    manifest=attachments()
    for s in manifest['sockets']:
        ob=bpy.data.objects.new('SOCKET_'+s['parent']+'_'+s['name'],None);coll.objects.link(ob)
        ob.parent=obs[s['parent']];ob.location=co(s['position_cm'])
        f=co([v*100 for v in s['forward']]).normalized();u=Vector((0,0,1));r=u.cross(f)
        ob.rotation_euler=Matrix((r,-f,u)).transposed().to_euler()
        ob.empty_display_type='ARROWS';ob.empty_display_size=.12;ob['unreal_socket_name']=s['name']
    write('attachments.json',manifest,PROD)
    mapping=dict(primary_mesh='SM_Rift_Hull',native_asset=None,engineering_base='e4b5715',
                 parts=[dict(PartName=name,MeshSource='SM_Rift_'+('Pulse' if name.startswith('Pulse_') else name),ParentSocket=None,
                             RelativeTransform=dict(translation_cm=p,rotation_degrees=[0,0,0],scale=[1,1,1]),UnrealAssetPath=None)
                        for name,p in PARTS.items() if name!='Hull'],
                 dynamic_ring_animation_implemented=False,weapon_specific_runtime_binding_implemented=False)
    write('unreal_visual_parts.json',mapping,PROD)
    bpy.context.view_layer.update();return obs

def build():
    if not bpy.app.background or bpy.data.filepath:raise RuntimeError('Use new background factory-startup process')
    OUT.mkdir(parents=True,exist_ok=True);started=time.perf_counter()
    for ob in list(bpy.data.objects):bpy.data.objects.remove(ob,do_unlink=True)
    sc=bpy.context.scene;sc.unit_settings.system='METRIC';sc.unit_settings.scale_length=1;sc.unit_settings.length_unit='METERS'
    materials();hull();weapons();portal_and_engines();meshes=combine();obs=assemble(meshes)
    sc['stage']='refined_foundation_manual_review_pending';sc['claim_commit']='e4b5715'
    write('build_manifest.json',dict(blender=bpy.app.version_string,triangles=sum(len(o.data.polygons) for o in obs.values()),
          instances=len(obs),materials=len(MATS),mesh_roles=len(meshes),elapsed_seconds=time.perf_counter()-started,
          source_route='repository_MIT_hardsurface_kit_features_ART03_Lead_dispatch_e4b5715',
          native_imported=False,manual_approval=False))
    bpy.ops.wm.save_as_mainfile(filepath=str(PROD/'Rift_M01_work.blend'))
    return obs

if __name__=='__main__':build()
