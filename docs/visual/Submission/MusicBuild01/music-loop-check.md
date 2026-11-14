# Music loop listening check

Both eight-second excerpts come from the Windows mixer while the production music player was running on the Verdance map. The wrap is four seconds into each clip.

- [Steady Verdance loop](verdance-loop-steady.flac)
- [Verdance loop during the takeoff fade](verdance-loop-during-fade.flac)

The original theme ended and restarted through a game-thread callback. Matching the source to the earlier capture found a 32 ms break across the wrap. The decoded original also went directly from +2233 to −148 in the right channel (16-bit samples).

The revised SoundWave loops inside the audio renderer. Four-millisecond raised-cosine ramps bring both stereo channels to zero at the join. The track still contains all 9,100,800 frames at 48 kHz. Every source sample outside the two edge ramps is unchanged. A fresh Unreal export verified the endpoints, original level, pitch, Music class and full duration. The exported samples differed from the prepared master by at most one 16-bit unit.

Independent source matches on either side of each recorded wrap identified the same output sample, both at steady level and during the six-second diagnostic fade. The adjacent left/right sample changes in the mixed recording were +6/−2 and −1/−3. Both were smaller than ordinary changes in the surrounding 50 ms. Playback released after the fade. The complete 396.89-second game recording had zero reported capture discontinuities. SHA-256 is `5123e1a79c44d0f75c459866a440f1942a93d8ee470accb39e6dae431c5e8581`. These checks used a staged arrival and departure on the actual map.

The [PR 11 menu audition](menu-pr11-loop.flac) wraps four seconds into the excerpt. Both sides aligned at 107.065 s in the full game recording, with a single 16-bit unit of change at the join. This test used PR 11's native loop. Main still has the non-looping Wave and a callback that restarts it. Nicholas's listening approval for PR 11 is pending.

Earlier game recordings also showed sample-aligned joins for the tutorial's return to 0:38 at 105.488 s and two Dominion boarding wraps. The tutorial's step at the join matches the original downbeat exactly. None of those recordings exhibited the Verdance restart gap. The other score paths still need repeat-boundary listening in the matched build, including quiet space flight, combat and assault.
