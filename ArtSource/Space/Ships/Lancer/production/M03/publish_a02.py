"""Offline publisher for the bounded Structural A02 review; no Blender launch."""
from pathlib import Path
import json, hashlib, html
P=Path(__file__).resolve().parent;ROOT=P.parents[1];OUT=ROOT/'review/M03/StageA02'
q=json.loads((OUT/'validation_summary.json').read_text());assert q['technical_pass']
source=Path(json.loads((OUT/'source_record.json').read_text())['source_file']);assert source.exists()
def img(path,alt):return f'<a href="{path}"><img src="{path}" alt="{alt}"></a>'
def crop(path,box,w,h):
    return f'<svg viewBox="{box}" role="img"><image href="{path}" width="{w}" height="{h}"/></svg>'
ref='../../../production/M03/refs/Lancer_M03_authority.png';old='../StageA01/'
body='<h1>Lancer M03 · Structural A02</h1><p>Agent: Ships &amp; Weapons Agent · Structural review only · Nick approval pending</p>'
body+='<p class="notice">Broader armored wedge, thicker angular wings and connected roots, lower armored canopy, continuous engine shoulders, shaped cannon supports and recessed rocket bores. Detailed panels, weathering and final PBR/emissive refinement remain deferred. No Unreal integration.</p>'
body+=f'<p>{q["assembly_triangles"]:,} triangles · {q["assembly_instances"]} parts · {q["materials"]} shared materials · all {len(q["attachment_checks"])} frames and {len(q["ray_clearance"])} clearance rays pass · assembly PASS.</p>'
body+='<h2>Approved artwork and actual A02</h2><div class="two">'+img(ref,'Approved reference artwork')+img('01_hero_front_3q.png','Actual A02 Blender render')+'</div>'
body+='<h2>A01 → A02: identical camera, lens and studio lighting</h2><div class="two">'+img(old+'01_hero_front_3q.png','Actual A01 before')+img('A02_same_A01_camera.png','Actual A02 after with A01 camera')+'</div>'
body+='<h2>Front, side and top form comparisons</h2><p>Columns: approved concept / A01 clay / A02 clay. Clay views share the same world scale. Reference artwork is not calibrated CAD. Browser crops display the original images without changing them.</p>'
for title,rb,ab,new in [('Front','0 518 347 128','712 6 700 700','Lancer_M03_clay_front.png'),('Side','650 518 465 128','1418 6 700 700','Lancer_M03_clay_right.png'),('Top','15 653 325 326','6 712 700 700','Lancer_M03_clay_top.png')]:
    body+=f'<h3>{title}</h3><div class="three">'+crop(ref,rb,1536,1024)+crop(old+'Lancer_M03_clay_sheet.png',ab,2124,1418)+img(new,'Actual A02 '+title)+'</div>'
body+='<h2>Geometry inspection</h2>'+img('Lancer_M03_clay_sheet.png','Six views of actual A02 geometry')+img('Lancer_M03_wire_sheet.png','Actual edge wireframe views')
body+='<h2>Remaining differences and handoff limits</h2><ul><li>The intake, engine and receiver shapes remain simpler than the reference. Legacy rocket exits constrain the lower fairings, which still add underside bulk.</li><li>The opaque canopy has heavier framing and a lower profile; glazing/interior treatment remains unfinished.</li><li>Armor panel construction, fine mechanical detail and final materials are deliberately deferred until structural approval. Existing diagnostic materials are not a finished surface treatment.</li><li>Native socket binding, material baking/reconstruction, export round trip, collision and gameplay performance remain unverified.</li></ul>'
body+='<h2>QA</h2><p>Original33-frame byte contract preserved. Positive unit transforms and mesh-health checks pass. Retained warnings:</p><pre>'+html.escape('\n'.join(q['geometry_warnings']))+'</pre>'
body+=f'<p><a href="../../../production/M03/{source.name}">Editable Blender source: {source.name}</a> · <a href="validation_summary.json">Measured QA</a> · <a href="process_metrics.json">Process resources</a> · <a href="report.md">Report</a></p><p>Stop at this checkpoint for Nick’s structural review. No final quality, native or gameplay approval is implied.</p>'
page='<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Lancer Structural A02</title><style>body{margin:0;background:#111923;color:#e1e9ef;font:16px/1.55 system-ui}main{max-width:1450px;margin:auto;padding:30px}h2{margin-top:38px}.notice{padding:15px;background:#253444;border-left:4px solid #6ed9ff}.two,.three{display:grid;gap:16px;align-items:center}.two{grid-template-columns:1fr 1fr}.three{grid-template-columns:1fr 1fr 1fr}img,svg{width:100%;display:block;background:#202830}svg{max-height:410px}a{color:#71dbff}pre{white-space:pre-wrap}@media(max-width:800px){.two,.three{grid-template-columns:1fr}}</style><main>'+body+'</main></html>'
(OUT/'index.html').write_text(page,encoding='utf-8')
report=f'''# Lancer M03 — Structural A02

Agent: Ships & Weapons Agent. Role: ship and modular weapon visuals.
Stage: technically verified structural prototype; Nick visual approval pending.
Source: {source}
Review: {OUT/'index.html'}

{q['assembly_triangles']} triangles,12parts,8unique meshes,6materials. Dimensions L/W/H cm: {q['dimensions_cm']}.
Mesh health,assembly contact,36attachment frames,12clearance rays and unchanged legacy33-frame bytes pass. No QA threshold relaxation.
Canopy roof lowered104→84cm in primary geometry; complete assembled height202.10→{q['dimensions_cm'][2]:.2f}cm. Terminal keel widened36→50cm, with a flattened armored mouth. Wing/root geometry thickened; shoulder backbones and tapered weapon fairings connect the machines. Contract origins remain fixed.
Actual render/clay/wire evidence all comes from this Blender model. A01 remains unchanged. A02_same_A01_camera.png uses the original A01 camera/lens/light setup. Clay comparisons use the same world scale.
First A02 attempt is preserved in attempt01 and structure_v02.blend. Its slot compaction incorrectly reset material indices, and blanket flat shading exposed triangulation seams. A separately granted correction restores indices after slot rebuild and uses sharp/weighted normals; geometry and canonical frame positions do not change.
Warnings: {q['geometry_warnings']}
Flight-centered origin deliberately extends belowZ0. No claim of export/collision/LOD/gameplay validation.
Remaining visual limitations: simpler intake/engine/receiver forms and underside bulk from fixed rocket origins; opaque canopy; no detailed armor/PBR/weather/emissive pass. No final/AAA approval.
Stop for Nick Structural A02 review before detailed refinement, native integration or another ship.
'''
(OUT/'report.md').write_text(report,encoding='utf-8')
manifest=dict(source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),review=str((OUT/'index.html').resolve()),technical_pass=True,visual_approved=False)
(OUT/'delivery_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest))
