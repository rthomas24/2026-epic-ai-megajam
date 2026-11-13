# Story reconciliation `fbc7240` against the build and storyline.md: for the Lead

Compared: each of the eight changed files at `fbc7240~1` and `fbc7240` (text extracted from the Word files, diffed by
sentence), and `storyline.md` on `origin/main` (`7ef3321`, which now has §26-32). Code read on `lead/int` (`eb217c7`):
`Flow/TDGameInstance.h`, `Flow/TDSaveGame.h`, `Input/TDHintComponent.h`, `Director/Intro/TDIntroDirector.h`,
`Waves/TDWaveDirector.cpp`, `World/TDVerdanceApproach.*`, `World/TDGalaxyLayout.cpp`, `Story/TDStorySubsystem.cpp`.
Nothing was built, run or played. "Not found" means a text search of `Source/` found no such thing.

Short version: the commit removes work more than it adds. Three documents (06, 07, 08) only swap "next to document"
for the names of their successors. The real changes are in the tutorial (01), the Verdance arrival (02), the facility
defence (03) and Aftermath's last shot. One new requirement: the tutorial is mandatory on a first playthrough and
offers replay or skip afterwards. The game has no flag for that today.

## 1. Per document: what changed that matters to engineering

### Gameplay 01, Tutorial Battle (§13)
| Before | After |
|---|---|
| "No second cinematic is needed between the opening and Aftermath; the tutorial battle is the entire playable bridge." | "Tutorial gameplay is required on the first playthrough; subsequent playthroughs offer replay or skip into Cinematic 02 with the same completed-tutorial state. Cinematic replay and death retries remain separate. The tutorial is the playable bridge into Aftermath." |
| Phase 03: "Introduce the squad-selection/switching interface if available." | "Introduce instant switching among Rift, Lancer and Bastion, with AI wingmen controlling the other two ships." |
| Phase 03 ends: "Demonstrate each available signature ability or finish its equivalent squad-order tutorial." | "Teach switching and the available starting powers; final demonstration gates follow confirmed gameplay support." |
| Phase 04 ends: "Drive off or disable the heavy craft; exact enemy and health values TBD." | "Heavy-craft resolution remains pending: destroy the gunship before withdrawal, or withdraw while it remains active. Do not assume a mandatory kill until Nicholas decides." |
| "HUD: DISABLE HEAVY CONTACT" | "HUD: HEAVY CONTACT (final objective depends on the pending gunship decision)" |
| "Multiplayer / solo: The narrative assumes a three-ship squad, not necessarily three human players. Tutorials should work whether allied ships are AI-controlled or player-controlled. Exact switching and co-op implementation must be confirmed with the gameplay team." | "Solo squad: One player switches instantly among three ships; AI wingmen fly the other two. Tune teaching and progress gates to one active player. No human co-op or simultaneous three-player action is required." |
| Confirm list: "...ship-switching versus ability-command UX; which starting powers are ready for tutorial demonstrations; heavy-craft asset and win condition; solo/co-op handling; checkpoint support; expected run time; whether Verdance transition uses orbital combat or direct insertion." | "Actual movement/control bindings and switching prompts; starting-power demonstration gates; heavy-craft asset and whether its destruction is required before Command orders withdrawal; checkpoint support; final tutorial run time. Opening-cinematic duration is a separate pending decision. Verdance uses the adopted compact arrival and insertion, without an added orbital battle." |

What it means: the design now states the game as built (solo, instant switching, AI wingmen, starting powers). Two
things are new: the first-play rule, and the heavy-craft phase has **no settled win condition**.

### Cinematic 02, Aftermath (§12)
| Before | After |
|---|---|
| "MATCH CUT from the jump effect to the next playable sequence: approach to Verdance, orbital combat, or planetary insertion depending on the implemented mission." | "MATCH CUT from the jump effect into Gameplay 02: the compact Verdance approach and planetary insertion. Show the enemy transport traffic as a clue; no separate orbital combat encounter is required." |

