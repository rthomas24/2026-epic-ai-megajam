# World density: review and build plan

Written Nov 13, 2026, 16:30 EST, by a fresh reviewer who built none of this. Read from `lead/int` at `825d886`.
Nothing was launched: every number below comes from the code or from captures other agents had already taken
(listed in section 1.5). No capture in this document is mine. Nothing here has been built or tested yet.

Ryan's direction: the planets must look like real places with many materials, trees, rocks and buildings across
the **whole** map, not one small dressed area; space must be full everywhere (power-ups, debris, asteroids, moving
things, other ships) and keep generating as you fly.

**Scope change (Ryan, Nov 13, after the first draft of this plan).** The planets do not need a whole dressed map.
Ryan: "anywhere you fly into the world ... no matter what you get routed to that same objective place where we
have most of the terrain really nice and the objects ... so you don't have to create an actual entire map ...
disguise it in a really AAA nice way where a user can't really notice." So each planet gets **one richly dressed
play zone** around the pad, and the rest is disguised. Space is unchanged: full everywhere, generating as you fly.
The planet parts below are rewritten for that decision; the findings are unchanged because they describe today.
Also from Ryan, for the record and not planned here (the effects builder and T1 have it): both the entry and the
exit get an atmosphere-piercing sequence with motion blur, streaks and debris or sparks flying past.

Starting point: the environment builder (`.wt/env`, branch `eng/env`) is redoing terrain materials, palette, fog
and sky. This plan covers everything else and adds seven requirements for that builder (section 3.7).

---

## 0. One-page summary

**What is true today**

| Place | Size the player can reach | Area or volume with anything on it | Things in it |
|---|---|---|---|
| Verdance | 16 x 16 km of ground (25,600 hectares), plus a drawn-only ring out to 80 x 80 km | One box of 172 x 207 m around the pad: 3.6 hectares, **0.014 %** | At most about 960 instances, all inside that box. Outside it: zero |
| Cinder | Same 16 x 16 km | One box of 232 x 202 m: 4.7 hectares, **0.018 %** | At most 546 instances, all inside that box. Outside it: zero |
| Space | A sphere 9 km in radius (about 3,050 cubic km) | About 2.5 cubic km, **under 0.1 %**: four rock fields and thin tubes along the travel lanes | About 1,040 instances. None of them moves. 155 can be hit |

- Nothing is generated around the player anywhere. Every object is placed once at load from a fixed list.
- There is no pickup or power-up of any kind in space or on the ground. The only pickup class in the project is
  the unused template one (`Variant_Shooter/Weapons/ShooterPickup.*`).
- The only ambient traffic is two convoys (seven haulers) that exist only during the Verdance phase.
- The terrain's closest detail level is one polygon every 15.6 m, so even with good materials the ground will read
  as faceted up close unless scatter and a detail normal cover it.
- Arrivals land up to 6 km from the pad, on whatever heading the ship entered with, and the player may then fly
  to the edge of the 16 km map: there is no disguise today, only a turn-back warning 8 km out.

**What to build (six systems, about 36 engineer-hours, about 25 MB of package)**

Each planet: one dressed zone, a disc of **3 km radius** around the pad (2,830 hectares, 11 % of the core), fully
dressed in every direction; a taper to 4 km; a ring of mountains and haze beyond; arrivals pulled in to 2.5 km
and turned to face the zone; the flight zone closed at 5 km.

| # | System | Owner | Hours | Seen by the player as |
|---|---|---|---:|---|
| A | Planet scatter driven by the terrain field, inside the dressed zone only, streaming around the player | T4 (+ T5 for the course hand-off) | 6 + 2 | Forest, rocks, crystals and ground cover in every direction inside the zone |
| B | Eight hand-placed sites per planet, one on each of four approach corridors and four near the pad, with a light beam each | T4, T5, asset builder, Grok Bot | 3 | Ruins, survey sites, wreck fields, drill rigs under every approach and around the course |
| F | The edge: rim mountains, a prebuilt sparse far ring, arrival ring and heading, a tighter flight zone | T4 (terrain, far ring, arrival), T1 (flight zone) | 4 | No empty map is ever seen; four different ways in |
| C | Space field: a whole-galaxy layer built from a seed, plus a streaming ring of small debris around the ship | T1 | 7 | Belts, clusters, battle debris, wrecks, gas, buoy chains in every direction |
| D | Ambient traffic and one comet | T3 (T4 reviews) | 4 | Hauler columns on four lanes all game, a comet crossing the sky |
| E | Pickups in space (new gameplay): hull repair, shield cell, power cell, overcharge | T2 (+ T4 for kill drops) | 6 | Glowing pickups from kills and in debris fields; a HUD toast and a buff bar |
| | New meshes (grass, canopy clumps, boulders, charred trees, wreck hulls) | Asset builder | 4 | |

**Order.** Tonight: A on Verdance with the meshes we already have, F's arrival ring and flight zone, C's
whole-galaxy layer, E behind a switch, the small new meshes. Tomorrow morning: A on Cinder, F's rim and far ring,
B, C's streaming ring, D. Tomorrow evening: tuning, the frame-rate pass, Ryan's taste calls. Cut order if time
runs out: site rewards, the comet, gas clouds, field caches for pickups, sites down to four per planet, the far
ring's clumps (keep the haze and rim). Never cut: A's mid ring on both planets, F's arrival and flight-zone
numbers, and C's whole-galaxy layer.

**What the zone decision saves.** The ground that must be convincing drops from 256 to 28 square km, a factor
of nine. Trees that the generator can ever be asked for drop from about 1.5 million to about 170,000. The far ring
stops streaming and is built once. Sites drop from 30 placed by rule to 8 placed by hand, which can be art
directed. Hours on the planets stay about the same (16 before, 15 now) because the disguise work replaces the
coverage work; what is bought is quality and certainty, not time.

**The three rules that protect the game**

1. Nothing new that can be hit is placed within 1.5 km of the combat zone centre, in a travel lane's 150 m core, or
   near a gate, return point, planet or the Dominion. The dogfight pocket stays exactly as it is today.
2. Everything small is visual only. On planets nothing in the scatter blocks a ship.
3. Every new system has a console switch (`td.Scatter`, `td.SpaceField`, `td.Traffic`, `td.Pickups`) so a bad one
   can be turned off in the submitted build without a code change.

---

## 1. Findings

### 1.1 Verdance and Cinder: how the ground is made

- `ATDPlanetTerrain` builds 256 chunks of 1 km each at spawn: `ChunkSize = 100000` and `QuadsLod0 = 64`
  (`World/TDPlanetTerrain.h:43-44`), `CoreHalfSize = 800000`, `FarHalfSize = 4000000`
  (`World/TDTerrainField.h:39-40`). So: a 16 x 16 km core with collision, and a ring to 80 x 80 km with no collision
  (`TDPlanetTerrain.cpp:124-168`).
