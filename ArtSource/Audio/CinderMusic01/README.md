# Cinder music — confirmed source selections

Nicholas explicitly assigned **Cinder World** to exploration and **Cinder Fall** to combat. The complete original MP3s are preserved under source/, with catalog hashes and the full48kHz/stereo16-bit import masters under export/.

| Track | Role | Full duration | Integrated loudness | True peak | Applied gain |
|---|---|---:|---:|---:|---:|
| Cinder World | Exploration |150.000s|−14.4LUFS|−2.2dBTP|0dB|
| Cinder Fall | Combat |148.800s|−15.0LUFS|−2.3dBTP|0dB|

Both pass the project's music category checks with no clipping. No normalization was needed. A separate complete source decode matched every output PCM sample; full frame counts are7,200,000 and7,142,400. No trimming, fades, looping, crossfades, pitch or tempo changes were applied to either master. Source MP3s remain byte-identical.

Nick additionally notes the slow openings and suggests skipping forward to the combat beat drop. **No exact start offset has been approved or applied.** Private energy/onset diagnostics and any listening excerpt are review evidence only; they do not define gameplay timing or a loop edit. Keep Cinder World's exploration opening intact unless Nicholas changes that direction.

Provenance: supplied by Nicholas for Triad Drift; no external pack acquisition, purchase or generation during preparation. Original attachment paths were in Downloads; actual originals were found in C:/Users/nicho/OneDrive/Desktop/GameJam/Muisc/. Private backup copies are also preserved. Exact source/master hashes, original paths and full measured QA are in source_manifest.json. Lead owns the final shared music map/credits record.

## Native import and fresh reload

Both full masters are now imported as Unreal 5.9 SoundWaves under `/Game/TriadDrift/Audio/CinderMusic01/`:

| Asset | Package SHA-256 |
|---|---|
| `MUS_Cinder_Exploration_World_Full_01` | `07a595ccc27801752292c079c7531893a024dda58365ccb30716e8c6bafe235d` |
| `MUS_Cinder_Combat_Fall_Full_01` | `076a5f2984308046acf490cc3d9600d71c9fe0ba8fef5f5f657271e68eb1d66c` |

The isolated compatible host was `lead-music-build-1113` at base `28f03b4f`; claim `5b79e474` authorized the operation. Import PID 30488 and independent reload PID 14532 both exited 0 with engine result `ok` and zero errors. Minimum free RAM was 1.839 GiB and 2.469 GiB respectively. All 2,459 pre-import protected files and 2,461 pre-reload protected files were unchanged. Only the two expected packages were created. Small evidence records are in [native-checks](native-checks/).

Both exports retained full frame counts, stereo 48 kHz, matched import versus reload byte-for-byte, and differed from the approved masters by at most one 16-bit rounding step. Package hashes were verified again after transfer. Both use the existing Music sound class, unity volume/pitch, no looping and PLAY_WHEN_SILENT virtualization. Unity asset volume is not proof of balance against older assets at 0.5; the eventual component gain and final mix need explicit playback checks.

**Stage: native data technically verified only.** No Cinder event hookup, start offset, loop edit, Presentation default change, gameplay playback, cook or listening acceptance is claimed. Full originals and earlier source evidence remain unchanged. T1/T5 own the N-089 event contract; Audio owns the presentation hookup once the matching host is available.
