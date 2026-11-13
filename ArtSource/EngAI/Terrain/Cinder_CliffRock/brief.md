# Cinder_CliffRock - terrain PBR tileable r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-027 (ranked request list, item #3, Terrain PBR tileables for ENG-34).
This is not a hero asset: no concept gate.

## Spec (from the Lead's reply)
- **Where it shows:** both planets' new mountains, everywhere on foot and from the air; this set is dark brown-black volcanic cliff rock face with layered strata, cracks and rusty mineral stains.
- **Budget / format:** 2K, BC + N + ORM + height, PNG; seamless.
- **Scale:** 4 m tile at 2K is the near scale.
- **Target:** `ArtSource/EngAI/Terrain/Cinder_CliffRock/`; the materials builder wires it into `M_TD_Terrain`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`r2`, ...).
- 2K textures at most. Normals are DirectX. ORM layout is R=AO, G=roughness, B=metallic.

## What this round delivers
- `T_Cinder_CliffRock_BC.png`, `T_Cinder_CliffRock_N.png`, `T_Cinder_CliffRock_ORM.png`, `T_Cinder_CliffRock_H.png` at this folder root (2048×2048 PNG).
- `source/`: the original PBRgen zip contents, untouched.
- `contact_sheet.png`: BC tiled 2×2, N, ORM, H, and a simple lit preview.
- An overview of all nine BC tiles lives at `ArtSource/EngAI/Terrain/contact_sheet_overview.png`.
