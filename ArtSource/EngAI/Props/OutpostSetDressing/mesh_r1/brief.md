# #40 Outpost & landing-pad set dressing — mesh_r1 (brief)

Requested in `MESSAGEBOARD.md` (R-030 / R-042 list item 7, GitHub issue #40): **8–10 small props at real size, sizes in
the manifest**, after Nicholas's landing-pad set-dressing board
(`ArtSource/Reference/LandingPadSetDressing/Landing Pad Set Dressing Modeling Reference.png`).

## Rules that bind this asset
- **R-043:** nothing walkable is an AI mesh; Grok Bot delivers set dressing #40 as out-of-reach / distant dressing.
  These props are Route C Blender hard-surface (Claude audit D3), with simple `UCX_` boxes.
- Ryan: polished in Blender with sharp geometry: live Bevel + WeightedNormal, an editable `.blend` per prop with a
  hidden `SRC_` copy, real bevelled edges.
- Sizes in cm in `manifest.json`; FBX `UnitScaleFactor` 100, Unreal import scale 1.0; pivot at the ground-base centre,
  +X forward, +Z up.
- One trim material, 1K delivery textures (2K masters), BC dark metal median sRGB ≥ 55, DirectX normals, ORM = AO/R/M.
- Contact sheet on a neutral HDRI with a 180 cm figure; `mesh_report.py` and `tex_stats.py` numbers in `SOURCE.md`.
- One PR for Lead review; the Lead merges.

## The ten props (brief sizes, cm)
Cargo crate 120 × 100 × 100 · Supply crate 110 × 90 × 90 · Field terminal 100 × 70 × 125 · Data kiosk 110 × 85 × 145 ·
Comm antenna 90 × 90 × 260 · Comms dish 160 × 160 × 230 · Floodlight 95 × 85 × 220 · Bollard light 45 × 45 × 110 ·
Generator 170 × 110 × 125 · Power junction 120 × 75 × 135.
