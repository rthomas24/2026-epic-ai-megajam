# Engineering status ledger (Lead-owned)

The Lead updates this after every review and merge. An item is DONE only when it is merged to
`main`, the build is clean, its automation tests pass headless on `main`, and review findings are closed.
The short cross-team view (in progress / coming next / done) is the board in `/CLAIMS.md`.

Status values: `todo` / `in progress` / `in review` (merged, tests pass, review open) / `DONE` / `BUG`

Nothing below has been hand-flown in a windowed session yet: every "tests pass" is headless automation.

| Item | Owner | Status | On main | Tests (filter, count) | Notes |
|---|---|---|---|---|---|
| C-01 project bootstrap | Lead | DONE | `802aa8c` | - | Project, LFS, collision channels, claims board, message board, five maps (World Partition, streaming off), game modes by map name |
| C-02 contracts (Core) | Lead | DONE | `1cf7fe1` + additions | `TriadDrift.Core` (8) | Types, interfaces, event bus, health, registry, statics, feel, presentation assets, test harness, dev HUD, placeholder tint |
| C-03 #1 switching is a cut | T3 | in review | this merge | `TriadDrift.Squad.Switch.CutPayload` | `OnSwitchCut(FTDSwitchCut)` payload for Nick's cut and name card. Review running |
| C-03 #2 director-cam money shots | T3 | in review | this merge | `TriadDrift.Director` (9) | `UTDDirectorSubsystem`: auto highlights, rate limit, slow-mo, camera hint. Needs T2 / T5 / T4 call sites and tags. Review running |
| C-03 #3 scenes and takes | T5 | in review | this merge | `TriadDrift.Flow.ScenesAndTakes` | Real delayed path, `OnSceneCut` broadcast. Needs second review pass |
| ENG-01 flight model | T1 | in review | `802aa8c` | `TriadDrift.Flight` (22) | All 15 review findings reported fixed (`e046c82`), merged; second review pass queued; not hand-flown |
| ENG-02 input map | T1 | in review | `802aa8c` | `TriadDrift.Flight` | Power keys routed with press and release, real key-injection tests; power release on switch still open; second review queued |
| ENG-03 health / death / respawn | Lead + T3 | in review | `448f658` | `TriadDrift.Core.Health`, `TriadDrift.Squad` | Component done; squad respawn rules merged. Flight and weapon state reset on respawn still open (T1 / T2 / T3) |
| ENG-04 three ships | T3 | in review | `448f658` | `TriadDrift.Squad` (21) | Review running |
| ENG-05 ship weapons | T2 | in review | `6bd14ec` | `TriadDrift.Combat` (19) | Review done: pool identity, retake cleanup, tracer pooling, 11 minor, test gaps. T2 fixing |
| ENG-06 targeting | T2 | in review | `6bd14ec` | `TriadDrift.Combat` | Review done: lead and aim assist wrong under drift (MAJOR), HUD data not on `ITDHUDData` (`ITDHUDData` getters merged in `0734b8c`; squad forwarders pending). T2 fixing |
| ENG-07 squad + switching | T3 | in review | `448f658` | `TriadDrift.Squad` | Review running |
| ENG-08 wingman AI | T3 | in review | `448f658` | `TriadDrift.Squad` | Review running. Form-up on hyperdrive spool and passive-for-landing added |
| ENG-09 Rift portal blaster | T2 | in review | `6455ee0` merged | `TriadDrift.Powers` (16) | Merged, tests pass; independent review running |
| ENG-10 Lancer ram | T2 | in review | `6455ee0` merged | `TriadDrift.Powers` (16) | Merged, tests pass; independent review running |
| ENG-11 Bastion swarm + dome | T2 | in review | `6455ee0` merged | `TriadDrift.Powers` (16) | Merged, tests pass; independent review running |
| ENG-12 unlockable abilities | T2 | in review | `6455ee0` merged | `TriadDrift.Powers` (16) | Merged, tests pass; independent review running |
| ENG-13 enemy ships + AI | T4 | in review | this merge (`7ae379c`) | `TriadDrift.Enemies` (27) | Blocker and all four majors reported fixed with real-path tests (two real bugs found by them); minors open; second review queued |
| ENG-14 wave director | T4 | in review | this merge (`7ae379c`) | `TriadDrift.Waves` (7) | Leash, spawn-at-announce and no-spawn-ahead merged; minors open; second review queued |
| ENG-15 hyperdrive | T1 | in review | `802aa8c` | `TriadDrift.Travel` (9) | First pass merged. Known: no Idle broadcast when switching mid-spool, override kept by a switched-away ship. Not reviewed yet |
| ENG-16 portal gates | T4 | in review | this merge | `TriadDrift.World` (14 with ENG-17, 18) | On T2's `UTDPortalTransit`; includes a real enemy projectile returned through a gate. Not reviewed yet |
| ENG-17 galaxy layout | T4 | in review | this merge | `TriadDrift.World` | `ATDGalaxyLayout`, atmospheres, obstacle registry, enemy avoidance. Bounds / gravity wells switch to T1's Travel API when it lands. Not reviewed yet |
| ENG-18 Dominion assault + finale | T4 | in review | `571d85f` | `TriadDrift.World.Dominion` (5) | Cues wired, silent resume. Check generators and turrets for the same reachability bug as carrier bays. Not reviewed yet |
| ENG-19 game instance / saves / travel | T5 | in review | this merge | `TriadDrift.Flow` (16) | All 12 review findings reported fixed (`125146f`); needs a second review pass to close |
| ENG-20 atmosphere flight + landing | T1 | in progress | `802aa8c` (first pass) | `TriadDrift.Travel` | First pass merged; end-to-end level change, landing hand-off and take-off with T5 in progress |
| ENG-21 first-person character | T5 | in review | `802aa8c` | `TriadDrift.Ground` (19 with 22, 24) | Merged; tests drive the functions behind the bindings (no LocalPlayer headless); not reviewed, not hand-played |
| ENG-22 energy blade | T5 | in review | `802aa8c` | `TriadDrift.Ground` | Merged; real projectile deflect tests at 30/60/120 fps; not reviewed |
| ENG-23 crew enemies | T5 | in progress | | `TriadDrift.Ground` | Stand-in dummies until this lands |
| ENG-24 objective framework | T5 | in review | `802aa8c` | `TriadDrift.Ground` | Merged; training-ground sandbox runs on `L_Planet_Verdance`; not reviewed |
| ENG-25 Verdance gameplay | T5 | todo | | `TriadDrift.Ground` | Objective ids fixed: `Obj_WreckCore`, `Obj_Relay` |
| ENG-26 Cinder gameplay | T5 | todo | | `TriadDrift.Ground` | Objective ids fixed: `Obj_Outpost`, `Obj_Jammer` |
| ENG-27 balance + difficulty | Lead | todo | | | |
| ENG-28 integration / perf / packaging | Lead | in progress | main | all (202) | Build, test, screenshot and packaging tools work. Baseline Development package was 1.1 GB with template content: must get under 1 GB |
| ENG-29 docking (stretch) | - | todo | | | Only after core is solid |
| ENG-30 interior + core fight (stretch) | - | todo | | | |
| ENG-31 escape (stretch) | - | todo | | | |

