# Dominion Mothership hull - mesh_r2 (brief)

Issue **#41**, R-042 (18:55 EST reply). This is the second 3D delivery of the finale capital ship **`SM_DominionMothership`**. It extends approved mesh_r1 to close the visual gap to the **v3** turnaround Nicholas approved as-is (`../concept_r1/source/sheet-DominionMothership-v3_PICK.png`, thumbnail in `refs/`), while keeping the gameplay contract identical.

## What to import
| File | What it is |
|---|---|
| `SM_DominionMothership.fbx` | **Use this one in Unreal.** Render mesh `SM_DominionMothership` (1 material, 1 UV set, custom / weighted normals, triangulated), 121 `UCX_*` convex hulls (the original r1 59 are `UCX_SM_DominionMothership_00..58` with identical bounds, plus 62 extras for the new superstructure, spires and flank terraces) and 12 `SOCKET_*` empties parented to the mesh. `UnitScaleFactor` = 100, so import at uniform scale **1.0**. |
| `SM_DominionMothership.glb` | The same mesh and the same 12 socket nodes, in glTF metres with +Y up. It has no UCX and embeds an OpenGL-green copy of the normal map, as glTF requires. |
| `SM_DominionMothership.blend` | Editable Blender 4.5.3 source. The visual mesh keeps live, unapplied `Bevel` (weight-limited, 2 segments) and `WeightedNormal` modifiers plus `smooth_by_angle(30°)`. Collections: `Visual`, `Collision` (`UCX_*`), `Sockets`. 2K preview textures are packed. |
| `T_DominionMothership_{BC,N,ORM,E}.png` | One unified 2048 PBR set (downsampled with Lanczos from the 4096 masters in `source/textures_4k/`). BC is sRGB. N is **DirectX**. ORM is R = AO, G = Roughness, B = Metallic. E is sRGB emissive. |
| `T_DominionMothership_Detail_{N,ORM}.png` | A 2048 tileable detail layer (panel seams, rivets, vents) so the trench holds up at 30-80 m. Detail_ORM is R = AO/cavity, G = roughness variation, B = 0. The suggested tile is about 8 m. |
| `contact_sheet.png` | v3 vs r2 beauty, side/front/rear orthos, trench, bay, **180 cm pilot**, generator plinth, close-ups of the superstructure and flank armour, a clay / edge-highlight check, and top/side blueprints with sockets and UCX. |
| `manifest.json` | Sockets, UCX list with bounds, tri count, clearances, texture QA, mesh_report, file hashes. |

## What changed vs r1 (visual only)
- A stacked, stepped superstructure ("city") along both sides of the spine and a taller aft citadel, with multiple command / sensor spires. The main centreline mast now reaches Z 19800. The 50 m trench, its walls at y = ±2500, Z 5000..8000, and the forward entry stay completely clear.
- Vivid maroon / crimson hull bands and original Dominion trident emblem plates (three per flank, plus two smaller plates on the aft-spine stacks).
- Denser lit window rows and warm amber strip lighting on flank tiers, superstructure setbacks and spires.
- Heavier layered flank armour: vertical ribs, terraced bevelled bulkheads and extra sponsons, so the side and 3/4 views no longer read as a long flat box.
- Live Bevel + WeightedNormal + smooth-by-angle 30° in the `.blend` (evaluated, triangulated in the FBX) so armour edges catch light.

## Contract (Unreal cm, +X bow, +Y starboard, +Z up, origin = hull centre) — unchanged
- **Hull core:** 60000 × 16000 × 10000 (X ±30000, Y ±8000, Z ±5000). New flank ribs and terraces stand a few metres off the 80 m skin (same idea as r1's shingles and turret sponsons).
- **Bridge / citadel:** the r1 base block is unchanged (7500 × 4000 × 6000, centred around (-19500, 0, 8000)). The forward brow still overhangs the trench end only at Z ≥ 8700. The centreline mast now reaches Z 19800.
- **Flyable dorsal trench:** X -18000..+24000. Floor flat at Z 5000. Walls |Y| 2500..3400, Z 5000..8000. Measured clear: **0 tris in |Y| < 2350, Z 5000..8500**. Forward entry (X > 24000, |Y| < 2490) is open.
- **Reactor** (16500, 0, 6500): nearest render triangle 1507.7 cm, nearest UCX 1500 cm (≥ 1400 required).
- **Bay** (30000, 0, -2000), +X out: mouth 5000 × 3500 (Y ±2500, Z -3750..-250), 6000 cm glide pocket. **0 tris and 0 UCX** in the launch channel and glide pocket.
- **Generators** at (±27500, ±27500, 0): outriggers, plinths (r 2400, top at Z -50), dome keep-clear and guard perches unchanged.
- **Sockets:** all 12 at the exact r1 coordinates and yaws (`Generator_0..3`, `Reactor`, `Bay`, `TrenchTurret_0..3`, `HullTurret_0..1`).
- **UCX:** `UCX_SM_DominionMothership_00..58` are identical to r1 (same bounds, vert and tri counts, convex and closed). Indices 59..120 are new hulls for the r2 superstructure, spires and flank terraces; none of them enter the trench, reactor keep-clear, bay or generator dome volumes.

## Budget
131,670 tris (≤ 150,000), one material slot, one UV channel. Emissive covers about 1.89% of the surface (≤ 5%).

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands. The crimson banner and white stylised trident are the Dominion faction marking from Nicholas's board, redrawn here. There is no text or other insignia.
- Licence, tool and method are recorded in `SOURCE.md`. A new round goes in a new folder.
