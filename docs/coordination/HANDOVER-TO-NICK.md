# Handover from Ryan's Lead to Nicholas's Lead (Nov 14, 2026, about 21:30 EST)

Ryan's agents (Lead, T1-T7, the builders) are stopping. This file is everything Nicholas's Lead needs to finish the
game from here. It is written for a machine with 16 GB of memory and a modest graphics card: every step below says how
heavy it is. Feature freeze is **Nov 15, 12:00 EST**; submission is **Nov 15, 20:59 EST**.

Ryan's instruction for this handover: Nicholas's agents may now work in engineering folders (`Source/`, `Config/`,
`Content/TriadDrift/Gameplay/**`) to finish what is listed here. The git rules in `AGENTS.md` section 6 still hold
(no force pushes, no history rewrites, no binary conflict resolved alone). Ryan can overrule anything on the board.

## 1. Where things stand

- **Main:** `6a33e972` plus docs (compiles for the editor and for Shipping; checked by our packaging agent).
- **Packages** (Shipping, Windows, each about 918.6 MB zipped, under the 1 GB limit):
  - `TriadDrift_3f6d29ac_Shipping.zip` = main `6a33e972`. Everything from tonight. See section 6 for the release link.
  - `TriadDrift_96a66f7c_Shipping.zip` = the last package before the Cinder Continue problem; played GOOD in Shipping
    through waves 1-2, the jump and landing on Verdance.
