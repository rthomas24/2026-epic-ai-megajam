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

## 14. Gameplay Sequence 02: *Verdance Arrival* (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Gameplay_Sequence_02_Verdance_Arrival.docx`. Status in the document: "proposed, not
verified implementation". It sits between *Aftermath* (§12) and Cinematic 03 (§15): it starts as Triad leaves the
Dominion perimeter for Verdance and ends when the team reaches the outpost's **geological survey terminal**.
**Suggested length: 10-15 minutes, "subject to playtesting".**
**Player goal:** reach Verdance, find the missing research outpost, recover evidence of why Dominion machines have
landed nearby. "Reconnaissance under pressure": not yet a mission to destroy the enemy or obtain the Origin Core.

| Phase | Player does | Ends when | Story beat / draft line |
|---|---|---|---|
| 1 Orbital approach | Flies the Verdance approach corridor and scans hostile traffic; most drones ignore Triad and ferry equipment to the surface | The insertion sector is reached and **one convoy is scanned** | Orbital clue: the craft carry scanner assemblies and extraction equipment, not troop columns; Kato notes a pattern but cannot infer the target. WREN: "That's a lot of machines for a geology expedition." |
| 2 Secure landing | Evades or defeats a small interception force while protecting a safe landing path | The landing zone is clear and the squad reaches the drop point (**checkpoint**) | COMMAND: "The outpost went dark twelve hours ago. Find its survey archive. Do not destroy the facility." (the document does not place this line in a phase; put here by us) |
| 3 Jungle traverse | Moves through bioluminescent terrain using cover and navigation markers; meets survey beacons and limited Dominion ground patrols | The outpost perimeter is found and the unusual digging is identified (**checkpoint** before the perimeter) | Ground clue: instruments placed over a narrow subsurface corridor, measuring, not stripping resources. KATO: "They're concentrating their scans along one underground line. That's not random." Quiet unease: a sensor marker briefly labels a squad transponder **"verified"**, then resets to **"unknown"**; nobody explains it |
| 4 Reach outpost | Disables or bypasses the perimeter defences; finds lights and equipment on and nobody answering | The facility is entered and the geological-data console is activated → Cinematic 03 | Outpost arrival: the station looks maintained, no obvious battlefield; why the researchers are gone stays unresolved. MOSS: "Lights are on. Doors are open. Nobody's answering. I don't like this." |

Objective text on screen throughout: **"Investigate the silent research outpost."**

**Rules from the proposal:** a modest orbital encounter and a compact ground route if production is constrained; a
seamless flight-to-ground transition is optional (a short skippable insertion beat is acceptable); do not require ship
switching, co-op, advanced stealth or unique traversal abilities unless already confirmed working; solo play must not
depend on three humans; the "verified" marker must not name future Triad; Cinematic 03 triggers when all squad members
are at the console, or when the local player interacts in solo mode; the positions of the terminal and of the dormant
drone are kept for the return to gameplay.

**How it maps to the game today:**
- **Exists:** the hyperdrive jump to Verdance orbit and the gate (ENG-15, 16); the descent and landing (ENG-20, with
  ENG-38 replacing the arrival, so the "optional" seamless transition is already planned and better than asked);
  wingmen landing beside the player; the on-foot character, the blade and the crew enemies (ENG-21, 22, 23); route
  posts, markers, objective lines, interactables, encounter volumes and scene checkpoints with "CUT!" retakes
  (ENG-24, 25); transmissions by id. The game is solo with AI wingmen, which is what the document asks to be safe for.
- **Does not match:** `storyline.md` §5 act 2 and ENG-25 have a **wrecked** research site, a **signal-analysis
  device** (objective 1, unlocks Gravity Well) and a **relay** that plays **voices** (objective 2, unlocks Cloak).
  This document has an **intact, silent** outpost and a **geological survey terminal**, and names no device, relay,
  voices or ability rewards. See the open questions.
- **New work if adopted:** an orbital beat with drone convoys that ignore the squad, and a "scan" (no scan action
  exists; the nearest thing is holding a lock on a convoy: *our inference*) (T4, with T2 for the prompt); a small
  interception force over the landing zone (T4); the outpost as a place: perimeter defences to disable or bypass, an
  interior room, the console (T5, builders / Grok Bot for the set); the survey-instrument and dig dressing along the
  route (builders); the "verified / unknown" marker flicker (T2); the lines as transmissions (T3; ids to reserve,
  suggested `G2_*`: *our suggestion*); Nicholas for every line and voice.
- **Owners:** T1 none beyond what exists; T2 scan prompt and marker flicker; T3 transmissions and the cinematic
  trigger; T4 orbital convoys and interceptors; T5 the ground route, the outpost objective and its checkpoints.

