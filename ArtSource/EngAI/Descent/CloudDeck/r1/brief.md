# CloudDeck r1 (brief)

R-030 / R-035 item (a): a tileable 2K top-down soft cloud-deck sheet with
alpha, for the planetary descent cloud layer (ENG-38). Stylised-realistic,
original. Generated as a still (Nano Banana 2) and made seamless locally,
because image models do not emit a guaranteed wrap.

## What this round delivers
- `T_Descent_CloudDeck.png`: 2048×2048 RGBA, **tileable**. Soft white /
  pale-pearl cumulus with gaps. Alpha is derived from luminance so the black
  gaps become transparent; RGB colour is preserved.
- `source/deck_s71717.png`: untouched 2K Nano Banana 2 still (the pick).
- `source/deck_s72727.png`: unused warmer alternate (not packed).
- Pack settings and the prompt are in `SOURCE.md`. The contact sheet includes
  a 2×2 tiling preview.

## Notes for the effects builder
- Sample with wrap (U/V address mode Wrap). Tint per planet if wanted
  (Verdance teal, Cinder ash) — the sheet itself is a neutral deck.
- A 96 px cosine wrap-blend was applied so opposite edges match exactly
  (pixel-wrap MSE is 0). A faint soft blend can still be seen at the wrap if
  you look for it; it is much weaker than a hard seam. The 2×2 preview is on
  the contact sheet.
