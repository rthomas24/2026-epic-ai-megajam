# Interceptor - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (ranked request list, item #4, Null Armada
fleet). This is the first 3D delivery of the Null Armada Interceptor, built in Hyper3D Rodin Gen-2.5
from the approved concept-gate 3/4 hero
(`ArtSource/EngAI/Ships/NullFleet/Interceptor/concept_r1/source/hero-NullInterceptor-g26a_PICK.png`,
PR #15, branch `grok/assets-concepts-r2`). Builders do the final import and cleanup;
this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's reply)
- **Where it shows:** enemy interceptors, on both planets and in the trailer. Today a placeholder from `ArtSource/EngShips/NullFleet/review/`.
- **Style:** Null Armada — black armour, red-orange glowing seams, white glowing eye sensors, chevron shapes; a hooked interceptor. Keep on theme with the Warden and Drone, and as a visible descendant of Triad's ships (R-034).
- **Budget / format:** 6–10k tris, 2K PBR (BC, N, ORM, E), GLB or FBX.
- **Scale / pivot:** ~660 cm long, pivot at the bounding-box centre, nose along +X, +Z up.
- **Rig / anims:** none. Engineering adds sockets.
- **Target:** `Gameplay/Characters/NullFleet/DA_Enemy_Interceptor_Visuals`.
- **Lead note:** make sure the red-orange seams and the white 'eye' bar survive in the texture.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SM_NullInterceptor.glb` and `SM_NullInterceptor.fbx`: derived, engine-ready copies. Length is
  **660.0 cm** (matched to the brief), overall 660.0 × 535.3 × 248.0 cm (X × Y × Z), pivot at
  the bounding-box centre, +Z up, nose facing +X. **8,974 tris** (inside the 6–10k budget).
  The FBX uses the repo exporter's Apply Unit + FBX Units Scale (`UnitScaleFactor` 100) so
  Unreal should import it at 660 cm with uniform scale 1.0. The GLB stores centimetre numbers
  with `export_yup=false` (see `SOURCE.md` if an importer assumes glTF metres).
- **No `T_NullInterceptor_{BC,N,ORM,E}.png`.** The Rodin download (and the GLB itself) has
  no PBR maps — only the mesh and a UV layout. The contact sheet is rendered as clay for
  that reason. The red-orange seams and the white eye bar cannot be checked until a
  material pack is downloaded or regenerated. See `SOURCE.md`.
- `source/`: the raw Rodin download, untouched (`base.glb`, `base.fbx`, viewer still). Zip omitted.
- `contact_sheet.png`: front, side, top, 3/4 (clay) and the approved r1 concept pick, rendered in Blender EEVEE.

The silhouette is faithful (hooked wings, pointed nose, eye socket, chin guns, single rear
engine, tail fin). Paint, seams and the eye disc are missing from this drop.
