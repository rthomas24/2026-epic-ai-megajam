# Music loop listening check

Both eight-second excerpts come from the Windows mixer while the production music player was running on the Verdance map. The wrap is four seconds into each clip.

- [Steady Verdance loop](verdance-loop-steady.flac)
- [Verdance loop during the takeoff fade](verdance-loop-during-fade.flac)

The original theme ended and restarted through a game-thread callback. Matching the source to the earlier capture found a 32 ms break across the wrap. The decoded original also went directly from +2233 to −148 in the right channel (16-bit samples).

The revised SoundWave loops inside the audio renderer. Four-millisecond raised-cosine ramps bring both stereo channels to zero at the join. The track still contains all 9,100,800 frames at 48 kHz. Every source sample outside the two edge ramps is unchanged. A fresh Unreal export verified the endpoints, original level, pitch, Music class and full duration. The exported samples differed from the prepared master by at most one 16-bit unit.

Independent source matches on either side of each recorded wrap identified the same output sample, both at steady level and during the six-second diagnostic fade. The adjacent left/right sample changes in the mixed recording were +6/−2 and −1/−3. Both were smaller than ordinary changes in the surrounding 50 ms. Playback released after the fade. The complete 396.89-second game recording had zero reported capture discontinuities. SHA-256 is `5123e1a79c44d0f75c459866a440f1942a93d8ee470accb39e6dae431c5e8581`. These checks used a staged arrival and departure on the actual map.

The [PR 11 menu audition](menu-pr11-loop.flac) wraps four seconds into the excerpt. Both sides aligned at 107.065 s in the full game recording, with a single 16-bit unit of change at the join. This test used PR 11's native loop. Main still has the non-looping Wave and a callback that restarts it. Nicholas's listening approval for PR 11 is pending.

Earlier game recordings also showed sample-aligned joins for the tutorial's return to 0:38 at 105.488 s and two Dominion boarding wraps. The tutorial's step at the join matches the original downbeat exactly. None of those recordings exhibited the Verdance restart gap. The other score paths still need repeat-boundary listening in the matched build, including quiet space flight, combat and assault.

## Remaining full-song repeats: source check, November 13

Audio checked the prepared combat, idle-space and assault masters against their existing receipts. Lead independently rechecked all three SHA-256 hashes. No source was changed and no game was launched for this check.

| Track | Full duration | Maximum last-to-first sample step | Quiet span around the join, 10 ms RMS below -60 dBFS |
|---|---:|---:|---:|
| Ethereal Pulse Remix (combat) | 139.200 s | Zero in both channels | 0.14 s |
| Ambient1 (space idle) | 144.800 s | -63.46 dBFS | 0.60 s |
| Push The Attack (assault) | 144.680 s | -66.23 dBFS | 1.28 s |

The sample steps do not establish a significant source discontinuity. The quiet spans are threshold measurements of the songs' natural endings and openings, not measured engine gaps or claims about perceived silence. Another endpoint fade would not remove these musical pauses.

Preserve the complete masters. The next test must capture a natural repeat and exit under the actual gameplay owner, then compare the output with these source boundaries. Existing combat footage does not reach its 139.2-second wrap; idle and assault import/reload checks do not prove continuous playback. An overlapping return would change the arrangement and needs a separate listening review.

Verified master hashes:
- Combat: a705d8a00948cbf96bb4bbd33911cffa35e1cad102c7abaa1a839739df2a44a5
- Idle: 3474e84a97dea20125ccdb5b06c6356ea4aea30cd40156149a2f74061d3d2ab6
- Assault: aebd45f1302afbe2069936a35f95160797f87e66ae72147753e2f15ae378bcdd

Detailed local measurements and exact six-second tail/head diagnostic copies remain under the Audio worktree's Saved/AudioMusic/RemainingLoopReview01. These are source diagnostics, not gameplay recordings or human sound approval.

## Combat: one natural in-game repeat recorded

[Hear the actual combat repeat](combat-loop-native.flac). This lossless 14-second excerpt has its inferred wrap seven seconds in; decoded PCM matches the original recording exactly.

The existing private 28f03b4f host ran L_Galaxy with its real MusicDirector. Wave 3 was entered with a debug command; the 139.2-second song then repeated naturally, with only one music-start log and the same audio component. No direct Play or restart was injected. Debug travel to the main menu after 160 seconds left only MenuMusic active in subsequent component enumerations.

Late first-cycle source matches and second-cycle matches give a local repeat period of **139.20000 seconds**, with no additional loop-specific timing step resolved at 0.25 ms precision. This is timing evidence, not approval of musical continuity or perceived seamlessness.

The run was **not glitch-free**: an earlier 42.75 ms alignment shift occurred between first-cycle source positions 40 and 44 seconds. An AudioMixer underrun was logged at 03:41:53.181 UTC, about 39.95 seconds into the capture, consistent with that interval. Another underrun preceded recording. Zero recorder discontinuities does not exclude game-mixer starvation. Preserve the master; an endpoint fade cannot repair this earlier runtime interruption.

The 179.980-second recording had no clipping (-6.5 dBTP), and the process exited 0 with all 2,457 protected files unchanged. Minimum free RAM was 2.827 GiB. The original WAV SHA-256 is 1f771237af956e81c992ad8e2fa8cd29caf1cf086b874fc60ab12efff0882ca1. [Playback measurements](combat-repeat-qa.json) and [additional pre-wrap source matches](combat-repeat-prewrap-timing.json) retain the timing evidence.

This older compatible host does not establish batch-L exclusive ownership, idle/assault playback, natural campaign transitions, Shipping behavior or visual acceptance. The next matched build should repeat the check with lighter instrumentation and investigate mixer starvation separately.
