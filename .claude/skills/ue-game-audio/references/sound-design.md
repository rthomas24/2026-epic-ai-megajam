# Sound design method: from library or generated material to a finished sound

The aim is a sound that is **built**, not found. Every finished sound in the game is a layered
design, in several takes, with a written recipe. Sources for the method are listed at the end;
where a rule is this skill's own judgement it says so.

## 1. Write the brief first (5 lines, in `ArtSource/Audio/<Category>/<Asset>/brief.md`)

1. What the player does or sees, and how often (once a minute, or ten times a second).
2. What the sound must tell the player (I fired; I hit; I am in danger; the ship changed).
3. The family it belongs to (Rift, Lancer, Bastion, enemy, UI, Director's Cut, world).
4. Length and shape: attack time, body length, tail length. A weapon fired 10 times a second gets
   150-300 ms; a one-off event can take 2-4 s.
5. Category (`sfx`, `ui`, `sting`, `loop`, `ambience`, `music`, `vo`) and its place in the level
   table (references/mix-standard.md).

## 2. Layers: transient, body, tail, sweetener

A designed one-shot is three or four layers, each doing one job, each in its own frequency range.
(The four-name model is assembled from sources 1-3; none of them uses exactly these four words.)

| Layer | Job | Where it lives | Typical source | Typical length |
|---|---|---|---|---|
| **Transient** | The first 5-30 ms: says "now" and gives the sound its hardness | 2-8 kHz | A real recording: snap, click, mechanical latch, gun mechanism, wood or metal hit | 10-60 ms |
| **Body** | The identity: what it is and whose it is | 150 Hz-2.5 kHz | Designed: processed recordings, an AI generation, a pitched tonal element | 80-400 ms |
| **Tail** | Size and space: how big, how far | Below 2 kHz, falling | Reverb of the body, a low rumble, debris, an air release | 150 ms-3 s |
| **Sweetener** | The detail that makes it this game's sound | Anywhere there is room | A reversed swell before the hit, a sub drop, an electric crackle, a glass ping | varies |

Rules that make layers work:
- **One layer per job.** Two transients fight; two bodies blur. If a layer does not change the sound
  when muted, delete it.
- **Carve, do not stack.** High-pass the transient (800-1500 Hz), low-pass the tail (1.5-2.5 kHz),
  keep the body in the middle. `audio_layer.py` gives each layer `highpass_hz` / `lowpass_hz`.
- **Offset by a few milliseconds.** Start the body 3-8 ms and the tail 20-40 ms after the
  transient, so the peaks do not land on the same sample and the attack stays clean (source 2).
- **The transient decides the level.** After summing, the result is peak-normalised
  (`audio_prep` / `audio_layer` do it); if the body is louder than the transient the sound will
  feel soft however loud it is played.
- **The tail must end.** A weapon that fires 10 times a second with a 2 s tail builds a wall of
  noise. Give rapid-fire sounds a 100-200 ms tail; put the long tail only on single big events.
- **Real under synthetic.** Sci-fi sounds that convince start from a real-world structure and add
  the unreal layer on top (source 1). The classic examples are recordings, not synthesis: a struck
  radio-tower guy wire for a blaster (source 5), a projector hum mixed with television interference
  for an energy sword (source 4). Use this as method, not as something to imitate.

## 3. Shaping tools (all available as recipe fields or `audio_prep.py` options)

| Tool | Use it to | How |
|---|---|---|
| Pitch (tape-style) | Change size and weight: down 3-12 semitones makes it bigger and longer, up makes it smaller and faster | Layer field `pitch_semitones` (pitch and length change together) |
| Trim | Use only the good 60 ms of a 2 s recording | `trim_start_ms`, `duration_ms` |
| Reverse | Make a swell into the hit (sweetener before the transient) | `reverse: true` with a `fade_in_ms` |
| High-pass / low-pass | Put each layer in its own range | `highpass_hz`, `lowpass_hz` (12 dB per octave) |
| Fades | No clicks at the edges; shape the attack | `fade_in_ms`, `fade_out_ms`; master `fade_out_ms` |
| Level | Balance the layers | `gain_db` |
| Reverb, distortion, compression | Tail and glue | Not in the recipe format: render the layer through ffmpeg first (`aecho`, `afir` with an impulse response you are licensed to use, `acompressor`, `alimiter`) and use the result as a source file. Keep the command in `brief.md` |

ffmpeg facts that matter (ffmpeg filter documentation, read 2026-11-12):
- `amix` scales its inputs down by default; a true sum needs `normalize=0` (the scripts use it).
- `asetrate` + `aresample` changes pitch and length together; `atempo` changes length only;
  `rubberband` (if the build has it) changes them independently.
- `loudnorm` is a dynamic normaliser unless run two-pass in linear mode; the scripts never use it
  and apply one linear gain instead, so a sound's own dynamics survive.
- Integrated loudness needs complete 400 ms blocks (EBU Tech 3341). A 200 ms one-shot has no
  integrated LUFS; `audio_report.py` reports K-weighted ungated loudness for anything under 3 s.

## 4. Variations (3-6 takes of everything that repeats)

Repetition is what makes game audio sound cheap. Two defences, used together:

1. **Different takes on disk.** Give `audio_layer.py` two or three alternative files per layer
   (`"files": [...]`) and small `vary` ranges; ask for 4 takes. Takes rotate through the files, so
   they differ in material, not only in pitch. Typical `vary`: transient pitch ±0.5 semitone and
   ±1 dB, body pitch ±0.4 and start ±2 ms, tail ±1 dB. Splitting a sound into layers with a few
   variants each multiplies the combinations (source 3).
2. **Random pitch and level at play time.** The MetaSound picks a take without repeating the last
   one and adds ±0.7 semitone (about ±4%) and ±1.5 dB. Those two numbers are the ones that worked
   on the owner's previous game (`client/src/audio/variation.ts`).

Limits:
- Takes of one sound must stay within 2 LU of each other (`audio_report.py --takes` checks), or the
  variation reads as a mistake.
- **Signature sounds do not vary:** the switch cut per ship, "CUT!", the slate, ability-unlocked,
  UI confirm, alarms, tonal cues. They must be recognised instantly (source 3 makes the same
  point). The previous game excluded its tonal cues from variation for the same reason.

## 5. Loops

1. Pick 4-12 s of steady material with no event that the ear can count (a click, a wobble, a
   voice). Shorter loops reveal their cycle.
2. `audio_prep.py --category loop --loop-crossfade 0.5`: the file is split in the middle, the
   halves are swapped, and the new inner seam is cross-faded with equal-power curves. The loop
   point is then the original, continuous middle of the recording (the method in source 6).
3. The check: seam step at most -40 dBFS and the two ends within 1.5 dB
   (`audio_report.py --category loop`). In the test for this skill the same recording failed
   without the cross-fade (ends 31.8 dB apart) and passed with it (0.6 dB).
4. Listen to it loop ten times before importing. Numbers do not catch a slow wobble or a tonal
   beat between the two halves.
5. Engines: the loop is authored at mid-throttle. The MetaSound pitches it from about -2 to +5
   semitones and from -8 to 0 dB with `Speed01`; listen to both ends, a loop that is fine at 0 can
   whine at +5.

## 6. One family, three ships

The three ships must be told apart by ear with the screen off, and still sound like one squad.
(These slots are this project's design, consistent with the earlier starter plan in
`ArtSource/Audio/StarterPlan/README.md`; they are not from an outside source.)

| | Rift (magenta, trickster) | Lancer (cyan, interceptor) | Bastion (amber, heavy) |
|---|---|---|---|
| Frequency home | Upper mids and air, 1.5-6 kHz | Presence, 3-8 kHz, little low end | Low and low-mid, 60-400 Hz |
| Envelope | Soft attack, a swell or a bend, medium tail | Hardest and shortest attack, almost no tail | Slow, heavy attack, the longest tail |
| Pitch gesture | Rising or bending (phasing, glassy) | Flat and tight, a quick double | Falling |
| Texture | Glass, air, phase, shimmer | Dry, metallic, electric snap | Thick, rumbling, mechanical |
| Engine | Airy turbine with a harmonic whistle | Fast, focused whine | Low, weighty drone |
| Weapon | Rounded phase "zap" | Dry rapid snap | Heavy low-mid pulse, short boom |
| Switch cut layer | Airy rising interval | Quick high double tick | Low descending thud |

What keeps them one family:
- **Shared DNA layer:** one sweetener source (for example the same electric crackle or the same
  air release) used in all three ships' weapons and switch cuts, pitched to each ship's range.
- **Same space:** one tail treatment (the same short reverb or the same low rumble) across the squad.
- **Same loudness standard and the same peak discipline** (references/mix-standard.md).
- **Same transient family:** all three transients from the same kind of recording.

The enemy is the opposite on every axis: detuned and dissonant, distorted, falling pitch, noisy
tails. Use the same sources as the squad, pitched down 5-12 semitones and distorted, so the world
stays coherent while the sides stay distinct. Enemy telegraph sounds are the one enemy sound that
must cut through everything: give them a clear rising tone in 1-3 kHz and their own level row.

UI is one tonal family: pick one key for the whole game (the music's key), build confirm as a
rising pair, back as a falling pair, error as a non-harmonic low knock. (Consonant reads as
positive and inharmonic as negative; this is common practice, taken from an Audiokinetic article
that could not be fetched, so treat it as unverified.)

## 7. Director's Cut sounds

The theme is judged, and sound is the cheapest way to make it felt.

| Moment | Design | Notes |
|---|---|---|
| **Ship switch = a cut** | A hard film-cut whoosh (150-350 ms, energy peaking at the cut, not after it) + a short mechanical "gate" click on the frame of the cut + the new ship's tonal layer from the table above | A whoosh is movement over a short time: a noise or air source through a swept filter with a pitch and level arc (source 8). Three assets, one per ship, sharing the whoosh and the click and differing in the tonal layer. Signature sound: no random variation. Quick cuts (a second switch within 1.2 s) should play a shorter, quieter version |
| **Slate / new take** | A real slate clap: sharp wooden clap + 60-100 ms of wooden resonance. Optional: a 1 kHz tone one frame long (about 42 ms) just before it | The clap exists to line up sound and picture; the "2-pop" is a 1 kHz, one-frame tone placed two seconds before the programme starts (source 9). Record it |
| **"CUT!"** | A shouted voice through a megaphone band (band-pass about 300 Hz-3 kHz, a little distortion) + a tape-stop of the game audio under it | A tape stop is a pitch-down as the "machine" slows (source 10). In Unreal the tape stop is a Sound Mix on the SFX and Music classes, not a baked file |
| **Slow-motion money shot** | Enter: a reverse swell into a low hit, 300-500 ms. During: every game sound pitched down and low-passed by a Sound Mix; the music keeps its pitch but is low-passed and ducked. Exit: a short rising whoosh back to full range | The published pattern for bullet time is: one-shot enter and exit sounds, a duck of the normal mix, pitch-down and low-pass on the world (source 11). The enter and exit are assets; the rest is mix (references/unreal-implementation.md) |
| **Perfect deflect** | The shortest money shot (0.15 s): a bright metallic ring with a hard transient; no enter sound, it has to be instant | |
| **Generator, reactor kill** | The biggest explosions in the game, with a sub drop and a long tail; all other SFX duck under them | |

## 8. Cohesion checklist (run it before asking for a listen)

- [ ] Every sound has a brief, a recipe and a source list in its folder.
- [ ] All sources are from the approved list and logged in `CREDITS.md`.
- [ ] Each repeating sound has 3-6 takes within 2 LU; each signature sound has exactly one.
- [ ] Each ship's sounds sit in that ship's slot (look at the takes side by side:
      `audio_view.py --sheet`).
- [ ] No sound in a category is more than 2 LU away from its neighbours after prep.
- [ ] Loops pass the seam check and have been listened to for ten cycles.
- [ ] Nothing imitates a named film, game or brand sound.

## Sources (read 2026-11-12 unless marked)

1. A Sound Effect, "How To Design Supreme Sci-Fi Weapon Sound Effects" (2015): real-gun structure
   first, then sci-fi layers; each layer in its own range; four to six variations; shared elements
   across related weapons. http://www.asoundeffect.com/supreme-scifi-weapon-sound-effects/
2. Splice, R. Mraz, "How to design a weapon sound for video games" (2021): mechanics, low end,
   close and far detail, tail; millisecond offsets between layers; fuller layer sets for the player
   than for others. https://splice.com/blog/?p=16617
3. A Sound Effect, B. Jacobsen, on immersion and repetition (2018): layered variants multiply
   combinations; signature sounds should stay recognisable. https://www.asoundeffect.com/game-audio-immersion/
4. Ben Burtt interview (energy sword from projector hum and television interference). http://www.filmsound.org/starwars/burtt-interview.htm
5. Blaster from a struck radio-tower guy wire. http://filmsound.org/starwars/lasergunstory.htm
6. GANG, J. Menhorn, "Game Audio Basics: Looping and Volume" (2013): split, swap halves, cross-fade
   the seam. https://www.audiogang.org/?p=1351
7. EBU Tech 3341 (loudness metering, 400 ms gating blocks). https://tech.ebu.ch/docs/tech/tech3341.pdf
8. Whoosh construction (two vendor blogs, low authority): https://violetrecording.com/how-to-make-whoosh-sounds/ ,
   https://pixflow.net/blog/cinematic-whoosh-sound-effects/
9. Clapperboard and 2-pop: https://en.wikipedia.org/wiki/Clapperboard , https://en.wikipedia.org/wiki/2-pop
10. Tape stop as a pitch-down: https://www.soundbridge.io/create-a-tape-stop-effect
11. CRI Middleware blog, bullet-time audio (2024): https://blog.criware.com/index.php/2024/06/03/bullet-time/
12. Overwatch "play by sound" (enemy and ally sounds mixed differently so the player can read the
    fight): https://www.gamedeveloper.com/audio/video-how-i-overwatch-i-was-designed-so-people-could-play-by-sound-

Not verified (page could not be fetched): the Audiokinetic UI-audio article; the DICE "HDR audio"
talk page; any written source for heartbeat or reverb swells as slow-motion conventions.
