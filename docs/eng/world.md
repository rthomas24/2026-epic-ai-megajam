# World (Teammate 4): ENG-16 portal gates, ENG-17 galaxy layout, ENG-18 Dominion finale

Code: `Source/unrealGameJam/TriadDrift/World/`. Tests: `World/Tests/` (`TriadDrift.World.*`), plus
`Enemies.Carrier.*` for real-collision checks on the Dominion's generators.

## ENG-17: `ATDGalaxyLayout`

One actor builds the whole galaxy at runtime from `FTDGalaxyLayoutData` (C++ defaults in
`MakeDefaultLayout()`, world units in cm). The sandbox game mode and the real `L_Galaxy` map use the
same code.
- **Game mode entry point:** `ATDGalaxyLayout::SpawnDefaultGalaxy(this)`, a UFUNCTION. It returns
  the placed layout or spawns one. Layout data, `FindReturnPoint` and `GetPlayerStart` are valid
  right after spawn, before BeginPlay.
- **Combat zone at the origin:** an asteroid belt with a clear pocket 14 000 cm across, a denser
  flank field, and 6 station-ruin blocks as cover. The asteroids and ruins are instanced, with
  collision.
- **Planets:** `ATDPlanetAtmosphere` for Verdance (450 000, 260 000, 0) and Cinder (−420 000,
  330 000, 40 000), each with an approach asteroid field.
- **Return points:**
  - `Verdance_Orbit` and `Cinder_Orbit` sit at about 1.3× the atmosphere radius, facing the combat
    zone (outside the trigger, facing away from the planet).
  - `Dominion_Hangar_Exit` is 45 000 cm in front of the Dominion's nose.
  - The player start is (−25 000, 0, 0).
- **Dominion:** spawned at (0, −520 000, 20 000), nose toward the combat zone.
- **Gate pairs:**
  - Zone ↔ Verdance orbit and Zone ↔ Cinder orbit are always open.
  - The flank pair West ↔ East is open in Waves3to4, Wave5 and DominionAssault. The layout checks the
    phase once a second.
