# Triad Drift: gameplay dialogue (barks)

The short lines pilots say **during play**: calling targets, taking hits, using powers, switching, landing, fighting
on foot. Story dialogue (the intro, transmissions, recordings, the ending) is in [`storyline.md`](storyline.md) and
`docs/story/`; this file is everything else. Every agent that adds a gameplay moment worth a line looks here first:
use an existing id, or add a row.

**Status of the words:** these are **working lines written by Ryan's side (Nov 13)** so the game is never mute and
every trigger has text. **Nicholas owns the final wording and all voices** (ART-11): his `DT_Barks` rows, keyed by the
same ids, replace these with no code change. Change a line here freely; change an **id** only with T3.

**November 13 review status.** Nicholas's [second script draft](Story/Triad_Drift_Complete_Script_Second_Draft.docx) is published at 2240d3cd and is the current team review copy at his request. That draft has no recording approval yet. The approved archive-defense lines are maintained in [the transmission table](docs/story/transmissions.md), including Kato's “Archive almost complete. Keep them back.” Keep these story cues under their existing IDs.

## 1. Voices (from `storyline.md` §3; keep every line in character)
| Speaker | Voice | Says things like | Never |
|---|---|---|---|
| **Wren** (Rift) | Bold, instinctive, quick; grins at danger; short sentences | "Got a gap. Taking it." | Hesitates, explains |
| **Kato** (Lancer) | Analytical, cautious, precise; reports numbers and facts; dry | "Shields at forty. Adjusting." | Brags, guesses |
| **Moss** (Bastion) | Steady, warm, good-humoured; looks after the others; unhurried | "Easy. I've got you covered." | Panics, snaps |
| **Command** | Calm military controller; orders and information only | "Triad, new contacts, bearing two-seven." | Jokes |
| **Armada** | No words. Machine tones, a clipped synthetic burst | (sound only) | Speaks before act 4 |

Rules for every line: **under 8 words where possible, 2 seconds spoken at most**; plain radio speech, no jargon the
player has not been taught; says something the player can use or feel; no line reveals or hints at the truth about the
enemy (before act 4 nobody says "future", "time" or "us" about them); no swearing; no real-world references.

## 2. How a line is fired (so nothing is spammed)
- Code asks for a line with `RequestBark(BarkId, Speaker)` (`Squad/TDSquadBarks.h`, ids in `TDBarkIds`). A limiter decides
  if it plays: a global gap between lines, a longer gap before the same id repeats, and a priority.
- **Priorities:** `Critical` (danger the player must act on: always plays, interrupts) · `High` (state changes: ship
  down, power ready, objective) · `Normal` (callouts) · `Flavour` (banter; only when nothing else spoke for a while,
  never in the last seconds of a fight).
- **Variety:** each id has several lines per speaker; the same line never plays twice in a row, and a line is not
  reused within 60 s.
- **Who speaks:** the pilot it concerns. The flown ship's pilot speaks for the player's own actions; wingmen speak
  for theirs and react to the player. During a money shot, the slate or "CUT!", nothing but `Critical` plays.
- **Shown as:** a one-line subtitle with the speaker's name in their colour (Settings: subtitles on by default), plus
  audio when Nicholas's voice exists.
- A story transmission (`storyline.md`) always wins over a bark.

## 3. Space combat
Ids marked **(exists)** are fired by the game today; the rest are **(new)**: the owner adds the trigger.

