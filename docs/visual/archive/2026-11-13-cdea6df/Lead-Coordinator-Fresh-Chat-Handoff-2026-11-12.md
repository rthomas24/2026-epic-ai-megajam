# Lead Coordinator — fresh-chat handoff

Current actionable checkpoint: November 12, 2026, approximately 20:47 EST.
This transfers context, not production authority. **The continuous goal is PAUSED.**
Nick requests a readiness report and then a complete stop until his next authorization.
All four specialist chats were inspected through read-only app snapshots and were idle.
No agent was restarted, no heavy tool launched, and no asset changed for this handoff.

## 1. Project, authority and reading order

**Triad Drift**, Unreal Engine **5.9 MegaJam Preview, CL 60351404**, project
`unrealGameJam.uproject`. Submission deadline: **November 15, 2026, 8:59 PM Eastern**;
feature freeze November 15 at noon Eastern. Protect the playable three-ship switching,
combat and complete mission before optional details. Boarding finale alone is stretch.

Read the current remote versions of `AGENTS.md` (especially §§6.2–6.10 and 11),
`AAA.md`, `CLAIMS.md`, `MESSAGEBOARD.md`, `features.md`, then this handoff and
`docs/visual/STATUS.md`. Claims control ownership; features controls scope/contracts;
AAA controls quality gates. The board records questions/decisions, not file ownership.

Lead coordinates Nick's specialists, claims, serialized Git synchronization/publication,
PR reviews/integration, resource scheduling, engineering handoffs and honest reporting.
Do not become the asset developer or direct Ryan's workers. Nick delegates routine naming,
paths, sequencing and safe integration of already authorized work. Nick retains creative
direction, final subjective acceptance, major scope/architecture, risky operations and
changes to shared ownership. Ryan's Lead decides engineering matters under Ryan's delegation.

Every report begins **Agent: Lead Coordinator / Role: Visual-team coordination and
integration / Task: ...** and states branch/worktree, commit/push, verification limits,
blockers and approvals. Do not infer model, effort or usage from conversation labels.

## 2. Git checkpoint and integration disposition

Repository: https://github.com/rthomas24/2026-epic-ai-megajam (private).

- Asset integration checkpoint: `7c917f15275f2759414ada3c7912a3438fbbf55d`.
- Latest remote main verified before drafting: `6adcca7` (paused Lead transition claims).
  This document's publication and the subsequent N-019 board-only notification will advance
  main again. Resolve their full SHAs with Git; a document cannot contain its own commit SHA.
