# Enemies & waves (Teammate 4): ENG-13, ENG-14

Code: `Source/unrealGameJam/TriadDrift/Enemies/` and `Waves/`. Tests: `Enemies/Tests/`, `Waves/Tests/`
(`TriadDrift.Enemies.*`, `TriadDrift.Waves.*`). World pieces (Dominion, gates, layout) are in
[world.md](world.md).

## For Nick: which asset to fill per enemy

Make one `UTDEnemyVisuals` data asset per type at exactly this path (the same names as
`docs/visual/Presentation-Asset-Paths.md`). The ship loads it by name, so no code or Blueprint edit is
needed:

| Enemy | Asset path |
|---|---|
| Shard Fighter | `/Game/TriadDrift/Presentation/DA_Enemy_Fighter_Visuals` |
| Hook Interceptor | `/Game/TriadDrift/Presentation/DA_Enemy_Interceptor_Visuals` |
| Wall Gunship | `/Game/TriadDrift/Presentation/DA_Enemy_Gunship_Visuals` |
| Maul Bomber | `/Game/TriadDrift/Presentation/DA_Enemy_Bomber_Visuals` |
| Hive Carrier (also its bays and turrets) | `/Game/TriadDrift/Presentation/DA_Enemy_Carrier_Visuals` |

Which slot plays when:
- **`Mesh` / `MaterialOverrides`:** replaces the placeholder cone and wings. Author it +X forward,
  sized to the gameplay radius in the table below.
- **`EngineTrail`:** an attached loop from spawn until death.
- **`WarpIn`:** when the wave director spawns the ship.
- **Wind-ups:** `TelegraphMissile` / `TelegraphHeavyBolt` / `TelegraphSalvo` / `TelegraphMineTrail`
  play attached for the telegraph duration. An empty slot falls back to `Telegraph`.
- **Hits:** `HullHit` on hull hits (rate-limited to about 12 per second), `ShieldHit` on gunship
  frontal-shield hits.
- **Gunship frontal shield:** `FrontalShieldBreak` / `FrontalShieldRestore`.
- **Damage and status:** `DamagedSmoke` as an attached loop once hull drops below 50%; `Stunned` on EMP.
- **Death:** `DeathTumble` at death start, then `Explosion` when the tumble ends.
- **Carrier:** `BayLaunchTelegraph` (doors opening), `BayLaunch`, `BayDestroyed`; `TurretTelegraph`
  (falls back to `Telegraph`), `TurretDestroyed`.

Two ways to override the convention asset:
- `FTDEnemySpec::Visuals` (soft pointer) in C++.
- `ATDEnemyShip::Visuals` set on a BP child or a placed instance.

**Without any asset:** a placeholder that carries the colour code on its own (`UTDStatics::TintPlaceholder`):
a glowing red-orange body (`PlaceholderBodyGlow` 2.5), dark-red wings, two white glowing "eyes" at the nose
and a white-hot engine (`PlaceholderEmissiveBoost` 8), a cyan plate across a gunship's nose while its frontal
shield is up, a shape per role, and a visual-only readability scale by size (small ×2.2, medium ×1.6, large
×1.4; collision keeps the gameplay radius). All of it hides as soon as `Mesh` is set.

## What exists

### Enemy table: `UTDEnemyLibrary`
The five Null Armada ships from features.md ENG-13 are C++ defaults in `FTDEnemySpec`, and every
field is tunable. Ids live in `TDEnemyIds::*`. The weapon ids in `TDEnemyWeaponIds::*` match T2's
`TDWeaponIds::Enemy*`.

| Id | Hull / Shield | Speed | Radius | Attacks | Telegraph |
|---|---|---|---|---|---|
| `ShardFighter` | 30 / 0 | 7000 | 220 | Pulse guns 5 dmg, 4/s | none (guns) |
| `HookInterceptor` | 50 / 20 | 9000 | 260 | Pulse guns + homing missile 25 every 8 s | 0.8 s |
| `WallGunship` | 400 + 300 frontal (recovers 15 s after breaking) | 3000 | 700 | Heavy bolt volley (3 × 20) every 4.5 s, missile salvo (3 × 20) every 12 s | 0.6 s / 1.2 s |
| `MaulBomber` | 150 / 50 | 4000 | 500 | Plasma mine trail (4 × 60 dmg, 800 cm), every 5 s | 0.5 s |
| `HiveCarrier` | 1500, 3 bays × 300, 4 PD turrets (6 dmg, 1 s on / 2 s off) | 1500 | 1800 | 3 fighters per bay every 12 s, at most 3 launches per bay, 8 alive | 1.5 s per launch |

