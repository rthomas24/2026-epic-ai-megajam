# Rift / Veil 01 — new direction prototype

**Agent:** Audio & Music Agent  
**Role:** Cinematic sound design and music  
**Task:** One new Rift spatial-shimmer concept for approval

Open `audition/index.html` or play
`audition/SW_TD_Rift_Veil_Concept01.wav` directly. Exactly one new designed WAV,
1.42 seconds, 48 kHz, PCM24 stereo. M02/M03 remain unchanged for comparison.

## New concept and source

This is a sample-led glass-resonance veil, not a new version of the previous
modal/pressure design. It imports no M02/M03 generation code or sound layers.
There are no synthesized oscillators, impacts, bass drops, friction grains,
added noise or distortion. The retained published glass sample supplies all
timbral content. The author describes it as “Shimmering glass”; we do not infer
that the underlying source was an unprocessed field recording.

Source: **Shimmer glass**, **Dasgoat**, Freesound sound **361704**:
https://freesound.org/people/Dasgoat/sounds/361704/ . The original page's **Creative
Commons 0** label and CC0 deed link were checked before acquisition. License:
https://creativecommons.org/publicdomain/zero/1.0/ . Attribution is retained
voluntarily. `source-license.json` records source/page hashes, exact public media
URL, author, acquisition timestamp and license evidence. Retained source is
`source/Dasgoat_ShimmerGlass_HQ.mp3` (168,462 bytes).

The publicly linked HQ MP3 preview was acquired. The original downloadable file
requires login and was not accessed. No login, access-control bypass, payment,
subscription or large pack was used.
The 24-bit/48 kHz output is a delivery format; conversion does not restore
information lost in the MP3. Before any final game acceptance, this source
quality is a limitation to consider alongside the concept's character.

## Edit decisions

- The source has roughly half a second of lead-in; use the sustained material
  at 0.72 seconds so playback begins promptly without the original attack.
- Slower source playback with a small continuous downward drift makes the
  glass texture less literal, without introducing a sequence of new notes.
- Gentle causal high-/low-pass filtering removes low clutter and bright edges.
- Reduce excessive source width, then add quiet 21/31 ms crossed sample
  reflections and a slight smooth stereo drift. No external impulse response.
- Smooth 8–10 ms entrance, an exponential body and a long smooth release shape
  one gesture. The final 25 ms are exact silence after an inaudible taper.
- Set approximately -9 dB interpolated peak headroom, rather than the louder
  M02/M03 target. No limiter, saturation or aggressive loudness maximization.

The intended identity is subtle and mysterious. This is a prototype for a
direction decision, not a claim of realistic/AAA sound or an approved effect.

## Validation / stages

`verification.json` measures the actual exported PCM after writing: format,
clipping, onset, DC, waveform endpoints, quiet tail, correlation and a guarded
4x FFT peak estimate. It also measures eight unrestricted overlapping repeats
400 ms apart. No repeat-test WAV is exported. M02 and M03 reference hashes are
retained in the report and checked against their prior records before handoff.

Prototype complete and technical export checks passed. Subjective audition,
naturalness, timbral identity, comfort when repeated and small-speaker translation
remain for Nicholas. Browser playback was not exercised for this new page; it
uses standard native WAV controls with local links and no scripting. No hearing
claim is made. The earlier M03 page was opened visually, but its control test was
inconclusive and local navigation was blocked by in-app browser policy; no bypass.

Not manually approved, integrated or gameplay validated. No Unreal assets,
gameplay, UI, configuration, audio routing or native import changed. Ryan confirmed
gameplay plays SwitchIn/SwitchOut at the cut; never also fire widget playback.
That eventual integration remains outside this prototype.

## Reproduce / coordinate

Run `render_concept.py` with Python 3.13.9 + NumPy 2.3.4. It decodes only the
retained source using existing `C:/Program Files/Shotcut/ffmpeg.exe` (8.0.1),
through a captured float32 pipe; it makes no network calls. Installed Blender's
standalone Python interpreter is used, but Blender and Shotcut UI are not opened.
Original NumPy code shapes the sample; libraries/tools are not redistributed.

Lead dispatch/main claim `85119edad6c3853f862aecfc4fd339926e8d1a55` authorizes
only `ArtSource/Audio/StarterPlan/RiftConcept01/**` on existing `nick/audio-music`
in `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-music`. M02 is frozen
at `5979cff`; uncommitted M03 is preserved/frozen, superseded by this one-cue task.
Lead handles claim/board updates, serialized Git preflight and branch publication.
No main merge, further ship cues or combat/music before Nicholas approval.

Next step: listen to **Veil 01** and decide whether to adopt this quieter,
sample-led direction before producing a new Lancer or Bastion concept.
