# CloudWisp flipbook r1 (brief)

R-030 / R-035 item (a): a looping 8×8 wisp / vapour flipbook for UE Niagara
during planetary descent (ENG-38). Stylised-realistic, original. Packed from
a fal video generated on pure black, because image models struggle with
consistent SubUV grids.

## What this round delivers
- `T_Descent_CloudWisp_8x8.png`: 2048×2048 RGBA, 8×8 SubUV (64 frames, 256 px
  cells). Intended as a **loop**. Alpha derived from luminance so the black
  video background becomes transparent; RGB is preserved.
- `source/wisp_s80808.mp4`: untouched 5 s, 960×960, 30 fps, 150-frame Wan 3.0
  Prime clip.
- Pack settings and the prompt are in `SOURCE.md`.

## Notes for the effects builder
- SubUV: 8 columns, 8 rows, playback ~16–20 fps is a natural speed (the 64
  frames cover ~4.0 s of source). Loop the sheet.
- An 8-frame crossfade was applied at the end of the pack so the loop seam
  is softer; it is not a mathematically perfect cycle (the source model has
  no loop flag).
