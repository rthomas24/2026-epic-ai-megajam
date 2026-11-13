# Smoke flipbook r1 (brief)

R-027 item #7: a looping smoke flipbook with alpha for UE Niagara (ENG-37).
Stylised-realistic, original. Packed from a fal video generated on pure black.

## What this round delivers
- `T_VFX_Smoke_8x8.png`: 2048×2048 RGBA, 8×8 SubUV (64 frames, 256 px cells). Intended
  as a **loop**. Alpha derived from luminance so soft wisps survive; RGB (grey-white
  smoke colour) is preserved.
- `source/smoke_s20202.mp4`: untouched 5 s, 960×960, 30 fps, 150-frame Wan 3.0 Prime clip.
- Pack settings and the prompt are in `SOURCE.md`.

## Notes for the effects builder
- SubUV: 8 columns, 8 rows. Loop the sheet. Use the alpha channel; the smoke is
  light-on-black in RGB.
- An 8-frame crossfade was applied at the pack seam. The source plume slowly evolves,
  so the loop is usable but not a perfect cycle.
