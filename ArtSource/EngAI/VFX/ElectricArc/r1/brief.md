# Electric arc sheet (EMP) r1 (brief)

VFX r2: an electric arc / EMP flipbook for UE Niagara. Procedural, not a fal
generation, so it cost nothing and the sheet is a perfect loop (all motion is
integer-harmonic).

## What this round delivers
- `T_VFX_ElectricArc_8x8.png`: 2048×2048 RGBA, 8×8 SubUV (64 frames, 256 px cells).
  **Loopable.** Branching white-cyan lightning radiating from the centre (EMP)
  plus crossing chords, with soft bloom. Alpha from the summed intensity.

## Notes for the effects builder
- SubUV: 8 columns, 8 rows. Loop. Tint in the material if you need a colder or
  warmer EMP. No source media folder (procedural).
