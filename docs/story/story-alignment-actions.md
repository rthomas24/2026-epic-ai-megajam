# Story alignment (cc8c31d): actions for the Lead

Source: Nicholas's commit `cc8c31d` on `origin/main` (ten documents revised, two new, `Story/README.md` rewritten),
read against `storyline.md` §1-22, `features.md`, `docs/story/*.md`, `dialogs.md` and the story code on `origin/main`.
Nothing in the repo was changed. The matching `storyline.md` text is in
`C:\Users\ryant\AppData\Local\Temp\claude\8b6ffadc90d86d5d\0915cc49-a623-444a-8b28-4f546c72c7cd\scratchpad\storyline_23_plus.md`.

**What I checked and what I did not.** Read in full: the README, all twelve documents, the text diffs of the ten
revised ones, `storyline.md`, the three `docs/story` files, the story test. Looked at in code: where the `A2_`, `A3_`
and `A4_` lines fire, the wave 5 flag, the Dominion's field check, the four planet objective titles. Not checked: what
T1-T5 have built since `origin/main` (their worktrees), whether wave 5 can be cleared before Cinder is finished, what
the HUD shows during the assault, the docking and interior code. Hours are rough guesses, as in the two earlier scope
lists. Transmission ids marked "suggested" are ours.

**The headline.** Nicholas accepted both reduced scopes and Ryan's field rule unchanged. Most of this list is wording.
Three things are more than wording: the Verdance reward order (a2), the field / shield split in the assault lines
(a16), and the story test's idea of where the reveal starts (b).

## (a) Changes engineering must now make

### Verdance
| # | Owner | Change |
|---|---|---|
| a1 | T3 | `A2_Voices`: placeholder text becomes the one fragment in Cinematic 03, "Maintain the sequence" (Unknown, Static). It now plays in Cinematic 03 at the station's relay, when the device is connected, not as a separate beat |
| a2 | T5 (Ryan to confirm, see c1) | Rewards and order per Gameplay 02 / 03: picking up the device completes objective 1 and awards **Gravity Well**; connecting it to the station console triggers Cinematic 03; the 60-90 s transfer defence is "the relay copy" and awards **Cloak**; then extraction. §18 had the transfer on objective 1 and the relay as a separate objective |
| a3 | T3 | Cinematic 03 lines (suggested `C3_*`): Kato's second line is now "Those are human voices. Buried in their signal. And this shape beneath the station is artificial." Length 40-60 s, beats at 0:12 / 0:30 / 0:45 / 1:00 |
| a4 | T3 | Gameplay 03 (suggested `G3_*`): Kato's last line is "Device and recordings secured. I've got the Cinder coordinates. Let's move." Cinematic 04 as transmissions over take-off (suggested `C4_*`) with the four changed lines: Moss "All that for a recorder full of voices. None of them answering."; Wren "They wanted that device and its records. We need to know why."; Kato "...Cinder's mining complex has matching readings. They're extracting stabilising crystals there."; Command "Proceed to Cinder. Investigate the mining complex and recover the stabilising crystals. Compare them with the signal-analysis records. Maintain reconnaissance priority." Drop any shielding wording |

Objective 1's title in code already reads "Recover the Signal-Analysis Device": no change. After the cutscene the
objective line is "Protect the data transfer".

