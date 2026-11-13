# ShockCone r1 (brief)

R-030 / R-035 item (b): a shock-cone ring texture for UE Niagara during
atmospheric entry (ENG-38). Stylised-realistic, original. Built procedurally
(like the glow and the shield-hex ring) so the ring stays perfectly circular
and the background stays clean black.

## What this round delivers
- `T_Descent_ShockCone.png`: 2048×2048 RGBA still. A front-on Mach /
  bow-shock ring, pale orange-white, with a soft halo and a very faint
  inner wall. Tintable.
- Construction is in `SOURCE.md`. No source media folder is required.

## Notes for the effects builder
- The ring is centred, axis facing the viewer (as if looking along the
  ship's velocity). Place ahead of the nose or around the plasma sheath.
- Additive or alpha-composite both work; RGB goes to black outside the
  ring, so additive will not lift the background.
