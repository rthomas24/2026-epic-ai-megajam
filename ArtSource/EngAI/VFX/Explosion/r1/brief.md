# Explosion flipbook r1 (brief)

R-027 item #7: a one-shot explosion flipbook for UE Niagara (ENG-37).
Stylised-realistic, original. Packed from a fal video generated on pure black.

## What this round delivers
- `T_VFX_Explosion_8x8.png`: 2048×2048 RGBA, 8×8 SubUV (64 frames, 256 px cells).
  **One-shot**, not a loop: starts at the first visible flash, expands, then fades
  toward black. Alpha derived from `max(R,G,B)`; RGB colour is preserved.
- `source/explosion_s30303.mp4`: untouched 5 s, 960×960, 30 fps, 150-frame Wan 3.0 Prime clip.
- Pack settings and the prompt are in `SOURCE.md`.

## Notes for the effects builder
- SubUV: 8 columns, 8 rows. Play once (do not loop). A couple of near-black lead-in
  cells are kept so the ignition is in-sheet; the last rows are the fade-out.
