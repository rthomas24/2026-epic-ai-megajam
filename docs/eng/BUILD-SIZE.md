# Build size: where the bytes go, and how to get them back

Audit date: Nov 13, 2026 (evening). Read-only audit: nothing in the repo was changed except this file.

**The rule:** the packaged Windows build, zipped, must be under 1 GB.

**Units.** MB here means 10^6 bytes everywhere. The numbers quoted on the board mixed units: the zip is
907,225,952 bytes = 907.2 MB (865 MiB); the `.ucas` is 780.1 MB (744 MiB); the unzipped package is 1,064.4 MB.
If the jam limit is 10^9 bytes we have 92.8 MB of room in that build; if it is 2^30 bytes, 166.5 MB. This plan
assumes the stricter 10^9.

**Measured** means read from the n043 package, its zip or its cook logs. **Estimated** means computed from those
numbers; no trimmed build has been made yet, so every saving except items 1 and 3 is an estimate until a build
confirms it.

## Summary

| | Zipped MB |
|---|---:|
| n043 build (commit 870a753), measured | 907.2 |
| `lead/int` working tree today (6a5190a plus the uncommitted ship rewiring), estimated | about 987 (965 to 1,010) |
| `lead/int` by the freeze, with the Rodin carrier wired, more effects and interior pieces, estimated | about 1,000 to 1,020 |
| After the trims that need no visual look (section 4, items 1 to 6) | about 760 |
| After those plus the trims that need a visual check (items 7 to 9) | about 630 |

Three things nobody asked for are in the build and account for 280 MB of the zip:

1. **A path-tracer denoiser we never use: 117 MB.** The engine plugin `NNEDenoiser` is on by default and always
   cooks six neural-network models (103.2 MB), and it drags in `onnxruntime.dll` and `DirectML.dll`.
2. **Shader code: 134.7 MB stored.** 12,020 global SM6 shaders. About 46 MB of it is for MegaLights, the path
   tracer and hardware ray tracing; another 11 MB is a second copy of everything for DirectX 11.
3. **Both Visual C++ installers: 29.8 MB**, including the ARM64 one, which an x64 build cannot use.

## 1. What is in the 907 MB build (measured)

Tool: Epic's `Engine/Extras/AgentTools/Skills/unreal-trim/scripts/trim_report.py stage <pkg>` plus the zip's own
directory.

### By file, as zipped

| File | Unzipped MB | Zipped MB | Note |
|---|---:|---:|---|
| `unrealGameJam-Windows.ucas` (all cooked content and shaders) | 780.1 | 770.1 | Already Oodle-compressed; zip removes 1.3 % |
| `unrealGameJam-Win64-Shipping.exe` | 177.5 | 70.2 | 244 plugins compiled in |
| `vc_redist.x64.exe` + `vc_redist.arm64.exe` (`-prereqs`) | 30.3 | 29.8 | 18.3 + 11.5 |
| `unrealGameJam-Windows.pak` (config, fonts, localisation, Slate images) | 11.7 | 10.4 | Fonts 4.8, `Engine.locres` 2.3 |
| `DirectML.dll` | 18.5 | 9.3 | Only there because of NNEDenoiser |
| `onnxruntime.dll` | 17.3 | 6.2 | Only there because of NNEDenoiser |
| Other DLLs (Aftermath, D3D12 Agility, MsQuic, DbgHelp, XAudio, Vorbis, TBB) | 23.8 | 9.5 | |
| `global.ucas`, `.utoc`, launcher, manifests | 4.9 | 1.7 | |
| **Total** | **1,064.4** | **907.2** | |

### Inside the `.ucas`, by category (stored bytes; these are also the zipped bytes, within 1.3 %)

| Category | Packages | MB | Share |
|---|---:|---:|---:|
| Textures | 456 | 436.9 | 56 % |
| Shader code (SM6 and SM5) | | 134.7 | 17 % |
| Neural denoiser models (`/NNEDenoiser`, plugin content) | 6 | 103.2 | 13 % |
| Static meshes, including Nanite data | 231 | 80.8 | 10 % |
| Audio | 285 | 8.9 | 1.1 % |
| Skeletal meshes | 11 | 5.3 | |
| Animation | 277 | 4.8 | |
| Materials, Niagara, data assets, UI, levels | 590 | 2.0 | |
| Container overhead and alignment padding | | 7.4 | |

By origin: project content `/Game` 523.9 MB; `/NNEDenoiser` 103.2; `/Engine` 9.8; Interchange `/Assets` 2.9 (one
2.65 MB LUT); ControlRig 0.95; SpeedTree 0.7; all other plugins under 0.5 together. The maps themselves are 0.01 MB.

### Textures by source size (measured cooked bytes, dimensions read from the `.uasset` files)

| Source size | Count | Cooked MB |
|---|---:|---:|
| 2048 x 2048 | 165 | 264.0 |
| 1024 x 1024 | 150 | 67.3 |
| 8192 x 4096 (the nebula sky, HDR, no mips) | 1 | 26.4 |
| 4096 x 2048 (planet globes) | 9 | 31.0 |
| 4096 x 4096 (old Null carrier set) | 5 | 19.1 |
| Smaller | about 125 | about 29 |

By kind: normal maps 89 files, **217.5 MB** (BC5; AI-made normals are noisy and store at 0.80 bytes per pixel, almost
uncompressed); ORM masks 78.0; base colour 57.2; emissive 8.5; other (sky, flipbooks, LUTs) 75.3.
All but 15 textures are in the `World` or `WorldNormalMap` texture group. Virtual textures are off.

By folder: planet set-piece kit `Props/Planet` 113.8; `Props/SpaceDressing` 93.5; `Props/Planets` (globes and
ground) 54.8; old Null fleet 42.1; sky 26.4; VFX flipbooks 24.1; old squad ships 15.0; pilot 8.4.

### Shader code (cook log and `ShaderTypeStats-PCD3D_SM6.csv`)

