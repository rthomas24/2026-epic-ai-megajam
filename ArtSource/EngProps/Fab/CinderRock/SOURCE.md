# SM_Fab_CinderRock (Quixel Megascans "Icelandic Sandy Volcanic Rock")

A volcanic rock mound with loose scree on a ground skirt, 455 x 417 x 174 cm. Source: glTF tier 1, 28,753 tris; 4K base colour, normal, ORM, height. Non-walkable dressing (R-043): one convex hull for blocking only.

## Source and licence

- "Icelandic Sandy Volcanic Rock" by **Quixel Megascans** (slnv6), https://www.fab.com/listings/567fc8f7-aa53-4945-ae78-342c6c7b7ea6 (issue #196). Supplied by Nicholas Tosti
  (release `asset-sources-nicholas-20261114`, archive `QuixelVolcanicRock-Nicholas-20261114.zip`, SHA-256 `1b51f25f3627c45b27c6279427395aa7da5b48ee519629aaccd5f478fcd0df7d`; original kept in the git-ignored `Builds/intake6/QuixelVolcanicRock/`).
- **Licence: Fab Standard License** (https://www.fab.com/eula); tier acquired not recorded. **Not AI-generated**. No standalone redistribution.

## What was done (Ryan's side, Claude Code, Nov 14 2026)

- `../scripts/prep_fab_quixel_mesh.py` (Blender 5.2): mesh stood up and its origin put on the floor centre, decimated to 4,000 tris (Megascans normal map kept for the detail), one material.
- Maps: `T_Fab_CinderRock_BC` 2048 (seller colour kept as `export/T_Fab_CinderRock_BC_orig.jpg`), `_N` and `_ORM` 1024 (Megascans ORM as packed; the normal was OpenGL by the height-map check and is flipped to DirectX).
- Look: Desaturated, cooled and scaled to the volcanic ground set's mean brightness (T_Fab_TundraVolcanic_BC, mean luma 0.17; scale 0.65) so it sits on Cinder's basalt.
- `export_for_unreal.py`, then `../scripts/ue_import_fab_multi.py` (TD_FAB_ONLY=CinderRock): `/Game/TriadDrift/Gameplay/Props/Fab/CinderRock/` (`SM_Fab_CinderRock`, `MI_Fab_CinderRock`, `T_Fab_CinderRock_*`), Nanite on, one UCX hull.
- Estimated cooked size about 4.9 MB (BC 2.7, N 1.3, ORM 0.7, mesh about 0.2).
- Sheets: `review/Fab_CinderRock_material_sheet.png` (ours), `review/Fab_CinderRock_seller_sheet.png` (the package's own renders).
- Suggested use: Cinder: rock clusters and outcrops round the outpost, the dig site and the lava fields; scale 0.5-2.0 and random yaw. Not placed anywhere; the environment builder scatters it.
