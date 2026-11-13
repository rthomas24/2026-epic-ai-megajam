# CombatMusic01 - Ethereal Pulse (Remix)

Nick supplied his own original AI-generated track and requested automatic music when gameplay enters combat, including ship battles and ground/melee fights. The unique new download is `C:/Users/nicho/Downloads/Ethereal Pulse (Remix).m4a`, 2,251,052 bytes, duration 139.2 seconds. Original and preserved-copy hashes are in source_receipt.json. Nick explicitly affirms ownership and game-use permission; no purchase or further license confirmation is required.

Current stage: source preparation/category QA, native import and fresh Wave reload passed. The exact verified Wave has been copied into the isolated audio feature worktree for publication. Automatic current-Core binding/playback, listening and Shipping remain pending. Lead published the exact source/one-wave claim at 79d3ab76508f6a69eb875977c09150e8539f296b and sent the consumer request to Core in MESSAGEBOARD N029. No Audio-owned Source, map, config or style change is included here.

## Sound and mix

- Imported test-host wave: `/Game/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01`.
- Reuses `/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music` unchanged so the existing Music volume contract covers it.
- Full track: 6,681,600 frames, 139.2 seconds, 48 kHz, 16-bit stereo. Linear -1.65 dB reduction and 5 ms/50 ms edge fades; no recomposition, tempo edit, dynamics compression or new generation.
- File QA: -18.0 LUFS integrated, -5.8 dBTP, 4.4 LU loudness range, no clipping, DC 0.000027, first/last samples zero. Sample boundaries are clean; no beat-perfect musical loop is claimed.
- Proposed native playback: looping true, pitch 1, SoundWave volume 0.5. This supplies the designer gain once; the runtime consumer should not accidentally apply it twice.

The most energetic five-second music window (130-135 s), at the planned gain, plus the existing Lancer stress mix with constant-power mono-to-stereo panning measured -17.7 LUFS and -4.6 dBTP with no clipping. This is an offline model, not a game mixer recording. The initial full-volume L/R mono-duplication model is preserved: -15.6 LUFS, -2.3 dBTP, no clipping, with a mix-loudness range flag. See both mix QA/receipt files; final native balancing/listening remains open.

## Integration and verification

Engineering has implemented `UTDUIStyle::CombatMusic` (dd6c36f341379e8ba20d4d197bb18cb6797b15cd) and `UTDMusicDirectorSubsystem` (dd595c8a13e2a3b0635b263a8ab16df86331aaa1). The actual director is world-scoped with one 2D UI-sound component per map, intentionally differing from the earlier proposed GameInstance design. Use this existing implementation; do not duplicate it.

The cue point is actual combat activation: an active space wave, Dominion assault, or an authored on-foot encounter's IsActive state. Code supplies 2-second fade-in, 6-second hold, 4-second fade-out, continued playback during a new fight in hold/fade, and a 1-second stop path for travel/victory/CUT. The sound's authored class/volume are used once, and an empty slot is silent. Audio will write ONLY CombatMusic after Root provisions a matching compiled host and publishes a serialized style handoff; retain MenuMusic and every other style field.

MusicIntensity01 alone is unsuitable because idle Ground already targets 0.3. The earlier core_handoff_draft.md and N029 archive retain proposal/contract history; actual published implementation is authoritative. No combat-binding or new-controller implementation is part of this Wave publication.

Verify import/reload references, real wave and encounter activation/clear, no duplicates, Music volume control, travel/retake/menu cleanup and an audible loop. Source QA does not establish those results, Shipping acceptance or subjective approval.

## Reproduction

`prepare.py` recreates the prepared WAV in ignored Saved storage from Nick's original. Existing outputs are verified rather than overwritten. `import_combat_music_02.py` validates the source by default; native mutation requires a separate Lead-granted host and SPEC.execute. It refuses an existing target and never saves the shared music class. `manifest_02.json` and READY_02.json pin the current source and final proposal; the earlier two-package proposal was withdrawn and never executed.

All original/prepared audio remains in private ignored storage. Existing Lancer, rejected audio history and menu-theme assets are preserved. No Main integration is included.

## Native import checkpoint

Import03 created exactly one 17,294,337-byte Wave in the Lead-granted day1-integration test host. OS exit 0, structured reason ok, zero engine errors/warnings/dialogs, 34.86 seconds, minimum free RAM 3.232 GiB. All 1,123 pre-existing protected files, including the shared Music class and menu/style assets, were unchanged. Wave SHA256: 1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa. See native_import_03.json. No branch copy, fresh reload or combat trigger playback is implied.

The final authoring runner is import_job_03.py, pinned with its reused guard in READY_03.json. The audio specification/importer02 is unchanged; runner03 adds owned-process exception cleanup and an explicit private port. Earlier runners are preserved rather than overwritten.

## Overnight held checkpoint (preserved history)

Lead reports Nick chose KEEP NATIVE JOBS QUEUED while available RAM is below the launch floor. The six-pin fresh-reload proposal is reviewed but has not run. The N029 contract is received, while engineering implementation and actual Audio binding remain unverified; no duplicate controller, style-field write, gameplay/map/config edit, further audio iteration or automatic native retry is included. The imported Wave remains only in the test host. Feature-package copy requires a future fresh-reload PASS and separate serialized Lead clearance; it has not occurred.

A fault-injection check of the exact runner monitor used only a harmless owned sleeping Python child, not Unreal. Injecting a monitoring exception reached finally, stopped that child and preserved its actual error/exit code. See guard_fault_test_01.json. This safety test is distinct from the successful actual Unreal Import03.

## Morning fresh reload and feature copy

Reload01 PID2956 exited0 in13.797seconds, structured reason ok, zero errors/warnings/dialogs, minimum free RAM3.817GiB; all1,124 protected files unchanged. Looping,139.2seconds,48kHz/stereo,volume0.5,pitch1 and the shared Music class persisted. The old day1 host proves the Wave only; it does not contain the new director or CombatMusic slot. See native_reload_01.json and native_package_freeze.json. The exact17,294,337-byte Wave is now in this feature worktree; no style/Core/config/map change or Main activation is included.
