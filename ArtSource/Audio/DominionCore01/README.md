# Cathedral Pulse — original core-chamber selection

Nicholas supplied the original **Cathedral Pulse.mp3** and selected it for the core chamber, with a possible loop to be decided later. This is distinct from **Cathedral Pulse (1)**, the longer track already imported for credits. Do not substitute one for the other.

## Files and preparation

- `source/Cathedral Pulse.mp3`: unchanged supplied original, 1,648,672 bytes. SHA-256 `4bec6bc4b5921ff65449701ddfb2560ed15e2b100f9c22798ad252bacc49307f`.
- `export/MUS_DominionCore_CathedralPulse_Full_01.wav`: complete 69.560-second, 3,338,880-frame stereo 48 kHz PCM16 import master. Only decoding/format conversion with triangular dither; unity gain, no trim, loop, rearrangement or crossfade. The embedded cover image is excluded from the audio master.
- `manifest.json`: original/master hashes, PCM hash, processing and pending gates.
- `quality-report.json`: the project's music-category check passes: -14.7 integrated LUFS, -2.7 dBTP, no clipping, DC 0.000203, no failed criteria. This is a file measurement, not an in-game mix or listening approval.
- `prepare.py`: reproducible preparation using the supplied source and an explicit FFmpeg executable. Refuses to overwrite earlier outputs.

Source receipt: Nicholas's local `C:/Users/nicho/OneDrive/Desktop/GameJam/Muisc/Cathedral Pulse.mp3`. Original source and complete musical duration are preserved. Preparation used Python 3 and FFmpeg; no new music generation or third-party acquisition.

## Handoff

Stage: **native import and fresh reload verified** in the compatible private 28f03b4f host. The package is `/Game/TriadDrift/Audio/DominionCore01/MUS_DominionCore_CathedralPulse_Full_01`, SHA-256 `e43a39a8c3a49480779da9c38728ff3dd8fc6b48bb1078d1c92436a7ba4229a4`. It uses the existing Music sound class, unity asset volume/pitch, no loop, and PLAY_WHEN_SILENT virtualization. Export after import and after a fresh process reload preserved all frames; maximum PCM difference is one 16-bit rounding step, and both exports match exactly.

`native-checks/` contains the two data checks and process guards: both exited 0, no protected files changed, and no unexpected Content files appeared. Minimum free RAM was 2.883 GiB for import and 3.297 GiB for reload. This verifies native data only, not playback. Establish the core scene's entry/exit events with Ryan's lead; looping remains undecided. No map/style/default is activated and no cook, Shipping or gameplay listening approval is claimed.
