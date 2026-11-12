# Combat (Teammate 2): ENG-05 weapons, ENG-06 targeting

Code: `Source/unrealGameJam/TriadDrift/Combat/`. Tests: `Combat/Tests/TDCombatTests.cpp` and
`TDCombatGapTests.cpp` (`.\Tools\td-test.ps1 -Filter TriadDrift.Combat`, 35 tests).

## What exists

| Class | What it does |
|---|---|
| `TDWeaponTypes.h` | `FTDWeaponSpec` (trigger, rate, salvo, spread, aim assist, heat/ammo, charge) and `FTDProjectileSpec` (damage, splash, knockback, speed/range, homing, proximity fuse, shards, placeholder look). `TDWeaponIds::` constants |
| `UTDWeaponLibrary` | `Get(FName)`: all 12 weapons as C++ defaults (table below) |
| `UTDWeaponComponent` | Two slots. `EquipWeapon`, `SetTriggerHeld`, `SetAimDirection`, `SetAimTarget`, `FireOnce`, `GetHeat01`, `GetAmmo`, `IsOverheated`, `GetCharge01`, `CanFire`, `ResetWeapons`, `CancelCharge`. Damage multipliers by key (`SetOutgoingDamageMultiplier("SwitchStrike", 1.25)`), a one-shot multiplier (`SetNextShotDamageMultiplier`, for Cloak), and `SetWeaponsBlocked(key, bool)` (EMP, Gravity Well). The pilot's trigger request survives a block, so fire resumes when the block ends. A dead owner counts as blocked, and a charge fires only on a pilot release. `GetLastFiredProjectile(i)` checks the launch serial. Muzzles: `Muzzle_0..7` sockets on any static mesh of the owner, else `MuzzleOffsets` |
| `ATDProjectile` | Pooled actor with no collision of its own. Each tick it sweeps a sphere on `TDCollision::Projectile`, skipping friendlies and dead ships, so it never tunnels. Implements `ITDTeam` and `ITDDeflectable`, plus `SetDamageMultiplier`, `ClearHoming`, `BreakTracking`, `Kill`, `TeleportProjectile`, and `GetLaunchSerial()` (unique per launch: **pooled actors are reused, so never key anything by the actor alone**). Damage goes only through `UTDStatics::ApplyTDDamage`. `HitDirection` is the travel direction for direct hits and (victim − blast centre) for splash; the gunship frontal shield reads it. The proximity fuse measures to each hostile's own colliding box |
| `UTDProjectileSubsystem` | FIFO pool plus a live index: `GetLiveProjectiles`, `FindHostileMissiles`, `FindMissilesHomingOn`, `KillAll(bPlayFX)`. `KillAll` runs automatically on `OnSceneCut` and on retakes (`OnTakeStarted`, take > 1); `TD.KillAll` should call it too |
| `UTDTargetingComponent` | Soft target (closest to view centre ≤ 25°, sticky with a 3° switch margin), `CycleTarget` (by distance in the front hemisphere, held until it dies), `SetManualTarget` / `IsManualTarget` (the squad carries a cycled target across a switch), Bastion lock painting (`BeginLockPainting` / `EndLockPainting`: 6 × 0.25 s), `GetLeadIndicator` / `ComputeInterceptPoint`, HUD `GetMarkers()`, and missile warning `None / Tracking / Imminent` with the nearest direction and seconds. `ITDHUDData` exposes `GetTargetMarkers`, `GetMissileWarning` and `GetLeadIndicator` (forwarded by `ATDSquadManager`) |

**One aiming model in three places.** A shot fired along direction D flies at
`Speed + max(0, ShooterVel · D)` along D: only the forward part of the shooter's motion is inherited.
The lead solver (`ComputeInterceptPoint`) uses that ground speed with the target's own velocity, refined
twice, and 3° aim assist snaps to the same point. So: crosshair on a stationary target = hit, even while
drifting sideways past it (the signature move); on a moving target, aim at the lead marker.

**HUD markers.** Target selection runs at `UpdateHz` (30). Markers (screen position, edge-arrow
direction, current/locked/missile flags) are re-projected **every frame**, so brackets don't lag the
camera. `ScreenDirection` is unit 2D with x right and y down (screen space). Without a viewport (AI,
tests), "on screen" is a 90° cone.

