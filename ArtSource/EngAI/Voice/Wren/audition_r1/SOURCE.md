# Wren voice audition r1: future Wren, "Don't—" (R-042 / N-049)

**For:** Nicholas (listen), Nick's Audio (integrate the approved master), Ryan / Lead.
**Stage:** technically verified only. Nobody has listened to these yet: the agent that made them can't hear.
Nicholas approves by ear.

**Use:** one processed master, picked from the `_radio` files, serves both **`O_Unknown_Dont`** (the anonymous opening)
and **`F_FutureWren_Dont`** (the recovered finale) through **`SharedSounds[Dont]`**. No other line uses it.
`F_FutureWren_FullWarning` is separate (docs/story/story-0708-actions.md).

## Files

| File | What it is |
|---|---|
| `VO_FutureWren_Dont_0N_dry.wav` (N = 1-5) | Dry take, no effects. Cut mid-word on the dash, 45 ms pre-roll, 48 kHz, mono, 24-bit |
| `VO_FutureWren_Dont_0N_radio.wav` | Radio candidate built from the dry take with the same number (chain below) |
| `raw/*.wav` | The untouched xAI TTS outputs the takes were cut from (48 kHz, 16-bit mono WAV straight from the API) |
| `tools/build_wren_dont_r1.py`, `tools/cut_detect.py` | Rebuild every file above from `raw/`. The output matches these files bit for bit (checked) |
| `build_log.json` | Per take: source file, detected onset, cut time, gains |
| `audition_r1_sheet.png` | Spectrogram and waveform of all ten files on one time scale |

**Recommended:** take **01** (`ara`, exclaimed prompt). Alternative: **04** (`liora`), which is calmer.

## Tool and model

