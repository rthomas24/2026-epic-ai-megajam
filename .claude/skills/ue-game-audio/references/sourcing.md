# Sourcing: where each sound comes from, and what its licence allows

Checked on **2026-11-12**. Licences change: re-open the linked page on the day you acquire something
and record its version and date in `CREDITS.md`. Licence terms below are paraphrased from the live
pages read that day (the link is next to each); "not verified" means the page was not read, so do
not rely on it. Read the page itself before acting on anything that matters.

Jam rules that apply (jam page, read 2026-11-12, https://itch.io/jam/2026-epic-ai-megajam):
third-party assets are allowed with the right licence and must be listed in the submission; any AI
tool is allowed, audio included; the tools field feeds partner prizes; sound and music are a judged
category. The full Official Rules on Box were not fetched.

## Hard rules before anything is acquired

1. **Nicholas approves every acquisition.** `docs/visual/STATUS.md` (ART-10): no spending, no pack
   download and no import until he has approved the source. An agent never creates an account, signs
   in, pays, or enters an API key; a human does that and hands over files or a key.
2. **Never use CC-BY-SA or GPL audio.** The Unreal Engine EULA §6(c) forbids combining the engine
   with GPL or Creative Commons ShareAlike content
   (https://www.unrealengine.com/en-US/eula/unreal, read in a browser 2026-11-12). The Fab EULA
   §6(a) has the same bar. Also no CC-BY-NC or other "non-commercial" material if the game may be
   sold later.
3. **Never feed licensed library audio to an AI tool** (audio-to-audio, "extend", "variations",
   training). Sonniss, Fab NoAI items, Zapsplat, Ovani, WeLoveIndies and Pixabay all forbid AI
   training or development use.
4. **Raw library files do not go into a public place.** Sonniss, Fab, Pixabay, Mixkit and 99Sounds
   forbid supplying the sounds on their own. A private two-person repo fits the Sonniss own-team
   clause and the Fab private-repository clause; a public repo does not. **Whether this repo is
   private was not checked: ask a human before committing any third-party audio.** Keep whole
   downloaded libraries outside the repo in any case (they are gigabytes).
5. **Every source is logged** in `CREDITS.md` the moment it is acquired (references/project-conventions.md).

## Routing: which source first, per category

"Library" means a professional recorded library (Sonniss first). "AI SFX" means a text-to-sound
generator on a plan with a commercial licence. Whatever the source, the material then goes through
`references/sound-design.md` (layer, shape, make takes) and `scripts/audio_prep.py`; a raw download
or a raw generation is never the finished sound.

| Category | First choice | Second choice | Do not |
|---|---|---|---|
| Ship weapons (fire) | Library: sci-fi weapon, gun and mechanical recordings, layered (recorded transient + designed body) | AI SFX for the body or energy layer only | Pure synthesis; one AI generation as the whole shot |
| Impacts, explosions | Library: explosions, metal impacts, debris, glass | AI SFX for tails and debris sweeteners | Synthesised noise bursts |
| Engine, boost, drift loops | Library loop (jet, turbine, electric motor, rocket) cut to a seamless loop, played by a MetaSound that maps `Speed01` to pitch and level | AI SFX with the loop flag | A MetaSound oscillator as the whole engine |
| Hyperdrive (spool, tunnel, drop-out) | AI SFX for risers and tunnel beds + a library impact for the drop-out | Library whooshes and risers | |
| Portals (open, idle, transit, close) | Library whoosh + glass or ice element + an AI SFX "energy tear" layer | | |
| Shield dome, shield hits | Library electric hum, glass, resonant metal; AI SFX energy shimmer | | |
| UI and HUD | Library UI packs (Sonniss; Kenney as one layer); one key for the whole set | AI SFX one-shots; a MetaSound sine layer for tonal confirmation | Stock "8-bit" blips on their own |
| Director's Cut: switch cut | Library whoosh + mechanical "film gate" click + a per-ship tonal layer | AI SFX whoosh | |
| Director's Cut: slate, "CUT!" | **Record it**: two blocks of wood or a real clapperboard, and a team member shouting "CUT!" | Library slate clap; AI voice (credited) | |
| Director's Cut: slow motion | No new asset for the effect itself: a Sound Mix (pitch down + low-pass) in Unreal; plus an enter and an exit whoosh | | Pre-rendered slow versions of every sound |
| Blade (ignite, hum, swing, hit, deflect) | Library electric hum and whooshes, layered; a hum loop | AI SFX | Anything that closely imitates a film's sword sound (other people's IP) |
| Footsteps | Library footsteps by surface (Fab has free footstep packs; check each listing) | Record them | |
| Enemy ships, crew | Same sources as the player, pitched down and harsher (family rules in sound-design.md) | | |
| Ambience (space bed, Verdance, Cinder) | AI SFX or Stable Audio for long beds; library nature and industrial recordings | | Silence |
| Music | Nicholas decides. Default: AI music with stems at one tempo and key (Stable Audio 3, or Eleven Music on a paid plan), cut to bar-length loops | WeLoveIndies (jam only, non-commercial); CC0 / CC-BY packs below | Suno free, Udio, Mixkit music (their terms rule it out) |
| Voice barks (ART-11) | AI voice, credited (features.md ART-11) | Team members | |

**When MetaSounds synthesis is the right tool:** as the *player* of recorded material (random take,
pitch and level variation, parameter-driven loops, layered music), and for single synthetic layers
recordings do not give you: a sine sub-drop under an explosion, a noise riser, a tonal UI
confirmation, an LFO shimmer on a shield loop. **When it is not:** as the whole sound of a weapon,
impact, engine or whoosh. The first all-synthesised attempt was rejected, and `AAA.md` rules out
placeholder-quality and retro-style audio.

## Recorded libraries

### Sonniss #GameAudioGDC bundles (first choice for SFX)
- Pages read 2026-11-12: https://sonniss.com/gdc-bundle-license/ (read twice: by a research agent
  and directly), https://sonniss.com/gameaudiogdc, https://gdc.sonniss.com/
