"""Fitted service hardware and deliberate painted-armor separation on preserved M05."""
from pathlib import Path
import os,json,hashlib,runpy,math,shutil
import bpy,bmesh
from mathutils import Vector
from mathutils.bvhtree import BVHTree

P=Path(__file__).resolve().parent;ROOT=P.parents[1];REPO=ROOT.parents[3]
OUT=ROOT/'review/M03/Detail02';SOURCE=P/'Lancer_M03_Detail01_v03.blend'
SHA='29c037ccf2353ba4e9ebf70e9e8a3d14eed0166ce86ca5a7a6f062053bdf6aba'
FINAL=P/'Lancer_M03_Detail02_v01.blend';WORK=P/'Lancer_M03_Detail02_work.blend'
PREFIX='M_LancerM03_'

def write(name,data):(OUT/name).write_text(json.dumps(data,indent=2),encoding='utf-8')
def digest(data):return hashlib.sha256(json.dumps(data,separators=(',',':'),sort_keys=True).encode()).hexdigest()
def signature(ob):
    record=dict(world=[list(r) for r in ob.matrix_world],local=[list(r) for r in ob.matrix_local],parent=ob.parent.name if ob.parent else None)
    if ob.type=='MESH':
        m=ob.data
        record.update(vertices=[list(v.co) for v in m.vertices],faces=[list(f.vertices) for f in m.polygons],normals=[list(f.normal) for f in m.polygons],smooth=[f.use_smooth for f in m.polygons],uv=[[list(d.uv) for d in a.data] for a in m.uv_layers],indices=[f.material_index for f in m.polygons])
    return digest(record)

