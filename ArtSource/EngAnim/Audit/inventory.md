# Animation inventory (audit, Nov 13)

Made by the animation builder on `lead/int` (1050b21 + today's new clips). Numbers come from the headless scripts in `ArtSource/EngAnim/Audit/scripts/`
(`ue_i00_inventory.py`, `ue_i10_metrics.py`, `ue_i20_visuals.py`) and from reading `Source/unrealGameJam/TriadDrift/Ground/**`. All clips are 30 fps.

## Headline numbers

- Before today: **37 animation sequences** of ours (23 pilot, 14 crew), 3 retargeters, no animation blueprint, no blend space, no montage, no notify on any clip.
- Template content that can be retargeted: **97 sequences** on `SK_Mannequin` (`/Game/Characters/Mannequins/Anims/**`): unarmed 8-way walk and jog, rifle 8-way walk and jog, pistol 8-way walk and jog, 4 unarmed attacks, dash, jumps, 6 deaths, 8 hit reacts, reloads, equips, aim-offset poses. 22 of them were already in use; **70 more were retargeted today**.
- `Content/Variant_Shooter`, `Variant_Horror`, `FirstPerson` hold **no sequences of their own**: only animation blueprints (`ABP_FP_Pistol`, `ABP_FP_Weapon`, `ABP_TP_Pistol`, `ABP_TP_Rifle`, `ABP_FP_Copy`) and one montage (`FP_Rifle_Shoot_Montage`) that play the mannequin clips above. `Content/Weapons` has three weapon meshes with skeletons and no clips.
- Engine content: nothing useful for a humanoid was found referenced by the project (the engine ships only tutorial and test sequences; not checked plugin by plugin).
- Nicholas's Sentinel sample GLB: **6 clips, all 6 already imported** (see section 3).
- Added today: **93 retargeted clips** (+ 11 additive copies) and **12 derived clips** (re-timed or full-pose copies). Listed in section 5.

## 1. What existed before today

| Clip | Folder | Length (s) | Frames | Additive | Root travel left in the clip (cm) | How the code uses it |
|---|---|---|---|---|---|---|
| `MF_Rifle_Idle_ADS` | `Crew/Anims` | 8.000 | 240 | no | 0 | trooper / heavy / warden idle loop |
| `MF_Rifle_Jog_Fwd` | `Crew/Anims` | 1.533 | 46 | no | 0 | trooper / heavy loop >= 300, rate speed/450 |
| `MF_Rifle_Walk_Fwd` | `Crew/Anims` | 1.533 | 46 | no | 0 | trooper / heavy loop 40-300, rate speed/200 |
| `MF_Unarmed_Jog_Fwd` | `Crew/Anims` | 1.767 | 53 | no | 0 | grunt loop >= 300, rate speed/450 |
| `MF_Unarmed_Walk_Fwd` | `Crew/Anims` | 1.500 | 45 | no | 0 | grunt loop 40-300 cm/s, rate speed/200 |
| `MM_Attack_01` | `Crew/Anims` | 1.000 | 30 | no | 151 | one-shot at the START of a melee wind-up (grunt 0.6 s, heavy 0.7 s) |
| `MM_ChargedAttack` | `Crew/Anims` | 1.833 | 55 | no | 150 | one-shot at the start of ChargeSlam / Summon (warden) |
| `MM_Death_Back_01` | `Crew/Anims` | 1.100 | 33 | no | 195 | same |
| `MM_Death_Front_01` | `Crew/Anims` | 1.100 | 33 | no | 127 | PlayAnimation on death, by hit direction |
| `MM_Death_Left_01` | `Crew/Anims` | 0.967 | 29 | no | 203 | same |
| `MM_Death_Right_01` | `Crew/Anims` | 1.100 | 33 | no | 218 | same |
| `MM_HitReact_Front_Lgt_01` | `Crew/Anims` | 0.700 | 21 | no | 0 | one-shot on hull damage (not grunt, not while attacking) |
| `MM_Idle` | `Crew/Anims` | 7.567 | 227 | no | 0 | grunt idle loop (speed < 40) |
| `MM_Rifle_Fire` | `Crew/Anims` | 0.533 | 16 | no | 0 | one-shot at the start of a burst / shotgun / fan wind-up (once per attack, not per shot) |
| `A_Pilot_CasualWalk` | `Pilot` | 4.167 | 125 | no | 0 | unused |
| `A_Pilot_Idle02` | `Pilot` | 1.867 | 56 | no | 0 | unused |
| `A_Pilot_Idle3` | `Pilot` | 9.967 | 299 | no | 0 | DA_Pilot_Visuals.Idle (speed < 60) |
| `A_Pilot_Idle6` | `Pilot` | 7.367 | 221 | no | 0 | unused |
| `A_Pilot_Run` | `Pilot` | 0.633 | 19 | no | 0 | DA_Pilot_Visuals.Run (>= 520 cm/s, rate speed/650 clamped 0.6-1.5) |
| `A_Pilot_Walk` | `Pilot` | 1.033 | 31 | no | 0 | DA_Pilot_Visuals.Walk (60-520 cm/s, rate speed/300 clamped 0.6-1.5) |
| `A_Pilot_Alt_LeftHook` | `Pilot/Anims` | 1.000 | 30 | no | 0 | unused |
| `A_Pilot_Alt_SpinKick` | `Pilot/Anims` | 1.667 | 50 | no | 0 | unused |
| `A_Pilot_Block` | `Pilot/Anims` | 1.000 | 30 | no | 0 | upper-body loop (spine_01 down) while Blocking, 0.12 s fade |
| `A_Pilot_Dash` | `Pilot/Anims` | 0.967 | 29 | no | 0 | one-shot x1.8 on dash, x0.8 on slide |
| `A_Pilot_Death_Back` | `Pilot/Anims` | 1.100 | 33 | no | 0 | unused (death is a ragdoll) |
| `A_Pilot_Death_Front` | `Pilot/Anims` | 1.100 | 33 | no | 0 | unused (death is a ragdoll) |
| `A_Pilot_FallLoop` | `Pilot/Anims` | 3.000 | 90 | no | 0 | loop while falling (rate 1) |
| `A_Pilot_Heavy` | `Pilot/Anims` | 1.833 | 55 | no | 0 | one-shot x1.4 on HeavyLunge and DashStrike |
| `A_Pilot_HitReact_Back` | `Pilot/Anims` | 0.867 | 26 | LOCAL_SPACE_BASE | 0 | asked for on damage from behind, REFUSED (additive): never plays |
| `A_Pilot_HitReact_Back_Add` | `Pilot/Anims` | 0.867 | 26 | LOCAL_SPACE_BASE | 0 | unused (needs an additive layer in code) |
| `A_Pilot_HitReact_Front` | `Pilot/Anims` | 0.767 | 23 | LOCAL_SPACE_BASE | 0 | asked for on damage and GuardBroken, but REFUSED by PlayOneShot (additive): never plays |
| `A_Pilot_HitReact_Front_Add` | `Pilot/Anims` | 0.767 | 23 | LOCAL_SPACE_BASE | 0 | unused (needs an additive layer in code) |
| `A_Pilot_JumpStart` | `Pilot/Anims` | 0.867 | 26 | no | 0 | one-shot x1.4 on MANTLE only (a jump plays nothing) |
| `A_Pilot_Land` | `Pilot/Anims` | 0.867 | 26 | no | 0 | unused (no code plays it) |
| `A_Pilot_Light1` | `Pilot/Anims` | 1.000 | 30 | no | 0 | one-shot x1.6 on Light1 (upper body only when moving > 150 cm/s) |
| `A_Pilot_Light2` | `Pilot/Anims` | 1.000 | 30 | no | 0 | one-shot x1.6 on Light2 |
| `A_Pilot_Light3` | `Pilot/Anims` | 1.100 | 33 | no | 0 | one-shot x1.5 on Light3; x1.2 on AirSlam |

Retargeters and rigs: `Pilot/Anims/RTG_Manny_to_Pilot` (`IK_Pilot_SrcManny` -> `IK_Pilot`), `Crew/RTG_Manny_to_NullCrew` (`Crew/Anims/IK_Crew_Manny` -> `Crew/IK_NullCrew`), `Crew/Anims/RTG_Crew_Manny_Identity` (no longer needed).
Skeletons: `SKEL_Pilot` (75 bones, Meshy rig with mannequin names, no twist bones), `SKEL_NullCrew` (shared by `SK_NullTrooper`, `SK_NullGrunt`, `SK_NullHeavy`; the drone is a static mesh).
Visuals assets: `DA_Pilot_Visuals` has Idle = `A_Pilot_Idle3`, Walk = `A_Pilot_Walk`, Run = `A_Pilot_Run`, no AnimBPClass. `DA_Crew_Trooper/Grunt/Heavy_Visuals` have a mesh and no AnimBPClass; `UTDEnemyVisuals` has **no clip slots at all**.

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

## 3. Nicholas's Sentinel Armor sample GLB (read only)

`ArtSource/Characters/Samples/SentinelArmor/Meshy_AI_Sentinel_Armor_Turnaround_20261113_animated_ue5_mannequin_All_animation.glb`: 1 skin, 74 joints, 6 animations, all in place. Every one was imported on Nov 12 (`ArtSource/EngCharacters/Pilot/SOURCE.md`). **Nothing is left to import from it.**

| GLB clip | Length (s) | Keys | Imported as | Used |
|---|---|---|---|---|
| Running | 0.633 | 20 | `A_Pilot_Run` | Run slot |
| Walking | 1.033 | 32 | `A_Pilot_Walk` | Walk slot |
| Casual_Walk | 4.167 | 126 | `A_Pilot_CasualWalk` | no (6 cm pop at the loop point) |
| Idle_02 | 1.867 | 57 | `A_Pilot_Idle02` | no (guard stance; a blade-ready idle candidate) |
| Idle_3 | 9.967 | 300 | `A_Pilot_Idle3` | Idle slot |
| Idle_6 | 7.367 | 222 | `A_Pilot_Idle6` | no (idle variant candidate) |

## 4. Template content available for retargeting (`/Game/Characters/Mannequins/Anims/**`, skeleton `SK_Mannequin`)

Root travel is what the clip moves the root bone (root-motion clips); our players are in place, so it is removed on retarget. "Now" = the asset of ours made from it (bold = made today).

| Template clip | Length (s) | Frames | Additive | Root travel (cm) | Ledger row it can fill | Now |
|---|---|---|---|---|---|---|
| `Death/MM_Death_Back_01` | 1.100 | 33 | no | 195 | 5.1 / 5.2 death variety | `Crew/Anims/MM_Death_Back_01`, `A_Pilot_Death_Back` |
| `Death/MM_Death_Front_01` | 1.100 | 33 | no | 127 | 5.1 / 5.2 death variety | `Crew/Anims/MM_Death_Front_01`, `A_Pilot_Death_Front` |
| `Death/MM_Death_Front_02` | 1.133 | 34 | no | 190 | 5.1 / 5.2 death variety | **`A_Crew_Death_Front_02`**, **`A_Pilot_Death_Front_02`** |
| `Death/MM_Death_Front_03` | 0.933 | 28 | no | 167 | 5.1 / 5.2 death variety | **`A_Crew_Death_Front_03`**, **`A_Pilot_Death_Front_03`** |
| `Death/MM_Death_Left_01` | 0.967 | 29 | no | 203 | 5.1 / 5.2 death variety | `Crew/Anims/MM_Death_Left_01`, **`A_Pilot_Death_Left`** |
| `Death/MM_Death_Right_01` | 1.100 | 33 | no | 218 | 5.1 / 5.2 death variety | `Crew/Anims/MM_Death_Right_01`, **`A_Pilot_Death_Right`** |
| `Pistol/Aim/MF_Pistol_Idle_ADS1` | 9.033 | 271 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 heavy aim offset | not used |
| `Pistol/Aim/MF_Pistol_Idle_ADS_AO_CD` | 0.067 | 2 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 heavy aim offset | not used |
| `Pistol/Aim/MF_Pistol_Idle_ADS_AO_CU` | 0.067 | 2 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 heavy aim offset | not used |
| `Pistol/Jog/MF_Pistol_Jog_Bwd` | 1.033 | 31 | no | 600 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jog/MF_Pistol_Jog_Bwd_Left` | 0.967 | 29 | no | 578 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jog/MF_Pistol_Jog_Bwd_Right` | 0.967 | 29 | no | 578 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jog/MF_Pistol_Jog_Fwd` | 1.067 | 32 | no | 640 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jog/MF_Pistol_Jog_Fwd_Left` | 1.000 | 30 | no | 619 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jog/MF_Pistol_Jog_Fwd_Right` | 1.000 | 30 | no | 619 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jog/MF_Pistol_Jog_Left` | 1.033 | 31 | no | 600 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jog/MF_Pistol_Jog_Right` | 1.000 | 30 | no | 600 | 5.2 heavy (not needed at 330 cm/s) | not used |
| `Pistol/Jump/MM_Pistol_Jump_Fall_Loop` | 2.000 | 60 | no | 0 | none | not used |
| `Pistol/Jump/MM_Pistol_Jump_RecoveryAdditive` | 0.700 | 21 | LOCAL_SPACE_BASE | 0 | none | not used |
| `Pistol/Jump/MM_Pistol_Jump_Start` | 0.200 | 6 | no | 175 | none | not used |
| `Pistol/MF_Pistol_Idle_ADS` | 9.033 | 271 | no | 0 | 5.2 heavy idle, shield up | **`A_Crew_Heavy_Idle_Guard`** |
| `Pistol/MM_Pistol_DryFire` | 0.800 | 24 | no | 0 | none | not used |
| `Pistol/MM_Pistol_Equip` | 1.400 | 42 | no | 0 | 5.2 heavy shield up; 5.1 pilot blade draw / pickup candidate | **`A_Crew_Heavy_GuardRaise`**, **`A_Pilot_Interact_Draw`** |
| `Pistol/MM_Pistol_Fire` | 0.667 | 20 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 heavy shotgun fire (additive kick) | not used |
| `Pistol/MM_Pistol_Reload` | 2.000 | 60 | no | 0 | 5.2 heavy shotgun reload; 5.1 pilot console interact candidate | **`A_Crew_Heavy_Reload`**, **`A_Pilot_Interact_Console`** |
| `Pistol/Walk/MF_Pistol_Walk_Bwd` | 1.767 | 53 | no | 520 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Bwd`** |
| `Pistol/Walk/MF_Pistol_Walk_Bwd_Left` | 1.767 | 53 | no | 520 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Bwd_Left`** |
| `Pistol/Walk/MF_Pistol_Walk_Bwd_Right` | 1.767 | 53 | no | 524 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Bwd_Right`** |
| `Pistol/Walk/MF_Pistol_Walk_Fwd` | 1.433 | 43 | no | 452 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Fwd`** |
| `Pistol/Walk/MF_Pistol_Walk_Fwd_Left` | 1.433 | 43 | no | 453 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Fwd_Left`** |
| `Pistol/Walk/MF_Pistol_Walk_Fwd_Right` | 1.433 | 43 | no | 453 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Fwd_Right`** |
| `Pistol/Walk/MF_Pistol_Walk_Left` | 1.800 | 54 | no | 520 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Left`** |
| `Pistol/Walk/MF_Pistol_Walk_Right` | 1.867 | 56 | no | 560 | 5.2 heavy slow walk (two hands forward = shield presented) | **`A_Crew_Heavy_Walk_Right`** |
| `Rifle/AIM/MM_Rifle_Idle_ADS_AO_CC` | 0.033 | 1 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 trooper aim (aim offset poses up / centre / down) | **`A_Crew_Trooper_Aim_Center`** |
| `Rifle/AIM/MM_Rifle_Idle_ADS_AO_CD` | 0.033 | 1 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 trooper aim (aim offset poses up / centre / down) | **`A_Crew_Trooper_Aim_Down`** |
| `Rifle/AIM/MM_Rifle_Idle_ADS_AO_CU` | 0.033 | 1 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 trooper aim (aim offset poses up / centre / down) | **`A_Crew_Trooper_Aim_Up`** |
| `Rifle/HitReact/MM_HitReact_Back_Med_01` | 0.867 | 26 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | `A_Pilot_HitReact_Back`, **`A_Crew_Hit_Back_Med_01`** |
| `Rifle/HitReact/MM_HitReact_Front_Hvy_01` | 0.800 | 24 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | **`A_Crew_Hit_Front_Hvy_01`**, **`A_Pilot_HitReact_Front_Hvy`** |
| `Rifle/HitReact/MM_HitReact_Front_Lgt_01` | 0.700 | 21 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | `Crew/Anims/MM_HitReact_Front_Lgt_01`, **`A_Pilot_HitReact_Front_Lgt_01`** |
| `Rifle/HitReact/MM_HitReact_Front_Lgt_02` | 0.800 | 24 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | **`A_Crew_Hit_Front_Lgt_02`**, **`A_Pilot_HitReact_Front_Lgt_02`** |
| `Rifle/HitReact/MM_HitReact_Front_Lgt_03` | 1.000 | 30 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | **`A_Crew_Hit_Front_Lgt_03`**, **`A_Pilot_HitReact_Front_Lgt_03`** |
| `Rifle/HitReact/MM_HitReact_Front_Lgt_04` | 1.233 | 37 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | **`A_Crew_Hit_Front_Lgt_04`** |
| `Rifle/HitReact/MM_HitReact_Front_Med_01` | 0.767 | 23 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | `A_Pilot_HitReact_Front`, **`A_Crew_Hit_Front_Med_01`** |
| `Rifle/HitReact/MM_HitReact_Front_Med_02` | 0.800 | 24 | LOCAL_SPACE_BASE | 0 | 5.2 hit react variety (all crew), 5.1 pilot hit react variety | **`A_Crew_Hit_Front_Med_02`** |
| `Rifle/Jog/MF_Rifle_Jog_Bwd` | 1.800 | 54 | no | 1025 | 5.2 trooper run (8-way strafe set) | **`A_Crew_Trooper_Jog_Bwd`** |
| `Rifle/Jog/MF_Rifle_Jog_Bwd_Left` | 1.800 | 54 | no | 1024 | 5.2 trooper run (8-way strafe set) | **`A_Crew_Trooper_Jog_Bwd_Left`** |
| `Rifle/Jog/MF_Rifle_Jog_Bwd_Right` | 1.800 | 54 | no | 1024 | 5.2 trooper run (8-way strafe set) | **`A_Crew_Trooper_Jog_Bwd_Right`** |
| `Rifle/Jog/MF_Rifle_Jog_Fwd` | 1.533 | 46 | no | 934 | 5.2 trooper run (8-way strafe set) | `Crew/Anims/MF_Rifle_Jog_Fwd` |
| `Rifle/Jog/MF_Rifle_Jog_Fwd_Left` | 1.533 | 46 | no | 934 | 5.2 trooper run (8-way strafe set) | **`A_Crew_Trooper_Jog_Fwd_Left`** |
| `Rifle/Jog/MF_Rifle_Jog_Fwd_Right` | 1.533 | 46 | no | 1868 | 5.2 trooper run (8-way strafe set) | **`A_Crew_Trooper_Jog_Fwd_Right`** |
| `Rifle/Jog/MF_Rifle_Jog_Left` | 0.467 | 14 | no | 276 | 5.2 trooper run (8-way strafe set) | **`A_Crew_Trooper_Jog_Left`** |
| `Rifle/Jog/MF_Rifle_Jog_Right` | 1.067 | 32 | no | 640 | 5.2 trooper run (8-way strafe set) | **`A_Crew_Trooper_Jog_Right`** |
| `Rifle/Jump/MM_Rifle_Jump_Apex` | 0.667 | 20 | no | 190 | 5.2 trooper (jump / drop; not in the ledger) | **`A_Crew_Trooper_JumpApex`** |
| `Rifle/Jump/MM_Rifle_Jump_Fall_Land` | 0.400 | 12 | no | 232 | 5.2 trooper (jump / drop; not in the ledger) | **`A_Crew_Trooper_FallLand`**, **`A_Pilot_Land_Hard`** |
| `Rifle/Jump/MM_Rifle_Jump_Fall_Loop` | 3.000 | 90 | no | 1587 | 5.2 trooper (jump / drop; not in the ledger) | not used |
| `Rifle/Jump/MM_Rifle_Jump_RecoveryAdditive` | 0.933 | 28 | LOCAL_SPACE_BASE | 62 | 5.2 trooper (jump / drop; not in the ledger) | not used |
| `Rifle/Jump/MM_Rifle_Jump_Start` | 0.233 | 7 | no | 104 | 5.2 trooper (jump / drop; not in the ledger) | **`A_Crew_Trooper_JumpStart`** |
| `Rifle/Jump/MM_Rifle_Jump_Start_Loop` | 2.000 | 60 | no | 860 | 5.2 trooper (jump / drop; not in the ledger) | not used |
| `Rifle/MF_Rifle_Idle_ADS` | 8.000 | 240 | no | 0 | 5.2 trooper idle / aim | `Crew/Anims/MF_Rifle_Idle_ADS` |
| `Rifle/MM_Rifle_DryFire` | 0.800 | 24 | no | 0 | 5.2 trooper (jam / empty) | **`A_Crew_Trooper_DryFire`** |
| `Rifle/MM_Rifle_Equip` | 1.667 | 50 | no | 0 | 5.2 trooper (weapon raise on alert); 5.1 pilot interact candidate | **`A_Crew_Trooper_Equip`**, **`A_Pilot_Interact_Shoulder`** |
| `Rifle/MM_Rifle_Fire` | 0.533 | 16 | ROTATION_OFFSET_MESH_SPACE | 0 | 5.2 trooper fire | `Crew/Anims/MM_Rifle_Fire` |
| `Rifle/MM_Rifle_Reload` | 2.200 | 66 | no | 0 | 5.2 trooper reload | **`A_Crew_Trooper_Reload`** |
| `Rifle/Walk/MF_Rifle_Walk_Bwd` | 1.200 | 36 | no | 360 | 5.2 trooper walk (8-way strafe set) | **`A_Crew_Trooper_Walk_Bwd`** |
| `Rifle/Walk/MF_Rifle_Walk_Bwd_Left` | 1.133 | 34 | no | 350 | 5.2 trooper walk (8-way strafe set) | **`A_Crew_Trooper_Walk_Bwd_Left`** |
| `Rifle/Walk/MF_Rifle_Walk_Bwd_Right` | 1.133 | 34 | no | 350 | 5.2 trooper walk (8-way strafe set) | **`A_Crew_Trooper_Walk_Bwd_Right`** |
| `Rifle/Walk/MF_Rifle_Walk_Fwd` | 1.533 | 46 | no | 456 | 5.2 trooper walk (8-way strafe set) | `Crew/Anims/MF_Rifle_Walk_Fwd` |
| `Rifle/Walk/MF_Rifle_Walk_Fwd_Left` | 1.533 | 46 | no | 456 | 5.2 trooper walk (8-way strafe set) | **`A_Crew_Trooper_Walk_Fwd_Left`** |
| `Rifle/Walk/MF_Rifle_Walk_Fwd_Right` | 1.533 | 46 | no | 456 | 5.2 trooper walk (8-way strafe set) | **`A_Crew_Trooper_Walk_Fwd_Right`** |
| `Rifle/Walk/MF_Rifle_Walk_Left` | 1.233 | 37 | no | 360 | 5.2 trooper walk (8-way strafe set) | **`A_Crew_Trooper_Walk_Left`** |
| `Rifle/Walk/MF_Rifle_Walk_Right` | 1.500 | 45 | no | 450 | 5.2 trooper walk (8-way strafe set) | **`A_Crew_Trooper_Walk_Right`** |
| `Unarmed/Attack/MM_Attack_01` | 1.000 | 30 | no | 151 | 5.2 grunt melee variety; 5.1 pilot (already used) | `Crew/Anims/MM_Attack_01`, `A_Pilot_Light1` |
| `Unarmed/Attack/MM_Attack_02` | 1.000 | 30 | no | 90 | 5.2 grunt melee variety; 5.1 pilot (already used) | `A_Pilot_Alt_LeftHook (mirrored: A_Pilot_Light2)`, **`A_Crew_Grunt_Attack_02`** |
| `Unarmed/Attack/MM_Attack_03` | 1.667 | 50 | no | 216 | 5.2 grunt melee variety; 5.1 pilot (already used) | `A_Pilot_Alt_SpinKick`, **`A_Crew_Grunt_Attack_03`** |
| `Unarmed/Attack/MM_ChargedAttack` | 1.833 | 55 | no | 150 | 5.2 grunt melee variety; 5.1 pilot (already used) | `Crew/Anims/MM_ChargedAttack`, `A_Pilot_Heavy (release: A_Pilot_Light3)` |
| `Unarmed/Jog/MF_Unarmed_Jog_Bwd` | 1.500 | 45 | no | 900 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | **`A_Crew_Grunt_Jog_Bwd`**, **`A_Pilot_Jog_Bwd`** |
| `Unarmed/Jog/MF_Unarmed_Jog_Bwd_Left` | 1.500 | 45 | no | 899 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | **`A_Crew_Grunt_Jog_Bwd_Left`**, **`A_Pilot_Jog_Bwd_Left`** |
| `Unarmed/Jog/MF_Unarmed_Jog_Bwd_Right` | 1.500 | 45 | no | 899 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | **`A_Crew_Grunt_Jog_Bwd_Right`**, **`A_Pilot_Jog_Bwd_Right`** |
| `Unarmed/Jog/MF_Unarmed_Jog_Fwd` | 1.767 | 53 | no | 1039 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | `Crew/Anims/MF_Unarmed_Jog_Fwd`, **`A_Pilot_Jog_Fwd`** |
| `Unarmed/Jog/MF_Unarmed_Jog_Fwd_Left` | 1.767 | 53 | no | 1041 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | **`A_Crew_Grunt_Jog_Fwd_Left`**, **`A_Pilot_Jog_Fwd_Left`** |
| `Unarmed/Jog/MF_Unarmed_Jog_Fwd_Right` | 1.767 | 53 | no | 1041 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | **`A_Crew_Grunt_Jog_Fwd_Right`**, **`A_Pilot_Jog_Fwd_Right`** |
| `Unarmed/Jog/MF_Unarmed_Jog_Left` | 1.533 | 46 | no | 900 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | **`A_Crew_Grunt_Jog_Left`**, **`A_Pilot_Jog_Left`** |
| `Unarmed/Jog/MF_Unarmed_Jog_Right` | 1.733 | 52 | no | 1040 | 5.1 pilot strafe / backpedal (run); 5.2 grunt charge strafes | **`A_Crew_Grunt_Jog_Right`**, **`A_Pilot_Jog_Right`** |
| `Unarmed/Jump/MM_Dash` | 0.967 | 29 | no | 886 | 5.2 grunt charge lunge; 5.1 pilot dash (already used) | `A_Pilot_Dash`, **`A_Crew_Grunt_Lunge`** |
| `Unarmed/Jump/MM_Fall_Loop` | 3.000 | 90 | no | 0 | 5.1 pilot jump (already used); 5.2 grunt | `A_Pilot_FallLoop`, **`A_Crew_Grunt_FallLoop`** |
| `Unarmed/Jump/MM_Jump` | 0.867 | 26 | no | 0 | 5.1 pilot jump (already used); 5.2 grunt | `A_Pilot_JumpStart`, **`A_Crew_Grunt_JumpStart`** |
| `Unarmed/Jump/MM_Land` | 0.867 | 26 | no | 0 | 5.1 pilot jump (already used); 5.2 grunt | `A_Pilot_Land`, **`A_Crew_Grunt_Land`** |
| `Unarmed/Jump/MM_WallJump` | 0.467 | 14 | no | 0 | 5.1 pilot double jump / mantle candidate | **`A_Pilot_WallJump`** |
| `Unarmed/MM_Idle` | 7.567 | 227 | no | 0 | 5.2 grunt idle (already used) | `Crew/Anims/MM_Idle`, **`A_Pilot_Idle_Mannequin`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Bwd` | 2.133 | 64 | no | 603 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | **`A_Crew_Grunt_Walk_Bwd`**, **`A_Pilot_Walk_Bwd`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Bwd_Left` | 2.133 | 64 | no | 602 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | **`A_Crew_Grunt_Walk_Bwd_Left`**, **`A_Pilot_Walk_Bwd_Left`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Bwd_Right` | 2.133 | 64 | no | 602 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | **`A_Crew_Grunt_Walk_Bwd_Right`**, **`A_Pilot_Walk_Bwd_Right`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Fwd` | 1.500 | 45 | no | 450 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | `Crew/Anims/MF_Unarmed_Walk_Fwd`, **`A_Pilot_Walk_Fwd`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Fwd_Left` | 1.500 | 45 | no | 450 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | **`A_Crew_Grunt_Walk_Fwd_Left`**, **`A_Pilot_Walk_Fwd_Left`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Fwd_Right` | 1.500 | 45 | no | 450 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | **`A_Crew_Grunt_Walk_Fwd_Right`**, **`A_Pilot_Walk_Fwd_Right`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Left` | 1.533 | 46 | no | 450 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | **`A_Crew_Grunt_Walk_Left`**, **`A_Pilot_Walk_Left`** |
| `Unarmed/Walk/MF_Unarmed_Walk_Right` | 1.533 | 46 | no | 450 | 5.1 pilot strafe / backpedal (walk); 5.2 grunt walk strafes | **`A_Crew_Grunt_Walk_Right`**, **`A_Pilot_Walk_Right`** |

Odd sources (as shipped by Epic, flagged so nobody trusts them blindly): `Rifle/Jog/MF_Rifle_Jog_Left` is 14 frames (0.467 s, half a cycle; its partner `_Right` is 32 frames); `Rifle/Jog/MF_Rifle_Jog_Fwd_Right` travels twice as far as `_Fwd_Left` in the same time; `Unarmed/Jog/MF_Unarmed_Jog_Left` / `_Right` and the pistol and rifle `Walk_Left` / `_Right` pairs have different lengths (a blend space must sync them by markers or normalised time).
Not retargeted: the pistol jog and jump sets, `MM_Pistol_DryFire`, `MM_Pistol_Fire` (pure additive kick), `Rifle/Jump/MM_Rifle_Jump_Fall_Loop` / `_Start_Loop` / `_RecoveryAdditive`, the single-frame pistol aim-offset poses.

## 5. Added today (all new files; nothing existing was changed)

Retargeted with the existing retargeters by `ArtSource/EngAnim/Retarget/scripts/ue_r10_retarget.py` (root translation zeroed and root motion off except deaths; ACL codec). `_Add` = an additive copy (local space, against its own first frame) saved beside the full-pose clip.

| New clip | Skeleton | From | Length (s) | Frames | Root travel removed (cm) | Additive copy |
|---|---|---|---|---|---|---|
| `A_Crew_Death_Front_02` | SKEL_NullCrew | `Death/MM_Death_Front_02` | 1.133 | 34 | kept (death) |  |
| `A_Crew_Death_Front_03` | SKEL_NullCrew | `Death/MM_Death_Front_03` | 0.933 | 28 | kept (death) |  |
| `A_Crew_Grunt_Attack_02` | SKEL_NullCrew | `Unarmed/Attack/MM_Attack_02` | 1.000 | 30 | 90 |  |
| `A_Crew_Grunt_Attack_03` | SKEL_NullCrew | `Unarmed/Attack/MM_Attack_03` | 1.667 | 50 | 216 |  |
| `A_Crew_Grunt_FallLoop` | SKEL_NullCrew | `Unarmed/Jump/MM_Fall_Loop` | 3.000 | 90 | 0 |  |
| `A_Crew_Grunt_Jog_Bwd` | SKEL_NullCrew | `Unarmed/Jog/MF_Unarmed_Jog_Bwd` | 1.500 | 45 | 900 |  |
| `A_Crew_Grunt_Jog_Bwd_Left` | SKEL_NullCrew | `Unarmed/Jog/MF_Unarmed_Jog_Bwd_Left` | 1.500 | 45 | 899 |  |
| `A_Crew_Grunt_Jog_Bwd_Right` | SKEL_NullCrew | `Unarmed/Jog/MF_Unarmed_Jog_Bwd_Right` | 1.500 | 45 | 899 |  |
| `A_Crew_Grunt_Jog_Fwd_Left` | SKEL_NullCrew | `Unarmed/Jog/MF_Unarmed_Jog_Fwd_Left` | 1.767 | 53 | 1061 |  |
| `A_Crew_Grunt_Jog_Fwd_Right` | SKEL_NullCrew | `Unarmed/Jog/MF_Unarmed_Jog_Fwd_Right` | 1.767 | 53 | 1061 |  |
| `A_Crew_Grunt_Jog_Left` | SKEL_NullCrew | `Unarmed/Jog/MF_Unarmed_Jog_Left` | 1.533 | 46 | 920 |  |
| `A_Crew_Grunt_Jog_Right` | SKEL_NullCrew | `Unarmed/Jog/MF_Unarmed_Jog_Right` | 1.733 | 52 | 1040 |  |
| `A_Crew_Grunt_JumpStart` | SKEL_NullCrew | `Unarmed/Jump/MM_Jump` | 0.867 | 26 | 0 |  |
| `A_Crew_Grunt_Land` | SKEL_NullCrew | `Unarmed/Jump/MM_Land` | 0.867 | 26 | 0 |  |
| `A_Crew_Grunt_Lunge` | SKEL_NullCrew | `Unarmed/Jump/MM_Dash` | 0.967 | 29 | 875 |  |
| `A_Crew_Grunt_Walk_Bwd` | SKEL_NullCrew | `Unarmed/Walk/MF_Unarmed_Walk_Bwd` | 2.133 | 64 | 643 |  |
| `A_Crew_Grunt_Walk_Bwd_Left` | SKEL_NullCrew | `Unarmed/Walk/MF_Unarmed_Walk_Bwd_Left` | 2.133 | 64 | 642 |  |
| `A_Crew_Grunt_Walk_Bwd_Right` | SKEL_NullCrew | `Unarmed/Walk/MF_Unarmed_Walk_Bwd_Right` | 2.133 | 64 | 642 |  |
| `A_Crew_Grunt_Walk_Fwd_Left` | SKEL_NullCrew | `Unarmed/Walk/MF_Unarmed_Walk_Fwd_Left` | 1.500 | 45 | 450 |  |
| `A_Crew_Grunt_Walk_Fwd_Right` | SKEL_NullCrew | `Unarmed/Walk/MF_Unarmed_Walk_Fwd_Right` | 1.500 | 45 | 450 |  |
| `A_Crew_Grunt_Walk_Left` | SKEL_NullCrew | `Unarmed/Walk/MF_Unarmed_Walk_Left` | 1.533 | 46 | 456 |  |
| `A_Crew_Grunt_Walk_Right` | SKEL_NullCrew | `Unarmed/Walk/MF_Unarmed_Walk_Right` | 1.533 | 46 | 463 |  |
| `A_Crew_Heavy_GuardRaise` | SKEL_NullCrew | `Pistol/MM_Pistol_Equip` | 1.400 | 42 | 0 |  |
| `A_Crew_Heavy_Idle_Guard` | SKEL_NullCrew | `Pistol/MF_Pistol_Idle_ADS` | 9.033 | 271 | 0 |  |
| `A_Crew_Heavy_Reload` | SKEL_NullCrew | `Pistol/MM_Pistol_Reload` | 2.000 | 60 | 0 |  |
| `A_Crew_Heavy_Walk_Bwd` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Bwd` | 1.767 | 53 | 530 |  |
| `A_Crew_Heavy_Walk_Bwd_Left` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Bwd_Left` | 1.767 | 53 | 530 |  |
| `A_Crew_Heavy_Walk_Bwd_Right` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Bwd_Right` | 1.767 | 53 | 535 |  |
| `A_Crew_Heavy_Walk_Fwd` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Fwd` | 1.433 | 43 | 465 |  |
| `A_Crew_Heavy_Walk_Fwd_Left` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Fwd_Left` | 1.433 | 43 | 466 |  |
| `A_Crew_Heavy_Walk_Fwd_Right` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Fwd_Right` | 1.433 | 43 | 466 |  |
| `A_Crew_Heavy_Walk_Left` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Left` | 1.800 | 54 | 529 |  |
| `A_Crew_Heavy_Walk_Right` | SKEL_NullCrew | `Pistol/Walk/MF_Pistol_Walk_Right` | 1.867 | 56 | 569 |  |
| `A_Crew_Hit_Back_Med_01` | SKEL_NullCrew | `Rifle/HitReact/MM_HitReact_Back_Med_01` | 0.867 | 26 | 0 | `A_Crew_Hit_Back_Med_01_Add` |
| `A_Crew_Hit_Front_Hvy_01` | SKEL_NullCrew | `Rifle/HitReact/MM_HitReact_Front_Hvy_01` | 0.800 | 24 | 0 | `A_Crew_Hit_Front_Hvy_01_Add` |
| `A_Crew_Hit_Front_Lgt_02` | SKEL_NullCrew | `Rifle/HitReact/MM_HitReact_Front_Lgt_02` | 0.800 | 24 | 0 | `A_Crew_Hit_Front_Lgt_02_Add` |
| `A_Crew_Hit_Front_Lgt_03` | SKEL_NullCrew | `Rifle/HitReact/MM_HitReact_Front_Lgt_03` | 1.000 | 30 | 0 | `A_Crew_Hit_Front_Lgt_03_Add` |
| `A_Crew_Hit_Front_Lgt_04` | SKEL_NullCrew | `Rifle/HitReact/MM_HitReact_Front_Lgt_04` | 1.233 | 37 | 0 | `A_Crew_Hit_Front_Lgt_04_Add` |
| `A_Crew_Hit_Front_Med_01` | SKEL_NullCrew | `Rifle/HitReact/MM_HitReact_Front_Med_01` | 0.767 | 23 | 0 | `A_Crew_Hit_Front_Med_01_Add` |
| `A_Crew_Hit_Front_Med_02` | SKEL_NullCrew | `Rifle/HitReact/MM_HitReact_Front_Med_02` | 0.800 | 24 | 0 | `A_Crew_Hit_Front_Med_02_Add` |
| `A_Crew_Trooper_Aim_Center` | SKEL_NullCrew | `Rifle/AIM/MM_Rifle_Idle_ADS_AO_CC` | 0.033 | 1 | 0 |  |
| `A_Crew_Trooper_Aim_Down` | SKEL_NullCrew | `Rifle/AIM/MM_Rifle_Idle_ADS_AO_CD` | 0.033 | 1 | 0 |  |
| `A_Crew_Trooper_Aim_Up` | SKEL_NullCrew | `Rifle/AIM/MM_Rifle_Idle_ADS_AO_CU` | 0.033 | 1 | 0 |  |
| `A_Crew_Trooper_DryFire` | SKEL_NullCrew | `Rifle/MM_Rifle_DryFire` | 0.800 | 24 | 0 |  |
| `A_Crew_Trooper_Equip` | SKEL_NullCrew | `Rifle/MM_Rifle_Equip` | 1.667 | 50 | 0 |  |
| `A_Crew_Trooper_FallLand` | SKEL_NullCrew | `Rifle/Jump/MM_Rifle_Jump_Fall_Land` | 0.400 | 12 | 388 |  |
| `A_Crew_Trooper_Jog_Bwd` | SKEL_NullCrew | `Rifle/Jog/MF_Rifle_Jog_Bwd` | 1.800 | 54 | 1064 |  |
| `A_Crew_Trooper_Jog_Bwd_Left` | SKEL_NullCrew | `Rifle/Jog/MF_Rifle_Jog_Bwd_Left` | 1.800 | 54 | 1064 |  |
| `A_Crew_Trooper_Jog_Bwd_Right` | SKEL_NullCrew | `Rifle/Jog/MF_Rifle_Jog_Bwd_Right` | 1.800 | 54 | 1064 |  |
| `A_Crew_Trooper_Jog_Fwd_Left` | SKEL_NullCrew | `Rifle/Jog/MF_Rifle_Jog_Fwd_Left` | 1.533 | 46 | 955 |  |
| `A_Crew_Trooper_Jog_Fwd_Right` | SKEL_NullCrew | `Rifle/Jog/MF_Rifle_Jog_Fwd_Right` | 1.533 | 46 | 1909 |  |
| `A_Crew_Trooper_Jog_Left` | SKEL_NullCrew | `Rifle/Jog/MF_Rifle_Jog_Left` | 0.467 | 14 | 276 |  |
| `A_Crew_Trooper_Jog_Right` | SKEL_NullCrew | `Rifle/Jog/MF_Rifle_Jog_Right` | 1.067 | 32 | 640 |  |
| `A_Crew_Trooper_JumpApex` | SKEL_NullCrew | `Rifle/Jump/MM_Rifle_Jump_Apex` | 0.667 | 20 | 236 |  |
| `A_Crew_Trooper_JumpStart` | SKEL_NullCrew | `Rifle/Jump/MM_Rifle_Jump_Start` | 0.233 | 7 | 117 |  |
| `A_Crew_Trooper_Reload` | SKEL_NullCrew | `Rifle/MM_Rifle_Reload` | 2.200 | 66 | 0 |  |
| `A_Crew_Trooper_Walk_Bwd` | SKEL_NullCrew | `Rifle/Walk/MF_Rifle_Walk_Bwd` | 1.200 | 36 | 360 |  |
| `A_Crew_Trooper_Walk_Bwd_Left` | SKEL_NullCrew | `Rifle/Walk/MF_Rifle_Walk_Bwd_Left` | 1.133 | 34 | 350 |  |
| `A_Crew_Trooper_Walk_Bwd_Right` | SKEL_NullCrew | `Rifle/Walk/MF_Rifle_Walk_Bwd_Right` | 1.133 | 34 | 350 |  |
| `A_Crew_Trooper_Walk_Fwd_Left` | SKEL_NullCrew | `Rifle/Walk/MF_Rifle_Walk_Fwd_Left` | 1.533 | 46 | 467 |  |
| `A_Crew_Trooper_Walk_Fwd_Right` | SKEL_NullCrew | `Rifle/Walk/MF_Rifle_Walk_Fwd_Right` | 1.533 | 46 | 467 |  |
| `A_Crew_Trooper_Walk_Left` | SKEL_NullCrew | `Rifle/Walk/MF_Rifle_Walk_Left` | 1.233 | 37 | 370 |  |
| `A_Crew_Trooper_Walk_Right` | SKEL_NullCrew | `Rifle/Walk/MF_Rifle_Walk_Right` | 1.500 | 45 | 450 |  |
| `A_Pilot_Death_Front_02` | SKEL_Pilot | `Death/MM_Death_Front_02` | 1.133 | 34 | kept (death) |  |
| `A_Pilot_Death_Front_03` | SKEL_Pilot | `Death/MM_Death_Front_03` | 0.933 | 28 | kept (death) |  |
| `A_Pilot_Death_Left` | SKEL_Pilot | `Death/MM_Death_Left_01` | 0.967 | 29 | kept (death) |  |
| `A_Pilot_Death_Right` | SKEL_Pilot | `Death/MM_Death_Right_01` | 1.100 | 33 | kept (death) |  |
| `A_Pilot_HitReact_Front_Hvy` | SKEL_Pilot | `Rifle/HitReact/MM_HitReact_Front_Hvy_01` | 0.800 | 24 | 0 | `A_Pilot_HitReact_Front_Hvy_Add` |
| `A_Pilot_HitReact_Front_Lgt_01` | SKEL_Pilot | `Rifle/HitReact/MM_HitReact_Front_Lgt_01` | 0.700 | 21 | 0 | `A_Pilot_HitReact_Front_Lgt_01_Add` |
| `A_Pilot_HitReact_Front_Lgt_02` | SKEL_Pilot | `Rifle/HitReact/MM_HitReact_Front_Lgt_02` | 0.800 | 24 | 0 | `A_Pilot_HitReact_Front_Lgt_02_Add` |
| `A_Pilot_HitReact_Front_Lgt_03` | SKEL_Pilot | `Rifle/HitReact/MM_HitReact_Front_Lgt_03` | 1.000 | 30 | 0 | `A_Pilot_HitReact_Front_Lgt_03_Add` |
| `A_Pilot_Idle_Mannequin` | SKEL_Pilot | `Unarmed/MM_Idle` | 7.567 | 227 | 0 |  |
| `A_Pilot_Interact_Console` | SKEL_Pilot | `Pistol/MM_Pistol_Reload` | 2.000 | 60 | 0 |  |
| `A_Pilot_Interact_Draw` | SKEL_Pilot | `Pistol/MM_Pistol_Equip` | 1.400 | 42 | 0 |  |
| `A_Pilot_Interact_Shoulder` | SKEL_Pilot | `Rifle/MM_Rifle_Equip` | 1.667 | 50 | 0 |  |
| `A_Pilot_Jog_Bwd` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Bwd` | 1.500 | 45 | 928 |  |
| `A_Pilot_Jog_Bwd_Left` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Bwd_Left` | 1.500 | 45 | 927 |  |
| `A_Pilot_Jog_Bwd_Right` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Bwd_Right` | 1.500 | 45 | 927 |  |
| `A_Pilot_Jog_Fwd` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Fwd` | 1.767 | 53 | 1093 |  |
| `A_Pilot_Jog_Fwd_Left` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Fwd_Left` | 1.767 | 53 | 1093 |  |
| `A_Pilot_Jog_Fwd_Right` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Fwd_Right` | 1.767 | 53 | 1093 |  |
| `A_Pilot_Jog_Left` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Left` | 1.533 | 46 | 948 |  |
| `A_Pilot_Jog_Right` | SKEL_Pilot | `Unarmed/Jog/MF_Unarmed_Jog_Right` | 1.733 | 52 | 1072 |  |
| `A_Pilot_Land_Hard` | SKEL_Pilot | `Rifle/Jump/MM_Rifle_Jump_Fall_Land` | 0.400 | 12 | 400 |  |
| `A_Pilot_Walk_Bwd` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Bwd` | 2.133 | 64 | 662 |  |
| `A_Pilot_Walk_Bwd_Left` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Bwd_Left` | 2.133 | 64 | 662 |  |
| `A_Pilot_Walk_Bwd_Right` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Bwd_Right` | 2.133 | 64 | 662 |  |
| `A_Pilot_Walk_Fwd` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Fwd` | 1.500 | 45 | 464 |  |
| `A_Pilot_Walk_Fwd_Left` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Fwd_Left` | 1.500 | 45 | 464 |  |
| `A_Pilot_Walk_Fwd_Right` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Fwd_Right` | 1.500 | 45 | 464 |  |
| `A_Pilot_Walk_Left` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Left` | 1.533 | 46 | 470 |  |
| `A_Pilot_Walk_Right` | SKEL_Pilot | `Unarmed/Walk/MF_Unarmed_Walk_Right` | 1.533 | 46 | 478 |  |
| `A_Pilot_WallJump` | SKEL_Pilot | `Unarmed/Jump/MM_WallJump` | 0.467 | 14 | 0 |  |

Derived by `ue_r20_derive.py` (copies; the originals are untouched):

| New clip | From | Length (s) | Frames | What changed |
|---|---|---|---|---|
| `A_Pilot_Light1_v2` | `A_Pilot_Light1` | 0.533 | 16 | source frames 8-24: the wind-up before the swing is cut. Hand at full speed 0.12 s after the press and at full reach at 0.23 s at rate 1.0 (was 0.24 / 0.31 s at rate 1.6, i.e. after the 0.28 s swing was over). |
| `A_Pilot_Light2_v2` | `A_Pilot_Light2` | 0.533 | 16 | as Light1_v2 (peak 0.12 s, reach 0.20 s at rate 1.0). |
| `A_Pilot_Light3_v2` | `A_Pilot_Light3` | 0.667 | 20 | source frames 7-27: peak 0.12 s, reach 0.23 s inside the 0.36 s swing at rate 1.0. |
| `A_Pilot_Heavy_v2` | `A_Pilot_Heavy` | 0.733 | 22 | source frames 28-50: the release only. Peak 0.15 s, reach 0.27 s after the lunge starts (lunge lasts 0.2 s). The original hits 0.77 s after the lunge starts at rate 1.4. |
| `A_Pilot_Heavy_Windup` | `A_Pilot_Heavy` | 0.467 | 14 | source frames 14-28 (0.467 s): the wind-up alone, for the 0.35 s the attack button is held before the lunge fires (play at 1.33). |
| `A_Pilot_AirSlam_Fall` | `A_Pilot_Heavy` | 0.500 | 15 | the wound-up pose of the heavy held for 0.5 s (a loop): the pose to fall in during an air slam. |
| `A_Pilot_AirSlam_Land` | `A_Pilot_Heavy` | 0.900 | 27 | source frames 28-55: release on landing with the full recovery (0.9 s). |
| `A_Crew_Grunt_Attack_01` | `MM_Attack_01` | 1.133 | 34 | MM_Attack_01 with the wind-up stretched so the fist is at full reach at 0.60 s = the grunt MeleeWindup (was 0.47 s: the punch landed 0.13 s before the damage). |
| `A_Crew_Heavy_Bash` | `MM_Attack_01` | 1.233 | 37 | MM_Attack_01 with the wind-up stretched so the hit is at 0.70 s = the heavy MeleeWindup. |
| `A_Pilot_HitReact_Front_v2` | `A_Pilot_HitReact_Front` | 0.767 | 23 | full-pose (non-additive) copy: the original is flagged additive, and UTDLoopBlendAnimInstance::PlayOneShot refuses additive sequences, so the pilot flinch never plays. |
| `A_Pilot_HitReact_Back_v2` | `A_Pilot_HitReact_Back` | 0.867 | 26 | full-pose (non-additive) copy: the original is flagged additive, and UTDLoopBlendAnimInstance::PlayOneShot refuses additive sequences, so the pilot flinch never plays. |
| `A_Pilot_Dash_v2` | `A_Pilot_Dash` | 0.233 | 7 | source frames 5-12 (0.233 s): only the forward dive of the dash clip, so at rate 1.0 it covers the 0.18 s dash and fades out (0.1 s) into the run. The original at rate 1.8 lasts 0.54 s: the leap and landing play after the dash is over. |

