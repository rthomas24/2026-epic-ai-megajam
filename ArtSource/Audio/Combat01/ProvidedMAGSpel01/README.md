# PR12 — Nick's supplied Magic Spell Impact recordings

PR12 is Lancer Needle Cannon firing, hull impacts and shield impacts, not ship switching. Its preserved head is `69c18e33e519096eea065988e6874bd7f86c9cd5`.

| Existing cue | New recording numbers, takes 1–3 |
|---|---|
| MSS_NeedleCannon | Impact19, Impact109, Impact117 |
| MSS_Hull | Impact109, Impact29, Impact117 |
| MSS_Shield | Impact3, Impact29, Impact19 |

`index.html` provides the unchanged supplied sources, previous PR12 source takes and prepared replacements. These are source previews; they do not represent distance attenuation or the complete game mix. No autoplay.

`replacement_manifest.json` identifies the current exports (`export_v4`), exact source windows and hashes. Earlier exports remain preserved for comparison. Preparation is limited to mono conversion for spatial playback, trimming, DC correction, short fades and linear levels; no synthetic layers, filtering, pitch or tempo processing. The existing MetaSounds retain their authored runtime take/pitch/level variation. Supplied and derived WAVs are ignored, kept private locally, and excluded from the Git checkpoint.

All nine current exports pass the project's `sfx` category checks, and each role's takes are within 2 LU. They are 48 kHz, 16-bit mono PCM, with zero first/last samples and no clipping. See `prepared_qa.json`. The stress preview is an offline scheduling model with six firing/four impact windows, **not** an engine concurrency test or the actual eight-impact concurrency setting. Its −4 dBFS source-model peak does not certify the game's mix.

## Native scope and status

Claim: `98c956c9`, branch `nick/audio-pr12-replacements-1113`.

The compatible private host is `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-tutorial-production-64490fb`, HEAD `28f03b4f788ed25ddf2b3a5cb406d366b953a05b`, game module SHA256 `3c034cf024ac91d9e7546fd1935decbe26e9401706be02227ac05b2c6a676c24`. All seventeen original PR12 package hashes match `native_package_freeze.json` before editing.

Only the nine existing Wave assets changed. The three MetaSounds, attenuation, Sound Class, concurrency and presentation data asset stayed byte-identical. No maps, gameplay code or settings changed. `native_job.py import` backs up original Combat01 packages, protects the host and enforces the 4 GiB launch threshold and bounded memory/time guard. `native_job.py reload` is read-only. Do not run without the current Lead machine slot.

## Verified checkpoint

- Import01: PID28536, exit0,59.79s,min3.023GiB; exactly nine Wave changes,2460protected files checked,no unexpected Content.
- Reload02: PID3284,exit0,22.66s,min3.064GiB; all2460protected files unchanged. All nine export as the correct48k/16mono PCM with exact frame counts and at most one16-bit quantization step from source. The earlier Reload01 byte-identity assertion was too strict about the engine's float conversion; it stopped with no writes and remains preserved.
- GameplayAudioOnly02: PID3704,exit0,51.84s,min2.696GiB; all2460protected files unchanged. Real Lancer possession and fire input; native NeedleCannon and Shield cue activity observed.23.990sec same-PID48kstereo audio,−7.4dBTP,−25.3LUFS,zero clipping/discontinuities. The initial GameplayAudioOnly01 control-connection timeout was preserved; bounded startup retry passed in02.
- Hull's three Waves load and export correctly, but this run did not observe an actual Hull-hit event. No final subjective mix approval, full-fight concurrency validation, current Main integration or Shipping claim.
- `copy_verified.py` copied exactly the nine tested Wave bytes to this feature worktree, preserved previous Waves under its ignored Saved folder, and checked the other eight PR12 packages unchanged. See `native_handoff.json`.

Actual recording: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-tutorial-production-64490fb/Saved/PR12Provided01/GameplayAudioOnly02/game_pid_audio.wav`. This was audio-only gameplay validation, without video; source A/B players remain the clearest way to compare the individual takes.

## Provenance and publication

Nick supplied the five files directly. Embedded metadata identifies rawAmbience / Magic Spell Impact. Original hashes are in `source_receipts.json`; private originals remain git-ignored. No purchase or new pack download occurred.

The official free pack lists five 48 kHz, 24-bit stereo WAVs totaling about ten seconds, consistent with these files. That alone does **not** prove the acquisition or exact licensed bundle. References for Lead's receipt review:

- https://rawambience.com/product/free-magic-spells-sound-effects/
- https://rawambience.itch.io/free-magic-spells-sound-effects
- https://rawambience.com/terms-conditions/

The author's free-pack clarification permits commercial games and excludes redistribution as a standalone sound collection. Keep supplied sources and derived library clips private until the receipt/publication handling is resolved through Lead. This note is not an acquisition receipt or an unconditional redistribution clearance. Lead owns the final shared credits entry and publication; this worker has not modified CREDITS.md.
