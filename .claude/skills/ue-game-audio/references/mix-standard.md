# Mix standard for Triad Drift

What made the owner's previous game sound good was not its sound sources (all synthesised) but its
mix layer. Those rules transfer to Unreal directly; they are listed first, then turned into numbers
for this game. The numbers in sections 3-6 are **starting values chosen for this project**, not an
industry standard: tune them in the running game and let Nicholas approve the result.

## 1. Rules carried over from the previous game

Read on 2026-11-12 in the paintball project (`client/src/audio/README.md`, `soundDefs.ts`,
`gainMath.ts`, `variation.ts`, `voiceSteal.ts`, `occlusion.ts`, `docs/AUDIO_WIRING.md`):

| Rule there | Value there | In Unreal |
|---|---|---|
| Every sound's level is one number in dB relative to one anchor sound (the player's own shot = 0 dB) | Table in `soundDefs.ts`: reload -9, impacts -4 to -12, footsteps -11 to -14, UI -4 to -20, ambience -24, biggest burst +1 | `VolumeBase` (dB) on each MetaSound, from the table in section 4 |
| Source buffers are normalised the same way, so the table means something | Peak-normalised to 0.9 | `audio_prep.py`: true peak to the category target |
| The player's own actions are head-locked and a little louder than anyone else's | Own shot +2 dB, no panning, no distance loss | Attenuation with a full-level, non-spatialised inner radius that contains the player's own ship (section 5); 2D for UI and Director's Cut |
| Buses with user volume | sfx / ui / ambience, then master | Sound Classes: Master > SFX, UI, Music, Ambience, VO (features.md ART-10 asks for Master, Music, SFX, VO, UI) |
| Distance attenuation that reaches silence | Inverse curve, then a smooth fade to zero over the last 40% of the range | Sound Attenuation: falloff distance per preset; the engine cuts at the end of the falloff |
| Distance low-pass: farther is duller | Cutoff = 20 kHz / (1 + distance / half-distance), never below 700 Hz | Attenuation low-pass: 20 kHz at the inner radius down to 2.5 kHz at the far end |
| Occlusion | Up to -7 dB and a 2.5 kHz ceiling, cached and smoothed over 80 ms | Attenuation occlusion on the ground presets only (space has nothing to hide behind): low-pass 2.5 kHz, volume 0.45, 0.1 s interpolation |
| A separate "far" take beyond a distance | Duller body with a short tail, swapped in beyond 20 m, +3 dB to match | Optional, explosions only, after the first pass. Not built by the scripts |
| Random pitch and level per play | ±4% pitch, ±1.5 dB; never the same take twice in a row | MetaSound: ±0.7 semitone, ±1.5 dB, Random Get with No Repeats |
| Several takes per sound | Up to 8 numbered files per id | 3-6 takes per repeating sound |
| Tonal cues are not varied | Chimes, ticks, ambience, the rocket loop | Signature sounds: spread 0 |
| Voice cap with least-audible stealing; UI can never be stolen | 32 world voices, quietest at the listener is cut (ties: farthest, then oldest); 12 separate head-locked voices | Sound Concurrency groups with `StopQuietest` (weapons, impacts) and `StopFarthestThenOldest` (ambient world); UI and stings have no concurrency limit and higher priority |
| A master limiter that only works on pile-ups | -3 dBFS, hard knee; one own shot peaks just under it | Dynamics processor (Limiter) on the main submix at -3 dB; the anchor sound alone must not trigger it |
| Calm ambience bed | -24 dB against the anchor, about 20 dB quieter than before | Ambience class, source at -30 LUFS, never off |
| Start signals resolve musically | Countdown ticks on one note that the start chime resolves | UI and stings in the music's key |

## 2. Anchor

**The anchor is the player's primary weapon fire heard from the chase camera** (Lancer's needle
cannon is the reference because it fires most often). It is 0 dB in the table below. In the engine
it plays at `VolumeBase = -6 dB` from a file normalised to -1 dBTP, so a single shot peaks near
-7 dBFS and leaves room for overlaps before the limiter at -3 dBFS.

