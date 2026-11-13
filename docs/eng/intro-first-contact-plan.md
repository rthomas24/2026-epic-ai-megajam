# First Contact, the tutorial battle and Aftermath: build plan (T3, director)

Status: **plan only; built ONCE, after the art gate** (storyline.md §11). Gate: the Rodin player ships, the Null Armada
fleet including a space drone, the Dominion exterior with hangars that open, the effects builder's round 4 (engines,
tracers, explosions, the distortion field) and the new HUD, all on `main`. The Lead says when. Until then nothing here
is shot; the lines are already in the game as transmissions (ids below, `docs/story/transmissions.md`).

Sources: `docs/story/First-Contact-Cinematic-Brief.md` (the opening and the finale payoff), storyline.md §11-13
(tutorial battle, Aftermath), `Story/Triad_Drift_Sequence_02_Aftermath.docx`. Words, voices and score are Nicholas's;
every line below is a transmission id whose text and audio he replaces through `DA_Story_Transmissions`.

## 1. The shape of the first ten minutes

| Part | Length | Plays as | Ends with |
|---|---|---|---|
| **First Contact** (cinematic) | ~2:20 (140 s) | `UTDIntroDirector` code shots, real-time, skippable after 1 s | "Don't-", Wren calls the break, the camera settles into the player's ship, HUD on: `OnIntroFinished` |
| **Tutorial battle** (play) | 6-10 min | today's wave 1 (+ wave 2's gunship) reshaped into 5 phases, one scene with 3 checkpoints | the drones stop at the perimeter: "They're breaking off." |
| **Aftermath** (cinematic) | 75-100 s | the same director, a second shot list (`UTDIntroDirector::PlaySequence(Aftermath)`), skippable after a first viewing | match cut on the jump to Verdance (ENG-38 descent) |

## 2. First Contact: shot list (140 s)

Times are from the first frame. `Reuse` names what exists in `TDIntroDirector.cpp` today (shot ids Nebula, Eclipse,
Squad, Belt, Gate, ThreatFar, ThreatClose, Title). Lines are transmissions fired by the shot at the time given (they
queue, so the transmission system keeps them from overlapping; the subtitle shows over the letterbox).

