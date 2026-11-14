# Wren voice r1: Opening, A1 and S2 (R-042 ask 4)

**For:** Nicholas (listen and pick or confirm the recommended take), Nick's Audio / T3 (integrate the approved file), Ryan / Lead.
**Stage:** technically verified only. The agent that made these cannot hear them.
Nicholas approves by ear. Recommended takes below were picked from timing, median F0,
loudness and a faster-whisper `small.en` transcript, not from a listen.

**Use:** the rest of Wren's current draft lines in this chapter batch, in take 04's voice
(PR #78 master / PR #93 setup). The shared interrupted **Don't—** master and **C8_Wren_Order**
are not in this PR and must not be regenerated.

Opening comms, the unreachable-field line, and the S2 reunion / form-up lines.

## Files

| File | What it is |
|---|---|
| `VO_<Id>[_Variant]_0N_dry.wav` | Dry take, 50 ms pre-roll, 100 ms tail, 48 kHz, mono, 24-bit |
| `VO_<Id>[_Variant]_0N_radio.wav` | Radio version of the same take (chain below). Used for space / comms / Static lines |
| `raw/VO_*_liora.wav` | Untouched xAI TTS outputs (48 kHz, 16-bit mono WAV straight from the API) |
| `tools/process_voice.py`, `tools/audio_common.py` | Rebuilds the dry / radio / echo files from `raw/` |
| `build_log.json` | Per take: speech bounds, mid-sentence pause, gains, loudness |
| `stt.json` | Local faster-whisper `small.en` transcripts, median F0, SHA256 |
| `opening_s2_r1_sheet.png` | Waveform and spectrogram of every delivered file |

## Recommended takes

The agent cannot hear these. Pick below is from timing, pitch and transcript only.
`small.en` is noisy on short lines and regularly writes 'cash' for 'cache' and 'chip' for 'ship'.
Those known confusions were treated as a match when the rest of the words lined up.

| Line | Recommended | Alternative | Spoken text | Why |
|---|---|---|---|---|
| `O_Wren_HowBad` | **take 03** | take 02 | Three ships, one mystery. How bad could it be? | Only exact transcript of the full line. Take 02 is close but STT heard a repeated 'how'. Take 01 looks like a wording slip or a bad STT ('how the hell', 'missed'). |
| `O_Wren_BigShip` | **take 03** | take 01 | That's all one ship? | Clean short question. STT wrote 'chip' for 'ship' (a known small.en confusion); take 01 has a repeated 'that's all'. |
| `O_Wren_WhyNotFiring` | **take 01** | take 02 | What? Why didn't it shoot us? | Exact transcript, mid-sentence pause 516 ms. Take 02 is also exact but a higher median F0 (234 Hz). Take 03 STT only returned 'What?' with a long 1150 ms gap. |
| `O_Wren_Break` | **take 02** | take 03 | Break formation! | This pass of small.en returned empty transcripts on all three short shouts. An earlier pass of the same model on takes 02 and 03 read 'Break formation' / 'Break Formation'. Take 02 is slightly more compact. |
| `A1_FieldUnreachable` | **take 01** | take 02 | That shimmer around the ship: some kind of field. We can't get near it. | STT truncated all three; take 03 loops the last clause. Take 01 keeps a natural mid pause (552 ms) and the first half of the line in the transcript. Nicholas should listen — wording may be complete even where STT dropped words. |
| `S2_Wren_WelcomeParty` | **take 02** | take 03 | Is it over? I think I blacked out... | Exact transcript, tighter than take 03's 836 ms pause. |
| `S2_Wren_WhatDoing` | **take 01** | take 03 | Would you rather they had? | Exact transcript. Take 03 is also exact and a touch shorter; take 02 dropped the end. |
| `S2_Wren_Hostile` | **take 02** | take 03 | Fighters. A lot of them. All three of us got out. | Exact transcript at the −19.0 LUFS target. Take 03 is also exact but a little quieter and longer. |
| `S2_Wren_FormUp` | **take 02** | take 01 | All right. Verdance. Stay on me. | Exact after normalising 'Alright' / 'All right'. Tight 294 ms pause. Take 03 stutters 'verdance Dance'. |

## Tool and model

- **Model:** xAI Text to Speech (Grok voice), run on **fal.ai**. Endpoint **`xai/tts/v1`**
  ([model page](https://fal.ai/models/xai/tts/v1)). Run on Ryan's fal account on 2026-11-14.
- **Voice:** `liora` (xAI: "calm, grounded, luminous"). This is the voice of PR #78 take 04,
  which Nicholas selected as the shared `Dont` radio master, and of the PR #93 present-Wren Order audition.
- **Common settings (same as `audition_r1` / `present_order_r1`):** `language: "en"`,
  `output_format: {codec: "wav", sample_rate: 48000}`, `enable_safety_checker: true`,
  all other inputs at their defaults. No `speed` parameter was sent. No seed parameter exists,
  so a rerun will not produce the same audio. The `raw/` files are the record.
- **Cost (this batch):** 27 generations, 1039 characters, at $0.015 per 1,000 characters,
  so **about $0.016**. Speech-to-text checks ran locally (faster-whisper `small.en`) at no cost.
- **Credit:** Voice: xAI Text to Speech (Grok), via fal.ai. Commercial-use / Partner model.
  Do not present as human-generated. Do not use to train models.

Story lines have three takes. Takes 01 and 02 send the draft line with no speech tags.
Take 03 of a multi-sentence line inserts a documented `[pause]` tag between sentences;
the spoken words stay exact. Barks have two exact takes and no pause tags.

### Generations

| Stem | Exact `text` sent | fal `request_id` |
|---|---|---|
| `VO_O_Wren_HowBad_01` | `Three ships, one mystery. How bad could it be?` | `01a1dad9-7c1c-7501-ae8f-d03adb43d1e3` |
| `VO_O_Wren_HowBad_02` | `Three ships, one mystery. How bad could it be?` | `01a1dad9-7c64-7723-a1f4-840ddde15e88` |
| `VO_O_Wren_HowBad_03` | `Three ships, one mystery. [pause] How bad could it be?` | `01a1dad9-7c55-7882-8e58-d5f7ade036fd` |
| `VO_O_Wren_BigShip_01` | `That's all one ship?` | `01a1dad9-7c59-7993-8baf-fee199b2a80b` |
| `VO_O_Wren_BigShip_02` | `That's all one ship?` | `01a1dad9-7c1c-7430-b9f7-9a93bd36bc81` |
| `VO_O_Wren_BigShip_03` | `That's all one ship?` | `01a1dad9-7c66-7731-abc5-7fac367c9c46` |
| `VO_O_Wren_WhyNotFiring_01` | `What? Why didn't it shoot us?` | `01a1dad9-7c70-7ff3-8ed7-045595052bbf` |
| `VO_O_Wren_WhyNotFiring_02` | `What? Why didn't it shoot us?` | `01a1dad9-7c9d-7672-b2a7-262bcd250636` |
| `VO_O_Wren_WhyNotFiring_03` | `What? [pause] Why didn't it shoot us?` | `01a1dad9-8a20-72a1-bc10-0a5ee07a1900` |
| `VO_O_Wren_Break_01` | `Break formation!` | `01a1dad9-8a28-7e90-a0f0-3e11ef2c32df` |
| `VO_O_Wren_Break_02` | `Break formation!` | `01a1dad9-8a64-7d60-8850-b9db2cc216ee` |
| `VO_O_Wren_Break_03` | `Break formation!` | `01a1dad9-8a60-7890-850b-89761d1ee390` |
| `VO_A1_FieldUnreachable_01` | `That shimmer around the ship: some kind of field. We can't get near it.` | `01a1dad9-8a21-7810-bc0d-89f81de1187a` |
| `VO_A1_FieldUnreachable_02` | `That shimmer around the ship: some kind of field. We can't get near it.` | `01a1dad9-8a7f-7803-b5de-7ab0482a83a4` |
| `VO_A1_FieldUnreachable_03` | `That shimmer around the ship: some kind of field. [pause] We can't get near it.` | `01a1dad9-8a63-7a43-b156-5d15da7a2a54` |
| `VO_S2_Wren_WelcomeParty_01` | `Is it over? I think I blacked out...` | `01a1dad9-8a5e-7380-b592-ffe020aff088` |
| `VO_S2_Wren_WelcomeParty_02` | `Is it over? I think I blacked out...` | `01a1dad9-9bd8-7f83-b593-deefaa1d795e` |
| `VO_S2_Wren_WelcomeParty_03` | `Is it over? [pause] I think I blacked out...` | `01a1dad9-9b50-7492-9efb-73e1b4dc2076` |
| `VO_S2_Wren_WhatDoing_01` | `Would you rather they had?` | `01a1dad9-9aee-79f3-a5fc-9b1b264d7b03` |
| `VO_S2_Wren_WhatDoing_02` | `Would you rather they had?` | `01a1dad9-9aea-7a31-bcd7-e80c241efc04` |
| `VO_S2_Wren_WhatDoing_03` | `Would you rather they had?` | `01a1dad9-9b4a-78a1-82bc-de2ad8320583` |
| `VO_S2_Wren_Hostile_01` | `Fighters. A lot of them. All three of us got out.` | `01a1dad9-9b17-7ed2-aab6-4fc1882adfe1` |
| `VO_S2_Wren_Hostile_02` | `Fighters. A lot of them. All three of us got out.` | `01a1dad9-9b41-7762-8d19-c9378c39e9ff` |
| `VO_S2_Wren_Hostile_03` | `Fighters. [pause] A lot of them. [pause] All three of us got out.` | `01a1dad9-9af4-7351-8649-0dd77d2a2dfd` |
| `VO_S2_Wren_FormUp_01` | `All right. Verdance. Stay on me.` | `01a1dad9-ab3a-7663-828f-5666c15b9614` |
| `VO_S2_Wren_FormUp_02` | `All right. Verdance. Stay on me.` | `01a1dad9-ab7a-76d2-8ceb-1891c3abf660` |
| `VO_S2_Wren_FormUp_03` | `All right. [pause] Verdance. [pause] Stay on me.` | `01a1dad9-ab82-7db1-baf0-3d09e7cf9ae3` |

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

Radio files ship for: `O_Wren_HowBad`, `O_Wren_BigShip`, `O_Wren_WhyNotFiring`, `O_Wren_Break`, `A1_FieldUnreachable`, `S2_Wren_WelcomeParty`, `S2_Wren_WhatDoing`, `S2_Wren_Hostile`, `S2_Wren_FormUp`.

## Levels

Loudness is K-weighted (ungated under 3 s, gated integrated at or over 3 s). TP is true peak.
The `vo` window is −21..−17 LUFS. One known miss is noted in the recs if present.

| File | Dur s | LUFS | TP dBTP | Lead ms | Tail ms | Mid pause ms | Median F0 Hz | STT (short) |
|---|---|---|---|---|---|---|---|---|
| `VO_O_Wren_HowBad_01_dry` | 2.998 | -19.00 | -2.3 | 50.0 | 100.0 | 392.0 | 166 | Three, three ships, one missed, how the hell could it be? |
| `VO_O_Wren_HowBad_02_dry` | 3.334 | -19.00 | -2.3 | 50.0 | 100.1 | 426.0 | 184 | Three ships, one mystery, how And how bad could it be? |
| `VO_O_Wren_HowBad_03_dry` | 3.626 | -19.00 | -1.5 | 50.0 | 100.0 | 652.0 | 174 | three ships, one mystery. How bad could it be? |
| `VO_O_Wren_BigShip_01_dry` | 1.094 | -19.00 | -3.4 | 50.0 | 100.0 | 98.0 | 168 | That's all, that's all one ship. |
| `VO_O_Wren_BigShip_02_dry` | 1.112 | -19.00 | -2.9 | 50.0 | 100.0 | 114.0 | 171 | that's all-ship? |
| `VO_O_Wren_BigShip_03_dry` | 1.142 | -19.00 | -3.6 | 50.0 | 100.0 | 114.0 | 172 | That's all one chip. |
| `VO_O_Wren_WhyNotFiring_01_dry` | 2.176 | -19.00 | -2.8 | 50.5 | 100.0 | 516.0 | 211 | What? Why didn't it shoot us? |
| `VO_O_Wren_WhyNotFiring_02_dry` | 1.914 | -19.00 | -2.7 | 50.0 | 100.0 | 460.0 | 234 | What? Why didn't it shoot us? |
| `VO_O_Wren_WhyNotFiring_03_dry` | 2.634 | -19.00 | -2.5 | 50.9 | 100.0 | 1150.0 | 226 | What? |
| `VO_O_Wren_Break_01_dry` | 1.094 | -19.00 | -3.9 | 52.3 | 100.0 | 0.0 | 211 |  |
| `VO_O_Wren_Break_02_dry` | 1.140 | -19.00 | -3.4 | 50.0 | 100.0 | 0.0 | 239 |  |
| `VO_O_Wren_Break_03_dry` | 1.218 | -19.00 | -4.2 | 50.0 | 100.0 | 0.0 | 208 |  |
| `VO_A1_FieldUnreachable_01_dry` | 4.286 | -19.00 | -2.9 | 50.0 | 100.0 | 552.0 | 197 | r around the ship, some kind of |
| `VO_A1_FieldUnreachable_02_dry` | 3.998 | -19.00 | -2.8 | 52.0 | 100.0 | 496.0 | 180 | some kind of field, we can't get near it. |
| `VO_A1_FieldUnreachable_03_dry` | 5.428 | -19.00 | -1.7 | 51.3 | 100.0 | 1142.0 | 176 | of field, we, we can't get near it, we can't get near it,… |
| `VO_S2_Wren_WelcomeParty_01_dry` | 2.160 | -19.00 | -3.6 | 50.0 | 100.2 | 340.0 | 176 | I think I blacked |
| `VO_S2_Wren_WelcomeParty_02_dry` | 2.250 | -19.00 | -2.7 | 54.0 | 100.0 | 326.0 | 184 | Is it over? I think I blacked out. |
| `VO_S2_Wren_WelcomeParty_03_dry` | 2.618 | -19.00 | -1.9 | 50.0 | 100.0 | 836.0 | 172 | Is it over? I think I blacked out. |
| `VO_S2_Wren_WhatDoing_01_dry` | 1.376 | -19.00 | -2.9 | 50.2 | 100.0 | 0.0 | 155 | Would you rather they had? |
| `VO_S2_Wren_WhatDoing_02_dry` | 1.364 | -19.00 | -3.2 | 50.4 | 100.0 | 0.0 | 160 | Would you rather |
| `VO_S2_Wren_WhatDoing_03_dry` | 1.288 | -19.00 | -3.1 | 51.7 | 100.0 | 0.0 | 164 | Would you rather they had? |
| `VO_S2_Wren_Hostile_01_dry` | 2.970 | -19.24 | -1.5 | 50.1 | 100.2 | 410.0 | 163 | A lot of them, all three of us got out. |
| `VO_S2_Wren_Hostile_02_dry` | 3.020 | -19.00 | -1.6 | 50.0 | 100.3 | 486.0 | 164 | fighters, a lot of them, all three of us got out. |
| `VO_S2_Wren_Hostile_03_dry` | 3.444 | -19.80 | -1.5 | 50.0 | 102.2 | 490.0 | 156 | fighters, a lot of them, all three of us got out. |
| `VO_S2_Wren_FormUp_01_dry` | 3.010 | -19.00 | -2.7 | 50.0 | 100.0 | 532.0 | 168 | Alright, verdance. Stay on. |
| `VO_S2_Wren_FormUp_02_dry` | 2.616 | -19.09 | -1.5 | 50.1 | 100.0 | 294.0 | 166 | Alright, verdance, stay on me. |
| `VO_S2_Wren_FormUp_03_dry` | 3.920 | -19.00 | -2.5 | 50.6 | 100.2 | 942.0 | 169 | Alright, verdance Dance, stay on me. |

## SHA256 of delivered takes

| File | SHA256 |
|---|---|
| `VO_A1_FieldUnreachable_01_dry.wav` | `521156e87e73c1d1045b84a38db825542402661a4f9adc03f08ad4f34f96eb67` |
| `VO_A1_FieldUnreachable_01_radio.wav` | `ac547e7645f29d89f14ada562b3e6ae1072f31d4da2497d70332684ab3eb9f90` |
| `VO_A1_FieldUnreachable_02_dry.wav` | `b951af9455e64e4671af9e65cc68739e53389c3f1df2848d3a786f37b1d5f62f` |
| `VO_A1_FieldUnreachable_02_radio.wav` | `0b6214e9e41afc502f64dff2d57ce9272d658a4bef90123cb8e3412f50265a3c` |
| `VO_A1_FieldUnreachable_03_dry.wav` | `5fdc6f7a2d6a8cdfc364fbad7d643947dfeb2ace69ddff9a331fe00feee90b02` |
| `VO_A1_FieldUnreachable_03_radio.wav` | `44b9bd79ef684284b40d4f4ab22ce986a559961de0aaeb82bfe0ff5f1cf1967c` |
| `VO_O_Wren_BigShip_01_dry.wav` | `75de78e99a33728c14b4d8c649895a4bcf265428390dd5f078f4139ae2df173d` |
| `VO_O_Wren_BigShip_01_radio.wav` | `bc26d08abffb2cec22d4fd6740da60556f6d0a4a9c62c450ec7502bae8d5187d` |
| `VO_O_Wren_BigShip_02_dry.wav` | `b0d2a466921e441ee96f1bf0c3b969d13eeebb17d8f08fde7b453bbf58f97a66` |
| `VO_O_Wren_BigShip_02_radio.wav` | `785948620425421608a6162dc7daef2b489a0f735bc5554a45d7dd8164711b20` |
| `VO_O_Wren_BigShip_03_dry.wav` | `58e4e57fea1b2bbe914a982812cc4da4cbc76552546fa7ed3720efd9118db09f` |
| `VO_O_Wren_BigShip_03_radio.wav` | `3c85f9fd5ae22f48515f9625a23c7e7baffc7a1a83e52177b2cd56df7df0b771` |
| `VO_O_Wren_Break_01_dry.wav` | `f0255ff7caaa8b9bed7885c4748c53c0d93a7b6b35e27ade3f9d97664159ab70` |
| `VO_O_Wren_Break_01_radio.wav` | `c0fc74d1835281ed45fe37c047024df00f46760953539cb8d925848d41981923` |
| `VO_O_Wren_Break_02_dry.wav` | `b84f6d98a0b44075f6cbe31c4af3e484b5434a6734023cc3e77b95cbc2583ded` |
| `VO_O_Wren_Break_02_radio.wav` | `927f493a1342ce11ea7a03b4916f12be8dab7fddb7682ea7cfcbc041fb5a2375` |
| `VO_O_Wren_Break_03_dry.wav` | `b7666800c7fa26d8c07ca9de1992b76a6825bac090f8e12d0566e62935c70fa5` |
| `VO_O_Wren_Break_03_radio.wav` | `6bccc51fff8ed062abcdb37560c0d11258d8749d83fe1783820b7c52c0555977` |
| `VO_O_Wren_HowBad_01_dry.wav` | `fff16a4d031ede6c703fce1a9e6fa89f0e2c730d0fc8362e50cab2717a427d6e` |
| `VO_O_Wren_HowBad_01_radio.wav` | `71702144ed1224e0378b9bde0eec9c5deb4cb35fa38eec3ea4ec85f2208519bd` |
| `VO_O_Wren_HowBad_02_dry.wav` | `c8b54d96ffe4dc44ef5ebddc115c4f14404f6a1f8af35b6337852775a4fd5e75` |
| `VO_O_Wren_HowBad_02_radio.wav` | `9db6cb6229943a0edbaf30e5cb28e882ffba9c3842826e0141d20ad155e11574` |
| `VO_O_Wren_HowBad_03_dry.wav` | `f02d47a9b49a5da64a0bec37e6598296b593111afe3d733acc223a8bb773724c` |
| `VO_O_Wren_HowBad_03_radio.wav` | `a55498b783efec5f6b6e05a86b0a2c9dd852fc5ccc50c90fe55c5694a43031b3` |
| `VO_O_Wren_WhyNotFiring_01_dry.wav` | `135c8cd8bb1921fd31bed6fd2934da73cc23342d444306695e50461d6bf7ba76` |
| `VO_O_Wren_WhyNotFiring_01_radio.wav` | `5be84e30c45934a3b83996faf991eb4a288c99aa01afeaee10b7da352c60a995` |
| `VO_O_Wren_WhyNotFiring_02_dry.wav` | `39fdb2ed45feac6dedb8513be73f55a97ce61aa2371be8d9dd461d3820ffb4b2` |
| `VO_O_Wren_WhyNotFiring_02_radio.wav` | `2a068934f96680781837a07957202cd885f32917ca58cb954bdb0f160bdafa0b` |
| `VO_O_Wren_WhyNotFiring_03_dry.wav` | `f7d86bd683824f17676101a19e2ad9e24a9e03ad54d0a084b562f030cfea142c` |
| `VO_O_Wren_WhyNotFiring_03_radio.wav` | `4beff3b1ebd0a8bce1e25fc2c6a3b5918e473fe2dedc634b08d011e9baaf9444` |
| `VO_S2_Wren_FormUp_01_dry.wav` | `0b1cf11dae2924fcaa4f42f7f0b647901a150ce8fe109b3e51de895e949865fd` |
| `VO_S2_Wren_FormUp_01_radio.wav` | `4fa162d25f2c1dc5525a2427d569be981a5b7caefbeb64b900d2bfbc3a04da89` |
| `VO_S2_Wren_FormUp_02_dry.wav` | `c7ef68d93a87f3c366a06ef6c0d1c877c809adda72e040cf831101e54c789e28` |
| `VO_S2_Wren_FormUp_02_radio.wav` | `d09575f41c41c63f094b536420d74d0c9b69ee2e47b9755abdc08c81d1aeadcc` |
| `VO_S2_Wren_FormUp_03_dry.wav` | `8d8da9ab4b740d3c20b91871e3a3b5947706311e97e206c5b7049079479b510f` |
| `VO_S2_Wren_FormUp_03_radio.wav` | `0a5265ffe5682785c8189bcfc7a0b6148a660187094dc01c2d2fcc23d3f8c1b4` |
| `VO_S2_Wren_Hostile_01_dry.wav` | `7a2d72bdf26514a3dfed9daca85cd98b534fb76741a0389bea4e767b141a96c0` |
| `VO_S2_Wren_Hostile_01_radio.wav` | `b1e806e2cba7ed84f57f9e963114a053f68a12eb99dae06c87003d10217aa9fe` |
| `VO_S2_Wren_Hostile_02_dry.wav` | `e8fd3d4beaa67cee6b7c8abfe85d663b316444a0fa17a3b206bad97d33323d6d` |
| `VO_S2_Wren_Hostile_02_radio.wav` | `a68ad557d746c52a8f53242c0b8e5048842f4df48ffddce632e909b4c291d0a3` |
| `VO_S2_Wren_Hostile_03_dry.wav` | `e15127605553e2d07ebe0d14f2c8ef6197746462a9a165254ad1d4a4fa811971` |
| `VO_S2_Wren_Hostile_03_radio.wav` | `43f6954a7aea78fdeb77d25a13f5c661f1e9a7bfdf847622864e5fb13b0b6e7a` |
| `VO_S2_Wren_WelcomeParty_01_dry.wav` | `da8e272e717c65278f7ddbb981b46a98bb713c5a08050275b0cb34dbc90f4eb7` |
| `VO_S2_Wren_WelcomeParty_01_radio.wav` | `13dc59d7f965f2aa046de9666ce77b2917b8dab54db582585a18f5c64fc52890` |
| `VO_S2_Wren_WelcomeParty_02_dry.wav` | `a01634ce97bd0e6304390653ea4ab62c56f20966ee9343bb1f230e2d3bd0de35` |
| `VO_S2_Wren_WelcomeParty_02_radio.wav` | `60176228dd17cf3c74af34fef6036aea38a9071b1f73e9782f4e60e5617ac5bd` |
| `VO_S2_Wren_WelcomeParty_03_dry.wav` | `dea7af73d1998f42a74b495a50248b8b0f44173953d856e13b48b81b1f84860c` |
| `VO_S2_Wren_WelcomeParty_03_radio.wav` | `15b4c28e5c83c4a1c07e5b83348334b5bdbc4658376f614f19d17d0b3e473bcc` |
| `VO_S2_Wren_WhatDoing_01_dry.wav` | `e586f839b773e037e32c153a354bf22fefd9996d2b295ebcca8da00aa31348e9` |
| `VO_S2_Wren_WhatDoing_01_radio.wav` | `b753dda0be74860b0e06abcd910dcce616c70d845cf79cd3b2d927ac45eb29e1` |
| `VO_S2_Wren_WhatDoing_02_dry.wav` | `7010e5beae3e82ec54220269bd9045b72f9b6caf2fcf7a225e5c945271b44110` |
| `VO_S2_Wren_WhatDoing_02_radio.wav` | `0230c42a1a7082323a36dd4ef9a64adbb6c5cc30a3beffdf7e96ff3f77b10938` |
| `VO_S2_Wren_WhatDoing_03_dry.wav` | `b13cd8465824af989af0c3eaa1722d5647deece4d25cb0b7154ef02592f86f7c` |
| `VO_S2_Wren_WhatDoing_03_radio.wav` | `644ec417fdc4f653ebf2f5fdd55891057fc9480bcabadcf5caf9b55b6338bd2b` |

## Skipped / out of scope

- `O_Unknown_Dont` / `F_FutureWren_Dont` — shared master from PR #78 take 04 radio.
- `C8_Wren_Order` — present-Wren audition in PR #93 (Nicholas has not picked a master yet).
- Retired N-087 ids and Control/Unknown lines that only mention Wren are not in this set.

Working-draft wording is recorded as it stands in `dialogs.md` / `docs/story/transmissions.md`
on the day of generation. Nicholas may later replace a line; the take is still useful as a temp.

