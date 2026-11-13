"""Offline publication of actual Detail01 evidence; does not launch Blender."""
from pathlib import Path
import json,hashlib,html,os
P=Path(__file__).resolve().parent;ROOT=P.parents[1];OUT=ROOT/'review/M03/Detail01';PASS=OUT/'Correction03'
q=json.loads((PASS/'validation_summary.json').read_text());gate=json.loads((PASS/'milestone_gate.json').read_text())
structure=json.loads((PASS/'approved_structure_checks.json').read_text());fit=json.loads((PASS/'cassette_surface_fit_checks.json').read_text())
assert gate['passed'] and q['technical_pass'] and fit['passed'] and structure['approved_geometry_uv_material_indices_and_transforms_unchanged']
source=Path(json.loads((PASS/'source_record.json').read_text())['source_file']);assert source.name=='Lancer_M03_Detail01_v01.blend'
render=json.loads((PASS/'render_manifest.json').read_text());assert len(render['images'])==11
sha=hashlib.sha256(source.read_bytes()).hexdigest()
def image(path,caption):return f'<figure><a href="{path}"><img src="{path}" alt="{html.escape(caption)}"></a><figcaption>{html.escape(caption)}</figcaption></figure>'
body='<h1>Lancer M03 · Materials &amp; Detail01</h1><p>Ships &amp; Weapons Agent · Actual Blender candidate · Correction03</p>'
body+='<div class="notice"><strong>Technical gate passed. Look approval is pending.</strong> Independent review retains a MAJOR material-direction gap: surfaces remain lighter and more uniform than the charcoal reference. This is a review candidate, with no final AAA, native or gameplay acceptance.</div>'
body+=f'<p>{q["assembly_triangles"]:,} triangles · {q["assembly_instances"]} mesh instances · six shared materials · {q["emissive_surface_fraction"]*100:.3f}% emissive surface. All 12 approved base meshes and 36 frames unchanged; zero bounds growth. Cassette minimum clearance {fit["minimum_normal_clearance_m"]*1000:.3f} mm across {fit["samples"]:,} samples.</p>'
body+='<h2>Approved reference and actual candidate</h2><div class="two">'+image('../../../production/M03/refs/Lancer_M03_authority.png','Approved reference artwork')+image('Correction03/01_hero_front_3q.png','Actual material/detail candidate')+'</div>'
body+='<h2>A02 before → Detail01 after</h2><p>Same approved geometry framing, camera, lens, studio lights and exposure. Changes are materials and the added detail layer.</p>'
for title,file in [('Three-quarter','01_hero_front_3q.png'),('Front','02_front.png'),('Side','04_left.png'),('Top','06_top.png')]:
    body+=f'<h3>{title}</h3><div class="two">'+image('../StageA02/'+file,'Approved A02 structure / before')+image('Correction03/'+file,'Detail01 / after')+'</div>'