## Open review findings (owner fixes, Lead re-verifies)
- Flight / Input (T1): drift burst cap, camera roll, power routing; boost stutter, pinned-contact impact spam, world-level boundary, flight reset on respawn, balanced drift events, pause while paused, disabled-input push, LeftAlt+Tab, tunables; camera and key-injection tests.
- Combat (T2): lead under drift, pool launch serial, tracer pooling, `KillAll` on CUT, HUD data on `ITDHUDData`; ripple target capture, blocked trigger restore, dead-owner gate, breaker heat tap exploit, `BreakTracking`, splash direction, mesh reset on reuse, charge loop stop, weak pointers, manual target carry-over; behaviour tests at 30 / 60 / 120 fps.
- Enemies / Waves (T4): carrier parts reachability (BLOCKER), leash and spawn point timing, time-to-impact break-off and non-solid wrecks, per-type visuals, real-path tests; large ships spawning ahead, double rotation, re-entrant `BeginWave`, scene id source of truth (with T5), marker re-announce, retarget cost, aim seed, separation.
- Flow (T5): reported fixed, awaiting second review.
- Squad / Director (T3): review done. BLOCKER: highlight producers bypassed the director; wingmen stuck in form-up after a switch during hyperdrive. MAJOR: stale aim target after hand-over, per-ship art not assignable, no death tumble or flight reset, farmable switch invulnerability, cooldown on dilated time, no switch guard for landing / hyperdrive, wingman collisions, multi-kill escalation.
- Combat (T2) fixes are finished on `eng/t2` `b4a6dc2` but held out of `main`: unity-build name collisions (`AdvanceUntil`, `CountLive`, `Steps`).

## Integration log
- `1cf7fe1` Core foundation on main. Build clean, `TriadDrift.Core` 8/8 passing headless (14 s).
- `dcb9469` T1 flight/input, T2 combat, T5 flow merged. 17/17.
- `2fa9a05` T1 + T2 + T4 test commits merged. Full suite 63/63 headless (20 s). Baseline packaging (Development) succeeded in 4.5 min, 1.1 GB with template content.
- T3 `0e8d85f` held back: `Squad.Wingmen.HoldFormation` failed against T1 `ef345ca`. Root cause was a formation chase U-turn; fixed with T1's `SetThrottleTopSpeed` hook.
- `642f232` Core: extra enemy cue slots, `UTDDominionVisuals`; `td-shot.ps1` visual capture working.
- `448f658` T3 squad, T4 Dominion (ENG-18) merged. 86/86.
- `609ffb1` Nick's ten text-only art-source commits integrated; Ryan-side claims published in `CLAIMS.md`.
- This merge: T5 `125146f` (flow review fixes), T3 `1309c5b` (C-03 #1, #2), T4 `ea32ff5` (ENG-16, ENG-17), Core dev HUD + placeholder tint + pooled cues, `CLAIMS.md` board and AGENTS.md start / finish rule. Build clean, 114/114 headless (26 s): Combat 19, Core 8, Director 9, Enemies 9, Flight 11, Flow 16, Squad 21, Waves 7, World 14. Sandbox capture shows the dev HUD with squad cards, wave counter and hostile markers.
- T2 `6455ee0` powers and abilities (ENG-09 to ENG-12) and `0734b8c` targeting HUD data on `ITDHUDData` merged. Build clean, 130/130 headless (28 s): Powers 16 added.
- `802aa8c` Five maps created headless (`Tools/td-make-maps.py`), `MESSAGEBOARD.md` + AGENTS.md 6.10, T1 `75649e8` (15 flight review fixes, ENG-15, ENG-20 first pass, placeholder look), T3 `9c93bd1`, T4 `caabffe`, T5 `fc329d0`. Build clean, 189/189. Captures: `L_Galaxy` shows the magenta Rift, planet and asteroid field with the dev HUD; `L_Planet_Verdance` shows the first-person training ground.
- This merge: T4 `7ae379c`, T1 `fb07787`, T5 `85af40b` (menu and transition game modes), dev HUD pawn fix. Build clean, 202/202 before T5's last merge: Combat 19, Core 8, Director 10, Enemies 27, Flight 22, Flow 18, Ground 19, Powers 16, Squad 31, Travel 9, Waves 7, World 16.
