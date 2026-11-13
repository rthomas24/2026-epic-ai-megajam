# Cinder_Basalt - terrain PBR tileable r2 (brief)

Redo requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (continuing R-027 request #3 / PR #16).
The Lead accepted 8 of 9 terrain sets, but r1 `Cinder_Basalt` "reads as man-made hexagon paving and its pattern repeats visibly": please redo it as natural columnar / fractured basalt with irregular cell sizes (`Cinder_Basalt/r2/`).
This is not a hero asset: no concept gate. r1 is left untouched; this round lives only in `r2/`.

## Spec (from the Lead's reply)
- **Where it shows:** both planets' new mountains, everywhere on foot and from the air.
- **What changed vs r1:** r1 looked like manufactured, uniform hexagonal pavers. r2 is natural volcanic columnar basalt in the Giant's Causeway / Icelandic manner: organic, irregular fractured 4-to-7-sided stone columns with widely varying widths and uneven stepped heights; rough weathered charcoal-black basalt faces; chipped irregular edges and fractured surfaces; dark volcanic ash and grit settled in deep irregular crevices. No uniform grid, no repeating symmetrical hexagons, no artificial pavers.
- **Budget / format:** 2K, BC + N + ORM + height, PNG; seamless.
- **Scale:** 4 m tile at 2K is the near scale.
- **Target:** `ArtSource/EngAI/Terrain/Cinder_Basalt/r2/`; the materials builder wires it into `M_TD_Terrain`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`r2`, ...).
- 2K textures at most. Normals are DirectX. ORM layout is R=AO, G=roughness, B=metallic.

## What this round delivers
- `T_Cinder_Basalt_r2_BC.png`, `T_Cinder_Basalt_r2_N.png`, `T_Cinder_Basalt_r2_ORM.png`, `T_Cinder_Basalt_r2_H.png` at this folder root (2048×2048 PNG).
- `source/`: the original PBRgen zip contents, untouched (PNGs, `README.txt`, `import_pbrgen.py`, `material-preview.png`). No `.zip` is committed.
- `contact_sheet.png`: r2 BC tiled 2×2, N, ORM, H, the PBRgen 3D preview, and a side-by-side comparison of r1 vs r2 base colour (both tiled 2×2) so the Lead can see the fix from uniform hex pavers to natural organic columnar basalt.
