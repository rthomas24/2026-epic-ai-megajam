# Energy blade (SM_TD_BladeHilt, SM_TD_BladeBeam, M_TD_BladeBeam)

Engineering-made blade for the on-foot sections (Ryan's side, Nov 13). Original design. Nicholas keeps art
direction: restyle or replace freely. Built by `scripts/build_blade.py` (seeded, reproducible):
`blender --background --factory-startup --python scripts/build_blade.py -- --tag r2`.

## Files

| What | Path |
|---|---|
| Hilt .blend / .fbx / manifest / baked 1K set (`T_TD_BladeHilt_BC`, `_N` DirectX, `_ORM`, `_E`) | `ArtSource/EngProps/Blade/SM_TD_BladeHilt/` |
| Beam .blend / .fbx / manifest | `ArtSource/EngProps/Blade/SM_TD_BladeBeam/` |
| Hilt + beam against black and mid grey (wide + close) | `review/Blade_r2_black_grey.png` |
| Hilt material sheet + game-distance strip, clay sheet | `review/SM_TD_BladeHilt_r2_review.png`, `review/SM_TD_BladeHilt_r2_clay_sheet.png` |
| Health (asset_report, assembly, UVs, bake stats) | `review/SM_TD_BladeHilt_health.json`, `review/SM_TD_BladeBeam_health.json` |
| Scripts | `scripts/build_blade.py`, `scripts/eng_finish.py`, `scripts/eng_lib.py` (snapshot of SpaceDressing's `sd_lib.py`, Nov 13) |

Unreal (`/Game/TriadDrift/Gameplay/Props/Blade/`): `SM_TD_BladeHilt`, `T_TD_BladeHilt_BC/N/ORM/E`,
`MI_TD_BladeHilt` (child of `/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured`), `SM_TD_BladeBeam`,
`M_TD_BladeBeam`, `MI_TD_BladeBeam`. Import: `ArtSource/EngProps/Sky/scripts/ue_import_sky_blade_pad.py`.

## SM_TD_BladeHilt

* 27.8 cm long along **+X**, 4.9 cm tall, 7.2 cm across the collar fins (Y). 7,392 tris, one material slot,
  not Nanite, one UCX convex hull.
* **Pivot = grip centre** (where the right hand closes): pommel at X = -8.5 cm, emitter face at X = +19.3 cm.
  The activation slide and its cyan pip are on top (+Z), at the thumb, just behind the collar.
* **Socket `SOCKET_BladeEmitter`** at (19.3, 0, 0) cm, X pointing along the blade: attach the beam there.
* Parts: domed pommel with a cyan band; ribbed rubber grip (8 rounded ribs, steel ferrules); flared guard
  collar in dark steel-blue with a cyan band and two swept fins; steel neck with three vent rings; steel-blue
  emitter head; recessed lens housing, glowing lens and a thin cyan emitter ring.
* Palette: gunmetal `#3A414C` (60%), dark steel-blue `#2E4A5E` (30%), steel and cyan `#3FE9FF` accents.
  Edge wear on chamfers, baked AO cavity tint.
* Texture set 1024 (about 3,600 px/m): BC sRGB, N DirectX (do not flip green), ORM linear (AO / roughness /
  metallic), E sRGB. MI EmissiveStrength = 2 (bake) x 8 = 16.
* Checks: asset_report 0 errors / 0 warnings (4 planar n-gons, triangulated on export), no floating parts,
  UVs inside 0-1 with 0 overlap pixels, export ok.

## SM_TD_BladeBeam

* **Origin at the emitter**, 100 cm along **+X**, 4.0 cm wide (Unreal Y) x 2.0 cm thick (Z): a lens-shaped
  cross-section that swells just above the emitter and tapers to a point. 576 tris, a shell with a fan at each
  end (its UV seam shows as open edges until the exporter welds positions), no collision, not Nanite (translucent).
* UV0: **U = 0 at the emitter -> 1 at the tip** (drives the tip fade), V = around the blade.
* asset_report: 0 errors, 1 warning (71 duplicate vertices = intentional UV seam / end-fan copies, welded on export).

## M_TD_BladeBeam / MI_TD_BladeBeam

Unlit, **Additive**, single-sided. Emissive =
`lerp(BladeColor, white, (1 - Fresnel)^CoreSharpness) * lerp(EdgeBrightness, 1, 1 - Fresnel) * saturate((1 - U) * 7) * Intensity`
so the face toward the camera is a white-hot core that fades to the blade colour and dims at the silhouette
(the two-layer look), and the last ~14% of the blade fades out at the tip.

| Parameter | Default | Notes |
|---|---|---|
| `BladeColor` (vector) | 0.1, 0.65, 1.0 (cyan) | squad / ability colour |
| `Intensity` (scalar) | 12 | raise for more bloom |
| `CoreSharpness` (scalar) | 2.5 | higher = thinner white core |
| `EdgeBrightness` (scalar) | 0.3 | brightness at the silhouette |

Fresnel exponent 2 (constant). Blender's preview uses the same idea (Layer Weight facing), viewed through AgX.

## Honest status (AAA.md stages)

* **Prototype complete + technically verified in Blender** (two review rounds; r1 had the beam mounted at the
  pivot instead of the socket and a nearly black hilt, fixed in r2).
* Rubric self-assessment (not an independent review): silhouette 3 (a lathed hilt; the fins and collar give
  it a read but it is still a cylinder), proportion 3-4, detail 3, construction 4, edges 3-4, materials 3-4,
  style fit 4, technical 4. Below the 4-everywhere bar for a hero first-person prop: **the hilt needs an
  art pass** (more asymmetric hard-surface detail, a grip wrap or panel inlays) if it is seen up close.
* Emissive covers about 11.5% of the hilt's surface (the lens face and three bands), above the 5% guideline.
* Unreal: imported headless; the beam material cannot be compiled headless, so it is **not yet seen rendering**
  in the editor or game. Not presentation approved, not attached to a character, not played.