README adds for this hand-off: Aftermath begins when "Triad completes withdrawal and the pursuers stop at the
perimeter", and exits to "Gameplay 02 without an added orbital battle".

### Gameplay 02, Verdance Arrival (§14)
| Before | After |
|---|---|
| Phase 1: "Fly through Verdance approach corridor; scan hostile traffic. Most drones ignore Triad and ferry equipment toward the surface." Ends: "Reach designated insertion sector; scan one convoy." | "Fly the compact Verdance approach; observe enemy traffic carrying equipment toward the surface. No separate scanning mechanic is required." Ends: "Reach the designated insertion sector; traffic is an environmental clue, not a scan objective." |
| Phase 2: "Evade or defeat a small interception force while protecting a safe landing path." Ends: "Landing zone clear; squad reaches drop point." | "Follow the marked descent and landing path. Keep resistance on the adopted ground route; no added orbital interception encounter is required." Ends: "Reach the landing point and transfer safely to ground control." |
| "Use the adopted compact route and an open-sided survey station re-dressed from the research site; a walk-in facility is not required." | "Use the adopted compact route and open-sided survey station; a walk-in facility is not required." |
| "Avoid requiring ship switching, co-op, advanced stealth, or unique traversal abilities unless already confirmed working. Solo play must not depend on three humans." | "Flight uses the established solo squad and ship switching. Ground progress requires no ship power, advanced stealth or human co-op." |

What it means: the convoy scan and the landing interception are no longer asked for. The convoys stay as something
to see.

### Gameplay 03, Facility Defense (§16)
| Before | After |
|---|---|
| "The transfer can use a flexible progress timer, but it should not be an arbitrary long wait. In solo or co-op, enemy pressure must scale to actual players. If AI partners cannot meaningfully defend, tune the arena to one active player. An emergency reboot interaction is an optional simple mechanic, not a confirmed feature." | "Keep the transfer within the adopted 60-90 second, three-wave defence. Tune pressure to one active player and the available AI support; progress never requires human co-op. An emergency reboot interaction remains optional, not a confirmed feature." |

What it means: the reduced form of §18 and §25 (60-90 s, three waves) is now the document's own rule. Nothing new.

### Gameplay 06, The Dominion Assault (§24)
| Before | After |
|---|---|
| "...enter the boarding transition once. The next cinematic opens on bay entry or hangar landing, then hands off to on-foot exploration. Its title and script remain to be documented." | "...trigger Cinematic 07: Boarding the Dominion once. It carries the selected ship into the hangar and hands off to Gameplay 07: Inside the Dominion at the safe on-foot checkpoint." |
| "Nicholas finalises combat lines and the next scene." | "Nicholas finalises combat performances; Cinematic 07 documents the boarding scene." |

No engineering effect.

### Gameplay 07, Inside the Dominion (§27)
| Before | After |
|---|---|
| "...triggering the next cinematic. That scene will replay the opening transmission... Reactor destruction and the 90-second escape belong to the following sequence." | "...triggering Cinematic 08: The Recovered Transmission. That scene replays the opening transmission from the future commanders' perspective and develops their case for the Origin Core. Gameplay 08 then destroys the reactor; Gameplay 09 covers the escape." |
| "Next to document: the future command-deck recording, the exact opening 'Don't-'..." | "Cinematic 08: The Recovered Transmission documents the future command-deck recording, exact opening 'Don't-'..." |

No engineering effect.

### Gameplay 08, The Reactor Fight (§29)
| Before | After |
|---|---|
| "...that flight's duration and presentation are documented next." | "After the escape starts, its CUT retake belongs to Gameplay 09: Escape from the Dominion. Current behaviour restarts at the reactor-room exit facing the hangar without requiring this fight again. Reaching the hangar transfers to the separate escape flight; Gameplay 09 documents its default limit and handoff to Cinematic 09." |
| "Next to document: the 90-second return... Show temporal failure through echoes or repeats without taking away readable control. Only after the three pilots survive does the future-dated distress signal question whether they prevented or created the threat." | "Gameplay 09: Escape from the Dominion documents the 90-second return, reboarding and separate escape flight. Cinematic 09: The Paradox carries the breakup and future-dated distress signal after all three present-day pilots survive. Collapse effects preserve readable control; the ending leaves whether Triad prevented or created the threat unresolved." |