- [PR #9](https://github.com/rthomas24/2026-epic-ai-megajam/pull/9), targeted menu-only
  `nick/integrate-menu-theme-20261112@3d7a5f5a4b8ee144708d1d3099a524bf6384c503`,
  **merged** `fae4c15274d9dd4045a96f86b5d50aae720a5f6f`: 9 source/evidence files
  plus Music SoundWave/SoundClass, 11 files total. Rejected audio history excluded.
- [PR #10](https://github.com/rthomas24/2026-epic-ai-megajam/pull/10), original
  `nick/ships-lancer-m03@a19f4788e3b638e10353482b980db3b610329763`, **merged source-only**
  `98187b93c5f06df5175d710cd0912c145773600d`: 213 Lancer M03 source/review files.
  No export, import, runtime replacement or final-look approval.
- `nick/characters-lancer@8e5de0d0bf8e6b8a98ee350080ec35d1f2892f38`: **held; no PR**,
  explicitly agreed by Ryan in N-018, because Blockout03 fails six QA checks.
- GitHub rechecked: **zero open PRs**, #9/#10 closed and merged. Prior #5 rejected audio
  and #7 superseded violet Rift were closed unmerged; preserve branches/history. #8 HUD
  merged `07610fe069ca0cd8b592abff1a2e722e37581fa2`; #4 is its older merged foundation.
  Do not repeat the historical full-PR audit without a new reason.

N-018 contains explicit Ryan approval of the narrowed menu assets and source-only Lancer
exception; claims record it in `0d177af10cdb667eb32db66ae7da1d2494ddd54c`. Ryan required
no additional premerge engineering tests for those inactive/nonruntime scopes. Postmerge:
224 changed paths exactly inside approved roots, no gameplay Source/Config/maps/project
change, original worker heads and dirty sets unchanged. Prior lightweight verification:
22 Python ASTs, 141 JSONs; all 86 distinct LFS objects locally size/SHA verified and GitHub
download-metadata availability checked. This is integrity evidence, not gameplay acceptance.

## 3. Workspaces, agent inventory and preserved local changes

All worktrees below are under the absolute prefix
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/`.
There are **five role lanes but four specialist chats**: UI and Characters share one chat.
No separate Characters & Enemies chat was found; do not invent or automatically create one.

| Role / existing chat ID | Branch; worktree; head | Current state and next prerequisite (NOT a dispatch) |
|---|---|---|
| Lead Coordinator, old chat `01a1ccec-9bc6-78a3-bc56-cf175be639c3` | `nick/visual-assets`; `visual-assets`; this handoff publication | Documentation/integration coordinator only; stop after handoff. Reuse this tree after ensuring the old Lead has stopped; do not create a competing writer. |
| GameJam \| Ships & Weapons, `01a1d0b1-0631-7193-ba5d-984a972a6711` | `nick/ships-lancer-m03`; `ships-lancer-m03`; `a19f478` | Idle/frozen. Detail01 technically verified, final material hierarchy still MAJOR. `material04.py` plus modified `qa_m03.py` are local/uncommitted, never run. Needs Nick resume, source review and a fresh resource grant. |
| GameJam \| UI, UX & HUD, `01a1d0bb-2a88-7d63-9538-0992fc30b835`, currently acting as Characters & Enemies | `nick/characters-lancer`; `characters-lancer`; `8e5de0d` | Idle/frozen. Failed Blockout03 preserved; Revision04 region-fitting source corrected, not executed. Dirty `brief.md`; untracked `source/build_lancer_revision04.py`, `render_lancer_revision04.py`, `run_armor_revision04.ps1`. Needs renewed execution authorization; no rig/native/next-character approval. |
| Same UI chat, HUD role frozen | `nick/hud-lancer-polish`; `hud-lancer-polish`; `ca579ab7380f830647aa3d49e4f442f5cf3714a2` | Clean, PR8 already merged. Lead owns integration follow-through; do not assign simultaneous HUD and character edits to the same chat. Old `hud-native@2f55512` retains a staged CLAIMS.md change: preserve, do not apply blindly. |
| GameJam \| Environment & VFX, `01a1d0cc-6ad6-7553-b6fe-8d1e64dcd59d` | `nick/environment-vfx`; `environment-vfx`; `5fde8c35208075e62253d714f7c40c8dfc8e5003` | Idle/frozen. HeroM02 captured natively; controlled real-Galaxy test not run. Untracked `HeroM02/GameplayReadability01/` holds five prepared files. Reported material-object identity and live-PIE spawning source blockers fixed; source review passed, runtime behavior unverified. |
| GameJam \| Dedicated Audio & Music Agent, `01a1d0e9-aaa8-7f61-85c7-86b6fd3f1db7` | `nick/audio-prototype-v1`; `audio-prototype-v1`; `2fa17b39f664aca217a4eb4570cb94865af3b046` | Idle/frozen. Two native menu packages now main via PR9. Three untracked MenuTheme01 files: `MENU_INTEGRATION.md`, `check_menu_dependency.py`, `create_menu_style.py`. No style created; awaits compiled current-game host, source review and fresh native grant. |

Exclusive active ownership is exactly the current CLAIMS rows: Ships M03 production/review
roots; Characters Lancer/M01; Environment HeroM02 source/native roots; HUD plugin/source/UI
roots; Audio PrototypeV1, MenuTheme01 and exact `Content/TriadDrift/Presentation/DA_UI_Style.uasset`.
Lead alone updates claims. Broader reservations/future paths are not active edit grants.
Audio's style claim permits **only MenuMusic**, with default/null widget fields; later UI
style editing must be serialized and ownership coordinated first. No claimed paths overlap
between these active deliverables; UI/Characters are one owner with separate frozen scopes.

Other preserved trees: `day1-integration` / `nick/day1-integration@32582f8f37b2adb90c7c37b521bdca2187b5861b`
is clean, compiled before newer menu-flow changes, NOT a current-main binary host.
`characters-infantry@372a9ca74cd185c3e2f7f71d1a00567b38aab6b1` is frozen references/preparation
only; two untracked Manny export scripts remain. Legacy source trees stay frozen.
Original `C:/Users/nicho/OneDrive/Desktop/GameJam/2026-epic-ai-megajam` is an old main
checkout with a pre-existing local EngineAssociation change: never reset/sync it automatically.
The chat's Documents cwd is not the working repository; use explicit `workdir`.

## 4. Actual asset readiness and review evidence

| Asset | Verified stage | Outstanding gate |
|---|---|---|
| Blue Lancer M03 | A02 v03 structure approved by Nick; Detail01 `.blend`, 34,516 tris/6 materials, geometry/contact/36 frames/clearance pass; source on main via PR10 | Still too silver/lilac/uniform versus reference; final look unapproved. No current M03 FBX/native/runtime replacement. Material-only next pass prepared, not run. Preserve 12 base meshes/36 frames and legacy 33-frame mapping. |
| White/cyan player | Manny scale/reference exported and checked; actual original armor Blockout03 saved/rendered on held branch | Six QA failures (body breakthrough, crown coplanars, detached visor) and poor visual fidelity. Revision04 source only; no accepted armor, valid exported character rig, Unreal import or player attachment. Reference skeleton 88 bones plus root object is explicitly NOT export-ready. |
| Bastion / green Rift / enemies | Concepts, old source references and engineering placeholders exist | New redesign production queued after Lancer gates; not current finished models. Superseded violet Rift excluded. No new enemy art dispatch; Null Armada variants remain secondary. |
| Native HUD | PR8 main; current-runtime Editor build, 10 UI tests, 31.3s agent game/12 captures/0 errors; earlier real landing lifecycle evidence | Ryan reports Shipping native attachment/default-on (below). No human full journey/final readability acceptance; extra telemetry/layout fixes not dispatched. |
| HeroM02 sky | 22 files/15 LFS, six runtime packages on specialist branch; isolated native capture/reload passed; Nick approves look ONLY for controlled gameplay test | No actual Galaxy A/B, persistent map integration, cooked acceptance or performance claim. Textures about 61.44 MiB; scene-capture timings are not GPU FPS. VFX starter source kit has no validated Niagara/gameplay effects. |
| Selected menu song | 107s, stereo 48kHz/16-bit; native import/reload pass, correct Music class, 78 protected hashes unchanged; two assets now main | No DA_UI_Style on main, so inactive/not cooked through that reference. Needs exact style creation/reload, current-menu playback/travel checks, engineering Shipping cook/run and Nick in-game listening approval. Retro V1/M02 sound direction remains rejected; no library overhaul. |

Existing local review entry points (absolute; preserve, do not regenerate):

- Lancer: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/ships-lancer-m03/ArtSource/Space/Ships/Lancer/review/M03/Detail01/index.html` and production `Lancer_M03_Detail01_v01.blend`.
- Character actual failed checkpoint: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/characters-lancer/ArtSource/Characters/Lancer/M01/review/Blockout03/Lancer_Blockout03_beauty.png`; source `source/armor/Blockout03/Lancer_Armor_Blockout_v03.blend`.
- HUD: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/hud-lancer-polish/ArtSource/UI/TriadHUD/Unreal/review/M03ObjectiveFollowup/preview.html`.
- Hero: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/environment-vfx/ArtSource/Environment/SpaceFoundation/UnrealNative/HeroM02/review/compare.html`.
- Audio: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/MenuTheme01/export/MUS_Menu_EtherialPulse_01.wav` (ignored, local; not a new lossless recording, derived from the selected original MP3).

## 5. Engineering contracts and gameplay evidence

Ryan owns Source, Config, Gameplay content, gameplay actors, packaging and all shared
interface changes. Nick owns presentation instances/assets under published claims.
`features.md` §3 and `docs/visual/Presentation-Asset-Paths.md` define fixed flat DA paths.
`UTDShipVisuals.Mesh/Parts` supports modular ship visuals; do not change canonical attachment
transforms without coordination. Crew uses `UTDEnemyVisuals.SkeletalMesh/AnimBPClass`,
Stagger/Death; Manny A-pose, cm, +X, feet origin, about 180cm. The player body hook is separate.

Confirmed N-017 routing: Ryan Lead/Core owns HUD host and UTDUIStyle; T1 flight/input/travel,
T2 combat/powers and HUD combat data, T3 squad/provider data, T4 world/sky/enemies,
T5 flow/ground/objectives/maps. These are engineering lanes, not locally addressable chats.

Latest Ryan evidence (R-017, not a new Lead playtest): Shipping `Builds/ship_2100`, Windows,
808 MB, cook 0 errors, windowed 45 seconds/no input, native HUD live in wave1 plus fallback
wing arrow/hit chevron. Default enabled in `e4e17f8`; actual remote Config confirms
`bUseNativeSpaceHUD=True` and boot map `L_MainMenu`. Screenshot paths:
`docs/eng/evidence/shipping_native_hud_ship_2100.png`, `shipping_menu_ship_2120.png`.
Latest recorded engineering full suite: 503 passing tests twice at `c8128d1`, before later
metadata/assets; not rerun here. Engineering artifact lives on Ryan's machine, not assumed local.
No human full journey or current final-submission acceptance is established. The failed
earlier no-HUD package was fixed; do not keep reporting it as the current state.

R-017 still OPEN; N-015 ANSWERED carries continuing implementation. Next authorized Lead
should acknowledge Shipping evidence without duplicating tests, check the promised removal
of duplicate interaction prompt, and coordinate remaining HUD extras. `GetInteractPrompt`
is already bound in PR8; missing native surfaces use engineering fallback. Native coverage
flags `bCovers<Surface>` prevent duplicates as future fields are adopted. Damage/squad
markers, hyperdrive reason, altitude, dodge, bark subtitles and pause still need bounded
UI decisions; avoid simultaneous character/HUD dispatch. Ryan noted close target text
occlusion, missile banner overlap, missing auto-switch card and wipe respawn-zero display.

## 6. Communication, skills, safety and approvals

App read_thread snapshots work for the listed IDs. Earlier send_message_to_thread exchanges
with Nick specialists were tested; discover tool schema and verify availability in the new
chat. Sending to an idle chat may start work: do not send a resume disguised as a status check.
No direct remote Claude chat transport is verified. **Lead-only MESSAGEBOARD.md is the tested
cross-team channel**; round-trip replies N-017/N-018 prove it. Workers hand questions to Lead.
If app messaging is unavailable, use existing Git handoffs and give Nick exact relay text.

Board: read on every fetch/task/commit/push and every 30 minutes while actually working.
Use `rg -n '^### .* (OPEN|ANSWERED|INFO)' MESSAGEBOARD.md`. Answer R-xxx addressed to Nick,
act on ANSWERED N-xxx, preserve words/history; append Reply with identity/Eastern time.
Do not answer Needs: Nick creative decisions yourself. Status changes are allowed; new N IDs
are sequential after latest remote. Commit board alone and push promptly; no force. Copy
decisions into the appropriate claims/area plan. Current handoff notification is N-019.

Read applicable repository skill before work: `.claude/skills/blender-mcp/SKILL.md`,
`blender-game-asset`, `blender-texturing`, `blender-unreal-export`, plus available native
Blender QA/export/rigging skills as applicable. Audio skill now EXISTS on main at
`.claude/skills/ue-game-audio/SKILL.md`; it is no longer unpublished. It covers licensing,
layered takes, ffmpeg levels/mix, Unreal classes/concurrency/attenuation and human ear approval.
Full fresh-chat/specialist adoption and ignored skill-mirror freshness are **unverified**;
read source skill rather than assume discovery. Host Blender actually used is 5.1; repo
skill examples mention 5.2. Preserve tested sources and verify API compatibility, never upgrade
or rebuild blindly. Engine agent skills live in `Engine/Extras/AgentTools/Skills/`.

AAA is an aspirational standard within jam scope: implement → verify → refine → retest →
human review → integrate. Technical, presentation, performance and gameplay gates differ.
No automated pass or attractive render establishes final AAA quality. Original colorful
sci-fi identity remains; existing squad magenta/cyan/amber coding is unchanged. Requested
green Rift redesign does not authorize changing runtime color/interface contracts.

One heavy job at a time on this **16GB machine**, fresh free-RAM/process check before each
grant, bounded timeout/own-process monitoring, checkpoints first. Last lightweight check:
5.42 GiB free, no Blender/UnrealEditor/UBT process; this is not a reusable grant. Default
headless/background/CLI; GUI only for required inspection/interaction/gameplay. Human MCP
port 8000 is protected; Nick agent ports 8020–8029, serialized per editor. No needless build
for documentation or unchanged inactive source. Real-game/package testing remains necessary
when presentation is actually activated, with separately coordinated resources.

Git: preserve dirty files listed above; fetch/status/branch/incoming commits/path comparison
and remote claims/board before task/commit/push. Each worker has an isolated nick/* branch;
Lead alone serializes sync/integration. Shared AGENTS §6.9 is stricter than old local wording:
workers may edit/commit only through Lead-coordinated preflight; **no independent pull/rebase,
merge/push/switch/worktree operations**. Do not silently weaken this to resolve a policy
discrepancy. No force, destructive reset/restore/cleanup, binary conflict choice, pushed-history
rewrite, branch deletion or automatic stash. Never commit EngineAssociation/caches/builds.
Git LFS must be hydrated/verified for actual asset use; pointer text is not an asset.

Day1 exception in `Day1-Integration-Policy-2026-11-12.md` expires **Nov13 00:00 Eastern**.
No extension. Thereafter approval-based end-of-day integration unless Nick/Ryan explicitly
renew. Scope-specific N-018 source-only permission is not general runtime promotion.
GitHub connector access was unreliable/404; authenticated local Git + REST worked. Do not
claim GitHub MCP connected without a successful authorized read. Never print credentials.

## 7. Critical path and pending human decisions

After Nick authorizes a next milestone, prefer one tangible result per worker:

1. Protect playable submission: Ryan's real-input full journey, planet rewards/return,
   finale/retake/Continue, current Shipping package and human feel/readability check.
2. Finish approved Lancer foundation's material hierarchy, actual multiview Nick review,
   then separately authorize bake/export/import/DA binding/gameplay checks. Main source
   availability is not a working replacement. White pilot Revision04 QA/visual checkpoint
   follows in the same serialized resource schedule; no promise of tonight's full target.
3. Small near-ready gameplay improvements: exact menu style/music activation and Shipping
   listening acceptance; controlled Hero real-Galaxy readability test; HUD functional gaps
   after a safe character checkpoint. Bastion/Rift/enemy variants must not displace these.

Nick decisions: final Lancer materials/look; corrected white pilot appearance; player
body/shadow versus first-person arms (Ryan holds new hook pending answer); subjective
in-game music acceptance; any major redesign/scope/shared-ownership change.
Ryan confirmations: current uninterrupted full journey/package evidence, pilot actor/hook
after Nick decides, promised interaction fallback fix, current-game host for music creation
and subsequent cook/playback. No unresolved PR merge request remains.

## 8. Reconciliation and efficient operation

This handoff supersedes old **current-state summaries**, not historical evidence or shared
rules. Recovery report and Environment-VFX handoff remain historical; they incorrectly
describe several PRs as open, no character geometry and unpublished Hero assets at their
old timestamps. STATUS current rows and CLAIMS top banner have been reconciled in this
publication. Old timestamped task boards remain history. No production dispatch survives
the explicit present pause. The shared instruction to keep lanes productive does not
override Nick's stop/readiness-only transition.

Do not audit all PRs repeatedly, poll idle chats, rebuild unchanged content, make a dashboard,
add more infrastructure or launch tools to prove a source-only merge again. Use one bounded
task, existing evidence and meaningful milestones; report exact blockers promptly. Preserve
all four specialists' safe checkpoints. Recheck only what changed or is required for the
next explicitly authorized acceptance gate. No new /goal or automatic continuation.

## 9. Ready-to-paste startup prompt

```text
You are Triad Drift's new Lead Coordinator for Nick. This is a context/readiness transition,
not authorization to resume production or a continuous /goal.

Use C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/visual-assets on nick/visual-assets.
Read docs/visual/Lead-Coordinator-Fresh-Chat-Handoff-2026-11-12.md, then latest AGENTS.md,
AAA.md, CLAIMS.md, MESSAGEBOARD.md, features.md and docs/visual/STATUS.md. Fetch safely,
inspect status/current branch/remote main and incoming paths; preserve all dirty worktrees.
Verify PR9/10 merged and current PR state once. Restore the existing four specialist chat
IDs/five role lanes and engineering routing; do not create or restart chats, goals or jobs.
Read board replies and identify pending actions, but don't dispatch production or perform
new merges/asset edits. Respect Lead-only Git/board coordination, shared ownership, AAA
gates, 16GB headless-first resource scheduling and the Day1 exception's midnight expiry.
Don't repeat completed audits. Report readiness, current main SHA, relevant new changes,
precise blockers/decisions, then WAIT for Nick's next authorization. Begin with Agent:
Lead Coordinator / Role: Visual-team coordination and integration / Task: Restore readiness.
```
