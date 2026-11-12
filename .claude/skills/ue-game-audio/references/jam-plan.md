# Jam plan: what to make, in what order, with what

Written 2026-11-12. Feature freeze is Nov 15, 12:00 PM EST; submission closes Nov 15, 8:59 PM EST.
Audio is a judged category and today the game is silent, so the order below is "what a judge hears
in the first two minutes and in the first five seconds of the gameplay video", not completeness.

Time figures are estimates for one agent working the pipeline in SKILL.md, including one revision
after Nicholas listens. They are not measured: nobody has yet made a finished sound with this
skill. Re-plan after the first three sounds.

## Step 0: setup (about 3 hours, mostly waiting on people)

| Task | Who | Time |
|---|---|---|
| Nicholas picks the sources from the options table (sourcing.md, last section); a human downloads the Sonniss GDC 2026 parts and, if chosen, activates the AI plan | Nicholas | 30 min + download |
| Ask a human whether the repo is private; write the answer in `ArtSource/Audio/README.md` | Lead | 5 min |
| Search the bundle's track list for: sci-fi weapons, gun mechanics, explosions, metal impacts, whooshes, electric hum, jet or turbine, UI | agent | 30 min |
| Mix assets from one spec: 6 sound classes, 5 attenuation presets, 5 concurrency presets, `MIX_TD_SlowMo`, `MIX_TD_Cut`, `MIX_TD_DuckForVO` (mix-standard.md §5-6) | agent | 30 min |
| `BP_TD_AudioDirector` skeleton: music component, event bindings, mix push / pop, limiter (unreal-implementation.md §8); claim and place it in `L_Galaxy` | agent + Lead | 90 min |
| Post the four requests for Ryan on the message board (parameters, `EngineLoop`, plugin, limiter) | Lead | 10 min |
| `CREDITS.md` rows for the bundle, the AI tool, ffmpeg | agent | 10 min |

## Tier 1: the first impression (20 sounds, about 17 agent-hours + 3 hours of listening)

| # | Sound | Slot | Source first | Method | Est. |
|---|---|---|---|---|---|
| 1 | **Lancer primary fire** (the anchor) | `DA_NeedleCannons_FX.Muzzle` | Library: gun mechanism or snap for the transient, sci-fi zap for the body | 3 layers, 5 takes, 150-250 ms, dry. Everything else is levelled against this | 60 min |
| 2 | Rift primary fire | `DA_PhasePulseLasers_FX.Muzzle` | Library zap + glass or phase layer; AI SFX body if the library has none | 3 layers, 4 takes, rounded attack, rising bend | 40 min |
| 3 | Bastion primary fire | `DA_BreakerBlaster_FX.Muzzle` | Library low-mid thump + mechanical transient | 3 layers, 4 takes, falling pitch, short boom | 40 min |
| 4 | Impact on hull | `ImpactHull` on all six weapon assets | Library metal impact + debris tick | 2-3 layers, 5 takes, under 300 ms | 35 min |
| 5 | Impact on shield | `ImpactShield` | Library glass or resonant metal + AI SFX energy shimmer | 2 layers, 5 takes | 35 min |
| 6 | Fighter explosion | `DA_Enemy_Fighter_Visuals.Explosion` | Library explosion + debris + a sine sub drop | 4 layers, 4 takes, 1-1.5 s | 45 min |
| 7-9 | **Switch cut** ×3 | `DA_<Ship>_Visuals.SwitchIn` | Library whoosh + mechanical click, shared; one tonal layer per ship | One recipe, three variants; no random variation; 250-350 ms; 2D | 90 min |
| 10-12 | Engine loop ×3 | `DA_<Ship>_Visuals.EngineTrail` | Library jet / turbine / electric motor loop | `--loop-crossfade`; MetaSound loop with `Speed01`; one source may serve two ships at different pitch and filter | 120 min |
| 13 | Space music, 3 layers | audio director | Stable Audio 3 or Eleven Music: three stems, one BPM and key | Cut to 16 bars; `--same-length`; music MetaSound; intensity from `OnWaveStarted` | 180 min |
| 14 | Space ambience bed | audio director | AI SFX or Stable Audio: low room tone, distant shimmer | 40-60 s loop, -30 LUFS, stereo | 30 min |
| 15 | **"CUT!"** | audio director on `OnSceneCut` | Record a team member | Megaphone band-pass, slight distortion; with `MIX_TD_Cut` (tape stop) | 40 min |
| 16 | **Slate clap** | audio director on `OnTakeStarted` | Record it (or library) | Clap + wooden resonance; one take | 20 min |
| 17 | Slow-motion enter and exit + `MIX_TD_SlowMo` | audio director on the director shot | Library or AI SFX reverse swell and low hit; rising whoosh | Two stings; the mix does the rest | 45 min |
| 18 | Player hull hit (taken) | `DA_<Ship>_Visuals.HullHit` | Library metal impact, lower and closer than #4 | 2 layers, 4 takes; one asset for all three ships first | 30 min |
| 19 | UI click + confirm | widgets | Library UI, one key | Two sounds from one source family | 30 min |
| 20 | Boost start | `DA_<Ship>_Visuals.BoostStart` | Library whoosh + engine burst | 2 layers, 3 takes; shared by the three ships first | 30 min |