No engineering effect. The phrase "echoes or repeats" is gone from this document; Gameplay 09's own wording stands
("may appear as brief afterimages and repeated alarms or hull sounds").

### Story/README.md
- Three new rows at the top of the dependency table (Cinematic 01, Gameplay 01, Cinematic 02), quoted above where
  they matter. Gameplay 01: "Required on the first playthrough; replay or skip on subsequent playthroughs." "Whether
  gunship destruction is required remains pending."
- The last line ("Future cinematic scripts... will be stored here.") is replaced by a section, **Final narrative
  continuity review**: "All 18 sequence documents and all 17 consecutive handoffs have been checked. No remaining
  narrative handoff blockers were found after correcting stale successor references, the Verdance arrival gates and
  the solo tutorial specification."
- "Tutorial gameplay is mandatory on the first playthrough, with replay or skip offered on later playthroughs. The
  solo player switches among three ships with AI wingmen. Opening-cinematic duration (tested 49 seconds or proposed 2
  minutes 20 seconds) and whether the tutorial gunship must be destroyed before withdrawal remain decisions for
  Nicholas; neither is resolved by this review. Both gunship options preserve Command's withdrawal order and the
  Aftermath handoff."
- "This review establishes readiness of the campaign narrative draft for implementation and production coordination.
  Final dialogue performances, staging, encounter tuning, integration and human/Shipping validation remain separate."

## 2. Engineering changes

