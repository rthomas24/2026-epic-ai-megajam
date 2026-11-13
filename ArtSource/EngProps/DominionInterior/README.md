# DominionInterior: modular kit for the inside of the Dominion (boarding finale, ENG-29 / ENG-30)

22 static meshes for the code-built interior: corridors, doorway and door, room shell, cover, hangar pad, the reactor
core with its rings and shield, conduit, walkway, debris, console, plus a light-well shaft and a loose bulkhead rib.
Engineering-made (route C, code-built, original work, no downloads); Nicholas keeps art direction. Brief:
[`brief.md`](brief.md).

**Stage reached (AAA.md): technically verified.** Sizes measured in Blender and again in Unreal (bounds min / max,
so pivots too), triangles, material slots and collision hull counts checked on a headless import (`TDPY: OK`).
Not seen in a running game, not lit in Unreal, not presentation approved, not integrated (nothing spawns it yet).

## Rules every mesh follows

- Unreal cm, **+X forward**, scale 1. **Pivot on the walkable plane (Z = 0) at the centre of the footprint.**
- Sizes are exact (table below; `review/size_check_r3.json`, `review/ue_import_report.json`).
- 400 cm grid. Every trim in the atlas repeats over 400 cm, so pieces that share a wall, floor or ceiling line up.
- Two materials for the kit: `MI_DI_Atlas` (slot `M_DI_Atlas`, always slot 0) and the emissive family (below).
- Simple collision on every mesh except the two rings: convex boxes inside the stated box (corridors: floor,
  ceiling, each wall, each junction column; doorway: two posts and the lintel; walkway: deck and rail; shaft: four
  walls; core: base drum and a tapered body; debris and shield: one convex hull). No Nanite, no lightmap UVs.
- The `.blend` is the Y MIRROR of what Unreal shows (FBX hands Blender +Y to Unreal as -Y; `di_lib.Piece.build`
  mirrors on purpose and the import script checks the walkway rail and the corner column land on +Y).

## Meshes (`/Game/TriadDrift/Gameplay/Props/DominionInterior/`), measured in Unreal

| Mesh | Size X x Y x Z cm | Bounds min .. max (from the pivot) | Tris | Slots | Hulls |
|---|---|---|---|---|---|
| `SM_DI_Corridor_Straight` | 400 x 400 x 640 | (-200, -200, -20) .. (200, 200, 620) | 152 | Atlas, GlowRed, GlowWhite | 4 |
| `SM_DI_Corridor_Corner` | 400 x 400 x 640 | (-200, -200, -20) .. (200, 200, 620) | 162 | Atlas, GlowRed, GlowWhite | 5 |
| `SM_DI_Corridor_T` | 400 x 400 x 640 | (-200, -200, -20) .. (200, 200, 620) | 124 | Atlas, GlowRed, GlowWhite | 5 |
| `SM_DI_Doorway` | 60 x 400 x 600 | (-30, -200, 0) .. (30, 200, 600) | 72 | Atlas, GlowRed, GlowWhite | 3 |
| `SM_DI_Door` | 30 x 300 x 400 | (-15, -150, 0) .. (15, 150, 400) | 48 | Atlas, GlowRed | 1 |
| `SM_DI_Room_Wall` | 60 x 400 x 600 | (-30, -200, 0) .. (30, 200, 600) | 82 | Atlas, GlowRed | 1 |
| `SM_DI_Room_Floor` | 400 x 400 x 20 | (-200, -200, -20) .. (200, 200, 0) | 18 | Atlas | 1 |
| `SM_DI_Room_Ceiling` | 400 x 400 x 20 | (-200, -200, 600) .. (200, 200, 620) | 28 | Atlas, GlowWhite | 1 |
| `SM_DI_Cover_Low` | 60 x 200 x 110 | (-30, -100, 0) .. (30, 100, 110) | 76 | Atlas, GlowRed | 1 |
| `SM_DI_Hangar_Pad` | 1200 x 1200 x 40 | (-600, -600, -40) .. (600, 600, 0) | 90 | Atlas, GlowRed, GlowWhite | 1 |
| `SM_DI_Core` | 800 x 800 x 1200 | (-400, -400, 0) .. (400, 400, 1200) | 6090 | Atlas, CoreGlow, GlowRed | 2 |
| `SM_DI_Core_Ring_A` | 999.5 x 991 x 60 | (-495.5, -495.5, -30) .. (504, 495.5, 30) | 810 | Atlas, GlowRed, GlowWhite | 0 |
| `SM_DI_Core_Ring_B` | 1150 x 1133 x 48 | (-575, -566.5, -24) .. (575, 566.5, 24) | 796 | Atlas, GlowRed, GlowWhite | 0 |
| `SM_DI_Core_Shield` | 1280 x 1280 x 1380 | (-640, -640, 0) .. (640, 640, 1380) | 108 | Shield | 1 |
| `SM_DI_Conduit` | 120 x 120 x 300 | (-60, -60, 0) .. (60, 60, 300) | 296 | Atlas, Cell | 1 |
| `SM_DI_Walkway_Ring` | 400 x 300 x 130 | (-200, -150, -20) .. (200, 150, 110) | 68 | Atlas, GlowRed | 2 |
| `SM_DI_Debris_A` | 150 x 123.1 x 52.7 | (-75, -61.6, 0) .. (75, 61.6, 52.7) | 60 | Atlas | 1 |
| `SM_DI_Debris_B` | 150 x 145.1 x 96.9 | (-75, -72.6, 0) .. (75, 72.6, 96.9) | 56 | Atlas | 1 |
| `SM_DI_Debris_C` | 114.9 x 150 x 133.6 | (-57.5, -75, 0) .. (57.5, 75, 133.6) | 48 | Atlas | 1 |
| `SM_DI_Console` | 60 x 120 x 130 | (-30, -60, 0) .. (30, 60, 130) | 48 | Atlas, GlowRed, Screen | 1 |
| `SM_DI_Shaft` (optional) | 400 x 400 x 1200 | (-200, -200, 0) .. (200, 200, 1200) | 128 | Atlas, GlowWhite | 4 |
| `SM_DI_Bulkhead_Rib` (optional) | 14 x 24 x 600 | (-7, -12, 0) .. (7, 12, 600) | 18 | Atlas | 1 |