- **Play bounds:** centre (0, −50 000, 0), radius 900 000 cm. The layout stores them in
  `UTDWorldSubsystem::SetPlayBounds` and pushes `SetBoundary` onto every spacecraft in the registry
  every 2 s (this switches to T1's `UTDTravelSubsystem::SetPlayBounds` when it lands; see gaps).
- **Obstacles:** every asteroid, ruin, planet and the Dominion hull is registered in
  `UTDWorldSubsystem`. Its `FindAvoidance(Origin, Dir, Probe, Radius, OutDir)` drives enemy obstacle
  avoidance, and T3 uses it for wingmen. Gate frames and moving ships are not in it.

### `ATDPlanetAtmosphere`
- **Shape:** a solid placeholder planet with a larger trigger. The trigger overlaps every channel,
  including Ship; with the default response it would block.
- **Open/closed:** it opens in its planet's phase (or on `OnMarkerRevealed`) and closes once the
  story has moved past that phase.
- **Entry:** when the player-controlled ship enters (overlap, or a per-frame distance check as
  backup), it fires `OnShipEnteredAtmosphere`. Then, after `EntryDelaySeconds`,
  T1's `GetAtmosphereEntry()->BeginEntry(Destination, ReturnPoint, Center, PlanetRadius)`, with
  `Flow->TravelTo(Destination, ReturnPoint)` only as a fallback for a ship without the component.

### Art hooks on the layout (for Nick)
- `GateClass`, `PlanetClass`, `DominionClass`: BP subclasses to spawn instead of the C++ classes.
- `TravelFX` for every gate; empty = the convention asset `/Game/TriadDrift/Presentation/DA_Travel_FX`.
- `AsteroidMesh`, `RuinMesh`, `GateFrameMesh`: art meshes that replace the placeholders (collision kept).
- **Editor preview:** a placed ATDGalaxyLayout builds its asteroid belt and ruins in the editor and
  draws planets (solid + atmosphere), gate pairs, the Dominion box, return points (arrows) and the
  play bounds, so the shared map can be dressed around the real gameplay positions.
- **Keep-out:** no asteroid within `AsteroidKeepOut` (7000 cm) of a gate, return point or the player
  start. Asteroids are uniformly scaled, so the rock you see is the sphere you hit.

### Nebula sky and space lighting (art audit, Nov 13)
- `ATDSkySphere` (World/TDSkySphere): the builder's `SM_TD_SkySphere` + `MI_TD_NebulaSky`
  (`/Game/TriadDrift/Gameplay/Props/Sky/`, README in `ArtSource/EngProps/Sky/`), 100 km radius, re-centred on the
  camera every frame (post-update), unrotated, no collision, no shadow. Tagged `TD.Engineering.Sky`; hides while
  an actor tagged `TD.Presentation.Sky` (Nick's) exists. The layout spawns it (`bSpawnSky`) when the art exists;
  the space game mode then skips the placeholder star backdrop.
- The one sun tunable: `ATDSkySphere::SunYawDeg` / `SunPitchDeg` (35 / 18, from the bake). The placeholder
  space sun uses `SunLightRotation()` (Pitch -18, Yaw 215) and is the atmosphere sun light; each planet sets its
  material's `SunDirection` from that light.
- Space post: manual exposure, bloom 0.8 with threshold 1.5 (only emitters bloom; no whiteout), motion blur 0.2,
  a soft shadowless violet fill (1.2 vs the sun's 8).
- The player start (250 m out) is turned to yaw 10: the opening frame holds the sun and the magenta nebula.

### Space dressing (Nov 13, the SpaceDressing kit)
- Everything is instanced, visual only (no collision), Static, culled like the belt; `bSpaceDressing` turns it off,
  `TD.World.Dressing 0|1` toggles it live (dev builds).
- Belt variety: each `FTDAsteroidFieldSpec` has `IceShare`, `MetalShare`, `CrystalShare`, `bTealCrystal`; kit rocks
  join `AsteroidLooks`, normalised so their bounding sphere matches the collision sphere. Combat belt: metal + a few
  violet crystals; Verdance approach: ice + teal crystals; Cinder approach: metal + violet crystals.
- Ruins: truss art (`SM_StationTruss_01` girders, `_03` hub modules, `_02` panels on some girders) over the cube
  collision boxes, which stop drawing; 2-3 debris clusters around each ruin. Nick's `RuinMesh` wins.
- Landmarks: a satellite pair in high orbit around each planet; cargo pod clusters at a third and two thirds of each
  lane (zone to planets and the Dominion); beacon buoys lining both sides of every gate approach (300 m apart).
- Lane rocks: `LaneRocksPerLane` (70) small kit rocks in a hollow tube along every lane (zone to each planet, gate
  end and the Dominion); `LaneClearRadius` (90 m) keeps the lane itself open.
- Never: in the dogfight pocket (`CombatZoneRadius`), at the player start / return points, in a gate opening or its
  approach (`TriadDrift.World.Layout.SpaceDressingFillsButNeverBlocks`). Cost measured: about +1 ms GPU at the start
  view (offscreen 1080p).

## ENG-16: `ATDPortalGate`
- **Transit:** a permanent two-way pair built on T2's shared `UTDPortalTransit::TransitActor`, so
  ships, enemies and projectiles follow the same rules as Rift's portals:
  - ships keep their speed and get 0.3 s of invulnerability
  - enemies lose tracking for 2 s
  - enemy shots are returned to the Player team
- **Detection:** a swept plane crossing each tick, over registry spacecraft and
  `UTDProjectileSubsystem` live projectiles. It works at hyperdrive speed and for collision-less shots.
- **Facing:** entering along the gate's +X exits along the partner's +X, and entering from the back
  exits out of the partner's back. Re-entry cooldown is 0.5 s.
- **Placeholder look:** a square frame of 4 bars (with collision) and a disc that's hidden when the
  gate is closed. `SetGateActive` broadcasts `OnPortalState(Open/Closed)`.
- **Presentation:** `TravelFX->PortalIdle` (attached loop; when its VFX is set the placeholder disc
  hides) and `PortalTransit` (at both ends). `FrameMeshOverride` replaces the four bars (collision kept).
- **Not a reflector:** permanent gates pass shots through unchanged (no return, no stat, no highlight).
- **Arrival guard:** ships teleported to within 6000 cm of a gate's plane (follow-warps, transits out of
  the partner) ignore that gate for 1.5 s, so wingmen don't bounce back.

## ENG-18: `ATDDominion` (core space finale)
- **Arrival:** hidden until progress has `bDominionRevealed` (set after wave 5; it polls twice a
  second), then a 6 s warp-in.
- **Hull:** a 600 m placeholder hull with a bridge and a trench, built from engine cubes. The hull
  actor is Neutral, so it blocks every team's shots (the trench is real cover). With a HullMesh in
  the visuals, the placeholder bridge and trench-wall visuals hide (their collision stays); with a
  ShieldIdle VFX, the emitter towers hide. Parts are laid out in OnConstruction, so a placed Dominion
  looks right in the editor.
- **Shield phase:**
  - 4 `ATDShieldGenerator`s (800 HP) sit on pylons outside the 360 m main shield. Each has 2
    turrets, plus 2 more on the hull: 10 telegraphed charge-shot turrets (Enemy_HeavyBolt).
  - Fighter groups launch from the hangar on a timer.
- **Main shield:** the reactor's `DamageFilter` blocks all damage, and the reactor is untargetable,
  until every generator is down.
  - Each generator kill hides one shield emitter and fires `OnShieldSectionDown`.
  - The GeneratorKill money shot comes from T3's Director (generators are tagged).
- **Defenses down:** `SetDominionState(true,true)` and the ShieldCollapse cue. The trench opens, and
  4 dormant trench turrets wake up.
- **Reactor:** `ATDDominionReactor` has 3 phases × 1200 HP.
  - A single hit can't skip a phase, and each phase change has a 1.5 s invulnerable beat.
  - Each phase escalates via `FTDDominionStage`:
    - launch interval 15 → 12 → 9 → 6 s
    - turret pause 2.5 → 1.0 s
    - telegraph 1.2 → 0.8 s
- **Victory:** the reactor kill (ReactorKill via the Director tag) quiets the turrets and fires
  `OnBreakupStarted` with the BreakupSequence cue. Then, after `BreakupSeconds` (8 s of game time,
  stretched by the slow-mo beat), `Flow->SetPhase(Victory)` runs exactly once. Victory is a phase
  on L_Galaxy with no map (T5). From the reactor kill on, the squad is invulnerable and every enemy
  ship and live shot is removed.
- **Resume:** with `bDominionDefensesDown`, the Dominion goes straight to the exposed reactor. That
  path triggers no kills, highlights or duplicate `SetDominionState`.
- **Stretch ENG-29:** `bDockingFinale` plus the virtual `BeginDockingFinale()`. The base version
  logs and falls back to the reactor ending.
- **Presentation:** one `UTDDominionVisuals` asset at the convention path
  `/Game/TriadDrift/Presentation/DA_Dominion_Visuals` (or set `Visuals`). Slots used:
  HullMesh, GeneratorMesh, ReactorMesh, WarpIn, ShieldIdle (loop while shields are up), ShieldHit
  (blocked reactor hits), ShieldSectionDown, ShieldCollapse, GeneratorHit, GeneratorExplosion,
  ReactorExposed, ReactorHit, ReactorPhaseChange, ReactorDestroyed and BreakupSequence.
  DockingBayOpen is for the stretch finale.

## How to try it
- **Tests:** `.\Tools\td-test.ps1 -Filter TriadDrift.World`.
- **Sandbox:** ATDGalaxyLayout is spawned by T5's game mode once it's on main.
- **Reaching the finale fast:** use `TD.Phase DominionAssault`, or skip waves until the Dominion
  reveal. `ATDDominion::DebugDropShields()` kills the generators.

## Known gaps
- **Done with T1's Travel/:** bounds via `UTDTravelSubsystem::SetPlayBounds`, gravity wells at 1.8×
  the atmosphere radius (planets) and about 60 000 cm (Dominion approach), entry via `BeginEntry`.
- **Not flown by hand:** the layout scale, asteroid density and the Dominion's size and turret
  pressure need a PIE pass.
- **No shield bubble geometry:** the main shield has no physical bubble. It's a damage filter plus
  Nick's ShieldIdle cue, because an opaque placeholder sphere would hide the fight.

## Placeholder look and dev commands
- **Placeholder colours:** every placeholder mesh goes through `UTDStatics::TintPlaceholder` (lit for
  armour, emissive for weak points). Enemies: lit red/orange hull, dark wings, white emissive engine
  "eyes"; per-role hull shape (interceptor long and thin, gunship wide, bomber fat, carrier long and
  flat). Bays glow orange, generator cores and Dominion emitters glow cyan, the reactor glows gold.
  Real art replaces all of it through the `DA_*_Visuals` assets.
- **Dev console commands** (`TDWorldDebug.cpp`, not in Shipping):
  - `TD.World.Goto <Dominion|Reactor|Verdance|Cinder|Gate|Belt|Start>` teleports the player's ship.
  - `TD.World.SpawnEnemy <Id> [distance cm] [pilot 0/1]` spawns an enemy ahead (unpiloted holds still).
  - `TD.World.Dominion <Reveal|DropShields>` drives the finale for captures.
