# SM_Fab_OilBarrel (Fab "Industrial Oil Barrel - Game Ready PBR")

## What it is

A realistic, dented steel oil drum: red top and bottom bands with heavy rust and black drips, a yellow middle band with the
seller's fictional red gear-and-spiral logo (the listing says it is original and not copyrighted), silver lid.

| | Source | Game-ready |
|---|---|---|
| Triangles | 5,066 (2,592 verts; quads and n-gons, triangulated on export) | 5,066 (Nanite fallback 1,606) |
| Size | 58.7 x 58.8 x 81.3 cm (FBX in cm) | same |
| Textures | 4096 16-bit PNG base colour, normal, roughness, metallic, height; 2048 AO | 2048 BC, N (DirectX already: green kept, checked against the height map), ORM (real AO) |
| Collision | none | one convex hull |

## Source

- Fab listing **"Industrial Oil Barrel - Game Ready PBR"**, uid `54701df2-644c-4691-a3d2-e6c2acde4788`,
  https://www.fab.com/listings/54701df2-644c-4691-a3d2-e6c2acde4788, seller **dplHK**, published 2026-06-09.
- Fab metadata: `isAiGenerated: true` (the description also says "100% hand-crafted"; the flag is what Fab records). The
  description mentions a "Standard License", but that is the seller's text, not a licence record for our acquisition.
- Supplied by Nicholas Tosti (issue #177), archive `OilBarrel-Nicholas-20261114.zip`,
  SHA-256 `51ae799fede42a6f666495a89aba78e4d4e1cde2b33eaba435abec2422df5861`.

## Licence

**Licence: not yet evidenced; acquired by Nicholas on Fab; confirmation requested on issue #175.** The Fab metadata in
the archive has no licence-tier field and the listing page's licence terms field is empty (Nick's side, #175). Until the
licence is recorded in CREDITS.md these assets are **for evaluation only: do not place them in any map and do not
reference them from any data asset, class or code.** Unreferenced, `/Game/TriadDrift/Gameplay/Props/Fab/` is not cooked.

## How it was made (Ryan's side, Claude Code, Nov 14 2026)

1. Archive downloaded with `gh release download asset-sources-nicholas-20261114` (Ryan's approval) to
   `Builds/intake/OilBarrel/` (git-ignored, not committed); ZIP SHA-256 matched the issue; the archive's own
   `SHA256SUMS.txt` matched every file. Treated as untrusted: `.blend` opened only with `--disable-autoexec`.
2. `scripts/prep_fab.py` (Blender 5.2.2 headless): one mesh `SM_Fab_OilBarrel`, transforms applied, origin on the floor centre,
   front kept on -Y; textures re-written at 2048 as `T_Fab_OilBarrel_BC/N/ORM` (N DirectX; ORM = AO, roughness, metallic).
   Report: `prep_report.json`.
3. `export_for_unreal.py` (blender-unreal-export skill) wrote `export/SM_Fab_OilBarrel.fbx` with one auto convex hull
   `UCX_SM_Fab_OilBarrel_00` and the manifest. (The skill script's cleanup loop raised on a stale `None` in the view layer after a
   successful export; run with a local copy guarded by `vl.update()` and `if o is not None`. Reported to the Lead.)
4. `scripts/ue_import_fab.py` (UnrealEditor-Cmd, -nullrhi, via `Tools/td-python.ps1`): Interchange with
   `PL_Ship_StaticMesh`, Nanite on, MI on `M_TD_PropTextured`. Report: `../ue_import_report.json`.
5. Checks: `review/asset_report.json`, `review/assembly_check.json`, sheets `review/Fab_OilBarrel_material_sheet.png`, `_clay_sheet.png`.
