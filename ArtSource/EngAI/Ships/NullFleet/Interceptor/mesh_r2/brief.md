# Interceptor - mesh_r2 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (ranked request list, item #4, Null Armada
fleet). This is a textured follow-up to the untextured Interceptor `mesh_r1` (PR #34). It is a
**new round folder**: `mesh_r1` was not overwritten. The r2 mesh is a new Rodin export
(8,589 tris) with a Native / High 2K PBR material, built from the approved concept-gate 3/4 hero
(`ArtSource/EngAI/Ships/NullFleet/Interceptor/concept_r1/source/hero-NullInterceptor-g26a_PICK.png`,
PR #15, branch `grok/assets-concepts-r2`). Builders do the final import and cleanup;
this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's reply)
- **Where it shows:** enemy interceptors, seen small and in groups, on both planets and in the trailer. Today a placeholder from `ArtSource/EngShips/NullFleet/review/`.
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
  **660.0 cm** (matched to the brief), overall 660.0 × 632.9 × 220.9 cm (X × Y × Z), pivot at
  the bounding-box centre, +Z up, nose facing +X. **8,589 tris** (inside the 6–10k budget).
  This is a different mesh from `mesh_r1` (8,974 tris, 660.0 × 535.3 × 248.0 cm, no maps).
  The FBX uses the repo exporter's Apply Unit + FBX Units Scale (`UnitScaleFactor` 100) so
  Unreal should import it at 660 cm with uniform scale 1.0. Prefer the FBX: the GLB stores
  centimetre numbers with `export_yup=false` (see `SOURCE.md` if an importer assumes glTF metres).
- `T_NullInterceptor_BC.png`, `T_NullInterceptor_N.png`, `T_NullInterceptor_ORM.png`, `T_NullInterceptor_E.png` at 2K.
  N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
- `source/`: the raw Rodin Base Model pack for this material / r2 mesh, untouched. Zip omitted.
- `contact_sheet.png`: front, side, top, 3/4 and the approved r1 concept pick, rendered in Blender EEVEE.

The red-orange seams survive clearly. The white eye is present but much dimmer and smaller than
the concept's bright disc (see `SOURCE.md`). The hooked wings, chin guns and tail are on theme;
the mesh has two rear engines where the concept (and the r1 clay mesh) has one, and the nose is
a little blunter. A matching heat-glow mask, baked into this r2 UV layout, is in `../heatmask_r1/`.
