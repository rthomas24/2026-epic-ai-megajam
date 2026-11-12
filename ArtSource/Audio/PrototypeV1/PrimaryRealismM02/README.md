# Primary Realism M02 — listening review

Agent: Audio & Music. Task: primary-fire realism revision only. AAA stage: Verify complete for offline files; human Review pending. No Unreal import, gameplay test, native retune or final presentation approval.

Nick found the V1 five-shot preview too retro/arcade-like. This concept replaces its `Sci-Fi_Gun_1-1` body entirely. Two existing human-designed gunshot effects provide the discharge and low-mid decay; three recorded hardware-button takes provide physical action. There is no oscillator, pitch sweep, laser-bloop layer, distortion, saturation, generated audio or new acquisition. Fixed filtering, small constant pitch changes, short envelopes and millisecond offsets shape the existing material. Whether it is convincing and cinematic is Nick's listening decision.

## Listen

Open `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/ArtSource/Audio/PrototypeV1/PrimaryRealismM02/index.html` for A/B players, three takes and a rapid-fire check.

New five-shot WAV:
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/PrimaryRealismM02/Primary_Realism_M02_Five.wav`

Original V1 reference, unchanged:
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Host/Saved/BouncedWavFiles/ue-game-audio/V1_Primary_Five.wav`

Three individual 48 kHz/16-bit mono category exports:

- `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/PrimaryRealismM02/export/Primary_Realism_M02_01.wav`
- `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/PrimaryRealismM02/export/Primary_Realism_M02_02.wav`
- `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/PrimaryRealismM02/export/Primary_Realism_M02_03.wav`

The five-shot comparison is 48 kHz/16-bit stereo, with the exact V1 duration and measured shot positions. Individual takes are louder category exports; the page plays them at 50% player volume.

## Sources, rights and limits

| Used material | Role and provenance | Official pack |
|---|---|---|
| `Gunshot_1-1`, `Gunshot_7-1` | Alternate discharge body and short filtered decay. Publisher states human-designed original effects; actual firearm field-recording provenance is **unverified**. | PlaceHolder Inc, [50 Free Game Sounds Pack](https://www.fab.com/listings/b8cc7270-5e0c-4ef7-a277-6a1d4b69358d) |
| `MACHAppl_VCRButton_HA_TerminalUI_04_Wav`, `_08_Wav`, `_13_Wav` | Three distinct hardware-action sources. Publisher identifies real recorded hardware alongside its synthesized UI sounds; only hardware-named recordings are used. | Hove Audio, [Terminal User Interface Sound Effects Pack LITE Edition](https://www.fab.com/listings/183a6ec7-cbc3-451a-8fa7-08da97c07fcf) |

These packs were already owned, locally cached and authorized for the prototype. Official descriptions and AI flags were rechecked November 12, 2026. Both list no AI generation and disallow AI use; nothing was supplied to an AI audio/training/generation service. Audio was processed only with local ffmpeg/Python. Publisher language such as "copyright-free" is not treated as CC0.

[Fab Standard terms](https://www.fab.com/eula) support incorporated commercial projects and private collaboration; exact historical acquisition-license metadata remains unresolved, as in V1. All library media and processed WAVs stay ignored/local. No raw or native audio is published in Git. Lead owns root credits/devlog; this table, `source_inventory.json`, the recipe and logs are the credit handoff. Third-party assets must be listed for the jam even when attribution is optional.

`Draw_Weapon_Metal_1-1` was inspected and omitted; it is not a layer. No purpose-recorded cannon, firearm mechanism or air-release recording with documented field-session provenance was identified in this selected local set. Hardware action is a substitution, not a claim of a real gun mechanism. A dedicated recorded weapon library remains a source gap if Nick finds this concept insufficient; no thin synthetic fallback is added.

## Comparison fairness and verification

- Three different source combinations, each with a different recorded mechanical take; two alternate gun bodies. These are not three verified field-recorded firearm takes.
- Source category QA checks 48 kHz/16-bit mono, −1 dBTP, silence, DC, clipping and take spread. `takes_qa.json` is authoritative.
- Final takes measure **0.2123–0.2212 seconds**, **−1.0 dBTP**, **0.26 LU spread**, zero clipping and zero category failures. The 11-shot rapid check peaks at **−6.3 dBTP** without clipping.
- Discharge onsets are aligned using a 1 ms RMS envelope, retaining 2 ms pre-roll and a 1 ms fade. `alignment.json` records the 24 ms gunshot-7 trim. The final strong attacks occur within 1–7 ms, with quiet endpoints and a roughly 0.22-second decay.
- Five-shot positions come from V1's measured native-preview metadata, not its editor wall-clock schedule. Both files are 13.952 seconds, at 0.03 / 1.99 / 3.99 / 6.00 / 7.98 seconds.
- A single linear preview gain matches **full-file K-weighted ungated energy to 0.00 LU measured difference**. Same 400 ms post-trigger windows differ by −0.72 dB RMS (M02 lower). The new transient has more crest: V1 true peak −15.0 dBTP, M02 −8.1 dBTP. This is not a +20 dB loudness comparison; no limiting or loudness compression is applied to the A/B.
- The additional rapid-fire file checks 11 shots at 90 ms spacing with documented linear headroom trim. It is a numerical overlap check, not a native concurrency/game-mix test.
- All 53 V1 native packages plus the original five-shot WAV retain their saved hashes. All six inspected original cache packages are unchanged. No other V1 file is retuned or overwritten.

Agents inspected waveforms/spectrograms and measured files; no subjective hearing or AAA approval is claimed. Nick should compare A/B, then listen to the three takes and rapid-fire sample for physical weight, sharpness, fatigue and decay. No expansion or integration before his decision.

## Reproduce

Use the already installed Blender Python 3.13.9 and ffmpeg/ffprobe 8.0.1. Run `inspect_sources.py`, `build_primary.py`, then `finish_review.py`. Inputs are already present in VaultCache; scripts do not download or launch Unreal. The read-only extraction helper comes from the preserved `audio-music/ArtSource/Audio/EpicLibraryAudit01/build_audit.py`. Preparation/layer/report helpers are Ryan's published `ue-game-audio` scripts. Recipe, exact layer selections, source hashes, preparation log and QA provide the production trail; licensed outputs stay in `Saved/AudioMusic/PrototypeV1/PrimaryRealismM02/`.
