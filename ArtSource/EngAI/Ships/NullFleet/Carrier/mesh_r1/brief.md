# Carrier - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (ranked request list, item #4, Null Armada
fleet). This is the first 3D delivery of the Null Armada Carrier, built in Hyper3D Rodin Gen-2.5
from the approved concept-gate 3/4 hero
(`ArtSource/EngAI/Ships/NullFleet/Carrier/concept_r1/source/hero-NullCarrier-g26a_PICK.png`,
PR #15, branch `grok/assets-concepts-r2`). Builders do the final import and cleanup;
this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's reply)
- **Where it shows:** the enemy capital / carrier, on both planets and in the trailer. Today a placeholder from `ArtSource/EngShips/NullFleet/review/`.
- **Style:** Null Armada — black armour, red-orange glowing seams, white glowing eye sensors, chevron shapes; a horned carrier. Keep on theme with the Warden and Drone, and as a visible descendant of Triad's ships (R-034).
- **Budget / format:** 30–60k tris (Nanite), 2K PBR (BC, N, ORM, E); the carrier may use 2 × 2K sets, GLB or FBX.
- **Scale / pivot:** ~4600 cm long, pivot at the bounding-box centre, nose along +X, +Z up.
- **Rig / anims:** none. Engineering adds sockets.
- **Target:** `Gameplay/Characters/NullFleet/DA_Enemy_Carrier_Visuals`.
- **Lead note:** make sure the red-orange seams and the white 'eye' bar survive in the texture.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SM_NullCarrier.glb` and `SM_NullCarrier.fbx`: derived, engine-ready copies. Length is
  **4600.0 cm** (matched to the brief), overall 4600.0 × 2276.4 × 1785.2 cm (X × Y × Z), pivot at
  the bounding-box centre, +Z up, nose facing +X. **8,906 tris**. A 50K Raw Rodin setting was
  requested, but the downloaded pack is 8,906 tris (no higher-poly file in the download). It was
  kept as shipped and was not decimated; that is under the 30–60k Nanite brief. The FBX uses the
  repo exporter's Apply Unit + FBX Units Scale (`UnitScaleFactor` 100) so Unreal should import
  it at 4600 cm with uniform scale 1.0. Prefer the FBX: the GLB stores centimetre numbers with
  `export_yup=false` (see `SOURCE.md` if an importer assumes glTF metres). One 2K PBR set (not 2 × 2K).
- `T_NullCarrier_BC.png`, `T_NullCarrier_N.png`, `T_NullCarrier_ORM.png`, `T_NullCarrier_E.png` at 2K.
  N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
- `source/`: the raw Rodin Base Model pack, untouched. Zip omitted.
- `contact_sheet.png`: front, side, top, 3/4 and the approved r1 concept pick, rendered in Blender EEVEE.

The red-orange seams, hangar-bay glow and the white eye between the horns are present
(see `SOURCE.md`). The silhouette is faithful (forward horns, side hangars, tower, outrigger
pods). The tower is a little shorter than the concept, and at 9k tris some hangar and plating
detail is softer than a 50k mesh would have been. A matching heat-glow mask is in `../heatmask_r1/`.