- Licence **version 2.0, effective 27 August 2026**. Earlier downloads keep the version that was
  published on their download date, so record the date.
- Personal and commercial projects are allowed, sounds may be modified, and no attribution is needed.
- The sounds may not be handed to anyone else as sounds, and that covers modified and re-designed
  versions too. A finished project (a game) may be published and sold normally.
- Sharing with people in your own team who work on your projects is allowed.
- No use for training, developing or enhancing AI systems.
- Free, no account. Sizes as listed: GDC 2026 7.47 GB in 5 zips; 2024 23.5 GB; 2021-2023 38.7 GB;
  2020 48.2 GB; 2019 26.2 GB; 2018 29.3 GB; 2017 19.2 GB; 2016 12.9 GB; 2015 9.0 GB. Direct links per
  part, a mirror and a torrent. Parts are not themed: each bundle links a track-list sheet; search it
  first, then download only the parts that hold what you need. **A human starts the download.**
- Not verified: sample rates and bit depths; which part holds which library.

### Fab (free and paid packs, installable from the editor)
- Pages read 2026-11-12: https://www.fab.com/eula (last updated 1 October 2024),
  https://www.fab.com/category/audio?is_free=1
- Standard License: content may ship inside games sold to the public, in any engine, with no credit
  required, and may be shared with collaborators in a private online repository while working on
  the project (§5(a)).
- Each listing states its own licence and two AI flags (allows usage with AI; generated with AI).
  Some listings are CC-BY. **Read the listing's licence line before use.** Content flagged NoAI may
  not be used as training input for generative AI.
- An Epic account is needed to add to library: a human does that. Unreal 5.9 has a `FabToolset` on
  its MCP server (`SearchFabListings` with `bFreeOnly`, `GetFabListingDetails`, `SearchFabMyLibrary`,
  `InstallFabListing`, `FabSignIn`); searching works signed out, installing needs the human's
  signed-in session. Tool names read from the running editor on 2026-11-12; installing was not tried.
- Free audio seen on the first page of the free filter (licences per listing not checked): Modern
  Pistols SFX, Essential Footsteps SFX, Essential Ambience SFX, 50 Free Game Sounds Pack, Free Weapon
  Sound Effects, Interface & Item Sounds Pack, Procedural Wind.

### Epic's own content
- Unreal Engine EULA (read in a browser 2026-11-12): the engine licence covers engine code, Examples
  (the Samples and Templates folders) and Starter Content. Template content already in this repo
  (First Person template, `Variant_Shooter`, `Variant_Horror`) counts as Examples, which §5(b)
  allows distributing to anyone, modified or not.
- Engine and Starter Content audio may ship cooked inside an Unreal game. The EULA does not
  expressly allow putting it in a repo as loose source; treat a public repo as not allowed.
- The engine's own MetaSounds (`/Metasound/DefaultAssets/Sources/MSS_PlayRandom_Oneshot` and
  `MSS_PlayRandom_Loop`, seen in the editor) are graph logic with no audio; presets of them are fine.
- The jam does not allow relying only on Epic's starter templates. Template sounds are legal but
  generic; they must not be the game's identity.
- Not verified: whether sample projects other than Lyra carry a UE-only restriction.

