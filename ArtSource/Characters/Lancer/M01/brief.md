# Lancer player M01 — first armor blockout

Stage: executable SOURCE PREPARATION ONLY; model construction is not yet run.
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

Two separately granted jobs, each <=120s/two threads, fresh>=4GiB/no heavy job,
abort only its own process on pressure<1.5GiB/error. BUILD creates
source/armor/Blockout01/{Lancer_Armor_Blockout_v01.blend,blockout-qa.json}.
RENDER creates review/Blockout01/Lancer_Blockout01_{front,side,back,top,beauty}.png
and views.json. These are720x960 studio-color form-review views, not final PBR
beauty renders. Every image must be opened and compared with the reference.
Immutable source/base hashes and input blend are checked after both jobs.

Stop after this first form review for Nick's silhouette approval and Lead-routed
independent critique, following the repo asset skill. Do not proceed to secondary
detail, new rigging or native integration by interpreting source preparation,
native exit0 or numerical QA as subjective visual approval.
