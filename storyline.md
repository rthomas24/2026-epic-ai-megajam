# Triad Drift: The Paradox (storyline)

**Adopted by Ryan on Nov 13, 2026.** The story is **Nicholas's** ("Triad Drift: The Paradox, narrative concept
brief"); this file is that brief, plus how each beat is played in the game we have. **Every feature, objective name,
line of dialogue, set piece and asset now serves this story.** `features.md` says what to build; this file says what it
means. The short lines said during play are in [`dialogs.md`](dialogs.md). Contains ending spoilers: it is for the team, not for the store page.

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

**Descent (ENG-38):** every landing is a flight from orbit through the atmosphere into the world, with no cut. Use it for the story: on Verdance the
machines' landers are visible burning in ahead of you; on Cinder the dig's glow shows from orbit; the Dominion hangs in the sky from the ground.

## 6. Clues throughout play (reward attention, never give the twist away early)
| Clue | Where | Owner |
|---|---|---|
| The Dominion always visible or referenced as the looming, unexplained destination | Galaxy sky from wave 1, intro, planet skies, objective line | T4, T3; Lead's environment builder (sky, lighting) |
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

## 11. The opening and its payoff: *First Contact* (Nicholas's cinematic brief, received Nov 13)
Full text: [`docs/story/First-Contact-Cinematic-Brief.md`](docs/story/First-Contact-Cinematic-Brief.md). It replaces
the opening row of §5 and sharpens acts 1, 4 and 5.

