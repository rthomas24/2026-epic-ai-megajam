# Triad Drift: Animation plan and ledger

Ryan, Nov 13: "we need lots and lots of animations for all the movements, and everything needs to be AAA quality."
This file is the single place for **every animation in the game**: who makes it, how, where it lives, and its state.
`features.md` says what the game needs and `storyline.md` what it means; this file says how each movement gets there. Keep it current: a row changes
when the work changes.

## 1. Who leads

| Role | Who | What they do |
|---|---|---|
| **Lead of animation** | Ryan's Lead (Claude Code), for Ryan | Owns this file and the queue; decides the route for each animation; briefs Grok Bot and the animation builders; reviews every capture; integrates to `main` |
| **Raw motion and rigged models** | Grok Bot (Ryan's asset-generation agent) | Generates clips and rigged characters with third-party tools; delivers sources only to `ArtSource/EngAI/Animations/**` (queue in `MESSAGEBOARD.md` R-030) |
| **Animation builders** | Lead's sub-agents (Claude) | Retarget, clean up, cut to gameplay lengths, author in Blender or in Unreal, build animation blueprints and blend spaces, capture every move in the running game |
| **Gameplay timing** | T5 (on foot), T1 / T3 / T4 (ships, camera, enemies) | Which animation plays when; hit frames, deflect windows, root motion vs in place; notifies and state machines in code |
| **Direction and sign-off** | Nicholas | Approves the look of every animation, as with all art. His own animation at the Presentation name replaces ours with no code change |

Nick's agents are **not asked to build animations**. To ask about one, send a note or request a change, post on
`MESSAGEBOARD.md` with **`Animation:`** in the subject (same process as `Asset:`, R-031 / R-032).

## 2. Three ways an animation gets made (the Lead picks one per row)

| Route | Use it for | How | Strength / limit |
|---|---|---|---|
| **A. Generated or library motion** (Grok Bot; third-party sites) | Full-body humanoid motion: combat combos, locomotion sets, hit reacts, deaths, boss moves | Grok Bot generates or sources clips on a standard humanoid (UE5 Manny-compatible where possible) with a recorded licence, FBX at 30 fps, one clip per file. A builder retargets with the existing retargeters | Fast and natural. Needs clean-up; timing rarely matches gameplay; must be licensed and logged in `CREDITS.md` |
| **B. Authored in Blender** (animation builder, `blender-*` skills) | Mechanical and prop animation (the blade unfolding, doors, turrets, the drone's shell, landing gear, generator petals), short bespoke character moves, fixing or re-timing route A clips | Keyframed on the asset's own rig by script, exported as FBX with the `blender-unreal-export` skill | Exact control and exact lengths. Slow for long organic motion |
| **C. In-engine, procedural** (animation builder + the owning teammate) | Everything that should react to gameplay: aim offsets, additive hit reacts, lean and bank, foot IK on slopes, look-at, recoil, hover bob, idle breathing, blade trail timing, ship banking and death tumble, camera animation | Animation blueprints, blend spaces, Control Rig, additive layers, curves and code | Always matches gameplay, no clips to manage. Cannot invent a whole combat move |

**Approved library source (Ryan, Nov 13): Epic's Game Animation Sample Project.** "If we need super realistic
animations for easy access, we can use Epic Games' Animation Sample Project." It is Epic's free sample (Fab / the Epic
launcher): several hundred motion-captured clips on the UE5 mannequin, made for motion matching. Use it as a route A
source for **body movement**: idles, starts, stops, pivots, turn in place, strafes and backpedals at walk / run /
sprint, jumps, hard and soft landings, traversal (vault, mantle, climb). It has no blade combat, so combat moves still
come from Grok Bot or Blender. Rules: retarget with our existing retargeters (`RTG_Manny_to_Pilot`,
`RTG_Manny_to_NullCrew`); import only the clips we use (the package is near its size limit); confirm the licence shown
on its Fab page before first use and log it in `CREDITS.md` and the submission's third-party asset list (jam rule).
It replaces the template mannequin set wherever the audit scored a move low for foot sliding or weight.

Most finished moves use two routes: a clip (A or B) for the body, plus C on top so it reacts.

## 3. Pipeline and stages (one row passes through these in order)

1. **Requested**: a row in §5 with the gameplay timing it must hit (owner teammate fills the timing).
2. **Source**: clip or rigged model in `ArtSource/EngAI/Animations/<Set>/` (route A) or `ArtSource/EngAnim/<Set>/` (route B),
   with `SOURCE.md` (tool, licence, prompt or reference) and a preview GIF or contact strip.
3. **Retargeted and cleaned**: on our skeleton, feet planted, no pops, looped where it loops, cut to length, root motion
   decided, notifies placed (hit frame, footsteps, trail on / off, deflect window).
4. **In the game**: imported under `Content/TriadDrift/Gameplay/Characters/<Character>/Anims/`, wired through the character's
   visuals asset or animation blueprint, with a capture strip of the move **played in the running game from the real
   camera** in `ArtSource/EngAnim/<Set>/review/`.
5. **Timing verified**: the owning teammate confirms hit frames and windows in code match the motion, with a test.
6. **Reviewed**: a fresh agent scores it against §4; the Lead looks at the strip.
7. **Approved by eye**: a human (Ryan or Nicholas) has watched it in play.

Stages map to `AAA.md`: 2-3 = prototype, 4-5 = technically verified, 6 = reviewed, 7 = presentation approved.
Older versions are never deleted (`docs/eng/ART-HISTORY.md`).

## 4. The quality bar (what "AAA" means for a move here)

- **Readable from the gameplay camera**: the pose at the hit frame reads as that action in a single still, in first
  person and from the third-person chase camera.
- **Anticipation, hit, follow-through**: every attack has all three; light attacks reach the hit in 0.35-0.5 s, heavy
  in about 0.6 s; nothing starts or ends on a pop.
- **Weight**: hips and spine move, not only arms; feet stay planted or step; no sliding at any speed.
- **Responsive**: input is never locked longer than the gameplay rule allows; every move can be cancelled where the
  design says it can (dash, block).
- **Blends**: idle to move to attack to hit react with no visible snap; additive hit reacts on top of anything.
- **Matches the gameplay**: hit frames, deflect windows and trails line up with what the code does, to the frame.
- **Variety**: at least two variants where a move repeats often (deaths, hit reacts, idles, footsteps).
- **Consistent style**: one-handed, confident blade style for the pilot; heavy and mechanical for the Null Armada.
- **Performance**: 30 fps clips, compressed; a crowd of ten crew costs under 1 ms of animation time.

## 5. Ledger: every animation the game needs

State values: `none`, `placeholder` (works, not good enough), `source in`, `in game`, `timing verified`, `reviewed`,
`approved`. "Now" is what exists on `lead/int` on Nov 13.

### 5.1 Pilot, on foot (skeleton `SKEL_Pilot`, Nicholas's Sentinel Armor; retarget `RTG_Manny_to_Pilot`)
| Move | Now | State | Route | Owner / next |
|---|---|---|---|---|
| Idle (3 variants), walk, casual walk, run | `A_Pilot_Idle02/3/6`, `Walk`, `CasualWalk`, `Run` | in game | A (from the Sentinel sample) | Audit; add start / stop and turn-in-place; blend space by speed and direction (strafe, backpedal) |
| Jump start, fall loop, land | `A_Pilot_JumpStart`, `FallLoop`, `Land` | in game | A | Audit; hard-landing variant |
| Dash | `A_Pilot_Dash` | in game | A | Audit against the dash distance and time in code |
| Blade light 1, 2, 3 (combo) | `A_Pilot_Light1/2/3` | in game | A | Audit first; replace with Grok Bot's one-handed blade set if it scores under 4 |
| Blade heavy lunge | `A_Pilot_Heavy` | in game | A | Same |
| Air slam | none (uses the heavy) | none | A, then C | Grok Bot clip |
| Block hold, deflect flick, perfect-deflect flourish | `A_Pilot_Block` only | placeholder | A + C | Flick and flourish needed; block as an upper-body layer so the pilot can walk |
| Hit react front / back (+ additive) | `A_Pilot_HitReact_*`, `*_Add` | in game | A | Audit |
| Death front / back | `A_Pilot_Death_Front/Back` | in game | A | Audit; ragdoll blend on impact |
| Unarmed alternates | `A_Pilot_Alt_LeftHook`, `Alt_SpinKick` | in game, unused | A | Decide: use as a guard-break shove, or drop |
| Blade ignite / retract, blade idle (lit) | none | none | B + C | Short hand flourish; the blade mesh's own unfold (route B) once the Grok Bot blade arrives |
| Interact (console, relay, core pickup), board ship, exit ship | none (cut to camera) | none | A or B | 1-2 s each; needed for the planet objectives to read |
| Aim / look offset, foot IK on terrain slopes, lean into a run, breathing | none | none | C | The terrain is now mountains: foot IK matters |
| First-person arms: swing set, block, deflect | code-posed blade | placeholder | B + C | Arms-only versions matched to the third-person timing |

### 5.2 Null Armada crew (skeleton `SKEL_NullCrew`; retarget `RTG_Manny_to_NullCrew`; template mannequin set today)
| Move | Now | State | Route | Owner / next |
|---|---|---|---|---|
| Trooper: idle, walk, run, aim, fire, reload, take cover, hit react, death x2 | template mannequin set, retargeted | placeholder | A + C | Audit; rifle set with aim offset; deaths with variety |
| Grunt: idle, charge, melee swing, hit react, death | template set | placeholder | A | Melee set |
| Heavy: idle, slow walk, shield up, shield bash, shielded hit, guard-break stagger, death | template set | placeholder | A | Shield set; the stagger must read (it opens the heavy to damage) |
| Deflected-bolt hit (their own shot returned) | generic hit react | placeholder | C | A bigger additive hit |
| Drone: hover bob, strafe tilt, eye track, fire recoil, death tumble | code hover, physics death | placeholder | C (+ B for shell parts) | Tilt into movement, eye look-at, thruster flicker |
| **Warden (boss)**: idle, walk, ground slam, heavy weapon fire, summon, shield block, stagger, phase-change roar, death | none (no model yet) | none | A (Grok Bot, with the rigged model) + C | Concept approved; the slam and summon telegraphs must be readable 1 s ahead |

### 5.3 Ships and space (no skeletons; code, curves and effects)
| Movement | Now | State | Route | Owner / next |
|---|---|---|---|---|
| Player ships: bank, pitch, drift lean, boost surge, dodge roll | flight model + camera | in game | C | T1: compare the three ships' feel; add control-surface / thruster motion if the Rodin ships have movable parts |
| Ship switch, hyperdrive wind-up and jump, portal entry | camera and effects | in game | C | T1 / T3 with the effects builder |
| **Atmospheric entry (ENG-38)**: ship attitude into the burn (nose-up angle of attack), buffet, control-surface or airbrake deployment, heat-shield plates if the Rodin ships have them; wingmen burning in alongside | level change behind a short entry | none | B + C | T1 (+ Grok Bot for deployable parts, effects builder for the plasma) |
| Landing and take-off: gear, settle, lift | scripted path | placeholder | B + C | Landing gear and a settle bounce once the Rodin ships are in |
| Wingmen formation motion | wingman AI | in game | C | T3 |
| Enemy ships: attack runs, evasive rolls, death tumble, carrier bay doors, turret tracking | AI + physics | in game | C (+ B for doors) | T4: death tumble variety; turret barrels recoil |
| Dominion: generator petals, reactor exposure, breakup | code + placeholder bursts | placeholder | B + C | T4 with the effects builder; a trailer-worthy breakup |
| Rift blade-style props: the pilot's energy blade unfolding | none | none | B | After Grok Bot's blade model |

### 5.4 Cameras and presentation
| Movement | Now | State | Route | Owner / next |
|---|---|---|---|---|
| Intro shots (49 s), money-shot cameras, switch cut | code-driven camera moves | in game | C | T3 |
| Chase camera feel (speed FOV, boost kick, shakes) | done Nov 13 | in game | C | T1; human judgement |
| Menu backdrop fly-by, victory scene | code | in game | C | T3 |
| UI motion (HUD pulses, slate, name card, onboarding ticks) | code-drawn | in game | C | T2 / T3; Nick designs the cards |

## 6. Order of work (Nov 13-14)
1. **Audit what exists** (animation builder, now): a capture strip of every pilot and crew move from both cameras,
   scored against §4, the worst five named. This decides which Grok Bot clips are needed first.
2. **Pilot blade set** (Grok Bot → builder → T5): light 1-2-3, heavy, air slam, block / deflect / flourish, with trails
   and hit-stop matched. This is the on-foot game.
3. **Pilot body mechanics** (builder, route C): strafe blend space, start / stop, foot IK on slopes, upper-body block.
4. **Crew sets** (Grok Bot or library → builder): trooper rifle, grunt melee, heavy shield; death and hit variety.
5. **Warden** (Grok Bot rigged model + clips → builder → T5).
6. **Interactions** (builder, route A or B): console, relay, core pickup, boarding.
7. **Mechanical** (builder, route B): blade unfold, landing gear, bay doors, turret recoil, generator petals.

## 7. Files and names
- Sources from Grok Bot: `ArtSource/EngAI/Animations/<Set>/` (never edited in place).
- Authored sources, scripts, review strips: `ArtSource/EngAnim/<Set>/`.
- Unreal assets: `Content/TriadDrift/Gameplay/Characters/<Character>/Anims/` — `A_<Character>_<Move>[_NN]`,
  `BS_<Character>_<Name>` (blend space), `ABP_<Character>`, `RTG_*`, `IK_*`, `CR_*` (Control Rig), montages `AM_*`.
- Wiring: the character's visuals asset (`DA_Pilot_Visuals`, `DA_Crew_*_Visuals`: `AnimBPClass` and the clip slots).
- Licences and tools: `CREDITS.md`. Replaced versions: `docs/eng/ART-HISTORY.md`.
