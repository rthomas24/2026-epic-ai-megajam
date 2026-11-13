# Triad Drift: The Paradox (storyline)

**Adopted by Ryan on Nov 13, 2026.** The story is **Nicholas's** ("Triad Drift: The Paradox, narrative concept
brief"); this file is that brief, plus how each beat is played in the game we have. **Every feature, objective name,
line of dialogue, set piece and asset now serves this story.** `features.md` says what to build; this file says what it
means. Contains ending spoilers: it is for the team, not for the store page.

Who decides what: **Nicholas owns the story** (wording, tone, what is revealed when, every line and voice). **Ryan's
side owns how it is played** (which gameplay moment carries each beat, the hooks, the timing). A change to the story
goes through Nicholas; a change to how a beat is delivered goes through Ryan's Lead. Ask with a GitHub issue or a board
message whose title starts with **`Story:`**.

## 1. Logline
When an enormous, silent mothership appears in a populated star system, an elite three-pilot reconnaissance unit is
sent to discover why its machines are combing the planets. To reach the vessel, the pilots must follow the invaders'
trail across two worlds, only to learn that the enemy is their own future, desperate to prevent a catastrophe.

## 2. Premise
A colossal unidentified ship appears without warning. Authorities designate it **the Dominion**. It ignores every
attempt at communication while smaller fighters, drones and ground units emerge from it, seize facilities and perform
precise searches on nearby planets. It looks like an invasion, but the targets are oddly selective: buried structures,
scientific records, unusual energy signatures.

A military unit known as **Triad** (Wren, Kato and Moss) is deployed to identify the vessel, find out what its forces
are seeking, and report back. A direct approach is impossible: the Dominion is protected by a powerful **distortion
field**. The squad follows the enemy's activity from world to world, securing the information and technology needed to
reach the ship.

## 3. The pilots
| Pilot | Ship | Ability earned | Character |
|---|---|---|---|
| **Wren** | Rift | Cloak | Bold and instinctive; the pilot most willing to take a dangerous opening |
| **Kato** | Lancer | EMP Pulse | Analytical and cautious; wants to understand the threat before committing |
| **Moss** | Bastion | Gravity Well | Steady and good-humoured; holds the team together when the mission falls apart |

Their different instincts create friction; the campaign depends on their complementary abilities and the trust they
build. (This replaces the earlier working personalities in `features.md`.) The player is the wing lead and can take any
cockpit: switching ships is switching whose instincts lead.

## 4. The hidden truth (for the team, never stated to the player before the Dominion)
The Dominion is a **time-displaced vessel built by survivors of a ruined future**. Its commanders are **older versions
of Wren, Kato and Moss**; its drones and fighters are extensions of the desperate operation they lead. They have come
back to recover the **Origin Core**, an ancient temporal mechanism whose power may allow one pivotal event to be
changed. Their mission is not conquest: they believe it is humanity's last chance to avert extinction.

The future Triad remembers a catastrophe connected to the original mission against the Dominion. Their interference
may be the very thing that brings it about. The exact mechanics stay uncertain until the final confrontation, so the
paradox is unsettling, not over-explained.

What the player is told the enemy is: the authorities' designation, the **Null Armada** ("null": no signal, no answer,
no known origin). Its ground units are automated: armour with nobody inside that we ever see.

## 5. The campaign, beat by beat, and how the game plays it

