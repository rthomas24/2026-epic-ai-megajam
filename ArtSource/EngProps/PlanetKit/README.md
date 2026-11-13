# PlanetKit: on-foot set pieces for Verdance and Cinder

28 static meshes that replace the engine basic shapes (cubes, cylinders, cones) gameplay used for the planet set
pieces (features.md ENG-25 Verdance, ENG-26 Cinder). Engineering-made under the Nov 12 small-props rule (AGENTS.md
section 11); Nicholas keeps art direction and may restyle or replace any of it. Brief: [`brief.md`](brief.md).

Built in Blender 5.2 headless by `scripts/build_planet_kit.py` (seeded, reproducible), imported into Unreal 5.9 by
`scripts/ue_import_planet_kit.py`. Review sheets, health reports and the footprint check: `review/`.

**Stage reached (AAA.md): technically verified** (footprints measured, mesh health clean, FBX exported, imported
headless). Not presentation approved, not integrated (gameplay still spawns its basic shapes), not gameplay validated.

## Rules every mesh follows

- **Units** cm in Unreal (1 Blender m = 100 uu). **Forward = Unreal +X** (Blender +X, exported with `front_axis="+X"`).
- **Pivot at the bottom centre** (min Z = 0, X / Y centred on the footprint) except `SM_Cind_DebrisChunk` and
  `SM_ExtractionCore`, which pivot at their centre.
- **Footprints are exact** (the mesh bounds equal the gameplay box to the millimetre; see the table). Walkable tops
  are flat at exactly the stated height; flagstone joints, glyph grooves and the node socket sit below it.
- **No collision on the meshes.** Gameplay keeps its own collision boxes. The import removes simple shapes and sets
  "use simple as complex", so these meshes never collide.
- **Nanite on**, one material slot, one material instance `MI_<name>` of the master `M_TD_PlanetKit`.
- Scale: gameplay may scale the crates, console and route post a little; everything else is authored to size.

## Unreal paths

All under `/Game/TriadDrift/Gameplay/Props/Planet/` (the existing `SM_LandingPad`, `MI_LandingPad` and
`T_LandingPad_*` there are untouched):

- meshes `SM_<name>`, instances `MI_<name>` (name = the asset name without `SM_`)
- textures `Textures/T_<name>_BC` (sRGB), `_N` (DirectX normal), `_ORM` (AO / roughness / metallic, Masks),
  `_E` (emissive colour); `Textures/T_PK_DetailStone_N` (tiling detail normal)
- master `Materials/M_TD_PlanetKit`: `BaseColorTex`, `ORMTex`, `NormalTex`, `EmissiveTex` x `EmissiveStrength`, plus
  `DetailNormalTex` on texture coordinate 1 x `DetailTiling`, mixed in by `DetailStrength` (0 = off; 0.55 on the stone
  and rock pieces, whose second UV channel `UVDetail` is a stretch-free box projection at 4 m per tile)
- Interchange pipeline `Pipelines/PL_PK_StaticMesh`

## Assets

