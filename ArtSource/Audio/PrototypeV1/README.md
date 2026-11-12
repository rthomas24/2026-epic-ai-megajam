# Audio Prototype V1

Isolated Unreal 5.9 audio prototype using Nick's existing owned/free sources. The corrected Editor target compiled successfully; 53 native assets passed validation, and imported waves plus offline MetaSound previews are verified. Runtime controls, game-mixer behavior and human listening remain pending. The first editor failure and its corrected retest are preserved separately. No shared gameplay code, configuration, maps, Blueprints or presentation data assets are changed.

The native host and licensed media are local and ignored. Native assets can be previewed in the editor; runtime audition verification is still pending:

`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Host/TDAudioV1.uproject`

Created map: `/Game/TriadDrift/Audio/PrototypeV1/L_AudioPrototypeV1`.

Physical map: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Host/Content/TriadDrift/Audio/PrototypeV1/L_AudioPrototypeV1.umap`.

## Intended controls in Play / the standalone game — pending runtime verification

| Key | Test |
|---|---|
| 1 / 2 / 3 | Primary fire / missile launch / explosion |
| 4 / 5 | Shield / hull impact |
| 6 / 7 / 8 | Rift / Lancer / Bastion switch |
| 9 / 0 | Target acquisition / ability feedback |
| E; W / S; B | Engine start/stop; throttle up/down; acceleration transient |
| R | Repeated primary-fire stress toggle |
| F1 / F2 / F3 | Exploration / combat / fade music out |
| Z / X | Decrease / increase SFX volume |
| C / V | Decrease / increase UI and switch-feedback volume |
| N / M | Decrease / increase music volume |
| P; PageUp / PageDown | Toggle positional world effects; change emitter distance |
| Backspace | Stop repeated fire, engine, music and effect tails with fades |

Start at a comfortable system volume. Listen to each event, then hold rapid fire while changing music and throttle; switch repeatedly; test each volume group at zero; compare positional shots near and beyond 600 m. Human listening approval is required for harshness, realism, fatigue and musical phrasing.

## Event mapping for future coordinated integration

V1 sources are saved under `/Game/TriadDrift/Audio/PrototypeV1/Sources/MSS_V1_<Event>` in the private host. Each repeating one-shot specifies three prepared variations, no-repeat selection, ±0.7 semitone / ±1.5 dB runtime spread. These variations derive from the same source recording/design; they are not three newly recorded takes. Switches and UI signatures use one stable take. The native primary-fire preview contains five distinct 0.30–0.33-second bursts with 1.54 dB peak variation; other signatures still need runtime listening.

| V1 event | Proposed gameplay destination — not connected in V1 |
|---|---|
| Engine | `DA_<Ship>_Visuals.EngineLoop`; `Speed01` input; leave `EngineTrail.Sound` empty when this bed is assigned to avoid doubling |
| Acceleration | `DA_<Ship>_Visuals.BoostStart.Sound` |
| Primary | `DA_NeedleCannons_FX.Muzzle.Sound` (prototype anchor) |
| Missile | `DA_SpikeRockets_FX.Muzzle.Sound` |
| Explosion | Weapon `Detonation.Sound` / enemy `Explosion.Sound` |
| Shield / Hull | Ship `ShieldHit.Sound` / `HullHit.Sound`, weapon impact slots |
| Rift / Lancer / Bastion | Respective `SwitchIn.Sound`; keep `SwitchOut.Sound` empty to prevent doubling |
| Target / Ability | Future UI/director trigger; no shared hook added |
| Explore / Combat | Two-track director crossfade on wave / phase state, when coordinated |

Lead's R014 handoff identifies the published `TDAudioStateSubsystem.GetState` snapshot (main `f2db3ce`) for future music/mix binding, including `MusicIntensity01`, scene/phase, speed/boost, health/shield, wave/combat and slow-motion state. V1 does not duplicate that API or bind to it. A read-only inspection of published `TDShip.cpp` confirms that `EngineLoop` is read and `ApplyEngineAudioParams` supplies `Speed01`, `Boost01`, and `Drift01` to both engine components each frame; MetaSounds avoid a second external pitch multiplier. This supersedes the older skill's unused/in-progress notes. No shared data asset has been assigned.

## Native settings verified; runtime behavior pending

WAV files are prepared and checked at 48 kHz / 16-bit PCM. All 26 imported waves remain non-silent at 48 kHz. The 53 saved assets include 14 MetaSound Sources, five Sound Classes, two attenuation presets, four concurrency presets, one settings Sound Mix and the demo map. Native inspection confirms the source settings, four master child classes, and PlayWhenSilent on all three looping sources. Compiled C++ defines independent volume controls, a −3 dB master submix limiter with 2 ms look-ahead, a `Speed01` engine control and a two-second equal-power music crossfade, with cleanup on teardown. Their game playback, levels and teardown remain acceptance gates. The offline engine previews show a 5.008 dB sustained RMS increase at full throttle. Prepared loop-boundary measurements pass.

## Reproduction and evidence

Run `prepare.py` with the existing Blender Python executable. It reads the frozen audit helpers and only the approved VaultCache clips, snapshots Ryan's published scripts into ignored storage, runs prep/layer/report checks, and generates `build.spec.json`, `check.spec.json`, `sources.json`, category QA JSON and a preparation log. `make_host.py` stages the native source template under the ignored host. No script downloads assets.

Only run native builds/editor/game after Lead grants the sole heavy slot, port **8025**. Authoring uses `-NonRealtimeAudio`, not `-nosound`, and the skill's `ue_run_python.py` / `ue_audio_build.py` / `ue_audio_check.py`. The builder's private copy only adds immediate exception propagation to honor the first-error stop rule. The reviewed stock Cascade correction passed its Editor rebuild and authoring retest. Full results are in `VERIFICATION.md` and `native_author_02.json`.

Offline previews are at `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Host/Saved/BouncedWavFiles/ue-game-audio/`: `V1_Primary_Five.wav`, `V1_Engine_Speed0.wav`, `V1_Engine_Speed1.wav`. They are non-silent and unclipped. This editor's offline clock renders about four seconds of audio per wall-clock second; recorded durations are not gameplay timing evidence. Preview levels also differ from the game mix.

Licensing and Lead credits handoff: [CREDITS_PROTOTYPE.md](CREDITS_PROTOTYPE.md). Exact source names and hashes: `sources.json`.

## Known substitutions / review gates

- The engine is recorded computer machinery, not a purpose-recorded spacecraft turbine. Acceleration is an appliance power-up.
- Missile, shield and ability feedback are prototype substitutions. Three-ship identity and brightness/harshness still need listening review.
- Music consists of two existing tracks, not synchronized stems or a beat-quantized combat score. Crossfade behavior does not establish artistic fit.
- Native/raw media remain outside Git pending license clearance. Code/manifests/QA are versioned; generated host assets can be rebuilt locally.
- No actual gameplay integration, packaged Shipping validation, full-wave mix measurement or final presentation approval is implied.
