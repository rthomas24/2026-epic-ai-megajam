# Engineering status ledger (Lead-owned)

The Lead updates this after every review and merge. An item is DONE only when it is merged to
`main`, the build is clean, its automation tests pass headless on `main`, and review findings are closed.

Status values: `todo` · `in progress` · `in review` (merged, tests pass, review open) · `DONE` · `BUG`

| Item | Owner | Status | Branch / merge | Tests (filter) | Notes |
|---|---|---|---|---|---|
| C-01 project bootstrap | Lead | in progress | main | — | Project, LFS, collision channels done. Maps + `Content/TriadDrift` tree pending |
| C-02 contracts (Core) | Lead | DONE | main `1cf7fe1` | `TriadDrift.Core` (8) | Types, interfaces, event bus, health, registry, statics, feel, presentation assets, test harness |
| C-03 Director's Cut hooks | T3 (#1, #2), T5 (#3) | in progress | main (#3) | `TriadDrift.Flow.ScenesAndTakes` | #3 scenes/takes merged and passing; #1 and #2 assigned to T3 |
| ENG-01 flight model | T1 | in review | main `ef345ca` | `TriadDrift.Flight` (11) | Tests pass; independent review running; not yet hand-flown |
| ENG-02 input map | T1 | in review | main `ef345ca` | `TriadDrift.Flight` | Bindings tested for existence; simulated key routing untested |
| ENG-03 health / death / respawn | Lead (component) + T3 (respawn) | in progress | main / eng/t3 | `TriadDrift.Core.Health`, `TriadDrift.Squad` | Component done; squad respawn rules on T3's branch |
| ENG-04 three ships | T3 | BUG | eng/t3 `0e8d85f` | `TriadDrift.Squad` | Not merged: `HoldFormation` fails on new main (Bastion lags); T3 fixing |
| ENG-05 ship weapons | T2 | in review | main `6bd14ec` | `TriadDrift.Combat` (19) | Tests pass; independent review running |
| ENG-06 targeting | T2 | in review | main `6bd14ec` | `TriadDrift.Combat` | Tests pass; independent review running |
| ENG-07 squad + switching | T3 | BUG | eng/t3 `0e8d85f` | `TriadDrift.Squad` (17) | Same branch as ENG-04 |
| ENG-08 wingman AI | T3 | BUG | eng/t3 `0e8d85f` | `TriadDrift.Squad` | Same branch as ENG-04 |
| ENG-09 Rift portal blaster | T2 | in progress | | `TriadDrift.Powers` | Design plan requested first |
| ENG-10 Lancer ram | T2 | todo | | `TriadDrift.Powers` | |
| ENG-11 Bastion swarm + dome | T2 | todo | | `TriadDrift.Powers` | |
| ENG-12 unlockable abilities | T2 | todo | | `TriadDrift.Powers` | |
| ENG-13 enemy ships + AI | T4 | in review | main `d484243` | `TriadDrift.Enemies` (9) | Tests pass; independent review running; AI feel not hand-tested |
| ENG-14 wave director | T4 | in review | main `d484243` | `TriadDrift.Waves` (7) | Tests pass; independent review running |
| ENG-15 hyperdrive | T1 | in progress | | `TriadDrift.Travel` | |
| ENG-16 portal gates | T4 | todo | | `TriadDrift.World` | Shares T2's transit helper |
| ENG-17 galaxy layout | T4 | todo | | `TriadDrift.World` | |
| ENG-18 Dominion assault + finale | T4 | in progress | | `TriadDrift.World` | |
| ENG-19 game instance / saves / travel | T5 | in review | main `a2fdf3e` | `TriadDrift.Flow` (9) | Tests pass; independent review running |
| ENG-20 atmosphere flight + landing | T1 | todo | | `TriadDrift.Travel` | After ENG-15 |
| ENG-21 first-person character | T5 | in progress | | `TriadDrift.Ground` | |
| ENG-22 energy blade | T5 | todo | | `TriadDrift.Ground` | |
| ENG-23 crew enemies | T5 | todo | | `TriadDrift.Ground` | |
| ENG-24 objective framework | T5 | todo | | `TriadDrift.Ground` | |
| ENG-25 Verdance gameplay | T5 | todo | | `TriadDrift.Ground` | |
| ENG-26 Cinder gameplay | T5 | todo | | `TriadDrift.Ground` | |
| ENG-27 balance + difficulty | Lead | todo | | | |
| ENG-28 integration / perf / packaging | Lead | in progress | main | all | Build, test, screenshot and packaging tools work; baseline package built |
| ENG-29 docking (stretch) | — | todo | | | |
| ENG-30 interior + core fight (stretch) | — | todo | | | |
| ENG-31 escape (stretch) | — | todo | | | |

## Integration log
- `1cf7fe1` Core foundation on main. Build clean, `TriadDrift.Core` 8/8 passing headless (14 s).
- `dcb9469` T1 flight/input, T2 combat, T5 flow merged. 17/17.
- `2fa9a05` T1 + T2 + T4 test commits merged. Full suite 63/63 headless (20 s). Baseline packaging (Development) succeeded in 4.5 min, 1.1 GB with template content.
- T3 `0e8d85f` held back: `Squad.Wingmen.HoldFormation` fails against T1 `ef345ca` (Bastion lags 11,700 cm). Fix in progress.
- `642f232` Core: extra enemy cue slots, `UTDDominionVisuals`; `td-shot.ps1` visual capture working.