| Id | When | Speaker | Priority | Wren | Kato | Moss |
|---|---|---|---|---|---|---|
| `Bark_SquadEngage<Dir>` (exists; 12 directions) | A wingman picks a target; direction is from the player's view | that wingman | Normal | "Bandit, {dir}. Mine." / "On the one {dir}!" | "Contact {dir}. Engaging." / "Target {dir}, locked." | "I see him, {dir}. Going in." / "{Dir}. I've got this one." |
| `Bark_SquadWingmanKill` (exists) | A wingman destroys an enemy | that wingman | Normal | "Splash one!" / "And he's gone." | "Target destroyed." / "Confirmed kill." | "That's one less." / "Down he goes." |
| `Bark_PlayerKill` (new, T3) | The player destroys an enemy; a wingman reacts, 1 in 3 | a wingman | Flavour | "Nice shot!" / "Leave some for me." | "Clean hit." / "Efficient." | "Good shooting." / "That's the way." |
| `Bark_MultiKill` (new, T3) | 3 kills within 4 s | a wingman | High | "Now that's flying!" | "Three down. Noted." | "Ha! Save some for us." |
| `Bark_SquadLowHull` (exists) | A pilot's hull under 35% | that pilot | Critical | "Taking a beating here!" / "Hull's going, need a second!" | "Hull critical. Thirty percent." / "I'm losing armour." | "She's hurting. Could use a hand." / "Hull's low. Still here." |
| `Bark_ShieldDown` (new, T3) | A pilot's shield breaks | that pilot | High | "Shield's gone!" | "Shields down. Recharging." | "Lost my shield. Holding." |
| `Bark_ShieldBack` (new, T3) | Shield full again after a break | that pilot | Flavour | "Shield's back. Let's go." | "Shields restored." | "Shield's up. Much better." |
| `Bark_SquadShipDown<Ship>` (exists) | A ship is destroyed; a squadmate reacts | a squadmate | Critical | "Kato's down!" / "Moss is out, cover him!" | "Wren is down." / "We've lost Bastion." | "Wren's hit! Hang on." / "Kato's out. I'll hold." |
| `Bark_SquadRespawn` (exists) | A downed pilot is back | that pilot | High | "Back in it. Miss me?" | "Systems restored. Rejoining." | "I'm back. Where do you want me?" |
| `Bark_SquadWipe` / `Bark_Wipe` (exists) | The whole squad is down (before "CUT!") | last pilot | Critical | "No, no, no..." | "That's all of us." | "Sorry, team." |
| `Bark_MissileIncoming` (new, T2 / T3) | A missile locks the flown ship | a wingman | Critical | "Missile on you, break!" | "Missile lock. Evade." | "Missile! Move, move." |
| `Bark_MissileEvaded` (new) | That missile misses or is returned | flown pilot | Flavour | "Too slow." | "Evaded." | "That was close." |
| `Bark_OnMySix` (new, T4 / T3) | An enemy holds behind the flown ship for 3 s | a wingman | High | "One on your tail!" | "Bandit behind you." | "Check your six." |
| `Bark_Anticipated` (new, T4 / T3; storyline §6 clue) | An enemy pre-empts the player's switch or dodge (T4's UTDArmadaForesight::OnAnticipated, wave 3 on, rare) | a wingman | High | "It moved before I did. How?" | "It anticipated that. Noting it." | "Huh. That one saw it coming." |
| `Bark_TargetShielded` (new, T2) | Player fires on a shielded gunship with no effect | Kato or a wingman | Normal | "It's shielded, hit it harder!" | "Shielded. Break it first." | "That one's armoured up." |
| `Bark_NewEnemy_<Type>` (new, T4): `Interceptor`, `Gunship`, `Bomber`, `Carrier` | First time each type appears | Kato (he identifies) | High | - | "New type. Fast, carries missiles." / "Heavy contact. Shielded." / "That one is laying mines." / "Carrier. It will keep launching." | - |
| `Bark_MinesAhead` (new, T4) | Player flies toward a mine cluster | a wingman | High | "Mines! Pull up!" | "Mines ahead. Avoid." | "Careful, mines." |
| `Bark_CarrierLaunch` (new, T4) | A carrier launches fighters | a wingman | Normal | "More fighters coming out!" | "Carrier launching." | "Here come more." |
| `Bark_AmmoLow` / `Bark_AmmoOut` (new, T2) | Missile ammo at 25% / empty | flown pilot (Moss mostly) | Normal / High | "Running light on missiles." / "I'm dry!" | "Ammunition low." / "Out. Guns only." | "Not many left." / "That's the last of them." |
| `Bark_WeaponHot` (new, T2) | Weapon overheats | flown pilot | Normal | "Guns are cooking!" | "Overheat. Cooling." | "Too hot. Give it a second." |
| `Bark_BoostEmpty` (new, T1) | Boost runs out while held | flown pilot | Flavour | "Out of burn!" | "Boost depleted." | "That's all she's got." |
| `Bark_Boundary` (new, T1) | Leaving the flight area | Command, or a wingman | Critical | "Wide of the zone. Bring it round." | "You're drifting out. Come about." | "Nothing out there for us. Swing back." · Command: "Triad, you're leaving the area. Turn back." |
| `Bark_WaveCleared` (exists) | A wave ends | a pilot, rotating | High | "That's all of them. For now." | "Area clear. No contacts." | "All quiet. Nice work, everyone." |
| `Bark_WaveIncoming` (new, T4) | Next wave warps in | Command / Kato | High | "Here they come again." | "New contacts. Many." | "Round two. Stay close." · Command: "Triad, more contacts inbound." |
| `Bark_Breather` (new, T3) | 4 s into the gap between waves | a wingman | Flavour | "Anyone else enjoying this?" | "They are not behaving like raiders." | "Check your gauges while it's calm." |