- Detail: 64 quads per kilometre at the nearest level = **15.6 m per polygon**; 42 m at the middle level; 125 m at
  the far level; 1 km in the outer ring. LOD0 only applies within 1.8 km (`Lod0Distance`).
- The height field is good and varied: ridged mountains, a hero peak or volcano, four named regions by bearing
  (canyon, mesas, lake or lava sea, plateau), river beds on Cinder (`TDTerrainField.h:9-70`). It already returns
  slope, height and two masks at any point and is safe to call from worker threads. **This is the right base for
  scatter; nothing in the terrain needs rewriting.**
- One material covers everything. There is one sea disc and one crater disc, both tinted engine cylinders
  (`TDPlanetTerrain.cpp:107-122`).
- Likely bug (not verified in a running game): the outer ring is given the base material, not the per-planet
  dynamic instance that carries `HeightRangeM` (`TDPlanetTerrain.cpp:167` uses `Material`; the chunks use
  `TerrainMaterial`, line 300). That would make the ring's height bands wrong, and may be why the far ranges in the
  captures are flat white. One-line fix for T4; tell the environment builder.

### 1.2 Verdance and Cinder: what is on the ground

All dressing is placed by the objective course, `ATDPlanetCourse::ScatterMeshes`
(`Ground/Planets/TDPlanetCourse.cpp:332-372`), and every point must pass `IsOnGround`, which is the course's
ground box (`TDPlanetCourse.h:229-232`).

| | Verdance | Cinder |
|---|---|---|
| Ground box | x -31..141 m, y -106..101 m (`TDPlanetCourse.cpp:744`) = 172 x 207 m = 3.56 ha | x -31..201 m, y -106..96 m (`:974`) = 232 x 202 m = 4.69 ha |
| Share of the 25,600 ha core | 0.014 % | 0.018 % |
| Requested instances | 161 trees, 110 mushrooms, 260 ferns, 320 glow plants, 108 rocks = 959 (`:940-960`), plus route markers | 176 crystals, 160 basalt columns, 200 basalt rocks = 536 (`:1290-1305`), plus set-piece blocks |
| Buildings | One station deck with posts, roof and racks; one ruin plateau with pillars; the pad | One outpost compound, one jammer tower, the pad |
| Anything beyond the box | Nothing | Nothing |

- **The "canopy ring" is mostly thrown away.** The Kapok giants ask for a radius of 170 to 240 m from the middle of
  the course, and the Brazil nut trees 150 to 240 m (`:942-943`), but the farthest corner of the ground box is
  about 134 m from that middle. Every one of those 28 trees fails `IsOnGround` and is dropped; most of the 30
  trumpet trees (120 to 220 m) are dropped too. This is arithmetic from the code, not a count in a running game, but
  the on-foot capture agrees: the trees stand close in and thin. T5 fixes this in task 2.
- Scatter is placed at z = 0, the flat course floor. That is only correct inside the flattened basin, which is the
  second reason it cannot simply be given a bigger radius.
- Trees and plants have no collision. 48 large rocks on Verdance and 190 basalt pieces on Cinder block the pilot.
- T4's own plan row says biome scatter is still to do (`features.md:265`, `:89`).

**What the player sees, 5 km in each direction (Verdance).** The pad sits at the centre. At the atmosphere speed
cap (36 m/s cruise, about 65 m/s with boost, `Flight/TDFlightSim.h:85`, `features.md:1127`) 5 km is 80 to 140
seconds of flight.
- East (toward the massif): bare slopes, then a bare mountain 2.5 km high.
- North-east (plateau), south (canyon), west (mesas), north-west (lake): each region has a different shape and
  exactly the same nothing on it: no tree, rock, building, light or moving thing.
- Past 8 km the collision stops and the flight zone turns the ship back; the capture at 4 km altitude already
  shows "LEAVING THE FLIGHT AREA".
- On foot, walking 150 m in any direction leaves the last plant behind.

**Where arrivals land and how far the player can go today.**
- An entry anywhere on the globe maps to a ground point at most 6 km from the pad
  (`World/TDPlanetEntry.h:34`, `MaxArrivalRadius = 600000`; entries more than 60 degrees from the landing zone
  clamp to that ring, `TDPlanetEntry.cpp:22-27`), 1,000 m above the ground (`Travel/TDDescentSubsystem.h:92`).
- The arrival heading is the ship's entry heading laid onto the ground, **not** a heading toward the pad. Only a
  near-vertical dive is turned to face the pad (`TDPlanetEntry.cpp:73-80`). A ship that skims in sideways arrives
  6 km out looking along or away from the zone.
- The flight zone is the whole collision core plus 500 m: a box 8.5 km to each side of the pad, with the warning
  500 m inside its edge (`Travel/TDAtmosphereZones.cpp:416`, `TerrainEdgeExtension`,
  `Flight/TDFlightTypes.h:261`). Its ceiling is 400 m above the highest ground in the core
  (`TDAtmosphereZones.h:204`), so about 2.9 km.
- So today a player can arrive 6 km from the only dressed 4 hectares, facing away from them, and fly 8 km of bare
  ground in any direction before anything turns them back.

**Rated against "a real place": 1 out of 5.** The landform is a 3; what is on it is a 0 outside one small garden.

### 1.3 Space

Everything is placed once by `ATDGalaxyLayout` from `MakeDefaultLayout()` (`World/TDGalaxyLayout.cpp:196-298`).

| What | Where | Count | Can be hit | Moves |
|---|---|---:|---|---|
| Combat belt, with a clear pocket 140 m in radius | Origin, 420 m radius (`:208`) | 60 rocks | Yes | No |
| Flank field | 0.67 km from origin (`:211`) | 35 | Yes | No |
| Verdance approach field | 2.6 km out (`:213`) | 30 | Yes | No |
| Cinder approach field | 2.7 km out (`:217`) | 30 | Yes | No |
| Small debris in those four fields | 0.8 per rock (`TDGalaxyLayout.h:321`) | about 124 | No | No |
| Station ruins around the pocket | Ring at 220 m (`:221-231`) | 6, plus about 15 debris clusters | Ruins yes | No |
| Lane rocks, 3 to 25 m across | A hollow tube 90 to 310 m from each of 9 lane lines (`:771-801`) | 70 per lane = 630 | No | No |
| Cargo pods | Two clusters on each of 3 lanes (`:747-768`) | about 36 | No | No |
| Satellites | Two per planet | 4 | No | No |
| Buoys | Beside the gates | up to 72 | No | No |
| Convoys | Two lanes near Verdance, Verdance phase only (`World/TDVerdanceApproach.cpp:21`, `TDGalaxyLayout.cpp:483-485`) | 7 haulers | No | Yes, 30 m/s |

