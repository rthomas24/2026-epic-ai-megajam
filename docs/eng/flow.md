# Game flow (ENG-19, C-03 #3): Teammate 5

Code: `Source/unrealGameJam/TriadDrift/Flow/`. Tests: `Flow/Tests/` (`TriadDrift.Flow.*`).

## What exists

| Class | What it does |
|---|---|
| `UTDGameInstance` (`ITDGameFlow`) | Holds `FTDProgress`. Covers travel, checkpoints, the save slot with Continue, scenes and takes, run stats and the run timer. Get it with `UTDGameInstance::Get(ctx)` or `UTDStatics::GetGameFlow(ctx)`. |
| `UTDSaveGame` | One versioned slot: `Version` + `FTDCheckpoint` + scene order. A slot with a different version is refused, never half-loaded. |
| `UTDFlowSettings` | Project Settings → Game → **Triad Drift Flow**: destination → map table, transition map, fade and transition times, slot name, retake delay, phase-after-wave table, ability owners, and the sandbox class paths. |
| `ATDSpaceGameMode` | A zero-asset playable sandbox (see below). |
| `UTDCheatManager` | `TD.*` console commands (see below). The logic is compiled out of Shipping. |
| `TDFlowReflection` | Late-bound calls (`SpawnSquad`, `GetActiveShip`, `StartFromProgress`, `StartWave`, `SkipWave`), so Flow compiles without T3 or T4. |

### Progress rules
- `ReportWaveCleared(WaveNumber)` is **1-based** and idempotent: it ignores numbers ≤ `WavesCleared`. It closes the scene,
  applies `PhaseAfterWaveCleared` (2 → Verdance, 4 → Cinder, 5 → DominionAssault) and takes a checkpoint.
- `CompleteObjective(Id, Reward, Ship)` records the objective and unlocks the reward, firing
  `OnAbilityUnlocked` (once only). It also closes the scene and takes a checkpoint.
- **Checkpoints** are taken on `NotifyLevelEntered` (game modes call it from StartPlay), on wave clear, on objective
  complete, and on `SaveCheckpoint()`. Each one writes the slot (`bWriteSlotOnCheckpoint`).
- **Stats:** `AddStat(FieldName, Amount)` resolves the `FTDRunStats` field by name. The GI also listens to the
  bus `OnActorKilled`:
  - an enemy killed by the player team counts as Kills +1
  - the death of the pawn the player is piloting counts as Deaths +1; wingmen being downed don't count
- **Run timer:** real time (slow motion doesn't stretch it). It's paused on the main menu, during travel and on Victory.

### Scenes and takes (C-03 #3)
- A scene is one wave (`"Wave3"`, `UTDGameInstance::WaveSceneId`) or one objective (its objective id).
- `BeginScene(Id)` opens it: take 1 the first time, and it fires `OnTakeStarted(Id, Take)` on the bus. The GI
  also calls this itself from the bus's `OnWaveStarted`. Re-opening the take already announced doesn't fire the slate again.
- `ReportPlayerDefeated()` is **"CUT!"**: after `RetakeDelaySeconds` (1.75 s, for Nick's CUT! card) the last
  checkpoint is restored. **Takes, stats and run time are kept.** The scene's take goes up, and then:
  - native `OnSceneRestart(Scene, Take)` fires: the game mode respawns the squad, and T4's director restarts its wave
  - bus `OnTakeStarted(Scene, Take)` fires
- `GetSceneTakes()` gives the victory screen its rows (scene id, slate number, takes) in play order.

### Travel
`TravelTo(Dest, ReturnPoint)` checks that the destination's map exists before doing anything. If it doesn't, it
logs a warning and returns false with no state change. Otherwise it fades out over `TravelFadeSeconds`, opens
`L_Transition` if it exists (and holds there for `MinTransitionSeconds`), then opens the target map.
`ContinueRun()` applies the slot and travels to the saved destination.

## How to try it

The sandbox needs no assets:
```
"C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor.exe" "<worktree>\unrealGameJam.uproject" /Engine/Maps/Entry?game=/Script/unrealGameJam.TDSpaceGameMode -game -windowed
```
On an empty map it spawns:
- a key light, a violet fill light, a sky light and fixed-exposure post-processing
- about 1,200 emissive star spheres and 24 asteroids (engine basic shapes, seeded layout)
- T3's squad manager (`SpawnSquad(SquadOrigin)`, then possesses `GetActiveShip()`)
- T4's wave director (`StartFromProgress()` after `FirstWaveDelaySeconds`, 3 s)

Missing classes are logged and skipped. Without a squad you get a free-fly `ADefaultPawn`.

Console (`~`):

| Command | Effect |
|---|---|
| `TD.Phase Cinder` | set the phase |
| `TD.Unlock Cloak [Rift]` | unlock an ability (the ship defaults to the ability's owner) |
| `TD.God` | toggle invulnerability for the player's team |
| `TD.KillAll` | kill every enemy |
| `TD.SkipWave` | skip the current wave |
| `TD.Travel Verdance` | travel to a destination |
| `TD.Take` | force CUT! and a retake of the current scene |

Tests:
```powershell
.\Tools\td-test.ps1 -Filter TriadDrift.Flow
```
- `ProgressMutations`
- `AbilityUnlocks`
- `CheckpointRestore`
- `SaveSlotRoundTrip`
- `ScenesAndTakes`
- `StatsAndTimer`
- `Events`
- `TravelMissingMapFailsSafely`
- `SandboxBootsOnEmptyWorld`

The tests use the save slot `TriadDrift_AutomationTest` and never touch the real save.

## Known gaps
- **Config:** `GameInstanceClass=/Script/unrealGameJam.TDGameInstance` must be set in `DefaultEngine.ini` (Lead).
  Until it is, the sandbox logs that progress, checkpoints and takes are off.
- **Maps:** none of the maps exist yet (`L_Galaxy`, `L_Planet_*`, `L_Transition`, `L_MainMenu`), so `TD.Travel` fails
  safely for now. The fade uses the camera manager; Nick's loading widget belongs on `L_Transition`.
- **Defeat menu:** ENG-19's "Retry from checkpoint / Quit" is a UI choice. Retry is `RestartSceneNow()`, and Quit
  is `TravelTo(MainMenu)`. The ENG-19 F10 debug menu widget is Nick's UI, and it calls the `UTDCheatManager::Do*` functions.
- **God mode:** it applies to actors alive when toggled. Ships respawned later aren't invulnerable until you toggle again.
- **Late binding:** the sandbox reaches T3 and T4 by reflection. Their functions must stay `UFUNCTION`s with these names.
- **Not tested by hand:** the 9 `TriadDrift.Flow.*` tests pass headless, and the sandbox boots in a test world.
  No one has played the windowed sandbox yet, and the stars use `EmissiveMeshMaterial`, which is still unverified.
