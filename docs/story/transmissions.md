# Story transmissions: event ids and placeholder lines

Every story line the game plays is one **transmission**: an id that gameplay fires at a beat, with a speaker, a
placeholder line and a style. **Nicholas writes and voices against these ids**; nothing else is needed to get his words
into the game. The story is in [`storyline.md`](../../storyline.md); the short in-play barks are in
[`dialogs.md`](../../dialogs.md). Built by T3 (`Source/unrealGameJam/TriadDrift/Story/`), on the engineering integration
branch since Nov 13 and on `main` with engineering batch B.

## How to replace a line (Nick's side)
- Create a data asset **`DA_Story_Transmissions`** (class `UTDTransmissionSet`) in `Content/TriadDrift/Presentation/`.
- `Entries`: one per id: { `Id`, `Speaker`, `Text`, `Sound`, `SharedSound`, `Duration`, `Style`, `bRepeatable` }.
  Your entry replaces ours **field by field**: an entry with only a `Sound` keeps the placeholder text; an entry with
  only `Text` keeps our timing. Ids that only you define work too (we then need to know where to fire them).
- `SharedSounds` map: the key **`Dont`** is the ONE processed "Don't-" fragment, used by both `O_Unknown_Dont` (opening)
  and `F_FutureWren_Dont` (finale). Deliver one file; both lines play the same asset.
- Styles: `Clean`, `Static` (flickers), `Echo` (doubled). Speakers: Command, Control, PatrolSeven, Wren, Kato, Moss,
  Unknown, FutureWren, FutureKato, FutureMoss.
- On-screen speaker tags today: COMMAND, CONTROL, PATROL SEVEN, WREN, KATO, MOSS, UNKNOWN SIGNAL, and **WREN? / KATO? /
  MOSS?** for the Future voices (deniable until the reveal). Say if you want a different tag.
- Behaviour: once per run unless repeatable; saved, so Continue never replays a line; one line at a time, queued,
  held (not dropped) during a money shot or the intro; a transmission wins over a bark.
- Try them: console `TD.Story.Fire <Id>`, `TD.Story.List`, `TD.Story.Reset`.
- A test guards the twist: nothing before act 4 may have a Future speaker or say "future", "time" or "it's us".

## Ids and placeholder lines (the drafts are yours to rewrite)

### Opening narration (today's 49 s intro subtitles)
| Id | Speaker | Line |
|---|---|---|
| `Intro_N0` | Control | A ship appeared without warning. It answers nothing. |
| `Intro_N1` | Control | Its machines descend on our worlds, searching for something. |
| `Intro_N2` | Control | Triad is deployed. Wren. Kato. Moss. |
| `Intro_N3` | Control | Identify the ship. Learn what it wants. Report back. |
| `Intro_N4` | Control | And out of the dark... the Null Armada. |

### First Contact opening (the cinematic brief's draft lines; used when the opening is rebuilt)
| Id | Speaker (style) | Line |
|---|---|---|
| `O_PatrolSeven` | PatrolSeven (Static) | Command, we have an unidentified structure on the outer perimeter. It's not on any chart. Wait, something's coming out of- |
| `O_Command_Orders` | Command | Triad Squadron, investigate Patrol Seven's last coordinates. Identify the contact and report. Recon only. Do not engage. |
| `O_Wren_HowBad` | Wren | Three ships, one mystery. How bad could it be? |
| `O_Kato_StopSaying` | Kato | Please stop saying that. |
| `O_Moss_Especially` | Moss | Especially before we've seen it. |
| `O_Wren_BigShip` | Wren | Okay. That is a big ship. |
| `O_Kato_NoSignature` | Kato | No propulsion signature. No answer to our hails. Wait, we're receiving something. |
| `O_Moss_Message` | Moss | A message? |
| `O_Kato_Handshake` | Kato | More like a handshake. And now it's gone. |
| `O_Wren_WhyNotFiring` | Wren | Why isn't that one firing? |
| `O_Kato_Contacts` | Kato | Unknown. Multiple contacts are turning toward us. |
| `O_Command_Disengage` | Command | Triad, disengage. Repeat, disengage immediately. |
| `O_Unknown_Dont` | Unknown (Static, shared sound `Dont`) | Don't- |
| `O_Wren_Break` | Wren | Triad, break formation! |
| `O_Command_Evade` | Command | Pilot, incoming fire! Evade now! |

