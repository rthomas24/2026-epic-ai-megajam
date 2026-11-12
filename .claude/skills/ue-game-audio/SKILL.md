---
name: ue-game-audio
description: How to produce cohesive, professional-sounding game audio for Triad Drift in Unreal Engine 5.9 during the jam - which source to use for each kind of sound (licensed libraries, AI sound and music generators, recording, MetaSounds) and what each licence allows, layered sound design with repeatable JSON recipes and 3-6 takes, seamless loops, one sound family for the three ships, the Director's Cut sounds (switch cut, slate, "CUT!", slow motion), MetaSound sources built through Unreal's MCP server, attenuation / sound classes / concurrency / sound mixes, layered music driven by a 0..1 intensity, a mix standard with measured targets, tested ffmpeg scripts that measure, prepare, layer and picture audio, and how a sound reaches gameplay through an FTDFXCue slot. Use whenever a task touches sound effects, music, ambience, voice, mixing or loudness, ART-10 or ART-11 in features.md, Content/TriadDrift/Audio or ArtSource/Audio, a Sound field of a DA_* presentation asset, or anyone says "make a sound for", "we need audio", "it sounds cheap / harsh / repetitive / too loud", "add music", "the switch sound", "normalise", "LUFS", "make it loop".
compatibility: Unreal Engine 5.9.0 (MegaJam Preview, CL 60351404) with the EditorToolset and MetaSoundToolset plugins (enabled in unrealGameJam.uproject). Scripts need Python 3 (standard library only) and ffmpeg + ffprobe on PATH; the ue_* scripts also need this repo's Tools/td_mcp.py and a running editor. Checked 2026-11-12.
---

# Game audio for Triad Drift

Audio is a judged category, the game is silent today, and the first attempt (pure synthesis) was
rejected. This skill is the path that replaces it: **recorded or generated material, designed in
layers, levelled to one standard, played by MetaSounds, approved by ear.**

Read `references/project-conventions.md` before touching the repo and `references/sourcing.md`
before acquiring anything.

## Rules that do not bend

1. **Nicholas approves sources and sounds.** No download, sign-up, payment or API key by an agent.
   A sound is "presentation approved" only when he has heard it in the running game and said so.
2. **No sound ships raw.** Every sound is designed from layers (`audio_layer.py`) or at least
   prepared (`audio_prep.py`), and passes `audio_report.py` for its category.
3. **Not synthesis-only, not retro** (`AAA.md`). MetaSounds play and vary recorded material; a
   synthetic layer is a sweetener, never the whole sound.
4. **Everything that repeats has 3-6 takes** plus random pitch (±0.7 semitone) and level (±1.5 dB)
   at play time. Signature sounds (switch cut, "CUT!", slate, UI confirm) have one take and no spread.
5. **Every 3D sound carries its own attenuation, sound class and concurrency.** Gameplay plays cues
   with no overrides, so a sound without attenuation is heard at full level from anywhere.
