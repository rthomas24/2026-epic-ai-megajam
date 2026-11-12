# Lancer Blender silhouette review — 2026-11-12

**Agent:** Ships & Weapons Agent\
**Role:** Ship and modular weapon visual asset development\
**Task:** Validate the six existing source meshes and prepare silhouette approval images

Open [the review overview](index.html) or [the nine-view image](Lancer_overview.png).
The review task is complete; **production geometry QA does not pass**. Manual
silhouette approval is still pending. No final hull, Unreal asset or gameplay
feature is approved by this report.

## Verified geometry and attachment results

- Blender **5.1.1**, build `b70da489d7f4`, Python 3.13.9, background process.
  This was the installed version; the skill library targets 5.2+. No software
  install or unsupported claim of Blender 5.2 testing is made.
- All six mesh JSON files and `attachments.json` match their original SHA-256
  fingerprints. Loaded vertex positions match within 1 micrometre and every
  triangle index is unchanged. No mesh regeneration or silhouette edits occurred.
- Assembly: **10,528 triangles**, 11 mesh instances, 98 closed solid islands,
  six materials, **798 × 680 × 213 cm** (length, span, height).
- All six canonical source meshes: **zero open boundary edges, non-manifold
  edges, inconsistent winding edges, inverted islands or degenerate faces**.
- Every mesh instance has unit positive scale and unapplied rotation zero.
  Hull pivot remains (0,0,0). Rotor pivots remain (65,±100,-57) cm in hull space,
  corresponding to local gun-space (35,0,-15) cm. No pivot relocation occurred.
- All **33 attachment positions and forward axes pass** after fixing a loader
  conversion error. Ten grouped muzzle/tube rays and both thruster rays are
  unobstructed against the entire assembly for **100 m**. Manifest offsets remain
  3 cm ahead of the minigun fronts and 4 cm ahead of rocket tube lips.
- **Ray clearance is not projectile-volume clearance.** The centre-pivoted rocket
  extends 54 cm rearward. Placing its centre directly at a rocket socket x=112 cm
  puts its tail at x=58 cm, **50 cm behind the tube lip** at x=108 cm. A +54 cm
  visual offset along the socket's forward axis keeps the complete axial envelope
  4 cm beyond the lip. Alternatively, Ryan must explicitly handle tube egress.
  The rocket's fins also exceed the bore radius, so unoffset placement cannot be
  treated as a validated collision-free launch. The conservative axial check is
  in `projectile_envelope.json`; no gameplay solution is implemented or presumed.
- Provisional Smart UV Project maps were created in Blender memory only. The
  checker found no zero-area UV faces or sampled overlaps. This is not a final
  texture unwrap or a baked texture set.

## Preserved failures and warnings

| Finding | Measured evidence | Production implication |
|---|---|---|
| Five detached hull vent details | Vent_L_2, Vent_L_3, Vent_R_1, Vent_R_2, Vent_R_3; nearest main-group gaps approximately 3.1–9.0 cm | Their mounting needs correction after the silhouette decision. |
| Two detached front rotor clamps | FrontClamp in Rotor_L and Rotor_R; approximately 2.9 cm from the main contact group | Each clamp needs a believable connection; the standalone rotor QA also flags its disconnected mechanical pieces. |
| Four coplanar mount interfaces | MountPrimary/Primary and MountSecondary/Secondary, both sides; 3 overlapping coplanar triangle pairs each (12 total) | Remove/offset hidden coincident surfaces during production cleanup. |
| Duplicate hull interface vertices | 48 pairs within 0.1 mm in the combined hull mesh | Separate source solids share positions; weld only where appropriate after design approval. |
| Hidden opposing faces | Hull 192 triangle-pair findings; each rocket pod 8 | Potential wasted geometry at intentional intersections; not removed for this review. |
| Unused material slots | Six palette slots retained on each reusable mesh | Compact slots during final export to reduce ambiguity and potential draw-call costs. |
| Projectile visual launch envelope | Centre-pivoted rocket extends 50 cm back into the pod if placed directly at its muzzle socket | Ryan must agree the visual offset/egress contract; straight-ray tests alone do not validate this volume. |

