# Credits playlist in the standalone game

The 13-second [listening excerpt](credits-handoff.flac) starts 85 seconds into a recording of the game process. At about 5.3 seconds in the excerpt, Cathedral Pulse (1) gives way to Ethereal Pulse (Remix). The excerpt decodes to exactly the same PCM samples as the corresponding interval of the capture. No music was added in editing.

The Development game ran on L_MainMenu using the three player files from `78caa422`. A private console fixture stopped the menu music and invoked the actual credits player. At five seconds, a duplicate start kept the original component. Following the first track’s natural completion, the player started the second track once. Stopping during the second track cleared the component. Starting again created a new component for track one. The final stop cleared it. Component snapshots showed one playing voice during playback and zero after stop.

A waveform check compared 2.5-second windows against both 48 kHz stereo source masters, downmixed to mono at 500 Hz for alignment. Cathedral Pulse matched at 4.228 and 42.228 seconds (correlations 0.9992 and 0.9997); Ethereal Pulse matched at 92.342 and 95.342 seconds (0.9036 and 0.8630). The capture ran for 109.96 seconds with zero reported discontinuities. Full capture SHA-256: `4b4228371206af7ed0afdc19df62571005d4233c273f2cbcbbbed54502db16f90`.

The recording verifies the credits component and its audible order. Natural victory entry after the stinger, return to menu, completion of the full second track, Shipping playback, and final mix approval remain open.
