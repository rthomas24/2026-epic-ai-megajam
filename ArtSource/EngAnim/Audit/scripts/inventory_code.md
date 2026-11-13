## 2. How the code picks and plays animations, and the timing each move must hit

### 2.1 The player (shared by pilot and crew): `UTDLoopBlendAnimInstance` (`Ground/TDPilotAnim.h/.cpp`)

There is no animation blueprint, no state machine, no blend space, no montage and no notify anywhere. One small C++ anim instance does everything:

- **One loop** (`SetLoop(clip, rate)`): cross-fades to a new loop in 0.2 s and keeps the stride phase.
- **One one-shot at a time** (`PlayOneShot(clip, rate, bUpperBodyOnly, bHold)`): fades in over 0.1 s, plays to the end, fades out over the last 0.1 s. A new shot replaces the running one with **no cross-fade between the two** (the pose jumps). **Additive sequences are refused** (`IsValidAdditive()` returns early).
- **One upper-body loop** (`SetUpperLoop`): `spine_01` and its children, 0.12 s fade (used for the block). The block is therefore already an upper-body layer: the pilot can walk while blocking.
- **Always in place**: the root bone's X and Y are zeroed at evaluation, so root motion in a clip never moves the body.
- A procedural fallback pose (C++ only) when a crew look has no usable clips.
- Not there: additive layering, aim offsets, foot IK, turn in place, start / stop, lean, per-direction movement (strafe and backpedal play the forward clip), events on a frame of a clip.

### 2.2 Pilot (`Ground/TDFirstPersonCharacter.cpp`, `Ground/TDEnergyBladeComponent.cpp`)

Loops come from `DA_Pilot_Visuals` (Idle, Walk, Run slots; `AnimBPClass` is empty, and a non-empty one would bypass all of the below). Every other clip is loaded **by name** from `/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_<Name>` (`LoadPilotAnim`), so a new clip is used only when C++ names it.

