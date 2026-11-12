# Project conventions for Triad Drift audio

Audio is ART-10 (and ART-11 for voice) in `features.md`, owned by Nicholas's side. Read `AGENTS.md`
§6 (git protocol, `CLAIMS.md`, `MESSAGEBOARD.md`) before touching any `.uasset`: binaries cannot be
merged, so every asset is claimed before it is edited, and workers never pull, push or merge.

## Folders

```
ArtSource/Audio/                                  sources and working files (Nicholas's folder)
  <Category>/<Asset>/
    brief.md                                      5-line brief (sound-design.md §1)
    source/                                       the clips actually used (see "third-party files" below)
    recipe.json                                   audio_layer.py recipe, if layered
    export/                                       <Name>_01.wav … + .prep.json / .layers.json
    review/                                       audio_view.py pictures, audio_report JSON
  specs/                                          ue_audio_build.py and ue_audio_check.py spec files
  StarterPlan/                                    the earlier plan and 32-cue manifest (reference only)

Content/TriadDrift/Audio/                         Unreal assets (Nicholas's folder)
  Mix/                                            SC_TD_*, ATT_TD_*, SCON_TD_*, MIX_TD_*, limiter preset
  SFX/Weapons  SFX/Ships  SFX/Engines  SFX/Impacts  SFX/Explosions  SFX/Travel  SFX/Abilities
  SFX/Enemies  SFX/Dominion  SFX/Ground  SFX/UI  SFX/Cine
  Ambience/  Music/  VO/
  BP_TD_AudioDirector                             music, mixes, limiter (unreal-implementation.md §8)

Content/TriadDrift/Presentation/DA_*              the data assets gameplay reads (always cooked)
```
`<Category>` under `ArtSource/Audio/` mirrors the `SFX/` sub-folders, plus `Music`, `Ambience`, `VO`.
Never write under `Source/`, `Config/`, `Content/TriadDrift/Gameplay/` or `Tools/` (Ryan's).

**Third-party files:** whole downloaded libraries stay outside the repo. Whether the clips you
actually use may be committed under `source/` depends on the licence and on whether the repo is
private (sourcing.md, rule 4): ask a human once, write the answer at the top of
`ArtSource/Audio/README.md`, and follow it. If the answer is no, keep `source/` out of git and
record each clip's library, pack and file name in `brief.md` so the sound can be rebuilt.

## Names

| Asset | Pattern | Example |
|---|---|---|
| Source take (WAV) | `SFX_<Group>_<Thing>_<NN>.wav`, `MUS_<State>_L<n>_<Part>.wav`, `AMB_<Place>.wav`, `VO_<Speaker>_<BarkId>_<NN>.wav` | `SFX_Weapon_Lancer_Cannon_03.wav` |
| Sound Wave | `SW_` + the take name without its `SFX_` prefix | `SW_Weapon_Lancer_Cannon_03` |
| MetaSound Source | `MSS_<Group>_<Thing>` (the engine's own prefix, as in `MSS_PlayRandom_Oneshot`) | `MSS_Weapon_Lancer_Cannon`, `MSS_Engine_Bastion`, `MSS_Music_Space` |
| MetaSound Patch | `MSP_<Purpose>` | `MSP_TD_RandomPitch` |
| Sound Class | `SC_TD_<Bus>` | `SC_TD_SFX` |
| Attenuation | `ATT_TD_<Space>_<Range>` | `ATT_TD_Space_Medium` |
| Concurrency | `SCON_TD_<Group>` | `SCON_TD_Weapon` |
| Sound Mix | `MIX_TD_<Purpose>` | `MIX_TD_SlowMo`, `MIX_TD_Cut`, `MIX_TD_DuckForVO` |
| Data asset | fixed in `docs/visual/Presentation-Asset-Paths.md` | `DA_Lancer_Visuals`, `DA_NeedleCannons_FX` |

Groups: `Weapon`, `Ship`, `Engine`, `Impact`, `Explosion`, `Travel`, `Ability`, `Enemy`, `Dominion`,
`Blade`, `Foot`, `Crew`, `UI`, `Cine`. Use the gameplay names: ships `Rift`, `Lancer`, `Bastion`;
weapons `PhasePulseLasers`, `SplitterBolt`, `NeedleCannons`, `SpikeRockets`, `BreakerBlaster`,
`FlakBurst`; abilities `Cloak`, `EMPPulse`, `GravityWell`.

## Slot map: which sound goes in which `FTDFXCue` slot

From `Source/unrealGameJam/TriadDrift/Core/TDPresentation.h` (read 2026-11-12). Every slot is
optional; an empty slot does nothing. "3D" = needs an attenuation preset; "2D" = none.

| Data asset (class) | Slot | Sound | Category | Space |
|---|---|---|---|---|
| `DA_<Ship>_Visuals` (`UTDShipVisuals`) | `EngineTrail.Sound` | Engine loop (MetaSound loop with `Speed01`) | loop | 3D near |
| | `BoostStart` / `BoostLoop` | Boost kick / boost loop | sfx / loop | 3D near |
| | `DriftLoop` / `DriftRelease` | Drift screech loop / release burst | loop / sfx | 3D near |
| | `CollisionScrape` | Hull scrape | sfx | 3D near |
| | `SwitchIn` | **The switch cut for this ship** | sting | 2D |
| | `SwitchOut` | Empty (or a quiet departure) | | |
| | `PowerWindup` / `PowerActive` / `PowerEnd` / `Power2Active` | Signature power: portal blaster (Rift), afterburner ram (Lancer), missile swarm and shield dome (Bastion) | sfx (dome: loop) | 3D medium |
| | `ShieldHit` / `ShieldBreak` / `HullHit` | Hits the ship takes | sfx | 3D near |
| | `LowHullLoop` | Low-hull alarm | loop | 3D near (2D reads better: ask) |
| | `Explosion` / `RespawnWarpIn` | Ship destroyed / respawn | sfx | 3D medium |
| | `EngineLoop` | **Unused by gameplay at `f0bee9c`**: leave empty | | |
| `DA_<Weapon>_FX` (`UTDWeaponFX`) | `Muzzle` | Fire | sfx | 3D medium |
| | `ChargeLoop` / `Overheat` | Charge / overheat | loop / sfx | 3D near |
| | `ImpactHull` / `ImpactShield` / `ImpactWorld` | Impacts | sfx | 3D near |
| | `Detonation` / `Deflected` | Rocket or flak burst / deflected shot | sfx | 3D medium |
| `DA_Enemy_<Type>_Visuals` (`UTDEnemyVisuals`) | `WarpIn`, `Telegraph` and the per-attack telegraphs, `ShieldHit`, `HullHit`, `Stunned`, `Explosion`, `DamagedSmoke`, `DeathTumble`, carrier `BayLaunch*`, `BayDestroyed`, `TurretDestroyed`, `FrontalShieldBreak` / `Restore` | Enemy sounds | sfx | 3D medium (carrier events: far) |
| `DA_Crew_<Type>_Visuals` (`UTDEnemyVisuals`) | `Telegraph`, `HullHit`, `Stagger`, `Death` | Crew on foot | sfx | 3D ground |
| `DA_Dominion_Visuals` (`UTDDominionVisuals`) | `WarpIn`, `ShieldIdle`, `ShieldHit`, `ShieldSectionDown`, `ShieldCollapse`, `GeneratorHit`, `GeneratorExplosion`, `ReactorExposed`, `ReactorHit`, `ReactorPhaseChange`, `ReactorDestroyed`, `BreakupSequence`, `DockingBayOpen` | Finale | sfx / loop | 3D far |
| `DA_Ability_<Name>_FX` (`UTDAbilityFX`) | `Activate`, `Loop`, `End`, `AffectedTarget` | Cloak, EMP Pulse, Gravity Well | sfx / loop | 3D medium |
| `DA_Travel_FX` (`UTDTravelFX`) | `HyperdriveSpool`, `HyperdriveTunnel`, `HyperdriveDropOut` | Hyperdrive | sting / loop / sting | 2D |
| | `PortalOpen`, `PortalIdle`, `PortalTransit`, `PortalClose` | Portals | sfx / loop / sting / sfx | 3D medium (transit 2D) |
| | `AtmosphericEntry`, `LandingDust`, `TakeOff`, `EnemyWarpIn` | Planet travel, enemy arrival | sfx | 2D / 3D |
| `DA_Blade_FX` (`UTDBladeFX`) | `Ignite`, `Retract`, `SwingLight`, `SwingHeavy`, `Hit`, `Block`, `Deflect`, `PerfectDeflect`, `GuardBreak`, `AirSlam` | Energy blade | sfx | 3D ground near (first person: inside the inner radius) |

Not on a cue slot, so played by the audio director (unreal-implementation.md §8): music, ambience
beds, the slate, "CUT!", slow-motion stings and mixes, stingers (ability unlocked, marker revealed,
wave banners), UI sounds played by widgets, voice barks.

The earlier manifest (`ArtSource/Audio/StarterPlan/cues.json`, 32 cues, all unacquired) maps like
this: `Engine_*` → `EngineTrail`; `Switch_*` → `SwitchIn`; `Fire_*` → the weapon's `Muzzle`;
`Impact_Hull` / `_Shield` / `_Rock` → `ImpactHull` / `ImpactShield` / `ImpactWorld`;
`Hyperdrive_Spool` / `_Travel` / `_Exit` → the three hyperdrive slots; `PortalOpen` / `PortalClose`
→ the portal slots; `Explosion` → the `Explosion` slots; `UI_*` and `Warning_*` → widgets or the
audio director; `Music_*` → the audio director.

## Credits: log every source when it is acquired

`CREDITS.md` at the repo root is the submission's asset list (jam rule). Add at the bottom only.

Tools (once per tool, under "Tools used"):
```
- <Tool and plan> (<vendor>): <what it was used for>; <who>, <date>. Terms read <date>: <URL>.
```
Assets (one row per pack or per original recording):
```
| <Asset or pack, with pack/file names used> | <Source URL / author / tool and model> | <Licence name, version, date read; attribution line if required> | <Nicholas / agent name> |
```
Examples of what a row must make clear: for a library, the bundle, the licence version and the
download date; for an AI generation, the tool, model, plan and the prompt's location
(`brief.md`); for a recording, who performed it; for a designed sound, the layers' sources.
ffmpeg and Python are tools too (already listed in spirit; add ffmpeg). The earlier rejected
synthesised candidates stay credited where they are.

Pre-jam declaration: this skill and its scripts were written during the jam (2026-11-12); nothing
audio existed before it.

## Honest stages (`AAA.md` §3) for an audio asset

Every report names the stage reached, in these words:

| Stage | Means, for audio | Evidence |
|---|---|---|
| **Prototype complete** | Takes exist; brief, recipe and sources are written | Files in `export/`; `audio_report.py` passes the category |
| **Technically verified** | Imported, MetaSound built, assets validate, the asset renders non-silent, varies, does not clip; the slot is set | `ue_audio_build.py` result, `ValidateAssets`, `ue_audio_check.py` render + report + picture |
| **Presentation approved** | **Nicholas has listened to it in the running game and approved it.** Nothing else counts: not a measurement, not a picture, not an agent's opinion | His words, quoted with the date, in `docs/visual/STATUS.md` |
| **Integrated** | On `main` through his Lead, referenced from a `DA_*` slot or the audio director, cooks | The `main` commit |
| **Gameplay validated** | Heard in context in a playtest of real play (packaged at least once), with the checklist in mix-standard.md §7 | Dated `DEVLOG.md` entry: who or what played, what was heard |

An agent can reach "technically verified" alone. It never writes "sounds good", "professional" or
"AAA" about its own work; it writes what it measured and what a human still has to hear.

## Working rules

- Before work: `git fetch`, read `CLAIMS.md` and `MESSAGEBOARD.md` on `origin/main`; the Lead adds
  the task row and the claims (`Content/TriadDrift/Audio/<area>/*`, the `DA_*` assets touched).
- One heavy application at a time on Nicholas's machine (docs/visual/STATUS.md): the ffmpeg scripts
  are light and can run any time; an Unreal editor is the heavy slot.
- Commit message: `audio: <what changed>`. Stage only your files. Never commit `Saved/`
  (rendered checks live there) or downloaded libraries.
- Questions for Ryan's side (parameters, plugin, limiter, the unused `EngineLoop`) go through the
  Lead onto `MESSAGEBOARD.md`.
- Add a short `DEVLOG.md` entry for each batch: what was built, with which tools, what the checks
  showed, what is waiting for a listen. The dev log is judged (Glass Box prize).
