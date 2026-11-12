# Game audio practices for Triad Drift

Research note behind the `ue-game-audio` skill (`.claude/skills/ue-game-audio/`). Written
2026-11-12, day one of the jam, by Ryan's side for Nicholas's audio agents. Everything dated below
was read or run on that day. Licence terms are paraphrased, with the link to the page; read the page
itself before relying on a term.

## 1. Summary

- **The problem.** The game has no audio. The first attempt (browser synthesis) was rejected, and
  `AAA.md` rules out placeholder-quality and retro-style audio. `docs/visual/STATUS.md` (ART-10)
  stops further synthetic work and asks for licensed professional options.
- **The answer the skill gives.** Recorded or generated material, designed in layers from a written
  recipe, levelled to one standard, played by MetaSound sources that vary every play, with every
  3D sound carrying its own attenuation, class and concurrency, and approval by ear from Nicholas.
- **What transferred from the owner's previous game.** Not its synthesis, its mix layer: one anchor
  and a dB table, own actions head-locked and loudest, distance attenuation with a distance
  low-pass, ±4% pitch and ±1.5 dB per play, several takes, voice stealing by audibility, a limiter
  that only works on pile-ups, a quiet ambience bed. `references/mix-standard.md` §1 maps each rule
  to its Unreal equivalent.
- **What was verified.** Four ffmpeg scripts and three Unreal scripts were written and run. The
  Unreal side was exercised live through the engine's built-in MCP server on a scratch project:
  imports, sound classes, attenuation, concurrency, a sound mix, mono and stereo one-shot
  MetaSounds, a parameter-driven loop and a three-layer music MetaSound were created; the results
  were rendered offline and measured (pitch, level and layer sync as designed).
- **What was not.** Nothing has been heard by a person. Nothing was run inside the Triad Drift
  project. The audio director Blueprint, a runtime master limiter, Quartz and a capture of the whole
  game mix are described, not built.

## 2. Findings that change the plan

1. **Gameplay plays cues with no overrides** (`UTDStatics::PlayCue` →
   `PlaySoundAtLocation(World, Sound, Location, Rotation)`; loops → `SpawnSoundAttached`). A sound
   asset without its own attenuation is 2D at full level.
2. **Gameplay sets no audio parameters**, and `UTDShipVisuals::EngineLoop` is read by no code at
   `f0bee9c`; the engine loop that plays is `EngineTrail.Sound`. An engine that follows speed needs
   a one-line `SetFloatParameter` from engineering.
3. **Slow motion does not touch audio by itself.** No engine audio source file refers to time
   dilation. A Sound Mix does the job: with pitch 0.6 on the SFX class pushed, a MetaSound loop
   rendered at 58.5 Hz instead of 98 Hz and 1.96 dB quieter (volume 0.8).
4. **Audio Modulation is not enabled** in the project and is off by default in the engine, so
   control buses would need a `.uproject` change. Sound Class + Sound Mix needs nothing.
5. **`Tools/td-play-mcp.ps1` starts the editor with `-nosound`**: MetaSound pre-save is skipped
   (warning in the log) and nothing can be previewed or rendered. `-NonRealtimeAudio` renders
   offline with nothing on the speakers.
6. **Unreal 5.9 can measure and render audio from a script**: `AudioAnalysisSubsystem`
   (loudness, true peak, silence, loop seam), `AudioPreviewSubsystem`, and
   `AudioMixerLibrary.start/stop_recording_output`. `MediaToolset` has no audio tool.
7. **The engine ships two MetaSound sources** that already do random take + pitch + level:
   `/Metasound/DefaultAssets/Sources/MSS_PlayRandom_Oneshot` and `MSS_PlayRandom_Loop` (both stereo).
8. **Licences.** CC-BY-SA and GPL audio cannot be combined with Unreal (EULA §6(c)). Sonniss's
   licence (v2.0, 27 Aug 2026) allows commercial game use without credit, forbids passing the sounds
   on (modified or not) and any AI training use, and allows sharing inside the team. The jam's audio
   partner (WeLoveIndies) is non-commercial, project-locked, capped at 100 sounds and 10 tracks, and
   asks for its intro. Mixkit's music licence excludes video games. Udio's terms forbid downloads.
   ElevenLabs' free plan is non-commercial; Stable Audio's pricing page states a commercial licence
   on every plan including the free one (read in a rendered browser; re-check).
