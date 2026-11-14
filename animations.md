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
| Idle (3 variants), walk, casual walk, run | `A_Pilot_Idle02/3/6`, `Walk`, `CasualWalk`, `Run` **Nov 13 night (builder):** `A_Pilot_Idle3_v2` and `A_Pilot_Walk_v2` (wrist turned so the blade hangs forward and down, not across the body; hilt 3-13 cm clear of the thigh through both clips) are what `DA_Pilot_Visuals` plays; originals kept. Right-hand grip: `A_Pilot_Grip_R` over every clip (grip layer in `UTDLoopBlendAnimInstance`) and `BladeSocket` on `SKEL_Pilot`; the thumb skin still spikes (Meshy weights). | in game (captured Nov 13; not played by a human) | A (from the Sentinel sample) | Audit; add start / stop and turn-in-place; blend space by speed and direction (strafe, backpedal) |
| Jump start, fall loop, land | `A_Pilot_JumpStart`, `FallLoop`, `Land` | in game | A | Audit; hard-landing variant |
| Dash | `A_Pilot_Dash` = Grok Bot's dash (frames 2-20, in place); the old leap is kept as `A_Pilot_Dash_v1` | in game (captured Nov 13) | A | T5: the same clip plays for the 0.7 s slide at x0.8 (a sprint pose, not a slide): a slide clip and slot are still missing |
| Blade light 1, 2, 3 (combo) | `A_Pilot_Light1/2/3` = Grok Bot's one-handed slashes, trimmed so the hand peaks 0.10 / 0.13 / 0.13 s after the press at the code's rates; Light 3 is the Grok chop on Light 1's legs (the full chop ducks out of the close-combat camera). The boxing clips are kept as `_v1` **Nov 13 night (builder):** `A_Pilot_Light1_v2` / `_Light2_v2` / `_Light3_v2`: right arm re-authored in engine (blade forward at the cut, 0.10 s after the press at x1.6 / x1.5; Light1 right to left, Light2 left to right, Light3 an overhead cut). Originals pointed the blade backward. | made, captured; **wired by T5 (names in `UpdateBodyActions`)** | A | T5: play the `_v2` names; effects builder: a swing trail (the Light1 cut is hidden behind the body from the chase camera) |
| Blade heavy lunge | `A_Pilot_Heavy` = Grok Bot's thrust from frame 17 (thrust 0.14 s into the 0.2 s lunge at x1.4); `A_Pilot_Blade_HeavyWindup` (0.57 s) made, not wired; the old clip is kept as `A_Pilot_Heavy_v1` **Nov 13 night (builder):** `A_Pilot_Heavy_v3`: wrist follows the forearm line through the thrust (blade forward 0.98, down 0.10 at mid-thrust). | made, captured; wired by T5 | A | T5: play `A_Pilot_Blade_HeavyWindup` at about x1.6 during the 0.35 s hold (no slot today: the press fires a light swing first) |
| Air slam | the code plays `A_Pilot_Light3` x1.2; Grok Bot's r1 clip is rejected (a 4.8 m jump with the arms overhead, not a blade slam; kept unwired as `A_Pilot_Blade_AirSlam`) **Nov 13 night (builder):** `A_Pilot_AirSlam_Land_v2`: blade driven down in front on landing (the original pointed backward). | made, captured; no slot | A, then C | Grok Bot r2; T5: slots for a fall pose and a landing hit (`A_Pilot_AirSlam_Fall` / `_Land` exist as stand-ins) |
| Block hold, deflect flick, perfect-deflect flourish | `A_Pilot_Block` kept (in game its upright blade reads better from behind than Grok Bot's high guard, `A_Pilot_Blade_BlockHold`, unwired). `A_Pilot_Blade_DeflectFlick` (0.97 s, flick at 0.13 s) and `A_Pilot_Blade_Flourish` (1.97 s, peak 0.43 s) are retargeted, **not wired: no code slot** **Nov 13 night (builder):** `A_Pilot_Blade_DeflectFlick` checked in game (blade sweeps forward). | source in (flick, flourish) | A + C | T5: an upper-body one-shot `A_Pilot_Blade_DeflectFlick` on each deflect and `A_Pilot_Blade_Flourish` on a perfect deflect (its first 0.6 s fits the slow-motion beat; the rest turns the body: cut it) |
| Hit react front / back (+ additive) | `A_Pilot_HitReact_Front/_Back` = Grok Bot's reacts at 55 % amplitude, full pose: the flinch plays for the first time (the old additive clips were refused; kept as `_v1`). Full-strength `A_Pilot_Blade_HitFront/_HitBack` unwired | in game: front captured Nov 13, back not captured | A | T5: one flinch per hull hit restarts on every bolt of a burst: rate-limit it; guard-broken uses the same clip. The `*_Add` clips still name the old paths as their base pose: rebuild or drop them |
| Death front / back | ragdoll in code; `A_Pilot_Death_*` unused. Grok Bot's `A_Pilot_Blade_Death1` (onto the back, 2.47 s) and `_Death2` (side crumple) are retargeted, **not wired: no code slot**; the lying body rests about 15-30 cm above the floor in both (in-place bake) **Nov 13 night (builder):** `A_Pilot_Blade_Death1_v2` / `_Death2_v2`: lying pose lowered 19.8 / 6.7 cm so hands and feet rest about 4 / 10 cm off the floor. | made, captured; no slot | A | T5: play a death clip for 0.5-1 s, then ragdoll; builder: drop the lying pose to the ground first |
| Unarmed alternates | `A_Pilot_Alt_LeftHook`, `Alt_SpinKick` | in game, unused | A | Decide: use as a guard-break shove, or drop |
| Blade ignite / retract, blade idle (lit) | `A_Pilot_Blade_Idle` (Grok Bot's mid-guard loop, 2.97 s, loop closed) retargeted, **not wired**: the only idle slot is `DA_Pilot_Visuals.Idle`, used everywhere on foot, and a guard stance with the blade off looks wrong while exploring | source in | B + C | T5: a combat-idle slot (enemy within `CloseCombatRange`, or blade lit) for `A_Pilot_Blade_Idle`; ignite / retract still to make |
| Interact (console, relay, core pickup), board ship, exit ship | none (cut to camera) **Nov 13 night (builder):** `A_Pilot_Interact_Console` / `_Draw` / `_Shoulder` exist; `A_Pilot_Board_Ship` (1.10 s) and `A_Pilot_Exit_Ship` (0.63 s) composed in place from the jump, fall and hard-landing clips. | made, captured; no slot **Nov 14, ART-08 audit (builder):** `A_Pilot_Interact` (1.33 s; `A_Pilot_Interact_Shoulder` at rate 1.25): one clip for console / relay / device, the LEFT hand reaches about 50 cm forward at chest height at about 1.04 s, so the first-person blade hand is not used. Board (1.10 s) and exit (0.63 s) stay as made; the camera cut covers the rest. None has a code slot: T5 plays them with `PlayBodyAction`. **Nov 14, round 5 (builder):** captured in third person: the reach reads from the side (left arm out at chest height from 0.53 s, held to the end); the blade hand swings at about 0.5 s (review MAJOR, open) and the reach is hidden by the body from the chase camera. **Nov 14, round 6 (builder):** `A_Pilot_Interact` r2: the blade arm is locked to the idle pose, the clip starts from idle and the left arm returns to rest over the last 0.3 s. Review: the blade no longer swings up, but at 0.80 s the left hand rises to the helmet between two forward reaches (it reads as a salute; open). | made; interact not yet captured; no slot (T5) | T5: slots; retract the blade during board, exit and interact |
| Aim / look offset, foot IK on terrain slopes, lean into a run, breathing | none | none **Nov 14, ART-08 audit (builder):** foot IK: **not cheap, listed.** The pilot's body runs on the native `UTDLoopBlendAnimInstance` (C++ proxy, no animation blueprint), so foot IK means ground traces per foot and a two-bone solve in the proxy (T5's file): about half a day with testing. Not started. | C | The terrain is now mountains: foot IK matters |
| First-person arms: swing set, block, deflect | code-posed blade | placeholder | B + C | Arms-only versions matched to the third-person timing |

### 5.2 Null Armada crew (skeleton `SKEL_NullCrew`; retarget `RTG_Manny_to_NullCrew`; template mannequin set today)
| Move | Now | State | Route | Owner / next |
|---|---|---|---|---|
| Trooper: idle, walk, run, aim, fire, reload, take cover, hit react, death x2 | template mannequin set, retargeted **Nov 13 night (builder):** Grok Bot's set imported and normalised (root scale 100 and metre offsets removed): `A_Crew_Trooper_Idle_Ready`, `_Idle_Aim`, `_Alert`, `_Fire_Burst` (kicks 0.40 / 0.57 / 0.77 s), `_Reload_Rifle`, `_Cover_Enter`, `_Cover_Idle`, `_HitReact_Front`, `_HitReact_Back`, `_Death_Front/_Back/_Left/_Right`. In-place ground deaths for every type: `A_Crew_Death_Front/_Back/_Left/_Right` (2.57 s; Front and Right tipped so hips and legs lie on the floor). Today's `MM_Death_*` end in a standing crouch (captured on a real kill). | made, captured; no slots (T5: per-type lookup, death code) **Nov 14 (builder):** deaths: `A_Crew_Trooper_Death_Front/_Back/_Left/_Right` are now the in-place settled falls (Grok's root-motion originals kept as `A_Crew_Trooper_DeathRM_*`); real kills end at the capsule (1-4 cm), pelvis 20-23 cm. T5 hooked `Idle_Aim`, `Fire_Burst` (0.40 s), `HitReact_Front/_Back`; `Reload_Rifle`, `Cover_*`, `Alert` have no AI state. | made, captured on real kills; deaths and fire hooked (T5) | T5: slots; the Trooper mesh has no rifle attached |
| Grunt: idle, charge, melee swing, hit react, death | template set **Nov 13 night (builder):** `A_Crew_Grunt_Melee`, `_Melee_02`, `_Melee_03`, `_HitReact_01`, `_Death_01` (in-place copies); `A_Crew_Grunt_Charge` (0.97 s) authored on the jog: 26 deg lean, dropped lead shoulder, a foot down throughout (step every 0.27 s). Deaths: `A_Crew_Death_*`, all four directions. | made, captured; no slots **Nov 14 (builder):** deaths per type, in place on real kills: `A_Crew_Grunt_Death_Front` (Grok's EnemyDeaths r1, hips tipped down, pelvis 26 cm), `_Back` (Grok's, 19 cm), `_Left/_Right` (the Trooper's falls: Grok's Grunt sides ended as a curled lump). Grok's root-motion set kept as `A_Crew_Grunt_DeathRM_*`. Charge: ours stays (Grok's `A_Crew_Grunt_Charge_Grok` reads as a shuffle). **Nov 14, round 3 (builder):** contact times measured for T5: `Melee_02` 0.45 s (left hand), `Melee_03` 0.48 s (a right-leg kick), `Melee` 0.53 s. Left-side death on open ground still reads as a compact shape from the front quarter (review MAJOR, open). **Nov 14, round 4 (builder):** `A_Crew_Grunt_Death_Left` is now Grok's Back fall (the Trooper's Left fall read as a lump). **Nov 14, round 5 (builder):** `td.Crew.LogHits`: damage lands at clip time 0.48-0.57 s on `Melee_02` (contact pose 0.45 s) and 0.49-0.57 s on `Melee_03` (0.48 s): the hit is 0.03-0.12 s after the pose (T5: the Grunt's damage instant varies). | made, captured on real kills; hooked (T5) | Melee set |
| Heavy: idle, slow walk, shield up, shield bash, shielded hit, guard-break stagger, death | template set **Nov 13 night (builder):** `A_Crew_Heavy_ShieldBash` authored (1.20 s, **contact 0.55 s**: guard, twist back, left foot steps, shield face driven forward); `A_Crew_Heavy_Stagger` authored (1.00 s, lean back with a step). Deaths: `A_Crew_Death_Back/_Left/_Right` (avoid Front: the shield stands on edge). | made, captured; no slots **Nov 14 (builder):** live `A_Crew_Heavy_ShieldBash` is Grok's (1.83 s, **contact 0.70 s** = `MeleeWindup`), starting from and returning to the authored guard pose; ours kept as `A_Crew_Heavy_ShieldBash_Authored` (0.55 s). Foot slide while blending into the lunge (about 40 cm left, 20 cm right over the clip). Deaths in place: `A_Crew_Heavy_Death_Front` (Grok's, on its back, seen on a real kill after `TD.Ground.BreakGuard`), `_Back` (Grok's), `_Left` and `_Right` = the Back fall (shield rule). Originals kept as `A_Crew_Heavy_DeathRM_*`. **Nov 14, round 3 (builder):** `A_Crew_Heavy_ShieldBash`: only the upper body blends to the guard pose and the feet step the 41 cm drift back (leg IK, `ue_body_tool.py`): planted slide 3 cm left / 6 cm right (was 41 / 20). `Heavy_Death_Front` tipped 18 deg about the head: no legs above the head on a real kill, pelvis 27 cm. **Nov 14, round 5 (builder):** `td.Crew.LogHits`: bash damage at clip time 0.71-0.75 s against the 0.70 s contact pose (five hits). **Nov 14, round 6 (builder):** bash r2: the torso is raised 28 deg at the hit (0.62-0.82 s) so the shield leads; it sits at the Heavy's shoulder height, tilted up. Contact still 0.70 s; planted slide 3 cm left / 7 cm right. | made, captured; hooked (T5) | Shield set; the stagger must read (it opens the heavy to damage) |
| Deflected-bolt hit (their own shot returned) | generic hit react | placeholder | C | A bigger additive hit |
| Drone: hover bob, strafe tilt, eye track, fire recoil, death tumble | code hover, physics death | placeholder | C (+ B for shell parts) | Tilt into movement, eye look-at, thruster flicker |
| **Warden (boss)**: idle, walk, ground slam, heavy weapon fire, summon, shield block, stagger, phase-change roar, death | `SK_Warden` (Grok Bot's Rodin model, 260 cm) on `SKEL_Warden`, which lists `SKEL_NullCrew` as compatible, so it plays the crew's clips as they are: rifle idle / walk / jog, `MM_ChargedAttack` for the slam and the summon, `MM_Rifle_Fire` for the fan, `MM_HitReact_Front_Lgt_01`, `MM_Death_*`. Shield rigid on the chest, gun arm rigid on the right forearm **Nov 13 night (builder):** `A_Crew_Warden_Walk`, `_Guard` (chest turned 45 deg so the chest-mounted shield covers the front; off hand lowered, gun level), `_GuardRaise` (0.87 s into the loop), `_Slam` authored (1.83 s, overhead wind-up 0.30-0.95 s, **contact 1.10 s**), `_Stagger`, `_Death`. Grok Bot's `A_Crew_Warden_Fire_01/_02` and `_Summon_01/_02` imported on `SKEL_Warden` and rescaled to the skeleton's reference lengths (release 0.50 s; the gun is level and on target at exactly 0.50 s in both fire clips; Fire_01 holds the aim from 0.30 s). Death: `A_Crew_Death_Back` for every direction (the slab stands up on the others). `PA_Warden` fixed: its chest body had been fitted round the shield (a 220 x 136 x 173 cm capsule) and bounced the ragdoll to 259 cm without settling; cut to the torso (32 cm radius, 70 cm), depenetration capped: from the standing pose it now settles at pelvis 33 cm in 2 s with no launch (one run in game). | made, captured; no slots **Nov 14 (builder):** live `A_Crew_Warden_Slam` (2.03 s, **contact 1.30 s** = `ChargeWindup` 0.8 + `ChargeDuration` 0.5): Grok's two-handed wind-up into the authored impact and recovery, ends in a crouch and rises; ours kept as `A_Crew_Warden_Slam_Authored` (1.10 s), Grok's as `A_Crew_Warden_Slam_Grok`. Deaths on `SKEL_Warden`, in place, 2.80 s: all four sides are Grok's Back fall (face down; his other sides leave the shield standing or propped), originals kept as `A_Crew_Warden_DeathRM_*`. Ragdoll (PA_Warden) from mid-slam, mid-fire and guard-raised poses with the kill impulse: settles at pelvis 33-37 cm within 2.5 s, peak at most 12 cm above the start, no launch. **Nov 14, round 3 (builder):** death: knee dip (14 cm, feet held by IK), 22 deg torso twist, shield-side arm drop and a landing bounce added; review: the first 0.6 s still reads as a held stagger, and the chest-mounted shield reads as a separate panel in every standing pose (it is skinned to `spine_05` with a gap to the body: a mesh / rig fix, not an animation one). Slam: off hand keeps the guard pose behind the shield; the shield still hangs clear of the body in the wind-up for the same reason. **Nov 14, round 4 (builder):** **rig change:** the shield is rigid on `lowerarm_l` (was `spine_05`; old mesh kept as `SK_Warden_ShieldSpine`), so the left arm carries it. Guard and Walk hold it upright in front of the body; `Fire_01/_02`, `Fire`, `Summon_01/_02` and `Slam` keep the guard left arm (the shield swung out behind or sideways with the old arm motion); the deaths start with an eased-in progressive buckle (knees by 0.3 s, hips by 0.58 s, torso folded 20 deg by 0.84 s, feet held by IK to 1.3 s) and the shield arm falls with the body. **Nov 14, round 5 (builder):** deaths rebuilt on the Trooper's Back fall (2.77 s): frame 0 is the Guard pose, hips dip 16 cm by 0.38 s, torso folds, the shield arm holds guard to 0.7 s, the leg kick is capped (feet at most 10 cm above the hips); no flip. Review: the drop still reads late (standing to 0.8 s, down by 1.4 s) and the shield rests on edge at the head. `Summon_01` re-authored on the Guard pose (shield planted in front, gun arm raised by 0.5 s). Guard, Walk, Stagger, GuardRaise and Slam were on `SKEL_NullCrew` and so were never picked by the per-type lookup; they are now on `SKEL_Warden`. `td.Crew.LogHits`: slam damage at clip time 1.303 s (contact 1.30 s). **Nov 14, round 6 (builder):** death: the 0.4-0.8 s lean plays in 0.2 s, so the drop is continuous (pelvis 93, 86, 78, 69, 61, 49, 35, 27, 22 cm every 0.2 s). Live after the skeleton fix: Guard holds the shield toward the player; the summon shows the gun arm up and a Grunt appearing; an 80-damage `TD.Ground.CrewHit` shows no visible stagger. | made, captured; hooked (T5; slam contact to be set to 1.30 s) | Its own set: shield idle and walk (then the shield goes back to the hand), slam, fire from the gun arm, summon, stagger, roar. Rodin's 7 library clips (`ArtSource/EngAI/Characters/Warden/mesh_r1/A_Warden_*.fbx`) are not used: they are on the Rodin skeleton, and the game reads crew clips by name. T5: the Warden's capsule (70 / 130) is commented out by a stray `//` in `TDCrewTypes.cpp`, so hits land on a 40 / 90 capsule inside a 260 cm body |

### 5.3 Ships and space (no skeletons; code, curves and effects)
| Movement | Now | State | Route | Owner / next |
|---|---|---|---|---|
| Player ships: bank, pitch, drift lean, boost surge, dodge roll | flight model + camera | in game | C | T1: compare the three ships' feel; add control-surface / thruster motion if the Rodin ships have movable parts |
| Ship switch, hyperdrive wind-up and jump, portal entry | camera and effects | in game | C | T1 / T3 with the effects builder |
| **Atmospheric entry (ENG-38)**: ship attitude into the burn (nose-up angle of attack), buffet, control-surface or airbrake deployment, heat-shield plates if the Rodin ships have them; wingmen burning in alongside | level change behind a short entry | none | B + C | T1 (+ Grok Bot for deployable parts, effects builder for the plasma) |
| Landing and take-off: gear, settle, lift | scripted path | placeholder **Nov 14, ART-08 audit (builder):** landing gear: **not possible with today's meshes.** `SM_Rift`, `SM_Lancer` and `SM_Bastion` are single static meshes (hull / glow / parts sections; only the canopies and Rift's portal disc are separate meshes), with no gear geometry, sockets or bones. A gear state needs gear meshes modelled and attached per ship (Blender, route B) and a hook on `ETDLandingStep::GearDown / GearUp`. | B + C | Landing gear and a settle bounce once the Rodin ships are in |
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
