# Unreal Engine 5.9 implementation

Everything here was checked on **2026-11-12** against the engine on this machine
(`C:\Program Files\Epic Games\UE_5.9`, UE 5.9.0 MegaJam Preview, CL 60351404) and this repo at
commit `f0bee9c`. Each fact says how it was checked:

- **[live]** exercised through Unreal's built-in MCP server in a headless editor on a scratch
  project (toolsets `EditorToolset`, `MetaSoundToolset`, `MediaToolset`, `ConsoleToolset` and the
  `AudioModulation` plugin enabled);
- **[source]** read in the engine or repo source;
- **[docs]** read on Epic's documentation site (UE 5.9 pages);
- **[not verified]** stated from general knowledge or seen only as an API name.

Nothing here was tested inside the Triad Drift project itself or heard by a person.

## 1. How gameplay plays your sounds (read this first)

From `Source/unrealGameJam/TriadDrift/Core/TDStatics.cpp` **[source]**:

| Cue kind | Engine call | Consequence for the asset |
|---|---|---|
| One-shot (`UTDStatics::PlayCue`) | `UGameplayStatics::PlaySoundAtLocation(World, Sound, Location, Rotation)` with every other argument left at its default | No attenuation, concurrency or volume is passed in. **The sound asset must carry its own** Sound Class, Attenuation and Concurrency. No attenuation on the asset = it plays 2D at full level, wherever in the galaxy it happened |
| Attached, and loops (`PlayCueAttached`, `PlayCueAttachedLoop`) | `UGameplayStatics::SpawnSoundAttached(Sound, Component, Socket)` | Same. Loops are stopped by gameplay with a 0.25 s fade (0.1 s for weapon charge) |

Other facts **[source]**:
- Every `FTDFXCue` has a `Sound` (`TSoftObjectPtr<USoundBase>`), so a MetaSound Source, a Sound Wave
  or a Sound Cue all fit. Use MetaSound Sources.
- Gameplay sets **no audio parameters** today: there is no `SetFloatParameter` call anywhere in
  `Source/`. An engine loop that should follow speed needs Ryan to add one line per parameter
  (request below). Until then a parameter stays at its default.
- `UTDShipVisuals::EngineLoop` is declared but **nothing reads it** at `f0bee9c`. The loop that
  plays is `EngineTrail.Sound` (started in `ATDShip::StartEngineTrail`), plus `BoostLoop.Sound` and
  `DriftLoop.Sound`. Put the engine loop in `EngineTrail.Sound` and tell Ryan about `EngineLoop`.
- Ship switch: engineering plays `SwitchIn` attached to the new ship and `SwitchOut` attached to the
  old one, in the same frame as the switch (`TDSquadManager.cpp`). Put the cut sound in
  `SwitchIn.Sound` as a 2D sound and leave `SwitchOut.Sound` empty or very quiet, so the cut never
  plays twice. Do not also play it from a widget (docs/visual/Presentation-Asset-Paths.md).
- Slow motion is `UTDFeelSubsystem::RequestSlowMo` setting world time dilation. **The engine's audio
  code does not react to time dilation** (no reference to it in `AudioDevice.cpp`, `ActiveSound.cpp`,
  `AudioComponent.cpp`, `SoundWave.cpp` or the AudioMixer module), so slow motion is silent to the
  ear unless audio does something (section 7).
- The chase camera, and so the listener, is 1400 cm behind the player's ship (`ATDShip`
  `CameraTuning.ArmLength`). Weapon range is 30 000 cm, enemy engage range 12 000 cm.

Requests for Ryan's side and where they stand (2026-11-12 15:33 EST): (1) is in progress with the flight
owner (`Speed01`, `Boost01`, `Drift01`; `EngineLoop` is read by `ATDShip` since `1588fdb`); a live-state object
for music and mixes is on `main` (section 8). The original list:
1. Set float parameters on the ship's loop audio components each tick: `Speed01`, and `Boost01` if
   wanted (`UAudioComponent::SetFloatParameter`).
2. Decide what `EngineLoop` is for, or remove it.
3. Enable the `AudioModulation` plugin only if control buses are wanted (not needed for this plan).
4. Main submix limiter: set in project audio settings, or let the audio director add it at runtime
   (section 8).

## 2. An editor session for audio work

Unreal's MCP server starts with `-ModelContextProtocolStartServer -ModelContextProtocolPort=<port>`.
Ports: Nicholas's side 8020-8029, Ryan's side 8010-8019 and 21000-29000 (AGENTS.md §11). One agent
per editor; calls run one at a time.

`Tools/td-play-mcp.ps1 -Editor` starts a headless editor, but **with `-nosound`**. With `-nosound`
**[live]**:
- saving a MetaSound logs "PreSaveAsset for MetaSound … is doing nothing because … CanEverRenderAudio
  [is] false". The assets still saved, loaded and validated in the test, but the pre-save step was
  skipped, so author audio in an editor that can render audio;
- `AudioPreviewSubsystem.is_preview_available()` is false and nothing can be rendered.

