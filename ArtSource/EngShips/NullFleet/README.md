# Null Fleet: the Null Armada's enemy ships

Engineering-made (route C: code-built from a written design, Blender 5.2 headless) enemy spaceships for
Triad Drift, replacing the basic-shape placeholders of `ATDEnemyShip`. Same faction language as the
NullTrooper foot soldiers: matte black gunmetal plates with recessed panel breaks and worn bevelled edges,
red-orange emissive seams and pinstripes, glowing white eyes, angular and insectile, heavier at the front.
No squad colours anywhere. Nicholas keeps art direction: he can restyle or replace any of this, and a
`DA_Enemy_*_Visuals` in `/Game/TriadDrift/Presentation/` always wins over the ones here.

## Files

| Path | What |
|---|---|
| `scripts/nf_ships.py` | the designs: one builder per ship + the carrier's bay and turret |
| `scripts/nf_lib.py` | pipeline (copied from `EngProps/SpaceDressing/scripts/sd_lib.py`, adapted): role materials, UVs, bake, checks, review sheets, export |
| `scripts/nf_build.py` | builds one asset headless: `blender --background --factory-startup --python nf_build.py -- --asset SM_Null_Fighter --tag r3 [--stage blockout]` |
| `scripts/ue_import_null_fleet.py` | Unreal import: `powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\NullFleet\scripts\ue_import_null_fleet.py` |
| `SM_Null_*/` | per asset: `.blend`, `.fbx` (+ UCX hulls and SOCKET_ empties), `.manifest.json`, `T_Null_*_BC/N/ORM/E.png` |
| `review/` | `<asset>_<tag>_clay_sheet.png`, `<asset>_<tag>_review.png` (material views + true game-scale strip), `<asset>_health.json`; tags `b1`-`b3` blockouts, `r1`-`r3` full rounds |

Everything is seeded: a re-run reproduces the asset.

## Conventions (all ships)

- Units cm in Unreal (metres in Blender, 1 m = 100 uu); **+X forward** in Unreal (authored nose toward
  Blender -Y; the exporter rotates it), +Z up; object scale 1.
- One material slot per mesh, one baked texture set (BC sRGB, N DirectX, ORM masks, E emissive). In Unreal
  `MI_Null_<ship>` of `M_TD_NullFleet`; `EmissiveStrength` per MI (20-30, see the engine glow pass below); eyes, seams and engine
  cores all live in the one E map, so one scalar drives the whole ship's glow.
- Collision: 2-6 UCX convex hulls per mesh (gameplay uses its own sphere / box; these are for traces only).
- Sockets: `Muzzle_0..n` are picked up automatically by `UTDWeaponComponent` (it fires from mesh sockets
  named `Muzzle_0..7` when they exist, alternating if the weapon alternates), `Engine_n` mark nozzle exits for
  FX (unused by code today), `Launch` on the bay marks the door centre.
- Size: matched to the gameplay collision radius in `TDEnemyLibrary.cpp`; the art stays within 1.25 x the
  collision diameter. Positions below are Unreal cm relative to the actor origin.

## Summary

| Asset | Gameplay size | Art size X x Y x Z (cm) | Tris | Texture | Pivot | Final sheet |
|---|---|---|---|---|---|---|
| `SM_Null_Fighter` | sphere r 220 | 549 x 468 x 163 | 7,282 | 2048 | bbox centre (pinned) | `review/SM_Null_Fighter_e2_review.png` |
| `SM_Null_Interceptor` | sphere r 260 | 661 x 443 x 206 | 6,986 | 2048 | bbox centre (pinned) | `review/SM_Null_Interceptor_e2_review.png` |
| `SM_Null_Gunship` | sphere r 700 | 1524 x 1709 x 677 | 17,586 | 2048 | centre of mass (pinned) | `review/SM_Null_Gunship_e2_review.png` |
| `SM_Null_Bomber` | sphere r 500 | 1177 x 1222 x 257 | 11,800 | 2048 | bbox centre (pinned) | `review/SM_Null_Bomber_e2_review.png` |
| `SM_Null_Carrier` | sphere r 1800 | 4643 x 3956 x 2506 | 33,706 | 4096 | design origin = actor origin | `review/SM_Null_Carrier_e1_review.png` |
| `SM_Null_CarrierBay` | box 300 x 600 x 400 | 356 x 649 x 465 | 4,772 | 1024 | box centre | `review/SM_Null_CarrierBay_e1_review.png` |
| `SM_Null_Turret` | cylinder 150 x 100 | 221 x 143 x 102 | 2,184 | 1024 | cylinder centre | `review/SM_Null_Turret_e2_review.png` |

