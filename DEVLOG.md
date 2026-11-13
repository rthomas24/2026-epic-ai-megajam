# Triad Drift dev log

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
