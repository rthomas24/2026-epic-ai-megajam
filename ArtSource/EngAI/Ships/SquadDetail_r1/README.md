# SquadDetail_r1 - 1K seamless surface detail sets

For `docs/eng/SHIP-QUALITY.md` (texture resolution at 3-10 m walking distance) and R-030 #1.
Overlay on the squad ships' 2K sets with Unreal `DetailNormal` / `BlendAngleCorrectedNormals`.

| File | Content |
|---|---|
| `T_Rift_Detail_N.png` / `_RM.png` | curved aerospace panel seams, chamfered access hatches, fastener dots, brushed ceramic grain |
| `T_Lancer_Detail_N.png` / `_RM.png` | angular armour seams incl. 45 deg cuts, louvre vents, rivets, brushed titanium grain |
| `T_Bastion_Detail_N.png` / `_RM.png` | heavy bevelled plate grid (running bond), recessed bolts, painted-armour grit |
| `contact_sheet.png` | 2x2 tiles of each set, sphere + flat at 3 m with a 1 m tile, Lancer/Bastion textures_r2 before/after |
| `metrics.json` | mean colour and wrap-edge metrics |
| `detail.py`, `rdet.py` | generator and preview render |

- All 1024 x 1024, 8-bit, **linear (sRGB off)**.
- **N: DirectX (Y-)**, mean RGB ~ (127.5, 127.5, 240-248), so it does not tilt world shading.
- **RM: R = cavity AO (255 = none, seams darker), G = roughness variation centred on 128
  (add `(G - 0.5) * k` to roughness), B unused (0).**
- **Seamless by construction**: every feature is drawn into a 3x3 torus canvas and the centre tile
  cropped, noise is FFT-periodic, normals use wrapped finite differences. Tiling the image therefore
  reproduces the canvas exactly (tile-vs-canvas max diff 0). The wrap edge (col 0 vs col 1023,
  row 0 vs row 1023) has the same step size as any interior neighbour pair:

| Set | N wrap step | N interior step | RM wrap | RM interior |
|---|---|---|---|---|
| Rift | 9.48 | 9.68 | 1.58 | 1.59 |
| Lancer | 11.58 | 12.13 | 1.32 | 1.43 |
| Bastion | 15.01 | 15.05 | 1.53 | 1.56 |

(A literal `|col0 - col1023| == 0` would mean a duplicated row, not a seamless tile; the right test
is that the wrap step matches interior steps, which it does.)

Original procedural work (Grok Bot, Blender 4.5.3 + NumPy/OpenCV, 2026-11-13). No third-party content.