- Play space: a sphere of radius 9 km centred 500 m south of the origin (`:201-202`). Planets sit at 5.2 and
  5.4 km, the Dominion at 5.2 km. Cruise is 60 m/s, boost 108 m/s, hyperdrive 800 m/s
  (`Flight/TDFlightTypes.h:26,30`, `Travel/TDHyperdriveComponent.h:62`).
- The dressing components are `Static` and have no rotation or drift (`TDGalaxyLayout.cpp:92`). Rocks disappear
  between 2.5 and 3.2 km (`:109`).
- Lane density: 70 rocks in a tube of about 0.7 cubic km is one small rock per ten million cubic metres. A 6 m rock
  200 m off the lane line is about 30 pixels wide at 1080p. That is why the lane captures look empty.
- **5 km in each direction from the start:** up or down, nothing after 210 m (the belt is flattened). West or east,
  nothing after 450 m. North, two gates at 350 m then nothing. South, the flank field at 0.7 km, then a thin tube of
  pebbles to the Dominion. Only three bearings (the two planets and the Dominion) have anything at all, and what
  they have is the tube.
- Nothing is generated as the player flies. No pickups. No patrols. No comet, cloud or wreck.

**Rated against "full": 2 out of 5 inside the combat zone, 0 to 1 everywhere else.** The combat zone itself looks
good and busy; it is about 0.01 % of the space.

### 1.4 Size and frame-rate facts the plan must respect

