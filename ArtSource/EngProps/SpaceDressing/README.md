# SpaceDressing kit (engineering-made space props)

20 Nanite static meshes to dress the galaxy level: asteroid belts, debris fields, derelicts near planets,
cargo drifting on shipping lanes, nav buoys and the portal gate frame. Built in Blender 5.2 headless by
`scripts/build_space_dressing.py` (seeded, reproducible), imported into Unreal 5.9 by
`scripts/ue_import_space_dressing.py`. Brief: `brief.md`. Review sheets and per-asset health reports: `review/`.

**Unreal:** `/Game/TriadDrift/Gameplay/Props/SpaceDressing/` - `SM_<name>` + `MI_<name>` per asset, textures in
`Textures/`, master material `Materials/M_TD_SpaceDressing`. All meshes are real scale (1 uu = 1 cm), pivot at the
centre of mass, Nanite on, one material slot, simple collision from authored convex hulls (`UCX_`).
Safe for instanced static meshes / foliage / PCG scattering (the master is flagged for instancing and Nanite).

Sizes below are the Unreal bounding box X x Y x Z in metres. Random rotation and 0.7 to 1.4 uniform scale are fine
for every rock and debris piece; keep satellites, pods and the buoy at 0.8 to 1.2 so their known-size details
(hatches, latches, hazard stripes) still read at the right scale.

## Asteroids (scatter in belts and fields)

| Asset | Size (m) | Tris | Place it | Description |
|---|---|---|---|---|
| `SM_Ast_Ice_01` | 47 x 72 x 42 | 13,000 | outer belt, near the cold side of a planet, hero rock of an ice field | Big faceted ice slab with a polygonal fracture network, glacial-blue facets, frost sparkle, faint glow in the deepest cracks |
| `SM_Ast_Ice_02` | 36 x 46 x 30 | 11,000 | ice fields, mid size | Elongated ice rock split by a deep chasm |
| `SM_Ast_Ice_03` | 16 x 26 x 13 | 6,500 | ice fields, filler and clusters | Angular ice shard with many fracture facets |
| `SM_Ast_Metal_01` | 46 x 55 x 39 | 12,000 | combat-zone belt, cover rocks | Dense metallic rock: hammered thumbprint dents, raised gold veins, rust in the creases |
| `SM_Ast_Metal_02` | 16 x 34 x 15 | 9,000 | belts, mid size | Long faceted iron splinter with gold veins |
| `SM_Ast_Metal_03` | 15 x 17 x 12 | 5,000 | belts, filler | Compact iron chunk |
| `SM_Ast_Crystal_01` | 46 x 44 x 43 | 10,160 | near Cinder, around gates, landmark rock | Dark slate rock with four large violet crystal clusters erupting from pits, glowing roots |
| `SM_Ast_Crystal_02` | 25 x 28 x 24 | 6,984 | near Verdance, mixed into belts as accents | Slate rock with three teal crystal clusters |

## Wrecks and traffic (debris fields, shipping lanes, near planets and stations)

