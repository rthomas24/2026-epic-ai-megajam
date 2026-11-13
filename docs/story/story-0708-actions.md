# Story sequences 07, 08 and 09 against the built Dominion finale: actions for the Lead

Read on `origin/main` (not fetched by me): `Story/README.md` (18 entries), the six Word documents not yet in
`storyline.md` (Cinematic 07, Gameplay 07 from commit `63f3863`; Cinematic 08, Gameplay 08 from `243e72d`; Gameplay
09, Cinematic 09 from `c357779`), `storyline.md` §1-25, `docs/story/story-alignment-actions.md`,
`docs/story/transmissions.md`, features.md ENG-29-31. Read on `lead/int`:
`Ground/Interior/TDDominionInteriorCourse.inl`, the escape flight in `World/TDDominion.cpp`, the stinger in
`Director/Menu/TDMenuBackdrop.cpp`, the act 4-5 rows of `Story/TDStorySubsystem.cpp`, the twist test in
`Story/Tests/TDStoryTests.cpp`. Nothing was built, run or played by me. Hours are rough guesses. The draft sections
for `storyline.md` are §26-32 in `storyline_26_plus.md`.

The findings that matter most:
1. **The five `F_*` command-deck lines are never fired by gameplay.** They exist in the table and tests only. The
   payoff of the opening "Don't-" does not play in the game today.
2. **Every console is optional.** The documents put the reveal on the main route and make the last record a gate
   that starts Cinematic 08 before the reactor fight. Today a player can reach victory without hearing any of it.
3. **The reactor fight, the door opening, the 2.5 s, the 90 s clock and the retake points already match Gameplay 08.**
   The work there is words and four radio hooks.
4. **The run, the flight, the 18 s stinger and the 0.35 s fade already match Gameplay 09 and Cinematic 09**, which
   were written around them. One required change: the stinger's caption must say "decades ahead", not "forty-one
   years". The ending's seven radio lines have no hooks.
5. **Nicholas has nothing left to write for the boarding route.** Cinematic 09 "closes the main campaign's documented
   story". What remains on his side is review of the draft lines, voices, sounds and the open questions in (d).

## (a) Every line and beat, mapped to a hook

Ids marked *new* are my suggestions. "Free" means an existing hook that needs no code, only Nicholas's entry in
`DA_Story_Transmissions`.

### Cinematic 07, Boarding the Dominion (all before the reveal)
| Line or beat | Hook | Owner, cost |
|---|---|---|
| WREN "Keep the bay covered. We need those records." | Near-duplicate of the existing `G6_Wren_CoverTheBay` (fires at docking). Use that id with the new text; no new hook | Nicholas picks one line. Free |
| MOSS "We'll keep a way out clear." | *new* `C7_Moss_WayOut`, fired on docking commitment after the line above | T3 id, T4 one fire call. 0.25 h |
| KATO "That layout. We use the same arrangement." | *new* `C7_Kato_Layout`, fired on arrival in the hangar (interior start, first visit only) | T3 id, T5 one fire call. 0.25 h |
| Scanner accepts the pilot; inner door unlocks | Exists: the hangar door, "ACCESS GRANTED - ID RECOGNISED" | Free |
| WREN "They shouldn't have our access codes." | `A4_DoorsKnowUs` (fires as that door opens) | Free |
| KATO "There's a local command archive ahead." | *new* `C7_Kato_Archive`, queued after `A4_DoorsKnowUs`; or the room-start line of `Obj_DI_Corridor` | Free if the room line is used; else T3/T5 0.25 h |
| WREN "Record everything. Find who's giving the orders." | *new* `C7_Wren_Record`, queued after it | T3/T5 0.25 h |
| Objective "Reach the command archive." | Objective title of `Obj_DI_Corridor` (today "Into the Dominion") | T5, a text change |
| Equipment rack, route stencil, Triad-like layout | **No hook; new dressing** | Ryan's environment builders, 1-2 h, optional |
| Threat indicators beyond the first door | Exists (the deflect corridor is behind the door; nothing attacks in the hangar) | Free |

