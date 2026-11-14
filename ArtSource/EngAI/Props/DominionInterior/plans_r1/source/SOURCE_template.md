# Dominion Interior — plans_r1 (R-042 queue item 3, R-043; follows #54 / PR #77)

Route C procedural: Blender 4.5.3 LTS + matplotlib + numpy. **No AI 3D or image generator, no fal spend.**
Branch `grok/assets-dominion-interior-plans-r1`. The docking-bay part is PR #77 (`ArtSource/EngAI/Props/DominionDockingBay/plans_r1/`,
merged); this folder covers the rest of the walkable interior and reuses that folder's frame, scripts and style.

## Scope (R-043)

| Who | What |
|---|---|
| **Builders (Unreal, structures builder)** | Build the walkable interior from `ArtSource/EngProps/DominionInterior` (400 cm grid, 600 cm storeys, collision = visual floor). Follow the plans; `blockout/` is a layout check, not a mesh to ship. |
| **Grok (this folder)** | Measured top plan, gameplay overlay, long + cross sections with kit piece names, cm dimensions, door / clearance widths, 180 cm figures, ship clearance, sightlines and the walkability standard; a 2K trim sheet + decal atlas with a UV-band guide; a blockout `.blend` assembled from the real kit FBXs; renders; this file; `manifest.json`. |

Nothing under `Source/`, `Content/`, `CLAIMS.md` or Nick's folders was touched. C++ mismatches are listed below for the code owner (T5).

## Files

