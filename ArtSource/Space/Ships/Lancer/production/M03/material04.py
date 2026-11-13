"""Scheduled material-only look test; no geometry, UV, frame or camera changes."""
from pathlib import Path
import os,json,hashlib,runpy,shutil
import bpy
P=Path(__file__).resolve().parent;ROOT=P.parents[1];OUT=ROOT/'review/M03/Detail01/Material04'
SOURCE=P/'Lancer_M03_Detail01_v01.blend';SHA='a883c6947b21fa9ddaa25691d8da5f8c5825a8bf85b465609c68967a7029e66b'
os.environ['LANCER_M03_REVIEW_DIR']=str(OUT)
os.environ['LANCER_M03_MATERIAL_PARAMS']=str(OUT/'material_parameters.json')
os.environ['LANCER_M03_MATERIAL_INTENT']=str(OUT/'material_intent.json')
os.environ['LANCER_M03_APPROVED_FRAME']='A02'
PREFIX='M_LancerM03_'
APPROVED=P/'Lancer_M03_structure_v03.blend'
APPROVED_SHA='3e05f82bd2ab265cb32cc91c072f4fb5060231a2c777945129350dbd41f921c2'

def fingerprint(ob):
    d=dict(matrix=[list(x) for x in ob.matrix_world],parent=ob.parent.name if ob.parent else None,part=ob.get('part_name'))
    if ob.type=='MESH':
        d.update(vertices=[list(v.co) for v in ob.data.vertices],faces=[list(f.vertices) for f in ob.data.polygons],
                 normals=[list(f.normal) for f in ob.data.polygons],smooth=[f.use_smooth for f in ob.data.polygons],
                 uv=[[list(v.uv) for v in layer.data] for layer in ob.data.uv_layers])
    return hashlib.sha256(json.dumps(d,separators=(',',':')).encode()).hexdigest()

def role_names(ob):return [ob.data.materials[f.material_index].name for f in ob.data.polygons]
def desired_role(ob,face,old):
    part=ob['part_name']
    armor=part in ['Engine_L','Engine_R','Wing_L','Wing_R'] or (part=='Hull' and -(ob.matrix_world@face.center).y*100<200)
    return PREFIX+'Graphite' if old==PREFIX+'Gunmetal' and armor else old

def coverage(obs):
    total={};armor={};armor_parts={'Hull','Wing_L','Wing_R','Engine_L','Engine_R','Secondary_L','Secondary_R'}
    for ob in obs:
        for f in ob.data.polygons:
            role=ob.data.materials[f.material_index].name
            total[role]=total.get(role,0)+f.area
            if ob['part_name'] in armor_parts:armor[role]=armor.get(role,0)+f.area
    return dict(assembly_surface_m2=total,predefined_base_armor_surface_m2=armor,
                graphite_fraction_of_base_armor=armor.get(PREFIX+'Graphite',0)/sum(armor.values()))

