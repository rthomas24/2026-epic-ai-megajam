# Engine exhaust flipbook r1 (brief)

VFX r2 (Lead request after accepting VFX r1): a loopable engine-exhaust flame
flipbook for UE Niagara. Horizontal jet, bright core, soft falloff.
Stylised-realistic, original. Packed from a fal video generated on pure black.

## What this round delivers
- `T_VFX_EngineExhaust_8x8.png`: 2048×2048 RGBA, 8×8 SubUV (64 frames, 256 px cells).
  **Loopable.** Horizontal jet, origin on the left, streams to the right. Bright
  white-blue core, soft falloff through yellow-orange into a dimmer red-orange
  tail. Alpha derived from `max(R,G,B)`; RGB colour is preserved.
- `source/exhaust_s51515.mp4`: untouched 5 s, 960×960, 30 fps, 150-frame Wan 3.0 Prime clip.
- Pack settings and the prompt are in `SOURCE.md`.

## Notes for the effects builder
- SubUV: 8 columns, 8 rows. Loop. The last row has a short crossfade toward the
  first cells so the seam is soft (same method as Fire r1).
- The video model drew a residual thruster rim on the left; that rim was cropped
  out of the packed sheet (128 px of the 960 source, right-padded with black).
  The flame origin sits on the left edge of each cell so you can socket it to
  a ship nozzle. The untouched source clip still has the rim.