To use the table:
- **One-shots** (`sfx`, `ui`, `sting`; peak-normalised files): `volume_db` = `-6 + the table value`.
- **Loops and beds** (`loop`, `ambience`; loudness-normalised files): gain in dB = the table value
  itself. A weapon take at -1 dBTP measured about -16 LUFS (ungated) in the test for this skill, so
  at -6 dB the anchor sits near -22 LUFS, about where a -23 LUFS loop file sits at 0 dB.
- These are starting points. One-shots and loops are not measured the same way; the ear decides.

## 3. Source-file targets (what `audio_prep.py` produces and `audio_report.py` checks)

All files: 48 000 Hz (the engine's mixer runs at 48 kHz on this machine: editor log, 2026-11-12),
16-bit PCM WAV (Epic's import documentation says imports become 16-bit and recommends 16-bit
sources), no clipping, DC offset at most 0.002.

| Category | For | Channels | Level | True peak | Loudness | Silence at start / end | Other |
|---|---|---|---|---|---|---|---|
| `sfx` | 3D one-shots: weapons, impacts, explosions, blade, footsteps, enemies | mono | true peak to -1 dBTP | -3 to -0.9 | not judged (too short) | 5 ms / 50 ms | 0.03-8 s |
| `ui` | HUD and menu one-shots | mono or stereo | true peak to -3 dBTP | -5 to -2.9 | not judged | 5 ms / 50 ms | up to 3 s |
| `sting` | 2D cinematic one-shots: switch cut, "CUT!", slate, slow-mo in / out, drop-out, stingers | mono or stereo | true peak to -1 dBTP | -3 to -0.9 | not judged | 10 ms / 100 ms | up to 12 s |
| `loop` | 3D loops: engine, boost, drift, dome, portal idle, charge, alarm | mono | -23 LUFS | at most -3 | -24.5 to -21.5 | 1 ms / 1 ms | seam step at most -40 dBFS, ends within 1.5 dB, 2-30 s |
| `ambience` | 2D beds | stereo | -30 LUFS | at most -6 | -32 to -28 | 1 ms / 1 ms | seam checks, 20-180 s |
| `music` | Each stem or track | stereo | true peak to -1 dBTP | at most -0.9 | each stem at most -14 LUFS | not judged | all stems the same length (`--same-length`); the sum of all stems -16 LUFS ±2 |
| `vo` | Barks | mono | -19 LUFS | at most -1 | -21 to -17 | 50 ms / 150 ms | |

Why two ways of levelling: a one-shot is too short for a loudness measurement to mean much (see
sound-design.md), so one-shots are levelled by peak and balanced by the table; anything long is
levelled by loudness. Takes of one sound must stay within 2 LU of each other.

Loudness units: LUFS per ITU-R BS.1770 / EBU R 128; true peak in dBTP (oversampled peak).

## 4. In-game levels, in dB relative to the anchor

| Group | Sound | dB | Notes |
|---|---|---|---|
| Player weapons | Primary fire (anchor) | 0 | |
| | Secondary fire (rocket, flak, splitter) | +1 | Fires less often, may be bigger |
| | Charge loop | -10 | |
| | Overheat | -4 | |
| Player powers | Ram, missile swarm launch, portal blaster | +2 | The signature moments |
| | Power wind-up | -3 | |
| | Shield dome loop | -14 | |
| | Ability activate (Cloak, EMP, Gravity Well) | +1 | |
| Hits the player lands | Impact on hull | -6 | |
| | Impact on shield | -7 | |
| | Hit-confirm tick (UI) | -8 | Kill confirm -5 |
| Hits the player takes | Hull hit | -3 | Must be noticed |
| | Shield hit | -5 | |
| | Shield break | -1 | |
| | Low-hull alarm loop | -12 | Never a constant siren: gate it |
| Explosions | Fighter, small | -1 | 3D: distance does the rest |
| | Gunship, bomber | +1 | |
| | Carrier, generator | +3 | SFX ducks under it |
| | Reactor, breakup | +4 | The loudest thing in the game |
| Enemies | Weapons | -6 | 3D |
| | Telegraph (wind-up before a big attack) | -3 | Must cut through |
| | Missile-lock warning (UI) | -5 | |
| | Engines, pass-bys | -12 | |
| Engines | Player engine loop | -14 at rest, -9 at full speed | The MetaSound's gain range |
| | Boost start | -4 | |
| | Wingman engines | (same asset) | Attenuation makes them quieter |
| Travel | Hyperdrive spool | -4 | |
| | Hyperdrive tunnel loop | -8 | |
| | Hyperdrive drop-out | -1 | |
| | Portal open / transit / close | -3 / -2 / -5 | |
| | Portal idle loop | -16 | |
| Director's Cut | Switch cut | -4 | Frequent: it must read without tiring |
| | Slate (take start) | -3 | |
| | "CUT!" | -1 | Everything else is ducked |
| | Slow-mo enter / exit | -4 / -6 | |
| On foot | Blade ignite / retract | -4 / -8 | |
| | Blade swing | -8 | |
| | Blade hit, block | -3, -4 | |
| | Deflect / perfect deflect | -3 / -1 | |
| | Footsteps | -16 | |
| | Crew weapons | -6 | 3D |
| UI | Click / hover / confirm / back | -12 / -20 / -10 / -12 | |
| | Banner (wave, ability ready) | -6 | |
| | Ability unlocked (stinger) | -3 | |
| Beds | Ambience | -24 | Always on |
| Music | See section 6 | | |

## 5. Presets

### Attenuation (distances in cm; the chase camera sits 1400 cm behind the player's ship, `ATDShip` `ArmLength`)

| Preset | Full level inside | Silent beyond (inside + falloff) | Low-pass at the far end | Non-spatialised inside | For |
|---|---|---|---|---|---|
| `ATT_TD_Space_Near` | 2 500 | 20 000 | 4 kHz | 2 500 | Engines, small ship sounds, impacts |
| `ATT_TD_Space_Medium` | 3 000 | 60 000 | 2.5 kHz | 2 500 | Weapons, small explosions (weapon range is 30 000) |
| `ATT_TD_Space_Far` | 8 000 | 200 000 | 1.5 kHz | 2 500 | Big explosions, carrier, Dominion events |
| `ATT_TD_Ground_Near` | 300 | 3 000 | 3 kHz | 150 | Footsteps, blade, small crew sounds; occlusion on |
| `ATT_TD_Ground_Medium` | 600 | 8 000 | 2.5 kHz | 150 | Crew weapons, drones, hazards; occlusion on |
| (none) | - | - | - | - | UI, Director's Cut, music, ambience, hyperdrive: 2D |

The inner radius is what makes the player's own ship "head-locked": everything attached to it is
inside the full-level, non-spatialised zone, while the same asset on a wingman or an enemy is
attenuated and panned. **A 3D sound without an attenuation asset plays at full level wherever it
happens**, because gameplay plays cues with no override (references/unreal-implementation.md).

### Concurrency

| Preset | Max voices | When full | Retrigger | For |
|---|---|---|---|---|
| `SCON_TD_Weapon` | 6 | Stop quietest | 0.03 s | Each weapon's fire (set "limit to owner" off: the group is global) |
| `SCON_TD_Impact` | 8 | Stop quietest | 0.02 s | Impacts on hull, shield, world |
| `SCON_TD_Explosion` | 4 | Stop quietest | 0.05 s | Explosions |
| `SCON_TD_Loop` | 12 | Stop farthest, then oldest | - | Engine and ability loops |
| `SCON_TD_Footstep` | 4 | Stop oldest | 0.05 s | Footsteps |
| (none) | - | - | - | UI, stings, music, ambience |

The engine's overall voice count is a project setting (Ryan's file); these groups keep any one
kind of sound from taking it over, which is what the previous game's 32-voice cap did.

