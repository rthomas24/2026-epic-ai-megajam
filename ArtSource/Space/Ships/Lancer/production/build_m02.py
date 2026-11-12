"""Lancer M02: refine approved interceptor direction, preserving M01 attachment ABI.

Run in a new scheduled background Blender process with --factory-startup -t 2.
All writes stay in production/ or review/M02/. No Unreal or source regeneration.
Uses project MIT hardsurface kit; user-approved refinement of the M01 design.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import runpy
import sys
import time

import bpy
import bmesh
from mathutils import Matrix, Vector

PROD=Path(__file__).resolve().parent
ROOT=PROD.parent
REPO=ROOT.parents[3]
REVIEW=ROOT/"review/M02"
REVIEW.mkdir(parents=True,exist_ok=True)
HS=runpy.run_path(str(REPO/".claude/skills/blender-game-asset/scripts/hardsurface.py"))
GROUPS={}
SPECS=[
    ("Armour","#CBD5DB",.26,.40,.26,0),
    ("CyanPaint","#159DB7",.28,.28,.45,0),
    ("Composite","#172A34",.0,.70,.02,0),
    ("Titanium","#93A5AF",1.0,.25,.14,0),
    ("Canopy","#102F43",.42,.12,.72,0),
    ("Emission","#29DAF5",.0,.32,.0,2.2)
]
MATERIALS=[]


def source_hashes():
    files=sorted(ROOT.glob("*.mesh.json"))+[ROOT/"attachments.json"]
    return {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in files}


def write(name,data):
    (REVIEW/name).write_text(json.dumps(data,indent=2)+"\n",encoding="utf-8")


def co(v):
    x,y,z=v
    return Vector((y/100,-x/100,z/100))


def materials():
    params=[]
    for role,hexcode,metal,rough,coat,emissive in SPECS:
        rgba=(*HS["hex_color"](hexcode),1)
        mat=bpy.data.materials.new("M_Lancer_M02_"+role)
        mat.use_nodes=True
        mat.diffuse_color=rgba
        bsdf=next(n for n in mat.node_tree.nodes if n.type=="BSDF_PRINCIPLED")
        bsdf.inputs["Metallic"].default_value=metal
        bsdf.inputs["Roughness"].default_value=rough
        bsdf.inputs["Coat Weight"].default_value=coat
        bsdf.inputs["Coat Roughness"].default_value=.22
        tint=mat.node_tree.nodes.new("ShaderNodeVertexColor")
        tint.layer_name="PanelTint"
        mix=mat.node_tree.nodes.new("ShaderNodeMixRGB")
        mix.blend_type="MULTIPLY";mix.inputs[0].default_value=1
        mix.inputs[1].default_value=rgba
        mat.node_tree.links.new(tint.outputs["Color"],mix.inputs[2])
        mat.node_tree.links.new(mix.outputs[0],bsdf.inputs["Base Color"])
        if emissive:
            bsdf.inputs["Emission Color"].default_value=rgba
            bsdf.inputs["Emission Strength"].default_value=emissive
        mat["BaseColor_sRGB"]=hexcode
        mat["PanelTint_attribute"]="PanelTint"
        MATERIALS.append(mat)
        params.append(dict(name=mat.name,base_color_srgb=hexcode,metallic=metal,
                           roughness=rough,coat_weight=coat,emissive_strength=emissive,
                           vertex_color_attribute="PanelTint",textures_required=0))
    (PROD/"material_parameters.json").write_text(json.dumps(params,indent=2)+"\n")


def finish(ob,group,mat,bevel=.5,tint=1):
    ob.data.materials.append(MATERIALS[mat])
    color=ob.data.color_attributes.new(name="PanelTint",type="FLOAT_COLOR",domain="CORNER")
    for d in color.data:
        d.color=(tint,tint,tint,1)
    HS["smooth_by_angle"](ob,angle_deg=36)
    if bevel:
        HS["add_bevel"](ob,width=bevel/100,segments=2,angle_deg=38)
        HS["add_weighted_normal"](ob)
    GROUPS.setdefault(group,[]).append(ob)
    return ob


def loft(group,name,sections,mat=0,bevel=.6,tint=1,segments=24,n=3.0):
    # X-forward cm source sections -> Blender -Y nose, metre mesh data.
    ss=[]
    for sec in sections:
        x,w,h,*rest=sec
        y,z=(rest+[0,0])[:2]
        ss.append(dict(y=-x/100,w=w*2/100,h=h*2/100,x=y/100,z=z/100,n=n))
    ob=HS["loft"]("SRC_M02_"+name,ss,ring_verts=segments,collection="M02_Construction")
    return finish(ob,group,mat,bevel,tint)


def solid(group,name,verts,faces,mat=0,bevel=.5,tint=1):
    bm=bmesh.new()
    vv=[bm.verts.new(co(p)) for p in verts]
    for f in faces:
        bm.faces.new([vv[i] for i in f])
    ob=HS["_finish"]("SRC_M02_"+name,bm,"M02_Construction")
    return finish(ob,group,mat,bevel,tint)


def tube(group,name,profile,mat=3,y=0,z=0,segments=24,bevel=.25,tint=1):
    verts=[];faces=[]
    for x,r in profile:
        for i in range(segments):
            a=i*2*math.pi/segments
            verts.append((x,y+math.cos(a)*r,z+math.sin(a)*r))
    for j in range(len(profile)):
        for i in range(segments):
            nj=(j+1)%len(profile);ni=(i+1)%segments
            faces.append((j*segments+i,j*segments+ni,nj*segments+ni,nj*segments+i))
    # The closed radial profile already contains the mechanical chamfers.
    # An extra bevel here multiplies barrel/pod cost without visible benefit.
    return solid(group,name,verts,faces,mat,0,tint)


def bolt(group,name,position,axis="Z",tint=.7):
    # 4cm service fasteners: visible at the intended chase-camera distance.
    ob=HS["cylinder"]("SRC_M02_"+name,radius=.02,depth=.028,segments=6,
                      axis=axis,location=co(position),collection="M02_Construction")
    # Bake position into the part so canonical module origins remain exact.
    HS["apply_transform"](ob,location=True,rotation=True,scale=True)
    return finish(ob,group,3,.15,tint)


WING_STATIONS=[(64,100,-252,28,-5),(125,54,-239,24,-5),
               (218,82,-142,17,-3),(300,182,28,10,1),(334,220,139,5,5)]
AIRFOIL=[(0,.00),(.10,.30),(.36,.50),(.72,.39),(.96,.11),(1,0),
         (.96,-.10),(.72,-.32),(.35,-.43),(.08,-.24)]


def airfoil_part(group,side,stations,name,bevel=.65):
    vertices=[];faces=[]
    for span,lead,trail,thick,z in stations:
        for u,h in AIRFOIL:
            vertices.append((trail+(lead-trail)*u,side*span,z+thick*h))
    n=len(AIRFOIL)
    for s in range(len(stations)-1):
        for j in range(n):
            faces.append((s*n+j,s*n+(j+1)%n,(s+1)*n+(j+1)%n,(s+1)*n+j))
    faces.extend([tuple(reversed(range(n))),tuple(range((len(stations)-1)*n,len(vertices)))])
    return solid(group,name,vertices,faces,0,bevel)


def wing(group,side):
    body=airfoil_part(group,side,WING_STATIONS,"Wing_"+str(side),0)
    fairing=airfoil_part(group,side,[(43,167,-304,76,5),(73,145,-286,62,0),
                         (108,110,-256,44,-4),(142,65,-218,27,-5),(167,54,-195,13,-5)],
                         "SweptRoot_"+str(side),0)
    # A true structural union replaces the intersecting cylinder-like root masses.
    HS["boolean"](body,fairing,operation="UNION",solver="EXACT")
    HS["apply_modifiers"](body)
    GROUPS[group].remove(fairing)
    bpy.data.objects.remove(fairing,do_unlink=True)
    for d in body.data.color_attributes["PanelTint"].data:
        d.color=(1,1,1,1)
    body.data.materials.append(MATERIALS[3])
    body.data.materials.append(MATERIALS[2])
    for face in body.data.polygons:
        centre=sum((body.data.vertices[i].co for i in face.vertices),Vector())/len(face.vertices)
        x,y=-centre.y*100,abs(centre.x*100)
        if face.normal.z<-.45:
            face.material_index=2
        for a,b in zip(WING_STATIONS,WING_STATIONS[1:]):
            if a[0]<=y<=b[0]:
                t=(y-a[0])/(b[0]-a[0]);lead=a[1]+(b[1]-a[1])*t;trail=a[2]+(b[2]-a[2])*t
                u=(x-trail)/(lead-trail)
                if u>.93 or u<.045:
                    face.material_index=1
                break
    HS["add_bevel"](body,width=.005,segments=2,angle_deg=38)
    HS["add_weighted_normal"](body)
    # Cyan inlaid panel follows the actual upper airfoil surface; its back
    # penetrates the supporting skin. A 1.6cm reveal keeps the border readable.
    stations=WING_STATIONS[1:4]
    vv=[]
    for layer in [-.9,.9]:
        for span,lead,trail,thick,z in stations:
            for u,h in [(.57,.436),(.70,.396)]:
                vv.append((trail+(lead-trail)*u,side*span,z+thick*h+layer))
    ff=[(0,2,3,1),(2,4,5,3),(6,7,9,8),(8,9,11,10),
        (0,1,7,6),(4,10,11,5),(0,6,8,2),(2,8,10,4),
        (1,3,9,7),(3,5,11,9)]
    solid(group,"WingInlay_"+str(side),vv,ff,1,.25,.92)
    surface_patch(group,side,"ServiceGasket",[193,232,272],.168,.442,-.6,.35,2,1)
    surface_patch(group,side,"ServicePanel",[195,232,270],.18,.43,.1,1.4,0,.83)
    for y in [208,257]:
        x,z=wing_surface(y,.30)
        bolt(group,"WingPanelLatch"+str(side)+str(y),(x,side*y,z+1.7),tint=.65)


def wing_surface(y,u):
    for a,b in zip(WING_STATIONS,WING_STATIONS[1:]):
        if a[0]<=y<=b[0]:
            t=(y-a[0])/(b[0]-a[0])
            lead,trail,thick,z=[a[k]+(b[k]-a[k])*t for k in [1,2,3,4]]
            for p,q in zip(AIRFOIL[:6],AIRFOIL[1:6]):
                if p[0]<=u<=q[0]:
                    h=p[1]+(q[1]-p[1])*(u-p[0])/(q[0]-p[0])
                    return trail+(lead-trail)*u,z+thick*h
    raise ValueError("Wing surface sample out of range")


def surface_patch(group,side,name,spans,u0,u1,bottom,top,material,tint):
    # Include each supporting airfoil knot so the shallow plate cannot cut
    # through the wing hump between two endpoint samples.
    spans=sorted(set(spans+[s[0] for s in WING_STATIONS if spans[0]<s[0]<spans[-1]]))
    us=sorted(set([u0,u1]+[p[0] for p in AIRFOIL[:6] if u0<p[0]<u1]))
    vv=[]
    for layer in [bottom,top]:
        for y in spans:
            for u in us:
                x,z=wing_surface(y,u)
                vv.append((x,side*y,z+layer))
    rows,cols=len(spans),len(us);n=rows*cols;ff=[]
    for r in range(rows-1):
        for c in range(cols-1):
            a=r*cols+c;b=a+1;d=(r+1)*cols+c;e=d+1
            ff.extend([(a,b,e,d),(a+n,d+n,e+n,b+n)])
    boundary=list(range(cols))+[r*cols+cols-1 for r in range(1,rows)]+list(range(n-2,n-cols-1,-1))+[r*cols for r in range(rows-2,0,-1)]
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        ff.append((a,a+n,b+n,b))
    return solid(group,name+str(side),vv,ff,material,.15,tint)


def unit_marking():
    font=bpy.data.curves.new("SRC_M02_UnitMarking","FONT")
    font.body="07";font.size=.16;font.extrude=.0015;font.resolution_u=4
    ob=bpy.data.objects.new("SRC_M02_UnitMarking",font)
    bpy.data.collections["M02_Construction"].objects.link(ob)
    ob.location=co((-55,240,0));ob.rotation_euler.z=-math.pi/2
    name=ob.name
    bpy.context.view_layer.objects.active=ob
    ob.select_set(True)
    with bpy.context.temp_override(object=ob,active_object=ob,selected_objects=[ob],selected_editable_objects=[ob]):
        bpy.ops.object.convert(target="MESH")
    ob=bpy.data.objects[name]
    HS["apply_transform"](ob,location=True,rotation=True,scale=True)
    for v in ob.data.vertices:
        x,y=-v.co.y*100,v.co.x*100
        for a,b in zip(WING_STATIONS,WING_STATIONS[1:]):
            if a[0]<=y<=b[0]:
                t=(y-a[0])/(b[0]-a[0])
                lead,trail,thick,z=[a[k]+(b[k]-a[k])*t for k in [1,2,3,4]]
                u=(x-trail)/(lead-trail)
                for p,q in zip(AIRFOIL[:6],AIRFOIL[1:6]):
                    if p[0]<=u<=q[0]:
                        h=p[1]+(q[1]-p[1])*(u-p[0])/(q[0]-p[0])
                        v.co.z+=(z+thick*h+.07)/100
                        break
                break
    ob.data.update();HS["smart_uv"](ob)
    finish(ob,"Hull",2,0,.9)


def build_hull():
    g="Hull"
    loft(g,"Core",[(-347,44,23),(-280,60,29),(-160,70,35),(-45,72,37),
         (88,59,32),(200,40,22),(300,22,12),(375,6,4),(400,1.6,1.8)],2,.7,n=3.6)
    deck=[(-326,40,17,0,17),(-270,51,23,0,19),(-175,57,27,0,21),
         (-60,60,29,0,23),(66,49,25,0,26),(160,37,20,0,20),(255,21,13,0,13),
         (340,9,6,0,6),(388,2,2,0,1)]
    under=[(x,w-1.5,h-1.5,y,z) for x,w,h,y,z in deck]
    under[0]=(under[0][0]+1.3,*under[0][1:])
    under[-1]=(under[-1][0]-1.3,*under[-1][1:])
    loft(g,"DeckUnderlayer",under,2,.1,n=3.1)
    def interpolate(x):
        for a,b in zip(deck,deck[1:]):
            if a[0]<=x<=b[0]:
                t=(x-a[0])/(b[0]-a[0])
                return tuple([x]+[a[k]+(b[k]-a[k])*t for k in range(1,5)])
        return deck[0] if x<deck[0][0] else deck[-1]
    cuts=[(-326,-230.8),(-229.2,-110.8),(-109.2,69.2),(70.8,224.2),(225.8,388)]
    for i,(start,end) in enumerate(cuts):
        section=[interpolate(start)]+[p for p in deck if start<p[0]<end]+[interpolate(end)]
        loft(g,"DeckPanel"+str(i),section,0,.45,[.81,.94,1,.88,.98][i],n=3.1)
    loft(g,"VentralArmour",[(-295,28,10,0,-23),(-120,38,12,0,-30),(40,33,12,0,-34),
         (170,20,8,0,-22),(310,8,4,0,-9)],3,.7,.58)
    # Split deck service covers create broad panel value variation, not noise.
    loft(g,"AftServiceCover",[(-302,22,6,0,37),(-248,25,7,0,41),(-177,27,8,0,46),(-124,23,4,0,47)],0,.45,.81)
    loft(g,"NoseSensorSeat",[(326,9,4,0,-3),(375,5.5,3,0,-3),(397,2,1.6,0,-3)],3,.25,.75)
    loft(g,"NoseSensor",[(388,1.6,1.6,0,0),(401,.8,.8,0,0)],5,0)
    loft(g,"CanopyGasket",[(-84,40,10,0,44),(-35,52,18,0,51),(59,45,17,0,54),(132,22,5,0,46)],2,.45)
    loft(g,"CanopyFrame",[(-79,38,9,0,48),(-32,50,16,0,55),(58,43,15,0,58),(128,20,4,0,50)],3,.45,.73)
    loft(g,"CanopyGlass",[(-73,34,8,0,52),(-29,46,14,0,59),(57,39,13,0,62),(121,17,3,0,54)],4,.4,n=2.8)
    for side in [-1,1]:
        loft(g,"CanopyHingeLug"+str(side),[(-80,5,7,side*42,52),(-42,6,8,side*42,54)],2,.35,n=4)
        hinge=HS["cylinder"]("SRC_M02_CanopyHinge"+str(side),radius=.035,depth=.08,segments=16,
                             axis="X",location=co((-61,side*47,54)),collection="M02_Construction")
        HS["apply_transform"](hinge,location=True,rotation=True,scale=True)
        finish(hinge,g,3,.2,.6)
    # Top spine contains the overdrive insert; ability socket remains untouched.
    loft(g,"DriveSpine",[(-315,16,9,0,37),(-252,19,10,0,43),(-175,20,10,0,49),(-101,16,6,0,47)],2,.5)
    loft(g,"DriveSpineInlay",[(-287,4,1.6,0,48),(-245,4.8,1.8,0,53),(-174,4.8,1.8,0,59),(-127,3,1.5,0,54)],5,.12)
    for side in [-1,1]:
        tag="L" if side<0 else "R"
        wing(g,side)
        loft(g,"IntakeBed_"+tag,[(40,11,5,side*72,17),(91,12,5,side*65,17),(115,8,4,side*58,14)],2,.35)
        for j in range(4):
            x=46+j*14
            y=side*(72-(x-40)*.14)
            loft(g,"IntakeSlat_"+tag+str(j),[(x,9,1.8,y,21),(x+6,8,1.8,y,21)],3,.18,.58,segments=8,n=4)
        # A tapered chine flows into the primary mount, hiding the square joint.
        cradle=loft(g,"PrimaryFairing_"+tag,[(-110,15,15,side*100,-11),(-45,24,24,side*100,-20),
             (15,27,26,side*100,-25),(58,24,26,side*100,-26),(77,19,20,side*100,-27)],0,0,.89)
        cutter=HS["cylinder"]("CUT_M02_WeaponBore"+tag,radius=.224,depth=1.4,segments=32,
                              axis="Y",location=co((55,side*100,-57)),collection="M02_Construction")
        HS["boolean"](cradle,cutter,operation="DIFFERENCE",solver="EXACT")
        HS["apply_modifiers"](cradle)
        bpy.data.objects.remove(cutter,do_unlink=True)
        for d in cradle.data.color_attributes["PanelTint"].data:
            d.color=(.89,.89,.89,1)
        HS["add_bevel"](cradle,width=.005,segments=2,angle_deg=38)
        HS["add_weighted_normal"](cradle)
        loft(g,"PrimaryHardpoint_"+tag,[(-10,12,14,side*100,-29),(28,16,17,side*100,-31),
             (61,10,13,side*100,-27)],3,.55,.65)
        loft(g,"PodPylon_"+tag,[(-143,16,18,side*220,-16),(-110,20,23,side*220,-22),
             (-67,17,22,side*220,-22),(-38,10,14,side*220,-16)],0,.65,.86)
        loft(g,"Nacelle_"+tag,[(-373,29,28,side*111,-3),(-345,45,42,side*111,-3),
             (-284,48,43,side*111,-1),(-214,44,39,side*111,1),(-151,32,29,side*111,3),
             (-105,14,16,side*111,5)],0,.7,segments=32,n=2.5)
        loft(g,"NacelleUpperPanel_"+tag,[(-332,24,7,side*111,34),(-279,31,8,side*111,41),
             (-211,26,7,side*111,38),(-166,16,4,side*111,29)],3,.4,.55,n=4.5)
        for j,x in enumerate([-303,-269,-230]):
            for dy in [-15,15]:
                panel=[(-332,24,7,34),(-279,31,8,41),(-211,26,7,38),(-166,16,4,29)]
                for a,b in zip(panel,panel[1:]):
                    if a[0]<=x<=b[0]:
                        t=(x-a[0])/(b[0]-a[0])
                        w,h,centre=[a[k]+(b[k]-a[k])*t for k in [1,2,3]]
                        z=centre+h*(1-(abs(dy)/w)**4.5)**(1/4.5)+.3
                        break
                bolt(g,"NacelleFastener"+tag+str(j)+str(dy),(x,side*111+dy,z))
        tube(g,"NozzleBell_"+tag,[(-399,25),(-389,33),(-363,37),(-350,35),(-352,29),(-378,25),(-399,21.5)],3,side*111,-3,32,.3,.78)
        tube(g,"NozzleCeramic_"+tag,[(-394,21.9),(-365,26.5),(-357,25),(-357,17.5),(-385,17.5),(-394,17.5)],2,side*111,-3,32,.15)
        tube(g,"NozzleEmitter_"+tag,[(-396.5,22.2),(-393.5,22.2),(-393.5,18.2),(-396.5,18.2)],5,side*111,-3,32,.1)
        loft(g,"JetBackplate_"+tag,[(-370,19,19,side*111,-3),(-363,19,19,side*111,-3)],3,.15,.35,segments=24,n=2)
        # Twelve heat-resistant petals around each exhaust, touching the bell.
        for i in range(12):
            a=i*2*math.pi/12
            y=side*111+math.cos(a)*31.8;z=-3+math.sin(a)*31.8
            petal=loft(g,"NozzlePetal_"+tag+str(i),[(-395,5,2.8,y,z),(-383,6,3,y,z),(-369,5,2.3,y,z)],3,.22,.5 if i%2 else .86,segments=8,n=4)
            ca,sa=math.cos(a+math.pi/2),math.sin(a+math.pi/2)
            for vertex in petal.data.vertices:
                dy,dz=vertex.co.x*100-y,vertex.co.z*100-z
                vertex.co.x=(y+ca*dy-sa*dz)/100
                vertex.co.z=(z+sa*dy+ca*dz)/100
            petal.data.update()
        # Raked stabilizer, thinner and longer than the blockout paddle.
        loft(g,"Fin_"+tag,[(-345,3,12,side*111,44),(-288,4.8,29,side*116,60),
             (-235,4,15,side*119,48),(-187,2,3,side*117,32)],0,.55,segments=12,n=4)
        loft(g,"FinHeel_"+tag,[(-330,7,7,side*111,38),(-260,8,8,side*114,40),(-199,5,5,side*116,31)],1,.4,.9,n=4)
        # Radiator blocks are seated inside the shoulder, removing M01 floaters.
        loft(g,"RadiatorBed_"+tag,[(-238,11,3,side*43,43),(-221,15,4,side*43,46),
             (-131,14,4,side*43,46),(-110,10,2,side*43,43)],2,.3,n=4)
        for dy in [-13,13]:
            loft(g,"RadiatorFrame_"+tag+str(dy),[(-229,2,3,side*43+dy,49),
                 (-130,2,3,side*43+dy,49)],0,.25,.86,segments=8,n=4)
        for i in range(6):
            x=-219+i*17
            loft(g,"RadiatorFin_"+tag+str(i),[(x,10.8,1.8,side*43,50.3),
                 (x+4,10.8,1.8,side*43,50.3)],3,.18,.62,segments=8,n=4)
        # Narrow wing livery and spine/drive emitters provide restrained colour;
        # no floating or coplanar decorative chine strip is required.


def build_weapons():
    g="Mount"
    loft(g,"MountFoot",[(-38,19,4,0,3),(-31,25,7,0,3),(25,25,7,0,3),(34,20,4,0,3)],3,.45,.64,n=4)
    loft(g,"MountCradle",[(-25,13,7,0,-7),(20,14,7,0,-7),(27,10,4,0,-7)],2,.4,n=4)
    g="Primary"
    loft(g,"Receiver",[(-44,14,14,0,-16),(-31,22,21,0,-16),(17,22,21,0,-16),(32,16,17,0,-16)],2,.6,n=4)
    loft(g,"ReceiverArmour",[(-37,18,5,0,1),(-18,21,6,0,1),(21,16,4,0,1)],0,.4,.88,n=4)
    for side in [-1,1]:
        loft(g,"ReceiverSide"+str(side),[(-28,3,11,side*20,-17),(10,3.5,12,side*20,-17),(24,2,7,side*18,-17)],3,.3,.62,segments=8,n=4)
        for j in range(3):
            x=-20+j*11
            loft(g,"ReceiverVent"+str(side)+str(j),[(x,2.6,5,side*23,-17),(x+5,2.6,5,side*23,-17)],2,.18,segments=8,n=4)
    tube(g,"BearingHousing",[(20.3,19),(28,23),(37,22),(37,15),(20.3,15)],3,0,-15,32,.25,.84)
    g="Rotor"
    # Exact original axial envelope and muzzle points are preserved.
    tube(g,"Carrier",[(-7,18.5),(1,20),(11,18.8),(11,4.8),(-7,4.8)],3,0,0,32,.2,.64)
    loft(g,"Axle",[(-8,6.1,6.1),(123,6.1,6.1)],3,.18,.45,segments=16,n=2)
    for i in range(6):
        a=i*math.pi/3;y,z=12*math.cos(a),12*math.sin(a)
        barrel=tube(g,"Barrel"+str(i),[(4,4.1),(120,4.1),(134,4.3),(136,3.9),(136,2.5),(4,2.5)],3,y,z,16,.12,.83)
        barrel.data.materials.append(MATERIALS[2])
        for face in barrel.data.polygons:
            x=sum(-barrel.data.vertices[v].co.y*100 for v in face.vertices)/len(face.vertices)
            if x>125:
                face.material_index=1
    tube(g,"FrontClamp",[(101,20),(106,21),(118,21),(122,19),(122,15),(101,15)],3,0,0,32,.25,.56)
    g="Secondary"
    loft(g,"PodSpine",[(-66,10,24,0,-22),(-57,12,30,0,-22),(80,12,30,0,-22),(100,8,25,0,-22)],2,.5,n=4)
    loft(g,"PodTopArmour",[(-63,28,4,0,5),(-48,34,5,0,6),(75,34,5,0,6),(98,27,3,0,5)],0,.45,n=4)
    for side in [-1,1]:
        loft(g,"PodCheek"+str(side),[(-58,3.5,17,side*33,-24),(-35,5,19,side*33,-24),
             (57,5,19,side*33,-24),(91,3,14,side*33,-24)],0,.55,.85,n=4)
        loft(g,"PodCyanInlay"+str(side),[(-37,1.7,4,side*37,-24),(44,1.7,4,side*37,-24),(64,1.4,2,side*36,-24)],1,.2)
        for x in [-28,53]:
            for z in [-12,-35]:
                bolt(g,"PodFastener"+str(side)+str(x)+str(z),(x,side*37.4,z),axis="X",tint=.9)
    for i,(y,z) in enumerate([(y,z) for y in [-16,16] for z in [-9,-39]]):
        launch=tube(g,"LaunchTube"+str(i),[(-59,13.6),(86,13.6),(101,12.8),(108,12),(108,10),(-59,10)],3,y,z,24,.2,.65)
        launch.data.materials.append(MATERIALS[2])
        for face in launch.data.polygons:
            r=sum(math.hypot(launch.data.vertices[v].co.x*100-y,launch.data.vertices[v].co.z*100-z) for v in face.vertices)/len(face.vertices)
            if r<10.5:
                face.material_index=1
        tube(g,"MuzzleCollar"+str(i),[(88,14.1),(98,14.1),(105,12.9),(105,11.4),(88,11.4)],3,y,z,24,.2,.95)
        loft(g,"LoadedNose"+str(i),[(57,7.3,7.3,y,z),(77,6.7,6.7,y,z),(90,1.4,1.4,y,z)],1,.15,segments=16,n=2)
    g="Projectile"
    # Reuse original mesh shape; volume/egress decision is not silently changed.
    src=json.loads((ROOT/"SM_Lancer_SpikeRocket.mesh.json").read_text())
    for p in src["parts"]:
        solid(g,p["name"],p["vertices"],p["triangles"],p["material"],.08)


def combine():
    meshes={}
    for group,parts in GROUPS.items():
        ob=HS["join"](parts,name="SM_Lancer_M02_"+group,apply_mods=True)
        bm=bmesh.new();bm.from_mesh(ob.data)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-5)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-5)
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(ob.data);bm.free();ob.data.update()
        HS["smooth_by_angle"](ob,angle_deg=36)
        HS["add_weighted_normal"](ob)
        # Collapse only coincident points created by helper cap/side seams.
        # No decimation changes the approved shape; each source is then triangulated.
        tri=ob.modifiers.new("GameTriangulation","TRIANGULATE")
        tri.keep_custom_normals=True
        HS["apply_modifiers"](ob)
        # Boolean/bevel caps can produce sub-0.01 mm triangulation slivers.
        # Weld and dissolve at the QA duplicate tolerance, then re-triangulate;
        # the manifold/contact gates still run on the actual delivered result.
        bm=bmesh.new();bm.from_mesh(ob.data)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-5)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-5)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        bm.to_mesh(ob.data);bm.free();ob.data.update()
        HS["smooth_by_angle"](ob,angle_deg=36)
        HS["add_weighted_normal"](ob)
        HS["apply_modifiers"](ob)
        HS["smart_uv"](ob,angle_deg=60,island_margin=.015)
        uv=ob.data.uv_layers.active.data
        tiny=[]
        for face in ob.data.polygons:
            points=[uv[i].uv.copy() for i in face.loop_indices]
            area=abs(sum(a.x*b.y-b.x*a.y for a,b in zip(points,points[1:]+points[:1])))*.5
            if area<1e-12:
                tiny.append(face)
        if tiny:
            # Reserve a separate strip for micro-chamfer UV islands. These tiny
            # faces must have real UV area even though the finish uses vertex tint.
            for d in uv:
                d.uv.x*=.93
            for i,face in enumerate(tiny):
                x=.942+(i%4)*.013;y=.01+(i//4)*.013
                if y>.98:
                    raise RuntimeError("Too many UV slivers; repair source topology")
                for li,value in zip(face.loop_indices,[(x,y),(x+.009,y),(x,y+.009)]):
                    uv[li].uv=value
            ob["micro_uv_islands_repaired"]=len(tiny)
        meshes[group]=ob
    return meshes


def assemble(meshes):
    manifest=json.loads((ROOT/"attachments.json").read_text())
    asset_group={"SM_Ship_Lancer_Proxy":"Hull","SM_Lancer_UniversalMount":"Mount",
                 "SM_Lancer_MinigunBody":"Primary","SM_Lancer_MinigunRotor":"Rotor",
                 "SM_Lancer_RocketPod":"Secondary","SM_Lancer_SpikeRocket":"Projectile"}
    coll=bpy.data.collections.new("ASSET_Lancer_M02")
    bpy.context.scene.collection.children.link(coll)
    obs={}
    for group,ob in meshes.items():
        ob.name="SRC_CANONICAL_M02_"+group
        ob.data.use_fake_user=True
    for inst in manifest["instances"]:
        source=meshes[asset_group[inst["asset"]]]
        ob=bpy.data.objects.new("SM_Lancer_M02_"+inst["name"],source.data)
        coll.objects.link(ob)
        ob.location=co(inst["position_cm"])
        ob["instance_name"]=inst["name"]
        ob["canonical_asset_group"]=asset_group[inst["asset"]]
        obs[inst["name"]]=ob
    # Remove canonical construction objects, keeping their shared mesh datablocks.
    for ob in meshes.values():
        bpy.data.objects.remove(ob,do_unlink=True)
    for spec in manifest["sockets"]:
        ob=bpy.data.objects.new("SOCKET_"+spec["parent"]+"_"+spec["name"],None)
        coll.objects.link(ob);ob.parent=obs[spec["parent"]]
        ob.location=co(spec["position_cm"])
        forward=co([100*x for x in spec["forward"]]).normalized()
        up=co([100*x for x in spec["up"]]).normalized()
        right=up.cross(forward).normalized()
        ob.rotation_euler=Matrix((right,-forward,up)).transposed().to_euler()
        ob.empty_display_type="ARROWS";ob.empty_display_size=.12
        ob["unreal_socket_name"]=spec["name"]
    derived={"status":"source_handoff_only_no_native_assets","primary_mesh_source":"SM_Lancer_M02_Hull",
             "engineering_contract":"UTDShipVisuals.Mesh plus FTDShipVisualPart array; flattened root transforms",
             "engineering_reply_commit":"8cd1561","parts":[],
             "spike_rocket_visual_offset_cm":[54,0,0],"collision_and_muzzle_changed":False,
             "dynamic_parenting_implemented":False,"canonical_attachment_manifest_modified":False}
    for inst in manifest["instances"]:
        if inst["name"]=="Hull":
            continue
        derived["parts"].append({"PartName":inst["name"],"MeshSource":"SM_Lancer_M02_"+asset_group[inst["asset"]],
                                 "UnrealAssetPath":None,"ParentSocket":None,
                                 "RelativeTransform":{"translation_cm":inst["position_cm"],
                                                      "rotation_degrees":[0,0,0],"scale":[1,1,1]}})
    (PROD/"unreal_visual_parts.json").write_text(json.dumps(derived,indent=2)+"\n")
    bpy.context.view_layer.update()
    return coll,obs


def build():
    start=time.perf_counter();before=source_hashes()
    if not bpy.app.background or bpy.data.filepath:
        raise RuntimeError("Requires a new background factory-startup process")
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob,do_unlink=True)
    bpy.context.scene.unit_settings.system="METRIC"
    bpy.context.scene.unit_settings.scale_length=1
    bpy.context.scene.unit_settings.length_unit="METERS"
    materials();build_hull();unit_marking();build_weapons()
    meshes=combine();coll,obs=assemble(meshes)
    report={"revision":"M02","blender":bpy.app.version_string,"m01_hashes_before":before,
            "m01_hashes_after":source_hashes(),"m01_unchanged":before==source_hashes(),
            "assembly_objects":len(obs),"triangles":sum(len(o.data.polygons) for o in obs.values()),
            "materials":len(MATERIALS),"elapsed_seconds":time.perf_counter()-start,
            "final_visual_approval":False,"unreal_imported":False}
    write("build_manifest.json",report)
    if not report["m01_unchanged"]:
        raise RuntimeError("Historical prototype was modified")
    bpy.ops.wm.save_as_mainfile(filepath=str(PROD/"Lancer_M02_work.blend"))
    print("M02_BUILD "+json.dumps(report),flush=True)


if __name__=="__main__":
    build()
