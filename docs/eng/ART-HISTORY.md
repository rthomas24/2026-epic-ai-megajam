# Art history: how to go back to an older look

Ryan, Nov 13: "make sure you keep copies of old things you get rid of for art in case we want to go back at any
point." This file is the index. Nothing listed here has been deleted: every version is a commit on `main`, and the
older looks that the game can still show are listed with the switch that brings them back.

## The rule (every agent and builder on engineering's side)

1. **Never delete or overwrite an art asset that is not in git yet.** Commit the old version first, then replace it.
   The Lead does the commit; a builder that is about to replace something says so and waits for it.
2. **Keep the source.** Every asset keeps its `.blend`, scripts and review sheets under `ArtSource/Eng*/**`, with one
   review sheet per round (`*_rN_*.png`), so any round can be rebuilt from its script.
3. **A replaced look stays reachable** in one of three ways, in this order of preference:
   - a setting or console variable that switches it back on (the code-built placeholder shapes, the HUDs);
   - a tag on `main` (`art/<name>`), for a whole-game snapshot;
   - the commit in the table below, restored with `git checkout <commit> -- <path>` on a branch.
4. **Add a row here** whenever a look is replaced: what, the commit that holds the old version, how to see it again.
5. A large binary that is replaced many times (a `.blend`, a texture set) keeps every committed version in Git LFS;
   nothing is pruned before the jam ends.

## Snapshots (tags on `main`)

| Tag | Commit | What the game looked like |
|---|---|---|
| `art/before-overnight-nov13` | `ce54338` | Before the night of Nov 12-13: placeholder ships and Dominion, flat planets, no terrain kit. |
| `art/morning-nov13` | `fd0cb7a` | What Ryan saw on the morning tour: engineering ships, fleet, Dominion r9, planet kit round 2, ground anti-tiling, magma, old (bulky) HUD, flat-glow effects, flat planet ground. |

Restore a whole snapshot in a separate checkout: `git worktree add ../old art/morning-nov13`.

## Looks that can be switched back on in the current game

| Look | How |
|---|---|
| Code-built placeholder ships, enemies, Dominion, crew | Remove or rename the matching `DA_*_Visuals` under `Content/TriadDrift/Gameplay/Characters/**`; the code falls back to its shapes. |
| Nick's HUD (`WBP_SpaceHUD`), the developer HUD | The HUD choice in `TDUIHostSettings` (`Config/DefaultGame.ini`); the developer HUD also with `td.DebugHUD 1`. Both stay selectable after the ENG-36 redesign. |
| Flat-glow placeholder effects (trails, flashes, explosions) | They remain the fallback whenever a cue has no Niagara effect (ENG-37): clear the cue's `VFX` field. |
| Space dressing off | `TD.World.Dressing 0`. |
| Menu backdrop off | `td.MenuBackdrop 0`. |
| Intro off | `td.Intro 0`. |

## Versions by asset (each row's commit contains that version; the row above it is the newer one)

| Asset | Version | Commit | Path |
|---|---|---|---|
| Null Armada fleet | engine glow pass (current) | `7b6305c` | `Content/TriadDrift/Gameplay/Characters/NullFleet`, `ArtSource/EngShips/NullFleet` |
| | first version | `b603476` | same |
| Squad ships (Rift, Lancer, Bastion) | r7 colour blocking (current) | `3270de4` | `Content/TriadDrift/Gameplay/Characters/Squad`, `ArtSource/EngShips/Squad` |
| | r6, mostly white hulls | `b96f8bd` (+ `86256b2` socket fix) | same |
| Dominion exterior | glow strengths for the running game (current) | `3a6c3ff` (+ Trim `bf5394b`) | `Content/TriadDrift/Gameplay/Props/Dominion`, `ArtSource/EngShips/Dominion` |
| | r9 glow pass | `688c979` | same |
| | r8 silhouette pass | `31e6121` | same |
| | first kit (slab-like) | `f98124f` | same |
| Dominion generator, reactor | first version (current) | `c9f541a` | same |
| Dominion turrets | Null Armada turret mesh (current) | `bf5394b` | `Content/TriadDrift/Gameplay/Characters/Dominion` |
| Planet set-piece kit | round 2 + altar + prototype console (current) | `db8d405` | `Content/TriadDrift/Gameplay/Props/Planet`, `ArtSource/EngProps/PlanetKit` |
| | round 1 (26 pieces) | `a94988c` | same |
| Cinder magma surface | first version (current) | `d3a4643` | same |
| Planets from space | self-lit with night side (current) | `219057b` | `Content/TriadDrift/Gameplay/Props/Planets`, `ArtSource/EngProps/Planets` |
| Planet terrain materials | `M_TD_Terrain` r1: slope rock, peaks, lava valleys, procedural textures (current) | `a101603` | `Content/TriadDrift/Gameplay/Props/Planets`, `ArtSource/EngProps/Planets` (`review/terrain_r1_*.jpg`); before it the terrain wore `MI_Ground_*` |
| Planet terrain shape | M2: 2.5 km hero peaks, regions, skyline (current on `lead/int`) | `e77e701` | `Source/unrealGameJam/TriadDrift/World/TDTerrainField.*` (code; `td.PlanetTerrain 0` brings back the flat slab) |
| | M1: first procedural mountains | `71d11c5` | same |
| Niagara effects (ENG-37) | tier 1 round 2, unwired | `02bfcbc` | `Content/TriadDrift/Gameplay/VFX`, `ArtSource/EngVFX` (`review/tier1_r1_*`, `tier1_r2_*`, `rounds/r2/`) |
| Fallback sound set (ENG-35) | first version (current) | `31f2120` (+ `7f2c175`) | `Content/TriadDrift/Gameplay/Audio`, `ArtSource/EngAudio` (recipes rebuild every sound) |
| HUD | ENG-36 flight HUD P1 (in progress, not default) | T2 `9754f24` | spec `docs/eng/hud-redesign.md`; Nick's `WBP_SpaceHUD` and the developer HUD stay selectable |
| AI concepts and terrain texture sets (Grok Bot) | r1 / r2 per folder, never overwritten | `20060b1` | `ArtSource/EngAI/**` |
| Planet ground | anti-tiling (current) | `44d790a` | same |
| Crew (trooper, grunt, heavy) | centimetre rig on `SKEL_NullCrew` (current) | `5dc659f` | `Content/TriadDrift/Gameplay/Characters/{NullTrooper,NullGrunt,NullHeavy,Crew}` |
| | mannequin-skeleton versions | `17e0437` and earlier | same |

Earlier versions of anything: `git log --oneline -- <path>`. Local review renders and capture sets that are not in
git (`Saved/TDShots/**`, `Saved/*/` builder scratch) are kept on Ryan's machine and are not cleaned up.