Events: `UTDEventBus::OnHitConfirmed(Target, bKill)` fires for player-controlled shooters (or with
`bForceReportHits`). A kill also calls `UTDFeelSubsystem::RequestHitStop()` (40 ms).

Presentation: `UTDWeaponFX` slots (Muzzle, ChargeLoop, Overheat, Tracer, ProjectileMesh, Color,
ImpactHull / ImpactShield / ImpactWorld, Detonation, Deflected) go on the weapon component's
`PrimaryFX` / `SecondaryFX`. Every slot is optional.
- Each pooled projectile keeps **one** Tracer Niagara component (SetAsset + Activate on launch,
  Deactivate on death, so ribbons finish).
- The ChargeLoop VFX and sound both stop when a charge ends.
- If `ProjectileMesh` is set, it renders with its own materials. Otherwise the engine sphere is
  stretched along velocity and tinted in squad colours (Rift magenta, Lancer cyan, Bastion amber,
  enemies red and orange). The mesh and tint are reset on every launch.

## Weapon table (C++ defaults)

| Id | Dmg | Rate | Speed / range | Resource | Notes |
|---|---|---|---|---|---|
| PhasePulseLasers | 8 | 8/s | 40 000 / 40 000 | heat 4 s, 1.5 s lockout | alternating, 3° aim assist |
| SplitterBolt | 40 + 5×12 | charge 0.8 s | 25 000 / 35 000 | 6 ammo, +1 / 4 s | splits on impact and at max range |
| NeedleCannons | 4 | 20/s | 55 000 / 30 000 | heat 5 s | 1.2° spread |
| SpikeRockets | 4 × 25, 300 cm splash | 1 / 1.5 s | 30 000 / 40 000 | 8 salvos, +1 / 5 s | 0.07 s ripple |
| BreakerBlaster | 30, 250 cm splash | 2.5/s | 28 000 / 35 000 | heat 6 s | 1500 cm/s knockback on ships ≤ 600 cm |
| FlakBurst | 15, 800 cm | 1 / 1.2 s | 22 000 / 20 000 | 10 shells, +1 / 3 s | 500 cm proximity fuse (arms at 0.12 s), kills missiles |
| Enemy_PulseGuns | 5 | 4/s | 30 000 / 20 000 | none | |
| Enemy_HomingMissile | 25 | AI-paced | 9000, ~7 s | none | 100°/s homing, shoot-downable |
| Enemy_HeavyBolt | 20 | AI-paced | 12 000 / 30 000 | none | |
| Enemy_SalvoMissile | 3 × 20 | one `FireOnce` per salvo | 9500, ~7 s | none | fanned, homing; the target is captured at fire time |
| Enemy_PlasmaMines | 60, 800 cm | AI-paced | near-static, 20 s | none | 500 cm fuse, arms after 0.75 s, shoot-downable, fizzles at 20 s |
| Enemy_PointDefense | 6 | 6/s | 30 000 / 8000 | none | shoots down squad missiles |

Heat is `1/(HeatSeconds × rate + 1)` per shot, so continuous fire from cold overheats exactly
`HeatSeconds` after the first shot. Heat cools over 2 s, but only with the trigger released and after
`max(0.25 s, 1.5 × refire)`, so tap-firing at the max rate can't dodge it.

## How to try it by hand

Launch the sandbox (see the `ue-cpp-gameplay` skill) and fly the squad: LMB / RMB fire, T cycles
targets. `TD.God` helps. Things to watch:
- Phase lasers alternate muzzles and overheat after 4 s.
- Hold RMB on Rift for 0.8 s and release: the bolt splits.
- Bastion's flak pops near missiles.
- Drift sideways past a stationary enemy with the nose on it: shots land.

## Known gaps / next

- Projectiles block on any `TDCollision::Projectile`-blocking component, including rocks (world impact
  FX plays). Neutral destructibles take damage from both teams.
- Splash is flat (no falloff), measured centre-to-bounds; features.md gives flat numbers. I don't use
  Core's `ApplyTDRadialDamage`, because hit markers need per-victim kill detection.
- `ProjectileMesh` and `Tracer` load synchronously on first use: a one-time hitch per asset. Nick's assets
  should be preloaded with the level.
- Not verified in a live play session yet (headless tests only).