### CC0 and small free sources
| Source | Licence (page read 2026-11-12) | Use for | Notes |
|---|---|---|---|
| Kenney, https://kenney.nl/support | CC0: commercial use allowed, no attribution | UI ticks and clicks as one layer; placeholders | Packs: Sci-fi Sounds (70), Interface Sounds (100), UI Audio (50), Impact Sounds (130), Digital Audio (60). Stylised and light: not for hero weapons. File formats not verified |
| Freesound, https://freesound.org/help/faq/ | Per sound: CC0, CC-BY 4.0, CC-BY-NC 4.0 | One specific sound you cannot find elsewhere | Filter `license:"Creative Commons 0"`. Login needed to download (human). Skip BY-NC and Sampling+. Quality varies; uploads can be mislabelled |
| OpenGameArt, https://opengameart.org/content/faq | Per asset: use CC0, OGA-BY, CC-BY only | Music loops | CC-BY-SA and GPL are out (rule 2) |
| 99Sounds, https://99sounds.org/license/ | Commercial use, credit optional, no redistribution of the files | Designed whooshes, impacts | |
| Mixkit SFX, https://mixkit.co/license/#sfxFree | Video games allowed; no redistribution with source files | Fillers | **Mixkit's music licence excludes video games** |
| Pixabay, https://pixabay.com/service/license-summary/ | Commercial use, no credit, not standalone | Fillers | Tracks with a shield icon are registered with Content ID and can trigger claims on the gameplay video |
| Zapsplat free, https://www.zapsplat.com/license-type/standard-license/ | MP3 only, credit required, single user, automated access prohibited | Avoid | Does not fit a shared repo or agents |

### WeLoveIndies (the jam's audio partner)
- Pages read 2026-11-12: https://www.weloveindies.com/en/epicmegajam , https://www.weloveindies.com/en/agb (terms dated 10.08.2026)
- A professional library (Dynamedion, BOOM Library): 3,200+ tracks, 250,000+ sounds, 48 kHz / 16-bit WAV.
- The free licence covers demos, pitches and non-commercial releases without budget, asks for their
  intro to be included, is tied to one project and one licensee, and is capped at 100 sounds and 10
  music tracks per project. A project that earns money needs a commercial licence from them. AI
  training use is forbidden.
- Account required (human). Counts as a third-party asset to list.
- Verdict: the best-sounding free option for the jam build, with strings: their intro, tight caps
  for a game this size, and a paid licence later. Not verified: the exact form of the intro; whether
  a git repo is permitted storage.

### Music with clear licences
| Source | Licence | Notes |
|---|---|---|
| Tallbeard / Abstraction Music Loop Bundle, https://tallbeard.itch.io/music-loop-bundle | CC0 | 200+ loops, mostly ambient and chiptune: check the style against `AAA.md` first |
| Void sci-fi pack, https://dancramp.itch.io/void-music-pack | CC BY 4.0 (credit required) | 13 WAV tracks; the page says no generative AI was used |
| OpenGameArt CC0 tracks | CC0 | e.g. "Space Music: Out There", "Fast fight / battle music (looped)" |
| incompetech (Kevin MacLeod), https://incompetech.com/music/royalty-free/faq.html | CC BY 4.0, exact credit line required | Widely used, so recognisable |

Library music is rarely available as same-tempo, same-key layers; with it, use two tracks and
cross-fade between calm and combat instead of vertical layers (references/unreal-implementation.md).

## AI generators (state on 2026-11-12)

Read by a research agent on the live pages that day unless marked. Re-read directly: the ElevenLabs
API reference. Prices and plans move fast.