| Library | Shaders | Size before container compression, MB |
|---|---:|---:|
| Global, SM6 (DirectX 12) | 12,020 | 165.2 |
| Project materials, SM6 | 2,742 | 25.8 |
| Global, SM5 (DirectX 11) | 4,766 | 14.9 |
| Project materials, SM5 | 1,123 | 2.4 |

Stored in the container: 134.7 MB (ratio 0.61). SM6 families, scaled to stored MB (estimated split of a measured
total): MegaLights 22.3; hardware ray tracing 17.4; Lumen and reflections 17.6; Nanite 11.6; shadows 7.0;
path tracer 6.4; hair 1.4; everything else 37.

### The 40 largest cooked assets (measured)

| # | MB | Asset | Source size |
|---:|---:|---|---|
| 1 | 46.34 | `/NNEDenoiser/NNE_oidn2-3-0_rt_hdr_calb_cnrm_large..._DenoiseAlpha` | model |
| 2 | 42.93 | `/NNEDenoiser/NNE_oidn2-3-0_rt_hdr_calb_cnrm_large..._ForwardAlpha` | model |
| 3 | 26.42 | `Props/Sky/T_TD_NebulaSky` | 8192 x 4096 HDR |
| 4 | 11.11 | `Characters/NullFleet/Textures/T_Null_Carrier_N` | 4096 x 4096 |
| 5 | 10.85 | `Props/Planets/T_Planet_Cinder_N` | 4096 x 2048 |
| 6 | 7.87 | `Props/Trees/SM_TDVeg_Kapok_S100_11` | mesh |
| 7 | 6.86 | `/NNEDenoiser/NNE_oidn2-3-0_rt_hdr_alb_nrm..._DenoiseAlpha` | model |
| 8 | 5.75 | `Props/Planets/T_Planet_Verdance_N` | 4096 x 2048 |
| 9 | 5.59 | `Props/Planet/Textures/T_Cind_MagmaSurface_N` | 2048 |
| 10 | 5.59 | `Props/Planets/T_Ground_Verdance_N` | 2048 |
| 11 | 5.59 | `Props/Planets/T_Ground_Cinder_N` | 2048 |
| 12 | 5.13 | `Characters/Pilot/T_Pilot_N` | 2048 |
| 13 | 5.08 | `Props/Trees/SM_TDVeg_TrumpetTree_S100_31` | mesh |
| 14 | 4.98 | `Props/Planet/Textures/T_Cind_MagmaSurface_M` | 2048, BC7 |
| 15 | 4.76 | `Props/DominionInterior/Textures/T_DI_Atlas_N` | 2048 |
| 16 | 4.69 | `Characters/NullFleet/Textures/T_Null_Carrier_ORM` | 4096 x 4096 |
| 17 | 4.47 | `VFX/Textures/T_TD_VFX_Grok_Smoke_8x8` | 2048, BC7 |
| 18 | 4.27 | `Props/SpaceDressing/Textures/T_Derelict_Satellite_02_N` | 2048 |
| 19 | 4.23 | `Props/Planet/Textures/T_Cind_OutpostWall_N` | 2048 |
| 20 | 4.23 | `Props/Planet/Textures/T_Cind_ChokeRock_B_N` | 2048 |
| 21 | 4.14 | `Props/Planet/Textures/T_Cind_ChokeRock_A_N` | 2048 |
| 22 | 4.01 | `Props/SpaceDressing/Textures/T_Derelict_Satellite_01_N` | 2048 |
| 23 | 4.01 | `Props/Planet/Textures/T_Verd_RelayPlatform_M_N` | 2048 |
| 24 | 3.98 | `Props/Planet/Textures/T_Verd_Plateau_N` | 2048 |
| 25 | 3.92 | `Props/SpaceDressing/Textures/T_StationTruss_03_N` | 2048 |
| 26 | 3.91 | `Props/Planet/Textures/T_Verd_RelayPlatform_L_N` | 2048 |
| 27 | 3.87 | `Props/SpaceDressing/Textures/T_GateRing_01_N` | 2048 |
| 28 | 3.83 | `Props/Planet/Textures/T_Cind_JammerTower_N` | 2048 |
| 29 | 3.76 | `Props/Planet/Textures/T_Verd_RelayPlatform_S_N` | 2048 |
| 30 | 3.71 | `Props/Planet/Textures/T_Verd_Wreck_Hull_N` | 2048 |
| 31 | 3.67 | `Props/SpaceDressing/Textures/T_DebrisCluster_01_N` | 2048 |
| 32 | 3.66 | `VFX/Textures/T_TD_VFX_Grok_Explosion_8x8` | 2048, BC7 |
| 33 | 3.61 | `Props/SpaceDressing/Textures/T_Ast_Ice_01_N` | 2048 |
| 34 | 3.60 | `Props/SpaceDressing/Textures/T_Ast_Ice_02_N` | 2048 |
| 35 | 3.57 | `Props/SpaceDressing/Textures/T_StationTruss_01_N` | 2048 |
| 36 | 3.44 | `/NNEDenoiser/NNE_oidn2-3-0_rt_hdr_alb_nrm..._ForwardAlpha` | model |
| 37 | 3.42 | `Characters/NullFleet/Textures/T_Null_Gunship_N` | 2048 |
| 38 | 3.41 | `Props/SpaceDressing/Textures/T_Ast_Metal_02_N` | 2048 |
| 39 | 3.40 | `Props/SpaceDressing/Textures/T_DebrisCluster_02_N` | 2048 |
| 40 | 3.35 | `Characters/NullFleet/Textures/T_Null_Bomber_N` | 2048 |

Paths without a leading mount are under `/Game/TriadDrift/Gameplay/`. Thirty of the forty are normal maps.

## 2. What `lead/int` adds (estimated)

Since 870a753: 134 content files added and 40 changed in commits, plus, in the working tree at audit time, 37
modified files and four untracked ship folders (`Ships/NullBomber`, `NullCarrier`, `NullInterceptor`, `NullTurret`).
`Config/DefaultGame.ini` gained `L_Dominion_Interior` in the cook list. No file was deleted.