9. **A one-shot has no integrated LUFS.** BS.1770 gating needs complete 400 ms blocks. The engine's
   own analysis returns -160 for a 0.4 s file, ffmpeg returns -70. The scripts use K-weighted
   ungated loudness under 3 s and level one-shots by true peak.

## 3. Sources read

### Repo (commit `f0bee9c`)
`AGENTS.md` (§2, 5, 6, 10, 11), `AAA.md`, `features.md` (§1-3, ART-09 to ART-12, I-4),
`docs/visual/STATUS.md`, `docs/visual/Presentation-Asset-Paths.md`, `docs/eng/director.md`,
`docs/eng/world.md`, `CREDITS.md`, `Tools/td_mcp.py`, `Tools/td-play-mcp.ps1`, `Tools/td-python.ps1`,
`Source/unrealGameJam/TriadDrift/Core/TDPresentation.h`, `TDEventBus.h`, `TDStatics.cpp/.h`,
`TDTypes.h`, `Flight/TDShip.cpp/.h`, `Squad/TDSquadManager.cpp`, `Squad/TDSquadIntegration.cpp`,
`Director/TDDirectorSubsystem.h`, `Combat/TDWeaponComponent.cpp`, `unrealGameJam.uproject`,
`ArtSource/Audio/StarterPlan/README.md` and `cues.json` (32 cues, all unacquired).

### The owner's previous game (read only; nothing copied)
`client/src/audio/README.md`, `soundDefs.ts`, `gainMath.ts`, `variation.ts`, `voiceSteal.ts`,
`occlusion.ts`, `synth/markerRecipes.ts`, `synth/render.ts` (names only), `docs/AUDIO_WIRING.md`.
Rules extracted:
- Level table in dB relative to the marker shot (linear 0.53 for 0 dB); buffers peak-normalised to
  0.9; own shot +2 dB and head-locked; UI on its own bus at -3 to -20 dB; ambience -24 dB.
- Distance: inverse model (full inside a reference distance), with a smoothstep fade to zero over
  the last 40% of the maximum distance; per-sound reference, roll-off and maximum.
- Distance low-pass: cutoff = 20 kHz / (1 + distance / half-distance), floor 700 Hz.
- Occlusion: 0..1 from a game ray-cast; fully occluded = -7 dB and a ceiling of 2.5 kHz; cached per
  1 m source cell for 0.25 s; loops re-probed every 70 ms and smoothed over 80 ms.
- A far take for the shot at 20 m and beyond (duller, short tail), +3 dB to match at the hand-over.
- Per play: rate ±4%, gain ±1.5 dB, never the same take twice in a row; tonal cues excluded.
- Up to 8 numbered takes per id; recorded files override synthesised ones.
- 32 world voices; when full, the quietest at the listener is cut (ties: farthest, then oldest); a
  new sound quieter than all playing ones is dropped; 12 separate head-locked voices that world
  sounds cannot steal.
- Master limiter at -3 dBFS, hard knee; one own shot peaks just under it.
- A recipe = a few layers with one job each (the shot: band-passed burst, airy tail, low thump,
  valve tick), rendered offline with a seed.

