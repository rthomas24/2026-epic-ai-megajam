# Rodin ships: Grok Bot's Hyper3D Rodin meshes as the game's ships

Intake of the Rodin Gen-2.5 meshes in `ArtSource/EngAI/Ships/**` (Grok Bot's, read-only: never edited here) as
the look of the three squad ships and the Null Armada fighter and gunship. They replace the code-built
engineering ships (`ArtSource/EngShips/Squad`, `ArtSource/EngShips/NullFleet`), which stay in the repo untouched.

Like the ships they replace these are **engineering fallbacks**: the visuals data assets live in
`/Game/TriadDrift/Gameplay/Characters/{Squad,NullFleet}/`, and a `DA_*_Visuals` in `/Game/TriadDrift/Presentation/`
still wins field by field.

## Rebuild

```
cd ArtSource/EngShips/Rodin/scripts
blender --background --factory-startup --python rs_prep.py -- Rift          # Lancer, Bastion, NullFighter, NullGunship, NullInterceptor
powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Rodin\scripts\ue_import_rodin_ships.py   # from the repo root
```

- `rs_common.py`: where each ship's source, textures and heat mask are.
- `rs_analyze.py`: prints emissive clusters and forward / rear tips of a source mesh (used to find the sockets).
- `rs_prep.py`: imports the derived FBX, removes loose vertices and degenerate faces only (UVs and Rodin's
  normals are kept: the heat masks are baked in that UV layout), adds `SOCKET_*` empties, renders
  `review/<Ship>_r1_sockets_material_sheet.png` (rear, nose, top, side, two 3/4 views, sockets as dots: green
  muzzles, blue engines, yellow others), saves `../<Ship>/SM_<Ship>.blend`, exports `SM_<Ship>.fbx` and the manifest
  with the repo exporter. `--no-review` skips the sheet, `--no-export` stops before the FBX.
- `ue_import_rodin_ships.py`: Unreal import, idempotent. `TD_RODIN_MI_ONLY=1` only rewrites the material instances
  and visuals (tuning); `TD_RODIN_REVERT=Rift,NullFighter,...` points a ship's visuals back at the old mesh
  (`OldFleet` also drops the material override from the interceptor, bomber and carrier visuals).
  Since Nov 13 evening (ship builder): `TD_RODIN_MI_ONLY` also takes a list (`Lancer,Bastion`); `TD_RODIN_ONLY=<Ship>,...`
  limits the run; `TD_RODIN_REBUILD=Portal,Glow,Master` rebuilds those graphs **in place** (same asset, every instance
  keeps its parent); every save is checked and a failed save fails the run (`TDPY: FAILED ... could not be saved`): a
  game of this checkout that has the ships loaded holds their files open (Windows error 32), so close it first.
  `TD_SQUAD_DETAIL_DIR` / `TD_TEX_R2_ROOT` point at copies of Grok Bot's `SquadDetail_r1` / `textures_r2` taken from
  `origin/main` before they are merged here.

Textures are imported straight from `ArtSource/EngAI/Ships/**` (no copies here).

## In Unreal (`/Game/TriadDrift/Gameplay/Ships/`)

| Asset | What |
|---|---|
| `Materials/M_TD_Ship` | one lit master for every ship, Nanite. Parameters below |
| `<Ship>/SM_<Ship>` | Nanite static mesh, one slot, one auto convex hull (gameplay uses its own sphere) |
| `<Ship>/MI_<Ship>` | instance of `M_TD_Ship` |
| `<Ship>/T_<Ship>_BC, _N, _ORM, _E` | 2K (squad) or capped to 1K with `max_texture_size` (Null Armada). N is DirectX |
| `<Ship>/T_<Ship>_HeatMask` | squad only, linear greyscale, capped to 1K |

`M_TD_Ship` parameters: `BaseColorTex` x `BaseTint`; `ORMTex` (AO, roughness x `RoughnessScale`, metallic);
`NormalTex`; `EmissiveTex` x `EmissiveTint` x `EmissiveStrength`; **`HeatGlow` 0..1 (default 0)** x `HeatMaskTex` x
`HeatColor` x `HeatStrength` (20): the re-entry / afterburner glow, to be driven from code on a dynamic instance;
`SelfLight`: base colour added to emissive, so the squad colours read on the side facing away from the sun (the
chase camera mostly sees that side).
Close-range pass (Nov 13 evening, Ryan in the Dominion hangar at 3-4 m): `BaseFloor` (lifts the darkest albedo to a
grey floor without blowing out the whites: the Rift's ring and engines read as black chrome), `MetallicScale`,
`RoughnessMin` (no mirror-like paint), `DetailNormalTex` + `DetailRMTex` (Grok Bot's per-ship `SquadDetail_r1`; RM =
cavity AO / roughness variation) at `DetailTiling` (UV tiles; about 0.8 m tiles: Rift 10, Lancer 7, Bastion 13 from
the measured texel density 258 / 370 / 193 texels per metre at 2K) x `DetailStrength` / `DetailAO` / `DetailRough`,
all faded out between `DetailFadeStart` (15 m) and + `DetailFadeRange` (15 m). `M_TD_RiftPortal` r2: three log-spiral
arms over a dark centre, polar noise, a thin rim, emissive limited by its brightest channel (`MaxEmissive`).
`M_TD_ShipGlow` r2: nozzle back wall with a dark hub, radial vanes and a groove (`Vanes`, `VaneDepth`, `HubRadius`).
`RangeBoost` (default 1 = off) multiplies the emissive map at distance, ramping in from `RangeStart` (60 m) to
`RangeEnd` (600 m): the Null fleet use 3-6 (and EyeBoost x 1.6) so each type still shows its red / orange seams and
white eyes at 300 m - 1 km; close-up looks do not change.

| Ship | EmissiveStrength | BaseTint | EmissiveTint | SelfLight |
|---|---|---|---|---|
| Rift, Lancer | 6 | 1 | 1 | 0.12 |
| Bastion | 6 | 1.5 (the Rodin hull is mid grey, it read as a black hulk from behind) | 1 | 0.18 |
| NullFighter | 10 | 0.35 (the albedo came out light silver; brief: near-black armour) | (1, 0.55, 0.35) | 0 |
| NullGunship | 8 | 0.6 | (1, 0.55, 0.35) | 0 |

The enemy emissive tint pushes the seams to red-orange; it also warms the white eyes to a pale peach. At 24 x
untinted the whole fleet glowed white, which broke the red / orange enemy colour code.

**The visuals name the material instance in `MaterialOverrides`.** Gameplay tints the placeholder hull with a
dynamic glow material in slot 0 before the art is applied, and `SetStaticMesh` keeps component overrides, so an
enemy whose visuals carry only a mesh draws as a flat orange glow. The import also sets this on the old
interceptor, bomber and carrier visuals (their own `MI_Null_*`), so the fleet looks consistent.

## Ships (Unreal cm, pivot = bounding-box centre, +X forward, +Y right, +Z up)

| Ship | Size L x W x H | Source tris | Sockets |
|---|---|---|---|
| Rift | 500 x 417 x 226 | 42,371 | `Muzzle_0/1` (252, -/+37, -60); `Engine` (-252, 0, -34); `Engine_0/1` (-252, -/+64, -34); `Portal` (-170, 0, 55) ring centre; `Cockpit`; `WingTip_L/R` |
| Lancer | 524 x 270 x 150 | 49,718 | `Muzzle_0` (152, -45, -59), `Muzzle_1` (108, 45, -55); `Engine` = `Engine_0` (-264, 0, -35); `Engine_1/2` (-209, -/+55, -38); `Ram` (262, 0, -58); `Cockpit`; `WingTip_L/R` |
| Bastion | 524 x 472 x 139 | 49,871 | `Muzzle_0/1` (264, -/+44, -37); `Engine` (-264, 0, 0); `Engine_0/1` (-264, -/+77, 0); `Dome` (-26, 0, 68); `Cockpit`; `MissilePod_L/R` (124, -/+165, -20) |
| NullFighter | 550 x 503 x 171 | 8,912 | `Muzzle_0/1` (268, -/+57, -38); `Engine_0/1` (-277, -/+63, -14), `Engine_2` (-275, 0, -14) |
| NullGunship | 1497 x 1432 x 1116 | 19,800 | `Muzzle_0..3` (748, -280 / -210 / 210 / 280, ~300) turret barrel tips; `Engine_0..3` (-740, -270 / -90 / 75 / 255, 421) |
| NullInterceptor | 660 x 535 x 248 | 8,948 | exported with sockets (`Muzzle_0..3`, `Engine_0`), **not imported**: see below |

All are the same length as the ships they replace (gameplay collision is unchanged: sphere r 220 for the squad
and the fighter, r 700 for the gunship). Nanite is on (as on the ships they replace), so there is no LOD chain;
the Nanite fallback meshes are 5-17k triangles.

## Known limits (honest list)

- **NullInterceptor is not in the game.** Rodin's pack for it has no textures (a grey clay mesh), so it cannot
  carry the faction look. The interceptor keeps the engineering mesh `SM_Null_Interceptor`. The FBX with sockets
  is ready in `../NullInterceptor/` for when a textured drop arrives.
- **Lancer's guns are asymmetric** in the Rodin mesh: a long barrel on the left, a short pod on the right. The
  muzzle sockets sit on the real tips, so the two shots leave 44 cm apart in X.
- **Rift has three nozzles** (the brief has two): `Engine_0/1` are the outer pair, `Engine` the larger middle one.
- **Bastion has two big nozzles** where the brief has four engine pods; `Engine_2/3` do not exist.
- **Gunship**: the four "engine bells" Rodin made are upward-pointing cones on the rear deck; the `Engine_N`
  sockets sit at the rear face of those pods. The hull is much taller than the engineering gunship (1116 vs 677 cm).
- Rodin meshes are not symmetric and have small floating islands (`assembly_check`: Rift 35 groups, Lancer 6);
  nothing was re-modelled.
- The review sheets were rendered before two socket corrections (Lancer `Muzzle_1`, fighter `Engine_N` height).
- The intro (`TDIntroDirector.cpp`) loads `SM_Null_Fighter` / `SM_Null_Interceptor` by path and still shows the
  old fighter.

## Round 2 (Nov 13, later): fleet r3, white eyes, effects wiring

**Sources.** `Interceptor/mesh_r2`, `Bomber/mesh_r1`, `Carrier/mesh_r1` and `Turret/mesh_r1` under
`ArtSource/EngAI/Ships/NullFleet/`. When a source is on `main` but not merged into the checkout yet, set
`TD_RODIN_AI_ROOT` to a folder that mirrors the repo tree (`<root>/ArtSource/EngAI/...`); `rs_prep.py` and the
import read from there, and the manifests still record the repo-relative paths.

| Ship | Size L x W x H (cm) | Tris | Sockets (Unreal cm) | In the game |
|---|---|---|---|---|
| NullInterceptor (mesh_r2) | 660 x 633 x 221 | 8,585 | `Muzzle_0/1` (320, -/+72, -55) chin guns; `Engine_0/1` (-332, -/+80, -13), `Engine_2` (-332, 0, -15) | yes, `DA_Enemy_Interceptor_Visuals` |
| NullBomber | 1180 x 1035 x 216 | 8,471 | `Muzzle_0` (-100, 0, -105) under the belly (mines); `Engine_0/1` (-563, -/+165, -10), `Engine_2` (-592, 0, 15) | yes, `DA_Enemy_Bomber_Visuals` |
| NullTurret | 221 x 141 x 112 | 8,819 | `Muzzle_0` (110, -11, 9), `Muzzle_1` (108, 23, 12) | yes: `TurretMesh` of `DA_Enemy_Carrier_Visuals` and of `Characters/Dominion/DA_DominionTurret_Visuals` |
| NullCarrier | 4600 x 2276 x 1785 | 8,890 | `Engine_0/1` (-1514, -/+805, 50) rear outrigger pods, `Engine_2/3` (-370, -/+1040, -290) fore pods | **imported, not wired** (`wire=False`) |

**Why the carrier is not wired.** `ATDEnemyShip::MountPart` puts the three bays 2100 cm out along +/-Y and
(-0.3, 0, 1), and the four turrets 2070 cm out on the raised diagonals, around the r 1800 gameplay sphere. The old
carrier has arms and horn pads exactly there. The Rodin hull is 1138 cm half-wide and 893 cm half-tall, so the
bays and turrets float 9-11 m off it (seen in the running game). It needs the mounts to follow the mesh (sockets
such as `Bay_0..2`, `Turret_0..3`) or a mesh built around the existing mounts. The carrier keeps `SM_Null_Carrier`.

**White eyes.** `M_TD_Ship` now separates emissive texels by saturation: coloured texels (seams, engines) take
`EmissiveTint`, near-white texels (eyes) are left untinted and take `EyeBoost`; `WhiteSharpness` (default 4) sets
how neutral a texel must be to count as white. Squad ships use tint 1 / boost 1, so they are unchanged.

| Ship | EmissiveStrength | BaseTint | EmissiveTint | EyeBoost | WhiteSharpness |
|---|---|---|---|---|---|
| NullFighter | 10 | 0.35 | (1, 0.55, 0.35) | 1.6 | 9 |
| NullGunship | 8 | 0.6 | (1, 0.55, 0.35) | 1.6 | 4 |
| NullInterceptor | 10 | 1 | (1, 0.55, 0.35) | 2.5 (the eye is dim in the texture) | 4 |
| NullBomber | 9 | 1 | (1, 0.55, 0.35) | 2 | 4 |
| NullTurret | 9 | 1 | (1, 0.55, 0.35) | 1.6 | 4 |
| NullCarrier (unwired) | 3 | 1 | (1, 0.55, 0.35) | 2 | 4 |

**Effects wiring.** `ue_rewire_enemy_fx.py` runs the enemy-visuals part of `ArtSource/EngVFX/scripts/wire_r5.py`
(`EngineTrail` = `NS_TD_Engine_Enemy` at 1 / 1 / 2 / 1.5 / 5, carrier `Explosion` scale 2.5) and checks that
`Mesh` and `MaterialOverrides` are unchanged. `NS_TD_Engine_Enemy` attaches one plume per `Engine_N` socket
(`ATDShip::GetNozzleSockets`); enemy meshes carry only `Engine_N`, no bare `Engine`.

**Capturing ships for review.** Offscreen dev-camera captures (`SceneCaptureToolset.Capture`) of a ship that is
far from the player show smeared, blotchy textures because the texture streamer only knows the player's view:
run `r.TextureStreaming 0` first. `TD.World.SpawnEnemy ... 0` ships still creep forward; `TD.Power.TimeScale 0.02`
freezes them for a set of views.

**Known limits, round 2.**
- The Rodin interceptor has the fighter's silhouette (crescent wings, three engines): the two types are hard to
  tell apart at range. It has no missile pods: it has two muzzles (the chin guns) where the old mesh had four; where its missiles leave from was not checked in play.
- The fighter's seams are near-white in the Rodin emissive, so they read white-peach, not orange.
- The review sheets for the Lancer and the fighter were re-rendered with the corrected sockets.

## Hybrids (Nov 13 night): Rift v11, Lancer v05

The close-range pass is one pipeline for the three squad ships: `ArtSource/EngShips/RiftHero/scripts/rh_hybrid.py -- --ship
<Rift|Lancer|Bastion> --tag <v> --close`, configured in `rh_ships.py` (source, sockets, accent colour, canopy / gun / nose
measurements). `--out-root <dir>` builds into another folder (a check run: the Rift's result, manifests and textures
are identical to v11 after the generalization). Outputs: `ArtSource/EngShips/<Ship>Hero/{export,review}` and
`<Ship>_hybrid_<tag>.blend`.

**Lancer v05** (524.0 x 269.5 x 149.9 cm as before; 58,451 triangles; slots `M_Lancer`, `M_Lancer_Glow`, `M_Lancer_Parts`;
3 auto convex hulls, 2 before; gameplay uses its own sphere). Sockets as in the table above **except `Muzzle_1`, now
(152, 45, -59)**: the mirror of `Muzzle_0` (Lead's decision: a clean symmetric pair). New: the canopy (a designed glass
bubble on the measured crown and hull line, as its own non-Nanite mesh `SM_Lancer_Canopy` at `Cockpit`; sills, three
bows, a coaming and a rear deck that close the cut; tub, seat, helmeted pilot, dash) and the cannons (a small receiver
set into each Rodin gun housing, two parallel barrels, the lower one on the muzzle socket's axis). The hull is cut
along straight sections (`_slice`) where new parts meet it. Kept: the Rodin prow and deck (a modelled prow failed two
fresh-eyes reviews: "a featureless cone, out of style with the hull"; its builder stays in `rh_closeparts.py`,
`nose=NEEDLE` data in `rh_ships.py`). `TD_RODIN_PLAIN=Lancer` imports the plain Rodin Lancer again.
Review (third fresh-eyes round, in-game sheets `Builds/for-ryan/ships/lancer_hybrid_before_after*.png`): REPLACE, no
BLOCKER or MAJOR. Open MINORs: the glass is paler on the bright planet pad than in space; the pilot reads as a seat
and helmet shape, no face or visor detail; the gun receivers are plain next to the Rodin belly.

**Rift v12**: v11 with three review MINORs closed (straight end to the magenta side panel under the canopy: the repaint
is tested per texel; the stray glowing sliver ahead of it cleared; the nose slit glows in the accent colour). Not
re-reviewed by a fresh agent (in-game sheet `Builds/for-ryan/ships/rift_v12_before_after.png`).

**Bastion v01** (524.0 x 472.3 x 139.1 cm as before; 52,208 triangles (51,535 was the first build, before the muzzle bore was reshaped); slots `M_Bastion`, `M_Bastion_Glow`,
`M_Bastion_Parts`; 3 auto convex hulls; sockets unchanged). The Rodin Bastion is the soundest of the three (blocky
armour, a symmetric pair of cannons, six-tube launchers), so only the two lumpy barrel ends were replaced by clean
muzzles on the same axes; the hull keeps its maps (`textures="raw"`; in the game base colour and emissive stay Grok
Bot's `textures_r2` as imported: `keep_tex`). Fresh-eyes review: REPLACE, no BLOCKER or MAJOR; MINORs: the muzzles are
plain next to the textured hull and no longer catch amber light at the tip.

`ue_import_rodin_ships.py` also sets `CollisionScrape.VFX` = `NS_TD_Ship_ImpactScrape` (VFXScale 1, sound untouched) on the
three squad visuals **once that system is in the checkout** (it prints "CollisionScrape NOT set" until then).

## Enemy fleet at close range (Nov 14): fighter and gunship v01

Same pipeline (`rh_hybrid.py --ship NullFighter | NullGunship`, config `_null()` in `rh_ships.py`): the low-poly Rodin
hulls stay exactly as they are (not welded: welding the fighter wound faces against their neighbours; Rodin's normals
and maps kept), only the lumpy parts are rebuilt: the fighter's three nozzle ends (`build_engines`), the gunship's
four turret barrels (`_guns_barrels`). Second slot `MI_Null*_Parts` takes the hull's emissive tint and range boost.
Sizes and sockets unchanged (gunship length 1500.0 cm, it was 1497.4: the new barrels end exactly on the old tips).

**Honest result** (fresh-eyes review, sheets `Builds/for-ryan/ships/fleet_{fighter,gunship}_before_after_*.png`): no
BLOCKER or MAJOR, both REPLACE, but the change is not visible from the chase camera at 20-80 m. What hurts the fleet
at that range is not lumpy geometry: (1) the engine flame sprite covers the nozzles from every rear angle; (2) the
fighter's seams break into flecks and no white eye shows from behind or the side; (3) the gunship hull is near pure
black with mirror-like reflections, its eyes show only from the front 3/4 and its accents are sparse for its size.
The next pass should be material and emissive work (rim / seam glow, roughness up, eye placement), not more modelling.

## Null fleet readability pass (Nov 14)

Materials and emissive maps only, for all five types. `M_TD_Ship` (rebuilt in place, `TD_RODIN_REBUILD=Master`; run it
with a squad ship first in `TD_RODIN_ONLY`, the master's defaults need a heat mask) has three new blocks, default off:
`PanelSteps` x `PanelCell` (value steps per cell of the pixel's local position), `RimStrength` x `RimColor` (fresnel,
added to emissive), `GlowSpread` (the emissive map sampled four mips down and added from `RangeStart` on).
Per type (`_read()` in the import script): RoughnessMin 0.42, MetallicScale 0.5, BaseFloor 0.012-0.016, PanelSteps
0.35 with cells of 0.9 m (fighter) to 5.2 m (carrier), RimStrength 0.07 small ships / 0.025 gunship and carrier,
GlowSpread 0.5 / 0.4 / 0.22 / 0.18, RangeStart 30 m, RangeEnd 240 m.
`rs_fleet_lights.py` writes `ArtSource/EngShips/FleetLights/T_<Ship>_E.png`: Rodin's emissive map with seams dilated by
2 texels at half strength (not the carrier) and dim white (0.55) running lights painted wherever the surface lies in
small boxes placed by rule on the hull's extremes (rear corners, rear top, flank stations, tail top, keel).
`TD_FLEET_READ=0` imports the old values and maps.

What two rounds taught (values above are the third set): rim 0.5-0.9 and floor 0.05-0.07 turned the hulls pale
grey-blue; rim 0.13-0.16 still read slate from behind and laid a blue sheet on the gunship's flat keel; lights of 1.3-2 %
of the hull length at full white were blown-out blobs brighter than the eyes; a spread of 0.9-2.2 washed the gunship
tan at 50-130 m.

Open (fresh-eyes review of the second set; the third set was only self-checked): from the chase camera at real
combat range the enemies are a few dozen pixels and are found by the HUD markers, so none of this changes the read
there; fighter, interceptor and bomber cannot be told apart at that size; the carrier's Rodin maps are copper / cream
rather than black (its base colour was halved, little visible change); carrier and gunship look alike in size in
isolated views. Hull art cannot fix type and facing at range: that needs HUD or silhouette work.

## Squad follow-ups and the master check (Nov 14)

- **Master rebuilt, squad unchanged:** the three squad ships captured at one fixed spot (`TD.World.Goto Start`, speed 0,
  same free cameras) with the old `M_TD_Ship` (`6fea15a3`) and the rebuilt one are the same picture
  (`Builds/for-ryan/ships/master_old_vs_new_fixed_spot.png`). A reviewer's "maroon band" on the Bastion in an earlier pair
  was the light of a different place in the level. The Dominion mothership renders with its panels and amber lines; the
  hauler convoy was not in frame in the one planet-approach capture.
- **Rift v13**, **Lancer glass v06**, **Bastion v02**: see `docs/eng/ART-HISTORY.md`. A fresh-eyes review saw Rift v13 and
  the Lancer glass (slit fixed, specks reduced, glass now dark tinted glass; pilot still hard to read); Bastion v02 was
  made after that review and is self-checked only.
- **Bastion still lags on one point** (review): its cockpit is an opaque dark windscreen with no glass depth, seat or
  pilot, where the Rift and Lancer have them. Its hard surfaces are the crispest of the three. A designed canopy was not
  attempted: the windscreen is faceted and flush with the armour, and a bubble like the Lancer's would change the design.

- **Rift v14** (crescent wings): see `docs/eng/ART-HISTORY.md`; fixed-spot and on-foot pairs in
  `Builds/for-ryan/ships/rift_v14_before_after.png`. Self-checked only.
- **Bastion glass cockpit, tried as v03 and stopped in Blender** (kept: v02). The Rodin "windscreen" is not a flush
  pane: it is a raised cab block standing in a recess with dark sloped sides. `_canopy_facets` (in `rh_closeparts.py`,
  unused) cut the dark faces into a glass part over a tub; the cab block stayed standing inside brown panes
  (`Builds/for-ryan/ships/bastion_v03_cockpit_attempt_blender.png`). A real cockpit means redesigning that block.
- **Hauler and Dominion turrets on the rebuilt master:** the convoy (`TD.World.ApproachView 1`) renders textured and lit
  (`hauler_convoy_check.png`, at a distance); the Dominion's eight turrets use `SM_NullTurret` / `MI_NullTurret`
  (`TD.World.DominionParts`), the same mesh and instance as on the carrier; they were not captured close.
