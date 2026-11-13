# Drone - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-027 (ranked request list, item #2).
This is the first 3D delivery of the Null Armada Drone, built in Hyper3D Rodin Gen-2.5
from the approved concept-gate 3/4 hero (`ArtSource/EngAI/Characters/Drone/concept_r1/source/Drone_hero_gptimg2_PICK.png`,
PR #14, branch `grok/assets-concepts-r1`). Builders do the final import and cleanup;
this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's reply)
- **Where it shows:** drones on both planets and in the trailer. No mesh exists today (red placeholder blocks).
- **Style:** Null Armada — black armour, red-orange seams, glowing white eyes, chevron shapes; a hovering
  hard-surface eye-drone with a thruster ring. Match the trooper language in
  `ArtSource/EngCharacters/NullTrooper/`.
- **Budget / format:** ≤ 8k tris, 1K–2K textures, GLB or FBX.
- **Scale / pivot:** ~90 cm across, pivot at the centre, facing +X.
- **Rig / anims:** none.
- **Target:** new `DA_Crew_Drone_Visuals` under `Gameplay/Characters/`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SM_NullDrone.glb` and `SM_NullDrone.fbx`: derived, engine-ready copies. Largest horizontal extent is
  90.0 cm, overall 78.9 × 90.0 × 88.2 cm (X × Y × Z), pivot at the bounding-box centre, +Z up, eye facing +X.
  6,188 tris (under budget). The GLB stores centimetre numbers with `export_yup=false` (see `SOURCE.md` if
  an importer assumes glTF metres). The FBX uses the repo exporter's Apply Unit + FBX Units Scale
  (`UnitScaleFactor` 100) so Unreal should import it at 90 cm with uniform scale 1.0.
- `T_NullDrone_BC.png`, `T_NullDrone_N.png`, `T_NullDrone_ORM.png`, `T_NullDrone_E.png` at 2K.
  N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
- `source/`: the raw Rodin Base Model pack, untouched.
- `contact_sheet.png`: front, side, top, 3/4 and a 3/4 wireframe, rendered in Blender EEVEE.