### Engine on this machine (`C:\Program Files\Epic Games\UE_5.9`, 5.9.0, CL 60351404)
- `Engine\Extras\AgentTools\Skills\README.md` (skill format and where the loader looks).
- `Engine\Plugins\Experimental\Toolsets\MetaSoundToolset`: `MetaSoundAuthoringToolset.h`,
  `MetaSoundDiscoveryToolset.h`, `MetaSoundInspectionToolset.h`, and the seven Python agent skills
  in `Content\Python\metasound_toolset\skills\`.
- `…\Toolsets\EditorToolset`: `SoundWaveToolset.h`, `ObjectToolset.h`, `AssetToolset.h`,
  `DataAssetToolset.h`, `Content\Python\editor_toolset\toolsets\python_scripting.py` (names).
- `…\Toolsets\MediaToolset\…\MediaToolset.h` (images and video only).
- Plugin descriptors: `Metasound` (on by default), `AudioModulation` (off by default, not
  experimental), `Harmonix` (off, experimental), `Soundscape` (off, beta), `ModelContextProtocol`
  (on by default, experimental), `MetaSoundToolset` (off by default; enabled by the project).
- `Engine\Source\Runtime\Engine\Classes\Kismet\GameplayStatics.h` (signatures of
  `PlaySoundAtLocation`, `SpawnSoundAttached`, `PushSoundMixModifier`, `SetSoundMixClassOverride`,
  `SetGlobalPitchModulation`), `Classes\Sound\SoundClass.h`, `SoundBase.h`, `SoundConcurrency.h`.
- `Engine\Source\Runtime\Engine\Private\AudioDeviceManager.cpp` (`-NonRealtimeAudio`,
  `-DeterministicAudio`), `AudioDevice.cpp`, `ActiveSound.cpp`, `Components\AudioComponent.cpp`,
  `SoundWave.cpp`, and the `AudioMixer` module (searched for time dilation: none).

### Web (research agents read the pages on 2026-11-12; two were re-read directly)
Licences and libraries: Sonniss licence and bundle pages; Fab EULA and audio category; Unreal Engine
EULA and Epic Content License Agreement; Kenney; Freesound FAQ and terms; OpenGameArt FAQ; Pixabay;
Zapsplat; 99Sounds; Mixkit; Soundly FAQ; Ovani; WeLoveIndies jam page and terms; the jam page;
Tallbeard and Void music packs; incompetech FAQ; Free Music Archive licence guide.
AI tools: ElevenLabs (sound effects docs and API reference, pricing, terms, sound-effects terms,
music docs and terms), Stability AI (Stable Audio pricing, FAQ, terms, API docs, model cards), Adobe
Firefly help, Meta AudioCraft repository, Google Lyria docs and terms, SFX Engine, Suno terms and
pricing, Udio terms, AIVA, Soundraw, Mubert, Beatoven, Loudly, fal model pages, Steam content
survey, itch.io quality guidelines.
Standards and method: Sony ASWG-R001 (PDF, third-party host), GANG IESD mix recommendations (PDF),
EBU Tech 3341 (PDF), ITU-R BS.1770 page, ffmpeg filter documentation and `f_ebur128.c` /
`af_loudnorm.c`, A Sound Effect (two articles), Splice, GANG looping article, filmsound.org (two
pages), Designing Sound and DICE slides on HDR audio, CRI Middleware on bullet-time audio, MCV on
Sniper Elite 4, Berklee Online and Game Developer on adaptive music, and Epic's UE 5.9 documentation
pages for MetaSounds, attenuation, concurrency, submixes, sound classes, audio modulation, Quartz,
importing and stream caching. URLs are in the skill's reference files next to each claim.

Could not be fetched (so anything resting on them is marked unverified in the skill): every
audiokinetic.com page, the FMOD music docs, the EA page for the Frostbite HDR talk, the jam's
Official Rules on Box, Stable Audio's pricing page by plain fetch (rendered browser only).
Re-read directly by the author: the Sonniss licence page (version, date, commercial use, no
attribution, no passing on, team sharing, AI clauses all confirmed) and the ElevenLabs sound
generation API reference (endpoint, `duration_seconds` 0.5-30, `prompt_influence` default 0.3,
`loop`, `model_id` default v2, `output_format`).

## 4. Licence findings

Details, links and what is not verified: `.claude/skills/ue-game-audio/references/sourcing.md`.

| Source | Use in the jam build | Later commercial use | Credit | Raw files in a shared repo | AI clause |
|---|---|---|---|---|---|
| Sonniss GDC bundles (licence v2.0) | Yes | Yes | None | Private team only | No AI training, development or enhancement |
| Fab Standard License | Yes | Yes | None | Private repo with collaborators: yes | NoAI-flagged items: no training input |
| Unreal template content ("Examples") | Yes | Yes | None | Yes | None |
| Engine / Starter Content audio | Cooked in a UE game | Yes | None | Not expressly granted | None |
| Kenney | Yes | Yes | None (CC0) | Yes | None |
| Freesound | CC0 and CC-BY only | Same | CC-BY: yes | Yes | Per uploader |
| OpenGameArt | CC0, OGA-BY, CC-BY only | Same | Except CC0 | Yes | None |
| Pixabay | Yes | Yes | None | Not standalone | No scraping for ML |
| Mixkit SFX / music | SFX yes / **music no** | Same | None | Not with source files | None |
| WeLoveIndies (jam offer) | Yes, with their intro, 100 sounds / 10 tracks | **Needs a paid licence** | Their intro | Not addressed | No AI training |
| ElevenLabs SFX | Paid plan only | Paid plan | None on paid | Outputs are yours | No resale as a sound library |
| Stable Audio 3 | Stated yes on all plans (re-check) | Stated yes | None found | Outputs are yours | No training competing models; not to be presented as human-made |
| Suno | Paid plans, permitted downloads only | Same | None | - | No training on outputs |
| Udio | **No** | No | - | - | - |
| Meta AudioCraft weights | **No** (CC-BY-NC) | No | - | - | - |

## 5. What was built

```
.claude/skills/ue-game-audio/
  SKILL.md
  references/sourcing.md  sound-design.md  unreal-implementation.md  mix-standard.md
             project-conventions.md  jam-plan.md
  scripts/audio_common.py    targets + measuring (ffmpeg / ffprobe, pure-Python K-weighting)
          audio_report.py    measure and judge files
          audio_prep.py      trim, fade, 48 kHz 16-bit, level, numbered takes, loop cross-fade
          audio_layer.py     build takes from a JSON recipe of layers
          audio_view.py      spectrogram + waveform PNG, contact sheet
          ue_run_python.py   send a Python file to a running editor (MCP execute_python)
          ue_audio_build.py  in-editor: mix assets, imports, MetaSound one-shots / loops / music
          ue_audio_check.py  in-editor: engine wave analysis, offline render of an asset