| Asset (`/Game/TriadDrift/Gameplay/Props/Planet/`) | Size X x Y x Z cm (measured) | Pivot | Walkable top cm | Footprint check | Tris | Tex | Final round | What it is |
|---|---|---|---|---|---|---|---|---|
| `SM_Cind_ChokeRock_A` | 400 x 1200 x 600 | bottom centre | - | PASS | 9378 | 2048 | r7 | basalt column wall with amber vein cracks and crystal clusters |
| `SM_Cind_ChokeRock_B` | 400 x 1400 x 600 | bottom centre | - | PASS | 10866 | 2048 | r7 | basalt column wall with amber vein cracks and crystal clusters |
| `SM_Cind_CollapseSlab` | 610 x 320 x 60 | bottom centre | 60 | PASS | 4798 | 1024 | r14 | columnar basalt bridge: 5-7 sided column tops (Voronoi), four settled pieces split by deep cracks with crystals below the walking surface, magma only at the lava line |
| `SM_Cind_Crate_A` | 300 x 220 x 140 | bottom centre | 140 | PASS | 3300 | 1024 | r8 | armoured supply crate (cover) |
| `SM_Cind_Crate_B` | 360 x 300 x 140 | bottom centre | 140 | PASS | 3300 | 1024 | r8 | armoured supply crate (cover) |
| `SM_Cind_DebrisChunk` | 125 x 150 x 126 | centre | - | PASS | 722 | 1024 | r12 | four fused basalt column shards with fracture planes and an amber crystal cluster, physics debris (largest dimension 150 cm) |
| `SM_Cind_JammerTower` | 1400 x 1600 x 4200 | bottom centre | - | PASS | 13972 | 2048 | r7 | signal jammer: raked-leg base 5 x 5, 34 m shaft, raked antenna arms 16 m (Y) / 14 m (X), caged emitter crown |
| `SM_Cind_OutpostWall` | 150 x 1500 x 600 | bottom centre | - | PASS | 10764 | 2048 | r8 | Null Armada outpost wall segment, tiles along Y; inner face -X flush, buttresses + raked blade merlons on +X |
| `SM_Cind_PrototypeConsole` | 160 x 160 x 140 | bottom centre | - | PASS | 2420 | 1024 | r13 | Null prototype console: blade-cut base, raked spikes, crimson plates, raked head with a red-orange screen facing +X, caged glowing cell |
| `SM_Cind_OutpostWall_Doorway` | 150 x 600 x 600 | bottom centre | - | PASS | 2240 | 2048 | r8 | door frame filling the 6 m gap between walls; opening 440 x 500 centred; lintel; lit framing |
| `SM_Cind_SteppingStone` | 280 x 280 x 160 | bottom centre | 160 | PASS | 2296 | 1024 | r8 | hex basalt column cluster, magma glow at the base |
| `SM_Console` | 60 x 60 x 120 | bottom centre | - | PASS | 932 | 1024 | r7 | neutral terminal, angled head with a glowing screen facing +X |
| `SM_Door` | 30 x 620 x 600 | bottom centre | - | PASS | 2064 | 1024 | r9 | sliding outpost door, two interlocking halves, split seam lit |
| `SM_ExtractionCore` | 55 x 55 x 110 | centre | - | PASS | 1620 | 1024 | r7 | energy core canister (carried), guard bars, handle |
| `SM_RoutePost` | 34 x 34 x 154 | bottom centre | - | PASS | 1196 | 1024 | r7 | slim three-vane post, teardrop glowing bulb in a cradle |
| `SM_Trailhead` | 500 x 500 x 480 | bottom centre | 40 (plinth) | PASS | 1416 | 1024 | r7 | plinth + tapered marker column with glyph strips and a glowing crown |
| `SM_Verd_Log` | 500 x 110 x 140 | bottom centre | - | PASS | 9923 | 1024 | r16 | gnarled log: bark fissures, root flare and roots, hollow splintered end with glowing mushrooms, tiered shelf fungus, soft moss sheet |
| `SM_Verd_NodeStub` | 320 x 320 x 180 | bottom centre | 180 (rim; socket below) | PASS | 1864 | 1024 | r7 | octagonal node pedestal with a glowing core in a socket |
| `SM_Verd_Plateau` | 2600 x 2600 x 600 | bottom centre | 600 | PASS | 26084 | 2048 | r7 | carved plateau: ashlar, pilasters, glyph band, 3 medallions per side, glyph rings on top |
| `SM_Verd_RelayAltar` | 200 x 200 x 150 | bottom centre | 150 (rim; socket below) | PASS | 2992 | 1024 | r1 | relay awakening altar: steps, octagonal glyph drum, 2 x 2 m top slab round a funnel socket holding the glowing glyph core |
| `SM_Verd_RelayPlatform_L` | 1000 x 1000 x 330 | bottom centre | 330 | PASS | 19208 | 2048 | r7 | relay platform: ashlar, glyph band, 3 medallions per side |
| `SM_Verd_RelayPlatform_M` | 600 x 600 x 290 | bottom centre | 290 | PASS | 8576 | 2048 | r7 | relay platform: ashlar, glyph band, medallions |
| `SM_Verd_RelayPlatform_S` | 600 x 600 x 110 | bottom centre | 110 | PASS | 3224 | 2048 | r7 | relay platform: ashlar courses, glyph band |
| `SM_Verd_RuinPillar` | 120 x 120 x 600 | bottom centre | - | PASS | 1352 | 1024 | r7 | fluted pillar with a glowing band |
| `SM_Verd_Wreck_Engine` | 900 x 700 x 700 | bottom centre | - | PASS | 17457 | 2048 | r11 | engine pod pitched and 1.2 m deep in soil: broken intake fan, top breach with drum and rings, exhaust end sheared off, torn livery band |
| `SM_Verd_Wreck_Fin` | 500 x 160 x 220 | bottom centre | - | PASS | 8075 | 1024 | r12 | torn swept tail fin: airfoil with skin thickness, ribs, spars out of the torn root, hanging rudder, tip nav pod and antenna |
| `SM_Verd_Wreck_Hull` | 2600 x 900 x 700 | bottom centre | - | PASS | 26038 | 2048 | r7 | half-buried torn hull section: chined skin, livery, windows, hatch, fin stub, moss, vines, glow caps |
| `SM_Verd_Wreck_Wing` | 600 x 160 x 240 | bottom centre | - | PASS | 9268 | 1024 | r11 | torn wing: airfoil with skin thickness, ribs and I-beam spars at the torn root and a skin breach, hanging aileron, dark leading edge, tip nav pod |