**Open questions (the first five are the document's own "open decisions"):**
1. The outpost's final name.
2. The landing / insertion method (we have the ENG-38 descent and the pad landing).
3. Number and type of ground enemies (the document says "limited Dominion ground patrols" and nothing more).
4. How the squad is represented on foot (today: one first-person player; Kato "at the console" would be a voice).
5. Optional environmental interactables.
6. Does the survey terminal **replace** the signal-analysis device and the relay's voices of §5 act 2, or sit beside
   them? Where do Gravity Well and Cloak come from in this version? (Nicholas for the story, Ryan for the rewards.)
7. The length: 10-15 minutes here is half of the whole game's 20-30 minute target (Ryan).

## 15. Cinematic 03: *Beneath the Surface* (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Cinematic_Sequence_03_Beneath_the_Surface.docx`. "The campaign's first investigative
payoff."
**When:** the player activates the outpost's geological survey terminal at the end of §14.
**Length:** 90-120 seconds, one outpost interior.
**Its job:** reveal that the Dominion is hunting a hidden artificial structure, not launching a conventional invasion.
It must not reveal the future-self identity, the final catastrophe or the complete function of the Origin Core.

| Time | On screen | Lines (drafts) |
|---|---|---|
| 0:00-0:20 | **Empty but operating.** The camera tracks through a lit, intact research room: untouched equipment, unanswered comms, recently interrupted work. Triad enters, weapons lowered but cautious | MOSS: "No bodies. No signs of a fight. Where did everybody go?" WREN: "Kato, see if the survey station can tell us anything." |
| 0:20-0:55 | **Below Verdance.** Kato projects layered geological scans: a gigantic, geometrically precise structure deep under the jungle; several Dominion scan sites align across its outline | KATO: "These aren't mineral readings. That shape is artificial. And it predates this facility by a long way." WREN: "So they didn't come here to take the planet." KATO: "They're looking for something beneath it." |
| 0:55-1:15 | **The acknowledgment.** A disabled drone in the corner powers on silently; its optic glows **blue**; it emits **three evenly spaced pulses**; Triad's comms respond without anyone touching them; the drone pauses, then shuts down | WREN: "Did we just answer that thing?" KATO: "I didn't send anything." |
| 1:15-1:40 | **A second coordinate.** The archive shows correlated scan coordinates at **Cinder**. Before the transfer completes, motion alerts: reinforcements approaching | KATO: "There's another match. Cinder. I'm copying the data-" MOSS: "Company incoming. A lot of it." WREN: "Keep that transfer running. We'll hold them off." |

**Hand-off:** cut straight from the alarm to the player's combat view in the same room or a defensive exterior, with
the objective **"Protect the data transfer."** No teleport or unexplained change in squad positions; the transfer is
incomplete when control returns.

**Rules from the proposal:** the three pulses are a future version of the squad's authentication handshake; on first
viewing they read as alien scanning, and **the same rhythm returns in the Dominion finale** with future Triad's command
authorisation; neither Kato nor Wren decodes it now; reuse the opening's encrypted-handshake motif (§11) but **do not
repeat the spoken "Don't-"**, which is reserved for the finale. Voice-only staging is acceptable if on-foot character
animation is constrained; a monitor graphic can stand in for the 3D hologram; the purpose of Cinder's resource stays
unresolved. From `Story/README.md`: Verdance yields survey evidence, not the buried structure or the Origin Core.

**How it maps to the game today:**
- **Exists:** transmissions by id with speaker, style and queueing (T3); the director's in-engine camera work (C-03,
  ENG-33); interactables that can fire an event (ENG-24); a sentry drone enemy class (no finished mesh yet); the blue
  targeting light is already the planned "holds fire" colour of the opening (§11).
