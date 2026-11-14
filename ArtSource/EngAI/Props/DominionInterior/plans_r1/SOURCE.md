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

- Hangar door (1200,0) -> troopers (4750,±100): 35.5 m, clear (cover blocks y <= 30 only past x 4470)
- Core door ckpt (11500,0) -> conduit (13777,777): 24.1 m, BLOCKED by the core body
- Core door ckpt (11500,0) -> conduit (12222,777): 10.6 m, clear
- Core door ckpt (11500,0) -> conduit (12222,-777): 10.6 m, clear
- Core door ckpt (11500,0) -> conduit (13777,-777): 24.1 m, BLOCKED by the core body
- Arena door to door: 24.0 m, clear (covers at |y| 500 off-axis)
- Warden door (8400) -> Warden spawn (9900): 15.0 m, clear

## C++ / brief mismatches (for T5; nothing in `Source/` was changed)

1. Hangar X0 wall: C++ Room(HX0, HX1, HY, 2, false, true) closes the hangar at x -1200; PR #77 (merged) opens it 2800 x 1200 onto the 60 m glide pocket (x -7200..-1200). The interior map has no pocket yet: builders add it per PR #77 or keep the wall.
2. Deflect-corridor cover (line 287, y -70): it spans y -170..30, leaving 130 cm to the +Y kick plate (|y| 160) - the only way past, under R-042's 140. Recommended y -100: gap 0..160 = 160 cm (a one-number change for the code owner).
3. No ship in the hangar: the escape goal is a trigger on Pad A (-200, 0) ('GET BACK TO YOUR SHIP') but nothing spawns the boarding pilot's ship there. The 524 x 488 x 230 footprint fits (970 cm headroom, 1126 cm wing-to-wall).
4. Header comment (line 7) says hangar 24 x 24 m and core 32 x 32 m; the code builds 24 x 28 m (HY 1400) and 36 x 36 m (KY 1800). The plans follow the code.
5. Escape debris lane |y| <= 150 (line 409) covers the corridor cover: one 150 cm SM_DI_Debris_A landing at y ~ +95 near x 4500 closes the 130 cm gap (if debris collides). Keep debris out of x 4300..4700 or move the cover.
6. Core chamber: from the core door the two far conduits (13778, +-778) hide behind the core body; only the near two are in view. Not a bug: the HUD/trailhead must point round the ring.
7. Heart at core + (-760, 0), r 80, sits at r 680..840: outside the shield shell (r 640), so it reads as outside the shield even while it is up (it only takes damage in step 1).
8. N-052 trio (future Wren, Kato, Moss) has no encounter volume in the core chamber yet; the plan keeps the ring r 700..1500 round the core clear for it.
9. Brief vs kit: corridors are 400 x 400 x 640 BOUNDS (-20..620); the clear height is 600 (586 under the portal ribs). Ramps: R-042 says <= 20 deg (the brief said 30); the plan uses 20.
10. Hull mapping (PR #77 frame hull = 22800 - x): the core chamber lands at hull X ~9800, Z -3750..-1950, not at the exterior Reactor socket (16500, 0, 6500). Separate level, so info only.

## Trim sheet `T_DI_InteriorTrim` (2048 x 2048, 5.12 px/cm, tiles in U every 400 cm)

| Band | V px | UV v | Height | Use |
|---|---|---|---|---|
| WALL_PANEL | 0-512 | 0.7500..1.0000 | 512 px = 100.0 cm | armour wall panels, gunmetal, 4 plates per tile (100 cm each), grooves + bolt rows |
| DECK_PLATE | 512-768 | 0.6250..0.7500 | 256 px = 50.0 cm | floor deck: diamond tread plate, matte, 2 plates per 100 cm |
| CRIMSON_BAND | 768-896 | 0.5625..0.6250 | 128 px = 25.0 cm | Dominion crimson band with brass pinstripes + rivets (hull r2 band language) |
| HAZARD | 896-1024 | 0.5000..0.5625 | 128 px = 25.0 cm | hazard chevrons orange #BF541A on black, 45 deg, period 256 px (50 cm) |
| VENT | 1024-1152 | 0.4375..0.5000 | 128 px = 25.0 cm | ribbed vent / grille, 6.25 cm pitch |
| KICK_PLATE | 1152-1280 | 0.3750..0.4375 | 128 px = 25.0 cm | kick plate / skirting, scuffed, worn top edge |
| STRIP_RED | 1280-1344 | 0.3438..0.3750 | 64 px = 12.5 cm | emissive strip, red-orange (1.0, 0.14, 0.02) = MI_DI_GlowRed |
| STRIP_AMBER | 1344-1408 | 0.3125..0.3438 | 64 px = 12.5 cm | emissive strip, amber (1.0, 0.55, 0.10) = MI_DI_Cell family |
| STRIP_CYAN | 1408-1472 | 0.2812..0.3125 | 64 px = 12.5 cm | emissive strip, cyan (0.25, 0.85, 1.0) = MI_DI_Screen family |
| STRIP_WHITE | 1472-1536 | 0.2500..0.2812 | 64 px = 12.5 cm | work-light strip, cold white (0.9, 0.95, 1.0) = MI_DI_GlowWhite |
| CABLE_TRAY | 1536-1792 | 0.1250..0.2500 | 256 px = 50.0 cm | cable tray: 5 pipes / cable runs with clamps every 50 cm |
| TRIM_CAP | 1792-2048 | 0.0000..0.1250 | 256 px = 50.0 cm | bevelled trim caps / door-jamb panels, 2 rows of small plates with bolts |

Palette: gunmetal #565B63 / #4A4F57 / #646971 (lifted from the kit's #33373D so the BC median clears sRGB 55), deck #5C5F64, crimson #7A1626
with brass pinstripes, hazard #BF541A on #26272B, emissive red-orange (1.0, 0.14, 0.02) = MI_DI_GlowRed, amber (1.0, 0.55, 0.10), cyan
(0.25, 0.85, 1.0) = the console screen, white (0.9, 0.95, 1.0) = MI_DI_GlowWhite. Edge wear, grime and AO are baked; height -> normal with
wrap-around gradients, so every band tiles.

## Decals `T_DI_Decals` (2048 x 2048, four 1024 cells)

- **TRIDENT** px [0, 0, 1024, 1024], UV (0.00, 0.50)-(0.50, 1.00): Dominion trident emblem plate (original): crimson disc, gunmetal ring, pale trident; 160 cm on door heads / bulkheads
- **TRIAD_MARK** px [1024, 0, 2048, 1024], UV (0.50, 0.50)-(1.00, 1.00): three-chevron Triad unit marking + bar-and-dot bay mark, worn stencil (brief: on lintels, half-covered); 120 cm
- **HAZARD_FRAME** px [0, 1024, 1024, 2048], UV (0.00, 0.00)-(0.50, 0.50): hazard-striped keep-clear frame for door thresholds / pad edges (no text); 400 cm floor
- **ROUTE_ARROW** px [1024, 1024, 2048, 2048], UV (0.50, 0.00)-(1.00, 0.50): squadron route stencil: three amber emissive chevrons pointing +U with dot/bar marks; 200 cm floor

## Texture stats (`/tmp/audit-scripts/tex_stats.py`)

```
T_DI_Decals_BC.png RGBA 2048x2048 lum p1/5/25/50/75/95/99=[  0.   0.   0.   0.  43. 181. 192.] | <20: 64.5% <40: 69.9% >235: 0.0% | mean [47. 31. 27.]
T_DI_Decals_E.png RGB 2048x2048 lum p1/5/25/50/75/95/99=[  0.   0.   0.   0.   0.   0. 156.] | <20: 97.6% <40: 97.6% >235: 0.0% | mean [6. 3. 1.]
T_DI_Decals_N.png RGB 2048x2048 NORMAL tilt<2deg 98.9% | <6deg 98.9% | 6-24deg 0.1% | >24deg 1.0% | meanRGB [128. 128. 256.]
T_DI_Decals_ORM.png RGB 2048x2048 lum p1/5/25/50/75/95/99=[104. 120. 146. 146. 146. 158. 158.] | <20: 0.0% <40: 0.0% >235: 0.0% | mean [250. 125.  16.]
T_DI_InteriorTrim_BC.png RGB 2048x2048 lum p1/5/25/50/75/95/99=[ 37.  41.  59.  87.  92. 134. 249.] | <20: 0.0% <40: 3.4% >235: 1.2% | mean [91. 83. 86.]
T_DI_InteriorTrim_E.png RGB 2048x2048 lum p1/5/25/50/75/95/99=[  0.   0.   0.   0.   0.   0. 240.] | <20: 95.3% <40: 95.3% >235: 1.2% | mean [9. 7. 6.]
T_DI_InteriorTrim_N.png RGB 2048x2048 NORMAL tilt<2deg 71.7% | <6deg 73.8% | 6-24deg 10.4% | >24deg 15.8% | meanRGB [128. 128. 253.]
T_DI_InteriorTrim_ORM.png RGB 2048x2048 lum p1/5/25/50/75/95/99=[ 79. 106. 126. 133. 157. 179. 185.] | <20: 0.0% <40: 0.0% >235: 0.0% | mean [251. 106. 146.]
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
