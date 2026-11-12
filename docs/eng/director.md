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
| `Director.Real.GeneratorAndReactorKills` | T4's real Dominion: each of 4 generators and the reactor = one highlight + one shot + slow-mo |
| `Director.Real.CarrierKill` / `PortalMissileReturn` / `RamKill` | real carrier, real portal return through Rift's portals, real Lancer ram: one each |
| `Director.MultiKillSustainedAndCapped` / `WingmanKillsDontCountForMultiKill` | multi-kill re-triggers, stays below story tier, real-time window, active ship only |
| `Squad.Switch.CutPayload` | same-frame payload, name card, quick cut, auto-switch cut |

## Known gaps

- None open. Producers: T4 tags generators/reactor/carriers, T2's portals and powers and T5's blade
  call `ReportHighlight`; `UTDEventBus::BroadcastHighlightMoment` is private to the Director, so a
  producer can't bypass it.

## The director camera (C-03 #2, `TDDirectorCamera.h`)

The money shot now really cuts:
- On an accepted shot (mode **Full**), `UTDDirectorCameraRig` makes `ATDDirectorCameraActor` the
  player's view target **starting exactly on the live chase camera** (T1's
  `GetChaseCameraTransformNow` / `GetChaseCameraFOVNow`), blends in (0.12 s; story beats 0.18 s),
  holds framed on subject + secondary (tracks them live, FOV 65) for the shot, then blends back onto
  the live chase camera (0.2 s / 0.3 s) and hands the view back. All in real time, so slow-mo
  doesn't stretch it. Input is never touched: the player keeps flying and aiming (T1 da82e13 made
  aim independent of the view target).
- **Placement** (`UTDDirectorCameraRig::ResolveCameraLocation`): the hint spot if a camera-sized
  sweep from the focus is clear of WorldStatic / WorldDynamic / Pawn / Ship and T4's obstacle
  registry; else the other side, the quarter turns, then above; pulled in when blocked; never
  within 900 cm of the focus or of the player's ship. No safe spot → no takeover (FOV punch instead).
- **Cancelled instantly** (view straight back to the pawn) on "CUT!", travel, a ship switch, the
  player's ship dying, a subject destroyed mid-shot, the mode being switched off, or anything else
  taking the view. A bigger shot mid-shot re-places the camera and blends from where it is.
- **Modes** (accessibility): Project Settings > Game > Triad Drift Director `CameraMode`
  (Full / Reduced / Off), overridden by `td.DirectorCam` (-1 setting, 0 off, 1 reduced, 2 full).
  Reduced = slow-mo + a 9° FOV punch for 0.35 s real time on the player's own camera
  (`UTDDirectorFOVPunch` camera modifier). Off = slow-mo and events only.

| Test | Proves |
|---|---|
| `Director.Camera.RamKillCutsAndReturns` | real ram kill → view taken, hold away from the ship, player still flying, back onto the live chase camera (30/60/120) |
| `Director.Camera.CarrierAndGeneratorKills` | real carrier and real Dominion generator kills cut and return |
| `Director.Camera.PortalMissileReturn` | real portal missile return cuts and returns |
| `Director.Camera.EveryCancelPathReturnsTheView` | CUT, switch (view on the new ship), player down, disabled mid-shot (30/60/120), travel, bigger shot takeover; nothing stuck afterwards |
| `Director.Camera.NeverInsideGeometry` | wall on the preferred side → other side at full distance; registry asteroid avoided; ≥ 900 cm from the player's ship |
| `Director.Camera.ReducedAndOffModes` | reduced: no takeover, FOV punch in real time; off: neither, slow-mo still |

## Fallback cut presentation (C-03 #1 / #2 / #3, `TDCutPresentation.h`)

So the theme reads in every build even before Nick's HUD exists, the Director draws a minimal
gameplay-side presentation **only when no real HUD claims it**. Nick's HUD (or any host) sets
`UTDDirectorSubsystem::bCutPresentationHandledExternally = true` and the fallback draws nothing.

- **Ship switch** (`ATDSquadManager::OnSwitchCut`): a 1-frame white flash; letterbox bars easing
  with `FTDSwitchCutTiming` (in 0.08 s / hold 0.45 s / out 0.25 s; none on a quick re-switch); the
  pilot name card ("KATO · LANCER") with a tab in the squad colour, 1.4 s.
- **"CUT!"** (`OnSceneCut`): a big CUT! card with full bars for the retake delay (min 1.2 s).
- **New take** (`OnTakeStarted`): a slate "SCENE Wave 3 · TAKE 2" for 2.2 s (CamelCase scene ids
  are spaced; an unnamed take shows none).
- **Money shot**: thin letterbox bars (6 %) for the shot and its return.
- Implementation: `UTDCutPresentation` (the model, real-time, tested) + a hit-test-invisible Slate
  leaf widget added to the game viewport at z 900 (under T5's F10 menu) that paints it with engine
  fonts and the white brush. Never blocks input; cleared while travel is pending; removed with the
  world. Headless worlds have no viewport: model only.

| Test | Proves |
|---|---|
| `Director.CutPresentation.SwitchFlashLetterboxNameCard` | 1-frame flash, bars in/out, name card text and colour, all gone after (30/60/120) |
| `Director.CutPresentation.RealTimeUnderSlowMoAndQuickCut` | slow-mo doesn't stretch it; quick re-switch = flash + card, no bars |
| `Director.CutPresentation.ExternalHUDSuppressesIt` | external flag suppresses everything; clearing it restores the fallback |
| `Director.CutPresentation.CutCardThenSlate` | CUT! held for the delay, replaced by "SCENE Wave 3 · TAKE 2"; no slate for an unnamed take |
| `Director.CutPresentation.MoneyShotThinBars` | thin bars during an accepted shot |
| `Director.CutPresentation.NothingSurvivesTravelOrShutdown` | travel clears it; shutdown clears and removes the widget |

## Playtesting the Director's Cut live (agents)

`Tools	d-play-mcp.ps1` (port 8013 for t3) + `Tools	d_mcp.py`. Dev-build console commands
(`Director/TDDirectorDebugCommands.cpp`) drive the real paths, so no input injection is needed:
`TD.Squad.Switch <Rift|Lancer|Bastion|Next|Prev>`, `TD.Squad.Wipe` (real wipe → CUT! → retake +
slate), `TD.Director.RamKill` (Lancer's real ram into a parked fighter), `TD.Director.RevealDominion`
then `TD.Director.GeneratorKill`, and `TD.Director.Status` (logs mode, shot count, camera phase,
overlay state). One MCP round trip plus a capture takes 2-3 s, longer than a shot, so set the
dev-only cheat `td.Director.DebugHoldSeconds 3` first: it lengthens camera holds and every card
(0 by default, compiled out of Shipping). Captures land in `Saved/TDShots/mcp/<name>/`.