- **New work if adopted:** an outpost interior room (T5 layout, builders / Grok Bot for the set); the survey display:
  a monitor graphic of the buried structure with the scan sites and the Cinder match (T2, or a builder's material);
  a dormant drone prop with a blue optic and a three-pulse light and sound cue (T4 / effects builder; the rhythm
  becomes a shared asset so the finale reuses it, like the `Dont` sound); a HUD "comms responded" acknowledgement
  (T2); the cutscene itself: camera track, timing, skip, and no replay on a retry (T3); the lines as transmissions
  (suggested ids `C3_*`: *our suggestion*); Nicholas for lines, voices and the pulse's sound.
- **Staging note (ours):** the game shows no pilot bodies on foot beyond the player, so "Triad enters" and "Kato
  projects" would be played voice-only over the room and the display, which the document allows.
- **Owners:** T3 the sequence; T2 the display and HUD beat; T4 the drone beat; T5 the room and the trigger.

**Open questions:**
1. Confirm a monitor graphic instead of a 3D hologram, and voice-only staging (the document invites both).
2. The length: 90-120 s, on top of a 2:20 opening and a 75-100 s *Aftermath*. A 40-60 s cut would keep all four beats.
3. The three-pulse rhythm needs one definition (interval, tone) that the opening handshake (§11), this scene and the
   finale share. Who writes it down: Nicholas (sound) with T3?
4. Words: Wren's "tell us anything" and Kato's "by a long way" are fine; note for §16 that the story test rejects the
   word "time" in any line before act 4.

## 16. Gameplay Sequence 03: *Facility Defense* (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Gameplay_Sequence_03_Facility_Defense.docx`. Status in the document: "proposed gameplay
specification". It begins the instant Cinematic 03 (§15) ends, with reinforcements closing on the outpost, and ends
when the complete survey archive is secured and Triad reaches extraction, which triggers Cinematic 04 (§17).
**Suggested length: 7-12 minutes.**
**Primary goal:** keep the archive transfer alive long enough to copy the geological records and the Cinder
coordinates. **Secondary:** survive and regroup for evacuation. The player does not retrieve the buried structure.

| Phase | Player objective | Gate / feedback | Beat / draft line |
|---|---|---|---|
| 1 Set defensive positions | Move to the terminal perimeter; identify the approach routes and the limited cover | The first hostile wave enters; the transfer visibly begins | Start: the quiet room of §15 becomes an arena under emergency lights and approaching engines. KATO: "Transfer's moving. If they destroy this station, we lose everything." MOSS: "Then nobody gets near the station." |
| 2 Protect the archive | Stop drones from disabling the terminal while a **progress meter** advances | The transfer reaches a milestone; **checkpoint** (the transfer midpoint) | WREN: "Left approach! Keep them off Kato." Midpoint: **a drone reaches the archive but pauses on identifying Triad** while the others keep attacking; "a half-second targeting hesitation rather than a complex AI state" |
| 3 Counter the disruption | A tougher machine or a jamming unit interrupts the transfer: destroy or disable it, or restore local power | The transfer resumes and completes | Pressure: Kato reports the archive almost complete; a heavier unit forces the squad to reposition instead of camping one doorway |
| 4 Extract | Collect the verified data package; fight or evade to the landing location | The squad reaches extraction and the immediate pursuit breaks → Cinematic 04 | Resolution: the data resolves into two leads, a map of Verdance's subterranean structure and a match on Cinder. KATO: "Data secured. I've got the Cinder coordinates. Time to go." COMMAND: "Triad, extraction corridor is open. Return with the archive intact." |

**Rules from the proposal:** clear attack indicators and spawn limits; a timed wave count or objective-based progress,
not endlessly escalating spawns; a checkpoint at the transfer midpoint, and a failure retries the current encounter,
never Cinematic 03; readable on-screen controls; no specific ship or pilot ability required unless already
implemented; the transfer may run on a flexible timer but must not be an arbitrary long wait; enemy pressure scales to
the actual players, and if AI partners cannot meaningfully defend, the arena is tuned for one active player; the
"emergency reboot" interaction is optional, not a confirmed feature; carry visible minor battle damage into Cinematic
04 if possible; do not reveal the Dominion's origin. **Assets the document lists:** the outpost arena, an archive
progress UI, 2-3 escalating enemy compositions, an extraction marker, the transition into the ship departure.

**How it maps to the game today:**
- **Exists, and is close:** Verdance objective 1 already has "hold-interact for 4 s while 2 waves drop in", then
  "carry the core back (no sprint or block while carrying)" to the ship (ENG-25): that is phases 2 and 4 in small.
  Encounter volumes spawn crew in waves with attack tokens and spawn limits; checkpoints and retakes that never
  replay a cutscene; mercy tiers for a fight the player keeps losing (ENG-26 note); grunts, troopers, heavies, sentry
  drones (ENG-23); the board point as the extraction marker; take-off (ENG-20).
- **New work if adopted:** a **defend-the-object mode**: the terminal as a damageable or interruptible objective that
  enemies path to and "disable", with a transfer that advances on a timer and pauses when disrupted (T5, with T4 for
  the enemy behaviour of targeting an object instead of the player: no enemy does this today, *as far as the
  features.md entries say*); the archive progress meter (T2); the disruption unit: a Heavy can stand in, a "jamming
  unit" would be a new enemy (T4); the half-second hesitation on one drone (T4, small); a second checkpoint inside one
  encounter (T5); emergency lighting state for the room (environment builder); the lines as transmissions (suggested
  ids `G3_*`: *our suggestion*), Nicholas for lines and voices.
- **The player is alone on foot today.** "Keep them off Kato" and "the squad repositions" are voices; the arena must be
  tuned for one player, which the document itself allows.
- **Owners:** T5 the mode, arena and flow; T4 enemy compositions, object-targeting and the hesitation; T2 the meter and
  attack indicators; T3 lines, checkpoints as scenes, the hand-over to §17.

**Open questions:**
1. Is a defence encounter wanted at all before the freeze, and at what length (Ryan)? See the reduced form proposed in
   the scope list: the existing hold-interact stretched into a 60-90 s transfer with three waves.
2. Encounter composition: the document says "drones" and "a tougher machine or jamming unit"; which of our crew types
   (grunt, trooper, heavy, sentry drone) play them (Nicholas for the fiction, T4 / T5 for the fight)?
3. Which ability, if any, is the reward here? The document names none; today objective 1 gives Gravity Well.
4. Kato's "Time to go." contains "time", which `storyline.md` §8 forbids before act 4 and the story test rejects
   (it checks for " time" in every earlier line). A rewording such as "We're leaving." is Nicholas's call.
5. "Carry forward visible minor battle damage" into the departure: no persistent ship damage look exists; skip?

## 17. Cinematic 04: *Verdance Departure* (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Cinematic_Sequence_04_Verdance_Departure.docx`. Status in the document: "narrative
production proposal".
**When:** successful extraction at the end of §16.
**Length:** 70-95 seconds; reuses the Verdance departure shots, the three ship models, a tactical overlay and the
Dominion silhouette; no detailed character animation.
**Its job:** give a clear next mission and turn apparent enemy chaos into a pattern. The squad has evidence that the
Dominion is hunting a prehistoric artificial structure; the data points to Cinder but does not explain what the
structure does.

| Time | On screen | Lines (drafts) |
|---|---|---|
| 0:00-0:20 | **Leaving the jungle.** Triad's craft lift away from the glowing forest; below, the outpost still stands but its perimeter swarms with enemy machines; the squad says little at first | MOSS: "That was a lot of trouble for a few survey files." WREN: "Apparently they wanted those files just as badly." |
| 0:20-0:50 | **The same shape.** Kato overlays the Verdance scans with a second partial pattern from Cinder: two related components or locations; the architecture and purpose stay unreadable | KATO: "They weren't searching at random. These two sites have the same geometric signature." WREN: "Verdance and Cinder. What connects them?" KATO: "I don't know. But there's a survey station on Cinder that may have recorded the missing section." |
| 0:50-1:10 | **A way forward.** Command confirms a hazardous approach to Cinder: Triad needs a heat-resistant stabiliser or locally sourced **shielding component** to cross its volcanic environment. "An operational requirement, not yet a confirmed artifact from the Dominion" | COMMAND: "Proceed to Cinder. Recover the missing survey records and secure any shielding material necessary for the approach. Maintain reconnaissance priority." MOSS: "Volcanoes. Great. I was getting tired of the jungle." |
| 1:10-1:25 | **The Watcher.** As Triad jumps, a brief cut to the distant Dominion: a navigation marker flashes at the exact instant Wren chooses the departure vector | WREN: "Triad, set course for Cinder." |

**Rules from the proposal:** the matching navigation pulse is "nearly imperceptible" and is not proof of time travel;
do not show future Wren, Kato or Moss, a human insignia or a decrypted message; the full opening warning stays unheard
until the finale; do not claim the shielding equipment, the exact resource or a traversal system is implemented;
whether the protection is a craft upgrade, a ground-suit requirement or a general equipment objective is undecided
and is not lore yet. **Next:** "Gameplay 04: Cinder Approach" (not written): optional objectives to navigate a
hazardous orbit, secure environmental protection and locate the archived survey site.

**How it maps to the game today:**
- **Exists:** take-off and the return to Verdance orbit (ENG-20, ENG-38 take-off return point); the three ships; the
  Dominion visible in the galaxy sky (§5 act 1 change); the hyperdrive jump; the director's cameras; transmissions.
  The pulse on a navigation marker repeats the clue of *Aftermath* (§12), so its HUD pulse (T2) is built once and
  used twice.
- **Does not match:** `storyline.md` §5 has **waves 3-4 in the galaxy between Verdance and Cinder**; this sequence
  jumps straight to Cinder. §5 act 3 has a **mining complex** and **crystals that stabilise a passage through the
  distortion field**; this document has a **survey station** with "the missing section" and a **shielding component
  against heat**. `Story/README.md` lists Cinder's shielding, resource and traversal as undecided.
- **New work if adopted:** the cutscene: take-off shots, the overlay of the two scan patterns as one 2D composite
  (T2 or a builder's material), the Dominion cut with the marker flash, skip (T3); the lines as transmissions
  (suggested ids `C4_*`: *our suggestion*); Nicholas for lines and voices. A "shielding" requirement on Cinder would
  be new gameplay (T5 / T1) and is not proposed for building here.
- **Owners:** T3 the sequence; T2 the overlay and the marker pulse; T4 the Dominion shot and the swarm around the
  outpost seen from above; T1 the jump-out.

**Open questions:**
1. Where do waves 3-4 go: before this sequence's jump (Triad is intercepted leaving orbit), on arrival at Cinder, or
   dropped from the story's telling while staying in the game (Ryan for the flow, Nicholas for the telling)?
2. Does the Cinder survey station replace the mining complex and the stabilising crystals of §5 act 3, and what then
   opens the passage through the distortion field (Nicholas)? Until answered, §5 act 3 and ENG-26 stand.
3. The shielding component: Command's line can ship as words only, with no mechanic, or be cut. Which?
4. The length: 70-95 s. A 30-45 s cut (lift-off, the two shapes, the marker flash) keeps the three beats.

## 18. Verdance scope: the decision on sections 14-17
**Decided by Ryan, Nov 13 (14:43 EST): Verdance is built in the reduced form, about 8-10 minutes in all.**
Details per question: `docs/story/verdance-scope-questions.md`.

| Part | Reduced form (what gets built) | Owner |
|---|---|---|
| Arrival and route (3-4 min) | The hyperdrive jump, the descent (ENG-38), a landing and a short route to the station. The cheap clue beats are kept: convoys that ignore the squad carry scanning gear, instruments measure one buried line, a transponder marker flickers | T4 (space and clues), T1 (descent), T5 (route) |
| Cinematic 03, *Beneath the Surface* (40-60 s) | Voice over the room with a monitor graphic; the dormant drone wakes and glows blue. Built in the same pass as the opening and *Aftermath* | T3, with T2 (monitor graphic) |
| Facility defence and extraction (3-4 min) | Objective 1's hold-interact becomes a 60-90 s archive transfer with a progress meter and three fixed waves; the transfer pauses while an enemy stands at the terminal; the third wave brings a Heavy as the disruption; the existing carry-back is the extraction with the data package. No new enemy AI, no enemy that damages the terminal | T5, with T2 (meter) |
| Cinematic 04, *Verdance Departure* (30-45 s) | Three transmissions over the take-off, or a short voice-over shot if the cutscene pass has time | T3 |
| The outpost | One open-sided survey station re-dressed from the wreck site, holding the console and the dormant drone. Not a walk-in facility | Environment / prop builder, T5 fits it |
| Cinder shielding | Words only; no mechanic | T3 (lines) |
| Co-op | Not in the game; the documents are read as solo | - |

**Unchanged:** both Verdance objectives and both rewards (Gravity Well, Cloak), waves 3-4 between the planets, the
20-30 minute target for the whole game, and the rule that nothing reveals the twist before the Dominion.
**With Nicholas (story words, issue #29):** objective 1 takes the new fiction (intact station, survey archive, buried
structure); what the relay (objective 2) means now; what opens the distortion field on Cinder now that the survey
station replaces the mining complex; a replacement for Kato's "Time to go." (the story test rejects " time" before
act 4).

## 19. Gameplay Sequence 04: *Cinder Approach* (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Gameplay_Sequence_04_Cinder_Approach.docx`. Status in the document: "a narrative gameplay
specification; implementation and playtest verification remain separate". It sits between Cinematic 04 (§17) and
Cinematic 05 (§20): it starts as the squad sets course for Cinder and ends when the player activates the **mining
complex survey terminal**, which shows a geometric match with the Verdance archive.
**Suggested length: "a compact 5-7 minute first pass", subject to encounter tuning and the campaign budget.**
**Player goal:** follow the recovered Verdance coordinates to Cinder, identify the excavation, and reach a survey
station overlooking the dig. The player investigates the link between the two worlds "before committing to an
assault". Crystal recovery, the Warden, the EMP unlock and the field anchor all belong to Gameplay 05 (§21).

| Phase | Player does | Ends when | Story beat / draft line |
|---|---|---|---|
| 1 Follow the route | Flies toward the Cinder approach marker; **one bounded interception encounter** pressures the squad while transports continue toward the planet | The approach sector is reached and the transports' destination is identified (**checkpoint** at the Cinder approach) | A deliberate search: most enemy traffic carries excavation equipment to one site; the interceptors defend that route and do not chase Triad indefinitely. COMMAND: "Follow the survey coordinates. Identify the excavation before you engage." KATO: "Those transports are all heading for the same site." |
| 2 Read the excavation | Uses the objective marker to inspect the mining complex from orbit; excavation lights and concentrated traffic single out one search site | The squad marks a safe landing route near the survey station | WREN: "Then that's where we start. Keep the landing route clear." |
| 3 Descend and land | Flies through Cinder's atmosphere to the marked pad, with readable terrain and volcanic hazards on the approach | A safe landing; control passes to the on-foot character (**checkpoint** after landing) | MOSS: "Solid ground. Preferably the kind that stays solid." |
| 4 Reach the survey station | Crosses a short industrial route with cover and a limited patrol encounter; inspects exposed ancient geometry beside the active dig | The survey terminal is used while outside active combat → Cinematic 05 | A familiar response: one intercepted transmission repeats a short phrase Kato has just used; he puts it down to feedback. The matching structure: the terminal compares the Verdance archive with Cinder's exposed geometry and stops at the visual match. KATO: "That pattern again. Get me to the survey terminal." |

(The document lists the five radio lines without placing them in a phase; put in phases by us.)

Objective text on screen: **"Investigate the mining complex,"** then **"Access the survey records."**

**Rules from the proposal:** any space encounter after Verdance belongs inside this approach, "avoiding a separate,
repeated travel battle"; the echoed phrase stays distorted and deniable, and no character identifies an older self; the
Origin Core's function stays unexplained; Cinder's **stabilising crystals** will help create a passage through the
distortion field, and Cinematic 04's heat protection "remains an equipment question" (the crystals are not thermal
shielding); environmental danger shapes the route **without a mandatory heat meter or a new equipment hunt**; abilities
earned on Verdance are kept; nothing here may require EMP Pulse; a ground retry restores the landed state without
replaying the departure or the descent; patrol counts bounded and the safe route readable in solo play; not every pilot
has to stand at the terminal; the terminal is offered when nearby hostiles are cleared or disengaged; on interaction the
player's position is kept and hostile pressure is suspended; a skip applies its story flags once and returns to
Gameplay 05 at the same site.

