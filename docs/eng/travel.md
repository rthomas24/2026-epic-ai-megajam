# Travel: hyperdrive, atmosphere, landing (ENG-15, ENG-20)

Owner: Teammate 1. Code: `Source/unrealGameJam/TriadDrift/Travel/`. Tests: `TriadDrift.Travel.*`.

## What exists

| Class | What it does |
|---|---|
| `UTDTravelSubsystem` (world subsystem) | Gravity wells (`RegisterGravityWell(Id, Location, Radius, Owner)`, `UnregisterGravityWell`, `FindWellContaining`, `FindWellOnSegment`), world play bounds (`SetPlayBounds` / `ClearPlayBounds`), landing-pad registry, and the `OnShipLanded` / `OnShipTookOff` events |
| `ATDGravityWellMarker` | Placeable well (`WellId`, `Radius`, `SetWellEnabled`); registers itself and follows its actor |
| `UTDHyperdriveComponent` (on `ATDShip`) | ENG-15 hyperdrive; see below |
| `UTDAtmosphereEntryComponent` (on `ATDShip`) | Atmospheric entry, called by T4's galaxy `ATDPlanetAtmosphere`: `BeginEntry(Destination, ReturnPoint, PlanetCenter)` |
| `ATDAtmosphereFlightZone` (planet maps) | Every squad ship inside its box flies in atmosphere mode, with soft walls and a ceiling; the ceiling exit leads back to the galaxy |
| `ATDLandingPad` | A pad: beacon, prompt range 12000 cm, "Hold F to land", wingman pads (`bWingmanPad`, `SidePads`), reservation |
| `UTDLandingComponent` (on `ATDShip`) | Auto-landing and take-off state machine, with the state Nick animates from |

### Hyperdrive (ENG-15)

**States:** Idle → Spooling → Jumping → DropOut → Idle. The player-flown ship also broadcasts each state on `UTDEventBus::OnHyperdriveState`; T3's wingmen form up on Spooling and ride along above 60,000 cm/s.

**Spooling:**
- Hold H / D-pad down for 1.2 s. Releasing early cancels.
- Can't start within 2 s of taking damage, in atmosphere, inside a gravity well, during the 1 s cooldown, or while disabled.
- A hit while spooling cancels it.

**Jumping:**
- 80,000 cm/s at 10% steering, through `SetSpeedOverride`. It accelerates at 120,000 cm/s².
- Pressing H again drops out (after 0.5 s in the jump).