- **Played by an agent in a real New Game on main:** menu, intro, waves 1-2, the withdraw, travel and landing on
  Verdance, the Verdance route on foot, ship switching and CUT retakes, shadow mode toggling, the escape flight to
  victory (your Session48). **Not played end to end by anyone:** Cinder to its end, waves 3-5 in sequence, the Dominion
  assault into the interior, the reactor fight, a full run in the Shipping package (#152).
- **Ledgers:** `features.md` (markers, last aligned 20:30), `CLAIMS.md`, `docs/eng/STATUS.md`, `DEVLOG.md` (the 21:05
  entry lists tonight's work, what is unproven and the Lead's mistakes).

## 2. How to work on a 16 GB machine

Light (do freely): reading and editing C++, `.ini`, Markdown, Python tools; `git`; issue and board work; data-table and
data-asset edits through the headless editor commandlet; cvar tuning in a running game; log reading.

Medium (one at a time, nothing else running): a C++ compile (`Tools/td-build.ps1`; 1-5 minutes incremental, up to
25 minutes after a large merge; about 6-8 GB of memory); a headless automation test (`Tools/td-test.ps1 -Filter
<name>`, `-nullrhi`, no graphics card needed); one offscreen game session at 1280x720 (`Tools/td-play-mcp.ps1
-Offscreen`, then `python Tools/td_mcp.py`; 4-5 GB of video memory and 4-5 GB of memory, so only one, and close it
when the step is done).

Heavy (do not do on that machine): cooking or packaging (about 30 minutes here, 7 GB of intermediate files per
checkout, memory peaks above 16 GB with the editor target built); opening `L_Galaxy` or a planet map in the full
editor; Niagara or material authoring; Nanite or texture re-imports; Blender bakes; two games at once. Ask Ryan for a
package instead (section 6).

Rules that saved us: prove a change **headless first** (a test or a log line), then one short offscreen session.
Prefer a cvar or data-asset change over a C++ change, and a C++ change over an asset change. Never use an editor-only
API outside `#if WITH_EDITOR`: it compiles in the editor and breaks the Shipping package (it happened tonight with
`UStaticMesh::IsNaniteEnabled`; use `HasValidNaniteData`).

## 3. What must still be done, in order

### 3.1 Blocker: Cinder on foot (#194)
State on main: the bridge ramps reach the ground, the survey marker leads through the outpost doorway's centre, the
pilot steps up to 70 cm on planets, trailhead totems no longer block, a counted step whose targets fell early
completes on arrival, the river waypoint reads CROSS THE RIVER. Each piece was walked by its builder; **the whole route
has not been re-walked on `6a33e972`** (T7's result, if it arrived, is in section 7).

Acceptance (medium: one offscreen session): `TD.Stage CinderOnFoot`, do the survey with the room **cleared** (the
terminal refuses while hostiles are near; `TD.Ground.CrewKill`), quit to menu, `TD.Story.Continue`, then follow the
marker: pad, trailhead, door, river, bridge, gate, both real conduits (`TDShatterCrystal_0` at about (9300,5350) and
`_1` at (9300,6650); stand within 1.8 m; five light swings each), then the yard, the anchor and the shore. Use
`TD.Ground.Floor` at each step (it prints what the pilot stands on) and `TD.Ground.Where`. Then your original
Session45 save. Also check: break both conduits before the gate step starts; the step must complete on arrival.

Known root problem, not fixed: the relief ground's **collision sits 60 cm below its mesh** (the pilot's feet are at
z -58 on Cinder's flats; the field, the vertices and the component are all at 0). File:
`Source/unrealGameJam/TriadDrift/World/TDPlanetTerrain.cpp`, `ATDPlanetTerrain::BuildDetailGround`
(`CreateMeshSection(..., bCreateCollision true)`, `bUseAsyncCooking false`). Next probe (branch `eng/t4`, commit
`4726ce7d`, has `TD.World.Under` and a vertex probe): log `DetailGround->GetBodyInstance()->GetUnrealWorldTransform()`
z against the component's z, and the body setup's triangle-mesh bounds. Cheapest fix to try: call
`DetailGround->RecreatePhysicsState()` right after the sections are created. **If you move the ground to 0, re-walk the
bridge and the door in the same pass**: the ramp toes were extended to reach -60 and still work at 0 (they are buried),
but nothing has been walked that way. If time is short, leave the ground low: the route works with it.

### 3.2 Things Ryan asked for that are only partly there (#201, #209)
- **Easier mouse steering: not delivered.** Measured: with the current rate stick a 90 degree turn overshoots by
  68-81 degrees and a target 30 degrees off takes 1.8-8.3 s to settle. The attempted replacement measured worse and is
  parked as `docs/eng/patches/t1-mouse-follow-and-standby.patch.txt` (read its header). The measuring command is
  `TD.Flight.MouseProbe` (in the last batch; see section 7). Cheapest safe step (light): lower the mouse rate gain and
  add damping through the flight tunables rather than replacing the model; measure with the probe before and after.
- **Aim assist: on main, never seen on screen by an agent.** Cvars: `td.Aim.LockConeDeg` 16, `td.Aim.ConvergeDeg` 15,
  `td.Aim.TurnScale` 2, `td.Aim.MaxTurnDeg` 45. Headless test: `TDCombat.AimAssist.HitRate` (prints hit rates per
  ship with the assist off and on; no graphics card). A human should fly it once with a mouse; tune the cvars, then
  copy the values into `Source/unrealGameJam/TriadDrift/Combat/TDAimAssist.cpp`.
- **Wingmen in space:** shadow mode (G / pad View), powers, "your fight" (wingmen leave enemies the player hit at 15%
  hull, half damage in waves 1-2). Cvars under `td.Squad.Shadow.*` and `td.Squad.Solo.*`; `TD.Squad.Dump` prints the
  state and the kills per side; every enemy death logs `TDKILL <enemy> by <killer> (player|other|none)`. Unmeasured:
  shadow-on clear times from wave 3 (wingmen can no longer finish the player's target). If waves 3-5 drag, raise
  `td.Squad.Shadow.YourFightFloor` toward 0 or set `td.Squad.Shadow.YourFight 0` from wave 3.
- **More but weaker enemies:** built, **default off** (`td.Balance.Swarm 1` turns it on). Its balance numbers were
  never taken. Leave it off unless a human plays it and likes it.
- **Wingmen on foot (#209):** on main and ON. Played on Verdance and in the hangar only. Unplayed: a real interior
  corridor and doorway, boarding, Cinder. Known: the knock-down pose is a frozen stagger; the two follower barks never
  appeared in the log. One switch removes the feature and its 1.6x enemy count if anything goes wrong:
  `TD.Ground.Allies 0` (to ship it off, set the default in
  `Source/unrealGameJam/TriadDrift/Ground/Squad/TDGroundSquadSubsystem.cpp`).
- **Not started:** the shadow-mode hint card (G / View); the Verdance approach hold (the ship is held at the atmosphere
  edge for up to a minute while the Aftermath lines play; fix: in `IsStoryHoldingEntry` hold only while
  `!UTDStorySubsystem::HasLineAired(this, TEXT("S2_Wren_FormUp"))`; a STAND BY plate is in T1's patch file).

### 3.3 Smaller open items
- **Tactical panel and Continue ship (#103):** fixed and log-proven on main (`35660d69`); one natural-Aftermath frame
  of the panel is still owed. `TD.Flow.Slot` prints the saved ship; `TD.UI.Layout` checks overlaps.
- **Survey console up close:** E from within 1 m with the room cleared is unverified.
- **#96:** an old save restores behind the Warden exit correctly; the final record was not played to its end.
- **#111 escape look-back shot:** not built. Script: `docs/eng/patches/t3-escape-lookback-shot.patch.py` (run from the
  repo root, compile, `TD.Stage EscapeFlight`). Your Session48 finding stands: flown forward, the player never sees the
  Dominion go.
- **Turret telegraph:** main has the small flare. The larger ring (`eng/vfx` `eabdcd82`) is too big at 400 m; the
  capped version rendered nothing. Leave it.
- **Controller (#152):** every action has a pad mapping by audit (table in the issue). Check in play: Y is both
  Settings (global) and the unlocked ability in flight. A physical controller on the package needs a human.
- **First load:** the window stops answering for 5-6 s behind the animated loading card. Cosmetic: the card's picture
  is low resolution and LOADING appears twice.
- **Performance:** space 8-12 ms; Verdance about 9 ms; Cinder pad 14.5 ms (render-thread bound). Cheapest levers are in
  the Cinder scatter tables (`TDPlanetScatter` / `TDPlanetCourse`): cull crystals and basalt columns at about 1,200 m,
  no shadows past 300 m. Measure with `TD.Perf.Instances 40` and `TD.SpaceField.FrameWatch 1`.

### 3.4 Yours already (unchanged)
By-ear approvals (#146 intro bed, #151 enemy engine loops, `C8_Wren_Order`, #171 Wren takes), appearance reviews
(#142, #103, #149), ART-09/10/11/12/14/19/20/21, the submission page, the gameplay video, credits and the dev log.
What can change **without a rebuild**: every story line's words, speaker and voice in `DA_Story_Transmissions`;
`DA_UI_Style` (music, the intro bed and its fade); `DT_Barks` (regenerated from `dialogs.md` by
`Tools/td_barks_from_dialogs.py` and `Tools/td-barks-import.py`, headless). What needs C++
(`Story/TDStorySubsystem.cpp`): new line ids, sequence order, the must-hear set, intro line timings.

## 4. Dev commands worth knowing (all work offscreen)
`TD.Stage <Wave1|Wave3|Waves3to4|Wave5|DominionAssault|EscapeFlight|VerdanceOnFoot|CinderOnFoot|DominionInterior>`,
`TD.God 1`, `TD.Story.Continue` (the menu's CONTINUE), `TD.Story.Skip`, `TD.Take` (force a CUT),
`TD.Ground.SkipTo <Survey|Outpost|Jammer|WreckCore|Relay|Warden|Core|Escape>`, `TD.Ground.Floor`, `TD.Ground.Where`,
`TD.Ground.CrewKill`, `TD.Ground.Blade <light|heavy|perfect|pilot rift|lancer|bastion>`, `TD.Ground.Allies 0/1`,
`TD.Ground.AlliesDump`, `TD.Squad.Shadow 0/1`, `TD.Squad.Dump`, `TD.Squad.Switch <Rift|Lancer|Bastion>`,
`TD.Flow.Slot [write]`, `TD.UI.Layout`, `TD.UI.Status`, `TD.Input.Keys`, `TD.Input.TurnToward`, `TD.Look`,
`TD.World.Hold <s> [m/s]` (holds only the player's ship; the world keeps running), `TD.Perf.Instances`,
`TD.SpaceField.FrameWatch 1/0`. Capture notes: `pause` stops Niagara; use `slomo 0.2`. A screen grab with
CopyFromScreen captures whatever window is on top; use PrintWindow.

## 5. Parked work (not on main, kept on branches)
`eng/t4` `b1643bcc` (flat ground under bridges, unwalked) and `4726ce7d` (collision probe); `eng/vfx` `eabdcd82` and
`55b674c0` (telegraph ring); `eng/vfx` `d973ec63` (a cheap death pop, unwired); `eng/t7` (playtest scripts and notes).
Branches are local to Ryan's machine unless pushed; ask Ryan to push one if you want it.

## 6. Packages
See `docs/coordination/PACKAGING-HOWTO.md` (the one command Ryan runs, the Shipping compile check, the launch check).
Do not cook on the 16 GB machine. If a fix of yours must ship: push it to main, post the sha on the board with
"package please", and Ryan runs the command. If nothing new is proven by the freeze, submit `3f6d29ac` if the Cinder
walk passed, otherwise `96a66f7c` is the fallback that was played in Shipping (it lacks tonight's features).

## 7. Last results and the release link (Ryan's Lead, 21:35 EST)

**Source on main:** `58751acb` = `6a33e972` plus a Shipping-safe dev probe (`TD.Flight.MouseProbe`) and the two patch
files in `docs/eng/patches/`. Compiles for the editor; the Shipping target was checked at `3f6d29ac` (the probe is
inside `#if !UE_BUILD_SHIPPING`).

**Release (private, prerelease):** https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/playable-main-3f6d29ac
`TriadDrift_3f6d29ac_Shipping.zip`, 918,583,703 bytes, SHA-256
`a0cca31e557cd8cc9e9249de4f9f317dd17505a5631459f5d87e56e9a059a201`. Launch check PASS: splash 1.4 s, loading card at
2.3 s, no white frame, never "Not Responding" (longest stall 4.8 s), menu at 11.2 s, wave 1-2 HUD, pause and Settings,
New Game confirm with a save present. Earlier prereleases: `playable-main-cf4ed834`, `-7bf9491a`, `-36bee71d`.

**T7's acceptance walk on `6a33e972`** (Development, offscreen, real walking input):
- Cinder: Continue after a real survey GOOD; pad to trailhead GOOD (the totem no longer blocks); outpost doorway
  through its centre GOOD both ways; survey console with the room cleared GOOD (E works at 1.1 m); CROSS THE RIVER and
  the bridge GOOD (step flips to BREACH THE GATE 0/2 at the far foot); conduit A by blade from 1.3 m GOOD (1/2).
- **Not proven:** conduit B (the walker stood 40 degrees off and missed; T5 broke both earlier on `7688b7dd`); the
  yard, the anchor and the shore after the gate; the console refusing while hostiles are near; the early-conduits
  case (the conduits did not break before their step started, and no stuck 2/2 state occurred).
- So the Cinder blocker (#194) is **close but not closed**: one clean pass through both conduits to the end of Cinder,
  then the Session45 save, closes it.
- Not run on this main: the tactical panel in a natural Aftermath, the cold Continue ship, wave 1 kills with shadow on
  (each is log-proven by its author only; T7's last data on these is from `27415ffc`, before the fixes).
- Followers on foot were seen on Cinder: Kato cyan, Moss amber beside Wren magenta, in their suits; Moss fought crew
  at the gate.

**T2's notes (aim assist and HUD checks):** all aim cvars are in `Combat/TDAimAssist.cpp` (`td.Aim.Enabled`,
`LockConeDeg` 16, `ConvergeDeg` 15, `TurnScale` 2, `MaxTurnDeg` 45, `PadScale` 1.35, `StoryScale` 1.3, `AceScale`
0.75). Headless test, no graphics card, about 90 s: `Tools/td-test.ps1 -Filter TDCombat.AimAssist.HitRate -ShowLog`
then search the log for `AIMRATE` (now Rift 7 to 56%, Lancer 3 to 51%, Bastion 23 to 33%). To try values without a
rebuild put them under `[ConsoleVariables]` in `Saved/Config/WindowsEditor/Engine.ini`. `TD.Weapons.Accuracy [reset]`
counts live accuracy. `TD.UI.Tactical [s]` shows the tactical panel, `TD.UI.Caption <s> <text>` any caption. Never
seen in a frame: the lock bracket and lead marker, bolts curving onto a lock, the tactical panel in a natural
Aftermath, a generator diamond fading under a subtitle.

**Ryan's machine after cleanup:** only `Builds/for-ryan/TriadDrift_3f6d29ac_Shipping.zip`, the fallback
`TriadDrift_96a66f7c_Shipping.zip` and the unpacked `Builds/playable-3f6d29ac` remain; agent worktrees keep their
captures and logs under `.wt/<name>/Saved/` but their build caches are deleted (a rebuild takes about 20 minutes).

**Suggested order for the hours left:** (1) the Cinder pass above; (2) a human flies with the mouse and a controller
on the release and reports feel; (3) one offscreen New Game on main for the tactical panel, Continue ship and wave 1
kills; (4) the follower corridor and boarding check, or `TD.Ground.Allies 0` as the shipped default if it misbehaves;
(5) the mouse-steering gain, only if (2) says it is needed; (6) everything in 3.4; then "package please".

## 8. Mouse steering: final finding (T1, 21:30 EST, played from main 58751acb)

No existing setting makes mouse steering easier; do not spend time on cvars. Tested live on all three ships with
`TD.Flight.MouseProbe` (turn 90 / turn 180 / acquire a target 30 degrees off): today's defaults (recentre 3,
sensitivity 1) overshoot a 90 degree turn by 68-82 degrees and never settle; recentre 8, Settings sensitivity 0.6 and
`TD.Flight.ShipFeel 1` were each as bad or worse (several make 180 degree turns never complete). The controller's
`MouseSensitivity`, `MouseRecenterRate`, `MouseDeadzone` and `MouseResponseExponent` are plain properties (an ini
cannot set them). The probe's hand is a proxy, so absolute numbers are pessimistic; the comparison holds.

The fix is a change of model, in `ATDSpacePlayerController::PlayerTick`: the rate stick turns mouse position into turn
rate, so an aiming hand overshoots. The parked patch (`docs/eng/patches/t1-mouse-follow-and-standby.patch.txt`) makes
the reticle follow the ship: `VirtualStick -= ship angular rate * dt / ReticleFullDegrees` each frame. It needs
debugging before it ships: in its one test the stick capped at 0.11-0.15 and the Lancer ran on for 3 s after the hand
stopped, so clamp the stick and look at the run-on. Medium weight (C++ compile, then the probe offscreen). If there is
no time, leave steering as it is: aim assist (on main) carries most of "easier to hit".