```

## 6. Tests: ffmpeg scripts

Machine: Windows 11, Python 3.14.0, ffmpeg 9.0.2 (gyan.dev full build). All test audio was generated
with ffmpeg; nothing was downloaded.

### Test material
```
# three raw "zaps": 44.1 kHz stereo 24-bit, 300 ms of silence before, 500 ms after, different levels
for i in 1 2 3:  ffmpeg -f lavfi -i "anoisesrc=d=0.5:c=white:a=0.{4,6,8}:s=$i,afade=t=out:st=0.01:d=0.45:curve=exp,lowpass=f={3000,6000,9000},adelay=300:all=1,apad=pad_dur=0.5" -ar 44100 -ac 2 -c:a pcm_s24le raw/zap_raw_$i.wav
# a raw engine bed: brown noise with tremolo + an 82 Hz tone, with a 1.2 s fade in (so its ends do not match)
ffmpeg -f lavfi -i "anoisesrc=d=6:c=brown:a=0.6:s=9,tremolo=f=0.37:d=0.6[n];sine=f=82:d=6,volume=3[s];[n][s]amix=inputs=2:normalize=0,afade=t=in:d=1.2" -ar 44100 -ac 2 -c:a pcm_s16le raw/engine_raw.wav
# defects
ffmpeg -f lavfi -i "sine=f=220:d=1,volume=40dB" -ar 48000 -ac 1 -c:a pcm_s16le bad/clipped.wav
ffmpeg -f lavfi -i "sine=f=220:d=1,volume=6dB,dcshift=0.05" -ar 48000 -ac 1 -c:a pcm_s16le bad/dc_offset.wav
ffmpeg -f lavfi -i "anullsrc=r=48000:cl=mono:d=1" -c:a pcm_s16le bad/silent.wav
# two "stems" of different length
ffmpeg -f lavfi -i "sine=f=110:d=8,volume=12dB"    -ar 48000 -ac 2 -c:a pcm_s16le stems/stem_a.wav
ffmpeg -f lavfi -i "sine=f=165:d=8.25,volume=12dB" -ar 48000 -ac 2 -c:a pcm_s16le stems/stem_b.wav
# layer sources for the recipe
ffmpeg -f lavfi -i "aevalsrc='0.8*sin(2*PI*(180*t-200*t*t))*exp(-9*t)':d=0.35:s=48000" -c:a pcm_s16le recipe/src/body_sweep.wav
ffmpeg -f lavfi -i "anoisesrc=d=0.7:c=pink:a=0.5:s=4,afade=t=out:st=0.05:d=0.65:curve=exp" -ar 48000 -ac 1 -c:a pcm_s16le recipe/src/tail_noise.wav
```

### Output (final versions of the scripts, one run)
Real output. Shortened only where marked: repeated table headers, rule lines and the legend line are
dropped, and text in round brackets or with "…" stands for lines identical to ones already shown.
```
py_compile: 6 files ok
compile with top-level await: ue_audio_build.py, ue_audio_check.py ok

