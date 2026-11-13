# Cutscene and transmission plan, the whole game (T3, director)

Status, Nov 13 evening. Story order follows `Story/README.md` (entries 1-18) and `storyline.md` §11-32. Every line is a
transmission id in `UTDStorySubsystem` (`docs/story/transmissions.md`); Nicholas replaces words and voices through
`DA_Story_Transmissions`, field by field, with no code change. "Built" means on `eng/t3` and handed to the Lead; nothing
here is gameplay validated until it is played.

## The three forms we build

| Form | What it is | Cost | Used for |
|---|---|---|---|
| **Full cutscene** | `UTDIntroDirector` code shots: our camera, letterbox, name cards, subtitles, hold-to-skip, wall-clock timeline | High, needs final art on screen | Cinematic 01 (the opening) only |
| **Voice over the room** | The player keeps control (or stands at a console); a story sequence of lines plays over the place, with T2's scan monitor (`ShowScanMonitor(1..5)`) or tactical display (`ShowTacticalDisplay`) where the document asks for a graphic | Medium, no camera, no faces | Cinematics 03 and 05; Aftermath (02) |
| **Transmissions only** | A queued set of lines (`PlaySequence` or a departure set), subtitled, once per run, skippable when it gates the game | Low | Cinematics 04, 06, 07, 08, 09 and every gameplay sequence's radio |

All three keep the same rules: once per run (kept in the save), never replayed on a retake, held during money shots and
the intro, the slate waits under T1's entry hold, combat presentation never covers them.

## Every cinematic and transmission set, in story order

