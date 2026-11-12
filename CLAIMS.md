# Triad Drift active claims

Claim handling follows AGENTS.md §6.5 and §6.9. No binary work is authorized by
an unpublished claim. The Lead preserves all other active claims when updating this file.

| Who / agent | Files / folders / assets | Since | Branch / worktree | Status |
|---|---|---|---|---|
| Nicholas / Lead | AGENTS.md; CLAIMS.md (coordination infrastructure only) | 2026-11-12 EST | nick/agent-infrastructure / Worktrees/agent-infrastructure | Awaiting main integration; no binary work |

## Current Nick-owned reservations — November 12, 2026

Additive publication by Lead; the preceding claims are preserved unchanged.
These narrow active lanes supersede the unpublished broad placeholders in PR #1,
not any teammate's active claim. Shared Source/Config/maps/interfaces are excluded.
See docs/visual/Day1-Integration-Policy-2026-11-12.md for the dated integration exception.

| Owner | Exact writable paths | Branch | State |
|---|---|---|---|
| Nicholas / Lead Coordinator | docs/visual/**; CREDITS.md; DEVLOG.md; coordinated CLAIMS.md updates | nick/visual-assets; nick/day1-integration | Coordination and eligible integration only |
| Nicholas / Environment & VFX Agent | ArtSource/Environment/SpaceFoundation/UnrealNative/**; Content/TriadDrift/Environment/SpaceFoundation/** | nick/environment-vfx | Active first native environment milestone; original source files outside UnrealNative stay read-only |
| Nicholas / UI/UX & HUD Agent | Plugins/TriadDriftHUD/**; ArtSource/UI/TriadHUD/Unreal/**; Content/TriadDrift/UI/TriadHUD/** | nick/hud-native | Active; original browser prototype outside Unreal stays read-only; native/plugin work withheld from integration |
| Nicholas / Ships & Weapons Agent | ArtSource/Space/Ships/Lancer/** | nick/ships-lancer | Active review/loader/QA outputs only; preserve mesh sources; unapproved silhouette and failed assembly QA withheld |

Completed Environment/VFX/HUD/Audio source snapshots are read-only shared references.
Future combat VFX output paths and Audio/music production are not activated or
assigned by these rows. No existing claim is released merely by publishing this table.

## Current Ryan-owned reservations (engineering) — November 12, 2026

Additive publication by Ryan's Lead (Claude Code); every preceding claim is preserved unchanged.
All of Ryan's side is C++ / text so far: **no `.uasset`, `.umap` or other binary file is claimed or
edited yet.** Maps and gameplay content will be claimed here, and pushed, before anyone creates them.
The per-item status ledger is `docs/eng/STATUS.md`; the code-ownership contract is
`docs/eng/ARCHITECTURE.md`. Paths below are under `Source/unrealGameJam/TriadDrift/` unless they
start at the repo root.

| Owner | Exact writable paths | Branch / worktree | State |
|---|---|---|---|
| Ryan / Lead (Claude Code) | `Core/**`; `Tests/**`; `/Source/unrealGameJam/unrealGameJam.Build.cs`; `/Config/**`; `/Tools/**`; `/docs/eng/**`; `/.claude/skills/ue-cpp-gameplay/**`; coordinated `/CLAIMS.md` updates (own rows only) | `lead/int` / `.wt/int`, integrates to `main` | Active: contracts, integration, review, ENG-27, ENG-28 |
| Ryan / Teammate 1 | `Flight/**`; `Input/**`; `Travel/**`; `/docs/eng/flight.md` | `eng/t1` / `.wt/t1` | Active: ENG-01, 02 review fixes; ENG-15 hyperdrive; ENG-20 atmosphere + landing |
| Ryan / Teammate 2 | `Combat/**`; `Powers/**`; `/docs/eng/combat.md` | `eng/t2` / `.wt/t2` | Active: ENG-05, 06 review fixes; ENG-09 to ENG-12 powers and abilities |
| Ryan / Teammate 3 | `Squad/**`; `Director/**`; `/docs/eng/squad.md` | `eng/t3` / `.wt/t3` | Active: ENG-04, 07, 08; C-03 #1 switch-as-cut, #2 director cam |
| Ryan / Teammate 4 | `Enemies/**`; `Waves/**`; `World/**`; `/docs/eng/enemies.md` | `eng/t4` / `.wt/t4` | Active: ENG-13, 14 review fixes; ENG-16 gates; ENG-17 galaxy layout; ENG-18 Dominion |
| Ryan / Teammate 5 | `Flow/**`; `Ground/**`; `/docs/eng/flow.md` | `eng/t5` / `.wt/t5` | Active: ENG-19 review fixes; C-03 #3; ENG-21 to ENG-26 on-foot gameplay |

Workers commit only on their own branch and never pull, merge or push; the Lead integrates in
`.wt/int`, builds, runs the headless suite, then fast-forwards and pushes `main`.

Not claimed yet (will be published here first): `Content/TriadDrift/Maps/**` (shared),
`Content/TriadDrift/Gameplay/**`, `unrealGameJam.uproject`. Nick's `Plugins/TriadDriftHUD/**` row
implies a `.uproject` plugin entry and a module dependency: that is a shared change, coordinate
through Ryan before integrating it.
