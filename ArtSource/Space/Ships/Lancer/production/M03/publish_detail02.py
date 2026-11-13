"""Publish verified Detail02 geometry, renders and handoff without heavy tools."""
from pathlib import Path
import json,hashlib,html
P=Path(__file__).resolve().parent;ROOT=P.parents[1];OUT=ROOT/'review/M03/Detail02'
def read(name):return json.loads((OUT/name).read_text(encoding='utf-8-sig'))
def write(name,data):(OUT/name).write_text(json.dumps(data,indent=2),encoding='utf-8')
def image(path,label):return f'<figure><a href="{path}"><img loading="lazy" src="{path}" alt="{html.escape(label)}"></a><figcaption>{html.escape(label)}</figcaption></figure>'
def main():
    q=read('validation_summary.json');gate=read('milestone_gate.json');preserve=read('preservation_checks.json');render=read('render_manifest.json')
    assert gate['passed'] and q['technical_pass'] and preserve['prior20_geometry_uv_normals_smoothing_indices_transforms_and36frames_unchanged']
    source=Path(read('source_record.json')['source_file']);assert source.name=='Lancer_M03_Detail02_v01.blend'
    sha=hashlib.sha256(source.read_bytes()).hexdigest();metrics=read('process_metrics.json')
    body='<header><p>TRIAD DRIFT / SHIPS &amp; WEAPONS</p><h1>Lancer M03 · Detail02</h1><p>Fitted service hardware and painted-armor hierarchy</p></header><aside><b>Technical verification passed · Nick visual approval pending</b><p>New wing access hatches/cooling grilles and engine service rails are actual geometry. Painted panel chamfers, piece-level finish variation and a less blue paint response replace the uniform bright outline treatment. No silhouette restart or Unreal integration.</p></aside>'
    body+=f'<p>{q["assembly_triangles"]:,} triangles · {q["assembly_instances"]} instances · {q["unique_meshes"]} unique meshes · {q["materials"]} materials · {q["emissive_surface_fraction"]*100:.3f}% emissive surface.</p>'
    body+='<h2>Reference and actual candidate</h2><div class="two">'+image('../../../production/M03/refs/Lancer_M03_authority.png','Approved reference artwork')+image('01_hero_front_3q.png','Actual Detail02 Blender render')+'</div>'
    body+='<h2>Material05 → Detail02 · same camera and original studio</h2>'
    for slug,title in [('01_hero_front_3q','Three-quarter'),('06_top','Top'),('08_rear_3q','Rear three-quarter')]:
        body+=f'<h3>{title}</h3><div class="two">'+image('../Detail01/Material05/'+slug+'.png','Before · Material05')+image(slug+'.png','After · Detail02')+'</div>'
    body+='<h2>Mechanical construction close-ups</h2><div class="two">'+image('12_wing_service.png','Fitted hatch, fasteners and shoulder grille')+image('13_engine_service.png','Case-mounted saddle, pipes and retaining clamps')+'</div>'
    body+='<h2>All actual material views</h2><div class="three">'
    for v in render['images']:body+=image(v['file'],v['title'])
    body+='</div><h2>Clay and actual mesh edges</h2>'+image('Lancer_M03_clay_sheet.png','Six-angle clay geometry')+image('Lancer_M03_wire_sheet.png','Actual mesh-edge wire review')
    body+='<h2>Verification and limits</h2><ul><li>Prior20mesh geometry, normals, smoothing, UVs, polygon indices and transforms remain fixed. All36frames and33legacy contracts preserved. Twelve approved base meshes retain their vertex colors and role assignments.</li><li>Four new service mesh instances follow their Wing/Engine parents. Existing detail-layer paint tint and selected exposed-edge material names intentionally changed. No other ship was edited.</li><li>No final AAA/look approval, baked textures, export round trip, native collision/LOD, cook or gameplay validation. Procedural brush and PanelTint require an explicit engine transfer after approval.</li></ul><pre>'+html.escape('\n'.join(q['geometry_warnings']))+'</pre>'
    links=[('Editable Blender source','../../../production/M03/'+source.name),('QA','validation_summary.json'),('Preservation','preservation_checks.json'),('Fitted roof samples','service_surface_samples.json'),('Paint response','paint_response_change.json'),('Panel finishes','panel_finish_changes.json'),('Runtime handoff','runtime_handoff.json'),('Independent review','art_review.md'),('Report','report.md')]
    body+='<p>'+' · '.join(f'<a href="{path}">{label}</a>' for label,path in links)+'</p>'
    css='body{margin:0;background:#101820;color:#dce6ee;font:16px/1.55 system-ui}main{max-width:1500px;margin:auto;padding:28px}aside{padding:16px;background:#30291e;border-left:4px solid #e8b45c}h2{margin-top:42px}.two,.three{display:grid;gap:16px}.two{grid-template-columns:1fr 1fr}.three{grid-template-columns:repeat(3,1fr)}figure{margin:0 0 18px}img{display:block;width:100%;height:auto}figcaption{padding:6px;color:#a9becd}a{color:#79d7ff}pre{white-space:pre-wrap}@media(max-width:850px){.two,.three{grid-template-columns:1fr}}'
    (OUT/'index.html').write_text('<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Lancer Detail02 review</title><style>'+css+'</style></head><body><main>'+body+'</main></body></html>',encoding='utf-8')
    handoff=json.loads((ROOT/'review/M03/Detail01/Material05/runtime_handoff.json').read_text(encoding='utf-8'))
    handoff.update(source=str(source),source_sha256=sha,stage='Detail02 technical PASS; Nick visual approval pending; no native authority',
        materials='Six shared roles; Detail02 material_parameters.json, paint_response_change.json and panel_finish_changes.json define shader/tint changes. Painted detail chamfers replace blanket Titanium wear. Bake or explicitly reconstruct procedural brush/PanelTint before native use.',
        source_detail_layer='Prior8detail instances plus4newservice meshes. Service_Wing_L/R and Service_Engine_L/R parent to corresponding approved components. Merge/bake into component-local export COPIES only after coordinated approval; preserve every canonical frame.',
        export_readiness='Not exported/imported. Source geometry and rays verified; appearance gate, shader transfer, calibration/round trip, collision/LOD and actual Unreal testing remain pending.')
    write('runtime_handoff.json',handoff)
    write('delivery_manifest.json',dict(source=str(source),source_sha256=sha,review=str(OUT/'index.html'),technical_pass=True,appearance_approved=False,native_imported=False))
    report=f'''**Agent:** Ships & Weapons Agent
**Role:** Ship and modular weapon visuals
**Task:** Lancer M03 Detail02 refinement

Technical PASS; visual approval pending. Source {source}; SHA256 {sha}.
Review {OUT/'index.html'}.

Added fitted wing access frames/covers/fasteners and shoulder cooling grilles, plus engine saddles/pipes/retainers. Four new parented instances. Replaced blanket Titanium detail-edge outlines with painted chamfers, introduced piece-level tint/roughness variation and adjusted Graphite to less-blue #24282D/specular.35; satin.56/zero coat retained. Bare metal, glass and cyan unchanged.

QA:{q['assembly_triangles']:,}triangles/{q['assembly_instances']}instances/{q['unique_meshes']}unique meshes/{q['materials']}materials. Prior20geometry/UV/normals/smoothing/indices/transforms preserved;36frames/33legacy contracts and12clearance rays PASS;12approvedbase vertexcolors/roles preserved. No source overwrite or gameplay/native edits. All material angles, clay/wire and dedicated mechanical close-ups come from the actual candidate.

Warnings: {json.dumps(q['geometry_warnings'])}. See independent art_review.md for remaining visual limitations; technical PASS is not visual acceptance. No finalAAA, texturebake/export roundtrip/collisionLOD/native/gameplay acceptance. Runtime handoff retains contracts and later engine-transfer gates.

Branch nick/ships-lancer-m03, cyclebase9e59ddc; serial Lead preflight/commit/push required, no Main merge authority. Measured Blender{q['blender']}, PID{metrics['pid']}, exit{metrics['exit_code']}, {metrics['elapsed_seconds']:.3f}s, peak{metrics['peak_working_set_mib']:.2f}MiB, minfree{metrics['minimum_free_gib']:.3f}GiB,2threads. Codex model/effort/weeklyusage/reset telemetry unavailable. Cycle started November12,21:49:34 Eastern; final handoff records end time. Next priority: Nick's Detail02 visual review before another spacecraft or native integration.
'''
    (OUT/'report.md').write_text(report,encoding='utf-8');print(str(OUT/'index.html'))
if __name__=='__main__':main()
