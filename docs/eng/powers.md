# Powers (Teammate 2): ENG-09 portals, ENG-10 ram, ENG-11 swarm + dome, ENG-12 abilities

Code: `Source/unrealGameJam/TriadDrift/Powers/`. Tests: `Powers/Tests/` (`.\Tools\td-test.ps1 -Filter
TriadDrift.Powers`, 25 tests; every test body runs at 30, 60 and 120 fps).

## The power base

`UTDPowerComponent` is one slot per power: `Signature1` (Q tap), `Signature2` (Bastion hold-Q, 0.5 s),
`Ability` (E). These are T1's `ETDPowerInput` values.
- **Contract:** `TryActivate`, `GetCooldown01` (1 = ready), `GetPowerName`, `IsPowerActive` (not
  `IsActive`, which is component activation), `GetWindup01`, `EndPower`, `ResetPower`, `ResetCooldown`.
- **Routing:** `FindPower(Ship, Slot)` and `RoutePowerInput(Ship, Slot, bPressed)` (T1's controller
  calls the second).
- **Events:** every activation broadcasts `UTDEventBus::OnPowerUsed(Ship, PowerName)`, which arms T3's
  Switch Strike.
- **Player only:** `bRequirePlayerControl` (on by default) means a power fires only while the player
  flies that ship. Wingmen never use powers.
- **After a switch:** a power that is already running keeps running.
- **On death:** `bEndOnOwnerDeath` ends the ram and the cloak when their ship dies. Domes and portals
  keep running for the rest of the squad.
- **Scene cuts and retakes:** `UTDPowerSubsystem::ResetAllPowers` runs on `OnSceneCut` and on
  retakes (`OnTakeStarted`, take > 1). It closes portals, collapses domes, ends gravity wells, ends
  every running power and readies all cooldowns.