**Automatic drop-out** happens:
- inside any registered gravity well (checked along the path flown, so 80k can't skip a well)
- at the play-space warning band
- on impact
- after 12 s

**DropOut:** brakes hard (400,000 cm/s²) to a 15,000 cm/s burst for 0.6 s, then the flight model bleeds back to throttle speed.

**Read-outs:** `GetCharge01()` for the HUD (T3 returns the active ship's), `GetBlockReason()` for tips, `GetLastDropOutWell()`.

**Cues:** `UTDTravelFX` `HyperdriveSpool` (attached loop), `HyperdriveTunnel` (attached loop) and `HyperdriveDropOut`.

### Atmosphere (ENG-20)

**Entry (galaxy → planet, features.md §7 steps 2–3):**
- T4's `ATDPlanetAtmosphere` calls `Ship->GetAtmosphereEntry()->BeginEntry(...)` when the player ship enters.
- The ship is committed for 2.5 s: the nose turns into the planet if it was grazing away, speed is held at 9,000 cm/s with 35% steering, the hyperdrive is off and the `AtmosphericEntry` cue plays.
- Then `Flow->TravelTo(Destination, ReturnPoint)` is called. With no game flow, or if travel fails, control is handed back.

**Atmosphere flight mode** (flight sim):
- gravity 1,200 cm/s² (grip makes the ship sag rather than fall)
- speed capped at 60% of space cruise
- no boost and no hyperdrive
- altitude from `ATDSpacecraft::GetAltitudeMeters()`
- The zone also switches the player controller to `IMC_Atmosphere` (F / A = Land).

**Zone:**
- `ATDAtmosphereFlightZone` is a box; its walls and ceiling are soft boundaries.
- The exit opens when all `RequiredObjectives` are in `FTDProgress::CompletedObjectives`, or when `SetExitAllowed(true)` is called.
- When it's open, climbing 3,000 cm above the ceiling calls `TravelTo(Galaxy, "<PlanetId>_Orbit")`.
- While it's closed, the ceiling pushes back and `ShowTip(CeilingBlockedTip)` fires once per approach.

### Landing and take-off (ENG-20, §7 steps 6, 7, 9 and 10)

**When landing is offered:**
Rule decided by the Lead on Nov 12, replacing 30 m / 40% of space cruise:
- The pad's beacon is within 120 m (`ATDLandingPad::PromptRange`).
- The speed is at or below 60% of the plain top speed (`MaxSpeedFraction`). On a planet that is the atmosphere
  cap, 3,600 → 2,160 cm/s; in space it is cruise.
- The hyperdrive is idle.
- When all three hold, `GetLandPrompt()` returns "Hold F to land".
- Within `SlowDownPromptRange` (400 m) of a pad but too fast, it returns "SLOW DOWN TO LAND - HOLD SPACE / LT TO
  BRAKE", so a player who never brakes is told why.
- This is the text `ITDHUDData::GetInteractPrompt` should carry in flight.
- Measured: a hands-off arrival from the descent start is told to slow down. Braking from that first prompt,
  touchdown follows 17.8 s later, most of it crawling at brake speed.
- Hold F / A for 0.5 s to call `RequestLand`.

**Landing sequence:**
1. Aligning, 1.5 s: fly to the hover point 900 cm above the pad and turn to the pad's facing.
2. GearDown, 0.6 s.
3. Descending, 1.2 s.
4. Touchdown, 0.3 s: the `LandingDust` cue plays.
5. EnginesOff, 0.5 s.
6. CanopyOpening, 0.8 s.
7. **Landed**: `UTDTravelSubsystem::OnShipLanded(Ship, Pad)` fires and T5's first person takes over.

The flight model is parked throughout, so controller and AI input is ignored.

**Take-off** (`RequestTakeOff`, called by T5 on re-board):
1. CanopyClosing.
2. EnginesOn.
3. Ascending, 1.6 s: climb 1,500 cm above the hover height. The `TakeOff` cue plays.
4. GearUp.
5. Flying again at 40% of cruise, the pad is released, and `OnShipTookOff` fires.

**Wingmen:**
- Squadmates within 600 m land on the pad's `SidePads`, or the nearest wingman pads, at the same time.
- They take off with the player.
- T3's `SetWingmenPassive(true / false)` is called around this, compiled in automatically when present.

**State for Nick's animations:** `GetLandingState()`, `IsGearDown()`, `IsCanopyOpen()`, `AreEnginesOn()`, and `GetLanding01()`, which runs 0 → 1 through landing and 1 → 0 through take-off.

**Checkpoints:** `SnapLanded(Pad)` puts a ship straight onto a pad, in the Landed state.

**Exit point:** `GetExitTransform()` is where the pilot steps out and where re-boarding is offered. It sits 320 cm to the ship's right and 96 cm (capsule half height) above the deck, facing the ship's forward. Tune it with `ExitSideOffset` and `ExitHeightAboveDeck`.

**Agreed with T5** (docs/eng/ground.md, "Planet arrival and hand-off"):
- **Fresh arrival:** the squad spawns flying at `PlayerStart_Descent`, and the zone takes over.
- **Checkpoint or retake after landing:** `SnapLanded`, with the pilot on foot.
- **On `OnShipLanded`** for the player-flown ship: T5 spawns and possesses the first-person character at `GetExitTransform()`.
- **Re-board:** T5 possesses the ship and calls `RequestTakeOff`.

### Planet travel rig and debug commands
**Rig:** `ATDPlanetTravelRig::SpawnForPlanet(World, PlanetId, PadTransform, RequiredObjectives)` (or a placed rig) sets up a planet map in one call. It spawns:
- the flight zone
- the player pad with two linked wingman pads
- a placeholder ground slab, until the environment has ground

`GetDescentStart()` gives a fresh-arrival transform: high and behind the pad, facing it.

**Entry safety:** `BeginEntry(..., PlanetSurfaceRadius)` limits the committed dive so it stays `EntrySurfaceClearance` (3,000 cm) above the planet body, and the ship is invulnerable while committed.

**Dev console commands** (not in shipping builds; `Travel/TDTravelDebug.cpp`):

| Command | Does |
|---|---|
| `TD.Travel.ToPlanet [Verdance]` | opens that planet and puts the player ship at its atmosphere edge, nose in |
| `TD.Travel.Land` | hops next to the rig's player pad and auto-lands |
| `TD.Travel.TakeOff` | re-possesses the landed player ship and takes off |
| `TD.Travel.Exit` | opens the ceiling and climbs out |
| `TD.Travel.Status` | logs the player ship's travel state |

Scripted run: `td-shot.ps1 -Map /Game/TriadDrift/Maps/L_Galaxy -ExecAt '4:TD.Travel.ToPlanet','7:TD.Travel.Status'`. Use command names without spaces or quotes; quoted `-ExecAt` arguments currently fire at 0 s.

## How to try it

```powershell
.\Tools\td-test.ps1 -Filter TriadDrift.Travel
```

By hand in the sandbox:
- Hold H to spool and jump.
- Place an `ATDGravityWellMarker` ahead of you to see the drop-out.
- Planet entry and landing need T4's planet atmosphere and the planet maps. On a planet map, place an `ATDAtmosphereFlightZone` and an `ATDLandingPad`.

| Test | Proves |
|---|---|
| `Hyperdrive.SpoolJumpAndSteering` | 1.2 s spool, charge read-out, 80,000 cm/s, ≤ 11°/s steering, manual drop-out gated by `MinJumpSeconds`, drop-out burst, back to Idle, release cancels |
| `Hyperdrive.DamageBlocksAndCancels` | a hit cancels the spool; 2 s lockout; atmosphere and disabled blocks |
| `Hyperdrive.DropsOutAtGravityWell` | auto drop-out at a registered well's edge; blocked inside a well; marker register/unregister; the segment check |
| `Atmosphere.SpeedCapAndGravity` | 3,600 cm/s cap, no boost, gravity sag, box ceiling push-back |
| `Atmosphere.ZoneModeAltitudeAndExit` | zone applies the mode and the input layer, altitude, hyperdrive blocked, ceiling shut until allowed, exit fires, the objectives gate |
| `Atmosphere.EntryCommitsAndHandsBack` | 2.5 s entry, nose into the planet, speed held, hyperdrive off, hand-back without a flow |
| `PlayBounds.AppliedToEveryShip` | existing and later-spawned ships pick up `SetPlayBounds` |
| `Landing.SlowDownPromptAndAtmosphereRule` | 60% of the atmosphere cap; slow-down text at 300 m and at 100 m when too fast; landing prompt at 100 m when slow; nothing at 500 m |
| `Landing.HandsOffArrivalNeverSoftLocks` | from the rig's descent start, hands-off: told to slow down, never touches the ground; braking then lands on the player pad |
| `Landing.PromptSequenceWingmenTakeOff` | prompt range and speed, the full sequence and its states, pad reservation, a wingman on the side pad, input ignored while landed, take-off with the wingman |
| `Camera.CockpitToggle` | chase / cockpit switching; V binding |

## Known gaps
- **End-to-end travel is untested.** It needs the planet maps (landing on main from the Lead) and T5's arrival spawn: who spawns the ship at `ReturnPoint`, and whether it arrives in atmosphere flight or landed (`SnapLanded`). This is being agreed with T5.
- **The first-person hand-off** on `OnShipLanded`, and re-boarding → `RequestTakeOff`, are T5's.
- **No obstacle avoidance:** the landing path is a straight move, so pads need clear air above them.
- **Uncapped jump distance:** the hyperdrive has no fuel or charge limit; it relies on wells, boundary and timeout.

## Safety: what is proven and what isn't

### Proven by tests (`TriadDrift.Travel.Safety.*`)
- **Speed-override ownership:**
  - Hyperdrive (`"Hyperdrive"`), entry (`"AtmosphereEntry"`), T2's ram (`"Ram"`) and unkeyed legacy callers each own one request. The most recent request is the one flown.
  - Clearing an owner restores the request underneath: hyperdrive under a ram, entry under a jump, ram under a drop-out.
  - `ResetFlightState` clears every owner.
- **Hyperdrive never ends inside geometry** at 30, 60 and 120 fps. This holds for an asteroid wall, a planet body with no gravity well, and another ship.
  - The look-ahead sweep (300 m plus one frame of travel) ends the jump at cruise speed without a burst. There's no contact and no damage.
  - A spool is refused with anything solid within 200 m along the nose (`ObstacleAhead`).
- **Wells and boundary:** gravity wells stop the jump at the well's edge, and the play boundary stops it inside the play space, at all three frame rates. The spool takes 1.2 s at every frame rate.
- **Hyperdrive locks:** while `UTDTravelSubsystem::AddHyperdriveLock(Owner, Reason)` is active, no ship can spool. A lock added mid-jump drops the ship out, with end cause `Locked`.
- **Atmospheric entry:**
  - It's refused for wingmen, dead ships, while an entry is already running, and while a retake or level travel is pending.
  - A death during entry aborts it cleanly: override and invulnerability released, no travel.
  - The dive stays above the planet surface.
- **Level travel** goes through `UTDTravelSubsystem::RequestLevelTravel`: at most once per level, never over a pending travel or retake.
- **Timing:** landing (4.9 s), take-off (3.5 s) and entry (2.5 s) are the same at 30, 60 and 120 fps.

### Not proven yet
- **Whether a wave or the finale should lock the hyperdrive is a policy call** for the Lead. The lock mechanism exists, but nothing adds a lock yet.
- **The look-ahead only checks straight along the nose.** A jump steered at 10% around a large body is caught by the per-frame look-ahead, not by a full path prediction.
- **Real level travel firing exactly once is only partly tested.** Headless tests can't perform a real `TravelTo`; the guard is tested with the flow's pending flags.
- **The full planet round trip** (descent, landing, hand-off, re-board, take-off, return) still waits on T5's ground mode.
