# Actual Shipping gameplay — 6fe3e90f

[Watch or download the 3:20 gameplay recording](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/nick-gameplay-demo-6fe3e90f-1114).

This is the released Windows Shipping build `6fe3e90f`, not newer branch work. The source package was verified against its exact 938,981,901-byte size and SHA-256 `45c2d4e010c02e3000f47f5cc80ff0c911a8d79b03243136b4b6d890e6ba6fa6` before isolated extraction. The recording shows the main menu, opening, control handoff, ship changes and combat into wave 2. A later unrecorded observation reached the Jump to Verdance/Aftermath presentation; that is not proof of recorded exit timing.

This requested demonstration is separate from the eventual 30–60-second submission trailer; that trailer remains to be edited with gameplay in its first five seconds.

## What is and is not verified

- The corrected recording contains actual changing game frames and same-process game audio. Sample frames were inspected at the opening, flight and late combat. The full video decoded successfully during review-frame extraction. No soundtrack was substituted and no presentation candidate or gameplay overlay was activated.
- New Game was started through OS-level automated input. Later external input was detected, so further automated flight inputs were withheld. Do not use this demonstration to certify a physical keyboard/controller test or a fully instrumented agent playtest.
- The game used an isolated `-UserDir`; its settings and three save files are under the private Run02/User/Saved directory. All pre-existing extracted build files retained their hashes. Minimum free RAM was 3.500 GiB. The game was stopped by the bounded owned-process cleanup (exit code 1), not reported as a natural clean exit. No Unreal processes remain.
- This is a gameplay demonstration, **not final visual/audio acceptance, a full campaign playtest, or verification of the requested 0:38–1:35 tutorial loop and complete 20-second exit fade**. Those tests remain open under #24/N-039.
- Batch M's music bridge, designed cards/reward mount, crew-death changes, later Cinder imports and rebuilt ships are absent from this released revision. Keep Ryan's [package limitations](https://github.com/rthomas24/2026-epic-ai-megajam/releases/tag/playable-main-6fe3e90f) attached to the evidence.

## Capture integrity

The first HWND-only GDI video take returned a stale menu surface despite the live game progressing. It was rejected and retained privately as Run01, never presented as gameplay video. Run02 captured the visible foreground game client rectangle only, with an ongoing focus/geometry guard; no desktop area outside the game was included. The 1280×720 preview has 5,745 video frames over 200 seconds (28.725 average captured frames/s; this is not a game-performance benchmark).

The original stereo 48 kHz PCM recording contains 199.91 seconds, peak -6.428 dBFS and RMS -25.945 dBFS. The recorder reported zero discontinuity flags. Packet timestamps nevertheless include a maximum 70 ms gap; preserve that limitation rather than claiming sample-accurate synchronization or a clean-loop measurement. Initial video/audio clocks were aligned with an audio offset of -0.013531 seconds. AAC compression is for the review copy; raw video, PCM, packet timestamps and logs remain preserved privately.

Review file: `TriadDrift_6fe3e90f_review.mp4`, 94,871,398 bytes, SHA-256 `7e7e7a8cffd6b08939a1b60fd3606ded422eea3d68fa66c9e061f256c905d73b`.

Evidence: [media measurements](media-verification.json), [native guard result](native-result.json), [flight frame](flight.png).

Independent Story review: [timestamped caption findings](caption-review.json). Five legacy Intro_N captions are visible in the opening; this confirms a presentation-route fidelity gap against the approved second draft. Known older combat wording is separated from new regressions. The finding was delivered to T3 through N-088 at 091ac013; broader narrative, voice performance and later-act acceptance remain unverified.

Next: consume the matching batch M bridge/capture package, verify music handoffs and full tutorial fade, and review the designed UI candidates in actual gameplay. Nicholas retains final look/sound approval.
