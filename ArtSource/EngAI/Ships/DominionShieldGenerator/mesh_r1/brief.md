# SM_DominionShieldGenerator + SM_DominionShieldGeneratorCore — mesh_r1 brief

**Issue:** [#41](https://github.com/rthomas24/2026-epic-ai-megajam/issues/41)
**Path:** `ArtSource/EngAI/Ships/DominionShieldGenerator/mesh_r1/`
**Route:** C (procedural hard-surface in Blender 4.5.3 LTS). No AI 3D generator.
**Presentation slots:** `UTDDominionVisuals.GeneratorMesh` + `GeneratorCoreMesh`.

## Purpose and camera
One of the four Dominion shield generators that sit on the mothership outrigger plinths (`SOCKET_Generator_0..3` at (±27500, ±27500, 0) cm). Shootable while the main shield is up (800 HP). Seen from fighter chase-cam (tens of metres) and in the GeneratorKill money shot, so the glowing core has to read as a weak point at a glance and the pylon has to leave a solid stump when it dies.

## Size, pivot, collision
| | |
|---|---|
| Pivot | Actor / `SOCKET_Generator` origin. `+Z` up. Base seats on the plinth top at **Z = −50 cm**. |
| Footprint | 14.88 × 14.88 m (max radius 7.44 m) — inside the mothership **r = 18 m, Z 0..40 m** keep-clear cylinder. |
| Height | 19.68 m (Z −50 .. +1918 cm), matching the 12 × 20 m placeholder cylinder. |
| Core | Visual centre at **Z = +1100 cm** in the lantern cradle. See SOURCE.md for the C++ `(0,0,60)` offset. |
| Tris | Pylon 16,512 evaluated; core 6,040 evaluated. |
| Collision | `UCX_SM_DominionShieldGenerator_00..42` convex hulls on the base, collar, pylons, rings and prongs. Core has no collision (`NoCollision` in `TDShieldGenerator.cpp`). |
| Socket | `SOCKET_Core` at (0, 0, 1100) cm. |

## Shape language
**Fortress lantern.** Heavy lathed base ring and stepped armored collar (the wreckage stump), six flying buttresses, an open containment lantern holding a faceted cyan energy core, and a crown of emitter prongs / spikes that throw the shield.

## Palette
Dominion hull r2 language: dark gunmetal armour (albedo median sRGB ~77), crimson / maroon trim and trident banners, warm amber indicator strips, electric cyan `#4CCFFF` / `FLinearColor(0.3, 0.8, 1.0)` weak-point core.

## Deliverables
| File | Role |
|---|---|
| `SM_DominionShieldGenerator.blend` / `.fbx` / `.glb` | Editable source (live Bevel + WeightedNormal + `smooth_by_angle(30°)`) and Unreal import. |
| `SM_DominionShieldGeneratorCore.blend` / `.fbx` / `.glb` | Paired core, one `.blend` per exported FBX. |
| `T_DominionShieldGenerator_*` and `T_DominionShieldGeneratorCore_*` | 2K PBR (`BC`, DirectX `N` 255-G, `ORM`, `E`). 4K masters in `source/textures_4k/`. |
| `contact_sheet.png` | Studio HDRI, AgX. Hero, orthos, details, 180 cm pilot, `ShowDestroyed()` stump, plinth check, clay, edge, UCX + socket. |
| `SOURCE.md`, `manifest.json` | Measured contract, C++ notes, audit numbers. |
