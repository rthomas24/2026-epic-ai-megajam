# Engineering status ledger (Lead-owned)

The Lead updates this after every review and merge. An item is ✅ only when it is merged to `main`,
the build is clean, and its automation tests pass headless on `main`.

Legend: ⬜ not started · 🟨 in progress · 🔎 in review · ✅ done and verified · 🐞 has bugs

| Item | Owner | Status | Branch / merge | Tests (filter) | Notes |
|---|---|---|---|---|---|
| C-01 project bootstrap | Lead | 🟨 | main | — | Project, LFS, collision channels done. Maps + `Content/TriadDrift` tree pending (needs editor or Python commandlet) |
| C-02 contracts (Core) | Lead | ✅ | main `1cf7fe1` | `TriadDrift.Core` (8) | Types, interfaces, event bus, health, registry, statics, feel, presentation assets, test harness |
| C-03 Director's Cut hooks | T3 (#1, #2), T5 (#3) | 🟨 | | | #3 takes implemented by T5 (unverified) |
| ENG-01 flight model | T1 | 🟨 | eng/t1 | `TriadDrift.Flight` | API headers `d34187a` |
| ENG-02 input map | T1 | 🟨 | eng/t1 | `TriadDrift.Input` | |
| ENG-03 health / death / respawn | Lead (component) + T3 (respawn) | 🟨 | main / eng/t3 | `TriadDrift.Core.Health`, `TriadDrift.Squad` | Component done; squad respawn rules with T3 |
| ENG-04 three ships | T3 | 🟨 | eng/t3 | `TriadDrift.Squad` | API headers `0054c24` |
| ENG-05 ship weapons | T2 | 🟨 | eng/t2 | `TriadDrift.Combat` | |
| ENG-06 targeting | T2 | 🟨 | eng/t2 | `TriadDrift.Combat` | |
| ENG-07 squad + switching | T3 | 🟨 | eng/t3 | `TriadDrift.Squad` | |
| ENG-08 wingman AI | T3 | 🟨 | eng/t3 | `TriadDrift.Squad` | |
| ENG-09 Rift portal blaster | T2 | ⬜ | | `TriadDrift.Powers` | |
| ENG-10 Lancer ram | T2 | ⬜ | | `TriadDrift.Powers` | |
| ENG-11 Bastion swarm + dome | T2 | ⬜ | | `TriadDrift.Powers` | |
| ENG-12 unlockable abilities | T2 | ⬜ | | `TriadDrift.Powers` | |
| ENG-13 enemy ships + AI | T4 | 🟨 | eng/t4 | `TriadDrift.Enemies` | API headers `5dbc3b3` |
| ENG-14 wave director | T4 | 🟨 | eng/t4 | `TriadDrift.Waves` | |
| ENG-15 hyperdrive | T1 | ⬜ | | `TriadDrift.Travel` | |
| ENG-16 portal gates | T4 | ⬜ | | `TriadDrift.World` | |
| ENG-17 galaxy layout | T4 | ⬜ | | `TriadDrift.World` | |
| ENG-18 Dominion assault + finale | T4 | ⬜ | | `TriadDrift.World` | |
| ENG-19 game instance / saves / travel | T5 | 🟨 | eng/t5 `e3d2915` | `TriadDrift.Flow` | Code complete, not yet built |
| ENG-20 atmosphere flight + landing | T1 | ⬜ | | `TriadDrift.Travel` | |
| ENG-21 first-person character | T5 | ⬜ | | `TriadDrift.Ground` | |
| ENG-22 energy blade | T5 | ⬜ | | `TriadDrift.Ground` | |
| ENG-23 crew enemies | T5 | ⬜ | | `TriadDrift.Ground` | |
| ENG-24 objective framework | T5 | ⬜ | | `TriadDrift.Ground` | |
| ENG-25 Verdance gameplay | T5 | ⬜ | | `TriadDrift.Ground` | |
| ENG-26 Cinder gameplay | T5 | ⬜ | | `TriadDrift.Ground` | |
| ENG-27 balance + difficulty | Lead | ⬜ | | | |
| ENG-28 integration / perf / packaging | Lead | 🟨 | main | all | Build + test tools done; packaging pending |
| ENG-29 docking (stretch) | — | ⬜ | | | |
| ENG-30 interior + core fight (stretch) | — | ⬜ | | | |
| ENG-31 escape (stretch) | — | ⬜ | | | |

## Integration log
- `1cf7fe1` Core foundation on main. Build clean, `TriadDrift.Core` 8/8 passing headless (14 s).
