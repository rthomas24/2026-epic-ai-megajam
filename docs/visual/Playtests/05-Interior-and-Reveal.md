# 05 Interior and reveal playable test

Owner: T7 for execution; Story Director for narrative expectations; T5 for interior flow; T3 for transmissions, labels and playback; T4 for docking. ART-21 script, prepared 13 November 2026. **Not executed. No gameplay or presentation pass is claimed.**

This test starts with the selected ship committing to Dominion docking and ends when Cinematic 08 finishes or is skipped and the future-crew defender encounter becomes available. It includes the defender handoff and identity checks; reactor damage, detailed boss/combat balance, the timed escape and victory belong to subsequent act tests. Retake/Continue checks here must also prove that they do not accidentally start those later acts.

## References and revision discipline

Source baseline: `abcfb578`, the assigned worktree base. Claim: `de6286a6`. Subsequent approved encounter direction: [Ryan's decision in issue 29](https://github.com/rthomas24/2026-epic-ai-megajam/issues/29#issuecomment-6868085209), relayed by Lead with claim `2ce1868e`. Record the actual tested revision and compare later source changes before using this baseline description. A newer packaged build can differ; report the difference rather than assuming the older implementation is still current.

- [Approved narrative and decisions](../../../Story/README.md), Gameplay 07 and Cinematic 08 in [Story](../../../Story/), [storyline sections 26–28](../../../storyline.md).
- [Complete-game checklist and ART-21](../../../features.md), [quality stages](../../../AAA.md), [playtest/process rules](../../../AGENTS.md).
- [Transmission contract and existing IDs](../../story/transmissions.md), [interior reconciliation](../../story/story-0708-actions.md).
- Runtime baseline: `Source/unrealGameJam/TriadDrift/Ground/Interior/TDDominionInteriorCourse.inl` (`BuildInterior`, `HandleStoryConsole`, `FireStepBeats`, `GetInteriorStart`); `Story/TDStorySubsystem.cpp` (`Defaults`, queue/played-state logic); `Flow/TDGameInstance.cpp` (`ExecuteRetake`, `MarkTransmissionPlayed`); `Flow/TDJourney.cpp`; `World/TDDominion.cpp`. Paths after the first are relative to `Source/unrealGameJam/TriadDrift/`.

Latest explicit approvals in the index take precedence over an older DOCX draft or a placeholder table. This test introduces no dialogue, physical squad staging, casting, encounter or scope decision.

## What exists versus what must be verified

| Area | Source at the baseline | Approved acceptance expectation / status |
|---|---|---|
| Entry and route | `L_Dominion_Interior`: hangar → deflect corridor → arena → link → Warden → link → reactor. Objectives `Obj_DI_Corridor`, `Obj_DI_Arena`, `Obj_DI_Warden`, `Obj_DI_Core`, then `Obj_DI_Escape`. | Existing rooms/corridors may carry the investigation/security/archive beats. No separately required shaft or archive room. Verify actual walkability and placement. |
| Boarding pilot | Normal docking hands off to one on-foot character. | The pilot of the ship the player lands boards; exercise Rift/Wren, Lancer/Kato and Bastion/Moss. Do not infer the pilot from an objective's `AbilityShip` field. Other pilots' physical waiting positions and extraction craft are not settled by this approval. |
| Four consoles | `A4_Recording_Hangar`, `_Corridor`, `_Barracks`, `_Core`; interactable, single use; the handler fires a transmission and lights the screen. Arena entrance/exit hold the middle two. Core console is inside the core chamber. | Approved words, order and identification rules below. Source defaults still contain different speakers/words. Overrides must be demonstrated, not assumed. |
| Essential playback | The shown handler has no encounter-clear or mandatory final-record gate. Entering `Obj_DI_Core` step 0 fires `A5_Choice`. | Safe listening; final record after Warden; recovered warning and confrontation before reactor combat/damage. Missing gates are pending integration and fail narrative acceptance, even if combat remains finishable. |
| Callback / C08 | Five `F_*` entries exist in the table; no call starting their sequence is present in this course. New full-warning/confrontation hooks are proposals in the reconciliation document. | Existing console, subtitles, safe local movement; hold reactor entrance/combat until completion or supported skip. Do not invent `C8_*` runtime IDs. Capture the IDs the tested build actually uses. |
| Retakes / saves | Played IDs survive `ExecuteRetake`; transmissions are marked played when queued, before airing. `GetInteriorStart` chooses an unfinished room's entry or hangar. | No repeated completed scene or erased evidence. Special attention to quitting or dying with essential audio queued: a played flag is not proof the player heard it. |
| Sound and optional presentation | Shared warning uses key `Dont`; authentication and archive comparison require delivered assets/hooks. | Exact shared warning; realistic three-pulse cue and clear archive comparison as approved. Final audio needs listening approval. Optional dressing is not a new pass requirement. |
| Confirmed defender change, not present in this source baseline | Two sentries appear in the baseline reactor room. | Ryan approved future versions of the two pilots who did not board; T5 builds it, with drones as fallback. Voices over a sealed threshold precede the encounter and the player keeps control. Reactor scope adds only the sweeping laser; no mandatory ring timing, acceleration or deflect-only heart. Report implemented mode and authorising revision; do not claim this change exists in `abcfb578`. Engage/fall dialogue remains draft until Nicholas approves it. |

## Setup and evidence

1. Use a Lead-approved matching packaged build; an editor run is supplementary. Record build SHA, branch, package/configuration, platform, input device, difficulty, resolution/UI scale, subtitle setting and tester. Do not call a Development-only test a Shipping pass.
2. Use an isolated test profile/save, retaining a recoverable pre-docking checkpoint for each pilot. Preserve personal saves and other agents' files. Obtain the build owner's supported setup/reset method rather than deleting saves or editing progress by hand.
3. Use one real, controllable windowed session. Coordinate the native slot with Lead. Any agent-owned launch follows `AGENTS.md`: private port and bounded lifetime (`-TDQuit=900` maximum); close only your own instance after evidence capture. This document launches nothing.
4. At least one pass must reach the interior by actual assault/docking, with the approach open and all four external generators destroyed. Checkpoints may shorten repeat passes. `TD.Stage DominionDocking` / `TD.Stage DominionInterior` exist for isolated Development reproduction; they seed progression and **cannot prove** natural docking, record gates, identity flags or save continuity. Label such runs accelerated.
5. Capture game audio, subtitles, HUD and player input together. Note video timecodes for docking, each console, each identity/tag change, Warden death, final record, C08 start/skip/end and reactor activation. Retain logs for the same session. Record absent or silent audio explicitly.
6. `TD.Story.List` may document available/played entries. `TD.Story.Fire <Id>` tests an isolated line only; it does not prove its natural event fires. `TD.Story.Reset` is allowed only on a separate disposable audition run, never during persistence/retake/Continue tests. These commands may be unavailable in Shipping.

Record each case as **PASS**, **FAIL**, **BLOCKED** (external prerequisite), or **NOT RUN**. For missing approved functionality also state **pending integration / narrative acceptance FAIL**; do not hide it as optional or mark the act green. Separately report whether the existing baseline route is playable.

## Approved evidence and reveal order

The present crew must enter without already knowing the commanders' identities. Cinematic 07's familiar layout and access response are clues. In Gameplay 07, recognition grows into identity evidence; the final record confirms the conclusion. Do not postpone all recognition to that last sentence, or reveal it through a speaker tag at the first console.

| Order | Existing hook / location | Exact approved recording | Player-facing identity |
|---|---|---|---|
| 1 | `A4_Recording_Hangar`, hangar | “Keep the recovery teams on the Core search.” | **UNKNOWN**. Future crew Kato's voice is filtered; neither subtitle nor HUD may name Kato/future crew yet. |
| 2 | `A4_Recording_Corridor`, arena entrance | “Bring every recovery team back.” | Future crew Moss becomes recognisable through the discovery; use **MOSS (RECORDED)** once identified. |
| 3 | `A4_Recording_Barracks`, arena exit / archive beat | “Recover the Core. Protect the younger Triad.” | Future crew Wren and the authorisation evidence establish older-self command. Use **WREN (RECORDED)** once identified. |
| 3a | Archive comparison; runtime hook not established by this baseline | “Same construction.” | A clear **KATO (RECORDED)** source after identity evidence, before final Core confirmation. Not a second unexplained Cinder echo. |
| 4 | `A4_Recording_Core`, after Warden, before C08/reactor | “We are you, from the future. We came back for the Origin Core.” | **KATO (RECORDED)**; explicit confirmation. |

These four main recordings/order, the archive comparison, and recorded/live naming are approved in the Story index. Present crew radio retains plain pilot names. Keep the first playback UNKNOWN until discovery; do not retrospectively reveal it early. Tests may be queued safely in existing rooms; IDs containing `Corridor` or `Barracks` do not require new rooms.

### C08 warning and confrontation reference

After the final record, the recovered opening event plays in this exact callback order:

1. `F_FutureKato_Signatures`: “All three signatures confirmed. It’s us.”
2. `F_FutureMoss_Where`: “They’re exactly where we were.”
3. `F_FutureWren_Channel`: “Open a channel. Now.”
4. `F_FutureKato_Interference`: “Temporal interference. Less than a second.”
5. `F_FutureWren_Dont`: “Don’t—” — the **identical processed asset** used by `O_Unknown_Dont`, through `SharedSounds[Dont]`.

The complete intended warning is separate recovered context: “Don’t engage the Dominion. You don’t understand what happens if you do.” It must not extend the opening's transmitted fragment or imply the present crew originally heard the whole warning. No new live dialogue with the future crew is implied by a recording.

Then check the approved exchange, in order. These are event descriptions, not invented source IDs:

| Cue | Label | Exact approved text |
|---|---|---|
| Archive case begins | KATO (RECORDED) | “The Origin Core might let us change one moment in the past—before we lost everything.” |
| Next recording | WREN (RECORDED) | “We believe recovering it can prevent the catastrophe. It’s the last chance we have.” |
| Recorded case closes | MOSS (RECORDED) | “We came back to save you.” |
| Present crew checks the warning | KATO | “I can’t verify a safe shutdown. Continuing could put the whole system at risk.” |
| Present crew response | MOSS | “Then we can’t let them finish.” |
| Last order before combat | WREN | “Take out the reactor. Keep the route to the hangar clear.” |

Screen-only readout: **`REACTOR WARNING: Unverified Safety Shutdown...`**. Do not silently rewrite it or add it as another spoken line. The ancient Origin Core and the ship's destructible reactor remain distinct. The future crew's plea is belief, not proof; no line or objective settles which crew causes/prevents the catastrophe. All six wording approvals are in the Story index; live Wren is also recorded in board N-046 / `2ebbfeca`. Voice creation is assigned to Ryan's team. The rejected Microsoft Zira preview is not an approved production asset.

## Test cases

### IR-01 Docking and selected-pilot variants

- Start from each pilot's pre-docking checkpoint. Select **Rift/Wren**, **Lancer/Kato**, then **Bastion/Moss** on separate passes; record the active ship immediately before commitment. At least the first run uses normal gameplay entry.
- Commit to docking once. Observe control/camera transition, collision, lighting, arrival objective and the safe on-foot spawn. The camera follows the ship actually landed; no forced switch to Rift. Confirm which pilot the build represents on foot from available identity/telemetry, not from a generic pawn mesh alone.
- Verify no hostile damage during uncontrollable arrival, no duplicate docking/arrival transmission, no walking below/through the deck, no blocked first doorway. Do not require three on-foot bodies or a specific wingman parking arrangement. A mismatch in intended versus implemented pilot is a failure to route to T4/T5, not permission to change the fiction.

### IR-02 Clues and first console

- Before using a console, inspect speaker labels, door text and objective/HUD for an early “future” or named recorded-speaker leak. `A4_DoorsKnowUs` is a clue at `Obj_DI_Corridor` step 0; it must not announce future command identity.
- Use the Hangar console. Verify the first approved line, UNKNOWN attribution, filtered intelligibility, and once-only interaction. Re-press/hold interact: no overlapping repeats. A lit screen alone is insufficient evidence of audible/visible playback.
- Capture subtitles at the tested resolution. Check contrast over the actual background, safe margins, complete text, legible duration and no overlap with objectives/slates. Test subtitles on and off in separate passes; with voices unavailable, report that the subtitle-only fallback still lacks final VO acceptance.

### IR-03 Interior combat and archive progression

- Traverse `Obj_DI_Corridor` into `Obj_DI_Arena` using ordinary movement/blade/block/deflection. Test route readability, collision, hazard telegraphs, cover and visible feedback; no ship power or extra equipment gate is required.
- Try the two arena consoles while enemies can attack, then after the encounter clears. Record the actual behavior. Approved listening must not compete with unavoidable damage: a safe playback window or encounter gate is acceptable within existing rooms; a defenseless playback in live fire fails acceptance.
- Follow the recording order above. Verify the second recording's recognisable Moss, then Wren/archive authorisations. If a later console can be consumed first, determine whether the build preserves the approved order or skips essential context. Do not manually fire the desired sequence to conceal this issue.
- At the security/archive beat, capture the identity text and recognition response. If the three-pulse cue is present, compare its pattern with opening/Verdance evidence: three short identical evenly spaced pulses, realistic equipment character, corresponding visual pulses where used. Exact milliseconds/final sound are not approved; approximately 0.6 seconds was a proposed reference, not a pass threshold. If the cue or evidence is unavailable, record pending integration/listening approval.
- After the identities are established, check the clear older-Kato “Same construction.” comparison. No new Cinder occurrence and no replay of the opening warning here. Record the actual runtime ID if implemented.

### IR-04 Warden and mandatory final-record gate

- Finish `Obj_DI_Warden`. The link/core-approach path must be readable and safe enough to access the final record. Warden difficulty/take-mercy behavior is engineering-owned; record issues without changing it.
- **Negative test:** attempt to enter the reactor, damage a conduit/heart, or start its objective without accessing the final record. Approved behavior blocks premature combat and preserves the required evidence opportunity. Baseline optional-console behavior must be reported as an integration failure, not accepted because the reactor is beatable.
- Access `A4_Recording_Core` once. Verify explicit confirmation, correct Kato recorded tag, and entry into the recovered warning/C08 before damage is possible. Repeated interaction must not duplicate the queue or unlock twice. No countdown or victory is triggered.
- Check the confirmed defender handoff on a build containing Ryan's later decision. The pair excludes the pilot who boarded: Wren boards → future crew Kato and Moss; Kato boards → future crew Wren and Moss; Moss boards → future crew Wren and Kato. The playable character remains the present crew pilot. Record the actual pair, model/voice attribution and implemented mode. Drones are an authorised fallback, not a future identity reveal; label that run fallback and do not count it as proof the future-pilot pair works.
- The sealed-threshold recordings precede the defender encounter with the player still in control. Attempt to aggro or damage defenders before playback/skip completion: no encounter may bypass the warning/decision. After release, verify no soft lock, duplicate spawn or wrong-pilot identity. Combat balance and each defender's full fall belong to the reactor encounter test; record the handoff here. No new engage/fall line is approved by this script.

### IR-05 Full C08 playback with safe movement

- Listen without skipping. Walk, turn, approach the held exit and attempt normal interactions during playback. Remain safe; no unavoidable attack, unintended travel, soft lock or movement reversal. Confirm actual behavior, not just subtitles in a debug audition.
- Verify the callback order, identical shared fragment, separate full warning, six approved case/response lines and exact screen-only caption. Current source defaults may differ; record wrong text/tag/speaker as a finding. Confirm the Core is not called the reactor target.
- Judge speech intelligibility under music/room tone, queue spacing, lost syllables, doubled voice, clipping/harshness and whether future-crew recordings sound distinct from present-crew comms without comic or retro treatment. Capture the exact file/revision and timecode; subjective approval belongs to Nicholas.
- Only after completion may the threshold release once into the confirmed defender encounter (or authorised drone fallback). Record the actual new event IDs; none is established by this source baseline. Verify reactor damage cannot pre-empt that encounter's implemented gate. The following reactor objective is `Obj_DI_Core`, approved text “Break the reactor conduits”, with reactor undamaged and no escape timer at entry. Do not invent an extra boss phase or silently remove Ryan's approved defenders. Full fight/end/escape outcome is outside this test.

### IR-06 C08 skip and rapid input

- On fresh isolated checkpoints, use the build's supported skip control near the beginning, during the shared warning, and during a later confrontation line. Record the actual input and time; a console close, subtitle disable or pause is not automatically a skip feature.
- Each skip must apply essential evidence/decision state once, end pending scene audio cleanly, restore the same safe gameplay endpoint and release the defender encounter once. It must not grant defender defeat, conduit destruction, reactor completion, escape, rewards or victory. No remaining recording may speak over newly unavoidable combat.
- Repeated/held skip plus interact must not stack duplicate playback, teleport past the reactor, or leave a sealed door with a consumed single-use console. If no supported skip exists in the tested build, report pending integration and do not claim this case passed.

### IR-07 CUT retakes and interrupted playback

- Die once in the corridor, arena and Warden encounter, using normal gameplay. Record scene/take, checkpoint position, enemies/doors and already-heard evidence after CUT. Completed rooms and recordings must not replay unnecessarily; progression and prior planetary rewards must remain coherent. Do not re-run Cinematic 07 on an interior death.
- Verify retake recovery retains the selected pilot and does not reset the reactor or escape to a different act. Current `ExecuteRetake` retains played transmission IDs; the room checkpoint/encounter must agree with that state.
- On a disposable run, interrupt a queued essential record through an available death/retake path. If safe C08 makes death impossible, keep that as a successful safety observation and use IR-08 for queue interruption. Do not require an artificial damage hazard inside the approved safe scene.
- Critical risk to check: `MarkPlayed` occurs at queue time in the baseline. A CUT/restore must not consume the only route to essential evidence that never aired, or leave a final-record gate closed with no usable console. Any unavailable recovery is a narrative progression failure, even when the played-ID list looks complete.

### IR-08 Save, quit and Continue

Use a test profile and the actual menu/quit flow, without resetting story IDs between steps. Capture both the before-quit state and Continue result:

| Save/quit point | Required observation on Continue |
|---|---|
| Fresh hangar before first console | Safe entry into unfinished interior, correct pilot, no future identity granted or evidence skipped. |
| After Hangar / after archive evidence | Completed encounter/record state retained, no repeat arrival or earlier mandatory fights; later context remains reachable. |
| Immediately after requesting a record while it is queued, and separately while it is speaking | Record whether the save persists queued/played state. Essential context cannot be silently lost or a mandatory gate become unrecoverable. |
| After Warden, before final record | Warden stays defeated under the checkpoint policy; final record remains accessible before combat. |
| During C08 / after skip / after full completion | No repeated fight/arrival, duplicate sequence or consumed-console soft lock. Essential story state, door state and current objective agree. A completed/skipped C08 does not replay by accident; defenders/fallback do not spawn twice or become silently defeated. |

The baseline has no demonstrated full record-replay UI or explicit reveal-state gate. Do not promise one. If the current build cannot recover an interrupted essential record, report the precise failure and owner; engineering must supply a safe policy. Do not declare a solution or new replay feature in this test script.

### IR-09 Readability, audio and handoff regression

- Review key UNKNOWN, identification, Core confirmation, readout and last-order frames at 1280×720, 1920×1080 and one supported ultrawide resolution (record exact size). Actual gameplay captures at those sizes are needed; resized screenshots are not separate resolution tests.
- Observe controller/keyboard prompt consistency, subtitle size/contrast, speaker distinction, HUD/objective overlap, pause/resume audio and one decision at a time. Record any unavailable accessibility setting rather than pretending it was tested.
- Music must not mask the recordings; note any delivered interior music transitions/ducking. Do not demand a new fade value or audio asset not in the tested contract. Do not approve the rejected Zira audition by absence of technical clipping.
- At the defender/reactor handoff, verify the approach distortion field, four external generators and Cinder tasks remain completed; no new ability/reward or repeat planetary objective is granted. The fight's internal conduits are distinct from the four external generators. Record whether the approved sweeping laser is present and readable before it hits; detailed damage/balance belongs to the reactor test. Do not fail the act for absent unapproved ring timing, acceleration or perfect-deflect-only gates.
- Final narrative check: player can explain that future crew commands the Dominion, seeks the Origin Core and believes recovery can avert catastrophe; present crew fears continuing and cannot verify safe shutdown. Neither knows whether reactor destruction causes or prevents the disaster. Do not require an unapproved new warning, reactor malfunction, precise future date or new exposition to make the test pass.

## Timing and practical run plan

Planning allowance, not measured performance or a new campaign budget: **60–90 minutes** for three selected-pilot passes, adverse-path retries/Continue and readability captures. If time is limited, first complete one full natural docking-to-reactor pass, then label incomplete variant/skip/persistence cases NOT RUN rather than passing the entire act.

Record actual elapsed times separately for docking/arrival, hangar–arena, arena–Warden, final record, C08 (full and skipped), and total to reactor handoff. The earlier story reference suggested a 35–45 second C07 and 60–80 second expanded C08; approved reduced console staging must be measured on the current build, not forced to those estimates. Quote observed timings and any overall budget issue for Ryan. Do not speed up/omit approved evidence to satisfy this script's scheduling allowance.

## Result packet for Lead and T7

Supply one case table: `case / build / pilot / input / resolution / setup(normal or accelerated) / result / expected / observed / timecode-log / owner`. Include untouched pre-test save copies where permitted, actual captures/logs, measured stage times, source revision differences, and specific unrun cases. Keep generated media/logs in the approved local Saved/Builds evidence area; this file is the script, not a result ledger. Lead records the run in DEVLOG and routes findings to the owning lane.

- **BLOCKER:** cannot progress/Continue, essential identity or decision permanently missed, unsafe mandatory playback, reveal bypass into reactor, or a premature state/reward/escape transition.
- **MAJOR:** wrong approved recording/identity order or pilot, early revealing label, lost/duplicated mandatory audio, unusable skip, missing callback, wrong shared warning asset, unreadable essential text, or material audio masking. Distinguish missing integration from a regression, but keep acceptance failed until corrected and retested.
- **MINOR:** non-blocking polish with a concrete observed effect. Keep subjective performance/art approval separate from technical health.

Exit requires natural progression plus demonstrated gates, full playback/skip, all three pilot variants, retake/Continue and readable evidence. Any pending integration, missing final voice, untested platform/resolution or human listening gate must be listed. Baseline playability is not a narrative-complete or presentation-approved verdict. This authoring task ran no game and left no Unreal processes running.
