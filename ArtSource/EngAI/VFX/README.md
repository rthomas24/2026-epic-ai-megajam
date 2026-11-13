# VFX r1 — fire, smoke, explosion flipbooks, shockwave ring, glow, spark

R-027 item #7 (jumped the queue at the Lead's request: the effects builder needs
fire / smoke / explosion flipbooks and a shockwave ring now). Stylised-realistic,
original, for UE Niagara.

| Folder | Final texture | Notes |
|---|---|---|
| `Fire/r1/` | `T_VFX_Fire_8x8.png` (2048, RGBA, SubUV 8x8 loop) | Packed from a 5s Wan 3.0 Prime clip |
| `Smoke/r1/` | `T_VFX_Smoke_8x8.png` (2048, RGBA, SubUV 8x8 loop) | Alpha from luminance |
| `Explosion/r1/` | `T_VFX_Explosion_8x8.png` (2048, RGBA, SubUV 8x8 one-shot) | Starts at the first flash, fades out |
| `Shockwave/r1/` | `T_VFX_ShockwaveRing.png` (2048, RGBA still) | Also includes a bonus `T_VFX_Shockwave_8x8.png` of the expansion |
| `Glow/r1/` | `T_VFX_Glow.png` (2048, RGBA) | Procedural soft radial glow, tintable |
| `Spark/r1/` | `T_VFX_Spark.png` (1024, RGBA) | Procedural point + streak, slightly warm |

Overview contact: `contact_sheet_overview.jpg`.

All flipbooks and the ring are on a transparent (or derived-alpha) background, packed
from video generated on pure black. Niagara: SubUV 8 columns × 8 rows, use alpha or
additive as the material requires; RGB was kept so fire/explosion colour survives.
Power-of-two, 2048 max (spark is 1024). See each folder's `SOURCE.md` for prompts,
seeds, pack settings and licence.
