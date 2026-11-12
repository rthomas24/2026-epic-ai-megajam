# Powers (Teammate 2): ENG-09 portals, ENG-10 ram, ENG-11 swarm + dome, ENG-12 abilities

Code: `Source/unrealGameJam/TriadDrift/Powers/`. Tests: `Powers/Tests/` (`.\Tools\td-test.ps1 -Filter
TriadDrift.Powers`, 17 tests).

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

**Installing:** `UTDPowerLibrary::InstallShipPowers(Ship, ShipId)` adds the ship's signature powers and a
`UTDAbilityComponent`, and is idempotent. T3's Squad installs them data-driven from `FTDShipSpec` with
the same defaults.

| Ship | Signature1 | Signature2 | Ability (unlocked) |
|---|---|---|---|
| Rift | `UTDPortalBlasterPower` | | `UTDCloakPower` |
| Lancer | `UTDRamPower` | | `UTDEMPPower` |
| Bastion | `UTDMissileSwarmPower` | `UTDShieldDomePower` | `UTDGravityWellPower` |

`UTDAbilityComponent` fills the Ability slot from `ITDGameFlow::GetProgress().UnlockedAbilities` on
BeginPlay and from `OnAbilityUnlocked`.

## ENG-09 Rift: Portal Blaster (+ Blink fallback)

**Placing portals:**
- Q places a portal where a 100 cm sweep along the aim direction hits world geometry (pulled 300 cm off
  the surface), or at 12 000 cm.
- Press 1 = A (this starts the 12 s window), press 2 = B, and press 3+ recycles the oldest.
- At 12 s both portals close and the 8 s cooldown starts.

**Facing (for Nick's portal VFX): each portal's forward/normal faces BACK toward Rift, like a door.**
Going in through a portal's front brings you out of the partner's front (along its normal); in through
the back, out of its back.

**Transit:** `ATDPortal` (600 cm radius) tests every registry spacecraft's frame-to-frame motion and
every live projectile against its disc. There's no trigger volume, so nothing tunnels through.
Teleports by others (gates, hyperdrive) are ignored via `OnSpacecraftTeleported`.

**Safety:**
- Placement pushes out up to 4 × 300 cm along the normal to find clearance, otherwise it's refused
  (`PortalClose` cue as "can't place").
- A ship is never sent to an exit that has no clearance.

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
- **Dash:** 1.2 s at 3 × boost speed with 40% steering (T1's `SetSpeedOverride`).
- **Invulnerable** throughout, plus 0.25 s of grace after the dash.
- **Damage:** 300 Ram damage, once per target; fighters are smashed through.
- **Shields and doors:** gunship frontal shields break (`BreakFrontalShield`; T4's filter also bypasses
  them on Ram damage) and carrier bay doors break (`BreakDoors`).
- **Big targets:** enemies of `StopOnSize` (Large) and up, or world geometry, stop the dash with a
  150-damage, 1500 cm shockwave.
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
- 3000 cm radius for 6 s, drifting after Bastion at 2500 cm/s.
- Squad ships inside take no damage and regenerate shields at 2×.
- Hostile shots crossing it are destroyed; hostile homing missiles are deflected back at their shooter.
- 16 s cooldown, counted from when it collapses.

Wingmen query `UTDPowerSubsystem::FindDomeCovering(Location)` / `GetNearestDome(Location)`.

## ENG-12 Abilities

| Ability | Behaviour (tested) | Cooldown |
|---|---|---|
| Gravity Well | Orb at 5000 cm/s for exactly 0.8 s, then a 5000 cm vortex for 5 s. Pulls enemies in (moved directly, so it works whatever their flight model does), `SuppressFire` on enemy ships, 10 dps, bends and swallows enemy projectiles, then a 120-damage implosion | 18 s |
| Cloak | 6 s with `SetTargetable(false)`: AI can't find Rift and missiles already in flight lose their lock. First shot out does 3× (`SetNextShotDamageMultiplier`). Firing or using any other power breaks it; expiring unused leaves no banked 3× | 15 s (from end) |
| EMP Pulse | 6000 cm: `ApplyStun(3)` on enemy ships (T4's +25% stunned damage), `DropFrontalShield(5)` on gunships, hostile missiles fizzle | 14 s |

FX: each power's `FX` (`UTDAbilityFX`: Activate / Loop / End). Placeholders are debug spheres in dev
builds.

## How to try it by hand

In the sandbox:
- **Rift:** Q twice (aim at two spots) and fly through. Have a Hook Interceptor fire at you, and put a
  portal in the missile's path.
- **Lancer:** Q rams a fighter pack.
- **Bastion:** look at enemies until brackets lock, then tap Q. Hold Q for the dome.
- **Abilities:** `TD.Unlock` gives abilities, then press E.
- **Blink:** `TD.PortalBlink 1` switches Rift to Blink.

## Known gaps

- Not verified in a live play session yet (headless tests only).
- The Gravity Well moves enemies by `SetActorLocation`. That's robust, but it doesn't add velocity, so
  they don't fling out when it ends.
- The Blink reflect field reuses the dome class with protection off.
- The portal "can't place" refusal reuses the `PortalClose` cue until Nick adds a dedicated one
  (features.md §11 request).
- The ram doesn't yet use T1's `SetOverrideAcceleration` instant kick, or check the hyperdrive state
  (neither was on main when I built it). It reaches dash speed in about 0.6 s.
