# Triad Drift active claims

Claim handling follows AGENTS.md §6.5 and §6.9. No binary work is authorized by
an unpublished claim. The Lead preserves all other active claims when updating this file.

The root file on shared `main` is authoritative. Before assigning work or editing
any file, consult the latest remote record through Lead-coordinated preflight.
Lead publishes each new or changed exact-path claim to `main` before edits begin.
A local or branch-only claim is not a replacement for this record; preserve local
changes and reconcile them through Lead before any shared integration. An absent
claim never grants permission to edit Ryan-owned or unassigned paths.
Release ownership only after safe commit/push, no active editing, and the existing
AGENTS.md lifecycle conditions; branch publication alone does not release it.

| Who / agent | Files / folders / assets | Since | Branch / worktree | Status |
|---|---|---|---|---|
| Nicholas / Lead | AGENTS.md; CLAIMS.md (coordination infrastructure only) | 2026-11-12 EST | nick/agent-infrastructure / Worktrees/agent-infrastructure | Released historical infrastructure claim: merged in 9e8fd4d; no current AGENTS.md editing claim |

## Current Nick-owned reservations — November 12, 2026

Lead-maintained current ownership; the historical Lead claim above is marked released.
All Ryan claims below are preserved unchanged. Worktree paths are on Nick's PC
under `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/`.
These narrow active lanes supersede the unpublished broad placeholders in PR #1,
not any teammate's active claim. Shared Source/Config/maps/interfaces are excluded.
See docs/visual/Day1-Integration-Policy-2026-11-12.md for the dated integration exception.