Method: every `.uasset` on disk was scanned for the `/Game/...` paths it names, and the same for `Source/`,
`Plugins/` and `Config/`; the cook set was predicted from the six maps, the always-cooked folders and the
Presentation folder. New textures were sized from their pixel count and the stored bytes per pixel measured in
n043 for the same compression setting (normal 0.80, BC7 0.74, masks 0.31, default 0.14); new meshes from the
cooked-to-disk ratio measured in n043 (static 1.1, skeletal 0.1, Niagara 0.015).

| New in the cook | Disk MB | Estimated cooked MB | Why it cooks |
|---|---:|---:|---|
| Warden (`Characters/Warden`, 19 files, twelve 2K maps) | 40.0 | 17.9 | Folder is always cooked |
| Rodin squad ships: Bastion, Lancer, Rift | 53.1 | 24.7 | Referenced by `DA_*_Visuals` |
| Rodin enemy ships: Gunship, Fighter, Turret, Bomber, Interceptor | 60.8 | 33.3 | Referenced by `DA_Enemy_*_Visuals`, `DA_DominionTurret_Visuals` |
| Pilot blade animation sources (`Dev/EngAnim/PilotBlade`) | 6.3 | 0.6 | The always-cooked pilot clips reference them |
| New and changed Niagara systems, pilot clips | 33 | 0.7 | Niagara assets are large in the editor and tiny cooked |
| **Total** | | **about 77** | |

Not in that total: the Rodin carrier (`Ships/NullCarrier`, about 6.4 MB cooked) is on disk but nothing references
it yet, and `Gameplay/Ships` is not an always-cooked folder, so it does not ship until it is wired.
`Content/TriadDrift/Environment` (12 assets) and the duplicate tree meshes under `Gameplay/Vegetation/Verdance`
are not referenced by anything that cooks and do not ship; only that folder's materials do, through `Props/Trees`.

**Expected zip for `lead/int` today: about 987 MB** (907.2 + 77 x 0.99 + about 3 for new shaders and code).
Uncertainty: 965 to 1,010 MB. The texture estimate is the main unknown (plus or minus 20 %), and the working tree
was being edited by other agents during the audit. With the carrier wired and the remaining interior and effect
work, expect 1,000 to 1,020 MB: over the limit.

The old ships did not leave when the new ones arrived. They sit in always-cooked folders, so they still ship
(section 4, item 5).

## 3. What cooks, and what cooks for nothing

Cook rules today (`Config/DefaultGame.ini`): six maps and what they reference; every asset under
`Gameplay/{Materials, Props, Characters, Audio, VFX}`, `UI/TriadHUD`, `/Game/Characters/Mannequins/Anims`,
`/Game/Variant_Shooter/Anims`, `/Engine/BasicShapes`; every data asset in `Presentation`. Engine plugins can add
their own always-cook folders, which is how `/NNEDenoiser` got in.

Assets in always-cooked folders that nothing references. "Verified" here means: no other `.uasset` or `.umap` on
`lead/int` names the package, and no file in `Source/`, `Plugins/` or `Config/` contains the asset's name. It is a
string scan, not a cook dry run. Code that builds a name at run time defeats it, so folders loaded that way are
listed separately.

| Assets | Cooked MB (measured in n043) | Status |
|---|---:|---|
| Old squad ships: `Characters/Squad/SM_Squad_*`, `MI_Squad_*`, `Materials/`, `Textures/` (17 files) | 17.2 | Verified unreferenced in the working tree: the three `DA_*_Visuals` now point at `Gameplay/Ships` |
| Old Null gunship, bomber, turret: `Characters/NullFleet/SM_Null_{Gunship,Bomber,Turret}`, their `MI_` and textures (16 files) | 13.6 | Verified unreferenced in the working tree |
| Old Null fighter and interceptor (meshes, `MI_`, 8 textures) | about 11 | **Still used**: `TDIntroDirector.cpp` lines 150 and 151 load both meshes by path |
| Old Null carrier and carrier bay (4K texture set) | about 19 | **Still used** by `DA_Enemy_Carrier_Visuals` until the Rodin carrier is wired |
| Eleven tree variants in `Props/Trees` (`*_S070_*` and `*_S030_*`, except `Kapok_S070_12`) | 10.9 | Verified: `TDPlanetCourse.cpp` names seven meshes, these are the other eleven |
| Four Grok flipbooks: `T_TD_VFX_Grok_Fire_8x8`, `_ShieldHexRipple_8x8`, `_DebrisChunks_4x4`, `_PlasmaBolt` | 7.7 | Verified unreferenced |
| Niagara probes `VFX/Probe/NS_TD_Probe_*` (10) | 0.13 | Verified unreferenced; too small to matter |
| Mannequin and shooter-variant clips not used by the crew | about 2 | Not verified one by one; small |
| Verdance wreck parts in `Props/Planet` (24 files, 19.5 MB), Dominion interior kit (34 files, 8.0 MB), Warden, Null drone | | **Could not verify.** These are loaded by constructed names (`Props/Planet/%s.%s` in `TDGroundActors.cpp:64`, `Props/DominionInterior/%s.%s`, crew visuals by name). Treat as used |

Do not add `Content/TriadDrift/Dev` to a never-cook list: 22 always-cooked pilot clips reference
`Dev/EngAnim/PilotBlade`, and the cook fails when kept content references a never-cooked folder. The rest of
`Dev` is not referenced and does not ship.

## 4. Trim plan, ranked

Savings are zipped MB against the `lead/int` build. Config and tools are Ryan's side, so every item is ours to do.

### Safe tonight: no change to any rendered pixel

