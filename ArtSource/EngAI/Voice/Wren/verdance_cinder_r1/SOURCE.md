# Wren voice r1: Verdance and Cinder (R-042 ask 4)

**For:** Nicholas (listen and pick or confirm the recommended take), Nick's Audio / T3 (integrate the approved file), Ryan / Lead.
**Stage:** technically verified only. The agent that made these cannot hear them.
Nicholas approves by ear. Recommended takes below were picked from timing, median F0,
loudness and a faster-whisper `small.en` transcript, not from a listen.

**Use:** the rest of Wren's current draft lines in this chapter batch, in take 04's voice
(PR #78 master / PR #93 setup). The shared interrupted **Don't—** master and **C8_Wren_Order**
are not in this PR and must not be regenerated.

Ground and cinematic lines from Verdance through the Cinder cache route.

## Files

| File | What it is |
|---|---|
| `VO_<Id>[_Variant]_0N_dry.wav` | Dry take, 50 ms pre-roll, 100 ms tail, 48 kHz, mono, 24-bit |
| `VO_<Id>[_Variant]_0N_radio.wav` | Radio version of the same take (chain below). Used for space / comms / Static lines |
| `raw/VO_*_liora.wav` | Untouched xAI TTS outputs (48 kHz, 16-bit mono WAV straight from the API) |
| `tools/process_voice.py`, `tools/audio_common.py` | Rebuilds the dry / radio / echo files from `raw/` |
| `build_log.json` | Per take: speech bounds, mid-sentence pause, gains, loudness |
| `stt.json` | Local faster-whisper `small.en` transcripts, median F0, SHA256 |
| `verdance_cinder_r1_sheet.png` | Waveform and spectrogram of every delivered file |

## Recommended takes

The agent cannot hear these. Pick below is from timing, pitch and transcript only.
`small.en` is noisy on short lines and regularly writes 'cash' for 'cache' and 'chip' for 'ship'.
Those known confusions were treated as a match when the rest of the words lined up.

| Line | Recommended | Alternative | Spoken text | Why |
|---|---|---|---|---|
| `G2_Wren_Geology` | **take 01** | take 03 | They brought all this just to dig? | All three are exact; take 01 is the tightest delivery, F0 179 Hz in the liora range from PR #93. |
| `C3_Wren_Survey` | **take 01** | take 02 | Kato. Tell me what we've got. | Exact, shorter pause than take 02. Take 03 is the long [pause] roll and dropped 'Kato'. |
| `C3_Wren_NotThePlanet` | **take 02** | take 03 | They're digging for that? | Exact. Take 01 STT reads 'Thank you for that' — do not use it without a listen, it may be a real wording miss. |
| `C3_Wren_Answered` | **take 02** | take 01 | That was our radio. What did you send? | Exact transcript, mid pause 472 ms. Take 03 is the long [pause] roll (1056 ms). |
| `C3_Wren_HoldThem` | **take 02** | — | Keep copying. We'll cover the station. | Exact, natural 182 ms pause. Take 03 is a very long 2.6 s [pause] hold. Take 01 dropped the first sentence in STT. |
| `G3_Wren_LeftApproach` | **take 02** | take 03 | Left side! They're going for the console. | Exact transcript. Take 01 dropped 'Left side' / changed the verb. |
| `C4_Wren_Badly` | **take 03** | take 01 | They wanted that device and its records. We need to know why. | Only exact full wording. Take 01 is very close ('in its records' vs 'and its records'). |
| `G4_Wren_StartThere` | **take 02** | take 03 | Then we follow them down. | All three STT as exact, but take 01's median F0 is 90 Hz (octave error or a badly low delivery). Take 02 sits at 171 Hz, in the liora range. |
| `C5_Wren_Guarding` | **take 02** | take 01 | Let's get past the thing guarding it first. | All three exact; take 02 is a continuous delivery with no mid pause. |
| `C5_Wren_Route` | **take 01** | take 02 | Fine. We start with the cache. | Exact ('cache' heard correctly). Loudness −20.17 LUFS, still in the −21..−17 window. Take 03 is −22.66 LUFS, outside the window, because the −1.5 dBTP cap blocked the gain. |
| `G5_Wren_ClearRoute` | **take 01** | take 02 | Cache is behind it. Stay out of its reach. | Full line after treating STT 'cash' as 'cache'. Take 03 collapses into a stutter; take 02 dropped the second sentence in STT. |
| `G5_Wren_BackToShips` | **take 02** | take 01 | We've got what we came for. Back to the ships. | STT was unreliable on this line. Take 02 is the only one that still shows both clauses ('we came for' / 'Back to the ships'). Needs a listen. |

## Tool and model

- **Model:** xAI Text to Speech (Grok voice), run on **fal.ai**. Endpoint **`xai/tts/v1`**
  ([model page](https://fal.ai/models/xai/tts/v1)). Run on Ryan's fal account on 2026-11-14.
- **Voice:** `liora` (xAI: "calm, grounded, luminous"). This is the voice of PR #78 take 04,
  which Nicholas selected as the shared `Dont` radio master, and of the PR #93 present-Wren Order audition.
- **Common settings (same as `audition_r1` / `present_order_r1`):** `language: "en"`,
  `output_format: {codec: "wav", sample_rate: 48000}`, `enable_safety_checker: true`,
  all other inputs at their defaults. No `speed` parameter was sent. No seed parameter exists,
  so a rerun will not produce the same audio. The `raw/` files are the record.
- **Cost (this batch):** 36 generations, 1420 characters, at $0.015 per 1,000 characters,
  so **about $0.021**. Speech-to-text checks ran locally (faster-whisper `small.en`) at no cost.
- **Credit:** Voice: xAI Text to Speech (Grok), via fal.ai. Commercial-use / Partner model.
  Do not present as human-generated. Do not use to train models.

Story lines have three takes. Takes 01 and 02 send the draft line with no speech tags.
Take 03 of a multi-sentence line inserts a documented `[pause]` tag between sentences;
the spoken words stay exact. Barks have two exact takes and no pause tags.

### Generations

| Stem | Exact `text` sent | fal `request_id` |
|---|---|---|
| `VO_G2_Wren_Geology_01` | `They brought all this just to dig?` | `01a1dad9-ab68-7d62-9285-c7ae783d7e13` |
| `VO_G2_Wren_Geology_02` | `They brought all this just to dig?` | `01a1dad9-ab8e-74f2-af8d-ac452f37b95f` |
| `VO_G2_Wren_Geology_03` | `They brought all this just to dig?` | `01a1dad9-abe0-7792-88db-3a35e77f9f05` |
| `VO_C3_Wren_Survey_01` | `Kato. Tell me what we've got.` | `01a1dad9-ab85-7b51-9256-6aa4ba70b1e3` |
| `VO_C3_Wren_Survey_02` | `Kato. Tell me what we've got.` | `01a1dad9-abb3-74c1-aa04-01fa4b570183` |
| `VO_C3_Wren_Survey_03` | `Kato. [pause] Tell me what we've got.` | `01a1dadb-0fcb-7f61-8519-da3e6387d313` |
| `VO_C3_Wren_NotThePlanet_01` | `They're digging for that?` | `01a1dadb-0fb0-77d2-8d94-385d691aa848` |
| `VO_C3_Wren_NotThePlanet_02` | `They're digging for that?` | `01a1dadb-0ff6-70d0-bb19-9aa5a6bbea73` |
| `VO_C3_Wren_NotThePlanet_03` | `They're digging for that?` | `01a1dadb-1029-7831-9258-c856496e52b6` |
| `VO_C3_Wren_Answered_01` | `That was our radio. What did you send?` | `01a1dadb-101d-7993-ac0f-66108f415e5d` |
| `VO_C3_Wren_Answered_02` | `That was our radio. What did you send?` | `01a1dadb-0fde-76d3-bd44-f6d31ed7532d` |
| `VO_C3_Wren_Answered_03` | `That was our radio. [pause] What did you send?` | `01a1dadb-1023-7101-ba80-5cf285f77108` |
| `VO_C3_Wren_HoldThem_01` | `Keep copying. We'll cover the station.` | `01a1dadb-1018-72e3-8af9-e8b602548d47` |
| `VO_C3_Wren_HoldThem_02` | `Keep copying. We'll cover the station.` | `01a1dadb-1f27-76d0-a4e7-8f1af5363bfe` |
| `VO_C3_Wren_HoldThem_03` | `Keep copying. [pause] We'll cover the station.` | `01a1dadb-1f1a-7931-8b3a-b04a20a92ce2` |
| `VO_G3_Wren_LeftApproach_01` | `Left side! They're going for the console.` | `01a1dadb-1f13-7b83-9bd1-085cc8a27b5f` |
| `VO_G3_Wren_LeftApproach_02` | `Left side! They're going for the console.` | `01a1dadb-1ece-72e1-9049-34c4710809c3` |
| `VO_G3_Wren_LeftApproach_03` | `Left side! [pause] They're going for the console.` | `01a1dadb-1ed0-7691-9605-5ec12cc4fd32` |
| `VO_C4_Wren_Badly_01` | `They wanted that device and its records. We need to know why.` | `01a1dadb-1f0c-7912-96ed-d41f73629348` |
| `VO_C4_Wren_Badly_02` | `They wanted that device and its records. We need to know why.` | `01a1dadb-2034-7ae3-8f1f-4423698ffb64` |
| `VO_C4_Wren_Badly_03` | `They wanted that device and its records. [pause] We need to know why.` | `01a1dadb-1f10-7133-8b3b-17feab1dd295` |
| `VO_G4_Wren_StartThere_01` | `Then we follow them down.` | `01a1dadb-2ef3-7453-a8e5-0c607d592522` |
| `VO_G4_Wren_StartThere_02` | `Then we follow them down.` | `01a1dadb-2ef7-7702-a2dc-7ab3ec386379` |
| `VO_G4_Wren_StartThere_03` | `Then we follow them down.` | `01a1dadb-2f41-79d2-90ca-ea9da1d342fa` |
| `VO_C5_Wren_Guarding_01` | `Let's get past the thing guarding it first.` | `01a1dadb-2f61-74d0-b366-c5b2f34ce51a` |
| `VO_C5_Wren_Guarding_02` | `Let's get past the thing guarding it first.` | `01a1dadb-2f3f-7723-9127-95a1d4493554` |
| `VO_C5_Wren_Guarding_03` | `Let's get past the thing guarding it first.` | `01a1dadb-2f2b-7f00-87dd-e9a7749f6de6` |
| `VO_C5_Wren_Route_01` | `Fine. We start with the cache.` | `01a1dadb-2f4d-7321-93cb-9e946dd1b64c` |
| `VO_C5_Wren_Route_02` | `Fine. We start with the cache.` | `01a1dadb-2f31-7880-a73e-b54c5e74b350` |
| `VO_C5_Wren_Route_03` | `Fine. [pause] We start with the cache.` | `01a1dadb-45a9-78f2-a2c6-d9f5386afc95` |
| `VO_G5_Wren_ClearRoute_01` | `Cache is behind it. Stay out of its reach.` | `01a1dadb-45ba-7731-8f6f-8891dbb95166` |
| `VO_G5_Wren_ClearRoute_02` | `Cache is behind it. Stay out of its reach.` | `01a1dadb-4644-7e70-87f8-21b8b7d2e00f` |
| `VO_G5_Wren_ClearRoute_03` | `Cache is behind it. [pause] Stay out of its reach.` | `01a1dadb-462b-7062-bbeb-a99958bf61b9` |
| `VO_G5_Wren_BackToShips_01` | `We've got what we came for. Back to the ships.` | `01a1dadb-4624-7401-8828-b3d845dc5296` |
| `VO_G5_Wren_BackToShips_02` | `We've got what we came for. Back to the ships.` | `01a1dadb-4626-71a1-ac82-a4347ad2e21f` |
| `VO_G5_Wren_BackToShips_03` | `We've got what we came for. [pause] Back to the ships.` | `01a1dadb-462e-7cd2-8079-84a821e9481a` |

## Edit (dry takes)

Same dry edit as PR #93 (`present_order_r1`):

1. Find the first and last speech on a 10 ms RMS envelope at −45 dBFS. Everything between those
   bounds is kept, including mid-sentence pauses. DC is removed.
2. Pad **50 ms** of silence before the onset and **100 ms** after the offset.
3. 3 ms linear fade-in, 20 ms cos² fade-out, so the pads are click-free.
4. One linear gain to **−19 LUFS** (the project's `vo` target). Gain is also capped so true peak
   stays at or under −1.5 dBTP (`vo` allows −1.0). 24-bit WAV, 48 kHz, mono.

## Radio chain

Same chain as PR #78 take 04, **without** the mid-word static tail (that tail was only for
the interrupted "Don't—" cut). Built from each normalised dry take, numpy and scipy,
seeded by `sha1(stem)`:

1. Band-limit: 4th-order Butterworth high-pass at 300 Hz, then 6th-order Butterworth low-pass at 3.4 kHz.
2. Light saturation: `tanh(2.2·x)/tanh(2.2)` on the peak-normalised signal, then the 3.4 kHz low-pass again.
3. Carrier hiss under the voice: band-limited noise at −38 dBFS.
4. 250 Hz high-pass and mean removal (DC), a 3 ms fade-in and 12 ms fade-out, one gain to −19 LUFS.

Radio files ship for: `G2_Wren_Geology`, `C3_Wren_HoldThem`, `G3_Wren_LeftApproach`, `G4_Wren_StartThere`, `G5_Wren_ClearRoute`, `G5_Wren_BackToShips`.

## Levels

Loudness is K-weighted (ungated under 3 s, gated integrated at or over 3 s). TP is true peak.
The `vo` window is −21..−17 LUFS. One known miss is noted in the recs if present.

| File | Dur s | LUFS | TP dBTP | Lead ms | Tail ms | Mid pause ms | Median F0 Hz | STT (short) |
|---|---|---|---|---|---|---|---|---|
| `VO_G2_Wren_Geology_01_dry` | 1.616 | -19.00 | -3.8 | 50.1 | 100.0 | 0.0 | 179 | They brought all this just to dig. |
| `VO_G2_Wren_Geology_02_dry` | 1.848 | -19.00 | -3.3 | 54.8 | 100.0 | 0.0 | 185 | they brought all this just to dig. |
| `VO_G2_Wren_Geology_03_dry` | 1.756 | -19.00 | -3.4 | 53.0 | 100.0 | 0.0 | 183 | They brought all this just to dig. |
| `VO_C3_Wren_Survey_01_dry` | 2.046 | -19.00 | -3.6 | 50.0 | 102.9 | 438.0 | 195 | Kato, tell me what we've got. |
| `VO_C3_Wren_Survey_02_dry` | 2.130 | -19.00 | -2.1 | 50.3 | 100.0 | 598.0 | 211 | Kato, tell me what we've got. |
| `VO_C3_Wren_Survey_03_dry` | 2.962 | -19.75 | -1.5 | 51.2 | 100.1 | 1434.0 | 173 | Tell Tell me what we've got. |
| `VO_C3_Wren_NotThePlanet_01_dry` | 1.178 | -19.00 | -2.6 | 50.7 | 100.0 | 0.0 | 173 | Thank you for that. |
| `VO_C3_Wren_NotThePlanet_02_dry` | 1.182 | -19.00 | -3.6 | 51.8 | 100.3 | 0.0 | 176 | They're digging for that. |
| `VO_C3_Wren_NotThePlanet_03_dry` | 1.116 | -19.00 | -4.5 | 50.9 | 100.3 | 0.0 | 176 | They're digging for that. |
| `VO_C3_Wren_Answered_01_dry` | 2.312 | -19.00 | -3.0 | 53.6 | 100.0 | 212.0 | 178 | That was our radio. What, did you send? |
| `VO_C3_Wren_Answered_02_dry` | 2.374 | -19.00 | -2.0 | 50.0 | 100.0 | 472.0 | 184 | That was our radio. What did you send? |
| `VO_C3_Wren_Answered_03_dry` | 2.964 | -19.18 | -1.5 | 50.0 | 100.0 | 1056.0 | 179 | That was our radio. What did you send? |
| `VO_C3_Wren_HoldThem_01_dry` | 2.316 | -19.00 | -1.7 | 50.0 | 100.0 | 156.0 | 183 | We'll cover the station. |
| `VO_C3_Wren_HoldThem_02_dry` | 2.240 | -19.80 | -1.5 | 50.1 | 100.0 | 182.0 | 175 | Keep copying, we'll cover the station. |
| `VO_C3_Wren_HoldThem_03_dry` | 4.690 | -19.40 | -1.5 | 50.4 | 100.0 | 2594.0 | 176 | Keep copying. We'll cover the station. |
| `VO_G3_Wren_LeftApproach_01_dry` | 2.390 | -19.00 | -2.2 | 50.7 | 100.0 | 366.0 | 199 | They're going to the console. |
| `VO_G3_Wren_LeftApproach_02_dry` | 2.356 | -19.00 | -3.4 | 51.2 | 100.0 | 328.0 | 197 | Left side, they're going for the console. |
| `VO_G3_Wren_LeftApproach_03_dry` | 2.914 | -20.52 | -1.5 | 53.5 | 100.0 | 910.0 | 181 | the left side. They're going for the console. |
| `VO_C4_Wren_Badly_01_dry` | 3.214 | -19.40 | -1.5 | 50.4 | 100.0 | 192.0 | 175 | They wanted that device in its records. We need to know why |
| `VO_C4_Wren_Badly_02_dry` | 3.054 | -19.00 | -2.6 | 50.0 | 100.0 | 0.0 | 177 | They wanted that in its records. We need to know why. |
| `VO_C4_Wren_Badly_03_dry` | 3.870 | -19.00 | -3.2 | 51.8 | 100.0 | 708.0 | 185 | They wanted that device and its records. We need to know … |
| `VO_G4_Wren_StartThere_01_dry` | 1.534 | -19.00 | -2.6 | 50.0 | 100.0 | 0.0 | 90 | then we follow them down. |
| `VO_G4_Wren_StartThere_02_dry` | 1.518 | -19.00 | -1.9 | 50.8 | 100.0 | 0.0 | 171 | then we follow them down. |
| `VO_G4_Wren_StartThere_03_dry` | 1.362 | -19.00 | -2.2 | 50.5 | 100.0 | 0.0 | 158 | then we follow them down. |
| `VO_C5_Wren_Guarding_01_dry` | 2.300 | -19.00 | -1.6 | 50.8 | 100.0 | 84.0 | 177 | Let's get past the thing guarding it first. |
| `VO_C5_Wren_Guarding_02_dry` | 2.314 | -19.00 | -1.9 | 51.1 | 100.0 | 0.0 | 179 | Let's get past the thing guarding it first. |
| `VO_C5_Wren_Guarding_03_dry` | 2.376 | -19.40 | -1.5 | 51.1 | 100.0 | 102.0 | 176 | Let's get past the thing guarding it first. |
| `VO_C5_Wren_Route_01_dry` | 2.252 | -20.17 | -1.5 | 50.0 | 100.0 | 482.0 | 167 | Fine, we start with the cache. |
| `VO_C5_Wren_Route_02_dry` | 1.992 | -20.64 | -1.5 | 50.1 | 100.0 | 200.0 | 171 | Fine, we start with the cash. |
| `VO_C5_Wren_Route_03_dry` | 2.976 | -22.66 | -1.5 | 50.0 | 100.0 | 1270.0 | 167 | Fine, we start with the cash. |
| `VO_G5_Wren_ClearRoute_01_dry` | 2.552 | -19.00 | -1.6 | 50.0 | 100.0 | 246.0 | 182 | Cash is, behind it, stay out of its reach. |
| `VO_G5_Wren_ClearRoute_02_dry` | 2.788 | -20.57 | -1.5 | 50.5 | 100.0 | 436.0 | 169 | Cash is behind it. |
| `VO_G5_Wren_ClearRoute_03_dry` | 3.312 | -19.00 | -1.5 | 50.0 | 100.0 | 992.0 | 167 | Cash is Cash is is is |
| `VO_G5_Wren_BackToShips_01_dry` | 2.708 | -19.81 | -1.5 | 50.0 | 100.0 | 336.0 | 172 | We came for. R. R. a I. the. |
| `VO_G5_Wren_BackToShips_02_dry` | 2.900 | -20.08 | -1.5 | 54.1 | 100.0 | 496.0 | 178 | we came for,. Back to the ships. |
| `VO_G5_Wren_BackToShips_03_dry` | 3.058 | -19.30 | -1.5 | 50.0 | 100.0 | 852.0 | 172 | - |

## SHA256 of delivered takes

| File | SHA256 |
|---|---|
| `VO_C3_Wren_Answered_01_dry.wav` | `7fcf9918d38507dfec0518dab8ebb1622abc15e2e739b041b43f7a3edfcf223a` |
| `VO_C3_Wren_Answered_02_dry.wav` | `8ad3c25f0a0ea930b83ee0dcf3be85237969285a958fe3ffabf403ed51835f95` |
| `VO_C3_Wren_Answered_03_dry.wav` | `4b8e67782637c070a7bc07c1edf9c1dc67d56db93b920441aa963d37ebff8a4e` |
| `VO_C3_Wren_HoldThem_01_dry.wav` | `2a3a35018b36a6651a8225966a645ddae1d1c58cce4ffd0d1387429d0c0843d1` |
| `VO_C3_Wren_HoldThem_01_radio.wav` | `f0bd12046d7a564d89290c00f38c403184c3ee23fd733049b61f5b98ec34dc22` |
| `VO_C3_Wren_HoldThem_02_dry.wav` | `08893f00a8d9dd68c130383f51b1409c0f324a18728555b7f9fca93608da9607` |
| `VO_C3_Wren_HoldThem_02_radio.wav` | `ff16622bf35f860ea0e41a0d71556e2dda12275fe376661ac3da1826f4e2348c` |
| `VO_C3_Wren_HoldThem_03_dry.wav` | `d64a6cd054b89161ba779feeefd90fc30a1d50f5d9e129377a4946c179731aff` |
| `VO_C3_Wren_HoldThem_03_radio.wav` | `287dd182b5891fd3a74e6a73d93143da3848ddb591606fe52df24f5dbb1f4dfa` |
| `VO_C3_Wren_NotThePlanet_01_dry.wav` | `526e68acd21cabc9b60042158dd2ba35c9237238885209b3675bc045c9739b93` |
| `VO_C3_Wren_NotThePlanet_02_dry.wav` | `0547774cd586d3fb03fcbba75f2c42c8a3979cd993efd54749f82b39a71f9fa5` |
| `VO_C3_Wren_NotThePlanet_03_dry.wav` | `571ff929cf659d255090380c76b51e5f80fd6bb8c572fce9f236a143a90709d0` |
| `VO_C3_Wren_Survey_01_dry.wav` | `762fd286485646f831163fac0dc4eb69f334b5e3ed9bc4bf9cf2e568f50e0d39` |
| `VO_C3_Wren_Survey_02_dry.wav` | `42909b58158ae93c8625c46f32c91d1e856328c4084e021acb55dac362475d60` |
| `VO_C3_Wren_Survey_03_dry.wav` | `0035dff3d6c7345768c149a2d598e448518a57dd9ba920783bea3724905ad076` |
| `VO_C4_Wren_Badly_01_dry.wav` | `cf6fb3cd0ee90ba32e296d05aae2252c563eb4aa9726c0fe4c5c262db10d5600` |
| `VO_C4_Wren_Badly_02_dry.wav` | `af5a75901bf983f41a192725b6fea463edebd9b0e104b57a86b010f21fc1d6a4` |
| `VO_C4_Wren_Badly_03_dry.wav` | `c0a8c509b2235baa152013f21097b45b98cb12ae7a6bf16f60a197d919168f43` |
| `VO_C5_Wren_Guarding_01_dry.wav` | `1839d342a24debf9de4aa2676c5639b8c1dbdacf9aacb3e0cfb377567cdb0b3e` |
| `VO_C5_Wren_Guarding_02_dry.wav` | `1495411897c502028aaf52d942f39f9b37c4f20e2382b89550d35d0c3fc780cd` |
| `VO_C5_Wren_Guarding_03_dry.wav` | `0fde7223757975bcc18465932ba93c0400ef66d4b43d8be98abc7f8a3010f98a` |
| `VO_C5_Wren_Route_01_dry.wav` | `8dacc482965b6f785d9d0efce870bc76239b2abc608acef6ba3066818c5a82ce` |
| `VO_C5_Wren_Route_02_dry.wav` | `afac72b56554a347fb44ccd9747bdf61245a2de252a3489d2471025092542b51` |
| `VO_C5_Wren_Route_03_dry.wav` | `31de96d24c355535b73b03ed8ecf74efdabbbc4fa55c14c9630abd0eb445a819` |
| `VO_G2_Wren_Geology_01_dry.wav` | `f87c7344df8f2add6bb0c4ec96a9d23e58d1db5a479bfc4c26880f66487d2f62` |
| `VO_G2_Wren_Geology_01_radio.wav` | `3e74c93f2461afba7e940dcdafea7d3604749304fdf945623ab57f962b55e65f` |
| `VO_G2_Wren_Geology_02_dry.wav` | `9252dc4d5212bbbbc4dfc54a11cc7a38ffce61ec51a9d15f19036df0d1109d1f` |
| `VO_G2_Wren_Geology_02_radio.wav` | `95347b089d85a2c374302df4ddf3689d28928492f27a5a59eb00628356f610f6` |
| `VO_G2_Wren_Geology_03_dry.wav` | `7d1b98c7886483b24b97b88f0dae65f4c1417cfa8da27e7f183fa2dfc362b73f` |
| `VO_G2_Wren_Geology_03_radio.wav` | `8295695be7328208a4187a65298f14b7effe98b87f8e58ff5638f8cb2ea3f8bb` |
| `VO_G3_Wren_LeftApproach_01_dry.wav` | `a31acdf7f36dab511f84868340dabbfb6f4a2ff78ae1b6d9e47d11a00d14296e` |
| `VO_G3_Wren_LeftApproach_01_radio.wav` | `058a2bec64caaf32e72c816f13b5942f796104c97ca059f0a9b5ef33f2a65371` |
| `VO_G3_Wren_LeftApproach_02_dry.wav` | `3672f7670c8914ebf7675dfa95b19a94c23966553be1806f0e0fee566e4399aa` |
| `VO_G3_Wren_LeftApproach_02_radio.wav` | `1d5c4e4be66c8d49a2f643242445b584b8d21db514255b506542005a72fb3208` |
| `VO_G3_Wren_LeftApproach_03_dry.wav` | `b385f7390c59ff46fb58dffaadcdac801a9bf68c220694353b1a827a73103304` |
| `VO_G3_Wren_LeftApproach_03_radio.wav` | `b423832dd30f7acd352edd97bd0bddae760f3f05c053f30c71df26a36d6a1601` |
| `VO_G4_Wren_StartThere_01_dry.wav` | `66a6432c1febe267846467f54a9ad656a6f4884d934ea042a06eb9c859f441ce` |
| `VO_G4_Wren_StartThere_01_radio.wav` | `274462e27a3ced915690fe872734d9a96fece29068252d8aa926c34ed7ad8368` |
| `VO_G4_Wren_StartThere_02_dry.wav` | `c0adc1875f05b7f2ef6c73f413a06789970ecb0c4f8bccc02322d9a2261d9a0b` |
| `VO_G4_Wren_StartThere_02_radio.wav` | `969e78838f3d920e36f60991b91eeea31abe54b5db859d5a5f3941ac46a93979` |
| `VO_G4_Wren_StartThere_03_dry.wav` | `301a974fa9871246dbe7599197025528a2d44e34dcfa7824a2ef82d13f8676c1` |
| `VO_G4_Wren_StartThere_03_radio.wav` | `a237b690bd926a3e6f3dfac7f05e83644592c540e088165a6d1cfa28c7260395` |
| `VO_G5_Wren_BackToShips_01_dry.wav` | `3d92cdfc4b7f975e258f9a305c246230d315bc5e0ef126891a92027393813f4a` |
| `VO_G5_Wren_BackToShips_01_radio.wav` | `63a72df8237cf95b9f420e7ee9856c337d6243f8d7aac28e04ae55489e65e997` |
| `VO_G5_Wren_BackToShips_02_dry.wav` | `7144e59a102eeddb14cf1f59581e4ca25ea4496c726e873ce4213cf1f04002f5` |
| `VO_G5_Wren_BackToShips_02_radio.wav` | `15f580684e74918b12a987cf4540d97e5b0934743ae104c3c4220c8b9d42183e` |
| `VO_G5_Wren_BackToShips_03_dry.wav` | `dc8ffc6c8f7bb557bf5f179efaf4251789a70aa77ee704f5c4d38e881d903e57` |
| `VO_G5_Wren_BackToShips_03_radio.wav` | `590e397586af98af271cfcf004365a5363661d695fb95eddfa6b2c0387cd4e9b` |
| `VO_G5_Wren_ClearRoute_01_dry.wav` | `23d38f666f6bca958cc855927a701d66ae76d6152840e02f97cfebc0cc924982` |
| `VO_G5_Wren_ClearRoute_01_radio.wav` | `b6f3ded9658155bed75ec4aa26c7c91578fda7b970ac7aedd4e2446d0311fe3f` |
| `VO_G5_Wren_ClearRoute_02_dry.wav` | `613df73b4307226a0b532084a46296707e3bf0879d81c838b63b7b2eee0d1764` |
| `VO_G5_Wren_ClearRoute_02_radio.wav` | `88f5184f86f6c6d5310744abc88ff809b9eb0ed15dc480b8b773569224501cfd` |
| `VO_G5_Wren_ClearRoute_03_dry.wav` | `465a2fb1032fbdb58e8bd3c4b9ad4ea52146641d52e10479667a77805fb991c9` |
| `VO_G5_Wren_ClearRoute_03_radio.wav` | `94fc77fb8b4b8564f2d88aedd945d78ff8a1e7d624eb5d31ef81cf3307e03220` |

## Skipped / out of scope

- `O_Unknown_Dont` / `F_FutureWren_Dont` — shared master from PR #78 take 04 radio.
- `C8_Wren_Order` — present-Wren audition in PR #93 (Nicholas has not picked a master yet).
- Retired N-087 ids and Control/Unknown lines that only mention Wren are not in this set.

Working-draft wording is recorded as it stands in `dialogs.md` / `docs/story/transmissions.md`
on the day of generation. Nicholas may later replace a line; the take is still useful as a temp.

