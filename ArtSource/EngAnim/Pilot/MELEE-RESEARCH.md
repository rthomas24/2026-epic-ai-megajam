# Third-person melee animation: how it is built in practice (animation builder, Nov 14)

Research for the pilot's one-handed energy blade, seen in third person. General craft only: no signature
moves of any specific game. Sources are marked **read** (page fetched and read) or **summary** (search result
summaries only).

## 1. Timing of a swing (anticipation, contact, follow-through, recovery)

- Every attack has three stages: start-up (anticipation), active (contact) and recovery. Only the active
  frames hurt. (summary: Defold forum on active frames; Gnomon "Animating stylized attacks")
- Too much anticipation feels like input lag; too much recovery feels sticky. Heavy weapons earn longer
  wind-ups and recoveries; light attacks must stay snappy. (summary: Gnomon; slynyrd pixelblog 9)
- One-handed sword, at 30 fps (read: MoCap Online sword melee guide):
  - light: a slight coil of 0.1-0.2 s, swing 6-12 frames, the whole clip 20-35 frames;
  - heavy: a full visible wind-up, swing 12-24 frames, whole clip 45-80 frames;
  - the fastest point of the arc sits 60-70 % through the arc, not at its end;
  - a horizontal follow-through carries 30-60 deg past centre before slowing;
  - recovery in the clip must match the mechanical recovery exactly.
- A plausible worked example of a sword swing: about 6-7 frames of anticipation, about 10 frames of swing
  with a 5-frame hit window, the rest follow-through and recovery. (summary: "The Amalur Problem", aztez.com)
- A three-hit chain escalates: a fast low-commitment first cut, a second cut in the opposite direction, a
  finisher with the most commitment and the longest recovery. (read: MoCap Online)

## 2. Combo and cancel windows

- The combo window opens at about 70 % of the recovery; earlier feels automatic, later unresponsive. Buffer an
  early press so it still chains. (read: MoCap Online)
