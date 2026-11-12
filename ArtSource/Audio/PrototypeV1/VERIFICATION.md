# Audio Prototype V1 — verification record

Agent: Audio & Music. Task: Audio Prototype V1. Current AAA stage: Verify (native assets/playback queued).

## Verified source checkpoint — November 12, 2026

- 14 event groups / 26 prepared WAV files: all category checks pass, 48 kHz / 16-bit PCM, no clipping, no silent files. Six repeating effect families have three small derived variations each; switches/UI remain stable signatures.
- All 14 source references retain their original SHA-256 hashes. No VaultCache or old audit files were edited. No purchases, downloads or generation services were used.
- Primary fire: approximately 0.314–0.321 seconds, −1 dBTP before the −6 dB runtime anchor gain. Variation loudness differs by 0.03 LU.
- Machinery/engine loop: 4.365 seconds, −23 LUFS, −10.3 dBTP, boundary step −54.2 dBFS, endpoint level difference 1.03 dB.
- Music: two 46-second excerpts, exploration −22.4 LUFS / −10 dBTP, combat −22.1 LUFS / −3 dBTP. Boundary steps −48.5 / −55.8 dBFS; endpoint level differences 0.27 / 0.53 dB. Combat's initial edit failed the endpoint-level check; moving its selection 0.75 seconds later resolved it without changing thresholds.
- Waveform/spectrogram sheets inspected for primary variations and machinery loop. This is visual and numerical verification, not a claim that an agent heard or artistically approved the sounds.
- Python source parses, all JSON parses, and tracked diff whitespace checks pass. The private native source template is staged in ignored storage.

## Native build checkpoint — November 12, 2026, 15:55 EST

Lead granted a bounded exclusive 8025 window with a fresh 4.5 GiB free-RAM/no-heavy-process guard, one compiler action and `-NoUBA`. Both targets compiled successfully, actual OS exit code 0: `TDAudioV1Editor` in 67.97 seconds (8 actions), `TDAudioV1` in 50.77 seconds (11 actions). Every action reports `[NoUba]`; UBT's generic executor banner still says Unreal Build Accelerator local executor. No compiler errors. Binaries remain in the ignored private host.

The next fresh pre-editor guard found 4.4186 GiB free, zero heavy processes. The launcher stopped before starting Unreal. This was a resource hold, not an engine crash. Lead was informed and the slot released; no automatic retry or native authoring occurred.

Lead renewed phase 2 after a read-only check showed about 4.89 GiB free. The immediate launch guard then found 4.2478 GiB / zero heavy processes and again stopped before creating any Unreal process. The slot was released and a stable resource handoff requested. No native-authoring session has yet started.

Build command/result/log evidence: ignored `Saved/AudioMusic/PrototypeV1/TDAudioV1Editor_build{.log,_result.json}` and `TDAudioV1_build{.log,_result.json}`. Compact versioned results: `native_builds.json`.

## First editor session — failed before authoring, November 12, 2026, 16:08 EST

Lead explicitly lowered the NullRHI-only startup guard to 4.0 GiB, with no other heavy process and a ten-second resource monitor. A private monitor helper's initial import-path typo was corrected before Unreal started. The single actual host session started with 4.1393 GiB free, PID 24684, `-nullrhi -NonRealtimeAudio`, port 8025, a 600-second exit bound and 90-second hang bound. It reached its MCP toolsets without shader compilation or a memory-pressure stop.

Startup logged **two real errors**: `RendererSettings failed to find console variable fx.GPUSimulationTextureSizeX/Y`. No waves, MetaSounds or demo map were authored. The owned host was stopped through `ConsoleToolset.RunConsoleCommand(QUIT_EDITOR)` as required by Lead's first-error rule. Actual OS exit 0, engine result `reason: ok`, 85.7 seconds engine duration, but **not a passing session** because the result records two errors. The ten-second monitor measured 1.9264 GiB peak working set and 2.1134 GiB minimum free RAM. Seven startup warnings are retained, including unused animation-compression defaults, initial missing Content directory, MCP's EULA notice and editor layout migration.

