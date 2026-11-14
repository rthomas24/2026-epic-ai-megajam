# Wren voice r1: Dominion, finale and Future Wren (R-042 ask 4)

**For:** Nicholas (listen and pick or confirm the recommended take), Nick's Audio / T3 (integrate the approved file), Ryan / Lead.
**Stage:** technically verified only. The agent that made these cannot hear them.
Nicholas approves by ear. Recommended takes below were picked from timing, median F0,
loudness and a faster-whisper `small.en` transcript, not from a listen.

**Use:** the rest of Wren's current draft lines in this chapter batch, in take 04's voice
(PR #78 master / PR #93 setup). The shared interrupted **Don't—** master and **C8_Wren_Order**
are not in this PR and must not be regenerated.

Dominion approach through the finale, plus the Future Wren full warning, last-chance recording, barracks recording and 'Open a channel' line.

## Files

| File | What it is |
|---|---|
| `VO_<Id>[_Variant]_0N_dry.wav` | Dry take, 50 ms pre-roll, 100 ms tail, 48 kHz, mono, 24-bit |
| `VO_<Id>[_Variant]_0N_radio.wav` | Radio version of the same take (chain below). Used for space / comms / Static lines |
| `VO_<Id>[_Variant]_0N_echo.wav` | Echo (doubled) version of the same take, for Future Wren recordings |
| `raw/VO_*_liora.wav` | Untouched xAI TTS outputs (48 kHz, 16-bit mono WAV straight from the API) |
| `tools/process_voice.py`, `tools/audio_common.py` | Rebuilds the dry / radio / echo files from `raw/` |
| `build_log.json` | Per take: speech bounds, mid-sentence pause, gains, loudness |
| `stt.json` | Local faster-whisper `small.en` transcripts, median F0, SHA256 |
| `dominion_finale_r1_sheet.png` | Waveform and spectrogram of every delivered file |

## Recommended takes

The agent cannot hear these. Pick below is from timing, pitch and transcript only.
`small.en` is noisy on short lines and regularly writes 'cash' for 'cache' and 'chip' for 'ship'.
Those known confusions were treated as a match when the rest of the words lined up.

| Line | Recommended | Alternative | Spoken text | Why |
|---|---|---|---|---|
| `C6_Wren_StayTogether` | **take 01** | — | Stay together. Clear the guard line, then the generators. | Only exact full-line transcript. Takes 02 and 03 dropped words or came back empty. |
| `G6_Wren_ClearCarriers` | **take 02** | — | Take the carriers. Don't let them keep launching. | Only exact transcript. |
| `G6_Wren_CoverTheBay` | **take 01** | take 03 | Keep that bay covered. We need a way back out. | No exact STT. Take 01 and 03 both show the second sentence; take 01 is tighter. Needs a listen. |
| `C7_Wren_Record` | **take 01** | — | Record everything. Find who's giving the orders. | Only exact full-line transcript. |
| `G7_Wren_FullRecord` | **take 02** | take 03 | We hear the full record before we touch that reactor. | Exact, compact. Take 03 is also exact. |
| `F_FutureWren_FullWarning` | **take 01** | take 02 | Don't engage the Dominion. You don't understand what happens if you do. | Exact, including the period between the two sentences. Take 02 is also exact. This is the full sentence and must not reuse the shared Don't— master. |
| `C8_RecWren_LastChance` | **take 03** | take 01 | We believe recovering it can prevent the catastrophe. It's the last chance we have. | Only exact full-line transcript. Echo treatment (Future Wren recording). Take 02 STT collapsed. |
| `G8_Wren_ConduitsFirst` | **take 03** | take 02 | Conduits first. Then the heart. | All three exact; take 03 hits the −19.0 LUFS target, 01 and 02 sit a little quiet. |
| `G9_Wren_HoldingWayOut` | **take 01** | take 03 | Keep moving. We're holding the way out. | Exact and tight (196 ms pause). Take 03 is exact but is the long [pause] roll (2.4 s). |
| `G9_Wren_PullAway` | **take 02** | take 01 | Triad, pull away. Follow the clear vector. | No exact STT. Take 02 has both 'pull away' and 'Follow the clear vector'; take 01 dropped 'pull away' in STT. |
| `C9_Wren_Clear` | **take 01** | take 02 | Rift clear. All three accounted for. | Exact and tighter than take 02. Take 03 dropped 'all three'. |
| `C9_Wren_Source` | **take 01** | take 03 | The source? | All three exact; take 01 is the tightest (0.78 s). |
| `A4_GeneratorDown3` | **take 01** | take 03 | Three! One more and it's open. | Exact, natural 188 ms pause. Take 03 is exact but a long 1.7 s [pause] hold. |
| `A4_Recording_Barracks` | **take 02** | take 01 | Recover the Core. Protect the younger Triad. | All three exact. Echo treatment (Future Wren recording). Take 02 has a mid pause (376 ms) in the same neighbourhood as the approved present-Wren Order takes. |
| `F_FutureWren_Channel` | **take 02** | take 01 | Open a channel. Now. | Exact and the tightest of the three. Take 01 is also exact. |

## Tool and model

- **Model:** xAI Text to Speech (Grok voice), run on **fal.ai**. Endpoint **`xai/tts/v1`**
  ([model page](https://fal.ai/models/xai/tts/v1)). Run on Ryan's fal account on 2026-11-14.
- **Voice:** `liora` (xAI: "calm, grounded, luminous"). This is the voice of PR #78 take 04,
  which Nicholas selected as the shared `Dont` radio master, and of the PR #93 present-Wren Order audition.
- **Common settings (same as `audition_r1` / `present_order_r1`):** `language: "en"`,
  `output_format: {codec: "wav", sample_rate: 48000}`, `enable_safety_checker: true`,
  all other inputs at their defaults. No `speed` parameter was sent. No seed parameter exists,
  so a rerun will not produce the same audio. The `raw/` files are the record.
- **Cost (this batch):** 45 generations, 2084 characters, at $0.015 per 1,000 characters,
  so **about $0.031**. Speech-to-text checks ran locally (faster-whisper `small.en`) at no cost.
- **Credit:** Voice: xAI Text to Speech (Grok), via fal.ai. Commercial-use / Partner model.
  Do not present as human-generated. Do not use to train models.

Story lines have three takes. Takes 01 and 02 send the draft line with no speech tags.
Take 03 of a multi-sentence line inserts a documented `[pause]` tag between sentences;
the spoken words stay exact. Barks have two exact takes and no pause tags.

### Generations

| Stem | Exact `text` sent | fal `request_id` |
|---|---|---|
| `VO_C6_Wren_StayTogether_01` | `Stay together. Clear the guard line, then the generators.` | `01a1dadb-45d3-7520-a95d-59ebc13e07c0` |
| `VO_C6_Wren_StayTogether_02` | `Stay together. Clear the guard line, then the generators.` | `01a1dadc-6d27-7001-87d5-46aab7355edf` |
| `VO_C6_Wren_StayTogether_03` | `Stay together. [pause] Clear the guard line, then the generators.` | `01a1dadc-6f5c-72c2-8ed6-41241ffd5529` |
| `VO_G6_Wren_ClearCarriers_01` | `Take the carriers. Don't let them keep launching.` | `01a1dadc-6f57-78f3-a873-7c3c323a3284` |
| `VO_G6_Wren_ClearCarriers_02` | `Take the carriers. Don't let them keep launching.` | `01a1dadc-6f5f-72d0-8ae1-5e4a05774728` |
| `VO_G6_Wren_ClearCarriers_03` | `Take the carriers. [pause] Don't let them keep launching.` | `01a1dadc-6f5b-7e80-8051-05d24999a337` |
| `VO_G6_Wren_CoverTheBay_01` | `Keep that bay covered. We need a way back out.` | `01a1dadc-6f5a-7ba0-bf9c-9ae7e14b264f` |
| `VO_G6_Wren_CoverTheBay_02` | `Keep that bay covered. We need a way back out.` | `01a1dadc-6e19-7180-83ca-38f3574597e9` |
| `VO_G6_Wren_CoverTheBay_03` | `Keep that bay covered. [pause] We need a way back out.` | `01a1dadc-6df1-7d90-b4b9-f1a6dbb9fab2` |
| `VO_C7_Wren_Record_01` | `Record everything. Find who's giving the orders.` | `01a1dadc-7d01-7eb2-81b9-0d2864253412` |
| `VO_C7_Wren_Record_02` | `Record everything. Find who's giving the orders.` | `01a1dadc-7d4d-7fe1-8e5e-68904c942bda` |
| `VO_C7_Wren_Record_03` | `Record everything. [pause] Find who's giving the orders.` | `01a1dadc-7d48-7110-99c8-4e2bb0a38707` |
| `VO_G7_Wren_FullRecord_01` | `We hear the full record before we touch that reactor.` | `01a1dadc-7d91-7741-821c-cd4479c0d4b7` |
| `VO_G7_Wren_FullRecord_02` | `We hear the full record before we touch that reactor.` | `01a1dadc-7d5b-7b01-bdaf-de8978e33c7c` |
| `VO_G7_Wren_FullRecord_03` | `We hear the full record before we touch that reactor.` | `01a1dadc-7d06-79b0-a2ea-45aea509f69e` |
| `VO_F_FutureWren_FullWarning_01` | `Don't engage the Dominion. You don't understand what happens if you do.` | `01a1dadc-7d50-7811-a4a5-c5324e4c65f3` |
| `VO_F_FutureWren_FullWarning_02` | `Don't engage the Dominion. You don't understand what happens if you do.` | `01a1dadc-7d5e-7fe0-ad2c-83fac9c1b7dc` |
| `VO_F_FutureWren_FullWarning_03` | `Don't engage the Dominion. [pause] You don't understand what happens if you do.` | `01a1dadc-8e40-7483-91ea-d6c0f001787b` |
| `VO_C8_RecWren_LastChance_01` | `We believe recovering it can prevent the catastrophe. It's the last chance we have.` | `01a1dadc-8e5f-79e2-8193-731da96bcdc8` |
| `VO_C8_RecWren_LastChance_02` | `We believe recovering it can prevent the catastrophe. It's the last chance we have.` | `01a1dadc-8e02-7a71-a07e-7c3965bac6bf` |
| `VO_C8_RecWren_LastChance_03` | `We believe recovering it can prevent the catastrophe. [pause] It's the last chance we have.` | `01a1dadc-8e42-7302-8717-c8f4930b4b98` |
| `VO_G8_Wren_ConduitsFirst_01` | `Conduits first. Then the heart.` | `01a1dadc-8ddb-7e22-a411-bf378cdb928c` |
| `VO_G8_Wren_ConduitsFirst_02` | `Conduits first. Then the heart.` | `01a1dadc-8e34-74f0-8fb5-9fe90c00ba7b` |
| `VO_G8_Wren_ConduitsFirst_03` | `Conduits first. [pause] Then the heart.` | `01a1dadc-8e05-7611-bfd8-4b2e72f94c20` |
| `VO_G9_Wren_HoldingWayOut_01` | `Keep moving. We're holding the way out.` | `01a1dadc-8e63-7072-a486-dfc5f0e53806` |
| `VO_G9_Wren_HoldingWayOut_02` | `Keep moving. We're holding the way out.` | `01a1dadc-a0b1-7e32-89c3-7181930a3bf1` |
| `VO_G9_Wren_HoldingWayOut_03` | `Keep moving. [pause] We're holding the way out.` | `01a1dadc-a06c-7912-bafb-34fcc52c8748` |
| `VO_G9_Wren_PullAway_01` | `Triad, pull away. Follow the clear vector.` | `01a1dadc-a0ed-72d0-87ba-33863e3787c7` |
| `VO_G9_Wren_PullAway_02` | `Triad, pull away. Follow the clear vector.` | `01a1dadc-a0b6-73b2-93cb-df61c1caa566` |
| `VO_G9_Wren_PullAway_03` | `Triad, pull away. [pause] Follow the clear vector.` | `01a1dadc-a0bb-7ee1-9994-3469c0bc434f` |
| `VO_C9_Wren_Clear_01` | `Rift clear. All three accounted for.` | `01a1dadc-a0b9-70d0-a32c-764b1cefde53` |
| `VO_C9_Wren_Clear_02` | `Rift clear. All three accounted for.` | `01a1dadc-a0b7-7a61-8e16-48bd2b8b2ff5` |
| `VO_C9_Wren_Clear_03` | `Rift clear. [pause] All three accounted for.` | `01a1dadc-a0bf-7f13-b025-acaf77a20975` |
| `VO_C9_Wren_Source_01` | `The source?` | `01a1dade-5147-7583-89f3-dff9cbf58e05` |
| `VO_C9_Wren_Source_02` | `The source?` | `01a1dade-5146-7941-86a8-d977eb4dad6d` |
| `VO_C9_Wren_Source_03` | `The source?` | `01a1dade-5145-79b3-9752-b2565d5b026e` |
| `VO_A4_GeneratorDown3_01` | `Three! One more and it's open.` | `01a1dade-513f-78a3-b7f0-25f668d21be3` |
| `VO_A4_GeneratorDown3_02` | `Three! One more and it's open.` | `01a1dade-513b-7e93-a064-8118bb98abf9` |
| `VO_A4_GeneratorDown3_03` | `Three! [pause] One more and it's open.` | `01a1dade-5133-7fb3-a643-f385e6915715` |
| `VO_A4_Recording_Barracks_01` | `Recover the Core. Protect the younger Triad.` | `01a1dade-5137-7531-825e-0296e0d53138` |
| `VO_A4_Recording_Barracks_02` | `Recover the Core. Protect the younger Triad.` | `01a1dade-5130-7cb1-bbcf-3c36e042c52a` |
| `VO_A4_Recording_Barracks_03` | `Recover the Core. [pause] Protect the younger Triad.` | `01a1dade-5f95-7440-90e7-59b565311ff9` |
| `VO_F_FutureWren_Channel_01` | `Open a channel. Now.` | `01a1dade-5fc8-78e1-a04f-9d7649587405` |
| `VO_F_FutureWren_Channel_02` | `Open a channel. Now.` | `01a1dade-5f4c-7e62-a920-b2b3c00bad92` |
| `VO_F_FutureWren_Channel_03` | `Open a channel. [pause] Now.` | `01a1dade-5fa4-7923-a745-76c007433f0d` |

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

Radio files ship for: `C6_Wren_StayTogether`, `G6_Wren_ClearCarriers`, `F_FutureWren_FullWarning`, `G9_Wren_HoldingWayOut`, `G9_Wren_PullAway`, `C9_Wren_Clear`, `C9_Wren_Source`, `A4_GeneratorDown3`, `F_FutureWren_Channel`.

## Echo chain (Future Wren recordings)

In-engine style is "Echo (doubled)". Built from the dry take:
dry plus a 95 ms delayed copy at −7 dB, low-passed at 3.4 kHz, then −19 LUFS.

Echo files ship for: `C8_RecWren_LastChance`, `A4_Recording_Barracks`.

Dry-only (in-person / on-foot, no radio or echo), same convention as `C8_Wren_Order`: `G6_Wren_CoverTheBay`, `C7_Wren_Record`, `G7_Wren_FullRecord`, `G8_Wren_ConduitsFirst`.

## Levels

Loudness is K-weighted (ungated under 3 s, gated integrated at or over 3 s). TP is true peak.
The `vo` window is −21..−17 LUFS. One known miss is noted in the recs if present.

| File | Dur s | LUFS | TP dBTP | Lead ms | Tail ms | Mid pause ms | Median F0 Hz | STT (short) |
|---|---|---|---|---|---|---|---|---|
| `VO_C6_Wren_StayTogether_01_dry` | 3.430 | -19.20 | -1.5 | 50.0 | 100.0 | 258.0 | 167 | Stay together, clear the guard line, then the generators. |
| `VO_C6_Wren_StayTogether_02_dry` | 3.868 | -19.60 | -1.5 | 50.0 | 100.0 | 472.0 | 161 | together, then the generators. |
| `VO_C6_Wren_StayTogether_03_dry` | 4.394 | -20.20 | -1.5 | 50.0 | 100.0 | 1350.0 | 158 |  |
| `VO_G6_Wren_ClearCarriers_01_dry` | 2.896 | -20.18 | -1.5 | 50.0 | 100.0 | 358.0 | 176 | carriers don't let them think |
| `VO_G6_Wren_ClearCarriers_02_dry` | 2.860 | -19.48 | -1.5 | 50.3 | 100.0 | 384.0 | 175 | Take the carriers, don't let them keep launching. |
| `VO_G6_Wren_ClearCarriers_03_dry` | 2.686 | -20.90 | -1.5 | 50.2 | 100.1 | 308.0 | 172 | Take the Take the |
| `VO_G6_Wren_CoverTheBay_01_dry` | 2.278 | -19.00 | -3.3 | 50.2 | 100.0 | 114.0 | 175 | covered, we need a way back out. |
| `VO_G6_Wren_CoverTheBay_02_dry` | 2.468 | -19.00 | -2.9 | 50.9 | 100.6 | 180.0 | 173 | Keep that bay covered R |
| `VO_G6_Wren_CoverTheBay_03_dry` | 3.266 | -19.00 | -3.8 | 50.0 | 100.0 | 1026.0 | 178 | covered. We need a way back out. |
| `VO_C7_Wren_Record_01_dry` | 3.064 | -19.10 | -1.5 | 51.3 | 100.0 | 470.0 | 174 | record everything find who's giving the orders |
| `VO_C7_Wren_Record_02_dry` | 2.930 | -19.71 | -1.5 | 50.0 | 100.0 | 230.0 | 171 | Find who's- |
| `VO_C7_Wren_Record_03_dry` | 3.944 | -19.00 | -2.0 | 50.0 | 100.0 | 1426.0 | 175 | record find who |
| `VO_G7_Wren_FullRecord_01_dry` | 2.842 | -19.51 | -1.5 | 52.0 | 100.0 | 112.0 | 162 | We hear |
| `VO_G7_Wren_FullRecord_02_dry` | 2.850 | -19.00 | -3.1 | 50.5 | 100.0 | 104.0 | 171 | We hear the full record before we touch that reactor. |
| `VO_G7_Wren_FullRecord_03_dry` | 2.886 | -19.00 | -2.2 | 50.0 | 100.1 | 116.0 | 168 | We hear the full record before we touch that reactor |
| `VO_F_FutureWren_FullWarning_01_dry` | 3.628 | -19.00 | -1.9 | 50.0 | 100.0 | 206.0 | 170 | Don't engage the Dominion. You don't understand what happ… |
| `VO_F_FutureWren_FullWarning_02_dry` | 3.718 | -19.00 | -2.3 | 50.0 | 100.0 | 204.0 | 173 | Don't engage the Dominion, you don't understand what happ… |
| `VO_F_FutureWren_FullWarning_03_dry` | 4.090 | -19.00 | -2.3 | 53.7 | 100.0 | 776.0 | 178 | the Dominion you don't understand what happens if you do |
| `VO_C8_RecWren_LastChance_01_dry` | 4.750 | -19.00 | -1.8 | 50.8 | 100.0 | 478.0 | 169 | covering it can prevent the catastrophe it's the last cha… |
| `VO_C8_RecWren_LastChance_02_dry` | 4.708 | -19.00 | -2.2 | 50.7 | 100.0 | 472.0 | 175 | and |
| `VO_C8_RecWren_LastChance_03_dry` | 5.074 | -19.00 | -2.4 | 51.1 | 100.0 | 820.0 | 175 | We believe recovering it can prevent the catastrophe. It'… |
| `VO_G8_Wren_ConduitsFirst_01_dry` | 2.096 | -20.84 | -1.5 | 50.2 | 100.0 | 324.0 | 166 | conduits first, then the heart. |
| `VO_G8_Wren_ConduitsFirst_02_dry` | 2.136 | -20.43 | -1.5 | 50.0 | 100.0 | 454.0 | 176 | conduits first, then the heart. |
| `VO_G8_Wren_ConduitsFirst_03_dry` | 2.562 | -19.00 | -1.7 | 50.0 | 100.3 | 640.0 | 180 | conduits first, then the heart. |
| `VO_G9_Wren_HoldingWayOut_01_dry` | 2.066 | -19.00 | -3.8 | 50.1 | 100.5 | 196.0 | 189 | Keep moving, we're holding the way out. |
| `VO_G9_Wren_HoldingWayOut_02_dry` | 2.076 | -19.00 | -4.2 | 51.7 | 100.0 | 152.0 | 177 | Keep moving, we're holding |
| `VO_G9_Wren_HoldingWayOut_03_dry` | 4.266 | -19.00 | -5.7 | 50.2 | 102.0 | 2386.0 | 183 | Keep moving. We're holding the way out. |
| `VO_G9_Wren_PullAway_01_dry` | 3.070 | -19.00 | -1.8 | 50.1 | 100.0 | 446.0 | 162 | triad, follow the clear vector. |
| `VO_G9_Wren_PullAway_02_dry` | 3.060 | -19.00 | -2.8 | 50.0 | 100.0 | 524.0 | 188 | pull away. Follow the clear vector. |
| `VO_G9_Wren_PullAway_03_dry` | 3.482 | -19.00 | -2.9 | 50.0 | 100.0 | 886.0 | 181 | tri Follow the clear vector. |
| `VO_C9_Wren_Clear_01_dry` | 2.392 | -19.11 | -1.5 | 50.4 | 100.0 | 282.0 | 172 | Rift Clear, all three accounted for. |
| `VO_C9_Wren_Clear_02_dry` | 2.630 | -20.09 | -1.5 | 52.5 | 100.0 | 420.0 | 175 | Rift Clear, all three accounted for. |
| `VO_C9_Wren_Clear_03_dry` | 2.724 | -20.93 | -1.5 | 50.5 | 100.0 | 490.0 | 171 | Rift Clear, accounted for. |
| `VO_C9_Wren_Source_01_dry` | 0.782 | -19.00 | -2.2 | 51.6 | 100.0 | 0.0 | 189 | The Source? |
| `VO_C9_Wren_Source_02_dry` | 0.856 | -19.00 | -3.1 | 52.2 | 100.0 | 0.0 | 168 | the source. |
| `VO_C9_Wren_Source_03_dry` | 0.870 | -19.00 | -2.4 | 50.6 | 100.0 | 0.0 | 178 | The Source? |
| `VO_A4_GeneratorDown3_01_dry` | 2.024 | -19.00 | -5.6 | 50.0 | 100.0 | 188.0 | 208 | Three! One more and it's open. |
| `VO_A4_GeneratorDown3_02_dry` | 1.958 | -19.00 | -5.6 | 50.0 | 100.0 | 216.0 | 216 | Three, one more and it's |
| `VO_A4_GeneratorDown3_03_dry` | 3.478 | -19.00 | -4.9 | 51.5 | 100.3 | 1672.0 | 210 | Three. One more and it's open. |
| `VO_A4_Recording_Barracks_01_dry` | 3.022 | -19.00 | -1.6 | 51.5 | 100.0 | 530.0 | 172 | Recover the core. Protect the younger triad. |
| `VO_A4_Recording_Barracks_02_dry` | 3.070 | -19.30 | -1.5 | 50.0 | 100.0 | 376.0 | 175 | Recover the core. Protect the younger triad. |
| `VO_A4_Recording_Barracks_03_dry` | 3.434 | -19.50 | -1.5 | 50.7 | 100.0 | 714.0 | 181 | Recover the core. Protect the younger triad. |
| `VO_F_FutureWren_Channel_01_dry` | 1.592 | -19.00 | -2.3 | 50.4 | 100.0 | 184.0 | 165 | Open a channel now. |
| `VO_F_FutureWren_Channel_02_dry` | 1.580 | -19.00 | -2.6 | 50.2 | 100.0 | 194.0 | 166 | Open a channel now. |
| `VO_F_FutureWren_Channel_03_dry` | 1.916 | -19.11 | -1.5 | 50.1 | 100.0 | 478.0 | 162 | Open a channel now. |

## SHA256 of delivered takes

| File | SHA256 |
|---|---|
| `VO_A4_GeneratorDown3_01_dry.wav` | `6791927c2719332ab538a03fdcf5388bbcad3b8b7097b8a4c4263dba6a450276` |
| `VO_A4_GeneratorDown3_01_radio.wav` | `e7ae3e829c4c9e126ac9552a35b5d42649189256154481cc961254b542a8b360` |
| `VO_A4_GeneratorDown3_02_dry.wav` | `0513c8ffeb6431c95f7ce2e03adebdb971ce2c04a93f0bdc3548b11602927af4` |
| `VO_A4_GeneratorDown3_02_radio.wav` | `8df421e2c1550b497088f841a4418ebb039957f72e5c0f5178885d76721d91c2` |
| `VO_A4_GeneratorDown3_03_dry.wav` | `fb766150cae7e26e47bfedaf87facca57c6f74403ec18f1c38ca6b8af05f3ff9` |
| `VO_A4_GeneratorDown3_03_radio.wav` | `ed5dcaf0f2afe994bf2daf616b6b948ea38f926e5825a6f0753ac9a5eb0b79bc` |
| `VO_A4_Recording_Barracks_01_dry.wav` | `32346c45fec35c439673d500a70b8e7522a25510c0207396400bef7213956455` |
| `VO_A4_Recording_Barracks_01_echo.wav` | `e4402ca9b06d81c26f72479bf698f185327922f447bdf08dd416a7b5646c6386` |
| `VO_A4_Recording_Barracks_02_dry.wav` | `178f49b197803766dad6cc2aaae2818a7fbcc40aebc6d0856053403e1fb4b4fa` |
| `VO_A4_Recording_Barracks_02_echo.wav` | `3e564eed21eb3d4b4775c156427f666ecef7eca07d56a4139b6e52061dda8b4f` |
| `VO_A4_Recording_Barracks_03_dry.wav` | `ba1aa66e8f509b0d29f4d61b51bab5b90494cf256c68859bbbd36cdf5cefc729` |
| `VO_A4_Recording_Barracks_03_echo.wav` | `b863ad00b4fc170b272c7ff4798a5d0c8a2cee6482acad6a89b8fe5ce5e97f4c` |
| `VO_C6_Wren_StayTogether_01_dry.wav` | `89f650290aa8ee31548d0cd91de4410808d547d9e2efce97c99afa1dda8e37c3` |
| `VO_C6_Wren_StayTogether_01_radio.wav` | `92e6220f8a1fe935521cec0239277e95061492392a8d0efae3a41a02a2c0e802` |
| `VO_C6_Wren_StayTogether_02_dry.wav` | `dde6833cd77767cc29e340474c71c8fcaa31fa2760e5f6b625bbdab80261f4f8` |
| `VO_C6_Wren_StayTogether_02_radio.wav` | `88fac2ffbe6eb54c7192aac1bad63b3aae4534997e177dcf2b3a91c407455ccc` |
| `VO_C6_Wren_StayTogether_03_dry.wav` | `3efe43e6ed09343ea899be83a4b4f9391a3bb59ec9f52a9b5219b286bd0b4934` |
| `VO_C6_Wren_StayTogether_03_radio.wav` | `ef95b219d0628b5469b694b6a686487c1ae0b85c3197dc0194cca203f0816e36` |
| `VO_C7_Wren_Record_01_dry.wav` | `3462fd1de445be7c1a304d5a1f848ee43f150dff2b54b4d24ac7be4cd5a25258` |
| `VO_C7_Wren_Record_02_dry.wav` | `e4fd66159c71d6e679391a0c31320e038993b9de0536e637cb4157f47e861845` |
| `VO_C7_Wren_Record_03_dry.wav` | `10c5725143933cb2767d94b80b18aa17dc4fdf1ff76ab30a51d2a0729115d8ae` |
| `VO_C8_RecWren_LastChance_01_dry.wav` | `8f7011721c203ff1b8dfc5ef74eb28971b4db8ef2790987b9b1c3abe60b57ff6` |
| `VO_C8_RecWren_LastChance_01_echo.wav` | `1cce8550d927685b6dd1f25ef7f462cb85f0b997bf4d89ad31e705267699ebad` |
| `VO_C8_RecWren_LastChance_02_dry.wav` | `83ef26f7bf66f18a5170b1715c3b3b37a3c6eaa944a623da10293b1838ca0a26` |
| `VO_C8_RecWren_LastChance_02_echo.wav` | `474138cf1a3acbb607c68737c979609956e0c25480ab25e9123deafce403670c` |
| `VO_C8_RecWren_LastChance_03_dry.wav` | `1596d7e4dd09ec6ca839e045551340db0291b5980e5b0b4670d024cedff3730c` |
| `VO_C8_RecWren_LastChance_03_echo.wav` | `1f12373fbcd9700a339b8cdc0d12652621c9e926141038ae6ebfec89873a8e9f` |
| `VO_C9_Wren_Clear_01_dry.wav` | `049f05f3fc7956f20cb0169582ca53815233f023973fd55d8e29ace9338a1a77` |
| `VO_C9_Wren_Clear_01_radio.wav` | `91378819805ae195d8019ded25e64d7e25a2a73fca909a55caffe54af1f95502` |
| `VO_C9_Wren_Clear_02_dry.wav` | `4d8cbbccc10753ddb52cf2ffa022e1cab79e2e33021337e24d5f3d5b56757ec0` |
| `VO_C9_Wren_Clear_02_radio.wav` | `9b3565a0b83f1327b00bda3f23fe105a9be3dec7f8fe4e8514fe87080334474d` |
| `VO_C9_Wren_Clear_03_dry.wav` | `3613874b40b1948fb714cc9ad52bd6d0cbe9856fd9b58cd92453f7f237829c7f` |
| `VO_C9_Wren_Clear_03_radio.wav` | `0289873397e330b2e7ad863ffa929032c6d768c883d12be7cd90ff0bf263395e` |
| `VO_C9_Wren_Source_01_dry.wav` | `0f812c40e7b9d44142a347cc4129e0cd1f1fac887d900a07b619d0bc56d39504` |
| `VO_C9_Wren_Source_01_radio.wav` | `476bad783a5e6dd647e95b4b6ac00324318258458a290ec40e8b8d94eb68aa91` |
| `VO_C9_Wren_Source_02_dry.wav` | `66c84bfb4459b490370c0fe12ea2864c4425ce1f7bb0a830cf6777dc75cc0c86` |
| `VO_C9_Wren_Source_02_radio.wav` | `131abb4bb81c10355e12f1fa5b442fe4160fbcf750dd42456977ac50c34063a8` |
| `VO_C9_Wren_Source_03_dry.wav` | `2016d4cd1522b0b46e7c7be48f81b44544cbb8fed05537e8b266811d26c4698b` |
| `VO_C9_Wren_Source_03_radio.wav` | `6f9452efba54c2f1c39978a44dedcf948630af5b96a6436218e06fb453cec64b` |
| `VO_F_FutureWren_Channel_01_dry.wav` | `cd9affc3abaf71d547a69b0022bcedbd00b38c56561c8e48a2385645a8d4b6ab` |
| `VO_F_FutureWren_Channel_01_radio.wav` | `53b3751101590e09695807a9baf2ee943caec014a585c7bc3c84ecc1b3a69a78` |
| `VO_F_FutureWren_Channel_02_dry.wav` | `13c4bc95042c34b77f6d2a1b7a3762315c50ddbab6f0643c176f1fe36138afc0` |
| `VO_F_FutureWren_Channel_02_radio.wav` | `58931bc6e7c049ea05a2788f5465143549af6ba21f98df075ae483ad6ba95f22` |
| `VO_F_FutureWren_Channel_03_dry.wav` | `63813daa0f3c0093f57b9b96f0fb8c8c5d632cd7f4428c9400dd73ecc683347f` |
| `VO_F_FutureWren_Channel_03_radio.wav` | `86ff478fb70e02134f27b9cd8d396c1f0257fa97b126a4847440c806b242ad37` |
| `VO_F_FutureWren_FullWarning_01_dry.wav` | `9b81587455259072d2a9208ab6fbdab5e36280e7aa56b9301fa6268a5ea86562` |
| `VO_F_FutureWren_FullWarning_01_radio.wav` | `38edc903afcbb6cdfe0419546785bbc0b80782b2f32304340d888ca8d2538b3c` |
| `VO_F_FutureWren_FullWarning_02_dry.wav` | `5f18bbd91132ec8220e654da2d4829af6831600f0b1c41aebb4481c3740947bc` |
| `VO_F_FutureWren_FullWarning_02_radio.wav` | `e4486a361dc4a00008aa39e8318a36986395e7fc9f44da0bbf6e075c6ab6a5e3` |
| `VO_F_FutureWren_FullWarning_03_dry.wav` | `9c662ac9f23d5e7a6adf54a220316e1ac8272d2e6227858dfc508b8667495fa1` |
| `VO_F_FutureWren_FullWarning_03_radio.wav` | `1a0524d5b4d9e338c1e7b6bb744641eea3447a0a4c08b685618affa06c002721` |
| `VO_G6_Wren_ClearCarriers_01_dry.wav` | `29b07e4b08a44af2684759e2f3894a1d58fd2f0b23cc49d1e369048324fbef01` |
| `VO_G6_Wren_ClearCarriers_01_radio.wav` | `20b51d9193d8569ceacdde0f33c82eb151378eed3d8257175bc53811003bfc0e` |
| `VO_G6_Wren_ClearCarriers_02_dry.wav` | `a52e858cdf5ea7a0f0f2ca42a95d252ec2199dde59de92d72e2b088f3518120a` |
| `VO_G6_Wren_ClearCarriers_02_radio.wav` | `b078f011a643f7a6f58dea1246f812e5788e5d92ab67698818c1f57aee9018f3` |
| `VO_G6_Wren_ClearCarriers_03_dry.wav` | `be9198f1caff62a12481177aa9883bd3ca216067f9b2faed128fbce69cd7ba82` |
| `VO_G6_Wren_ClearCarriers_03_radio.wav` | `bd5836fb36e88a8737717da95976773613b6b548cb638e57338e43c7441b5f2e` |
| `VO_G6_Wren_CoverTheBay_01_dry.wav` | `9ca9ed96792175a5945d5fa30c50c133409afe6fd2e91817899ed3e56bb2319b` |
| `VO_G6_Wren_CoverTheBay_02_dry.wav` | `8d064d3f57aae104c25168e20a508f870a54e9b59b16e8e622b4d97b03211a82` |
| `VO_G6_Wren_CoverTheBay_03_dry.wav` | `60df0490347ab87a242b59f0e8534ade1efbbfc137f2750c01c85c0a073dd231` |
| `VO_G7_Wren_FullRecord_01_dry.wav` | `a56fd51c9427b851abffa3d36f1a1421c456abcb60171a9646def6b8c2e85323` |
| `VO_G7_Wren_FullRecord_02_dry.wav` | `4f9ebb3098ff2bb3f481c7445f3a1ea5f904cf8d57c232441dd316570e758a6c` |
| `VO_G7_Wren_FullRecord_03_dry.wav` | `3b262b84bc5d1d27c93d30609d2c4ab4406ef222a3f429e8df2f869e15df4afc` |
| `VO_G8_Wren_ConduitsFirst_01_dry.wav` | `5384e08b0b00bb0053177b87ebd10fb8d44a0f0f1e239a7595c427812c435b8f` |
| `VO_G8_Wren_ConduitsFirst_02_dry.wav` | `2f31d3ee46dd1cbe0a81dbafef91439f39ea4c18964bf3bb5318c2bed9fcdaf7` |
| `VO_G8_Wren_ConduitsFirst_03_dry.wav` | `3d50136ff74e95bc7181de47f1f8f0446d81c538a0818900ba3a8b2a96fee23e` |
| `VO_G9_Wren_HoldingWayOut_01_dry.wav` | `e08136306df9489ffb69b27f01582ab2f5c6d78ae5b5ad6e08e734ae94ccdef6` |
| `VO_G9_Wren_HoldingWayOut_01_radio.wav` | `7a73b98d40ac04772fd6d5b14b96c89bdfbdf54bec3c42bb1c2f56a9ef2cbe50` |
| `VO_G9_Wren_HoldingWayOut_02_dry.wav` | `5a4472154674f33e79e7eb22ebd570d4dd2640084b287d338c85d41924ebc18c` |
| `VO_G9_Wren_HoldingWayOut_02_radio.wav` | `7a2e8531cc9a13c1c88b0c6b1c3ea3ead770747487a8217621a4531416a47656` |
| `VO_G9_Wren_HoldingWayOut_03_dry.wav` | `4a8b4fd715746b00d4043ee8a40041a080cc6ccbe383fcf6aaf3552905e703ba` |
| `VO_G9_Wren_HoldingWayOut_03_radio.wav` | `cd73e41fcc9f6af4804bfad2b1c11ed5e95150176b6a90fb6ece705e27be272e` |
| `VO_G9_Wren_PullAway_01_dry.wav` | `47cd0f5d3a4725da11f9d2992c97d0765959ec5592eefc0728672087833abc20` |
| `VO_G9_Wren_PullAway_01_radio.wav` | `d4aa5ff18e66b37b23d2afe6a382444c4f427db0bcd0b93080b1bc09777917af` |
| `VO_G9_Wren_PullAway_02_dry.wav` | `406b0fda7e8a267d314ddbffc1b784a36e2fbcaf71204eb35a71a481ad6a5acc` |
| `VO_G9_Wren_PullAway_02_radio.wav` | `29f24acd65c069c146db25ce46ab09c8911f7f4ac9cca47d8b7b0e802e087d05` |
| `VO_G9_Wren_PullAway_03_dry.wav` | `a6cc016ed1164cbdb01143fc27fa4d5c2e930fc286f7d068aa6c9dd759f2dd73` |
| `VO_G9_Wren_PullAway_03_radio.wav` | `28462f868f9e6eec779556b2aee5af9c5467569b621ed98ac524f67c3795c874` |

## Skipped / out of scope

- `O_Unknown_Dont` / `F_FutureWren_Dont` — shared master from PR #78 take 04 radio.
- `C8_Wren_Order` — present-Wren audition in PR #93 (Nicholas has not picked a master yet).
- Retired N-087 ids and Control/Unknown lines that only mention Wren are not in this set.

Working-draft wording is recorded as it stands in `dialogs.md` / `docs/story/transmissions.md`
on the day of generation. Nicholas may later replace a line; the take is still useful as a temp.