- Package: 760 MB of a 1,000 MB limit after the trims (Lead's figure; `docs/eng/BUILD-SIZE.md` has the method).
  Textures are 56 % of the content. A 2K texture costs about 1.6 MB cooked, a 1K about 0.45 MB (same file,
  "Textures by source size"). **Rule for this plan: no new 2K textures. New meshes reuse a material we already ship
  or get one 1K set.**
- A terrain chunk takes 180 to 440 ms to generate on a worker and about 4 ms to upload (Lead's figure and
  `TDPlanetTerrain.h:176`). The terrain allows itself 1.5 ms a frame on the game thread (`BudgetMs`). Scatter must
  fit beside that, not on top of it.
- The belt was measured as too small for hierarchical instancing to pay off (`TDGalaxyLayout.cpp:86-87`). That stops
  being true at the counts below; see each system.

### 1.5 Captures looked at (all taken by other agents)

| Capture | Shows |
|---|---|
| `.wt/t4/Saved/TDShots/mcp/fe1/mcp_01_43832.png` | Verdance from 1,145 m, 2.5 km from the pad: bare knitted-looking green and brown facets to the horizon, white far ranges, not one object on the ground |
| `.wt/t4/Saved/TDShots/mcp/fe5/mcp_01_13876.png` | Verdance from 4,057 m: bare ground, flat blue-white disc for the lake, "LEAVING THE FLIGHT AREA" |
| `.wt/t4/Saved/TDShots/mcp/m3_V_foot_sheet.png` | On foot at the pad: the dressed garden; thin trees, mushrooms, the station; bare ground and fog behind |
| `.wt/t1/Builds/descent/descent_two_directions.png` | Two arrivals at different places on the globe: both come down over empty white and grey terrain |
| `.wt/t4/Saved/TDShots/mcp/aq1/mcp_01_41768.png`, `ap2/mcp_01_35440.png` | The lane to Verdance, 7.2 to 7.4 km to go: nebula, three squad ships, one convoy dot, nothing else |
| `.wt/t4/Saved/TDShots/mcp/dress_after_belt/mcp_01_30060.png` | The combat belt: rocks, trusses, gates, a planet. This is the one full view in the game |
| `.wt/t4/Saved/TDShots/mcp/gv1/mcp_01_34256.png` | Verdance globe from 3.5 km: flat teal patches on near-black; no orbit dressing |

These were enough to judge coverage, so I did not start a game session.

---

## 2. The target

"Full" for this jam means: **from any point the player can reach, at any height, there is something within sight
that is not terrain or sky, and it changes as they travel.** Numbers below are instances per hectare (1 ha =
100 x 100 m) on planets and per cubic kilometre in space.

### 2.0 One dressed zone per planet (Ryan's decision, Nov 13)

Distances are from the pad. The same layout serves both planets.

| Band | Radius | What is there | Purpose |
|---|---|---|---|
| The course | 0 to 150 m | Pad, routes, objectives, as today, with the course's own dressing | Play on foot |
| The heart | 150 m to 1 km | Full density of every zone type in 2.1 or 2.2; four sites | Everything seen from the pad and on foot |
| The approaches | 1 to 3 km | Full density; four corridors with their own character and one site each | What every arrival flies over |
| The taper | 3 to 4 km | Density falls from full to a quarter | No visible line where dressing stops |
| The rim | 4.5 to 7 km | A ring of mountains and cliffs 1.2 to 1.8 km high, sparse canopy clumps or spires, haze | Hides the bare map from low and middle heights |
| Beyond | 7 km to the 40 km far edge | Terrain with colour variation only, bent down by the curvature that is already there, behind haze and a cloud layer | Reads as distance, not as empty |

Limits that keep the player inside it:

| | Today | Proposed |
|---|---|---|
| Arrival distance from the pad | Up to 6 km | **Up to 2.5 km** (one constant, shared by the descent, the globe bake and take-off) |
| Arrival heading | The entry heading, any direction | The entry heading, **turned to within 50 degrees of the bearing to the pad** |
| Arrival height | 1,000 m | 1,000 m (unchanged; the glide down is where the zone is revealed) |
| Flight warning | 8 km | **4 km** |
| Flight turn-back | 8.5 km | **5 km** |
| Ceiling | About 2.9 km | **About 1.6 km above the pad**, below the rim's peaks; the take-off climb is unchanged |

**Four ways in that feel different.** The terrain already has four named regions by bearing and a hero peak
(`World/TDTerrainField.cpp:38-72`). Each becomes an approach corridor with its own first view and its own site:

| Verdance, bearing | First view | Site under the glide | Cinder, bearing | First view | Site under the glide |
|---|---|---|---|---|---|
| Plateau, 20 degrees, with the massif behind | Down off a high forested shelf | Survey station on the edge | Plateau, 320 | Across ash dunes toward the glow | Buried wreck |
| Canyon, 110 | Along the rift between glowing groves | Ruin ring on the lip | Canyon, 60 | Down a lava-lit rift | Drill rig and dig site |
| Mesas, 200 | Between flat tops | Relay pylon on a mesa | Mesas, 150 | Between basalt tables | Outpost fragment |
| Lake, 290 | Low over water to the shore | Wreck field on the beach | Lava sea, 230 | Over the lava sea to the shore | Crystal spire field |

Entries between two bearings blend the two, so there are no hard seams. All four converge on the same heart.

### 2.1 Verdance: six zones from the terrain field

The zone at a point is a pure function of what `FTDTerrainField::Sample` already returns (height, slope, the broad
mask) plus the four region weights and the distance to the sea. The same function drives the material, so trees
stand where the ground is painted as forest floor.

| Zone | Rule | Canopy trees | Understory | Ground cover (near only) | Rocks | Special |
|---|---|---:|---:|---:|---:|---|
| Jungle floor | Slope under 0.25, low to mid height, broad mask above 0.45 | 40 | 80 | 600 | 15 | A Kapok giant every 2 to 3 ha |
| River valley and lake shore | Within 150 m of sea level, slope under 0.2 | 15 | 60 (stilt palms, tree ferns) | 800 (ferns, glow plants) | 20 | Fallen logs, 4 |
| Glowing grove | Broad mask under 0.2 and fine mask above 0.6: patches of 1 to 3 ha | 5 | 20 | 900 (mushrooms, glow plants) | 5 | Giant mushrooms: the three mushroom meshes at 3 to 8 times size, 25 |
| Cliffs and canyon walls | Slope 0.45 to 0.8 | 0 | 5 | 0 | 60 (boulders at the foot, 2 to 15 m) | Hanging tree ferns on ledges |
| High ground | Top third of the height range | 0 | 3 | 100 | 40 | Lichen rocks; bare above the snow line |
| Mesa tops and the plateau | Region weight above 0.5, slope under 0.15 | 10 | 20 | 300 | 10 | Where ruins and survey sites go (2.3) |

About half the ground is slope or summit, so the average is near 60 trees per hectare. The dressed zone (2,830 ha
plus the taper) holds about 170,000 trees; the whole core would have needed 1.5 million. It still streams around
the camera, because 170,000 trees with their understory is too many to hold at once, but only inside the zone.

### 2.2 Cinder: five zones

| Zone | Rule | Columns and spires | Boulders | Rubble (near only) | Crystals | Special |
|---|---|---:|---:|---:|---:|---|
| Basalt field | Slope under 0.3, mid height | 25 | 60 | 300 | 2 | Charred trees, 6 |
| Lava river bank | `RiverNoise` under 0.08, outside the glowing bed | 5 | 40 | 200 | 0 | Steam vents, 2; the river itself glows (material) |
| Crystal outcrop | Broad mask above 0.75: patches of 0.5 to 2 ha | 5 | 10 | 100 | 120, with 3 to 6 times size spires at 4 | Amber toward the outpost side, cyan toward the jammer side, as on the course |
| Ash dunes | Region weight of the plateau or the lava-sea rim, slope under 0.15 | 0 | 5 | 30 | 0 | Half-buried columns; deliberately empty so the others read as dense |
| Volcano flanks and cliffs | Slope above 0.45 or within the hero radius | 10 | 50 | 0 | 5 | Mining works (2.3) |

### 2.3 Buildings and landmarks on both planets

Eight per planet, placed by hand from a table: one under each of the four approach glides at 1.5 to 2.5 km, and
four in the heart within 1 km of the pad, so at least two are in view from any arrival and from the pad. Each
has a thin light beam 400 m tall in its own colour so it is a destination from the air.

| Verdance | Made from | Cinder | Made from |
|---|---|---|---|
| Ruin ring | `SM_Verd_RuinPillar`, `SM_Verd_RelayPlatform_S/M/L`, the stone arch from Nicholas's library | Outpost fragment | `SM_Cind_OutpostWall`, `_Doorway`, `SM_Cind_Crate_A/B` |
| Survey station | `SM_RoutePost`, `SM_Console`, crates; later Grok Bot's survey outpost | Drill rig and dig site | Grok Bot's board (new); until then `SM_Cind_JammerTower` at 0.6 size with crates |
| Wreck field | `SM_Verd_Wreck_Hull/Wing/Fin/Engine`, `SM_Verd_Log`, `SM_DebrisCluster_01/02` | Crystal spire field | `SM_Cind_Crystal_03/04` at 4 to 8 times size |
| Ancient relay pylon | `SM_Verd_RelayAltar`, `SM_Verd_NodeStub`; later Grok Bot's relay | Collapsed column henge | `SM_Cind_BasaltColumn_01/02` laid over |
| Giant grove | One Kapok at 2 times size with a mushroom ring | Buried wreck | Verdance wreck parts, half sunk, charred material |

### 2.4 Space

| Element | Where | How many | Size | Can be hit | Moves |
|---|---|---:|---|---|---|
| Two belts | Tilted rings at 1.6 km and 3.4 km from the origin, 300 m thick, with a gap wherever a lane crosses | 900 each | 8 to 150 m | Only rocks over 25 m, about 15 % | 60 of them tumble slowly |
| Clusters | 14, seeded, between 1.5 and 8 km out in all directions including above and below | 60 each | 5 to 120 m | Over 25 m only | No |
| Battle debris fields | 5, each tied to a wreck | 150 pieces each | 1 to 20 m | No | Slow spin in the material |
| Drifting wrecks | 8 | 1 hull each, 60 to 200 m | | Yes | Drift at 3 m/s and turn |
| Gas clouds | 20, coloured to match the nebula | 200 to 600 m | | No | Scroll in the material |
| Buoy chains | A blinking buoy every 500 m beside every lane | about 40 | | No | Blink |
| Lane rocks | As today but 250 per long lane and up to 60 m | 750 | | No | No |
| Small debris ring | Always around the ship, out to 1.2 km | 2,000 to 3,500 alive | 0.5 to 6 m | No | Spin in the material |
| Traffic | Hauler columns on 4 lanes all game; one comet | 14 haulers, 1 comet | | No | Yes |
| Pickups | From kills; in debris fields and wrecks | Up to 3 alive in the combat zone, about 20 caches | | Collected | Bob and spin |

Gaps are the point: a belt you must find a way through, with the lanes as the obvious safe road.

### 2.5 What "continuously generating as you fly" means here

One method for both planets and space:

- The world is cut into square (planet) or cubic (space) cells. The contents of a cell are computed from
  `hash(world seed, cell coordinates, layer)` and the terrain or density function at that place. Nothing is stored.
- A ring of cells around the camera is kept alive. Cells entering the ring are generated on a worker thread and
  added; cells leaving it are removed and their components reused.
- Because a cell is a pure function of its coordinates, the same rock is in the same place every run, after every
  retake, and after flying away and back. There is nothing to save. The only saved state is the set of collected
  cache ids and visited site ids.
- Objects large enough to be seen from far away (belts, clusters, wrecks, POIs) are not streamed: they are built
  once at load from the same seed, so nothing large ever pops in.

---

## 3. The systems to build

### 3.1 System A: planet scatter (`ATDTerrainScatter`)

- **What:** one actor per planet map, spawned by `ATDPlanetTerrain::Setup`, that fills two rings around the camera
  from the terrain field, **only inside the dressed zone**: full density to 3 km from the pad, falling to a quarter
  at 4 km, nothing beyond. The far ring is no longer streamed; it is built once by system F.
- **Owner:** T4. T5 does the course hand-off.
- **Files:** new `World/TDTerrainScatter.{h,cpp}`, `World/TDBiome.{h,cpp}` (the zone function and the per-zone
  lists as plain C++ tables), `World/Tests/TDTerrainScatterTests.cpp`; one call added in `TDPlanetTerrain.cpp`;
  T5 adds `GetKeepOuts()` and `GetRoutes()` to `TDPlanetCourse.h` and removes the course's outer-ring rows.
- **Rings:**

| Ring | Cell | Radius | Contents | Alive at once |
|---|---|---|---|---:|
| Near | 50 m | 120 m | Ground cover, rubble, small rocks, grass | about 3,000 |
| Mid | 200 m | 800 m | Trees, understory, boulders, columns, crystals | about 12,000 |
| Far (system F, built once at load) | none | 3 to 8 km from the pad | Canopy clumps (Verdance), large spires and boulder piles (Cinder), 50 per square km | about 8,600 in total |

  Budget for the whole planet at any moment: **under 25,000 instances alive**, of which at most 12,000 are trees.

- **How a cell is made:** a jittered grid of candidates at the zone's density; each candidate samples height and
  slope (three height evaluations), is accepted or rejected by its zone rule, gets a seeded yaw, scale and a 5 to
  30 cm sink, and is aligned upright (trees) or to the slope (rocks). Below sea level: rejected.
- **Performance:**
  - Generation runs on the thread pool, at most two cells at a time so it never starves terrain chunks. A mid cell
    is about 400 candidates, roughly 20 to 40 ms on a worker by the terrain's own cost per sample.
  - Applying a finished cell is one `AddInstances` per mesh on a pooled `UInstancedStaticMeshComponent`; budget
    0.5 ms a frame, measured, with a hard stop.
  - The first view is built inside `BuildFirstViewAt` with `ParallelFor`, mid ring only, target under 300 ms, hidden
    behind the descent's white cover like the terrain. Arrivals now come from a 2.5 km ring, so this is at most
    about 50 mid cells and always inside the zone.
  - Trees and the space-kit rocks are Nanite. Ground cover gets `InstanceEndCullDistance` of 150 m and no shadows.
    Mid ring fades between 700 and 800 m. The far clumps dither **out** nearer than 800 m in their material, so the
    two tiers swap without a pop.
  - At 65 m/s the mid ring takes in about three cells a second: inside the budget.
- **Collision:** ships collide with nothing in the scatter. The pilot is blocked by boulders over 1.5 m and columns
  in the near ring only (one collision-enabled pooled component per such mesh); trees, plants, crystals under 1.5 m
  and everything in the mid and far rings are visual only. Nothing affects navigation.
- **Staying out of the way:** nothing inside the course's ground box plus 20 m (the course keeps dressing its own
  floor), on a route with 12 m clearance, in any course keep-out, within 40 m of the pad, inside the arrival cone
  the terrain already keeps clear (`ApproachHalfAngleDeg`, `ApproachLength`), or within a site's footprint.
