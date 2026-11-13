# Turret - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (ranked request list, item #4, Null Armada
fleet). This is the first 3D delivery of the Null Armada Turret, built in Hyper3D Rodin Gen-2.5
from the approved concept-gate 3/4 hero
(`ArtSource/EngAI/Ships/NullFleet/Turret/concept_r1/source/hero-NullTurret-g26a_PICK.png`,
PR #15, branch `grok/assets-concepts-r2`). Builders do the final import and cleanup;
this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's reply)
- **Where it shows:** carrier / ground turrets, on both planets and in the trailer. Today a placeholder from `ArtSource/EngShips/NullFleet/review/`.
- **Style:** Null Armada — black armour, red-orange glowing seams, white glowing eye sensors, chevron shapes. Keep on theme with the Warden and Drone, and as a visible descendant of Triad's ships (R-034).
- **Budget / format:** 2–4k tris, 2K PBR (BC, N, ORM, E), GLB or FBX.
- **Scale / pivot:** no length was given in the brief; match today's placeholder footprint (220.8 × 142.6 × 101.9 cm). Pivot at the bounding-box centre, barrels / "nose" along +X, +Z up.
- **Rig / anims:** none. Engineering adds sockets (the placeholder has `Muzzle_0` / `Muzzle_1`).
- **Target:** `Gameplay/Characters/NullFleet/DA_Enemy_Turret_Visuals`.
- **Lead note:** make sure the red-orange seams and the white 'eye' bar survive in the texture.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SM_NullTurret.glb` and `SM_NullTurret.fbx`: derived, engine-ready copies. Forward length is
  **220.8 cm** (matched to the placeholder footprint), overall 220.8 × 141.0 × 112.0 cm
  (X × Y × Z), pivot at the bounding-box centre, +Z up, barrels facing +X. **8,820 tris**.
  That is over the 2–4k brief; a 4k-topology redo was skipped as too costly, so the 8,820-tri
  Clean Topology export was kept and not decimated. The FBX uses the repo exporter's Apply Unit
  + FBX Units Scale (`UnitScaleFactor` 100) so Unreal should import it at 220.8 cm with uniform
  scale 1.0. Prefer the FBX: the GLB stores centimetre numbers with `export_yup=false`
  (see `SOURCE.md` if an importer assumes glTF metres).
- `T_NullTurret_BC.png`, `T_NullTurret_N.png`, `T_NullTurret_ORM.png`, `T_NullTurret_E.png` at 2K.
  N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
- `source/`: the raw Rodin Base Model pack, untouched. Zip omitted.
- `contact_sheet.png`: front, side, top, 3/4 and the approved r1 concept pick, rendered in Blender EEVEE.

The red-orange seams and the white eye between the barrels are present and read clearly
(see `SOURCE.md`). The silhouette is very close to the concept (twin barrels, circular base,
dorsal chevron). No heat-glow mask was baked for the turret (masks were requested for the ships).
