# Descent r1 — cloud and entry-plasma textures (ENG-38 / R-035)

Planetary descent assets for the Lead and the effects builder: a tileable cloud
deck, wisp and cloud-punch flipbooks, a looping plasma-sheath flipbook, a heat
streak / ember sheet and a shock-cone ring. Stylised-realistic, original, for
UE Niagara. Folder `ArtSource/EngAI/Descent/` as specified in R-030 (13:47 EST).

| Folder | Final texture | Notes |
|---|---|---|
| `CloudDeck/r1/` | `T_Descent_CloudDeck.png` (2048, RGBA still) | Tileable top-down soft cloud deck with alpha. 2×2 check on the contact sheet. |
| `CloudWisp/r1/` | `T_Descent_CloudWisp_8x8.png` (2048, RGBA, SubUV 8×8 loop) | Soft vapour wisps. Packed from a 5 s Wan 3.0 Prime clip. |
| `CloudPunch/r1/` | `T_Descent_CloudPunch_8x8.png` (2048, RGBA, SubUV 8×8 one-shot) | Flying through a cloud wall, camera-facing. Fades out; do not loop. |
| `PlasmaSheath/r1/` | `T_Descent_PlasmaSheath_8x8.png` (2048, RGBA, SubUV 8×8 loop) | Bow shock, white-hot core to orange, tip up, no hull. |
| `HeatStreak/r1/` | `T_Descent_HeatStreak.png` (2048×1024, RGBA still) | Procedural heat streak and trailing embers, tip on the right. |
| `ShockCone/r1/` | `T_Descent_ShockCone.png` (2048, RGBA still) | Procedural front-on Mach / bow-shock ring. |

Overview contact: `contact_sheet_overview.jpg`.

Flipbooks are on a derived-alpha background, packed from video generated on
pure black (wisps and punch: alpha from luminance; sheath: alpha from
`max(R,G,B)`). Niagara: SubUV 8 columns × 8 rows; use alpha or additive as
the material requires; RGB is kept so colour survives. Power-of-two, 2048 max
(the heat streak is 2048×1024). See each folder's `SOURCE.md` for prompts,
seeds, pack settings and licence.

This batch is items (a) and (b) of the R-030 queue addition. Heat-glow masks
per ship, deployable airbrake / heat-shield parts, and per-planet globe maps
are not in this PR.
