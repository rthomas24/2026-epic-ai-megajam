# Rift M01 — refined fleet foundation

Phase 1 of Nick's fleet-expansion brief. **Manual Rift approval is required before
Bastion production.** This source/review package does not contain Unreal assets,
gameplay, effects, collision, animation or final AAA acceptance.

Open [the offline browser review](../review/M01/index.html) or the
[multi-angle overview](../review/M01/Rift_M01_overview.png). The technical report,
independent art assessment and remaining limitations accompany the images.

## Source package

- `Rift_M01_v01.blend`: promoted only after assembled technical checks pass. Seven
  assembled parts use six canonical mesh roles: Hull, Pulse, Splitter, PortalGuard,
  PortalRotor and Engine. Both Pulse instances share one mesh.
- `build_rift.py`: parameterized centimeter-authored modeling using the project's
  MIT `hardsurface.py` helper kit; no downloaded models/textures or paid generator.
- `qa_rift.py`: full assembled topology/contact/material/attachment/clearance QA.
- `run_pass.py`: one scheduled build → QA → conditional multi-angle render run.
- `attachments.json`: proposed component-local and root-space socket frames.
- `unreal_visual_parts.json`: hull plus six flat static part entries, no native paths.
- `material_parameters.json`: six material parameter sets and vertex-color contract.
- `publish_review.py`: measured offline HTML review and image hashes.

Rift's crescent planform is one continuous airfoil shell, with a compact central
keel, canopy, drive fairings and nose-ring supports. Mechanism detail is clustered
around those functions rather than distributed as unrelated kitbash pieces.
Read brief.md for dimensions, performance target, direct authorization and the
documented features.md-versus-local-routing reconciliation (Lead e4b5715).

## Coordinate / material contract

UE source units: centimetres, +X forward, +Y right, +Z up. Blender uses metres,
-Y forward, +X right, +Z up. All mesh origins are documented mounting/spin pivots;
scale is positive and applied. Hull is centered on the flight origin, not on the
floor. PortalGuard and PortalRotor share the bearing center; only the inner rotor
is intended for future X-axis spin. No animation is implemented.

Six materials: tinted dielectric Paint, Composite, bare Titanium, opaque Canopy,
controlled Magenta emission and light Ceramic. No image textures, missing texture
dependencies, procedural bake assumptions or transparency. On future Unreal
import preserve vertex colors (do not replace with white):

```
BaseColor = parameter BaseColor * VertexColor.RGB
Roughness = parameter Roughness * (0.8 + 0.4 * VertexColor.A)
```

Paint's neutral-white parameter is colored by the vertex data into violet/plum
and small livery panels; actual surfaces are not pure white. Material instances
should expose base color, roughness, coat and emission strength. Alpha controls
finish variation, not opacity. The numeric emission-area gate is <=5% of assembled
triangle surface; rendered bloom is disabled, so glow does not conceal form.

## Proposed Ryan / VFX handoff

No new gameplay interface is implemented or assumed. Current `TDPresentation.h`
supports Hull as `UTDShipVisuals.Mesh` and six `FTDShipVisualPart` entries using
`ParentSocket=None`, root-relative translations, identity rotations and unit
scale from the derived JSON. `MaterialOverrides` remains available. Proposed
future instance path: `/Game/TriadDrift/Presentation/DA_Rift_Visuals`.

The part named **Engine** matches `ATDShip::GetEngineAttach`'s preferred component.
Its origin is the aft twin-drive midpoint; separate Thruster_L/R local locators
give VFX both exhaust positions. Effects should emit along the documented -X
direction. VFX/audio owners supply their effects; no Niagara/audio files are owned
or edited by this task.

**Muzzle-binding dependency:** current `UTDWeaponComponent::GetMuzzleTransforms`
enumerates contiguous `Muzzle_0...N` on the first static mesh that has them. The hull
aliases Muzzle_0/1 identify the two Phase Pulse guns. The separate Splitter cannon
has Muzzle_Secondary. Engineering must review weapon-specific selection before
native integration; simply importing these sockets would not make the current
generic lookup choose the Splitter origin. Do not silently change firing/spawn
offsets to hide that mismatch. Other proposed locators cover portal spin/exit,
cloak/shield origin, cockpit camera, weapon mounts and a chase-camera reference.
They are an art manifest awaiting engineering adoption, not finalized runtime API.

Potential future native roots are exclusively reserved under Art/Ships/Rift,
Art/Weapons/Rift and the agreed flat Presentation asset path, but **remain
read-only in this source-only phase**. No native asset, export preset, collision
proxy, LOD chain, cook result, engine performance or gameplay playtest is claimed.
Later export/import must recheck normals, vertex colors, sockets and UE scale.

## Reproduce and review

Use installed Blender 5.1.1 (the skill targets 5.2; report the actual version).
From the Rift directory, in a Lead-approved heavy slot:

```
blender --background --factory-startup --threads 2 --python-exit-code 1 --python production/run_pass.py
```

Check fresh RAM and competing Blender/Unreal/compiler processes first. Use one
two-thread process on the 16 GB host. Save versioned source before temporary
studio/overlay objects. Failed runs keep reports plus hero/clay diagnostics;
only a passing source earns the full review set. Actual-edge wire curves are
temporary overlays, not a Wireframe modifier changing the mesh. The 128 px black
silhouettes are enlarged in their inspection sheet. Views are independently
framed; numeric dimensions establish size.

UVs are a valid simple unwrap for flat/vertex-driven materials, not a production
baked atlas. Mechanical overlaps are deliberate where independent closed modules
need connected mounting surfaces; all retained warnings require explicit reasons
in the technical report. Screenshots and numeric checks do not establish final
visual or gameplay acceptance.

## Agent continuity

Agent: **Ships & Weapons Agent**. Role: **ship and modular weapon visual asset
development**. Completion/partial/blocker reports start with Agent, Role and Task
and include paths, stage, branch/worktree, commit/push/PR, verification, ownership,
remaining approval, verified usage if available and task duration.

Work only on `nick/ships-rift`, `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/ships-rift`,
in this production directory and review/M01. Main e4b5715 published the exact claim
before the first edit. Lancer 4b7ef12 is frozen and byte-preserved; its unmodified
hero reference is copied into this review for fleet comparison. Bastion stays
queued, with no production before Nick approves Rift. Lead owns all Git remote
sync/precommit/publication, shared boards and completion sound (once). Do not edit
other agents, maps, gameplay/config, native assets or shared instructions.
