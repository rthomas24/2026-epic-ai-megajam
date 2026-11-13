"""Lead-scheduled additive detail pass on the immutable approved A02 structure."""
from pathlib import Path
import os, json, hashlib, runpy, shutil, sys
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree
P=Path(__file__).resolve().parent;ROOT=P.parents[1]
correction_tag='Correction03' if '--correction03' in sys.argv else 'Correction02' if '--correction02' in sys.argv else ''
OUT=ROOT/'review/M03/Detail01'/correction_tag
SOURCE=P/'Lancer_M03_structure_v03.blend'
SOURCE_SHA='3e05f82bd2ab265cb32cc91c072f4fb5060231a2c777945129350dbd41f921c2'
os.environ['LANCER_M03_REVIEW_DIR']=str(OUT)
os.environ['LANCER_M03_MATERIAL_PARAMS']=str(OUT/'material_parameters.json')
os.environ['LANCER_M03_APPROVED_FRAME']='A02'

def digest(value):return hashlib.sha256(json.dumps(value,separators=(',',':')).encode()).hexdigest()
def signature(ob):
    record=dict(matrix=[list(row) for row in ob.matrix_world],parent=ob.parent.name if ob.parent else None)
    if ob.type=='MESH':
        record.update(vertices=[list(v.co) for v in ob.data.vertices],faces=[list(f.vertices) for f in ob.data.polygons],
                      materials=[f.material_index for f in ob.data.polygons],
                      uv=[[list(d.uv) for d in layer.data] for layer in ob.data.uv_layers])
    return digest(record)
def box(obs):
    points=[o.matrix_world@Vector(p) for o in obs for p in o.bound_box]
    return [min(p[i] for p in points) for i in range(3)]+[max(p[i] for p in points) for i in range(3)]

