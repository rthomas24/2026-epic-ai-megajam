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
| `O_PatrolSeven` | PatrolSeven (Static) | Command, there's something out here. |
| `O_Command_Orders` | Command | Triad, identify the ship and report. Recon only. Do not engage. |
| `O_Wren_HowBad` | Wren | Three ships, one mystery. How bad could it be? |
| `O_Kato_StopSaying` | Kato | Please stop saying that. |
| `O_Moss_Especially` | Moss | Especially before we've seen it. |
| `O_Wren_BigShip` | Wren | That's all one ship? |
| `O_Kato_NoSignature` | Kato | No engine readings. And they're not answering. |
| `O_Moss_Message` | Moss | A message? |
| `O_Kato_Handshake` | Kato | Wait—a handshake. Encrypted. It's gone. |
| `O_Wren_WhyNotFiring` | Wren | What? Why didn't it shoot us? |
| `O_Kato_Contacts` | Kato | Unknown. Multiple contacts are turning toward us. |
| `O_Command_Disengage` | Command | Triad, pull out. Now. |
| `O_Unknown_Dont` | Unknown (Static, shared sound `Dont`) | Don't- |
| `O_Wren_Break` | Wren | Break formation! |
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
| `A2_DeviceFound` | Kato | Signal-analysis gear. They were listening to the machines. |
| `A2_Voices` | Unknown (Static) | Maintain the sequence. |
| `A2_TrailToCinder` | Moss | Human voices, under their signal. The data points to Cinder. |

