# Rift - mesh_r2 (brief)

Requested in `MESSAGEBOARD.md` **R-030** (16:20 EST, priority #1) and `docs/eng/SHIP-QUALITY.md`
("Result of the Rift pilot ... the route is now a hybrid", step 2, ship polish builder), with the
Lead / Issue #63 note (17:07 EST): Rift showed "a giant hole in the back" from the chase camera.

## Spec
- Keep the approved Rodin silhouette, scale, pivot, sockets and UVs on the fuselage, canopy,
  crescent wings and nose guns (welded and cleaned, same UVs, so `heatmask_r1` still matches).
- Replace the ragged portal ring with a designed hard-surface hoop that stays OPEN (centre for
  Engineering's animated portal membrane at `SOCKET_Portal`).
- Replace the rear engine block with its hollow centre nozzle: armoured back plate, two outer
  nozzles (`Engine_0/1`), a closed central turbine nacelle (`Engine`), glowing interiors.
- One material `M_Rift`, one 2048 set `T_Rift_{BC,N,ORM,E}.png`; new parts packed into the UV
  space freed by the deleted portal disc, zero overlap with kept islands.
- 40-60k tris; FBX (UnitScaleFactor 100) + GLB; same size 500.0 x 417.4 x 226.0 cm.
- Clean the kept textures for close range (calmer BC mottling, calmer roughness, softer wing
  normal lumps, razor-clean emissive).

## Rules
Original work derived from our own approved `mesh_r1` (Hyper3D Rodin output, see
`mesh_r1/SOURCE.md` for licence). No logos, text or likeness. Nothing already in git was
overwritten; this is a new folder.