The jammer's footprint is its 500 x 500 base; 1400 x 1600 is the span of the crossbars. The debris chunk's largest dimension is 150 cm (footprint_check verifies it).

## Notes for gameplay

- **Outpost wall** tiles end to end along Y every 15 m (panel seams, buttresses at a 3.75 m rhythm and the cap
  merlons continue across the joint). The **inner face is -X** (flush on the box); the **outer face is +X**: its
  buttresses and merlon blades reach the box, the panels between the buttresses sit 50 cm inside it.
- **Doorway** (`SM_Cind_OutpostWall_Doorway`) is the frame that fills the 6 m gap between two wall segments: posts
  0.8 m wide whose outer faces (Y = +-300) meet the neighbouring walls flush; opening 440 cm wide x 500 cm tall,
  centred, floor at 0; lintel built in, top at 600. `SM_Door` (30 x 620 x 600) is the sliding panel.
- **Jammer tower** is one rigid mesh (sink / tilt it as a whole). Base 5 x 5 m; crossbars 16 m (Y) and 14 m (X);
  emitter core centred at about Z 4000.
- **Relay platforms, plateau, node stub, stepping stone, collapse slab, trailhead plinth**: walkable tops flat at the
  table height. Platform sides are flush masonry (block relief 0-2 cm, medallions and glyph strokes within the box).
- **Wreck pieces** are cover: their bounds equal the boxes but the surfaces are organic, so the player can stop up
  to the side gaps listed in `review/footprint_check.json` before touching the visible hull.

## Files

```
PlanetKit/
  brief.md, README.md
  scripts/build_planet_kit.py     build driver (one fresh scene per asset; --nobake for clay rounds)
  scripts/pk_lib.py               shared pipeline (copy of SpaceDressing sd_lib.py: parts, role materials, UVs, bake)
  scripts/pk_ext.py               PlanetKit additions: stone / basalt / bark materials, footprint fit, eye-height
                                  review renders, detail UV, finish (no collision)
  scripts/pk_geo.py               geometry helpers
  scripts/pk_cinder.py            Null Armada outpost, jammer, crates, door, basalt pieces
  scripts/pk_verdance.py          relay stone, log, wing / fin
  scripts/pk_wreck.py             wreck hull and engine (round-3 rebuild; superseded by pk_r9)
  scripts/pk_r9.py                round 9+ rebuild: wreck hull / wing / fin / engine, log, collapse slab, debris;
                                  new relay altar and prototype console (registered last, so these builders win)
  scripts/pk_small.py             console, extraction core, route post, trailhead
  scripts/footprint_check.py      measures every mesh against the footprint table (PASS / FAIL)
  scripts/make_detail_normal.py   the tileable detail normal
  scripts/ue_import_planet_kit.py Unreal import (headless)
  textures/T_PK_DetailStone_N.png
  SM_<name>/                      .blend, .fbx, .manifest.json, T_<name>_BC/_N/_ORM/_E.png
  review/                         <asset>_r<N>_review.png and _clay_sheet.png per round, <asset>_health.json,
                                  footprint_check.json
```