- Cancels end one animation by starting another: partial cancels in set windows (most often before the hit
  starts and after it ends) and complete cancels at any time (dodge, block). A buffered input can be
  overridden before its window opens. (summary: Game Developer, "Combat Canceled", a combat designer's article)
- Put the windows in the clip as markup (notifies / tags) so all systems read the same timing. (summary:
  realtime-animation-tips, "Animation tactile response")

## 3. Root motion versus in place, lunge and warping

- In place for light and heavy swings that cover 1-2.5 m; root motion only for long lunges (2.5-4 m) that
  must travel with the animation. (read: MoCap Online)
- Motion warping bends a root-motion clip's travel to reach the target; with code-driven movement the same
  effect comes from moving the capsule toward the target during the anticipation, with the clip in place.
  (summary: Epic forum threads on attack while moving; Epic layered animation docs)

## 4. Upper and lower body

- Standard setup: locomotion cached, then a Layered Blend per Bone from `spine_01` with the attack on the
  upper input, so the legs keep walking while the arm swings. (summary: Epic "Using layered animations")
- Upper-only attacks look wrong standing still: switch to the full-body clip when the speed is near zero
  (blend by bool / speed). (summary: Epic forum threads "play full body if not moving", "attack while moving")
- A blend from the spine with the pelvis rotating in the legs' clip twists the torso off target; better rigs
  add a counter-rotation on the lower body (or blend in mesh space) for the length of the attack. (summary: forum)

## 5. Hit stop and camera

- 1-4 frames of hit stop on a hit; even 1-2 frames makes impact read. A 2-4 frame slowdown of the weapon
  at contact helps too. (read: MoCap Online; summary: mygamedesign "crunchy melee")

## 6. Locomotion: foot sliding, warping, turns

- Distance matching changes the play rate so the feet match the distance travelled (starts and stops); stride
  warping scales the step length to the capsule speed; orientation warping turns the legs toward the
  direction of travel and counter-twists the spine so the chest keeps facing. (read: Epic "Pose warping")
- Keep play-rate and stride-warp changes within about 15-20 %; past that it looks worse than a little sliding.
  (summary: Epic's Lyra animation tech blog, quoted in search results; the page itself refused the fetch)
- Orientation warping covers up to 90 deg per side from one forward jog; it needs root-motion clips and an IK
  rig in Epic's node. Directional clips (forward, left, right, back) blended by angle are the classic alternative.
  (read: Epic "Pose warping"; summary: forum distance-matching threads)
- Foot locking with IK removes what sliding is left; foot IK on slopes needs per-foot traces and a two-bone
  solve. (summary: forum; bugnet "control rig foot IK sliding")

## 7. Grip, stance, block

- Weapon in hand: attach the weapon to a hand socket and close the fingers with a grip pose; a two-bone IK
  on the off hand (or a virtual bone) is only needed for two-handed weapons. (general practice; summary: forums)
- Ready stance for a one-handed blade: weight slightly forward, knees bent, blade hanging relaxed near mid-thigh,
  off hand relaxed and out of the way. (read: MoCap Online)
- Block idle is watched for 5-10 s at a time: keep it calm. A parry flick lasts 0.2-0.3 s at most, active
  window 4-8 frames; on a deflect the blade rebounds. (read: MoCap Online)

## 8. Blending

- Short cross-fades (about 0.1-0.2 s) between combat clips; a new attack must never snap to its first frame.
  Inertialization (blend from the outgoing pose's velocity, not a second evaluated pose) is the cheap modern way
  to hide the cut. (summary: GDC 2018, "Inertialization: high-performance animation transitions")

## Sources
- MoCap Online, sword melee animation guide: https://mocaponline.com/blogs/mocap-news/sword-melee-animation-guide (read)
- Epic, Pose warping in Unreal Engine: https://dev.epicgames.com/documentation/unreal-engine/pose-warping-in-unreal-engine (read)
- Epic, Adapting Lyra animation to your UE5 game: https://www.unrealengine.com/tech-blog/adapting-lyra-animation-to-your-ue5-game (summary; fetch refused)
- Epic, Using layered animations: https://dev.epicgames.com/documentation/en-us/unreal-engine/using-layered-animations-in-unreal-engine (summary)
- Epic forum, attack while moving / full body when still: https://forums.unrealengine.com/t/can-i-play-upper-body-attack-animation-when-the-character-is-on-the-move-and-full-body-attack-animation-when-not-on-the-move/786411 (summary)
- Game Developer, Combat Canceled: https://www.gamedeveloper.com/pc/combat-canceled-i-god-of-war-i-action-game-design (summary)
- GDC Vault, Inertialization: https://gdcvault.com/play/1025165/Inertialization-High-Performance-Animation-Transitions (summary)
- aztez.com, The Amalur Problem: https://aztez.com/?p=1365 (summary)
- Gnomon, Animating stylized attacks in Maya: https://bmc.thegnomonworkshop.com/workshops/animating-stylized-attacks-in-maya (summary)
- realtime-animation-tips, tactile response: https://sites.google.com/view/realtime-animation-tips/tutorials/animation-tactile-response (summary)

## What the game does today (from the code, Nov 14)

- Body: native `UTDLoopBlendAnimInstance` (`TDPilotAnim.cpp`), no AnimBP. Idle / walk / run loops by speed
  (60 and 520 cm/s thresholds), play rate = speed / 300 or 650, clamped 0.6-1.5; 0.2 s loop cross-fade.
- One-shots (`PlayOneShot`): 0.1 s fade in and out; upper body from `spine_01` (local space) when moving faster
  than 150 cm/s, full body when standing. A new one-shot replaces the old one at once: the next combo swing
  snaps to its own first frame (a pop on every chained hit).
- Light swings: 0.28 / 0.28 / 0.36 s lockout, damage 0.10 s after the press, combo window 0.45 s after the
  swing, presses buffered. Clips played: `A_Pilot_Light1/2/3` at x1.6 / x1.6 / x1.5. The re-authored
  `_v2` copies (blade forward through the cut) are NOT the names the code plays.
- Heavy: hold 0.35 s, lunge 6 m in 0.2 s, `A_Pilot_Heavy_v3` x1.4 full body. Air slam plays `A_Pilot_Light3`.
- Block: `A_Pilot_Block` as an upper loop (0.12 s fade); deflect flick upper one-shot.
- Third person faces the movement direction (no strafe), and turns to the camera on a combat action; while
  swinging and moving sideways the legs run forward and slide sideways. Eight directional walk and jog clips
  exist (`A_Pilot_Walk_*`, `A_Pilot_Jog_*`) but are not used.

## What we will do in this game today (about 8 working hours)

1. Audit in game (third person, Verdance on foot): a strip per move; list what is wrong. (1 h)
2. Play the good swings: move the re-authored `_v2` swings onto the names the code plays (originals kept as
   `_v1b`), or point the code at the `_v2` names (one line each for T5). (0.5 h)
3. No pop between chained swings: the anim instance keeps the outgoing one-shot pose and cross-fades into the
   new swing over 0.08-0.1 s (inertialization-style, in `TDPilotAnim.cpp`). (1 h)
4. Swing timing to the code: anticipation 0.05-0.08 s, contact at 0.10 s, follow-through with chest yaw and a
   small hip turn, recovery ending in the ready stance so the next swing starts from where the last ended;
   three distinct arcs (diagonal down, back-hand flat, rising/overhead finisher with the biggest commitment).
   With the existing in-engine arm authoring tool (`ue_pilot_fixes.py`). (2 h)
5. Moving while attacking or locked to the camera: pick the directional walk / jog clip by the angle between
   velocity and facing (strafe and backpedal) so the feet stop sliding; play rate kept within about 20 %. (1 h)
6. Idle with the blade out: blade low and ready, weight on one leg (existing `A_Pilot_Blade_Idle` or the
   `Idle3_v2` with a hip shift) for the close-combat state. (0.5 h)
7. Heavy wind-up and lunge, air slam with a plunging pose on the way down and the landing hit; block and
   deflect checked from behind. (1.5 h)
8. Captures after every item, animations.md rows, ART-HISTORY for replaced clips. (0.5 h, as we go)
