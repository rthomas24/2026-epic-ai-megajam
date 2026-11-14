# Triad Drift dev log

## 2026-11-14 - Nicholas Lead team attribution text

Ryan Lead already corrected DA_Credits in94ea8edc under Ryan's instruction. Preserved that binary; synchronized the source TEAM Ryan name/role and role-detail line to its stored strings, added both full team roles to the itch draft, and specified final-video attribution. JSON parsing and exact diff checks passed. Stored-string comparison is not an Unreal reload or packaged-roll verification; those remain required. Historical survey-credits-reload.json is unchanged. No native process or video render was run, and specialists remain paused. The former #181 returned410 (deleted); ongoing submission coordination remains #137.

## 2026-11-14 - Nicholas Lead historical credits reconciliation (#137)

Applied the existing Auditor/Issue Coordinator corrections to Nicholas's preparation entries: scoped historical Space Foundation and audition status; distinguished the unused standalone combat asset from tutorial/credits composition uses; restored precise tutorial attribution; retained Infinity Zone's reactor placement and original source/tool history. Added a separately qualified owner-reported Shipping f40a39c3 inventory note ([source](https://github.com/rthomas24/2026-epic-ai-megajam/issues/137#issuecomment-6882860191)), limited to its four folders. Ryan engineering/source entries and the prior menu correction remain intact. Reviewed the exact patch, current-main delta, source-receipt references and owner inventory; patch applicability and whitespace checks passed. No native/build work was needed for this documentation-only change. No final inclusion, rights expansion, playback, listening or appearance acceptance. #137 remains open for final submission evidence; #181 asset/name work is separate. Existing specialists remain paused.


## 2026-11-14 - Nicholas Lead offscreen playtest and feedback batches

Played editor-game baseline **8b4eabae**, 1280x720, private user/save directory, engine MCP real input with steering assistance. Fresh start: intro hold-skip, tutorial phases, firing, ship switching/power, Pause/Settings, withdrawal/Aftermath, hold-H hyperdrive and Verdance entry. Continue restored the isolated checkpoint; hold-F landing and on-foot walking worked. No teleports, kill cheats or progression skips were used to reach Verdance. The final subtitle test deliberately staged C4_Command_Proceed with Story.Fire/Reset, not a natural departure playthrough.

Observed: story line clipped at both viewport edges and Small/Large did not change this overlay; Pause labels showed through Settings in flight and on foot. Additional captured observations: transponder edge clipping and fragmented-looking canopy undersides. Generic power hint needs clearer wording (suggestion). Known missing ground waypoint and already-fixed ability hint issues were excluded as new tickets. No subjective audio/performance or full campaign pass claimed. Source comparison through 9155a6dc supports triage only; newer main still needs play.

[Batch 2 PDF and original media](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/evidence-agent-feedback-batch2-20261114), [owner handoff #90](https://github.com/rthomas24/2026-epic-ai-megajam/issues/90#issuecomment-6879961986). [Batch 1 human menu/intro report](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/evidence-manual-feedback-batch1-20261114) was also sent and acknowledged. No code/assets changed. Session 1 hit its configured 960-second watchdog bound (no game-thread stall reported); session 2 exited cleanly with code 0. Original human save hashes unchanged; no Unreal processes left. Existing specialists remain paused.


## WAKE-UP REPORT for Ryan: overnight Nov 13-14 (Ryan's Lead, written 08:50 EST Nov 14)

**Update 10:00:** main is batch Z (`80a2ee4d`, gate 1005/1005) plus documents; newest package `playable-main-80a2ee4d`. Ryan's new bar (09:15): agent-played and judged good = done. Played and good since the report below: the Warden fight, the reactor conduits and heart, the escape run and flight, victory, stinger and credits music (T7); landing and take-off on both planets and drift (T1); the reward screen, portal blaster, ram, EMP Pulse, shield dome, Gravity Well and boost pickup (T2). Open from play: no on-foot marker on the "head to" legs, the Cloak effect, the hangar start, the ability hint card, enemies left after the space-reactor victory. PRs: #78 and #93 merged, #83 and #12 closed, #11 and #85 wait on Nicholas until 10:30.

**Update 09:15:** main is now batch Y (`8b4eabae`, gate 999/999); newest package `playable-main-8b4eabae` (zip `TriadDrift_8b4eabae_Shipping.zip` in the for-ryan builds folder, 877 MB, cook 0 errors 0 warnings, checked in Shipping to wave 1 only). Batch Z is gating. PR #78 (Wren take 04, chosen by Nicholas) needs Ryan's merge click.

**Main:** `c8f78e1e` (batch X, gate 991/991). Batch Y is gating (`lead/int-y` 305453c7).
**Package:** private prerelease `playable-main-6cc4f254` (batch W), `Builds\for-ryan\TriadDrift_6cc4f254_Shipping.zip`, 876 MB, cook 0 errors 0 warnings, six maps. Checked in Shipping only to the start of wave 1; audio, flight input and everything later are NOT verified in the package. A package of batch Y follows when it passes.

**On main and played by an agent (never by a person):** opening, hold-to-skip, tutorial, Aftermath hold when boosting into Verdance, natural Verdance entry and glide, Verdance on foot through orbit, Continue after relaunch, 13 mid-action ship switches (all instant, worst frame 15 ms), the Dominion assault, docking with all three ships, the interior from the hangar to the Warden.
**On main, tests only:** Cinder magma route over the new bridge, wreck-core trim, six travel barks, world and on-foot barks, subtitles setting, interception/docking/assault story lines, journey keep-outs in the space field.
**Not played by anyone:** reactor conduits and heart, escape run and flight, victory, credits, the space-reactor fallback, Cinder after the magma fix.

**Not finished, and why:**
- Interior (ENG-30) has bugs: dying in the Warden fight skipped the Warden; the Heavy's guard never broke; the arena had no mercy tiers (17-38 takes for the scripted player). Fixes are in batch Y, tests only.
- Docking crawled (38-44 s); fixed and re-flown by T1 at 12-13.5 s; in batch Y.
- Effects not accepted: enemy engine plumes, region beacon, region haze, tractor beam (do not read at distance). Their slots are empty or unchanged.
- Verdance wreck scar reads from above but weakly from the landing approach.
- Lancer is nearly lost against the cyan part of the nebula (Yaw 166, Pitch -10); a contrast fix is with T1.
- The ship builder agent hit the model usage limit at about 07:00 and stopped; the Lead session was also stalled from about 07:00 to 08:30.
- features.md: item headings 0 todo / 45 in progress / 24 built / 2 done / 1 bugs; checklist 36 in progress, 1 bugs. Nothing is "done" without your play.

**Decisions made for you (overrule any):** ENG-38 smallest complete glide (8-10 s); cloud deck fitted to the glide corridor; opening rebuilt from the approved O_* lines behind `td.Intro.Lines`; Warden shield re-rigged to the forearm; BP_Wreck stays cooked; never-cook rules applied; no package from batches S, T, V, X; Shipping compile checked per batch; corridor keeps the Verdance bearing with a jump-point waypoint; wreck-core fight trimmed (ambush two Grunts, site Grunt + Trooper, Heavy moved to the relay, mercy from take 2); crew bodies stay 30 s; geyser row moved off the bridge exit; Heavy guard opens after 6 quick slashes (was 9); docking targets (full speed to 600 m, about 12 s); one shared rendering slot outside T7; declined OS-level input injection on your desktop; T6's branch re-cut without unaccepted effects; shard fine-grid mesh change not requested.
**Held for you:** DirectX 11 shader set removal (36fd0fac); `MI_TD_DominionGlowRed` 20 to about 6 (truer red, dimmer docking cue); Bastion cockpit design; pilot foot IK; precompiled PSO cache; T3's stash "t3-failed-merge-lead-int-u-0921"; the ENG-38 left-for-Ryan list; whether a boosting player should be made to use the one hyperdrive jump into Verdance; the ENG-47 decision due 20:00.

**Needs your eyes first:** (1) play the package past wave 1 with real controls and sound; (2) one real click on CONTINUE; (3) the interior from the hangar to the Warden once batch Y is packaged; (4) the Cinder bridge crossing; (5) docking feel.
**Nick's side:** Nicholas came back online at 08:18; his four new board messages are answered (Director card evidence to T3, the "Thirty seconds. Keep going." line to T5; the victory eyebrow is confirmed by him). Still waiting on Nicholas: Wren take (PR #78), music ear approvals and the escape track, closing #49 and the eleven asset issues, words and voices for placeholder lines.
Trackers: bugs #90, performance #89.


## 2026-11-12 — Parallel presentation source milestone (Nicholas / Codex Lead)

Prompt: develop independent environment, modular VFX, three-ship HUD, and audio plans while protecting Ryan's engineering work. Four specialized native agents worked in isolated branches. Nicholas selected source-files-only and no merge; claims remain in PR #1 pending approval.

Produced deterministic sky authoring/material builders, four procedural VFX shaders, an interactive HUD with twelve SVG icons, and a 32-cue audio manifest with original Web Audio auditions. No gameplay or shared maps edited. Browser review checked visual composition and HUD switching; responsive width fixes followed review. Python/JSON/XML/JavaScript and numerical/mock-state checks passed. Native assets, Unreal shader compilation, in-game performance and final audio listening remain unverified. No Unreal or Blender process was launched. Candidate audio licenses were reviewed but no recordings were downloaded.

Coordinated the separately assigned Ship & Weapons chat in an exclusive source-only Lancer lane. Lead published its source branch without native tool execution; provisional source geometry and attachment checks are recorded with the design proxy. This milestone does not approve final hero art.

## 2026-11-12 — Cinematic switch audio candidates, awaiting audition

Nicholas rejected the retro browser-tone direction. Lead transferred the narrow
SwitchM02 claim to the dedicated Audio & Music chat in main24d5ddd. The worker
produced three actual48kHz/24bit/stereo WAVs, offline original layered DSP, local
audio-player audition page and provenance/decoded QA. Lead independently rechecked
PCM headers/bytes/hashes, clipping, durations, source syntax/JSON/JS/local links
and LFS pointers; published exact11files on nick/audio-music5979cff / draft PR5.
No third-party audio/spend/shared gameplay/config/native import. Manual listening
and Unreal validation remain pending; no media merged to main or quality acceptance
asserted. Broader audio/music work waits for Nicholas's audition approval.

## 2026-11-12 — Day1 isolated native sky integration

Lead audited/published Environment ec3e47f/c9beebc, then integrated its37owned
additions as9c629e8 under the temporary Day1 authorization. Source6files unchanged;
worker isolated UE build/reload9assets reports0errors/warnings; Lead checked12native
headers/hashes,5capture hashes/images,7PNG CRCs,JSON/AST/launcher syntax and19LFS
pointers. Detected stale bytes after PNG IEND, fixed only trailing container data,
pixel payload unchanged. Source/Config/sharedmaps/Plugins/uproject exactly preserved.
Sky remains an independently reusable foundation; no gameplay placement/PIE/package
or GPU frame-time claim. Pole star stretching remains a documented review limitation.
HUD activation and unapproved Lancer/M02audio remain withheld. M03audio claim
published before edits; legacy dirty project association and HUD stagedclaims preserved.

## 2026-11-12 14:15 EST — Engineering playtest 1 (Ryan / Lead, Claude Code)

**Build played:** `main` at `5ca0ae6` (300 headless tests passing), on `L_Galaxy`.
**How:** an agent driving the running game through Unreal 5.9's built-in MCP server on a private
port (`Tools/td-play-mcp.ps1`, `Tools/td_mcp.py`): viewport captures, frame timings, console
cheats. No input injection yet (the engine's input toolset could not find our player in a `-game`
session), so nothing was flown by hand: this is a look, not a full playtest. Also the first
packaged-build smoke test: a Development package booted, ran 16 s and exited cleanly.

**Worked:** boots straight into the galaxy; the Rift reads as a magenta ship with a glowing
engine; the dev HUD shows the three squad cards and the wave counter; wingmen clear wave 1 on
their own and wave 2 starts; `TDGod`, `TDSkipWave` and `TDTake` (CUT, take 2) work; about 4 ms a
frame with 17 enemies; the packaged build renders the same scene. Shipping package is 750 MB
including a 239 MB symbol file.

**Broken or wrong (each sent to its owner):**
- An engine warning sits on screen: two directional lights compete (fix already on a branch).
- `TDPhase DominionAssault` changes the phase but the world does not follow: waves carry on and
  the Dominion never appears, so the finale cannot be reached by cheat.
- The two wingmen are never in frame from the chase camera. The player cannot see the wing they lead.
- With no input the ship flies off and every enemy ends up as an edge arrow; nothing brings the
  fight back in front of the player.
- The Development game target did not compile (an `INFINITY` macro in a test); fixed. Only
  packaging caught it.
- Packaged log is noisy with "package does not exist" warnings for art that is not authored yet;
  a single quiet loader is now in Core for every area to use.

**Captures:** `Saved/TDShots/playtest1/` and `Saved/TDShots/packaged1/` (local, not in git).
**Stage, in AAA.md terms:** technically verified and integrated. Not gameplay validated.

## 2026-11-12 14:32 EST — Nick Lead integration check

PR4 head2f55512 merged as8963477, only37UI-owned files. Current-main real-project
UE5.9 Editor build passed, one compiler action at a time;326automationtests passed,
0failed/engineerrors/suppressed dialogs. Expected automation forced exit-request0;
9123Ground CharacterMovement tick-cycle warnings sent to engineering in N005.

Played revision8963477 (board-only cabcf77 has same runtime tree): default Galaxy
windowed scripted-capture minimum on private8020,3captures at3/10/20seconds,
28seconds play/43.8seconds process, exit0, reasonok,0errors/dialogs. Viewed all3PNG:
Rift magenta placeholder, squad-color debugHUD, moving flight and enemy combat
visible; shields change. This is not injected-input or human gameplay validation.
NewHUD is correctly NOT attached before engineering's guarded host; native sky
not placed. No proof of boost/switch/abilities/retake/portal/ground/finale or frame
budget. Timed cheat commands were requested, their effects not independently proven.
Warnings: missing WorldPartitionHLODUtilities classes on supplied Galaxy HLODlayers
(3Linker),23EditorDataStorageUI,1MCPlicense notice; no crash. Engineering receives
these for owner triage. No Source/config/maps/project changes made by Nick.
Captures/results/logs: Saved/LeadIntegration/ (local ignored). PR3/PR5 held for
subjective approval/rejected direction; M02Lancer source4b7ef12 safely branch-published.

## 2026-11-12 14:51 EST — Full PR audit and approved source integration

Live GitHub audit all six PRs:1closed/superseded;2merged957b45e;3approvedLancer
M02sourcefoundation merged100f21c7d973928ca86a3d406c3235dd2fd99a73;4merged8963477;
5rejectedarchiveheld;6closed-as-exact-cherry-pick9c629e8 (not GitHub-merged).
PR6all37 and PR4all37 files match main exactly; PR2source40files unchanged, credits/
log later appended. All258PR-path checks,74LFS pointer/object hashes,51PNG CRC/
exactends,77JSON and21Python syntax checks passed;14native package headers and
7WAV headers valid. NoCI checks configured, no new heavyweight verification.

Fresh independent Lancer review and postmerge check PASS:111additions only in
ArtSource/Space/Ships/Lancer, byte-identical4b7ef12, entire runtime/sharedproject
tree unchanged from immediate premerge main8c5aa47. Source-only no-runtime change
does not justify a new heavy boot during UI verification. Latest Lead actual-game
coverage remains prior8963477 scripted captures, not latest engineering gameplay
or attachedHUD. N007 requests RyanT2host handoff; Source/UI host not yet on fetched
main100f21c. No new native asset, activation, map or configuration changes by Nick.

Direct Nicholas approval: LancerM02 refined production FOUNDATION only, recorded
in Ship-chat task01a1d145-290e-7740-87fc-532c9d76e0da and latestfull-PRauditauthority;
finalAAA polish/Unrealintegration remain unapproved. Historical failedM01/rounds
retained unchanged; currentproduction M02 technicalgates pass. N008 announces plan.

## 2026-11-12 14:54 EST — Engineering playtest 2 (Ryan / Lead, Claude Code)

**Build played:** `main` at `b8027b7` (347 headless tests passing).
**How:** an agent flying the running game through Unreal's MCP server with real injected input
(`TD.Input.Keys W+LeftShift`, mouse, fire, Tab, Q) and the new `TD.Stage <stage>` journey jumps.
Still not a human at the controls.

**Worked:** boost (110 m/s, meter drains), mouse turn, firing (heat rises, hits land), Tab switch
Rift to Lancer with held input carrying over, Lancer's ram on Q (335 m/s), the take slate
("SCENE Wave 2 · TAKE 1") when a wave starts, wave 5 running with 26 enemies, enemies now red and
orange with a glow instead of black, a real level change to `L_Planet_Verdance` arriving in flight
and then on foot with the blade and the ground HUD. 6 to 8 ms a frame throughout.

**Broken or wrong (each sent to its owner):**
- Projectiles are two or three pixels: weapon fire is nearly invisible.
- After a turn the ship stays banked about 30 degrees with no input.
- The Dominion stage starts with the mothership out of view.
- On foot, the placeholder blade covers a quarter of the screen.
- Stars smear into dashes while turning.
- The planet surface is a featureless plain until the planet content lands.

**Also from teammates' own live sessions today:** the director camera lost a fast ram (now a
tracking shot); a far generator kill framed empty space; enemies rendered black; the dome and
well lattices rendered grey; the ram's streak effects leaked. All fixed with tests. None of these
was visible to the automation suite.

**Captures:** `Saved/TDShots/playtest2/` (local, not in git).
**Stage, in AAA.md terms:** flight, switching, the ram, waves, the journey jumps and the planet
hand-over are gameplay validated by an agent. Nothing is validated by a human yet.

## 2026-11-12 15:33 EST — Engineering playtest 3 and second reviews (Ryan / Lead, Claude Code)

**Build played:** integration head `8a6da52` (now on `main` at `f2db3ce`, 369 headless tests passing).
**How:** an agent flying the running game through Unreal's MCP server (port 8010, 1280x720) with real
injected input, the real HUD switched on (`TD.UI.NativeHUD 1`), and the `TD.Stage` journey jumps.
Not a human at the controls.

**Worked:**
- Nick's HUD, hosted by engineering, live in the real game for the first time: squad cards, heat,
  ammo, signature power, wave and hostile count, target bracket with distance and LOCKED, the
  incoming-missile warning, edge arrows. All three abilities read LOCKED on a fresh run (the fix for
  his reviewers' finding).
- Weapon fire is now visible: Lancer's cyan stream and the enemies' orange bolts read clearly
  (playtest 2's top problem).
- A Tab switch cuts instantly and the turn settles wings-level. No star smear.
- `TD.Stage DominionAssault` now starts facing the Dominion. Frame time 3.8 ms in wave 5.

**Broken or wrong (each sent to its owner):**
- Half the sky has no stars at all: the placeholder stars are lit spheres, dark on their far side.
- Enemy ships are specks: a locked hostile at 111 m is about 10 pixels and dark.
- Bastion's muzzle flash is a white ball the size of the ship; its shells are a single small dot.
- Rift's portal placeholder is a solid pink disc that hides everything ahead.
- With the real HUD on: two name cards on a switch, the scene slate behind the objective block, the
  objective reads "AWAITING MISSION DATA", and the wave block reads "WAVE 05 / 00" during the assault.

**What the tests could not see, again:** a full suite run crashed once in a Powers test. Cause: the
test made data assets with nothing holding them, so a garbage collection in the middle freed them.
The owner reproduced it by forcing a collection, fixed it, and left the forced collection in the
test. The game itself holds those assets properly.

**Second independent reviews (fresh agents, code only):**
- Combat: muzzle and impact placeholders would vanish the moment a sound-only cue is assigned;
  Bastion's missile locks never expire, so a later swarm flies at old targets; the space HUD had no
  objective direction. Fixes in progress; the missile-lock rule is now written into features.md.
- Enemies and waves: fighters do not fire at a player who faces them, interceptors launch at
  point-blank range on a head-on pass, gunships face a point ahead of the player and never start
  their volley against a flanking ship, and pack followers escape the Gravity Well. One order of
  events (last enemy dies during the "CUT!" delay) can leave a wave empty. All with the owner.
  None of this failed a test: the tests never asserted that the fighter actually fired.

**New on `main` today for presentation:** a live game-state object for music and mixes
(`UTDAudioStateSubsystem`: scene, phase, speed, hull, wave numbers, slow motion, and smoothed
`MusicIntensity01`, `Combat01`, `Danger01`), the audio skill (`.claude/skills/ue-game-audio/`), and
the objective-marker tag (`TD.Objective.Marker`).

**Captures:** `Saved/TDShots/mcp/pt3_*` (local, not in git).
**Stage, in AAA.md terms:** the HUD host and the visible projectiles are technically verified,
integrated and played by an agent. ENG-05, ENG-06 and ENG-13 are back to "in review" with open
MAJOR findings. Nothing is validated by a human yet.

## November 12, 2026, 16:07 EST — Lead actual HUD checks and production handoffs

Lead played pinned `0543dbd` in a real windowed Galaxy through MCP input, not preview data. Editor build and 7 UI/host tests passed. Successful live02 driver/result exit 0, reason ok, 30.1 seconds, no engine errors: ten viewport captures cover all three ship switches, boost/speed, fire heat, earned/charging ability, force off/on, on-foot removal and space return. Saved Blueprint class and squad-manager provider were confirmed. Health/shields remained full; damage/regen, target lock, hyperdrive and the whole journey were not exercised. First quiet-game attempt hit the log-silence watchdog and remains preserved, not erased. Evidence is local ignored `Worktrees/day1-integration/Saved/LeadIntegration/HUDHostValidation/`; tested revision predates newer objective/name-card fixes, which are not claimed replayed here. Human gameplay/Shipping acceptance remains open.

Dedicated HUD M03 checkpoint `b21a98f` is pushed: 26 owned paths, 16 actual-game captures, successful build and 8 UI tests; independent fresh source/viewport review found no BLOCKER/MAJOR. Nick approves current prototype styling only. Lead synchronized its clean branch to current gameplay at `a4b91fb` and dispatched optional published `GetObjectiveText()` consumption and late-attach regression before final focused native verification. No shared configuration activation or gameplay edit by Nick's agents.

Audio skill `4096f47` is published and adopted by Audio & Music. Lead inspected its full skill/relevant references and research, memory-compiled all eight scripts, and ran `audio_report.py --targets`. ffmpeg/ffprobe are not on the Lead shell PATH; Audio uses the existing Shotcut binaries, no install. Scratch-project research is distinct from actual-project verification. Audio PrototypeV1's source QA and both private-host builds pass; native assets/playback are still pending. Root granted only bounded NullRHI author/check after fresh resource guard, no parallel heavy application or licensed media in Git. No paid/AI acquisition, map placement or audio-director integration is authorized.

Ships' latest direct Nick instruction holds all spacecraft redesign until three new references and an approved revised plan. Lancer `4b7ef12`/main `100f21c` and Rift `a7c0463`/draft PR7 stay preserved technical references, not final accepted designs. No rollback, remodeling or import. Lancer's exact source/LFS availability was independently verified and handed to Ryan in N-009 (`2310d19`); no FBX/glTF/native export exists. Provisional new accent colors do not alter gameplay contracts.

## 2026-11-12 16:31 EST — Engineering playtest 4, more reviews, and engineering starts making small props (Ryan / Lead, Claude Code)

**Build played:** integration head on top of `main` `c7a909b` (389 headless tests at the time; `main` is now past 416).
**How:** an agent flying the running game through Unreal's MCP server (port 8010, 1280x720), real injected input,
Nick's HUD switched on, `TD.Stage` journey jumps. Also a packaged Development build run from the command line.
Not a human at the controls.

**Worked:**
- A Tab switch with the real HUD on shows only the HUD's "CUT TO / BASTION" card: our fallback name card steps aside.
- The HUD draws an objective marker with distance, and on the planet approach the objective line read
  "LAND AT THE BEACON" with the pad marked "OBJECTIVE 130 m".
- The new landing assist: hold F from arrival, the ship flies itself to the pad, lands, and the pilot is on foot
  with the blade. One motion, no crash.
- The packaged build hosts Nick's widget Blueprint from the cook, runs wave 5 and quits cleanly.

**Broken or wrong (each sent to its owner):**
- The new unlit stars were almost invisible. Cause: the engine's glow material is not allowed on instanced meshes, so
  the game fell back to the grey default material (a log warning said so). In a packaged game it would be worse.
  Fix, ours: one project-owned glow material (`M_TD_PlaceholderGlow`) that every placeholder now shares, with a test
  that checks it is unlit, has the colour parameter and works on instanced meshes.
- The HUD objective line still read "AWAITING MISSION DATA" in space when the HUD was switched on after the wave
  started; fixed since by a getter and a re-broadcast.
- The planet was an empty plain with nothing to do. Fixed the same hour: both planets now have two finishable
  objectives each (tests drive the real landing, every step, the rewards and the way back to space).

**Reviews, round two (fresh agents that did not write the code):**
- Squad: after a wipe and retake, the ship you had been flying never flew again under AI once you switched away.
  A blocker the suite never saw, because the test switched ships but never looked at the old one. Fixed with a test
  that fails without the fix, then seen live.
- Travel: nothing in the game could complete a planet objective, so a run ended at Verdance; and dying once in the
  finale let the next take jump away from it. Both fixed.
- Powers: the ram mostly does no damage to carrier bays, turrets and shield generators at 30 and 60 frames a second,
  because it stops on them before its damage sweep. The existing test applied the damage by hand and never dashed.
- Ground: the hidden pilot can still be killed after boarding; a trooper can pin itself against its own cover and
  never fire; there is no pause on foot.
- One lesson repeated in every review: a test must make the real thing happen (a real dash, a real shot, a real
  switch) and then assert the result, at 30, 60 and 120 frames a second.

**A test that told us something by failing:** at one intermediate merge "the fighter fires head-on and hits" failed
three times out of three at 60 frames a second, then passed again one merge later, with no change to the fighter.
We did not shrug and re-run: it means head-on hits sit on a knife edge, so the owner is measuring the miss margin.

**New way of working, from Ryan:** engineering agents now build the small environment pieces the art side has not
started (asteroids, rocks, crystals, mushrooms, ferns, and trees through Unreal's Procedural Vegetation Editor),
announce it on the message board first so it comes off the art side's list, and keep it in engineering's own folders.
Art direction stays with Nicholas, who can restyle or replace any of it. Three agents are building now.

**Captures:** `Saved/TDShots/mcp/pt4_*`, `Saved/TDShots/pkg_1538/` (local, not in git).
**Stage, in AAA.md terms:** landing assist, HUD hosting and the cut presentation are gameplay validated by an agent.
Both planets are technically verified and not yet played. Nothing is validated by a human yet.

### Nov 12, 19:45 EST: first run of a Shipping package found it had no HUD (engineering Lead)
- **Played:** `Builds/ship_1945` then `ship_2005` (Shipping, 778 MB), launched windowed with no input for 40 s each, window captured from outside the process. Not a human session; nobody touched the controls.
- **Found:** the first Shipping build booted into `L_Galaxy` with stars, planet, belt and the squad, but with no HUD of any kind: the fallback HUD was compiled out of Shipping and Nick's HUD is still off by default. Every earlier check had been a Development build, so nothing caught it.
- **Fixed:** the fallback HUD now exists in Shipping too (it hides wherever a real HUD widget is up). `ship_2005` shows ship cards, wave counter, objective line, wingman arrows, enemy markers and hit chevrons; wave 1 runs and the enemies attack.
- **Lesson:** a package that builds is not a package that plays. Each integration batch now ends with the Shipping build launched and looked at.

### Nov 12, 20:35 EST: why the props were dark, and the menu in Shipping (engineering Lead)
- **Seen in the running game (agent over MCP, Cinder and Verdance on foot):** crystals and big mushrooms were black silhouettes although their material values were correct. The log had the answer: `M_TD_PropTextured` "Failed to compile ... Sampler type is Masks, should be Color" because the master's default mask texture was the engine's sRGB default; every prop had been drawing with the engine's default material. The master now defaults to one of our own mask textures; captures `Saved/TDShots/mcp/pt7_c_glow2`, `pt7_v_glow2` show amber and cyan crystals and glowing mushrooms.
- **Lesson:** a headless import cannot compile materials, so a broken master only shows in a rendered game. After every import, grep the game log for "Failed to compile Material".
- **Shipping runs (no input, window captured):** `ship_2100` with Nick's HUD on by default: his HUD attaches, and the fallback draws wing arrows and hit chevrons over it. `ship_2120` boots to the placeholder menu (NEW GAME / CONTINUE / QUIT); a dev note that showed behind the menu was removed. Not human-played.

### Nov 12, 20:55 EST: the Shipping build played from the menu with real keys (engineering Lead)
- **Played:** `Builds/ship_2200` (Shipping, 808 MB, `main` at `cf17cb6`). Launched windowed; key messages posted to its window (Enter, Tab, Esc), window captured from outside the process. An agent pressing three keys, not a human session.
- **Worked:** boots to the menu; Enter on NEW GAME starts wave 1; Nick's HUD is up with live data; Tab cut from Lancer to Bastion; Esc paused ("PAUSED - ESC TO RESUME"); enemies read clearly as red-orange placeholders with lock brackets and distance. Capture: `docs/eng/evidence/shipping_keys_ship_2200.png`.
- **Wrong:** with the player idle, Rift was down inside 30 s of wave 1 (too harsh for a first-time player: wave 1 gets an opening grace); the fallback's hyperdrive line sat on top of the player's ship (moved); hit chevrons too thin and in a squad colour for shield hits (thicker, near-white).

### Nov 12, 23:30 EST: two characters in one evening, and what only playing showed (engineering Lead)
- **Enemy trooper (Ryan's concept, taken over from Nick's list, R-019).** Route: fitted armour built by script in Blender on the project's own mannequin body and skeleton, by a builder sub-agent, with the Lead and then an independent reviewer sub-agent judging contact sheets against the concept. Rounds: clay form (thin plates on a slim body, small head) -> more mass -> textured (reviewer: 2.6 of 5, "a competent soldier on Manny, not the concept": boxy limbs, dark face, 0.55% glow against the concept's 3 to 4%) -> polish (angular glowing eye panels, forearm ovals, pinstripes, tapered limbs, one-piece helmet) -> v11 (dark hoses, scribed panel lines) -> Grunt and Heavy variants told apart by silhouette (shoulder width -13.5% and +11%). Sheets for every round: `ArtSource/EngCharacters/NullTrooper/review/`. The lesson repeated: my first note ("more mass") was half wrong, and only a reviewer who had not watched the build said so.
- **Player body (Nicholas's rigged sample, N-021).** Not on the UE5 mannequin skeleton despite its file name (UE4-style bones), so it keeps its own skeleton and six loops; a small C++ animation instance blends idle, walk and run by speed. Its design is close to a well-known game's armour; that was put to Ryan, who decided to ship it.
- **What tests did not catch, found by spawning the characters in the running game:** a socket on the trooper's hand made every shot miss (aimed from the chest, launched from the hand); with the fallback animation a living Grunt lies collapsed on the ground; the Heavy's pellets are large pink blobs that hide it. 538 tests were green through all of it. New rule for character work: assert the pose (head above pelvis), not that an animation mode is set, and look at every character in play before calling it integrated.

### Nov 13, 03:10 EST: ships for both sides, a 1 GB scare, and a crew bug that was a unit (engineering Lead)
- **Enemy fleet and fallback player ships.** Two builder sub-agents code-built the five Null Armada ships (plus the carrier's bay and turret) and fallback Rift / Lancer / Bastion in Blender, each with baked textures, sockets (`Muzzle_N`, `Engine`, ability points) and a `DA_*_Visuals`, after 5-6 rounds scored by fresh reviewer agents. Scores reached about 3 / 5 (enemies) and 3.3 / 5 (squad): a big step up from spheres and planks, still below our hero bar of 4. Review sheets: `ArtSource/EngShips/{NullFleet,Squad}/review/`. Lesson: a re-import kept the previous mesh's socket positions until the import script wrote them explicitly.
- **Package size.** A Shipping package measured 1,026 MB against the jam's 1 GB limit. 247 MB of it was the debug-symbols file, which players never need; `Tools/td-package.ps1` now passes `-nodebuginfo` for Shipping and warns over 1 GB. Without it the same build is 779 MB, 631 MB zipped.
- **Crew meshes collapsing under every animation.** Not a skeleton mismatch after all: the crew FBX exports stored bones in metres, so Unreal gave the root bone a scale of 100 in the reference pose and every animation (root scale 1) shrank the body a hundredfold. The pilot never had it because its build script bakes the 0.01 unit scale. Fix: centimetre re-exports, in progress.
- **Nick's PR #12 (Lancer cannon sounds).** Cooked on current `main`, all 17 packages in the Shipping package, and in a Development run with the audio engine on `au.DumpActiveSounds` showed four live `MSS_NeedleCannon` voices during a burst, 0 audio warnings. Posted "no objection" (N-031); Nicholas's listening is the remaining gate.

### Nov 13, 06:05 EST: seeing tonight's art in the running game (engineering Lead playtest, agent-driven)
- **How:** Development game of `main` 959d468, offscreen 1080p on port 8010 over the engine's MCP server; console stages and ship switches, eight captures in `.wt/int/Saved/TDShots/audit3/` (local).
- **What works:** the ringed, textured Verdance against the magenta nebula and sun; squad colours on the new Rift, Lancer and Bastion hulls; the red-and-black Null Armada fighters read inside their brackets; the HUD sits cleanly over all of it.
- **What doesn't:** (1) in the Dominion assault, huge flat orange-yellow glowing slabs blow out across the screen next to the hull (sent to T4 to identify and fix); (2) engine trails are long thick colour bars on the wingmen and still reach the HUD from the flown ship (sent to T1); (3) the flown Lancer showed as a blown-out white-cyan ghost two seconds after a switch (T1 to check).
- **Lesson:** headless renders and per-asset review sheets scored every asset 3-3.5/5, but the worst problems only appear when everything is combined under the game's real lighting, bloom and camera. Capture the running game after every art batch, not just the asset sheets.

Nov 13, presentation team. The 139.2-second combat track reopened with its loop and Music class intact. Main has no DA_UI_Style package, and the director reads combat state but does not consume MusicIntensity01 for gain. We staged the verified Wave from f5d9cf9d and the existing PR11 style in a clean copy at dae4943a, requested the gain hookup in N037 and started the local editor build. Battle playback and listening approval are pending.

Nov 13, music integration. Added the five previously reloaded Unreal packages for Dominion boarding, the two credits tracks and Verdance. Their bytes match the published Audio checkpoint 52f84e54. The music class is present on main. T7 owns the gameplay events listed in N-045. Hearing the tracks in a matching playable build remains pending.

Nov 13, Dominion assault. Infinity Zone passed a fresh Unreal process reload as a 48 kHz stereo SoundWave in the Music class. Only its new package changed during import. Native checkpoint 70987397, verification 6db3aa98. Listening in the four-generator assault is still pending.

Nov 13, Dominion assault music. Published the full 144.68-second Push The Attack cue with its original ending and return, the preparation source and an eight-second loop sample. A fresh Unreal process reopened the SoundWave with looping enabled, the Music class and volume 0.5. The post-wave-five handoff, exclusive playback through the four generators, actual sequence-end stop and packaged listening check remain open.

Nov 13, survey terminals. Both approved 1920x1080 scan graphics reopened as native Unreal Texture2D assets in a second editor process, with their UI colour settings, clamped edges and no mipmaps. The next check is reading the buried structure on the Verdance terminal and the cyan/amber overlay on Cinder during the actual scenes.

Nov 13, credits. Imported the first eight-section credits roll through the existing victory panel data contract and reopened all 28 lines in a fresh editor. Its team, story, music, assets and tools text matched the source, including punctuation. Final cooked-asset reconciliation and checking the scrolling text at gameplay resolution remain open.

Nov 13, space idle music. The full 144.8-second Ambient1 track reopened as a looping Music SoundWave in a fresh Unreal process. The import added one asset and the reload changed none. Quiet-flight playback, giving way to the tutorial, combat and assault, and the cooked build still need a gameplay check.

Nov 13, lower-memory audio review. Audio recorded the live Rift in L_Galaxy at 1280x720 with audio from the same game process. The eight-second capture had no dropped video frames or reported audio discontinuities. It demonstrates the capture method. The menu loop, Lancer damage sounds and full tutorial timing still need separate played evidence. Tools: Unreal Engine GameViewport capture, Windows process-loopback audio and FFmpeg. [Clip and receipts](docs/visual/Submission/AudioReview01/).

November 13, menu and Lancer audio auditions. The isolated 28f03b4f editor host with the PR review Waves recorded its own game audio and viewport. The full menu capture lasts 111.74 seconds and includes the expected 107-second return. The source fingerprint places the return near 107.09 seconds; the same menu Wave was resolved again after New Game and a debug return from Galaxy. Separate 15.17-second combat footage verifies Lancer possession, with the active pawn and view confirmed before and after the trials. Individual fire, hull and shield cues still require identification. Both recordings completed without dropped video frames or reported audio discontinuities. [Review clips](docs/visual/Submission/AudioReview01/).

November 13, presentation music. A private UE 5.9 Windows Editor build compiled the 78caa422 credits, boarding and Verdance plugin using a single worker. Disabling shared precompiled headers kept RAM within the guard and exposed a missing Pawn header in the existing HUD file. The corrected module passed all three state tests. The first test launch on a sparse host failed at editor startup; the successful run used the full project content and headless agent mode. Actual sound and event timing are the next checks. [Build and test receipt](docs/visual/Submission/MusicBuild01/results.json).

November 13, credits playback. The Windows game played Cathedral Pulse (1) through its natural end and started Ethereal Pulse (Remix) on the next audio component. A repeat start left the first track running. An early stop and restart returned to track one. The published 13-second excerpt preserves the captured samples across the change. The tested presentation player also exposes the Dominion boarding and Verdance controls for the gameplay owners. The victory entry, stinger, shaped screen and Shipping playback still need a matched build.

November 13, combat music. The full Ethereal Pulse (Remix) SoundWave from f5d9cf9d is available beside the tutorial assets for waves 3–5. The current presentation style serializes the tutorial reference. Connecting the independent combat field and hearing its intensity changes in a live wave remain pending.

November 13, victory credits. The authored roll now credits Nicholas Tosti’s UI team and OpenAI image generation for the Verdance and Cinder survey scans. A fresh Unreal Editor process read back eight sections and 31 lines, including the 28 existing lines. The revised victory screen and Shipping roll still require a gameplay check.

November 13, music loop repair. Nicholas noticed a click when music restarted. A Verdance map recording revealed a 32 ms restart gap. The revised SoundWave loops in the audio renderer with smoothed endpoints. Both recorded repeats aligned at the same sample on either side of the join, including one during the takeoff fade. [Listen to both loops](docs/visual/Submission/MusicBuild01/music-loop-check.md).


### Nov 13 — Settings focus correction verified in packaged candidate

Nick's Lead reviewed Ryan''s i71-focus-662b8c04-dev contact sheets at 1280x720, 1920x1080 and 2560x1080, the outline report and the raw 14/14 passing tests. The selected Settings outline returns immediately after Esc or simulated pad B, stays visible without navigation, skips disabled Continue and clears behind Settings. Volume remains 90% after reopening. This is technical verification of the released candidate, not a main integration or human approval claim. Main pickup requested in #71. No local Unreal process was launched for this review.


### Nov 13 — reward build checkpoint and Lancer replacement audition

The industrial reward frame passed import and reload. The first widget build exposed a missing Slate bracket; UI corrected it and Lead rebuilt successfully, then passed the existing dismissal/next-unlock lifecycle test with 2,434 protected files unchanged. Actual rendered appearance remains queued for memory. Audio published the nine supplied-recording replacements on its isolated branch at 2b074bb5; its actual game capture observed fire and shield cues, but not a hull-hit event. Neither result is a final appearance/listening approval or a main asset integration.


### Nov 13 — second script circulated and reward review unblocked remotely

At Nicholas's request, Lead published the exact reviewed second script draft at 2240d3cd, preserved all earlier documents, and directly tagged Ryan in issue 29 with an acknowledgement request (N-080). Current script review now uses draft two; prior explicit approvals stand and final recording remains unlocked. The industrial reward candidate is published at 59ec2f79 / draft PR 85. N-081 requests matching T2 captures while local rendered work is memory-queued. Nicholas selected Audio's supplied-recording previews; the existing PR 12 candidate was fast-forwarded to 2b074bb5 without merging to main or claiming final in-game mix approval.

### November 13 — separating a loop seam from a runtime interruption

Nick's Audio recorded a natural 139.2-second combat-track return on the actual Galaxy music director in the preserved 28f03b4f test host. Comparing late-first-cycle and second-cycle source matches found no added loop delay at 0.25 ms resolution. A separate 42.75 ms timing shift occurred earlier, near a logged mixer underrun; the recorder itself reported zero discontinuities. We kept the master unchanged and routed the runtime finding separately. The process exited normally with all 2,457 protected files unchanged; debug menu travel left only menu music active. This is an older-host audio check, not a current campaign or Shipping pass. The actual excerpt and measurements are in docs/visual/Submission/MusicBuild01/music-loop-check.md.

### November 13 — preserve the separate core theme while native work queues

Audio continued the reflected gameplay-event subscriptions while Lead prepared Nicholas's original Cathedral Pulse for the core chamber. We preserved the MP3 and all 3,338,880 decoded frames, producing a 69.560-second stereo master with Python and FFmpeg. The music file check passed at -14.7 LUFS and -2.7 dBTP without clipping. This prevents confusing the core selection with Cathedral Pulse (1), already used for credits. Looping, native import, real scene timing and listening approval remain open; no Unreal process was launched for this preparation. Sources, reproducible preparation and measurements are in ArtSource/Audio/DominionCore01.

Follow-up: the scoped core SoundWave import and independent fresh-process reload both exited 0. Native exports matched one another exactly and differed from the master by at most one 16-bit rounding step, with all 3,338,880 frames preserved. All 2,458 pre-existing protected files stayed unchanged during import; reload protected the new asset as well. No style, map, gameplay trigger or looping behavior was changed. Both Unreal processes exited; native data is verified, while actual gameplay and Shipping remain pending.

### November 13 — two confirmed Cinder music roles

Nicholas assigned Cinder World to exploration and Cinder Fall to combat. Audio preserved both originals and prepared complete 150.000-second and 148.800-second stereo masters with Python and FFmpeg. Both passed the music-category file checks without gain changes: World -14.4 LUFS/-2.2 dBTP; Fall -15.0 LUFS/-2.3 dBTP. Lead verified all original/master hashes for publication. A later combat beat-drop entry is being auditioned separately; the full masters are untrimmed, and no offset, loop or native gameplay integration is claimed here.

### 2026-11-14 — Cinder music native data verification

The complete World (150.0 s) and Fall (148.8 s) masters imported and reopened in separate guarded Unreal processes. Both exited cleanly; exported PCM matched across runs and the masters within one 16-bit step. Protected files remained unchanged. Verified packages transferred to the integration checkout; reports under ArtSource/Audio/CinderMusic01/native-checks. No loop, offset or gameplay binding was guessed. Next: T1/T5 event contract, Audio hookup, cook and in-game listening. No Unreal processes left.

## 2026-11-14 00:10 EST: overnight run begins (Ryan asleep; decisions by Ryan's Lead)

- **Batch L on main, 6fe3e90f** (full suite 870 of 870 with Nick's menu plugin and audio merged). First engineering batch since 16:10 on Nov 13. The gate caught three real problems on the way: Cinder could not be finished (the anchor tower's collision box swallowed the charge points and blocked the walk back), a profile test that assumed no story listener, and the relay transfer's first wave arriving in 3 s. All fixed by their owners before the push. Shipping package cooking from this revision.
- **Playtest of batch L (Teammate 7, agent-driven over MCP, some stages by labelled debug moves):** front end to victory with no blocker. Findings sent to owners: silent ability awards, crew dying in a standing crouch, the Heavy's guard with no lunge hint, unmarked reactor conduits, an escape that says ninety seconds and takes six.
- **Decisions made for Ryan tonight** (he can overrule any of them):
  1. Every destroyed rock or junk piece pays a small boost top-up (3 / 6 / 12% by size, capped at 15% per second); specials keep their pickup drop. From his instruction that destroying asteroids should be rewarded.
  2. The Aftermath sequence may not be lost: atmosphere entry is held while it plays (T1) and a cut Aftermath replays on the next galaxy load (T3).
  3. The relay's first wave waits until "Company incoming" airs, so Cinematic 03 does not play over a fight (T5).
  4. Dominion interior: hangar, arena and corridor keep their value (enemy contrast over black gunmetal); the reactor room lost its red floor wash; crew read at 30 m through their materials.
  5. Asset overlaps between our own builders in batch M: the r2 Dominion data asset and the refitted anchor tower from the import builder; DA_Travel_FX from the effects builder.
  6. Nick's side has eight pieces of light work for the night (board R-045, issue #88).
- **What worked:** asking every builder for a fixed-camera before/after sheet and looking at it before accepting; the asset trace that found 31 of 54 new effects wired to nothing.
- **What didn't:** one build at a time across a dozen worktrees. The final gate for batch L sat in the queue three times; each run is ten minutes once it starts.

### 2026-11-14 — Story fidelity handoff

Story Director reconciled all 150 screenplay entries, 259 barks and staging records into StoryFidelity01, preserving the historical baseline while adding current batch-L source evidence. Existing console-log, Wren-order and final-caption fixes are resolved at source stage; queued owner work is separated from new findings. Lead verified the frozen three-file hashes and matrix coverage. The approved Cinder temporal-device line was reconfirmed to Ryan in issue29 comment6872047241. No new voice performance or gameplay acceptance is implied; no Unreal processes launched.

### 2026-11-14 — Actual Shipping demonstration, including a failed capture method

The verified 6fe3e90f package ran locally with game audio. HWND-only GDI recording misleadingly retained the menu surface while the game progressed, so that take was rejected. A second bounded run captured the foreground game client rectangle and same-PID WASAPI audio, yielding 200 seconds of opening and early combat. The review copy preserves the actual soundtrack; no music was replaced. Raw evidence and the 70 ms packet-timestamp limitation are recorded in docs/visual/Submission/LatestGameplayDemo01. Full tutorial-loop/fade and later campaign checks remain open. The isolated game/profile were closed after capture; protected build files were unchanged. No Unreal processes left.

### November 14 — submission drafts and an opening-route finding

The itch draft now names the actual Windows launcher and includes the boarding route without revealing the ending. Glass Box distinguishes engineering traversal, package launch checks and the actual 200-second recording, including the failed capture method. Story's bounded video review confirmed that five legacy Intro_N captions still play in the opening; the timestamped finding went to T3 through N-088 at 091ac013. No new story wording, final trailer, public page or human acceptance is claimed. Native music import evidence remains separate from the pending scene-event hookups and listening checks.

## 2026-11-14 01:00 EST: the machine crashed, and what we changed

- At 00:30 Ryan's machine blue-screened (bugcheck 0x119, VIDEO_SCHEDULER_INTERNAL_ERROR) with seven teammates, six builders, a cook and a test run all using the GPU. It rebooted; git was intact; every agent's uncommitted work survived except one document that came back zero-filled (restored from its last commit). All lanes were running again by 00:50.
- New rule (AGENTS.md section 11, board R-046): at most three rendering Unreal processes on the machine at a time; count before launching. Commit work in progress often.
- Earlier the same hour the one-at-a-time compile queue had nine builds waiting; binaries for a commit are now copied from whichever worktree built it first instead of being compiled again in each.

### November 14 — gameplay-first trailer rough cut

The existing Shipping recording yielded a 50-second edit that starts directly in Rift combat, shows the actual Lancer switch and then later Lancer/Bastion action. The matching recorded game audio was retained, with only brief fades at edit edges. Full audio/video decode passed; reviewed interval frames show gameplay from the first frame. The exact three source intervals, hashes and checks are in docs/visual/Submission/Trailer01. This is a review draft of 6fe3e90f, not the final submission or proof of the tutorial's full fade. No native game/editor process or new build was needed; final-build planet/on-foot/finale footage and Nicholas's acceptance remain open.

### November 14, 01:16 EST — Nicholas's Codex team stops safely

Nicholas requested shutdown at safe checkpoints. Lead paused its goal; Story, UI/HUD, Audio and Auditor preserved their existing files and reported no owned operations. The recurring issue check was paused. Normal01 tutorial capture failed its recorder-duration bound; Normal02 stopped before recording when the game was not foreground, followed by physical Escape. Neither proves tutorial timing; both preserved package hashes and closed the owned processes. Story's three held-line review and Auditor's exact-opacity review finished without production edits. CLAIMS.md and docs/visual/STATUS.md record unfinished files and restart conditions; N-090 notifies Ryan. Ryan's agents remain unaffected. No automatic restart is authorized.

## 2026-11-14 02:10 EST: batch N on main; decisions made for Ryan

- **Main `f39a8372`** (batch N, 908/908 headless tests): music event bridge, entry hold and boundary rule (T1), ENG-40 and the story fidelity checks (T3), spoiler-safe credit, animation and effects rounds, T4's finale balance. Batch O is in its gate (per-type crew deaths and attacks, warm-up and background music loading, enemy marker shapes per type, the music fallback, Rift v13 / Lancer glass / Bastion v02, planet landmarks and story camps).
- **Decision (Lead, under Ryan's delegation): ENG-38 ships as a smallest complete version.** An 8-10 s forced glide from above the cloud deck down to 900 m (not the full 15-20 s from 3 km), fog by altitude, scatter ready before the reveal, an altimeter tape and hull-temperature bar, an exposure push into the white cover, a wind layer, and the take-off return point mirrored from the entry. Left for Ryan, nothing cut silently: the full-length glide, map preload in the gravity well, the in-space atmosphere phase, scatter fade-in by distance, canopy droplets and heat haze, a full reverse glide on take-off, the sonic boom.
- **Decision: two hitch rows re-attributed.** T7 traced the wave-3 start (473 ms) and Verdance entry (714 ms) hitches to the stage cheats, not natural play; both are held until a natural run measures them.
- **Reviewed by the Lead from captures (agent-driven, no human play):** the Cinder derrick landmark reads from the approach and the ground; the Verdance relay mast is too thin and is being rebuilt at about 100 m; Bastion v02 holds its amber (cockpit depth still to do); the Null fighter is only a few pixels tall at fight range in the chase camera, so T2 is checking that the per-type markers carry the read.

## 2026-11-14 02:50 EST: batches O and P on main; what the Lead decided and found

- **Main `4e894365`** (batch P, 925/925 headless tests), after batch O `61404a79` (922/922). O: per-type crew deaths and attacks, the warm-up against first-use hitches, enemy marker shapes per type, music fallbacks, the pilot card fix (#49), Rift v13 / Lancer glass / Bastion v02, planet landmarks and story camps. P: the planet glide and mirrored take-off (ENG-38), Rift v14, the 104 m relay mast, space-region set pieces (third round), Verdance cliffs and Cinder magma retune. Neither batch has been played end to end; T7 plays after its performance window (02:42-03:32, every other agent told not to render).
- **Package size is the risk:** the Shipping zip of `eb1bf334` is 981 MB, 19 MB under the limit. The import builder measured the staged build (textures 610 MB of 880 MB) and wrote reversible cook-exclusion rules for 134 unreferenced old packages (about 120 MB). They are proven only by a trial cook, which runs after the window. **Held for Ryan:** dropping the DirectX 11 shader set (10 MB; the game would then need DirectX 12).
- **Decisions made for Ryan:** the Warden's shield gets a rig fix (moved from the spine to the left forearm; old mesh kept) because it floated beside the body in every pose; the Bastion glass cockpit was attempted and stopped (the hull's windscreen is a raised cab block; v02 stays, a design decision for Ryan); the story-kit asset overlap between two builders was resolved to the structures builder's versions, which own that kit; moon-shard set pieces keep the glowing crack network over the reviewer's cold-face suggestion.
- **Found from captures the Lead looked at (agent-driven, no human play):** the first planet entry's 330 ms comes from scatter meshes loaded by name on the game thread (T4 is making it asynchronous); the Verdance research outpost shows the planet's ground as its floor (T5); Verdance canopy clumps read as green blobs from the air (environment builder, another round); the glide's cloud moment read as smoke puffs (replaced by a full-screen layer, not yet seen through the real trigger); a scripted Continue from the menu started a fresh run (T3 is checking whether that is the game or the script).
- **Workflow note:** three agents launched renderers within a minute at 02:14 and passed the two-renderer cap; launchers now recount 10 seconds after starting and back off if they are the third.

## Nov 14, 11:45 EST: ENG-47 decision record (Lead, at Ryan's word "go with the best option that fits our game and direction")
- **Decision: the boarding ending is the game's ending.** `td.Finale.Boarding` stays 1 for the freeze build: break the four generators, dock through the tractor beam, fight the interior on foot, the reveal, the reactor, the escape. It carries the story (storyline.md) and the on-foot blade combat, and T7 played it through today with no blocker (#120, #129).
- The space-only reactor ending stays in the build as the emergency fallback (`td.Finale.Boarding 0`) and is kept finishable: played to VICTORY by T4 today; still to be seen good: the generators killed by real fire and the breakup on screen (T4's outside camera, 361b56bc).
- Revisit only if a blocker appears on the boarding route before the freeze (Nov 15, 12:00 EST).
- Same instruction, new flight HUD (#160): stays open until it is drawn in the game and matches the approved mockup; pieces are pushed as they compile.
### Nov 14, 12:30 - Pilot melee animation pass (animation builder, eng/anim)
- Research (ArtSource/EngAnim/Pilot/MELEE-RESEARCH.md): light swings coil 0.1-0.2 s, contact, 30-60 deg follow-through, recovery = the lockout; play-rate / stride changes within about 20 %; legs on a strafe layer when the body faces the target.
- Found by measuring, not by eye: the game played the original swings (blade pointing back, through the thigh and belly), the walk loop slid at 2-3x (its true speed is 130 cm/s, the code assumed 300) and the run at 25 %, block and run put the blade into the chest / thigh. A headless clearance check (ue_p01_clearance.py) now measures blade-to-body gaps for every clip.
- New: v3 three-arc combo with chained cross-fades, heavy v4 thrust, air-slam fall and landing, block / flick v2, run v2, hit reacts v3, combat idle, strafe legs with chest counter-twist. TD.Pilot.MeleePass 0 brings the old set back. Before / after sheets in ArtSource/EngAnim/Pilot/review/.


## 2026-11-14 13:00 EST - Nicholas Lead: Batch 3 and natural Dominion checkpoint

Offscreen input-driven playtests continued Verdance wreck/relay and waves3-4 through Cinder survey, excavation/EMP, anchor charges/escape and departure. Cinder evidence b82ed996; flight subtitle settings retest c58285f5; natural Wave5 completion b5d1549c. Steering/aim assistance, private saves and inspection pauses were used, not teleport/damage/progression overrides. Wave5 cleared take1, Dominion generators0/4 opened; no full campaign or human difficulty/feel verdict. Three final combat clips and native screenshot/log preserve provenance in the private AgentBatch3 session11 record.

[Batch3](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/evidence-agent-feedback-batch3-20261114): six-page PDF visually inspected, original PNG/MP4 media, detailed notes, logs/results and hash manifest; uploaded assets verified and report read back on #140. Concrete findings: required dialogue carrying between runs (#139, source-only, now source-fixed pending runtime); Large subtitles colliding with radar (#103 after #160 consolidation); anchor fall frozen after success (#119, owner refinements in progress); route-clarity candidate. Later native assault screenshot exposed radar WAVE1/5 after Wave5 completion, sent to #103. Original subtitle edge overflow/size failures passed reviewed cases and #154/#155 closed. No claim that remaining HUD or audio is approved.

Nicholas approved the written art guide only (#132). Lead accepted Bastion tap/hold clarification under routine editorial delegation (#159). Menu-loop join and new Wren take decisions remain Nicholas's. Existing specialists/schedules remain paused; separately user-authorized Issue Coordinator performs bounded issue/source work. Human saves unchanged; session11 exited0 and no Unreal processes remained at its checkpoint. Native rebuilding/play continues only in the isolated Lead worktree. No production code/assets changed by this update.


## November 14 — Nick Lead: historical menu credit status clarified

CREDITS.md now distinguishes the November 12 menu-music preparation checkpoint from Ryan's reported ddc50a2f package inclusion, with the original source URL and authorship/Pro-at-generation record preserved. The new scoped note does not apply ddc inclusion/exclusion to later packages or qualify Ryan's separately dated entries. No loop-join/mix approval, PR11/style merge, final-container claim or asset change. Reused the stopped Auditor's ResumeCredits01 and Issue Coordinator's Submission137 delta at91a70782; no repeated inventory audit. Verified exact diff/whitespace and retained source attribution. Remaining credit-row corrections and final package inventory stay open in #137.
### Nov 14, 14:40 - Pilot animation round 2 (animation builder, eng/anim)
- Measured, not assumed: a per-frame foot trace in the running game (TD.Pilot.FootTrace) showed planted feet sliding 72-83 cm/s at walk / run; the reference speeds ignored the body's 1.04 mesh scale and the walk / run switch sat too high. After the fix: 10-15 cm/s steady, 18-19 in the start, 29-32 at sprint.
- The blade carry was re-posed three times from T7's chase-camera playtests (stub at idle, tip in the ground behind the walk, hidden in the run) until the whole beam reads from behind in idle, walk and run.
- Deflect proven by the log (flick clip playing on a returned bolt); foot IK proven by numbers on a found 14 deg slope.


## Nick Lead evidence and ownership checkpoint — November 14, 17:35 EST

**Nick Lead checkpoint, November 14, 17:35 EST:** Lead headless playtesting explicitly resumed; original Story/UI/Audio/Auditor tasks and schedules remain held. Issue Coordinator and Asset Handler have separate direct authorizations. Batch 4 PDF and original evidence delivered on [#140](https://github.com/rthomas24/2026-epic-ai-megajam/issues/140#issuecomment-6884669972). Private Build14 b8a55662 compiled; sessions31-32 reproduced Face to Face replay after Quit to Menu, process exit and cold Continue; [original saves/logs/inputs delivered](https://github.com/rthomas24/2026-epic-ai-megajam/issues/96#issuecomment-6885161858), T3/T5 accepted the defect and fix is pending. This is separate from the legacy final-record bypass. Human main remains at8b4eabae with its local project change preserved; private test updates do not update Nicholas's launch project. No whole-game, audio, performance, package or AAA acceptance is implied. Historical checkpoints below remain preserved.

- Campaign coverage: session19 on91a70782 reached victory through the natural route with disclosed input/camera assistance. This is historical evidence, not a current-main full-campaign pass.
- Batch4: session26 Fire instruction carry and manual switch; session29 explicitly staged AutoOnDeath did not satisfy the manual-switch lesson; session30 Moss blade-light readability and light-versus-heavy input received narrow passes. Buffered/RespawnRescue, full combat balance and all-pilot presentation remain unverified by these sessions.
- Story96: session28 original legacy save bypass remains unresolved; sessions31-32 independently found the once-per-run disk-persistence gap onb8a55662. Existing T3/T5 owners handle both. Next verification uses the supplied unchanged input save and two cold launches after an integrated fix SHA.
- HUD103: Ryan's latest card/subtitle overlap report is assigned to T2; matched three-resolution/all-ship/power-state evidence remains pending. No competing Codex HUD implementation.
- Audio146: Hero's Wake Candidate01 selected for in-game audition; T3 owns the optional opening-bed field and narrowly authorized asset import/binding. Audio151 engine Candidate01 selected for audition only; exact owner claims/settings still required. No by-ear/final mix or packaged acceptance.
- Presentation142/149: prior scoped menu/canopy decisions stand; remaining owner runtime/package/motion checks are not replaced by static approval. Original specialists do not restart from these dependencies.
- Asset Handler's separate Quixel collection authorization supplies optional source candidates; source QA and license receipts do not certify UE import, cook, gameplay or final appearance.
- No Lead native operation remains at this checkpoint. Human saves and uncommitted specialist work are preserved. Current local operations and reserve gates remain in the Lead's local state/receipts.


## Nick Lead runtime checkpoint — November 14, 18:20 EST

Lead reconciled private Builds16–18 and sessions34–39 into the current Nick checkpoint in `docs/visual/STATUS.md` and `features.md`. Evidence delivered to existing #96, #103 and #194: cold-Continue persistence narrowly passed; legacy saved-position recovery and Cinder terrain/course alignment failed with reproducible progression blockers. Original saves, exact input transcripts, native logs/results, screenshots and the Cinder clip are preserved in verified private-release packets. All runs exited normally, with no owned Unreal process left. Source/assets, Ryan claims and feature completion markers were not edited; no human project update or whole-game acceptance is implied. Existing owners retain fixes and the next exact regressions.
