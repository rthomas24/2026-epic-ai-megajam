# SM_Fab_CrateStack (Fab "Free Crates")

## What it is

A stack of five stylized, cartoon-proportioned equipment crates (four on the ground, one on top), fused into **one mesh**.
It is not wizard hats: the listing description (copied from another of the seller's packs) talks about wizard hats; the
title, tags, thumbnail and mesh are crates. The supplied `fbx/plane.fbx` is only a 1.24 m flat plane (32 tris) and was not
used; the mesh comes from `additional-files/crates_extracted/crates.blend`. Painted pink, orange and red with garbled
AI lettering on the panels.

| | Source | Game-ready |
|---|---|---|
| Triangles | 14,942 (closed, one island) | 14,942 (under the 15k cap, not decimated); Unreal's Nanite fallback 8,412 |
| Size | 165.9 x 162.5 x 145.1 cm (metres, unit scale 1) | same; each crate about 80 cm |
| Textures | packed 4096: base colour, metallic-roughness (G rough, B metal), normal (OpenGL) | 2048 BC, N (green flipped to DirectX), ORM (AO = 1) |
| Collision | none | one convex hull over the whole stack |

## Source

- Fab listing **"Free Crates"**, uid `ef859200-bcc7-4f44-9718-9059407313c1`,
  https://www.fab.com/listings/ef859200-bcc7-4f44-9718-9059407313c1, seller **LapaModels**, published 2026-08-13.
- Fab metadata: `isAiGenerated: true` ("ATTENTION: AI-GENERATED ASSETS", AI-assisted then cleaned by the seller).
- Supplied by Nicholas Tosti (issue #175), archive `Free-Crates-Nicholas-20261114.zip`,
  SHA-256 `556d0372931c22fada665b5b72f3a2069dc09a8c41ce1e67d6aabdd50303d80d`.

## Licence

**Licence: not yet evidenced; acquired by Nicholas on Fab; confirmation requested on issue #175.** The Fab metadata in
the archive has no licence-tier field and the listing page's licence terms field is empty (Nick's side, #175). Until the
licence is recorded in CREDITS.md these assets are **for evaluation only: do not place them in any map and do not
reference them from any data asset, class or code.** Unreferenced, `/Game/TriadDrift/Gameplay/Props/Fab/` is not cooked.

## How it was made (Ryan's side, Claude Code, Nov 14 2026)

1. Archive downloaded with `gh release download asset-sources-nicholas-20261114` (Ryan's approval) to
   `Builds/intake/FreeCrates/` (git-ignored, not committed); ZIP SHA-256 matched the issue; the archive's own
   `SHA256SUMS.txt` matched every file. Treated as untrusted: `.blend` opened only with `--disable-autoexec`.
2. `scripts/prep_fab.py` (Blender 5.2.2 headless): one mesh `SM_Fab_CrateStack`, transforms applied, origin on the floor centre,
   front kept on -Y; textures re-written at 2048 as `T_Fab_CrateStack_BC/N/ORM` (N DirectX; ORM = AO, roughness, metallic).
   Report: `prep_report.json`.
3. `export_for_unreal.py` (blender-unreal-export skill) wrote `export/SM_Fab_CrateStack.fbx` with one auto convex hull
   `UCX_SM_Fab_CrateStack_00` and the manifest. (The skill script's cleanup loop raised on a stale `None` in the view layer after a
   successful export; run with a local copy guarded by `vl.update()` and `if o is not None`. Reported to the Lead.)
4. `scripts/ue_import_fab.py` (UnrealEditor-Cmd, -nullrhi, via `Tools/td-python.ps1`): Interchange with
   `PL_Ship_StaticMesh`, Nanite on, MI on `M_TD_PropTextured`. Report: `../ue_import_report.json`.
5. Checks: `review/asset_report.json`, `review/assembly_check.json`, sheets `review/Fab_CrateStack_material_sheet.png`, `_clay_sheet.png`.