Static engine-source diagnosis located both missing CVars in stock Cascade's `ParticleSimulationGPUGlobals.cpp`. Lead approved enabling **only Cascade** in the isolated host template and creating its Content directory before launch. Those source changes are staged, untested, and need the Editor target receipt/build recheck before another separately granted authoring session. The earlier Game build predates this plugin change; no final monolithic or packaged acceptance is implied. No automatic native retry, game or rendered launch occurred. Failure logs/results/resource samples are preserved under ignored `Saved/AudioMusic/PrototypeV1/author_01/`; compact versioned evidence is `native_author_01.json`.

## Corrected editor session — native authoring and preview pass, November 12, 2026, 16:30 EST

After Nick freed RAM, the immediate Editor-build guard passed at 4.2011 GiB/no competing heavy process. The Cascade-corrected Editor target rebuilt successfully in 38.62 seconds, actual OS exit 0, five actions tagged `[NoUba]`, maximum one action. No Game/monolithic rebuild was performed.

The authoring host started at 5.5613 GiB free, PID 888, NullRHI/NonRealtimeAudio on 8025. It created 26 Sound Waves, 14 MetaSound Sources, 12 class/mix/attenuation/concurrency assets and the demo map. Unreal validated **53/53 assets, zero invalid, skipped or validation warnings**. Native inspection confirms all 14 source settings, four children under the master class, map existence and PlayWhenSilent on Engine/Explore/Combat. Imported-wave analysis reports 26 non-silent 48 kHz waves, maximum true peak −0.946 dBTP.

Native previews rendered five primary shots and engine Speed01=0/1. All captures are 48 kHz/16-bit stereo, non-silent and unclipped. Five measured shot bursts last 0.30–0.33 seconds, with 1.54 dB peak variation. Engine sustained RMS increases **5.008 dB** at full throttle. Preview true peaks: primary −15.0 dBTP, engine idle −33.3, engine full −28.3. These are editor-preview levels; game-mix acceptance is not implied. NonRealtimeAudio produced approximately four seconds of output per wall-clock second: captured durations 13.952 / 41.963 / 42.005 seconds. The native `loop_seam_jump_db` is normalized against adjacent-sample RMS (`AudioAnalysisMeasurements.cpp:453`), unlike the absolute dBFS source check; those metrics must not share a threshold.

The owned host exited through QUIT_EDITOR, **OS exit 0 / engine reason ok / zero errors**, 307.8 seconds engine duration. Six warnings remain: four unused stock ACL animation-compression-default warnings, MCP's EULA notice and the published checker's deprecated world-access call. No audio reference/validation warning, suppressed dialog, memory-pressure stop or shader compilation occurred. Peak working set **2.0873 GiB**, minimum free RAM **2.0114 GiB**, sampled every ten seconds. Prior failure `author_01` remains intact; full corrected evidence is archived in ignored `Saved/AudioMusic/PrototypeV1/author_02/`. Compact source evidence: `native_author_02.json`; updated Editor-build result: `native_builds.json`.

## Native acceptance gates

Remaining real-game checks are queued after UI's exclusive slot; a fresh Lead grant is required on 8025:

1. **Passed:** compile standalone Editor and Game targets, maximum one compiler action.
   **Passed:** Cascade-corrected Editor rebuild. Game build validation still predates that correction.
2. **Passed:** import/author 26 waves, 14 MetaSounds, 12 mix assets and the isolated demo map.
3. **Passed:** validate 53 assets and source bindings; analyze imported waves; render repeated primary fire and both engine parameter extremes.
4. Run the scripted in-game demonstration with the real mixer, capture its runtime output and HUD, inspect exit/result/logs and measure the output for silence/clipping/tails.
5. Exercise positional effects, bus controls, repeated playback and music transitions; record results and remaining limitations here.

Native offline preview is verified. Real-game playback, frame rate, packaged build, gameplay integration and human listening acceptance remain pending. Shared gameplay and configuration are unchanged. Native/raw licensed media stays ignored pending per-pack publication clearance. Nick's listening review remains the presentation gate.

## Evidence

Source metrics: `qa_*.json`, `music_loop_qa.json`, `preparation.log`. Exact original source paths/hashes and pack links: `sources.json`. Mapping and controls: `README.md`. License/credits handoff: `CREDITS_PROTOTYPE.md`.
