# Bomber - heatmask_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 / R-035 (planetary descent): one emissive
heat-glow mask per ship, for the nose, leading edges and belly, matching the Rodin UVs,
delivered with the ship.

This drop is a derived texture only. It does not replace `mesh_r1`. Builders import the mask
alongside `SM_NullBomber` and drive a descent-entry emissive with it.

## Spec
- **Where it shows:** atmospheric entry (ENG-38): the ship pierces the atmosphere with a
  visible heat glow (plasma sheath, leading-edge and belly heating).
- **Format:** one 2K (2048×2048) linear grayscale PNG, `T_NullBomber_HeatMask.png`.
- **UVs:** the existing `UVMap` on `mesh_r1` (`SM_NullBomber.fbx` / `.glb` in this PR). No unwrap
  was changed.
- **Axes / units:** same as the mesh — centimetres, **+X nose**, **+Z up**, pivot at the
  bounding-box centre. Delivery mesh is 1180.0 × 1034.6 × 215.6 cm (L×W×H), 8,478 tris.
- **Target:** a mask texture on `Gameplay/Characters/NullFleet/DA_Enemy_Bomber_Visuals` (sRGB off),
  multiplied into an orange-to-white-hot emissive during descent. Existing `T_NullBomber_E.png`
  (engines and accent strips) is unchanged.

## Rules that bind this asset
- Original work derived from the approved `mesh_r1` geometry only; no logos, text, or
  likeness. The mask is a geometric bake, not a new generated artwork.
- Never overwrite a file already in git; a new round goes in a new folder (`heatmask_r2`, …).
- 2K textures at most. Import this map as a mask / data texture, sRGB off.

## What this round delivers
- `T_NullBomber_HeatMask.png`: 2048×2048 8-bit linear grayscale (no sRGB / no gAMA chunk).
- `preview.png`: EEVEE Next still, 3/4 front and below, with the mask driving an orange
  heat glow on the `mesh_r1` hull.
- `SOURCE.md`: method, settings, licence and import notes.