- **Hours:** 6 (T4) + 2 (T5). Two hours less than a whole-map scatter: no streamed far ring, and only the zone
  types that occur within 4 km of the pad need tuning and capturing.
- **Tests:** the same cell generated twice is identical; no instance inside any keep-out; no instance below the sea
  or on a slope outside its zone's range; counts per ring under the caps; no instance farther than 4 km from the pad; a 60-second automated flight at boost
  across the zone with the apply time logged, worst frame under 0.5 ms; a walk test that the pilot is never stuck.
  Then captures at the `fe1` spot (1.1 km up, 2.5 km from the pad), from the pad, and on foot 300 m from the pad.

### 3.2 System B: points of interest (`FTDPoiPlan`, `ATDPoiSite`)

- **What:** eight sites per planet from a hand-written table (type, bearing, distance): the four corridor sites and
  four in the heart (2.0, 2.3). The code only snaps each to the flattest spot within 150 m of its table position
  (slope under 0.15, above the sea, outside the course). Sites are built at load, not streamed. No rule-based
  placement is needed for eight.
- **Each site:** 5 to 20 kit meshes from a small layout table per type, sunk 1 to 3 m so a slope of up to 8 degrees
  is hidden; a ring of denser scatter around it; a beam; a HUD marker inside 1.5 km.
- **Reward (cut first):** one console per site, hold E (reuse `ATDInteractableActor`): a line from `dialogs.md`, and
  "Sites found N / M" on the end card. No gameplay bonus, so no balance change.
- **Owner:** T4 for the plan and placement; T5 for the interact and marker; asset builder and Grok Bot for pieces.
- **Files:** new `Ground/Planets/TDPoiSite.{h,cpp}` with the two tables, tests.
- **Performance:** eight actors with instanced parts.
- **Collision:** walls, pillars and hulls block the pilot and ships (simple boxes). Small parts do not.
- **Staying out of the way:** never within 250 m of the pad or inside the course or its keep-outs; never in the
  final landing cone the terrain keeps clear.
- **Hours:** 3 (was 6 for 30 rule-placed sites).
- **Tests:** the same sites every run; none on a slope over 0.15, under the sea or inside the course; from each of
  eight arrival bearings at least two beams are in the forward view.

### 3.3 System C: space field (`ATDSpaceField`)

- **What:** two layers from one seed and one density function `D(p)` (belts + clusters + debris fields, zero inside
  keep-outs).
  - **Whole-galaxy layer, built at load:** belts, clusters, debris fields, wrecks, gas clouds, buoy chains, the
    heavier lanes. About 4,500 instances.
  - **Ring layer, streaming:** cubic cells of 400 m, radius 1.2 km around the camera (about 110 cells), 6 to 40
    small pieces per cell by `D(p)`, never fewer than 6, so even "empty" space has motes going past the canopy.
- **Owner:** T1 (it must feel right at cruise, boost and hyperdrive, and T4 is on the planets). T4 reviews because
  it touches `World/`.
- **Files:** new `World/TDSpaceField.{h,cpp}`, `World/Tests/TDSpaceFieldTests.cpp`; one spawn call in
  `ATDGalaxyLayout::BeginPlay`; `LaneRocksPerLane` raised in `TDGalaxyLayout.h:325`.
- **Performance:** the galaxy layer uses hierarchical instancing (it is 30 times the count the old measurement was
  made at) with cull distances by size: under 10 m, 1.5 km; 10 to 40 m, 4 km; larger, 9 km. The ring layer uses
  pooled plain instanced components, no shadows, no ray tracing, cull at 1.3 km; generation is a hash and a density
  lookup, under 0.2 ms a cell, done on the game thread within a 0.3 ms budget. At hyperdrive (800 m/s) the ring
  stops updating; the star streaks cover it and it refills on drop-out. Spin and drift of small pieces are done in
  the material from a per-instance random value: no CPU cost. The 60 tumbling rocks and 8 wrecks are one movable
  instanced component each, updated in a batch each frame.