### Gameplay 07, Inside the Dominion (the reveal)
| Line or beat | Hook | Owner, cost |
|---|---|---|
| First console: filtered command log, speaker not identifiable. RECORDED KATO "Keep the recovery teams on the Core search." | `A4_Recording_Hangar`. **Change the placeholder's speaker from FutureWren to Unknown** (today its tag reads "WREN?" in the first room) | T3, a table edit; Nicholas's entry |
| Second recording, Moss's manner in an older voice. RECORDED MOSS "Bring every recovery team back." | `A4_Recording_Corridor` (the console just inside the arena) | Free |
| MOSS "That's my voice in those records. Older." | *new* `G7_Moss_MyVoice`, fired by the console handler after that recording | T3 id, T5 0.25 h |
| Security system displays authorisations for Wren, Kato and Moss | The Warden-room door's access text (a per-door property): "ACCESS GRANTED · WREN · KATO · MOSS" | T5, a text change |
| Three-pulse authentication motif | **Needs a new hook:** a sound slot on that door's open | T5 0.5 h; Nicholas's sound; the motif is still undefined |
| KATO "Those authorisations match all three of us." | *new* `G7_Kato_Authorisations`, fired at `Obj_DI_Warden` step 0 (the door opens) or as that room's start line | Free if the room line is used |
| Archive entries pair identities with older voices. RECORDED WREN "Recover the Core. Protect the younger Triad." | `A4_Recording_Barracks` (the console near the arena exit) | Free |
| Compare the Cinder echo with its older-Kato source | **Needs a new hook and a line that is not written:** *new* `G7_Archive_Echo`, queued after the archive entry | T3/T5 0.25 h, only if Nicholas writes it |
| Final record. RECORDED KATO "We are you, from the future. We came back for the Origin Core." | `A4_Recording_Core`, **made mandatory** (see b2) | T5 1.5-2 h for the gate |
| WREN "We hear the full record before we touch that reactor." | *new* `G7_Wren_FullRecord`, queued after it, then Cinematic 08 | T3 id |
| Unit markings and equipment like Triad's | No hook; dressing | Environment builders, optional |