## 4. Switching, powers and abilities
| Id | When | Speaker | Priority | Wren | Kato | Moss |
|---|---|---|---|---|---|---|
| `Bark_SquadSwitchIn` (exists) | Player switches to a ship | the pilot switched to | Normal | "Rift's yours." / "My turn." | "Lancer, ready." / "You have Lancer." | "Bastion's yours." / "I'm with you." |
| `Bark_SquadSwitchStrike` (exists) | Switch Strike window opens | pilot switched to | High | "Strike's hot!" | "Strike window. Now." | "Hit them now!" |
| `Bark_SquadFocusAck` (exists) | Wingmen ordered onto a target | a wingman | Normal | "On it!" | "Focusing fire." | "Got your target." |
| `Bark_PowerReady` (new, T2) | The flown ship's power finishes cooling, 1 in 2 | flown pilot | Flavour | "Portals are up." | "Ram's ready." | "Missiles loaded." |
| `Bark_PowerNotReady` (new, T2) | Player presses a power on cooldown | flown pilot | Normal | "Not yet!" | "Still charging." | "Give it a moment." |
| `Bark_Portal_Open` / `_Through` / `_MissileReturn` (new, T2) | Rift: portals placed / flown through / a missile sent back | Wren | Normal / Flavour / High | "Door's open." / "And... through!" / "Return to sender!" | - | - |
| `Bark_Ram_Go` / `_Kill` (new, T2) | Lancer: ram starts / kills | Kato | Normal / High | - | "Ramming speed." / "Through and through." | - |
| `Bark_Swarm_Fire` / `Bark_Dome_Up` / `_Saved` (new, T2) | Bastion: swarm launched / dome raised / dome blocks a hit on a wingman | Moss | Normal | - | - | "Swarm away!" / "Get under the dome." / "Told you I had you." |
| `Bark_AbilityUnlocked_<Ability>` (new, T5 / T3) | Cloak, EMP Pulse or Gravity Well earned | that ship's pilot | High | "Cloak's online. Now you don't see me." | "EMP pulse installed. This will stop them cold." | "Gravity well. Oh, I'm going to enjoy this." |
| `Bark_Cloak_On` / `_Ambush` (new, T2) | Rift cloaks / attacks from cloak | Wren | Normal / High | "Going dark." / "Surprise." | - | - |
| `Bark_EMP_Fire` / `_Hit` (new, T2) | Lancer EMP used / 3+ enemies stunned | Kato | Normal / High | - | "Pulse out." / "They're dead in space. Go." | - |
| `Bark_Gravity_Fire` / `_Caught` (new, T2) | Bastion's well placed / 3+ enemies caught | Moss | Normal / High | - | - | "Well's open." / "All bunched up. Help yourselves." |
| `Bark_AbilityLocked` (new, T2) | Player presses a locked ability | flown pilot | Normal | "Haven't got that yet." | "Not installed." | "We don't have that one." |

## 5. Travel, planets and landing
| Id | When | Speaker | Priority | Wren | Kato | Moss |
|---|---|---|---|---|---|---|
| `Bark_Hyper_Spool` / `_Blocked` (new, T1) | Hyperdrive charging / refused in combat | flown pilot | Normal | "Spooling up!" / "Can't jump, too hot here!" | "Drive charging." / "Jump blocked. Hostiles in range." | "Here we go." / "Not while they're on us." |
| `Bark_FieldBlocked` (new, T4) | Player tries to approach the Dominion before the passage is open | Kato | High | - | "That field will tear us apart. No approach." | - |
| `Bark_Gate_Enter` (new, T4) | Through a portal gate, 1 in 3 | flown pilot | Flavour | "Shortcut!" | "Transit complete." | "Never get used to that." |
| `Bark_Atmo_Entry` (new, T1) | Atmospheric entry begins | flown pilot | Normal | "Going in hot!" | "Entering atmosphere. Slowing." | "Hold on, it gets bumpy." |
| `Bark_Land_Ready` / `_Down` (new, T1 / T5) | Landing zone in range / touchdown | flown pilot | Normal | "There's the pad." / "Down. Let's look around." | "Landing zone marked." / "Touchdown. Scanning." | "I see the pad." / "We're down. Stay sharp." |
| `Bark_TakeOff` (new, T5) | Back in the ship on a planet | flown pilot | Normal | "Lifting off." | "Leaving the surface." | "Back to the sky." |
| `Bark_Planet_Arrive_Verdance` / `_Cinder` (new, T5) | First view on foot | a pilot | Flavour | "It glows. Everything glows." / "Hot enough for you?" | "Bioluminescence. Extensive." / "Volcanic. Unstable ground." | "Would you look at that." / "Mind the lava, please." |