| # | Trim | Saving | Basis | Risk to look | Who |
|---:|---|---:|---|---|---|
| 1 | Disable the `NNEDenoiser` plugin | **117** | Measured: 103.2 MB of models, 9.3 + 6.2 MB of DLLs | None. It denoises the path tracer, an offline renderer the game never runs | Lead |
| 2 | Stop compiling MegaLights and path-tracer shaders | **about 28** | Estimated from shader type sizes (22.3 + 6.4 stored) | None. MegaLights is off by default (`r.MegaLights.EnableForProject=0`) and no asset or source file names it; no asset names the path tracer | Lead |
| 3 | Leave the ARM64 Visual C++ installer out of the zip | **11.5** | Measured | None. An x64 build cannot use it | Lead |
| 4 | Raise container compression from Kraken level 4 to 9 | **about 12** (10 to 20) | Estimated; Epic measured about 1 MB per 100 on export and shader data, less on BC textures | None. Same decoder, same load speed; packaging takes longer | Lead |
| 5 | Move the dead old-ship assets, the four unused flipbooks and the eleven unused tree variants out of the always-cooked folders | **about 49** | Measured sizes (17.2 + 13.6 + 7.7 + 10.9); unreferenced by string scan | None if the scan is right; a cook dry run proves it | Ship lane agent (ships), VFX agent (flipbooks), environment agent (trees) |
| 6 | Drop the DirectX 11 (SM5) shader set | **about 11** | Estimated (17.3 MB of 220 before compression) | None on DirectX 12. The game loses its DirectX 11 fallback, which could not run Nanite or Lumen anyway | Lead decides |
| | **Total** | **about 228** | | | |

Items 1, 2, 3, 4 and 6 are text edits only (about 180 MB). Item 5 needs the editor.

**1. NNEDenoiser.** Add to the `Plugins` array of `unrealGameJam.uproject` (do not touch `EngineAssociation`):

```json
{ "Name": "NNEDenoiser", "Enabled": false }
```

Why it is there: the plugin is `EnabledByDefault`, and its own `Config/Game.ini` adds
`+DirectoriesToAlwaysCook=(Path="/NNEDenoiser")`. It is the only enabled plugin that needs `NNERuntimeORT`, which
is what stages `onnxruntime.dll` and `DirectML.dll`. Epic's trim skill names this exact case ("a denoiser plugin
cooked 100 MB of models in two projects"). Check: the cook log lists no `/NNEDenoiser/` package, the package has
neither DLL, the game boots and plays.

**2. Shader switches.** In `Config/DefaultEngine.ini`, section `[/Script/Engine.RendererSettings]`:

```ini
r.MegaLights.Supported=False
r.PathTracing=0
```

Both are read-only settings that exist in this engine build (checked in `MegaLights.cpp` and `PathTracing.cpp`).
Cost: every editor recompiles its global shaders at the next start (minutes, once per machine thanks to the
shared cache), and the next cook is a full shader cook. Check: `ShaderTypeStats-PCD3D_SM6.csv` in the cook's
`Metadata` folder has no `*LightSamples*` or `FPathTracing*` rows; boot and play one space wave and one planet.
If anyone later turns MegaLights on in a post-process volume, this switch has to come back out.

**3. ARM64 installer.** In `Tools/td-package.ps1`, after UAT succeeds and before anything is zipped, delete
`<archive>\Windows\Engine\Extras\Redist\en-us\vc_redist.arm64.exe`. Keep the x64 one: removing it too saves
18.3 MB more, but a machine without the Visual C++ runtime would then fail to start the game. Only do that if we
are still over.

**4. Compression level.** The package log shows `-compresslevel=4 -compressmethod=Kraken`: without
`-distribution`, a Shipping build uses `PackageCompressionLevel_TestShipping`, which the engine sets to 4. In
`Config/DefaultGame.ini`, section `[/Script/UnrealEd.ProjectPackagingSettings]`:

```ini
PackageCompressionLevel_TestShipping=9
PackageCompressionLevel_Distribution=9
```

Do not add `-distribution` to the UAT line just for this; it changes other things. Leviathan instead of Kraken
would save a few MB more and make every texture and mesh load 34 to 44 % slower to decode: not worth it.

**5. Dead assets in always-cooked folders.** A never-cook rule cannot do this: the old meshes share folders
with the live `DA_*_Visuals`, and never-cooking only `Textures/` fails the cook because the old `MI_*` in the
same always-cooked folder still reference them. So move them, in the editor, to a folder that is not always
cooked, for example `/Game/TriadDrift/Archive/Ships/...`, fix up redirectors, and index the move in
`docs/eng/ART-HISTORY.md` (nothing is deleted). Claim the paths on `CLAIMS.md` first; these are binary assets.

- Now: `Characters/Squad/{SM_Squad_*, MI_Squad_*, Materials, Textures, Pipelines}`;
  `Characters/NullFleet/{SM_Null_Gunship, SM_Null_Bomber, SM_Null_Turret, MI_Null_Gunship, MI_Null_Bomber, MI_Null_Turret}`
  and their `T_Null_Gunship_*`, `T_Null_Bomber_*`, `T_Null_Turret_*`; the four flipbooks; the eleven tree meshes.
- Only once the ship rewiring that was uncommitted during this audit is committed. Re-run the reference scan first.
- Later, 30 MB more: repoint `TDIntroDirector.cpp` lines 150 and 151 to the Rodin fighter and interceptor, wire
  the Rodin carrier, then move the old fighter, interceptor, carrier and carrier bay too.
- Check with a cook dry run (`-run=cook -targetplatform=Windows -CookList -showrejected -cookshowinstigators`)
  that none of the moved packages is listed, then boot and look at all three squad ships and every enemy type.

**6. DirectX 11 shaders.** In `Config/DefaultEngine.ini`, section
`[/Script/WindowsTargetPlatform.WindowsTargetSettings]`, delete the line
`+D3D11TargetedShaderFormats=PCD3D_SM5` and keep the `-D3D11TargetedShaderFormats=PCD3D_SM5` line above it.
After this the game needs a DirectX 12 card with Shader Model 6 (GTX 900 series or newer).

