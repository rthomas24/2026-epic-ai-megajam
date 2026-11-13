# EngCharacters / Warden: the Null Armada Warden in the game (Nov 13)

Built by the animation and character builder (Claude Code, Ryan's side) from Grok Bot's deliveries, which are only read:
`ArtSource/EngAI/Characters/Warden/mesh_r1` (body, Rodin Human auto-rig, 2K maps) and `props_r1` (shield and gun arm).
Tool, plan, licence and prompts: the `SOURCE.md` files in those folders (Hyper3D Rodin Gen-2.5, paid Business plan).

## What is in the game
`Content/TriadDrift/Gameplay/Characters/Warden/`: `SK_Warden` (42,939 triangles, 260.0 cm, three material slots),
`SKEL_Warden`, `PA_Warden`, `MI_Warden`, `MI_Warden_Shield`, `MI_Warden_GunArm`, twelve 2K textures, and
`DA_Crew_Warden_Visuals` (the name `ATDCrewCharacter::FindCrewVisuals("Warden")` looks up; an asset of that name in
`/Game/TriadDrift/Presentation` wins).

## How it was made
1. `scripts/w10_build.py` (Blender 5.2, headless; `blender --background --factory-startup --python scripts/w10_build.py -- <repo root>`):
   - the crew armature (`ArtSource/EngCharacters/Crew/export/NullHeavy/SK_NullHeavy.fbx`: the bone names and bone axes of
     `SKEL_NullCrew`) is fitted to the Rodin joints: each mapped bone moves to the Rodin joint and its frame is turned by
     the smallest rotation from the crew limb direction to the Warden limb direction. A crew pose therefore gives the
     Warden the same limb directions; lengths stay the Warden's.
   - body weights renamed Rodin to crew (spine to spine_01 / 03 / 05, fingers to the hand), at most 4 influences;
   - the gun welded to the body (right forearm, 3,334 vertices) is cut off 4 cm below the elbow and replaced by
     `props_r1/SM_Warden_GunArm`, rigid on `lowerarm_r` (cuff at the elbow, barrels along the forearm);
   - the softly skinned shield is dropped; `props_r1/SM_Warden_ShieldProp` is rigid on **`spine_05`**, placed where the
     left hand holds it in the crew rifle idle (see "Shield" below);
   - sockets: `Muzzle` and `GunArm` on `lowerarm_r`, `Shield` on `spine_05`, `HeadGlow` on `head`;
   - outputs `export/SK_Warden.fbx` (cm; the armature object `root` is the root bone, as the crew export),
     `Warden_v01.blend`, and `review/bind_*.png`, `review/pose_*.png` (the rig posed by crew clips, computed in Blender;
     kept in git as the two sheets `review/blender_bind.jpg` and `review/blender_crew_poses.jpg`).
   - needs `Saved/EngAnim/fbx/MF_Rifle_Idle_ADS.fbx` and friends (Unreal exports of the crew clips:
     `ArtSource/EngAnim/Retarget/scripts/ue_r30_export.py` with the clip paths in `Saved/EngAnim/export_extra.txt`).
2. `scripts/ue_w20_import.py` (`powershell -File Tools\td-python.ps1 -Script ...`): textures, material instances on the
   crew master `M_NullTrooper`, `SKEL_Warden`, the mesh, `PA_Warden`, `DA_Crew_Warden_Visuals`.

## Skeleton and animation: no retargeted copies
`SKEL_Warden` is a copy of the template mannequin skeleton asset (`/Game/Characters/Mannequins/Meshes/SK_Mannequin`, Epic
template content, read only). It has the crew's bone names, and Epic's per-bone translation retargeting (bones keep
this mesh's lengths, the pelvis height is scaled), which Unreal's Python cannot set. `SKEL_NullCrew` is listed on it as
a compatible skeleton. So **every clip in `Crew/Anims` plays on the Warden as it is**: the C++ loads the crew clips by
name for every crew type, and nothing had to be copied or changed in code. `SKEL_NullCrew` was not touched.

## Shield: on the chest bone, not the hand (deviation from the brief)
The brief was "shield on hand_l via a socket". Two facts changed that:
- `UTDEnemyVisuals` has no attachment slot, so a socketed static mesh would never be attached. The props are part of the
  skeletal mesh, rigid on one bone each (the same picture as a socket attachment).
- The Warden plays the crew's rifle clips. With the shield on `hand_l` it stands upright in the idle and the walk, but
  in the rifle jog (which the Warden uses whenever it moves at its 380 cm/s) the left hand drops and the shield swings
  through the legs (seen in the Blender pose preview of that build, discarded). On `spine_05` it stays upright in
  front-left of the body in every clip; the left hand is at its grip only in the idle and the walk.
`WARDEN_CFG='{"shield_bone":"hand_l"}'` rebuilds the hand-held version once a real shield set exists.

## Checked / not checked
- Blender: 260.0 cm, 0 unweighted vertices, mapped joints 0.000 cm from the Rodin joints, pose previews of idle, walk,
  jog, charged attack, fire, hit, death.
- Unreal: import report (bounds 0 to 260 cm, sockets, slots, retarget modes).
- In the running game (`Tools\td-play-mcp.ps1 -Port 8014 -Offscreen`, `TD.Stage VerdanceOnFoot`, `TD.Ground.Spawn Warden`,
  a trooper beside it): `review/ingame_warden_r1.jpg`. Idle, walk / jog, charge slam and bolt fan, death were seen; the
  hit row shows the pilot striking it, but a flinch cannot be made out in those frames.
- Not done: no LODs, no UCX, physics asset is the automatic one, gun-arm cuff does not meet the upper arm when the arm
  is straight (it does at a bent elbow), triangle count is 2.9k over the 40k brief.