## 6. On foot (the pilot with the blade; the other two on the radio from their ships)
| Id | When | Speaker | Priority | Wren | Kato | Moss |
|---|---|---|---|---|---|---|
| `Bark_Foot_Contact` (new, T5) | Enemies first seen in an encounter | radio (a wingman) | High | "You've got company!" | "Movement ahead of you." | "Heads up, you're not alone." |
| `Bark_Foot_<Type>` (new, T5): `Trooper`, `Heavy`, `Drone`, `Warden` | First time each type appears | Kato on the radio | High | - | "Riflemen. Their shots can be turned back." / "Shielded. Break its guard." / "Drones. Fast, fragile." / "That one is in charge. Careful." | - |
| `Bark_Foot_Deflect` / `_Perfect` (new, T5) | A bolt returned / a perfect deflect kill | player's pilot (their voice on foot is the flown ship's) | Normal / High | "Back at you." / "Right between the eyes!" | "Returned." / "Precisely." | "Yours, I believe." / "Now that was pretty." |
| `Bark_Foot_GuardBroken` (new, T5) | The player's guard breaks | player's pilot | Critical | "Guard's gone!" | "Guard broken." | "Lost my guard!" |
| `Bark_Foot_Hurt` / `_LowHealth` (new, T5) | Hit hard / health under 30% | player's pilot / a wingman | Normal / Critical | "Ow!" / "I'm hurt, I'm hurt." | "I'm hit." / "Vitals low." | "Felt that." / "Need a breather." · wingman: "Get to cover!" |
| `Bark_Foot_Heal` (new, T5) | A checkpoint or pickup restores health | player's pilot | Flavour | "Better." | "Stabilised." | "That's the stuff." |
| `Bark_Foot_Kill` / `_LastOne` (new, T5) | An enemy killed with the blade, 1 in 3 / the last of an encounter | player's pilot | Flavour / High | "Next!" / "That's the lot." | "One down." / "Area secure." | "Sorry about that." / "All clear here." |
| `Bark_Foot_Dash` / `_Slam` (new, T5) | Dash through fire / air slam hits 2+ | player's pilot | Flavour | "Whoo!" / "Boom." | "Clear." / "Effective." | "Coming through." / "And down." |
| `Bark_Foot_ObjectiveNear` / `_Done` (new, T5) | Within 30 m of the objective / objective complete | radio / player's pilot | Normal / High | "That's it, right ahead." / "Got it. Heading back." | "Signal source, thirty metres." / "Objective secured." | "Almost there." / "Done. Come on home." |
| `Bark_Foot_DoorLocked` / `_DoorOpen` (new, T5) | Locked door tried / a door opens | player's pilot | Normal | "Locked. Figures." / "We're in." | "Sealed. Find another way." / "Access granted." | "Not that way." / "After you." |
| `Bark_Foot_Hazard` (new, T5) | Entering lava or a hazard volume | radio | Critical | "Get out of there!" | "Hazard. Move." | "Not there, not there." |
| `Bark_WardenPhase2` (exists) | The Warden changes phase | Armada (sound) + radio | High | "It's angry now!" | "It changed its pattern." | "Second wind. Watch it." |
| `Bark_Foot_Idle` (new, T5) | 25 s with no input and no enemies | radio | Flavour | "You sightseeing down there?" | "Awaiting your move." | "Take your time. We've got the sky." |