### Needs a visual check before it ships

| # | Trim | Saving | Basis | What could be seen | Who |
|---:|---|---:|---|---|---|
| 7 | Oodle Texture RDO for all textures | **about 90** (60 to 120) | Estimated. Epic reports minus 57 % on BC5 and BC7 for one game; 250 MB of ours is BC5 normals and BC7 | Faint 4x4 block steps in smooth gradients: the nebula sky, soft glows, UI gradients. Noisy normals hide it well | Lead sets it; Ryan or the Lead's playtest looks |
| 8 | Turn hardware ray tracing off for the project | **about 18** (after item 2) | Estimated from shader type sizes | Lumen switches from hardware to software tracing on RTX cards: reflections and bounce light on thin or small objects change; frame rate usually improves | Ryan decides after an A/B capture |
| 9 | Nebula sky from 8192 x 4096 to 4096 x 2048 | **about 20** | Measured 26.4 MB, a quarter remains | The sky is about one texel per pixel at 1080p today; at 4K it becomes soft at 1440p and above. Stars suffer first | Environment agent; Ryan looks |
| 10 | Cap normal maps to 1024 for space dressing and the planet kit | about 50 to 75 after item 7 (123 before it) | Measured sizes, scaled by pixel count | Softer surface detail up close on foot (walls, rocks, wreck); hard to see on asteroids at flight distance | Environment agent; Ryan looks |

**7. RDO.** In `Config/DefaultEngine.ini`:

```ini
[/Script/Engine.TextureEncodingProjectSettings]
bFinalUsesRDO=True
FinalRDOLambda=30
```

The cook already uses the "Final" encoder (`CookUsesSpeed=Final`). This re-encodes every texture on the next cook
(a few extra minutes) and changes nothing in the source assets, so it is reverted by deleting two lines. Look at:
the sky in the main menu and in space, the hyperdrive tunnel, a shield dome, the slate cards and HUD, one planet
on foot. If only the sky shows steps, give `T_TD_NebulaSky` a per-texture "Lossy Compression Amount" of None and
keep RDO for the rest. If lambda 30 is clean, 40 saves more.

**8. Ray tracing.** `r.RayTracing=True` came with the template. In this engine build `r.Lumen.HardwareRayTracing`
defaults to 1, so on an RTX card the game is using hardware Lumen today, and that is what the judges with RTX
cards would see. Setting `r.RayTracing=False` (and removing `r.RayTracing.RayTracingProxies.ProjectEnabled`)
removes the ray-generation and hit shaders and the per-material hit groups. Capture the same views before and
after: the Dominion interior, a planet at dusk, a ship close-up. Note the NVIDIA RTX Award is for NvRTX, DLSS 4.5
or ACE, not for this switch.

**9 and 10. Texture caps.** The sky has no mip chain, so a texture-group cap does not touch it: set
"Maximum Texture Size" 4096 on the asset (binary edit, claim it). For item 10 the config-only route is a group
cap, `[Windows DeviceProfile]` in a new `Config/DefaultDeviceProfiles.ini` with
`+TextureLODGroups=(Group=TEXTUREGROUP_WorldNormalMap,MinLODSize=1,MaxLODSize=1024,LODBias=0,MinMagFilter=aniso,MipFilter=point,MipGenSettings=TMGS_SimpleAverage)`,
but that also halves the normal maps of the ships, the pilot and the planet ground (136 MB in all before RDO), which
we should not do to hero assets. The targeted route is an editor script that sets `LODBias=1` on the normal maps
under `Props/SpaceDressing/Textures` and `Props/Planet/Textures` and saves them (about 60 binary files, claim
first). Hold this one in reserve; items 1 to 7 should make it unnecessary.

### Looked at and not worth doing

| Idea | Finding |
|---|---|
| Zip tool or level | No effect. The `.ucas` deflates to 99.8 % at level 6 and at level 9 (measured on a 64 MiB sample); the exe is 72.7 MB at level 6 and 69.8 at level 9, and the n043 zip already has it at 70.2. LZMA inside a zip would take the exe to 47 MB, but Windows Explorer cannot open such a zip: do not |
| Audio quality | All audio is 8.9 MB. Leave it alone; audio is a judged category |
| BC7 to DXT | 11 textures use BC7 (flipbooks, magma mask, about 30 MB). Same bits per pixel as DXT5; RDO (item 7) is the lever that works on them |
| Virtual textures | Already off |
| Nanite fallback meshes | Static meshes are 80.8 MB in all, 30.6 of it the 18 trees; item 5 removes the unused eleven. Hardware Lumen traces the fallback, so coarsening it changes lighting |
| Engine content | `/Engine` is 9.8 MB, fonts and localisation 7 MB. `bSkipEditorContent=True` and font excludes could give 3 to 5 MB; fiddly, and the fallback font is needed if any name contains non-Latin characters |
| Unused engine plugins compiled into the exe | The exe is 70 MB zipped with 244 plugins. Cutting plugins could give 10 to 25 MB but a wrong cut is a crash at boot. Not during the jam |
| Dropping `-prereqs` entirely | 18.3 MB more than item 3, with a small chance that a judge's machine cannot start the game. Last resort |
| IoStore padding | 3.5 MB of zeros that the zip already removes |
| `Content/TriadDrift/Dev` | Only the pilot blade sources ship (0.6 MB), and they are referenced. Nothing to gain |

## 5. Order of work and how to confirm

1. Tonight, one commit each, in this order: items 1, 3, 4 (no shader recompile), then 2 and 6 together (one
   shader recompile). Package with `Tools/td-package.ps1`, zip, and measure with
   `python "<UE_5.9>/Engine/Extras/AgentTools/Skills/unreal-trim/scripts/trim_report.py" stage <Builds\name\Windows> --md <out.md> --compare <previous report.json>`.
   Expected: about 987 down to about 807 MB.
2. Boot the package and play the engineering checklist (AGENTS.md section 11). A build that was not played is
   not a baseline.
