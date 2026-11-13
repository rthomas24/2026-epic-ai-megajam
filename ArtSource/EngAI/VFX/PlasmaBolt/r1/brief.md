# Plasma / energy bolt streak r1 (brief)

VFX r2: a plasma / energy bolt streak for UE Niagara (projectiles, beams).
Procedural, not a fal generation, so it cost nothing and the falloff is smooth
and tintable.

## What this round delivers
- `T_VFX_PlasmaBolt.png`: **2048×512 RGBA** still. Horizontal streak, origin /
  tail on the left, pointed bright tip on the right. White core, cyan sheath,
  soft blue-violet halo, faint helical tendrils and longitudinal striations.
  Alpha carries the same falloff. Tint in the material for faction colours.

## Notes for the effects builder
- Wide (4:1) beam texture, not a square. Map it as a beam / ribbon / projectile
  sprite; rotate and tint as needed. No source media folder (procedural).