| Move | What the code does | Timing the code needs | Clip, as played | Mismatch |
|---|---|---|---|---|
| Idle | speed < 60 cm/s -> Idle slot | none | `A_Pilot_Idle3`, 9.97 s loop | none. One idle only (Idle02 and Idle6 are unused) |
| Walk | 60-520 cm/s -> Walk slot, rate = speed / `BodyWalkSpeed` (300), clamped 0.6-1.5 | the feet must cover the ground speed | `A_Pilot_Walk`: its feet travel **137 cm/s** at rate 1 | **Foot slide x2.2**: at 300 cm/s the clip plays at rate 1.0 (feet do 137). The walk loop is in fact only seen while blocking (325 cm/s) and for a few frames while accelerating |
| Run / sprint | >= 520 cm/s -> Run slot, rate = speed / `BodyRunSpeed` (650), clamped 0.6-1.5. Run is 650, sprint 950 cm/s | same | `A_Pilot_Run`: its feet travel **490 cm/s** at rate 1 | **Foot slide x1.33** at both run (rate 1.0) and sprint (rate 1.46, feet do 716) |
| Strafe, backpedal, turn | the same forward loop in every direction | | | Legs run forwards while the body moves sideways or backwards (first-person-first design; visible in third person) |
| Jump | nothing at take-off; in the air `A_Pilot_FallLoop` (rate 1) | jump is 650 cm/s up, double jump 500 | `A_Pilot_JumpStart` and `A_Pilot_Land` exist but **no code plays them on a jump or a landing** (JumpStart is used for the mantle) | No take-off, no landing recovery |
| Mantle | `A_Pilot_JumpStart` x1.4 one-shot | `MantleDuration` 0.35 s | 0.867 s / 1.4 = 0.62 s | Plays 0.27 s longer than the mantle |
| Dash | `A_Pilot_Dash` x1.8 one-shot, full body | `DashDuration` **0.18 s**, 1200 cm, exits at 950 cm/s | 0.967 s / 1.8 = **0.54 s**; the clip is a crouch, a 40 cm leap and a landing | The leap is seen **after** the dash has ended (capture: `Pilot_audit.png`, dash row) |
| Slide | `A_Pilot_Dash` x0.8 | `SlideDuration` 0.7 s | 1.21 s | No slide clip; the dash leap is reused |
| Light 1 | `StartSwing`: the damage arc (`HitArc`) is applied **on the frame the button is pressed**. Lockout `Duration` 0.28 s, then a 0.45 s combo window; a press during the lockout is queued and fires as it ends | hit at 0.00 s; next swing can start at 0.28 s | `A_Pilot_Light1` x1.6: the hand is fastest at 0.38 s of the clip = **0.24 s** real, full reach at 0.50 s = **0.31 s** real; whole clip 0.63 s | The enemy is hit (damage, hit-stop, sound) 0.24-0.31 s **before** the blade gets there, and a queued Light 2 cuts the clip off at 0.28 s, just as it reaches |
| Light 2 | as Light 1 (0.28 s) | hit at 0.00 s | `A_Pilot_Light2` x1.6: fastest 0.24 s, reach 0.29 s | same |
| Light 3 | lockout 0.36 s, knockback 900 | hit at 0.00 s | `A_Pilot_Light3` x1.5: fastest 0.23 s, reach 0.31 s; whole clip 0.73 s | same; ends the combo, so its follow-through is seen |
| Heavy lunge | attack held 0.35 s (`HeavyHoldSeconds`): the press first fires a light swing, then `HeavyLunge`: a 0.2 s, 600 cm forced move; the stab is resolved **at the start** | wind-up during the 0.35 s hold, hit during the 0.2 s lunge | `A_Pilot_Heavy` x1.4 started **when the lunge starts**: 0.93 s of wind-up, fastest at 1.08 s = **0.77 s** real, reach 0.86 s; whole clip 1.31 s | The pilot lunges standing upright, then winds up and swings at the air 0.6 s after the lunge has ended. Nothing plays during the hold |
| Dash strike | 0.2 s move; `A_Pilot_Heavy` x1.4 | hit inside 0.2 s | as heavy | as heavy |
| Air slam | `A_Pilot_Light3` x1.2 at the start; dives at 3000 cm/s; resolves on landing (300 cm radius) | a held pose while falling, the hit **on landing** | 0.92 s swing started in the air | The swing is over (or half-way) before the landing; nothing plays on impact |
| Block | upper-body loop `A_Pilot_Block` (0.12 s fade) while `Blocking`; move speed x0.5; 3 s of stamina | guard up within the perfect window (0.2 s) | 1.0 s static pose | Fine as a hold. Reads as "blade upright in front" from behind |
| Deflect / perfect deflect | no animation call at all (effects, slow motion 0.15 s at 0.25) | a flick on each deflected shot; a flourish inside the 0.15 s slow motion | none | Missing |
| Guard broken | `A_Pilot_HitReact_Front`, upper body, 1.2 s state | 1.2 s stagger | refused (additive) | **Never plays** |
| Hit react | `A_Pilot_HitReact_Front` / `_Back` x1.2, upper body, on hull damage when not attacking | short flinch | refused (additive) | **Never plays** (capture: health 100 -> 70 with no flinch) |
| Death | ragdoll (`SetSimulatePhysics`), then "CUT!" | | `A_Pilot_Death_Front` / `_Back` unused | By design; the clips are dead weight unless a death is played before the ragdoll |
| Interact (hold E), board / exit ship | no animation | hold times per interactable | none | Missing |
| First-person blade | `UpdateBladePose`: the blade mesh is moved by code along one arc per state, eased by `GetMoveProgress01()` (0-1 over the lockout): Light 1 flat cut right to left, Light 2 backhand left to right, **Light 3 overhead chop**, heavy = thrust, air slam = raised, block = blade across the lower view. No arms | matches the lockout exactly (it is driven by it) | no clip | First person and third person **disagree**: third-person Light 3 is a horizontal hook, first-person Light 3 is an overhead chop; the first-person heavy is a thrust, the third-person heavy is a wide hook |