**How it maps to the game today:**
- **Exists:** waves 3-4 in the galaxy between the planets (ENG-13, 14: gunships, bombers); the hyperdrive jump and
  gate to Cinder orbit (ENG-15, 16); the descent (ENG-38; §5 already asks for the dig's glow to show from orbit); the
  landing and the wingmen landing beside the player (ENG-20); Cinder's hazards: magma, timed geysers, crystals that
  shatter (ENG-26); the on-foot character, blade, crew enemies, route posts, markers, objective lines, interactables,
  encounter volumes and scene checkpoints (ENG-21-24); transmissions by id, with an echo style already used by
  `A3_EchoWren / Kato / Moss`.
- **Matches the adopted story:** the mining complex, the crystals and the field anchor of §5 act 3 are all kept
  (`Story/README.md`: "Gameplay 04 follows the adopted storyline"). This answers §17's open question 2: the survey
  station does **not** replace the mining complex; it is a station **at** the complex, overlooking the dig.
- **Does not match:** the document wants **one** bounded interception on the way to Cinder; the game has **two
  waves** (3 and 4) there. The game has **no survey station or terminal on Cinder**, and nothing happens on Cinder
  before objective 3: today the player lands and goes straight to the outpost or the jammer, in either order. There is
  no "inspect from orbit" or "mark a landing route" action (as on Verdance, no scan action exists).
- **New work if adopted:** transports that ignore the squad on the Cinder approach (the same non-hostile convoy lane
  as §14 and the tutorial battle, built once) (T4); waves 3-4 told as the interception on the Cinder route, by lines
  only (T3 / T4: *our suggestion*); "read the excavation" as the dig's glow and traffic seen from orbit plus one line,
  with no new action (*our reading*) (T1 / T4, environment builder); a short route from the pad to a survey station
  with one patrol (T5); the station itself and its terminal (the Verdance station kit re-used: *our suggestion*)
  (prop builder, T5); the ancient-geometry prop beside the dig, which is the §6 artefact clue (Grok Bot / builders);
  the terminal offered only when no enemy is engaged (T5); the first echo as a transmission (T3); the lines as
  transmissions (suggested ids `G4_*`: *our suggestion*); Nicholas for every line and voice.
- **Owners:** T1 the descent; T4 the transports and the interception; T5 the ground route, the terminal objective and
  its checkpoints; T3 transmissions and the cinematic trigger; T2 objective lines and markers in the HUD.

**Open questions:**
1. Are waves 3-4 the document's "bounded interception", played where they are today and told as the Cinder route, or
   is one of them cut or moved (Ryan for the flow, Nicholas for the telling)? This also settles §17's question 1.
2. The length: 5-7 minutes before any Cinder objective starts (Ryan).
3. Is the survey terminal a new stop before the two existing objectives, and does it lock their order (terminal, then
   excavation, then anchor)? Today both objectives can be done in either order (Ryan / T5).
4. The echo in play repeats a phrase "Kato has just used", so a line of Kato's must be chosen and fired first. Which
   line, and does it replace `A3_EchoKato` (Nicholas)?
5. The speaker is "COMMAND"; act 1-3 placeholders use Control (`A3_PassageOpen`). One name (Nicholas).

## 20. Cinematic 05: *The Pattern* (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Cinematic_Sequence_05_The_Pattern.docx`. Status in the document: "narrative production
proposal; no implementation or voice recording is implied".
**When:** the player activates the survey terminal at the end of §19, outside active combat.
**Length:** 90-110 seconds (target ending about 100 s); the existing terminal, a scan overlay, pilot radio and a view
of the excavation; no new speaking character and no face animation.
**Its job:** turn reconnaissance into a specific recovery mission. The same ancient geometry is on both worlds, and the
mined crystals can stabilise a route through the Dominion's interference. It must not identify the voice in the echo,
show future pilots or mention time travel.

| Time | On screen | Lines (drafts) |
|---|---|---|
| 0:00-0:20 | **The matching shape.** Begins on the terminal where the player interacted. Kato aligns the Verdance outline with an exposed section beneath the mining complex; the scans agree at several distinctive angles; neither shows the complete structure | KATO: "Different worlds. Same construction. They knew where to dig." WREN: "And the survey records gave them the coordinates." |
| 0:20-0:45 | **A usable lead.** The display overlays crystal readings from the excavation with the distortion readings recorded near the Dominion: a narrow, stable band appears inside the interference. The **crystal cache** and the rig's **field anchor** are marked on the local map | KATO: "These crystals hold a stable response inside the interference. With the signal data, we may have a way through." MOSS: "So we take a sample and get out?" WREN: "First we find out what's guarding it." |
| 0:45-1:10 | **The echo.** A hostile transmission cuts across the squad channel: a filtered fragment repeats Kato's earlier words, **"Same construction."** The cadence is familiar but the processing prevents identification; the recording ends before Kato can isolate it | MOSS: "Was that you?" KATO: "No. Could be our channel feeding back. Keep the recording." |
| 1:10-1:40 | **The guarded excavation.** Cut from the terminal map to the excavation beyond the station. The Warden guards the route to the secured crystal cache; an active field anchor distorts the outbound route on the display. The places shown are the ones the player uses next | KATO: "The rig's field anchor is interfering with the route out. We need the crystals, then we shut it down." COMMAND: "Recover the samples and preserve the survey data. Confirm a passage before approaching the Dominion." WREN: "We've got our route. Triad, move." |

**Hand-off:** ends on the access route toward the excavation; control returns in the same survey-station area with the
player facing the first objective and enemies still outside immediate attack range; the objective reads **"Secure the
excavation."** The cache and anchor locations are recorded but neither objective is complete. Combat resumes only once
the player can react.

**Rules from the proposal:** skipping applies the same map markers, recorded-transmission state and objective once,
and grants no crystals, EMP unlock or anchor completion; a later death retries the gameplay checkpoint and never
replays this scene; the repeated phrase stays "an ambiguous interception or feedback event"; the squad does not
identify the speaker or discuss time travel; no future pilots, decrypted personal records or explicit identity
confirmation; **the three-pulse motif is not used here** (kept for its payoff) and **"Don't-" stays reserved for the
finale**; the crystals stabilise passage through the distortion field and are not heat shielding; **the field anchor
"prepares an approach corridor"; the Dominion's four defensive generators remain a later assault objective, and
access to its interior still requires that assault and docking**; the Warden may be shown by its visible position or
by a sensor contact; camera positions and the transmission treatment are production choices. Nicholas owns the final
dialogue, tone and reveal timing; Ryan's Lead confirms the crystal and anchor feedback, the enemy activation point and
the return position.

**How it maps to the game today:**
- **Exists:** transmissions by id with speaker, style (echo, static) and queueing, held during cutscenes (T3); the
  director's cameras (C-03, ENG-33); interactables that fire an event (ENG-24); objective markers; the Warden
  encounter with a checkpoint before it (ENG-26); the monitor graphic of the Verdance scan is already wanted for
  Cinematic 03 (§18), so the Cinder overlay is a second layer on the same graphic.
- **Does not match:** `A3_CrystalsFound` ("These crystals stabilise the field. They were digging a way through it.")
  says the machines dig for the crystals; here the machines dig for the **structure** and the crystals are what Triad
  takes. The three placeholder echoes `A3_EchoWren / Kato / Moss` use character phrases nobody has said in play; the
  document has **one** echo, of words Kato said a minute earlier. §5 act 3 says destroying the anchor "gives the
  passage through the distortion field"; here the crystals (with "the signal data") give the way through and the
  anchor only interferes with "the route out". "The signal data" is from §5's old Verdance (the signal-analysis
  device); §14-18 Verdance yields a **survey archive**, so the phrase needs a source (*our note*).