### Cinder
| # | Owner | Change |
|---|---|---|
| a5 | T5 | Stop firing `A3_EchoWren`, `A3_EchoKato`, `A3_EchoMoss` (three `Beat(...)` calls in `Ground/Interior/TDDominionInteriorCourse.inl`, about lines 507-509). README: the single terminal echo "replaces the approach-route echo and all three engineering `A3_Echo*` placeholders" |
| a6 | T3 | Remove the three `A3_Echo*` entries from the table, from `docs/story/transmissions.md` and from the test's `Required` list |
| a7 | T5, T3 | **No echo on the route to the station** (§22 had one). The only echo is at the terminal |
| a8 | T3 | Cinematic 05 at the terminal, 40-50 s, in this order (suggested ids): `C5_Kato_SameConstruction` Kato "Different worlds. Same construction. They knew where to dig."; `C5_Unknown_SameConstruction` Unknown, Echo, heavily processed, "Same construction."; `C5_Kato_Feedback` Kato "Could be feedback. Keep the recording."; `C5_Kato_OriginCore` Kato "The records call it the Origin Core. These look like components—or keys—to a temporal device."; Moss "So we take a sample and get out?"; Wren "First we find out what's guarding it."; `C5_Kato_Anchor` Kato "The anchor is blocking the approach. We need the samples, then we shut it down."; Command "Recover the samples and preserve the survey data. Confirm a passage before approaching the Dominion."; Wren "We've got our route. Triad, move." Removed from the old draft: Wren "And the survey records gave them the coordinates.", Kato "These crystals hold a stable response...", Moss "Was that you?" Set a "recording kept" and an "Origin Core named" story flag; a skip sets both once |
| a9 | T3 | `A3_CrystalsFound` (fires when the excavation objective completes): speaker stays Kato, text becomes "Samples secured. EMP prototype recovered. Lancer's pulse is online." The old text ("They were digging a way through it") is wrong now. Keeping the id avoids touching the trigger |
| a10 | T3 | `A3_PassageOpen` (fires when the anchor objective completes): speaker Control → **Kato**, text becomes "The anchor is down. The approach is prepared; passage is still blocked." The old text ("There's a passage through the field. Go.") now contradicts the story. Consider renaming the id to `A3_AnchorDown`; if renamed, update the trigger and the test |
| a11 | T5 | Objective and marker text, in order: "Investigate the mining complex" → "Access the survey records" (terminal, offered only out of combat) → "Secure the excavation" → "Recover the stabilising crystals" → "Disable the field anchor" → "Return to extraction". In code today: "Stop the Excavation", "Destroy the Field Anchor" |
| a12 | T5 | The cache after the Warden: **one** interaction gives the crystals and the EMP prototype; both saved together; the reward prompt must not block the player while exposed. No on-foot EMP, no installation step |
| a13 | T2 | The HUD marker at anchor shutdown is a **pending** approach marker toward the Dominion ("passage still pending"), not an open corridor. It becomes "open" when wave 5 clears |
| a14 | T3 | Gameplay 04 / 05 radio (suggested `G4_*`, `G5_*`): unchanged draft lines except the two Kato lines in a9 and a10 |

