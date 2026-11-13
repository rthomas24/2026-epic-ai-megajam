# Dominion Mothership hull - mesh_r1 (brief)

Issue **#41**, R-030 item 6. This is the first 3D delivery of the finale capital ship **`SM_DominionMothership`**. It is built from the
**v3** turnaround that Nicholas approved as-is (`../concept_r1/source/sheet-DominionMothership-v3_PICK.png`, thumbnail in `refs/`).
It follows the T4 local-space contract for the game's real size: a 600 m hull, four off-hull generator positions at radius 388 m,
and the R-042 socket list.

## What to import
| File | What it is |
|---|---|
| `SM_DominionMothership.fbx` | **Use this one in Unreal.** It has the render mesh `SM_DominionMothership` (1 material, 1 UV set), 59 `UCX_SM_DominionMothership_00..58` convex collision hulls and 12 `SOCKET_*` empties parented to the mesh. `UnitScaleFactor` = 100, so import at uniform scale **1.0**. |
| `SM_DominionMothership.glb` | The same mesh and the same 12 socket nodes, in glTF metres with +Y up. It has no UCX and embeds an OpenGL-green copy of the normal map, as glTF requires. |
| `T_DominionMothership_{BC,N,ORM,E}.png` | One unified 2048 PBR set (downsampled with Lanczos from the 4096 masters in `source/textures_4k/`). BC is sRGB. N is **DirectX**. ORM is R = AO, G = Roughness, B = Metallic. E is sRGB emissive. |
| `T_DominionMothership_Detail_{N,ORM}.png` | A 2048 tileable detail layer (panel seams, rivets, vents) so the trench holds up at 30-80 m. Detail_ORM is R = AO/cavity, G = roughness variation, B = 0. The suggested tile is about 8 m. |
| `contact_sheet.png` | v3 vs beauty renders, side/front/rear orthos, inside the trench (reactor keep-clear and turret sockets), the bay mouth and glide pocket, a **180 cm pilot** at the bay mouth (R-042), a generator plinth with the dome keep-clear, and top and side blueprints with all 12 sockets and 59 UCX hulls. |
| `manifest.json` | Sockets, UCX list with bounds, tri count, clearances, texture QA, file hashes. |

## Contract (Unreal cm, +X bow, +Y starboard, +Z up, origin = hull centre)
- **Hull core:** 60000 × 16000 × 10000 (X ±30000, Y ±8000, Z ±5000). It is an octagonal armoured hull with forward-raked flank shingles, dashed amber seams, white eye slits and three deep-red trident banners per side.
- **Bridge / citadel:** the base block is 7500 × 4000 × 6000 (X -23250..-15750, Y ±2000, Z 5000..11000), centred around (-19500, 0, 8000). Its forward brow overhangs the trench end only at Z ≥ 8700. A tiered stern citadel flanks it, and the spire mast reaches Z 18400.
- **Flyable dorsal trench:** X -18000..+24000. The floor is flat at Z 5000. The walls run |Y| 2500..3400 and Z 5000..8000. Measured clear: **0 tris in |Y| < 2350, Z 5000..8500** along the full length. The forward entry (X > 24000, |Y| < 2490) is open.
- **Reactor** (16500, 0, 6500): the nearest render triangle is 1508 cm away and the nearest UCX is 1500 cm away (≥ 1400 required).
- **Bay** (30000, 0, -2000), +X out: the mouth is 5000 × 3500 (Y ±2500, Z -3750..-250), with a 6000 cm glide pocket X 24000..30000, amber rim lights and an aft bulkhead. There are **0 tris and 0 UCX** in X > 30000, |Y| < 2800, Z -4000..0, so the launch point (33000, 0, -2000) is open.
- **Generators** at (±27500, ±27500, 0): four swept armoured outrigger arms come off the flanks with red trim. Each ends in a circular plinth (r 2400, top at Z -50). The volume r 1800, Z 0..4000 above each socket is empty for the standalone dome. Guard-turret perches sit ±3000 along the ring tangent, with their mount top at Z 1420 for turrets at +1500.
- **Hull turrets** (-18000, -8000, 5500) and (18000, 8000, 5500) sit on armoured flank sponsons.
- **Trench turrets** (X -15000 / -6000 / 3000 / 12000, Y ∓1700, Z 7700) have wall-rim brackets that stay at |Y| ≥ 2369.

## Budget
87,756 tris (≤ 150,000), one material slot, one UV channel. Emissive covers about 1.4% of the surface (≤ 5%).

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands. The deep-red banner and white stylised trident are the Dominion faction marking from Nicholas's board, redrawn here. There is no text or other insignia.
- Licence, tool and method are recorded in `SOURCE.md`. A new round goes in a new folder.
