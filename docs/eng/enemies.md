# Enemies & waves (Teammate 4): ENG-13, ENG-14

Code: `Source/unrealGameJam/TriadDrift/Enemies/` and `Waves/`. Tests: `Enemies/Tests/`, `Waves/Tests/`.

## What exists

### Enemy table: `UTDEnemyLibrary` (`TDEnemyLibrary.h/.cpp`)
The five Null Armada ships from features.md ENG-13 are C++ defaults in `FTDEnemySpec` (all fields
tunable). Ids live in `TDEnemyIds::*`, and the weapon ids in `TDEnemyWeaponIds::*` match T2's
`TDWeaponIds::Enemy*`.

| Id | Hull / Shield | Speed | Attacks | Telegraph |
|---|---|---|---|---|
| `ShardFighter` | 30 / 0 | 7000 | Pulse guns 5 dmg, 4/s | none (guns) |
| `HookInterceptor` | 50 / 20 | 9000 | Pulse guns + homing missile 25 every 8 s | 0.8 s |
| `WallGunship` | 400 + 300 frontal | 3000 | Heavy bolt volley (3 × 20) every 3.5 s, missile salvo (3 × 20) every 10 s | 0.6 s / 1.2 s |
| `MaulBomber` | 150 / 50 | 4000 | Plasma mine trail (4 × 60 dmg, 800 cm), every 5 s | 0.5 s |
| `HiveCarrier` | 2000, 3 bays × 300 | 1500 | 3 fighters per bay every 12 s, 4 PD turrets (6 dmg) | 1.5 s per launch |

`OverrideSpec` / `ResetToDefaults` handle runtime tuning.

### `ATDEnemyShip : ATDSpacecraft, ITDWaveEnemy`
- `InitFromSpec(Id)` sets health, flight tuning (cruise, turn), collision radius, weapons (T1's
  `Weapons` component) and the placeholder look. An engine shape is scaled to the radius and tinted
  red; `Visuals` (`UTDEnemyVisuals`) replaces it when Nick assigns one.
- **Attacks:** `TryStartAttack(Kind, Target)` either telegraphs and then fires, or fires
  immediately. Big attacks always telegraph, and you can read the state with `IsTelegraphing()`,
  `GetTelegraphAttack()`, `GetTelegraph01()`, `GetTelegraphDuration()` and `OnTelegraphStarted`.
  `Visuals->Telegraph` plays attached. The guns use `SetGunsFiring(bool, Target)`, which leads the
  target and applies the spec's aim error. Heavy attacks use T2's `FireOnce` (a salvo is one
  `FireOnce` = 3-missile ripple). The cooldown runs start-to-start.
- **Status:** `ApplyStun(s)` gives a dead stick and no fire, cancels the telegraph, pauses carrier
  bays and turrets, and makes the ship take +25% damage. `LoseTracking(s)` makes the AI forget its
  target and blocks re-acquiring (cloak, portals). `SuppressFire(s)` is for Gravity Well.
- **Gunship frontal shield:** a `UTDHealthComponent::DamageFilter` using `FTDDamageInfo.HitDirection`
  (falling back to the Causer→ship direction). Hits within 70° of the nose drain the 300-HP shield
  first and overflow goes to the hull; flank and rear hits ignore it. `ETDDamageKind::Ram` breaks it.
  `DropFrontalShield(s)` is the EMP drop without draining. It recovers 15 s after breaking. Events:
  `OnFrontalShieldBroken` / `Restored`.
- **Death:** the ship tumbles for a size-scaled time (0.25 s fighter up to 2.5 s carrier) and fires
  `OnEnemyDied`, then explodes (`Visuals->Explosion`, `OnEnemyExploded`) and is destroyed. A carrier
  kill fires `OnHighlightMoment(CarrierKill)`.
- **Carrier:** spawns 3 `ATDCarrierBay` and 4 `ATDEnemyTurret` children. Launched fighters fly in a
  pack and are registered with the wave director. `MaxLaunchedAlive` (12) caps them.

### `ATDCarrierBay`
Has its own health (300). It telegraphs `TelegraphSeconds` before each launch, launches every 12 s
(first launches staggered 6 / 8 / 10 s) and stops for good when destroyed. `BreakDoors()` (or any
Ram hit) doubles the damage it takes.