- **Model:** xAI Text to Speech (Grok voice), run on **fal.ai**. Endpoint **`xai/tts/v1`**
  ([model page](https://fal.ai/models/xai/tts/v1)). Run on Ryan's fal account on 2026-11-13, ~21:00 EST.
- **Voices used:** `ara` (xAI: "warm and friendly"), `iris` ("friendly, upbeat, naturally charming"), `eve` ("energetic and
  upbeat", British accent) and `liora` ("calm, grounded, luminous"). Source: the [xAI TTS voice list](https://docs.x.ai/developers/model-capabilities/audio/text-to-speech).
  `zenith` was rejected because it's a male voice (median F0 158 Hz). `aurora` and `celeste` were rejected because the
  transcript showed the first word missing or extra words.
- **Common settings:** `language: "en"`, `output_format: {codec: "wav", sample_rate: 48000}`, all other inputs at their
  defaults (safety checker on, no speech tags). No seed parameter exists, so a rerun won't produce the same audio.
  The `raw/` files are the record.
- **Cost:** 16 generations, about 320 characters in total, at $0.015 per 1,000 characters, so under $0.01. Speech-to-text
  checks ran locally (faster-whisper `small.en`) at no cost.

| Take | Voice | Exact `text` sent | Raw file | Cut at (s into raw) |
|---|---|---|---|---|
| 01 | `ara` | `Don't! Don't do it!` | `raw/b2_1_ara_E.wav` | 0.330 |
| 02 | `iris` | `Don't! Don't do it!` | `raw/b2_3_iris_E.wav` | 0.279 |
| 03 | `eve` | `Don't! Don't do it!` | `raw/b2_6_eve_E.wav` | 0.348 |
| 04 | `liora` | `Don't— don't go in there!` | `raw/b1_6_liora.wav` | 0.359 |
| 05 | `ara` | `Don't— don't go in there!` | `raw/b1_0_ara.wav` | 0.343 |

**Why a longer prompt and then a cut:** a bare `Don't—` comes out as a finished, falling statement ("Don't."), which is
the wrong performance. An exclaimed or continuing line makes the model start the word with urgency, as a warning with more to
say. Only the first syllable is kept. The other words never reach the take.

## Edit (dry takes)

1. Find the first syllable on a 2.5 ms RMS envelope: its peak, the point where the vowel drops 9 dB into the /n/, and the
   end of voicing (30 dB below the peak).
2. **Cut halfway through the /n/,** before the /t/ release, so it reads "Don—" with the voice still sounding. The level in
   the last 15 ms before the cut is only 6-12 dB below the word's average, so the cut is heard, not a natural fade.
3. 4 ms cos² fade-out at the cut (no click), 3 ms fade-in. The leading silence is trimmed or padded to 45 ms before the
   onset (the /d/ voice bar). DC is removed.
4. One linear gain to −19 LUFS (the project's `vo` target). 24-bit WAV.

## Radio chain (radio candidates)

Built from each normalised dry take (`tools/build_wren_dont_r1.py`, numpy and scipy, seeded by take number):
1. Band-limit: 4th-order Butterworth high-pass at 300 Hz, then 6th-order Butterworth low-pass at 3.4 kHz.
2. Light saturation: `tanh(2.2·x)/tanh(2.2)` on the peak-normalised signal, then the 3.4 kHz low-pass again to remove
   the harmonics it adds.
3. Carrier hiss under the voice: band-limited noise at −38 dBFS.
4. **Static tail at the cut:** 200 ms of band-limited noise starting at the cut sample. 2 ms attack, starts 12 dB
   below the voice peak, decays with a 60 ms time constant. 14 sparse crackle impulses are scattered over it.
5. 250 Hz high-pass and mean removal (DC), a 12 ms fade at the end, one gain to −19 LUFS.

## Levels (`audio_report.py --category vo`: all 10 PASS)

Loudness is K-weighted and ungated (the files are under 3 s). TP is true peak.

| File | Duration s | Loudness LUFS | True peak dBTP | Leading silence ms |
|---|---|---|---|---|
| 01_dry | 0.358 | −19.0 | −6.3 | 46 |
| 01_radio | 0.558 | −18.7 | −4.2 | 0 (hiss) |
| 02_dry | 0.286 | −19.0 | −6.5 | 46 |
| 02_radio | 0.486 | −19.0 | −6.1 | 0 |
| 03_dry | 0.318 | −19.0 | −9.7 | 49 |
| 03_radio | 0.518 | −18.9 | −6.5 | 0 |
| 04_dry | 0.351 | −19.0 | −6.0 | 48 |
| 04_radio | 0.551 | −18.9 | −5.8 | 0 |
| 05_dry | 0.293 | −19.0 | −5.7 | 45 |
| 05_radio | 0.493 | −18.8 | −4.1 | 0 |

## Checks the agent can make without listening

- **Speech-to-text** (faster-whisper `small.en`, local) on every final file: each one is transcribed as the single word
  "Don't" and nothing else. One exception: `05_dry`, where the transcript came back empty (too short for the model). Its
  radio version is transcribed as "Don't". The model fills in the "t" it expects, so this confirms the word and that no
  other words are present. It can't confirm the cut: the cut is shown by the cut-level figures above and the spectrogram sheet.
- **Pitch** (median F0): 01 311 Hz, 02 331 Hz, 03 294 Hz, 04 253 Hz, 05 314 Hz. All are in the female range. The
  higher values are the more urgent, exclaimed readings.
- **Not verifiable without ears:** whether any take sounds human rather than synthetic. **Nicholas's listen decides.**
  If none passes, the brief's fallback applies (ElevenLabs, done by Nick's side).

## Usage terms for a commercial game jam entry

Read on 2026-11-13. Ryan's account calls xAI's model through fal, so **fal's terms govern the account**, and xAI's terms
govern the model.

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
   performer. This matters for the jam credits, `CREDITS.md`, and any "cast" line.
2. **Don't use it to train other models.**
3. **The voice is a stock xAI voice and isn't exclusive.** Other xAI users can produce the same timbre.

**What is unclear:**
- fal's terms say nothing explicit about output ownership.
- xAI's enterprise terms bind xAI's direct customers. For a call through fal, they apply through fal's partnership,
  which isn't published.
- Neither document addresses game jams specifically.

I'd call it **clear enough for a jam entry**, but it isn't a written licence naming us. If the jam's rules ask for proof
of a licence, ask fal support for a short written confirmation (support@fal.ai).
