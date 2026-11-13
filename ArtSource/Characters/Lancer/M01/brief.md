# Lancer player M01 — first armor blockout

CURRENT STAGE: Revision05 is saved and explicitly REJECTED by Nick for visual
quality/reference mismatch. It has3open clearance findings; no rig/native or
presentation approval. README.md records actual results. All current source/model/
review outputs are frozen for Lead publication. ReferenceRebuild/PROPOSAL.md is
queued for review; no further heavy work is authorized this cycle.

Revision05 source-plan history follows. The refinement instruction superseded
the earlier read-only hold for that pass. Existing Attempt02 outputs stay frozen.
Its actual status:20,494armor/112,672total triangles,62objects; hostQA0errors/
20warnings and assemblyPASS, but technicalFAIL for5customclearance findings.
Upper-left thigh original ray had negative radial exit/facing; measured20mm
distal upper-row shifts on both thighs resolved the regional preflight.

Revision05 restores upper-chest/clavicle coverage using explicitly associated
torso/neck skin, broadens the measured deltoid cap, tapers limb plate ends and
defines longitudinal facets. Helmet cross-sections use authored angular anchors
for brow/cheeks/chin, a peaked lower visor, and connected white ear rims with
graphite centres. Original body/anatomy/rig/reference remain unchanged.

Clearance fix: simultaneous per-iteration maximum vertex corrections, affecting
only nonzero barycentric contributors, replace sequential neighbouring-face
pushes. Forty-mm cumulative correction bound remains. Flagged breastplates,
pelvis and boots target3mm more inner projection clearance; the full-body2mm
acceptance threshold, source-region association and all QA remain unchanged.
This is an actual geometry correction, never a warning waiver. Results await
the separately cleared build and actual visual review.

New scripts source/{build_lancer_revision05.py,render_lancer_revision05.py,
run_armor_revision05.ps1}. BUILD outputs source/armor/Revision05/
{Lancer_Armor_Revision05.blend,blockout-qa.json,surface-fit.json,regional-preflight.json}.
RENDER outputs review/Revision05/Lancer_Revision05_{front,side,back,left,top,beauty}.png
and views.json; six720x960views include both sides, retaining the five previous
camera framings for comparison. Manifest records actual technical status.
Saved/Characters/Lancer/M01/ArmorRevision05-{BUILD,RENDER} is private runtime data.
Same25karmor/130ktotal budgets, headless/two threads/120s, fresh4GiB/zeroheavy,
own abort<1.5GiB, separate Lead grants/no overwrite/no automatic retry.
Stop saved by21:43EST November12 unless Lead relays a new explicit instruction.
No rigging, native integration, gameplay edits or additional character.

Revision04 Attempt02: SOURCE PREPARATION ONLY, separate versioned scripts/output.
Original Revision04 build stopped after5.9051833s/OS1 at upper-left-thigh
t=.145/angle1.0 before saving; the combined rejection message did not expose
whether hit/radius/facing/axis failed. Logs and its empty output folder stay frozen.

Attempt02 records all four rejection fields for every planned limb-grid sample,
including the original failed sample, before constructing any limb armor. It
tests the original row first. If invalid, it may use the measured SAME regional
cross-section's centre (<=30mm correction), and move that entire row inward
along its bone span by<=25mm and less than half its row spacing. It retains
the exact anatomical BVHs, normals/radial/axial rejection thresholds and separate
full-body collisions. This corrects the assumption that an animation bone axis
necessarily lies inside each authored plate cross-section; actual cause/selected
corrections remain unverified until Blender records regional-preflight.json.
No full-body-fit fallback, guard suppression, independent point clipping or
automatic retry. If any row cannot fit, the whole grid reports failures together
and stops; missing armor is not silently omitted or called complete.

Attempt02 uses source/build_lancer_revision04_attempt02.py,
source/render_lancer_revision04_attempt02.py and source/run_armor_revision04_attempt02.ps1.
New outputs source/armor/Revision04Attempt02/{Lancer_Armor_Revision04Attempt02.blend,
blockout-qa.json,surface-fit.json,regional-preflight.json}; review/Revision04Attempt02/
Lancer_Revision04Attempt02_{front,side,back,top,beauty}.png +views.json.
Saved/Characters/Lancer/M01/ArmorRevision04Attempt02-{BUILD,RENDER} is private.
Earlier sources and attempts are unchanged. Background/2threads/120s/RAM guards
and separate Lead execution grants remain mandatory. This cycle stops saved by
21:43EST November12; no continuous goal or next-character work.

