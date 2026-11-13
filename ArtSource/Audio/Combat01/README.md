# Combat01 - Lancer firing and impacts

The Lancer primary-fire, hull-impact and shield-impact family is authored, freshly reloaded and played through the real game input/projectile path. The exact 17 tested packages are copied into the isolated audio-prototype-v1 feature worktree. Branch publication, subjective listening approval, Shipping/cook and Main integration remain pending.

## Gameplay mapping

- DA_NeedleCannons_FX.Muzzle.Sound -> /Game/TriadDrift/Audio/Combat01/MSS_NeedleCannon
- DA_NeedleCannons_FX.ImpactHull.Sound -> /Game/TriadDrift/Audio/Combat01/MSS_Hull
- DA_NeedleCannons_FX.ImpactShield.Sound -> /Game/TriadDrift/Audio/Combat01/MSS_Shield

Only these three Sound fields are assigned. Every VFX, shake, scale, color, tracer, mesh and other cue field retains the class default. Sound-only cues preserve engineering's HasVisualArt fallback and squad colors. No Source, Config or map changes. Environment's later Niagara integration must serialize shared DA edits through Lead.

## Sources and mix

Primary: three existing owned AK-47 with animations / A.D fire4-6 recordings/designs, credited Dennis/dennish18/Pixabay in the pack. Hull/shield: existing 50 Free Game Sounds Pack / PlaceHolder Hit_Generic_2-1 and Shield_Metal_Impact_1-3. Exact sources, hashes and rights records are in sources.json. No new purchase, download, AI generation or rejected procedural/M02/V1 primary content. Original WAVs remain unchanged and private/ignored; no standalone library distribution. No field-recording provenance is claimed.

Nine 48 kHz, 16-bit mono takes pass category QA. Primary takes are 0.32 s with responsive attacks and clean fades. Native MetaSounds provide take selection and subtle pitch/level variation, with their own SoundClass, spatial attenuation and concurrency presets. Cue gains are fire -8 dB, hull -14 dB, shield -16 dB; variation is +/-0.7 semitone and +/-1.5 dB. Presets configure weapon concurrency 6 and impact concurrency 8 with 30 ms stealing fade. Active-sound dump counts can include retiring/deferred-stop voices and are not a measurement of a six-rendered-voice hard cap.

The offline stress mix combines 80 shots at 20 Hz plus 80 alternating impacts at maximum +1.5 dB variation without attenuation/concurrency reduction: -4.2 dBTP, -19.1 LUFS, no clipping. These are offline measurements; no in-game PCM mix/clipping measurement is claimed.

## Verified results

- Author02: exact 16 audio packages plus the new sound-only data asset; OS 0, 18.77 s, zero logged engine errors, 1,106 pre-existing protected files unchanged, no unexpected Content writes.
- Reload01: all 17 assets load, SoundClass/attenuation/concurrency references and three sound slots match; all remaining defaults preserved. OS 0, 18.82 s, zero logged engine errors, 1,123 protected files unchanged.
- Gameplay01: real Two/LeftMouseButton input in L_Galaxy, stationary ShardFighter and HookInterceptor trials, active cannon/hull/shield cues observed. The capture shows Lancer marked YOU and 67% weapon heat. Headphones (Xbox Controller), WASAPI, 48 kHz stereo. OS 0, 39.72 s, zero logged engine errors, 1,123 protected files unchanged, no new Content. Minimum available RAM 1.553 GiB.
- The startup-gated Author01 attempt made no writes and remains preserved as a failed attempt.

See native_author_01.json, native_author_02.json, native_reload_01.json, native_gameplay_01.json and native_package_freeze.json. The latter pins the exact 17 copied packages (669,897 bytes), all matching the tested host. Menu theme/style hashes are unchanged. author_combat.py and verify_combat.py preserve the granted authoring/read-only reload entrypoints.

## Listen

PLAYBACK.md gives the exact compiled test-project path and controls. Private prepared-file listening page:

C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Combat01/audition.html

Current acceptance: actual event playback technically verified in the compiled test host; final ear, native PCM balancing/clipping, exact rendered concurrency behavior and Shipping remain open. No automatic Main merge or full audio overhaul is authorized by this milestone.
