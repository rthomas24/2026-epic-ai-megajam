# Lancer - heatmask_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (queue addition of 2026-11-13 13:47 EST)
and R-035 / ENG-38 (planetary descent): one emissive heat-glow mask per player ship, for the
nose, leading edges and belly, matching the Rodin UVs, delivered with the ship.

This drop is a derived texture only. It does not replace `mesh_r1`. Builders import the mask
alongside `SM_Lancer` and drive a descent-entry emissive with it.

## Spec
- **Where it shows:** atmospheric entry (ENG-38): the ship pierces the atmosphere with a
  visible heat glow (plasma sheath, leading-edge and belly heating).
- **Format:** one 2K (2048×2048) linear grayscale PNG, `T_Lancer_HeatMask.png`.
- **UVs:** the existing `UVMap` on `mesh_r1` (`SM_Lancer.fbx` / `.glb` in PR #22). No unwrap
  was changed.
- **Axes / units:** same as the mesh — centimetres, **+X nose**, **+Z up**, pivot at the
  bounding-box centre. Delivery mesh is 524.0 × 269.5 × 149.9 cm (L×W×H), 50,000 tris.
- **Target:** a mask texture on `Gameplay/Characters/Squad/DA_Lancer_Visuals` (sRGB off),
  multiplied into an orange-to-white-hot emissive during descent. Existing `T_Lancer_E.png`
  (engine and accent strips) is unchanged.

## Rules that bind this asset
- Original work derived from the approved `mesh_r1` geometry only; no logos, text, or
  likeness. The mask is a geometric bake, not a new generated artwork.
- Never overwrite a file already in git; a new round goes in a new folder (`heatmask_r2`, …).
- 2K textures at most. Import this map as a mask / data texture, sRGB off.

## What this round delivers
- `T_Lancer_HeatMask.png`: 2048×2048 8-bit linear grayscale (no sRGB / no gAMA chunk).
- `preview.png`: EEVEE Next still, 3/4 front and below, with the mask driving an orange
  heat glow on the `mesh_r1` hull.
- `SOURCE.md`: method, settings, licence and import notes.
