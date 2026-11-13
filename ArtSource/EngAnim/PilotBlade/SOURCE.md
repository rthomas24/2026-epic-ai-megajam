# EngAnim / PilotBlade: Grok Bot's blade set on the pilot (Nov 13)

Source clips: `ArtSource/EngAI/Animations/PilotBlade/r1` (HY-Motion 1.0 through fal.ai, 52-bone SMPL-H, 30 fps; tool,
seeds, prompts and licence in that folder's `SOURCE.md`). Those files are only read. Built by the animation builder
(Claude Code, Ryan's side).

## Rebuild (headless, from the repo root)
1. `powershell -File Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotBlade\scripts\ue_pb00_probe.py`
   imports the SMPL-H reference mesh to `/Game/TriadDrift/Dev/EngAnim/PilotBlade/SK_SMPLH` (a folder that is not cooked).
2. `... ue_pb10_retarget.py` imports the 14 clips there (`Src_*`), builds `IK_SMPLH` and `RTG_SMPLH_to_Pilot` (same route
   as `RTG_Manny_to_Pilot`: chains named as `IK_Pilot`'s, target auto-aligned to the source T-pose) and writes
   `Pilot/Anims/A_Pilot_Blade_<Clip>`. `PB_REDO=1` rebuilds them.
3. `... ue_pb20_wire.py` keeps each replaced clip as `<name>_v1` and puts the trimmed new clip at the name the C++ reads.
   `PB_REDO=1 PB_ONLY=<names>` rebuilds named ones.
4. Strips: `ArtSource/EngAnim/Retarget/scripts/ue_r30_export.py` (clip paths in `Saved/EngAnim/export_extra.txt`), then
   `b_strip.py` with ABSOLUTE paths, then `compose.py`. In game: `scripts/play.py` (the audit driver plus a `relist`
   step) against `Tools\td-play-mcp.ps1 -Port 8014 -Offscreen`.

## Two things the retarget needed
- The SMPL-H rest pose has its origin in the chest (pelvis 19 cm below it). Unreal's pelvis-motion op divides by the
  source pelvis height, so the first run threw the pilot 80 m into the air. The source retarget pose is lifted 116.3 cm
  (`set_root_offset_in_retarget_pose`), which puts the pelvis at its standing height.
- `add_retarget_chain("Head", ...)` is refused on the SMPL-H rig; the chain is `HeadC`, mapped to the pilot's `Head` by hand.

## Clean-up applied to every `A_Pilot_Blade_*`
- root bone zero; pelvis horizontal travel removed (the 0.5 s moving average: 132 cm on the heavy lunge, 313 cm on the
  dash); everything is in place, because the C++ zeroes the root and the capsule does the moving;
- foot anchoring (not on the lunge, dash, air slam, deaths): Grok Bot's in-place bake locked the pelvis, which makes a
  weight shift slide the feet under a fixed pelvis. The body is moved against the velocity of whichever foot is down,
  at most 18 cm from the capsule centre; the two loops are closed. Measured "distance a foot travels while it is
  within 3 cm of its lowest point": idle 53 to 33 cm over 3 s, hit front 72 to 53, flourish 57 to 38, light 3 0.4.
  This is a reduction, not a fix: a step that was baked out still slides.
- ACL compression, 30 fps, lengths equal to the source.

## What the code plays (no C++ was changed)
`ATDFirstPersonCharacter` loads action clips by name (`A_Pilot_<Move>`) at fixed rates; only Idle / Walk / Run are slots
on `DA_Pilot_Visuals`. So a replacement is a new clip at the old name; the old one is kept as `<name>_v1`.

| Name the code reads | Now | Trim | Hand speed peak, real time at the code's rate |
|---|---|---|---|
| `A_Pilot_Light1` (x1.6) | Grok Light1 | frames 2-35 | 0.10 s (lockout 0.28 s) |
| `A_Pilot_Light2` (x1.6) | Grok Light2 | frames 4-35 | 0.13 s |
| `A_Pilot_Light3` (x1.5) | Grok Light3 arms, spine and head on Grok Light1's pelvis and legs | frames 3-35 | 0.13 s (lockout 0.36 s) |
| `A_Pilot_Heavy` (x1.4) | Grok HeavyLunge | frames 17-59 | 0.14 s after the lunge starts (lunge 0.2 s) |
| `A_Pilot_Dash` (x1.8 / x0.8) | Grok Dash | frames 2-20 | 0.33 s long at x1.8 |
| `A_Pilot_HitReact_Front`, `_Back` (x1.2, upper body) | Grok HitFront / HitBack at 55 % amplitude, full pose | whole | first time a flinch plays at all (the old clips are additive and refused) |

Not wired (no code slot): `A_Pilot_Blade_DeflectFlick`, `_Flourish`, `_Death1`, `_Death2`, `_Idle`, `_HeavyWindup`,
`_BlockHold`, `_AirSlam`, and the full-strength `_HitFront` / `_HitBack`.

## Review
`review/blender_attacks_new_vs_old.png`, `review/blender_reacts_new_vs_old.png` (Blender strips, new beside old) and
`review/ingame_pilot_blade_r1.jpg` (the running game, third person, slow motion).