### Act 3 (Cinder)
| Id | Speaker (style) | Line |
|---|---|---|
| `A3_CrystalsFound` | Kato | Samples secured. EMP prototype recovered. Lancer's pulse is online. |
| `A3_PassageOpen` | Kato | Anchor's down. Route's prepared, but the field's still closed. |
### Sequences 02-09 (storyline.md §14-31; generated from the code table, Nov 13)
The one echo is at the Cinder terminal (`C5_Unknown_SameConstruction`). `C5_Kato_OriginCore` is the one line before
the reveal that says "temporal". The reveal is aboard the Dominion: the four recordings climb (Unknown, then future
Moss, Wren, Kato), then Cinematic 08 at the threshold (`PlaySequence("Cinematic08")`, skippable). "Boarding pilot" lines
are spoken by whoever flew the docked ship (issue #29). `F_FutureWren_FullWarning` does not reuse the "Dont" sound.

| Id | Speaker | Placeholder line |
|---|---|---|
| `A2_ConvoyScanned` | Kato | Those transports are carrying survey gear. Not troops. |
| `A2_TransponderSighted` | Moss | There's a transponder down there. It keeps flickering. |
| `G2_Wren_Geology` | Wren | They brought all this just to dig? |
| `G2_Command_Archive` | Command | Twelve hours since their last transmission. Recover the archive. Keep the station intact. |
| `G2_Kato_OneLine` | Kato | Look where they're scanning. One line, under the ground. |
| `G2_Moss_LightsOn` | Moss | Power's still on. You'd think someone would answer. |
| `C3_Moss_NoBodies` | Moss | No bodies. No sign of a fight, either. |
| `C3_Wren_Survey` | Wren | Kato. Tell me what we've got. |
| `C3_Kato_Artificial` | Kato | Those are people. Listen. And the thing under the station—someone built it. |
| `C3_Wren_NotThePlanet` | Wren | They're digging for that? |
| `C3_Kato_Beneath` | Kato | Every survey line runs over it. |
| `C3_Wren_Answered` | Wren | That was our radio. What did you send? |
| `C3_Kato_DidntSend` | Kato | Nothing. I didn't touch it. |
| `C3_Kato_CinderMatch` | Kato | Cinder. Same readings at a mining site. Hang on, I'm taking a copy— |
| `C3_Moss_Company` | Moss | We've got company. Coming in fast. |
| `C3_Wren_HoldThem` | Wren | Keep copying. We'll cover the station. |
| `G3_TransferStart` | Kato | Transfer's moving. If they destroy this station, we lose everything. |
| `G3_Disruption` | Kato | The copy's stalled. Something's interfering. |
| `G3_DataSecured` | Kato | Got it. Recordings, coordinates—the lot. Let's get out. |
| `G3_Moss_NobodyNear` | Moss | Heard you. Keep it moving. |
| `G3_Wren_LeftApproach` | Wren | Left side! They're going for the console. |
| `G3_Kato_AlmostThere` | Kato | Archive almost complete. Keep them back. |
| `G3_Command_Extraction` | Command | Your way out is marked. Bring that device back. |
| `C4_Moss_Trouble` (retired, N-087) | Moss | All that for a recorder full of voices. None of them answering. |
| `C4_Wren_Badly` | Wren | They wanted that device and its records. We need to know why. |
| `C4_Kato_SameSignature` (retired, N-087) | Kato | They weren't searching at random. These two sites have the same geometric signature. |
| `C4_Wren_WhatConnects` (retired, N-087) | Wren | Verdance and Cinder. What connects them? |
| `C4_Kato_SurveyStation` | Kato | I don't know. Cinder's mining complex has matching readings. They're extracting stabilising crystals there. |
| `C4_Command_Proceed` | Command | Proceed to Cinder. Investigate the mining complex and recover the stabilising crystals. Compare them with the signal-analysis records. Maintain reconnaissance priority. |
| `C4_Moss_Volcanoes` (retired, N-087) | Moss | Volcanoes. Great. I was getting tired of the jungle. |
| `C4_Wren_SetCourse` (retired, N-087) | Wren | Triad, set course for Cinder. |
| `G4_Command_FollowCoordinates` | Command | Find the excavation before you engage. |
| `G4_Kato_Transports` | Kato | Watch the transports. Same landing point. |
| `G4_Moss_NotShooting` | Moss | They've got us in sight. Still not interested. |
| `G4_Kato_Cargo` | Kato | Digging equipment. Heavy loads. |
| `G4_Wren_StartThere` | Wren | Then we follow them down. |
| `G4_Moss_SolidGround` | Moss | I'm putting down well clear of the lava. |
| `G4_Kato_TerminalAhead` | Kato | There. On the terminal. I've seen that shape. |
| `C5_Kato_SameConstruction` | Kato | Different worlds. Same construction. They knew where to dig. |
| `C5_Unknown_SameConstruction` | Unknown (Echo) | Same construction. |
| `C5_Kato_Feedback` | Kato | Could be feedback. Keep the recording. |
| `C5_Kato_OriginCore` | Kato | The records call it the Origin Core. These look like components—or keys—to a temporal device. |
| `C5_Moss_Sample` | Moss | How much of this stuff do we need? |
| `C5_Wren_Guarding` | Wren | Let's get past the thing guarding it first. |
| `C5_Kato_Anchor` | Kato | That's the field anchor. We'll need it shut down as well. |
| `C5_Command_Recover` | Command | Recover what you need and get clear. Report before you approach the Dominion. |
| `C5_Wren_Route` | Wren | Fine. We start with the cache. |
| `G5_Wren_ClearRoute` | Wren | Cache is behind it. Stay out of its reach. |
| `G5_Moss_Guardian` | Moss | It's not leaving the crystals. We'll have to get past it. |
| `G5_Command_AnchorMarked` | Command | Anchor's marked. Disable it, then head for extraction. |
| `G5_Wren_BackToShips` | Wren | We've got what we came for. Back to the ships. |
| `C6_Moss_SamplesAboard` (retired, N-087) | Moss | Samples aboard. I'm ready for somewhere with fewer volcanoes. |
| `C6_Kato_GuardLine` | Kato | The anchor is down. The approach is prepared. We still have to clear the guard line. |
| `C6_Kato_FourGenerators` | Kato | The crystals can stabilise the approach. Four generators are still sustaining its defensive shield. |
| `C6_Wren_FindAWayInside` (retired, N-087) | Wren | Then we take down the generators and find a way inside. |
| `C6_Command_FindOutWhy` | Command | They're collecting parts of the Origin Core. We need to know why. Disable the outer defences and secure access. Keep your channels recording. |
| `C6_Wren_Understood` (retired, N-087) | Wren | Understood. We find out what they intend to do with it. |
| `C6_Moss_WholeFleet` | Moss | They've put a whole fleet across our route. |
| `C6_Wren_StayTogether` | Wren | Stay together. Clear the guard line, then the generators. |
| `G6_Wren_ClearCarriers` | Wren | Take the carriers. Don't let them keep launching. |
| `G6_Kato_MarkingTargets` | Kato | Four shield generators. Targets marked. |
| `G6_Moss_TurretsCharging` | Moss | Hull turrets. Keep moving across them. |
| `G6_Kato_ShieldWeakening` | Kato | Generator down. Shield's weakening. |
| `G6_Command_ShieldCollapsed` | Command | Shield's down. Docking route marked. |
| `G6_Wren_CoverTheBay` | Boarding pilot | Keep that bay covered. We need a way back out. |
| `G6_Clue1` | Unknown (Static) | …hold formation… vector two… |
| `G6_Clue2` | Unknown (Echo) | …Triad… wait for authentication… |
| `C7_Moss_WayOut` | Moss | We'll keep a way out clear. |
| `C7_Kato_Layout` | Kato | That layout. We use the same arrangement. |
| `C7_Kato_Archive` | Kato | There's a local command archive ahead. |
| `C7_Wren_Record` | Boarding pilot | Record everything. Find who's giving the orders. |
| `G7_Moss_MyVoice` | Moss | That's my voice in those records. Older. |
| `G7_Kato_Authorisations` | Kato | Those authorisations match all three of us. |
| `G7_RecKato_SameConstruction` | Future Kato (reads KATO (RECORDED)) | Same construction. (R102: opens Cinematic08, distinct from the Cinder echo) |
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
| `G9_Moss_HoldingWayOut` | Moss (Wren when Moss boarded, N-087: `G9_Wren_HoldingWayOut`) | Keep moving. We're holding the way out. |
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
| `S2_Moss_Accounted` | Moss | Well, we're not dead. I'd say that's a win. |
| `S2_Wren_WelcomeParty` | Wren | Is it over? I think I blacked out... (approved; delivery: sarcastic, dry, deadpan) |
| `S2_Kato_NotPursuing` | Kato | Wait. Look behind us. |
| `S2_Kato_Vectors` | Kato | Most of their fleet never even joined the fight. |
| `S2_Wren_WhatDoing` | Wren | Would you rather they had? |
| `S2_Kato_Planetside` | Kato | They're after something. Doesn't look like it's us. |
| `S2_Moss_InTheWay` | Moss | So we got in the way. |
| `S2_Command_Report` | Command (Static) | Triad, answer me. We lost you near that ship. |
| `S2_Wren_Hostile` | Wren | Fighters. A lot of them. All three of us got out. |
| `S2_Kato_Coordinated` | Kato | They're sending the rest down to Verdance. They didn't launch them for us. |
| `S2_Command_Outpost` | Command | An outpost there sent a distress beacon. Machines in the survey zone. Then it went quiet. |
| `S2_Moss_FindOut` | Moss | Anyone answer the beacon? |
| `S2_Command_Orders` | Command | No response. Find the outpost and recover its survey records. Stay clear of the mothership. |
| `S2_Wren_FormUp` | Wren | All right. Verdance. Stay on me. |
| `S2_Kato_Recorders` | Kato | Leave the recorders on. All channels. |