## Conventions gameplay needs

- **Place by the pivot, at scale 1.** `TDPropArt::Dress(..., bFit = false)` puts the art's bounds bottom-centre on
  the shape's bottom-centre, which is NOT the pivot for the pieces whose bounds do not start at Z = 0 (corridors
  and floor start at -20, the pad at -40, the ceiling at 600, the walkway at -20). For those, set the component at
  the cell's floor-centre directly. `bFit = true` is exact only for a shape that equals the bounds in the table.
- **Cell convention:** floor top at Z = 0, clear height 600. Floor slabs occupy -20 .. 0 and ceiling slabs
  600 .. 620, in the corridor pieces and in `SM_DI_Room_Floor` / `SM_DI_Room_Ceiling` alike.
- **Ceiling:** `SM_DI_Room_Ceiling` goes at the SAME transform as the floor tile below it (pivot at floor level);
  its underside is then at Z = 600. It has one recessed white work light along X.
- **Corridors:** inner wall faces at Y = +-170 (clear width 340; kick plates and cable trays reach +-160, the portal
  rib at X = 0 reaches +-156 and crosses under the ceiling at Z = 586). Walls are 30 thick inside the footprint.
  Straight: open at both X ends. Corner: open at -X and +Y (walls on -Y and +X). T: open at -X, +Y and -Y (wall on
  +X). The corner and T have an 80 x 80 structural column on each open corner (at X -200 .. -120, |Y| 120 .. 200),
  which hides the neighbours' wall ends; it has collision. Straights have a grating lane, junctions have floor
  plates.
- **Doorway:** a full 400 x 600 frame, 60 deep, the same on both faces. Clear opening exactly 300 x 400 (Y +-150,
  Z 0 .. 400). Put it on the seam between two cells (it overlaps each by 30).
- **Door:** fills the opening at the doorway's own transform. **It slides up: +Z by 400** (it then sits inside the
  lintel and pokes 200 above the ceiling, which nothing sees). Sideways (Y by 300) also works where a wall hides it.
- **Room wall:** the same on both faces, so no facing to get wrong; tiles along Y with a rib at its centre. It is
  centred on its plane (X +-30): put it on the cell boundary.
- **Tiling pieces:** corridors along their open axes, `SM_DI_Room_Wall` along Y, `SM_DI_Room_Floor` and
  `SM_DI_Room_Ceiling` in X and Y, `SM_DI_Walkway_Ring` along X, `SM_DI_Shaft` in Z (1200 steps).
- **Walkway:** top at Z = 0, rail on **+Y** (the outer side) to Z = 110, a hazard strip and a red edge light on -Y.
- **Hangar pad:** the visible top is 0.4 cm under Z = 0 (the lights sit at 0); the collision top is exactly 0.
- **Core:** `SM_DI_Core_Ring_A` / `_B` lie flat in XY with the pivot at the ring centre and their axis on +Z; place
  them at the core's pivot + (0, 0, 600) and spin them (A has three armour blocks, B two, so the spin reads). Keep
  any tilt under about 20 degrees so they stay inside the shield (inner radius about 608 at its narrowest facet).
  `SM_DI_Core_Shield` shares the core's pivot; hide it or disable its collision when the shield drops.