### Act 1 (the first battle)
| Id | Speaker | Line |
|---|---|---|
| `A1_HailNoAnswer` | Kato | Hailing on every band. Nothing. It isn't even listening. |
| `A1_TheySearch` | Moss | They're not holding ground. They scan, they sweep, they move on. |
| `A1_FieldUnreachable` | Wren | That shimmer around the ship: some kind of field. We can't get near it. |
| `A1_TrailToVerdance` | Control | Their landers are converging on Verdance. Follow the trail. |

### Act 2 (Verdance)
| Id | Speaker (style) | Line |
|---|---|---|
| `A2_DeviceFound` | Kato | A signal-analysis device. Someone here was listening to them. |
| `A2_Voices` | Unknown (Static) | ...[static] ...maintain the sequence... [static]... |
| `A2_TrailToCinder` | Moss | Human voices, under their signal. The data points to Cinder. |

### Act 3 (Cinder)
| Id | Speaker (style) | Line |
|---|---|---|
| `A3_CrystalsFound` | Kato | Samples secured. EMP prototype recovered. Lancer's pulse is online. |
| `A3_PassageOpen` | Kato | The anchor is down. The approach is prepared; passage is still blocked. |

### Sequences 02-09 (storyline.md §14-31; generated from the code table, Nov 13)
The one echo is at the Cinder terminal (`C5_Unknown_SameConstruction`). `C5_Kato_OriginCore` is the one line before
the reveal that says "temporal". The reveal is aboard the Dominion: the four recordings climb (Unknown, then future
Moss, Wren, Kato), then Cinematic 08 at the threshold (`PlaySequence("Cinematic08")`, skippable). "Boarding pilot" lines
are spoken by whoever flew the docked ship (issue #29). `F_FutureWren_FullWarning` does not reuse the "Dont" sound.

| Id | Speaker | Placeholder line |
|---|---|---|
| `A2_ConvoyScanned` | Kato | Scanner assemblies. Extraction rigs. No troops. They're surveying. |
| `A2_TransponderSighted` | Moss | There's a transponder down there. It keeps flickering. |
| `G2_Wren_Geology` | Wren | That's a lot of machines for a geology expedition. |
| `G2_Command_Archive` | Command | The outpost went dark twelve hours ago. Find its survey archive. Do not destroy the facility. |
| `G2_Kato_OneLine` | Kato | They're concentrating their scans along one underground line. That's not random. |
| `G2_Moss_LightsOn` | Moss | Lights are on. Doors are open. Nobody's answering. I don't like this. |
| `C3_Moss_NoBodies` | Moss | No bodies. No signs of a fight. Where did everybody go? |
| `C3_Wren_Survey` | Wren | Kato, see if the survey station can tell us anything. |
| `C3_Kato_Artificial` | Kato | Those are human voices. Buried in their signal. And this shape beneath the station is artificial. |
| `C3_Wren_NotThePlanet` | Wren | So they didn't come here to take the planet. |
| `C3_Kato_Beneath` | Kato | They're looking for something beneath it. |
| `C3_Wren_Answered` | Wren | Did we just answer that thing? |
| `C3_Kato_DidntSend` | Kato | I didn't send anything. |
| `C3_Kato_CinderMatch` | Kato | There's another match. Cinder. I'm copying the data- |
| `C3_Moss_Company` | Moss | Company incoming. A lot of it. |
| `C3_Wren_HoldThem` | Wren | Keep that transfer running. We'll hold them off. |
| `G3_TransferStart` | Kato | Transfer's moving. If they destroy this station, we lose everything. |
| `G3_Disruption` | Kato | Something heavy is coming in. The transfer's stalling. |
| `G3_DataSecured` | Kato | Device and recordings secured. I've got the Cinder coordinates. Let's move. |
| `G3_Moss_NobodyNear` | Moss | Then nobody gets near the station. |
| `G3_Wren_LeftApproach` | Wren | Left approach! Keep them off Kato. |
| `G3_Kato_AlmostThere` | Kato | Archive almost complete. Keep them off the station. |
| `G3_Command_Extraction` | Command | Triad, extraction corridor is open. Return with the archive intact. |
| `C4_Moss_Trouble` | Moss | All that for a recorder full of voices. None of them answering. |
| `C4_Wren_Badly` | Wren | They wanted that device and its records. We need to know why. |
| `C4_Kato_SameSignature` | Kato | They weren't searching at random. These two sites have the same geometric signature. |
| `C4_Wren_WhatConnects` | Wren | Verdance and Cinder. What connects them? |
| `C4_Kato_SurveyStation` | Kato | I don't know. Cinder's mining complex has matching readings. They're extracting stabilising crystals there. |
| `C4_Command_Proceed` | Command | Proceed to Cinder. Investigate the mining complex and recover the stabilising crystals. Compare them with the signal-analysis records. Maintain reconnaissance priority. |
| `C4_Moss_Volcanoes` | Moss | Volcanoes. Great. I was getting tired of the jungle. |
| `C4_Wren_SetCourse` | Wren | Triad, set course for Cinder. |
| `G4_Command_FollowCoordinates` | Command | Follow the survey coordinates. Identify the excavation before you engage. |
| `G4_Kato_Transports` | Kato | Those transports are all heading for the same site. |
| `G4_Moss_NotShooting` | Moss | They're not even shooting back. They've got somewhere to be. |
| `G4_Kato_Cargo` | Kato | Heavy cargo. Excavation gear, all of it. |
| `G4_Wren_StartThere` | Wren | Then that's where we start. Keep the landing route clear. |
| `G4_Moss_SolidGround` | Moss | Solid ground. Preferably the kind that stays solid. |
| `G4_Kato_TerminalAhead` | Kato | That pattern again. Get me to the survey terminal. |
| `C5_Kato_SameConstruction` | Kato | Different worlds. Same construction. They knew where to dig. |
| `C5_Unknown_SameConstruction` | Unknown (Echo) | Same construction. |
| `C5_Kato_Feedback` | Kato | Could be feedback. Keep the recording. |
| `C5_Kato_OriginCore` | Kato | The records call it the Origin Core. These look like components—or keys—to a temporal device. |
| `C5_Moss_Sample` | Moss | So we take a sample and get out? |
| `C5_Wren_Guarding` | Wren | First we find out what's guarding it. |
| `C5_Kato_Anchor` | Kato | The anchor is blocking the approach. We need the samples, then we shut it down. |
| `C5_Command_Recover` | Command | Recover the samples and preserve the survey data. Confirm a passage before approaching the Dominion. |
| `C5_Wren_Route` | Wren | We've got our route. Triad, move. |
| `G5_Wren_ClearRoute` | Wren | Clear the route to the cache. Watch the rig. |
| `G5_Moss_Guardian` | Moss | That one's staying with the crystals. |
| `G5_Command_AnchorMarked` | Command | Field anchor marked. Shut it down and confirm your exit. |
| `G5_Wren_BackToShips` | Wren | Keep the samples intact. Back to the ships. |
| `C6_Moss_SamplesAboard` | Moss | Samples aboard. I'm ready for somewhere with fewer volcanoes. |
| `C6_Kato_GuardLine` | Kato | The anchor is down. The approach is prepared. We still have to clear the guard line. |
| `C6_Kato_FourGenerators` | Kato | The crystals can stabilise the approach. Four generators are still sustaining its defensive shield. |
| `C6_Wren_FindAWayInside` | Wren | Then we take down the generators and find a way inside. |
| `C6_Command_FindOutWhy` | Command | They're collecting parts of the Origin Core. We need to know why. Disable the outer defences and secure access. Keep your channels recording. |
| `C6_Wren_Understood` | Wren | Understood. We find out what they intend to do with it. |
| `C6_Moss_WholeFleet` | Moss | They've put a whole fleet across our route. |
| `C6_Wren_StayTogether` | Wren | Stay together. Clear the guard line, then the generators. |
| `G6_Wren_ClearCarriers` | Wren | Clear the carriers. We need that approach. |
| `G6_Kato_MarkingTargets` | Kato | Four generators. I'm marking the live targets. |
| `G6_Moss_TurretsCharging` | Moss | Those turrets are charging. Keep moving. |
| `G6_Kato_ShieldWeakening` | Kato | Generator down. Their shield is weakening. |
| `G6_Command_ShieldCollapsed` | Command | Dominion shield collapsed. Docking approach marked. |
| `G6_Wren_CoverTheBay` | Boarding pilot | Keep that bay covered. We need a way back out. |
| `G6_Clue1` | Unknown (Static) | ...[static] ...hold formation... [static] ...vector two... |
| `G6_Clue2` | Unknown (Echo) | ...Triad... authentication pending... hold formation... |
| `C7_Moss_WayOut` | Moss | We'll keep a way out clear. |
| `C7_Kato_Layout` | Kato | That layout. We use the same arrangement. |
| `C7_Kato_Archive` | Kato | There's a local command archive ahead. |
| `C7_Wren_Record` | Boarding pilot | Record everything. Find who's giving the orders. |
| `G7_Moss_MyVoice` | Moss | That's my voice in those records. Older. |
| `G7_Kato_Authorisations` | Kato | Those authorisations match all three of us. |
| `G7_Wren_FullRecord` | Boarding pilot | We hear the full record before we touch that reactor. |
| `F_FutureWren_FullWarning` | FutureWren | Don't engage the Dominion. You don't understand what happens if you do. |
| `C8_RecKato_OneEvent` | FutureKato (Echo) | The Origin Core may let us change one event. Before everything was lost. |
| `C8_RecWren_LastChance` | FutureWren (Echo) | We believe recovering it can prevent the catastrophe. It's the last chance we have. |
| `C8_RecMoss_SaveYou` | FutureMoss (Echo) | We came back to save you. |
| `C8_Kato_NoSafeShutdown` | Kato | I can't verify a safe shutdown. Continuing could put the whole system at risk. |
| `C8_Moss_CantLetThem` | Moss | Then we can't let them finish. |
| `C8_Wren_Order` | Boarding pilot | Take out the reactor. Keep the route to the hangar clear. |
| `G8_Wren_ConduitsFirst` | Boarding pilot | Conduits first. Then the heart. |
| `G8_Kato_ConduitDown` | Kato | Conduit destroyed. |
| `G8_Moss_KeepMoving` | Moss | Keep moving. Watch the way back. |
| `G8_Kato_HeartExposed` | Kato | The heart is exposed. |
| `G8_Command_ReactorLost` | Command | Reactor lost. Get back to the hangar. |
| `G9_Moss_HoldingWayOut` | Moss | Keep moving. We're holding the way out. |
| `G9_Kato_HangarConfirmed` | Kato | Hangar signal confirmed. Get the ship clear. |
| `G9_Wren_PullAway` | Wren | Triad, pull away. Follow the clear vector. |
| `C9_Kato_Clear` | Kato | Lancer clear. |
| `C9_Moss_Clear` | Moss | Bastion clear. Still here. |
| `C9_Wren_Clear` | Wren | Rift clear. All three accounted for. |
| `C9_Moss_TakeThat` | Moss | All three. I'll take that. |
| `C9_Kato_Identifiers` | Kato | A distress signal. Those are our identifiers. |
| `C9_Wren_Source` | Wren | The source? |
| `C9_Kato_Decades` | Kato | It's dated decades ahead. |

### Act 4 (the Dominion)
| Id | Speaker (style) | Line |
|---|---|---|
| `A4_GeneratorDown1` | Control | Generator one is down. Their shield is flickering. |
| `A4_GeneratorDown2` | Kato | Two down. The shield is losing shape. |
| `A4_GeneratorDown3` | Wren | Three! One more and it's open. |
| `A4_GeneratorDown4` | Moss | That's the last one. The shield is gone. |
| `A4_DoorsKnowUs` | Boarding pilot | They shouldn't have our access codes. |
| `A4_Recording_Hangar` | Unknown (Echo) | Keep the recovery teams on the Core search. |
| `A4_Recording_Corridor` | FutureMoss (Echo) | Bring every recovery team back. |
| `A4_Recording_Barracks` | FutureWren (Echo) | Recover the Core. Protect the younger Triad. |
| `A4_Recording_Core` | FutureKato (Echo) | We are you, from the future. We came back for the Origin Core. |

### Finale payoff (the future command deck)
| Id | Speaker (style) | Line |
|---|---|---|
| `F_FutureKato_Signatures` | FutureKato | All three signatures confirmed. It's us. |
| `F_FutureMoss_Where` | FutureMoss | They're exactly where we were. |
| `F_FutureWren_Channel` | FutureWren | Open a channel. Now. |
| `F_FutureKato_Interference` | FutureKato | Temporal interference. Less than a second. |
| `F_FutureWren_Dont` | FutureWren (Static) | Don't- |

### Act 5
| Id | Speaker (style) | Line |
|---|---|---|
| `A5_Choice` | Moss | We can't trust them, and we can't let them finish. We end it here. |
| `A5_Escape` | Control | The ship is coming apart. Ninety seconds. Move! |
| `A5_DistressSignal` | Unknown (Static) | DISTRESS SIGNAL  ·  TRIAD: WREN, KATO, MOSS  ·  DATED DECADES AHEAD |

### Sequence 02, *Aftermath* (plays on T7's OnAftermathBegins: the tutorial's escape, or a skipped tutorial)
| Id | Speaker (style) | Line |
|---|---|---|
| `S2_Moss_Accounted` | Moss | All three ships accounted for. I'm calling that a win. |
| `S2_Wren_WelcomeParty` | Wren | Is it over? I think I blacked out... (approved; delivery: sarcastic, dry, deadpan) |
| `S2_Kato_NotPursuing` | Kato | Hold on. They're not pursuing us. |
| `S2_Kato_Vectors` | Kato | Look at their vectors. Most of those craft never joined the fight. |
| `S2_Wren_WhatDoing` | Wren | Then what are they doing? |
| `S2_Kato_Planetside` | Kato | Heading planetside. Every group. Same destination. |
| `S2_Moss_InTheWay` | Moss | So we got in the way of something. |
| `S2_Command_Report` | Command (Static) | Triad, report. We lost your telemetry when you approached the unidentified vessel. |
| `S2_Wren_Hostile` | Wren | Confirmed hostile. Automated fighters, heavy numbers. We're intact, barely. |
| `S2_Kato_Coordinated` | Kato | They've launched a coordinated operation toward Verdance. This wasn't a defensive scramble. |
| `S2_Command_Outpost` | Command | We've received an emergency report from a research outpost there. Unidentified machines landed near a restricted survey zone. The outpost has gone dark. |
| `S2_Moss_FindOut` | Moss | And you want us to find out why. |
| `S2_Command_Orders` | Command | Affirmative. Avoid direct engagement with the mothership. Reach Verdance, locate the outpost, and recover its survey data. We need to know what they're looking for. |
| `S2_Wren_FormUp` | Wren | Triad, form up. Verdance it is. |
| `S2_Kato_Recorders` | Kato | And keep your recorders running. Nothing about this adds up. |
