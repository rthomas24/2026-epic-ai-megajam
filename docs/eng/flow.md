# Game flow (ENG-19, C-03 #3): Teammate 5

Code: `Source/unrealGameJam/TriadDrift/Flow/`. Tests: `Flow/Tests/` (`TriadDrift.Flow.*`).

## What exists

| Class | What it does |
|---|---|
| `UTDGameInstance` (`ITDGameFlow`) | Holds `FTDProgress`. Covers travel, checkpoints, the save slot with Continue, scenes and takes, run stats, the run timer, story phase transitions, and the F10 debug menu. Get it with `UTDGameInstance::Get(ctx)` or `UTDStatics::GetGameFlow(ctx)`. |
| `UTDSaveGame` | One versioned slot: `Version` + `FTDCheckpoint` + scene order. The file header is checked before parsing, so a corrupt or foreign file is ignored instead of crashing. |
| `UTDFlowSettings` | Project Settings → Game → **Triad Drift Flow**: destination maps, transition map, fade, transition and timeout times, slot, retake delay, phase-after-wave table, planet objective lists, ability owners, and the sandbox class paths. |
| `ATDSpaceGameMode` | A zero-asset space sandbox (see below). |
| `UTDCheatManager` | `TD.*` console commands. `InstallOn(PC)` replaces the stock cheat manager. Compiled out of Shipping. |
| `FTDDebugMenu` | The F10 overlay (plain Slate, dev builds only), with one button per cheat. |
| `TDFlowReflection` | Late-bound calls into T3 and T4 (`SpawnSquad`, `GetActiveShip`, `StartFromProgress`, `StartWave`, `SkipWave`). |

### Progress and story
- `ReportWaveCleared(N)` takes 1-based wave numbers and is idempotent. It's ignored while a CUT is pending. It
  checkpoints and then applies `PhaseAfterWaveCleared`: 2 → Verdance, 4 → Cinder, 5 → DominionAssault.
- **Back from a planet** (`NotifyLevelEntered(Galaxy)`):
  - phase Verdance with `Obj_WreckCore` + `Obj_Relay` done → **Waves3to4**
  - phase Cinder with `Obj_Outpost` + `Obj_Jammer` done → **Wave5**
  These objective ids are fixed for ENG-25/26 (`VerdanceObjectives` / `CinderObjectives`).
- `CompleteObjective(Id, Reward, Ship)` records the objective, unlocks the reward once (`OnAbilityUnlocked`) and checkpoints.
- **Story state is never rolled back by a retake.** `SetPhase`, `SetDominionState` and `UnlockAbility`
  patch the last checkpoint and the slot. A restore that changes the phase re-broadcasts `OnPhaseChanged`, and so does Continue.
