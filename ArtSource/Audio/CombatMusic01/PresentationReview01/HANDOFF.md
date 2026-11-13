# Presentation listening and music handoff — November 13

Prepared source review only. No new acquisition, media alteration, Unreal launch, native/style edit or commit is implied. Lead owns current-host build and remote preflight. Nick retains listening approval.

## Playable existing previews

Open `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/ListeningReview01/index.html`.

Twelve players: the 107-second menu theme, 139.2-second combat theme, three fire takes, three hull takes, three shield takes, and the existing five-second offline overlap mix. Only one player runs at a time; each has an optional loop. All WAVs are referenced in place. `media_receipt.json` beside the page records format, duration and hashes. `build_listening_page.py` recreates the page only in an empty destination; no media writes.

The nine PR12 inputs must match `ArtSource/Audio/Combat01/sources.json`, the menu input must match `menu_loop_runtime_04.json`, and combat input must match the frozen a705d8a0 prepared Wave. Source-file players do not apply runtime gain, attenuation, randomization or Settings. The stress mix already contains its stated cue gains. These are not recordings of the current game.

Sources and credits remain in the existing manifests: Nick's original Suno songs; A.D's free AK-47 pack (fire4–6 credited to Dennis/dennish18 on Pixabay); PlaceHolder's 50 Free Game Sounds Pack for hull/shield. Private team audition only; no raw-library redistribution or new licence claim.

## Exact missing recordings

1. PR11 menu: at least 112 seconds spanning the 107-second join, followed by New Game fade; menu re-entry separately. Retain actual voice/device/build evidence. Earlier menu tests observed the loop and 1→0→1 menu voice counts, but saved no PCM join recording. One long prior run ended watchdog125; its clean confirmation exited0 and must remain distinguished.
2. PR12 current-host Lancer: short and sustained firing, real hull and shield hits, near/distant placement and repeated overlap. Earlier old-host Gameplay01 exited0 and observed all three native cues; `pcm_output_captured=false`. Engineering N031 current-Main cook and eight-second fire/no-errors are technical evidence, not recorded listening approval.
3. Supplied combat song: real space wave and real ground encounter activation/clear; two-second entry, six-second hold and four-second exit; a new fight during hold/fade without restart. Record beyond139.2 seconds, CUT/retake, victory/travel and Music slider/mute. A Dominion phase-helper loop probe is not a full playable-route test.
4. Intensity: matched quiet/full combat captures after engineering adds the actual mix bridge. No intensity-responsive output exists yet on the reviewed code.

No `.wav`, `.mp4`, `.webm` or `.ogg` exists in the old day1 host's `Saved/AudioMusic` evidence tree at this review. The existing V1 bounces elsewhere are rejected earlier sounds and cannot stand in for PR11/12 in-game recordings. Recordings should remain private Saved evidence with device/rate, host/code/asset hashes, test actions, and measured whole-output levels. Do not launch capture or change output routing before Lead's grant.

## PR11 candidate versus current defaults

The actual PR11 candidate is `nick/integrate-menu-loop-20261112@e6428ec2`. Its existing `Content/TriadDrift/Presentation/DA_UI_Style.uasset` is1435bytes/SHA256 `0a1fa00da9771737cc36e3f0db16ef746fb284b784d210c8537acdc9a2d07df0`. The similarly named `nick/integrate-main-menu-20261113@79d3ab76` does not contain a tracked style. Lead reports current Main has none; no existing Main style should be replaced.

Original creation authored only MenuMusic and verified six then-existing defaults. Current Core `TDPresentation.h` at cb9c103c contains additional optional widget, combat/UI/Director, ambience and intro fields. The old asset has not been loaded against these current compiled defaults, so binary compatibility is still pending. Source says empty new slots preserve engineering fallbacks. Do not assign widget classes or legacy cue placeholders.

`audit_style.py` is a read-only compatible-host proposal: hash-check existing PR11 candidate and current reviewed header, derive every declared UTDUIStyle property from that header, compare loaded values to the current compiled class default object, and require MenuMusic to be the only difference. Any additional difference or missing reflected property stops the handoff. This is not an executed native result. Header-field coverage was checked offline; native Python struct/container equality still needs the real run.

## Intensity must reach the actual mix