### Wave 5, the field and the assault
| # | Owner | Change |
|---|---|---|
| a15 | T4 | **Field rule: no change in substance.** README: "Cinder complete plus wave 5 cleared opens the approach distortion field." Code opens the field on `bDominionRevealed`, set by wave 5's clear. Confirm wave 5 cannot be cleared while Cinder is unfinished; if it can, add the Cinder condition. Fix the comment in `TDDominion.cpp` ("unreachable, until Cinder's anchor falls") |
| a16 | T3, T4 | **Field is not shield.** The four generators hold the Dominion's "separate defensive shield"; the field is already open when the assault starts. Reword: `A4_GeneratorDown1` "...The field is flickering.", `A4_GeneratorDown2` "...The field is losing shape.", `A4_GeneratorDown4` "...The field is gone." Draft replacements from Gameplay 06: Kato "Generator down. Their shield is weakening." (per kill) and Command "Dominion shield collapsed. Docking approach marked." (all four). Same for `dialogs.md` `Bark_Dom_FieldDown` ("The field is down. The bay is open.") and the "field generator" wording of `Bark_Dom_GeneratorHit / _GeneratorDown` |
| a17 | T3 | Cinematic 06 as transmissions (suggested `C6_*`), fired over the Cinder take-off: Moss "Samples aboard. I'm ready for somewhere with fewer volcanoes."; Kato "The anchor is down. The approach is prepared. We still have to clear the guard line."; Kato "The crystals can stabilise the approach. Four generators are still sustaining its defensive shield."; Wren "Then we take down the generators and find a way inside."; Command "They're collecting parts of the Origin Core. We need to know why. Disable the outer defences and secure access. Keep your channels recording."; Wren "Understood. We find out what they intend to do with it." Then, as wave 5 forms up: Moss "They've put a whole fleet across our route."; Wren "Stay together. Clear the guard line, then the generators." Never replayed on a retake (already how transmissions work) |
| a18 | T2, T4 | Objective line on wave 5: **"Break the Dominion guard"**. Then a persistent generator count "0 / 4 ... 4 / 4" that agrees with the saved state after a wipe, then a docking line and marker. Placeholders for the two unnamed ones (ours): "Destroy the shield generators 0 / 4", "Dock with the Dominion". ENG-18's ledger still lists "HUD wave block during the assault" as open |
| a19 | T3 | Gameplay 06 radio (suggested `G6_*`): Wren "Clear the carriers. We need that approach."; Kato "Four generators. I'm marking the live targets." (when the targets are revealed); Moss "Those turrets are charging. Keep moving."; Wren "Keep that bay covered. We need a way back out." (at docking) |
| a20 | T4, T3, T2 | **No line, objective or card before docking may say "destroy the Dominion" or "destroy the reactor".** The mission is access: "Command does not order the ship destroyed here"; the generators are "an access decision"; with boarding on, nothing starts a reactor countdown or victory at 4/4. Check objective text, barks, the slate card and Control's lines for the assault |
| a21 | T4 | Reveal the four generator targets when the assault becomes active, with a clear sign that the bay is still protected; destroyed targets lose their markers |
| a22 | T4 | Docking: needs 4/4; from commitment to the transition no unavoidable damage; save the transfer state; the transition fires once; wingmen stay outside; never wait for three ships |
| a23 | T3 | The generator money shot must hand control back before a turret volley can land and must not cover the next objective marker |
| a24 | T4 | Tell Nicholas what is built for two things he asked engineering to confirm: does EMP Pulse affect a generator or its turrets; does the afterburner ram damage hull fixtures. Neither may be required |
| a25 | T4 | Start wave 5 "outside immediate unavoidable fire", with the Dominion in view; save when wave 5 is cleared, before the assault |

### Documents and ledgers
| # | Owner | Change |
|---|---|---|
| a26 | Lead | Append §23-25 to `storyline.md`; edit §5 acts 3 and 4, §6 and §8 as listed in §25.3; update `features.md` ("Cinder in the story": no route echo, pending marker; ENG-18's "the four shield generators are that field"; ENG-26's story line); `docs/story/transmissions.md`; `dialogs.md` |
| a27 | Lead | Post on the board / issue #29: the unanswered list of §25.2 (COMMAND or Control; Null Armada; the three-pulse definition; the interior document; the fallback reveal), plus c1 once Ryan has decided |

## (b) The story test

**Does "a suspected temporal device" fail the test today? No.** `TDStoryTests.cpp` rejects, in every entry whose id
does not start with `A4_`, `A5_` or `F_`: a Future speaker, the substring `future`, the substring ` time` (with the
leading space), and `it's us`. "These look like components—or keys—to a temporal device." contains none of them
("temporal" is not " time"). I also searched every draft line in the twelve documents: none before the finale
contains " time", "future" or "it's us" ("Time to go." is gone). The test was not run; this is from reading it.

**So the code passes, but three things around it are now wrong or too loose:**

1. **The written rule is stricter than Nicholas's decision.** `storyline.md` §8 ("Before act 4 nobody on the squad
   says 'future', 'time' or 'us'") and the test's comment should now read: before the Dominion **interior**, nobody
   says "future" or "time", nobody says "it's us", no Future speaker talks; **one approved exception:** Kato's Origin
   Core line at the Cinder terminal says "a temporal device". Nicholas's boundary, from the README: "Cinder supplies
   the name Origin Core and a suspected temporal purpose... Only aboard the Dominion does Triad identify its
   commanders as its future selves."

