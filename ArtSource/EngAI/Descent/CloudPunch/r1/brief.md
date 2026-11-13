# CloudPunch flipbook r1 (brief)

R-030 / R-035 item (a): an 8×8 "cloud punch" flipbook of flying through a
cloud wall, camera-facing, for UE Niagara during planetary descent (ENG-38).
Stylised-realistic, original. Packed from a fal video generated on pure
black, because image models struggle with consistent SubUV grids.

## What this round delivers
- `T_Descent_CloudPunch_8x8.png`: 2048×2048 RGBA, 8×8 SubUV (64 frames,
  256 px cells). **One-shot** (does not loop). Alpha derived from luminance
  so the black video background becomes transparent; RGB is preserved.
- `source/punch_s90909.mp4`: untouched 5 s, 960×960, 30 fps, 150-frame Wan
  3.0 Prime clip.
- Pack settings and the prompt are in `SOURCE.md`.

## Notes for the effects builder
- SubUV: 8 columns, 8 rows. Play once, starting on the dense wall (cell 0)
  and ending on a nearly clear frame. Do not loop.
- The clip already opens on a full cloud bank (no lead-in fade-up), peaks
  as the camera is inside the wall, and fades to almost black by the last
  cell.
