# Lancer reference construction — proposed next method

Status: SOURCE PROPOSAL ONLY. No new geometry or claimed quality improvement.
Design authority: Nick's white/graphite/cyan four-view sheet, most recently
C:/Users/nicho/AppData/Local/Temp/codex-clipboard-754a340c-3e67-4e68-92a4-d010f752556a.png.
Earlier approved2fa970f1 sheet depicts the same design.

## Concrete methodology change

Keep the owned Manny anatomy/scale immutable. Author the armor as individually
designed polygon components from matched front/side/back reference outlines,
with deliberate edge loops, thickness, chamfers and connected mounting surfaces.
Use body projection only for clearance validation and the inner fit. The outer
armor shape comes from the concept's panel silhouettes and structural landmarks.
This replaces the rejected method that derived broad primary forms from generic
body-following grids and angular/radial profiles.

No new people/anatomy are code-generated. This retains the existing owned-base
route; it requires no purchased assets, external generation account or licence.
Project hardsurface.py supplies finishing, inset and plate tools. Authored mesh
profiles and region-specific fitting remain editable; no random greeble layer.

## First measurable benchmark: helmet and upper torso

Calibrate the four references to the measured180.544cm body and common axes.
Record skull/chin, eye line, clavicle, deltoid width and sternum landmarks.
Reference arms are relaxed; Manny's A-pose remains unchanged. Compare helmet and
torso in matching local orthographic framing rather than misreading pose width.

Construct these concept-specific forms first:

- Helmet: segmented rounded crown with a central spine, a defined brow hood,
  angular wraparound visor with its lower central notch, separate cheek planes,
  tapered chin/respirator and recessed circular ear housings with white rims.
  Keep back-of-head panels/nape opening consistent with the side/back reference.
- Torso: broad upper breastplate shaped around the collar, paired lower rib
  facets and sternum closure; narrow nested abdominal plates; raised shoulder
  strap anchors; scapular/back plates organized around the central spine.
  Mount the pieces through actual support geometry and preserve shoulder motion.
- Deltoids: layered rounded-facet pauldrons with the reference's bevelled rim and
  tapered lower edge, with controlled clearance to the upper-arm plate.

Render front/side/back close-ups plus three-quarter material and wireframe views.
Use neutral three-point light and painted-white/graphite/rubber roughness contrast
from the look-development/PBR skills; retain clay views for silhouette assessment.
Score against the project's written rubric with a fresh reviewer. Nick's
silhouette/construction approval is the gate before applying this method below
the torso or adding tertiary detail. No fabricated approval or metric.

## Full-body component map after the benchmark gate

Use concept-shaped bicep plates/bracers with cyan forearm inserts, black elbow
flexion zones, armored glove backs, a graphite utility belt with functional
buckle/pouches and a fitted pelvis guard. Thigh armor needs shaped front/outer
facets and the lower inset; knees need distinct articulated caps. Shins/calf
plates narrow toward reinforced ankles. Boots need toe/heel armor and instep
straps following the reference's practical foot proportions.

Model the big/medium form hierarchy before vents, screws or surface wear.
Preserve existing25karmor/130ktotal targets; discuss measured budget needs with
Lead before expansion. Use mirrored editable components, UV seams following
hard edges, and a dedicated clean export copy only after visual acceptance.
No rigging, weights, native assets or gameplay changes in this milestone.

## Technical gates

Keep source skin/side-associated regional BVHs and the independent full-body
collision check. Preserve2mm clearance acceptance and40mm correction bounds.
Resolve the three remaining Revision05 findings through actual fitted support/
inner-shell geometry; do not change thresholds or hide body faces to pass.
Run asset_report, assembly_check and host asset-QA for each saved benchmark.
Inspect material/clay and wireframe views; keep full reports and actual failures.

Use project review_renders.py for comparable clay/material/wireframe contact
sheets and hardsurface.py for consistent bevels/weighted normals/insets. Apply
the host clothing, hard-surface modeling, retopology/UV and PBR/lookdev guidance
to inner fit, topology, materials and review, respectively. Skinning/export
recipes stay deferred until Nick approves the model.

## Skill gates and current hold

Project .claude/skills/blender-game-asset/SKILL.md says:
"If two rounds pass without the score improving, stop and escalate to the human:
change route or change the design." It also gates secondary work on human
silhouette approval. Revision05 has been rejected; this is an escalation proposal,
not a new accepted blockout or permission to begin another automatic build loop.

Lead coordinates checkpoint publication and the revised approach with Nick.
No further heavy operation is authorized in the current cycle, which stops saved
by21:43EST November12. A fresh explicit resource grant is required for the next
Blender job. The complete rejected history is preserved for comparison.