Revision04 original source plan (historical; its failed run is recorded above).
Independent review confirmed
three major reference-fidelity failures in frozen Build03. New versioned scripts
build_lancer_revision04.py / render_lancer_revision04.py /
run_armor_revision04.ps1 replace the fitting method without changing those
historical scripts or the model/images. Lead owns review and Git publication.

One coherent primary-form revision, using the existing approved reference:

- Broad filled, curved split breastplate/scapular surfaces with a narrow layered
  abdomen; deltoid caps cover the outside of the actual shoulders. Torso grids
  sample the complete surface rather than connecting only an outline's corners.
- Bone-axis radial sampling fits front/outer thigh plates, a distinct kneecap,
  tapered front shin and back calf, forearm bracer and measured foot perimeter.
  Oblique plate ends, tapered coverage and a central ridge replace tube cuffs.
- One closed curved helmet cage forms crown, brow, cheeks and tapered chin.
  Its shared-vertex graphite optic seat/cyan visor cannot float as a separate
  disconnected island; fitted faceted ear housings establish the side silhouette.
  A closed crown fan replaces the previous open shell stack.

All exposed armor grids are triangulated and tested at every face centroid and
edge midpoint against the complete owned body. Clearance corrections are only
outward, capped at40mm total per vertex, with residual failures explicitly in
the technical acceptance flag/report. Torso samples retain65mm inward bounds;
the separate measured pelvic boundary retains80mm vertical limit. Silhouette-
exterior sample counts and actual fit corrections are reported, not hidden.
Graphite attachment seats connect armor to the body. No body faces are hidden,
removed or replaced; no anatomy, rig, weights, gameplay or secondary detail.

Fitting uses explicit separate anatomical BVHs, classified from read-only source
skin membership (including twist bones) plus side/axial spatial bounds. Torso,
head, left/right arms, legs and feet cannot borrow another region's first ray
hit. Empty/missing skin association fails explicitly; there is no full-body
fitting fallback. Limb fits additionally require positive bounded radial exits,
correct facing normals and axial alignment; foot fits require the intended side.
Supports use the same intended region as their plate. The full-body BVH remains
separate for final signed nearest-surface collision tests at every face centroid
and edge midpoint on the inner armor skin. Its failures remain explicit and do
not move geometry using another region. All normal mesh/assembly QA still runs.

Budgets stay<=25k evaluated original armor and<=130k total model triangles.
Over-budget or QA/clearance failures save diagnostic output and remain failures.
Do not claim visual improvement before actually opening the rendered results.

Exact new BUILD outputs: source/armor/Revision04/Lancer_Armor_Revision04.blend,
blockout-qa.json, surface-fit.json. New RENDER outputs:
review/Revision04/Lancer_Revision04_{front,side,back,top,beauty}.png +views.json.
Same720x960studio form review, framing and original reference for comparison.
Private runtime records: Saved/Characters/Lancer/M01/ArmorRevision04-{BUILD,RENDER}.
Separate Lead grants required; background CLI/2threads/120s, fresh>=4GiB,
zero heavy including dotnet, own-process abort<1.5GiB, no overwrite/auto retry.

Frozen Build03 diagnostic history follows.

Stage: Build01 failed before saving (pectoral outline missed body); Build02
failed during batch prevalidation (lower crotch has no central torso surface).
Build03 saved a DIAGNOSTIC blockout and five studio-color views. Technical QA
and visual review FAILED; this is not a completed M01 or an approval candidate.

Actual Build03: Blender5.1.1 background CLI,2threads, OS0/5.601933s,5.9875GiB
free at start; render OS0/2.7363692s,5.9959GiB free. Input/reference hashes
unchanged. Model39objects,3material roles,11,752evaluated original armor triangles,
103,930total triangles. Asset report0errors/1warning; host QA6errors/20warnings;
assembly fails for detached visor6.5mm. Five coplanar pairs: core/crown34triangle
pairs, body/thigh right11,left8, body/back pectoral left2,right2. Other warnings
include inherited Manny UV overlap, n-gons, visor duplicates, enclosed knees.
Zero boundary/nonmanifold edges, degenerate faces or inconsistent normal edges
does not override these assembly/presentation failures.

