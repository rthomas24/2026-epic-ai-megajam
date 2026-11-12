# Engineering status ledger (Lead-owned)

The Lead updates this after every review and merge. An item is DONE only when it is merged to
`main`, the build is clean, its automation tests pass headless on `main`, and review findings are closed.
The short cross-team view (in progress / coming next / done) is the board in `/CLAIMS.md`.

Status values: `todo` / `in progress` / `in review` (merged, tests pass, review open) / `DONE` / `BUG`

Mapping to the stages in `/AAA.md`: `in progress` on a branch with passing tests is *prototype complete*;
`in review` is *technically verified* and *integrated*; `DONE` additionally needs every review BLOCKER and
MAJOR closed and the item *gameplay validated* (played in a running game, noted in `DEVLOG.md`).
*Presentation approved* is tracked on Nick's side in `docs/visual/STATUS.md`. **No item is `DONE` yet.**

Nothing below has been hand-flown in a windowed session yet: every "tests pass" is headless automation.

| Item | Owner | Status | On main | Tests (filter, count) | Notes |
|---|---|---|---|---|---|
| C-01 project bootstrap | Lead | DONE | `802aa8c` | - | Project, LFS, collision channels, claims board, message board, five maps (World Partition, streaming off), game modes by map name |
| C-02 contracts (Core) | Lead | DONE | `1cf7fe1` + additions | `TriadDrift.Core` (8) | Types, interfaces, event bus, health, registry, statics, feel, presentation assets, test harness, dev HUD, placeholder tint |
| C-03 #1 switching is a cut | T3 | in review | `d8f7dbb` | `TriadDrift.Squad.Switch.CutPayload` | Review: acceptable. Open: play `SwitchIn` / `SwitchOut` cues, camera of AI-flown ship (T1 API merged) |
| C-03 #2 director-cam money shots | T3 | in review | `d8f7dbb` | `TriadDrift.Director` (11) | Review blocker closed: all producers go through the director and the bus broadcast is private to it. Open: multi-kill rules, per-kind durations, real-actor tests |
| C-03 #3 scenes and takes | T5 | in review | `d8f7dbb` | `TriadDrift.Flow.ScenesAndTakes` | Second review running |
| ENG-01 flight model | T1 | in review | `d8f7dbb` | `TriadDrift.Flight` (22) | All 15 first-review findings reported fixed; second review running; not hand-flown |
| ENG-02 input map | T1 | in review | `d8f7dbb` | `TriadDrift.Flight` | Power keys routed with press / release and released on switch; real key-injection tests; second review running |
| ENG-03 health / death / respawn | Lead + T3 | in review | `d8f7dbb` | `TriadDrift.Core.Health`, `TriadDrift.Squad` | Death tumble, parked downed ships and full flight / weapon / power reset on respawn and retake merged (`26670f2`). Open: switch-in invulnerability rule |
| ENG-04 three ships | T3 | in review | `d8f7dbb` | `TriadDrift.Squad` (48) | Powers installed per ship from the spec. Open: per-ship art by path, switch cues |
| ENG-05 ship weapons | T2 | in review | `d8f7dbb` | `TriadDrift.Combat` (35) | All first-review findings and gap tests merged (`461d296`); second review queued |
| ENG-06 targeting | T2 | in review | `d8f7dbb` | `TriadDrift.Combat` | Lead under drift fixed with real-projectile tests; HUD data on `ITDHUDData` and forwarded by Squad; second review queued |
| ENG-07 squad + switching | T3 | in review | `d8f7dbb` | `TriadDrift.Squad` | Batch 1 merged: stale aim cleared, real-time cooldown, switch refused during hyperdrive / landing. Batch 2 in progress |
| ENG-08 wingman AI | T3 | in review | `d8f7dbb` | `TriadDrift.Squad` | Batch 1 merged: no stuck form-up after a switch during hyperdrive. Open: wingman collisions, formation under roll, gate follow |
| ENG-09 Rift portal blaster | T2 | in review | `d8f7dbb` | `TriadDrift.Powers` (17) | Review: not done. Portal placement ignores dynamic geometry (can teleport into the Dominion hull); shots fired right after an exit are sent back at 30 fps; no art path, invisible in Shipping. T2 fixing |
| ENG-10 Lancer ram | T2 | in review | `d8f7dbb` | `TriadDrift.Powers` (17) | Review: not done. Does not pass through real fighters, never stops on world geometry, 3x speed not reached, dies to collisions right after the dash. T2 fixing |
| ENG-11 Bastion swarm + dome | T2 | in review | `d8f7dbb` | `TriadDrift.Powers` (17) | Review: mechanics right. Open: no art path / invisible in Shipping, state survives a retake, dome cannot keep up with Bastion. T2 fixing |
| ENG-12 unlockable abilities | T2 | in review | `d8f7dbb` | `TriadDrift.Powers` (17) | Review BLOCKER: Gravity Well drags generators, the reactor and carrier parts. Also no art path, turrets ignore EMP. T2 fixing |
| ENG-13 enemy ships + AI | T4 | in review | `d8f7dbb` | `TriadDrift.Enemies` (27) | First-review blocker, majors and minors all reported fixed with real-path tests; second review queued |
| ENG-14 wave director | T4 | in review | `d8f7dbb` | `TriadDrift.Waves` (7) | Leash, spawn-at-announce, no-spawn-ahead and minors merged; second review queued |
| ENG-15 hyperdrive | T1 | in review | `d8f7dbb` | `TriadDrift.Travel` (12) | Ends cleanly on pilot loss, AI cannot spool; first review running |
| ENG-16 portal gates | T4 | in review | `d8f7dbb` | `TriadDrift.World` (17 with ENG-17, 18) | Review: acceptable once wingmen stop bouncing back through gates, gates stop reflecting enemy shots and can take FX. T4 fixing |
| ENG-17 galaxy layout | T4 | in review | `d8f7dbb` | `TriadDrift.World` | Uses T1's play bounds, gravity wells and atmosphere entry. Review: acceptable as gameplay layout; needs an art path, editor preview and gate keep-out. T4 fixing |
| ENG-18 Dominion assault + finale | T4 | in review | `d8f7dbb` | `TriadDrift.World.Dominion` | Review: acceptable after the reactor mesh is applied and victory can no longer be lost. T4 fixing |
| ENG-19 game instance / saves / travel | T5 | in review | `d8f7dbb` | `TriadDrift.Flow` (19) | 12 first-review findings reported fixed; menu and transition game modes added; second review running |
| ENG-20 atmosphere flight + landing | T1 | in progress | `d8f7dbb` (first pass) | `TriadDrift.Travel` | End-to-end level change, landing hand-off and take-off with T5 in progress; first review running |
| ENG-21 first-person character | T5 | in review | `d8f7dbb` | `TriadDrift.Ground` (31 with 22, 23, 24) | First review running; not hand-played |
| ENG-22 energy blade | T5 | in review | `d8f7dbb` | `TriadDrift.Ground` | First review running |
| ENG-23 crew enemies | T5 | in review | `d8f7dbb` | `TriadDrift.Ground.Crew` (12) | Grunt, trooper, heavy, drone, Warden with real AI / projectile / blade tests. Open: trooper cover; first review running |
| ENG-24 objective framework | T5 | in review | `d8f7dbb` | `TriadDrift.Ground` | First review running |
| ENG-25 Verdance gameplay | T5 | in progress | | `TriadDrift.Ground` | Objective ids fixed: `Obj_WreckCore`, `Obj_Relay` |
| ENG-26 Cinder gameplay | T5 | in progress | | `TriadDrift.Ground` | Objective ids fixed: `Obj_Outpost`, `Obj_Jammer` |
| ENG-27 balance + difficulty | Lead + T4 | todo | | | Headless balance run and difficulty presets queued for T4 after the World fixes |
| ENG-28 integration / perf / packaging | Lead + T4 | in progress | main | all (362) + `TDPerf` | Shipping package 750 MB (under 1 GB); Development package boots; stress test: 81 spacecraft + 120 projectiles at 4.2-6.4 ms world tick, live wave 5 at 2.6-4.2 ms per frame. Open: whole journey in a packaged build, HLOD warnings on the maps |
| ENG-29 docking (stretch) | - | todo | | | Only after core is solid |
| ENG-30 interior + core fight (stretch) | - | todo | | | |
| ENG-31 escape (stretch) | - | todo | | | |

## Open review findings (owner fixes, Lead re-verifies)
- Powers (T2): Gravity Well moves fixed parts (BLOCKER); no art path and no placeholders in Shipping (BLOCKER); portal clearance vs dynamic geometry; projectile previous-position inference; ram vs real fighters, world geometry, speed and aftermath; state surviving retakes; dome follow; turrets vs EMP.
- World (T4): reactor mesh never applied; victory travel not retried and squad unprotected during breakup; wingmen bounce back through gates; no art path or editor preview; gates reflect enemy shots; asteroid keep-out and collision shape; Dominion hull does not block enemy shots.
- Squad / Director (T3) batch 2: per-ship art by path and switch cues; switch-in invulnerability rule (0.35 s manual, 2 s auto / respawn, 6 s re-arm); wingman collisions and formation under roll; multi-kill rules; minors and test gaps.
- Flight / Input, Flow (second pass) and Travel, Ground (first pass): reviews running.
- Combat and Enemies / Waves second pass: queued.
- Known gaps reported by owners: trooper cover behaviour; real key input for the first-person character in headless tests; ENG-20 end-to-end run; nothing hand-played.

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
- `8cd1561` T1 `0ca7b22` (squad-review items), T2 `461d296` (16 combat review fixes), T3 `26670f2` (squad review batch 1), T4 `a1a52b9` (review fixes, Travel hand-off). Core: modular ship parts, projectile mesh offset, director-only highlight broadcast. 242/242.
- `d8f7dbb` T5 `0136518` ENG-23 crew enemies; Core slots requested by Nick's side on the message board (front-end widget classes, crew anim / stagger / death, blade mesh). Build clean, 254/254 headless (48 s).
- `adb1912` Unreal 5.9 agent toolsets enabled (21 engine plugins, Editor only) and MCP play-test tools added; a live game was driven through the engine's MCP server on a private port (viewport capture, frame timings, console). T1 `1588fdb` (modular ship parts, flight cues), T2 `fdc205e` (Powers review batch 1), T3 `da65cb8` (Squad batch 2, real-actor director tests), T4 World review fixes. Build clean, 276/276 headless (55 s): Combat 35, Core 8, Director 17, Enemies 27, Flight 24, Flow 19, Ground 31, Powers 19, Squad 53, Travel 13, Waves 7, World 23. Shipping package: 750 MB including a 239 MB debug symbol file, about 510 MB without it.
- First live look through MCP found: an on-screen engine warning about two competing directional lights (space game mode spawns a sun and a fill light), and the input toolset could not find its player target in a `-game` session (to investigate).
- `f31a1f1` (Nov 12 15:14 EST): T2 `3f1a8b0` HUD host (`UI/`, switch default off) and `96e7c25` visible placeholder projectiles; T3 `6520438` + `c77d246` (earned-ability HUD data, forward wedge, gate follow, director shots keep wingmen out of frame); T4 `b82a2e1` stress test `TDPerf.Stress.Wave5PlusDominion` (not in the default suite); T1 `99d5d14` (virtual stick recentres, motion blur off); T5 `74aa64d` + `43dc279` (tick cycle removed, held blade poses, crew look); audio skill `4096f47`; `Tools/td-play-mcp.ps1 -Audio`. Build clean, 362/362 headless.