$ python audio_report.py raw --category sfx
file                                 dur s   rate ch    LUFS       LRA     TP   peak   lead   tail       DC   seam   ends  result
engine_raw.wav                       6.000  44100  2   -13.7 int   1.0   -5.5   -5.5    8ms    0ms -0.00132      -      -  FAIL: sample rate 44100 (want 48000); 2 channel(s) (want 1); true peak -5.5 dBTP below -3.0 (not normalised); leading silence 8 ms (max 5)
zap_raw_1.wav                        1.300  44100  2   -35.7 ung     -  -15.4  -15.4  300ms  794ms -0.00008      -      -  FAIL: sample rate 44100 (want 48000); 2 channel(s) (want 1); true peak -15.4 dBTP below -3.0 (not normalised); leading silence 300 ms (max 5); trailing silence 794 ms (max 50)
zap_raw_2.wav                        1.300  44100  2   -29.1 ung     -   -9.7   -9.7  300ms  764ms -0.00021      -      -  FAIL: (same five failures)
zap_raw_3.wav                        1.300  44100  2   -24.4 ung     -   -6.1   -6.1  300ms  747ms  0.00022      -      -  FAIL: (same five failures)
4 file(s), 4 failure(s) against category 'sfx'
[exit 1]

$ python audio_report.py bad --category sfx
clipped.wav                          1.000  48000  1    -0.8 ung     -    0.3    0.0    0ms    0ms -0.00002      -      -  FAIL: clipping suspected (peak 0.00 dBFS, flat factor 40.302822); true peak 0.3 dBTP above -0.9
dc_offset.wav                        1.000  48000  1   -16.0 ung     -  -10.5  -10.5    0ms    0ms  0.05000      -      -  FAIL: true peak -10.5 dBTP below -3.0 (not normalised); DC offset 0.0500 (max 0.0020)
silent.wav                           1.000  48000  1       - ung     -   -inf   -inf 1000ms 1000ms  0.00000      -      -  FAIL: file is silent
3 file(s), 3 failure(s) against category 'sfx'
[exit 1]

$ python audio_prep.py raw/zap_raw_1.wav raw/zap_raw_2.wav raw/zap_raw_3.wav --category sfx --name SFX_Test_Zap --out export
source                       -> output                                dur s  gain dB      TP    LUFS    tail  result
zap_raw_1.wav                -> SFX_Test_Zap_01.wav                1.30>0.21    +11.8    -1.0   -16.0      5ms  PASS
zap_raw_2.wav                -> SFX_Test_Zap_02.wav                1.30>0.24     +5.9    -1.0   -15.9      7ms  PASS
zap_raw_3.wav                -> SFX_Test_Zap_03.wav                1.30>0.25     +2.1    -1.0   -15.3     10ms  PASS
3 take(s) written to export, 0 failure(s) against 'sfx'. Log: export\SFX_Test_Zap.prep.json
[exit 0]