Listening sessions with Nicholas: after #1 (the anchor sets everything), after #9, after #14, after #20.
Each is a build, a five-minute play, and the checklist in mix-standard.md §7.

## Tier 2: the rest of the core loop (about 14 agent-hours)

| Group | Sounds | Notes |
|---|---|---|
| Signature powers | Lancer ram (wind-up, hit), Rift portal blaster (fire, portal open, transit, close, idle loop), Bastion missile swarm (launch, per-missile whoosh), shield dome (up, loop, down) | The video's money shots: do the ram and the portal transit first |
| Hyperdrive | Spool (riser), tunnel loop, drop-out | AI SFX does risers and beds well |
| Secondary weapons | SplitterBolt, SpikeRockets (+ `Detonation`), FlakBurst | Derive from the primaries: same transient family |
| Enemy | Fighter and interceptor fire, **telegraph**, missile-lock warning, warp-in, gunship and carrier explosions | Telegraph and warning first: they are gameplay information |
| Damage states | Shield hit and shield break (taken), low-hull alarm, ship destroyed, respawn warp-in | |
| Dominion | Shield hit, generator explosion ×1 (used four times), reactor exposed, reactor destroyed, breakup | The reactor kill is the loudest sound in the game: give it the time |
| Abilities | Cloak, EMP Pulse, Gravity Well: activate (+ loop, end) | |
| Stingers | Ability unlocked, wave start, wave cleared, marker revealed, victory | In the music's key |
| On foot | Blade: ignite, hum loop, swing ×2, hit, block, deflect, perfect deflect; footsteps (one surface) | Perfect deflect is a money shot |
| Music and beds | Verdance and Cinder beds and music, Dominion music, menu, victory | Reuse the space stems' tempo and key so transitions are easy |

## Tier 3: only if everything above is approved

Per-surface footsteps, crew barks (ART-11), far takes for explosions, beat-quantised stingers
(Quartz), per-ship versions of the shared tier-1 sounds, drift loop and release, collision scrape,
atmospheric entry, landing and take-off, docking bay.

## Calendar

| When | Goal |
|---|---|
| Nov 12 (today) | Step 0 decisions and setup; sound #1 heard in the game |
| Nov 13 | Tier 1 complete and listened to; first whole-mix pass |
| Nov 14 | Tier 2; second mix pass; a listen in a packaged build |
| Nov 15 until 12:00 EST | Fixes from listening only; final mix pass; `CREDITS.md` complete |
| After the freeze | Level changes only (a number in a spec, re-run, listen). No new sounds |

## If time runs out

Cut from the bottom of tier 2 upward, never from tier 1. A game with twenty good sounds, music and
no silence scores better than one with eighty uneven sounds. A slot that has no approved sound stays
**empty** rather than getting a placeholder: silence in one slot is better than a rejected sound,
except for the anchor, impacts, explosions and music, which must exist.