- **Console:** screen faces **-X** (a user stands at -X). The screen is slot 2, `MI_DI_Screen`, dark by default.
  To light it: `CreateDynamicMaterialInstance(2)`, then `SetScalarParameterValue("GlowStrength", 6..12)` and, if
  wanted, `SetVectorParameterValue("GlowColor", ...)` (a pilot's colour when it recognises them).
- **Conduit:** the glowing cell is slot 1, `MI_DI_Cell` (same parameters); set `GlowStrength` 0 when broken.
- **Debris:** pivot at the bottom centre of its bounds, largest dimension 150.
- **What ships:** these are loaded by path, so the folder must be referenced by something cooked (a hard reference
  from the interior's class or data asset) or added to the cook list, or it will be missing from the packaged game.

## Materials

| Asset | Parent | Parameters |
|---|---|---|
| `MI_DI_Atlas` | `/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured` | `Textures/T_DI_Atlas_BC`, `_N`, `_ORM`; `EmissiveStrength` 0. `SelfIllum` (0) is there if the level is too dark |
| `MI_DI_GlowRed` | `/Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow` | `GlowColor` (1, 0.14, 0.02) as the fleet's red, `GlowStrength` 8 |
| `MI_DI_GlowWhite` | same | (0.9, 0.95, 1.0), 14 |
| `MI_DI_Screen` | same | (0.25, 0.85, 1.0), 0 (off) |
| `MI_DI_Cell` | same | (1, 0.42, 0.08), 14, slow pulse |
| `MI_DI_CoreGlow` | same | (1, 0.86, 0.7), 30, slow pulse |
| `MI_DI_Shield` | `Materials/M_DI_Shield` (new: unlit, translucent, two-sided, fresnel) | `ShieldColor`, `ShieldStrength` 6, `ShieldOpacity` 0.35 |

Glow strengths are first guesses for an interior (the fleet uses 20-30 in open space); none has been seen in
Unreal. The atlas is one 2048 set at 2.56 px/cm; layout in `scripts/di_regions.py`.

## Files

```
DominionInterior/
  brief.md, README.md
  DominionInterior.blend              every piece (Y-mirrored, see above) with its UCX_ children
  scripts/di_regions.py               atlas layout
  scripts/di_atlas.py                 paints textures/T_DI_Atlas_BC / _N / _ORM (numpy, seeded)
  scripts/di_lib.py                   Piece: polygons with atlas UVs, boxes, prisms, lathes, collision, mesh build
  scripts/di_kit.py                   the pieces and the size contract (EXPECT)
  scripts/build_di_kit.py             driver: build, size check, review sheets, export
  scripts/ue_import_di_kit.py         Unreal import and verification (headless)
  textures/T_DI_Atlas_BC|N|ORM.png
  export/SM_DI_*.fbx, *.manifest.json
  review/kit_b1_*, kit_r1_*, kit_r2_*, kit_r3_* (clay, material, assembly), size_check_*.json, health_*.json,
         ue_import_report.json
```

Rebuild (Blender 5.2, from this folder):
`blender --background --factory-startup --python scripts/di_atlas.py`, then
`blender --background --factory-startup --python scripts/build_di_kit.py -- --tag rN --render --export`, then
`powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\DominionInterior\scripts\ue_import_di_kit.py`
(the import reads `review/size_check_r3.json`; change the name in the script if the tag changes).

## Review and honest gaps

Rounds: b1 blockout (sizes verified before detail), r1, r2 (atlas normal noise cut: the gloss armour looked like
orange peel; glow strips thinned from 8 / 6 cm to 5 / 4 cm; core petals widened), r3 (Y mirror for Unreal; final).
Self-scored against `aaa-quality-bar.md` (no independent reviewer): silhouette 3, hierarchy 3, construction 3.5,
edges and shading 2.5, colour and materials 4, scale cues 4, technical 4.

- Most pieces are far under the 300-triangle floor of the brief (18-160): the detail is in the atlas, and hard
  edges have no bevel geometry, so edges do not catch light the way chamfered ones would.
- Debris is the weakest: folded plates that read as dark shards.
- The door and the doorway are plain in silhouette; the three-chevron mark is on the lintel (both faces), three
  times round the core's base and embossed on the lower petals, nowhere else.
- Stencil marks repeat every 400 cm (one bay mark and one bracket per cell).
- Emissive overlays (strips, lights, screen) are quads 0.3-0.6 cm off the surface, not recessed geometry.
- Not verified: anything under Unreal's renderer (material compile of `M_DI_Shield`, glow levels, normal map
  direction on the mirrored meshes, the look under the level's lights), collision hull extents in Unreal (the count
  is checked; 5.9's Python does not expose the hull vertices), and anything in game.
