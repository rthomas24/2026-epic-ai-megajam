# HeatStreak r1 (brief)

R-030 / R-035 item (b): a heat streak / ember sheet for UE Niagara during
atmospheric entry (ENG-38). Stylised-realistic, original. Built procedurally
(like the plasma-bolt and spark sheets) because a still streak with a clean
alpha and no baked-in hull is more reliable that way.

## What this round delivers
- `T_Descent_HeatStreak.png`: 2048×1024 RGBA still. White-hot core falling
  off through yellow into orange, tip on the **right**, trailing embers and
  micro-streaks. Tintable.
- Pack / construction is in `SOURCE.md`. No source media folder is required.

## Notes for the effects builder
- Use as a particle sprite or ribbon. The tip sits at about 0.92 of the
  width; leave a small margin past the tip.
- Additive or alpha-composite both work; RGB goes to black in the empty
  areas, so additive will not lift the background.
