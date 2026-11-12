# Lancer M03 — Production Review 01

Latest checkpoint: **Structural A02**, technically verified and awaiting Nick's
structural review. Open [A02 comparison page](../../review/M03/StageA02/index.html)
or `Lancer_M03_structure_v03.blend`. It preserves A01, compares actual front/side/
top and three-quarter views against A01 and the approved reference, and passes
mesh/contact/36-frame/12-ray/material-role validation. Detailed surface refinement,
other ships and native integration remain held. The earlier A01 failure record
below remains historical; v02 is the preserved first A02 attempt and v03 corrects
material-slot assignment and primary-plane shading. See the A02 report for limits.

Nick's approved blue military-spacecraft turnaround in `refs/Lancer_M03_authority.png`
is the visual authority for this redesign. The rounded white/cyan M02 remains
unchanged for history, technical reference and rollback. No old files are replaced.

Current checkpoint: structural A01 exists as `Lancer_M03_structure_v01.blend`.
It has clean mesh-health checks,36 passing frames and12 clear muzzle/exhaust rays,
but **fails the assembly gate** on one hidden nose insert. See the preserved
[A01 diagnostic browser page](../../review/M03/StageA01/index.html) and
[measured review](../../review/M03/StageA01/review.md). Targeted corrections are
prepared in source and await another Lead-cleared Blender pass. They are not yet
verified. Subsequent structural saves increment their version and preserve A01.
The final Production Review01 links below are planned deliverables, not present
or approved at this checkpoint.

Stage and actual measurements are in
[the review report](../../review/M03/Review01/report.md) and measured JSON files.
Open [the browser review](../../review/M03/Review01/index.html) for the actual
Blender renders, reference comparison, source links and remaining limitations.
Source names: `Lancer_M03_structure_v01.blend` (structural checkpoint) and
`Lancer_M03_v01.blend` (written only for a technically passing refined candidate).
The model remains subject to Nick's manual visual approval, not automatically
AAA-ready, Unreal-ready or gameplay validated because scripts passed.

## Geometry / material source

`build_m03.py` uses the repository MIT hard-surface helper and original,
reference-led polygon construction. Explicit phases establish primary structure,
weapons/engines, then armor/detail and microfinish materials. Canonical Pulse-like
energy hardware from other ships is not used; the new reference's two armored
single-bore cannons retain Lancer's existing frames through the explicit mapping.

Twelve assembled components use eight reusable mesh roles: Hull, Wing_L, Wing_R,
Canopy, Primary, Rotor (technical legacy name for new barrel), Secondary and Engine.
Both cannons, barrels, bays and drives reuse canonical meshes. Wings, canopy and
two drives remain independently swappable components with documented pivots.

Six materials separate graphite paint, gunmetal, titanium, dark composite, opaque
canopy and electric-blue emission. Two uninterrupted forward-fuselage conduits,
wing edges, engine cores, intake backing and weapon rings implement the supplied
lighting language. There are no randomly sprinkled emissive decorations.
`material_parameters.json` records base color, metallic, roughness, coat and
emission. Blue hue and intensity can be tuned at its material's Principled node.

Vertex color `PanelTint` controls per-panel albedo and roughness. Brushed grain and
subtle surface imperfections use low-amplitude object-space Noise/Bump on metal
and paint, and thin exposed-metal chamfers provide edge separation. They are
self-contained Blender nodes, not missing textures. **Export dependency:** bake
the microfinish or reconstruct an equivalent, exposed Unreal material before
native adoption; FBX does not carry arbitrary Blender shader graphs. No texture
bake, native material instance, engine shader-cost or runtime-performance claim.

## Immutable attachment baseline

`refs/legacy_attachments.json` is an exact byte copy of the original33-frame
manifest. `legacy_mapping_proposal.json` records every old local/root position,
axis, parent and part alias. The builder guards the canonical manifest SHA.
UE centimetres: +X forward, +Y right, +Z up. Blender metres: -Y forward,
+X right, +Z up. All source mesh scales are positive unit values.

- Primary receivers retain roots `(30, ±100, -42)` cm. Barrel components retain
  old Rotor roots `(65, ±100, -57)`; center muzzles remain `(204, ±100, -57)`.
  The old six barrel frames per gun remain clear technical reference points
  inside the single open bore. They do not claim six visible barrels.
- Flush underwing secondary bays preserve old roots `(-90, ±220, -44)` and all
  eight existing rocket exits. No firing mode, ammunition or damage changes.
  They add discreet underside equipment omitted by the concept; inspect bottom
  views before accepting its visual fit. This source proposal was accepted by Lead
  for frame preservation, not by Ryan as final native binding.
- Legacy Hull thrusters stay `(-402, ±111, -3)`, with independently placed drives
  around those centers. New Hull `Engine` midpoint and camera reference sockets
  are supplemental proposals, not changes to the original33. Wing pivots and
  canopy rear pivot are recorded in the component assembly metadata.
- Rocket visual-only offset `(54,0,0)` cm remains the agreed handoff. Projectile
  collision/spawn and existing gameplay collision are unchanged; none is authored
  here. Old projectile source remains the reference, not silently regenerated.

`unreal_visual_parts.json` describes Hull plus eleven root-relative static parts,
`ParentSocket=None`, identity rotation and unit scale, with mesh/material slot
names. Native paths remain unset. Existing `UTDShipVisuals` / `FTDShipVisualPart`
supports this static composition; no new gameplay interface is created.

Current generic first-mesh `Muzzle_0...N` lookup does not itself adopt these
legacy named primary/secondary frames or select weapon-specific banks. Ryan must
review native socket/bank binding before import; this package does not implement
that wiring. `Engine` is proposed for the existing Hull-socket fallback when
separate Engine_L/R parts are used. VFX/audio owners supply actual trails, flashes,
boost and EMP effects; only visual hardware/locators are owned here.

## Verification / reproduction

Run only in a Lead-approved heavy window, one process/two threads, with fresh
RAM/process checks on the16GB system. Installed Blender is5.1.1, not the skills'
target5.2. From the Lancer directory:

```
blender --background --factory-startup --threads 2 --python-exit-code 1 --python production/M03/run_pass.py -- --structural
blender --background --factory-startup --threads 2 --python-exit-code 1 --python production/M03/run_pass.py
```

`qa_m03.py` measures all assembled mesh geometry, normals, UVs, contacts, material
parameters, axes, frame positions and muzzle/exhaust rays. It does not prove
dynamic fold/animation clearance, collision/cook/LOD/export correctness or actual
gameplay performance. `render_m03.py` creates every angle from the same scene;
temporary actual-edge wire curves do not modify source topology. Bright/dark
views preserve the same geometry and emission parameters. Final source is saved
before temporary review lighting and cameras. Re-render only under a new slot.

## Ownership / continuation

Branch `nick/ships-lancer-m03`, worktree
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/ships-lancer-m03`.
Claim bcdd260 was published to main before writes. Only production/M03/** and
review/M03/** are writable. Shared maps, gameplay/config, native assets, old
Lancer, Rift, Bastion and Null Armada assets remain untouched. Lead owns remote
preflight/publication, shared boards and the single completion cue.

Agent: Ships & Weapons Agent; role: ship and modular weapon visuals. Reports begin
Agent/Role/Task and include absolute source/review paths, metrics, verification,
branch/worktree/commit/push/PR, conflicts, remaining approvals and duration.
Exact model/effort/usage only if verified. See campaign_queue.md: finish/publish
LancerReview01 and obtain Nick approval; then Bastion; after its approval green
Rift. Never interpret silence as approval or interrupt the active ship.