### `ATDEnemyShip : ATDSpacecraft, ITDWaveEnemy`
- **Setup:** `InitFromSpec(Id)` applies health, flight tuning, collision radius, T1's `Weapons`
  component and the visuals.
  - Gunships and carriers have `MinSpeedFraction = 0`, so they can hold position.
- **Attacks:** `TryStartAttack(Kind, Target)` telegraphs, then fires.
  - The telegraph state is exposed through `IsTelegraphing`, `GetTelegraphAttack`,
    `GetTelegraph01`, `GetTelegraphDuration` and `OnTelegraphStarted`.
  - Guns: `SetGunsFiring(bool, Target)` leads the target and applies a per-pilot aim error.
  - Heavy attacks use T2's `FireOnce`. `GetFireCount` counts only shots that really fired.
- **Status effects:**
  - `ApplyStun(s)`: dead stick, no fire, carrier bays paused, +25% damage taken.
  - `LoseTracking(s)`: the AI forgets its target and can't re-acquire one.
  - `SuppressFire(s)`.
  - All three are called by T2's EMP, Gravity Well and portals.
- **Gunship frontal shield:** a `DamageFilter` on `FTDDamageInfo.HitDirection`, covering a 70°
  half-angle at the nose.
  - It has 300 HP, overflow goes to the hull, and it recovers 15 s after it breaks.
  - Ram damage breaks it. `DropFrontalShield(s)` is the EMP version.
- **Death:**
  - The sphere stops colliding with ships, so wrecks are scenery.
  - The tumble lasts 0.25 s for a fighter up to 2.5 s for a carrier, then the ship explodes and is
    destroyed.
  - Carriers carry `TDDirectorTags::Carrier`, and T3's Director stages the CarrierKill shot.
- **Carrier:**
  - 3 `ATDCarrierBay` and 4 `ATDEnemyTurret` children are mounted outside the hull sphere, by
    radius + part extent + `CarrierPartClearance`.
  - The carrier and its parts ignore each other when moving.
  - Launched fighters form a pack and are registered with the wave director (`MaxLaunchedAlive` 12).
- **Bays:** each bay has its own 300 HP. It telegraphs, then launches every 12 s (first launches at
  6, 8 and 10 s) and stops once destroyed. `BreakDoors()` or any Ram hit doubles the damage it takes.

### `ATDEnemyAIController`
It flies only through `SetFlightInput`.
- **States:** Approach → AttackRun → BreakOff → re-approach, plus Formation, Idle and Stunned.
- **Breaking off:** a ship breaks off on time-to-impact under 0.8 s, so attack runs don't ram the
  player. The attack-run time limit only counts once the ship is inside gun range.
- **Leash:** with nothing in `AcquireRange`, the pilot goes after the player's ship at any range.
  Beyond 22 000 cm it uses catch-up speed (1.7× cruise) until inside 0.8× gun range, so a wave can
  always be finished.
- **Targeting:**
  - Interceptors hunt the possessed pawn.
  - About half the other pilots prefer the player's ship, and the rest take the nearest squad ship.
- **Role tweaks:**
  - Gunships face their target and hold range.
  - Bombers mine the squad's route.
  - Carriers sit broadside.
  - Pack followers hold a V slot, catching up faster than their leader, and go solo when the
    leader dies.
- **Steering:**
  - Obstacle avoidance uses `UTDWorldSubsystem::FindAvoidance` (see world.md).
  - Separation keeps pilots 1400 cm apart.
- **Debug:** `TD.EnemyAIDebug 1` draws state labels and target lines.

