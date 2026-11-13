"""Measured Lancer M03 QA, including immutable legacy and proposed new frames."""
from pathlib import Path
import hashlib, json, runpy, time, os
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree
P=Path(__file__).resolve().parent;ROOT=P.parents[1];REPO=ROOT.parents[3];OUT=Path(os.environ.get('LANCER_M03_REVIEW_DIR',str(ROOT/'review/M03/Review01')))

def write(name,data):
    (OUT/name).write_text(json.dumps(data,indent=2)+'\n')

def co(p):
    x,y,z=p;return Vector((y/100,-x/100,z/100))

def run():
    started=time.perf_counter();obs=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.get('part_name')]
    names=[o.name for o in obs]
    asset=runpy.run_path(str(REPO/'.claude/skills/blender-mcp/scripts/asset_report.py'))['main'](targets=names,tri_budget=60000)
    write('asset_report.json',asset)
    checker=runpy.run_path(r'C:/Users/nicho/.agents/skills/blender-asset-qa/scripts/qa_check.py')['run']
    geo=checker(objects=names,profile='game',eps=.001,tol=.002);write('geometry_qa.json',geo)
    contact=runpy.run_path(str(REPO/'.claude/skills/blender-mcp/scripts/assembly_check.py'))['main'](targets=names,tolerance=.002)
    write('assembly_contacts.json',contact)
    vertices=[];faces=[];owners=[];em_area=0;area=0
    for o in obs:
        n=len(vertices);vertices.extend(o.matrix_world@v.co for v in o.data.vertices)
        for f in o.data.polygons:
            faces.append(tuple(n+i for i in f.vertices));owners.append(o.name);area+=f.area
            if o.data.materials[f.material_index].name=='M_LancerM03_Blue':em_area+=f.area
    bvh=BVHTree.FromPolygons(vertices,faces,all_triangles=True)
    manifest=json.loads((P/'attachments.json').read_text());checks=[];rays=[]
    for s in manifest['sockets']:
        o=bpy.data.objects['SOCKET_'+s['parent']+'_'+s['name']];m=o.matrix_world.to_3x3()
        f=co([v*100 for v in s['forward']]).normalized();up=Vector((0,0,1))
        pe=(o.location-co(s['position_cm'])).length;we=(o.matrix_world.translation-co(s['root_position_cm'])).length
        fd=(m@Vector((0,-1,0))).normalized().dot(f);ud=(m@up).normalized().dot(up)
        checks.append(dict(parent=s['parent'],name=s['name'],local_error_m=pe,root_error_m=we,
                           forward_dot=fd,up_dot=ud,determinant=m.determinant(),
                           passed=pe<1e-6 and we<1e-6 and fd>.99999 and ud>.99999 and abs(m.determinant()-1)<1e-6))
        if s['purpose'] in ['muzzle','thruster'] or s['name']=='Portal_Exit':
            point,normal,face,distance=bvh.ray_cast(o.matrix_world.translation+f*.0001,f,100)
            rays.append(dict(name=s['name'],clear=point is None,hit=None if face is None else owners[face],distance_cm=None if point is None else distance*100))
    # Preserve actual contact positions for targeted fixes, not blind render-driven tweaks.
    overlaps=[]
    if geo['zfighting']:
        inside=checker.__globals__['_point_in_tri'];pairs=set()
        normals=[(vertices[t[1]]-vertices[t[0]]).cross(vertices[t[2]]-vertices[t[0]]).normalized() for t in faces]
        membership=[set(t) for t in faces]
        for i,t in enumerate(faces):
            a,b,c=[vertices[k] for k in t];n=normals[i];centre=(a+b+c)/3
            for p in [centre,centre.lerp(a,.6),centre.lerp(b,.6),centre.lerp(c,.6)]:
                for _,_,j,_ in bvh.find_nearest_range(p,.001):
                    if j is None or i==j or membership[i]&membership[j] or n.dot(normals[j])<.999:continue
                    aa,bb,cc=[vertices[k] for k in faces[j]];projected=p-normals[j]*normals[j].dot(p-aa);key=tuple(sorted([i,j]))
                    if key not in pairs and inside(projected,aa,bb,cc,normals[j]):
                        pairs.add(key);overlaps.append(dict(objects=[owners[i],owners[j]],position_cm=[-p.y*100,p.x*100,p.z*100],normal=list(n)))
    write('overlap_locations.json',overlaps)
    lo=[min(p[i] for p in vertices) for i in range(3)];hi=[max(p[i] for p in vertices) for i in range(3)]
    materials={m.name for o in obs for m in o.data.materials}
    params=json.loads(Path(os.environ.get('LANCER_M03_MATERIAL_PARAMS',str(P/'material_parameters.json'))).read_text())
    report=dict(blender=bpy.app.version_string,assembly_triangles=len(faces),assembly_instances=len(obs),
                unique_meshes=len({o.data.name for o in obs}),materials=len(materials),textures=0,
                dimensions_cm=[(hi[1]-lo[1])*100,(hi[0]-lo[0])*100,(hi[2]-lo[2])*100],
                geometry_pass=geo['pass'],geometry_errors=geo['errors'],geometry_warnings=geo['warnings'],
                contact_pass=contact['ok'],floating=contact['floating'],attachment_checks=checks,
                attachment_pass=all(c['passed'] for c in checks),ray_clearance=rays,ray_pass=all(r['clear'] for r in rays),
                unit_positive_scale=all(tuple(o.scale)==(1.,1.,1.) for o in obs),
                panel_tint_present=all(o.data.color_attributes.get('PanelTint') is not None for o in obs),
                emissive_surface_fraction=em_area/area,emissive_area_pass=em_area/area<=.05,
                material_parameters_pass=len(params)==6 and all(0<=p['metallic']<=1 and (.15 if 'Canopy' in p['name'] else .2)<=p['roughness']<=.85 and p['emissive_strength']<=3 for p in params),
                native_imported=False,presentation_approved=False,gameplay_validated=False,elapsed_seconds=time.perf_counter()-started)
    report['legacy_manifest_unchanged']=hashlib.sha256((ROOT/'attachments.json').read_bytes()).hexdigest()==hashlib.sha256((P/'refs/legacy_attachments.json').read_bytes()).hexdigest()
    intent_path=os.environ.get('LANCER_M03_MATERIAL_INTENT')
    detail02_path=os.environ.get('LANCER_M03_DETAIL02_INTENT')
    if detail02_path:
        intent=json.loads(Path(detail02_path).read_text())
        new_names={'SM_LancerM03_Service_'+p for p in ['Wing_L','Wing_R','Engine_L','Engine_R']}
        assignment_ok=(intent['rule_id']=='detail02_service_armor_v1' and hashlib.sha256(Path(intent['source']).read_bytes()).hexdigest()==intent['source_sha256'] and len(intent['before'])==20 and set(names)==set(intent['before'])|new_names)
        wear_parts={'Detail_Hull','Detail_Wing_L','Detail_Wing_R','Detail_Engine_L','Detail_Engine_R'}
        for ob in obs:
            actual=[ob.data.materials[f.material_index].name for f in ob.data.polygons]
            if ob.name in intent['before']:
                expected_roles=['M_LancerM03_Graphite' if old=='M_LancerM03_Titanium' and ob['part_name'] in wear_parts else old for old in intent['before'][ob.name]]
            else:
                expected_roles=intent['new_roles'][ob.name]
                assignment_ok &= ob.get('detail_for') in ['Wing_L','Wing_R','Engine_L','Engine_R'] and set(actual)<={'M_LancerM03_'+r for r in ['Graphite','Gunmetal','Titanium','Composite']}
            assignment_ok &= len(actual)==len(expected_roles) and actual==expected_roles
        report['material_assignment_pass']=bool(assignment_ok)
        report['material_assignment_mode']='Detail02 fixed selective painted-chamfer remap on prior detail only; new service-part role arrays checked; approved base role assignments unchanged'
    elif intent_path:
        intent=json.loads(Path(intent_path).read_text())
        assignment_ok=intent['rule_id']=='armor_paint_v1' and hashlib.sha256(Path(intent['source']).read_bytes()).hexdigest()==intent['source_sha256'] and set(intent['before'])==set(names)
        for ob in obs:
            before=intent['before'][ob.name];actual=[ob.data.materials[f.material_index].name for f in ob.data.polygons]
            expected_roles=[]
            for f,old in zip(ob.data.polygons,before):
                part=ob['part_name'];armor=part in ['Engine_L','Engine_R','Wing_L','Wing_R'] or (part=='Hull' and -(ob.matrix_world@f.center).y*100<200)
                expected_roles.append('M_LancerM03_Graphite' if old=='M_LancerM03_Gunmetal' and armor else old)
            assignment_ok &= len(before)==len(actual) and actual==expected_roles and actual==intent['after'][ob.name]
        report['material_assignment_pass']=bool(assignment_ok)
        report['material_assignment_mode']='Explicit intentional per-face armor-paint map; indices are not claimed unchanged'
    else:
        mc=json.loads((OUT/'material_assignment_checks.json').read_text())
        report['material_assignment_pass']=len(mc)==8 and all(c['passed'] and c['before_role_face_counts']==c['after_role_face_counts'] for c in mc)
    roles={o.get('part_name'):{o.data.materials[f.material_index].name for f in o.data.polygons} for o in obs}
    expected={'Canopy':{'Canopy','Graphite'},'Engine_L':{'Blue','Composite','Titanium','Gunmetal'},'Rotor_L':{'Blue','Titanium','Gunmetal'}}
    if intent_path or detail02_path:expected['Engine_L']={'Blue','Composite','Titanium','Graphite'}
    report['material_assignment_pass'] &= all({'M_LancerM03_'+v for v in values}<=roles[part] for part,values in expected.items())
    report['technical_pass']=all(report[k] for k in ['material_assignment_pass','geometry_pass','contact_pass','attachment_pass','ray_pass','unit_positive_scale','panel_tint_present','emissive_area_pass','material_parameters_pass','legacy_manifest_unchanged']) and len(faces)<=60000
    write('validation_summary.json',report)
    print('LANCER_M03_QA '+json.dumps({k:report[k] for k in ['technical_pass','assembly_triangles','geometry_errors','contact_pass','attachment_pass','ray_clearance']}),flush=True)
    return report

if __name__=='__main__':run()
