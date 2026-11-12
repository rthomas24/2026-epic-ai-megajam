# Standard infantry M01 — design and production brief

## Purpose and authority

One original Null Armada infantry humanoid for ground combat, primarily seen from
the player's first-person view at roughly1–15m. This is a source-model milestone,
not a gameplay implementation. Match Nick's supplied front/back/left/right sheet
`86c032e1` as closely as practical. Its dark gunmetal/charcoal, restrained crimson
and narrow orange visor override the older general white-eye faction shorthand
for this character only. No gameplay color/config change is implied.

Shape language: **athletic, angular, enclosed**. The same helmet, armor seams and
mounting construction must agree in all views. Avoid broad heavy-special shoulders,
sensor clusters, exaggerated anatomy, decorative spikes and franchise identifiers.

## Foundation, proportions and scale

Route B: reuse the already-owned UE5 Manny-compatible body as an anatomical and
joint-location foundation; author original modular armor. Never synthesize a human
from primitives. N011 approves read-only export of existing mannequin packages;
there is no approved source FBX yet. Do not use LFS pointer text as a mesh.

Target about180cm standing,1 Blender unit=1m, feet at origin. Preserve Manny's
existing A-pose proportions and joint centers for eventual SK_Mannequin compatibility.
Confirm measured base height and axes after export; do not assume the export is
180cm or silently distort the skeleton to match an illustration. Unreal target is
+X forward, +Z up,1unit=1cm. Preserve a source-coordinate record and calibration.
Existing capsule radius34/half-height90cm is integration reference, not new geometry.
The user reference has relaxed lowered arms; match armor proportions while retaining
the animation-compatible rest pose. Document that pose difference in review views.

## Reference-specific construction

| Region | Required form and attachment |
|---|---|
| Helmet | Enclosed narrow head, faceted crown, central brow/face keel, shallow swept V-shaped orange visor, layered cheek/jaw protection; round side mechanism and restrained rear vents |
| Shoulders | Angular caps over a visible flexible shoulder socket, modest athletic width; crimson recessed trim, no heavy-special oversized silhouette |
| Chest/back | Paired upper pectoral plates around center insert, layered lower rib/abdomen segments; compact back spine/service plate tied into shoulder mounts, no bulky backpack |
| Undersuit | Charcoal flexible body with directional joint ribbing; visible neck, armpits, inner elbows, waist/hip flex zones, knee backs and ankles |
| Arms/hands | Separate biceps, elbow caps and tapered bracers; hand dorsal plates plus articulated fingers, thumb web and wrist clearance; no solid mitten or forearm-to-hand bridge |
| Waist/hips | Narrow utility belt with coherent pouches/buckle and attached rear panel; separate central pelvis and lateral hip plates leaving groin and hip-fold clearance |
| Legs | Long tapered thigh plates with angular outer facets, distinct floating-on-mount knee caps, calf/shin armor open at the knee back and ankle |
| Boots | Layered toe/instep, ankle side hinge and practical sole; toe flex and believable foot volume, no oversized platform or fused shin/boot |

Model left/right geometry consistently. Preserve any intentional asymmetry visible
in the reference, but do not infer extra equipment from shaded image artifacts.
Retain broad calm panels. Seams follow plate boundaries and mechanics; bolts,
vents and wear belong near functional clusters rather than uniform random noise.
Every armor element must visibly mount to the undersuit or another plate. Avoid
coincident shells, hidden duplicate surfaces and unsupported floating accessories.

## Practical budget and materials

Provisional LOD0 target **35k–55k evaluated triangles**, ceiling **60k** including
body, armor, gloves and boots; revise only after actual base inventory through Lead.
Do not claim a measured count before geometry exists. No subdivision above what
silhouette needs. Mirrored reusable plate modules are editable source components;
future merged export copies/material batching are a separate approved stage.

Target **4 shared material roles**: coated gunmetal/charcoal armor, flexible black
undersuit, restrained crimson trim, orange optical insert. Reuse slots across
modules rather than creating one material per object. Painted/coated surfaces are
dielectric; bare-metal edge patches may use metallic1. Mid-to-dark albedo must
remain visible under neutral light; never pure black. Armor roughness about0.32–0.5,
undersuit about0.65–0.85, subtle original surface variation. The narrow visor is
the focal emissive; small forearm/back/heel accents stay restrained, under5% total
visible surface. No bloom that turns the orange white. No downloaded textures or
new asset generation service. M01 basic PBR materials first; bake/export later.

## Production sequence and approvals

1. Fresh Lead resource grant, read-only base export, version/environment report,
   inspect actual scale/axes/bones/topology and save provenance. No source overwrite.
2. Save a versioned base/primary-form checkpoint. Compare all four views with the
   supplied sheet before secondary detailing. Send silhouette evidence through Lead
   under the repo skill's staged review rules; no invented human approval.
3. Refine original armor and joint spaces, then restrained materials. Keep editable
   source and controlled backups. Fresh reviews follow the asset rubric.
4. Execute `qa-plan.md`, inspect actual clay/material/wire views, correct defects
   and record remaining limitations honestly. Build the complete M01 review set.
5. Lead reviews/preflights/commits/pushes only owned files. Stop for Nick's visual
   approval. No rigging, weights, animations, Unreal character import, presentation
   assignment or additional character before that gate and separate authorization.

M01 success is one convincing consistent full-body source model ready for manual
review. A brief, mannequin export, clean mesh report or generated image alone is
not completion and does not establish final production/gameplay readiness.