### `ATDWaveDirector`
- **Wave table:** the five-wave ENG-14 table is C++ defaults (`MakeDefaultWaves`), or an
  `FTDWaveDef` DataTable via `WaveTableOverride` (it logs a warning if the wave count isn't 5).
- **Spawning:**
  - Each group's spawn point is picked when its 1.5 s red warp-in is announced, led by the player's
    velocity.
  - Groups spawn behind or flanking the player, never ahead.
  - Events: `OnWarpInStarted` and the `UTDTravelFX::EnemyWarpIn` cue.
- **Clearing:** a wave clears only when every enemy is dead or destroyed, carrier launches included.
- **Between waves:** a 6 s breather with 8%/s hull repair on Player-team ships, then the next wave of
  the same phase. The last wave of a phase goes to `PhaseComplete`.
- **Events** (1-based numbering):
  - `OnWaveStarted` / `OnWaveCleared`.
  - `OnMarkerRevealed` after waves 2 and 4, re-broadcast by `StartFromProgress` in a planet phase.
  - `SetDominionState(true,false)` after wave 5.
  - Tips: switching (once), drift, hyperdrive, gunships, carriers.
- **Takes:**
  - T5's game instance owns takes. A higher `OnTakeStarted` for the current scene restarts the wave.
  - The director syncs its take with the flow before it broadcasts, to avoid re-entering the wave
    start.
  - Without a flow (tests), it announces take 1 itself.
- **API for other areas:** `StartWave`, `StartFromProgress` (resume; does nothing while a wave
  runs), `SkipWave`, `StopAndClear`, `RestartCurrentWave`, `RegisterEnemy`, `GetWaveInfo`.
  - `Get()` is cheap because directors register themselves.

## How to try it
- **Tests:** run `.\Tools\td-test.ps1 -Filter TriadDrift.Enemies` and `-Filter TriadDrift.Waves`.
  - Real-projectile and real-AI tests: `Enemies.Behaviour.*`, `Enemies.Carrier.*` and `Enemies.Play.*`.
- **Sandbox:** `UnrealEditor.exe <uproject> /Engine/Maps/Entry?game=/Script/unrealGameJam.TDSpaceGameMode -game -windowed`.
  - Waves start 3 s in.
  - `TD.SkipWave` jumps ahead; `TD.EnemyAIDebug 1` shows each pilot's state.

## Known gaps
- **Not flown by hand yet.** Every behaviour has a headless test, but feel and difficulty need a PIE
  pass and ENG-27 tuning: ranges, aim error, catch-up speed, separation.
- **Scene ids:** the director's `FTDWaveDef::SceneId` defaults to "WaveN". T5's flow still derives
  "Wave%d" itself, and the agreement that the flow reads `GetWaveInfo().SceneId` is pending with T5.
- **Leftover shots on stop:** `StopAndClear` doesn't clear live enemy projectiles yet. That waits on
  T2's `UTDProjectileSubsystem::KillAll` landing on main; retakes already clear them on T2's side.
- **Cooking:** the convention visuals assets are referenced only by path, so
  `/Game/TriadDrift/Presentation` must be in `DirectoriesToAlwaysCook` (Lead / Config).

## The Dominion assault across takes and phases (World review, Nov 12)
- **One scene:** the whole assault is the flow's "Dominion" scene. On a new take (`OnTakeStarted("Dominion", n > 1)`):
  destroyed generators **stay destroyed**, a reactor phase already passed **stays passed**, the current reactor
  phase **restarts at full**, live generators and turrets are repaired and re-armed, launched fighters are cleared
  and the next launch comes one interval later. A take during the breakup keeps the squad protected (victory wins).
- **Hyperdrive:** the "Finale" lock holds only once a squad ship has been inside the Dominion's gravity well
  (`HyperdriveLockRadius` = the layout's `DominionWellRadius`), so the first approach is a hyperdrive jump that
  drops out at the well. It is released at victory.
- **Reveal:** the reveal flag; a voided wave-5 clear never sets it (T5's flow ignores `SetDominionState` while a
  retake is pending).
- **Victory phase:** a won save or `TD.Stage Victory` leaves the Dominion `Finished`: hidden, no weapons, no
  launches, no lock.