| Asset | Size (m) | Tris | Place it | Description |
|---|---|---|---|---|
| `SM_Derelict_Satellite_01` | 2.5 x 15.2 x 7.6 | 5,738 | high orbit around a planet, alone or in pairs | Octagonal bus (ivory, gold foil), tilted dish, whip antenna, a micrometeoroid puncture; right wing intact, left wing bent and torn. Blinking warm-white light |
| `SM_Derelict_Satellite_02` | 5.0 x 8.3 x 2.9 | 4,944 | orbit, debris fields | Faded violet cylindrical probe, front dish, engine bell, one boom with a snapped hanging panel, snapped stub. Blinking teal light |
| `SM_CargoPod_01` | 8.4 x 3.3 x 3.4 | 4,596 | shipping lanes, around stations, spilled-cargo fields (scatter 3-12) | Gunmetal container: corrugated sides, gold end frames, corner castings, door locking bars, gold/teal hazard bands |
| `SM_CargoPod_02` | 6.9 x 2.9 x 2.8 | 2,992 | lanes, near gates | Ivory capsule pod with violet rings, hazard waist band, skids, hatch, docking ring, thrusters |
| `SM_CargoPod_03` | 4.6 x 3.3 x 3.0 | 2,936 | lanes, stacked clusters | Violet hexagonal crate in a gunmetal frame cage, hazard end caps, latches |
| `SM_BeaconBuoy_01` | 3.1 x 3.1 x 5.1 | 4,184 | lane markers, either side of a gate approach, planet approach corridors (place in lines 200-400 m apart) | Ivory spindle with gold band, armoured halo ring with a teal glow strip, solar petals, fins, warm-white tip. Pulses slowly |
| `SM_StationTruss_01` | 41 x 8.3 x 7.5 | 7,562 | debris fields, near the Dominion approach, wreck sites | 40 m square girder: docking flange at one end, pipe run on clamps, hull cladding, torn end with kinked longerons and spilled cables |
| `SM_StationTruss_02` | 15 x 23 x 12 | 7,670 | debris fields, wreck sites | Curved hull section torn from a station: ribs and stringers inside, a window row (two lit), ribs sticking past the torn edge |
| `SM_StationTruss_03` | 27 x 42 x 12 | 7,108 | landmark wreck, centre of a debris field | Hub module with three girders torn off at different lengths and a bent radiator panel |
| `SM_DebrisCluster_01` | 14 x 16 x 9 | 4,006 | scatter through debris fields | Tangle of two bent torn hull plates, a kinked I-beam, pipes and clamps |
| `SM_DebrisCluster_02` | 23 x 28 x 19 | 6,032 | debris fields, larger accents | Girder chunk, a big bent plate, crossing I-beams, a jammed tank and curling pipes |

## Portal gate frame

| Asset | Size (m) | Tris | Description |
|---|---|---|---|
| `SM_GateRing_01` | 9.5 x 158 x 158 | 24,608 | Ornate gate frame: gunmetal spine, ivory armour plates, gold rails and keystones, 8 node housings with teal emitter lenses (facing the centre) and front/back glow slots, crown blades |

- **Ring axis is Unreal X**: ships fly through it along X. Pivot at the ring centre.
- **Inner diameter 120 m** (12,000 uu) measured at the glowing inner lip; the 8 emitter lenses reach 2.4 m further in
  (115 m clear between lens tips). Frame radial depth about 10.4 m (12 m at the nodes, crown blades to 19 m).
- **Scaling:** uniform scale `s` gives an inner diameter of `120 * s` m (e.g. 0.5 for a 60 m gate). Scale uniformly only.
- The swirling portal surface is VFX and **not** part of this mesh; centre it at the pivot, radius 6,000 uu x s.
- Collision: 16 convex hulls around the frame (the opening is clear).

## Materials and parameters

`M_TD_SpaceDressing` (lit, opaque): `BaseColorTex`, `ORMTex` (R AO, G roughness, B metallic), `NormalTex` (DirectX),
`EmissiveTex` x `EmissiveStrength` x a blink factor (`BlinkRate` Hz, `BlinkAmount` 0..1; 0 = steady).
Each `MI_<name>` sets its textures, `EmissiveStrength` = bake strength 2 x 5 = 10 where the asset glows, and blink only on
the two satellites and the buoy (their whole emissive is the beacon light). Tune `EmissiveStrength` per instance or
per MI once seen in the real level; it has not been checked in a rendered Unreal view yet.

## Rebuild

```
blender --background --factory-startup --python ArtSource/EngProps/SpaceDressing/scripts/build_space_dressing.py -- --asset SM_GateRing_01 --tag r4
powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\SpaceDressing\scripts\ue_import_space_dressing.py
```
Scripts: `sd_lib.py` (parts, role materials, UVs, bake, review, export), `sd_asteroids.py`, `sd_hard.py`. The repo's
`.claude/skills/blender-*` helper scripts are used unchanged. Status: technically verified in Blender and imported
headless; not yet seen in a rendered Unreal view, not placed in a level, not approved by Nicholas.
