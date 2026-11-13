# Animation audit: scores (Nov 13)

Scored 1-5 against `animations.md` section 4 by the animation builder (the agent that also made today's new clips, so this is
**not** the independent review of stage 6). How it was seen: a real game session on `lead/int` (1050b21), offscreen, driven
through Unreal's MCP server on port 8016 (`TD.Stage VerdanceOnFoot`, `TD.Ground.ThirdPerson 1 / 0`, `TD.Input.Key(s)`,
`slomo 0.1-0.3`, viewport captures), plus the clip measurements in `inventory.md`. Stills in slow motion show poses and
timing; they do **not** show foot sliding or blend pops directly: those scores come from the measured speeds and from the code.
Nobody has judged these by eye at full speed.

Sheets: `review/Pilot_audit.png`, `review/Pilot_FirstPerson_audit.png`, `review/Trooper_audit.png`, `review/Grunt_audit.png`,
`review/Heavy_audit.png`. The crew sheets are taken from the pilot's first-person view (the crew keep their distance, so they
are small in frame; the close rows are the useful ones).

5 = meets the bar; 4 = good, small gaps; 3 = works, clearly a placeholder; 2 = wrong in a way a player notices; 1 = missing or broken.

## Pilot (third person unless noted)

| Move | Score | What is wrong (one sentence) |
|---|---|---|
| Idle (`A_Pilot_Idle3`) | 4 | Natural and loops cleanly, but one variant only and it is a hands-at-sides stand, not a blade-ready stance (the hand blade hangs at the hip). |
| Walk (`A_Pilot_Walk`) | 2 | The clip's feet cover 137 cm/s and it is played at rate 1 for 300 cm/s (slide x2.2); in play it is only seen under a block. |
| Run and sprint (`A_Pilot_Run`) | 3 | A good cycle with hip and arm motion, but the feet cover 490 cm/s against 650 (run) and 716 against 950 (sprint), and the same forward cycle plays when strafing or backing up. |
| Jump, fall, land | 2 | No take-off and no landing: the body switches to the arms-up fall loop in the air and snaps to idle on the ground (`A_Pilot_JumpStart` / `A_Pilot_Land` are never played for a jump). |
| Dash (`A_Pilot_Dash` x1.8) | 1 | The body stands upright through the 0.18 s dash, then plays a superhero leap and landing for 0.4 s after the dash is over. |
| Light 1 (`A_Pilot_Light1` x1.6) | 3 | The hit pose reads as a flat slash, but the blade arrives 0.24-0.31 s after the damage, the motion is a boxing cross with a blade in the fist, and a queued Light 2 cuts it at the moment it lands. |
| Light 2 (`A_Pilot_Light2` x1.6) | 2 | A mirrored left hook: the body hunches over and the blade trails low behind it; same late hit, and the pose jumps from Light 1 with no cross-fade. |
| Light 3 (`A_Pilot_Light3` x1.5) | 3 | The best of the three (big wound-up swing with follow-through), still 0.23-0.31 s late and horizontal where first person shows an overhead chop. |
| Heavy lunge / dash strike (`A_Pilot_Heavy` x1.4) | 1 | The pilot slides 6 m standing up, and only then winds up (0.66 s) and swings at nothing; the stab the game resolved has no motion at all. |
| Air slam (`A_Pilot_Light3` x1.2) | 1 | Not captured (no air slam was triggered); from the code: a horizontal swing starts in the air and nothing plays on the impact that does the damage. |
| Block hold (`A_Pilot_Block`) | 3 | A clear, steady guard on the upper body that works while walking; no raise, no motion, and it is a one-frame pose. |
| Deflect / perfect deflect | 1 | No motion exists: bolts bounce off a frozen guard, and the perfect deflect (a Director's Cut slow-motion beat) has no flourish. |
| Hit react front / back | 1 | Never plays: both clips are flagged additive and the player refuses additive clips (captured: health 100 to 70, no flinch). |
| Guard broken | 1 | Same clip, same refusal: the 1.2 s punish state has no pose. |
| Death | not scored | Ragdoll from an untuned automatic physics asset; not reached in the capture session. `A_Pilot_Death_*` are unused. |
| Interact, board, exit | 1 | None. |
| First person: light 1-2-3, heavy, block (code-posed blade) | 2 | Readable arcs that match the lockout exactly, but no arms, the swings cover a small part of the screen, and they do not match the third-person motion (overhead vs horizontal, thrust vs hook); looking down, the body's forearm swings through the view. |

## Null trooper

| Move | Score | What is wrong |
|---|---|---|
| Idle / aim (`MF_Rifle_Idle_ADS`) | 3 | A solid rifle stance; no aim offset, so it never points up or down at the player, and there is no rifle in its hands (bolts leave the right hand). |
| Walk / run | 2 | Feet turn over x1.3-1.4 too fast for the ground speed, and it backs away running forwards on the spot (no strafe or backpedal). |
| Fire (`MM_Rifle_Fire`) | 2 | One recoil kick during the 0.4 s wind-up; the three shots that follow have none. |
| Hit react | 2 | One light front flinch for every hit from every side; full pose, so it pops the arms. |
| Death | 2 | Falls by hit direction, but each direction is one clip that ends in a kneeling crumple and freezes there; then the body vanishes. |
| Cover, reload, alert | 1 | None. |

## Null grunt

| Move | Score | What is wrong |
|---|---|---|
| Idle | 3 | Template idle, relaxed: no menace for a melee enemy. |
| Charge (`MF_Unarmed_Jog_Fwd`) | 2 | Feet x1.3 too fast (560 cm/s on a 600 cm/s clip played at rate 1.24); a casual jog, not a charge. |
| Melee (`MM_Attack_01`) | 2 | The punch is at full reach 0.13 s before the damage lands, so the telegraph and the parry window do not match what the player sees; one variant. |
| Hit react | 1 | None (the only flinch holds a rifle, so the code skips it for the grunt). |
| Death | 2 | As the trooper. |

## Null heavy

| Move | Score | What is wrong |
|---|---|---|
| Idle | 1 | Aims an invisible rifle with a shield on its arm: the shield is not presented. |
| Walk | 2 | At 330 cm/s it plays the rifle jog at rate 0.73 instead of a heavy walk. |
| Melee / shield bash (`MM_Attack_01`) | 2 | A right cross, at full reach 0.23 s before the damage (wind-up 0.7 s). |
| Shield up, shielded hit | 1 | None. |
| Guard-break stagger (3 s) | 1 | None: the state that opens the heavy to damage has no pose, so the player cannot see that it worked. |
| Death | 2 | As the trooper. |

## The five worst existing moves

1. **Pilot heavy lunge / dash strike**: wind-up and swing play 0.6 s after the lunge has ended. A re-timed copy exists (`A_Pilot_Heavy_v2`, `A_Pilot_Heavy_Windup`); it needs two lines of code to use, and a real one-handed thrust from Grok Bot to be good.
2. **Pilot dash**: the leap plays after the dash. `A_Pilot_Dash_v2` (the dive only, 0.23 s) fixes the timing; a real dash pose is still needed.
3. **Pilot hit react and guard broken**: never play (additive clips refused). `A_Pilot_HitReact_Front_v2` / `_Back_v2` are full-pose copies that will play; they hold the arms in a rifle grip, so the right fix is an additive layer with the `_Add` clips.
4. **Pilot deflect and perfect deflect**: no motion at all on the game's signature defensive move. Cannot be made in-house from what exists.
5. **Null heavy guard set**: rifle idle with a shield, no shield-up, no guard-break stagger. `A_Crew_Heavy_Idle_Guard`, the `A_Crew_Heavy_Walk_*` set and `A_Crew_Hit_Front_Hvy_01` are stand-ins; a real shield set is needed.

Close behind: the light combo's late hit (re-timed copies made), the air slam, the jump with no take-off or landing, crew melee landing before its damage (re-timed copies made), crew foot speed (two numbers in code).
