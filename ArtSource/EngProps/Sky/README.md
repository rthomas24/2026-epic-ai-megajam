# Nebula sky (T_TD_NebulaSky, SM_TD_SkySphere, M_TD_NebulaSky)

Engineering-made sky for the space level (Ryan's side, Nov 13). Nicholas keeps art direction: restyle or
replace freely. Everything is procedural and seeded; re-running the scripts reproduces it exactly.

## Files

| What | Path |
|---|---|
| HDR sky, 8192 x 4096, equirectangular, linear half-float EXR (PIZ) | `ArtSource/EngProps/Sky/T_TD_NebulaSky.exr` |
| Same, 8-bit sRGB PNG (soft shoulder above 0.72 linear) | `ArtSource/EngProps/Sky/T_TD_NebulaSky.png` |
| Blender source scene of the bake (world shader, gas giant, rings, Sun lamp, pano camera) | `ArtSource/EngProps/Sky/NebulaSky_r2.blend` |
| Render stats (sun check, darkness shares) | `ArtSource/EngProps/Sky/sky_stats_r2.json` |
| Inverted sky sphere: .blend, .fbx, manifest | `ArtSource/EngProps/Sky/SM_TD_SkySphere/` |
| Review: six 90-degree views from inside the sphere | `ArtSource/EngProps/Sky/review/T_TD_NebulaSky_r2_inside_views.png` (r1 = previous round) |
| Sphere health (asset_report) | `ArtSource/EngProps/Sky/review/SM_TD_SkySphere_health.json` |
| Scripts | `scripts/build_sky.py` (bake), `scripts/sky_sphere_review.py` (sphere + review), `scripts/probe_equirect.py` (mapping proof), `scripts/ue_import_sky_blade_pad.py` (Unreal import) |

Unreal (`/Game/TriadDrift/Gameplay/Props/Sky/`): `T_TD_NebulaSky`, `SM_TD_SkySphere`, `M_TD_NebulaSky`, `MI_TD_NebulaSky`.

## Sun direction (match the level's directional light to this)

* **The sun sits at Unreal Yaw 35, Pitch +18** (yaw measured from +X toward +Y, pitch up from the horizon).
  As a direction vector in Unreal space: (0.779, 0.546, 0.309).
* A **Directional Light** that comes from that sun points the opposite way: **Rotation Pitch -18, Yaw 215, Roll 0**.
* Verified: the brightest texel of the baked map is at Yaw 35.0, Pitch 18.0 (`sky_stats_r2.json`), and the
  review views show the sun up and to the right of the Yaw 0 view.
* This holds when the sky sphere actor has **zero rotation**. Rotating the actor in yaw by Y moves the sun to
  Yaw 35 + Y.

## What is in the sky

* Deep background: very dark indigo (linear ~0.002-0.01), never pure black, with faint wide veils.
* Three nebula complexes, domain-warped fBm with ridged filaments, bright rims and dark dust lanes:
  magenta/pink (Yaw -18, Pitch 6, with a violet lobe at Yaw -52, Pitch 30), teal/cyan (Yaw 166, Pitch -10),
  violet with a gold core (Yaw 78, Pitch 46). The lower hemisphere and Yaw -90..-150 stay dark for readability
  (about 85% of the sky is below 0.05 linear).
* Stars: four Voronoi layers (about 50k faint, 4k medium, 200 bright), colours from warm orange to blue-white,
  brightness power law, denser along a faint galactic band and in the nebulae, dimmed by dust. The brightest
  get a soft glow and a short cross flare (post-process, latitude-corrected so they stay round on the sphere).
* A banded, ringed gas giant (original design): peach / rust / cream bands, a storm oval, pale ice-tan rings
  with a gap, at Yaw 126, Pitch 4, 10 degrees angular radius, lit by the sun (terminator and ring shadow agree
  with the sun direction).
* The sun: a 0.6-degree disc (linear 60) with a three-layer warm corona.

## Mapping

Equirectangular, `u = Yaw / 360 + 0.5`, `v = 0.5 - Pitch / 180` (v = 0 is straight up), Unreal axes.
The seam is at Yaw +-180 (behind -X). Proven by `probe_equirect.py` and by the inside-sphere review, which
samples the texture through `SM_TD_SkySphere`'s own UVs.

## SM_TD_SkySphere

* UV sphere, radius **100 cm** at scale 1, 128 x 64 segments, 16,128 tris, **all 8,192 faces point inward**
  (checked), smooth normals, pivot at the centre, no collision, one material slot.
* UV0 carries the equirectangular mapping above (seam vertices duplicated, a separate pole vertex per segment).
* asset_report: 0 errors, 1 warning (317 duplicate vertices = the intentional UV seam and pole copies;
  the exporter welds positions, the UV split stays), info: open boundary = the seam before welding.

## M_TD_NebulaSky / MI_TD_NebulaSky

* Unlit, opaque, **Is Sky = true**, single-sided (the faces point inward). Emissive = `SkyTex * Tint * Brightness`.
* Parameters: `SkyTex` (texture, default T_TD_NebulaSky), `Tint` (vector, default 1,1,1), `Brightness`
  (scalar, default 1.0). Expect to raise Brightness (2-8) to taste against the level's exposure; the sun texels
  are ~60, so bloom picks them up.
* Texture settings: HDR compressed (BC6H), sRGB off, **no mipmaps** (no pole / seam mip artefacts; the sky
  is seen at roughly one texel per pixel at a 90-degree FOV in 1080p), Skybox texture group, never streamed,
  wrap U, clamp V. Sampler type Linear Color.

### How to place it (depth)

The material does not try to disable depth testing (an opaque unlit sky cannot, and translucent skies sort
badly). It is a background by distance and by the Is Sky flag:
1. Drop `SM_TD_SkySphere` at the world origin with **scale 50000** or more (radius 5 km+, beyond all gameplay;
   UE's far plane is infinite, so any size is fine). Rotation 0.
2. On the component: Cast Shadow **off**, Collision **NoCollision**, Affect Distance Field Lighting off,
   Visible in Ray Tracing off is fine.
3. Is Sky lets a Sky Light with **Real Time Capture** pick the nebula up for ambient light, and Unreal
   treats the mesh as sky (not fogged as a surface, rendered as the background).
4. If the level has Exponential Height Fog or a SkyAtmosphere, turn the atmosphere off in space.

## Honest status (AAA.md stages)

* **Prototype complete + technically verified in Blender**: mapping proven numerically and visually, sun
  position measured, sphere normals and UVs checked, review views looked at in three rounds.
* **Unreal**: imported headless by `scripts/ue_import_sky_blade_pad.py` (TDPY: OK): texture 8192 x 4096, sphere
  radius 100.0 cm, Is Sky = true, MI assigned. `T_TD_NebulaSky.uasset` is 97 MB on disk (it keeps the source
  EXR); cooked BC6H without mips is about 32 MB. The material graph cannot be compiled headless, so it has
  **not been seen rendering in the editor or game**. Not presentation approved (Nicholas), not integrated
  into a level, not played.