Pivot: the brief asked for the centre of mass. For the fighter, interceptor and bomber the volume centre of mass
sits 15-150 cm off the bounding-box centre (engine mass at the back), which pushed the nose past 1.25 x the
collision radius, so those three pivot on the bounding-box centre (the collision sphere's centre). The gunship
pivots on its centre of mass. The carrier, bay and turret keep their design origin because the code mounts the
bays and turrets at fixed offsets from it.

## Per ship (Unreal cm, actor-relative, +X forward, +Y right, +Z up)

### Shard Fighter `SM_Null_Fighter` (DA_Enemy_Fighter_Visuals)
Dagger fuselage with a broad angular head, forward-swept crescent blade wings whose tips reach past the
nose, canted black V fins, a keel blade, forward mandible blades. One white chevron eye wrapped across the
head's raked face (+ a small chin eye for passes from below). Twin red-orange nozzles. Red-orange leading-edge
strips, chine seams, a hex glow on the spine.
- Muzzles: `Muzzle_0` (194, -60, -28), `Muzzle_1` (194, 60, -28): wing-root cannon tips.
- Engines: `Engine_0/1` (-262, -/+40, -20).
- Note: `ATDSpacecraft::EngineGlow` (a white cube at (-215, 0, 0), 35 x 90 x 35 cm) stays visible with real
  art; on the fighter it sits between the nozzles. On bigger ships it is buried inside the hull.

### Hook Interceptor `SM_Null_Interceptor` (DA_Enemy_Interceptor_Visuals)
Long hull with a deep hooked beak that overhangs a set-back jaw, a brow crest, mandible canards, swept wings
with separate down-turned claw tips, two missile pods under the wings (three red-tipped tubes each), twin
raked tail blades. Three white eyes: a lens on the brow, two slits on the head flanks.
- Muzzles: `Muzzle_0/1` chin guns (241, -/+20, -10); `Muzzle_2/3` missile pod mouths (111, -/+112, -16).
- Engines: `Engine_0/1` (-319, -/+24, 15).

### Wall Gunship `SM_Null_Gunship` (DA_Enemy_Gunship_Visuals)
A chevron shield wall of three stacked armour bands (forward-raked horns, chin blade) with two recessed
red-orange seams between the bands and a dark visor carrying four big white eye slits; it is the frontal
shield in gameplay (the placeholder shield disc is destroyed when art is set). Low armoured body behind,
cannon blisters outboard of the wall with twin barrels each, a dorsal three-hatch missile launcher, four
engines.
- Muzzles: `Muzzle_0..3` blister barrel tips (578, -/+730, 27 / -33).
- Engines: `Engine_0..3` (-852, -/+135 and -/+370, -28).

### Maul Bomber `SM_Null_Bomber` (DA_Enemy_Bomber_Visuals)
Thick manta flying wing with down-turned claw tips, armoured wing-root spines, big hooked nose mandibles
with a white eye pair above them and two eye slits on the nose, trailing-edge teeth, two heavy nacelles
sunk into the wing plus a central nozzle. Belly mine bay: parted doors with a glowing slot and three
hanging mines with red cores.
- Muzzle: `Muzzle_0` (-161, 0, -93): centre of the mine bay (mines drop from here).
- Engines: `Engine_0/1` (-546, -/+205, 7), `Engine_2` (-491, 0, 42).

### Hive Carrier `SM_Null_Carrier` (DA_Enemy_Carrier_Visuals, + BayMesh, TurretMesh)
Segmented insect hull of overlapping carapace shells, a plated head under a heavy brow with six white eye
slits in two rows, stag-beetle mandibles, two cranked arms ending in bay collars, a waisted, raked, ribbed
dorsal sail with trailing spines carrying the dorsal bay, four horn pylons ending in turret pads, five
engines. Nanite. Mount points (from `SpawnCarrierParts` / `MountPart`: radius 1800 + half extent + 150):
- Bay centres: port / starboard (0, -/+2100, 0), dorsal (-603, 0, 2011) (direction (-0.3, 0, 1)). The arm
  collars end at |y| = 1970, 20 cm into the bay's inner face (1950); the sail's top face is tilted to the
  dorsal bay's direction.
- Turret centres: (+/-1255, +/-1255, 1065); the pads' tops are at z 1015, under the turret's base.
- Engines: `Engine_0..3` (-2240, -/+240, 140 / -130).
- No muzzle sockets (the carrier hull does not fire; its turrets do).

### Carrier bay `SM_Null_CarrierBay` (BayMesh)
Matches the 300 (deep, +X outward) x 600 x 400 cm hit box; pivot at its centre. Wedge-shaped armoured pod with
chitin ribs, a heavy lip, recessed angled doors with glowing seams and a bright launch slot (the weak point),
clamp arms that bite into the carrier's collar (they reach 50 cm behind the box). `Launch` socket (160, 0, 0).

### Point-defence turret `SM_Null_Turret` (TurretMesh)
Matches the 150 x 100 cm cylinder (+Z up, centred); +X = barrels (the turret actor rotates to aim). Hex
armoured base ring with a red seam, a faceted mantlet with a glacis and a brow over a white sensor eye, twin
barrels with hot muzzle rings. `Muzzle_0/1` (147, -/+22, 10).

## Process

Blockouts `b1`-`b5` (clay), full rounds `r1`-`r5` (bake, review). Fresh-eyes reviews by separate agents after
r1 and r2 (scores and fixes in the dev report); r3+ applied those fixes. Each full round writes
`asset_report.py` and `assembly_check.py` results into `review/<asset>_health.json` (final: 0 errors,
0 warnings, no floating parts on all seven). Emissive surface share after e2: Fighter 5.9 %, Interceptor 5.6 %,
Gunship 5.5 %, Bomber 6.0 %, Carrier 3.3 %, Turret 6.7 %, Bay 6.4 % (its glowing doors are the weak point).

## Engine glow pass (e1/e2, after the Lead's in-game check)

The in-game check found no visible engine glow at range. Changes:
- Every nozzle now carries an **exhaust plume stub**: a hot red-orange emissive cone that starts inside the bell
  and stands 1.2 x the nozzle radius out of it, so the engine reads from the side and the front quarters as
  well as from behind. The separate core disc inside the bell was removed (the plume fills it), plus the
  glowing lip ring at the exit.
- The E map now has **two levels**: seams and pinstripes at half level, engines (hotter colour `#FF7A30`) and
  white eyes at full level. Pinstripes are thinner (`GLOW_K` 1.6 -> 1.0) to keep the emissive area down.
- **EmissiveStrength per MI** (bake strength 2 x scale): Fighter 30, Interceptor 30, Bomber 26, Gunship 24,
  Carrier 22, Turret 24, Bay 20 (was 12 for all). Seams therefore stay about as bright as before; engines and
  eyes are 2-2.5x brighter.
- Pivots are pinned to the earlier values (`PINNED_PIVOT`), so all sockets are unchanged. The plumes make each
  ship a little longer at the back: fighter +26 cm, carrier +154 cm (46.4 m, 3 % over 1.25 x its collision
  diameter). Thinner pinstripes no longer touch the hull everywhere; the pipeline's snap moved a few of them
  5-20 cm onto the surface (no mount or socket moved).
- Range strips: `review/<asset>_e0_range300.png` (before) and `_e2_range300.png` (after). These show the ship
  at 2.5x (the gameplay range scale), 300 m, true pixel density, enlarged 4x. The three views are chase, side and front 3/4.

## Not verified / known limits

- **Not seen in Unreal.** The import ran headless (no renderer): materials are not compiled there, nothing has
  been looked at in a viewport, PIE or a packaged build, with bloom, at speed or in a wave. The per-MI
  EmissiveStrength values (20-30) are judged from Blender renders; tune them on the `MI_Null_*` instances.
- **Readability at range is limited by gameplay size.** At 1080p / 90 deg FOV a 5 m fighter is ~16 px long at
  300 m and ~5 px at 1 km whatever the art does; the review strips show this honestly. The placeholders were
  drawn 1.4-2.2x larger than their collision (`Placeholder*ShipScale`), but `UpdatePlaceholderLook` sets
  `HullPivot` scale to 1 when a Visuals mesh is set, so with this art the small ships get visually smaller than
  the placeholders were. If they read too small in play, the fix is in code (an art scale on the visuals or
  keeping a readability scale), not in these meshes.
- Muzzle sockets change where enemy shots spawn (from the default (150, +/-60, 0) to the barrels), and the
  gunship's are 5.8 m forward of its centre. Not play-tested.
- Review renders are Blender EEVEE (AgX dims the glow); final colour and glow need Nicholas's eye in Unreal.
