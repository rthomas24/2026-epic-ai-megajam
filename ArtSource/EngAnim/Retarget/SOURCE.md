# EngAnim / Retarget: in-house clips of Nov 13

## Source and licence
- Every clip here comes from **Epic's template mannequin animations** shipped with the Unreal Engine First Person / Shooter
  template in this project (`/Game/Characters/Mannequins/Anims/**`, skeleton `SK_Mannequin`). Epic content, usable in
  Unreal Engine projects under the Unreal Engine EULA. Not third-party, not AI-generated. The template assets were only read.
- Retargeted with Unreal's IK Retargeter (`RTG_Manny_to_NullCrew`, `RTG_Manny_to_Pilot`, made Nov 12-13) by Claude Code.
- The pilot target mesh and skeleton (`SK_Pilot`) are Nicholas's Sentinel Armor sample (Meshy AI); its licence note is in
  `ArtSource/EngCharacters/Pilot/SOURCE.md` and is unchanged by this work. No new clip was taken from the sample GLB
  (all six of its clips were imported on Nov 12).

## How to rebuild (headless, from the repo root)
1. `powershell -File Tools\td-python.ps1 -Script ArtSource\EngAnim\Retarget\scripts\ue_r10_retarget.py` (retargets; skips existing)
2. `... ue_r20_derive.py` (re-timed copies and full-pose copies; skips existing)
3. `... ue_r30_export.py` (FBX with preview mesh to `Saved/EngAnim/fbx/`)
4. Blender 5.2: `blender --background --factory-startup --python ArtSource/EngAnim/Retarget/scripts/b_strip.py -- <abs in.fbx> <abs out.png>`
   (absolute paths; one at a time: parallel runs share a temp folder), then `python ArtSource/EngAnim/Retarget/scripts/compose.py`.

`retarget_report.json` and `derive_report.json` list every clip made, its source, length and what was changed.

## What was checked, and what was not
- Checked per clip: target skeleton, length equal to the source, root translation zero for loops and one-shots, the re-timed
  clips equal the source pose at the mapped times (0.000 cm), head-to-foot scale and foot height from the exported FBX
  (no collapsed or scaled-up pose), and a contact strip (front and side, 6 frames) in `review/`.
- **Not checked:** none of the new clips has been played in the running game (no code names them yet), nothing was watched
  in motion, and wrists / fingers cannot be judged at the strip resolution. Crew clips were viewed on `SK_NullTrooper` only
  (the grunt and heavy share its skeleton but have their own proportions).
- Stage (AAA.md): prototype complete. Not technically verified in game.

## Known limits of this set
- All the hit reacts, reloads, equips and "interact" candidates hold the arms in a rifle or pistol grip (that is their source).
  On the pilot they are only right as the `_Add` (additive) versions.
- The deaths end in a kneeling crumple (the template hands over to a ragdoll there).
- `A_Crew_Trooper_Aim_Up` / `_Down` are the +-90 degree extremes of an aim offset: blend toward them, never show them alone.
- `A_Crew_Grunt_Attack_01` and `A_Crew_Heavy_Bash` keep the 150 cm root travel of `MM_Attack_01` (the C++ player zeroes it).
- The blade moves are boxing punches re-timed: the timing is right, the style is not a one-handed blade.
