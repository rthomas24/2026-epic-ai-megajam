# Dominion boarding music

[Listen to the end of the boarding score and the queued credits entry](boarding-end-and-credits.flac). This 26-second lossless excerpt starts 68 seconds into the original Windows game recording. The first end request arrives about two seconds into the excerpt. The credits request follows two seconds later; Cathedral Pulse takes over near the 22-second mark.

The real Dominion interior map ran with rendering disabled and the Windows stereo mixer enabled. A private Development fixture invoked the production music component in `844064f1`. The same boarding component played through the encounter changes and the end fade. A second end request and a combat-state change during the fade did not restart the countdown. The last sample showing boarding ownership was 19.752 seconds after the first end request; the first showing the credits voice and released boarding ownership was 20.166 seconds after it. These are sampled bounds.

The recorder captured 109.98 seconds at 48 kHz stereo and reported zero discontinuities. The decoded excerpt matches the source interval sample for sample. Full recording SHA-256: `975e955279c1a930ba7d70a9da5f593c26bf5d492436cc44a4cd704272633c84`.

This audition covers the music player on its intended map. The encounter and end events were supplied by the fixture. Campaign-driven entry, a fade across actual map travel, the Shipping mix and Nicholas’s listening decision remain open.