**Installing:** `FTDShipSpec` (T3's `UTDShipLibrary`) is the single source of truth:
`Signature1PowerClass`, `Signature2PowerClass` and `AbilityComponentClass`. Squad creates them once
per ship and never re-creates them; on respawn and retake it calls `ResetPower()`. Powers has no
installer of its own.

| Ship | Signature1 | Signature2 | Ability (unlocked) |
|---|---|---|---|
| Rift | `UTDPortalBlasterPower` | | `UTDCloakPower` |
| Lancer | `UTDRamPower` | | `UTDEMPPower` |
| Bastion | `UTDMissileSwarmPower` | `UTDShieldDomePower` | `UTDGravityWellPower` |

`UTDAbilityComponent` fills the Ability slot from `ITDGameFlow::GetProgress().UnlockedAbilities` on
BeginPlay and from `OnAbilityUnlocked`. HUD: `GetAbility()` (None while locked), `HasAbility()`, and
`GetReadiness01()` (0 while locked, else the power's `GetCooldown01`).

## ENG-09 Rift: Portal Blaster (+ Blink fallback)

**Placing portals:**
- Q places a portal where a 100 cm sweep along the aim direction hits world geometry (pulled 300 cm off
  the surface), or at 12 000 cm.
- Press 1 = A, press 2 = B, and press 3+ recycles the oldest. Every press counts as using the power
  (so it breaks Cloak).
- The 12 s window restarts once, when B opens, so a late B still gets the full 12 s. Recycling doesn't
  extend it. Then both close and the 8 s cooldown starts.

**Facing (for Nick's portal VFX): each portal's forward/normal faces BACK toward Rift, like a door.**
Going in through a portal's front brings you out of the partner's front (along its normal); in through
the back, out of its back.

**Transit:** `ATDPortal` (600 cm radius) tests every registry spacecraft's frame-to-frame motion and
every live projectile against its disc. There's no trigger volume, so nothing tunnels through.
Teleports by others (gates, hyperdrive) are ignored via `OnSpacecraftTeleported`.

**Safety:**
- Placement, exit clearance and Blink test WorldStatic + WorldDynamic geometry (the Dominion hull, trench,
  pylons and planets are dynamic) plus T4's obstacle registry, sized to the traveller.
- Placement pushes out up to 4 × 300 cm along the normal to find clearance, otherwise it's refused
  ("can't place" cue).
- A ship is never sent to an exit that has no clearance, and anything bigger than the 600 cm disc (a
  carrier) is refused.
- Projectiles are tested Prev -> Cur with `ATDProjectile::GetPreviousLocation()`, so a shot fired just
  in front of an exit is never sent back.

**What gets transited** (all through `UTDPortalTransit::TransitActor`, shared with T4's gates):
- Ships keep their speed (and a ramming Lancer keeps its dash), take the exit facing, and get 0.3 s of
  invulnerability. The player's ship adds the `PortalsUsed` stat.
- Enemy fighters lose tracking for 2 s.
- Enemy projectiles are `Deflect()`ed to Player, re-home on whoever fired them and fly 1.5× speed.
  Missiles also add `MissilesReturned` and `UTDDirectorSubsystem::ReportHighlight(MissileReturn)`.

The re-entry cooldown is 0.5 s. Projectiles keep it per launch, so pooled reuse is never blocked.

**Fallback:** `bUseBlinkFallback = true` or console `TD.PortalBlink 1` switches Q to Blink: a teleport
to the same impact point (500 cm off the surface) keeping speed, 0.3 s of invulnerability, and a 3 s,
1500 cm missile-reflect field riding with Rift. Cooldown 8 s.

Events: `OnPortalState` (Opening, Open, Transit, Closing, Closed). FX: `UTDTravelFX` PortalOpen / Idle /
Transit / Close (power's `TravelFX`).

## ENG-10 Lancer: Afterburner Ram

- **Wind-up:** 0.3 s (`GetWindup01()` for the wing fold).
- **Dash:** 1.2 s at 3 × boost speed with 40% steering: a keyed override (`"Ram"`) on T1's movement,
  plus a velocity kick, so speed is there at once. Keyed overrides stack: the ram flies over atmosphere
  entry and restores it. It refuses (and ends) while the hyperdrive isn't Idle.
- **Pass-through:** during the dash Lancer ignores the Ship channel, so it really goes through fighters.
  Hits are found along the whole frame segment, in path order.
- **Invulnerable** throughout, then until its speed is back to boost speed (capped at 1.5 s). It's timed
  and refreshed, never the untimed flag that `TD.God` uses.
- **Damage:** 300 Ram damage, once per target.
- **Shields and doors:** gunship frontal shields break (`BreakFrontalShield`; T4's filter also bypasses
  them on Ram damage) and carrier bay doors break (`BreakDoors`).
- **Big targets:** enemies of `StopOnSize` (Large) and up stop the dash at first contact. So does any
  non-ship blocker the flight model hits (`OnFlightImpact`: rocks, the Dominion hull). The stop fires a
  150-damage, 1500 cm shockwave that spares the stopper.
- **Cooldown:** 7 s.
- **Highlight:** RamKill money shots come from the director (Ram damage kind on `OnActorKilled`).

## ENG-11 Bastion: Missile Swarm and Shield Dome

**Missile Swarm** (Signature1):
- Fires 12 micro-missiles in a fan, split round-robin across the painted locks, or the soft target if
  there are none.
- Each does 18 damage with a 200 cm splash, homes at 240°/s, and can be shot down by point defense.
- Cooldown 9 s.
- **Lock painting is automatic** while the player flies Bastion and the swarm is ready, because hold-Q
  is the dome. Locks appear in `GetLockedTargets()` and the markers' `bLocked`, so the HUD shows the
  brackets.

**Shield Dome** (Signature2, T1 fires it at 0.5 s of hold) is `ATDShieldDome`:
- 3000 cm radius for 6 s, following Bastion with an exponential lag (`FollowLagRate` 4/s): it trails a
  cruising Bastion by about 1200 cm, so Bastion never leaves its own dome.
- Squad ships inside take no damage and regenerate shields at 2×.
- Hostile shots stop exactly on the shell, inside the projectile's own swept step, so nothing leaks a
  frame-step inside. Hostile homing missiles are deflected back at their shooter.
- 16 s cooldown, counted from when it collapses.

Wingmen query `UTDPowerSubsystem::FindDomeCovering(Location)` / `GetNearestDome(Location)`.

## ENG-12 Abilities

| Ability | Behaviour (tested) | Cooldown |
|---|---|---|
| Gravity Well | Orb at 5000 cm/s for exactly 0.8 s, then a 5000 cm vortex for 5 s. Pulls only free-flying Small/Medium enemy ships: a swept move through their movement component, with their engines held at a crawl under a keyed `"GravityWell"` override (released on exit, implosion or reset). Turrets, bays, generators, the reactor and Large/Boss hulls stay put but take the damage. Ships inside can't fire (`SuppressFire`), turrets are held stunned, 10 dps, enemy shots are bent and dragged in, then a 120-damage implosion | 18 s |
| Cloak | 6 s with `SetTargetable(false)`: AI can't find Rift and missiles already in flight lose their lock. First shot out does 3× (`SetNextShotDamageMultiplier`). Firing or using any other power breaks it; expiring unused leaves no banked 3× | 15 s (from end) |
| EMP Pulse | 6000 cm: `ApplyStun(3)` on enemy ships (T4's +25% stunned damage) and on turrets (carrier, Dominion), `DropFrontalShield(5)` on gunships, hostile missiles fizzle | 14 s |

## Presentation (art plugs in by data)

- **Data assets** load through Core's `UTDStatics::LoadPresentation` from the flat folder
  `/Game/TriadDrift/Presentation/`. All optional.
  - **Signature powers:** `DA_<Rift|Lancer|Bastion>_Visuals` cues: PowerWindup (ram), PowerActive,
    Power2Active (dome), PowerEnd.
  - **Abilities:** `DA_Ability_<GravityWell|Cloak|EMPPulse>_FX`: Activate, Loop, End, AffectedTarget
    (EMP and well victims).
  - **Portals:** `DA_Travel_FX`: PortalOpen, PortalIdle, PortalTransit, PortalClose.
  - **Swarm missiles:** `DA_MissileSwarm_FX`.
- **Cue requests:** every cue goes through a named request (`FTDPowerPresentation::RequestCue`:
  `Ship.*`, `Ability.*`, `Travel.*`). The requests are counted, so a test asserts each moment asks for
  its cue before art exists.
- **Placeholders without art** (engine shapes tinted with `TintPlaceholder`, Shipping-safe, hard-referenced
  so they cook). Each hides as soon as its cue has art:
  - portals: a magenta disc and rim
  - dome: an amber lattice shell (Blink's reflect field is magenta)
  - gravity well: a violet core, plus a ring when the vortex opens
  - EMP: a cyan ring flash
  - ram: a cyan cone plus afterburner streaks along the hull
  - cloak: a magenta shimmer
- **Lattices are plain static meshes.** An instanced mesh fell back to the grey default material,
  because the engine glow material isn't instancing-enabled (found in play).

## How to try it by hand

Dev console (not in Shipping):
- `TD.Power.Use <Signature1|Signature2|Ability>`, `TD.Power.Ship <Rift|Lancer|Bastion>`.
- `TD.Power.Turn <yaw> [pitch]`, `TD.Power.Targets [count] [distance]`.
- `TD.Power.Ready`, `TD.Power.TimeScale <x>`.

Agents: `Tools\td-play-mcp.ps1` (t2 = port 8012), then
`python Tools\td_mcp.py --port 8012 call ConsoleToolset.ConsoleToolset RunConsoleCommand '{"command":"TD.Power.Use Signature1"}'`
and `CaptureGameViewport`.

In the sandbox:
- **Rift:** Q twice (aim at two spots) and fly through. Have a Hook Interceptor fire at you, and put a
  portal in the missile's path.
- **Lancer:** Q rams a fighter pack.
- **Bastion:** look at enemies until brackets lock, then tap Q. Hold Q for the dome.
- **Abilities:** `TD.Unlock` gives abilities, then press E.
- **Blink:** `TD.PortalBlink 1` switches Rift to Blink.

## Play status (AAA.md stages)

**Gameplay validated (agent):** played on L_Galaxy, Nov 12, over MCP on port 8012 (td-play-mcp). Every
power was triggered on the real squad ships and captured (`Saved/TDShots/mcp/01_portals` …
`07_gravity_well`): two portals, cloak, ram mid-dash, EMP ring, swarm fan, dome, gravity well.
Driven by console commands, not by a human on the controls. Feel and timing still want a human
session.

## Known gaps

- The Blink reflect field reuses the dome class with protection off.
- The portal "can't place" refusal reuses the `PortalClose` cue until Nick adds a dedicated one.
- The cloak placeholder is a shimmer around a fully visible ship. Actually hiding or translucing Rift is
  a presentation decision for Nick (`DA_Ability_Cloak_FX`).
- The ram cone sits ahead of Lancer and is hidden by the hull from the chase camera; the afterburner
  streaks carry the read from behind.
