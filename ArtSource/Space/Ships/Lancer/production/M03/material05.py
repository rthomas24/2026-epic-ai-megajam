"""One authorized material-response correction; no geometry or attachment edits."""
from pathlib import Path
import copy, hashlib, json, os, runpy, shutil, time
import bpy

P = Path(__file__).resolve().parent
ROOT = P.parents[1]
OUT = ROOT/'review/M03/Detail01/Material05'
PRIOR = OUT.parent/'Material04'
SOURCE = P/'Lancer_M03_Detail01_v02.blend'
SOURCE_SHA = '24ebb10fc055e26e7108f592300bdc21303d164295fa96217338b1a875fb4d49'
FINAL = P/'Lancer_M03_Detail01_v03.blend'
WORK = P/'Lancer_M03_Detail01_Material05_work.blend'
SPECS = [('01_hero_front_3q',35,24,True),('06_top',0,90,False),('08_rear_3q',215,22,True)]

def write(name, data):
    (OUT/name).write_text(json.dumps(data,indent=2),encoding='utf-8')

def digest(data):
    return hashlib.sha256(json.dumps(data,separators=(',',':'),sort_keys=True).encode()).hexdigest()

def value(v):
    if v is None or isinstance(v,(bool,int,float,str)):
        return v
    if isinstance(v,bpy.types.ID):
        return {'id':v.name_full,'type':v.bl_rna.identifier}
    try:
        return [value(x) for x in v]
    except TypeError:
        return str(v)

def fingerprint(ob):
    record=dict(matrix=[list(x) for x in ob.matrix_world],local_matrix=[list(x) for x in ob.matrix_local],
        parent=ob.parent.name if ob.parent else None,part=ob.get('part_name'),detail_for=ob.get('detail_for'))
    if ob.type=='MESH':
        mesh=ob.data
        record.update(vertices=[list(v.co) for v in mesh.vertices],faces=[list(f.vertices) for f in mesh.polygons],
            normals=[list(f.normal) for f in mesh.polygons],smooth=[f.use_smooth for f in mesh.polygons],
            uv=[[list(v.uv) for v in layer.data] for layer in mesh.uv_layers],
            slots=[m.name for m in mesh.materials],material_indices=[f.material_index for f in mesh.polygons],
            edges=[(list(e.vertices),e.use_seam) for e in mesh.edges],
            colors={a.name:[list(d.color) for d in a.data] for a in mesh.color_attributes})
    return digest(record)

def node_state(materials):
    result={}
    for m in materials:
        ns={}
        for n in m.node_tree.nodes:
            ns[n.name]=dict(type=n.type,operation=getattr(n,'operation',None),blend_type=getattr(n,'blend_type',None),
                inputs={str(i):value(s.default_value) for i,s in enumerate(n.inputs) if hasattr(s,'default_value')})
        result[m.name]=dict(nodes=ns,links=sorted((l.from_node.name,l.from_socket.identifier,l.to_node.name,l.to_socket.identifier) for l in m.node_tree.links))
    return result