| Path | What |
|---|---|
| `interior_layout.json` | Single source of truth: 1282 kit placements (mesh, x, y, z, yaw, group) + rooms, corridors, doors, gameplay volumes, spawns, checkpoints, consoles, core numbers, kit facts and recommendations. Made by `source/interior_layout.py` (imports PR #77's `dockbay_layout.py` for the pocket + hangar). |
| `plans/DI_Plan_Top.png` | Top plan in two strips (mouth -> corridor, arena -> core), floor z 0, ceilings off, every kit piece by pivot, 400 grid, hull X on the top axis, room / corridor / door sizes, console 150 cm clear zones, the corridor-cover call-out, legend, 20 m bars, figures. |
| `plans/DI_Plan_Gameplay.png` | The same plan with every C++ encounter / trigger volume, trooper and Warden spawns, entries, checkpoints, trailheads, lights, arena spawn radii, sightlines, and the N-052 trio ring. |
| `plans/DI_Section_Long.png` | Section A-A at y = 0 (two strips): level floor, ceilings 600 / 1200 / 1800 / 3600, doors open (+400), ship on Pad A, cover, core / rings / shield / conduits, Warden 260, 180 cm figures, hull Z on the right axis. |
| `plans/DI_Sections_Cross.png` | B-B corridor at the troopers' cover (the 130 cm gap), C-C doorway + door, D-D flight deck (and the Warden room), F-F core chamber, H-H hangar with the ship, and the walkability standard table + the ramp / stair rule for any future level change. |
| `textures/T_DI_InteriorTrim_{BC,N,ORM,E}.png` | 2K tiling trim sheet (12 bands, tiles in U, 5.12 px/cm). N is DirectX (Unreal green). ORM = AO, roughness, metallic. E is the coloured emissive. |
| `textures/T_DI_Decals_{BC,N,ORM,E}.png` | 2K decal atlas, 4 x 1024 cells: trident emblem, Triad three-chevron mark, hazard keep-clear frame, amber route arrow. BC is RGBA (alpha = opacity). |
| `textures/T_DI_InteriorTrim_Layout.png`, `textures/trim_layout.json` | Builder's guide: every band's V px range, UV v range, px and cm height; every decal's px and UV rectangle and suggested world size. |
| `blockout/DominionInterior_Blockout.blend` | 1359 objects: linked duplicates of the 15 kit meshes at scale 1 (sources in a hidden `KitSource` collection), 180 cm figures, trooper / Warden capsules at the C++ spawns, a ship-footprint box on Pad A, the C++ lights, and a `TrimPreview` bay (y +6000, off the route) with the new trim + decals. |
| `review/*.png` | Renders (Cycles, Blender's neutral `studio.exr` HDRI + the C++ point lights, AgX). `review/tex_stats.txt`, `review/sightlines.json`. |
| `contact_sheet.png` | Everything on one page. |
| `mismatches.json` | The C++ / brief mismatches below. |
| `source/*.py` | All scripts (rebuild order below). |

## Frame

Unreal cm, T5's course-local frame (`TDDominionInteriorCourse.inl` `BuildInterior`, main 188acc50): +X deeper into the ship,
floor top z = 0, 400 cm grid, 600 cm storeys; kit pieces by pivot at scale 1 exactly as `Kit()` / `Room()` / `CorridorRun()` /
`Doorway()` place them. Same frame as PR #77, so the pocket and the hangar line up. Hull r2 mapping: hull = (22800 - x, -y, z - 3750).

## Rooms and key dimensions (cm)

| Space | x range | Size (X x Y) | Clear height | Kit | Gameplay (C++) |
|---|---|---|---|---|---|
| Glide pocket (PR #77) | -7200 .. -1200 | 6000 x 5200 | 3600 | Room_Floor/Ceiling/Wall, Walkway_Ring gantries, Pads B/C | mouth 5000 x 3500 = SOCKET_Bay; threshold band; force field |
| Hangar | -1200 .. 1200 | 2400 x 2800 | 1200 (2 storeys) | Room_*, Hangar_Pad (Pad A at (-200, 0)), Console | pilot start (-600, 0); console A4_Recording_Hangar (-900, 900); escape trigger 1000 x 1000 on Pad A; HangarDoor at x 1200 |
| Deflect corridor | 1200 .. 5200 | 4000 x 400 (clear 340) | 600 (586 under ribs) | 10 x Corridor_Straight | lane encounter, 2 troopers at (4750, +-100) behind Cover_Low (4500, -70); sightline 35.5 m |
| Flight deck (arena) = "barracks" | 5200 .. 7600 | 2400 x 2800 | 600 | Room_*, 4 x Cover_Low at (5900/6900, +-500) | 3 waves, spawn r 800/900/900; consoles _Corridor (5450, -1100) and _Barracks (7300, 1100) |
| Link 1 | 7600 .. 8400 | 800 x 400 | 600 | 2 x Corridor_Straight | — |
| Warden's gate = "security checkpoint" | 8400 .. 10400 | 2000 x 2000 | 600 | Room_* | Warden (capsule half-height 130) at (9900, 0); 15 m from the door |
| Link 2 | 10400 .. 11200 | 800 x 400 | 600 | 2 x Corridor_Straight | — |
| Reactor core chamber | 11200 .. 14800 | 3600 x 3600 | 1800 (3 storeys) | Room_*, Core 800 x 800 x 1200, Ring A r500 / Ring B r575 at z 600, Shield r640 x 1380, 4 x Conduit 120 x 300 at r 1100 (45 deg) | heart r80 at (12240, 0); console _Core (11600, -1400); N-052 trio ring r 700..1500 kept clear |
| Doors (6) | x 1200, 5200, 7600, 8400, 10400, 11200 | Doorway 60 x 400 x 600 | clear **300 x 400** | Doorway + Door (slides +Z 400) | all recognise the pilot |

Route: pilot start to core centre 136 m; mouth to core centre 202 m. **The whole route is level (z 0)**: no stairs or ramps are needed.
If one is ever added: ramps <= 20 deg (R-042; the task brief's 30 deg is overruled), e.g. one 600 cm storey over 5 cells (2000 cm) = 16.7 deg;
stairs with risers <= 20 cm (never the 32-38 cm splits) on a ramp-proxy collision that is itself <= 20 deg (treads >= 55 cm); 400 x 400 landings.

Walkability (R-042) checked: doorways 300 x 400 (min 140 x 240), ceilings >= 586 (min 280), flat floors, consoles 150 cm clear in front (all four),
corridor 340 clear (312 at the ribs). **One fail:** the gap past the corridor cover is 130 cm (see mismatch 2).
Ship: the largest squad footprint 524 x 488 x 230 on Pad A has 970 cm headroom and 1126 cm to each side wall; the hangar opening to the pocket is 2800 x 1200.

## Sightlines (eye 170 cm, `review/sightlines.json`)

SIGHTLINES

## C++ / brief mismatches (for T5; nothing in `Source/` was changed)

MISMATCHES

## Trim sheet `T_DI_InteriorTrim` (2048 x 2048, 5.12 px/cm, tiles in U every 400 cm)

| Band | V px | UV v | Height | Use |
|---|---|---|---|---|
BANDS

Palette: gunmetal #565B63 / #4A4F57 / #646971 (lifted from the kit's #33373D so the BC median clears sRGB 55), deck #5C5F64, crimson #7A1626
with brass pinstripes, hazard #BF541A on #26272B, emissive red-orange (1.0, 0.14, 0.02) = MI_DI_GlowRed, amber (1.0, 0.55, 0.10), cyan
(0.25, 0.85, 1.0) = the console screen, white (0.9, 0.95, 1.0) = MI_DI_GlowWhite. Edge wear, grime and AO are baked; height -> normal with
wrap-around gradients, so every band tiles.

## Decals `T_DI_Decals` (2048 x 2048, four 1024 cells)

DECALS

## Texture stats (`/tmp/audit-scripts/tex_stats.py`)

```
TEXSTATS
```

## Rebuild (from this folder; `<repo>` = repo root)

```
python3 source/interior_layout.py interior_layout.json <repo>
python3 source/make_plans.py .
python3 source/make_trim_textures.py textures && python3 source/make_trim_guide.py .
blender -b --factory-startup --python source/build_blockout.py -- <repo> .
blender -b --factory-startup --python source/render_views.py -- . review 32
python3 source/make_contact_sheet.py . && python3 source/make_source_md.py . && python3 source/make_manifest.py .
```
