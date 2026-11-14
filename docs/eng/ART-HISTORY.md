# Art history: how to go back to an older look

Ryan, Nov 13: "make sure you keep copies of old things you get rid of for art in case we want to go back at any
point." This file is the index. Nothing listed here has been deleted: every version is a commit on `main`, and the
older looks that the game can still show are listed with the switch that brings them back.

## The rule (every agent and builder on engineering's side)

1. **Never delete or overwrite an art asset that is not in git yet.** Commit the old version first, then replace it.
   The Lead does the commit; a builder that is about to replace something says so and waits for it.
2. **Keep the source.** Every asset keeps its `.blend`, scripts and review sheets under `ArtSource/Eng*/**`, with one
   review sheet per round (`*_rN_*.png`), so any round can be rebuilt from its script.
3. **A replaced look stays reachable** in one of three ways, in this order of preference:
   - a setting or console variable that switches it back on (the code-built placeholder shapes, the HUDs);
   - a tag on `main` (`art/<name>`), for a whole-game snapshot;
   - the commit in the table below, restored with `git checkout <commit> -- <path>` on a branch.
4. **Add a row here** whenever a look is replaced: what, the commit that holds the old version, how to see it again.
5. A large binary that is replaced many times (a `.blend`, a texture set) keeps every committed version in Git LFS;
   nothing is pruned before the jam ends.

## Snapshots (tags on `main`)

| Tag | Commit | What the game looked like |
|---|---|---|
| `art/before-overnight-nov13` | `ce54338` | Before the night of Nov 12-13: placeholder ships and Dominion, flat planets, no terrain kit. |
| `art/morning-nov13` | `fd0cb7a` | What Ryan saw on the morning tour: engineering ships, fleet, Dominion r9, planet kit round 2, ground anti-tiling, magma, old (bulky) HUD, flat-glow effects, flat planet ground. |

Restore a whole snapshot in a separate checkout: `git worktree add ../old art/morning-nov13`.

## Looks that can be switched back on in the current game

| Look | How |
|---|---|
| Code-built placeholder ships, enemies, Dominion, crew | Remove or rename the matching `DA_*_Visuals` under `Content/TriadDrift/Gameplay/Characters/**`; the code falls back to its shapes. |
| Nick's HUD (`WBP_SpaceHUD`), the developer HUD | The HUD choice in `TDUIHostSettings` (`Config/DefaultGame.ini`); the developer HUD also with `td.DebugHUD 1`. Both stay selectable after the ENG-36 redesign. |
| Flat-glow placeholder effects (trails, flashes, explosions) | They remain the fallback whenever a cue has no Niagara effect (ENG-37): clear the cue's `VFX` field. |
| Space dressing off | `TD.World.Dressing 0`. |
| Menu backdrop off | `td.MenuBackdrop 0`. |
| Intro off | `td.Intro 0`. |

## Versions by asset (each row's commit contains that version; the row above it is the newer one)