All five720x960PNGs were opened. Chest/thigh body penetration, tubular armor,
blocky bucket-like helmet and inadequate back/shoulder coverage visibly fail
the approved white/cyan four-view reference. Lead independently agrees; a fresh
independent critique is pending. Preserve these outputs unchanged for diagnosis.
Next primary-form plan must restore reference-specific sculpted chest/back,
anatomical faceted limb/shoulder forms and angular helmet/faceplate/ear silhouette,
as well as measured surface clearance and visor seating. No secondary detail,
rigging or Unreal work is authorized by this checkpoint.
White reference2fa970f1 is the approved concept. One player humanoid only; no
enemies/ships, rig, animation, gameplay hook or native integration in this pass.

Route B retains the owned Manny anatomy and measured A-pose (180.544cm tall,
front -Y/up +Z in Blender). A separate static production copy gets graphite
material and retains its complete head/neck; original reference remains immutable,
render-hidden and nonexportable, including its88bones plus root object. That
reference representation is not an export-ready89bone skeleton.

Original primary forms: closed tapered helmet core, white crown and jaw shells,
separate cyan wrap visor; paired chevron pectorals, layered abdomen/back plates,
pelvis guard, athletic shoulder caps, bicep/bracer shells, hip/thigh/knee/shin
segments and practical boot shells. Chest/back plates fit body-ray intersections
and have actual support pads. Limb shells follow measured joint axes, taper
individually and stop before flex joints. No generated anatomy or random greebles.
Keep hands/inner elbows/knee backs flexible. The source A-pose differs from the
reference's relaxed arms; retain compatibility proportions rather than pose/rig edits.

Budgets: original armor <=25k evaluated triangles, model including production
body <=130k target. The92,178-triangle reference is excluded from that model
count and never destructively optimized. Three shared material roles: coated
white, graphite flex and restrained cyan optics. Basic blockout colors only;
no final baked textures, intricate surface detail or commercial AAA claim.

New armor has separated planar face UV charts to support basic QA; final texel
layout and baking remain later work. Build runs mesh-health, assembly and host asset-QA reports, then saves
editable source and numerical report. Errors/warnings must be reviewed, not
hidden or treated as automatically accepted. New armor UV/final topology remain
later work; no runtime/production-readiness claim from a saved blockout alone.

The two separately granted jobs used <=120s/two threads, fresh>=4GiB/no heavy job,
abort only its own process on pressure<1.5GiB/error. BUILD creates
source/armor/Blockout03/{Lancer_Armor_Blockout_v03.blend,blockout-qa.json,surface-fit.json}.
RENDER creates review/Blockout03/Lancer_Blockout03_{front,side,back,top,beauty}.png
and views.json. These are720x960 studio-color form-review views, not final PBR
beauty renders. Every image must be opened and compared with the reference.
Immutable source/base hashes and input blend are checked after both jobs.

Build02 fits all torso outline/support samples to measured horizontal body
cross-sections before constructing any panels. A missing point may move inward
at fixed height by at most65mm; central-body intersection, front/back-facing
normal and torso bounds must pass. Failed regions and collapsed outlines abort
explicitly. No outer-body extrapolation or missing anatomy is generated. Every
fitted sample/distance is recorded; visual review must assess chevron preservation.
Build03 treats the pelvis separately: find the first actual front-facing body
cross-section at nine samples across the lower guard edge, searching upward
by at most80mm and retaining at least35mm guard height. Upper vertices stay
fixed; its support recenters on the fitted panel. Actual vertical correction
and sample positions are reported separately. Torso inward65mm bounds,
front/back normals, and all QA checks remain unchanged. No surface is invented
in the crotch gap. All torso/pelvic outlines and mounts pass batch validation
before construction; limb/helmet forms still use measured bone axes/head bounds
and receive the full downstream mesh/assembly QA, not a torso-center assumption.
Failed Blockout01/02 directories and private runtime logs remain untouched.

Stop after this first form review for Nick's silhouette approval and Lead-routed
independent critique, following the repo asset skill. Do not proceed to secondary
detail, new rigging or native integration by interpreting source preparation,
native exit0 or numerical QA as subjective visual approval.
