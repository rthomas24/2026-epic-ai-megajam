# SM_Fab_PipeBeam (Fab "Sci-Fi Kit Free 08 Tubes Cables", SciFi_Prop_PipeBeam)

A 9 m square truss with diagonal bracing carrying bundled ribbed cables. Dressing only: nothing the player walks on.

## Source and licence

- "Sci-Fi Kit Free 08 Tubes Cables" by **REX Hsu**, https://www.fab.com/listings/3afde5db-b0ab-49b2-852f-a59ee454bb73
  (issue #180). Supplied by Nicholas Tosti (release `asset-sources-nicholas-20261114`, archive SHA-256
  `a47334878cf1e0a0c4b083172cc33861e92d116576fddc862c4f6e241f56f86d`, original kept in the git-ignored `Builds/intake2/PipeBeam/`).
- **Licence: Fab Standard License** (https://www.fab.com/eula); the listing offers Personal and Professional tiers with the
  same rights; the tier Nicholas acquired is not recorded. **Not AI-generated** (seller NoAI label). No standalone redistribution.
- Not used: `SKY_002.exr` (no stated provenance), the .max / OBJ copies, and the texture sets B-G the .blend references
  but the package does not contain.
- Lead's decision (TAKE) on #180; Null recolour requested by the Lead under Ryan's delegation.

## What was done (Ryan's side, Claude Code, Nov 14 2026)

| | Source | Game-ready |
|---|---|---|
| Triangles | 11,280 in four meshes | 11,280 in one mesh, two slots (Truss, Cable); extra UV channels dropped |
| Size | 900 x 110 x 110 cm | same; origin on the floor centre; long axis along Unreal Y |
| Textures | 4096 PNG (truss: BC, emissive, height, metallic, normal, roughness; cables: BC, metallic, normal, roughness) | 2048 `T_Fab_PipeBeam_{Truss,Cable}_BC/N/ORM`, `T_Fab_PipeBeam_Truss_E`; normal maps already DirectX (checked against the height map) |
| Look | orange paint, orange and blue light strips | Null black armour keeping the paint wear, red running lights (emissive strength 6), grey service box darkened, cables darkened |
| Collision | none | one convex hull `UCX_SM_Fab_PipeBeam_00` (a blocking box) |

The seller's colours are kept as `export/T_Fab_PipeBeam_Truss_BC_orig.png`, `T_Fab_PipeBeam_Cable_BC_orig.png`,
`T_Fab_PipeBeam_Truss_E_orig.png`. Scripts: `../scripts/prep_pipebeam.py` (Blender 5.2, opened with --disable-autoexec),
`export_for_unreal.py`, `../scripts/ue_import_fab_multi.py` (TD_FAB_ONLY=PipeBeam). Sheets: `review/Fab_PipeBeam_before_after.png`.

Unreal: `/Game/TriadDrift/Gameplay/Props/Fab/PipeBeam/` (`SM_Fab_PipeBeam`, `MI_Fab_PipeBeam_Truss`, `MI_Fab_PipeBeam_Cable`,
`T_Fab_PipeBeam_*`). Nanite on. Not placed anywhere; the environment builder places it.
