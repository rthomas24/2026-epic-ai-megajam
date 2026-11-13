# Lancer player M01 — first armor blockout

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
