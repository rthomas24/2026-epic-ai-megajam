# Triad Drift — switch audio M03

**Agent:** Audio & Music Agent  
**Role:** Cinematic sound design and music  
**Task:** Refine all three switches after M02 roughness/transition feedback

## Review files

Open `audition/index.html`, choose **Load six WAVs**, then use the ship buttons,
**A then B**, or **Rapid M02 / Rapid M03**. The standalone page embeds exact
byte copies of the original six WAVs, with hashes, for offline playback without
a server or `file://` fetch restrictions. It generates no oscillators. M02
original files remain in the sibling `SwitchM02/audition` folder, unchanged.
Exactly three new WAV originals are exported under this revision:

| File | Duration |
|---|---:|
| `audition/SW_TD_Rift_Switch_M03.wav` | 1.72 s |
| `audition/SW_TD_Lancer_Switch_M03.wav` | 1.46 s |
| `audition/SW_TD_Bastion_Switch_M03.wav` | 1.88 s |

All are PCM24, 48 kHz, stereo. The page starts at a -6 dB preview level, with
measured loudness matching enabled for fair comparison. Matching uses a common
short-SFX K-weighted reference; it is not a promise of perceived equality.

## What changed

M02's file endpoints alone did not establish natural layer transitions. This
revision redesigns the events instead of applying only a long fade to M02.

- Every contact, energy body and pressure layer receives smooth polynomial
  attack/release envelopes with continuous first and second derivatives.
- Rounded pressure contacts replace bright scratch/grain accents. Ringing modes
  are quieter and damped; secondary contacts sit inside the same event envelope.
- Saturation is removed completely. Broad upper-mid EQ reduction and two gentle
  low-pass stages reduce grit while preserving the lower mechanical transient.
- Causal finishing filters avoid circular FFT wraparound/pre-ringing on the
  finished file. Noise coloration remains an original procedural source process.
- A final 6 ms attack preserves fast activation. A 430 ms end taper leads into a
  20 ms exact-silence guard after the remaining signal is below the noise floor.
- Rift uses crossfaded pressure spectra and modest stereo movement. Lancer uses
  rounded armature contacts and a continuous turbine-pressure rise. Bastion
  blends armored contacts into a damped low body, with subdued upper resonances.
- Dark low-level diffusion replaces prominent isolated reflections. No external
  recordings, impulse responses, sound libraries or AI audio services were used.

These are original procedural designs aimed at smoother cinematic character.
They must be judged by listening; technical improvements alone cannot establish
realism, naturalness or AAA quality. A possible tradeoff is reduced brightness
and small-speaker presence. Compare with matched levels before approving.

## Verified checks and listening limitations

`verification.json` is measured from the actual decoded PCM bytes and compares
both revisions. It includes headers, hashes, onset, opening sample steps, tail
levels, DC, peak estimates, clipping, mono correlation and spectral energy.
M02 hashes are rechecked after rendering. Three M03 files only.

The rapid stress test covers all six ship-order permutations repeated four
times, at 0.4 s spacing, with both unrestricted natural tails and the preview's
older-voice replacement fade. Twelve scenarios contain twelve switches each.
The exact-sample mixes remain below clipping at unity; preview output is lower.
No stress WAVs are exported or substituted for the three deliverables.

`player-qa.json` records syntax, exact embedded WAV bytes/hashes, local links
and scheduling checks in a Node VM with mocked audio nodes. This is not actual
browser audio decoding. The page was opened and visually inspected in Chrome;
browser UI input did not reliably activate playback, and the in-app browser's
security policy blocked local-file navigation. No bypass was attempted.
Actual headphone/speaker audition, detection of unpleasant transitions by ear,
hardware output and subjective quality acceptance remain for Nicholas. No
Unreal import, PIE, concurrency implementation or gameplay-mix test was done.

Peak estimates use guarded 4x FFT interpolation, not a certified true-peak
meter. Loudness uses the existing 48 kHz BS.1770 K-weighting/gating helper; short
transients are unsuitable for treating that value as a long-program target.
The report describes opening sample-step reductions as a technical measure,
not proof of perceptual smoothness. Final tail dither remains below audibility.

## Reproduction and provenance

Python 3.13.9 + NumPy 2.3.4, using the existing standalone interpreter bundled
with Blender 5.1. Blender is never launched. Set single-thread numerical runtime
variables if needed, then run `render_m03.py` and `build_preview.py`. Run
`validate_player.cjs` with the installed Node runtime for player checks.

The renderer imports helper functions from immutable M02 `render_switches.py`
without calling its main routine; bytecode writing is disabled before import.
It writes only M03 outputs. Seed 261113. No downloads, paid credits or third-party
audio content. See `credits.json`; project-created assets remain governed by
the team's project licensing policy. Tools: OpenAI Codex, Python, NumPy, Node.js.

## Ownership / integration boundary

Lead task preflight and main claim `aac996bf4e1e4cb216d8a8d6486d3744c3456fee`:
only `ArtSource/Audio/StarterPlan/SwitchM03/**`, existing `nick/audio-music` at
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-music`.
M02 is read-only at `5979cff`, and remains unapproved/rejected for roughness.
Lead alone handles claims, MESSAGEBOARD, remote synchronization and publication.
Request serialized precommit audit; no independent fetch/merge/rebase/push.

Ryan confirmed gameplay plays `UTDShipVisuals.SwitchIn` / `SwitchOut` at the cut.
Do not also play audio from a widget listening to `OnSwitchCut(FTDSwitchCut)`.
No gameplay, UI, shared configuration, native data asset or main integration
change is part of M03. The preview's 60 ms older-tail fade is an audition policy,
not a runtime implementation. Full combat/music work waits for approval.

Next step: Nicholas compares each pair and both rapid sequences on headphones
and speakers, then approves M03 or supplies notes on weight, brightness, identity
and transition fatigue. Preserve both versions until that decision.