6. **One level standard.** Files are normalised per category; in-game levels are dB relative to one
   anchor (Lancer's primary fire). Numbers: `references/mix-standard.md`.
7. **Every source and tool is logged** in `CREDITS.md` when acquired. No CC-BY-SA, GPL or
   non-commercial audio; no licensed library audio into AI tools; no third-party raw files in a
   public place.
8. **Say what was measured, not how it sounds.** An agent reaches "technically verified" at most.

## The loop, per sound

Paths below are from the repo root; `S` is `.claude/skills/ue-game-audio/scripts`.

1. **Brief** (5 lines): `ArtSource/Audio/<Category>/<Asset>/brief.md`. What it tells the player,
   how often it plays, which family, length, category. → `references/sound-design.md` §1
2. **Source.** Pick from the approved sources by category (table below). Copy only the clips you
   use into `source/`; log them. → `references/sourcing.md`
3. **Design.** Write `recipe.json` (transient / body / tail / sweetener, each in its own frequency
   range, offset by a few ms) and render takes:
   `python S/audio_layer.py ArtSource/Audio/<Cat>/<Asset>/recipe.json --out ArtSource/Audio/<Cat>/<Asset>/export`
   A single clean recording that needs no layering goes through
   `python S/audio_prep.py source/*.wav --category sfx --name SFX_<Group>_<Thing> --out export`
   (loops: `--category loop --loop-crossfade 0.5`). → `references/sound-design.md`
4. **Measure and look.** `python S/audio_report.py export --category <cat> --takes` must exit 0.
   `python S/audio_view.py export --sheet review/takes.png`, then open the picture: is the attack
   sharp, does the tail end, are the takes different, is the energy where this ship's sounds live?
5. **Build in Unreal.** Start an editor that can render audio (`-NonRealtimeAudio`, your side's
   port), write a spec, run it:
   `python S/ue_run_python.py --port 8021 S/ue_audio_build.py --spec ArtSource/Audio/specs/<name>.json`
   It imports the takes, creates or updates the MetaSound source and sets its class, attenuation
   and concurrency. → `references/unreal-implementation.md` §2-5
6. **Check the asset.** `python S/ue_run_python.py --port 8021 S/ue_audio_check.py --spec <check>.json`
   renders what the asset really plays to a WAV; run `audio_report.py` and `audio_view.py` on it.
   Not silent, not clipped, plays differ, loop is clean, parameter changes it.
7. **Wire it.** Set the `Sound` of the right `FTDFXCue` slot in the `DA_*` asset
   (`/Game/TriadDrift/Presentation/`), or reference it from the audio director for music, mixes and
   stings. → `references/project-conventions.md` (slot map), `references/unreal-implementation.md` §8, §11
8. **Report "technically verified"** with the numbers, and hand it to Nicholas to hear in the game
   (checklist: `references/mix-standard.md` §7). Revise from his words. Record the listen in
   `DEVLOG.md`. Only then: "presentation approved".

## Where each sound comes from (short form)

| Sound | First source | The design carries |
|---|---|---|
| Weapons, impacts, explosions | Recorded library (Sonniss GDC bundles), layered | A recorded transient; designed body; short tail for rapid fire |
| Engines, boost, shield and portal loops | Library loop → seamless loop → MetaSound with a 0..1 parameter | Pitch and level follow speed |
| Hyperdrive, risers, beds, ambience | AI sound generator on a commercial plan, then prepared | Long evolving textures |
| UI | Library UI sounds, one key, tonal family | Consonant = good, low knock = no |
| Switch cut, slow-mo stings | Library whoosh + click + one tonal layer per ship | Signature: no variation |
| Slate, "CUT!" | Record it | Megaphone band; tape-stop is a Sound Mix |
| Music | Nicholas decides: AI stems at one BPM and key (default) or the jam partner's library | Layers of equal length, driven by `Intensity` |

Full table, licences read on 2026-11-12, AI tools and their terms, and the option sheet to put in
front of Nicholas: `references/sourcing.md`.

## Scripts (`scripts/`, all tested on 2026-11-12; output in the research note)

| Script | Does | Exit codes |
|---|---|---|
| `audio_report.py <files or folders> [--category C] [--takes] [--same-length] [--mix] [--json F]` | Duration, rate, channels, loudness (integrated from 3 s, K-weighted ungated below), LRA, true peak, sample peak, leading / trailing silence, DC offset, clipping, loop seam; pass / fail per category; table + JSON. `--targets` prints the targets | 0 pass, 1 fail, 2 error |
| `audio_prep.py <inputs> --category C --name N --out DIR [--loop-crossfade S]` | Mono / stereo as needed, DC removal, trim, fades, 48 kHz 16-bit, one linear gain to the category target, numbered takes, a `.prep.json` log, then the same check | 0 / 1 / 2 |
| `audio_layer.py recipe.json --out DIR` | Builds takes from a JSON recipe of layers (file, start, gain, pitch, filters, fades, reverse, random ranges); same seed = same files; `.layers.json` log | 0 / 1 / 2 |
| `audio_view.py <files> [--sheet PNG]` | Spectrogram over waveform as PNG, or one sheet comparing files on a shared time scale | 0 / 2 |
| `ue_run_python.py --port P script.py [--spec spec.json]` | Sends a Python file to a running editor's `execute_python` MCP tool, with the spec as `SPEC` | 0 / 1 |
| `ue_audio_build.py` (in-editor) | From one spec: sound classes, attenuation, concurrency, sound mixes; imports; MetaSound one-shots, parameter loops, layered music | via runner |
| `ue_audio_check.py` (in-editor) | The engine's own wave analysis; offline render of an asset to WAV | via runner |

Categories: `sfx` (3D one-shots), `ui`, `sting` (2D cinematic one-shots), `loop`, `ambience`,
`music`, `vo`. `python S/audio_report.py --targets` prints what each requires.

## What an agent can and cannot verify

Can, without ears: format and level, clipping, silence, DC, loop seams, that takes differ, where the
energy sits (picture), that the Unreal asset plays, varies, loops, follows its parameter, and that a
mix changes pitch and level (rendered and measured).

Cannot: whether it is good, fitting, tiring, readable in a fight, or balanced against everything
else. That is Nicholas, in the running game, on headphones and speakers
(`references/mix-standard.md` §7). Never report past "technically verified" on your own.

## References

| File | Read it for |
|---|---|
| `references/sourcing.md` | Source per category; licences (Sonniss, Fab, Epic content, CC0, the jam partner); AI generators and their terms; recording; the decision sheet |
| `references/sound-design.md` | Briefs, layers, shaping, takes, loops, the three-ship family, Director's Cut sounds, cohesion checklist |
| `references/unreal-implementation.md` | How gameplay plays cues; an audio editor session; every MCP audio tool by its real name; the build spec; MetaSound graphs and pins; import settings; slow motion and ducking; the audio director; music; in-editor checks; pitfalls |
| `references/mix-standard.md` | The rules carried over from the owner's previous game; anchor; file targets; in-game level table; attenuation, concurrency and class presets; music and master; the listening checklist |
| `references/project-conventions.md` | Folders, names, the slot map, `CREDITS.md` rows, honest stages, working rules |
| `references/jam-plan.md` | The 20 sounds to make first, in order, with source, method and time; the next tier; the calendar |
| `reports/Game audio practices for Triad Drift.md` (repo root) | The research and the test output behind all of this |

## Known gaps (2026-11-12)

- Nothing has been heard by a person, and nothing was run inside the Triad Drift project: the
  Unreal steps were exercised on a scratch project with the same engine and toolsets.
- The ship loop parameters `Speed01`, `Boost01` and `Drift01` are in progress on the engineering side (not on
  `main` yet); until then the engine loop only gets a pitch multiplier. The live state for music and mixes
  (`UTDAudioStateSubsystem`, `references/unreal-implementation.md` §8) is on `main` and tested.
- The audio director Blueprint, the master limiter at runtime, Quartz and capturing the whole
  game mix are described but not built or tested.
- The GitHub repo is private (checked 2026-11-12). Each library's licence still decides whether its clips may be
  committed; "no third-party raw files in a public place" stands.
