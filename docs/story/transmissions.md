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
| `A2_Voices` | Unknown (Static) | ...hold the line... vector two... [static] ...is anyone receiving... |
| `A2_TrailToCinder` | Moss | Human voices, under their signal. The data points to Cinder. |

### Act 3 (Cinder)
| Id | Speaker (style) | Line |
|---|---|---|
| `A3_CrystalsFound` | Kato | These crystals stabilise the field. They were digging a way through it. |
| `A3_EchoWren` | Unknown (Echo) | ...I see an opening. I'm taking it... |
| `A3_EchoKato` | Unknown (Echo) | ...understand it before we commit... |
| `A3_EchoMoss` | Unknown (Echo) | ...stay together. We're not done yet... |
| `A3_PassageOpen` | Control | The anchor is down. There's a passage through the field. Go. |

### Act 4 (the Dominion)
| Id | Speaker (style) | Line |
|---|---|---|
| `A4_GeneratorDown1` | Control | Generator one is down. The field is flickering. |
| `A4_GeneratorDown2` | Kato | Two down. The field is losing shape. |
| `A4_GeneratorDown3` | Wren | Three! One more and it's open. |
| `A4_GeneratorDown4` | Moss | That's the last one. The field is gone. |
| `A4_DoorsKnowUs` | Wren | The door opened for my ID. Why does it know my ID? |
| `A4_Recording_Hangar` | FutureWren (Echo) | If you're hearing this, you came anyway. Of course you did. |
| `A4_Recording_Corridor` | FutureKato (Echo) | We ran every number a thousand times. This is the only way. |
| `A4_Recording_Barracks` | FutureMoss (Echo) | We were you, once. We know what you'll choose. |
| `A4_Recording_Core` | FutureWren (Echo) | The Origin Core can stop it. Don't take that from us. |

### Finale payoff (the future command deck)
| Id | Speaker (style) | Line |
|---|---|---|
| `F_FutureKato_Signatures` | FutureKato | All three signatures confirmed. It's us. |
| `F_FutureMoss_Where` | FutureMoss | They're exactly where we were. |
| `F_FutureWren_Channel` | FutureWren | Open a channel. Now. |
| `F_FutureKato_Interference` | FutureKato | Temporal interference. Less than a second. |
| `F_FutureWren_Dont` | FutureWren (Static, shared sound `Dont`) | Don't- |

### Act 5
| Id | Speaker (style) | Line |
|---|---|---|
| `A5_Choice` | Moss | We can't trust them, and we can't let them finish. We end it here. |
| `A5_Escape` | Control | The ship is coming apart. Ninety seconds. Move! |
| `A5_DistressSignal` | Unknown (Static) | DISTRESS SIGNAL · TRIAD: WREN, KATO, MOSS · DATED FORTY-ONE YEARS FROM TODAY |

### To be added: Sequence 02, *Aftermath* (Nicholas's brief of Nov 13; ids reserved `S2_*`)
Moss "All three ships accounted for...", Wren "If that was a welcome party...", Kato "Hold on. They're not pursuing
us.", the vectors exchange, Command's new orders, Wren "Triad, form up. Verdance it is.", Kato "And keep your recorders
running." T3 seeds these with the brief's draft lines next.
