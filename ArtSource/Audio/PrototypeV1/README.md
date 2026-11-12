# Audio Prototype V1

Source for an isolated Unreal 5.9 listening demonstration using Nick's existing owned/free sources. Editor and Game C++ targets compiled successfully. Native audio import and gameplay playback are not yet verified: the first editor session stopped on two stock renderer-setting errors before authoring. The reviewed Cascade-plugin correction is staged and awaits Lead's next validation grant. No shared gameplay code, configuration, maps, Blueprints or presentation data assets are changed.

The staged native host and licensed media are local and ignored. This project is not yet playback-ready:

`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Host/TDAudioV1.uproject`

Planned map: `/Game/TriadDrift/Audio/PrototypeV1/L_AudioPrototypeV1` (not created yet).

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

The build spec places V1 sources under `/Game/TriadDrift/Audio/PrototypeV1/Sources/MSS_V1_<Event>` in the private host. Each repeating one-shot specifies three prepared variations, no-repeat selection, ±0.7 semitone / ±1.5 dB runtime spread. These variations derive from the same source recording/design; they are not three newly recorded takes. Switches and UI signatures use one stable take. The WAV preparation is verified; native graphs are not yet created.

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

## Intended native behavior — authored assets and playback pending

WAV files are prepared and checked at 48 kHz / 16-bit PCM, following the published skill's native import target. The native spec defines MetaSound Sources with Sound Class, attenuation and concurrency, Master/SFX/UI/Cine/Music classes and a settings Sound Mix. Compiled C++ defines independent volume controls, a −3 dB master submix limiter with 2 ms look-ahead, a `Speed01` engine control and a two-second equal-power music crossfade, with component/override cleanup on teardown. Their actual playback, levels and teardown remain acceptance gates. Prepared loop excerpts crossfade the internal edit seam and retain a continuous original sample boundary; their file-boundary measurements pass.

## Reproduction and evidence

Run `prepare.py` with the existing Blender Python executable. It reads the frozen audit helpers and only the approved VaultCache clips, snapshots Ryan's published scripts into ignored storage, runs prep/layer/report checks, and generates `build.spec.json`, `check.spec.json`, `sources.json`, category QA JSON and a preparation log. `make_host.py` stages the native source template under the ignored host. No script downloads assets.

Only run native builds/editor/game after Lead grants the sole heavy slot, port **8025**. Authoring uses `-NonRealtimeAudio`, not `-nosound`, and the skill's `ue_run_python.py` / `ue_audio_build.py` / `ue_audio_check.py`. The host is separate from Triad Drift's project and uses the same installed 5.9 build. Native test results are recorded separately in `VERIFICATION.md`; source QA and C++ compilation are not proof of successful Unreal playback. The reviewed minimal-plugin fix enables stock Cascade, which owns the missing renderer CVars; it does not restore every default plugin. Its editor target receipt still needs a recheck before another authoring run.

Licensing and Lead credits handoff: [CREDITS_PROTOTYPE.md](CREDITS_PROTOTYPE.md). Exact source names and hashes: `sources.json`.

## Known substitutions / review gates

- The engine is recorded computer machinery, not a purpose-recorded spacecraft turbine. Acceleration is an appliance power-up.
- Missile, shield and ability feedback are prototype substitutions. Three-ship identity and brightness/harshness still need listening review.
- Music consists of two existing tracks, not synchronized stems or a beat-quantized combat score. Crossfade behavior does not establish artistic fit.
- Native/raw media remain outside Git pending license clearance. Code/manifests/QA are versioned; generated host assets can be rebuilt locally.
- No actual gameplay integration, packaged Shipping validation, full-wave mix measurement or final presentation approval is implied.