**Opening cinematic, about 2 min 20 s** (today's intro is 49 s and will be rebuilt to this):
| Time | Beat | Lines (drafts) |
|---|---|---|
| 0:00-0:25 | **Missing patrol.** Black; a patrol distress call through static; a quiet system and the distant silhouette of something impossibly large; the score is held back | Patrol Seven |
| 0:25-0:55 | **Triad arrives.** Rift, Lancer and Bastion drop out of a jump and form up; short exchanges establish the three pilots | Command ("Recon only. Do not engage."), Wren, Kato, Moss |
| 0:55-1:30 | **The impossible vessel.** The approach; the surface seems dormant; instruments glitch; Kato catches an encrypted signal like a military authentication handshake, gone before he can resolve it | Wren, Kato, Moss |
| 1:30-2:00 | **The first launch.** Hangars open across the Dominion; drones stream toward the planets, mostly ignoring Triad; one drone scans Rift at close range, its targeting light goes **red to blue**, and it holds fire; a separate group breaks formation and attacks | Wren, Kato, Command ("disengage") |
| 2:00-2:20 | **The interrupted warning.** A heavily distorted voice: **"Don't-"**, cut off. Wren calls the break; the camera settles into the player's ship view and the HUD comes on | Unknown voice, Wren, Command |

**Tutorial battle, straight after the cut:** evasion (the first volley) → weapons (a small, readable group of drones) →
squad play (switching, then each ship's ability) → escalation (a larger craft arrives) → **withdrawal**: Command orders a
retreat and the drones stop pursuing at a boundary, as if guarding a task. **Hand-off:** a distress report from Verdance:
machines have landed around a remote research facility and are probing beneath the surface.

**Foreshadowing that only registers in hindsight:** the handshake (the future Dominion recognises Triad's
authorisation); the drone that goes blue and holds fire (it identifies Rift as friendly); the cut-off "Don't-" (future
Wren reaching across temporal interference); the Verdance ambush (they remember where their younger selves went); Cinder
navigation data that matches Triad's later missions; Dominion doors that recognise the squad.

**Finale payoff (inside the Dominion):** a recovered record leads to a short scene on the future command deck showing
the opening transmission from the other side: "All three signatures confirmed. It's us." ... "Don't-". **The identical
processed audio of "Don't-" is reused, never re-recorded.** The full warning ("Don't engage the Dominion. You don't
understand what happens if you do.") is learned from the record, not by changing what the squad heard.

**Rules from the brief:** future Triad is not a villain; the squad must still have a compelling reason to board and to
destroy the reactor; keep the opening lean (in-engine ship cameras, radio performances, a launch, a scan and hesitation,
one camera transition into play); no clear time-travel words in the opening.

**How and when we build it (Ryan, Nov 13): the intro is redone ONCE, after the models and graphics it shows are in**, so
it is not re-shot after every import. Gate: the Rodin player ships, the Null Armada fleet (incl. a space drone), the
Dominion exterior with hangars that open, the round-4 effects (engines, tracers, explosions, the distortion field) and
the new HUD are on `main`. Until then T3 prepares only what does not depend on art: the shot plan with timings, the
dialogue as transmission entries (ids `O_PatrolSeven`, `O_Command_Orders`, `O_Wren_HowBad`, ... `O_Unknown_Dont`), the
red-to-blue drone beat as a gameplay hook, and the hand-over into the tutorial battle. Owner: T3 (director), with T4
(the launch, the drone that holds fire, the withdrawal boundary), T1 (the jump-in), the effects builder, and
Nicholas for every line, voice and the score.

**Questions for Nicholas (asked in issue #29):**
1. The brief has each ship's ability in the tutorial as Cloak, EMP Pulse and Gravity Well. In the game those are
   **earned on the planets** (acts 2 and 3); the tutorial teaches each ship's signature power (Rift's portal blaster,
   Lancer's afterburner ram, Bastion's missile swarm and shield dome). Keep that progression?
2. Today's waves 1-2 become the tutorial battle: wave 1 = evasion, weapons, switching; wave 2 = the larger craft and the
   withdrawal. Is that the split you want, and does the tutorial music (#24) end at the withdrawal?
3. The intro grows from 49 s to about 2:20: confirm the length (a skip stays available from the first second).

## 12. Sequence 02: *Aftermath* (Nicholas's brief, received Nov 13)
Original documents from Nicholas are in [`Story/`](Story/) (the three `.docx` files and his README); readable copies
and our build notes are here and in `docs/story/`. This sequence answers how the tutorial battle ends.

**When:** immediately after the player wins the opening tutorial battle and escapes the Dominion's defence perimeter.
**Length:** 75-100 seconds, in-engine: existing ships, cockpit / HUD close-ups, radio dialogue, a distant Dominion. No
faces. Skippable after a first viewing.
**Its job:** change the assignment from "identify the vessel" to "follow its forces to Verdance", show that the enemy is
running a purposeful search, and plant one quiet clue.

| Time | On screen | Beat |
|---|---|---|
| 0:00-0:20 | Triad regroups beyond a faint boundary of interference; the last drones peel away; alarms settle; the HUD returns to normal | They survived; the mission has changed. Kato: "They're not pursuing us." |
| 0:20-0:45 | A tactical display: hostile contacts leaving the Dominion in disciplined lanes toward Verdance; some never targeted Triad. **A faint pulse; the player's navigation display briefly marks the corridor the drones use, then clears. Nobody comments** | The attackers follow an objective. The clue: the Dominion's systems can anticipate Triad |
| 0:45-1:10 | Mission Control reconnects through static: a research outpost on Verdance has gone dark near a restricted survey zone | New orders: avoid the mothership, reach Verdance, find the outpost, recover its survey data |
| 1:10-1:30 | Triad plots a course; the camera lingers on the Dominion; **a row of its exterior lights changes in the same rhythm as the pulse on the HUD**; match cut from the jump to Verdance | The connection is visible but unexplained |

**Rules from the brief:** the Dominion never speaks plainly here; the fleet defends its route then resumes its
operation; **the "Don't-" fragment is NOT repeated here** (it is kept for the finale); Command does not know what the
Dominion is; the outpost's name, the planetary resource and the future catastrophe are undecided and must not be
presented as lore.

**What it means for the game:** the first playable stretch is ONE tutorial battle (today's wave 1, shaped as the
*First Contact* tutorial: evasion, weapons, switching, a larger craft, the withdrawal where drones stop at a boundary),
then this sequence, then Verdance. Today's wave 2 becomes part of that battle or the approach to Verdance (T4 and T3
propose; Nicholas confirms in issue #29). New needs: a simple animated tactical display of drone lanes (T2, in the new
HUD's style), the navigation pulse on the HUD (T2), the Dominion's light row pulsing in the same rhythm (T4 / effects
builder), the lines as transmissions `S2_*` (T3). **Built once, with the opening, after the art is in** (same gate as
§11); T3 adds it to the shot plan now.

## 13. Gameplay Sequence 01: the tutorial battle (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Gameplay_Sequence_01_Tutorial_Battle.docx`. It sits between the opening cinematic (§11)
and *Aftermath* (§12): **one uninterrupted first mission, about 6-10 minutes**, no cinematic in between. The player
takes control as a drone group turns hostile: Triad was ordered to investigate, not fight, so it begins as an
emergency escape and becomes a short combat lesson.

| Phase | Player learns | Ends when | Call (draft) / HUD prompt |
|---|---|---|---|
| 01 Break formation | Steer, pitch / yaw, throttle or boost; evade a readable first volley | A safe corridor is reached / the opening attack is survived | COMMAND: "Pilot, incoming fire! Evade now!" / EVASIVE MANEUVERS |
| 02 Return fire | Acquire a marked drone, fire, read the target indicator and damage feedback | The required drones are destroyed | KATO: "Keep moving. I'm marking a hostile." / TARGET AND FIRE |
| 03 Fight as Triad | Switching ships; each ship's ability in a staged situation | Each available ability is shown, or its substitute | MOSS: "We've still got three ships. Use them." / SQUAD SYSTEMS |
| 04 Heavy contact | Everything under pressure; the squad as a team | The larger craft is driven off or disabled | WREN: "That one's bigger. Focus it together!" / DISABLE HEAVY CONTACT |
| 05 Withdraw | Flying to a marked escape vector under pursuit | The escape boundary is crossed; **the drones deliberately stop at the perimeter** | COMMAND: "Triad, disengage. Fall back to the marked corridor." / BREAK CONTACT, then KATO: "They're breaking off." → *Aftermath* |

**Rules from the proposal:** entry straight from the cinematic with no fade, reload or training room; most Dominion
craft keep heading for the planets and only a small defensive group attacks (never the whole fleet chasing the squad);
the drone that hesitated in the opening is not an immediate target; nobody labels the enemy; one action taught at a
time with large markers and hints for the real input device; checkpoints at the opening of the engagement, the heavy
contact and the retreat, with the cinematic never replayed after a death; no hard lock on an ability prompt (a timed
hint, a substitute objective or a reset); the "Don't-" audio is not repeated.

**How it maps to the game today (engineering's answer, posted in issue #29):**
- Flight, targeting, switching (`1` / `2` / `3` or the pad's D-pad), the three ships with AI wingmen (solo; no co-op),
  the onboarding hints (fly, fire, switch, power, one at a time, ticked off, a second chance on a miss) and scene
  checkpoints with "CUT!" retakes all exist. Waves 1-2 hold the content: wave 1's fighters are phases 01-02, wave 2's
  gunship is the heavy contact.
- **New work:** phase 01 as an evade-first beat; staged situations for each ship's power (phase 03); the withdrawal:
  a marked corridor, pursuers that stop at a boundary, the trigger for *Aftermath*; three checkpoints inside the one
  mission; most of the fleet streaming past toward Verdance in the background. Owners: T4 (encounter), T3 (hints,
  director, checkpoints as scenes), T2 (prompts in the HUD), T1 (escape vector).
- **Open with Nicholas:** phase 03 names Cloak, EMP Pulse and Gravity Well, which the game awards on the planets;
  today the tutorial would teach the signature powers (portal blaster, afterburner ram, missile swarm and dome).
- The Verdance transition is the hyperdrive jump, then the planetary descent (ENG-38).

