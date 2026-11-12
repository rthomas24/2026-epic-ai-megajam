# Combat (Teammate 2): ENG-05 weapons, ENG-06 targeting

Code: `Source/unrealGameJam/TriadDrift/Combat/`. Tests: `Combat/Tests/TDCombatTests.cpp`
(`.\Tools\td-test.ps1 -Filter TriadDrift.Combat`, 19 tests).

## What exists

| Class | What it does |
|---|---|
| `TDWeaponTypes.h` | `FTDWeaponSpec` (trigger, rate, salvo, spread, aim assist, heat/ammo, charge) and `FTDProjectileSpec` (damage, splash, knockback, speed/range, homing, proximity fuse, shards, placeholder look). `TDWeaponIds::` constants |
| `UTDWeaponLibrary` | `Get(FName)`: all 12 weapons as C++ defaults (table below) |
| `UTDWeaponComponent` | Two slots. `EquipWeapon`, `SetTriggerHeld`, `SetAimDirection`, `SetAimTarget`, `FireOnce`, `GetHeat01`, `GetAmmo`, `IsOverheated`, `GetCharge01`, `CanFire`. Damage multipliers by key (`SetOutgoingDamageMultiplier("SwitchStrike", 1.25)`), one-shot multiplier (`SetNextShotDamageMultiplier`, for Cloak), `SetWeaponsBlocked(key, bool)` (EMP, Gravity Well). Muzzles: `Muzzle_0..7` sockets on any static mesh of the owner, else `MuzzleOffsets` |
| `ATDProjectile` | Pooled actor with no collision of its own. Each tick it sweeps a sphere on `TDCollision::Projectile`, skipping friendlies and dead ships, so it never tunnels. Implements `ITDTeam` and `ITDDeflectable`. Damage only through `UTDStatics::ApplyTDDamage`, with the travel direction as `HitDirection` (the gunship's frontal shield uses it). Splash measures to bounds; shards fan out in an even ring and ignore the actor that was hit; the proximity fuse detonates at the closest approach; flak and point defense kill hostile missiles; homing acquires in a cone, loses lock on untargetable (cloaked) targets, and gets a mild lead |
| `UTDProjectileSubsystem` | Pool, plus a live index: `GetLiveProjectiles`, `FindHostileMissiles`, `FindMissilesHomingOn` (no `GetAllActorsOfClass`) |
| `UTDTargetingComponent` | Soft target (closest to view centre ≤ 25°, sticky with a 3° switch margin), `CycleTarget` (by distance in the front hemisphere, held until it dies), Bastion lock painting (`BeginLockPainting` / `EndLockPainting`: 6 × 0.25 s), `GetLeadIndicator` / `ComputeInterceptPoint`, HUD `GetMarkers()` (on-screen position or edge-arrow direction, current/locked/missile flags), missile warning `None / Tracking / Imminent` with nearest direction and seconds |

Events: `UTDEventBus::OnHitConfirmed(Target, bKill)` for player-controlled shooters (or `bForceReportHits`).
A kill also calls `UTDFeelSubsystem::RequestHitStop()` (40 ms). Presentation: `UTDWeaponFX` slots
(Muzzle, ChargeLoop, Overheat, Tracer, ProjectileMesh, Color, ImpactHull / ImpactShield / ImpactWorld,
Detonation, Deflected) go on the weapon component's `PrimaryFX` / `SecondaryFX`. Every slot is optional.
Until art exists, projectiles draw as an engine sphere stretched along their velocity, in squad
colours (Rift magenta, Lancer cyan, Bastion amber, enemies red and orange).

## Weapon table (C++ defaults)

| Id | Dmg | Rate | Speed / range | Resource | Notes |
|---|---|---|---|---|---|
| PhasePulseLasers | 8 | 8/s | 40 000 / 40 000 | heat 4 s, 1.5 s lockout | alternating, 3° aim assist |
| SplitterBolt | 40 + 5×12 | charge 0.8 s | 25 000 / 35 000 | 6 ammo, +1 / 4 s | splits on impact and at max range |
| NeedleCannons | 4 | 20/s | 55 000 / 30 000 | heat 5 s | 1.2° spread |
| SpikeRockets | 4 × 25, 300 cm splash | 1 / 1.5 s | 30 000 / 40 000 | 8 salvos, +1 / 5 s | 0.07 s ripple |
| BreakerBlaster | 30, 250 cm splash | 2.5/s | 28 000 / 35 000 | heat 6 s | 1500 cm/s knockback on ships ≤ 600 cm |
| FlakBurst | 15, 800 cm | 1 / 1.2 s | 22 000 / 20 000 | 10 shells, +1 / 3 s | 500 cm proximity fuse, kills missiles |
| Enemy_PulseGuns | 5 | 4/s | 30 000 / 20 000 | none | |
| Enemy_HomingMissile | 25 | AI-paced | 9000, ~7 s | none | 100°/s homing, shoot-downable |
| Enemy_HeavyBolt | 20 | AI-paced | 12 000 / 30 000 | none | |
| Enemy_SalvoMissile | 3 × 20 | one `FireOnce` per salvo | 9500, ~7 s | none | fanned, homing |
| Enemy_PlasmaMines | 60, 800 cm | AI-paced | near-static, 20 s | none | 500 cm fuse, arms after 0.75 s, shoot-downable |
| Enemy_PointDefense | 6 | 6/s | 30 000 / 8000 | none | shoots down squad missiles |

Heat: `1/(HeatSeconds × rate + 1)` per shot, so continuous fire from cold overheats exactly
`HeatSeconds` after the first shot. Heat cools (after 0.25 s, over 2 s) only with the trigger released.

## How to try it by hand

Launch the sandbox (see the `ue-cpp-gameplay` skill) and fly the squad: LMB / RMB fire, T cycles
targets. `TD.God` helps. Things to watch: Phase lasers alternate muzzles and overheat after 4 s;
hold RMB on Rift for 0.8 s, release, and the bolt splits; Bastion's flak pops near missiles.

## Known gaps / next

- Projectiles block on any `TDCollision::Projectile`-blocking component, including rocks (world
  impact FX plays). Neutral destructibles take damage from both teams.
- Splash is centre-to-bounds with no falloff (features.md gives flat numbers). Core's
  `ApplyTDRadialDamage` is unused because hit markers need per-victim kill detection.
- Tracer FX: `UTDWeaponFX::Tracer` attaches a Niagara system; `ProjectileMesh` loads synchronously
  on first use (one-time hitch per mesh).
- Not yet built: ENG-09 portals, ENG-10 ram, ENG-11 swarm and dome, ENG-12 abilities (`Powers/`). The
  hooks they need already exist: `Deflect`, `TeleportProjectile`, `BreakTracking`, `Kill`,
  `SetWeaponsBlocked`, the multipliers and lock painting.
- Not verified in a live play session yet (headless tests only).