**What the code has today for "first play" (the answer to the Lead's question): no first-play or tutorial-completed
flag exists.** What exists near it:
- `UTDHintSave` (slot `TD_Hints`, a profile save separate from the run save): the list of control hints already done
  and the hints on/off setting. Each hint shows at most once per profile.
- `UTDGameInstance::bRunFromContinue`: a run resumed with Continue keeps the control lessons off.
- `bIntroPending`, and the intro's hold-to-skip (any key held 0.5 s, after the first second), available on every
  play including the first.
- The tutorial battle itself is wave 1 with an opening grace period and the hints. The five-phase mission of §13 is
  not built as phases: the five `TB_*` lines are in the transmission table and **no code fires them**; the `S2_*`
  Aftermath lines are in the table and **no fire call was found** either. So "skip into Cinematic 02 with the same
  completed-tutorial state" has, today, no Cinematic 02 trigger to skip to.

| # | Owner | Change | State |
|---|---|---|---|
| e1 | T3 (with T1 for the hint profile) | **New: a profile flag "tutorial completed".** Store it in the profile save (`TD_Hints` is the one that exists, or a small new profile slot), set when the tutorial battle ends (the withdrawal, or wave 1's clear until the withdrawal exists). Not in the run save: a New Game must not clear it | Newly asked. About 1 h |
| e2 | T3, T2 for the prompt | **New: on New Game with the flag set, offer "Play the tutorial" or "Skip".** Skip starts the run in the state the tutorial leaves: wave 1 (and whatever else is "the tutorial") cleared, its lines marked played, hints off, then Aftermath or the first free flight. With the flag clear there is no offer and no skip of the gameplay | Newly asked. 2-3 h. Depends on c1 |
| e3 | T3 | Leave the opening cinematic's hold-to-skip as it is: the rule is about tutorial **gameplay**; "Cinematic replay and death retries remain separate" | No change |
| e4 | T4 | **Verdance approach: the scan is no longer wanted as an objective.** `ATDArmadaConvoy` has a scannable lead hauler (hold within `ScanRadius` for `ScanSeconds`, ring brightens, fires `A2_ConvoyScanned`). It is already optional, has no HUD mode and gates nothing, so it does not break the new text. Recommended: keep the convoys exactly as they are; make sure nothing presents the hold as a task (no objective line, no prompt, no marker); fire Kato's observation on **proximity or sight** of the convoy instead of a 3 s hold, so every player gets the clue | Built; reduce, do not remove. 0.5 h |
| e5 | T3 | Rename or re-describe `A2_ConvoyScanned` ("Scanner assemblies. Extraction rigs. No troops. They're surveying."): the line is fine, the word "scanned" in the id and in `transmissions.md` is now wrong. Keep the id if renaming costs a test; change the description | Text only |
| e6 | T4 | **No interception force over the landing zone.** §14 listed it as new work; do not build it. If anything spawns hostile on the Verdance approach or descent, remove it or make sure it is not an "encounter" (the document: "no added orbital interception encounter is required"; resistance stays on the ground route) | Drop from the queue; T4 to confirm nothing was built |
| e7 | T2 | Do not build a scan prompt (§14 gave T2 "scan prompt"). The "verified / unknown" marker flicker stays | Drop from the queue |
| e8 | T4, T2 | **Tutorial heavy contact: do not gate anything on the gunship's death yet.** Objective text "HEAVY CONTACT", not "DISABLE HEAVY CONTACT". Build the withdrawal so that it can start either on the gunship's death or on a timer or damage threshold while it is alive (one switch), until Nicholas decides | Newly constrained. If the tutorial is still wave 1 only, nothing to change today |
| e9 | T3, T4 | Aftermath's trigger is "Triad completes withdrawal and the pursuers stop at the perimeter"; its exit is the jump into the compact Verdance approach, with the convoy traffic visible and **no** orbital fight. Today's wave 2 must not sit between Aftermath and Verdance as "an added orbital battle": it is either part of the tutorial battle or it moves (see c2) | Open: depends on c2 |
| e10 | T5 | Facility defence: confirm the built transfer is 60-90 s with three waves, tuned for one player with no co-op assumption. No new mechanic; the emergency reboot stays unbuilt | Check only |
| e11 | T1 | Nothing new. The Verdance transition is the hyperdrive jump and the ENG-38 descent ("direct insertion"), as built | No change |
| e12 | Lead | Remove from any queue: co-op handling, "squad-order tutorial" substitutes, a Verdance orbital battle | Housekeeping |

Now unnecessary because of this commit: the scan as a required action (e4), the scan prompt (e7), the landing-zone
interception (e6), any co-op consideration (e12). Newly asked: the first-play flag and the replay-or-skip offer (e1,
e2), and a withdrawal that does not depend on the gunship kill (e8).

## 3. Corrections to storyline.md (ready to paste, keyed by section)

**§12, the last row of the beat table, replace the cell "match cut from the jump to Verdance" with:**
> match cut from the jump "into Gameplay 02: the compact Verdance approach and planetary insertion. Show the enemy
> transport traffic as a clue; no separate orbital combat encounter is required" (`fbc7240`)

**§12, "What it means for the game", replace the sentence "Today's wave 2 becomes part of that battle or the approach
to Verdance (T4 and T3 propose; Nicholas confirms in issue #29)." with:**
> Since `fbc7240` nothing is fought between *Aftermath* and Verdance ("proceed into Gameplay 02 without an added
> orbital battle"), so today's wave 2 is either part of the tutorial battle or moves to after Verdance (Ryan decides;
> see the scope list). *Aftermath* starts when "Triad completes withdrawal and the pursuers stop at the perimeter".

**§13, add after the opening paragraph:**
> **Since `fbc7240`:** "Tutorial gameplay is required on the first playthrough; subsequent playthroughs offer replay or
> skip into Cinematic 02 with the same completed-tutorial state. Cinematic replay and death retries remain separate."
> The squad is defined as the game has it: "One player switches instantly among three ships; AI wingmen fly the other
> two. Tune teaching and progress gates to one active player. No human co-op or simultaneous three-player action is
> required."

**§13, replace table rows 03 and 04 with:**
> | 03 Fight as Triad | "Instant switching among Rift, Lancer and Bastion, with AI wingmen controlling the other two ships"; the starting powers | "Teach switching and the available starting powers; final demonstration gates follow confirmed gameplay support" | MOSS: "We've still got three ships. Use them." / SQUAD SYSTEMS |
> | 04 Heavy contact | Everything under pressure; the squad as a team | **Pending with Nicholas:** "destroy the gunship before withdrawal, or withdraw while it remains active. Do not assume a mandatory kill until Nicholas decides" | WREN: "That one's bigger. Focus it together!" / HEAVY CONTACT ("final objective depends on the pending gunship decision") |

**§13, replace the bullet "Open with Nicholas: phase 03 names Cloak, EMP Pulse and Gravity Well..." with:**
> - **Still with Nicholas (`fbc7240`):** whether the gunship must be destroyed before Command orders the withdrawal
>   ("Both gunship options preserve Command's withdrawal order and the Aftermath handoff"); the final tutorial run
>   time; the opening cinematic's length ("tested 49 seconds or proposed 2 minutes 20 seconds"). The powers question is
>   closed: the tutorial teaches the starting powers (§25).
> - **Not in the build:** a "tutorial completed" flag on the profile and the replay-or-skip offer on a later New Game.
>   The nearest things are the hint profile (`TD_Hints`: each control hint once per profile) and the opening
>   cinematic's hold-to-skip. The five `TB_*` lines are in the table and nothing fires them yet.

**§13, replace the last bullet ("The Verdance transition is the hyperdrive jump, then the planetary descent
(ENG-38).") with:**
> - The Verdance transition is the hyperdrive jump, then the planetary descent (ENG-38). The document now agrees:
>   "Verdance uses the adopted compact arrival and insertion, without an added orbital battle."

**§14, replace table rows 1 and 2 with:**
> | 1 Approach | "Fly the compact Verdance approach; observe enemy traffic carrying equipment toward the surface. No separate scanning mechanic is required" | "Reach the designated insertion sector; traffic is an environmental clue, not a scan objective" | Orbital clue: the craft carry scanner assemblies and extraction equipment, not troop columns; Kato notes a pattern but cannot infer the target. WREN: "That's a lot of machines for a geology expedition." |
> | 2 Landing | "Follow the marked descent and landing path. Keep resistance on the adopted ground route; no added orbital interception encounter is required" | "Reach the landing point and transfer safely to ground control" (**checkpoint**) | COMMAND: "The outpost went dark twelve hours ago. Find its survey archive. Do not destroy the facility." (placed here by us) |

**§14, "Rules from the proposal", replace "do not require ship switching, co-op, advanced stealth or unique traversal
abilities unless already confirmed working; solo play must not depend on three humans" with:**
> "Flight uses the established solo squad and ship switching. Ground progress requires no ship power, advanced stealth
> or human co-op" (`fbc7240`)

**§14, "New work if adopted", replace the first two items (the orbital beat with a "scan", and the interception
force) with:**
> convoys that ignore the squad on the approach, **seen, not scanned**: built (`ATDArmadaConvoy`, T4). Its scannable
> lead hauler is optional and gates nothing; since `fbc7240` no scan is asked for, so Kato's line should fire on
> coming near the convoy and nothing may present the hold as a task. **No interception force and no scan prompt are
> built** (both removed by `fbc7240`);

**§14, "Owners", replace "T2 scan prompt and marker flicker" with "T2 the marker flicker" and "T4 orbital convoys and
interceptors" with "T4 the convoys".**

**§16, add to "Rules from the proposal" (or replace the sentence about a flexible progress timer and solo or co-op
scaling, if present):**
> "Keep the transfer within the adopted 60-90 second, three-wave defence. Tune pressure to one active player and the
> available AI support; progress never requires human co-op. An emergency reboot interaction remains optional, not a
> confirmed feature." (`fbc7240`)

**§24, "Rules from the proposal", replace the Docking bullet's quotation "The next cinematic opens on bay entry or
hangar landing, then hands off to on-foot exploration. Its title and script remain to be documented." with:**
> "On commitment, suppress unavoidable attack pressure, save the transfer state and trigger Cinematic 07: Boarding the
> Dominion once. It carries the selected ship into the hangar and hands off to Gameplay 07: Inside the Dominion at the
> safe on-foot checkpoint." (`fbc7240`; §26)

**§24, open question 5 ("The boarding cinematic and the interior are 'next to document'..."): mark as answered:**
> 5. Answered: Cinematic 07 and Gameplay 07 are written (§26, §27).

**§27, opening paragraph, replace "triggering the next cinematic" (§28)" wording and the sentence that follows with:**
> "ends after the interior Warden encounter when the player accesses the core-side command record, triggering
> Cinematic 08: The Recovered Transmission" (§28). "Gameplay 08 then destroys the reactor; Gameplay 09 covers the
> escape."

**§27, "Rules from the proposal", the bullet "The next scene": replace its quotation with:**
> "Cinematic 08: The Recovered Transmission documents the future command-deck recording, exact opening 'Don't-' replay
> and full warning, then the opposing cases about the Core. Reuse the identical processed opening fragment; do not
> re-record it."

**§29, "Rules from the proposal", replace the bullet "Next:" with:**
> - **Next:** "Gameplay 09: Escape from the Dominion documents the 90-second return, reboarding and separate escape
>   flight. Cinematic 09: The Paradox carries the breakup and future-dated distress signal after all three present-day
>   pilots survive. Collapse effects preserve readable control; the ending leaves whether Triad prevented or created
>   the threat unresolved." (`fbc7240`; §30, §31)

**§32, add as a last bullet before "What this changes in sections 1-10":**
> - **The continuity review (`fbc7240`):** "All 18 sequence documents and all 17 consecutive handoffs have been
>   checked. No remaining narrative handoff blockers were found after correcting stale successor references, the
>   Verdance arrival gates and the solo tutorial specification." The tutorial is "mandatory on the first playthrough,
>   with replay or skip offered on later playthroughs". Still Nicholas's to decide: the opening cinematic's length
>   ("tested 49 seconds or proposed 2 minutes 20 seconds") and whether the tutorial gunship must be destroyed before
>   the withdrawal. The convoy scan and the landing interception of §14 are withdrawn; co-op is out of every document.

Also in §18's and §22's scope tables the "Co-op" rows can now read "Removed from the documents (`fbc7240`)".

## 4. What needs Ryan, and what is still with Nicholas

**Ryan (scope):**
- **c1. The first-play rule.** Build the profile flag and the replay-or-skip offer (e1, e2, about 3-4 h), or ship
  with the tutorial always played? The opening cinematic is already skippable by hold on every run. The rule is in
  the story index as a requirement, not a draft line. My recommendation: build the flag (cheap), and make the "skip"
  start the run after wave 1 with hints off; do it after the withdrawal question (c2) is settled so the skip target
  is stable.
- **c2. Where today's wave 2 goes.** Nothing may be fought between Aftermath and Verdance. Wave 2 (the gunship wave)
  is the tutorial's heavy contact in §13, which fits; but then the tutorial battle is waves 1-2 plus a withdrawal,
  and the withdrawal and the Aftermath trigger are not built. Decide: build the withdrawal now (T4, T3, T1), or keep
  waves 1-2 as they are and fire Aftermath's lines on wave 2's clear as the reduced form.
- **c3. Keep the scannable hauler?** It is built and harmless. My recommendation: keep the convoys, drop the hold
  (fire the line on proximity), 0.5 h.

**Nicholas (story), still pending by his own README:**
- Whether the tutorial gunship must be destroyed before the withdrawal. Engineering should tell him what is built:
  wave 2 ends when its enemies are dead, so today the kill is mandatory by construction.
- The opening cinematic's length: 49 s (built and tested) or 2 min 20 s (his brief, §11).
- The final tutorial run time (his list still has it; ours to supply from a played run).
- Unchanged from before: COMMAND or Control; the three-pulse definition; review of every draft line in 07-09.

**To tell Nicholas (one `Story:` message):** the game has no first-play flag yet and what we propose (c1); the
gunship kill is mandatory in the current build; the convoy scan becomes a proximity line; no interception or orbital
battle exists on the Verdance approach (T4 to confirm before it is said).