Use `-NonRealtimeAudio` instead of `-nosound`: the engine loads its non-realtime renderer
(`AudioDeviceManager.cpp` **[source]**; module `NonRealtimeAudioRenderer` seen loading **[live]**),
audio is rendered offline and nothing plays on the speakers. The command that was tested (scratch
project, `-nullrhi`):

```powershell
& "C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<checkout>\unrealGameJam.uproject" `
  -nullrhi -nosplash -unattended -NoLiveCoding -NonRealtimeAudio `
  -ModelContextProtocolStartServer -ModelContextProtocolPort=8021 -abslog="<checkout>\Saved\Logs\audio-editor.log"
```
The server answered after about 15 s and all toolsets were registered about 25 s later. For the real
project the C++ module must already be built in that checkout (Ryan's `Tools/td-build.ps1`); that
was not tested here. Stop the process when done. Then:

```
python Tools/td_mcp.py --port 8021 toolsets
python Tools/td_mcp.py --port 8021 describe MetaSoundToolset.MetaSoundAuthoringToolset
python .claude/skills/ue-game-audio/scripts/ue_run_python.py --port 8021 <script.py> --spec <spec.json>
```

## 3. What the MCP server offers for audio **[live: names as served]**

Call as `python Tools/td_mcp.py --port P call <Toolset> <Tool> '<json>'`. Argument names are
camelCase (`folderPath`, `builderName`).

| Toolset | Tools | Use |
|---|---|---|
| `EditorToolset.SoundWaveToolset` | `ImportFile(folderPath, assetName, sourceFile, bReplaceExisting, bSave)`, `ExportWav(soundWave, filePath)` | Import wav / aif / ogg / flac / opus / mp3 as a Sound Wave; export one back to WAV |
| `EditorToolset.ObjectToolset` | `ListProperties`, `GetPropertySchema`, `GetProperties`, `SetProperties`, `ResetProperties`, `SearchSubclasses`, `GetClass` | Read and set any property of any asset: compression, looping, sound class, attenuation, concurrency, data-asset slots. Objects are passed as `{"refPath": "/Game/…/Name.Name"}` |
| `EditorToolset.AssetToolset` | `CreateFolder`, `ListFolders`, `Exists`, `Duplicate`, `Move`, `Delete`, `FindAssets`, `ValidateAssets`, `LoadAsset`, `SaveAsset`, `IsDirty`, `GetReferencers`, `GetDependencies`, `GetAssetClass`, tags and metadata tools | Asset housekeeping |
| `EditorToolset.DataAssetToolset` | `Create(folderPath, assetName, assetType)`, `Diff` | Create the `DA_*` presentation assets |
| `MetaSoundToolset.MetaSoundDiscoveryToolset` | `FindNodeClasses`, `DescribeNodeClass`, `FindInterfaces`, `FindDataTypes`, `FindRegisteredPages` | What a graph can be built from; **describe a node before wiring it** |
| `MetaSoundToolset.MetaSoundAuthoringToolset` | `FindMetaSoundAssets`, `CreateSourceBuilder`, `CreatePatchBuilder`, `CreateSourcePresetBuilder`, `CreatePatchPresetBuilder`, `AttachBuilderToAsset`, `DetachBuilderFromAsset`, `BuildToAsset`, `CommitBuilderToAsset`, `AddNodes`, `AddGraphInputs`, `AddGraphOutputs`, `RemoveNodes`, `Connect`, `Disconnect`, `ClearPins`, `SetDefaults`, `AddGraphVariables`, `RemoveGraphVariables`, `AddVariableNodes`, `LayoutGraph`, `SetBuildPage`, `AddGraphPage`, `RemoveGraphPage` | Build and edit MetaSound graphs |
| `MetaSoundToolset.MetaSoundInspectionToolset` | `DescribeGraph`, `FindAllNodes`, `FindAllConnections`, `DescribeNode`, `FindGraphInputs`, `FindGraphOutputs`, `FindGraphVariables`, `FindBuilderInterfaces`, `FindInterfaceInputNodes`, `FindInterfaceOutputNodes`, `FindGraphPages`, `FindGraphInputDefaults`, `DiffPages` | Read a graph back |
| `editor_toolset.toolsets.python_scripting.PythonScriptingTools` | `read_me`, `execute_python(script)`, `search_symbols`, `get_symbol` | Run editor Python. `script` is an object: `{"content": "<python>", "language": "Python"}`. Inside, `unreal`, `asyncio` and `await call_tool(fullToolName, **args)` exist |
| `MediaToolset.MediaToolset` | `ReadImageFile`, `GetVideoInfo`, `SkimVideo`, `SampleFrames` | Images and video only. **It has no audio tool**; use it to look at `audio_view.py` pictures or at a gameplay capture's frames |
| `ConsoleToolset.ConsoleToolset`, `ConsoleToolset.LogsToolset` | console variables and commands; `GetLogEntries` | `LogMetaSound`, `LogAudio`, `LogAudioMixer` |
| `FabToolset.FabToolset` | `SearchFabListings`, `GetFabListingDetails`, `SearchFabMyLibrary`, `InstallFabListing`, `FabSignIn` | Find free audio packs; installing needs the human's signed-in session and approval |
| `ToolsetRegistry.AgentSkillToolset` | `ListSkills`, `GetSkills`, `CreateSkill`, `UpdateSkill` | Unreal's own skill loader (section 12) |

The MetaSound toolset ships seven agent skills of its own (fundamentals, workflows, presets, trigger
topology, paged graphs, recipes, performance) in
`Engine\Plugins\Experimental\Toolsets\MetaSoundToolset\Content\Python\metasound_toolset\skills\`
**[source]**. The rules from them that bite:
- A graph is edited through a named **builder**. `CreateSourceBuilder` + `BuildToAsset` makes a
  **new** asset; `AttachBuilderToAsset` + `CommitBuilderToAsset` saves an existing one.
  `BuildToAsset` from an attached builder silently writes a duplicate.
- The output format (Mono, Stereo) and one-shot or not are fixed when the builder is created.
- An input pin takes exactly one edge. Merge triggers with `Trigger Any`; wait for several with
  `Trigger Accumulate`.
- A one-shot source must drive `UE.Source.OneShot.OnFinished`, or its voice never frees.
- Cycles are rejected at build, not at connect.
- Remove `Print Log` nodes and unused nodes: graphs are not optimised, and log nodes are not
  stripped from shipping builds.
- Active wave players are the main cost. One MetaSound, one purpose.

Scriptable editor subsystems found in the editor's Python API **[live]**:
- `AudioPreviewSubsystem`: `preview_sound`, `set_preview_float_parameter`, `stop_preview`,
  `is_preview_available` (used by `ue_audio_check.py`);
- `AudioAnalysisSubsystem.analyze_sound_waves`: per wave, integrated and ungated loudness, loudness
  range, sample and true peak, leading and trailing silence, `loop_seam_jump_db` (used by
  `ue_audio_check.py`; its numbers matched `audio_report.py` to 0.1 dB on the test files);
- `AudioMixerLibrary.start_recording_output` / `stop_recording_output`: record the main output to
  `<Project>/Saved/BouncedWavFiles/<path>/<name>.wav`;
- `AudioRuntimeStateSubsystem`: `get_active_sound_mixes`, `get_sound_class_current_properties`,
  mute and solo of classes and submixes (seen, not exercised);
- `AudioInsightsQuerySubsystem`: `find_output_loudness`, `find_sounds`, `find_virtual_loops`
  (seen, not exercised);
- `AudioAssetEditorSubsystem`: `import_sound_wave`, `set_submix_parent`, `save_audio_settings` (the
  last writes `DefaultEngine.ini`, which is Ryan's file: do not call it).

## 4. The fast path: `scripts/ue_audio_build.py` **[live]**

One JSON spec creates the mix assets, imports the waves and builds the MetaSounds. All of it was
run against the scratch editor: 14 assets created, `ValidateAssets` clean, values read back, and
the results rendered and measured (section 10).

```json
{
  "mix": [
    {"type": "SoundClass", "path": "/Game/TriadDrift/Audio/Mix/SC_TD_Master"},
    {"type": "SoundClass", "path": "/Game/TriadDrift/Audio/Mix/SC_TD_SFX",
     "properties": {"parentClass": "/Game/TriadDrift/Audio/Mix/SC_TD_Master"}},
    {"type": "SoundAttenuation", "path": "/Game/TriadDrift/Audio/Mix/ATT_TD_Space_Medium",
     "properties": {"attenuation.attenuationShapeExtents": {"x": 3000, "y": 0, "z": 0},
                    "attenuation.falloffDistance": 57000,
                    "attenuation.nonSpatializedRadiusStart": 2500,
                    "attenuation.bAttenuateWithLPF": true, "attenuation.lPFRadiusMin": 3000,
                    "attenuation.lPFRadiusMax": 60000, "attenuation.lPFFrequencyAtMin": 20000,
                    "attenuation.lPFFrequencyAtMax": 2500}},
    {"type": "SoundConcurrency", "path": "/Game/TriadDrift/Audio/Mix/SCON_TD_Weapon",
     "properties": {"concurrency.maxCount": 6, "concurrency.resolutionRule": "StopQuietest",
                    "concurrency.retriggerTime": 0.03, "concurrency.voiceStealReleaseTime": 0.03}}
  ],
  "import": [
    {"file": "export/SFX_Weapon_Lancer_Cannon_01.wav",
     "path": "/Game/TriadDrift/Audio/SFX/Weapons/SW_Weapon_Lancer_Cannon_01"}
  ],
  "oneshots": [
    {"path": "/Game/TriadDrift/Audio/SFX/Weapons/MSS_Weapon_Lancer_Cannon", "format": "Mono",
     "sounds": ["/Game/TriadDrift/Audio/SFX/Weapons/SW_Weapon_Lancer_Cannon_01"],
     "volume_db": -6, "volume_spread_db": 1.5, "pitch_semitones": 0, "pitch_spread": 0.7,
     "sound": {"soundClassObject": "/Game/TriadDrift/Audio/Mix/SC_TD_SFX",
               "attenuationSettings": "/Game/TriadDrift/Audio/Mix/ATT_TD_Space_Medium",
               "concurrencySet": ["/Game/TriadDrift/Audio/Mix/SCON_TD_Weapon"]}}
  ],
  "loops": [
    {"path": "/Game/TriadDrift/Audio/SFX/Engines/MSS_Engine_Lancer", "format": "Mono",
     "wave": "/Game/TriadDrift/Audio/SFX/Engines/SW_Engine_Lancer_Loop",
     "param": "Speed01", "pitch_range": [-2, 5], "gain_range_db": [-14, -9], "smooth_seconds": 0.15,
     "sound": {"soundClassObject": "/Game/TriadDrift/Audio/Mix/SC_TD_SFX",
               "attenuationSettings": "/Game/TriadDrift/Audio/Mix/ATT_TD_Space_Near"}}
  ],
  "music": [
    {"path": "/Game/TriadDrift/Audio/Music/MSS_Music_Space", "param": "Intensity", "smooth_seconds": 1.5,
     "layers": [{"wave": "/Game/TriadDrift/Audio/Music/SW_Music_Space_L0_Pad", "fade_in": [0, 0]},
                {"wave": "/Game/TriadDrift/Audio/Music/SW_Music_Space_L1_Pulse", "fade_in": [0.25, 0.55]},
                {"wave": "/Game/TriadDrift/Audio/Music/SW_Music_Space_L2_Drums", "fade_in": [0.6, 0.9]}],
     "sound": {"soundClassObject": "/Game/TriadDrift/Audio/Mix/SC_TD_Music"}}
  ]
}
```

Notes:
- Paths in `"properties"` and `"sound"` that start with `/` become object references. Relative
  `"file"` entries are resolved against the spec's own folder.
- Property names are the ones `ObjectToolset.ListProperties` prints: the C++ name with its first
  run of capitals lower-cased (`soundClassObject`, `attenuationSettings`, `concurrencySet`,
  `bLooping`, `lPFRadiusMin`, `dBAttenuationAtMax`). When unsure, list them first.
- Setting `parentClass` on a Sound Class also added it to the parent's `childClasses` **[live]**.
- Running the spec again is safe: waves are re-imported, mix-asset properties re-applied, a
  one-shot's `Sounds` and level / pitch values are updated in place (attach, set defaults, commit);
  an existing loop or music graph is left alone and only its `"sound"` block is re-applied.
- `falloffDistance` is measured from the inner shape outwards: silent beyond inner + falloff.
  The default attenuation is `distanceAlgorithm: Linear`, `attenuationShape: Sphere`, `bSpatialize:
  true` **[live]**; for a more natural curve set `"attenuation.distanceAlgorithm": "NaturalSound"`
  (enum value not verified: check `GetPropertySchema`).

## 5. What the built graphs are **[live]**

**One-shot** (`"oneshots"`): graph inputs `Sounds` (WaveAsset:Array), `PitchBase` and
`PitchRandomSpread` (semitones), `VolumeBase` and `VolumeRandomSpread` (dB). On play: `Random Get`
picks a take (`No Repeats` 1) and its `On Next` starts a `Wave Player`; two `Random (Float)` nodes
give pitch and level in ±spread; level goes through `Decibels to Linear Gain` into
`Multiply (Audio by Float)`; the player's `On Finished` drives the source's `OnFinished`. It is the
same pattern, with the same input names, as the engine's own
`/Metasound/DefaultAssets/Sources/MSS_PlayRandom_Oneshot` (stereo; a preset of it was also built and
worked). The script builds its own so that 3D sounds can be **mono**.

**Loop** (`"loops"`): not one-shot. Inputs `Wave` and one float parameter (default name `Speed01`).
`Wave Player` with `Loop` true; the parameter goes through two `MapRange` nodes (to semitones and to
dB) and two `InterpTo` smoothers into `Pitch Shift` and the gain. Rendered check: a 110 Hz test
loop came out at 98 Hz with the parameter at 0 and 147 Hz at 1 (range -2 to +5 semitones), and
8.7 dB louder (range -8 to 0 dB).

**Music** (`"music"`): stereo, not one-shot. One `Wave Player (Stereo)` per layer, all started by
`OnPlay`, `Loop` and `Maintain Audio Sync` true; each layer's gain is `MapRange(Intensity)` through
`InterpTo` into an `Audio Mixer (Stereo, N)`. A layer with `fade_in: [0, 0]` is always on; one with
`[0.25, 0.55]` is silent below 0.25 and full above 0.55. Rendered check with three identical test
layers: Intensity 1 was 9.5 dB louder than Intensity 0, which is exactly three coherent layers, so
they play in sync.

Node pins used (described through `DescribeNodeClass`; class triple is namespace / name / variant):

| Node | Class triple | Inputs | Outputs |
|---|---|---|---|
| Wave Player | `UE` / `Wave Player` / `Mono` or `Stereo` | `Play`, `Stop`, `Wave Asset`, `Start Time`, `Pitch Shift` (semitones), `Loop`, `Loop Start`, `Loop Duration`, `Maintain Audio Sync` | `On Play`, `On Finished`, `On Nearly Finished`, `On Looped`, `Out Mono` or `Out Left` / `Out Right`, … |
| Random Get | `Array` / `Random Get` / `WaveAsset:Array` | `Next`, `Reset`, `In Array`, `Weights`, `Seed`, `No Repeats` | `On Next`, `Value`, `Index` |
| Random (Float) | `UE` / `RandomFloat` / none | `Next`, `Reset`, `Seed`, `Min`, `Max` | `On Next`, `Value` |
| Multiply | `UE` / `Multiply` / `Audio by Float` or `Float` | `PrimaryOperand`, `AdditionalOperands` | `Out` |
| Add | `UE` / `Add` / `Float` | `PrimaryOperand`, `AdditionalOperands` | `Out` |
| dB to gain | `UE` / `Decibels to Linear Gain` / `Float` | `Decibels` | `Linear Gain` |
| Map range | `MapRange` / `MapRange` / `Float` | `In`, `In Range A`, `In Range B`, `Out Range A`, `Out Range B`, `Clamped` | `Out Value` |
| Smoother | `UE` / `InterpTo` / `Audio` | `Interp Time`, `Target` | `Value` (a float) |
| Mixer | `AudioMixer` / `Audio Mixer (Stereo, 3)` / none | `In 0 L`, `In 0 R`, `Gain 0`, … | `Out L`, `Out R` |
| Trigger merge | `TriggerAny` / `Trigger Any (2)` / none | `In 0`, `In 1` | `Out` |
| Wait for all | `TriggerAccumulator` / `Trigger Accumulate (3)` / none | `In 0`…, `Auto Reset` | `Out` |
| Envelopes | `AD Envelope` / `AD Envelope` / `Audio`; `ADSR Envelope` / `ADSR Envelope` / `Audio` | `Trigger` or `Trigger Attack` / `Trigger Release`, times, curves | `On Done`, `Out Envelope` |
| Filters | `UE` / `Biquad Filter`, `Ladder Filter`, `State Variable Filter`, `One-Pole Low Pass Filter` / `Audio` | `In`, `Cutoff Frequency`, … | `Out` |
| Generators | `UE` / `Sine`, `Saw`, `Noise`, `LFO` / `Audio` | `Frequency`, … | `Audio` (`LFO`: `Out`, a float) |
| Dynamics | `UE` / `Compressor`, `Limiter` / `Audio` | `Audio`, `Threshold dB`, … | `Audio` |
| Space | `UE` / `Plate Reverb` / `Stereo`; `UE` / `Stereo Panner`; `UE` / `Delay`, `Stereo Delay` | | |

Source interface nodes: `UE.Source.OnPlay`, `UE.Source.OneShot.OnFinished`,
`UE.OutputFormat.Mono.Audio:0`, `UE.OutputFormat.Stereo.Audio:0` and `:1`. Other registered
interfaces: `UE.Attenuation`, `UE.Spatialization`, `UE.Source.Location`, `UE.Listener.Location`,
`UE.Source.Occlusion`, `UE.World.Time`. Pin names differ by class and version: describe before
wiring anything not in this table.

## 6. Import settings **[live for names and defaults; docs for behaviour]**

- Epic's import page: wav, ogg, flac, aif, opus, mp3; 16 or 24 bit; any sample rate; mono, stereo,
  4.0, 5.1, 7.1. Imports become 16-bit internally and 24-bit files are not dithered on the way, so
  import the 16-bit files `audio_prep.py` writes.
- A fresh import has `soundAssetCompressionType: ProjectDefined`, `loadingBehavior: Inherited`,
  `compressionQuality: 80`, `sampleRateQuality: Max`, `virtualizationMode: Restart`.
- Compression choices: `BinkAudio`, `ADPCM`, `PCM`, `Opus`, `PlatformSpecific`, `ProjectDefined`,
  `RADAudio`. Leave `ProjectDefined` unless a sound has an audible codec problem (then `PCM` for a
  short UI sound, never for music: size).
- Loading: `Inherited`, `RetainOnLoad`, `PrimeOnLoad`, `LoadOnDemand`, `ForceInline`. Epic's stream
  caching page: retain on load for sounds that must start with no delay, prime on load for sounds
  expected soon. Set `RetainOnLoad` on player weapons, the switch cut, "CUT!", UI and the engine
  loops; leave the rest inherited.
- Loops played directly need `bLooping: true`; loops played by a MetaSound loop through the Wave
  Player's `Loop` pin instead (the flag does no harm).
- The Sound Wave has `lUFS` and `samplePeakDB` fields; they read 0 right after `ImportFile` in the
  headless editor. Use `ue_audio_check.py` (`analyze_sound_waves`) or `audio_report.py` instead.
- Budget: under 1 GB for the whole package. 60 s of stereo music at 48 kHz is about 11 MB as PCM
  before compression; five music states of three 30 s layers is about 90 MB uncompressed. Keep
  layers to 16 or 32 bars.

## 7. Slow motion, "CUT!" and ducking: Sound Mixes **[live]**

The `AudioModulation` plugin (control buses, bus mixes) is **not enabled** in
`unrealGameJam.uproject` and is off by default in the engine (`AudioModulation.uplugin`
`EnabledByDefault: false`) **[source]**. Enabling it is a `.uproject` change (Ryan's file). The
built-in Sound Class + Sound Mix system needs no plugin and was verified, so use it:

- A Sound Mix (`MIX_TD_*`) holds `soundClassEffects[]`: `soundClassObject`, `volumeAdjuster`,
  `pitchAdjuster`, `lowPassFilterFrequency`, `bApplyToChildren`; plus `fadeInTime`, `fadeOutTime`,
  `duration` (-1 = until popped), `initialDelay`, and an optional EQ.
- Push and pop at runtime: `UGameplayStatics::PushSoundMixModifier(World, Mix)` /
  `PopSoundMixModifier`; one-off overrides with `SetSoundMixClassOverride(World, Mix, Class, Volume,
  Pitch, FadeInTime, bApplyToChildren)`; all are Blueprint nodes.
- **Rendered check:** with a mix of pitch 0.6 and volume 0.8 on the SFX class pushed, a MetaSound
  loop in that class came out at 58.5 Hz instead of 98 Hz (×0.6) and 1.96 dB quieter (×0.8). So
  Sound Class pitch and volume do reach MetaSound sources.
- `UGameplayStatics::SetGlobalPitchModulation` exists but did nothing to the editor preview in the
  same test; its own doc says the fade only moves while a game world ticks. Not verified in a game:
  prefer the Sound Mix.
- Passive ducking needs no code: a Sound Class has `passiveSoundMixModifiers[]` (`soundMix`,
  `minVolumeThreshold`, `maxVolumeThreshold`): while a sound of that class is audible, the mix is
  applied. Put `MIX_TD_DuckForVO` on `SC_TD_VO` (seen as properties; not exercised).
- Mix values are in references/mix-standard.md §6.

If Ryan enables Audio Modulation later: control buses (`SoundControlBus`), bus mixes
(`SoundControlBusMix`) and `AudioModulationStatics.activate_bus_mix` / `set_global_bus_mix_value`
exist and the assets can be created by script (both factories worked) **[live for creation only]**.

## 8. The audio director (a Blueprint Nicholas's side owns; not built, not tested)

Sounds on cues need no code. Three things do: music, mixes, and the master limiter. Put them in one
actor, `BP_TD_AudioDirector` (`/Game/TriadDrift/Audio/`), placed once per map as a new One File Per
Actor actor (claim it on `CLAIMS.md` first; maps are shared). It holds one persistent 2D
`AudioComponent` per music MetaSound and binds to events that already exist **[source]**:

| Event (all `BlueprintAssignable`) | Audio director does |
|---|---|
| `UTDEventBus::OnWaveStarted(WaveNumber, Intensity01)` | `SetFloatParameter("Intensity", Intensity01)` on the music component (the MetaSound smooths it) |
| `OnWaveCleared` | Intensity to 0.15 |
| `OnPhaseChanged(Phase)` | Cross-fade to that phase's music MetaSound (2 s) |
| `OnHighlightMoment(Kind, Subject, Importance01)` or `UTDDirectorSubsystem::OnDirectorShot(Shot)` | Play the slow-mo enter sting, push `MIX_TD_SlowMo`; pop it and play the exit sting when the shot ends |
| `OnSceneCut(SceneId, NextTake, DelaySeconds)` | Play "CUT!", push `MIX_TD_Cut` |
| `OnTakeStarted(SceneId, TakeNumber)` | Pop `MIX_TD_Cut`, play the slate |
| `OnHyperdriveState`, `OnPortalState` | Only if a state has no cue slot (the `UTDTravelFX` cues cover most) |
| `OnAbilityUnlocked`, `OnMarkerRevealed`, `OnShipDowned`, `OnSquadWiped` | Stingers |
| `OnBarkRequested(BarkId, Speaker)` | VO queue (ART-11) |
| `ATDSquadManager::OnSwitchCut` | Nothing: engineering already plays `SwitchIn` |

### Live state: one object to read instead of many (on `main`, engineering-owned, tested)

`UTDAudioStateSubsystem` (`Source/unrealGameJam/TriadDrift/Core/TDAudioState.h`) is a world subsystem
that exists in every game world. Blueprint: **Get TD Audio State Subsystem → Get State**. It returns
one `FTDAudioState` snapshot, refreshed every frame, built only from gameplay contracts:

| Field | Meaning |
|---|---|
| `Scene` | `FrontEnd` (menu, transition), `Space`, `Ground` |
| `Phase` | Story phase (`Intro` ... `DominionAssault` ... `Victory`) |
| `ActiveShip`, `Speed01`, `Boost01`, `Heat01`, `Health01`, `Shield01` | The ship the player flies (health of the character on foot) |
| `Danger01` | 0 above 35% hull, 1 at 0%: low-health filter or heartbeat. Smoothed |
| `SquadAlive`, `bOutOfBounds` | 0..3; outside the play area |
| `WaveNumber`, `WaveIntensity01`, `EnemiesAlive`, `EnemiesTotal`, `bWaveActive`, `bInBreather` | The wave director's numbers |
| `MissileWarning` | `None`, `Tracking`, `Imminent` |
| `SecondsSinceHit`, `SecondsSinceKill`, `SecondsSinceHighlight`, `LastHighlight` | Real seconds (1000 = never) |
| `Combat01` | How hot the fight is now (enemies present, recent hits and kills, missiles). Smoothed |
| `Hyperdrive`, `HyperdriveCharge01` | `Idle`, `Spooling`, `Jumping`, `DropOut` |
| `TimeDilation`, `bSlowMotion` | `bSlowMotion` is a director beat, not a 40 ms hit-stop: push `MIX_TD_SlowMo` while it is true |
| `bSceneCut` | Between "CUT!" and the next take |
| `MusicIntensity01` | A ready-made music drive: front end 0, cruising 0.15, breather 0.25, wave 0.4 to 0.9, on foot 0.3 to 0.7, Dominion assault 0.8 or more, "CUT!" 0.1, victory 0.3. Rises 0.5 per second, falls 0.2 per second |

- Smoothed values move in **real** time, so slow motion does not slow the music's reaction.
- `MusicIntensity01` is a suggestion. The raw fields stay available for a different mapping, and
  `GetMusicIntensityTarget()` returns the unsmoothed value.
- Simplest music wiring: each tick, `SetFloatParameter("Intensity", GetMusicIntensity01())` on the
  music component; switch the music MetaSound when `Phase` or `Scene` changes.
- Tests: `TriadDrift.Core.AudioState.*` (three, at 30 / 60 / 120 fps). Not yet used by any audio.

Rules:
- **Slow motion slows game-time timers.** A Blueprint `Delay` of 0.6 s under a dilation of 0.3
  lasts two real seconds. End the slow-mo mix by polling `UTDFeelSubsystem::IsSlowMoActive()` or
  `UTDDirectorSubsystem::IsShotActive()` each tick (both `BlueprintPure`), not with a delay. The
  shot's `DurationSeconds` is real time.
- `OnHighlightMoment` is suppressed during a retake and during travel (docs/eng/director.md), so the
  mix cannot stick there; still pop every mix on `OnTakeStarted` and `OnPhaseChanged` as a safety.
- Master limiter without touching project settings: on BeginPlay, add a dynamics-processor preset to
  the main submix with `AudioMixerLibrary.AddMasterSubmixEffect` (Blueprint: "Add Master Submix
  Effect"). A `SubmixEffectDynamicsProcessorPreset` asset was created by script **[live]**; its
  settings (`dynamics_processor_type` Limiter, `threshold_db`, `look_ahead_msec`,
  `attack_time_msec`, `release_time_msec`, `key_source`) and the runtime call were not exercised.
  Types available: Compressor, Limiter, Expander, Gate, UpwardsCompressor; key sources: Default,
  AudioBus, Submix.

## 9. Music

- **Vertical layers** (the default here): one MetaSound per game state, 2-4 stems of identical
  length, one `Intensity` float. Vertical layering adds and removes instrument layers over a track
  that keeps playing; horizontal re-sequencing switches between cues (definitions: Michael Sweet,
  Berklee Online; Epic's Fortnite glossary).
- Stems must be the same length to the sample, the same tempo and key, and cut on a bar boundary:
  `audio_report.py <stems> --category music --same-length`. Bar length in seconds = 240 / BPM for
  4/4; a 16-bar loop at 120 BPM is 32.000 s.
- Layer plan that works with a 0..1 intensity: L0 pad or drone (always on), L1 pulse or bass
  (0.25-0.55), L2 drums and lead (0.6-0.9). `OnWaveStarted` already carries `Intensity01`.
- **Two-track fallback** (library music that has no stems): two music components, calm and combat,
  cross-faded over 2 s by the audio director on `OnWaveStarted` / `OnWaveCleared`. No sync needed.
- **Stingers on the beat:** Quartz. `QuartzSubsystem.create_new_clock`, the clock handle's
  `set_beats_per_minute`, `start_clock`, `subscribe_to_quantization_event`, and
  `AudioComponent.play_quantized` exist in 5.9 **[live: names only]**; Epic's overview describes
  Quartz as a Blueprint scheduling system for sample-accurate playback on bar and beat boundaries
  **[docs]**. Not exercised. For the jam, unquantised stingers in the music's key are acceptable;
  quantise only "ability unlocked" and "wave start" if time allows.
- State changes: fade the old MetaSound out over 2 s while the new one fades in; do not hard-cut
  except on "CUT!".

## 10. Checking without ears: `scripts/ue_audio_check.py` **[live]**

Needs the `-NonRealtimeAudio` editor of section 2.
- `"waves"`: the engine's own analysis of imported waves (loudness, peaks, silence, loop seam).
- `"render"`: plays a sound asset on the editor preview voice N times while recording the main
  output; the WAV lands in `<Project>/Saved/BouncedWavFiles/ue-game-audio/`. Then:
  `audio_report.py` (is it silent? clipped?), `audio_view.py` (are the plays different from each
  other? does the loop run clean? did the parameter change anything?).
- What it showed in the test: five plays of a one-shot, each a different take and level; the loop's
  pitch and level following its parameter; the music layers adding in sync; the slow-mo mix.
- Limits: the preview voice is not the game. Distance attenuation, the listener and gameplay timing
  are absent, and the absolute level differed from the expected in-game level by several dB. The
  recording's timeline is not wall-clock time in non-realtime mode. Use it for "what does this
  asset play" and for A/B comparisons only.

Capturing the **game's** mix: start the game (not the editor) with `-NonRealtimeAudio`, record the
main output during play (Blueprint "Start Recording Output" / "Stop Recording Output", or the same
`AudioMixerLibrary` calls through `execute_python` if the game session exposes the Python toolset),
then `audio_report.py --mix`. This path was **not tested**; `Tools/td-play-mcp.ps1` passes
`-nosound`, so it needs a variant of that script from Ryan's side.

## 11. Putting a sound into the game

1. The data asset: `/Game/TriadDrift/Presentation/DA_<Name>` (names in
   docs/visual/Presentation-Asset-Paths.md). Create it if missing:
   `EditorToolset.DataAssetToolset.Create(folderPath, assetName, assetType)` with the C++ class
   (`/Script/unrealGameJam.TDShipVisuals`, `TDWeaponFX`, `TDEnemyVisuals`, `TDDominionVisuals`,
   `TDAbilityFX`, `TDTravelFX`, `TDBladeFX`, `TDUIStyle`).
2. Find the slot: `ObjectToolset.ListProperties(instance, root="", maxDepth=2)`; an `FTDFXCue` slot
   lists as `<slot>.vFX`, `<slot>.sound`, `<slot>.shake`… (the exact spelling was not checked on
   these classes: list first).
3. Set it: `ObjectToolset.SetProperties(instance, values='{"muzzle.sound": {"refPath": "/Game/TriadDrift/Audio/SFX/Weapons/MSS_Weapon_Lancer_Cannon.MSS_Weapon_Lancer_Cannon"}}')`,
   then `AssetToolset.SaveAsset`.
4. Everything in `/Game/TriadDrift/Presentation/` is always cooked with what it references
   (AGENTS.md §11), so a sound referenced from a `DA_*` slot ships. A sound loaded only by a path
   string from anywhere else will be missing from the packaged game: music and mixes must be
   referenced by the audio director Blueprint (hard references), which must itself be placed in a
   cooked map.

Slot map: references/project-conventions.md.

## 12. Pitfalls

| Symptom | Cause and fix |
|---|---|
| A sound is heard at full level from across the map | No attenuation asset on the sound (section 1) |
| The player's own weapon pans hard left or right | The inner non-spatialised radius is smaller than the camera distance (1400 cm): use the `Space` presets |
| A one-shot never ends, voices run out | `OnFinished` not wired (hand-built graphs) |
| Rapid fire sounds like a machine | One take, no spread: 3-6 takes, `No Repeats`, ±0.7 semitone, ±1.5 dB |
| `BuildToAsset` made a second asset | It was called on an attached builder: use `CommitBuilderToAsset` |
| `SetProperties` returns false | Wrong property spelling: `ListProperties` first; object values need `{"refPath": …}` |
| `execute_python` rejects the call | `script` must be `{"content": …, "language": "Python"}`, not a string; imports only at top level |
| Preview unavailable, render fails | The editor was started with `-nosound` |
| Slow motion sounds normal | Nothing reacts to time dilation by itself: push `MIX_TD_SlowMo` |
| The slow-mo mix lasts too long | A game-time `Delay` under dilation: poll real-time state instead |
| Music layers drift apart | Stems differ in length, or `Maintain Audio Sync` is off |
| Sound missing in the packaged build | Referenced by path only, or the audio director is not in a cooked map |
| `/Game/...` becomes `C:/Program Files/Git/Game/...` | Git Bash path conversion: `export MSYS_NO_PATHCONV=1`, or put arguments in a JSON file |

## Unreal's own skill format

`Engine\Extras\AgentTools\Skills\README.md` **[source]**: a skill is `<root>/Skills/<name>/SKILL.md`
(front matter `name` and `description`) with its scripts in `scripts/` beside it. The editor's
`AgentSkillToolset` (`ListSkills`, `GetSkills`) finds skills in `<Project>/Skills/`, in each enabled
plugin's `Skills/` folder and in `Engine/Extras/AgentTools/Skills/`. This skill follows that layout;
mirror it for the editor with `robocopy .claude\skills Skills /MIR` (AGENTS.md §10).

## Epic documentation pages (UE 5.9, loaded 2026-11-12)

MetaSounds reference: https://dev.epicgames.com/documentation/en-us/unreal-engine/metasounds-reference-guide-in-unreal-engine ·
function nodes: https://dev.epicgames.com/documentation/en-us/unreal-engine/metasound-function-nodes-reference-guide-in-unreal-engine ·
attenuation: https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-attenuation-in-unreal-engine ·
concurrency: https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-concurrency-reference-guide ·
submixes: https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-submixes-in-unreal-engine ·
sound classes: https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-classes-in-unreal-engine ·
sound class mix (described there as due to be replaced by Audio Modulation): https://dev.epicgames.com/documentation/unreal-engine/sound-class-mix ·
audio modulation: https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-modulation-overview-in-unreal-engine ·
Quartz: https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-quartz-in-unreal-engine ·
importing: https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-audio-files ·
stream caching: https://dev.epicgames.com/documentation/en-us/unreal-engine/an-overview-of-audio-stream-caching-in-unreal-engine ·
Unreal MCP: https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor
