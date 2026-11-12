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