## 7. The Dominion (assault, boarding, escape): gameplay lines only; the story lines are transmissions
| Id | When | Speaker | Priority | Wren | Kato | Moss |
|---|---|---|---|---|---|---|
| `Bark_Dom_GeneratorHit` / `_GeneratorDown` (new, T4) | A field generator at half health / destroyed | a pilot | Normal / High | "It's cracking!" / "Generator's gone!" | "Generator failing." / "One down. {n} remain." | "Keep at it." / "That's another." |
| `Bark_Dom_Turrets` (new, T4) | Turret fire first hits the flown ship | a wingman | High | "Turrets, watch the turrets!" | "Defensive fire. Stay low." | "They've got guns everywhere." |
| `Bark_Dom_FieldDown` (new, T4) | All four generators destroyed | Kato | Critical | - | "The shield is down. The bay is open." | - |
| `Bark_Dom_Tractor` (new, T4) | The tractor assist takes hold | flown pilot | High | "Something's pulling us in." | "We are being guided. Controls limited." | "Easy. Let it take us." |
| `Bark_Dom_Boarded` (new, T5) | First step in the hangar | radio | High | "Bring something back." | "We will hold outside." | "Careful in there." |
| `Bark_Dom_CoreShield` / `_CoreOpen` (new, T5) | A conduit breaks / the heart is exposed | player's pilot | High | "Shield's dropping!" / "There's the heart!" | "Conduit down." / "Core exposed." | "One more like that." / "There it is." |
| `Bark_Dom_Escape_Start` / `_30` / `_10` (new, T5) | Escape run begins / 30 s / 10 s left | radio | Critical | "Run! Run now!" / "Thirty seconds!" / "Move it!" | "It's coming apart. Ninety seconds." / "Thirty." / "Ten seconds." | "Get out of there!" / "Thirty seconds. Keep going." / "Almost, almost!" |
| `Bark_Dom_EscapeFlight` (new, T4) | Launch from the hangar | a wingman | Critical | "Punch it!" | "Follow my line out." | "Stay with me, stay with me." |
| `Bark_Dom_Clear` (new, T4) | Clear of the blast | a pilot | High | "We're clear! We're clear!" | "Clear of the blast radius." | "Everyone here? Good." |

## 8. Command (mission control; sparse, never during a fight unless Critical)
| Id | When | Line |
|---|---|---|
| `Bark_Cmd_ObjectiveNew` (new, T5 / T3) | A new objective is set | "Triad, new tasking. Marker's on your scope." |
| `Bark_Cmd_ObjectiveDone` (new) | An objective completes in space | "Copy, Triad. Good work." |
| `Bark_Cmd_Retreat` (new, T4; tutorial battle) | The withdrawal is ordered | "Triad, disengage. Pull back now." |
| `Bark_Cmd_Retake` (new, T3) | First line of a retake after "CUT!", 1 in 2 | "Triad, reset. Go again." |
| `Bark_Cmd_Welcome` (new, T5) | Continue from a save | "Welcome back, Triad. Picking up where you left off." |

## 9. Banter (Flavour only; one exchange per calm stretch, never twice in a run)
Two or three lines, the second speaker answers the first. T3 plays them from the breather hook.
- Wren: "Bet you ten I get more than you this round." / Kato: "I do not bet on things I can measure." / Moss: "I'll take that bet."
- Moss: "Everybody drinking water?" / Wren: "We're in space, Moss." / Moss: "That's no excuse."
- Kato: "They search in a grid. Raiders do not use a grid." / Wren: "So they're tidy raiders." / Kato: "That is not reassuring."
- Wren: "Kato, say something nice about my flying." / Kato: "You are still alive." / Wren: "I'll take it."
- Moss: "When this is done, I'm cooking." / Wren: "Please not the stew." / Kato: "I liked the stew."
- Kato: "Their formations adapt to ours. Quickly." / Moss: "Then we keep changing the dance."

## 10. For the agents who wire this
- **T3** owns the bark system (`Squad/TDSquadBarks.*`): load these lines from a data table generated from this file
  (`Tools/` script: markdown tables → `DT_BarksFallback` under `Gameplay/`, Nicholas's `DT_Barks` wins row by row), add
  the per-id line rotation, the `Flavour` priority and the banter queue, and show the speaker in their colour.
- Each owner named in a row adds its trigger (`RequestBark(id, speaker)`) with a test that the line is requested once
  and not spammed (a held condition fires once until it clears).
- `{dir}`, `{n}` and similar are filled by code. Keep lines short enough for the subtitle line of the HUD (about 48
  characters).
- When a line here and the story disagree, the story wins: tell the Lead.
