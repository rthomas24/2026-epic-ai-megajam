# Director's Cut: gameplay side (Teammate 3)

C-03 #1 (switching is a cut) and C-03 #2 (director cam money shots). Presentation (cut transition,
letterbox, name card, camera rigs, grade) is Nick's (ART-09/12); this is the data and timing he
plugs into. Code: `Source/unrealGameJam/TriadDrift/Squad/TDSwitchCut.h` and
`Source/unrealGameJam/TriadDrift/Director/`.

## C-03 #1: switch-as-cut

- `ATDSquadManager::OnSwitchCut(const FTDSwitchCut&)` (BlueprintAssignable) fires **in the same
  call** as every successful switch, right after `UTDEventBus::OnShipSwitched`. The switch itself
  is never delayed: by the time the event fires, the player already flies the new ship.
  `GetLastSwitchCut()` returns the latest one.
- `FTDSwitchCut`: new/old ship ids and actors; `PilotName`, `ShipName`, `RoleName`, ready-made
  `NameCardText` ("KATO · LANCER"), `SquadColor`; `bAutoSwitch` (forced by death), `bSwitchStrike`
  (this switch armed +25%), `bQuickCut` (another switch within 1.2 s: play the light version),
  `SwitchIndex`; `FromCamera` / `ToCamera` world transforms of the two chase cameras at the cut;
  `Timing`.
- `FTDSwitchCutTiming` (tunable on the manager as `CutTiming`): 1-frame flash, letterbox in 0.08 s /
  hold 0.45 s / out 0.25 s, name card 1.4 s, quick-cut window 1.2 s. These are suggestions.
- The new ship's chase camera is snapped on switch-in (`ATDShip::SnapCamera`), so the cut is hard.

## C-03 #2: money shots (`UTDDirectorSubsystem`, a world subsystem)

`UTDDirectorSubsystem::Get(this)` exists in every game/PIE world; nothing needs spawning.

**Detected automatically** from `UTDEventBus::OnActorKilled` (player-team killer, hostile victim):

| Kind | Rule | Base importance |
|---|---|---|
| `ReactorKill` | victim tagged `TDDirectorTags::Reactor` (`TD.Highlight.Reactor`) | 1.0 |
| `GeneratorKill` | victim tagged `TD.Highlight.Generator` | 0.85 |
| `CarrierKill` | victim tagged `TD.Highlight.Carrier` | 0.8 |
| `RamKill` | damage kind `Ram` | 0.6 |
| `MultiKill` | 3+ player kills within 1.5 s (game time), +0.1 per extra kill | 0.5 |
| `SwitchStrikeKill` | killer is the ship with Switch Strike running | 0.5 |

A kill matching several picks the most important. **Reported by the owner** with
`ReportHighlight(Kind, Subject, Importance01 = -1, Secondary = nullptr)`: `MissileReturn` (T2
portals, 0.55), `PerfectDeflect` (T5 blade, 0.6), `AbilityCombo` (0.5), or any kind with a custom
importance.

**Rate limiting** (all real time, all in `Tuning`): ignored below 0.3; while a shot runs, a new one
replaces it only if 0.25 more important, but story beats (≥ 0.8) replace any shot that isn't more
important (so all four generator kills get their beat); after a shot, 3 s global cooldown and 6 s
per kind, both bypassed by story beats.

**On accept:** `UTDFeelSubsystem::RequestSlowMo(0.5–1.0 s by importance, dilation 0.4 → 0.2;
per-kind overrides in `Tuning.DurationOverrideSeconds`, default PerfectDeflect = 0.15 s per ENG-22)`,
`UTDEventBus::OnHighlightMoment(Kind, Subject, Importance01)`, `AddStat("HighlightMoments")`, and
`OnDirectorShot(const FTDDirectorShot&)` on the subsystem.

**Camera hint** (`GetCurrentShot()`, `IsShotActive()`): `Subject` / `Secondary`, `FocusLocation`
(two-shot weighted to the subject), `SuggestedCameraLocation` / `Rotation` (side-on, a little ahead
and above, 2500–7000 cm by importance, alternating sides shot to shot), duration, dilation, start.

**Never during a retake or travel:** suppressed from `OnSceneCut` until the next `OnTakeStarted`
(and a running slow-mo is cancelled on "CUT!"), and while `UTDGameInstance::IsRetakePending()` or
`IsTravelPending()`.

## How to try it

- `.\Tools\td-test.ps1 -Filter TriadDrift.Director` and `-Filter TriadDrift.Squad.Switch.CutPayload`.
- In the sandbox: ram an enemy with Lancer, or kill three fighters quickly.

| Test | Proves |
|---|---|
| `Director.RamKillIsAMoneyShot` | event, 0.5–1 s slow-mo, shot frames the rammer, ends cleanly |
| `Director.MultiKillWindow` | 3 kills in 1 s fire; 3 over 1.6 s don't |
| `Director.RateLimitedButBigMomentsPreempt` | suppression during a shot and cooldowns; generator preempts |
| `Director.ObjectiveKillsByTag` | carrier/reactor by tag; reactor preempts carrier; four generator kills = four beats |
| `Director.EnemyKillsIgnored` | enemy kills never trigger |
| `Director.SuppressedDuringRetake` | "CUT!" cancels slow-mo and blocks highlights until the next take |
| `Director.CameraHint` | focus, distance, look-at, alternating sides |
| `Director.ReportHighlightApi` | direct reports, default importance, minimum, null subject |
| `Director.SwitchStrikeKill` | kill during Switch Strike |
| `Director.SuppressedDuringTravel` | no highlights while `IsTravelPending()` (real TravelTo) |
| `Director.PerfectDeflectIsAShortBeat` | ENG-22's 0.15 s deflect beat |
| `Squad.Switch.CutPayload` | same-frame payload, name card, quick cut, auto-switch cut |

## Known gaps / needs from others

- T4 / ENG-18: tag the generator, reactor and carrier actors with `TDDirectorTags::*` (or call
  `ReportHighlight`).
- T2: call `ReportHighlight(MissileReturn, ...)` on a portal missile return. T5: `PerfectDeflect` on a
  perfect blade deflect.