| Asset | Version | Commit | Path |
|---|---|---|---|
| Null Armada fighter and gunship | Hyper3D Rodin meshes from Grok Bot (current; not committed yet when this row was written: the Lead's commit of `Content/TriadDrift/Gameplay/Ships`) | Lead's commit | `Content/TriadDrift/Gameplay/Ships/{NullFighter,NullGunship}`, `ArtSource/EngShips/{Rodin,NullFighter,NullGunship}`. The old meshes `SM_Null_Fighter` / `SM_Null_Gunship` are still in `Content/TriadDrift/Gameplay/Characters/NullFleet`, only the `Mesh` and `MaterialOverrides` fields of `DA_Enemy_{Fighter,Gunship}_Visuals` changed (old data assets: `f77d40a`). Switch back: `TD_RODIN_REVERT=NullFighter,NullGunship` + `ArtSource/EngShips/Rodin/scripts/ue_import_rodin_ships.py`, or re-run `ue_import_null_fleet.py` |
| Null Armada interceptor, bomber | Hyper3D Rodin meshes from Grok Bot, fleet r3 (current; not committed yet when this row was written) | Lead's commit | `Content/TriadDrift/Gameplay/Ships/{NullInterceptor,NullBomber}`, `ArtSource/EngShips/{NullInterceptor,NullBomber}`. The old meshes `SM_Null_Interceptor` / `SM_Null_Bomber` are still in `Content/TriadDrift/Gameplay/Characters/NullFleet`; only `Mesh` and `MaterialOverrides` of `DA_Enemy_{Interceptor,Bomber}_Visuals` changed. Switch back: `TD_RODIN_REVERT=NullInterceptor,NullBomber` + `ArtSource/EngShips/Rodin/scripts/ue_import_rodin_ships.py` (puts back the old mesh with its `MI_Null_*` override) |
| Null Armada turret (carrier and Dominion turrets) | Hyper3D Rodin mesh from Grok Bot, fleet r3 (current; not committed yet when this row was written) | Lead's commit | `Content/TriadDrift/Gameplay/Ships/NullTurret`, `ArtSource/EngShips/NullTurret`. Old `SM_Null_Turret` is still in `Characters/NullFleet`; only the `TurretMesh` field of `DA_Enemy_Carrier_Visuals` and `Characters/Dominion/DA_DominionTurret_Visuals` changed. Switch back: `TD_RODIN_REVERT=NullTurret` + the same script |
| Null Armada carrier | engineering mesh `SM_Null_Carrier`, drawn with its own `MI_Null_Carrier` (current). The Rodin carrier is imported (`Content/TriadDrift/Gameplay/Ships/NullCarrier`) but NOT wired: the bays and turrets are mounted by code around the r 1800 sphere and float 9-11 m off the slimmer Rodin hull | Lead's commit | To try the Rodin carrier: `wire=True` on `NullCarrier` in `ue_import_rodin_ships.py`; back: `wire=False` or `TD_RODIN_REVERT=NullCarrier` |
| Null Armada fleet | engine glow pass (still current for the interceptor, bomber, carrier, bay and turret) | `7b6305c` | `Content/TriadDrift/Gameplay/Characters/NullFleet`, `ArtSource/EngShips/NullFleet` |
| | first version | `b603476` | same |
| Squad ships (Rift, Lancer, Bastion) | Hyper3D Rodin meshes from Grok Bot (current; not committed yet when this row was written) | Lead's commit | `Content/TriadDrift/Gameplay/Ships/{Rift,Lancer,Bastion}`, `ArtSource/EngShips/{Rodin,Rift,Lancer,Bastion}`. The old meshes `SM_Squad_*` are still in `Content/TriadDrift/Gameplay/Characters/Squad`; only the `Mesh` and `MaterialOverrides` fields of `DA_{Rift,Lancer,Bastion}_Visuals` changed (old data assets: `f77d40a`). Switch back: `TD_RODIN_REVERT=Rift,Lancer,Bastion` + `ArtSource/EngShips/Rodin/scripts/ue_import_rodin_ships.py`, or re-run `ue_import_squad.py` |
| | r7 colour blocking (code-built engineering ships) | `3270de4` | `Content/TriadDrift/Gameplay/Characters/Squad`, `ArtSource/EngShips/Squad` |
| | r6, mostly white hulls | `b96f8bd` (+ `86256b2` socket fix) | same |
| Dominion exterior | glow strengths for the running game (current) | `3a6c3ff` (+ Trim `bf5394b`) | `Content/TriadDrift/Gameplay/Props/Dominion`, `ArtSource/EngShips/Dominion` |
| | r9 glow pass | `688c979` | same |
| | r8 silhouette pass | `31e6121` | same |
| | first kit (slab-like) | `f98124f` | same |
| Dominion generator, reactor | first version (current) | `c9f541a` | same |
| Dominion turrets | Null Armada turret mesh (current) | `bf5394b` | `Content/TriadDrift/Gameplay/Characters/Dominion` |
| Planet set-piece kit | round 2 + altar + prototype console (current) | `db8d405` | `Content/TriadDrift/Gameplay/Props/Planet`, `ArtSource/EngProps/PlanetKit` |
| | round 1 (26 pieces) | `a94988c` | same |
| Cinder magma surface | first version (current) | `d3a4643` | same |
| Planets from space | self-lit with night side (current) | `219057b` | `Content/TriadDrift/Gameplay/Props/Planets`, `ArtSource/EngProps/Planets` |
| Planet terrain materials | `M_TD_TerrainR2` (environment pass, Nov 13; current on `eng/env`): Grok Bot's PBRgen sets at 1K, colour = tint x texture detail, four-scale procedural variation, layers by slope / height in metres, wet low ground, river band, bioluminescent veins and patches | `eng/env` commit "env: terrain R2" | `Content/TriadDrift/Gameplay/Props/Planets/{M_TD_TerrainR2, T_Terr_*, MI_Terrain_Verdance, MI_Terrain_Cinder}`, `ArtSource/EngEnvironment/scripts`, sheets in `ArtSource/EngEnvironment/review`. Only the two instances changed (new parent and values); `M_TD_Terrain` and its textures are untouched. Switch back: `TD_TERRAIN_R2_MODE=revert` + `ArtSource/EngEnvironment/scripts/ue_terrain_r2_build.py` (puts both instances back on `M_TD_Terrain` with r1's values), or take the two `MI_Terrain_*` files from `a101603` |
| | `M_TD_Terrain` r1: slope rock, peaks, lava valleys, procedural textures (current) | `a101603` | `Content/TriadDrift/Gameplay/Props/Planets`, `ArtSource/EngProps/Planets` (`review/terrain_r1_*.jpg`); before it the terrain wore `MI_Ground_*` |
| Planet sky, light, fog and exposure (placeholder sky) | environment pass (current on `eng/env`): higher sun, fill from the far side, fixed exposure, two-layer fog (valley mist + aerial haze from 1.5 km / 1.2 km) | `eng/env` commit "env: terrain R2" | `Source/unrealGameJam/TriadDrift/Flow/TDPlaceholderSky.cpp` (`ApplyStyle` Verdance / Cinder blocks, `ApplyHorizon`). Before: sun 9 deg up, fill on the sun's side, auto exposure bias -0.7 / -0.4, one fog layer (density 0.03 / 0.035, falloff 1.8, start 250 m): `git show 5a6cb48:Source/unrealGameJam/TriadDrift/Flow/TDPlaceholderSky.cpp` |
| Lighting and grade per level (ART-05 / ART-13) | environment builder, Nov 14 (current on `eng/env`): a colour grade per style on its own post-process component (`Grade`, priority 20: galaxy and menu, Verdance, Cinder, the Dominion interior); Verdance teal-violet dusk (sun -15 deg, rose, violet sky, exposure -0.3), Cinder ember dusk (sun -16 deg, redder, exposure -0.2), galaxy fill 2.0 and bloom 0.7 / threshold 1.8 | `eng/env` commit "env: lighting and grade ..." | `Source/unrealGameJam/TriadDrift/Flow/TDPlaceholderSky.cpp` (`TDLook`, `ApplyStyle`, `ApplyHorizon`; old values are in the comments next to each change); `TD.Env.Look neutral 1` shows the ungraded look live; sheets `ArtSource/EngEnvironment/review/g3_*` |
| Planet globe bake palette (Verdance) | matches the R2 ground: teal / emerald, slate cliffs by slope, tinted lichen, cyan / violet night glow (current on `eng/env`) | `eng/env` commit "env: terrain R2" | `Source/unrealGameJam/TriadDrift/World/TDGlobeBake.cpp` (`TDGlobeBake_Shade`, Verdance branch). Before (white snow, grey rock by height): `5a6cb48`; `td.PlanetGlobeBake 0` still shows the art globe |
| Planet terrain materials, R3 | `M_TD_TerrainR2` graph R3 (environment builder, Nov 13 evening; current on `eng/env`): texture detail gone by ~550 m (the far tile read as a grid), repeating 37 m / 230 m noises only near, biome regions from the broad mask and height (dark rainforest floor, moss, bright open meadow, paler uplands, dark wet lowlands), groves at the 1.9 km scale, horizontal strata on cliffs | `eng/env` commit "env: terrain R3 ..." | Same assets as R2; script `ArtSource/EngEnvironment/scripts/ue_terrain_r2_graph.py`. R2's look: the three `M_TD_TerrainR2` / `MI_Terrain_*` files from `5af18d02` |
| Planet atmosphere shell (galaxy globes) | `M_TD_PlanetAtmosphere` with a camera-distance fade (environment builder, Nov 13; current on `eng/env`): the teal rim shell no longer fills the screen on the dive | `eng/env` commit "env: ..." | `Content/TriadDrift/Gameplay/Props/Planets/{M_TD_PlanetAtmosphere, MI_PlanetAtmo_*}`, script `ArtSource/EngEnvironment/scripts/ue_atmo_fade_build.py`. Before: the three files from `5af18d02` (built by `ArtSource/EngProps/Planets/scripts/ue_import_planets.py`) |
| Planet globe surface close detail | `M_TD_Planet` + two noise octaves in the globe's UV that fade in within ~2 radii (`DetailStrength`, `DetailTint`) | `eng/env` commit "env: ..." | `ArtSource/EngProps/Planets/scripts/ue_planet_surface.py` (+ `ue_rebuild_planet_surface.py`). Before: `M_TD_Planet` / `MI_Planet_*` from `5af18d02`; `DetailStrength 0` on an instance turns it off |
| Planet cloud deck | `ATDCloudDeck` + `M_TD_CloudDeck` (new, environment builder; current on `eng/env`) | `eng/env` | `Source/unrealGameJam/TriadDrift/World/TDCloudDeck.*`, `Content/TriadDrift/Gameplay/Props/Sky/*CloudDeck*`; `td.CloudDeck 0` = the look before it |
| Dominion interior materials | `M_DI_AtlasLit` (darker, glossier floors, wall tint, per-tile variation), cool core glow, cooler conduit cells, cool thin reactor shield (environment builder; current on `eng/env`) | `eng/env` | `Content/TriadDrift/Gameplay/Props/DominionInterior/{MI_DI_Atlas, MI_DI_CoreGlow, MI_DI_Cell, MI_DI_Shield, Materials/M_DI_AtlasLit}`. Switch back: `TD_DI_LOOK_MODE=revert` + `ArtSource/EngEnvironment/scripts/ue_di_look_build.py` (kit README values), or the four `MI_DI_*` files from `5af18d02` |
| Planet terrain shape | M2: 2.5 km hero peaks, regions, skyline (current on `lead/int`) | `e77e701` | `Source/unrealGameJam/TriadDrift/World/TDTerrainField.*` (code; `td.PlanetTerrain 0` brings back the flat slab) |
| | M1: first procedural mountains | `71d11c5` | same |
| Niagara effects (ENG-37) | tier 1 round 2, unwired | `02bfcbc` | `Content/TriadDrift/Gameplay/VFX`, `ArtSource/EngVFX` (`review/tier1_r1_*`, `tier1_r2_*`, `rounds/r2/`) |
| Fallback sound set (ENG-35) | first version (current) | `31f2120` (+ `7f2c175`) | `Content/TriadDrift/Gameplay/Audio`, `ArtSource/EngAudio` (recipes rebuild every sound) |
| HUD | ENG-36 flight HUD P1 (in progress, not default) | T2 `9754f24` | spec `docs/eng/hud-redesign.md`; Nick's `WBP_SpaceHUD` and the developer HUD stay selectable |
| AI concepts and terrain texture sets (Grok Bot) | r1 / r2 per folder, never overwritten | `20060b1` | `ArtSource/EngAI/**` |
| Planet ground | anti-tiling (current) | `44d790a` | same |
| Crew (trooper, grunt, heavy) | centimetre rig on `SKEL_NullCrew` (current) | `5dc659f` | `Content/TriadDrift/Gameplay/Characters/{NullTrooper,NullGrunt,NullHeavy,Crew}` |
| | mannequin-skeleton versions | `17e0437` and earlier | same |
| Pilot blade clips (light 1-3, heavy, dash, hit react front / back) | Grok Bot HY-Motion set, retargeted (current on `lead/int`; not committed at hand-over) | Lead's commit | `Content/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_{Light1,Light2,Light3,Heavy,Dash,HitReact_Front,HitReact_Back}`; sources `A_Pilot_Blade_*`; scripts `ArtSource/EngAnim/PilotBlade` |
| | template-mannequin boxing and rifle versions (the re-timed `*_v2` copies are kept too) | `1050b21` and earlier | same folder, as `A_Pilot_{Light1,Light2,Light3,Heavy,Dash,HitReact_Front,HitReact_Back}_v1`; to go back, copy `_v1` over the name |

Earlier versions of anything: `git log --oneline -- <path>`. Local review renders and capture sets that are not in
git (`Saved/TDShots/**`, `Saved/*/` builder scratch) are kept on Ryan's machine and are not cleaned up.