- **Collision:** only galaxy-layer rocks over 25 m, wrecks, and nothing else. They are registered with
  `UTDWorldSubsystem::AddObstacle` so enemies and wingmen steer round them. T1 must check `FindAvoidance` cost with
  about 400 more obstacles before merging; if it is a plain loop over all of them, add a coarse grid.
- **Staying out of the way:** `D(p)` is zero, and nothing that can be hit is placed, within 1.5 km of the combat
  zone centre (the existing belt and flank field are untouched), within 150 m of any lane line, within the existing
  `IsInKeepOut` zones, within 1.3 times a planet's atmosphere radius, within 1.2 km of the Dominion and its hangar
  exit, or on the convoy lanes. Visual-only pieces may enter the 150 m lane band but not its 90 m core.
- **Hours:** 7 (4 for the galaxy layer tonight, 3 for the ring tomorrow).
- **Tests:** deterministic for a seed; no collidable instance in any keep-out (extend
  `TriadDrift.World.Layout.SpaceDressingFillsButNeverBlocks`); from 26 sample points on a sphere of radius 4 km, at
  least 20 instances within 1.5 km of each; an automated 60-second cruise in six directions with frame time logged
  against the same run with `td.SpaceField 0`. Then captures at the `aq1` and `ap2` spots.

### 3.4 System D: ambient traffic and a comet

- **What:**
  - `ATDArmadaConvoy` loses its Verdance-only rule for the dressing convoys: four lanes are always running (zone
    edge to each planet, planet to planet, outer rim to the Dominion once it is revealed), 3 or 4 haulers each.
    They keep ignoring the squad and cannot be hit or targeted. The scannable convoy keeps its story rule.
  - One comet: an ice rock at 300 m with a tail, on a fixed ellipse through the galaxy, one pass every four minutes,
    never nearer than 2 km to the combat zone. Visual only.
  - Not proposed: live Null patrols that attack outside waves. They would start fights the wave director does not
    know about and are the likeliest way to hurt combat. If Ryan wants them, they are a separate decision.
- **Owner:** T3, reviewed by T4 who wrote the convoys.
- **Files:** `World/TDVerdanceApproach.{h,cpp}`, `TDGalaxyLayout.cpp:470-490`, new `World/TDComet.{h,cpp}`.
- **Performance:** 14 haulers of four components each and one actor.
- **Hours:** 4.
- **Tests:** convoys are active in every space phase and still never targetable; the story scan still fires once;
  the comet's nearest approach to the zone is over 2 km.

### 3.5 System E: pickups in space (new gameplay)

The smallest version that is worth having.

| Pickup | Colour | Effect | Uses (already in the code) |
|---|---|---|---|
| Hull repair | Green | +35 % hull on the ship you are flying | `UTDHealthComponent::RepairHull` (`Core/TDHealthComponent.h:106`) |
| Shield cell | Cyan | Full shield on your ship, half on both wingmen | `RestoreShield` (`:109`) |
| Power cell | White | Signature power and ability ready now | `UTDPowerComponent::ResetCooldown` (`Powers/TDPowerComponent.h:76`) |
| Overcharge | Amber | +50 % weapon damage for 8 s | `UTDWeaponComponent::SetOutgoingDamageMultiplier` (`Combat/TDWeaponComponent.h:145`) |

- **Where they come from:**
  - Kills: fighter 12 %, interceptor, gunship and bomber 25 %, carrier always two. At most 3 alive at once; each
    lasts 25 s and blinks for the last 5.
  - A safety rule: if your hull is under 35 % and no repair has dropped for 20 s, the next kill drops one.
  - Caches: about 20 fixed ones inside the debris fields and wrecks from system C, one use per run.
- **Collecting:** only the ship the player is flying. Within 60 m the pickup flies to the ship; contact collects.
  Switching ships to send the right one to a pickup is a small new reason to switch.
- **HUD:** a diamond marker in the pickup's colour inside 800 m; a one-second toast by the ship card ("+HULL",
  "+SHIELD", "POWER READY", "OVERCHARGE"); a thin amber bar while overcharged. One event on the Core bus so both the
  flight HUD and Nicholas's HUD plugin can show it; T2 posts the event shape on the board for Nick's side.
- **Owner:** T2; T4 adds the drop call where an enemy dies; T1 places caches from system C.
- **Files:** new `Combat/Pickups/TDPickup.{h,cpp}`, `TDPickupSubsystem.{h,cpp}`, tests; one event in `Core/`; HUD
  rows in `UI/`; one call in `Enemies/TDEnemyShip.cpp`. Meshes: `SM_CargoPod_01` with four emissive material
  instances (no new texture). Sounds: reuse a fallback UI confirm.
- **What it changes in combat balance (exactly):**
  1. **Hull stops being a one-way resource.** Ships have shield regeneration but hull regeneration is zero by
     default (`TDHealthComponent.h:55`). Repair pickups are the first in-fight hull healing, so every wave gets
     easier. Limit: at most two repair drops per wave plus the safety drop.
  2. **Time to kill drops by a third for 8 seconds** under overcharge. Limit: at most two per wave; it does not
     stack with itself.
  3. **Cooldowns can be skipped.** A power cell lets Lancer ram twice in a row or Bastion fire two missile swarms.
     Limit: at most two per wave.
  4. **Fewer deaths means fewer "CUT!" retakes.** The theme hook is seen less often by good players. Acceptable.
  5. **Wingmen get tougher** through the shield cell's half share.
  6. The wave balance in `docs/eng/balance.md` and the stress tests in `Waves/Balance/Tests` were tuned without any
     of this. T2 reruns them with pickups on and off and reports both. `td.Pickups 0` restores today's game exactly.
- **Hours:** 6.
- **Tests:** each effect applies once and only to the right ship; caps hold over a simulated wave; a pickup is never
  spawned inside a rock; nothing drops during the Dominion breakup or a money shot's slow motion; lifetimes use game
  time so a retake clears them.

### 3.6 System F: the edge (so the player never sees an empty map)

Six layers, nearest first. No single one has to be perfect.

| Layer | What | Owner | Hours |
|---|---|---|---:|
| 1. Arrivals | `MaxArrivalRadius` 6 km to 2.5 km (`World/TDPlanetEntry.h:34`; the globe bake and take-off read the same constant). In `ArrivalFromGlobe`, clamp the heading to within 50 degrees of the bearing to the pad, so the first view always holds the zone and the entry side still shows | T4, agreed with T1 | 1 |
| 2. Flight zone | In `ATDAtmosphereFlightZone::FitToTerrain` use a zone half-size of 5 km instead of the core plus 500 m, a warning distance of 1 km, and a ceiling 1.6 km above the pad. The existing turn-back does the rest | T1 | 1 |
| 3. Rim | A ring term in `FTDTerrainField::RawHeight`: ridged mountains rising from 4.5 km, peaking 1.2 to 1.8 km high at 5.5 to 6.5 km, broken by two or three passes (so it is a range, not a wall) that line up with the lake and canyon corridors and are closed by haze. The course basin and every existing test position are inside 4 km and do not move; the globe bake follows the field automatically | T4 | 1.5 |
| 4. Far ring | Built once at load: 50 canopy clumps (Verdance) or spires and boulder piles (Cinder) per square km from 3 to 8 km, about 8,600 instances, Nanite, no collision, dithering out nearer than 800 m where real trees take over | T4 | 0.5 |
| 5. Haze, cloud and colour | Environment builder, requirements 2, 6 and 7 below | Environment builder | in its work |
| 6. Curvature | Already there: the far ring bends down by distance squared over 2 x 530 km (`TDPlanetTerrain.h:92`). Keep | | 0 |

