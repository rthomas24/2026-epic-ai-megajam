# Cinder documents: scope questions for Ryan

From Nicholas's three documents in `Story/` (Gameplay 04 Cinder Approach, Cinematic 05 The Pattern, Gameplay 05 Cinder
Excavation), in the order of `Story/README.md`. They are proposals; each one says so, and the README says adding them
"does not replace that decision [the reduced Verdance] or approve new gameplay".

**About the hours:** they are my rough guesses from reading `features.md` and a few source files, not measured against
the team's speed and not checked with T1-T5. Treat them as small / medium / large.

**The short version:** these three documents fit the game far better than the Verdance ones did. They keep the mining
complex, the crystals, the Warden, the field anchor and Lancer's EMP exactly where `storyline.md` §5 and ENG-26 have
them. Gameplay 05 is, step for step, today's two Cinder objectives with new names. What is new is everything **before**
the first objective: an approach, a survey station with a terminal, and a cutscene. The lengths add up to 10.5-15
minutes; the recommendation is 6-8.

## (a) Proposed reduced form: Cinder in about 6-8 minutes (from the atmosphere to take-off)

| Part | Reduced form (what gets built) | Owner |
|---|---|---|
| Approach in space (no added time) | Waves 3-4 stay where they are, between the planets, and are told as the interception on the Cinder route: transports that ignore the squad fly past toward Cinder (the convoy lane already wanted for Verdance and the tutorial), and two lines from Command and Kato. No scan, no "mark a landing route" action; the dig's glow is visible from orbit | T4 (transports, waves), T3 (lines), T1 (nothing new) |
| Descent and route to the station (1.5-2 min) | The descent (ENG-38), the landing, and a short marked route from the pad past one existing patrol to a survey station on the rim of the dig. The existing hazards (magma, geysers) shape the route. One echo of a Kato line on the way. The ancient-geometry prop stands beside the dig | T1 (descent), T5 (route, patrol, terminal), T3 (echo) |
| The survey station | One open-sided station re-using the Verdance station kit, re-dressed for Cinder, holding the terminal. Not a walk-in facility. The terminal can be used once the patrol is dead or left behind | Environment / prop builder, T5 fits it |
| Cinematic 05, *The Pattern* (40-50 s) | Voice over the terminal with the Verdance monitor graphic plus a Cinder layer; the echo of "Same construction."; the cache and anchor markers appear. Built in the same pass as the opening, *Aftermath* and Cinematic 03. Fallback if that pass is full: five transmissions at the terminal | T3, with T2 (graphic, markers) |
| Secure the excavation and recover the crystals (2-2.5 min) | Today's objective 3 (the outpost) renamed: gate, the areas, the Warden, then the claim becomes the crystal cache and awards EMP Pulse as now. One gate console recognises the squad before opening. T5 may drop one of the three areas to hit the time. No new enemy AI | T5 |
| Disable the field anchor and extract (1.5-2 min) | Today's objective 4 (the jammer) renamed: crystal bridges, three charges under attack, the run as the tower collapses is the return to extraction. On shutdown the HUD shows a corridor marker toward the Dominion. Boarding the ship needs both objectives done, as now | T5, with T2 (corridor marker) |
| Order | Terminal first, then excavation, then anchor (today the two objectives can be done in either order) | T5 |
| Departure (Cinematic 06 is not written) | Two or three transmissions over the take-off (`A3_PassageOpen` or Nicholas's replacement) | T3 |
| Wave 5 and the field | Unchanged: wave 5 after Cinder, and its clear starts the assault as today. The anchor gives "a corridor" in words; the four generators are still the field. No change to the built distortion field unless Ryan decides question 3 otherwise | T4 |
| Heat / shielding, co-op | No mechanic (the document itself says no heat meter and no equipment hunt). Solo | - |

**Unchanged:** both Cinder objectives, the Warden and its mercy tiers, the EMP reward, waves 3-4 and wave 5, the 20-30
minute target, and the rule that nothing reveals the twist before the Dominion.

## (b) The questions

### 1. Length: Cinder would be 10.5-15 minutes
- **Asks:** Gameplay 04 at 5-7 min, Cinematic 05 at 90-110 s, Gameplay 05 at 4-6 min. Both gameplay documents call
  their lengths working targets "subject to the overall campaign budget".
- **Today:** Cinder is two objectives; the whole game targets 20-30 min; Verdance was decided at 8-10 min.
- **Cost:** none to decide.
- **Recommend: fits in a reduced form**, 6-8 min as in the table. With Verdance at 8-10, the tutorial battle, three
  waves, the assault and boarding, the game is already at or over 30 min; Cinder cannot be the long one.

### 2. Waves 3-4 and "one bounded interception"
- **Asks:** "Any post-Verdance space encounters belong within this approach, avoiding a separate, repeated travel
  battle": one bounded interception while transports head for the planet.
- **Today:** waves 3 and 4 are fought in the galaxy between the planets; Cinder's marker appears after them.
- **Cost:** told by lines only: 1-2 h (T3 lines, T4 transports shared with Verdance). Merging the two waves into one
  encounter or moving them to Cinder orbit: 3-5 h (T4) and a rebalance.
- **Recommend: fits as is.** Keep both waves where they are; the lines make them the interception on the Cinder
  route. This also answers `storyline.md` §17 question 1. Do not merge: waves 3-4 are where gunships, bombers and the
  portal gates are taught, and they are balanced.

### 3. What opens the distortion field (the important one)
- **Asks:** the crystals, with "the signal data", give "a way through"; the anchor's shutdown shows "a stable
  outbound corridor" and "prepares an approach corridor"; the four generators "remain a later assault objective".
- **Today:** `storyline.md` §5 act 3 and ENG-26 say destroying the anchor opens the passage (`A3_PassageOpen`: "The
  anchor is down. There's a passage through the field. Go."). §5 act 4 says the four generators are the field. In
  code on `main`, the Dominion's arrival is set when **wave 5** clears (`Wave.bRevealsDominion`,
  `TDWaveDirector.cpp`), not by Cinder objective 4. The distortion field that hides the Dominion and pushes ships
  away was built today; I did not find it by name in `Source/` on `main`, so I could not check what turns it off.
- **Cost:** words only: 0-1 h. A visible corridor through the field in space (a gap the player flies through): 2-4 h
  (T4, effects builder), on a system that was only just built.
- **Recommend: fits in a reduced form.** One rule for the game: the field pushes ships away until **both** Cinder is
  finished and wave 5 is cleared; then the squad can close on the Dominion and the generators are the fight. In the
  fiction: the crystals let the ships hold a course inside the field, the anchor's shutdown shows where the corridor
  is, wave 5 guards it, and the generators are what still shields the ship. Nicholas writes the sentence. No new
  corridor geometry before the freeze unless T4 says it is an hour's work.

### 4. The survey station and terminal on Cinder (new place)
- **Asks:** a survey station "overlooking the dig", reached by "a short industrial route with cover and a limited
  patrol encounter", with a terminal that is offered only outside combat; "exposed ancient geometry beside the dig".
- **Today:** nothing on Cinder before the two objectives; no station, no terminal. The Verdance reduced form builds
  one open-sided station; the artefact prop for both planets is already on the clue list (§6).
- **Cost:** 3-5 h if the Verdance station is re-used and re-dressed (prop builder 1-2 h, T5 2-3 h for the route,
  patrol, terminal and order gate). A new industrial station and route: 8-12 h.
- **Recommend: fits in a reduced form**, as in the table. If even that is too much, the terminal can stand at the
  landing pad with no route and no patrol (1-2 h) and the cutscene still triggers.

### 5. "Read the excavation" from orbit and marking a landing route
- **Asks:** "use the objective marker to inspect the mining complex from orbit"; "the squad marks a safe landing
  route".
- **Today:** no inspect or scan action; ENG-38 lets the player enter the atmosphere anywhere, with the landing zone
  as the reference point; §5 already asks for the dig's glow to show from orbit.
- **Cost:** as dressing and one line: 1-2 h (environment builder for the glow, T3). As an action: 3-4 h (T2, T4).
- **Recommend: fits as is** as something seen, not done: the glow, the traffic, a marker on the pad, Wren's line.

### 6. Cinematic 05
- **Asks:** 90-110 s in four beats: two scans matching, a "stable band" overlay with two map markers, the echo, a cut
  to the dig with the Warden in view; skip applies markers and flags once; never replayed on a retry.
- **Today:** the transmission system with an echo style; the director's cameras; the single cutscene pass is queued
  behind the art (§11 gate) and already holds the opening (2:20), *Aftermath*, Cinematic 03 and maybe Cinematic 04.
- **Cost:** 3-5 h at 40-50 s (T3, with T2 1-2 h for the graphic layer and markers). Full length with a camera move
  through the dig: about double. As transmissions only: 1 h.
- **Recommend: fits in a reduced form.** Voice over the terminal and the graphic; the Warden shown by a marker (the
  document allows "sensor contact"). It is the fifth cutscene in one pass, so rank it: below the opening and
  Cinematic 03's drone, above Cinematic 04. The echo is the beat to keep; it works as a transmission even with no
  cutscene.

### 7. The echo (and how many)
- **Asks:** one echo in play (Gameplay 04: a phrase "Kato has just used") and one in the cutscene ("Same
  construction."), both Kato, both put down to feedback; the recording is "kept" for later.
- **Today:** three placeholder echoes, one per pilot (`A3_EchoWren / Kato / Moss`), of phrases nobody says in play.
- **Cost:** 1-2 h (T3: fire a Kato line, then the same text through the echo style; a story flag for "recording kept").
- **Recommend: fits as is**, and use the documents' version in place of the three placeholders: an echo of words the
  player heard a moment ago is better storytelling and easier to deny. Nicholas decides whether any of the three stay.

### 8. Gameplay 05 against today's two objectives
- **Asks:** secure the excavation (bounded force, the Warden), recover the crystals at a cache (EMP awarded), disable
  the field anchor, reach extraction; checkpoints on entry, after the crystals, after the anchor; rewards once; no
  new survival timer; tuned for one player.
- **Today:** objective 3: gate with two conduits, three areas, the Warden (checkpoint before it, mercy tiers), claim
  the EMP prototype. Objective 4: crystal bridges, three charges under attack, run as the tower collapses. Scene
  checkpoints per objective. All built and finishable; being re-fitted to the new terrain.
- **Cost:** 2-4 h (T5: four objective lines, markers, the cache in place of the prototype, the order gate, a
  checkpoint after the cache if there is none; T2 the corridor marker).
- **Recommend: fits as is.** It is a rename. The only decisions are the fixed order (yes: the story needs the crystals
  before the anchor) and whether three areas are one too many for 2-2.5 min (T5 times it).

### 9. The EMP reward
- **Asks:** awarded when the crystals are recovered; "do not invent a claim that the crystals themselves create EMP
  technology"; never needed on foot; no re-boarding to show it off; first use after Cinder; the prompt must not block
  the player while exposed. Kato says "Samples secured. Lancer's EMP is available."
- **Today:** objective 3 ends with claiming "the EMP prototype" and unlocks EMP Pulse; the reward screen shows it;
  wave 5 is the first place to use it.
- **Cost:** 0-1 h. **Recommend: fits as is.** Simplest fiction that obeys the document: the cache holds the crystals
  and, beside it, the prototype the machines were keeping (today's claim, kept). Nicholas chooses; either way no code
  beyond a name. Check that the reward screen opens after the area is clear, not mid-fight (T5).

### 10. The console that recognises the squad
- **Asks:** "one access console may briefly recognise the squad before opening", impersonal.
- **Today:** on the clue list (§6) and in ENG-26 ("one door at the dig opens to the pilot's ID"); I did not check
  whether it is built.
- **Cost:** 1 h (T5). **Recommend: fits as is**, on the gate to the cache.

### 11. Transports and traffic toward the dig
- **Asks:** transports carrying excavation equipment to one site; interceptors that defend the route and do not
  chase forever.
- **Today:** not built; the same non-hostile lane is wanted for the tutorial battle and Verdance.
- **Cost:** 0-1 h on top of the shared lane (T4: a second destination). **Recommend: fits as is**, built once.

### 12. Pursuit on the way out, and extraction
- **Asks:** "only a bounded pursuit if the encounter budget supports it"; extraction refuses until crystals and
  anchor are done; a failed extraction returns to a safe ground checkpoint.
- **Today:** the run from the collapsing tower; the way back to space opens when both objectives are done.
- **Cost:** 0 h. **Recommend: fits as is**; no extra spawns.

### 13. Cinematic 06, *The Path to Dominion*
- **Asks:** triggered by extraction; "reviews the evidence and commits the squad to the Dominion approach". Not
  written.
- **Today:** take-off to Cinder orbit, then wave 5.
- **Cost:** unknown. **Recommend:** plan for transmissions over the take-off only, and tell Nicholas now that a sixth
  cutscene will not fit the single pass; ask that it hands into wave 5.

### 14. Heat, shielding, co-op, new enemies
- **Asks:** nothing. The documents rule out a heat meter and an equipment hunt, say "tune for one active player", and
  use only the Warden and "a bounded defending force".
- **Cost:** 0 h. **Recommend: fits as is.** The Warden still has no finished mesh (Grok Bot); that is the one asset
  the Cinder story now leans on twice (seen in the cutscene, fought after it).

## (c) Contradictions and risks

### With `storyline.md` §1-18
1. **What opens the field** (question 3). §5 act 3: the anchor's destruction "gives the passage through the
   distortion field". Documents: crystals + signal data = the way through; anchor = interference on "the route out",
   its shutdown shows a corridor; generators = a later assault. §5 act 4: the generators are the field. Three things
   now each "open" something. `A3_PassageOpen` and the ENG-26 story line follow the old wording.
2. **"The signal data" no longer has a source.** Cinematic 05 ("With the signal data, we may have a way through") and
   Gameplay 05 ("Combined with the Verdance signal data") rely on §5's signal-analysis device. Nicholas's own Verdance
   documents (§14-17) and the decision in §18 replaced that with a geological survey archive. Suggestion to put to
   Nicholas: the relay (Verdance objective 2) is where the signal data comes from, which also answers §18's open
   point "what the relay means now".
3. **§18 and §17 question 2 are out of date.** §18 says "now that the survey station replaces the mining complex".
   It does not: the documents keep the mining complex and put a survey station at it. Good news; §18's last
   paragraph needs that clause corrected when 19-21 are appended.
4. **Who wants the crystals.** §5 act 3 and `features.md` ("the machines dig for the crystals that stabilise a
   passage through the field"), and `A3_CrystalsFound`, have the enemy mining crystals. The documents have the enemy
   excavating the buried **structure**; the crystals are in a "secured cache" and are what Triad takes. Small, but the
   placeholder line says the opposite of the cutscene.
5. **The Origin Core.** §5 act 3: the squad learns the enemy is after the parts or keys of the Origin Core. The
   documents never have anyone say the name; they only say its function "remains unexplained". Unclear whether the
   name is heard before the Dominion.
6. **The echoes.** §5: "transmissions now use the squad's own call signs and phrases" (three placeholders, all three
   pilots). Documents: one phrase, Kato only, twice.
7. **Waves.** §5 and §18: waves 3-4 between the planets. Gameplay 04: one interception inside the approach. Wave 5 is
   not mentioned in any document; the chain ends "extraction → Cinematic 06".
8. **Objective names.** §5 / ENG-26: "Stop the Excavation", "Destroy the Field Anchor". Documents: "Investigate the
   mining complex", "Access the survey records", "Secure the excavation", "Recover the stabilising crystals", "Disable
   the field anchor", "Return to extraction". Story words are Nicholas's, so his win; `storyline.md` §5, `features.md`
   and the objective names in code follow.
9. **Names.** "Dominion excavation", "hostile transmission": the squad's word for the forces is the Null Armada (§8).
   "COMMAND" against the placeholders' Control. Same notes as for Verdance.
10. **Cinematic 04's shielding** is settled by these documents (an "equipment question", no mechanic), which agrees
    with §18.

### With what is built
11. **The Dominion's arrival is tied to wave 5, not to Cinder.** `features.md` ENG-26 says objective 4 "reveals the
    Dominion"; ENG-18 and the code say the flag is set after wave 5. With the Dominion now visible from wave 1 behind
    the field, "reveal" has no meaning left; the question is only when the field lets ships in (question 3).
12. **Order of objectives.** Built: either order. Documents: terminal, cutscene, excavation, crystals, anchor.
13. **The claim.** Built: "the EMP prototype". Documents: a crystal cache; EMP not to be explained by the crystals.
14. **No survey station, terminal or pre-objective route on Cinder.** Everything in Gameplay 04's phase 4 is new.
15. **No pilots on foot.** "Kato aligns", "Get me to the survey terminal", "Clear the route to the cache": all voice.
16. **Entering anywhere (ENG-38)** against "fly toward the marked pad": compatible, the pad is the marker; a player
    who enters elsewhere still has to fly to it.

### Twist risk (nothing may reveal the future-self truth before the Dominion)
17. **No rule is broken on paper.** No draft line contains "future", " time" or "it's us" (read by eye against the
    story test's three checks; the test was not run on them). The documents themselves forbid future pilots,
    decrypted records, identity confirmation, the three pulses and "Don't-" on Cinder.
18. **The echo is the nearest thing to a reveal so far.** A hostile channel repeating Kato's words in a cadence that
    is "familiar", and Moss asking "Was that you?", points the player straight at the answer; "feedback" is the only
    cover. It stays deniable if the echo is heavily processed and repeats words heard a moment ago. It stops being
    deniable if it uses a clean copy of Kato's voice, or if the three placeholder echoes (phrases nobody said, so
    they cannot be feedback) also play. Recommend: the documents' two echoes only, and "Was that you?" is the line to
    cut first if playtesters guess.
19. **Recognition beats are adding up.** Opening: handshake, blue drone. Verdance: "verified" marker, the drone's
    three pulses answered, the drone that pauses, the marker flash. Cinder: two echoes, the console that recognises
    the squad, "They knew where to dig". That is about ten before the Dominion. Each is small; together a player may
    guess by Cinder. The cheapest to drop on Cinder is the recognising console (the Dominion interior does the same
    job, better).
20. **"Keep the recording"** promises a payoff: the recording should be heard again in the Dominion (a console
    recording of older Kato saying the same words). Worth telling Nicholas it is a hook we can serve.