body+='<h2>All material views</h2><div class="three">'
for entry in render['images']:body+=image('Correction03/'+entry['file'],entry['title'])
body+='</div><h2>Full form and topology inspection</h2>'+image('Correction03/Lancer_M03_clay_sheet.png','Full six-angle clay geometry')+image('Correction03/Lancer_M03_wire_sheet.png','Actual mesh-edge wire review')
body+='<h2>Independent review and limits</h2><ul><li>Previously raised/colliding overlays visibly resolved; patches now fit the engine housings. Geometry and surface-clearance evidence pass.</li><li>MAJOR look gap remains: broad smooth silver/lilac-gray response and weak paint/metal/panel hierarchy compared with the charcoal authority. MINOR: sparse/diagrammatic panel variation.</li><li>Approved structural proportions, including the lower fairings, were preserved. Dark-studio images demonstrate cyan placement but are insufficient alone for judging surfaces.</li><li>Shaders remain procedural Blender materials with vertex tint. Bake or reconstruct them before Unreal export; no native material, export round trip, collision/LOD/cook or gameplay validation is claimed.</li></ul>'
body+='<h2>Retained technical warnings</h2><pre>'+html.escape('\n'.join(q['geometry_warnings']))+'</pre><p>These are hidden back-to-back internal joins between independently closed layers and the flight-origin ground-plane warning. No same-facing coplanarity, floating islands or mesh errors remain. Deleting approved base faces was deliberately excluded.</p>'
body+='<p><a href="../../../production/M03/'+source.name+'">Editable Blender source</a> · <a href="Correction03/validation_summary.json">QA</a> · <a href="Correction03/approved_structure_checks.json">Structure preservation</a> · <a href="Correction03/cassette_surface_fit_checks.json">Actual clearance samples</a> · <a href="runtime_handoff.json">Proposed runtime handoff</a> · <a href="art_review_final.md">Fresh independent review</a> · <a href="report.md">Report</a></p><p>Two failed attempts and their work sources are retained. Stop for Nick’s look review before native promotion, other craft or additional refinement.</p>'
page='<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Lancer materials/detail review</title><style>body{margin:0;background:#111923;color:#dce6ee;font:16px/1.55 system-ui}main{max-width:1450px;margin:auto;padding:30px}.notice{padding:16px;background:#30291f;border-left:4px solid #efb36d}h2{margin-top:40px}.two,.three{display:grid;gap:16px;align-items:start}.two{grid-template-columns:1fr 1fr}.three{grid-template-columns:repeat(3,1fr)}figure{margin:0 0 18px}img{width:100%;display:block}figcaption{padding:6px;color:#b4c4d2}a{color:#77d6fb}pre{white-space:pre-wrap}@media(max-width:800px){.two,.three{grid-template-columns:1fr}}</style><main>'+body+'</main></html>'
(OUT/'index.html').write_text(page,encoding='utf-8')
attachments=json.loads((P/'attachments.json').read_text())
parts=json.loads((P/'unreal_visual_parts.json').read_text())
handoff=dict(stage='Technically verified source; Nick look approval pending; NO native import authority',source=str(source),source_sha256=sha,
             approved_structure_source=structure['approved_source'],approved_structure_sha256=structure['approved_source_sha256'],
             blender_version=q['blender'],blender_units='metres: -Y forward,+X right,+Z up',unreal_units='centimetres: +X forward,+Y right,+Z up',
             positive_unit_scales=q['unit_positive_scale'],legacy_count=33,proposed_count=3,frames=attachments['sockets'],
             base_assembly=parts,source_detail_layer='Eight additional mesh instances parented to approved parts; merge/bake into component-local export copies only under a later verified handoff',
             proposed_native_mesh_paths={role:'/Game/TriadDrift/Art/Ships/Lancer/M03/SM_LancerM03_'+role for role in ['Hull','Wing_L','Wing_R','Canopy','Primary','Rotor','Secondary','Engine']},
             proposed_presentation_path='/Game/TriadDrift/Presentation/DA_Lancer_Visuals',native_paths_claimed=False,native_imported=False,
             materials='Six shared roles; read Correction03/material_parameters.json and retain PanelTint RGB/alpha. Procedural brush must be baked or reconstructed before native use.',
             counterpart_routing=dict(parts_engine_frames='T1 Flight/Input/Travel through Lead',weapon_muzzles='T2 Combat/Powers through Lead'),
             concrete_question='T2: confirm native weapon-bank binding for unchanged component-local Muzzle_Primary_L/R and Muzzle_Secondary_L/R_01..04 before import, including mapping to runtime Muzzle_N discovery; no transform change is requested.',
             prerequisites=['Nick look acceptance','Lead-published native-path claims','Verified material bake/reconstruction and mesh export round trip','Engineering socket/bank binding, collision/LOD/cook and runtime verification'])