| Tool | Makes | Commercial use | Output | Access |
|---|---|---|---|---|
| **ElevenLabs Sound Effects** (`eleven_text_to_sound_v3`, launched 2026-10-29) | SFX, ambience, short loops | Paid plans only. The free plan is non-commercial and requires attribution; Starter ($6/mo) lists a commercial licence. Free-plan output does not become commercial by upgrading later | 0.5-30 s; MP3 by default, WAV 48 kHz available; a `loop` flag for seamless loops | Web UI; `POST https://api.elevenlabs.io/v1/sound-generation` (`text`, `duration_seconds`, `prompt_influence` 0-1 default 0.3, `loop`, `model_id`, `output_format`). Set `model_id` yourself: the default is still v2 |
| **Stable Audio 3** (Stability AI, May 2026) | Instrumental music, SFX, loops | The pricing page and FAQ state that every plan, the free one included, carries a commercial licence (read in a rendered browser by the research agent; a plain fetch returns only the page title, so re-open it yourself). Outputs must not be presented as human-made | 44.1 kHz stereo WAV / MP3; music to 6 min, SFX to 30 s; no loop flag; Multi-Track stems marked experimental | Web app; API `POST https://api.stability.ai/v2beta/audio/stable-audio/text-to-audio`; open weights (Small SFX, Small, Medium) under the Stability Community License for under $1M revenue |
| Eleven Music (`music_v2_5`) | Music | Paid plans; large "Studio Games" are excluded, indie games under $500,000 revenue fall outside that exclusion. Free plan: no downloads | MP3 / WAV; a stem-separation endpoint | UI; `POST /v1/music` on paid plans |
| Suno v6 | Songs, instrumentals | Free plan: no commercial rights. Pro ($8/mo) and Premier: commercial use of permitted downloads, with monthly download caps | Stems reported, not verified | UI only; no public API |
| Udio | Songs | **Unusable**: the terms (revised 2025-11-12) forbid downloading outputs and limit use to personal, non-commercial purposes | - | - |
| Adobe Firefly Generate Sound Effects | SFX | Marketed as commercially safe; licence text not verified | Not verified | Web app only |
| SFX Engine | SFX | Paid plans include commercial use; the free trial cannot download | Up to 47 s | UI, API, MCP |
| Meta AudioCraft (AudioGen, MusicGen) | SFX, music | **No**: model weights are CC-BY-NC 4.0 | - | Local |
| Google Lyria 3.5 | Music | No clear commercial grant found; not verified | 44.1 kHz | Paid API |
| fal (jam sponsor, $30 credits) | Hosts ElevenLabs SFX, Stable Audio 3 Small SFX and others | fal's terms do not clearly grant output rights and the model owners' terms may apply: **not cleared** | per model | API |

Limits to plan around:
- No tool documents layers that are phase-aligned at a fixed tempo and key. Ask for the BPM and key
  in every prompt, cut loops on bar boundaries yourself, and check with
  `audio_report.py --same-length`. ElevenLabs' music guide says BPM is followed accurately and key
  often, which is not always.
- ElevenLabs' docs list 48 kHz WAV for non-looping effects; whether `loop` works with WAV and with
  v3 was not verified. Run every generated loop through `audio_prep.py --loop-crossfade` anyway.
- Generated one-shots usually arrive with leading silence, no consistent level, and as one take:
  `audio_prep.py` fixes the first two; make takes with `audio_layer.py` or by generating again.
- Generators are weakest at hard transients and sub-bass. Use them for bodies, tails, whooshes,
  risers, beds and textures; take the transient of a weapon or impact from a recording. (The
  author's judgement; no independent test of the newest models was found.)
- Listing is mandatory: every AI tool used goes under "Tools used" in `CREDITS.md`. itch.io asks
  projects to tag generative-AI content in the AI Disclosure section of the project's edit page
  (https://itch.io/docs/creators/quality-guidelines).

Prompt patterns the vendors document:
- ElevenLabs SFX (https://elevenlabs.io/docs/overview/capabilities/sound-effects): describe the
  source, the action, the space and the performance, in order, and use audio terms such as impact,
  whoosh, ambience, one-shot, loop, stem, braam, glitch, drone. Higher `prompt_influence` is more literal.
- Stable Audio (https://docs.stability.ai/stable-audio.md): give the track type first
  (`TrackType: SFX`, `TrackType: Music`, `TrackType: Instrument` for one stem), then source, action,
  duration and production (microphone position, room, processing).
- Music: tempo ("130 BPM"), key ("in A minor"), "instrumental only", the instruments, the mood, and
  what is absent. For one layer, name the instrument and say "solo".

## Recording your own

Worth it for three sounds that give the game an identity nobody else has: the slate clap, the
shouted "CUT!", and anything hand-held on foot (cloth, a switch, a latch). A phone in a quiet room
with soft furniture is enough; record 10 takes at arm's length, 48 kHz WAV if the app allows. Then
`audio_prep.py --category sting`. Credit the performer in `CREDITS.md` as an original recording.

## What to put in front of Nicholas (the decision he asked for)

| Option | Cost | What it gives | Strings |
|---|---|---|---|
| A. Sonniss GDC 2026 bundle (+ older parts as needed) | Free | Professional recorded SFX for every category | Private storage only; no AI use; a large download |
| B. A plus ElevenLabs Starter for gaps | $6/mo | Fast custom bodies, whooshes, risers, beds, loops | Tool must be listed; the paid plan must be active before generating |
| C. A plus Stable Audio 3 (free plan) for music and beds | Free | Instrumental stems at a chosen BPM and key | Confirm the free-plan commercial licence on the day |
| D. WeLoveIndies for music or everything | Free for the jam | The highest-quality library music | Non-commercial, their intro, 100 sounds / 10 tracks, a paid licence later |

Recommended: **B for sound effects, C for music**, with D as the fallback for music if the
generated stems do not pass Nicholas's listening. This is a recommendation, not a decision.