Rebuild: `blender --background --factory-startup --python scripts/build_planet_kit.py -- --assets all --tag rN`,
then `blender --background --factory-startup --python scripts/footprint_check.py`, then
`powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\PlanetKit\scripts\ue_import_planet_kit.py`.

## Review and honest gaps

- Process: clay / procedural rounds r1-r4 (`--nobake`), baked rounds r5-r8; every sheet kept in `review/`.
  One independent reviewer (fresh agent, rubric `aaa-quality-bar.md`) scored the r5 sheets: kit average about 2.8,
  no asset at the AAA ship threshold (4 in every category). Its top fixes applied in r6-r8: jammer crown rebuilt
  (raked blade arms, caged 2.8 m emitter, mid-shaft collar, base hatch and conduits); wing / fin tears re-cut as
  jagged edges; relay stone re-laid as large ashlar with medallions and pilasters and moss kept off the walkable
  tops; Null armour albedo lifted, crimson stronger, blade crests on the wall cap, layered plates on the doorway
  posts; wreck hull sunk deeper with a soil skirt, thicker vines and glowing mushroom clusters; crystal clusters
  enlarged; amber crack web thinned.
- Not re-scored after the fixes. Known weak spots: the wreck hull still reads as a tube more than a ship (hard
  edged moss patches); the wing / fin are thin single panels; the log and small interactables got no second round;
  the glyph band is the same stroke vocabulary on every face; the debris chunk is ~2 m, not 1.5 m.
- `SM_Verd_Wreck_Fin`: the dead nav-light lens sits 1 cm off the panel (assembly_check). All other meshes: no
  floating parts, asset_report 0 errors.
- Small items (console, core, route post, trailhead, slab) are 900-1,600 triangles, under the 2k floor of the
  brief's small-piece budget; nothing was added just to reach it.
- Not verified: the master material's shaders (compiled headless without a renderer; they compile on first editor
  load), the look under Unreal lighting, Nanite fallback quality, and anything in game. Gameplay still spawns its
  basic shapes; swapping them in is gameplay's change.

## Rounds 9-17 (Nov 13): the weakest pieces rebuilt, two new pieces

- `scripts/pk_r9.py` rebuilds the wreck hull (crashed lifting-body dropship: chines, ogive nose ploughed into a soil
  mound, cockpit canopy as the focal point, dorsal spine, torn tail-fin root, stub wings with spars at the torn tips,
  stern torn open with peeled plates, ribs, bulkhead and hanging conduits), wing and fin (real airfoils with skin
  thickness, ribs, I-beam spars, hanging control surfaces), engine (pitched, 1.2 m deep, sheared exhaust end), log,
  collapse slab (Voronoi basalt columns) and debris (fused column shards). Tears follow smooth curves; moss is a lifted
  sheet whose zero crossing gives a soft outline and creeps up from the soil; soil berms are continuous mounds;
  livery and panels are conforming shells; skins are solidified inward (`solidify_in`).
- `SM_Door`: the right half's outline was self-crossing (bow-tie face: a pale twisted sliver and see-through holes
  near the rails). Fixed in `pk_cinder.build_door`; a ray probe finds 0 see-through points (was 3,160).
- New: `SM_Verd_RelayAltar` and `SM_Cind_PrototypeConsole` (they replace a stretched `SM_Console`).
- Known gameplay trade-off: the hull's visible flanks sit about 0.8-1.4 m inside the 9 m box at chest height
  (r7 tube: about 0.45 m); the log fills its box to about 1.15 m with the root flare to 1.4 m.
- Independent review (fresh reviewer, rubric `aaa-quality-bar.md`): no piece reached 4 in every category; averages
  in the integration report. Sheets: `review/*_r9*` to `*_r17*` (r9c/r9d/r9e/r11c are procedural clay rounds).