def run():
    assert bpy.app.background and hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SHA
    assert hashlib.sha256(APPROVED.read_bytes()).hexdigest()==APPROVED_SHA
    OUT.mkdir(parents=True,exist_ok=True);bpy.ops.wm.open_mainfile(filepath=str(SOURCE));bpy.context.view_layer.update()
    obs=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.get('part_name')]
    frames=[o for o in bpy.context.scene.objects if o.type=='EMPTY' and o.name.startswith('SOCKET_')]
    assert len(obs)==20 and len(frames)==36
    original={o.name:fingerprint(o) for o in obs+frames}
    before={o.name:role_names(o) for o in obs};before_slots={o.name:[m.name for m in o.data.materials] for o in obs}
    before_coverage=coverage(obs);processed=set()
    for ob in obs:
        if ob.data.as_pointer() in processed:continue
        processed.add(ob.data.as_pointer());targets=[desired_role(ob,f,old) for f,old in zip(ob.data.polygons,before[ob.name])]
        # Build desired slot list and restore indices AFTER clearing the slots.
        slots=list(dict.fromkeys(targets));ob.data.materials.clear()
        for name in slots:ob.data.materials.append(bpy.data.materials[name])
        indices={name:i for i,name in enumerate(slots)}
        for f,name in zip(ob.data.polygons,targets):f.material_index=indices[name]
    for ob in obs:
        assert role_names(ob)==[desired_role(ob,f,old) for f,old in zip(ob.data.polygons,before[ob.name])]
    graphite=bpy.data.materials[PREFIX+'Graphite']
    mul=next(n for n in graphite.node_tree.nodes if n.type=='MATH' and n.operation=='MULTIPLY_ADD' and abs(n.inputs[1].default_value-.2)<.001)
    mul.inputs[2].default_value=.38-.1
    params=json.loads((ROOT/'review/M03/Detail01/Correction03/material_parameters.json').read_text())
    next(p for p in params if p['name']==PREFIX+'Graphite')['roughness']=.38
    (OUT/'material_parameters.json').write_text(json.dumps(params,indent=2),encoding='utf-8')
    nodes=[]
    for spec in params:
        m=bpy.data.materials[spec['name']];ns=m.node_tree.nodes;bs=next(n for n in ns if n.type=='BSDF_PRINCIPLED')
        mix=next(n for n in ns if n.type=='MIX_RGB');rough=next(n for n in ns if n.type=='MATH' and n.operation=='MULTIPLY_ADD' and abs(n.inputs[1].default_value-.2)<.001)
        nodes.append(dict(name=m.name,base_linear_rgb=list(mix.inputs[1].default_value),metallic=bs.inputs['Metallic'].default_value,
                          roughness_alpha_multiplier=rough.inputs[1].default_value,roughness_driver_offset=rough.inputs[2].default_value,
                          roughness_socket_linked=bs.inputs['Roughness'].is_linked,coat_weight=bs.inputs['Coat Weight'].default_value,
                          emission_strength=bs.inputs['Emission Strength'].default_value))
    (OUT/'actual_node_parameters.json').write_text(json.dumps(nodes,indent=2),encoding='utf-8')
    after={o.name:role_names(o) for o in obs};after_coverage=coverage(obs)
    intent=dict(rule_id='armor_paint_v1',source=str(SOURCE),source_sha256=SHA,before=before,after=after,
                before_slots=before_slots,after_slots={o.name:[m.name for m in o.data.materials] for o in obs},
                rule='Only Gunmetal to Graphite on Engine/Wing base parts and Hull cradles (rootX<200cm); bare prow, barrels/nozzles/trim, glass/cyan and all detail-layer role assignments retained',
                changed_faces={name:sum(a!=b for a,b in zip(before[name],after[name])) for name in before})
    (OUT/'material_intent.json').write_text(json.dumps(intent,indent=2),encoding='utf-8')
    (OUT/'material_coverage.json').write_text(json.dumps(dict(units='m2; actual assembled instance surface area, not triangle counts',before=before_coverage,after=after_coverage),indent=2),encoding='utf-8')
    preserved=all(fingerprint(bpy.data.objects[name])==value for name,value in original.items())
    paint_dominant=after_coverage['graphite_fraction_of_base_armor']>=.6
    structure=dict(source=str(SOURCE),source_sha256=SHA,mesh_count=20,frame_count=36,
                   all_geometry_uv_normals_smoothing_transforms_frames_unchanged=preserved,
                   material_indices_intentionally_changed=True,paint_dominant_area_gate=paint_dominant,
                   minimum_base_armor_graphite_fraction=.6)
    (OUT/'approved_structure_checks.json').write_text(json.dumps(structure,indent=2),encoding='utf-8')
    bpy.ops.wm.save_as_mainfile(filepath=str(P/'Lancer_M03_Detail01_Material04_work.blend'))
    q=runpy.run_path(str(P/'qa_m03.py'))['run']()
    passed=q['technical_pass'] and preserved and paint_dominant
    (OUT/'milestone_gate.json').write_text(json.dumps(dict(passed=passed,material_only=True,look_approved=False),indent=2))
    shutil.copyfile(ROOT/'review/M03/Detail01/Correction03/cassette_surface_fit_checks.json',OUT/'prior_cassette_surface_fit_checks.json')
    if passed:bpy.ops.wm.save_as_mainfile(filepath=str(P/'Lancer_M03_Detail01_v02.blend'))
    (OUT/'source_record.json').write_text(json.dumps(dict(source_file=bpy.data.filepath,stage='Material04 look test',technical_pass=passed),indent=2))
    runpy.run_path(str(ROOT/'review/M03/Review01/render_m03.py'))['run'](full=passed,structural=False)
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SHA
    assert hashlib.sha256(APPROVED.read_bytes()).hexdigest()==APPROVED_SHA
    if not passed:raise RuntimeError('Material-only gate failed; work and diagnostics preserved')

if __name__=='__main__':run()
