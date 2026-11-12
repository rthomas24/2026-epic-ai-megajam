# Space Foundation / native Unreal package

Owned follow-up to frozen checkpoint `e2d4139`. `prepare_sources.py` calls the
existing generator; `build_native.py` calls the existing material builder with an
owned source/output override. The original generator, builder and presets are
unchanged. Per-file input/output hashes are in `provenance.json`.

## Placement

Native package: `/Game/TriadDrift/Environment/SpaceFoundation`. Native creation and
separate-process validation passed on UE 5.9 CL60351404, November 12, 2026.

[Hero](review/Hero_Panorama.png) · [Combat](review/Combat_Panorama.png) ·
[Verdance](review/Verdance_Panorama.png) · [Longitude seam](review/Hero_LongitudeSeam.png) ·
[Near north pole](review/Hero_NorthPole.png). These are native scene captures.

Place exactly one `BP_GalaxySkybox_Hero`, `BP_GalaxySkybox_Combat` or
`BP_GalaxySkybox_Verdance` into a map you own. Each contains a sphere, sunlight
and a low-priority unbound post-process component. It does not spawn gameplay
actors, handle input, apply damage, or bind shared presentation contracts.
The map owner places the actor; this kit never edits `L_Galaxy`.

The sphere uses camera direction rather than mesh UVs, so movement inside it
does not slide the sky. Its radius is 50km; keep the playable region inside it.
Collision, shadows, decals, ray tracing and distance-field lighting are disabled
on that component. Actor tick is unused. No new or modified mesh is included.

## Art controls

| Asset/component | Editable controls |
|---|---|
| `MI_TD_Space_*` | `YawTurns`, `NebulaTint`, `NebulaIntensity`, `StarIntensity`, `SkyIntensity`, texture overrides |
| Master material | `TwinkleAmount` (0 disables), `TwinkleRate` (cycles/sec); inherited by all instances |
| `Sunlight` component | intensity in lux, temperature, relative rotation, shadows |
| `PreviewExposure` component | manual exposure, compensation, bloom, blend weight, priority; disable this component if the destination map already owns exposure |

Sky images are LDR art, not a physically calibrated HDR light source. The rig
has no skylight capture; fill lighting remains the destination map owner's choice.
Existing `sky_light_intensity` data is retained as reference only.

`native_tuning.json` records the native gain/rotation adjustments applied after
the frozen source presets. Resident BC7 textures and trilinear filtering preserve
the dark gradients and small stars. Hero is the richer menu/review preset, Combat reduces nebula brightness, and
Verdance rotates/tints the same authored banks toward teal. Defaults preserve a
dark area for targets. Rotate `YawTurns` to frame objectives, then check target
contrast at gameplay exposure before increasing emissive intensity or bloom.

## Rebuild and verification

Use UE **5.9.0 CL60351404**. Lead must release the single heavy-tool slot first.
Run `prepare_sources.py` using the engine's bundled Python, then
`launch_host.ps1 -Stage Build` and `launch_host.ps1 -Stage Verify` sequentially.
Both stages use a disposable content-only project under ignored `Saved/Host`.
Its only content mount points at the exact owned native package; it does not
load or build the shared game's C++ or edit the real `.uproject`/`Config`.
The launcher refuses insufficient RAM or competing Unreal/Blender/compile work.

Fresh builds refuse existing asset names and existing texture outputs; verification
can reload its three exact owned review maps. Inspect existing work before a deliberate rebuild.
Do not delete another artist's output or rerun with replacement flags.

Native evidence: `review/build.json`, `review/verification.json`, screenshots and
the isolated `/Review/L_SpaceFoundation_*` levels. Nine assets reload; final verification
has zero errors and zero warnings. Explicit texture/mesh completion avoids asynchronous
32px placeholders in commandlets; per-instance shader checks avoid skydome fallback.
Capture and sunlight rotations use named fields and are numerically recorded.
No gameplay PIE, combat timing, packaging or full G-15 completion is implied.

Intended rendering budget: one opaque sky draw, two texture samples, no particles,
volume raymarch or translucency. Host compiler reports 142 pixel instructions,
146 vertex instructions, two samplers and five estimated pixel texture lookups
(including renderer work). These are compiler/design counts, not measured frame times.

Known inherited limitation: the equirectangular star kernels stretch near the
zenith; the near-pole capture documents it. Nebula continuity is preserved and no
longitude line is visible in the wrap capture. Final in-game exposure/readability
and target-platform performance remain map-owner integration checks.

## Provenance and integration

Original project procedural work by Nicholas Tosti with OpenAI Codex. No external
textures, meshes or audio downloaded. Engine sphere is referenced under the team's
Unreal license; it is not copied into this package. Lead aggregates this folder's
provenance and review report into root credits/devlog during integration.

All work stays on `nick/environment-vfx` for Lead review. Initial binary exception
is environment-only; VFX binaries remain a later production milestone.

API references: [Epic's subobject subsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/SubobjectDataInterface/USubobjectDataSubsystem),
[texture sampling API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UMaterialExpressionTextureSample).
Installed 5.9 headers and actual engine runs determine compatibility.