$ python audio_report.py export --category sfx --takes --json report.json
SFX_Test_Zap_01.wav                  0.206  48000  1   -16.0 ung     -   -1.0   -1.0    0ms    5ms  0.00003      -      -  PASS
SFX_Test_Zap_02.wav                  0.236  48000  1   -15.9 ung     -   -1.0   -1.2    0ms    7ms  0.00013      -      -  PASS
SFX_Test_Zap_03.wav                  0.253  48000  1   -15.3 ung     -   -1.0   -1.0    0ms   10ms -0.00002      -      -  PASS
3 file(s), 0 failure(s) against category 'sfx'
JSON written to report.json
[exit 0]

$ python audio_prep.py raw/engine_raw.wav --category loop --name SFX_Test_Engine_NoXfade --out export_loop
engine_raw.wav               -> SFX_Test_Engine_NoXfade_01.wav     6.00>6.00     -9.2   -11.3   -23.0      0ms  FAIL: leading silence 11 ms (max 1); loop ends differ by 31.8 dB (max 1.5): audible bump
[exit 1]

$ python audio_prep.py raw/engine_raw.wav --category loop --name SFX_Test_Engine_Loop --out export_loop --loop-crossfade 0.5
engine_raw.wav               -> SFX_Test_Engine_Loop_01.wav        6.00>5.50     -9.2   -11.3   -23.0      0ms  PASS
[exit 0]

$ python audio_report.py stems --category music --same-length
stem_a.wav                           8.000  48000  2   -10.7 int   0.0   -9.1   -9.1    0ms    0ms -0.00000      -      -  FAIL: loudness -10.7 LUFS above -14.0
stem_b.wav                           8.250  48000  2   -10.2 int   0.0   -9.1   -9.1    0ms    0ms  0.00004      -      -  FAIL: loudness -10.2 LUFS above -14.0
FAIL (group): files differ in length: stem_a.wav=8.0000s, stem_b.wav=8.2500s
2 file(s), 3 failure(s) against category 'music'
[exit 1]

$ python audio_layer.py recipe/recipe.json --out recipe/export
SFX_Test_Cannon_01.wav  0.508 s  TP -1.0  -17.8 LUFS  gain -2.3 dB  PASS
    transient  SFX_Test_Zap_01.wav          start    0.0 ms   -2.3 dB  +6.79 st
    body       body_sweep.wav               start    4.2 ms   -0.4 dB  +0.02 st
    tail       tail_noise.wav               start   30.0 ms   -9.4 dB  +0.00 st
    sweetener  SFX_Test_Zap_03.wav          start    0.0 ms  -14.0 dB  -12.00 st
SFX_Test_Cannon_02.wav  0.507 s  TP -1.0  -19.0 LUFS  gain -4.1 dB  PASS
    transient  SFX_Test_Zap_02.wav          start    0.0 ms   -2.4 dB  +6.54 st
    body       body_sweep.wav               start    4.6 ms   +0.2 dB  -0.10 st
    tail       tail_noise.wav               start   30.0 ms   -9.3 dB  +0.00 st
    sweetener  SFX_Test_Zap_03.wav          start    0.0 ms  -14.0 dB  -12.00 st
SFX_Test_Cannon_03.wav  0.507 s  TP -1.0  -19.5 LUFS  gain -4.7 dB  PASS
SFX_Test_Cannon_04.wav  0.507 s  TP -1.0  -19.1 LUFS  gain -3.8 dB  PASS
4 take(s) written to recipe/export, 0 failure(s) against 'sfx'. Log: recipe/export\SFX_Test_Cannon.layers.json
[exit 0]

$ python audio_layer.py recipe/recipe.json --out recipe/export2     (same output)
repeatability: recipe/export and recipe/export2 WAVs byte-identical = True

$ python audio_view.py recipe/export --sheet recipe/review/cannon_takes.png
rows, top to bottom (each 0.51 s wide):
  SFX_Test_Cannon_01.wav … SFX_Test_Cannon_04.wav
wrote recipe/review/cannon_takes.png
[exit 0]