- **New work if adopted:** the cutscene: terminal shot, the two-scan overlay, the stable band, the map with two
  markers, the cut to the dig, skip, no replay on a retry (T3); the 2D graphics (T2, or a builder's material); the
  echo: the same line played twice, the second through the echo treatment (T3 hook, Nicholas's audio); a "recording
  kept" state for Cinematic 06 and the finale (a story flag: *our reading*) (T3); the Warden visible or marked from
  the station (T5); the lines as transmissions (suggested ids `C5_*`: *our suggestion*); Nicholas for lines and voices.
- **Staging note (ours):** as in §15, there are no pilot bodies on foot: the scene is voice over the terminal, the
  graphic and the dig, which the document asks for.
- **Owners:** T3 the sequence; T2 the overlay and the markers; T5 the trigger, the return position and where the
  Warden stands; T4 nothing new.

**Open questions:**
1. The length: 90-110 s. A 40-50 s cut keeps all four beats (the match, the lead, the echo, the two targets).
2. One echo (this document) or the three placeholder echoes of §5 (`A3_Echo*`), or both? Each extra one is a step
   closer to the twist (Nicholas).
3. "With the signal data": which Verdance object is that now, the survey archive (objective 1) or the relay
   (objective 2)? If the relay, this is also the answer to §18's "what the relay means now" (Nicholas).
4. What the player is told opens the way: crystals, anchor, generators, in that order? See §21's open question 1.
5. Is the name "Origin Core" spoken on Cinder? §5 act 3 says the squad learns the enemy is after its parts or keys;
   the Cinder documents only say its function stays unexplained (Nicholas).
6. If the single cutscene pass (§11 gate) runs out of room, this scene falls back to five transmissions at the
   terminal with the markers appearing (Ryan / T3).

## 21. Gameplay Sequence 05: *Cinder Excavation* (Nicholas's proposal, received Nov 13)
Original: `Story/Triad_Drift_Gameplay_Sequence_05_Cinder_Excavation.docx`. Status in the document: "proposed gameplay
specification; implementation and playtesting remain separate". It begins when Cinematic 05 (§20) returns control at
the survey station and ends when the crystal package is secured, the field anchor is disabled and the player reaches
extraction, which triggers **Cinematic 06, *The Path to Dominion*** (not written yet).
**Suggested length: a "working pacing target" of 4-6 minutes, "to be tuned against Gameplay 04 and the full campaign
rather than added as a fixed requirement".**
**Player goal:** clear the mining complex, secure the stabilising crystals and disable the excavation rig's field
anchor. "Combined with the Verdance signal data, the recovered material provides the means to attempt the Dominion
approach."

| Step | Player objective | Gate / feedback | Beat / draft line |
|---|---|---|---|
| 1 Secure the excavation | Follow the marked industrial route into the dig; clear a bounded defending force; defeat or disable the guarding **Warden** "through the existing encounter rules", with cover, blade, block and deflection where supported | Access to the secured crystal cache opens; the mining-complex objective is recorded (**checkpoint** on entry to the encounter) | A defended industrial site; the Warden forces movement around cover. WREN: "Clear the route to the cache. Watch the rig." MOSS: "That one's staying with the crystals." |
| 2 Recover the crystals | Interact with the cache; the sample container is confirmed secured; Kato compares the sample with the survey readings | The recovered-crystals state is saved and **Lancer's EMP Pulse is awarded** "at completion of the mining-complex objective" (**checkpoint**) | A short pause to understand the reward. KATO: "Samples secured. Lancer's EMP is available." |
| 3 Disable the field anchor | Advance to the rig's marked anchor and destroy or disable it "using the confirmed objective interaction" | The disruption ends; the navigation overlay shows a stable outbound corridor (**checkpoint** after shutdown) | Urgency returns; the shutdown visibly changes the route. COMMAND: "Field anchor marked. Shut it down and confirm your exit." KATO: "The interference is dropping. We have a corridor." |
| 4 Reach extraction | Return by a readable route to the landing area, with "only a bounded pursuit if the encounter budget supports it" | Crystals held and anchor done are confirmed, then extraction is accepted → Cinematic 06 as Triad departs | The withdrawal feels earned. WREN: "Keep the samples intact. Back to the ships." |

Objective text, in order: **"Secure the excavation" → "Recover the stabilising crystals" → "Disable the field
anchor" → "Return to extraction."**

**Rules from the proposal:** the reward is gameplay progression: "do not invent a claim that the crystals themselves
create EMP technology"; show the EMP unlock and its real control prompt without a blocking explanation while the
player is exposed; **EMP is Lancer's ship ability**: the on-foot character never has to use it on the anchor, no
re-boarding is added just to demonstrate it, and its first use belongs after Cinder; Verdance abilities stay
available; the anchor's shutdown prepares the route and **does not complete the mothership's four-generator assault**;
retries never replay Cinematic 05; samples, EMP and the open route survive retries, and rewards and completion events
apply once; a failed extraction returns to a safe ground checkpoint; reinforcement waves are bounded and cleared when
their encounter ends; tuned for one active player; existing difficulty and checkpoint systems, **no new survival
timer**; the echo recorded in Cinematic 05 is carried forward unexplained; **one access console may briefly recognise
the squad before opening**, kept impersonal (no named future pilot, no human identity reveal, no replay of the opening
warning); Cinematic 06 needs crystals, anchor and extraction all done and "does not grant unearned objectives or skip
the later Dominion assault". Enemy counts, Warden tuning, the anchor interaction and the extraction transition are for
Ryan's Lead; Nicholas finalises the dialogue and the next cinematic.

**How it maps to the game today:**
- **Exists, and is close:** Cinder objective 3, the outpost (ENG-26): breach the gate (2 conduits), 3 escalating
  areas, the Warden with a checkpoint before it and the director's mercy tiers, then a claim that unlocks EMP Pulse:
  that is steps 1 and 2. Objective 4, the jammer: collapsing crystal bridges, plant 3 charges under attack, then run
  as the tower collapses: that is steps 3 and 4. §5 act 3 and ENG-26 already call them the mining complex ("Stop the
  Excavation") and the field anchor ("Destroy the Field Anchor"). The board point, take-off (ENG-20, ENG-38),
  per-objective scene checkpoints with "CUT!" retakes, rewards that apply once, and the reward screen with the
  control prompt all exist. ENG-26 already plans one door at the dig that opens to the pilot's ID (§6).
- **Does not match:** the claim at the end of objective 3 is "the EMP prototype"; the document's is a **crystal
  cache**, with EMP awarded at the same moment and not explained by the crystals. Objective names differ from §5 and
  ENG-26. Today the two objectives can be done in either order; the document fixes the order. In code today the
  Dominion's arrival is set when **wave 5** clears (`bRevealsDominion` on the wave), not by Cinder objective 4; the
  document never mentions wave 5. The Warden has no finished mesh yet.
- **New work if adopted:** renames of objectives, markers and prompts to the document's four lines (T5); the claim
  becomes the crystal cache, with a carried or flagged "samples" state (T5); the order gate: anchor only after the
  crystals (T5); the anchor's shutdown feedback: the local distortion ends and a corridor marker appears on the HUD
  (T2, effects builder); extraction refuses until both are done (already how the way back to space opens); the
  recognising console at the dig gate (T5, small); the lines as transmissions (suggested ids `G5_*`: *our
  suggestion*), Nicholas for lines and voices.
- **The player is alone on foot today:** "Clear the route", "Back to the ships" and Kato's comparison are voices.
- **Owners:** T5 the objectives, order, cache, console and checkpoints; T2 the corridor overlay and reward prompt; T3
  lines and the hand-over to Cinematic 06; T4 the corridor's meaning in space (wave 5, the field, the approach).

**Open questions:**
1. **What opens the distortion field?** §5 act 3: destroying the anchor gives the passage. These documents: the
   crystals plus the signal data give a way through, the anchor's shutdown gives "a stable outbound corridor", and
   the four generators are still a later assault. §5 act 4: the four generators **are** the field. One sentence is
   needed for what each of the three does (Nicholas), and one rule for when the built field stops pushing ships away
   (Ryan / T4).
2. Where wave 5 goes: the documents end at extraction and hand to Cinematic 06. Today wave 5 ("Dominion's Guard")
   is fought after Cinder and its clear brings on the assault (Ryan for the flow; Nicholas so that Cinematic 06
   leads into it).
3. Where the EMP comes from in the fiction, if not from the crystals: a prototype stored at the complex (today's
   claim), or awarded with no explanation (Nicholas)?
4. Kato's "Samples secured. Lancer's EMP is available." names a game system in a pilot's voice; the reward screen
   already announces it. Keep, or leave the announcement to the screen (Nicholas)?
5. The length: 4-6 minutes; today's two objectives are not measured against it (T5 to time them on the new terrain).
6. Cinematic 06 is not written; until it is, the departure carries `A3_PassageOpen` or its replacement (T3).
7. The pursuit on the way out: today's "run as the tower collapses" stands in for it, with no extra spawns?