At cb9c103c, `UTDAudioStateSubsystem` computes and smooths MusicIntensity01, but its only other source consumers are tests. `UTDMusicDirectorSubsystem::ApplyEnvelope()` sets only the transition Envelope. The supplied asset is a direct SoundWave; setting a MetaSound float would not affect it. Wave/Dominion/encounter remain the combat gate because idle ground/space intensity is nonzero.

Request Ryan's existing director owner through Lead to multiply the transition envelope by a bounded intensity gain, proposed `10^((-4 + 4*clamp(MusicIntensity01,0,1))/20)`. This ranges -4dB..0dB and never exceeds the current full-level authored ceiling. Keep Wave volume0.5, class/Settings, playback position and travel/CUT rules. Use the already smoothed state; no new controller, pitch change or timer-driven restart. A single stereo song supports gain variation only; it cannot provide independent instrument stems. Final amount needs Nick's in-game listening. Engineering owns the code and its tests; no Source edits here.

Proof must correlate actual intensity and component gain at stable envelope, show SFX/Menu class unaffected, and capture the audible difference plus no restart. A changing state value alone is not mix proof. Future Music volume/VO ducking should compose with this gain rather than overwrite it.

## Isolated bind and verification sequence

Lead provisions/builds `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/presentation-runtime-1113` and supplies exact build/source pins, mounts PR11/PR12 dependencies and verified combat Wave, then grants one bounded operation at a time. No shared style or game asset is edited by these source preparations.

1. Read-only `audit_style.py`: SPEC execute/claim_commit/project_dir/header_sha256/host_build_commit/run_id; expect exact PR11 baseline0a1f. Protect all inputs and original saves in the parent guard.
2. Only after PASS and Lead clearance: `bind_combat_music.py`, preserved byte-identical from Morning01 (SHA aaa1cd4a5e5608264d3a7d25f59bc59a4bcb83028bf6878ae1a545a606832f10). SPEC execute/claim_commit/project_dir/style_sha_before/expected_previous_music(null)/run_id. One CombatMusic setter; backup original binary, export every property before/after, exclude only CombatMusic line and require exact equality. Existing MenuMusic and all other settings retained. Existing Wave1110 and Music class813 unchanged. This generic ObjectExporterT3D path is grounded in installed engine source but not yet native-tested.
3. Separate fresh process: `verify_bound_music.py`, explicit SPEC execute/project_dir/binding_report/binder_source/run_id. Pins binder SHA, bound style hash, Wave/class hashes, correct reference/format and full exported style equality. No asset saves. Native reload is still not runtime playback.
4. Granted actual game-only probes: existing attach-only `observe_music.py`, isolated UserDir, private port8025, real output device, -NoLiveCoding, bounded owned-process launcher and fresh RAM/process preflight. Lead must review the launcher after build/host pins are available. Keep actual victory/map cleanup separate from the real pause-menu fade test. Loop probe emits heartbeats and fits a reviewed300-second outer limit; separate ground/cleanup run as needed. No fake audio Play or director override.

The three copied Morning01 scripts preserve their original bytes. Old READY.json and frozen sources remain unchanged; this handoff supersedes only the assumption that the current host already has a style. No script auto-mounts or creates one. After tests, ask Lead for fresh remote preflight before any scoped commit/publication. No Main merge or listening approval implied.

## First native attempt and correction

Lead subsequently granted the built isolated host at dae4943a. GuardBind01/PID14108 stopped at the read-only audit, before the binder ran: OS4294967295, engine exit1/exit_request,53.859seconds, minimum available4.610GiB. All1682 protected files unchanged and no new Content; style remained0a1f. No process remained. Preserve both raw style_audit.json and engine/guard reports under that host's Saved/AudioMusic/CombatMusic01.

The audit flagged all12 FTDFXCue properties because their Python wrappers compare by identity. Every printed named value matched the CDO; only memory addresses differed. Other16 non-menu properties matched, and MenuMusic was the expected difference. This failed run is not a native compatibility PASS.

The corrected audit checks the current FTDFXCue schema has exactly the six understood members, then reads vfx/sound/shake reference paths, exact shake_scale/shake_radius values and all three vfx_scale components. It compares these value dictionaries for every cue, plus all remaining properties. Unknown members or unreadable properties fail. The corrected audit still requires MenuMusic to be the only real difference, and does not change or skip any style field. Lead must grant the new evidence-ID attempt after the Hero slot; no automatic native retry.
