# Cinder dig site — drill rig + walkable site cabin - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (Nicholas's six reference
boards, item for GitHub issue #38) and tightened by **R-042** walkability
(doorways at least 140 × 240 cm, interior ceilings at least 280 cm, flat
floors, ramps at most 20°): a tall excavator-mounted piling drill rig plus a
raised, walkable, empty-bay site cabin. Built in Hyper3D Rodin Gen-2.5 from
the cleaned concepts (`refs/38_drill_rig_clean.png`, `refs/38_cabin_empty.png`),
themselves derived from Nicholas's modelling reference
(`ArtSource/Reference/CinderDigSite/Cinder Dig Site Modeling Reference Board.png`,
copied into `refs/`). Builders do the final import, collision and placement;
this drop is source plus an engine-ready derived copy only.

## Spec
- **Where it shows:** the Cinder dig site. The cabin is a walk-in hero prop —
  hollow interior bay, open front doorway, wrap-around deck, front stairs.
  The drill rig is a separate tall piling / drill-mast vehicle that stands
  beside it.
- **Style:** industrial Cinder language — weathered charcoal-dark plates,
  yellow-amber hazard markings and railings, steel-grate deck, roof comms
  mast and dish on the cabin; yellow-and-gunmetal tracked chassis, hydraulic
  boom, vertical derrick mast and rotary head on the rig.
- **Budget / format:** each mesh **~20k–24k tris** (strictly under 30k), 2K
  PBR (BC, N, ORM, E where real lights exist), FBX preferred.
- **Scale / pivot:** rig mast height **11.0 m** (realistic excavator piling
  rig, 10.5–12.0 m). Cabin scaled so the interior soffit clears **285 cm**
  (safe margin above R-042's 280 cm floor-to-ceiling requirement). Pivot at
  the ground-base centre, +Z up, +X forward (rig mast/cab and cabin
  doorway/stairs face +X).

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no
  names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them
  to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder
  (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO,
  G=roughness, B=metallic. Delivery normals are DirectX.
- **R-042:** doorways ≥ 140 × 240 cm; interior ceilings ≥ 280 cm; flat floors;
  ramps at most 20°.

## What this round delivers
- `SM_CinderDrillRig.fbx` and `.glb`: the tall piling drill rig. **22,000
  tris**. Overall **781.5 × 413.8 × 1,100.0 cm** (X × Y × Z). Mast height
  **11.0 m**. Cab roof **311.1 cm**, track height **44.0 cm**. Pivot at the
  ground-base centre, min Z = **0.0 cm**, centred in X and Y (X −390.8 to
  +390.8 cm, Y −206.9 to +206.9 cm). +Z up, +X forward (tracks, mast and cab).
- `SM_CinderSiteCabin.fbx` and `.glb`: the raised walkable site cabin.
  **22,000 tris**. Overall **1,668.0 × 1,554.9 × 1,232.2 cm**. Doorway
  **625.5 × 312.1 cm** (R-042 ≥ 140 × 240). Interior clear height **285.0 cm**
  (R-042 ≥ 280), floor **289.9 cm** (p5–p95 spread 0.59 cm, no furniture),
  ceiling **574.8 cm**, bay depth **411.2 cm**, side-deck height **257.4 cm**.
  Pivot at the ground-base centre, min Z = **0.0 cm**, centred in X and Y
  (X −834.0 to +834.0 cm, Y −777.4 to +777.4 cm). +Z up, +X forward (open bay
  and front stairs). Combined **44,000 tris**.
- The FBX uses the repo exporter's Apply Unit + FBX Units Scale
  (`UnitScaleFactor` 100) so Unreal should import both at the sizes above
  with uniform scale 1.0. Prefer the FBX. The GLB is a metre-scale, Z-up
  companion.
- `T_CinderDrillRig_{BC,N,ORM,E}.png` and `T_CinderSiteCabin_{BC,N,ORM,E}.png`
  at 2K. N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness,
  B=metallic. Both E maps are kept (thresholded noise; real cab/beacon lights
  on the rig, interior light bars on the cabin).
- `source/drill_rig/` and `source/cabin/`: the raw Rodin Base Model packs,
  untouched, plus the Rodin viewer stills. Zips omitted. (The drill pack did
  not include a `base.obj`.)
- `refs/`: `38_drill_rig_clean.png`, `38_cabin_empty.png`, and a copy of
  Nicholas's Cinder Dig Site modelling reference board.
- `contact_sheet.png` (under 5 MB): multi-view of both meshes, a doorway and
  Y = 0 cutaway of the hollow bay with a 180 × 80 cm pilot standing on the
  interior floor, a composite placement of both assets with pilots at the rig
  and in the cabin, plus the concept thumbnails and a stats header.

**Cabin scale note:** Nicholas's board footprints the cabin at 9.6 m. At that
size Rodin's hollow bay only cleared 164.0 cm — short of R-042's 280 cm
interior ceiling. Uniform scale ×1.737507 from that 9.6 m mesh sets interior
clear height to 285.0 cm (and the doorway to 625.5 × 312.1 cm). The 16.68 ×
15.55 m footprint is larger than the loose 8–13 m guideline because
walkability is the hard requirement. The 11.0 m drill rig is unchanged.

**Stairs note:** the front access is stepped stairs (level treads), not a
ramp. The flight angle is 28.2° over a 540.4 cm run / 289.9 cm rise. R-042's
20° cap applies to ramps; this mesh has no ramps.