| Act | Story (Nicholas's outline) | What the player does (features.md) | How the beat is delivered | Changes needed |
|---|---|---|---|---|
| **Opening** | The Dominion appears; Triad is deployed to identify it and report back | Main menu, intro (ENG-33), 49 s | Intro narration and shots: the silent ship, machines descending on worlds, the three pilots | Intro text and narration to the premise (T3 hooks, Nicholas's words); the Dominion shown from the first shot |
| **1. First Contact: The Watcher** | Triad engages craft emerging from the Dominion; the ship cannot be approached; its units are searching, not occupying; the trail leads to Verdance | Galaxy, waves 1-2 (ENG-13, 14): learn flight, fire, switching, powers | The Dominion is **visible in the distance from the start** behind a shimmering distortion field; a hail gets no answer; barks note the enemy scanning, not holding ground; the objective turns from "identify" to "find a way through the field" | Dominion present and unreachable from wave 1 (T4: today it only appears after Cinder); field effect (effects builder); objective lines and barks (T5 / T3 hooks, Nicholas's lines) |
| **2. Verdance: The Voices** | At an abandoned research site Triad fights ground forces and retrieves a **signal-analysis device**; its recordings hold fragmented human voices under the enemy's communications; the data points to Cinder | Verdance landing and two on-foot objectives (ENG-25): today "Recover the Wreck Core" (unlocks Gravity Well) and "Wake the Ancient Relay" (unlocks Cloak) | Objective 1 becomes recovering the **signal-analysis device** from the wrecked research site; objective 2 wakes the ancient relay to play its recordings: static, then human voices and technical phrases that should not be there. Rewards unchanged | Rename objectives, markers and prompts (T5); the "voices" audio moment at the relay (T5 hook, Nicholas's audio); research-site dressing on the wreck set (builders / Grok Bot) |
| **3. Cinder: The Pattern** | At a volcanic mining complex the squad interrupts the machines' excavation, takes the **crystals** that stabilise a passage through the field, and finds the same ancient technology as on Verdance; enemy transmissions begin to echo the pilots' own voices; the enemy is after the parts or keys of the **Origin Core** | Galaxy waves 3-4, then Cinder (ENG-26): today "Clear the Armada Outpost" (unlocks EMP Pulse) and "Destroy the Signal Jammer" (reveals the Dominion) | The outpost is the **mining complex**; clearing it stops the excavation and yields the stabilising crystals; the jammer is the **excavation rig's field anchor**: destroying it gives the passage through the distortion field. Transmissions now use the squad's own call signs and phrases. The Warden guards the dig | Rename objectives; "the passage is open" replaces "the Dominion is revealed" (T5, T4); echo-voice transmissions (hooks T5 / T3, Nicholas's audio); matching ancient-technology prop on both planets (builders / Grok Bot) |
| **4. The Dominion: The Truth** | With the signal data and the crystals, Triad breaks through the field and **boards** the mothership: familiar military architecture, systems keyed to their identities, recordings of their older selves commanding the fleet. The older pilots argue the Origin Core can prevent a catastrophe; the present squad fears it will destroy their system | Wave 5, the Dominion assault (ENG-18): four field generators down, then **docking and the interior on foot** (ENG-29, ENG-30): hangar, corridors, the Warden, the core chamber | The four shield generators **are** the distortion field. Inside: doors and consoles that open to the pilots' IDs, their own unit markings under the armour, and a recording in each room, older and older-sounding versions of Wren, Kato and Moss. The argument plays over the approach to the core | **Boarding moves from stretch to wanted** (Ryan, Nov 13): T4 docking, T5 interior and core fight; interactable consoles with a recording per room (T5 hook, Nicholas's lines and voices); interior kit with familiar architecture (builders / Grok Bot) |
| **5. Reactor and Escape: The Choice** | Unable to trust the future commanders or stop the operation safely, Triad destroys the central reactor; a 90-second escape as the temporal machinery destabilises; they survive. Then their scanners receive a **distress signal bearing their own identifiers, sent from decades in the future**. Did they end the threat, or create the future that sent it back? | Core fight (ENG-30), escape run and escape flight (ENG-31), breakup, victory | The core fight is the choice. The escape run and flight play with time visibly failing (echoes, repeats). On the victory screen the distress signal arrives: three call signs, a date decades ahead, cut to black | Escape run and flight (T5, T4); the stinger on the victory screen (T3 / T5 hook, Nicholas's text and audio) |

**Fallback if boarding is not solid by the freeze:** the space-only ending (reactor destroyed from outside,
`td.Finale.Boarding 0`). The reveal then moves into the assault as transmissions that break through as each generator
falls, the reactor shield opening to the squad's IDs, and the same stinger. Nothing else in the story changes.

## 6. Clues throughout play (reward attention, never give the twist away early)
| Clue | Where | Owner |
|---|---|---|
| The Dominion always visible or referenced as the looming, unexplained destination | Galaxy sky from wave 1, intro, planet skies, objective line | T4, T3; Nicholas (sky, lighting) |
| Enemy tactics that anticipate Triad's manoeuvres | Wave 3 onward: enemies that pre-empt a ship switch or dodge once in a while, with a bark noticing it | T4 (AI), Nicholas (line) |
| Familiar markings under damaged armour | Destroyed enemy ships and troopers show Triad's unit insignia shape under the black plating | Grok Bot / builders (damage variants), effects builder |
| Transmission fragments using the squad's own terminology | Verdance relay, Cinder, the assault | T5 / T3 hooks; Nicholas (audio) |
| Doors and consoles that recognise the pilots | Cinder dig (one door), every door in the Dominion interior | T5 |
| The same ancient technology on both planets | One distinctive artefact prop on Verdance and Cinder, and again in the core chamber | Grok Bot / builders |
| The enemy's ships and the squad's share a design lineage | Null Armada silhouettes as harsher, older descendants of Rift, Lancer and Bastion | Grok Bot (already close: crescent fighter vs Rift's crescent wings) |

## 7. How it fits the Director's Cut theme
Scenes and takes: a death is "CUT!" and the scene is taken again. In a story about a future trying to redo one
pivotal event, that is the same idea in the player's hands. The slate, the retake counter and the money shots should
lean into it; from Cinder onward a take number may occasionally glitch or echo.

## 8. Names and words to use
- **Triad**: the unit. **Wren / Rift, Kato / Lancer, Moss / Bastion.**
- **The Dominion**: the mothership (the authorities' designation). **Null Armada**: its forces, as the squad knows them.
- **The distortion field**: what keeps the Dominion unreachable (in play: its four field generators).
- **Signal-analysis device**, **the relay**, **stabilising crystals**, **the mining complex**, **the Origin Core**.
- Never say "aliens" as fact. Before act 4 nobody on the squad says "future", "time" or "us".

## 9. Open story decisions (Nicholas's to make; none blocks the jam build)
Who built the Origin Core; precisely what it changes; what disaster befell the future; whether the catastrophe was
inevitable or caused by Triad's intervention; how aware and how responsible the future pilots are. The jam version needs
only the essential reveal: **the apparent alien enemy is the squad's own future.**

## 10. Rules for every agent
1. Before naming an objective, writing a line, placing a prop or briefing an asset, read §5 for that act. If it does
   not fit the story, change it or ask.
2. Story text and voice are Nicholas's: gameplay code exposes the hook (an objective line, a bark request, a console
   interaction, a transmission event with an id) and ships a plain placeholder line; his text and audio replace it
   through the presentation assets.
3. Nothing may reveal §4 before act 4. Clues are small and deniable.
4. Keep this file current: when a beat is built, add the commit and where it plays to §5.