### Cinematic 08, The Recovered Transmission
| Line or beat | Hook | Owner, cost |
|---|---|---|
| FUTURE KATO "All three signatures confirmed. It's us." | `F_FutureKato_Signatures`: exists, **never fired** | T3: one "fire these in order" call, 0.5 h |
| FUTURE MOSS "They're exactly where we were." | `F_FutureMoss_Where`, same | same call |
| FUTURE WREN "Open a channel. Now." | `F_FutureWren_Channel`, same | same call |
| FUTURE KATO "Temporal interference. Less than a second." | `F_FutureKato_Interference`, same | same call |
| FUTURE WREN "Don't-" (the identical asset) | `F_FutureWren_Dont`, shared sound `Dont`; the one-sound test already exists | same call |
| FUTURE WREN "Don't engage the Dominion. You don't understand what happens if you do." | **Needs a new id:** `F_FutureWren_FullWarning`. It must **not** use the shared `Dont` sound | T3 0.25 h |
| RECORDED KATO "The Origin Core may let us change one event. Before everything was lost." | *new* `C8_RecKato_OneEvent` | T3 |
| RECORDED WREN "We believe recovering it can prevent the catastrophe. It's the last chance we have." | *new* `C8_RecWren_LastChance` | T3 |
| RECORDED MOSS "We came back to save you." | *new* `C8_RecMoss_SaveYou` | T3 |
| Reactor readout: no verified safe shutdown | **Needs a new hook:** a line of text on the console screen or under the objective | T5 or T2 0.5 h; Nicholas's wording |
| KATO "I can't verify a safe shutdown. Continuing could put the whole system at risk." | *new* `C8_Kato_NoSafeShutdown` | T3 |
| MOSS "Then we can't let them finish." | *new* `C8_Moss_CantLetThem`; replaces today's `A5_Choice` text in meaning | T3 |
| WREN "Take out the reactor. Keep the route to the hangar clear." | `A5_Choice` (re-timed to the end of the scene instead of the fight's start), or *new* `C8_Wren_Order` | T3/T5 |
| The command-deck view (monitors, the three ships) | **No hook.** Optional picture on the console screen or a director letterbox | T3 2-3 h, optional |
| Hold combat; return control at the entrance; skip | **Needs new behaviour:** the core door stays shut until the queue ends or is skipped | T5 1 h, T3 skip 1 h |
| Objective "Break the reactor conduits." | Step line of `Obj_DI_Core` (today "BREAK THE SHIELD CONDUITS 0 / 4") | T5, a text change |

### Gameplay 08, The Reactor Fight
| Line or beat | Hook | Owner, cost |
|---|---|---|
| WREN "Conduits first. Then the heart." | The room-start line of `Obj_DI_Core`, or `A5_Choice` if Wren's order is not put there | Free |
| KATO "Conduit destroyed." | **Needs a new hook:** on a conduit's death (the step counter's progress). First conduit only, or a repeatable line | T5 0.5 h, T3 id `G8_Kato_ConduitDown` |
| MOSS "Keep moving. Watch the way back." | Same hook, on the second conduit (*my placement*); id `G8_Moss_KeepMoving` | included |
| KATO "The heart is exposed." | `Obj_DI_Core` step 1 start (the code already switches the shield there); id `G8_Kato_HeartExposed` | T5 one fire call |
| Objective "Destroy the reactor heart." | Step line (today "STRIKE THE HEART") | T5, a text change |
| COMMAND "Reactor lost. Get back to the hangar." | `Obj_DI_Core` completion (the doors open there); id `G8_Command_ReactorLost`. `A5_Escape` stays as the countdown call 2.5 s later (Gameplay 09 keeps it) | T5 one fire call |
| Doors open at once; escape 2.5 s later; 90 s | Exists exactly | Free |
| Collapse hit and escape music on that trigger | The core's completion and `A5_Escape` are the two events to key audio to | Nicholas's audio; T3 confirms the cue names |
| 0/4 count, markers removed | Count exists. Marker removal not checked | T2/T5 to confirm |

### Gameplay 09, Escape from the Dominion
| Line or beat | Hook | Owner, cost |
|---|---|---|
| COMMAND "The ship is coming apart. Ninety seconds. Move!" (run activation) | `A5_Escape`: the placeholder is this line word for word; only the speaker differs (Control) | Free |
| Full 90 s clock, hangar objective, open exit | Exists ("GET BACK TO YOUR SHIP · clock", all doors open) | Free |
| First direction marker visible | The course has a route from the hangar to the core; whether a marker shows at the run's start was not checked | T5/T2 to confirm |
| MOSS "Keep moving. We're holding the way out." | The room-start line of `Obj_DI_Escape`, queued after the countdown call; id `G9_Moss_HoldingWayOut` | Free |
| Temporal failure: brief afterimages, repeated alarms or hull sounds ("may") | **No hook.** Cheapest: Nicholas's alarm and hull sounds on the run's existing start and debris events | Nicholas's audio; visuals optional, T5/effects 1-2 h |
| KATO "Hangar signal confirmed. Get the ship clear." (run completion) | **Needs a new hook:** at the hangar trigger. The game travels at once, so either the queue survives the travel or T4 fires it at the flight's start | T5 or T4 one fire call; T3 id `G9_Kato_HangarConfirmed` |
| Reboarding staged "within that transition" | The travel from the interior to the hangar mouth | Free |
| WREN "Triad, pull away. Follow the clear vector." (regained ship control) | **Needs a new hook:** `BeginEscapeFlight` (it already fires `A5_Escape` there, which is a no-op after the run); id `G9_Wren_PullAway` | T4 one fire call |
| Flight: 60 s limit, debris, complete at the escape radius, retry the flight only | Exists (own scene, own checkpoint at the mouth, the clock restarts, the squad respawns there) | Free; T4 confirms the 60 s |
| Regrouped trio: Wren in Rift, Kato in Lancer, Moss in Bastion | Existing squad staging in the flight and the victory backdrop's flybys; not checked | T4/T3 to confirm |