def run():
    assert bpy.app.background
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SOURCE_SHA
    assert not FINAL.exists() and not WORK.exists(),'Never overwrite a checkpoint'
    assert not (OUT/'source_record.json').exists(),'Never rerun into existing evidence'
    OUT.mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE));bpy.context.view_layer.update()
    obs=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.get('part_name')]
    frames=[o for o in bpy.context.scene.objects if o.type=='EMPTY' and o.name.startswith('SOCKET_')]
    assert len(obs)==20 and len(frames)==36
    original={o.name:fingerprint(o) for o in obs+frames}
    materials=sorted({m for o in obs for m in o.data.materials},key=lambda m:m.name)
    assert len(materials)==6
    before=node_state(materials)
    graphite=bpy.data.materials['M_LancerM03_Graphite']
    bs=next(n for n in graphite.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    rough=next(n for n in graphite.node_tree.nodes if n.type=='MATH' and n.operation=='MULTIPLY_ADD' and abs(n.inputs[1].default_value-.2)<.001)
    coat_index=list(bs.inputs).index(bs.inputs['Coat Weight'])
    prior_rough=rough.inputs[2].default_value;prior_coat=bs.inputs['Coat Weight'].default_value
    assert abs(prior_rough-.28)<1e-6 and abs(prior_coat-.08)<1e-6
    rough.inputs[2].default_value=.56-.1
    bs.inputs['Coat Weight'].default_value=0
    after=node_state(materials)
    normalized=copy.deepcopy(after)
    normalized[graphite.name]['nodes'][rough.name]['inputs']['2']=before[graphite.name]['nodes'][rough.name]['inputs']['2']
    normalized[graphite.name]['nodes'][bs.name]['inputs'][str(coat_index)]=before[graphite.name]['nodes'][bs.name]['inputs'][str(coat_index)]
    assert normalized==before,'Unexpected material node/input/link change'
    write('material_node_changes.json',dict(before=before,after=after,only_authorized_two_inputs_changed=True,
        authorized_inputs=[dict(material=graphite.name,node=rough.name,input_index=2,meaning='Nominal roughness .38 to .56; alpha and microfinish unchanged'),
            dict(material=graphite.name,node=bs.name,input_index=coat_index,meaning='Coat Weight .08 to zero')]))
    params=json.loads((PRIOR/'material_parameters.json').read_text())
    spec=next(v for v in params if v['name']==graphite.name)
    spec.update(roughness=.56,coat_weight=0)
    write('material_parameters.json',params)
    # Keep the independent intentional-role QA from Material04; roles do not change now.
    shutil.copyfile(PRIOR/'material_intent.json',OUT/'material_intent.json')
    for name in ['material_coverage.json','prior_cassette_surface_fit_checks.json']:
        shutil.copyfile(PRIOR/name,OUT/name)
    os.environ['LANCER_M03_REVIEW_DIR']=str(OUT)
    os.environ['LANCER_M03_MATERIAL_PARAMS']=str(OUT/'material_parameters.json')
    os.environ['LANCER_M03_MATERIAL_INTENT']=str(OUT/'material_intent.json')
    os.environ['LANCER_M03_APPROVED_FRAME']='A02'
    preserved=all(fingerprint(bpy.data.objects[n])==s for n,s in original.items())
    assert preserved
    write('preservation_checks.json',dict(source=str(SOURCE),source_sha256=SOURCE_SHA,mesh_count=20,frame_count=36,
        geometry_topology_normals_smoothing_uv_transforms_parents_vertex_colors_slots_indices_frames_unchanged=preserved,
        before_fingerprints=original,only_authorized_two_node_inputs_changed=True))
    bpy.ops.wm.save_as_mainfile(filepath=str(WORK))
    q=runpy.run_path(str(P/'qa_m03.py'))['run']()
    passed=q['technical_pass'] and preserved
    write('milestone_gate.json',dict(passed=passed,appearance_approved=False,native_imported=False,gameplay_validated=False))
    if not passed:
        raise RuntimeError('Material05 technical QA failed; no final source or renders produced')
    bpy.ops.wm.save_as_mainfile(filepath=str(FINAL))
    write('source_record.json',dict(source_file=str(FINAL),source_sha256=hashlib.sha256(FINAL.read_bytes()).hexdigest(),
        prior_source=str(SOURCE),prior_sha256=SOURCE_SHA,technical_pass=True))
    renderer=runpy.run_path(str(ROOT/'review/M03/Review01/render_m03.py'))
    cam=renderer['studio']();sc=bpy.context.scene
    framing=[o for o in obs if not o.get('detail_for')]
    assert len(framing)==12
    records=[];started=time.perf_counter()
    def capture(prefix,regime,stage):
        for slug,az,el,perspective in SPECS:
            renderer['frame'](cam,framing,az,el,perspective)
            path=OUT/(prefix+slug+'.png');assert not path.exists()
            sc.render.filepath=str(path);bpy.ops.render.render(write_still=True)
            rec=renderer['check'](path)
            rec.update(lighting=regime,material_stage=stage,azimuth_deg=az,elevation_deg=el,
                camera_matrix=[list(v) for v in cam.matrix_world],lens=cam.data.lens,ortho_scale=cam.data.ortho_scale,
                camera_type=cam.data.type,shift=[cam.data.shift_x,cam.data.shift_y],exposure=sc.view_settings.exposure)
            records.append(rec)
    capture('','Original unchanged colored studio','Material05')
    lights=[o for o in sc.objects if o.type=='LIGHT' and o.name.startswith('Review_')]
    assert len(lights)==4
    bg=next(n for n in sc.world.node_tree.nodes if n.type=='BACKGROUND')
    def luminance(rgb):return sum(a*b for a,b in zip(rgb,(.2126,.7152,.0722)))
    lighting=[]
    for o in lights:
        old=list(o.data.color);neutral=luminance(old)
        lighting.append(dict(name=o.name,before=old,after=[neutral]*3,energy=o.data.energy,size=o.data.size,location=list(o.location)))
        o.data.color=(neutral,)*3
    old_world=list(bg.inputs['Color'].default_value);neutral=luminance(old_world[:3])
    bg.inputs['Color'].default_value=(neutral,neutral,neutral,old_world[3])
    write('neutral_lighting_diagnostic.json',dict(description='Achromatic area/world colors at matching linear Rec709 luminance; energy,size,camera,exposure,strength unchanged. Separate diagnostic, not original studio.',
        lights=lighting,world_before=old_world,world_after=list(bg.inputs['Color'].default_value),world_strength=bg.inputs['Strength'].default_value))
    rough.inputs[2].default_value=prior_rough;bs.inputs['Coat Weight'].default_value=prior_coat
    capture('neutral_before_','Neutral-light diagnostic','Material04')
    rough.inputs[2].default_value=.56-.1;bs.inputs['Coat Weight'].default_value=0
    capture('neutral_after_','Neutral-light diagnostic','Material05')
    assert node_state(materials)==after
    assert all(fingerprint(bpy.data.objects[n])==s for n,s in original.items())
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SOURCE_SHA
    write('render_manifest.json',dict(blender=bpy.app.version_string,engine='BLENDER_EEVEE',view_transform=sc.view_settings.view_transform,
        images=records,threads=2,elapsed_seconds=time.perf_counter()-started,same_geometry_all_views=True,
        inherited_clay_wire='../Material04/ (unchanged geometry; not newly rendered)',source_not_saved_with_diagnostic_lighting=True))
    print('MATERIAL05_COMPLETE '+json.dumps(dict(technical_pass=True,render_count=len(records),source=str(FINAL))),flush=True)

if __name__=='__main__':
    run()