| # | Shot | Start | Len | On screen | Lines (id at time) | Reuse / needs |
|---|---|---|---|---|---|---|
| 1 | **Black / static** | 0:00 | 6 s | Black, a scan line of static. No score. | `O_PatrolSeven` at 0:01 (Static) | New: black + static overlay (mine). Audio: Nicholas |
| 2 | **The quiet system** | 0:06 | 10 s | Fade up on Verdance and the sun; slow 35 mm push. | (Patrol Seven's call cut off at ~0:09) | Reuse **Nebula** / **Eclipse** framing |
| 3 | **The silhouette** | 0:16 | 9 s | 135 mm: the Dominion's outline on the horizon, dark, impossibly large. | | Reuse **ThreatFar** lens; T4: the Dominion visible and dormant (no warp-ins) |
| 4 | **Jump-in** | 0:25 | 8 s | Rift, Lancer, Bastion drop out of a jump and settle into a wedge. | `O_Command_Orders` at 0:26 | **T1: the jump-in arrival** (hyperdrive exit FX, ships decelerating into formation); effects builder: jump flash |
| 5 | **The three** | 0:33 | 14 s | Low 24 mm pass; a name card per pilot (Wren, Kato, Moss) with their trait line. | `O_Wren_HowBad` 0:34, `O_Kato_StopSaying` 0:38, `O_Moss_Especially` 0:41 | Reuse **Squad** (name cards, trait lines from `Trait_*`) |
| 6 | **Approach** | 0:47 | 8 s | Over the squad's shoulders toward the Dominion; it grows. | `O_Wren_BigShip` 0:49 | Reuse **Belt** dolly geometry, aimed at the Dominion |
| 7 | **The surface** | 0:55 | 16 s | Close along the hull: dormant, no lights; the HUD flickers (instrument glitch). | `O_Kato_NoSignature` 0:56, `O_Moss_Message` 1:03, `O_Kato_Handshake` 1:06 | Reuse **ThreatClose** under-hull lens. **T2: an instrument-glitch flicker on the HUD** (a 0.5 s noise state). The handshake: an on-screen code burst (mine, in the overlay) |
| 8 | **Hangars open** | 1:11 | 9 s | Wide: hangar doors open along the hull, light spills. | | **T4: Dominion hangars that open** (a timeline hook `SetHangarsOpen(Alpha)`) |
| 9 | **The launch** | 1:20 | 10 s | Drone streams pour out toward the planets in lanes, mostly ignoring the squad. | | **T4: launch streams** (showcase drones flying lanes to Verdance / Cinder; `SpawnShowcaseShip` exists) |
| 10 | **The drone that holds** | 1:30 | 12 s | One drone stops beside Rift, scans it, its eye light goes **red to blue**, it holds fire. | `O_Wren_WhyNotFiring` 1:36 | **T4: a drone with a settable eye colour and a scan pose** (`SetEyeColour`, hold). Close 85 mm two-shot (new shot type: two subjects) |
| 11 | **They turn** | 1:42 | 8 s | Another group breaks formation and turns toward the squad. | `O_Kato_Contacts` 1:43, `O_Command_Disengage` 1:47 | T4: a group of 4-6 fighters peeling toward the camera |
| 12 | **"Don't-"** | 1:50 | 10 s | The attack begins; the image tears for a frame on the voice. | `O_Unknown_Dont` 2:00 (Static, the shared "Dont" sound) | Mine: a 2-frame tear on the line (overlay). Effects: first tracers |
| 13 | **The break, into play** | 2:00 | 20 s | `O_Wren_Break`; the camera settles into the player's chase view (the existing hand-off, slowed to 3 s), the HUD comes on, `O_Command_Evade` as the first volley arrives. | `O_Wren_Break` 2:01, `O_Command_Evade` 2:10 | Reuse **Title**'s ease-in and `BeginHandOff`. **The control frame is 2:20**: `OnIntroFinished` (the music director's cue) |

**Total 2:20 (140 s).** `GetIntroLength()` returns it automatically (shots + hand-off), so T1's prelude logic needs no
change. The title card moves to the end of the tutorial battle, or off (Nicholas's call); the current 49 s cut and its
beat sheet stay valid until the rebuild.

**Skip:** unchanged. Hold any button after 1 s; the music fades in 1 s; the squad is placed exactly where shot 13 hands
off, so a skip lands in the same tutorial state as watching it. Lines queued by the cinematic are dropped on a skip
(new: `UTDStorySubsystem::ClearQueue()`), except the two that start play (`O_Wren_Break`, `O_Command_Evade`), which fire
at the control frame either way.

**Music:** one prelude cue lands on the control frame (`OnIntroFinished`, `GetIntroTime`, `GetIntroLength`). The brief
asks for no score in the first ~25 s, so the prelude starts at 0:25 at the earliest (it may be 115 s long).

**What I build after the gate (T3):**
- 13 shot entries (the data-driven `FTDIntroShot` list grows; the shot types exist; new: a two-subject framing for shot
  10, a black-with-static shot for 1);
- transmission timings per shot (`FTDIntroShot::Lines`: id + offset; fired through `UTDStorySubsystem::Fire`);
- the overlay effects: static, the handshake code burst, the 2-frame tear on "Don't-";
- the hooks into T4's hangars, launch streams and drone, and T1's jump-in;
- a live capture and a beat sheet like `intro-beats.md` for Nicholas's score.

## 3. The tutorial battle: one mission, five phases (storyline.md §13)

One scene ("FirstContact"), three checkpoints (retakes restart from the last one; the cinematic is never replayed). My
onboarding hints already teach one action at a time with a tick-off and a second chance; the phases give each hint its
moment. Prompts appear in T2's lower lane.

| Phase | Player learns | Hint (mine) | Call | Ends when | Owner of the encounter |
|---|---|---|---|---|---|
| 01 Break formation | steer, throttle, boost; evade a readable volley | Fly, then Boost ("evasive") | `O_Command_Evade` (from the cut) | a safe corridor reached / the volley survived | T4: a slow, readable first volley; T1: the evade corridor marker |
| 02 Return fire | target, fire | Fire (+ Target cycle) | `TB_Kato_Marking` | the marked drones down | T4: a small group of 3-4 fighters |
| 03 Fight as Triad | switching, each ship's power | Switch, then Power on each ship | `TB_Moss_ThreeShips` | each power used once (or a timed substitute) | T4: staged situations: a missile salvo for Rift's portals, a line of fighters for Lancer's ram, a cluster for Bastion's swarm |
| 04 Heavy contact | everything under pressure | none (the essentials are done) | `TB_Wren_Bigger` | the gunship driven off | T4: today's wave 2 gunship |
| 05 Withdraw | fly to a marked escape vector | (Hyperdrive later) | `TB_Command_FallBack`, then `TB_Kato_BreakingOff` | the boundary is crossed and the pursuers stop at it | T4: pursuers that stop at a boundary; T1 / T2: the corridor marker |

Checkpoints: the start of phase 01, the start of 04, the start of 05 (each a take of scene "FirstContact"; the slate
shows "SCENE 1 · FIRST CONTACT · TAKE n").

**Open with Nicholas (issue #29):** phase 03 names Cloak, EMP Pulse and Gravity Well, which the game awards on the
planets; as built, the tutorial teaches the signature powers (portal blaster, afterburner ram, missile swarm and dome).
Proposed: keep the progression and teach the signature powers. The "Don't-" audio is not repeated in the battle.

## 4. Aftermath (75-100 s, storyline.md §12)

Played by the same director as a second shot list, straight after "They're breaking off." Skippable after a first
viewing (the hint profile remembers it).

| # | Shot | Start | Len | On screen | Lines | Needs |
|---|---|---|---|---|---|---|
| 1 | **Regroup** | 0:00 | 20 s | The squad regroups beyond a faint interference boundary; the last drones peel away; the Dominion far behind; the HUD settles. | `S2_Moss_Accounted` 0:02, `S2_Wren_WelcomeParty` 0:06, `S2_Kato_NotPursuing` 0:12 | reuse the squad framing; the boundary shimmer (effects) |
| 2 | **A pattern** | 0:20 | 25 s | **The tactical display**: hostile lanes from the Dominion to Verdance; some drones never targeted Triad. A faint pulse: **the navigation display marks the drones' corridor, then clears**. Nobody comments. | `S2_Kato_Vectors` 0:21, `S2_Wren_WhatDoing` 0:27, `S2_Kato_Planetside` 0:30, `S2_Moss_InTheWay` 0:35 | **T2: the tactical display and the navigation pulse** (P3 list). Cockpit / HUD close-up framing (new: a HUD-only shot) |
| 3 | **New orders** | 0:45 | 25 s | Command through static; the squad side by side. | `S2_Command_Report` 0:45, `S2_Wren_Hostile` 0:50, `S2_Kato_Coordinated` 0:55, `S2_Command_Outpost` 1:00, `S2_Moss_FindOut` 1:07, `S2_Command_Orders` 1:09 | reuse Belt / Squad lenses |
| 4 | **Course to Verdance** | 1:10 | 20 s | They turn for Verdance; the camera lingers on the Dominion: **a row of its lights pulses in the same rhythm as the HUD pulse**; match cut on the jump. | `S2_Wren_FormUp` 1:12, `S2_Kato_Recorders` 1:16 | **T4 / effects: the Dominion light row**, driven by the same pulse clock as T2's (a shared `PulseTime` the director hands both) |

Ends on the hyperdrive jump: the hand-off to ENG-38's descent (no loading screen). "Don't-" is not used here.

## 5. Who does what (all after the gate unless marked)

- **T3 (me):** both shot lists, transmission timing, overlays (static, code burst, tear), skip and hand-overs, checkpoint
  scenes, hint pacing per phase, the captures and beat sheets. **Now:** the lines (done: `O_*`, `TB_*`, `S2_*`, `F_*`),
  this plan.
- **T4:** the Dominion dormant then hangars opening, launch streams in lanes, the drone that scans and goes blue, the
  attacking group, the pursuers that stop at a boundary, the light row, the tutorial encounters per phase.
- **T1:** the jump-in arrival, the evade corridor and escape vector markers, the music director (prelude to the control
  frame).
- **T2:** the instrument glitch, the tactical display, the navigation pulse, tutorial prompts in the lower lane.
- **Effects builder:** jump flash, first tracers, the boundary shimmer, the distortion field.
- **Nicholas:** every line and voice, the score, the "Don't-" fragment (one sound, used twice), the title placement.
