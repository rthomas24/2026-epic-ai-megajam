# EngAI Terrain r1

Nine 2K seamless PBR tileables for ENG-34 (R-027 request #3), generated in PBRgen and delivered for the materials builder to wire into `M_TD_Terrain`.

Overview (all nine base colours, each tiled 2×2): `contact_sheet_overview.png`.

| Set | Delivery prefix | Biome |
|---|---|---|
| Verdance_Moss | `T_Verdance_moss_*` | Verdance |
| Verdance_Soil | `T_Verdance_Soil_*` | Verdance |
| Verdance_CliffRock | `T_Verdance_CliffRock_*` | Verdance |
| Verdance_PeakLichenFrost | `T_Verdance_PeakLichenFrost_*` | Verdance |
| Cinder_Basalt | `T_Cinder_Basalt_*` | Cinder |
| Cinder_Ash | `T_Cinder_Ash_*` | Cinder |
| Cinder_Obsidian | `T_Cinder_Obsidian_*` | Cinder |
| Cinder_LavaCrust | `T_Cinder_LavaCrust_*` | Cinder |
| Cinder_CliffRock | `T_Cinder_CliffRock_*` | Cinder |

Each folder has `brief.md`, `SOURCE.md`, `source/` (original PBRgen zip contents, untouched), the final `T_*` PNGs at the folder root, and `contact_sheet.png`.

All normals are DirectX. ORM layout is R = ambient occlusion, G = roughness, B = metallic. Tile scale is 4 m at 2K.