### Cinematic 09, The Paradox
| Line or beat | Hook | Owner, cost |
|---|---|---|
| KATO "Lancer clear." MOSS "Bastion clear. Still here." WREN "Rift clear. All three accounted for." (0:00-0:08, crossing into clear space) | **Needs a new hook:** when the ship crosses the escape radius (the code already has that moment and a delay before victory); ids `C9_Kato_Clear`, `C9_Moss_Clear`, `C9_Wren_Clear` | T4 one fire call, T3 ids. 0.5 h |
| MOSS "All three. I'll take that." (0:08-0:18) | **Needs a new hook:** a timer in the victory backdrop at about 10 s; id `C9_Moss_TakeThat` | T3 0.25 h |
| Breakup completes behind the squad; warmth in the music | Exists (breakup backdrop); music is Nicholas's | Free |
| 0:18 distress alert, identifiers of Wren, Kato and Moss, date "Decades ahead" | `A5_DistressSignal` at `StingerSeconds = 18`. **Change the caption**: "DATED FORTY-ONE YEARS FROM TODAY" → "DATED DECADES AHEAD" | T3, a text change. Do it now |
| 0.35 s fade, text readable, panel usable | Exists exactly (a black layer under the panel and the transmission line) | Free |
| KATO "A distress signal. Those are our identifiers." WREN "The source?" KATO "It's dated decades ahead." | **Needs a new hook:** three ids queued at the stinger (`C9_Kato_Identifiers`, `C9_Wren_Source`, `C9_Kato_Decades`). The caption must stay on screen under them: today a queued line replaces the one before | T3 1-1.5 h (hold the caption as its own element) |
| "Let the final line sit over black and radio noise" | The stinger's sound slot | Nicholas's audio |
| Skip keeps the received-signal state and the text reachable | **Needs new behaviour:** leaving the victory panel before 18 s plays the stinger first | T3 0.5-1 h |
| Transition to credits or menu; a return never restarts the assault or the reactor | Exists (victory panel, credits); what Continue does after victory was not checked | T3/T5 to confirm |

**Total new ids:** about 28 (four `C7_`, four `G7_`, one `F_`, five or six `C8_`, four `G8_`, three `G9_`, seven
`C9_`). T3 seeds them with the documents' draft lines in one pass (about 2 h with the docs and test update).

## (b) What the documents ask that the build does not do, or does differently