2. **The test's "late" set starts too early.** Every `A4_` id is exempt, and the generator lines are `A4_`. On the
   boarding route the whole assault (wave 5, generators, docking) is **before** the reveal. What the test should do:
   - Treat as after the reveal only: the interior recordings and consoles (`A4_Recording_*`, `A4_DoorsKnowUs`), the
     command-deck payoff (`F_*`), act 5 (`A5_*`), and any fallback-only assault reveal ids (give those their own
     prefix, for example `A4F_*`, fired only with `td.Finale.Boarding 0`).
   - Check everything else, including `A4_GeneratorDown*` and the new `C5_*`, `G5_*`, `C6_*`, `G6_*` ids, with the
     existing three checks.

3. **"temporal" should be allowed in exactly one place, not everywhere.** Suggested tightening, so the approved hint
   cannot spread:
   - Reject `temporal` before the reveal except in the one id that carries Kato's terminal line (suggested
     `C5_Kato_OriginCore`).
   - Reject `origin core` in anything that plays before that line (ids starting `Intro_`, `O_`, `TB_`, `S2_`, `A1_`,
     `A2_`, `G2_`, `C3_`, `G3_`, `C4_`, `G4_`): the README says Verdance yields "the signal-analysis device and
     records, not the buried structure or Origin Core".
   - Add to the pre-reveal forbidden list (ours, for Nicholas to agree): `older`, `ourselves`, `our own voice`,
     `from the future`, `years from`.
   - The echo line (`C5_Unknown_SameConstruction`) must have speaker Unknown: already covered by the Future-speaker
     check; add an explicit assert that its text equals the tail of Kato's line before it, so it stays an echo.
   - Keep: no shared `Dont` sound outside `O_Unknown_Dont` and `F_FutureWren_Dont` (today only `S2_` is checked;
     extend to every pre-reveal id, since the documents reserve "Don't-" for the finale in each of them).

**Also needed in the test when (a) lands:** remove `A3_EchoWren`, `A3_EchoKato`, `A3_EchoMoss` from `Required`; add the
new required ids (`C5_Kato_SameConstruction`, `C5_Unknown_SameConstruction`, `C5_Kato_Feedback`, `C5_Kato_OriginCore`);
`A3_CrystalsFound` and `A3_PassageOpen` stay if their ids are kept.

**One caution for Nicholas, not a test matter:** Gameplay 06's "less distorted" transmissions with "familiar call
signs" come after the temporal hint and the echo. They are the most likely place for a line to cross the boundary.
The test in (2) is what guards them.

## (c) New scope questions for Ryan

### c1. Verdance: where the transfer defence sits, and which reward it gives
- **Asks:** device pick-up → Gravity Well; connect to the console → Cinematic 03; the 60-90 s transfer defence is "the
  relay copy" → Cloak; extraction.
- **Exists:** objective 1 (device; §18 stretched its hold-interact into the transfer defence; Gravity Well; carry
  back) and objective 2, a separate climb to an ancient relay with three resonance nodes (Cloak). Both built.
- **Cost:** follow Nicholas exactly, dropping the relay climb: 3-5 h (T5) and it throws away built, working content.
  Keep both objectives and only re-order the telling: 1-2 h (T5, T3).
