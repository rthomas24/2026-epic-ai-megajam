# Debris-chunk atlas r1 (brief)

VFX r2: a 4×4 debris-chunk atlas for UE Niagara (explosion debris, hull
fragments). Dark metal with glowing molten edges, isolated on black / alpha.
Sixteen distinct chunks. Packed from a fal image generated on pure black.

## What this round delivers
- `T_VFX_DebrisChunks_4x4.png`: 2048×2048 RGBA, 4×4 atlas (16 cells, 512 px).
  Each chunk is recentred in its cell with a small margin. Dark metal is
  nearly opaque; the molten-edge glow is in both RGB and alpha. Cell seams
  are fully transparent so sprites do not leak a grid.
- `source/debris_grid_s60606.png`: untouched 2048×2048 Nano Banana 2 image
  (the pack source). `source/debris_scatter_s61616.png` is an unused scatter
  alternate from the same session (not packed).

## Notes for the effects builder
- SubUV: 4 columns, 4 rows (or spawn individual cells). Pivot at each cell
  centre. Tint the glow in the material if you want a colder break edge.
