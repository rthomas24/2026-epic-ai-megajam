# Fighter - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (ranked request list, item #4, Null Armada
fleet). This is the first 3D delivery of the Null Armada Fighter, built in Hyper3D Rodin Gen-2.5
from the approved concept-gate 3/4 hero
(`ArtSource/EngAI/Ships/NullFleet/Fighter/concept_r1/source/hero-NullFighter-g26a_PICK.png`,
PR #15, branch `grok/assets-concepts-r2`). Builders do the final import and cleanup;
this drop is source plus an engine-ready derived copy only. Later fleet ships will come in
their own PRs on the same `grok/assets-fleet-r1` naming pattern, each as its own PR.

## Spec (from the Lead's reply)
- **Where it shows:** enemy fighters, seen small and in dozens, on both planets and in the trailer. Today a placeholder from `ArtSource/EngShips/NullFleet/review/`.
- **Style:** Null Armada — black armour, red-orange glowing seams, white glowing eye sensors, chevron shapes; a crescent fighter. Keep on theme with the Warden and Drone.
- **Budget / format:** 6–10k tris, 2K PBR (BC, N, ORM, E), GLB or FBX.
- **Scale / pivot:** ~550 cm long, pivot at the bounding-box centre, nose along +X, +Z up.
- **Rig / anims:** none. Engineering adds sockets.
- **Target:** `Gameplay/Characters/NullFleet/DA_Enemy_Fighter_Visuals`.
- **Lead note:** make sure the red-orange seams and the white 'eye' bar survive in the texture.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SM_NullFighter.glb` and `SM_NullFighter.fbx`: derived, engine-ready copies. Length is
  **550.0 cm** (matched to the brief), overall 550.0 × 502.6 × 170.5 cm (X × Y × Z), pivot at
  the bounding-box centre, +Z up, nose facing +X. **8,926 tris** (inside the 6–10k budget).
  The FBX uses the repo exporter's Apply Unit + FBX Units Scale (`UnitScaleFactor` 100) so
  Unreal should import it at 550 cm with uniform scale 1.0. The GLB stores centimetre numbers
  with `export_yup=false` (see `SOURCE.md` if an importer assumes glTF metres).
- `T_NullFighter_BC.png`, `T_NullFighter_N.png`, `T_NullFighter_ORM.png`, `T_NullFighter_E.png` at 2K.
  N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
- `source/`: the raw Rodin Base Model pack, untouched, plus the Rodin viewer still.
- `contact_sheet.png`: front, side, top, 3/4 and the approved r1 concept pick, rendered in Blender EEVEE.

The red-orange seams and the white eye / nose chevron are present in the 2K emissive and albedo
(see `SOURCE.md` for the honest read: the eye bar is a little softer than the concept, and the
mesh has three rear engines where the concept has two).