- **Recommend: fits in a reduced form.** Keep both built objectives. Objective 1: pick up the device (Gravity Well as
  now, at the pick-up or at the end of the objective, T5's choice), connect it at the station console, Cinematic 03,
  then the transfer defence and carry-back. Objective 2 stays the climb, told as "bringing the station's relay
  uplink back so the copy can be sent" (*our wording for Nicholas to replace*), Cloak on completion. Tell Nicholas the
  one difference: Cloak comes from the relay climb, not from the defence. If Ryan prefers the documents' order, the
  defence moves to the top of the relay climb.

### c2. Cinematic 06: eight lines where "two or three transmissions" were planned
- **Asks:** the document itself says transmissions over the take-off, but drafts eight lines in four beats, a tactical
  graphic with four generator markers, and a targeting sweep.
- **Exists:** the take-off, transmissions, wave 5.
- **Cost:** all eight as transmissions in two groups: 1-2 h (T3). A graphic: 2-3 h (T2). A shot sequence: 3-5 h (T3).
- **Recommend: fits in a reduced form.** Six lines over the take-off, two as wave 5 forms; no graphic (the generator
  markers appear on the HUD when the assault starts); no new shots. Not in the single cutscene pass.

### c3. Assault length: 5-7 minutes including wave 5, and the total
- **Asks:** a working target, "subject to balance and the full campaign budget".
- **Exists:** wave 5 and the assault, not timed end to end (ENG-18: "whole assault to victory not played").
- **Sum of the adopted budgets (ours):** opening 2:20, tutorial battle 6-10, Aftermath about 1:30, Verdance 8-10,
  waves 3-4, Cinder 6-8, assault 5-7, then the interior, core fight, 90 s run and 60 s flight. That is past 30 minutes
  before the interior starts.
- **Cost:** 0 to decide; 1 h (T4) to time wave 5 plus four generators.
- **Recommend:** accept 5-7 min here and tell Nicholas the game will run 35-40 min unless the tutorial battle is held
  to about 6. If the assault runs long, lower generator health before cutting anything he wrote.

### c4. Clue transmissions and the friendly-status flicker during the generators
- **Asks:** "short transmissions become less distorted: familiar call signs, phrasing or a partial authentication
  tone, with no identified speaker. A brief friendly-status flicker reinforces the mystery."
- **Exists:** one transmission per generator kill; Static and Echo styles; nothing hostile-voiced; no flicker.
- **Cost:** two or three Unknown-speaker fragments on generator kills: 1-2 h (T3) plus Nicholas's words. Flicker on
  one hostile marker: 1 h (T2). The authentication tone needs the three-pulse sound (Nicholas).
- **Recommend: fits in a reduced form.** Two fragments (after the second and fourth generator), Unknown speaker,
  guarded by the test in (b). The flicker is optional and the first thing to drop if playtesters guess the twist.

### c5. EMP on generators, ram on hull fixtures
- **Asks:** engineering to confirm; "neither interaction is a completion gate".
- **Exists:** not checked by me. **Cost:** 0 to answer; 1-2 h (T4) if Ryan wants EMP to silence a generator's turrets
  for a few seconds. **Recommend: fits as is:** report what is built, add nothing before the freeze.

### c6. When the finale mode is fixed (boarding or space-only)
- **Asks:** the documents assume boarding. For the fallback: "that change is coordinated before production",
  "Coordinate the reveal through assault transmissions and document the reactor sequence separately."
- **Exists:** both modes behind `td.Finale.Boarding`; §5's fallback paragraph; placeholders `F_*` and `A4_Recording_*`.
- **Cost:** fallback reveal as transmissions on the generator kills and the reactor: 2-3 h (T3, T4), plus Nicholas's
  lines. Deciding late costs more: the assault lines differ between the modes (pre-reveal with boarding, the reveal
  itself without).
- **Recommend:** Ryan names a go / no-go time for boarding (suggest the evening of Nov 14) and tells Nicholas now, so
  he can write the fallback lines once. Engineering keeps both line sets under separate ids.

### c7. The boarding cinematic and the interior story are not written
- **Asks:** "The next cinematic opens on bay entry or hangar landing, then hands off to on-foot exploration. Its title
  and script remain to be documented." The README lists the interior, "the future-Triad identity evidence and the
  opening-warning payoff" as next.
- **Exists:** the interior being built (hangar, deflect corridor, arena, Warden gate, 2-phase core, 90 s run, escape
  flight) on our placeholders: a recording per room, doors that open to the pilot's ID, the command-deck payoff.
- **Cost:** unknown until the document arrives. A new room or mechanic would not fit.
- **Recommend:** tell Nicholas what the interior is, room by room, and ask him to write to it: one recording per
  existing room, the payoff as a recording at the core door, the boarding cinematic as one hangar shot with two or
  three lines. Give a cut-off (suggest midday Nov 14); after it the placeholders ship. **Proposed reduced form if he
  asks for more:** no new rooms, no faces, no new enemy; voices, console recordings and the reused "Don't-" sound only.
  Note for him: the built interior has a Warden gate, and his Cinder now has the Warden guarding the cache; either he
  wants two Wardens or the interior one needs another name.

### c8. "The distinction between corridor and hull-field feedback"
- **Asks:** Ryan's Lead to confirm how the player tells the approach field opening from the shield falling.
- **Exists:** one distortion effect around the far Dominion; shield sections that hide as generators die.
- **Cost:** a different colour or motion for the hull shield against the outer field: 1-2 h (effects builder). A
  visible corridor through the field: 2-4 h (§22 already said no).
- **Recommend: fits in a reduced form:** the outer field simply drops when wave 5 clears (with the pending marker
  turning to "open"); the shield is the per-section effect on the hull. No corridor geometry.

## (d) Contradictions that remain

### With sections 18 and 22
1. **Verdance rewards and order (c1).** §18: the transfer defence is objective 1's and the relay is a separate
   objective. Documents: Gravity Well at the pick-up, the defence is the relay copy and gives Cloak. The README also
   says the scopes of §18 "continue to govern delivery", so the documents contradict the section they defer to.
2. **The route echo.** §22: "One echo of a Kato line" on the way to the station. Documents: none on the route.
   Nicholas's version wins (story words); §22 and `features.md` need the edit.
3. **The marker at anchor shutdown.** §22: "A corridor marker appears on the HUD at shutdown." Documents: a prepared
   approach "with passage still pending". Same marker, different state.
4. **Shielding.** §18: "Cinder shielding: words only". Documents: removed entirely.
5. **The field rule is not a contradiction:** the documents repeat §22's rule six times.

### With sections 1-10, `features.md` and the code
6. **"The four generators are the distortion field"** (§5 act 4, §8, ENG-18, the story table in `features.md`, the
   `A4_GeneratorDown*` lines, `Bark_Dom_FieldDown`) against the documents' two separate things.
7. **"Destroying the anchor gives the passage"** (§5 act 3, `A3_PassageOpen`, the `TDDominion.cpp` comment) against
   "prepares the approach".
8. **The reveal boundary.** §8 and the test say "before act 4"; the documents say "only aboard the Dominion". The
   assault is act 4 and is before the reveal.
9. **Cinematic 03 inside the station** against the built relay as an ancient structure reached by a climb (c1).
10. **The squad's motive.** `A5_Choice` and the built finale assume the reactor is the target; the documents hold
    that back until the interior. Fine with boarding on; with boarding off the generators lead straight to the reactor.
11. **Names:** COMMAND against Control; "Dominion drones" against the Null Armada. Unchanged, unanswered.

### Inside Nicholas's own documents
12. **Not fully reduced.** Gameplay 02's table still has "scan one convoy" and "disable or bypass perimeter defenses".
    Gameplay 03's still has "a tougher machine or jamming unit... or restore local power", a midpoint checkpoint and
    "In solo or co-op". Gameplay 01 still lists "solo/co-op handling". The README says no co-op and that §18 governs,
    so these read as leftovers; build to §18.
13. **Command's line in Gameplay 02,** "Find its survey archive", beside the objective "Recover the signal-analysis
    device". Both exist in the fiction; the line names the wrong one first.
14. **Command in Cinematic 05:** "Confirm a passage before approaching the Dominion." Passage cannot be confirmed on
    Cinder any more; it stays pending until wave 5.
15. **Kato says the anchor is down twice,** in Gameplay 05 and again in Cinematic 06, within about two minutes.
16. **Cinematic 05 is 40-50 s with nine lines** and four beats; at speaking pace it is nearer 55-60 s (*our estimate*).
17. **Cinematic 06 calls itself "two or three transmissions"** and drafts eight lines.
18. **Two Wardens** if the interior keeps its Warden gate (c7).

### Twist risk
19. Clues now stack up before the interior: handshake, blue drone, "verified" marker, three pulses answered, the drone
    that pauses, human voices in the signal, the echo of Kato's words, "a temporal device", the recognising console,
    and then call signs in clearing transmissions during the assault. Each is approved or deniable alone. If
    playtesters name the twist before docking, drop in this order (ours): the friendly-status flicker, the recognising
    console on Cinder, the assault call-sign fragments. The temporal line is Nicholas's approved hint and stays.