- **Why entries still feel different:** the arrival point is on the entry's bearing, 2.5 km out, heading inward
  within 50 degrees, at 1,000 m. The player sees a different corridor, a different site under the glide and the
  heart from a different side each time (2.0).
- **Collision:** unchanged. The rim is inside the collision core where it is under 8 km, so a ship that reaches it
  meets real ground and the turn-back, not a hole.
- **Tests:** every arrival from 24 bearings and 3 entry angles is within 2.5 km and faces within 50 degrees of the
  pad; the take-off mapping still inverts; no ground inside 4 km changes height (compare the field before and
  after at 1,000 points); a scripted flight outward on 8 bearings is turned back before 5 km; then a capture from
  each of the four corridors at arrival, and one from the ceiling above the pad looking at the horizon, checked by
  eye for any bare ground that reads as empty.
- **Hours:** 4.

### 3.7 Requirements added for the environment builder

1. Read the zone from the same inputs the scatter uses (vertex colour and `UV1`: height, slope, broad mask, fine
   mask), so forest floor, grove, cliff, ash and crystal ground appear under the matching scatter. T4 publishes the
   thresholds in `World/TDBiome.h`; the material uses the same numbers.
2. A far "canopy" colour for the forest zone from 3 km outward, where instances thin and then stop, so the forest
   does not end at a visible line. Beyond the rim, colour variation only: no new detail is needed there.
3. A detail normal and macro colour variation strong enough to hide 15.6 m polygons at walking distance.
4. Lava river beds that glow from `RiverNoise` (already in the broad mask on Cinder), and a real water and lava
   surface material for the sea disc in place of the tinted cylinder.
5. Check the outer ring's material instance (finding in 1.1).
6. Ground fog and height fog tuned with the scatter on: fog is what turns 800 m of real trees plus clumps into
   "forest to the horizon". Target: the rim at 5 to 6 km reads as a blue or ember silhouette, and nothing beyond
   7 km shows surface detail from any height up to the 1.6 km ceiling.
7. A cloud or ash layer that sits in the rim's passes and over the ground beyond the rim (cards or the descent's
   cloud deck reused), so looking out from the ceiling shows peaks standing in cloud, not a plain.

---

## 4. Assets

### 4.1 Exists now and can be instanced tonight (no size cost)

| Use | Meshes |
|---|---|
| Verdance trees | `SM_TDVeg_{Kapok, BrazilNut, TrumpetTree, StiltPalm, TreeFern, Cacao}_{S100, S070, S030}` (18 in `Props/Trees`; only 7 are used today, 11 ship unused) |
| Verdance plants and rocks | `SM_Verd_Fern_01/02`, `SM_Verd_GlowPlant_01/02`, `SM_Verd_Mushroom_01/02/03`, `SM_Verd_Rock_01/02/03`, `SM_Verd_Log` |
| Cinder | `SM_Cind_Crystal_01..04`, `SM_Cind_Basalt_01..03`, `SM_Cind_BasaltColumn_01/02`, `SM_Cind_ChokeRock_A/B`, `SM_Cind_DebrisChunk` |
| Site pieces | `SM_Verd_RuinPillar`, `SM_Verd_RelayPlatform_S/M/L`, `SM_Verd_RelayAltar`, `SM_Verd_NodeStub`, `SM_Verd_Wreck_Hull/Wing/Fin/Engine`, `SM_Cind_OutpostWall`, `SM_Cind_OutpostWall_Doorway`, `SM_Cind_Crate_A/B`, `SM_Cind_JammerTower`, `SM_Console`, `SM_RoutePost`, `SM_Trailhead`, `SM_LandingPad` |
| Space | `SM_Asteroid_01..04`, `SM_SpaceDebris_01..03`, `SM_Ast_Ice_01..03`, `SM_Ast_Metal_01..03`, `SM_Ast_Crystal_01/02`, `SM_DebrisCluster_01/02`, `SM_CargoPod_01..03`, `SM_Derelict_Satellite_01/02`, `SM_StationTruss_01..03`, `SM_BeaconBuoy_01` |
| Terrain textures | Grok Bot's nine sets in `ArtSource/EngAI/Terrain` (environment builder is wiring them) |

### 4.2 To make

| Asset | For | Budget | Who | Package cost |
|---|---|---|---|---:|
| `SM_Verd_GrassTuft_01/02` | Near ring ground cover | 12 to 24 triangles each, one 512 atlas | Asset builder, Blender | 0.3 MB |
| `SM_Verd_CanopyClump_01..03` | Far ring | 400 to 800 triangles, Nanite, uses the existing leaf material, dithers out under 800 m | Asset builder | 0.5 MB |
| `SM_Verd_Boulder_L_01/02`, `SM_Cind_Boulder_L_01/02` | Cliff feet, volcano flanks; 5 to 15 m | 1,500 triangles each, world-aligned material from the terrain cliff set, no own texture | Asset builder (rock kit in the `blender-game-asset` skill) | 0.6 MB |
| `SM_Cind_CharTree_01/02` | Basalt fields | Leafless, about 2,000 triangles, bark material only | `procedural-trees` skill | 1 MB |
| `SM_Wreck_Hull_A/B` | Space wrecks, buried wrecks | 6,000 to 10,000 triangles, kitbashed from the truss and hull-panel kit, existing textures | Asset builder | 1 MB |
| `M_TD_GasCloud`, `M_TD_PoiBeam`, four pickup material instances | Space clouds, site beams, pickups | No textures (use `T_TD_VFX_Noise`) | Environment builder / T2 | 0.1 MB |
| Survey outpost and console, ancient relay, crashed wreck, drill rig and dig site, pad dressing | Sites, from Nicholas's six boards (R-030) | Each under 20,000 triangles, Nanite, **one 1K set** (base colour, normal, packed mask) | Grok Bot, cleaned by the asset builder | about 3.5 MB each, 18 MB |
| Stone arch, Spaceship (as a wreck) | Ruin rings, a wreck field | From Nicholas's library, already shortlisted in `ASSET-INTAKE-REVIEW.md`; needs clean-up to under 15,000 triangles and a 1K set; **held until Nicholas confirms when it was made and the licence** | Asset builder | 3 MB each |