| # | Sequence | Form | Trigger hook (owner) | Length | Lines (ids) | Waits for (art gate) | State |
|---|---|---|---|---|---|---|---|
| 1 | **Cinematic 01, First Contact** | Full cutscene (today's approved 49 s cut) | New Game → `UTDIntroDirector::Play` (T3); control at `OnIntroFinished` (music) | 49 s | `Intro_N0-4` narration; `O_*` for the 140 s rebuild | The rebuilt squad ships (docs/eng/SHIP-QUALITY.md, Rift first), Rodin Null fleet (in), the Dominion exterior and generators (#41) | **Built** (49 s, wall clock 6cd9f45, cards #49 3c76874, Rodin fleet d8793bb). Re-shoot after the ships |
| 2 | Gameplay 01, Tutorial battle | Transmissions | Wave 1 start, first switch, gunship spawn (T4), withdrawal ordered, escape boundary (T4: `OnTutorialWithdrawalOrdered`, `OnTutorialEscaped`) | 6-10 min | `A1_HailNoAnswer`, `TB_Kato_Marking`, `TB_Moss_ThreeShips`, `TB_Wren_Bigger`, `TB_Command_FallBack`, `TB_Kato_BreakingOff` | None | Ids built; fires: mine queued on T4's events |
| 3 | **Cinematic 02, Aftermath** | Voice over the space, T2's tactical display + navigation pulse | `OnTutorialEscaped` (T4) | 75-100 s | `S2_*` (15) | None | Lines built; firing queued on T4's event |
| 4 | Gameplay 02, Verdance arrival | Transmissions | Descent, convoy scan, landing, device pick-up (T1, T4, T5) | 3-4 min | `A2_ConvoyScanned`, `A2_TransponderSighted`, `G2_*`, `A2_DeviceFound` | None | Built; T4/T5 fire |
| 5 | **Cinematic 03, Beneath the Surface** | Voice over the station, scan monitor stages 1-3, the dormant drone wakes blue (three pulses) | `ATDArchiveTransfer::OnAnyTerminalActivated` (T5); drone `ATDPlanetCourse::GetDormantDrone()` | 40-60 s | `C3_*` (10), `A2_Voices` ("maintain the sequence") | #42 survey station and console (Grok Bot), #37 relay artefact | Lines built; scene **to build** |
| 6 | Gameplay 03, Facility defense | Transmissions | Transfer start / Heavy / done (T5) | 60-90 s | `G3_*` | None | Built; T5 fires |
| 7 | Cinematic 04, Verdance departure | Transmissions over the take-off | Back in the galaxy after Verdance (T3, `UpdateDepartures`) | ~35 s | `C4_*` (8) | None | **Built** (d89a004) |
| 8 | Gameplay 04, Cinder approach | Transmissions | Waves 3-4, landing, terminal step (T4, T5) | 6-8 min chapter | `G4_*` (7) | None | Built; T5 fires two, T4 the transports |
| 9 | **Cinematic 05, The Pattern** | Voice at the terminal, scan monitor stages 4-5 (the Cinder overlay, cache and anchor markers) | `ATDSurveyTerminal::OnAnySurveyRecordsAccessed` (T5) | 40-50 s | `C5_*` (9, the one echo) | #38 dig site and drill rig (the view from the station), #42 | Lines built; scene **to build** |
| 10 | Gameplay 05, Cinder excavation | Transmissions | Excavation, Warden, cache, anchor, run (T5) | 4-5 min | `G5_*`, `A3_CrystalsFound`, `A3_PassageOpen` | None | Built; T5 fires |
| 11 | Cinematic 06, The Path to Dominion | Transmissions over the take-off, two as wave 5 forms | Back in the galaxy after Cinder; wave 5 start (T3) | ~45 s | `C6_*` (8) | None | **Built** (d89a004) |
| 12 | Gameplay 06, Dominion assault | Transmissions + clue fragments | Wave 5, generators revealed, each generator, docking (T4) | 5-7 min | `G6_*`, `A4_GeneratorDown1-4`, `G6_Clue1-2` | #41 generators (gameplay works without) | Built; T4 fires |
| 13 | Cinematic 07, Boarding | Transmissions over the docking and the hangar | `PlaySequence("Docking")` (T4), `"HangarArrival"`, `"FirstDoor"` (T5) | ~40 s | `G6_Wren_CoverTheBay`, `C7_*`, `A4_DoorsKnowUs` | Optional hangar dressing | **Built** (6c4979f); T4/T5 fire |
| 14 | Gameplay 07, Inside the Dominion | Console recordings + radio | Four consoles, Warden door (T5) | 6-8 min | `A4_Recording_*` (climbing identity), `G7_*` | None | Built; T5 fires |
| 15 | Cinematic 08, The Recovered Transmission | Voices over the sealed threshold; mandatory, skippable; opens the reactor door | `PlaySequence("Cinematic08")` at T5's console gate → `OnSequenceFinished` | 60-80 s | `F_*` (5, the shared "Dont"), `F_FutureWren_FullWarning`, `C8_*` | Optional: a command-deck picture on the console screen (cut first) | **Built** (6c4979f); T5's gate |
| 16 | Gameplay 08, The reactor fight | Transmissions | Room start, conduits, heart, completion (T5) | 2-3 min | `G8_*`, `A5_Escape` | None | Built; T5 fires |
| 17 | Gameplay 09, Escape | Transmissions | Run start, hangar trigger (T5), flight start (T4) | 90 s + 60 s | `G9_*` | None | Built; T4/T5 fire |
| 18 | Cinematic 09, The Paradox | Transmissions + the victory backdrop's wreck, the stinger caption held on black | Escape radius `PlaySequence("ClearSpace")` (T4); victory 10 s and 18 s (T3) | 18 s + ~10 s | `C9_*`, `A5_DistressSignal` | None | **Built** (6c4979f); leave-early path untested |

## The art gate

| Asset | Who | Needed by |
|---|---|---|
| Rebuilt squad ships (Rift first, then Lancer, Bastion) | Ship builder (Blender over Rodin) | Cinematic 01 re-shoot, name cards, trailer |
| Rodin Null fleet (fighter, interceptor) | In (lead/int) | Cinematic 01 (done) |
| #42 Verdance research outpost and console | Grok Bot | Cinematics 03 and 05 (the room both play in) |
| #37 Ancient relay / artefact | Grok Bot | Cinematic 03 |
| #39 Crashed ship wreck | Grok Bot | Gameplay 02 (not a cinematic) |
| #38 Cinder dig site and drill rig | Grok Bot | Cinematic 05 (the view of the excavation) |
| #40 Outpost and pad dressing | Grok Bot | Dressing only |
| #41 Dominion exterior and generators | Grok Bot | Cinematic 01 re-shoot, the assault |

Without them, Cinematics 03 and 05 still work as voice plus the scan monitor in the current station kit; the art makes
them look right, it does not unblock them.

## Build order (T3), with estimates

| Order | Work | Depends on | Estimate |
|---|---|---|---|
| 1 | Tutorial radio and **Aftermath** (S2_ + tactical display + nav pulse) on T4's `OnTutorialEscaped`; "HEAVY CONTACT" wording | T4's event in lead/int | 1.5-2 h |
| 2 | First play and repeat: profile flag at Aftermath's end, "skip the tutorial" on a later New Game | T1 (menu arm), T2 (prompt) | 2.5-3 h |
| 3 | **Cinematic 05** at the terminal: `C5_*` as a sequence, scan monitor stages 4 → 5 keyed to the lines, combat held, return to "Secure the excavation"; skip | None (T5's hook and T2's monitor are in) | 2 h |
| 4 | **Cinematic 03** at the station: `C3_*`, monitor stages 1 → 3, the dormant drone's three pulses red → blue, return with the transfer pending; skip | None for a first pass; #42 / #37 for the look | 2.5 h |
| 5 | Leave-early stinger and the ending played once end to end; transmissions.md for the new ids | None | 1 h |
| 6 | Cinematic 01: the 140 s First Contact rebuild (`docs/eng/intro-first-contact-plan.md`) | The rebuilt ships, #41, the Lead's go | 6-8 h; or keep the approved 49 s cut |
| 7 | Re-shoot: intro, name cards, trailer | The rebuilt ships | 1 h |

**The one with no art dependency, started next:** Cinematic 05 (order 3) as soon as order 1 is waiting on T4, since its
hook, its graphic and all nine lines exist today.
