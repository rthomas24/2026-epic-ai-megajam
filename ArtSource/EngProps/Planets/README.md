# Planet kit: Verdance and Cinder from space, plus walkable ground materials

Engineering-made (Ryan's side, Nov 13). Nicholas keeps art direction and may restyle or replace any of it.
Everything is procedural (Blender 5.2 noise baked in Cycles, plus numpy post-processing); no third-party or
AI-generated source assets.

## Unreal assets (`/Game/TriadDrift/Gameplay/Props/Planets/`)

| Asset | What | Use |
|---|---|---|
| `SM_Planet_Sphere` | UV sphere, radius 100 cm at scale 1, 192 x 96 segments, 36,480 tris, equirectangular UVs, simple sphere collision | The planet surface. **Verdance: uniform scale 600** (60 000 cm radius). **Cinder: uniform scale 550** (55 000 cm). |
| `SM_Planet_Atmosphere` | UV sphere, radius 102.5 cm, normals outward, 16,128 tris, no collision | Same transform as the planet (scale 600 / 550). Front faces only, so it disappears once the player is inside the atmosphere. |
| `SM_Planet_Ring` | Flat annulus in the XY plane, inner 140 cm, outer 230 cm (1.4 and 2.3 planet radii at scale 1), 512 tris, UV U = radial, V = around | Verdance only, same transform as the planet (scale 600, so 840 m to 1380 m). Equatorial: tilt the planet actor, not the ring. |
| `M_TD_Planet` | **Unlit** opaque surface that lights itself from `SunDirection` (Nov 13 night-side pass): BC / N / ORM / E + cloud layer panning in U, wrapped-Lambert day side, dim ambient night side, emissive glow only on the night side, day-side limb in-scatter | parent of `MI_Planet_*` |
| `MI_Planet_Verdance`, `MI_Planet_Cinder` | instances | on `SM_Planet_Sphere` |
| `M_TD_PlanetAtmosphere` | Unlit, additive, fresnel rim glow, bright at the limb, transparent centre, dimmer on the night side | parent of `MI_PlanetAtmo_*` |
| `MI_PlanetAtmo_Verdance` (cyan-green), `MI_PlanetAtmo_Cinder` (orange) | instances | on `SM_Planet_Atmosphere` |
| `M_TD_PlanetRing`, `MI_PlanetRing_Verdance` | Unlit translucent, two-sided, banded pale gold and teal | on `SM_Planet_Ring` |
| `M_TD_Ground` | Lit opaque, world-aligned UVs (no mesh UVs needed), tile every `TileSize` (400 cm), anti-repetition macro layer | parent of `MI_Ground_*` |
| `MI_Ground_Verdance`, `MI_Ground_Cinder` | instances | on the 6 km planet ground slab (any flat mesh, any scale) |
| `T_Planet_{Verdance,Cinder}_{BC,N,ORM,E,Clouds}` | 4096 x 2048 equirectangular | BC/E sRGB; N DirectX normal map (no green flip); ORM and Clouds linear masks (Clouds in R) |
| `T_PlanetRing_Verdance` | 2048 x 16 RGBA (alpha = ring density) | |
| `T_Ground_{Verdance,Cinder}_{BC,N,ORM,E}` | 2048 x 2048 tileable (one tile = 4 m) | |

The meshes' slot 0 defaults to the Verdance instances; set `MI_Planet_Cinder` / `MI_PlanetAtmo_Cinder` on the
Cinder actor's components.

## Material parameters worth knowing

`M_TD_Planet`
- **`CloudPanSpeed`** (scalar, UV units per second along U, default 0.0015 Verdance / 0.0010 Cinder): the cloud layer
  is offset by `frac(Time * CloudPanSpeed)` in U, so 0.0015 is one full lap in about 11 minutes. 0 freezes the clouds.
- `CloudOpacity`, `CloudColor` (Verdance white, Cinder ash grey), `EmissiveStrength` (6 / 8; tune in the level),
  `SurfaceRimColor`, `SurfaceRimStrength`.
- **`SunDirection`** (vector, direction from the planet TOWARDS the sun, world space; `ATDPlanetAtmosphere` sets it
  on the surface from the atmosphere-sun directional light). The surface is unlit on purpose: the space light rig's
  violet fill shines from the anti-sun side and the sky light adds more, which lit the night side of the old lit
  material almost like the day side. Now:
  `albedo * (SunColor * SunIntensity * wrapLambert * AO + AmbientColor * NightAmbient) + glow + rim`.
  `SunIntensity` (3.0 / 3.6), `SunColor`, `NightAmbient` (0.10 / 0.30, the night side's dim fill, ~3-5% of the day
  side), `AmbientColor`, `TerminatorSoftness` (0.12, wrap of the terminator).
- **Night glow**: `EmissiveTex * EmissiveStrength (4.5 / 9) * lerp(1, night, NightGlowMask) * (1 - clouds * CloudGlowOcclusion)`;
  `night` ramps from 0 at `dot(N, Sun) = NightGlowStart` (0.05) to 1 over `1 / NightGlowSharpness` (4). `NightGlowMask`
  1 = glow only on the night side (0 brings back glow everywhere).
- Graph source: `scripts/ue_planet_surface.py` (values per planet in `INSTANCE_VALUES`). To change it on the existing
  asset, run `scripts/ue_rebuild_planet_surface.py` with a renderer (`ArtSource/EngShips/Dominion/scripts/run_ue_rendered.ps1`);
  it rebuilds the master in place and keeps the instances. Review renders: `scripts/ue_render_planet_review.py`
  (`PL_REVIEW_TAG`), written to `Saved/PlanetNight/<tag>/`; delete the throwaway `Planets/Review/` level after it.
- The atmosphere instances have their own `SunDirection` (default = the baked sky sun (0.779, 0.546, 0.309));
  `SunMaskAmount` 0.92 (night limb at 8% of the lit limb).

`M_TD_PlanetAtmosphere`: `AtmoColor`, `AtmoIntensity`, `RimPower` (inner falloff), `EdgeSoftness` (outer fade,
4.5 matches the 1.025 shell), `SunMaskAmount`, `SunDirection`.

`M_TD_PlanetRing`: `RingTint`, `Brightness`, `Opacity`.

`M_TD_Ground`: `TileSize` (cm, 400), `BScale` (second detail sample at 1.37 x the tile with an offset; the two are
blended by a macro mask so neighbouring tiles differ), `MacroScale1` / `MacroScale2` (7.3 and 31.7 tiles, rotated
37 and -61 degrees, sampled at a forced mip 6), `MacroMean` / `MacroK` (normalisation measured from the texture;
written by the import script from `textures/T_Ground_*_stats.json`), `MacroStrength` (brightness variation),
`BlendContrast`, `MacroTintA` / `MacroTintB`, `GlowMacro` (how much the macro layer switches glow on and off over
tens of metres), `EmissiveStrength`, `AOToBaseColor`. World-aligned in X/Y, so it is meant for flat or gently sloped
ground.

## Source and rebuild

```
ArtSource/EngProps/Planets/
  scripts/pl_lib.py            node builder, Cycles EMIT bake, normal-from-height, PNG writer
  scripts/planet_graphs.py     the two planet surfaces as shader graphs
  scripts/build_planets.py     meshes + FBX, 4096 x 2048 bakes, ring texture, review renders
  scripts/build_ground.py      tileable 2048 ground sets (4D flat-torus noise), eye-height review renders
  scripts/ue_import_planets.py Unreal import, masters, instances (PL_REBUILD=1 rebuilds the masters)
  meshes/  textures/  review/  Planets_meshes.blend
```
```
blender --background --factory-startup --python ArtSource/EngProps/Planets/scripts/build_planets.py -- --stage all --planet all --width 4096 --tag final
blender --background --factory-startup --python ArtSource/EngProps/Planets/scripts/build_ground.py -- --size 2048 --tag final
powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_import_planets.py
```
Seamless by construction: planet noise is evaluated on the 3D unit-sphere position (no seam at the wrap, no
pinching at the poles; the baked normal map fades to flat within ~7 degrees of each pole where texels stretch);
ground noise is evaluated on a 4D flat torus, so every layer tiles (`seam_diff` vs `interior_diff` in the stats
JSON are equal).

## Review renders (`review/`)
`Verdance_final_{1_disc,2_limb,3_grazing}.png`, `Cinder_final_{...}.png`, `Ground_{Verdance,Cinder}_final_{eye,down}.png`
(Blender EEVEE preview materials that reproduce the Unreal graphs; earlier iterations kept as `*_a*`, `*_b1`).