**Total: about 25 to 30 MB.** With 760 MB today that leaves over 200 MB. Measure the package after the Grok Bot
pieces land; if any arrives with 2K textures, cap it at 1K on import.

---

## 5. The order

### 5.1 Tonight (highest visible gain per hour first)

| # | Task | Owner | Hours | Done when |
|---:|---|---|---:|---|
| 1 | Scatter v1: `TDBiome`, mid and near rings inside the 3 km zone with the taper, Verdance, existing meshes only, keep-outs, `td.Scatter` | T4 | 5 | Captures from the pad, and from 1,000 m at 2.5 km on four bearings, show dressed ground in every direction; tests pass |
| 1b | The edge, part 1: arrival ring 2.5 km and inward heading (T4); flight zone 5 km, warning 4 km, ceiling 1.6 km (T1) | T4, T1 | 1 + 1 | Arrival and turn-back tests pass; take-off still inverts |
| 2 | Course hand-off: expose keep-outs and routes; drop the course's outer-ring rows (the canopy ring that never spawns); walk test | T5 | 2 | No scatter on any route or arena; pilot not stuck |
| 3 | Space field, galaxy layer: belts, clusters, debris fields, heavier lanes, buoy chains, `td.SpaceField` | T1 | 4 | Capture at the `aq1` spot is full; keep-out test passes; frame time compared |
| 4 | Pickups v1: four types, kill drops, safety rule, toast and marker, `td.Pickups` (default on in dev, decision for ship is Ryan's) | T2 | 5 | Tests pass; balance runs reported on and off |
| 5 | Grass tufts, canopy clumps, four boulders, charred trees | Asset builder | 4 | Imported, in the scatter tables |
| 6 | Requirements 1 to 7 of section 3.7, starting with the shared zone thresholds | Environment builder | 2 on top of its current work | Material zones line up with the scatter in one capture |

### 5.2 Tomorrow (Nov 14)

| # | Task | Owner | Hours |
|---:|---|---|---:|
| 7 | Scatter on Cinder; the edge, part 2: rim mountains and the prebuilt far ring on both planets; first-view build inside the descent | T4 | 5 |
| 8 | Eight sites per planet from existing pieces, beams, markers | T4, T5 | 3 |
| 9 | Space ring layer; tumbling rocks; wrecks; caches for pickups | T1 | 4 |
| 10 | Traffic on four lanes; the comet | T3 | 4 |
| 11 | Grok Bot's site pieces cleaned, imported at 1K, swapped into the site tables | Grok Bot, asset builder | 6 |
| 12 | Frame-rate pass on the target machine, both planets and six space directions, packaged build | Lead, T1 | 2 |
| 13 | Independent review of A, C and E by an agent that wrote none of them; a played session of each | Lead | 2 |

### 5.3 Nov 15 before noon

Fixes from the review, density tuning from Ryan's notes, package size check, nothing new.

### 5.4 What to cut, in this order

1. Site consoles and the "Sites found" count.
2. The comet.
3. Gas clouds.
4. Pickup caches (keep kill drops).
5. Sites down to the four corridor sites per planet, existing pieces only.
6. The far ring's clumps (rely on the rim, the haze and the material's canopy colour).
7. The rim mountains (rely on haze and the tighter flight zone; riskier, check from the ceiling).
8. The space ring layer (keep the galaxy layer).

Not to be cut: the mid ring on both planets; the 2.5 km arrival ring with the inward heading and the 5 km flight
zone (two hours that make the whole zone idea work); the galaxy layer in space. If pickups cannot be balanced by tomorrow
night, ship with `td.Pickups 0`.

### 5.5 What Ryan must see (taste calls)

- Tree density and the look of the forest from 300 m and from 1 km up, on Verdance, after task 1.
- The edge: one look from the ceiling above the pad and one from each of the four arrivals. Can he tell where the
  dressed zone ends? Is 2.5 km out, 1,000 m up, the right first view, or should arrivals be closer?
- The 1.6 km ceiling and the 5 km turn-back: tight enough to hide the map, loose enough not to feel fenced in?
- Ships fly through tree crowns with no collision. Fine, or should the flight floor rise above the canopy?
- How empty Cinder's ash dunes should be.
- Belt density and whether the gaps read as a way through, after task 3.
- Pickups: on or off for the submitted build, and the four colours against the squad colours (cyan is Lancer's,
  amber is Bastion's: the shield cell and overcharge may need different hues).
- Whether hostile patrols outside waves are wanted at all.

### 5.6 Risks

| Risk | Why | Guard |
|---|---|---|
| Frame rate on planets | 25,000 instances alive, Nanite foliage, shadows | Caps per ring; no shadows on ground cover; cull distances; `td.Scatter` levels 0 to 3; measured flight test before merge |
| Hitches | Scatter competing with 180 to 440 ms chunk builds and 4 ms uploads | Two worker cells at a time; 0.5 ms apply budget; first view behind the white cover |
| Frame rate in space | 4,500 + 3,500 instances; translucent gas clouds overdraw | Hierarchical instancing with cull by size; at most three clouds on screen; clouds fade when the camera is inside |
| The 1 GB limit | Six Grok Bot pieces could arrive at 2K: 6 x 12 MB | 1K rule on import; measure after task 11 |
| Readability in combat | More things on screen behind enemies | Nothing new within 1.5 km of the zone; ring-layer pieces are dark and under 6 m; enemy glow and markers unchanged; compare `dress_after_belt` before and after |
| Collision lottery | Rocks in dogfights | The pocket and the zone are untouched; only rocks over 25 m can be hit and never in a lane; collision damage is already capped and never one-shots (`Flight/TDFlightTypes.h:241`) |
| Enemy and wingman steering | 400 more obstacles in `FindAvoidance` | T1 measures it; add a grid if it is a linear scan |
| Three-ship feel | Pickups change survival and cooldown rhythm | Caps per wave; only the flown ship collects; `td.Pickups 0` is today's game; balance runs both ways |
| Story | Always-on convoys could blunt the Verdance convoy beat | The scannable convoy keeps its phase rule and its ring; the others are plain haulers; T3 checks with `storyline.md` |
| The edge shows | From the ceiling the player looks over the rim onto bare ground | Ceiling below the rim's peaks; haze and cloud beyond; captures from the ceiling in the tests; lower the ceiling before adding detail |
| The rim changes the course | A new term in the height field | It starts at 4.5 km; a test compares 1,000 heights inside 4 km before and after; the existing hero peak and volcano sit about 7 km out and merge into the rim |
| Arrival change breaks the descent | One constant is shared by the descent, the globe bake and take-off | T4 and T1 change it together; the existing planet-entry round-trip test must still pass; rebake the globes |
| Two agents in `World/` | T4 and T1 both add files there | New files only; the two one-line call sites are named above; the Lead merges T4 first |
