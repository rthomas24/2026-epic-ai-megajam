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
| Lead Coordinator | docs/visual/**; CREDITS.md; DEVLOG.md; coordinated CLAIMS.md updates; Nick-side MESSAGEBOARD.md posts/replies under AGENTS §6.10 | nick/visual-assets; nick/day1-integration | visual-assets; day1-integration | Active coordination only; no asset implementation; serial claim publication/integration |
| Environment & VFX Agent | ArtSource/Environment/SpaceFoundation/UnrealNative/**; Content/TriadDrift/Environment/SpaceFoundation/** | nick/environment-vfx | environment-vfx | Active first native environment milestone; original source outside UnrealNative read-only; future VFX/Audio not assigned |
| UI/UX & HUD Agent | Plugins/TriadDriftHUD/**; ArtSource/UI/TriadHUD/Unreal/**; Content/TriadDrift/UI/TriadHUD/** | nick/hud-native | hud-native | Active one user-authorized readability pass from 6d9739e / draft PR #4; exact roots unchanged; no provider rewrite/menus/shared activation; legacy staged claim preserved/excluded; heavy work queued |
| Ships & Weapons Agent | ArtSource/Space/Ships/Lancer/production/**; ArtSource/Space/Ships/Lancer/review/M02/** | nick/ships-lancer | ships-lancer | Active user-authorized M02 refinement; M01 sources/reviews and attachments read-only; preserve 33 names/transforms/pivots; no Unreal import/final production approval |
| Audio & Music Agent (dedicated chat 01a1d0e9-aaa8-7f61-85c7-86b6fd3f1db7) | ArtSource/Audio/StarterPlan/SwitchM02/** | nick/audio-music | audio-music | Frozen verified 11-file package at pushed 5979cff; draft PR #5; reservation retained; Nicholas audition pending, no main/native/library/music/shared config; predecessor stopped/read-only |

Completed Environment/VFX/HUD/Audio source snapshots are read-only shared references.
Future combat VFX output paths and broader Audio/music production are not
activated by these rows; only the explicitly claimed three-candidate audio task is active. No existing claim is released merely by publishing this table.

## Nick task board — checkpoint November 12, 2026, 12:52 EST

Only Lead updates these task states. `done` requires verified main integration;
branch-verified work awaiting shared integration stays in progress or queued.
Earlier task start times were not recorded precisely; they are not invented here.

| Task | Owner | State | Start / checkpoint (EST) | Main finish / boundary | Paths |
|---|---|---|---|---|---|
| Completed four-lane source milestone | Lead / former source workers | done | Earlier Nov 12 | Nov 12 12:35; main 957b45e; source prototypes only | Published ArtSource source kits; CREDITS.md; DEVLOG.md; source handoff |
| Resume production / claim and publication coordination | Lead Coordinator | done | Nov 12 12:49 | Nov 12 this status push; main claims 2d4831f and b8e8b46, verified HUD branch publication / PR #4; no unverified asset integrated | Lead coordination paths only; heavy queue preserved |
| SpaceFoundation first native sky | Environment & VFX Agent | in progress | Earlier Nov 12; checkpoint 12:52 | Build/reload reported passing; capture exposure rejected, diagnostics ongoing; not ready to integrate | Exact environment lane above; no shared maps/config |
| Native HUD milestone / shared integration handoff | UI/UX & HUD Agent | in progress | Earlier Nov 12; standalone verified, branch published 12:53 | Branch 6d9739e / draft PR #4; no main acceptance; shared activation/packaging requires Ryan; local claim preserved/excluded | Exact HUD lane above; no shared-file edit |
| HUD focused health/shield/ability/controlled-ship readability pass | UI/UX & HUD Agent | in progress | Nov 12 13:18 EST; direct UI-chat user instruction verified | Owned source edits cleared; native build/test/captures queued after existing Ships repair slot; no shared/provider/menu changes | Existing exact HUD three roots only |
| HUD gameplay activation / packaging integration | UI/UX & HUD Agent + Lead, coordinate Ryan | queued | Not started | Requires Ryan approval before .uproject/module or shared gameplay changes; no such edit authorized here | No additional writable paths granted |
| Lancer M01 silhouette review | Ships & Weapons Agent | queued | Review branch published Nov 12 12:42 | Branch 43dc320; general direction approved, polish/assembly issues unresolved; no main acceptance | M01 sources/reviews read-only |
| Lancer M02 refinement | Ships & Weapons Agent | in progress | Nov 12 12:50; claim 2d4831f | User-authorized production refinement; source preparation only until heavy slot; render approval before Unreal | production/** and review/M02/** inside exact Lancer lane |
| Lancer M02 bounded Blender QA / renders | Ships & Weapons Agent | in progress | Nov 12 13:11; Env explicit final release, Lead process/RAM check | Single bounded 2-thread build/QA/hero pass; no Unreal; release slot after exit, render approval still required | Same M02 lane, no original source/interface changes |
| Three cinematic ship-switch candidates (Rift/Lancer/Bastion) | Audio & Music Agent | in progress | Nov 12 12:56; ownership transfer 13:01, direct dedicated-chat brief verified | Branch 5979cff / draft PR #5 verified and published; three PCM24 stereo48k WAVs, hashes/decoded QA passed; no main acceptance, Nicholas audition required before further audio | ArtSource/Audio/StarterPlan/SwitchM02/** only |
| Message-board adoption and R-001/002/003 technical replies | Lead Coordinator | done | Nov 12 13:07 | Nov 12 13:10; main 52d5a83 replies published; R-002 activation and N-001 engineering decisions remain pending; technical plan in docs/visual/Presentation-Asset-Paths.md | MESSAGEBOARD.md; docs/visual/**; Nick claims only |

The dedicated Audio & Music chat succeeds the stopped Audio & Polish worker for this three-candidate task; the predecessor is not resumed.
Broader audio-library/music and future combat VFX production remain unassigned;
no inactive source worker is being restarted. New task/path transfers must be published here before edits.

## Ryan-side engineering board (Lead: Claude Code)

**Last updated: 2026-11-12 13:10 EST** by Ryan's Lead. Updated on every task start and finish
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
| Independent code review of Squad + Director | Lead | done | Nov 12 12:43 | Nov 12 13:05; findings with T3, T1, T2, T4 | none |
| Independent code review of Powers + World (ENG-09 to 12, 16, 17, 18) | Lead | in progress | Nov 12 12:46 | | none |
| Second review pass on Flow fixes | Lead | queued | | | none |
| ENG-28 build / test / screenshot / package tools | Lead | done | Nov 12 | `642f232` | `/Tools/**` |
| ENG-28 Shipping package under 1 GB | Lead | queued | | | `/Config/DefaultGame.ini` (maps and `/Engine/BasicShapes` are in the cook list as of this push) |
| Message board (`MESSAGEBOARD.md`) + AGENTS.md §6.10 | Lead | done | Nov 12 13:02 | Nov 12 13:02, Nov 12 13:05 push; R-001 to R-004 open for Nick's side | `/MESSAGEBOARD.md`, `/AGENTS.md` |
| C-01 maps (`L_MainMenu`, `L_Galaxy`, `L_Planet_Verdance`, `L_Planet_Cinder`, `L_Transition`) | Lead | done | Nov 12 12:48 | Nov 12 13:02, this push | **Claim released.** `Content/TriadDrift/Maps/**` now exists: empty World Partition levels, streaming off, One File Per Actor. Both sides may place actors (they save as separate `__ExternalActors__` files): claim the map and what you add first. Do not re-save the `.umap` or change World Settings without a claim here; game modes are set by map name in `/Config/DefaultEngine.ini`. See `MESSAGEBOARD.md` R-001 |
| ENG-27 balance + difficulty | Lead | queued | | | none |
| ENG-01 flight model, ENG-02 input map | T1 | in review | Nov 12 | all 15 review fixes merged Nov 12 13:02, this push (`e046c82`); second review queued | none |
| ENG-15 hyperdrive, ENG-20 atmosphere flight + landing (first pass) | T1 | in review | Nov 12 | Nov 12 13:02, this push (`2686272`); not reviewed yet | none |
| ENG-20 end to end: planet arrival, landing, first-person hand-off, take-off (with T5) | T1 | in progress | Nov 12 13:02 | | none |
| Placeholder look: squad-coloured hulls, glowing engines, `UTDShipVisuals` slot | T1 | done | Nov 12 | Nov 12 13:02, this push (`ab8915b`) | none |
| ENG-05 ship weapons, ENG-06 targeting | T2 | in review | Nov 12 | `6bd14ec`; all 16 review fixes finished on `eng/t2` `b4a6dc2` but NOT on `main`: unity-build name collision with other branches, T2 fixing | none |
| ENG-09 Rift portal blaster, ENG-10 Lancer ram, ENG-11 Bastion swarm + dome, ENG-12 unlockable abilities | T2 | in review | Nov 12 | Nov 12 12:46, this push (`6455ee0`); independent review running | none |
| ENG-03 respawn rules, ENG-04 three ships, ENG-07 switching, ENG-08 wingman AI | T3 | in review | Nov 12 | `448f658`; review done Nov 12 13:05: 2 blockers, 8 majors; fixes in progress | none |
| C-03 #1 switch-as-cut, #2 director-cam money shots | T3 | in review | Nov 12 | Nov 12 12:45, this push (`1309c5b`); review running | none |
| Squad follow-ups: travel guard in director, powers installed on the three ships, weapon reset on respawn, targeting HUD forwarders | T3 | done | Nov 12 12:43 | Nov 12 13:02, this push (`9c93bd1`) | none |
| Squad follow-ups: flight reset on respawn, manual target carry-over, bullet-by-bullet spec table | T3 | in progress | Nov 12 13:02 | | none |
| ENG-13 enemy ships + AI, ENG-14 wave director | T4 | in review | Nov 12 | all review BLOCKER and MAJOR fixes merged Nov 12 13:10, this push (`7ae379c`: reachable carrier parts, leash, break-off, 11 real-path tests, per-type visuals by path convention); minors in progress; second review queued | none |
| Galaxy layout swap to T1's Travel API (play bounds, gravity wells, atmosphere entry) | T4 | in progress | Nov 12 13:10 | | none |
| ENG-18 Dominion assault + reactor finale | T4 | in review | Nov 12 | `571d85f` | none |
| ENG-16 portal gates, ENG-17 galaxy layout | T4 | in review | Nov 12 | Nov 12 12:45, this push (`ea32ff5`); not reviewed yet | none |
| ENG-19 game instance / saves / travel, C-03 #3 scenes and takes | T5 | in review | Nov 12 | `a2fdf3e`; all 12 review fixes merged Nov 12 12:45, this push (`125146f`); second review queued | none |
| ENG-21 first-person character, ENG-22 energy blade, ENG-24 objective framework | T5 | in review | Nov 12 | Nov 12 13:02, this push (`fc329d0`); not reviewed yet | none |
| Menu and transition game modes (`ATDMenuGameMode`, `ATDTransitionGameMode`) | T5 | done | Nov 12 | Nov 12 13:10, this push (`85af40b`) | `/Config/DefaultEngine.ini` map prefixes |
| ENG-23 crew enemies | T5 | in progress | Nov 12 13:02 | | none |
| ENG-25 Verdance gameplay, ENG-26 Cinder gameplay | T5 | queued | | | places gameplay in `L_Planet_*` from C++ only |
| ENG-29 docking, ENG-30 interior + core fight, ENG-31 escape (stretch) | unassigned | queued | | | only after core is solid |

### Coming next that touches shared things (heads-up for Nick's side)

- `/Config/DefaultEngine.ini` now boots into `L_Galaxy` and assigns game modes by map name
  (`L_Galaxy` space, `L_Planet*` and `L_Dominion*` ground). A menu game mode for `L_MainMenu`
  and the boot-map switch to it follow when the menu exists.
- `unrealGameJam.uproject` is unclaimed. Nick's `Plugins/TriadDriftHUD/**` row implies a plugin
  entry there and a module dependency. A plugin cannot include the game module's headers
  (`ITDHUDData`, `UTDEventBus`), so how the HUD reads gameplay data needs to be agreed between
  Ryan and Nick before that plugin is integrated.