| # | Ask | Built | Recommendation |
|---|---|---|---|
| b1 | **Order of rooms:** hangar and deflection corridor → barracks and service shaft → security checkpoint and command archive → Warden and core approach → reactor | Hangar → deflect corridor → flight deck (three waves) → Warden's gate → core chamber. No service shaft, no checkpoint room, no archive room | **Fits in a reduced form.** The flight deck plays the barracks; the door into the Warden room plays the security checkpoint (three names on its access text, the three-pulse tone); the arena-exit console plays the command archive. No new rooms before the freeze. Needs Nicholas's agreement (it is his proposal's own escape clause: "subject to the existing course") |
| b2 | **The reveal's place:** evidence climbs over four recordings; the final record after the Warden is mandatory, on the main route, with combat held, and starts Cinematic 08 | Four optional consoles, all in a Future voice from room 1; the core console is inside the core chamber with the fight already started | **Fits in a reduced form, and is the main engineering change.** Move the core console to the threshold before the core door; keep that door shut until the console is used; then run Cinematic 08 as a queue of lines; then open the door and start the fight. T5 about 2 h, T3 about 1.5 h. The other three consoles can stay optional: the final record and Cinematic 08 alone state the truth in full, which satisfies "no optional collectible may be the only source of the reveal" |
| b3 | **Recordings must not compete with combat** | Two consoles stand in the arena where three waves are fought | **Fits in a reduced form:** consoles in the arena answer only once the room is clear (T5, 0.5 h), or move the exit console into the link corridor |
| b4 | **Cinematic 07 takes control for 35-45 s** (camera through the hangar) | No scene; a quiet arrival with the player in control | **Fits in a reduced form:** six lines over the docking and the arrival. The document itself asks for "the existing docking and hangar transition". No camera work |
| b5 | **Cinematic 08 takes control for 60-80 s** ("while control is unavailable"), with a command-deck view | No scene. The five callback lines are never fired | **Fits in a reduced form:** voices and subtitles over the console in a sealed threshold, player free to move, skippable. The document allows "monitors and voices" and "no new face animation or live future-pilot encounter". A real control-away scene with a command-deck picture **does not fit before the freeze** unless Ryan wants it over other work (T3 4-6 h) |
| b6 | **"Its destruction opens the doors before the escape starts"** | On the heart's death every door in the interior opens at once; the escape objective starts 2.5 s later with 90 s; an escape retake starts at the core exit facing the hangar and never repeats the fight | **Fits as is.** The document was written from this behaviour and quotes the 2.5 s |
| b7 | **The Origin Core is distinct from the reactor.** Is there an object or room for it? | No. The only "core" aboard is the reactor. The document does not want one: Cinematic 08 "does not establish that the Origin Core is fully assembled aboard"; Gameplay 08 adds no "Origin Core boss" | **Fits as is, with a wording fix.** Nothing to build. Rename what the player reads: the room marker "THE CORE" → "THE REACTOR"; "The Reactor Core" → "The Reactor"; "BREAK THE SHIELD CONDUITS" → "BREAK THE REACTOR CONDUITS"; "STRIKE THE HEART" → "DESTROY THE REACTOR HEART". The scene id `Obj_DI_Core` can stay (not player-facing). T5 0.25 h. Also keep "the core" out of any bark that means the reactor |
| b8 | **A second Warden?** | One Warden, room 4 | **Fits as is.** The document says explicitly it adds no second Warden |
| b9 | **No mandatory perfect-deflect gate; ordinary blade combat; "sustain pressure through the existing encounter"; telegraph threats** | Four conduits, then a heart with 150 health, both broken with the blade. I saw no enemy or hazard in the core room in the code I read. A code comment plans orbs, a laser and a "deflect-only heart" | **Fits as is for the gate; pressure is the open point.** Do **not** build the deflect-only heart: it contradicts the document. Ask T5 whether a played run has any threat in the room. If none, one cheap source of pressure (a few drones or two troopers per conduit) is worth more than the ring timing, sweeper and acceleration of ENG-30, which the document treats as unverified. See c3 |
| b10 | **One boarding character, shown arriving in the ship the player flew; other pilots on radio** | One on-foot character; wingmen hold outside. Who the on-foot pilot is in the fiction, and whether the escape craft is the docked ship, were not checked | **Fits as is, with one check** (T4/T5: the ship flown in is the ship flown out). The lines assume Wren boards: a question for Nicholas (d) |
| b11 | **Checkpoints:** hangar entry, security approach, before the Warden, reactor entrance; retries never replay Cinematic 07 or 08, the Warden or the fight once won | Checkpoints at the hangar door, in the arena, before the Warden, at the core entrance and at the escape start; transmissions play once per run and are saved | **Fits as is.** Tell Nicholas the fight's reset rule: a death in the reactor fight restarts it from 0 / 4 with the shield up |
| b12 | **Skip:** skipping either cinematic keeps its discoveries and starts the next encounter once | No skip for a queue of transmissions that I know of | **Fits in a reduced form:** a skip that empties the queue and marks its lines as played (T3 1 h). Needed only for Cinematic 08's 70 s |
| b13 | **Replay of recordings** ("preserve it for replay", "through subtitles or record replay") | Subtitles while a line plays; once per run; no replay | **Fits in a reduced form:** subtitles only. A replay log **does not fit before the freeze** and is not needed if b2 is done |
| b14 | **New assets:** hangar equipment rack, squadron route stencil, Triad-like unit markings, a scanner, a core-room readout, a command-record display | The kit console lights its screen when played; doors show the access text | **Fits in a reduced form:** the readout as one line of text (b-table of (a)); one stencil decal and one rack in the hangar if an environment builder is free (1-2 h). Everything else is carried by lines |
| b15 | **Temporal failure on the escape** (Gameplay 09: "may appear as brief afterimages and repeated alarms or hull sounds, but must not falsify the countdown or reverse player input") | Debris and a clock. I saw no echo or repeat effect in the run or the flight code | **Fits in a reduced form:** sound only (repeated alarms and hull groans, Nicholas's audio on existing events). Afterimages are optional and the first thing to drop |
| b17 | **"Use the existing changed route rather than sending the player through the cleared reactor encounter again"** | There is no changed route: the run goes back through the same rooms (a code comment plans one as "step 5"). The run starts at the core chamber's exit, so the reactor room is never re-entered | **Fits as is in substance.** Tell Nicholas the route is the same rooms with doors open and debris. A changed route **does not fit before the freeze** unless T5 already has it |
| b18 | **The stinger's date: "Decades ahead"; "do not silently adopt the forty-one-year placeholder"** | Caption: "DATED FORTY-ONE YEARS FROM TODAY" | **Change it** (T3, one string, plus `transmissions.md`). The only hard requirement in the two ending documents |
| b19 | **Seven ending radio lines** (three "clear" reports, Moss, three at the signal), the signal text readable under the last one | No line hooks in the flight's end or the victory scene; one transmission shows at a time | **Fits in a reduced form:** the three reports at the escape radius (T4) and the three signal lines at 18 s with the caption held (T3). Moss's line at 10 s is the one to drop if time is short |
| b20 | **Skip and replay of the ending** (state kept, signal text reachable, replays without rerunning the escape) | The victory panel is usable from the start; leaving before 18 s misses the stinger; no replay of the ending | **Fits in a reduced form:** leaving early plays the stinger first. A replayable ending cinematic **does not fit before the freeze** and the document only says "may" |
| b21 | **Victory at 18 s: the cut to black must not block the panel or erase the text; coordinate with music** | Built exactly so (black under the panel and under the transmission line; 0.35 s) | **Fits as is.** Nicholas keys the music to 18 s; do not let a music edit move `StingerSeconds` without saying so |
| b22 | **The flight: 60 s limit, complete at the radius, no generator fight, no clear-all, retry the flight only, wingmen rejoin** | Own scene and checkpoint at the hangar mouth; debris and explosions; completes beyond the exit radius; a fail retakes the flight with the clock restarted and the squad respawned at the mouth | **Fits as is.** T4 confirms the 60 s default and the retake values to Nicholas ("require Ryan's validation") |
| b23 | **"Show or confirm the regrouped present-day trio: Wren in Rift, Kato in Lancer and Moss in Bastion"; no casualty; older commanders' fate left open** | Squad respawn in the flight; ship flybys in the victory backdrop. Not checked that all three ships are shown | **Fits as is if the three ships are visible**; otherwise the three "clear" reports of b19 carry it. Nothing in the victory text should say the future is safe or the older crew died: T3 to read the victory panel's words |
| b16 | **COMMAND speaks inside the Dominion** ("Reactor lost. Get back to the hangar.") | `A5_Escape` is Control | **Fits as is** once COMMAND / Control is settled (still open from §25) |

## (c) Scope questions for Ryan (only the ones that change work)

1. **Cinematic 08: voices over a sealed threshold, or a real scene?** My recommendation: voices, subtitles, a shut
   door and a skip (T5 2 h, T3 2.5 h in all, including firing the `F_*` lines for the first time). A control-away scene
   with a command-deck view is 4-6 h more and is the first thing to cut.
2. **Make the final record a mandatory gate before the reactor fight?** It adds about 70-80 s of standing and
   listening to a 5-8 minute interior. Without it the twist can be missed entirely. My recommendation: yes, with the
   skip.
3. **The reactor fight: ship "four conduits, then the heart", and spend what time is left on one source of pressure,
   not on the ring timing, sweeper, acceleration or a deflect-only heart?** The documents treat those as unverified
   and forbid a mandatory perfect-deflect gate. T5 to say what is in the room today.
4. **No new rooms** (service shaft, security checkpoint, command archive): accept the folding in b1? If Ryan wants
   one new space, the cheapest is a short "archive" alcove off the link corridor before the Warden (T5 1-2 h plus
   kit pieces), and it is not needed for the story to work.
5. **Hangar dressing for "familiar architecture"** (a rack, a stencil, Triad-like markings): 1-2 h of an environment
   builder, or lines only?

6. **Leaving the victory screen before 18 s: force the stinger first?** Today the last beat of the story can be
   skipped without knowing it exists. My recommendation: yes (T3 0.5-1 h).
7. **"Time failing" on the escape: sound only?** The document makes it optional. My recommendation: sound only, no
   afterimage effect, no changed route.

Not a question any more: the ending. The README now says "The confirmed ending boards the Dominion; the existing
space-only route is an emergency fallback, with no alternative writing requested here", which matches Ryan's decision.
No fallback reveal will be written; if the fallback is ever used, the twist is carried only by our placeholders.

## (d) What Nicholas still has to write, and what he needs from us

**Nothing is left to write for the boarding route.** With `c357779` the index has 18 entries and the "Next to
document" line is gone; Cinematic 09 says "With Gameplay 09, this closes the main campaign's documented story". The
space-only fallback gets no document ("no alternative writing requested here").

**What is still his to do:**
- Review and approve the draft lines of 07, 08 and 09 (every document says its new dialogue is a draft; only the five
  callback lines and the full warning are fixed wording). About 45 lines in all.
- Voices and sounds: the recordings and Future voices, the one shared "Don't-" asset, the three-pulse tone, the
  collapse hit, the escape music keyed to the run's start, the alarms and hull sounds for "time failing", the warmth
  and its interruption at 18 s, the radio noise under the last line.
- The stinger's caption with "decades ahead" (or a calendar value he approves).
- The readout wording in Cinematic 08.
- Answers to the questions below.

**What he needs from us (one board message or issue comment titled `Story:`):**
- Confirmation of the values his documents quote from the source "for Ryan's validation": the 2.5 s, the 90 s, the
  run's retake point and full clock; the flight's 60 s limit, its spawn and retake values (T4); the 18 s and 0.35 s
  (T3). Say which were confirmed in a played run and which only in code.
- Corrections to two things the documents assume: there is **no changed route** on the run (same rooms, doors open,
  debris); there are **no shaft, security-checkpoint or archive rooms** (b1).
- The reset rule of the reactor fight (a death restarts it from 0 / 4 with the shield up).
- The reduced forms in (b) for his yes or no: b1, b2 (the gate), b4 and b5 (lines instead of scenes), b15 (sound
  only), b19 (which ending lines).
- The list of ids from (a) once T3 has seeded them, so he can fill `DA_Story_Transmissions` against them.

**Questions for Nicholas from the six documents:**
1. Who boards? The lines are written for Wren on foot and Moss outside, but the player may dock in any ship. Neutral
   lines, a variant per pilot, or always Wren?
2. Which recording on which console, and is the first log's speaker Unknown? (Default in (a).)
3. `G6_Wren_CoverTheBay` and Cinematic 07's first line are near-duplicates: one of them goes.
4. Cinematic 08 has 13 lines for 60-80 s: which may be cut if it runs long?
5. The readout's words. Placeholder: "REACTOR · NO VERIFIED SAFE SHUTDOWN".
6. The Cinder-echo comparison in the archive: a line, or drop it?
7. The three-pulse motif: one definition (rhythm and sound), now needed in three places and on the checkpoint door.
8. `A5_Choice` (Moss) and `A5_Escape` (Control): kept, reworded or replaced by Wren's order and Command's "Reactor
   lost"? COMMAND or Control?
9. Tags for the Future voices after the reveal ("WREN?" today).
10. The ending: all seven lines, or only the three at the signal? The exact caption?
11. "Wren in Rift, Kato in Lancer and Moss in Bastion" at the end, when the player may have boarded and flown out in
    any of the three: is the report order enough, or must the ships be shown?

## (e) Twist check: the reveal happens aboard the Dominion, in Gameplay 07

**May carry the identity (older selves, "future", "it's us", a Future speaker):**
- `A4_Recording_Corridor` and `A4_Recording_Barracks` (the second and third recordings: an older Moss, then names
  paired with voices), and the squad's reactions to them (`G7_*`).
- `A4_Recording_Core` (the final record: "We are you, from the future.").
- Every `F_*` id (the command deck), including the new `F_FutureWren_FullWarning`.
- Every `C8_*`, `G8_*`, `G9_*` and `C9_*` id, and every `A5_*` id (`A5_Choice`, `A5_Escape`, `A5_DistressSignal`).
  None of the escape or ending drafts says anything about the twist beyond "Those are our identifiers" and "decades
  ahead", which is the stinger itself.

**Clues only, must not state it:**
- `A4_DoorsKnowUs` and all `C7_*` lines (Cinematic 07: "The recognised access code is a clue, not a spoken
  confirmation... Do not show older Triad here or replay 'Don't-'").
- `A4_Recording_Hangar` (the first console: "cannot yet identify the speaker"). **Today its placeholder breaks this:**
  speaker FutureWren, tag "WREN?", "If you're hearing this, you came anyway." Change it to an Unknown speaker with the
  command-log line.

**Nothing earlier carries it, as far as the table and test on `lead/int` show (read, not run):** the test treats as
after the reveal only `A4_Recording_*`, `A4_DoorsKnowUs`, `A4F_*`, `A5_*` and `F_*`; every other id, including the
generator lines `A4_GeneratorDown*`, the `G6_*` radio and the two assault clue fragments `G6_Clue1` and `G6_Clue2`
(speaker Unknown), is checked for Future speakers and for "future", " time", "it's us", "older", "ourselves", "our own
voice", "years from", and for "temporal" outside Kato's terminal line. The generator lines now say "shield", not
"field". "Don't-" is held to the opening and `F_FutureWren_Dont`.

**Changes the test needs when the new ids land (T3):**
- Add `G7_`, `C8_` and `G8_` to the after-reveal set. Leave `C7_` **out** of it, so Cinematic 07 is checked.
  (`G7_Moss_MyVoice` contains "older" and `C8_`/`F_` lines say "future" and "temporal": they fail unless exempt.)
- Check `A4_Recording_Hangar` by hand or take it out of the exempt set: it is exempt by prefix today, so the test
  cannot catch a Future speaker in the first room.
- Assert that `F_FutureWren_FullWarning` has **no** shared sound (the fragment is never extended).
- Optional: assert that something fires the five `F_*` ids (today nothing does, and no test noticed).
- Add `G9_` and `C9_` to the after-reveal set (Kato's "It's dated decades ahead." is safe today, but the set should
  be complete).
- Assert that `A5_DistressSignal` and every `C9_` id have **no** shared sound ("Do not reuse the shared 'Don't-' sound
  as this new packet"), and that no entry's text contains "forty-one".

**Rules for the ending that no test covers (T3 to read the victory panel and credits by eye):** no text says the
system or the future is safe, that the Origin Core was destroyed, that the older commanders died or escaped, or that
a retake is a new timeline.

**One thing outside the table:** objective titles, door texts and markers are not covered by the story test. The
proposed door text with three names (a-table) sits after two recordings and before the Warden, which is where the
document puts it; no title or marker before that door should name the pilots.
