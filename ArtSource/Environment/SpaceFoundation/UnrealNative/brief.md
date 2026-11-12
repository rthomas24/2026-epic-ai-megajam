# Native Space Foundation

Agent: Environment & VFX Agent. Human: Nicholas Tosti.

Convert the approved SpaceFoundation source checkpoint e2d4139 without changing
the six original files. Three ready-to-place sky and lighting actors: Hero
(teal/violet panorama), Combat (restrained brightness), Verdance (green/teal).
Preserve the broad dark combat corridor. No gameplay or map-owner integration.

One opaque, unlit, two-sided Engine sphere per rig; no imported/new mesh. Two
texture samples, 1024x512 nebula, 2048x1024 stars; no volume raymarch, particles,
collision, actor tick, real-time sky capture or translucent sky layers. Materials
expose the existing yaw, tint and intensity controls. Directional-light and
post-process components expose lighting, temperature, exposure and bloom.

Write scope: this UnrealNative folder and
Content/TriadDrift/Environment/SpaceFoundation/** only. Branch nick/environment-vfx.
Nicholas explicitly approved a branch-only native-assets exception on Nov 12;
Lead confirmed frozen previous workers, isolated workspace and exclusive ownership.
Lead schedules all Unreal execution. No main merge or shared project/config edits.

Validation: generator numeric checks; engine shader compile; save and separate-process
reload; Blueprint component/collision/shadow checks; three native screenshots and
longitude/pole review. No claim of full G-15 completion, gameplay PIE, measured combat
FPS or packaged-build verification from this isolated content-only test.