def run():
    assert bpy.app.background,'Scheduled headless process required'
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SOURCE_SHA,'Approved source changed'
    OUT.mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
    env=runpy.run_path(r'C:/Users/nicho/.agents/skills/blender-core/scripts/env_report.py')['run']()
    (OUT/'environment.json').write_text(json.dumps(env,indent=2),encoding='utf-8')
    base=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.get('part_name')]
    frames=[o for o in bpy.context.scene.objects if o.type=='EMPTY' and o.name.startswith('SOCKET_')]
    assert len(base)==12 and len(frames)==36
    original={o.name:signature(o) for o in base+frames};bounds=box(base)
    contract=(P/'attachments.json').read_bytes();legacy=(ROOT/'attachments.json').read_bytes()
    model=runpy.run_path(str(P/'build_m03.py'));hs=model['HS']
    roles=['Graphite','Gunmetal','Titanium','Composite','Canopy','Blue']
    model['MATS'].extend(bpy.data.materials['M_LancerM03_'+role] for role in roles)
    # Reuse six shared roles. Paint stays nonmetallic; bare metals remain metallic.
    settings=[('#1E2832',0,.50,.08,0),('#465460',1,.44,0,0),('#A8B4BD',1,.31,0,0),
              ('#151D23',0,.76,0,0),('#0B2532',0,.15,.65,0),('#16BEE8',0,.33,0,1.8)]
    params=[]
    for role,mat,(color,metal,rough,coat,em) in zip(roles,model['MATS'],settings):
        ns=mat.node_tree.nodes;bs=next(n for n in ns if n.type=='BSDF_PRINCIPLED')
        mix=next(n for n in ns if n.type=='MIX_RGB');mix.inputs[1].default_value=(*model['rgb'](color),1)
        bs.inputs['Metallic'].default_value=metal;bs.inputs['Coat Weight'].default_value=coat
        # Existing vertex-alpha roughness and low-amplitude microfinish are retained.
        mul=next(n for n in ns if n.type=='MATH' and n.operation=='MULTIPLY_ADD' and abs(n.inputs[1].default_value-.2)<.001)
        mul.inputs[2].default_value=rough-.1
        if role=='Blue':bs.inputs['Emission Color'].default_value=(*model['rgb'](color),1);bs.inputs['Emission Strength'].default_value=em
        mat.diffuse_color=(*model['rgb'](color),1)
        params.append(dict(name=mat.name,base_color_srgb=color,metallic=metal,roughness=rough,coat_weight=coat,emissive_strength=em))
    (OUT/'material_parameters.json').write_text(json.dumps(params,indent=2),encoding='utf-8')
    # Add surface layers without rebuilding, moving or modifying any approved base mesh.
    # Measured first-pass cap coincidences: keep the detail, separate its end planes.
    engine_mesh=bpy.data.objects['SM_LancerM03_Engine_L'].data
    engine_bvh=BVHTree.FromPolygons([v.co.copy() for v in engine_mesh.vertices],[tuple(f.vertices) for f in engine_mesh.polygons],all_triangles=True)
    engine_sections=[(3,38),(31,46),(118,46),(191,38),(215,27)]
    def roof(x,y):
        hit,normal,index,distance=engine_bvh.ray_cast(Vector((y/100,-x/100,10)),Vector((0,0,-1)),20)
        if hit is None or normal.z<.2:raise RuntimeError('Cassette misses approved engine roof')
        return hit.z,normal.z
    g=model['refinement'].__globals__;original_strip=g['strip'];original_loft=g['loft'];trimmed=[]
    def fitted_loft(group,name,sections,*args,**kwargs):
        if name.startswith('DeckPlate'):
            xs=sorted({v[0] for v in sections}|{v[0] for v in g['HULL'] if sections[0][0]<v[0]<sections[-1][0]})
            sections=[]
            for x in xs:
                w,h,z=g['hull_profile'](x);sections.append((x,w*.64,.6,0,z+h))
        elif name.startswith('NoseService'):
            sections=[(x,w,h,y,z-.5) for x,w,h,y,z in sections]
        elif name.startswith('DriveArmor'):
            a,b=sections[0][0],sections[-1][0]
            xs=sorted({a,b}|{x for x,w in engine_sections if a<x<b});rows=[]
            for x in xs:
                t=(x-a)/(b-a);tile_width=35-3*t
                for aa,bb in zip(engine_sections,engine_sections[1:]):
                    if aa[0]<=x<=bb[0]:
                        width=aa[1]+(bb[1]-aa[1])*(x-aa[0])/(bb[0]-aa[0]);break
                rows.append((x,[-tile_width,-.66*width,0,.66*width,tile_width]))
            vv=[];ff=[];n=len(rows)*5
            for sign in [1,-1]:
                for x,ys in rows:
                    for y in ys:
                        z,nz=roof(x,y);vv.append((x,y,(z+sign*.006/nz)*100))
            for row in range(len(rows)-1):
                for col in range(4):
                    a0=row*5+col;b0=a0+1;d0=a0+5;e0=d0+1
                    ff.extend([(a0,b0,e0,d0),(a0+n,d0+n,e0+n,b0+n)])
            edge=list(range(5))+[r*5+4 for r in range(1,len(rows))]+list(range(n-2,n-6,-1))+[r*5 for r in range(len(rows)-2,0,-1)]
            ff.extend((a0,a0+n,b0+n,b0) for a0,b0 in zip(edge,edge[1:]+edge[:1]))
            return model['solid'](group,name,vv,ff,0,args[1],.15,.65,True)
        return original_loft(group,name,sections,*args,**kwargs)
    def recessed_strip(group,name,path,width,depth,mat=0,tint=.9):
        if name.startswith(('ContinuousBlueConduit','WingEdgeBlue')):
            path=list(path)
            path[0]=tuple(Vector(path[0])+(Vector(path[1])-Vector(path[0])).normalized()*2)
            path[-1]=tuple(Vector(path[-1])+(Vector(path[-2])-Vector(path[-1])).normalized()*2)
            trimmed.append(name)
        return original_strip(group,name,path,width,depth,mat,tint)
    g['strip']=recessed_strip;g['loft']=fitted_loft
    model['refinement']()
    g['strip']=original_strip;g['loft']=original_loft
    petals=[o for o in model['GROUPS']['Engine'] if 'NozzlePetal' in o.name]
    assert len(petals)==12 and len(trimmed)==4
    for ob in petals:
        for v in ob.data.vertices:v.co.y-=.02  # UE +X 2cm; base engine remains untouched.
    # Fit existing overlay surfaces; do not alter topology or any approved mesh.
    for group,objects in model['GROUPS'].items():
        if group.startswith('Wing_'):
            for ob in objects:
                if 'SRC_M03_Armor' in ob.name:
                    for v in ob.data.vertices:v.co.z-=.007
    for side in [-1,1]:
        path=[(-101,side*45,44.5),(-85,side*48,44.8)]
        model['strip']('Canopy','InstrumentHousing'+str(side),path,4,2,3,.9)
        model['strip']('Canopy','InstrumentStatus'+str(side),[(x,y,z+1.3) for x,y,z in path],1.5,1,5,1)
    overlays=model['combine']()
    # Verify actual final beveled top surfaces against the approved mesh, not a proxy.
    clearances=[];engine_overlay=overlays['Engine']
    for face in engine_overlay.data.polygons:
        points=[engine_overlay.data.vertices[i].co for i in face.vertices]
        centre=sum(points,Vector())/len(points)
        if face.normal.z<=.2 or -centre.y*100<39.8:continue
        samples=points+[centre]+[(a+b)*.5 for a,b in zip(points,points[1:]+points[:1])]
        for point in samples:
            z,nz=roof(-point.y*100,point.x*100);clearances.append((point.z-z)*nz)
    fit_pass=bool(clearances) and min(clearances)>=.002
    (OUT/'cassette_surface_fit_checks.json').write_text(json.dumps(dict(samples=len(clearances),minimum_normal_clearance_m=min(clearances) if clearances else None,passed=fit_pass),indent=2))
    (OUT/'material_assignment_checks.json').rename(OUT/'detail_material_assignment_checks.json')
    shutil.copyfile(ROOT/'review/M03/StageA02/material_assignment_checks.json',OUT/'material_assignment_checks.json')
    coll=bpy.data.collections.new('DETAIL_Lancer_M03_Detail01');bpy.context.scene.collection.children.link(coll)
    destinations={'Hull':['Hull'],'Wing_L':['Wing_L'],'Wing_R':['Wing_R'],'Canopy':['Canopy'],
                  'Primary':['Primary_L','Primary_R'],'Engine':['Engine_L','Engine_R']}
    for group,template in overlays.items():
        for part in destinations[group]:
            ob=bpy.data.objects.new('SM_LancerM03_Detail_'+part,template.data);coll.objects.link(ob)
            ob.parent=bpy.data.objects['SM_LancerM03_'+part];ob.location=(0,0,0)
            ob['part_name']='Detail_'+part;ob['detail_for']=part
        bpy.data.objects.remove(template,do_unlink=True)
    bpy.context.view_layer.update()
    approved_unchanged=all(signature(bpy.data.objects[name])==value for name,value in original.items())
    all_mesh=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.get('part_name')]
    enlarged=box(all_mesh);max_growth=max(abs(a-b) for a,b in zip(bounds,enlarged))
    preserved=approved_unchanged and (P/'attachments.json').read_bytes()==contract and (ROOT/'attachments.json').read_bytes()==legacy
    structural=dict(approved_source=str(SOURCE),approved_source_sha256=SOURCE_SHA,base_mesh_count=len(base),frame_count=len(frames),
                    approved_geometry_uv_material_indices_and_transforms_unchanged=preserved,max_bounds_growth_m=max_growth,
                    envelope_pass=max_growth<=.05,instances=len(all_mesh),instance_budget_pass=len(all_mesh)<=24)
    (OUT/'approved_structure_checks.json').write_text(json.dumps(structural,indent=2),encoding='utf-8')
    work_name=f'Lancer_M03_Detail01_{correction_tag}_work.blend' if correction_tag else 'Lancer_M03_Detail01_work.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(P/work_name))
    q=runpy.run_path(str(P/'qa_m03.py'))['run']()
    checks=json.loads((OUT/'detail_material_assignment_checks.json').read_text())
    detail_roles=all(c['passed'] for c in checks)
    passed=q['technical_pass'] and preserved and structural['envelope_pass'] and structural['instance_budget_pass'] and detail_roles and fit_pass
    (OUT/'milestone_gate.json').write_text(json.dumps(dict(passed=passed,detail_material_roles_pass=detail_roles),indent=2))
    if passed:
        bpy.ops.wm.save_as_mainfile(filepath=str(P/'Lancer_M03_Detail01_v01.blend'))
    (OUT/'source_record.json').write_text(json.dumps(dict(source_file=bpy.data.filepath,stage='Materials and Detail01',technical_pass=passed),indent=2))
    runpy.run_path(str(ROOT/'review/M03/Review01/render_m03.py'))['run'](full=passed,structural=False)
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SOURCE_SHA
    if not passed:raise RuntimeError('Detail01 gate failed; work source and diagnostics retained')

if __name__=='__main__':run()