3. Item 5 when the ship rewiring is committed: about 807 down to about 760 MB.
4. Item 7 with a visual pass: about 760 down to about 670 MB. Items 8 and 9 if Ryan agrees after seeing captures:
   about 630 MB.
5. Add a size check to the package script: fail, not warn, when the zip is over 950 MB, and print the ten
   largest new packages against the previous report.

What I could not verify: the RDO, compression-level and shader savings (estimates until built); whether the
jam's 1 GB is decimal or binary; whether the Warden, the Null drone, the Verdance wreck parts and the interior kit
are all really loaded by code (they are loaded by constructed names, so they were treated as used); and the
state of the ship data assets, which another agent was editing in the `lead/int` working tree during the audit.

## 6. Update, Nov 14 03:30: the zip is at 981 MB (import builder)

Headless only: no cook, nothing rendered. **Measured** = read with Epic's `trim_report.py stage` from the packaging
agent's staged Shipping build of `eb1bf334` (`.wt/pkg/Saved/StagedBuilds/Windows`, 2,466 packages) or from the asset
files on `f39a8372`. **Estimated** = computed from those numbers. Every never-cook saving below is unproven until a
Shipping cook runs clean with the rules in. MB = 10^6 bytes; cooked package bytes are within 1.3 % of zipped bytes.

