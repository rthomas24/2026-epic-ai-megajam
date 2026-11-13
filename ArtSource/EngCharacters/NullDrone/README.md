# Null Armada Drone - Unreal import (r1)

First AI-generated 3D asset in the game: the Hyper3D Rodin drone from
`ArtSource/EngAI/Characters/Drone/mesh_r1/` (source, licence and settings in its `SOURCE.md`; that folder is
read-only input and was not changed). It replaces the red placeholder block of the on-foot drone enemy.

## What was done
`scripts/ue_import_drone.py` (headless, idempotent):
`powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\NullDrone\scripts\ue_import_drone.py`

Creates, all in `/Game/TriadDrift/Gameplay/Characters/NullDrone/`:

| Asset | Notes |
|---|---|
| `SM_NullDrone` | FBX at scale 1, normals and tangents imported, Nanite off, 1 LOD, one convex hull as simple collision, sockets `Muzzle` (41, 0, -2: the eye, +X), `Engine_0` (0, -33, -36), `Engine_1` (0, 33, -36), socket +X pointing down |
| `T_NullDrone_BC` / `_E` | 2K, sRGB |
| `T_NullDrone_N` | 2K, Normalmap compression, no green flip (the delivery map is already DirectX) |
| `T_NullDrone_ORM` | 2K, Masks, sRGB off (R AO, G roughness, B metallic) |
| `MI_NullDrone` | instance of the existing engineering master `/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured`, `EmissiveStrength` 6 |
| `DA_Crew_Drone_Visuals` | `UTDEnemyVisuals`, only `Mesh` set |

No new master material, no code, map or config change. Nothing was deleted: without the data asset the code falls
back to the placeholder block.

## Measured in Unreal
- Size 78.9 x 90.0 x 88.2 cm (X x Y x Z), bounding-box centre at (0, 0, 0).
- 6,188 triangles, 7,342 vertices, 1 material slot.

## How gameplay uses it
`ATDCrewCharacter::LoadDefaultVisuals` -> `FindCrewVisuals("Drone")` looks up `DA_Crew_Drone_Visuals`: first in
`/Game/TriadDrift/Presentation/` (Nick's asset wins), then any `UTDEnemyVisuals` of that name under
`/Game/TriadDrift/Gameplay/Characters/**`. `ApplyVisuals` puts `Mesh` on the `Body` static mesh component (scale 1,
centred on the capsule, no collision) and hides the placeholder eyes. Hits still land on the capsule
(radius 35, half-height 35). Shots start from the computed chest-front point; sockets are read only on skeletal looks,
so `Muzzle` / `Engine_*` are there for later use.

## Verification (stage: technically verified)
- Running game, offscreen, driven over MCP (port 8016): `TD.Stage VerdanceOnFoot`, `TD.Ground.Spawn Drone`.
  `review/NullDrone_ingame_r1.png`: drone at 6 m, eye facing the player, hovering at 220 cm, seams and white eye
  glowing, textured, not black or inside out. No `Failed to compile Material` in the log.
- `review/NullDrone_ingame_r1_corpse.png`: after `TD.KillAll` the body drops and tumbles as a physics body.
- `Tools\td-test.ps1 -Filter TriadDrift.Ground.Crew`: 21 passed, 0 failed.
- NOT shown in play: a kill by the blade or a deflected shot. Scripted blocking for about 40 s did not kill a drone
  (it strafes fast, and block timing by script is unreliable). The hitbox is the capsule, which this change does not touch.
- Not seen by a human, not in a packaged build.

## Notes
- The mesh (90 cm across, 88 cm tall) is larger than the drone's capsule (70 cm): the hitbox is smaller than the look.
- A strafing drone is strongly motion-blurred at 600 cm/s.
- The mesh was not run through `blender-ai-mesh-cleanup` (Rodin topology as delivered, already under the 8k budget).