### `ATDEnemyTurret`
A destructible gun that finds the nearest squad ship in range through the combat registry, turns
at `TurnRateDeg` and fires bursts. If `Guns.TelegraphSeconds > 0`, every burst is telegraphed,
which is meant for ENG-18's Dominion charge shots.

### `ATDEnemyAIController`
It flies only through `SetFlightInput`. The state machine runs Approach → AttackRun → BreakOff →
re-approach, plus Formation (pack followers hold a V slot on the leader and fire with it), Idle and
Stunned. Roles:
- interceptors hunt the possessed pawn
- gunships face their target and hold 60% of engage range
- bombers fly to a point ahead of the target's velocity and lay a mine trail
- carriers hold station broadside

Targets are any squad ship (combat registry, skipping cloaked/untargetable ones). The steering
maths is the pure `ComputeSteer()`. Console: `TD.EnemyAIDebug 1` draws state labels and target
lines.

### `ATDWaveDirector` (`Waves/`)
- **Table:** the five-wave ENG-14 table is in `MakeDefaultWaves()`, as `FTDWaveDef` rows (also
  usable as a `DT_Waves` DataTable via `WaveTableOverride`).
- **Flow of a wave:** `StartWave(idx)` (0-based) → each group gets a 1.5 s red warp-in
  (`OnWarpInStarted`, `TravelFX->EnemyWarpIn`) → enemies spawn behind or flanking the anchor (the
  player pawn) in V packs → the wave clears only when every enemy is dead or destroyed, carrier
  launches included → a 6 s breather (8%/s hull repair on Player-team ships) → the next wave of the
  same phase.
- **End of a phase:** the last wave of a phase goes to `PhaseComplete` and waits for the flow.
- **Events:** `OnWaveStarted(WaveNumber, Intensity01)` / `OnWaveCleared(WaveNumber)` (1-based).
  `OnMarkerRevealed(Verdance)` fires after wave 2 (plus a hyperdrive tip) and `(Cinder)` after
  wave 4. `SetDominionState(true, false)` is called after wave 5. Tips: switching (wave 1, once),
  drift (wave 2), gunships (3), carriers (5).
- **Takes:** T5's `UTDGameInstance` owns takes. A higher `OnTakeStarted` for the current scene
  restarts the wave. Without a flow (tests), the director broadcasts `OnTakeStarted(SceneId, 1)`
  itself.
- **Control:** `StartFromProgress()` resumes at wave `WavesCleared + 1` when it belongs to the
  current phase (Intro counts as Waves1to2); otherwise it stays idle. It does nothing while a wave
  is running. Also `SkipWave()`, `StopAndClear()`, `RestartCurrentWave()`, `RegisterEnemy()`, and
  `GetWaveInfo()` for the HUD.

## How to try it
- Tests: `.\Tools\td-test.ps1 -Filter TriadDrift.Enemies` (9) and `-Filter TriadDrift.Waves` (7).
- By hand: run the sandbox
  (`UnrealEditor.exe <uproject> /Engine/Maps/Entry?game=/Script/unrealGameJam.TDSpaceGameMode -game -windowed`).
  T5's game mode spawns the director and calls `StartFromProgress()` after 3 s. `TD.SkipWave` skips
  ahead, and `TD.EnemyAIDebug 1` shows what each pilot is doing.

## Known gaps
- **Not verified by hand yet.** Everything above is proven by headless tests; AI feel, pacing and
  difficulty haven't been flown. Expect tuning (ENG-27): engage and break-off ranges, aim error,
  spawn distance.
- **Visuals:** `UTDEnemyVisuals` has no per-attack telegraph cue (one `Telegraph` cue covers all
  attacks) and no damage-state cue. The bay launch telegraph only fires `ATDCarrierBay::OnLaunchTelegraph`
  (no cue). Nick may want slots; request them in features.md §11.
- **Per-ship power tips** (first time flying each ship) and the drift tip timing belong with T3 / T2
  hooks; the director only fires the per-wave tips listed above.
- **Gunships under drift-strafe:** gunships turn at 45°/s, so a circling player gets behind them,
  which is by design (counter = flank).
- **Enemies don't avoid obstacles** (no asteroids exist yet; ENG-17).
