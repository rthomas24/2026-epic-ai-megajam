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
| Lead Coordinator | docs/visual/**; CREDITS.md; DEVLOG.md; coordinated CLAIMS.md updates; Nick-side MESSAGEBOARD.md posts/replies under AGENTS §6.10; AAA.md (Nicholas/Ryan-approved shared guide) | nick/visual-assets; nick/day1-integration | visual-assets; day1-integration | Active coordination only; no asset implementation; serial claim publication/integration |
| Environment & VFX Agent | ArtSource/Environment/SpaceFoundation/UnrealNative/HeroM02/**; Content/TriadDrift/Environment/SpaceFoundation/HeroM02/** | nick/environment-vfx | environment-vfx | Active directly authorized Hero-only realism benchmark; baseline native12/source6 read-only at main9c629e8; no Combat/Verdance/VFX propagation before Nicholas review; Attempt01startup125 archived; SM6retry02 ended125/no_progress608.3s before assetscript, no native/captures; attempts01/02 preserved, no furtherlaunch; privatehostRendererSettings/defaultplugin/cache-alignment source correction approved after read-only diagnosis; no thirdlaunch until freshgrant |
| UI/UX & HUD Agent | Plugins/TriadDriftHUD/**; ArtSource/UI/TriadHUD/Unreal/**; Content/TriadDrift/UI/TriadHUD/** | nick/hud-lancer-polish (active); nick/hud-native (frozen PR4) | hud-lancer-polish; hud-native frozen | M02 main8963477 functional readability approved; M03 aca9334 source clean/frozen awaiting manual visual approval, screenshot delivery only; no new source/native work or other-ship propagation; old staged CLAIMS preserved |
| Ships & Weapons Agent | ArtSource/Space/Ships/Rift/production/**; ArtSource/Space/Ships/Rift/review/M01/** | nick/ships-rift | ships-rift | Active directly authorized Rift refined-foundation source milestone; stop for Nick review before Bastion; Lancer4b7ef12 manually approved foundation and fully frozen/read-only, final AAA/native integration not approved |
| Ships & Weapons Agent (future reservation only) | ArtSource/Space/Ships/Bastion/production/**; ArtSource/Space/Ships/Bastion/review/M01/**; Content/TriadDrift/Art/Ships/Rift/**; Content/TriadDrift/Art/Weapons/Rift/**; Content/TriadDrift/Art/Ships/Bastion/**; Content/TriadDrift/Art/Weapons/Bastion/**; Content/TriadDrift/Presentation/DA_Rift_Visuals.uasset; Content/TriadDrift/Presentation/DA_Bastion_Visuals.uasset | Future nick/ships-bastion; native not dispatched | None yet | EXCLUSIVE RESERVED, no binary/native/import edits or Bastion production authorized before respective human gates/new task dispatch; no FX/VFX ownership transfer |
| Audio & Music Agent (dedicated chat 01a1d0e9-aaa8-7f61-85c7-86b6fd3f1db7) | ArtSource/Audio/EpicLibraryAudit01/** (report/page/source only); ignored Saved/AudioMusic/EpicLibraryAudit01/** (private local audition copies only) | nick/audio-music | audio-music | Owned-library audit safely frozen after Nick Escape stop: four uncommitted report/page/code files and ignored private WAVs preserved, five provisional candidates; license-tier/playback/listening approval pending; NO resume, media push, purchases, large downloads, Unreal/import/shared integration; rejected concepts immutable8cd30fb |

Completed Environment/VFX/HUD/Audio source snapshots are read-only shared references.
Future combat VFX output paths and broader Audio/music production are not
activated by these rows; audio is limited to the explicitly authorized existing-library audition audit above, with no new sound production or gameplay integration. No existing claim is released merely by publishing this table.

## Nick task board — checkpoint November 12, 2026, 12:52 EST

Only Lead updates these task states. `done` requires verified main integration;
branch-verified work awaiting shared integration stays in progress or queued.
Earlier task start times were not recorded precisely; they are not invented here.

| Task | Owner | State | Start / checkpoint (EST) | Main finish / boundary | Paths |
|---|---|---|---|---|---|
| Completed four-lane source milestone | Lead / former source workers | done | Earlier Nov 12 | Nov 12 12:35; main 957b45e; source prototypes only | Published ArtSource source kits; CREDITS.md; DEVLOG.md; source handoff |
| Resume production / claim and publication coordination | Lead Coordinator | done | Nov 12 12:49 | Nov 12 this status push; main claims 2d4831f and b8e8b46, verified HUD branch publication / PR #4; no unverified asset integrated | Lead coordination paths only; heavy queue preserved |
| SpaceFoundation first native sky | Environment & VFX Agent | in review | Earlier Nov 12; checkpoint 12:52 | Main 9c629e8, Nov 12 13:28 EST; isolated foundation only, 12native/5captures/37files validated; known pole-star stretch; map integration/performance review pending, no shared placement/PIE/package claim | Exact environment lane above; no shared maps/config |
| Native HUD milestone / shared integration handoff | UI/UX & HUD Agent | in review | Earlier Nov 12; standalone verified, branch published 12:53 | PR4 merged8963477; current-project build/326tests passed, human readability approved; N004actual-unlock/host/cook/liveHUD gated; old stagedclaim preserved | Exact HUD lane above; no shared-file edit |
| HUD focused health/shield/ability/controlled-ship readability pass | UI/UX & HUD Agent | in review | Nov 12 13:18 EST; direct UI-chat user instruction verified | Complete branch checkpoint2f55512; max1 build PASS,3UItests PASS,7CRC/hash-checked captures, final centering0.5px1080/1px720; no final AAA acceptance | Existing exact HUD three roots only |
| HUD gameplay activation / packaging integration | UI/UX & HUD Agent + Lead, coordinate Ryan | queued | Not started | R007 approves plugin/content integration; Lead publishes approved revision, engineering owns host/config switch/build/suite/Shipping/capture before default-on; no Nick shared-file edits | No additional writable paths granted |
| Lancer M01 silhouette review | Ships & Weapons Agent | in review | Review branch published Nov 12 12:42 | Historical M01 preserved within PR3/100f21c; currentM02 foundation manually approved, finalAAA/native gates retained | M01 sources/reviews read-only |
| Lancer M02 refinement | Ships & Weapons Agent | in progress | Nov 12 12:50; claim 2d4831f | Frozen4b7ef12 source foundation manually approved and integrated100f21c; no further polish assigned, no native approval | production/** and review/M02/** inside exact Lancer lane |
| Lancer M02 bounded Blender QA / renders | Ships & Weapons Agent | in review | Round03 exited/released | Round04 PASS57,788tris,33locators/12rays/geometry,9beauties+clay;54newfiles published4b7ef12; source frozen pending Nick presentation review, no native/main acceptance | Same M02 lane only |
| Three cinematic ship-switch candidates M02 (Rift/Lancer/Bastion) | Audio & Music Agent | queued | Nov 12 12:56; ownership transfer 13:01, direct dedicated-chat brief verified | M02 rejected by Nicholas as abrasive/rough; immutable baseline 5979cff / draft PR #5 retained for A/B, superseded by authorized M03; no main integration | ArtSource/Audio/StarterPlan/SwitchM02/** only |
| Message-board adoption and R-001/002/003 technical replies | Lead Coordinator | done | Nov 12 13:07 | Nov 12 13:10; main 52d5a83 replies published; R-002 activation and N-001 engineering decisions remain pending; technical plan in docs/visual/Presentation-Asset-Paths.md | MESSAGEBOARD.md; docs/visual/**; Nick claims only |
| M03 smooth cinematic ship-switch revision | Audio & Music Agent | queued | Nov 12 13:26 EST; direct audio-chat user request verified | Superseded by RiftConcept01; Archived and pushed202b00d; rejected/superseded read-only reference, no more M03 edits/integration | ArtSource/Audio/StarterPlan/SwitchM03/** only |
| Live Git/ownership/integration audit | Lead Coordinator | done | Nov 12 13:26 EST | Finished Nov 12 13:32 EST; remote9c629e8 Environment integrated, PR6closed-as-integrated; PR3/4/5held; M03 claim/live board/five exclusive lanes reconciled; heavy queue resumed | Lead coordination paths and frozen Environment integration only |
| Team AAA quality / production alignment | Lead Coordinator | done | Nov 12 13:35 EST | Main454691f ledger/delegation and5744d6b R006/R007 replies published; all4worker chats aligned, separate stages and bounded benchmarks; no subjective AAA acceptance assumed | Lead coordination paths |
| Publish shared AAA quality guide | Lead Coordinator | done | Nov 12 13:53 EST; complete approved proposal supplied directly, Ryan approval reported | Finished Nov 12 13:58 EST; main3b870f2 full guide, N002/7117816 notice; all4asset chats read/ACK plus Lead, no instruction overwrite | AAA.md; Lead board/claims/docs paths |
| Ryan-approved three-PR integration audit | Lead Coordinator | in review | Nov 12 13:58 EST; AAA safe checkpoint complete | PR4 merged8963477, build/326tests and default-Galaxy scripted3capture smoke passed; PR3M02published4b7ef12/manualreviewheld, PR5rejectedarchiveheld; finalN005/DEVLOG; no fullgameplay/Shipping claim | Existing agent lanes; no shared ownership expansion |
| Independent Environment foundation review | Nick / native worker /root/review_env | in progress | Current audit; AGENTS11 fresh-review rule | Read-only: main9c629e8 assets/evidence at reviewed current main; no writes, Git mutations or heavy apps | Worktrees/day1-integration read-only; branch not mutated |
| Independent HUD integration review | Nick / native worker /root/review_hud | done | Current audit; AGENTS11 fresh-review rule | Fresh source reviewM02 clears sourceintegration; e356921 producer recheck closes N004MAJOR at codelevel (None/0 locked, earned state, respawn/retake); tests inspected not rerun, attached-widget sequence remains N007; writesNONE | Worktrees/hud-native and day1-integration, plugin + Source/Squad/Powers read-only references; assigned writers unchanged |
| Hero-only cinematic realism benchmark | Environment & VFX Agent | in progress | Nov 12 13:35 EST; direct Env-chat user instruction verified | Matched camera/exposure before-after, star-zenith correction, native stability/perf evidence; manual approval before other presets/main; heavy queued after HUD/Ships | Exact HeroM02 roots above; originals read-only |
| New Rift premium spatial-shimmer concept | Audio & Music Agent | queued | Nov 12 13:35 EST; direct Audio-chat correction verified | Rejected by Nicholas;8files committed/pushed8cd30fb, CC0 provenance checked, no further edits/integration; superseded by read-only professional-library research | ArtSource/Audio/StarterPlan/RiftConcept01/** |

| Lancer-only HUD styling benchmark M03 | UI/UX & HUD Agent | in review | Nov12 14:23 EST; direct user-approved benchmark verified | Branch aca9334 safely committed/pushed,27ownedfiles/10LFS,build4tests10captures pass, treeclean; sourcefrozen pending Nickmanualreview, no mainmerge/propagation; externalinput/fullgame/Shipping unverified | Same exact three HUD roots; no shared code/config |
| Existing Epic/Fab audio-library audition audit | Audio & Music Agent | in review | Nov12 14:23 EST; direct user request verified in audio chat01a1d131-3fc9-79d3-ade8-9a47132ced91 | Five provisional candidates/private page exist; safe freeze after Nick Escape, browser playback and exact acquisition license unresolved; original hashes preserved, no licensed audio in Git; no resume/import before approval | Exact audit roots above |

| Audit Ryan audio skill and adoption | Lead Coordinator | in review | Nov12 14:34 EST; after PR integration checkpoint09057ca | Checked main970b540/all fetched refs/path history: no audio skill published; N006 request39d89f5 sent, Audio text ACK; adoption awaits Ryan publication, dependencies/workflow unverified | Lead coordination paths only; skill read-only |

| Rift refined production foundation Phase1 | Ships & Weapons Agent | in progress | Nov12 14:39 EST; direct ship-chat user brief verified01a1d145-290e-7740-87fc-532c9d76e0da | Source/brief5scripts syntaxpass; Nov12 15:06 EST PASS01 exited0/about10s/slotreleased:38,624tris,6materials,19frames/6rays/contact/topologyUV pass; coplanars fail(no promotion), targetedsourcefixready; Nov12 15:11 EST ONE2thread<60sPASS02 repair QA/conditionalreview granted with fresh5.5GiB/noheavy guard,19frames/noQArelaxation; releasebeforeLeadHUDtest; no native/Bastion/manualapproval | Exact active Rift source roots above; native reservations read-only |
| Integrated HUD gameplay validation | Lead Coordinator | in progress | Nov12 14:42 EST; after scheduling checkpoint | Hoste66e471/2f53f4d nowonmain, switchdefaultoff, docseng/ui-host read; Lead builds/focused7UItests + controlledprivate8020 actualgame validation after Rift02 release; Source/UI/Config remain Ryan-owned/read-only; no sharedactivation, Shipping/cook gate separate | Lead coordination paths; runtime evidence ignored Saved/LeadIntegration only |

| Full PR1–6 audit and eligible source integration | Lead Coordinator | done | Nov12 14:46 EST; after N007 HUD-host handoff checkpoint | Finished Nov12 14:57 EST, main100f21c/372da05/N008 final0bbe63a; all6accounted, PR3source-onlymerged, freshpre/postreview/74LFS pass, no runtime change; distinct HUD/Shipping/native gates retained | Lead coordination paths, immutable approved PR3 integration only |
| Independent Lancer source integration review | Native reviewer /root/review_hud | done | Nov12 14:46 EST | Pre/post reviewPASS100f21c:111additions exactly4b7ef12, no runtime changes or BLOCKER/MAJOR; source foundation approved, finalAAA/native gates separate; writable pathsNONE | Worktrees/ships-lancer and day1-integration read-only; writable paths NONE |

| Fresh HUD-host independent review | Native reviewer /root/review_hud | in progress | Nov12 15:11 EST | Read-only hoste66e471 and merged dependencies; assess lifecycle/reflection/opt-in/safe test, no edits/Git/heavyapps | Source/UI and plugin/main references read-only, writesNONE |

The dedicated Audio & Music chat succeeds the stopped Audio & Polish worker for read-only professional-library research; all rejected media remain immutable and the predecessor is not resumed.
Broader audio-library/music and future combat VFX production remain unassigned;
no inactive source worker is being restarted. New task/path transfers must be published here before edits.

## Ryan-side engineering board (Lead: Claude Code)

**Last updated: 2026-11-12 15:33 EST** by Ryan's Lead. Updated on every task start and finish
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
| Independent code review of Powers + World (ENG-09 to 12, 16, 17, 18) | Lead | done | Nov 12 12:46 | Nov 12 13:15; findings with T2 and T4 | none |
| Core: `UTDShipVisuals.Parts` (modular ships), `UTDWeaponFX.ProjectileMeshOffset`, highlight broadcast restricted to the director | Lead | done | Nov 12 13:16 | Nov 12 13:16, this push (answers `MESSAGEBOARD.md` N-001) | none |
| ENG-28 build / test / screenshot / package tools | Lead | done | Nov 12 | `642f232` | `/Tools/**` |
| Enable Unreal 5.9 agent toolsets for the built-in MCP server; MCP play-test tools (`Tools/td-play-mcp.ps1`, `Tools/td_mcp.py`) | Lead | done | Nov 12 13:33 | Nov 12 13:38 (`adb1912`); **`/unrealGameJam.uproject` claim released**; see `MESSAGEBOARD.md` R-008 | `/unrealGameJam.uproject` (21 engine toolset plugins, Editor targets only) |
| Engineering host for Nick's HUD plugin (`UTDUIHostSubsystem`, switch `bUseNativeSpaceHUD` / `TD.UI.NativeHUD`, frozen-name and N-004 sequence tests) | T2 | in review | Nov 12 14:38 | Nov 12 15:08 (`2f53f4d`); hand-over posted under `MESSAGEBOARD.md` N-007; default OFF until played with the name-card and objective fixes; Shipping cook check open | new `Source/unrealGameJam/TriadDrift/UI/**`; `/Config/DefaultGame.ini` (cook line for `/Game/TriadDrift/UI/TriadHUD`). No module dependency on the plugin; `Plugins/TriadDriftHUD/**` stays read-only for engineering |
| Shared protocols in AGENTS.md section 11 + `MESSAGEBOARD.md` R-009; presentation assets always cooked (asset manager scan) | Lead | done | Nov 12 13:57 | Nov 12 13:57, this push | `/AGENTS.md`, `/Config/DefaultGame.ini`, `/.gitignore` |
| Test harness: local player controller + real key injection (`FTDTestWorld`) | Lead | done | Nov 12 13:48 | Nov 12 13:56 (`c911fa3`) | none |
| Second-pass reviews: Ground + Flow, Travel + Flight | Lead | done | Nov 12 13:21 | Nov 12 13:58; findings with T1 and T5 | none |
| Audio skill for Nick's agents (`.claude/skills/ue-game-audio/`: sourcing, layering, Unreal setup, level table, tested loudness scripts) + `Tools/td-play-mcp.ps1 -Audio` | Lead | done | Nov 12 14:03 | Nov 12 15:14 (`f31a1f1`); announced in `MESSAGEBOARD.md` R-013; technically verified, not heard by a person | `/.claude/skills/ue-game-audio/**`, `/reports/**`, `/Tools/td-play-mcp.ps1`, `/AGENTS.md` section 10 |
| Engineering playtest 1 through MCP + first packaged-build smoke test (logged in `DEVLOG.md`) | Lead | done | Nov 12 14:06 | Nov 12 14:15; five problems sent to owners | `/DEVLOG.md` (append only) |
| Core: one quiet loader for presentation assets (`UTDStatics::LoadPresentation<T>`), Python name clash fix | Lead | done | Nov 12 14:10 | Nov 12 14:15, this push | `/Source/unrealGameJam/unrealGameJam.Build.cs` (adds AssetRegistry) |
| C-03 #2 director camera that really cuts, #1 fallback cut presentation (flash, letterbox, name card, CUT!, slate), squad barks | T3 | in review | Nov 12 13:40 | Nov 12 14:15, this push (`ebe00a5`); live captures in progress | none |
| ENG-20 ground half (planet director: fly in, land, first person, re-board, take off) + on-foot soft-lock fixes | T5 | in review | Nov 12 13:50 | Nov 12 14:15, this push; blade feel fixes in progress | none |
| `MESSAGEBOARD.md` N-004 fix 2: one placeholder sky actor that yields to the map's sky (tag, light component or post-process) | T5 | done | Nov 12 14:16 | Nov 12 14:38 (`8c03f82`); posted for Nick's reviewers | none |
| Capture tools: verify a written PNG ends at IEND before it is used as evidence (pitfall from Nick's side) | Lead | queued | | | `/Tools/**` |
| Map housekeeping: remove or fix the unused default HLOD layer assets that warn on load | Lead | queued | | | **will claim `Content/TriadDrift/Maps/*_HLODLayer_*.uasset` and the five `.umap` World Settings here before editing** |
| Engineering playtest 2: real injected input and journey jumps through MCP (logged in `DEVLOG.md`) | Lead | done | Nov 12 14:52 | Nov 12 14:54; six problems sent to owners | `/DEVLOG.md` (append only) |
| Agent input injection (`TD.Input.*`) and journey cheats (`TD.Stage`) | T1, T5 | done | Nov 12 14:20 | Nov 12 14:47 (`8c5aa47`); played live | none |
| `features.md` engineering numbers updated to the tuned game | Lead | done | Nov 12 14:50 | Nov 12 14:52 (`b8027b7`) | `/features.md` (ENG sections only) |
| Visible placeholder projectiles (glowing streaks sized by speed, missile heads), pooled muzzle and impact flashes; each hides when the weapon FX has art | T2 | in review | Nov 12 15:05 | Nov 12 15:14 (`f31a1f1`); to be judged in the Lead's playtest 3 | none |
| Message board compaction (`Tools/td_board_compact.py`, archive in `docs/coordination/`), rule in AGENTS.md 6.10 | Lead | done | Nov 12 15:00 | Nov 12 15:00, this push; repeat when the board passes about 300 lines | `/MESSAGEBOARD.md`, `/AGENTS.md`, `/docs/coordination/**` |
| `MESSAGEBOARD.md` N-004 fix 1 (earned ability, None while locked) | T3 | done | Nov 12 14:16 | Nov 12 14:59 (`f96a4d6`); posted for Nick's reviewers | none |
| Core: live state for audio and music (`UTDAudioStateSubsystem`, `FTDAudioState`, smoothed `MusicIntensity01` / `Combat01` / `Danger01`), tests at 30 / 60 / 120 fps | Lead | done | Nov 12 15:14 | Nov 12 15:33 (`f2db3ce`); announced in `MESSAGEBOARD.md` R-014; no audio uses it yet | none |
| Mouse virtual stick recentres so a turn settles wings-level; motion blur off in space (playtest 2 findings) | T1 | in review | Nov 12 15:00 | Nov 12 15:14 (`f31a1f1`); to be judged in playtest 3 | none |
| On foot: held placeholder blade with per-move poses, solid crew with white eyes, no tick-cycle warnings (playtest 2 findings) | T5 | in review | Nov 12 15:00 | Nov 12 15:14 (`f31a1f1`); to be judged in playtest 3 | none |
| Engine loop audio parameters `Speed01` / `Boost01` / `Drift01` and stick reset on switch (`eeaeaa1`), landing prompts (`f2653f8`): on `eng/t1`, next integration; landing approach assist on held F; own MCP playtest; boundary experience | T1 | in progress | Nov 12 15:14 | | none |
| Combat review fixes (2 MAJOR, 7 minors); Bastion flash and shell presence, Rift portal placeholder as a ring (playtest 3); HUD re-capture with locked abilities; Shipping cook check of `WBP_SpaceHUD` | T2 | in progress | Nov 12 15:26 | | none |
| Fallback name card steps aside under the real HUD (`7c9c17d`, on `main` `f2db3ce`); objective markers for the space HUD by actor tag `TD.Objective.Marker`; landing prompt through the squad manager; slate clear of the objective block; wingmen in frame (measured with `TD.Squad.Frame`) | T3 | in progress | Nov 12 15:14 | | none |
| Enemy AI review fixes (4 MAJOR) and the wave soft-lock; enemy placeholder visibility and scale; opening pass that crosses in front (`1a372d2`, measured: enemy on screen 59-96% of the time); objective tags on generators / reactor / destinations; spawn points clear of obstacles | T4 | in progress | Nov 12 15:14 | | none |
| Star backdrop readable from every direction (playtest 3: half the sky is black); objective text for wave and finale scenes (`OnObjectiveUpdated`); objective destination tag with T4; star backdrop removed when a tagged sky appears late | T5 | in progress | Nov 12 15:14 | | none |
| Second independent reviews: Combat (ENG-05, 06), Enemies / Waves (ENG-13, 14) | Lead | done | Nov 12 15:14 | Nov 12 15:31; findings with T2, T3, T4, T5 (summary in `DEVLOG.md`) | none |
| Engineering playtest 3 with the real HUD on, visible projectiles and the flight fixes (logged in `DEVLOG.md`) | Lead | done | Nov 12 15:25 | Nov 12 15:28; nine problems sent to owners | `/DEVLOG.md` (append only) |
| `TD.Stage DominionAssault` / `ReactorExposed` start on the approach facing the Dominion; planet placeholder sky fill (no pure black shadows) | T5 | done | Nov 12 15:14 | Nov 12 15:33 (`f2db3ce`); captured live by T5 | none |
| Atmosphere soft ground floor (hands-off planet arrival levels out, never crashes); landing rule changed in `features.md` ENG-20 (120 m, 60% of the atmosphere cap, slow-down prompt) | T1 | in review | Nov 12 15:10 | Nov 12 15:33 (`f2db3ce`); prompt texts in progress | `/features.md` (ENG-20) |
| Powers test crash under garbage collection (test-made data assets unrooted; the game holds them properly): rooted, forced-collection probes kept as regression guards (`1421acb`) | T2 | done | Nov 12 15:19 | Nov 12 15:33 (`f2db3ce`); suite green three runs in a row on T2's side | none |
| Core: `TDTags` (`TD.Objective.Marker`, `TD.Presentation.Sky`); `features.md` ENG-06 lock and marker rules; dev HUD draws the landing prompt while flying (next integration) | Lead | done | Nov 12 15:32 | Nov 12 15:33 (`f2db3ce`) | `/features.md` (ENG-06, ENG-20) |
| ENG-28 Shipping package (750 MB with symbols, about 510 MB without; Development package boots and runs) | Lead | in progress | Nov 12 13:21 | | none (output goes to git-ignored `Builds/`) |
| Core: front-end widget classes on `UTDUIStyle`, crew `AnimBPClass` / `Stagger` / `Death`, `BladeMesh` (answers `MESSAGEBOARD.md` R-004) | Lead | done | Nov 12 13:21 | Nov 12 13:21, this push | none |
| Message board (`MESSAGEBOARD.md`) + AGENTS.md §6.10 | Lead | done | Nov 12 13:02 | Nov 12 13:02, R-001 to R-005 and N-001 complete; R-001 to R-007 complete; R-008 (toolsets) and R-009 (shared protocols) open for Nick's side | `/MESSAGEBOARD.md`, `/AGENTS.md` |
| C-01 maps (`L_MainMenu`, `L_Galaxy`, `L_Planet_Verdance`, `L_Planet_Cinder`, `L_Transition`) | Lead | done | Nov 12 12:48 | Nov 12 13:02, this push | **Claim released.** `Content/TriadDrift/Maps/**` now exists: empty World Partition levels, streaming off, One File Per Actor. Both sides may place actors (they save as separate `__ExternalActors__` files): claim the map and what you add first. Do not re-save the `.umap` or change World Settings without a claim here; game modes are set by map name in `/Config/DefaultEngine.ini`. See `MESSAGEBOARD.md` R-001 |
| ENG-27 balance + difficulty | Lead | queued | | | none |
| ENG-01 flight model, ENG-02 input map | T1 | in review | Nov 12 | second review done Nov 12 13:58: ENG-02 acceptable; ENG-01 needs drift-burst pumping and pinned-contact damage fixed; in progress | none |
| ENG-15 hyperdrive, ENG-20 atmosphere flight + landing | T1 | in review | Nov 12 | hardening batch merged Nov 12 13:59 (`479857b`: keyed speed overrides, look-ahead, hyperdrive locks, travel guards); review done: landing dead end (BLOCKER), wingman overshoot, landing state on death, cue loops; fixes in progress | none |
| Modular ship parts from `UTDShipVisuals.Parts`; flight and travel cues played from data | T1 | done | Nov 12 13:19 | Nov 12 13:38 (`1588fdb`) | none |
| ENG-20 end to end: first half verified live (`L_Galaxy` -> atmosphere -> `L_Transition` -> `L_Planet_Verdance`); landing hand-off, re-board, take-off and return wait on T5's ENG-25 / 26 ground mode | T1 + T5 | in progress | Nov 12 13:02 | | none |
| Placeholder look: squad-coloured hulls, glowing engines, `UTDShipVisuals` slot | T1 | done | Nov 12 | Nov 12 13:02, this push (`ab8915b`) | none |
| ENG-05 ship weapons, ENG-06 targeting | T2 | in review | Nov 12 | second review done Nov 12 15:26: not acceptable yet. MAJOR: placeholder flashes hide on a sound-only cue; Bastion locks never expire. 7 minors. Fixes in progress | none |
| ENG-09 Rift portal blaster, ENG-10 Lancer ram, ENG-11 Bastion swarm + dome, ENG-12 unlockable abilities | T2 | in review | Nov 12 | all Powers review findings merged Nov 12 14:38 (`8c03f82`): data-driven cues, placeholders that ship, retake state, every test at 30 / 60 / 120 fps; played by an agent through MCP with captures; second review queued | reads `/Game/TriadDrift/Presentation/DA_*` by path |
| ENG-03 respawn rules, ENG-04 three ships, ENG-07 switching, ENG-08 wingman AI | T3 | in review | Nov 12 | `448f658`; review done Nov 12 13:05: 2 blockers, 8 majors; fixes in progress | none |
| C-03 #1 switch-as-cut, #2 director-cam money shots | T3 | in review | Nov 12 | Nov 12 12:45, this push (`1309c5b`); review running | none |
| Squad follow-ups: travel guard in director, powers installed on the three ships, weapon reset on respawn, targeting HUD forwarders | T3 | done | Nov 12 12:43 | Nov 12 13:02, this push (`9c93bd1`) | none |
| Squad / Director review fixes batch 1 (wingmen stuck after switch during hyperdrive, stale aim, death tumble + flight reset, real-time cooldown, switch guard) | T3 | done | Nov 12 13:05 | Nov 12 13:16, this push (`26670f2`) | none |
| Squad / Director review fixes batch 2 (per-ship art by path, switch invulnerability rule, wingman collisions, multi-kill rules, 15 minors) and real-actor director tests | T3 | done | Nov 12 13:19 | Nov 12 13:38 (`da65cb8`); second review queued | reads `/Game/TriadDrift/Presentation/DA_<Ship>_Visuals` by path |
| Squad: missile-evade test; bark triggers; forward wedge; hyperdrive block text; director shots keep wingmen out of frame (`c77d246`) | T3 | done | Nov 12 13:38 | Nov 12 15:14 (`f31a1f1`); five theme captures retaken | none |
| ENG-13 enemy ships + AI, ENG-14 wave director | T4 | in review | Nov 12 | second review done Nov 12 15:31: ENG-14 acceptable with minors; ENG-13 not acceptable: fighters do not fire at a facing player, interceptor point-blank launch, gunship faces the lead point, pack followers escape the Gravity Well; one wave soft-lock order. Fixes in progress, then enemy placeholder visibility | none |
| ENG-27: headless balance run (scripted pilots, three skill levels), difficulty presets Story / Standard / Ace, first tuning pass (waves 1-5 in 8.4 min on Standard) | T4 | in review | Nov 12 13:36 | Nov 12 14:38 (`8c03f82`); numbers in `docs/eng/balance.md`; human play still needed | none |
| ENG-28: performance stress test `TDPerf.Stress.Wave5PlusDominion` (81 spacecraft + 120 projectiles: world tick 4.2-6.4 ms; live wave 5 frame 2.6-4.2 ms, Dominion 4.0-6.7 ms) | T4 | done | Nov 12 14:38 | Nov 12 15:14 (`f31a1f1`); within budget, no fixes needed; recheck in the packaged build | none |
| ENG-16 portal gates, ENG-17 galaxy layout, ENG-18 Dominion | T4 | in review | Nov 12 | all four review majors + minors merged Nov 12 13:38 (reactor mesh applied, victory is a phase change with the squad protected, no gate bounce-back, art path + editor preview, asteroid keep-out); second review queued | reads `/Game/TriadDrift/Presentation/DA_*` by path |
| ENG-19 game instance / saves / travel, C-03 #3 scenes and takes | T5 | in review | Nov 12 | `a2fdf3e`; all 12 review fixes merged Nov 12 12:45, this push (`125146f`); second review queued | none |
| ENG-21 first-person character, ENG-22 energy blade, ENG-24 objective framework | T5 | in review | Nov 12 | Nov 12 13:02, this push (`fc329d0`); not reviewed yet | none |
| Menu and transition game modes (`ATDMenuGameMode`, `ATDTransitionGameMode`) | T5 | done | Nov 12 | Nov 12 13:10, this push (`85af40b`) | `/Config/DefaultEngine.ini` map prefixes |
| ENG-21 to ENG-24 on-foot review fixes (objective soft-lock on retake, respawn point, encounter resets, blade on press, deflect aim) | T5 | in progress | Nov 12 13:58 | review done Nov 12 13:58: ENG-19 and Flow side of C-03 #3 acceptable; Ground items not done | none |
| ENG-23 crew enemies | T5 | in review | Nov 12 13:10 | Nov 12 13:24 (`0136518`); trooper cover merged 13:50 | none |
| ENG-25 Verdance gameplay, ENG-26 Cinder gameplay | T5 | in progress | Nov 12 13:24 | | places gameplay in `L_Planet_*` from C++ only; reads `/Game/TriadDrift/Presentation/DA_*` by path |
| ENG-29 docking, ENG-30 interior + core fight, ENG-31 escape (stretch) | unassigned | queued | | | only after core is solid |

### Coming next that touches shared things (heads-up for Nick's side)

- `/Config/DefaultEngine.ini` now boots into `L_Galaxy` and assigns game modes by map name
  (`L_Galaxy` space, `L_Planet*` and `L_Dominion*` ground). A menu game mode for `L_MainMenu`
  and the boot-map switch to it follow when the menu exists.
- `unrealGameJam.uproject` is unclaimed again. It now lists 21 engine toolset plugins (Editor targets only). Nick's `Plugins/TriadDriftHUD/**` row implies a plugin
  entry there and a module dependency. A plugin cannot include the game module's headers
  (`ITDHUDData`, `UTDEventBus`), so how the HUD reads gameplay data needs to be agreed between
  Ryan and Nick before that plugin is integrated.
