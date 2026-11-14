# SM_Fab_VerdanceStump (Quixel Megascans "Mossy Pine Stump")

A small moss-covered stump on a 140 x 128 cm ground patch of pine needles and cones, 36 cm high overall. Source: glTF tier 1, 16,814 tris; 4K base colour, normal, ORM, height and an F (fuzz) map, not used. Non-walkable dressing (R-043): one convex hull for blocking only.

## Source and licence

- "Mossy Pine Stump" by **Quixel Megascans** (rhiuf), https://www.fab.com/listings/cfa4e752-52ee-4a93-9a41-2256b921b155 (issue #197). Supplied by Nicholas Tosti
  (release `asset-sources-nicholas-20261114`, archive `QuixelMossyStump-Nicholas-20261114.zip`, SHA-256 `7ae58a27cec4339855f549b42cdbbc1babdf9f4aa04504f3dc54d50cbeebc7a5`; original kept in the git-ignored `Builds/intake6/QuixelMossyStump/`).
- **Licence: Fab Standard License** (https://www.fab.com/eula); tier acquired not recorded. **Not AI-generated**. No standalone redistribution.

## What was done (Ryan's side, Claude Code, Nov 14 2026)

- `../scripts/prep_fab_quixel_mesh.py` (Blender 5.2): mesh stood up and its origin put on the floor centre, decimated to 4,000 tris (Megascans normal map kept for the detail), one material.
- Maps: `T_Fab_VerdanceStump_BC` 2048 (seller colour kept as `export/T_Fab_VerdanceStump_BC_orig.jpg`), `_N` and `_ORM` 1024 (Megascans ORM as packed; the normal was OpenGL by the height-map check and is flipped to DirectX).
- Look: Moss shifted to Verdance teal-green (hue ~0.46, saturation x0.85); the pine-needle ground patch keeps its tan, which reads as Earth forest floor; it blends best half-sunk into Verdance's soil or where the ground is brown.
- `export_for_unreal.py`, then `../scripts/ue_import_fab_multi.py` (TD_FAB_ONLY=VerdanceStump): `/Game/TriadDrift/Gameplay/Props/Fab/VerdanceStump/` (`SM_Fab_VerdanceStump`, `MI_Fab_VerdanceStump`, `T_Fab_VerdanceStump_*`), Nanite on, one UCX hull.
- Estimated cooked size about 4.9 MB (BC 2.7, N 1.3, ORM 0.7, mesh about 0.2).
- Sheets: `review/Fab_VerdanceStump_material_sheet.png` (ours), `review/Fab_VerdanceStump_seller_sheet.png` (the package's own renders).
- Suggested use: Verdance: scatter in clearings and at the research site edges, partly sunk so the patch rim hides; scale 0.8-1.5. Not placed anywhere; the environment builder scatters it.
