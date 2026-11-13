# Landing pad (SM_LandingPad)

Engineering-made planet landing pad (Ryan's side, Nov 13). Original design. Nicholas keeps art direction:
restyle or replace freely. Built by `scripts/build_pad.py` (seeded, reproducible; uses the blade folder's
`eng_finish.py` / `eng_lib.py`): `blender --background --factory-startup --python scripts/build_pad.py -- --tag r1`.

## Files

| What | Path |
|---|---|
| .blend / .fbx / manifest / baked 2048 set (`T_LandingPad_BC`, `_N` DirectX, `_ORM`, `_E`) | `ArtSource/EngProps/Pad/SM_LandingPad/` |
| Standing eye height (1.7 m, two views), straight down from above, approach view | `review/SM_LandingPad_r1_eye_above.png` |
| Material sheet + game-distance strip, clay sheet | `review/SM_LandingPad_r1_review.png`, `review/SM_LandingPad_r1_clay_sheet.png` |
| Health (asset_report, assembly, UVs, bake stats) | `review/SM_LandingPad_health.json` |

Unreal (`/Game/TriadDrift/Gameplay/Props/Planet/`): `SM_LandingPad`, `T_LandingPad_BC/N/ORM/E`, `MI_LandingPad`
(child of `/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured`). Import:
`ArtSource/EngProps/Sky/scripts/ue_import_sky_blade_pad.py`.

## Size, pivot, collision

* Deck diameter **1000 cm**, height **40 cm**, **flat walkable top at Z = 40 cm**, origin at the **centre of
  the base** (Z = 0). The rim lights stand 0.75 cm proud of the deck at the very edge (max Z 40.75 cm).
* With the buttress feet and the two step blocks the footprint is 1074 x 1000 cm (the steps are on Unreal +-Y,
  50 cm deep, 140 cm wide, 20 cm high: a 20 + 20 cm step-up for the on-foot character).
* 8,024 tris, one material slot, not Nanite. Collision: three UCX convex hulls (the deck cylinder and the
  two steps); the small feet have none.

## Look

* Dark gunmetal centre field (r < 3 m) with the landing mark in warm off-white paint: an original target
  (rings at 1.05-1.20 m and 0.28-0.36 m, four bars, a centre dot) and a dashed guide ring at 2.55 m.
* Warm-grey outer deck (3.0-4.4 m) with 48 radial panel seams; 24 radial seams in the centre field;
  concentric seams at 1.4, 3.0, 4.40 and 4.76 m.
* Hazard band (diagonal amber `#F2A01E` / near-black stripes) at 4.42-4.74 m, then a chamfered rim with
  **24 cyan rim lights** (steel housings, emissive lenses) and a dark gunmetal skirt.
* Edge wear on the chamfers, scuffs and grime on the deck, worn paint on the mark, baked AO cavity tint.
* 2048 set (about 200 px/m on the deck). MI EmissiveStrength = 2 (bake) x 8 = 16.

## Checks

asset_report 0 errors / 0 warnings (16 planar n-gons, triangulated on export), no floating parts, UVs inside
0-1 with 0 overlap pixels, export ok. Emissive area 0.4% (the lenses).

## Honest status (AAA.md stages)

* **Prototype complete + technically verified in Blender**, one review round (looked at from standing eye
  height, close, above and on approach).
* Rubric self-assessment (not an independent review): silhouette 3, proportion 4, detail 4, construction 4,
  edges 4, materials 4, style fit 4, technical 4. Weakest point: the rim lights are small and read weakly in
  daylight in Blender; they depend on Unreal's emissive and bloom.
* Unreal: imported headless; **not yet placed on Verdance or Cinder, not seen in the running game**, not
  walked on, not presentation approved.
