# Shield hex-ripple ring r1 (brief)

VFX r2: a shield hex-ripple ring for UE Niagara. Stylised energy shield,
original, procedural (no fal spend). The official deliverable is a **single**
ring texture. A bonus 8×8 flipbook of the same ripple expanding is included
because it is cheap and useful as a one-shot SubUV.

## What this round delivers
- `T_VFX_ShieldHexRipple.png`: **2048×2048 RGBA still** — a cyan-teal hexagonal
  energy ring at mid-radius, sitting on a hex tessellation that lights up along
  the ripple, plus a fainter inner hex ring. Transparent background. This is
  the pick for a single-texture ring.
- `T_VFX_ShieldHexRipple_8x8.png`: bonus 2048×2048 RGBA 8×8 one-shot of the
  hex ripple expanding from the centre and fading (not required by the brief;
  provided as a convenience).

## Notes for the effects builder
- The still is the pick for a static or pulsing shield ring (tint in the material).
- The 8×8 is a one-shot: SubUV 8×8, play once (do not loop). Last rows are the fade-out.
