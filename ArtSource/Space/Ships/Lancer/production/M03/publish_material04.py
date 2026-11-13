"""Publish existing Material04 renders and QA without launching Blender."""
from pathlib import Path
import hashlib, html, json

P = Path(__file__).resolve().parent
ROOT = P.parents[1]
OUT = ROOT / 'review/M03/Detail01/Material04'

def read(name):
    return json.loads((OUT / name).read_text(encoding='utf-8-sig'))

def write(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2), encoding='utf-8')

def image(path, caption):
    return (f'<figure><a href="{path}"><img loading="lazy" src="{path}" '
            f'alt="{html.escape(caption)}"></a><figcaption>{html.escape(caption)}</figcaption></figure>')

def main():
    q, structure, gate = read('validation_summary.json'), read('approved_structure_checks.json'), read('milestone_gate.json')
    render, coverage, metrics = read('render_manifest.json'), read('material_coverage.json'), read('process_metrics.json')
    assert q['technical_pass'] and gate['passed'] and structure['all_geometry_uv_normals_smoothing_transforms_frames_unchanged']
    assert metrics['exit_code'] == 0 and metrics['abort'] == 'none' and len(render['images']) == 11
    source = Path(read('source_record.json')['source_file'])
    assert source.name == 'Lancer_M03_Detail01_v02.blend' and source.stat().st_size > 0
    sha = hashlib.sha256(source.read_bytes()).hexdigest()
    before = coverage['before']['graphite_fraction_of_base_armor'] * 100
    after = coverage['after']['graphite_fraction_of_base_armor'] * 100
    report = f'''**Agent:** Ships & Weapons Agent
**Role:** Ship and modular weapon visuals
**Task:** Lancer M03 Material04 review candidate

Technical PASS; appearance HOLD. Nick's direct decision relayed by Lead: "Hold for material correction." Material04 is NOT approved for export, native adoption or Main integration. Source: {source}
Review: {OUT / 'index.html'}
Source SHA-256: {sha}

Material-only change: selected engine cases, wing-root transitions and hull cradles now use Graphite instead of Gunmetal; Graphite nominal roughness .50 -> .38. Base-armor surface-area coverage {before:.2f}% -> {after:.2f}%. No color, lighting, exposure, camera, silhouette or attachment changes. Polygon material indices intentionally changed and independently checked against the fixed armor-paint rule. Area is summed assembled surface area, including hidden faces; it is not screen-space coverage.

Verified: {q['assembly_triangles']:,} triangles, 20 mesh instances / 14 unique meshes, six materials, zero image textures. Dimensions 804 x 720 x 175.693 cm. All20 geometry/topology/UV/normals/smoothing/transforms and36frames match passing Detail01_v01 fingerprints. All33 canonical legacy frames preserved. All12 muzzle/thruster clearance rays PASS; no floating geometry or geometry errors. Emissive surface {q['emissive_surface_fraction']*100:.3f}%. Prior cassette fit is inherited, not resampled, because all geometry is unchanged (1624 samples, minimum3.302mm).

Retained warnings: 24 hidden back-to-back face pairs at closed detail/base joins, retained to preserve approved geometry; flight origin is not at ground plane. These are not new failures. No same-facing coplanarity errors.

Visual assessment: darker engine/root coverage improves hierarchy, but broad silver/lilac highlights, smooth broad panels, simple canopy/weapons and sparse mechanical detail remain below the reference. A numeric material coverage gate is not final appearance approval. No automatic further refinement. See art_review.md for independent findings when available.

Unreal blockers: Nick appearance approval; select a baked-texture or documented native shader reconstruction for procedural brush/PanelTint; approved component-local export preparation, axes/scale/pivot/socket calibration and round-trip validation; collision/LOD/material budget checks and actual runtime/cook verification. Existing engine/muzzle/ability/camera contracts must survive; pending T1/T2 canonical-to-runtime muzzle-bank mapping remains in runtime_handoff.json. No export/import/native activation was performed.

Git: nick/ships-lancer-m03 at a19f478 before this checkpoint; prior source-only PR10 was merged by Lead at98187b9. This new delta awaits serialized Lead preflight/commit/push, never direct worker Main merge. No ownership conflicts found in the published exact-root claims. Work .blend and temp/cache files are preserved locally and excluded from delivery.

Measured Blender5.1.1, PID{metrics['pid']}, exit0, {metrics['elapsed_seconds']:.3f}s, peak{metrics['peak_working_set_mib']:.2f}MiB, minfree{metrics['minimum_free_gib']:.3f}GiB, two threads. Rendering {render['elapsed_seconds']:.3f}s. Codex model/effort/token/cost telemetry unavailable; no estimates. Active cycle began 2026-11-13 02:04:33 UTC; final handoff records total duration. Stop for Nick review; no native/other-ship work.
'''
    (OUT/'report.md').write_text(report, encoding='utf-8')
    handoff = json.loads((OUT.parent/'runtime_handoff.json').read_text(encoding='utf-8'))
    handoff.update(source=str(source), source_sha256=sha,
        stage='Material04 technically verified; Nick appearance HOLD; no native authority',
        materials='Six shared roles; Material04 material_parameters.json and actual_node_parameters.json are authoritative for this candidate. Intentional armor role remap recorded in material_intent.json. Procedural brush and PanelTint require bake or explicit native reconstruction.',
        material_slot_policy='Do not use historical numeric slot indices. Map by material role name; Material04 compacts slots after an intentional per-face role change.',
        unchanged_from_detail01_v01='All20 mesh geometry/UV/normals/smoothing/transforms and36frames unchanged; see approved_structure_checks.json',
        export_readiness='NOT exported. Appearance approval, shader transfer, component-local copies, axes/scale/pivot/socket calibration, round trip, collision/LOD and runtime verification pending.')
    write('runtime_handoff.json', handoff)
    write('delivery_manifest.json', dict(stage='Technical PASS; visual HOLD', source=str(source),source_sha256=sha,
        review=str(OUT/'index.html'),look_approved=False,native_imported=False,gameplay_validated=False,
        work_source_excluded=str(P/'Lancer_M03_Detail01_Material04_work.blend')))
    body = '<header><p>TRIAD DRIFT / SHIPS &amp; WEAPONS</p><h1>Lancer M03 · Material04</h1><p>Actual Blender geometry · bounded material review</p></header>'
    body += '<aside><strong>Technical PASS · Nick: Hold for material correction</strong><p>Material04 is not approved for export, native adoption or Main integration. Darker armor coverage is improved. Bright-studio silver/lilac highlights and sparse surface detail remain below the approved reference. No Unreal export or integration.</p></aside>'
    body += f'<div class="stats"><p><b>34,516</b> triangles</p><p><b>6</b> material roles</p><p><b>36 / 36</b> frames preserved</p><p><b>{before:.1f} → {after:.1f}%</b> base armor Graphite</p></div>'
    body += '<nav><a href="#reference">Reference</a> · <a href="#compare">Before / after</a> · <a href="#angles">All angles</a> · <a href="#qa">QA / handoff</a></nav>'
    body += '<h2 id="reference">Approved artwork beside actual Blender render</h2><div class="two">'
    body += image('../../../../production/M03/refs/Lancer_M03_authority.png','Approved design reference artwork')
    body += image('01_hero_front_3q.png','Material04 actual Blender render')+'</div>'
    body += '<h2 id="compare">Correction03 → Material04</h2><p>Same actual geometry, camera, studio rig and exposure. Only selected material roles and Graphite roughness changed. The reference uses different lighting; direct material equivalence is not claimed.</p>'
    for title, file in [('Three-quarter','01_hero_front_3q.png'),('Front','02_front.png'),('Left side','04_left.png'),('Top','06_top.png'),('Rear three-quarter','08_rear_3q.png')]:
        body += f'<h3>{title}</h3><div class="two">'+image('../Correction03/'+file,'Before · Detail01 Correction03')+image(file,'After · Material04')+'</div>'
    body += '<details><summary>Approved A02 structure comparisons</summary><p>A02 is preserved. Detail01 added surface geometry; Material04 did not change geometry.</p>'
    for file in ['01_hero_front_3q.png','02_front.png','04_left.png','06_top.png']:
        body += '<div class="two">'+image('../../StageA02/'+file,'Approved A02 structure')+image(file,'Current Material04')+'</div>'
    body += '</details><h2 id="angles">All eleven material views</h2><div class="three">'
    for entry in render['images']:
        assert (OUT/entry['file']).stat().st_size == entry['bytes']
        body += image(entry['file'],entry['title'])
    body += '</div><h2>Full form and actual topology</h2>'+image('Lancer_M03_clay_sheet.png','Six-angle clay · same mesh geometry')+image('Lancer_M03_wire_sheet.png','Actual mesh edges · open full-size for inspection')
    body += '<h2 id="qa">Technical evidence and integration gates</h2><ul><li>34,516 triangles; 20 instances / 14 unique meshes; 804 × 720 × 175.693 cm; six roles; no baked image textures.</li><li>All20 meshes and36frames unchanged. All33legacy anchors preserved; all12 clearance rays pass. Material indices intentionally differ.</li><li>1.525% emissive surface. Prior 1,624-sample cassette fit is inherited because geometry is identical, not newly sampled.</li><li>Retained: 24 hidden back-to-back pairs at closed-layer joins and flight-origin ground warning.</li><li>Before Unreal: Nick appearance approval, shader bake/reconstruction, calibrated modular export/round trip, collision/LOD decisions and actual runtime validation. No gameplay code or native asset changed.</li></ul>'
    links = [('Editable Blender source','../../../../production/M03/'+source.name),('QA','validation_summary.json'),('Preservation','approved_structure_checks.json'),('Material coverage','material_coverage.json'),('Actual node values','actual_node_parameters.json'),('Material role map','material_intent.json'),('Proposed runtime handoff','runtime_handoff.json'),('Report','report.md'),('Independent review','art_review.md'),('Correction proposal','material_correction_proposal.md'),('Measured process','process_metrics.json')]
    body += '<p>'+' · '.join(f'<a href="{path}">{title}</a>' for title,path in links)+'</p>'
    body += '<footer>All review angles come from the same actual Blender candidate. Earlier models, failure diagnostics and Git history are preserved. Final look, Unreal and gameplay approval are separate gates.</footer>'
    style = 'body{margin:0;background:#101820;color:#dce6ee;font:16px/1.55 system-ui}main{max-width:1500px;margin:auto;padding:28px}header p{letter-spacing:.08em;color:#91adbf}h1{font-size:38px;margin:8px 0}h2{margin-top:44px}aside{background:#30291e;border-left:4px solid #e8b45c;padding:16px}aside p{margin:6px 0}.two,.three,.stats{display:grid;gap:16px;align-items:start}.two{grid-template-columns:1fr 1fr}.three{grid-template-columns:repeat(3,1fr)}.stats{grid-template-columns:repeat(4,1fr);margin:24px 0}.stats p{background:#1a2935;padding:14px;margin:0}.stats b{display:block;font-size:22px}figure{margin:0 0 16px}img{width:100%;height:auto;display:block;background:#16242e}figcaption{padding:6px;color:#a9becd}a{color:#79d7ff}details,footer{margin-top:25px}summary{cursor:pointer}footer{border-top:1px solid #355064;padding-top:20px;color:#9cb0be}@media(max-width:850px){.two,.three{grid-template-columns:1fr}.stats{grid-template-columns:1fr 1fr}}'
    page = '<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Lancer Material04 review</title><style>'+style+'</style></head><body><main>'+body+'</main></body></html>'
    (OUT/'index.html').write_text(page, encoding='utf-8')
    print(json.dumps(dict(source_sha256=sha, review=str(OUT/'index.html'), technical_pass=True, look_approved=False)))

if __name__ == '__main__':
    main()
