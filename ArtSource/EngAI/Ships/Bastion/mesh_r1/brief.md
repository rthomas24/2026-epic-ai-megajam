# Bastion - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-027 (ranked request list, item #1).
This is the first 3D delivery of the player ship **Bastion**, built in Hyper3D Rodin Gen-2.5
from the approved concept-gate r2 3/4 hero (`ArtSource/EngAI/Ships/Bastion/concept_r2/source/hero-Bastion-g26a_PICK.png`,
PR #15, branch `grok/assets-concepts-r2`). Builders do the final import and cleanup;
this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's reply)
- **Where it shows:** centre screen all game, intro, trailer.
- **Style:** Heavy gunship: wide hull, four engine pods, missile pods, dome emitter on top. White hull with amber accents. Squad colour on ~30% of the hull. Bold readable silhouette.
- **Budget / format:** 40–60k tris (Nanite), 2K PBR (BC, N, ORM, E), GLB or FBX (FBX preferred for Unreal).
- **Scale / pivot:** brief ~524 × 488 × 145 cm (L × W × H); delivery is
  524.0 × 472.3 × 139.1 cm (length matched; width -3.2%, height -4.1%).
  Pivot at the bounding-box centre, nose along +X, +Z up.
- **Rig / anims:** none; engineering adds sockets (`Muzzle_N`, `Engine`, `Engine_N`, ability points).
- **Target:** `Gameplay/Characters/Squad/DA_Bastion_Visuals.Mesh`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SM_Bastion.fbx` (preferred) and `SM_Bastion.glb`: derived, engine-ready copies.
  524.0 × 472.3 × 139.1 cm (X × Y × Z), pivot at the bounding-box centre,
  +Z up, nose facing +X. 50,000 tris (inside budget). The FBX uses the repo exporter's Apply Unit
  + FBX Units Scale (`UnitScaleFactor` 100) so Unreal should import it at 524.0 cm with
  uniform scale 1.0. The GLB stores centimetre numbers with `export_yup=false` (see `SOURCE.md` if
  an importer assumes glTF metres).
- `T_Bastion_BC.png`, `T_Bastion_N.png`, `T_Bastion_ORM.png`, `T_Bastion_E.png` at 2K.
  N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
- `source/`: the raw Rodin Base Model pack, untouched.
- `contact_sheet.png`: front, side, top, 3/4 and the approved r2 concept pick, rendered in Blender EEVEE.