$ python audio_view.py export_loop/SFX_Test_Engine_Loop_01.wav --out recipe/review
wrote recipe/review\SFX_Test_Engine_Loop_01.png
[exit 0]
```
The contact sheet was opened and showed four rows, each a spectrogram (a bright broadband
transient, a low body decaying over about 300 ms) over a waveform.

The recipe used for `audio_layer.py` (four layers: transient from the three prepared zaps in
rotation, pitched up 7 semitones and high-passed at 1.5 kHz; body sweep low-passed at 2.5 kHz and
started 4 ms later; noise tail from 30 ms; a reversed, octave-down sweetener; `takes: 4`, `seed: 7`).

One defect found and fixed during testing: the first version of `audio_layer.py` left 222 ms of
dead tail below -60 dBFS and failed its own category check; a tail trim after levelling was added.

### Cross-check of the measurements against the engine
The same two test files were measured by `audio_report.py` and by Unreal's
`AudioAnalysisSubsystem.analyze_sound_waves`:

| File | Measure | `audio_report.py` | Unreal 5.9 |
|---|---|---|---|
| 0.4 s noise burst | ungated loudness | -31.5 LUFS | -31.484 |
| | integrated (gated) | not available | -160 (not measurable) |
| | sample peak / true peak | -14.7 / -14.2 | -14.731 / -14.290 |
| | trailing silence | 209 ms | 0.210 s |
| 2 s sine | loudness | -28.7 (ungated) | -28.782 integrated, -28.784 ungated |
| | sample peak | -24.1 | -24.082 |

## 7. Tests: Unreal Engine 5.9, live

A scratch project (no C++ module; plugins `EditorToolset`, `MetaSoundToolset`, `MediaToolset`,
`ConsoleToolset`, `AudioModulation`) was opened headless with `UnrealEditor-Cmd.exe … -nullrhi
-unattended -NoLiveCoding -ModelContextProtocolStartServer -ModelContextProtocolPort=24731`, first
with `-nosound`, then with `-NonRealtimeAudio`, and driven with the repo's `Tools/td_mcp.py`. The
project and its files were in a temporary folder and are not part of the repo.

1. **Toolsets as served:** 57 toolsets; the audio-relevant ones and every tool name are in
   `references/unreal-implementation.md` §3. `list_toolsets` also showed `FabToolset.FabToolset`.
2. **Import:** `EditorToolset.SoundWaveToolset.ImportFile` returned the new Sound Wave paths for
   four WAVs. Defaults read back: `soundAssetCompressionType: ProjectDefined`,
   `loadingBehavior: Inherited`, `compressionQuality: 80`, `sampleRateQuality: Max`,
   `virtualizationMode: Restart`, `lUFS: 0`, `samplePeakDB: 0`.
3. **MetaSound by hand:** `CreateSourceBuilder` (Mono, one-shot) → `AddGraphInputs` (3 applied) →
   `AddNodes` (6 applied) → `Connect` (14 applied) → `SetDefaults` (3 applied) → `LayoutGraph` →
   `BuildToAsset` returned `/Game/Probe/Audio/MSS_Probe_Shot.MSS_Probe_Shot`; `GetAssetClass` =
   `MetaSoundSource`.
4. **Preset of the engine's one-shot:** `CreateSourcePresetBuilder` on `MSS_PlayRandom_Oneshot`
   exposed `Sounds`, `PitchBase`, `PitchRandomSpread`, `VolumeBase`, `VolumeRandomSpread`;
   `SetDefaults` + `BuildToAsset` worked and the values read back.
5. **Mix assets by script:** Sound Class, Sound Attenuation, Sound Concurrency, Sound Submix, Sound
   Mix, Control Bus, Control Bus Mix and a submix dynamics-processor preset were created through
   `execute_python`; properties were set with `ObjectToolset.SetProperties` and read back
   (attenuation falloff 60000 and low-pass 2500; concurrency 6 / `StopQuietest`; the mix's class
   effect; class, attenuation and concurrency on the MetaSound).
6. **`ue_audio_build.py` with a 14-asset spec:**
   ```
   $ python ue_run_python.py --port 24731 ue_audio_build.py --spec spec_probe.json
   { "ok": [ "/Game/Probe2/Audio/Mix/SC_TD_Master.SC_TD_Master", "…/SC_TD_SFX", "…/SC_TD_Music",
             "…/ATT_TD_Space_Medium", "…/SCON_TD_Weapon", "…/MIX_TD_SlowMo",
             "…/SFX/SW_Probe_Shot_01", "…_02", "…_03", "…/SW_Probe_Loop",
             "…/SFX/MSS_Probe_Shot", "…/SFX/MSS_Probe_UI", "…/SFX/MSS_Probe_Engine",
             "…/Music/MSS_Probe_Music" ],
     "errors": [] }
   exit=0
   ```
   Read back: the parent class listed both children; `AssetToolset.ValidateAssets` on the four
   MetaSounds: `numChecked 4, numValid 4, numInvalid 0, numWarnings 0`. A second run reported the
   two one-shots as "(updated)" and the loop and music as "exists: only the sound block was
   applied", with no errors.
7. **`-nosound` warning** (LogMetaSound, on every MetaSound save): pre-save "is doing nothing"
   because the session is neither cooking nor a commandlet and cannot render audio.
8. **`ue_audio_check.py`, `-NonRealtimeAudio`:** five renders written to
   `Saved/BouncedWavFiles/ue-game-audio/`; measured with `audio_report.py`:
   ```
   MSS_Probe_Engine_Speed0.wav         10.645  48000  2   -43.2 int   6.3  -40.2  -40.2
   MSS_Probe_Engine_Speed1.wav         11.456  48000  2   -34.5 int   5.8  -32.2  -32.2
   MSS_Probe_Music_I0.wav              15.403  48000  2   -28.8 int   4.1  -26.1  -26.2
   MSS_Probe_Music_I1.wav              15.275  48000  2   -19.3 int   4.2  -16.6  -16.6
   MSS_Probe_Shot.wav                  19.563  48000  2   -42.1 int   1.3  -26.5  -26.6
   ```
   - Engine loop, parameter 0 → 1: +8.7 dB (designed range -8 to 0 dB) and, from zero crossings of
     the render, 98.0 Hz → 147.0 Hz for a 110 Hz source (designed -2 and +5 semitones = 98.0 and
     146.8 Hz).
   - Music, intensity 0 → 1: +9.5 dB = three identical layers adding coherently, so they are in sync.
   - The wave analysis part returned the report in §6 with `errors: []`.
   Two bugs in the first version of `ue_audio_check.py` were found by this run and fixed
   (`is_complete` is a property; the result value is the array itself); the fixed version was re-run.
9. **Slow-motion mix:** with `MIX_TD_SlowMo` (SFX class: pitch 0.6, volume 0.8) pushed via
   `GameplayStatics.push_sound_mix_modifier`, the engine loop rendered at 58.5 Hz, peak -42.14
   dBFS; without it 98.0 Hz, -40.18 dBFS. `set_global_pitch_modulation(0.5)` left the editor
   preview at 98.0 Hz (no effect outside a ticking game world).

Limits of these tests: the editor preview voice is not the game (no distance attenuation, no
listener, no gameplay timing); the absolute level of a preview render was several dB lower than the
level expected in game; the project's own C++ classes were not loaded, so setting a `Sound` in a
real `DA_*` asset was not exercised; no Blueprint was authored.

## 8. Open questions for the humans

1. Is the GitHub repo private? It decides whether library clips may be committed.
2. Which source options does Nicholas approve (the sheet at the end of `sourcing.md`)?
3. Will engineering set `Speed01` (and `Boost01`) on the ship loop audio components, and what is
   `UTDShipVisuals::EngineLoop` for?
4. Who builds `BP_TD_AudioDirector`, and may it be placed in `L_Galaxy` (a shared map)?
5. Should `Tools/td-play-mcp.ps1` get a switch that replaces `-nosound` with `-NonRealtimeAudio`,
   so agents can render and measure audio in the real project?
