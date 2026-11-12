# Engineering status ledger (Lead-owned)

The Lead updates this after every review and merge. An item is DONE only when it is merged to
`main`, the build is clean, its automation tests pass headless on `main`, and review findings are closed.
The short cross-team view (in progress / coming next / done) is the board in `/CLAIMS.md`.

Status values: `todo` / `in progress` / `in review` (merged, tests pass, review open) / `DONE` / `BUG`

Mapping to the stages in `/AAA.md`: `in progress` on a branch with passing tests is *prototype complete*;
`in review` is *technically verified* and *integrated*; `DONE` additionally needs every review BLOCKER and
MAJOR closed and the item *gameplay validated* (played in a running game, noted in `DEVLOG.md`).
*Presentation approved* is tracked on Nick's side in `docs/visual/STATUS.md`. **No gameplay item (`ENG-xx`, C-03) is `DONE` yet.**

"Played" below means an agent flying the running game through Unreal's MCP server with injected input, unless it says otherwise. No human has played yet.

| Item | Owner | Status | Tests (filter) | Reviews | Played (agent over MCP unless said) | Open |
|---|---|---|---|---|---|---|
| C-01 project bootstrap | Lead | DONE | - | - | boots from `L_Galaxy` in editor and packaged Development build | HLOD layer warnings on the five maps |
| C-02 contracts (Core) | Lead | DONE | `TriadDrift.Core` (12) | - | - | Additions since: presentation loader, audio live state, `TDTags`, `GetObjectiveText` |
| C-03 #1 switching is a cut | T3 | in review | `TriadDrift.Squad.Switch`, `TriadDrift.Director.CutPresentation` | 1 review, fixes merged | Tab switch, name card, flash: playtests 2 and 3 | Fallback name card vs real HUD fixed (`7c9c17d`); replay with the HUD on |
| C-03 #2 director-cam money shots | T3 | in review | `TriadDrift.Director` | 1 review, fixes merged | Ram and generator shots captured; ram shot not yet seen mid-shot after the wingman fix | One live capture per highlight kind |
| C-03 #3 scenes and takes | T5 + T3 | in review | `TriadDrift.Flow.ScenesAndTakes` | 2 reviews: acceptable | Slate and "CUT!" seen live | Slate position under the real HUD |
| ENG-01 flight model | T1 | in review | `TriadDrift.Flight` | 2 reviews; second: drift-burst pumping and pinned-contact damage | Fly, boost, turn settle, drift: playtests 2 and 3, T1's own | T1's full playtest; minors |
| ENG-02 input map | T1 | in review | `TriadDrift.Flight`, `TriadDrift.Core.Harness` | 2 reviews: acceptable | Real injected keys and mouse | - |
| ENG-03 health / death / respawn | Lead + T3 | in review | `TriadDrift.Core.Health`, `TriadDrift.Squad` | 1 review, fixes merged | Auto-switch on death seen live | Human play |
| ENG-04 three ships | T3 | in review | `TriadDrift.Squad` | 1 review, fixes merged | All three flown | Human play |
| ENG-05 ship weapons | T2 | in review | `TriadDrift.Combat` | 2 reviews; second (Nov 12 15:26): 1 MAJOR (fixed in `e37cd81`), 5 minors open | Visible streaks, muzzle and impact flashes: playtest 3 | Bastion flash and shell presence; frame-rate head start; salvo after death |
| ENG-06 targeting | T2 + T3 | in review | `TriadDrift.Combat`, `TriadDrift.Squad` | 2 reviews; second: Bastion locks never expire (MAJOR, open); objective markers (MAJOR, `d78e783` merged) | Brackets, lock, missile warning, arrows through the real HUD | Lock expiry; markers when 40 ships |
| ENG-07 squad + switching | T3 | in review | `TriadDrift.Squad` | 1 review, fixes merged | Switch stays instant; stick resets on switch (`eeaeaa1`) | Second review |
| ENG-08 wingman AI | T3 | in review | `TriadDrift.Squad` | 1 review, fixes merged | Wingmen in frame 67-80% in small samples | 70% over 20 cycles in waves 1, 3, 5; second review |
| ENG-09 Rift portal blaster | T2 | in review | `TriadDrift.Powers` | 1 review, fixes merged | Portal opened live | Placeholder is a solid disc; second review |
| ENG-10 Lancer ram | T2 | in review | `TriadDrift.Powers` | 1 review, fixes merged | Ram at 335 m/s, playtest 2 | Second review |
| ENG-11 Bastion swarm + dome | T2 | in review | `TriadDrift.Powers` | 1 review, fixes merged | Not fired live by the Lead yet | Lock rules (ENG-06); second review |
| ENG-12 unlockable abilities | T2 | in review | `TriadDrift.Powers`, `TriadDrift.UI.HUDHost.N004AbilitySequence` | 1 review, fixes merged | LOCKED state through the real HUD | Each ability used live; second review |
| ENG-13 enemy ships + AI | T4 | in review | `TriadDrift.Enemies` | 2 reviews; second (Nov 12 15:31): 4 MAJOR open (fighters do not fire at a facing player, interceptor point-blank launch, gunship faces the lead point, followers escape the Gravity Well) | Waves 1-5 run live; ships are specks | The four MAJORs; placeholder visibility and scale |
| ENG-14 wave director | T4 | in review | `TriadDrift.Waves` | 2 reviews; second: acceptable with minors | Waves advance, breather, wave 5 with 29 enemies | Soft-lock when the last enemy dies during the "CUT!" delay; warp-in telegraph; tips; difficulty not selectable |
| ENG-15 hyperdrive | T1 | in review | `TriadDrift.Travel` | 1 review, fixes merged | Spool, jump, drop-out (T1) | Second review |
| ENG-16 portal gates | T4 | in review | `TriadDrift.World.Gate` | 1 review, fixes merged | Not flown through by the Lead yet | Enemies can leave through planet gates |
| ENG-17 galaxy layout | T4 | in review | `TriadDrift.World.Layout` | 1 review, fixes merged | Belt, planet, gates seen | Stars black from half the sky (T5) |
| ENG-18 Dominion assault + finale | T4 | in review | `TriadDrift.World.Dominion` | 1 review, fixes merged | Stage starts facing the Dominion; turrets bracketed | Whole assault to victory not played; HUD wave block during the assault |
| ENG-19 game instance / saves / travel | T5 | in review | `TriadDrift.Flow` | 2 reviews: acceptable | Real level change to Verdance and back (T1, T5) | Difficulty in the save |
| ENG-20 atmosphere flight + landing | T1 + T5 | in review | `TriadDrift.Travel`, `TriadDrift.Flow` | 1 review, fixes merged | Arrival, braking, landing on the pad, on foot (T1) | Approach assist on held F; take-off and return played end to end |
| ENG-21 first-person character | T5 | in review | `TriadDrift.Ground` | 1 review, fixes merged | Walk, dash, HUD | Human play |
| ENG-22 energy blade | T5 | in review | `TriadDrift.Ground.Blade` | 1 review, fixes merged | Held blade with per-move poses (T5) | Deflect timing judged by a person |
| ENG-23 crew enemies | T5 | in review | `TriadDrift.Ground.Crew` | 1 review, fixes merged | Each type captured (T5) | Fights played end to end |
| ENG-24 objective framework | T5 | in review | `TriadDrift.Ground` | 1 review, fixes merged | Objective line for space scenes (`7ca95fe`) | Seen through the real HUD |
| ENG-25 Verdance gameplay | T5 | in progress | `TriadDrift.Ground` | - | Arrival and on-foot hand-over only | Both objectives finishable, rewards, return |
| ENG-26 Cinder gameplay | T5 | in progress | `TriadDrift.Ground` | - | - | Both objectives finishable, rewards, Dominion reveal |
| ENG-27 balance + difficulty | T4 + Lead | in review | `TriadDrift.Waves.Balance` | covered by the ENG-14 review | Headless balance run only: waves 1-5 in 7.1-7.9 min on Standard, enemy on screen 59-96% of the time | Human play; difficulty choice |
| ENG-28 integration / perf / packaging | Lead + T4 | in progress | all (see the log below) + `TDPerf` | - | Packaged Development build boots and runs; Shipping 750 MB | Whole journey in a packaged build; `WBP_SpaceHUD` in the cook; spikes rechecked |
| HUD host for Nick's plugin | T2 | in review | `TriadDrift.UI.HUDHost` (4) | Nick's side validating (N-007) | Played with the real HUD on, playtest 3 | Default still off; objective text and name card replay |
| Audio support (skill, live state, engine loop parameters) | Lead + T1 | in review | `TriadDrift.Core.AudioState` (3), `TriadDrift.Flight.Visuals.EngineLoopAudioParameters` | - | Nothing heard by a person | Used by no audio yet |
| ENG-29 docking (stretch) | - | todo | | | | Only after core is solid |
| ENG-30 interior + core fight (stretch) | - | todo | | | | |
| ENG-31 escape (stretch) | - | todo | | | | |

## Open review findings (owner fixes, Lead re-verifies)
Updated 2026-11-12 15:36 EST.
- Combat (T2), second review: Bastion locks never expire (MAJOR); frame-rate dependent head start on the press frame; salvo continues after death; hit confirmed on rocks; markers cut off at 24; portal plane clipped late at 30 fps; lifetime ignores inherited speed; test gaps.
- Enemies (T4), second review: fighters do not fire at a facing player; interceptor launches at point-blank on a head-on pass; gunship faces the lead point; pack followers escape the Gravity Well (all MAJOR). Wave soft-lock when the last enemy dies during the "CUT!" delay; enemies leave through planet gates; warp-in telegraph shows nothing; signature-power tips never fired; difficulty not selectable or saved; spawn points ignore obstacles; `docs/eng/enemies.md` asset names stale.
- Playtest 3 (Lead): stars black from half the sky (T5); enemy ships too small and dark (T4); Bastion muzzle flash and shell presence, Rift portal placeholder (T2); slate behind the objective block (T3); HUD wave block during the Dominion assault (T4 + T5).
- Second reviews still owed: Powers, World, Squad / Director, Travel, Ground.
- Nothing has been played by a human.

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
- `f2db3ce` (Nov 12 15:33 EST): Core `UTDAudioStateSubsystem` (live state for music and mixes, 3 tests); T2 `68bfb04` (streak thickness by camera distance); T1 `89022c2` (atmosphere ground floor); T5 `d70a4c8` + `ceb102a` (Dominion stages face the objective, planet fill light); T4 `18e4447` (strict gate assertion). Build clean, 369/369 headless. One earlier run of this merge crashed in `Powers.Presentation.EveryMomentRequestsItsCue` (dome FX freed under garbage collection): with T2.
- `850a557` (Nov 12 15:38 EST): T1 `eeaeaa1` + `f2653f8`, T2 `e37cd81` (+ `1421acb` test rooting), T3 `7c9c17d`, `35eed19`, `d78e783`, `88f5b3a`, T4 `1a372d2`, T5 `7ca95fe` + `a872bce`; Core `TDTags`, `GetObjectiveText`, dev HUD landing prompt. Build clean, 378/378 headless. Ledger table rewritten with review and play columns.
