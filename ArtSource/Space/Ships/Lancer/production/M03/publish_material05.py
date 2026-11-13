"""Offline publication of one material-response test; never launches Blender."""
from pathlib import Path
import hashlib,html,json

P=Path(__file__).resolve().parent;ROOT=P.parents[1];OUT=ROOT/'review/M03/Detail01/Material05'
def read(name):return json.loads((OUT/name).read_text(encoding='utf-8-sig'))
def write(name,data):(OUT/name).write_text(json.dumps(data,indent=2),encoding='utf-8')
def image(path,label):return f'<figure><a href="{path}"><img src="{path}" loading="lazy" alt="{html.escape(label)}"></a><figcaption>{html.escape(label)}</figcaption></figure>'

def main():
    q=read('validation_summary.json');gate=read('milestone_gate.json');source=read('source_record.json');render=read('render_manifest.json')
    assert q['technical_pass'] and gate['passed'] and len(render['images'])==9
    assert hashlib.sha256(Path(source['source_file']).read_bytes()).hexdigest()==source['source_sha256']
    handoff=json.loads((OUT.parent/'Material04/runtime_handoff.json').read_text(encoding='utf-8'))
    handoff.update(source=source['source_file'],source_sha256=source['source_sha256'],
        stage='Material05 technically verified; Nick appearance review pending; no native authority',
        materials='Six shared roles retained. Material05 changes only Graphite nominal roughness .38 to .56 and clearcoat .08 to zero; actual node-input/link diff is in material_node_changes.json. No slot/role/color/geometry/frame changes. Procedural nodes still require bake or documented native reconstruction.',
        unchanged_from_detail01_v02='All20 mesh topology/geometry/normals/smoothing/UV/colors/slots/indices/transforms/parents and36frames match before/after fingerprints.')
    write('runtime_handoff.json',handoff)
    body='<header><p>TRIAD DRIFT / SHIPS &amp; WEAPONS</p><h1>Lancer · Material05</h1><p>One material-response correction · actual Blender renders</p></header><aside><b>Technical PASS · Nick appearance review pending</b><p>Graphite roughness .38 → .56; clearcoat .08 → 0. Geometry, attachments, colors and all other shader inputs are unchanged. Stop here: no further refinement, export or Unreal integration.</p></aside>'
    body+='<p>Independent visual review finds a limited improvement: the glossy paint/metal ambiguity is resolved, while overall materials remain3/5. Uniform light-gray planes and the mechanical-detail gap remain MAJOR concerns against the reference. This is not final appearance approval.</p><h2>Same-angle before / after · original studio</h2><p>Identical camera, exposure, light color, energy and size. The original violet rim and cool fill are retained. These views judge the actual material change.</p>'
    for slug,title in [('01_hero_front_3q','Front three-quarter'),('06_top','Top'),('08_rear_3q','Rear three-quarter')]:
        body+=f'<h3>{title}</h3><div class="two">'+image('../Material04/'+slug+'.png','Before · Material04')+image(slug+'.png','After · Material05')+'</div>'
    body+='<h2>Separate neutral-light diagnostic</h2><p>Area and world colors are achromatic at matched linear Rec709 luminance. Cameras, exposure, energy, light size and world strength stay fixed. Both versions are rendered in this same diagnostic rig; it does not replace the original-studio comparison.</p>'
    for slug,title in [('01_hero_front_3q','Front three-quarter'),('06_top','Top'),('08_rear_3q','Rear three-quarter')]:
        body+=f'<h3>{title}</h3><div class="two">'+image('neutral_before_'+slug+'.png','Before · Material04 / neutral diagnostic')+image('neutral_after_'+slug+'.png','After · Material05 / neutral diagnostic')+'</div>'
    body+='<h2>Approved reference artwork</h2>'+image('../../../../production/M03/refs/Lancer_M03_authority.png','Reference artwork · different lighting and detail; final appearance is not approved')
    body+='<h2>Verification and limits</h2><ul><li>34,516 triangles; 20 instances / 14 unique meshes; six materials; 804 × 720 × 175.693 cm. Geometry, UVs, normals, vertex tint, material roles/indices and36attachment frames unchanged.</li><li>All12 muzzle/thruster clearance rays pass. Zero geometry errors or floating pieces. Retained24hidden back-to-back pairs and flight-origin ground warning.</li><li>The QA material-assignment wording describes the inherited Material04 role-map check. Material05 preservation_checks.json separately verifies that slots and polygon indices did not change in this pass.</li><li>This test addresses paint response only. Sparse panel variation and simplified mechanical detail are unchanged. No baked textures, export round trip, collision/LOD, native assets or runtime acceptance.</li><li>Neutral diagnostics and source technical PASS do not constitute Nick visual approval. No automatic follow-up iteration.</li></ul>'
    links=[('Editable source','../../../../production/M03/Lancer_M03_Detail01_v03.blend'),('QA','validation_summary.json'),('Preservation','preservation_checks.json'),('All node inputs before/after','material_node_changes.json'),('Neutral rig record','neutral_lighting_diagnostic.json'),('Render manifest','render_manifest.json'),('Runtime handoff','runtime_handoff.json'),('Independent review','art_review.md'),('Report','report.md'),('Unchanged clay','../Material04/Lancer_M03_clay_sheet.png'),('Unchanged wire','../Material04/Lancer_M03_wire_sheet.png')]
    body+='<p>'+' · '.join(f'<a href="{path}">{label}</a>' for label,path in links)+'</p>'
    css='body{margin:0;background:#101820;color:#dce6ee;font:16px/1.55 system-ui}main{max-width:1500px;margin:auto;padding:28px}h1{font-size:38px}h2{margin-top:44px}aside{padding:16px;background:#30291e;border-left:4px solid #e8b45c}.two{display:grid;grid-template-columns:1fr 1fr;gap:16px}figure{margin:0 0 18px}img{display:block;width:100%;height:auto}figcaption{padding:6px;color:#a9becd}a{color:#79d7ff}@media(max-width:850px){.two{grid-template-columns:1fr}}'
    (OUT/'index.html').write_text('<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Lancer Material05 comparison</title><style>'+css+'</style></head><body><main>'+body+'</main></body></html>',encoding='utf-8')
    metrics=read('process_metrics.json')
    report=f'''**Agent:** Ships & Weapons Agent
**Role:** Ship and modular weapon visuals
**Task:** Lancer Material05 controlled material correction

Source technical PASS. Appearance pending Nick review. No further work, export or Unreal integration.
Source: {source['source_file']}
SHA256: {source['source_sha256']}
Review: {OUT/'index.html'}

Only Graphite nominal roughness .38→.56 and clearcoat .08→0 changed. The snapshot of every input/link across six material graphs differs at exactly the two authorized inputs. All20mesh topology/geometry/normals/smoothing/UV/colors/slots/indices/transforms/parents and36frames are unchanged. Source Material04 v02 hash remains pinned. Polygon/material/frame QA passes;34,516triangles/20instances/14unique meshes/6materials/12clearance rays. Emissive area1.525%. Retained24hidden back-to-back pairs and non-grounded flight origin; no new geometry errors. QA's material_assignment_mode wording describes the inherited Material04 role-map validation, not an index change in Material05; the separate preservation fingerprint proves slots/indices unchanged in this pass.

Three original-studio same-angle images and six separately labeled neutral-light before/after diagnostics are actual Blender renders. Neutral area/world hue is achromatic at matched linear luminance; energy/size/exposure/strength/cameras fixed. Source saved before diagnostic lighting; prior clay/wire and cassette fit are inherited only because geometry is identical. Prior technical evidence is not represented as newly rendered/resampled.

Mechanical detail, panel-value variation and cockpit construction remain unchanged. Procedural shaders still need baking or documented Unreal reconstruction, calibrated export/round trip and later runtime validation after visual approval. No native/export/gameplay changes.

Fresh independent review: visible limited improvement; glossy paint/metal ambiguity resolved, but overall materials/detail/edges-shading remain3/5. Uniform light-gray hierarchy and mechanical-detail gaps are retained MAJOR findings. This review is not Nick's appearance approval.

Measured Blender{render['blender']}, PID{metrics['pid']}, exit{metrics['exit_code']}, {metrics['elapsed_seconds']:.3f}s, peak{metrics['peak_working_set_mib']:.2f}MiB, minimum free{metrics['minimum_free_gib']:.3f}GiB,2threads. Codex model/effort/token/cost telemetry unavailable. Source-only branch nick/ships-lancer-m03; serialized Lead commit/push required, no Main merge authority. See art_review.md for independent visual findings. Stop for Nick's same-angle review.
'''
    (OUT/'report.md').write_text(report,encoding='utf-8')
    write('delivery_manifest.json',dict(source=source['source_file'],source_sha256=source['source_sha256'],review=str(OUT/'index.html'),technical_pass=True,appearance_approved=False,native_imported=False))
    print(str(OUT/'index.html'))

if __name__=='__main__':main()