def run():
    assert bpy.app.background and hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SHA
    assert not FINAL.exists() and not WORK.exists() and not (OUT/'source_record.json').exists()
    OUT.mkdir(parents=True,exist_ok=True)
    os.environ['LANCER_M03_REVIEW_DIR']=str(OUT);os.environ['LANCER_M03_APPROVED_FRAME']='A02'
    os.environ['LANCER_M03_SERVICE_VIEWS']='1'
    os.environ['LANCER_M03_MATERIAL_PARAMS']=str(OUT/'material_parameters.json')
    os.environ.pop('LANCER_M03_MATERIAL_INTENT',None)
    os.environ['LANCER_M03_DETAIL02_INTENT']=str(OUT/'detail02_material_intent.json')
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE));bpy.context.view_layer.update()
    old=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.get('part_name')]
    frames=[o for o in bpy.context.scene.objects if o.type=='EMPTY' and o.name.startswith('SOCKET_')]
    base=[o for o in old if not o.get('detail_for')]
    assert len(old)==20 and len(base)==12 and len(frames)==36
    original={o.name:signature(o) for o in old+frames}
    before_roles={o.name:[o.data.materials[f.material_index].name for f in o.data.polygons] for o in old}
    base_colors={o.name:digest([[list(d.color) for d in a.data] for a in o.data.color_attributes]) for o in base}
    model=runpy.run_path(str(P/'build_m03.py'));hs=model['HS']
    mats=[bpy.data.materials[PREFIX+r] for r in ['Graphite','Gunmetal','Titanium','Composite','Canopy','Blue']]
    model['MATS'].extend(mats)
    graphite=mats[0];bs=next(n for n in graphite.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    mix=next(n for n in graphite.node_tree.nodes if n.type=='MIX_RGB')
    material_change=dict(before_base_linear=list(mix.inputs[1].default_value),before_specular=bs.inputs['Specular IOR Level'].default_value)
    mix.inputs[1].default_value=(*model['rgb']('#24282D'),1)
    bs.inputs['Specular IOR Level'].default_value=.35
    material_change.update(after_base_linear=list(mix.inputs[1].default_value),after_specular=bs.inputs['Specular IOR Level'].default_value,
        reason='Less blue charcoal paint with F0 about0.028; satin roughness.56/zero coat retained. Exposed metal/glass/cyan unchanged.')
    params=json.loads((ROOT/'review/M03/Detail01/Material05/material_parameters.json').read_text())
    next(s for s in params if s['name']==graphite.name).update(base_color_srgb='#24282D',specular_ior_level=.35)
    write('material_parameters.json',params);write('paint_response_change.json',material_change)
    # Remove the blanket exposed-metal outline on painted detail chamfers only.
    armor_parts={'Detail_Hull','Detail_Wing_L','Detail_Wing_R','Detail_Engine_L','Detail_Engine_R'}
    done=set();finish_records=[]
    for ob in old:
        if ob['part_name'] not in armor_parts or ob.data.as_pointer() in done:continue
        done.add(ob.data.as_pointer());mesh=ob.data
        for i,m in enumerate(mesh.materials):
            if m==mats[2]:mesh.materials[i]=graphite
        # Per-island finishes follow actual armor pieces, never noise per triangle.
        adjacency=[set() for v in mesh.vertices]
        for e in mesh.edges:
            a,b=e.vertices;adjacency[a].add(b);adjacency[b].add(a)
        remaining=set(range(len(mesh.vertices)));islands=[]
        while remaining:
            seed=min(remaining);stack=[seed];island=set();remaining.remove(seed)
            while stack:
                v=stack.pop();island.add(v)
                for n in adjacency[v]&remaining:remaining.remove(n);stack.append(n)
            islands.append(island)
        islands.sort(key=lambda ids:tuple(round(sum(mesh.vertices[i].co[k] for i in ids)/len(ids),5) for k in range(3)))
        palette=[(.58,.78),(.78,.40),(.66,.62),(.88,.30),(.72,.55)]
        attr=mesh.color_attributes['PanelTint'];vertex_island={v:i for i,ids in enumerate(islands) for v in ids}
        changed=0
        for f in mesh.polygons:
            if mesh.materials[f.material_index]!=graphite:continue
            tint,alpha=palette[vertex_island[f.vertices[0]]%len(palette)]
            for li in f.loop_indices:attr.data[li].color=(tint,tint,tint,alpha)
            changed+=1
        finish_records.append(dict(object=ob.name,islands=len(islands),painted_faces=changed,finish_palette=palette))
    write('panel_finish_changes.json',finish_records)
    # Sample actual existing wing geometry, including the existing overlay layer.
    wing_bvhs={}
    for label in ['L','R']:
        targets=[o for o in old if o.get('part_name')=='Wing_'+label or o.get('detail_for')=='Wing_'+label]
        verts=[];faces=[]
        for o in targets:
            offset=len(verts);verts.extend(o.matrix_world@v.co for v in o.data.vertices)
            faces.extend(tuple(offset+i for i in f.vertices) for f in o.data.polygons)
        wing_bvhs[label]=BVHTree.FromPolygons(verts,faces,all_triangles=True)
    fits=[]
    def roof(label,x,y):
        hit,normal,_,_=wing_bvhs[label].ray_cast(Vector((y/100,-x/100,3)),Vector((0,0,-1)),6)
        if hit is None or normal.z<.4:raise RuntimeError('Service panel misses actual wing roof')
        fits.append(dict(label=label,x_cm=x,y_cm=y,z_cm=hit.z*100,normal_z=normal.z))
        return hit.z*100
    def fitted(label,coords,bottom,top):
        return [(x,y,roof(label,x,y)+dz) for dz in [bottom,top] for x,y in coords]
    def plate(group,name,label,coords,bottom,top,mat,tint=.8,rough=.5,bevel=.12):
        n=len(coords);vv=fitted(label,coords,bottom,top)
        ff=[tuple(reversed(range(n))),tuple(range(n,n*2))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
        return model['solid'](group,name,vv,ff,mat,tint,bevel,rough)
    def ring(group,name,label,outer,inner,bottom,top,mat):
        vv=fitted(label,outer,bottom,top)+fitted(label,inner,bottom,top);n=len(outer);ff=[]
        for i in range(n):
            j=(i+1)%n;ff.extend([(i,j,j+n,i+n),(2*n+i,3*n+i,3*n+j,2*n+j),(i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)])
        return model['solid'](group,name,vv,ff,mat,.68,.12,.65)
    def octagon(x0,x1,y0,y1,chamfer):
        c=chamfer;return [(x0+c,y0),(x1-c,y0),(x1,y0+c),(x1,y1-c),(x1-c,y1),(x0+c,y1),(x0,y1-c),(x0,y0+c)]
    def bolt(group,name,point,axis='Z'):
        ob=hs['cylinder']('SRC_Detail02_'+name,radius=.010,depth=.010,segments=8,axis=axis,location=model['co'](point),collection='Detail02_Construction')
        model['finish'](ob,group,2,.5,.06,.7)
    for side,label in [(-1,'L'),(1,'R')]:
        group='Service_Wing_'+label
        # Hatch coordinates follow the swept wing panel's parametric surface.
        def hatch(y,u):
            x,_=model['wing_surface'](y,u);return x,side*y
        outer=[hatch(y,u) for u,y in [(.31,206),(.49,206),(.52,214),(.52,248),(.49,256),(.31,256),(.28,248),(.28,214)]]
        inner=[hatch(y,u) for u,y in [(.34,215),(.46,215),(.48,220),(.48,242),(.46,247),(.34,247),(.32,242),(.32,220)]]
        ring(group,'WingAccessFrame'+label,label,outer,inner,-.6,1.3,1)
        center=[hatch(y,u) for u,y in [(.345,216),(.455,216),(.475,221),(.475,241),(.455,246),(.345,246),(.325,241),(.325,221)]]
        plate(group,'WingAccessCover'+label,label,center,-.6,.4,0,.62,.8)
        for i,(y,u) in enumerate([(210,.4),(252,.4),(231,.30),(231,.50)]):
            x,yy=hatch(y,u);bolt(group,'HatchFastener'+label+str(i),(x,yy,roof(label,x,yy)+1.3))
        latch=[hatch(y,u) for u,y in [(.375,225),(.425,225),(.425,231),(.375,231)]]
        plate(group,'AccessLatch'+label,label,latch,-.4,.95,3,.85,.75)
        # A compact six-fin thermal grille is clustered on each intake shoulder.
        outer=octagon(-140,-101,side*110-15,side*110+15,3)
        inner=octagon(-135,-106,side*110-11,side*110+11,2)
        ring(group,'ShoulderGrilleFrame'+label,label,outer,inner,-.7,1.2,1)
        plate(group,'GrillePocket'+label,label,inner,-.7,.25,3,.9,.8)
        for i,x in enumerate([-131,-126.5,-122,-117.5,-113]):
            coords=[(x-1,side*110-10),(x+1,side*110-10),(x+1,side*110+10),(x-1,side*110+10)]
            plate(group,'ThermalFin'+label+str(i),label,coords,-.3,.95,1,.72,.75,.10)
    # Axial service pipes and secured saddles follow the actual engine's designed flank.
    def width(x):
        for a,b in zip([(3,38),(31,46),(118,46),(191,38)],[(31,46),(118,46),(191,38),(215,27)]):
            if a[0]<=x<=b[0]:return a[1]+(b[1]-a[1])*(x-a[0])/(b[0]-a[0])
        raise ValueError(x)
    group='Service_Engine'
    for side in [-1,1]:
        model['loft'](group,'EngineServiceSaddle'+str(side),[(50,3,11,side*44.5,2),(108,3,11,side*44.5,3),(164,3,10,side*40,4)],1,.6,.3,.7)
        for j,z in enumerate([-4,5]):
            vv=[];ff=[];xs=[59,108,118,157];n=12
            for x in xs:
                for i in range(n):
                    a=i*math.tau/n;vv.append((x,side*(width(x)+2)+1.5*math.cos(a),z+1.5*math.sin(a)))
            for k in range(len(xs)-1):
                for i in range(n):ff.append((k*n+i,k*n+(i+1)%n,(k+1)*n+(i+1)%n,(k+1)*n+i))
            ff.extend([tuple(reversed(range(n))),tuple(range(len(vv)-n,len(vv)))])
            model['solid'](group,'CoolantPipe'+str(side)+str(j),vv,ff,2,.58,0,.65)
        for i,x in enumerate([73,117,151]):
            w=width(x)
            model['loft'](group,'PipeRetainer'+str(side)+str(i),[(x-2,3.5,9,side*(w+1),1),(x+2,3.5,9,side*(w+1),1)],3,.9,.15,.8)
            bolt(group,'RetainerBolt'+str(side)+str(i),(x,side*(w+4.2),6),'X')
    material_checks=[];new=[]
    destinations={'Service_Wing_L':['Wing_L'],'Service_Wing_R':['Wing_R'],'Service_Engine':['Engine_L','Engine_R']}
    for group,parts in model['GROUPS'].items():
        if not parts:continue
        ob=hs['join'](parts,name='SRC_Detail02_'+group,apply_mods=True)
        before=[ob.data.materials[f.material_index].name for f in ob.data.polygons]
        bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free();ob.data.update()
        hs['smooth_by_angle'](ob,angle_deg=35);hs['add_weighted_normal'](ob);hs['apply_modifiers'](ob);hs['smart_uv'](ob,angle_deg=60,island_margin=.014)
        # Compact slots only after recording each polygon's intended role.
        expected=[ob.data.materials[f.material_index].name for f in ob.data.polygons];slots=list(dict.fromkeys(expected))
        ob.data.materials.clear()
        for name in slots:ob.data.materials.append(bpy.data.materials[name])
        indices={name:i for i,name in enumerate(slots)}
        for f,name in zip(ob.data.polygons,expected):f.material_index=indices[name]
        assert [ob.data.materials[f.material_index].name for f in ob.data.polygons]==expected
        material_checks.append(dict(group=group,passed=True,triangles=len(ob.data.polygons),roles=slots))
        for part in destinations[group]:
            parent=bpy.data.objects['SM_LancerM03_'+part];data=ob.data.copy() if group.startswith('Service_Wing') else ob.data
            if group.startswith('Service_Wing'):
                inv=parent.matrix_world.inverted()
                for v in data.vertices:v.co=inv@v.co
            instance=bpy.data.objects.new('SM_LancerM03_Service_'+part,data);bpy.context.scene.collection.objects.link(instance)
            instance.parent=parent;instance.location=(0,0,0);instance['part_name']='Service_'+part;instance['detail_for']=part;new.append(instance)
        bpy.data.objects.remove(ob,do_unlink=True)
    bpy.context.view_layer.update()
    preserved=all(signature(bpy.data.objects[n])==s for n,s in original.items())
    base_tints=all(digest([[list(d.color) for d in a.data] for a in o.data.color_attributes])==base_colors[o.name] for o in base)
    assert preserved and base_tints
    write('service_surface_samples.json',dict(count=len(fits),samples=fits))
    write('new_service_material_checks.json',material_checks)
    write('preservation_checks.json',dict(source=str(SOURCE),source_sha256=SHA,prior20_geometry_uv_normals_smoothing_indices_transforms_and36frames_unchanged=preserved,
        approved12_base_vertex_colors_and_roles_unchanged=base_tints,detail_vertex_tints_and_selected_detail_material_names_intentionally_changed=True,new_service_instances=len(new)))
    intent=dict(rule_id='detail02_service_armor_v1',source=str(SOURCE),source_sha256=SHA,before=before_roles,new_roles={o.name:[o.data.materials[f.material_index].name for f in o.data.polygons] for o in new})
    write('detail02_material_intent.json',intent)
    bpy.ops.wm.save_as_mainfile(filepath=str(WORK))
    q=runpy.run_path(str(P/'qa_m03.py'))['run']()
    passed=q['technical_pass'] and preserved and base_tints and q['assembly_instances']==24 and q['assembly_triangles']<=60000
    write('milestone_gate.json',dict(passed=passed,appearance_approved=False,native_imported=False))
    if passed:bpy.ops.wm.save_as_mainfile(filepath=str(FINAL))
    write('source_record.json',dict(source_file=bpy.data.filepath,technical_pass=passed,stage='Detail02 fitted service hardware and armor finish'))
    runpy.run_path(str(ROOT/'review/M03/Review01/render_m03.py'))['run'](full=passed,structural=False)
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SHA
    if not passed:raise RuntimeError('Detail02 gate failed; work/QA/diagnostic renders retained')

if __name__=='__main__':run()