### 2.3 Crew (`Ground/Crew/TDCrewCharacter.cpp`)

`DA_Crew_*_Visuals` (`UTDEnemyVisuals`) has a mesh and an `AnimBPClass` (empty) but **no clip slots**. With no AnimBP the crew use the same C++ player, and every clip is found **by its template name** in `/Game/TriadDrift/Gameplay/Characters/Crew/Anims/` ("the template mannequin set": `FallbackLoopFor`, `TDCrewAnims::*`, `CrewAnimPath`). So the `A_Crew_*` clips made today are not picked up until T5 names them.

| Move | What the code does | Timing the code needs | Clip, as played | Mismatch |
|---|---|---|---|---|
| Idle | speed < 40: grunt `MM_Idle`; trooper, heavy, warden `MF_Rifle_Idle_ADS` | | 7.6 s / 8.0 s loops | The heavy and the warden aim an invisible rifle |
| Walk | 40-300 cm/s, rate = speed / **200** clamped 0.6-1.5 | feet = ground speed | `MF_Unarmed_Walk_Fwd` / `MF_Rifle_Walk_Fwd`: feet travel **285 / 280 cm/s** at rate 1 (the source root motion says 300) | **Feet too fast x1.4** (reference should be about 285) |
| Run | >= 300, rate = speed / **450**. Speeds: grunt 560, trooper 480, heavy 330, warden 380 | same | `MF_Unarmed_Jog_Fwd` / `MF_Rifle_Jog_Fwd`: feet travel **575 / 600 cm/s** at rate 1 | **Feet too fast x1.3** (reference should be about 600). The heavy at 330 "jogs" at rate 0.73 instead of walking |
| Strafe, back away | forward clip in every direction | | | The trooper backs away from the player running forwards on the spot (capture: `Trooper_audit.png`) |
| Melee (grunt, heavy) | `MM_Attack_01` rate 1 **at the start of the wind-up**; damage when `MeleeWindup` ends: grunt **0.60 s**, heavy **0.70 s** | fist at full reach at 0.60 / 0.70 s | fastest 0.38 s, full reach **0.47 s**, clip 1.0 s | The punch lands **0.13 s (grunt) / 0.23 s (heavy) before the damage**, i.e. the player is hurt after the fist is already going back. The telegraph is the wind-up, so this also makes the parry timing unreadable |
| Charge slam / summon (warden) | `MM_ChargedAttack` at the start; `ChargeWindup` 0.8 s then a 0.5 s charge | hit at about 1.3 s | fastest 1.08 s, reach 1.20 s | Within 0.1 s: acceptable |
| Burst (trooper 3 x 0.18 s after 0.4 s), shotgun, bolt fan | `MM_Rifle_Fire` (0.53 s, one recoil) once, at the start of the wind-up | one kick per shot, on the shot | the single kick plays during the wind-up, before any shot | No recoil on the shots themselves |
| Hit react | `MM_HitReact_Front_Lgt_01` (0.7 s, full pose, rifle grip) when not attacking; **grunt: none** | | one clip, front only | No variety, no direction, none for the grunt, replaces the whole pose (arms snap to a rifle grip on the heavy) |
| Stagger, guard broken (heavy 3 s) | nothing | a held stagger for 1.2 s / 3 s | none | Missing: the guard break is the heavy's weak point and has no pose |
| Shield up (heavy) | nothing (rifle idle) | | none | Missing |
| Death | `PlayAnimation` (single node) of `MM_Death_Front/Back/Left/Right_01` by hit direction, holds the last frame | | 0.97-1.1 s, the body travels 1.3-2.2 m from its feet | Works; one clip per direction (two more fronts added today) |
| Cover (`TDCrewCover`), reload, alert | nothing | | none | Missing |
| Drone | no skeleton: code hover and physics death | | | Route C only |
