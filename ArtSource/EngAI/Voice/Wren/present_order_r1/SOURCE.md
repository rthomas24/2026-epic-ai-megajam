# Wren voice audition r1: present Wren, C8_Wren_Order (R-042 / N-097)

**For:** Nicholas (listen and pick the master), Nick's Audio / T3 (integrate the approved dry file), Ryan / Lead.
**Stage:** technically verified only. The agent that made these cannot hear them.
Nicholas approves by ear. This is a **clean in-person / present-Wren** audition, not a radio transmission.

**Use:** one dry take, once Nicholas picks it, is the candidate recording of the approved line
**`C8_Wren_Order`**: "Take out the reactor. Keep the route to the hangar clear."
The line is the present-day Wren order after Moss in Cinematic 08, leading into the reactor fight
(Story/README.md; `docs/story/transmissions.md`; Nicholas's wording, issue #29).
It is **not** the shared interrupted "Don't—" master from PR #78. No radio EQ, static, distortion,
music or reverb was applied.

No existing exact-line artefact was found in the repo, on any branch, or in the earlier
`audition_r1` set, so these takes were newly generated.

## Files

| File | What it is |
|---|---|
| `VO_PresentWren_Order_0N_dry.wav` (N = 1–5) | Dry take, no effects. Full sentence, 50 ms pre-roll, 100 ms tail, 48 kHz, mono, 24-bit |
| `raw/t0N_liora.wav` | The untouched xAI TTS outputs the takes were cut from (48 kHz, 16-bit mono WAV straight from the API) |
| `tools/build_wren_order_r1.py` | Rebuilds every dry file above from `raw/`. Uses the project's `audio_common.py` loudness meter |
| `build_log.json` | Per take: source file, detected speech bounds, mid-sentence pause, gain |
| `present_order_r1_sheet.png` | Waveform and spectrogram of all five dry files on one time scale |

**Recommended: take 03.** Alternative: **take 02.** See "Recommendation" below.

## Tool and model

- **Model:** xAI Text to Speech (Grok voice), run on **fal.ai**. Endpoint **`xai/tts/v1`**
  ([model page](https://fal.ai/models/xai/tts/v1)). Run on Ryan's fal account on 2026-11-14, ~09:19 EST.
- **Voice:** `liora` (xAI: "calm, grounded, luminous"). This is the voice of PR #78 take 04, which
  Nicholas selected as the shared `Dont` radio master (N-097). It is the starting identity for this
  clean present-Wren audition, not a blanket approval of the voice for all remaining lines.
- **Common settings (same as `audition_r1`):** `language: "en"`,
  `output_format: {codec: "wav", sample_rate: 48000}`, `enable_safety_checker: true`,
  all other inputs at their defaults. No `speed` parameter was sent (it is not in the fal schema).
  No seed parameter exists, so a rerun will not produce the same audio. The `raw/` files are the record.
- **Cost:** 5 generations, 293 characters in total, at $0.015 per 1,000 characters, so **under $0.01**
  (about $0.004). Speech-to-text checks ran locally (faster-whisper `small.en`) at no cost.

| Take | Voice | Exact `text` sent | Raw file | fal `request_id` |
|---|---|---|---|---|
| 01 | `liora` | `Take out the reactor. Keep the route to the hangar clear.` | `raw/t01_liora.wav` | `01a1da6d-8590-74d1-87ec-0c0ab6321fe8` |
| 02 | `liora` | `Take out the reactor. Keep the route to the hangar clear.` | `raw/t02_liora.wav` | `01a1da6d-85e7-7f41-94f5-643006bfe8ed` |
| 03 | `liora` | `Take out the reactor. Keep the route to the hangar clear.` | `raw/t03_liora.wav` | `01a1da6d-861d-7db2-b7fc-dafd6b0209b4` |
| 04 | `liora` | `Take out the reactor. [pause] Keep the route to the hangar clear.` | `raw/t04_liora.wav` | `01a1da6d-8645-7fd3-b45f-52acec71a186` |
| 05 | `liora` | `Take out the reactor. Keep the route to the hangar clear.` | `raw/t05_liora.wav` | `01a1da6d-85ff-70a2-b8c8-e6139bac0f98` |

Takes 01, 02, 03 and 05 send the approved line with no speech tags (four independent rolls;
the model has no seed, so delivery varies). Take 04 is the same words with a documented `[pause]`
tag between the two sentences, as a more deliberate delivery. No other tags were used
(no whisper, laugh, radio or reverb).

## Edit (dry takes)

1. Find the first and last speech on a 10 ms RMS envelope at −45 dBFS. Everything between those
   bounds is kept, including the pause between the two sentences. DC is removed.
2. Pad **50 ms** of silence before the onset and **100 ms** after the offset. The 50 ms head meets
   the project's `vo` category `lead_max` (the same family as the 45 ms pre-roll on `audition_r1`).
   The 100 ms tail is the short natural tail asked for in the brief (`vo` allows up to 150 ms).
3. 3 ms linear fade-in, 20 ms cos² fade-out, so the pads are click-free.
4. One linear gain to **−19 LUFS** (the project's `vo` target, same as `audition_r1`). Gain is also
   capped so true peak stays at or under −1.5 dBTP. 24-bit WAV, 48 kHz, mono.

## Levels (`audio_report.py --category vo`: all 5 PASS)

Loudness is K-weighted and ungated on the files under 3 s (takes 01, 02, 03, 05) and gated
integrated on take 04 (3.798 s). TP is true peak.

| File | Duration s | Loudness LUFS | True peak dBTP | Lead ms | Tail ms | Mid-sentence pause ms | Median F0 Hz |
|---|---|---|---|---|---|---|---|
| 01_dry | 2.966 | −19.0 | −2.1 | 50 | 100 | 204 | 170 |
| 02_dry | 2.964 | −19.0 | −2.1 | 50 | 100 | 236 | 173 |
| 03_dry | 2.994 | −19.0 | −1.9 | 50 | 100 | 234 | 181 |
| 04_dry | 3.798 | −19.0 | −3.6 | 50 | 100 | 958 | 193 |
| 05_dry | 2.976 | −19.1 | −1.5 | 50 | 100 | 254 | 183 |

Take 05 sits at −19.1 LUFS rather than −19.0 because the −1.5 dBTP cap limited the gain. That is
still inside the `vo` window of −21 to −17 LUFS.

SHA256 of the dry files:

| File | SHA256 |
|---|---|
| `VO_PresentWren_Order_01_dry.wav` | `38a92cafd9343912793ba7d32678ab4c4ed6535ebce2762b890190174a9f43c0` |
| `VO_PresentWren_Order_02_dry.wav` | `4c4ec7564d79012db24c9fb8d800460767a68cedaf7c0ea3b748c16014b3ff7d` |
| `VO_PresentWren_Order_03_dry.wav` | `e5be90ba51c35c1580e76ad5f012224036983dea4d285d3a83082196c4fc0e24` |
| `VO_PresentWren_Order_04_dry.wav` | `8922da88369deec45ac739e51a05a835d17317d54cc2d8cd2b06276640217ee1` |
| `VO_PresentWren_Order_05_dry.wav` | `cf21f24c506949e0abb9c782c5d7fe98499b7c976ad7ba59f71c4451683496d7` |

## Checks the agent can make without listening

- **Speech-to-text** (faster-whisper `small.en`, local), with each sentence cropped and transcribed
  on its own: every take comes back as **"Take out the reactor."** and **"Keep … the … clear."**
  The model consistently spells the second sentence's content words as "root" and "hanger". Those
  are ordinary homophone errors for **route** and **hangar**; they are not evidence of extra or
  missing words. A first-pass transcript of the whole of take 05 also heard "take" for "keep",
  primed by the opening word; the isolated second sentence of take 05 is clearly "Keep".
- **Pitch** (median F0, autocorrelation): 01 170 Hz, 02 173 Hz, 03 181 Hz, 04 193 Hz, 05 183 Hz.
  All are in the female range and sit below the short exclaimed "Don't—" of `audition_r1` take 04
  (253 Hz), which is expected for a full two-sentence line on the same `liora` voice. Take 01's
  10th-percentile F0 dropped to 79 Hz (an octave jump in the tracker, not a second speaker).
- **Not verifiable without ears:** whether any take sounds like natural human speech rather than
  synthetic, and whether the energy is "firm controlled urgency" rather than flat or over-acted.
  **Nicholas's listen decides.** If none passes, the established fallback is ElevenLabs on Nick's side.

## Recommendation

The agent cannot hear these. The pick below is from timing, pitch and transcript only.

- **Take 03** (recommended). Default delivery, no speech tags. A natural ~230 ms break between the
  two commands, not a long hold. Slightly higher pitch range than 01/02 (median 181 Hz, 90th
  percentile 266 Hz), which is the closest of the default rolls to "firm controlled urgency"
  without the nearly one-second wait of take 04. Isolated transcript is clean.
- **Take 02** (alternative). Same default delivery and a similar ~236 ms sentence break; a little
  calmer (median 173 Hz). Isolated transcript is also clean.

**Take 04** is available if a more deliberate reading is wanted: the `[pause]` tag produced a
**958 ms** gap between "reactor" and "Keep". That may feel slow or hesitant for an order given
just before the reactor fight. **Takes 01 and 05** are further default rolls with the same words
and a 200–250 ms break; they are in the folder for comparison, not the first pick.

## Usage terms for a commercial game jam entry

Read on 2026-11-13 for `audition_r1` and unchanged for this run. Ryan's account calls xAI's model
through fal, so **fal's terms govern the account**, and xAI's terms govern the model.

- **fal model page** ([fal.ai/models/xai/tts/v1](https://fal.ai/models/xai/tts/v1)): the model carries the **"Commercial
  use"** and **"Partner"** badges. The fal FAQ ([link](https://fal.ai/docs/documentation/model-apis/faq#can-i-use-generated-files-for-commercial-purposes))
  says: "Each model has its own license. Most models on fal are available for commercial use and are marked with a
  `Commercial use` badge on the model page. Models marked `Research only` are restricted to non-commercial use."
- **fal Terms of Service** ([fal.ai/terms](https://fal.ai/terms)), §5 / §6:
  - Output must be used "in a lawful manner" and must not "infringe … or otherwise violate any rights of Company or any
    third party".
  - fal "does not represent, warrant, or covenant that any Output Content will be original, will not infringe rights of
    any third party".
  - §14: third-party models "may be subject to additional terms" from their provider.
  - fal's terms say the customer owns the **Input**. They contain **no clause that explicitly assigns ownership of the
    Output** to the customer.
- **xAI Terms of Service, Enterprise** ([x.ai/legal/terms-of-service-enterprise](https://x.ai/legal/terms-of-service-enterprise),
  last updated October 27, 2026), "Rights in Input and Output":
  - The customer "owns all right, title, and interest in the Output in perpetuity and, to the fullest extent possible
    under applicable law, [xAI] hereby assigns to Customer all of its right, title, and interest in such Output".
  - The customer must not "use any Output to train any foundation models …" or "**misrepresent that any Output was
    human-generated**".
  - Outputs "may not be unique: the same or substantially similar Output may be generated for unrelated users".

**Summary.** Commercial use of the generated audio is permitted. fal marks this model "Commercial use" as a partner
model, and xAI's own terms give output ownership to the customer and let it be used in a product.

**Three conditions:**
1. **Credit it as AI-generated:** "Voice: xAI Text to Speech (Grok), via fal.ai". Never present it as a human
   performer. This matters for the jam credits, `CREDITS.md`, and any "cast" line. This PR does not edit `CREDITS.md`.
2. **Don't use it to train other models.**
3. **The voice is a stock xAI voice and isn't exclusive.** Other xAI users can produce the same timbre.

**What is unclear:**
- fal's terms say nothing explicit about output ownership.
- xAI's enterprise terms bind xAI's direct customers. For a call through fal, they apply through fal's partnership,
  which isn't published.
- Neither document addresses game jams specifically.

This is **clear enough for a jam entry**, but it isn't a written licence naming us. If the jam's rules ask for proof
of a licence, ask fal support for a short written confirmation (support@fal.ai).