### Sound classes

```
SC_TD_Master
├── SC_TD_SFX        weapons, impacts, explosions, engines, travel, blade, crew
├── SC_TD_UI         HUD, menus                     (bIsUISound: plays while paused)
├── SC_TD_Cine       Director's Cut stings          (its own class: no slow-motion or "CUT!" mix lists it)
├── SC_TD_Ambience   beds
├── SC_TD_Music      all music                      (bIsMusic)
└── SC_TD_VO         barks
```
Settings sliders (ART-09) drive the class volumes. `SC_TD_Cine` is separate so that mixes which
slow or duck the game never touch the sounds that announce them.

## 6. Music, ducking and the master

- **Whole mix:** integrated loudness **-18 LUFS ±2** over at least a few minutes of representative
  play, true peak at most **-1 dBTP**. Published targets for comparison: Sony ASWG-R001 gives
  -24 LUFS ±2 for home consoles and -18 ±2 for portables, true peak -1 dBTP, measured over at least
  30 minutes of the whole mix; the GANG IESD recommendation gives -24 for console and -16 for
  mobile. There is no formal PC standard; -18 is this project's choice for a jam build played on
  desktop speakers and headphones. `audio_report.py --mix` checks a captured WAV against it.
- **Music level:** with all layers playing, the music sits about 8 dB under the combat SFX bus:
  source stems sum to -16 LUFS and the Music class runs at about -10 dB. In calm scenes the same
  setting makes music the foreground. Tune by ear with combat running.
