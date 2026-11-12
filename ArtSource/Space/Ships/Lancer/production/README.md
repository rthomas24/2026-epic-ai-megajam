# Lancer M02 — production refinement candidate

**General direction and modular layout approved; updated polish approval pending.**
This is Blender source and visual review evidence. It has not been imported into
Unreal, accepted as final production art, or verified in gameplay. Technical
status is recorded in [validation_summary.json](../review/M02/validation_summary.json);
a failed candidate is never promoted by the build script.

## Review

Open [the offline review page](../review/M02/index.html), or the
[nine-view overview](../review/M02/Lancer_M02_overview.png). Full-resolution front,
rear, side, top, three-quarter, underside, isolated weapons and rocket images
are accompanied by a six-angle clay sheet. Historical failures and intermediate
images remain under `review/M02/round01`, `round02` and `round03`.

This pass refines the approved needle fuselage and forward-swept wings with
continuous root fairings, a framed canopy, shaped gun cradles, radiator banks,
service panels and machined engine hardware. Coated white alloy, dark composite,
bare titanium, cyan paint and small cyan emissive inserts share six materials.
Broad panels remain deliberately quiet. It is an incremental art candidate;
technical checks do not establish cinematic/AAA quality.

Independent hero review flags abrupt blue-grey/dark transitions at the wing roots,
coarse-looking cradle shoulders and a gloss mismatch between canopy/guns and broad
armour. Inspect these in the full-angle and clay views before approving further
production. The review evidence is deliberately candid about these limitations.

## Source and reproducibility

- `Lancer_M02_v03.blend`: written only after all technical gates pass; contains
  six reusable mesh roles, eleven assembled instances and 33 attachment empties.
- `build_m02.py`: deterministic refinement of the existing six M01 source roles;
  uses the repository's MIT hard-surface helper kit. No downloaded models,
  external textures or image-generated geometry.
- `run_m02_pass.py`: factory-startup build → QA → conditional full review.
- `qa_m02.py`: actual delivered geometry, contacts, axes, source hashes and rays.
- `material_parameters.json`: adjustable PBR values and vertex-tint contract.
- `unreal_visual_parts.json`: derived static assembly mapping, no native assets.
- `publish_review.py`: builds the offline review page from measured reports.

Run only in a Lead-approved heavy slot, normally with at least 3 GiB free RAM and
no other Blender, Unreal, compiler or shader-worker process. Lead explicitly
cleared the measured small final job below that threshold: 2.79 GiB free at launch,
one two-thread process, exit 0 after about 22 seconds. Tested tool is Blender
**5.1.1**, not the skills' target 5.2. Use background factory-startup, two threads,
`--python-exit-code 1 --python production/run_m02_pass.py` from the Lancer folder.
Rebuilding creates a working checkpoint; the versioned candidate is saved before
temporary studio cameras, lighting and the isolated projectile review instance.

## Geometry and attachments

Source UE coordinates are centimetres, +X forward, +Y right, +Z up. Blender uses
metres, -Y forward, +X right, +Z up. All mesh scales are positive unit values;
the hull origin and each module's original local pivot are retained. Geometry is
triangulated with UVs and `PanelTint` corner colour. Tiny cap slivers are welded
at 0.01 mm before QA. No decimation changes the approved form.

The six original mesh JSON files and `attachments.json` are read-only and
SHA-256 checked before/after each build. All 33 names, parent-space positions,
forward/up axes and right-handed bases are preserved. Ten muzzle and two thruster
rays are tested along their true forward axes for 100 m. Contact QA checks detail
islands and coplanar surfaces; it does not establish dynamic gameplay clearance.

Hidden back-to-back mechanical surfaces can remain at assembled joints to
preserve independently swappable closed modules. The spacecraft sits around its
flight origin, so its bottom is intentionally below ground zero. See the full
geometry report for exact warnings. UV packing is a simple valid game-jam unwrap,
not a baked texture atlas. Collision proxies, LODs, engine split-vertex cost,
draw calls and in-engine performance have not been validated in this stage.

## Ryan / presentation integration contract

After Nicholas approves this refinement, export/import requires a separate
verified handoff. Do not import this source automatically.

1. Reconstruct six Unreal materials from the parameter JSON. Multiply linear
   BaseColor by imported vertex colour RGB (`PanelTint`). Preserve material IDs;
   roughness, metalness, coat and cyan emission should remain instance parameters.
   The canopy is intentionally opaque; no refraction setup is required.
2. Per engineering reply `8cd1561`, use `UTDShipVisuals.Mesh` for the hull and
   ten `FTDShipVisualPart` entries. `ParentSocket=None`; root-space translations
   come from the derived JSON, identity rotation and unit scale. Keep PartName
   and reusable mesh pivots. Native asset paths are deliberately unset.
3. The original manifest's 33 component-local locators remain authoritative.
   Resolve each locator on its named visual part. Static flattening does not
   implement rotor animation; dynamic parent/rotor wiring remains engineering work.
4. For SpikeRockets use `UTDWeaponFX.ProjectileMeshOffset=(54,0,0)` cm. The original
   centre-pivot mesh extends 54 cm rearward; at the unchanged muzzle this visual
   offset gives 4 cm tail clearance beyond the pod lip. Do not move collision,
   spawn position or canonical muzzle locators to compensate.
5. Existing VFX/audio owners supply weapon, engine and ability effects at these
   locators. No firing, damage, cooldown, ability or VFX logic is included here.

## Ownership and agent continuity

Agent: **Ships & Weapons Agent**. Role: **Ship and modular weapon visual asset
development**. Every completion/partial/blocker report starts with Agent, Role
and Task fields, and includes branch/worktree, commit/push, files, verification,
ownership, integration readiness and remaining approval.

Work only on `nick/ships-lancer` in the existing ships-lancer worktree. This M02
task writes only `production/**` and `review/M02/**`, claimed through Lead at
`2d4831f`. Do not change M01 history, shared instructions, maps, configuration,
gameplay code, VFX or other agents' assets. Lead performs fresh remote ownership
and serialized Git preflights and writes MESSAGEBOARD. Obtain a heavy slot before
any Blender/Unreal run. No main merge or final quality self-approval.