**Where the 981 MB is (measured, `eb1bf334`).** `.ucas` 879.8 MB: textures 610.5 (639 packages), static meshes
113.1, shader code 89.9, audio 45.4 (Nicholas's music 39.5, engineering 5.8), skeletal 5.7, animation 5.3,
everything else under 6. Outside it: the Shipping exe 178.1 (about 70 zipped), `vc_redist.x64.exe` 18.6, other DLLs
about 23 (about 9 zipped), the `.pak` 11.0. Items 1 to 4 of section 4 (denoiser, MegaLights and path-tracer shaders,
ARM64 installer, compression level) are already in; the growth since is content: 174 more texture packages.

### 6.1 The plan, ranked (target: 120 MB of headroom with no visible or audible loss)

| # | Lever | Est. zip MB | Risk | Exact change | State |
|---:|---|---:|---|---|---|
| 1 | Never-cook 142 engineering packages that nothing reaches: the old Null fleet (42.1), old squad ships (16.3), Verdance planet-kit pieces no longer placed (51.1), the Rodin wreck set (7.8), old Dominion exterior pieces (5.1), two flipbooks with their materials and two scatter canopies (6.0) | **128** | None to the picture if the scan is right. If kept content does reference one, the cook fails ("Content is missing from cook"): loud, not silent. Nothing is deleted | `Config/DefaultGame.ini`, one `+DirectoriesToNeverCook` line per package, one commit per family: `48899d94`, `587802f1`, `713f9b0d`, `f7ed0a59`, `afc63afd`, `9a526f32` | **committed, unproven** |
| 2 | The earlier never-cook list (35 superseded packages, 2 walk-test classes, 13 pipelines) | **11** (cooked: 10.5 + 0.2, measured) | Same | `2229c130`, `17855ca5`, `d49ced2b` | committed, in the packaging agent's proving cook |
| 3 | Drop the DirectX 11 (SM5) shader set | **10** | The game no longer starts on a machine without DirectX 12 / SM6. It could not run Nanite or Lumen there anyway. Lead decides | `Config/DefaultEngine.ini`: the `+D3D11TargetedShaderFormats=PCD3D_SM5` line removed. `36fd0fac` | committed, optional |
| 4 | Leave `d3d12SDKLayers.dll` out of the package | **2** | None: the debug layer loads only with `-d3ddebug` | `Tools/td-package.ps1`. `8471b9b5` | committed |
| | **No-look total** | **about 150** | | | |
| 5 | Oodle Texture RDO on all textures | 80 to 130 | **Needs eyes.** 474 MB of engineering textures remain after item 1; 238 MB of that is BC5 normal maps, which RDO shrinks most. Possible faint 4x4 steps in smooth gradients (the nebula sky, glows, UI) | `Config/DefaultEngine.ini`: `[/Script/Engine.TextureEncodingProjectSettings]` `bFinalUsesRDO=True`, `FinalRDOLambda=20` (30 saves more). Exempt `T_TD_NebulaSky` and Nicholas's UI textures per asset (Lossy Compression Amount: None) if steps show. Full texture re-encode on the next cook | not applied: not provably invisible |
| 6 | Normal maps of space dressing and the planet kit at half size (`LODBias 1`) | 40 to 70 (54.1 + 40.2 MB of normals today; three quarters of each goes) | Softer close-up surface on foot (walls, choke rocks). Not visible on asteroids at flight distance | Per asset, in the editor: `LODBias=1` on `Props/SpaceDressing/Textures/*_N` first (keeps the mip chain; reversible by setting 0). Not a texture-group cap: the group also holds the ships, pilot and ground | not applied: binary edits, needs a look |
| 7 | Planet globe maps (nine at 4096 x 2048; `T_Planet_Cinder_N` 10.8, `T_Planet_Verdance_N` 5.7) at `LODBias 1` | 12 to 20 | The globes are seen whole from space and close during the dive. Needs a look at the dive | Per asset, editor | not applied |
| 8 | Nebula sky 8192 x 4096 to 4096 x 2048 | 19 | Stars soften at 1440p and above | Per asset `MaxTextureSize=4096` (it has no mips, so `LODBias` does nothing) | not applied: visible at high resolution |
| 9 | Music quality (Nicholas's, 39.5 MB cooked, 11 tracks at about 270 kbps) | about 17 at quality 50 | Audible only on a close listen; audio is judged; **his call** | Per SoundWave `Quality`. Nothing changed | **Nicholas's folders: listed only** (6.4) |
| 10 | Trees: Nanite data of the 18 `Props/Trees` meshes (28.8 MB; two are 7.5 and 4.8 MB) | 8 to 15 | Changes tree silhouettes up close; hardware Lumen traces the fallback | Per mesh: Nanite `Keep Triangle Percent` 50 on `SM_TDVeg_Kapok_S100_11` and `SM_TDVeg_TrumpetTree_S100_31` | not applied: needs a look |
| 11 | `vc_redist.x64.exe` out of the zip | 18 | A machine without the Visual C++ runtime cannot start the game. Last resort | `Tools/td-package.ps1` | not applied |
| 12 | Hardware ray tracing off (`r.RayTracing=False`) | about 18 | Changes Lumen on RTX cards (section 4, item 8): Ryan's decision after an A/B | `Config/DefaultEngine.ini` | not applied |

Items 1 to 4 need no look at the picture, only a clean cook and a boot. With all four in, the zip should be about
**830 MB** before batch O's additions. Item 5 is the next 100 MB if a look at the sky and the HUD passes.

### 6.2 The 40 largest cooked packages (measured, `eb1bf334`)

Paths are under `/Game/TriadDrift/Gameplay/` unless they start with `Audio/` or `UI/` (Nicholas's).

| # | MB | Class | Package | Use |
|---:|---:|---|---|---|
| 1 | 25.35 | texture | `Props/Sky/T_TD_NebulaSky` | used |
| 2 | 10.91 | texture | `Characters/NullFleet/Textures/T_Null_Carrier_N` | **unused**, never-cook line added |
| 3 | 10.84 | texture | `Props/Planets/T_Planet_Cinder_N` | used |
| 4 | 7.51 | staticmesh | `Props/Trees/SM_TDVeg_Kapok_S100_11` | used |
| 5 | 5.74 | texture | `Props/Planets/T_Planet_Verdance_N` | used |
| 6 | 5.68 | sound | `Audio/VerdanceTheme01/MUS_Verdance_Main_Full_01` | used |
| 7 | 5.59 | texture | `Props/Planet/Textures/T_Cind_MagmaSurface_N` | used |
| 8 | 5.59 | texture | `Props/Planets/T_Ground_Verdance_N` | used |
| 9 | 5.59 | texture | `Props/Planets/T_Ground_Cinder_N` | used |
| 10 | 5.10 | texture | `Characters/Pilot/T_Pilot_N` | used |
| 11 | 4.87 | sound | `Audio/DominionAssault02/MUS_DominionAssault_PushTheAttack_Full_01` | Nick's; named by no asset (played by his plugin) |
| 12 | 4.86 | sound | `Audio/CinderMusic01/MUS_Cinder_Combat_Fall_Full_01` | Nick's; named by no asset (played by his plugin) |
| 13 | 4.84 | staticmesh | `Props/Trees/SM_TDVeg_TrumpetTree_S100_31` | used |
| 14 | 4.80 | texture | `Props/Planet/Textures/T_Cind_MagmaSurface_M` | used |
| 15 | 4.77 | texture | `Props/Kit/Textures/T_KitS_Trim_N` | used |
| 16 | 4.71 | texture | `Props/DominionInterior/Textures/T_DI_Atlas_N` | used |
| 17 | 4.69 | texture | `Props/Kit/Textures/T_Kit_Trim_N` | used |
| 18 | 4.59 | texture | `Props/CinderAnchor/T_CinderAnchorEmitter_N` | used |
| 19 | 4.53 | staticmesh | `Ships/DominionMothership/SM_DominionMothership` | used |
| 20 | 4.51 | sound | `Audio/CinderMusic01/MUS_Cinder_Exploration_World_Full_01` | Nick's; named by no asset (played by his plugin) |
| 21 | 4.43 | texture | `Characters/NullFleet/Textures/T_Null_Carrier_ORM` | **unused**, never-cook line added |
| 22 | 4.42 | sound | `Audio/CreditsMusic01/MUS_Credits_EtherealPulseRemix_Full_02` | used |
| 23 | 4.39 | texture | `VFX/Textures/T_TD_VFX_Grok_Smoke_8x8` | used |
| 24 | 4.30 | texture | `VFX/Textures/T_TD_VFX_Grok_PlasmaSheath_8x8` | used |
| 25 | 4.22 | texture | `Props/SpaceDressing/Textures/T_Derelict_Satellite_02_N` | used |
| 26 | 4.19 | texture | `Props/Planet/Textures/T_Cind_ChokeRock_B_N` | used |
| 27 | 4.17 | texture | `Props/Planet/Textures/T_Cind_OutpostWall_N` | used |
| 28 | 4.13 | texture | `VFX/Textures/T_TD_VFX_Grok_CloudPunch_8x8` | used |
| 29 | 4.10 | texture | `Props/Planet/Textures/T_Cind_ChokeRock_A_N` | used |
| 30 | 3.99 | texture | `Props/Relay/Textures/T_Relay_N` | used |
| 31 | 3.98 | texture | `VFX/Textures/T_TD_VFX_Grok_CloudWisp_8x8` | used |
| 32 | 3.98 | texture | `Props/Planet/Textures/T_Verd_RelayPlatform_M_N` | **unused**, never-cook line added |
| 33 | 3.97 | texture | `Props/SpaceDressing/Textures/T_Derelict_Satellite_01_N` | used |
| 34 | 3.93 | texture | `Props/Planet/Textures/T_Verd_Plateau_N` | **unused**, never-cook line added |
| 35 | 3.88 | texture | `Props/Planet/Textures/T_Verd_RelayPlatform_L_N` | **unused**, never-cook line added |
| 36 | 3.84 | texture | `Props/SpaceDressing/Textures/T_StationTruss_03_N` | used |
| 37 | 3.83 | texture | `Props/CinderAnchor/T_CinderAnchorTower_N` | used |
| 38 | 3.83 | texture | `Props/Wreck/Textures/T_Wreck_N` | **unused**, never-cook line added |
| 39 | 3.83 | texture | `Props/SpaceDressing/Textures/T_GateRing_01_N` | used |
| 40 | 3.78 | texture | `Props/Planet/Textures/T_Cind_JammerTower_N` | used |

"Used" = reachable from a cooked map, a data asset, a Presentation asset or a name in `Source/`, `Config/` or
`Plugins/` (names built with `%d` / `%s` patterns included). The scan cannot see a name assembled from table data.
No package in the top 40 is duplicated; the duplicates that mattered were the old ships beside the Rodin ones.

### 6.3 Textures (610.5 MB cooked; 590.6 of it in engineering folders)

| Folder | Cooked MB today | After item 1 | Of which normal maps | Seen |
|---|---:|---:|---:|---|
| `Props/SpaceDressing` (textures and meshes) | 103.4 | 103.4 | 54.1 | flight distance, 100 m and more |
| `Props/Planet` (set-piece kit) | 120.8 | 69.7 | 40.2 | on foot, close |
| `Props/Planets` (globes, ground, terrain sets) | 72.0 | 72.0 | 43.1 | globes from space; ground on foot |
| `VFX/Textures` (flipbooks) | 39.1 | 32.0 | 0 | effects, BC7 |
| `Props/Sky` | 26.8 | 26.8 | 0 | full screen |
| `Props/Kit` | 17.8 | 17.8 | 12.1 | on foot, close |
| `Characters/NullFleet` + `Characters/Squad` (old ships) | 54.4 | about 4 (carrier bay) | - | not in the game |

Over 2048 on a side: `T_TD_NebulaSky` 8192 x 4096 (item 8); nine planet globe maps 4096 x 2048 (item 7); the old Null
carrier set 4096 x 4096 (19 MB: unused, gone with item 1). Nothing else is over 2048 (sizes from section 1's
audit; the textures added since are 2K and 1K deliveries), so a "cap at 2048" does nothing. The levers are item 1,
RDO (item 5) and half-size normals where the camera never gets close (item 6).

### 6.4 Audio (45.4 MB cooked)

Engineering's fallback set is 5.8 MB for 227 waves: leave it. Nicholas's music as cooked (measured), with the
duration estimated from the source size (about 7.7 s per MB of `.uasset`, checked against the one known length,
139 s): about 270 kbps each. Whether a wave streams and its quality setting were not read (that needs the editor),
so "current compression" here is the measured bit rate only.

| Track (`Content/TriadDrift/Audio/`) | Cooked MB | About | Est. saving at quality 50 |
|---|---:|---|---:|
| `VerdanceTheme01/MUS_Verdance_Main_Full_01` | 5.68 | 185 s | 2.5 |
| `DominionAssault02/MUS_DominionAssault_PushTheAttack_Full_01` | 4.87 | 146 s | 2.2 |
| `CinderMusic01/MUS_Cinder_Combat_Fall_Full_01` | 4.86 | 154 s | 2.2 |
| `CinderMusic01/MUS_Cinder_Exploration_World_Full_01` | 4.51 | 146 s | 2.0 |
| `CreditsMusic01/MUS_Credits_EtherealPulseRemix_Full_02` | 4.42 | 139 s | 2.0 |
| `IdleAmbient01/MUS_SpaceIdle_Ambient1_Full_01` | 3.44 | 115 s | 1.5 |
| `MenuTheme01/MUS_Menu_EtherialPulse_01` | 3.23 | 100 s | 1.4 |
| `CombatMusic01/MUS_Tutorial_EtherealPulse_Continuous95_01` | 3.10 | 92 s | 1.4 |
| `CreditsMusic01/MUS_Credits_CathedralPulse_Full_01` | 2.42 | 77 s | 1.1 |
| `DominionCore01/MUS_DominionCore_CathedralPulse_Full_01` | 2.05 | 66 s | 0.9 |
| `DominionInterior01/MUS_DominionInterior_Calm_01` | 0.96 | 32 s | 0.4 |
| **Total** | **39.5** | | **about 17** |

Batch O's new tracks will cook at the same rate (about 34 kB per second). `MUS_Combat_EtherealPulseRemix_01` and
`MUS_DominionAssault_InfinityZone_Full_01` exist in the repository and are not in the `eb1bf334` package.

### 6.5 Meshes (113.1 MB cooked)

By folder: `Props/Trees` 28.8 (18 trees, all named by code now), `Props/SpaceDressing` 13.1, `Props/Planet` 10.5
(about 6 of it unused, item 1), `Props/Scatter` 6.9, `Props/Dominion` 6.8 (the old hull 2.5 and four more unused,
item 1), old Null fleet 5.0 (item 1), the r2 Dominion hull 4.5, the anchor 4.3, ships 1.5 to 1.8 each. The hull, the
kits and the ships are small; only the trees are worth a setting (item 10). Nanite fallback and LOD settings were
not read: that needs the editor.

### 6.6 Engine content and plugins still in the package

Engine and plugin content in the `.ucas` is 14.5 MB in all: `/Engine` 9.3 (blue-noise and default textures, Substrate
tables), Interchange `/Assets` 2.1 (one LUT), Niagara vector fields 1.7, ControlRig 0.8, SpeedTree 0.5. Not worth a
rule tonight. The exe is 178 MB (about 70 zipped) with the default plugin set compiled in; cutting plugins is 10 to
25 MB with a crash-at-boot risk for each wrong cut (section 4, "not worth doing"): still not during the jam.
`GFSDK_Aftermath_Lib.x64.dll` (5.8 MB) and `msquic.dll` (2.5 MB) could go the way of the debug layer (item 4) for
about 3 MB zipped, but engine modules load both at start: not without a boot test.

### 6.7 Unused by the scan but deliberately not excluded

The drill rig set (7.6 MB) and `BP_DigSite_Cabin`, the ten outpost set-dressing props (1.1), unwired effects waiting
on a slot, about 2.3 MB of crew and pilot clips no code names. `BP_Wreck` is in item 1's wreck family: no code or
asset names it on `f39a8372`; if T5 means to place the wreck site again, drop commit `f7ed0a59`.
