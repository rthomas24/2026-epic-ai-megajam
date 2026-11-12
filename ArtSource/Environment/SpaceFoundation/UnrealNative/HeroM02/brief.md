# Hero M02 / cinematic realism benchmark

Owner: Nicholas Tosti / Environment & VFX Agent. Branch: nick/environment-vfx.
Task-specific claim published by Lead at 85119ed; reviewed workspace a01e9e7.
Only this HeroM02 source folder and matching native HeroM02 subtree are writable.
All M01 assets, source, settings, Combat/Verdance presets and shared maps are read-only.

Use the approved cyan/violet palette with natural gas/dust structure. Keep the
original broad cloud banks and dark combat corridor, but replace smooth gradients
with multiscale density, eroded boundaries and sparse filaments. Bake three optical
depth strata with approximate emission, absorption and localized illumination into
two colour/optical-depth textures. This is a light-transport approximation, not a
calibrated spectral simulation or a claim of physically exact multi-scattering.
No runtime volumetric simulation. Real astronomical clouds should not visibly
slide during a few hundred metres of camera translation; depth comes from light
and occlusion, not exaggerated near-field parallax.

Stars: original deterministic directions, varied apparent radiance and blackbody
temperature approximations, angular Gaussian kernels in HDR source, then native
TextureCube sampling. Angular distances and cube sampling remove the equirectangular
zenith distortion. No Earth-atmosphere twinkle or telescope-specific diffraction spikes.

Quality benchmark: 8K background, foreground capped at 4K and stars capped at
1024 per cube face. Three texture samplers, one opaque sky draw, no particles,
raymarch or actor tick. Report texture memory tradeoff; 4K background is a possible
later scalability decision, not a change to the untouched other presets.

Before/after: original Hero and additive Hero M02 in one isolated owned scene,
same 1920x1080 target, camera position (0,0,0), pitch10/yaw125/roll0, 90-degree FOV,
manual EV0-style exposure (physical camera exposure disabled), bloom0.2, gamma2.2,
same sunlight8lux and temperature. Include identical 50-degree detail and near-zenith
pairs. Store actual transform/exposure metadata with the captures. Camera and
sunlight use named Rotator fields.

Measure synchronized scene-capture/readback wall time with warmups and samples;
label it as including GPU readback/driver overhead, never gameplay FPS or isolated
GPU timing. Compare shader counters and configured compressed-mip memory budgets.
Check a separate native reload, texture readiness, static repeated captures and
baseline hashes. Gameplay testing on shared L_Galaxy remains a separate map-owner
step after visual approval; this task neither edits nor activates that map.

NASA references (viewed for structure, no pixel/texture sampling or downloads):
- [Webb Cosmic Cliffs](https://science.nasa.gov/asset/webb/cosmic-cliffs-in-the-carina-nebula-nircam-image/): eroded gas boundaries, cavities and dust pillars.
- [Hubble/Webb Cat's Head](https://science.nasa.gov/missions/hubble/ghostly-cats-head-nebula-lurks-within-sight-of-nasas-hubble-webb/): opaque dust in front of illuminated gas and fine wisps.

Stages reported separately: prototype complete; technically verified; manually
approved; integrated; gameplay validated. Stop for Nicholas's manual Hero approval
before any Combat, Verdance, VFX or shared integration propagation. A compile or
hash check is not an AAA visual-quality approval.