- **Ducking (Sound Mixes, pushed for the duration):**

| Trigger | What changes | Amount | In / out |
|---|---|---|---|
| VO bark playing | Music | -6 dB | 0.08 s / 0.35 s (the starter plan's values) |
| Carrier, generator, reactor explosion | SFX and Music | -4 dB for 0.8 s | 0.02 s / 0.5 s |
| Slow-motion money shot | SFX: pitch ×0.6, low-pass 1.2 kHz, -2 dB. Music: low-pass 2.5 kHz, -4 dB. Cine, UI: untouched | | 0.08 s / 0.25 s |
| "CUT!" | SFX and Ambience: pitch falls to ×0.5 and -12 dB (the tape stop). Music: -9 dB | | 0.4 s / 0.3 s at the next take |
| Pause menu | SFX, Ambience: -inf (paused by the engine); Music: -6 dB, low-pass 1.5 kHz | | 0.2 s |

- **Master:** a limiter on the main submix, threshold -3 dB, fast attack (about 2 ms look-ahead),
  release 100 ms. It is a safety net for pile-ups. If it works all the time, the SFX class is too
  loud: turn the class down, do not raise the threshold.
- **Headroom:** no single sound may reach -3 dBFS on its own except the reactor explosion.

## 7. Listening checklist (in the running game, on the real map)

An agent cannot do this part. A human listens, on headphones and on small speakers, at a normal
volume, for at least one full wave. Record the result in `DEVLOG.md`.

- [ ] Fire the primary weapon for ten seconds: does it stay pleasant, or does it grate or machine-gun?
- [ ] Switch ships five times: can you tell which ship you are in with your eyes shut?
- [ ] Is your own ship always the loudest and clearest thing, and centred?
- [ ] Wingmen and enemies: quieter with distance, duller far away, coming from the right side?
- [ ] An enemy telegraph during a dogfight: do you hear it in time to react?
- [ ] The biggest explosion: bigger than everything else, without distortion or pumping?
- [ ] Twenty enemies on screen: still clear, or mush? (If mush: lower the concurrency counts.)
- [ ] Music: audible in combat without masking the weapons; does it rise with the wave and calm after?
- [ ] Slow motion: does the world drop in pitch and go dull, and come back cleanly?
- [ ] "CUT!" and the slate: unmistakable, and not annoying the fifth time?
- [ ] Any click, pop, cut-off tail or loop seam anywhere? Any silence where something should sound?
- [ ] Ten minutes in: are your ears tired? (Harshness lives at 2-5 kHz; turn that down first.)
- [ ] Packaged build, not only the editor: same result?

## Sources

- Sony ASWG-R001 v1.10 (2013), read as a PDF on 2026-11-12 from a third-party host
  (http://gameaudiopodcast.com/ASWG-R001.pdf; no official public Sony URL was found).
- GANG IESD Mix Recommendations v03.02 (2015), https://www.audiogang.org/wp-content/uploads/2015/04/IESD-Mix-Ref-Levels-v03.02.pdf
- EBU Tech 3341 (2023), https://tech.ebu.ch/docs/tech/tech3341.pdf ; ITU-R BS.1770, https://www.itu.int/rec/R-REC-BS.1770
- Epic, importing audio files, https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-audio-files
- The paintball project files named in section 1 (the owner's own code).