| Owner / chat | Exact writable paths | Branch | Nick worktree | Ownership status |
|---|---|---|---|---|
| Lead Coordinator | docs/visual/**; CREDITS.md; DEVLOG.md; coordinated CLAIMS.md updates | nick/visual-assets; nick/day1-integration | visual-assets; day1-integration | Active coordination only; no asset implementation; serial claim publication/integration |
| Environment & VFX Agent | ArtSource/Environment/SpaceFoundation/UnrealNative/**; Content/TriadDrift/Environment/SpaceFoundation/** | nick/environment-vfx | environment-vfx | Active first native environment milestone; original source outside UnrealNative read-only; future VFX/Audio not assigned |
| UI/UX & HUD Agent | Plugins/TriadDriftHUD/**; ArtSource/UI/TriadHUD/Unreal/**; Content/TriadDrift/UI/TriadHUD/** | nick/hud-native | hud-native | Active publication/verification handoff; frozen prototype read-only; plugin/shared activation withheld pending Ryan approval |
| Ships & Weapons Agent | ArtSource/Space/Ships/Lancer/** | nick/ships-lancer | ships-lancer | Frozen review at pushed 43dc320; no active edits; reservation retained, not released; silhouette/assembly QA unresolved, no main integration |

Completed Environment/VFX/HUD/Audio source snapshots are read-only shared references.
Future combat VFX output paths and Audio/music production are not activated or
assigned by these rows. No existing claim is released merely by publishing this table.

## Ryan-side engineering board (Lead: Claude Code)

**Last updated: 2026-11-12 12:46 EST** by Ryan's Lead. Updated on every task start and finish
(AGENTS.md §6.5 start / finish rule). Additive publication: every claim above is preserved unchanged.

All of Ryan's side is C++ / text so far: **no `.uasset`, `.umap` or other binary file is claimed or
edited yet.** Detailed test and review status per item: `docs/eng/STATUS.md`. Code-ownership
contract: `docs/eng/ARCHITECTURE.md`.

### Lanes (who may write where)

Paths are under `Source/unrealGameJam/TriadDrift/` unless they start with `/` (repo root).

| Owner | Exact writable paths | Branch / worktree |
|---|---|---|
| Ryan / Lead (Claude Code) | `Core/**`; `Tests/**`; `/Source/unrealGameJam/unrealGameJam.Build.cs`; `/Config/**`; `/Tools/**`; `/docs/eng/**`; `/.claude/skills/ue-cpp-gameplay/**`; Ryan's section of `/CLAIMS.md` | `lead/int` / `.wt/int`, integrates to `main` |
| Ryan / Teammate 1 | `Flight/**`; `Input/**`; `Travel/**`; `/docs/eng/flight.md` | `eng/t1` / `.wt/t1` |
| Ryan / Teammate 2 | `Combat/**`; `Powers/**`; `/docs/eng/combat.md` | `eng/t2` / `.wt/t2` |
| Ryan / Teammate 3 | `Squad/**`; `Director/**`; `/docs/eng/squad.md`; `/docs/eng/director.md` | `eng/t3` / `.wt/t3` |
| Ryan / Teammate 4 | `Enemies/**`; `Waves/**`; `World/**`; `/docs/eng/enemies.md` | `eng/t4` / `.wt/t4` |
| Ryan / Teammate 5 | `Flow/**`; `Ground/**`; `/docs/eng/flow.md` | `eng/t5` / `.wt/t5` |

Workers commit only on their own branch and never pull, merge or push; the Lead integrates in
`.wt/int`, builds, runs the headless suite, then fast-forwards and pushes `main`.

### Tasks: in progress, coming next, done

States: `queued` (coming next) · `in progress` · `in review` (on `main`, tests pass, review fixes
open) · `done` (on `main`, tests pass, review closed).

| Item | Owner | State | Started (EST) | Finished / on `main` | Shared paths touched |
|---|---|---|---|---|---|
| C-02 Core contracts (types, interfaces, event bus, health, registry, statics, test harness) | Lead | done | Nov 12 | `1cf7fe1` | none outside lane |
| Core: dev HUD (`ATDDebugHUD`), placeholder tint, pooled cues | Lead | done | Nov 12 12:20 | Nov 12 12:45, this push | none |
| Independent code review of Squad + Director | Lead | in progress | Nov 12 12:43 | | none |
| Independent code review of Powers + World (ENG-09 to 12, 16, 17, 18) | Lead | in progress | Nov 12 12:46 | | none |
| Second review pass on Flow fixes | Lead | queued | | | none |
| ENG-28 build / test / screenshot / package tools | Lead | done | Nov 12 | `642f232` | `/Tools/**` |
| ENG-28 Shipping package under 1 GB, MapsToCook | Lead | queued | | | `/Config/DefaultGame.ini` (one line added now: always cook `/Engine/BasicShapes`) |
| C-01 maps (`L_MainMenu`, `L_Galaxy`, `L_Planet_Verdance`, `L_Planet_Cinder`, `L_Transition`) and `Content/TriadDrift` tree | Lead | queued | | | **`Content/TriadDrift/Maps/**` (binary, shared): will be claimed here before creation** |
| ENG-27 balance + difficulty | Lead | queued | | | none |
| ENG-01 flight model, ENG-02 input map | T1 | in review | Nov 12 | `ef345ca`; review fixes in progress (drift burst cap, camera roll, power routing) | none |
| ENG-15 hyperdrive | T1 | in progress | Nov 12 | | none |
| ENG-20 atmosphere flight + landing | T1 | queued | | | none |
| ENG-05 ship weapons, ENG-06 targeting | T2 | in review | Nov 12 | `6bd14ec`; review fixes in progress (lead under drift, pool identity, retake cleanup, HUD data) | one-off Core edit for targeting HUD data: merged `0734b8c`, permission closed |
| ENG-09 Rift portal blaster, ENG-10 Lancer ram, ENG-11 Bastion swarm + dome, ENG-12 unlockable abilities | T2 | in review | Nov 12 | Nov 12 12:46, this push (`6455ee0`); independent review running | none |
| ENG-03 respawn rules, ENG-04 three ships, ENG-07 switching, ENG-08 wingman AI | T3 | in review | Nov 12 | `448f658`; independent review queued | none |
| C-03 #1 switch-as-cut, #2 director-cam money shots | T3 | in review | Nov 12 | Nov 12 12:45, this push (`1309c5b`); review running | none |
| Squad follow-ups: travel guard in director, flight / weapon reset on respawn, target carry-over, targeting HUD forwarders | T3 | in progress | Nov 12 12:43 | | none |
| ENG-13 enemy ships + AI, ENG-14 wave director | T4 | in review | Nov 12 | `d484243`; review fixes in progress (carrier bays unreachable, leash, ramming) | none |
| ENG-18 Dominion assault + reactor finale | T4 | in review | Nov 12 | `571d85f` | none |
| ENG-16 portal gates, ENG-17 galaxy layout | T4 | in review | Nov 12 | Nov 12 12:45, this push (`ea32ff5`); not reviewed yet | none |
| ENG-19 game instance / saves / travel, C-03 #3 scenes and takes | T5 | in review | Nov 12 | `a2fdf3e`; all 12 review fixes merged Nov 12 12:45, this push (`125146f`); second review queued | none |
| ENG-21 first-person character, ENG-22 energy blade, ENG-24 objective framework | T5 | in progress | Nov 12 | | none |
| ENG-23 crew enemies, ENG-25 Verdance gameplay, ENG-26 Cinder gameplay | T5 | queued | | | none |
| ENG-29 docking, ENG-30 interior + core fight, ENG-31 escape (stretch) | unassigned | queued | | | only after core is solid |

### Coming next that touches shared things (heads-up for Nick's side)

- `Content/TriadDrift/Maps/**`: Ryan's Lead will create the five maps (empty, gameplay actors
  spawned from C++). The claim row moves to `in progress` here, on `main`, before the first `.umap`
  exists.
- `/Config/DefaultGame.ini` (`MapsToCook`, packaging) and `/Config/DefaultEngine.ini` (default maps)
  change when the maps land.
- `unrealGameJam.uproject` is unclaimed. Nick's `Plugins/TriadDriftHUD/**` row implies a plugin
  entry there and a module dependency. A plugin cannot include the game module's headers
  (`ITDHUDData`, `UTDEventBus`), so how the HUD reads gameplay data needs to be agreed between
  Ryan and Nick before that plugin is integrated.