- **Stats:** `AddStat(FieldName, Amount)` resolves the field by name. From the bus `OnActorKilled`:
  - an enemy killed by the player team counts as Kills
  - the death of the pawn the player is piloting counts as Deaths (wingmen don't count)
- **Run timer:** real time. It's stopped on the main menu, during travel and at Victory.

### Scenes and takes (C-03 #3)
- A scene is a wave (`"Wave3"`) or an objective (its id). `BeginScene(Id)` → take 1 → bus `OnTakeStarted`. The GI
  also opens wave scenes from the bus `OnWaveStarted`, and doesn't re-fire the slate for a take it has already announced.
- `ReportPlayerDefeated()` is **CUT!**:
  - Right away, it fires bus `OnSceneCut(Scene, NextTake, Delay)` for Nick's card.
  - After `RetakeDelaySeconds` (1.75 s) of **real time**, the last checkpoint is restored. Slow motion and pause don't stretch this delay.
  - Takes, stats and run time are kept. The take goes up and the slot is written.
  - Then native `OnSceneRestart` fires (game modes respawn the player or squad), and bus `OnTakeStarted` fires (T4 rebuilds its wave).
- A death outside any scene still cuts and restarts, with scene None and take 0, so the card always clears.
- `GetSceneTakes()` gives the victory screen its rows.

### Save safety
- The main menu and Victory never checkpoint or write.
- A pristine run is never written.
- A fresh boot doesn't touch an existing save until one of these happens:
  - `StartNewRun()` (New Game)
  - `ContinueRun()`
  - real progress (a wave cleared or an objective completed)
- `ContinueRun()` returns false if travel fails.
- `HasContinue()` is cached and refreshed on every write or delete.

### Travel
`TravelTo(Dest, ReturnPoint)` first checks that the map exists; if it doesn't, it returns false and changes
nothing. Otherwise travel runs on the real-time flow clock, which keeps working while the game is paused:
1. It unpauses the game.
2. It fades out over `TravelFadeSeconds`.
3. It opens `L_Transition`, if that exists, and holds there for `MinTransitionSeconds`.
4. It opens the target, once.

If a different level loads in the meantime, the travel is dropped. A watchdog (`TravelTimeoutSeconds`, 30 s)
clears a travel that never arrives. `IsTravelPending()` is public; T3 uses it to suppress slow motion.

## How to try it
```
"C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor.exe" "<worktree>\unrealGameJam.uproject" /Engine/Maps/Entry?game=/Script/unrealGameJam.TDSpaceGameMode -game -windowed
```
On an empty map it spawns:
- lights, fixed exposure, stars and asteroids (engine basic shapes)
- T3's squad: there's no login pawn when the squad class exists, and stray default pawns are destroyed
- T4's wave director

**F10** opens the debug menu. Console commands: `TD.Phase <phase>`, `TD.Unlock <ability> [ship]`, `TD.God`, `TD.KillAll`,
`TD.SkipWave`, `TD.Travel <destination>`, `TD.Take`.

Tests (`.\Tools\td-test.ps1 -Filter TriadDrift.Flow`):

| Area | Tests |
|---|---|
| Progress and story | `ProgressMutations`, `AbilityUnlocks`, `StoryPhaseTransitions` |
| Checkpoints and saves | `CheckpointRestore`, `SaveSlotRoundTrip`, `SaveSafety` (menu, pristine, fresh boot, Continue, foreign and corrupt saves), `StoryStateSurvivesRetakeAndSave` |
| Scenes and stats | `ScenesAndTakes` (real delayed CUT path), `StatsAndTimer`, `KillsAndDeathsFromBus`, `Events` |
| Travel | `TravelMissingMapFailsSafely`, `TravelSucceedsAndRecovers` (transition hold, arrival, superseded, watchdog) |
| Cheats and sandbox | `Cheats` (every `TD.*` command), `SandboxBootsOnEmptyWorld`, `SandboxPlayerHandOver` |

## Journey stages (cheats, playtests, F10)
`TD.Stage <stage>` (also the F10 menu's "Journey stage" row) puts **progress and the world** in a stage:

| Stage | Where | Progress |
|---|---|---|
| `Waves1to2` | Galaxy, wave 1 running | fresh |
| `VerdanceArrival` | Verdance, squad descending | waves 1-2 cleared |
| `VerdanceOnFoot` | Verdance, landed, pilot on foot (`Verdance_Pad`) | waves 1-2 cleared |
| `Waves3to4` | Galaxy at `Verdance_Orbit`, wave 3 running | Verdance done: Gravity Well (Bastion), Cloak (Rift) |
| `CinderArrival` / `CinderOnFoot` | Cinder, descending / at `Cinder_Pad` | waves 1-4 cleared, Verdance done |
| `Wave5` | Galaxy at `Cinder_Orbit`, wave 5 running | Cinder done: EMP Pulse (Lancer) |
| `DominionAssault` | Galaxy at `Dominion_Approach`, Dominion warped in, shields up, no wave | Dominion revealed |
| `ReactorExposed` | as above, all generators down | defenses down |
| `Victory` | phase Victory (clock stopped) | everything |

- **Galaxy stages in the galaxy** are applied in place:
  - the wave director runs `StopAndClear` and then `StartFromProgress`
  - the Dominion runs `SyncWithProgress`, and `DebugDropShields` for the reactor stage
  - the squad is respawned at the stage's return point
- **Any other stage** travels there with that return point.
- Takes, stats and run time are kept. `TD.Unlock All` puts every ability on its owner ship.

Tests:
- `Flow.Journey.GalaxyStagesApplyInPlace` (real space mode, galaxy layout, director and Dominion)
- `Flow.Journey.PlanetStagesTravelWithTheRightReturnPoint`
- `Flow.Journey.StageAndUnlockAllCheats`

## Placeholder sky (N-004, agreed with Nick's side)
All placeholder lighting and exposure is one actor, `ATDPlaceholderSky` (tag `TD.Placeholder.Sky`). It holds the sun (forward-shading priority 1), the fill (0), the sky light, a planet atmosphere, and an unbound post-process at priority -100. Both game modes spawn it only when the map has no sky of its own:

| The map has | Placeholder effect |
|---|---|
| An actor tagged `TD.Presentation.Sky` | Off entirely (and no star backdrop) |
| Any actor with a `UDirectionalLightComponent` | Our lights go |
| Any unbound `UPostProcessComponent` or `APostProcessVolume` | Our exposure and bloom go |

It re-scans every 0.5 s, so a sky that streams in late takes over. Test: `Flow.PlaceholderSkyYieldsToTheMapsSky`.

## Known gaps
- **Not tested by hand:** the windowed sandbox and the F10 menu haven't been played by hand (worktrees are headless).
- **Squad hand-over:** the check runs fully only once T3's squad manager is on main. Until then it logs and skips.
- **Cooking:** the maps don't exist yet. The cook needs `/Engine/BasicShapes` and the destination maps listed: the
  DefaultGame.ini lines are with the Lead.
- **God mode:** it applies to actors alive when it's toggled.