(OUT/'runtime_handoff.json').write_text(json.dumps(handoff,indent=2),encoding='utf-8')
review='''# Fresh independent Detail01 final review
Agent: Detail01 Final Reviewer. Role: independent visual QA. Task: Lancer materials/detail.
Reviewer /root/detail01_final_review inspected actual Correction03 hero/rear/top/dark/clay/wire, A02 and approved authority. No apps or edits.
BLOCKER: none identified in technical evidence. Previous raised-overlay MAJOR visibly resolved; patches fit housings and surface-fit JSON measures1624samples/min3.302mm.
MAJOR: material hierarchy remains weak. Hero/top still predominantly smooth silver/lilacgray vscharcoal authority. Uniform-gray concern improved, not resolved. No human look approval.
MINOR: sparse/diagrammatic wing-panel variation. Dark-studio suppresses surface assessment; hero/top/rear/clay adequate. Wire sheet small, cannot independently prove everytopology condition.
Approved A02 structural differences excluded from new-defect findings. Source12base meshes/36frames unchanged, zero bounds growth,34516tris20instances6mats1.525percentemission. Geometry/contact/role/frame/raychecks pass;24hiddenback-to-backpairs andflightoriginwarning retained.
Verdict: technically verified source candidate; retain LOOK-GATE HOLD. No finalAAA/native/gameplay acceptance. No more source work authorized by this review.
'''
(OUT/'art_review_final.md').write_text(review,encoding='utf-8')
report=f'''# Lancer M03 Materials & Detail01
Agent: Ships & Weapons Agent. Role: ship and modular weapon visuals.
Stage: technically verified source candidate; MAJOR material-direction gap remains; Nick look approval pending.
Source: {source}
Source SHA256: {sha}
Review: {(OUT/'index.html').resolve()}
Actual:34516triangles/20instances/6materials/1.525percentemission. Geometry/contact/material-role/36frame/12ray gates PASS.12approvedmeshes geometry/UV/materialIDs/transforms plus36frames unchanged, approvedv03 on-diskSHA unchanged, bboxgrowth0.
Actual engine-surface minimum3.302mm at1624vertices/centroids/edge samples;≥2mmgate passed. No QAthreshold weakened.
Additions: fitted shallowdeck/wing/cassetteplates, intake backing/louvers, engineered cyanconduits/edgechannels, localized receiver/belly/enginehardware, smallinstrumentstatuslamps. Six shared PBRroles with graphitepaint/baremetal/titanium/composite/opaquecanopy/cyanemission. No silhouette rebuild or othercraft.
Firstfailure:33772tris/coplanar enginepetal andconduit endcaps. Correction02:33836tris/thinnerpatches exposed additional hull/frame/case contacts. Both work sources+diagnostics preserved. Correction03 followsactualHULLknots andactualapprovedengineBVH, trims blueendcaps andseparates newpetal caps. Fullactual11materialviews/clay/wire/silhouettes fromsamepassingcandidate.
Retained warnings:24hiddenback-to-backpairs acrossindependentlyclosedlayers andgroundZflightorigin. No mesherrors/floating/same-facingcoplanarity.
Fresh reviewer: previousoverlaycontact/lipMAJORresolved; remainingMAJOR weak materialhierarchy/grayresponse; MINOR sparsepanelvariation. Do not report finalAAA or lookapproved.
No image textures baked, nativeassets imported, collision/LOD/cook or gameplay tests. ProceduralPBR/PanelTint needs baking or documentedUnreal reconstruction. Proposednativepaths andONEengineeringquestion are in runtime_handoff.json; not claimed or authorized.
Latestrun:Blender5.1.1/background/twothreads/PID13160 exit0,15.167s,peak889.05MiB,minfree5.239GiB,noabort. Model/effort/token/cost telemetry unavailable for this cleared-goal task; no estimates represented as metrics.
Stop for Nick look review. No Native/Main/othercraft/additionalrefinement.
'''
(OUT/'report.md').write_text(report,encoding='utf-8')
(OUT/'delivery_manifest.json').write_text(json.dumps(dict(stage='Technical PASS; look HOLD',source=str(source),source_sha256=sha,review=str((OUT/'index.html').resolve()),passing_evidence=str(PASS.resolve()),look_approved=False,native_imported=False),indent=2),encoding='utf-8')
print('Published actual Detail01 review; look hold retained; source SHA256 '+sha)