The 2 mm contact tolerance is explicit in `assembly_contacts.json`. These are
**existing source findings**, not new render defects. They remain unresolved
because this task requests source preservation and silhouette review, rather than
mesh redesign. The aircraft/spacecraft pivot intentionally sits above its lowest
vertex; the ground-plane warning at z=-0.97 m is expected for this asset.

## Loader and review fixes

The original adapter had two executable faults that source-only checks missed:

1. The project palette helper returns three RGB values. Blender's color inputs
   require RGBA; the adapter now adds alpha=1.
2. A reversed cross product created an improper/reflected socket basis and
   reversed the locators' forward direction. The adapter now uses
   `right = up.cross(forward)` for its local -Y forward convention.

`validation_initial_before_socket_fix.json` preserves the failed orientation
results. `validation_summary.json` contains the corrected pass for all 33
locators. These fixes do not change any source position, triangle, manifest or
ship geometry. The adapter also now accepts an explicit review-only flag without
confusing silhouette review with final production authorization.

The repository's Wireframe-modifier diagnostic initially produced long miter
spikes at acute corners; the first helper report records that clipping warning.
This is an overlay artifact, not loose source geometry. The final wire sheet uses
thin, independent curves on the actual mesh edges, with no connected miters and
no source changes. `wire_overlay_manifest.json` documents that correction.

## Visual inspection and decision

Nine material renders show front three-quarter, front, rear, starboard side,
top, rear three-quarter, underside mounts, isolated port weapon modules and the
separate spike rocket. Clay and actual-edge wire sheets supplement the material
views. Orthographic images in the HTML are framed independently for readability;
the clay sheet's orthographic cells share a common scale.

The cyan/white treatment is readable, the narrow prow and forward-swept wing tips
are visible, and both drive apertures and weapon systems can be inspected. The
design remains a simple proxy: the tall fins, thin wing profile, long prow and
large exposed pods are the main proportion choices to approve or request changes
to. Material renders show the source's facets and intersections rather than
masking them with added bevels or smoothing geometry. No emissive VFX, muzzle
flashes, trails or ability effects have been authored.

**Nicholas: approve the silhouette for the next production milestone, or specify
the desired proportion/silhouette changes.** Either decision leaves the documented
geometry cleanup pending. No final design approval has been inferred.

## Rendering, resources and verification scope

- EEVEE, Khronos PBR Neutral, neutral studio world with four area lights, no
  external HDRI or textures. Material images are 1600 × 1100 PNGs; the overview
  is a 3×3 grid. No depth of field, bloom or motion blur hides construction.
- One Blender process at a time; two render CPU threads. Lead reserved the slot
  and queued HUD work until the sequence exited. About 7.09 GiB RAM was free
  before the environment check and 6.93 GiB before the material sequence.
- Observed first-hero process working set was approximately 582 MiB. This is a
  point-in-time observation, not a full-session peak measurement. The warmed
  nine-view/clay/initial-wire render sequence completed in 8.663 seconds; its
  first shader-compilation hero check took approximately 25 seconds.
- PNG files were reopened and decoded, and the overview/clay/wire sheets were
  visually inspected. HTML local image/link targets and JavaScript syntax were
  checked. No claim of Unreal import, gameplay compatibility testing or runtime
  performance measurement is made.
- Exact Codex serving model/effort, token totals and cost were not exposed by
  available task tools; no estimates are reported.

## Ownership and integration

Branch/worktree: `nick/ships-lancer`,
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/ships-lancer`.
Changes are confined to the owned Lancer folder: loader-only fixes, the review
driver, review reports/HTML/PNGs and documentation. The Lead confirmed no incoming
Ryan or other Codex path overlap. The explicit user request authorizes these
review PNGs on the task branch; it does not authorize native Unreal production,
broader binary work, a new claim merge, or a merge of PR #3 into main.

End-of-day readiness: review artifacts and their documented failures are reviewable;
the ship is **not ready for final production integration** until design approval
and geometry cleanup. The Lead reviews the branch, dependencies and Ryan's active
work before any separately authorized main merge.
