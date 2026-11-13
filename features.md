# Triad Drift: Feature Spec & Work Split (Ryan × Nick)

> **Working title:** Triad Drift · **Jam:** 2026 Epic AI MegaJam · **Engine:** UE 5.9 (MegaJam Preview), project `unrealGameJam`
> **Theme:** **DIRECTOR'S CUT** (how the game expresses it: C-03)
> **Hard deadline:** Nov 15, 2026, **8:59 PM EST / 5:59 PM PST**. Late entries are disqualified.
> **Feature freeze:** Nov 15, 12:00 PM EST. After that, only fixes, polish, packaging and submission.

This file splits the game into features and assigns each one to **Ryan (Gameplay & Engineering)** or
**Nick (Art, Visuals & Player Experience)**, following the team role briefs (Nov 12). Read §1–§3 first:
the split, folder ownership, and the contracts that let the two of you work at the same time without
editing the same files.

## Scope
- **Core (must ship):** everything not tagged **STRETCH**. That includes:
  - flight, instant three-ship switching, AI wingmen and combat
  - enemies and waves, hyperdrive and portals
  - **all signature powers and unlockable abilities**
  - **both landable planets** with their on-foot objectives
  - the Dominion space-assault finale
  - full presentation (art, VFX, UI, audio) and packaging
- **STRETCH (optional; only once core is solid):** the **boarding finale**: docking, the Dominion
  interior on foot, the reactor-core fight, the escape run and the escape flight. Without it, the war
  ends in space when the squad destroys the exposed reactor from outside (ENG-18).
- **Added Nov 12 (Ryan), core:** a Settings page and an on-foot first / third-person camera choice (ENG-32).
- **Added Nov 12 (Ryan), core:** a high-quality cinematic intro of space, the squad, worlds and the Armada (ENG-33).
- **Added Nov 13 (Ryan), core:** both planets become large explorable worlds with procedural terrain and mountains (ENG-34), and engineering supplies a fallback sound set so the game is never silent (ENG-35).
- **Added Nov 13 (Ryan), core:** the HUD is redesigned: modern, sleek, small, and clear to a beginner (ENG-36).
- **Added Nov 13 (Ryan), core:** AAA projectiles, engine flames and particle effects across the whole game (ENG-37).
- **Protect above all:** the three-ship switching mechanic and satisfying combat.
- **C++ first;** use Blueprints when they are clearly faster or necessary.
- Keep a playable build at all times, coordinate shared-file edits before starting, verify licences
  for third-party resources, and freeze major features before final submission.

All numbers are **starting tuning values** (Unreal units: cm, cm/s, seconds). Keep them in data
assets or tables, not hard-coded.

**Status key** (edit inline): `⬜ todo` · `🟨 in progress` · `✅ done` · `🐞 has bugs`

**Engineering status (Nov 12, evening):** every core `ENG-xx` item is on `main` with automated tests and has
had two independent reviews; none is `✅` yet, because nothing has been played by a human. Item by item, with
open findings: `docs/eng/STATUS.md`. Rules decided during the day are written into the items below and marked
"(Nov 12)".

### Contents
0. [The game](#0-the-game) · 1. [The split](#1-the-split) · 2. [Folder & file ownership](#2-folder--file-ownership) ·
3. [Shared contracts](#3-shared-contracts) · 4. [Shared setup](#4-shared-setup-c-xx) ·
5. [🟦 Ryan: Gameplay & Engineering](#5--ryan-gameplay--engineering) ·
6. [🟩 Nick: Art, Visuals & Player Experience](#6--nick-art-visuals--player-experience) ·
7. [Landing: full sequence](#7-landing-full-sequence-core) · 8. [Boarding: full sequence](#8-boarding-full-sequence-stretch) ·
9. [Integration points](#9-integration-points) · 10. [Dependency order](#10-dependency-order-no-dates) ·
11. [Bugs & requests](#11-bugs--requests-append-only)

---

## 0. The game

A fast, bright, arcade space-combat game. You lead a wing of three ships (**Rift**, **Lancer** and
**Bastion**) and switch between them at any moment, while the two ships you aren't flying fight on
their own. You land on two worlds and complete objectives on foot to unlock new ship abilities, then
break the enemy mothership's defenses. As a stretch goal, you board it with an **energy blade** to
destroy its core.

### Fiction (working names: rename freely, but update this file)
- **The squad:** three pilots, one per ship. **Wren** flies **Rift** (playful, sly). **Kato** flies
  **Lancer** (cocky, fast-talking). **Moss** flies **Bastion** (calm, protective veteran). The player
  is the **wing lead**, who can take any cockpit.
- **The enemy:** the **Null Armada**, angular red-and-black ships with hollow white "eye" lights. Its
  mothership is the **Dominion**.
- **The worlds:**
  - **Verdance:** a bioluminescent jungle with giant flora, waterfalls and a ringed moon.
  - **Cinder:** black volcanic rock, glowing magma and huge cyan/amber crystal spires.

### Player journey

```
Main Menu ─► Intro (squad briefing, Armada arrives)
  └─► GALAXY · Waves 1–2: learn flight, shooting, switching, each signature power
        └─► Verdance: hyperdrive → atmospheric entry → fly down → land                (CORE)
              ├─► Obj 1 "Recover the Wreck Core" → unlock GRAVITY WELL (Bastion)
              └─► Obj 2 "Wake the Ancient Relay"  → unlock CLOAK (Rift) → take off
  └─► GALAXY · Waves 3–4 (gunships, bombers; portal gates matter)
        └─► Cinder: land                                                              (CORE)
              ├─► Obj 3 "Clear the Armada Outpost" → unlock EMP PULSE (Lancer)
              └─► Obj 4 "Destroy the Signal Jammer" → REVEALS THE DOMINION → take off
  └─► GALAXY · Wave 5 "Dominion's Guard" (carriers) → Dominion warps in (cinematic)
        └─► Destroy 4 shield generators + turrets → reactor exposed
              ├─► CORE ENDING: destroy the exposed reactor from space → breakup cinematic → Victory
              └─► STRETCH ENDING: dock → interior on foot (energy blade) → core fight → escape run
                                  → escape flight → breakup cinematic → Victory
```
Target length: 20–30 minutes.

---

## 1. The split

| | 🟦 **Ryan: Gameplay & Engineering Lead** | 🟩 **Nick: Art, Visuals & Player Experience Lead** |
|---|---|---|
| **AI assistant** | Claude Code | OpenAI Codex |
| **Mission** | Own the playable game systems, engineering integration and technical reliability, so the game stays fun, responsive and shippable | Own the visual identity, presentation quality and player-facing feel, compatible with the gameplay foundation and the 4-day window |
| **Owns** | Flight and controls; three-ship switching and wingman AI; weapons, damage, shields, enemy AI, waves, targeting, balance, signature and unlockable power *mechanics*; objectives, encounter pacing, hyperdrive and level transitions, mission flow, win/lose and restart; first-person gameplay on the planets (character, blade combat, crew AI, objectives); C++ architecture and interfaces; profiling, testing, packaging, critical bugs | Visual direction; ships, enemies, Dominion and prop *visuals*; space and planet environments, lighting, atmosphere, materials; all VFX (Niagara); HUD, UI/UX, menus, onboarding, ship-switch feedback, targeting clarity; all audio and music; character and ship animation polish; cinematics and the Director's Cut presentation; performance-minded art; trailer and store page |
| **Feature IDs** | `ENG-xx` | `ART-xx` |

**How you avoid stepping on each other:**
- **Ryan makes it work. Nick makes it look, sound and read great.** They meet only through the
  contracts in §3, especially the **presentation hooks** (§3.5): gameplay code reads Nick's data
  assets and fires events that Nick's UI, VFX and audio respond to.
- **Nick** doesn't independently change gameplay architecture, flight, AI, damage, progression or
  ship-switching logic. He coordinates interface needs with Ryan before modifying shared C++ classes,
  levels, assets or config.
- **Ryan** doesn't overwrite Nick's visual direction, art assets, Niagara systems, UI styling,
  lighting or audio without coordination. He provides stable gameplay hooks and data for the
  presentation work.

---

## 2. Folder & file ownership

```
Source/unrealGameJam/              🟦 RYAN: all C++ (Nick requests changes via §11 or a claim + chat)
├── TriadDrift/Core/               🔒 contracts (§3): frozen after C-02; changes agreed by both
├── TriadDrift/Space/              ships, squad, weapons, powers, abilities, enemies, waves, travel, Dominion
├── TriadDrift/Ground/             first-person character, energy blade, crew AI, objectives, hazards
├── TriadDrift/Flow/               GameInstance, game modes, saves, level travel, debug menu
└── Variant_Shooter/ Variant_Horror/ … template code: reference; Ryan owns
Config/*.ini                       🟦 RYAN: claim before editing (input, collision, maps & modes, rendering)
Content/TriadDrift/
├── Gameplay/                      🟦 RYAN: Blueprint children of C++ classes, AI (StateTree/BT), gameplay data
│                                         (DA_*_Stats, DT_Waves), input (IMC_*, IA_*), game modes
├── Maps/                          🔀 SHARED via World Partition One File Per Actor:
│                                         Ryan creates maps + places gameplay actors (spawns, triggers, objectives);
│                                         Nick places art, lighting, VFX, audio volumes. World Settings: claim first
├── Presentation/                  🟩 NICK: the presentation data-asset INSTANCES (DA_*_Visuals, DA_*_FX)
│                                         that Ryan's code reads (§3.5)
├── Art/                           🟩 NICK: Ships/ Enemies/ Dominion/ Props/ Weapons/ Characters/ (meshes,
│                                         materials, textures, skeletons, AnimBPs), Materials/ (masters
│                                         M_TD_Flat / M_TD_Textured), Pipelines/ (import pipeline asset)
├── Environment/                   🟩 NICK: skybox, nebulae, planet visuals, asteroid/crystal sets, terrain
│                                         materials, foliage, lighting & post-process presets
├── VFX/                           🟩 NICK: all Niagara systems, camera-shake classes, post-process materials
├── UI/                            🟩 NICK: all widgets (WBP_*), fonts, icons, DT_Tips
├── Audio/                         🟩 NICK: SFX, MetaSounds, music, sound classes/mixes, VO, DT_Barks
├── Cinematics/                    🟩 NICK: Level Sequences, director-cam rigs, slate cards
└── Developers/<name>/             personal scratch; never referenced by shipped content
ArtSource/                         🟩 NICK: Blender sources, AI originals, exports (blender-* skills)
    <Category>/<Asset>/{brief.md, refs/, source/, <Asset>_vNN.blend, review/, export/}
Template Content (Characters, FirstPerson, LevelPrototyping, Variant_*, Weapons): reference. Claim before editing.
```

**Rules**
- Never edit the other person's folders. Ask through your humans, or add a line to §11.
- `Source/.../Core/` is frozen after C-02. **Add new things; never rename or remove existing enums, fields or
  interface functions.** Who approves what (Nov 12, AGENTS.md §1 "Delegation"): an *addition* to Core is an
  engineering decision, made by Ryan's Lead under Ryan's delegation and announced on `MESSAGEBOARD.md`; a
  *rename or removal* of anything presentation reads needs both sides' agreement on the board first. Art,
  visual and audio direction and final quality sign-off are always Nicholas's.
- Maps: claim the **World Settings** and any shared persistent actors before editing them. With One
  File Per Actor, two people can edit different actors in the same level safely.
- Collision channels (C-01): `Ship`, `Projectile`, `Character`, `Interactable`, `Portal`.
- Placeholders: Ryan builds with template or primitive meshes. Nick's data assets swap in the final
  art with no gameplay edits.

---

## 3. Shared contracts

**C++ first.** Contracts are C++ in `Source/unrealGameJam/TriadDrift/Core/` (all `BlueprintType`).
Gameplay classes are C++ (`ATD*` / `UTD*`) with thin Blueprint children (`BP_*`) for tuning and
asset references. Ryan writes them (C-02). Nick reviews §3.5 before it is frozen.

### 3.1 Enums & structs
| Type | Values / fields (append new enum values at the **end** only) |
|---|---|
| `ETDShipId` | `Rift`, `Lancer`, `Bastion` |
| `ETDTeam` | `Player`, `Enemy`, `Neutral` |
| `ETDAbilityId` | `None`, `GravityWell`, `Cloak`, `EMPPulse` |
| `ETDDestination` | `MainMenu`, `Galaxy`, `Verdance`, `Cinder`, `DominionInterior` (stretch), `Victory` |
| `ETDDamageKind` | `Energy`, `Kinetic`, `Explosive`, `Melee`, `Ram`, `Environmental` |
| `ETDGamePhase` | `Intro`, `Waves1to2`, `Verdance`, `Waves3to4`, `Cinder`, `Wave5`, `DominionAssault`, `DominionInterior`, `Escape`, `Victory` |
| `FTDProgress` | `Phase`, `WavesCleared`, `CompletedObjectives (Set<Name>)`, `UnlockedAbilities (Map<ETDAbilityId,ETDShipId>)`, `bDominionRevealed`, `bDominionDefensesDown`, `ReturnPoint`, `LastShip`, `RunTimeSeconds`, `Stats (FTDRunStats)`, `Takes (Map<Name,int>)` (Director's Cut) |
| `FTDRunStats` | `Kills`, `ShipSwitches`, `PortalsUsed`, `ShotsDeflected`, `Deaths`, `MissilesReturned`, `HighlightMoments` |
| `FTDShipHUD` | `Ship`, `Health01`, `Shield01`, `Boost01`, `Heat01`, `SecondaryAmmo`, `PowerCooldown01`, `Power2Cooldown01`, `AbilityCooldown01`, `Ability`, `bPlayerControlled`, `bAlive`, `RespawnSeconds` |

### 3.2 Interfaces (all implemented by Ryan)
| Interface | Implemented by | Used by | Functions |
|---|---|---|---|
| `ITDGameFlow` | `UTDGameInstance` | both | `TravelTo(Dest, ReturnPoint)` · `GetProgress()` · `SetPhase()` · `ReportWaveCleared()` · `CompleteObjective(Id, Reward, ForShip)` · `SetDominionState(Revealed, DefensesDown)` · `ReportPlayerDefeated()` · `AddStat()` · `RequestBark(BarkId, Speaker)` · `ShowTip(TipId)` · `SaveCheckpoint()` |
| `ITDTeam` | everything damageable | weapons, AI | `GetTeam()` |
| `ITDHUDData` | `ATDSquadManager` (space), `ATDFirstPersonCharacter` (ground) | Nick's HUD widgets | `GetSquadHUD()` · `GetCurrentTarget()` · `GetLockedTargets()` · `GetWaveInfo()` · `GetHyperdriveCharge01()` · `GetObjectiveMarkers()` · `GetSpeed01()` · `IsOutOfBounds()` · `GetAltitude()` · ground: `GetHealth01()`, `GetBlockStamina01()`, `GetDashCharges()`, `GetInteractPrompt()` |
| `ITDDeflectable` | `ATDProjectile` | blade, portals, dome | `Deflect(NewInstigator, NewDirection, NewTeam, SpeedMultiplier)` |
| `ITDInteractable` | interactables | first-person character | `GetPrompt()` · `CanInteract()` · `Interact()` · `GetHoldTime()` |

### 3.3 Damage
Use Unreal's built-in `ApplyDamage` / `TakeDamage`, with a `UDamageType` subclass per `ETDDamageKind`.
Always check `GetTeam()` first: friendly fire is **off**.

### 3.4 Gameplay events (Ryan fires; Nick's UI/VFX/audio listen)
Multicast delegates on the owning actors, re-broadcast by `UTDGameInstance` for widgets:
`OnWaveStarted(Index, Intensity01)` · `OnWaveCleared(Index)` · `OnShipSwitched(New, Old)` ·
`OnShipDowned(Ship)` / `OnShipRespawned(Ship)` · `OnSquadWiped()` · `OnPowerUsed(Ship, PowerName)` ·
`OnAbilityUnlocked(Ability, Ship)` · `OnPhaseChanged(Phase)` · `OnObjectiveUpdated(Id, Step, Text)` ·
`OnHitConfirmed(Target, bKill)` · `OnHyperdriveState(State)` · `OnPortalState(Portal, State)` ·
`OnTipRequested(TipId)` · `OnBarkRequested(BarkId, Speaker)` ·
`OnHighlightMoment(Kind, Actor, Importance)` (Director's Cut, C-03) · `OnTakeStarted(SceneId, TakeNumber)`.

### 3.5 Presentation hooks: how Nick's art plugs in without shared-file edits
- Ryan's C++ defines **presentation data-asset classes** (`UPrimaryDataAsset`, soft references only):
  - `UTDShipVisuals`: mesh/skeletal mesh, materials, squad colour, HUD icon, engine-trail / boost /
    drift VFX, switch-in / switch-out VFX + SFX, power wind-up / active / end VFX + SFX, damage-state
    VFX, explosion tier, engine sound, camera shakes.
  - `UTDWeaponFX`: muzzle, tracer, impact on hull / shield / rock VFX, fire + impact sounds, shake,
    hit-marker style.
  - `UTDEnemyVisuals`, `UTDAbilityFX`, `UTDTravelFX` (hyperdrive spool / tunnel / drop-out, portal
    open / idle / transit / close, atmospheric entry, landing dust), `UTDBladeFX`, `UTDUIStyle`.
- **Gameplay actors hold one data-asset reference each and spawn from it at the event.** An empty
  slot does nothing (no crash), so Ryan's code runs before any art exists.
- **Nick owns the instances** in `Content/TriadDrift/Presentation/` and everything they point to.
  Ryan never edits them, and Nick never edits gameplay Blueprints or C++. Need a new slot? Nick
  requests it (§11); Ryan adds it to the class.
- **Animation state:** gameplay exposes read-only state on the pawns (`Speed01`, `bBoosting`,
  `bDrifting`, `PowerWindup01`, `bGearDown`, `bCanopyOpen`, `Damage01`, `BladeState`) for Nick's
  AnimBPs and timelines.
- **Cameras:** Ryan owns control-feel cameras (chase cam, FP cam). Nick owns cinematic cameras and
  sequences, and the camera-shake *assets*.

### 3.6 Stubs (nobody blocks)
- **Ryan:** placeholder data-asset instances with empty slots (in `Gameplay/Debug/`), plus a
  `BP_TD_StubGameFlow` until the real flow lands.
- **Nick:** `WBP_` widgets built against a `BP_TD_FakeHUDData` (UI folder; implements `ITDHUDData`
  with sliders). VFX and sounds are previewed in Nick's own test map
  (`Developers/Nick/L_Test_Presentation`).

---

## 4. Shared setup (`C-xx`)

### C-01 · Project bootstrap · 🟦 Ryan · `🟨`
- [x] Create the UE project (`unrealGameJam`, First Person C++ template, UE 5.9.0) and record the engine version in AGENTS.md.
- [x] `.gitignore` and Git LFS (`.uasset .umap .blend` + images, audio, meshes). Not `lockable`; we coordinate through `CLAIMS.md`.
- [ ] Create the `Content/TriadDrift/...` and `Source/unrealGameJam/TriadDrift/...` trees from §2.
- [ ] Collision channels (`Ship`, `Projectile`, `Character`, `Interactable`, `Portal`) and Maps & Modes.
- [ ] Maps (World Partition + One File Per Actor): `L_MainMenu`, `L_Galaxy`, `L_Planet_Verdance`, `L_Planet_Cinder`, `L_Transition`, plus stretch `L_Dominion_Interior`. Also `L_Test_Space` (Ryan) and `L_Test_Presentation` (Nick).
- [ ] Create `CLAIMS.md`, `CREDITS.md` and `DEVLOG.md`, then push.

### C-02 · Contracts · 🟦 Ryan writes, 🟩 Nick reviews §3.5 · `✅`
- [x] Write everything in §3 as C++ (`Core/`) and compile. Create the presentation data-asset classes. (No placeholder instances: every slot is optional and null-safe, and Nick's side creates the instances.)
- [x] Nick checks that the presentation slots cover what he needs for ships, weapons, enemies, travel, blade and UI. (Done over `MESSAGEBOARD.md` R-003 to R-005 and N-001; further slots are added on request.)
- [x] Push. **`Core/` is now frozen.**

### C-03 · Theme: "Director's Cut" · both · `⬜` 🎬
Theme fit is a judging category, so it must be **visible in gameplay**, not just the title.

**Decision (Nov 12): we build #1, #2 and #3.** All three are core. They're cheap,
presentation-heavy, and build on the switching mechanic. #4 and #5 are not planned.

1. **Switching is a cut** *(BUILD)*. A ship switch plays as a director cutting between cameras:
   a hard camera cut with a 1-frame flash, a quick letterbox snap and the pilot's name card
   ("WREN · RIFT"). It stays instant, with no lost control. 🟦 switch event + timing · 🟩 cut
   transition, card, sound.
   **(Nov 12)** While Nick's space HUD is up it draws the name card ("CUT TO / LANCER"); the fallback
   card hides and the fallback overlay sits under his HUD (N-007).
2. **Director cam "money shots"** *(BUILD)*. Big moments fire `OnHighlightMoment`: ram kills,
   portal missile returns, multi-kills, generator and reactor kills. Each triggers a 0.5–1 s
   slow-motion beat from a cinematic angle, rate-limited. 🟦 events + time dilation · 🟩 camera rigs,
   framing, grade.
   **(Nov 12, as built)** Also produced: carrier kills, Switch Strike kills and the on-foot perfect
   deflect (0.15 s, deliberately a parry flash rather than a beat). Limits: time dilation 0.4 at the
   lowest importance down to 0.2 at the highest; 3 s global cooldown and 6 s per kind; importance 0.8
   or higher bypasses the cooldowns; multi-kills are capped at importance 0.7.
3. **Scenes and takes, not lives** *(BUILD)*.
   - Each wave or objective is a "scene" opened by a slate card ("SCENE 3 · CINDER · TAKE 1").
   - Dying calls "CUT!" and the scene restarts as TAKE 2.
   - The victory screen shows the takes needed per scene.

   🟦 checkpoint/restart per scene · 🟩 slate cards, "CUT!" presentation.
4. ~~**Your Director's Cut**~~ *(not planned)*. The victory screen plays an auto-edited highlight reel built from
   the recorded highlight moments. 🟦 event log with positions and times · 🟩 camera-cut playback.
5. ~~**Director's commentary**~~ *(not planned)*. A "Director" voice (the squad commander) opens scenes and
   reacts to highlights; subtitles are styled as script pages. 🟩 VO + subtitles, driven by
   `OnBarkRequested`.

**Owners:**
- **#1:** ENG-07 + ART-09/12.
- **#2:** `OnHighlightMoment` + time dilation (ENG-07/09/10/18/22) + ART-12.
- **#3:** ENG-14/19 checkpoints and takes + ART-09/12 slate cards and "CUT!".

---

## 5. 🟦 Ryan: Gameplay & Engineering

**Start here (role brief, Day 1 focus):** a playable prototype with flight, shooting, instant ship
switching and baseline wingman AI. Expose the integration points (§3.4–3.5) so Nick's visual pass
can start.

**Deliverables:**
- a continuously playable build with flight, shooting and reliable three-ship switching
- two autonomous wingmen that keep engaging after a switch
- a complete, balanced combat loop with enemy waves and objectives
- a tested, packaged submission build

### Space core

#### ENG-01 · Space flight model · `🟨`
Arcade 6DOF-lite flight: fast, readable, with a signature **drift**. Gamepad and keyboard + mouse are
both first-class.

| Control | Behaviour | Start values |
|---|---|---|
| Throttle | Analog forward speed, min → cruise | min 1500, cruise 6000 cm/s (per-ship multipliers in ENG-04) |
| Aim (mouse / right stick) | The ship turns toward an aim reticle (mouse = virtual joystick with a deadzone) | 110°/s pitch and yaw |
| Roll | Manual; auto-banks into turns | 180°/s |
| Strafe (optional) | Lateral and vertical thrusters | 4000 cm/s |
| **Boost** | Hold: big speed increase that drains the meter | +80% speed, 3 s meter, regen after 1.2 s |
| **Drift** | Hold: velocity keeps its direction while the nose rotates freely. Release: snap to the nose plus a burst | 1.5× turn rate; +20% burst |
| Brake | Hard deceleration to min speed | |

- `ATDShip` pawn with custom velocity integration; values in `UTDShipData` (`DA_ShipStats_*`).
- Chase camera (rotation lag 8, location lag 12, FOV 90 → 110 with speed, 30% roll follow,
  look-ahead), with an optional cockpit camera.
- Collisions: glancing hits scrape and bounce (damage scales with impact angle); head-on hits deal
  heavy damage.
- Soft spherical play-space boundary: warn, then gently steer back (`IsOutOfBounds`).
- Feel hooks for Nick: boost, drift and speed state exposed; camera-shake slots in `UTDShipVisuals`.

**Done when:** flying through an asteroid field is fun on its own, and drifting around a rock while
shooting backwards feels great.

#### ENG-02 · Space input map · `🟨`
`IMC_Space` (plus `IMC_Atmosphere`, which adds Land and removes Hyperdrive):

| Action | KB/M | Gamepad |
|---|---|---|
| Throttle | W / S | RT / LT |
| Aim | Mouse | Right stick |
| Roll | A / D | Left stick X |
| Fire primary / secondary | LMB / RMB | RB / LB |
| Boost / Drift | Shift / C | L3 / B (hold) |
| Signature power 1 / 2 (hold, Bastion) | Q / hold Q | X / hold X |
| Unlocked ability | E | Y |
| Switch next / prev / direct | Tab / Shift+Tab / 1-2-3 | D-pad right / left / hold up = radial |
| Target cycle | T / MMB | R3 |
| Hyperdrive (hold) | H | D-pad down (hold) |
| Squad command / Land / Interact | F | A |
| Pause / Galaxy map | Esc / M | Start / Select |
| Dodge **(Nov 12)** | Left Alt, or double-tap A / D | Left stick double flick |
| Brake **(Nov 12)** | Space | LT fully pulled |
| Strafe **(Nov 12)** | Arrow keys | Left stick Y |
| Cockpit camera **(Nov 12)** | V | R3 (hold) |

**(Nov 12)** The galaxy map key is hidden until its screen exists (ART-09).

#### ENG-03 · Health, shields, death and respawn · `🟨`
- **Shield:** absorbs damage first; regenerates after 3 s without damage at 25%/s. **(Nov 12)** Set per
  ship in `FTDShipSpec` (`ShieldRegenDelay`, `ShieldRegenFractionPerSecond`); those are the defaults.
- **Hull:** doesn't regenerate in combat, but repairs at 8% per second in the breather between waves.
- **Death:** a 0.6 s tumble, then an explosion (`UTDShipVisuals` tier), then **auto-switch** to the
  healthiest living ship. A downed ship warps back in after 12 s. There are 2 s of invulnerability
  after a respawn or an automatic switch; a manual switch gives 0.35 s, at most once every 6 s per
  ship, so switching cannot be spammed for permanent invulnerability.
- **Squad wipe:** if all three ships are down at once, fire `OnSquadWiped`, then
  `ReportPlayerDefeated()` (Director's Cut: "CUT!", then the next take).

- **(Nov 12)** A manual switch made during your own ship's death tumble gets the same 2 s protection as
  the automatic switch. Between waves only the squad manager repairs (8% of max hull per second). After a
  retake or an in-place respawn every ship, including the one the player was flying, is flown by its AI
  again as soon as the player switches away.

#### ENG-04 · The three ships (gameplay stats) · `🟨`
| | **Rift** (Trickster) | **Lancer** (Interceptor) | **Bastion** (Heavy) |
|---|---|---|---|
| Hull / Shield | 100 / 100 | 70 / 80 | 180 / 160 |
| Cruise / Boost | 6000 / 11000 | 7500 / 15000 | 4800 / 8500 |
| Turn rate | 120°/s | 100°/s | 75°/s |
| Boost meter | 3 s | 4 s, fast regen | 2.5 s |
| Primary / Secondary | Phase Pulse Lasers / Splitter Bolt | Needle Cannons / Spike Rockets | Breaker Blaster / Flak Burst |
| Signature | Portal Blaster | Afterburner Ram | Missile Swarm + Shield Dome |
| Unlockable | Cloak | EMP Pulse | Gravity Well |

**(Nov 12, as built)** Stats are `FTDShipSpec` in `UTDShipLibrary` (C++), overridable per ship on
`ATDSquadManager` (`ShipSpecOverrides`); no `BP_Ship_*` children or `DA_ShipStats_*` assets are needed.
Each ship's look comes from its `DA_<Ship>_Visuals` (`DA_Rift_Visuals`, `DA_Lancer_Visuals`,
`DA_Bastion_Visuals`; Nick, ART-03; exact names in `docs/visual/Presentation-Asset-Paths.md`).

#### ENG-05 · Ship weapons · `🟨`
| Weapon | Ship | Type | Damage | Rate | Speed | Range | Resource | Notes |
|---|---|---|---|---|---|---|---|---|
| Phase Pulse Lasers | Rift | Twin alternating bolts | 8 each | 8/s | 40 000 | 40 000 | Heat: 4 s to overheat, 1.5 s lockout | 3° aim assist |
| Splitter Bolt | Rift | Charge shot (0.8 s) | 40 + 5×12 shards | — | 25 000 | 35 000 | 6 ammo, +1 per 4 s | Splits on impact; great vs swarms |
| Needle Cannons | Lancer | Rapid kinetic | 4 | 20/s | 55 000 | 30 000 | Heat: 5 s | 1.2° spread |
| Spike Rockets | Lancer | Dumb-fire salvo of 4 | 25 each, 300 cm splash | 1 per 1.5 s | 30 000 | 40 000 | 8 salvos, +1 per 5 s | |
| Breaker Blaster | Bastion | Heavy plasma | 30, 250 cm splash | 2.5/s | 28 000 | 35 000 | Heat: 6 s | Knocks back fighters |
| Flak Burst | Bastion | Proximity flak | 15 splash over 800 cm, 500 cm proximity fuse | 1 per 1.2 s | 22 000 | 20 000 | 10 shells, +1 per 3 s | **Shoots down missiles** |

- `UTDWeaponComponent` + `UTDWeaponLibrary` (the `FTDWeaponSpec` table, C++; looks in `UTDWeaponFX`); all projectiles derive from `ATDProjectile` (C++,
  implements `ITDDeflectable`).
- Lead indicator data for the HUD; `OnHitConfirmed` for hit and kill markers; 40 ms hit-stop on
  kills.
- Visuals and audio come from `UTDWeaponFX` (Nick).

#### ENG-06 · Targeting & lock-on · `🟨`
- Soft target = the enemy closest to screen centre within 25°, cycled by distance.
- Bastion missile locks: targets inside the 25° cone are painted automatically, up to 6, at 0.25 s
  each. A lock is dropped when its target dies, becomes invalid (cloaked, wrong team), goes beyond the
  swarm's range, or stays outside the cone for more than 1.5 s; when all six are used, a new
  candidate replaces the stalest lock.
- Off-screen arrows, incoming-missile and objective direction data → `ITDHUDData`. Objective
  markers are the live actors carrying the actor tag `TD.Objective.Marker`. The game flow owns the tag:
  the destination planet's atmosphere, live shield generators, the reactor, on-foot waypoints and the
  board-ship point; the landing pad while in atmosphere (flight). Nothing is tagged during a wave.
- Incoming-missile warning state (Nick supplies the sound and UI).
- More HUD data on the same source, all optional zero-input getters (added Nov 12): `GetObjectiveText`
  (current objective line), `GetBoundaryWarningText` (edge of the play area), `GetInteractPrompt` (landing
  prompt, and for 1.5 s why a switch was refused), `GetSquadMarkers` (where each squad ship is and whether
  the wingman is engaging: off-screen wing arrows), `GetDamageIndicators` (direction of recent hits on the
  player: "who is shooting me").
- Dodge (player ship only, added Nov 12): a 0.25 s sideways burst at 100 m/s with a 1.5 s cooldown and no
  invulnerability. Left Alt (toward the held roll or steering side), double-tap A or D, or a double flick of
  the left stick. Refused during the ram, landing, atmosphere entry, hyperdrive and director slow motion.
  Measured: weaving with a timed dodge takes about 40-50% of straight-line damage. Strafe is 40 m/s.
- Pause: Escape / gamepad Start, in space and on foot (`OnPauseChanged` on the player controller;
  `UTDUIStyle.PauseWidgetClass` for the menu). Refused during level travel, a pending retake and victory.

#### ENG-07 · Squad manager & instant switching (PROTECT) · `🟨`
- `ATDSquadManager` spawns, tracks and respawns all three ships, and owns who the player controls.
  It implements `ITDHUDData`.
- **Switching:** next, previous or direct; possesses the target instantly on the same frame; the ship
  you left hands control to its AI and keeps its velocity; 0.4 s cooldown; downed ships can't be
  selected.
- **Switch Strike:** switching within 1.5 s of using a power gives +25% damage for 4 s.
- A power started on one ship finishes after you switch away (portal stays open, dome stays up).
- Fires `OnShipSwitched`, which drives Nick's "cut" presentation (C-03 #1).

#### ENG-08 · Wingman AI (PROTECT) · `🟨`
- States: Formation → Engage → Evade → Regroup → Downed.
  - Formation: a loose V at 1500–2500 cm offsets; boosts to catch up beyond 8000 cm. **(Nov 12)** The V is
    swept forward (both wingmen ahead and outboard of the player, at different depths) so the chase
    camera shows it.
  - Engage: attacks whoever threatens the player first, then the nearest, then the lowest health;
    primary and secondary weapons only.
  - Evade: kicks in on shield break or an incoming missile.
- **Never uses signature powers or abilities.** Those fire only when the player is in that cockpit.
- Follows the player through portals and gates and jumps on hyperdrive. `IA_SquadCommand` = "focus my
  target" for 8 s.
- Avoids rocks and teammates; moves toward an active Bastion dome when hurt; 2–4° aim error.

- **(Nov 12)** Target choice: one wingman may defend against an off-screen threat at a time; the other
  prefers targets inside the player's view (within 80 m bands of its best target). Wingmen keep clear of
  the aim line and of an active director shot. In a furball a wingman shares the frame with an enemy well
  under half the time, so the wing is made readable through the HUD (`GetSquadMarkers`: named arrows in
  squad colours) and short position calls (`Bark_SquadEngage...`), not by formation flying.

#### ENG-09 · Rift: Portal Blaster · `🟨` ⚠️ highest risk
- **Placement:** Fire 1 opens portal A where it hits or at max range (12 000 cm); Fire 2 opens B; a
  third shot recycles the oldest. Open for 12 s, then an 8 s cooldown.
- **Ships:** pass through keeping their speed, take the exit's facing, and get 0.3 s of
  invulnerability.
- **Enemy projectiles** that enter are `Deflect()`ed to the Player team and **re-home on whoever
  fired them** (counts as `MissilesReturned`, and fires `OnHighlightMoment`).
- **Enemy fighters** that pass through lose tracking for 2 s.
- **Fallback if it's still unstable late:** "Blink" (teleport to the impact point) plus a 3 s
  missile-reflect field.
- Visuals: `UTDTravelFX` portal slots (Nick).

#### ENG-10 · Lancer: Afterburner Ram · `🟨`
- 0.3 s wind-up, then a 1.2 s dash at 3× boost speed. Lancer is invulnerable and steers at 40%.
- Deals **300 to whatever it touches**: passes through fighters, **breaks gunship shields** and
  carrier bay doors.
- Hitting something big stops the dash with a 150-damage shockwave over 1500 cm.
- 7 s cooldown. Exposes `PowerWindup01` for the wing-fold animation (Nick).

#### ENG-11 · Bastion: Missile Swarm & Shield Dome · `🟨`
- **Missile Swarm (tap):** 12 micro-missiles split across painted locks (or the soft target),
  18 damage each with a 200 cm splash; 9 s cooldown.
- **Shield Dome (hold 0.5 s):** a 3000 cm radius for 6 s with a 16 s cooldown.
  - Squad ships inside take no damage and regenerate shields twice as fast.
  - Enemy shots are destroyed on the dome, and missiles are deflected back.
  - The dome follows Bastion slowly.

#### ENG-12 · Unlockable abilities (CORE: powers aren't optional) · `🟨`
One ability slot per ship (`IA_Ability`), filled from `GetProgress().UnlockedAbilities` on spawn
and on `OnAbilityUnlocked`.

| Ability | Ship | Unlocked by | Behaviour | Cooldown |
|---|---|---|---|---|
| **Gravity Well** | Bastion | Verdance Obj 1 | A slow orb that becomes a 5000 cm vortex for 5 s, pulling enemies and their projectiles in. Enemies inside take 10 dps and can't fire; it ends with a 120-damage implosion | 18 s |
| **Cloak** | Rift | Verdance Obj 2 | 6 s untargetable (how it looks is Nick's, `DA_Ability_Cloak_FX`): AI loses lock and missiles lose tracking. The first shot out does **3× damage**; firing or using another power breaks it | 15 s |
| **EMP Pulse** | Lancer | Cinder Obj 3 | A 6000 cm pulse: stuns enemies for 3 s, drops gunship shields for 5 s, fizzles missiles. Stunned enemies take +25% damage | 14 s |

`UTDAbilityComponent` (the slot) installs one `UTDPowerComponent` child per ability (`UTDGravityWellPower`, `UTDCloakPower`, `UTDEMPPower`). Visuals: `UTDAbilityFX` (Nick).

**Powers and ship switching (decided Nov 12):** a power or ability that is running when the player
switches away (cloak, shield dome, an open portal pair, a gravity well) keeps running on that ship
and ends on its own timer. The new ship's presses never reach the old ship's powers, and an
AI-flown ship never starts one. Two exceptions end at the switch (also on an auto-switch after a
death): the ram (wind-up cancelled, dash ended, no invulnerability left behind) and a portal blaster
with only one portal placed (the lone portal closes).

#### ENG-13 · Enemy ships & AI · `🟨`
| Enemy | Role | HP / Shield | Speed | Weapons | Behaviour | Counter |
|---|---|---|---|---|---|---|
| Shard Fighter | Swarm | 30 / 0 | 7000 | Pulse guns (5, 4/s) | Packs of 4–8, strafing runs | Needles, Splitter, Flak |
| Hook Interceptor | Hunter | 50 / 20 | 9000 | Pulse guns + homing missile (25) every 8 s | Chases the player's ship | Portal missile return, drift |
| Wall Gunship | Tank | 400 / frontal shield 300 (recovers 15 s after it is broken) | 3000 | Heavy bolt volley (20) every 4.5 s + missile salvo (3×20) every 12 s | Faces its target; shield blocks frontal fire | Flank, portal behind it, Ram, EMP |
| Maul Bomber | Area denial | 150 / 50 | 4000 | Plasma mines (60, 800 cm) | Lays a trail of 4 mines every 5 s | Flak, Gravity Well |
| Hive Carrier | Spawner / mini-boss | 1500 hull + 3 bays × 300 | 1500 | 4 point-defense turrets (6; bursts of 1.0 s, then 2.0 s quiet) | Launches 3 fighters per bay every 12 s, at most 3 launches per bay and 8 of its fighters alive at once | Kill bays (Ram breaks doors), Missile Swarm |

- `ATDEnemyShip` + AI: approach, attack run, break off, re-approach; fighters fly in formation.
  Attack runs on the player's ship cross in front of its nose, so the fight stays on screen; a group
  that falls behind catches up, so a wave can always be finished.
- **Balance (Nov 12):** the numbers above were tuned with the headless balance run after wave 5 proved
  unwinnable at the first values; the reasoning and the per-wave tables are in `docs/eng/balance.md`.
- Every big attack has a **telegraph** state for Nick's VFX and audio.
- Visuals: `UTDEnemyVisuals` (Nick).

- **(Nov 12)** Enemy aim and AI choices use a deterministic seed per ship (spawn order in the world), so the
  same fight plays the same way. Fighters open fire at a range that grows with closing speed and never ram on
  a head-on pass. Only the interceptor's missile has a point-blank rule (no launch inside 60 m or with too
  little time to dodge); gunship bolts fire down to 15 m and its salvo from 40 m. Enemy ships do not use the
  planet gates. Placeholder ships glow red-orange with white eyes and are drawn larger than their collision.

#### ENG-14 · Wave director · `🟨`
- `ATDWaveDirector` + `DT_Waves`. Waves spawn behind or flanking the player, with a 6 s breather
  between them.
- Reads `GetProgress()` on load and resumes the right phase.
- Fires `OnWaveStarted` / `OnWaveCleared` and `OnTakeStarted` (C-03 #3).

| Wave | Composition | Intensity | Then |
|---|---|---|---|
| 1 | 2 × 4 fighters | 0.2 | |
| 2 | 3 × 5 fighters + 2 interceptors | 0.35 | Verdance marker appears |
| 3 | 2 gunships + 3 × 5 fighters | 0.55 | |
| 4 | 2 gunships, 2 bombers, 4 interceptors, fighters | 0.7 | Cinder marker appears |
| 5 | "Dominion's Guard": 2 carriers + escorts + bombers | 0.9 | Dominion reveal (ENG-18) |

Tutorial beats fire `ShowTip`: switching (wave 1), each signature power (first time in each ship),
drift, hyperdrive.

#### ENG-15 · Hyperdrive · `🟨`
- Hold to spool for 1.2 s; can't spool within 2 s of taking damage, and a hit cancels it.
- Jump at 80 000 cm/s with 10% steering. Auto-drops out near markers and the Dominion. Wingmen jump
  too and arrive in formation.
- Exposes state for `OnHyperdriveState` (spool, tunnel and drop-out VFX and audio are Nick's).

- **Difficulty:** three presets, Story / Standard / Ace (`UTDDifficultySettings`), scale enemy hull,
  damage, aim error and wave size. Target on Standard: the five waves take 8-10 minutes in total.
- **Hyperdrive lock:** the hyperdrive is locked while a wave is active and during the Dominion
  assault, and free in the breather between waves and in the planet phases.

#### ENG-16 · Portal gates (galaxy shortcuts) · `🟨`
Three permanent `ATDPortalGate` pairs: combat zone ↔ Verdance orbit, combat zone ↔ Cinder orbit, and
a flank pair used in waves 3–5. They reuse ENG-09's transit logic, and wingmen follow. Shots pass
through a gate unchanged: only Rift's own portals return enemy missiles.

#### ENG-17 · Galaxy level: gameplay layout · `🟨`
- **Zones:** the combat zone (asteroids, station ruins as cover), Verdance and Cinder orbits, and
  the Dominion approach.
- **Gameplay actors:** `ATDPlanetAtmosphere` triggers, spawn zones, portal gates and return points
  (`Verdance_Orbit`, `Cinder_Orbit`, `Dominion_Hangar_Exit` for the stretch).
- **Shared map:** Nick dresses the same map with art, lighting and VFX (One File Per Actor).

#### ENG-18 · Dominion assault & space finale (CORE) · `🟨`
- **Arrival:** when `bDominionRevealed` is set after wave 5, the Dominion warps in (Nick's reveal
  sequence plays).
- **Assault:**
  - 4 `ATDShieldGenerator`s (800 HP each, guarded by turrets), 10 `ATDDominionTurret`s on the hull
    and 4 in the reactor trench (telegraphed charge shots); fighters keep launching.
  - Each generator kill drops part of the shield and fires `OnHighlightMoment`.
  - All four down sets `SetDominionState(DefensesDown)` and **exposes the reactor**.
- **Core ending:** fly the exposed reactor trench and destroy the reactor (3 phases of 1200 HP with
  escalating turret fire and launches). When it dies the squad is protected and every remaining
  enemy is destroyed; victory is a phase change on the galaxy map after Nick's breakup sequence.
- **Stretch ending:** the docking bay opens instead and leads to ENG-29.
- **(Nov 12) Retakes and saves:** the assault is one scene ("Dominion"). After a squad wipe the squad restarts on
  the approach (at the trench once the defences are down); destroyed generators stay destroyed, reactor phases
  already passed stay passed, and the current phase restarts. The hyperdrive is locked only inside the
  Dominion's gravity well, so the first approach can jump and drops out at the well. Victory cancels a pending
  retake, and a won save does not continue into the fight.

### Game loop & progression

#### ENG-19 · Game instance, progression, saves & travel · `🟨`
- `UTDGameInstance` implements `ITDGameFlow` and holds `FTDProgress`.
- **Travel:** `TravelTo` fades to `L_Transition` (Nick's loading screen widget), then the target
  level.
- **Checkpoints:** taken on level entry, wave clear and objective complete. The **scene/take restart**
  (C-03 #3) reuses them. The save slot supports Continue.
- **Defeat:** "Retry from checkpoint" or "Quit". Run stats feed the victory screen.
- **Debug menu** (F10, dev builds only): jump to any phase, unlock abilities, god mode, kill all,
  skip wave.

#### ENG-20 · Atmosphere flight, landing & take-off (gameplay) · `🟨`
See §7.
- **Atmosphere flight mode:** gravity, speed capped at 60% of space cruise, no hyperdrive,
  altitude data.
- **Zone and pad actors:** `ATDAtmosphereFlightZone` and `ATDLandingPad` (beacon; "Hold F to land").
  - Within 120 m of the pad at or below 60% of the atmosphere speed cap, F lands directly
    (aligning takes 1.5 to 4 s by distance, up to 6 s when the path arcs over a prop).
  - Within 400 m, holding F is an approach assist: it turns the nose onto the pad (150°/s), flies in
    at up to 60 m/s, brakes at 30 m/s² but never below 15 m/s, and hands over to the landing 25 m
    from the hover point. Releasing F, boost, hyperdrive, a switch, death or a retake ends it.
    From 400 m the landing starts about 7 s after F and touchdown is at about 12 s.
  - A ship that cannot assist and is too fast within 400 m gets "SLOW DOWN TO LAND".
  - Near the ground the ship levels out by itself, so a hands-off arrival never crashes.
  - The player pad carries the objective marker while in atmosphere and not yet landed.
- **Landing and take-off:** auto-land (gear and canopy state exposed), wingmen land beside you, the
  exit hands off to first person, and boarding reverses it.

#### ENG-21 · First-person character (planets: CORE) · `🟨`
Extends the template's first-person character.

| Move | Input | Values |
|---|---|---|
| Run / sprint | WASD / Shift | 650 / 950 cm/s |
| Jump / double jump | Space | Z 650 / 500 |
| **Dash** | Ctrl or Q | 1200 cm in 0.18 s, 0.15 s i-frames, 2 charges, 1.2 s regen |
| Slide | Ctrl while sprinting | 0.7 s |
| Mantle | auto at ledges ≤ 180 cm | 0.35 s |
| Attack / Block / Heavy | LMB / RMB / hold LMB | ENG-22 |
| Interact / Objective ping | E / Tab | `ITDInteractable` |

- 100 health, regenerating after 4 s at 15/s; death → checkpoint respawn.
- Implements the ground side of `ITDHUDData`.

#### ENG-22 · Energy blade combat (CORE, used on planets) · `🟨`
| Move | Damage | Behaviour |
|---|---|---|
| Light combo ×3 | 25 / 25 / 45 | 180 cm, 100° arc sweeps; the third hit knocks back; 0.45 s combo window |
| Heavy lunge | 70 | 600 cm dash-stab; breaks Heavy guards; 2 s cooldown |
| Air slam | 50, 300 cm AoE | Downward plunge |
| Dash strike | 35 | Attack during a dash |
| Block | — | 120° front arc; move at 50% speed; 3 s stamina, then guard break |
| **Perfect deflect** (≤ 0.2 s) | Shot damage × 2 | Auto-aims back at the shooter, with a 0.15 s slow-motion beat (`OnHighlightMoment`) |
| Normal deflect | Shot damage × 1 | Reflects toward the crosshair |
| Melee parry | — | Staggers the attacker for 1.2 s; the next hit crits |

Hit-stop: 50 ms on hit, 120 ms on kill. Visuals and audio: `UTDBladeFX` (Nick).

#### ENG-23 · Crew enemies: AI & combat (CORE, planets) · `🟨`
| Enemy | HP | Attacks | Behaviour |
|---|---|---|---|
| Grunt | 60 | Telegraphed baton (15) | Groups of 3–5; flanks |
| Trooper | 50 | Slow, bright, **deflectable** bolts (10, 3-round bursts) | Keeps distance; uses cover |
| Heavy | 200 | Shield bash (25); deflectable shotgun (5 × 6) | Frontal shield blocks slashes |
| Sentry Drone | 30 | Fast small bolts (5) | Hovers out of melee range |
| Warden (mini-boss) | 600 | Charge slam (40 AoE), bolt fan, summons grunts | Phase 2 at 50% |

- Built on the template's `Variant_Shooter` NPC and StateTree AI: Idle/Patrol → Alert → Engage →
  Stagger → Dead.
- Attack tokens: at most 2 grunts attack at once.
- Ragdoll on death. Models and animations are Nick's (ART-08).

#### ENG-24 · Objective framework · `🟨`
`ATDObjective` (Id, steps, waypoint, reward ability and ship) and interactables (tap or hold;
interrupted by damage). Fires `OnObjectiveUpdated` and calls `CompleteObjective`, which brings up
Nick's reward screen. Encounter volumes spawn crew; clearing a room opens its doors.

#### ENG-25 · Verdance: gameplay layout & objectives (CORE) · `🟨`
- **Level:** a compact level (about 1 km²) with the landing pad and a descent start point. Nick
  dresses the look.
- **Obj 1, Wreck Core** (→ Gravity Well):
  1. Follow the beacon past a trooper ambush.
  2. Clear the wreck site.
  3. Hold-interact for 4 s while 2 waves drop in.
  4. Carry the core back (no sprint or block while carrying).
- **Obj 2, Ancient Relay** (→ Cloak): light platforming (double jump, dash, mantle) with drones,
  three resonance nodes, then the relay pulses.

- **(Nov 12)** Built in code around the pad (`ATDPlanetCourse`): both objectives can be done in either order,
  each starts at its trailhead as its own scene, with route posts, objective lines and markers. Boarding is
  allowed at any time; the way back to space opens when both are done. A required objective with no actor in
  the world is completed with an error in the log, so a content mistake never traps the player.

#### ENG-26 · Cinder: gameplay layout, hazards & objectives (CORE) · `🟨`
- **Hazards:** magma (heavy damage + respawn at the last safe spot), timed geysers, crystals that
  shatter when slashed.
- **Obj 3, Outpost** (→ EMP): breach the gate (2 conduits), 3 escalating areas, the Warden, then
  claim the EMP prototype.
- **Obj 4, Jammer** (→ reveals the Dominion): collapsing crystal bridges, plant 3 charges under
  attack, then run as the tower collapses.

- **(Nov 12)** The blade breaks blade-breakable props (crystals, gate conduits), not only enemies. The magma
  hurts at its surface only. A checkpoint precedes the Warden. **Director's mercy:** a fight the player keeps losing eases one
  notch at a time (Cinder Warden: from take 3, 75% health and one summon; from take 5, 50% and none); a player who
  wins first time never sees it. Any on-foot fight can opt in (`ATDEncounterVolume::MercyTiers`). Engineering-made props dress both planets
  (`Content/TriadDrift/Gameplay/Props`, trees in `Gameplay/Vegetation`) until Nick's art replaces them.

#### ENG-27 · Balance & tuning · `🟨`
Play the full run at least twice. Tune enemy counts, HP and cooldowns toward 20–30 minutes. Add a
"Pilot / Ace" difficulty setting that scales enemy damage and accuracy.

- **(Nov 12) Pressure targets on Standard, to be confirmed by a human:** a squad ship goes down in about 1 run
  in 5 in waves 1-2, about 1 in 2 in waves 3-4, most runs in wave 5 and the assault, with a squad wipe in about
  1 run in 5 there. Measured in the headless balance run (chance of a ship down and hull lost per minute for
  the active ship) before any tuning.

#### ENG-28 · Integration, performance, packaging & submission build · `🟨`
- **Integration and testing:** C++ architecture reviews, profiling (a steady 60 fps target on mid
  hardware), a smoke test after every merge.
- **Packaging:** Windows Shipping profile with all maps listed, startup map `L_MainMenu`, and the
  debug menu stripped.
- **Builds:** package whenever a major feature lands, into `Builds/` (git-ignored), and smoke-test
  start → finish.
- **Final zip:** `TeamName_GameName`, < 1 GB (otherwise use butler). Upload the build and fill in the
  submission form with Nick (ART-14).

#### ENG-32 · On-foot third-person camera & the Settings page (CORE, added Nov 12 by Ryan) · `🟨`
Ryan, Nov 12 (late): "a settings page, and you should be able to switch between first and third person ...
when you're using a character on the ground". Space flight keeps its own cameras (chase, cockpit); this item is
**on foot only** (planets, and the stretch interior).

**Camera modes on foot**
| Mode | What the player sees | Values |
|---|---|---|
| First person (default) | Today's view: blade on the camera rig, the body below the neck and its shadow | as ENG-21 |
| Third person | Over-the-shoulder chase view of the whole pilot | spring arm 320 cm, shoulder offset right 55 cm / up 65 cm, camera lag 12, rotation lag 18, collision probe 12 cm, FOV 85 |

- **Switching:** keyboard **V**, gamepad **View / Select** (on foot the galaxy-map binding does not apply), or the
  Settings page. Instant, no fade; never while a director shot or the death cam owns the view (it applies when
  they end). The choice is saved with the other options and restored at start-up and after every level travel.
- **Body in third person:** the whole pilot is shown (head included), the blade is held in the pilot's **right
  hand** (socket on the hand, set by eye), and the camera-rig blade is hidden. In first person nothing changes.
- **Facing:** the pilot turns toward the way it moves while just running; while attacking, blocking, aiming a
  deflect or standing still with the camera turned more than 60°, it turns to face the camera direction
  (strafe), so the blade always swings where the crosshair points. Melee targeting, the perfect-deflect return
  and interaction all use the camera's aim, exactly as in first person.
- **Animation readability:** idle / walk / run from `DA_Pilot_Visuals`; attacks, heavy lunge, block, dash, slide,
  air slam, mantle, hit react and death must visibly read in third person: retargeted mannequin animations onto
  the pilot's skeleton where that works headless, otherwise procedural upper-body poses in the pilot's animation
  instance. No T-pose, no sliding idle while moving.
- **Camera collision:** never inside walls, props, trees or crew; pulls in smoothly and eases back out; hides the
  pilot if the camera is closer than 60 cm; tight spaces (corridors, the interior) still readable.
- **Crosshair and HUD:** the crosshair stays centred and melee / interact prompts are unchanged; Nick's HUD and the
  fallback HUD work in both modes.
- **Combat fairness:** crew attacks, tells and deflect windows are identical in both modes; third person must not
  reveal enemies through walls.

**Settings page (in-game, reachable from the main menu and the pause menu)**
- Engineering ships a code-built page (keyboard, mouse and gamepad navigable) over `UTDSettingsSubsystem`:
  **On-foot camera (First / Third person)**, mouse sensitivity, invert mouse Y, gamepad sensitivity, gamepad dead
  zone, invert gamepad Y, control hints on / off, master / music / effects volume, window mode, resolution, and
  "Reset to defaults". Every change applies at once and is saved.
- Nick's side may supply a styled page through `UTDUIStyle::SettingsWidgetClass`; it calls the same Blueprint
  functions and replaces the code-built page with no engineering change.

**Done when:** on both planets a player can switch with V / View and from the Settings page, fights a full room in
third person with readable attacks and blocks, the camera never clips through the level, the choice survives a
restart, and tests cover the toggle, persistence, camera collision, facing rule and the space-only exclusion.

#### ENG-33 · Cinematic intro (CORE, added Nov 12 by Ryan) · `🟨`
Ryan, Nov 12 (late): the intro should be "really high definition cinematic views of space and the spacecraft going
by and worlds". **Reference for the cinematic language only** (pacing, scale, framing, grade), not its content, world
or style, which belong to another IP: the *Warhammer 40,000: Darktide* intro cutscene
(https://www.youtube.com/watch?v=CZgaAUA9brI, 2:54). What it does well, from frames reviewed by the Lead:
- very slow push-ins on a vast nebula; huge objects (a planet in silhouette, a colossal ship) that cross the frame
  slowly so scale reads; cuts to black between beats; narration as short subtitled lines; a consistent colour grade
  (warm nebula against cold metal); shallow depth of field on close subjects; long lenses for scale shots.
Triad Drift's version is **bright and saturated**, not grimdark (AGENTS.md section 2), and uses our own assets.

**Shot list (about 45 to 55 s, every shot real-time in `L_Galaxy` with the game's own assets)**
| # | Shot | Camera | Duration |
|---|---|---|---|
| 1 | Black. First narration line fades in. The nebula fades up behind it | locked, 35 mm, a very slow push | 6 s |
| 2 | Verdance in eclipse: ringed planet silhouetted, sun breaking over its limb (rim light, lens flare) | long lens 85 to 135 mm, slow lateral dolly | 6 s |
| 3 | The squad: Rift, Lancer and Bastion pass close in formation, engines streaking; each pilot's name card as it passes (C-03 #1 style) | 24 mm low angle, whip-pan follow, shallow depth of field | 8 s |
| 4 | Threading the asteroid belt (ice, metal and crystal rocks, debris) with the squad small in frame for scale | 50 mm drifting through the belt, foreground rocks parallax | 6 s |
| 5 | Cinder, red and volcanic, far off; a portal gate ring flares as the squad passes through | 35 mm, track with the squad | 6 s |
| 6 | The threat: red warp-in flashes; Null Armada fighters pour out; the Dominion's silhouette slides across the stars, vast | 135 mm from far away, slow push; then 24 mm as fighters scream past | 10 s |
| 7 | Hard cut to black. Title card **TRIAD DRIFT**, then the slate "SCENE 1 · WAVE 1 · TAKE 1"; the camera eases into the chase camera behind the player's ship and control starts | blend 1.0 s into the chase camera | 6 s |

- **Look:** 2.39:1 letterbox (the Director's Cut bars), Cine Camera with real filmback and focal lengths, depth of
  field focused on the subject, gentle handheld noise on close shots only, motion blur on, exposure locked per shot,
  bloom and lens flare on engines and suns, a light film grain, colour grade per the art direction (saturated
  nebula hues; squad colours magenta / cyan / amber on the ships, red-orange on the Armada).
- **Narration:** 4 to 6 short lines as subtitles (text cards), written in the Director's Cut voice (a director
  framing the story: "Scene one. A wing of three ..."), localisable text; an optional voice-over cue per line plays
  when presentation assigns one. Music: an intro cue that swells into wave 1 (presentation slot; silence is allowed
  until Audio assigns it).
- **Rules:** plays once on **New Game** (not on Continue, not on a retake); skippable at any time after 1 s with any
  key, click or pad button (hold for 0.5 s to avoid accidental skips; a "HOLD TO SKIP" hint); never takes input
  away for longer than the intro; the HUD is hidden during it; the wave director, enemies and squad AI do not start
  until it ends; the game state is identical whether it was watched or skipped.
- **Presentation override:** if Nick's side authors a Level Sequence (`UTDUIStyle::IntroSequence`), it plays
  instead of the code-driven shots, with the same skip and hand-off rules.
- **Performance:** 60 fps on the target machine at 1080p; shots may hide gameplay-only actors that are not in frame.

**Done when:** New Game plays the intro from the menu into wave 1 with a seamless hand-off; skip works by keyboard,
mouse and pad; Continue and retakes never play it; captures of every shot are reviewed against `AAA.md` by an
independent reviewer; tests cover play-once, skip, hand-off and that waves start only after it.

#### ENG-34 · Planet worlds: large procedural terrain (CORE, added Nov 13 by Ryan) · `🟨`
Ryan, Nov 13: "building out these worlds ... procedural generation, like mountains ... make it look big, not just
one little section on the map ... a massive, massive map that you should be able to explore."
- **What:** each planet (Verdance, Cinder) is a large generated world around the existing objective course, not a flat
  slab: mountain ranges, valleys, ridges, mesas and basins from layered noise (ridged for mountains), seeded per planet
  so the world is the same every run. Target extent: at least 16 x 16 km walkable and flyable, with distant ranges
  beyond it that close the horizon.
- **The course stays playable:** the objective sites, landing pad and the routes between them sit in terrain that is
  flattened and blended around them (a basin / plateau), at the same positions and heights gameplay uses today; no
  existing objective, encounter or test moves. Outside it the player can walk and fly anywhere.
- **Built at runtime, in code:** chunked terrain meshes with level of detail by distance, collision near the player
  and the ships, no map edits (the maps are shared). Nick's tagged environment replaces it when present.
- **Looks:** a terrain material that blends by slope and height (Verdance: moss and soil lowlands, rock faces, glowing
  ridgelines; Cinder: basalt plains, lava rivers in the valleys, ash and crystal on the heights), the planets' fog and
  sky, and biome scatter from the existing kits (trees, ferns, mushrooms, rocks, crystals, basalt columns) by height,
  slope and noise masks, instanced, never on the course's paths.
- **Flight:** atmosphere flight (ENG-20) flies over and between the mountains; arrival and take-off stay as they are;
  the landing assist still finds the pad.
- **Performance:** inside the ENG-28 budget on the target machine; generation is incremental (no hitch above 4 ms a
  frame) and the first view is ready before the arrival camera shows it.
- **Acceptance:** from the pad the player sees mountains in every direction; a 2-minute walk or flight in any direction
  finds new terrain, not an edge; every existing planet test and both objective chains still pass; captures from the
  air and on foot on both planets.
- Owner: T4 (world generation, terrain meshes, scatter), T5 (course fitting and on-foot checks), T1 (flight over
  terrain), Lead (terrain material and art).

#### ENG-35 · Engineering fallback sound set (CORE, added Nov 13 by Ryan's Lead under Ryan's instruction) · `🟨`
Audio is a judged category and Nick's audio lane is stopped with only the menu theme and one cannon sound made. Until
Nick's sounds exist, engineering supplies an original, cohesive fallback set, announced on the board (R-024):
- every weapon, power and ability; hits, shield breaks and explosions; engines, boost, drift and hyperdrive; the switch
  cut, the slate and "CUT!"; UI moves and confirms; on foot: footsteps, blade swings, hits, deflects, crew shots;
  ambience for space and each planet; a combat music bed if none exists.
- Original sounds made with synthesis and processing (the `ue-game-audio` skill), levelled to its mix standard, each
  with attenuation, class and concurrency; logged in `CREDITS.md`.
- Wired only through the existing cue slots, in engineering folders, as fallbacks: a sound of Nick's in the same slot
  always wins, and nothing of his is overwritten. An agent reaches "technically verified"; a person approves by ear.
- Owner: Lead (builder sub-agents) for the sounds, each lane's owner for any missing cue slot.

#### ENG-36 · HUD redesign: modern, sleek, minimal (CORE, added Nov 13 by Ryan) · `🟨`
Ryan, Nov 13: "The HUD currently is kind of bulky and it needs to be more modern and sleek, and not taking up as much
room. We need to just redo the entire HUD ... a similar space type of HUD feel that doesn't take up so much space and
it's easy to utilize, and even the beginner gamer can understand it."
- **Research first:** how current games lay out a flight / action HUD (what sits near the reticle, what sits in the
  corners, what hides until it matters), written up with a wireframe before any code. Original design; other games are
  references for principles only.
- **Budget:** in normal flight the HUD covers no more than about 8% of the screen (the current one covers about 25%
  along the bottom), with the centre and the lower middle clear for the ship.
- **Principles:** the few things a pilot needs every second sit close to the reticle or the ship (hull and shield of
  the flown ship, the signature power's readiness, the target and its health); everything else is small, at the edges,
  and appears only when it changes or matters (wingmen status, ammo and heat, objective, wave, hyperdrive, hints);
  icons and bars before numbers; one accent colour per squad ship; plain words; a new player can tell, without
  reading a manual, which ship they fly, how hurt they are, what to shoot and what the big button does.
- **States:** space combat, travel, atmosphere flight, on foot (first and third person), the Dominion assault, the
  director's moments (cut card, slate, "CUT!"), pause; keyboard and pad prompts that follow the device in use;
  1080p and 1440p, 16:9 and ultrawide safe areas; colour-blind-safe shapes as well as colours.
- **Built in engineering's own code and content** (not in `Plugins/TriadDriftHUD`, which stays Nick's and untouched),
  on the existing HUD data contracts (`ITDHUDData`, the UI host). It is the default HUD; a setting keeps Nick's
  widget selectable, and his HUD replaces this one whenever he says so.
- **Acceptance:** before / after captures in every state above; the coverage measured; a fresh reviewer playing the
  "first five minutes" checklist can name each element's meaning; tests for every data binding.
- Owner: T2 (build), Lead (research, design, review).

#### ENG-37 · Projectiles, engines and particle effects at AAA quality (CORE, added Nov 13 by Ryan) · `🟨`
Ryan, Nov 13: "really focus on projectiles across the entire game, from the blast of the engine to shooting.
Everything kind of looks pixely and AI generated right now ... AAA quality projectiles ... you can see the flames
coming out and there's movement ... a bunch of particles, and everything that makes a game look really solid."
- **What:** real particle effects (Niagara) replace the flat glow shapes everywhere something fires, burns, hits or
  explodes. Each effect has a core, motion and secondary particles, in the squad / enemy colour code:
  - **Projectiles:** each weapon's shot has a hot core, a soft glow, a tapered trail that lingers, and sparks or
    plasma wisps shed along the path: Rift bolts (magenta, phasing), Lancer needles (cyan, tight and fast), Bastion
    shells and missiles (amber, heavy, with smoke trails and a flickering motor flame), enemy fire (red-orange),
    turret and Dominion fire, crew blaster bolts on foot.
  - **Muzzle flashes** on every weapon: a brief shaped flash, sparks, a light pulse.
  - **Engines:** layered exhaust on every ship: a bright inner flame cone with turbulence, heat shimmer, a longer soft
    plume, boost afterburner rings and sparks, idle flicker; enemies in red-orange; wingman trails as thin ribbons.
  - **Impacts:** shield hits (a ripple on the shield surface and blue sparks), hull hits (sparks, debris flecks, a
    scorch flash), ricochets, on-foot hits and deflects.
  - **Explosions and deaths:** fireball with rolling motion, shockwave ring, sparks and hull chunks on the ship's
    velocity, smoke, embers; larger and multi-stage for big ships and the Dominion breakup.
  - **Powers and travel:** ram wake, portal rims and missile return, swarm launches, dome, cloak shimmer, EMP wave,
    gravity well, hyperdrive streaks and exit flash, gate transit, warp-ins.
- **Quality bar:** nothing reads as a flat disc, a billboard square or a low-resolution sprite at any gameplay
  distance; effects move (turbulence, flicker, stretch by velocity), fade smoothly, and stay readable in a 40-ship
  fight without hiding targets; HDR emissive tuned to the game's exposure and bloom so cores are hot but not blown out.
- **Performance:** effects are pooled, GPU-simulated where the count is high, bounded per system, and scale down by
  distance; a full wave-5 fight stays inside the ENG-28 frame budget.
- **Wiring:** through the existing cue slots (`FTDFXCue::VFX`) and engineering fallback data assets; the code
  placeholders stay as the fallback when an effect is missing; an effect of Nick's in the same slot always wins.
- **Acceptance:** before / after captures of every effect at gameplay distance and in slow motion; a fresh reviewer
  scores effects at 4 of 5 or better; frame-time measured in a wave-5 fight.
- Owner: Lead (effects builder sub-agents), T2 (combat cues and placeholders), T1 (engines and trails), T4 (enemy,
  Dominion and world cues), T5 (on-foot cues).

### STRETCH: boarding finale

#### ENG-29 · Docking & boarding transition · `⬜` STRETCH
`ATDDockingBay` approach cone with a tractor-beam assist that reduces control, then
`TravelTo(DominionInterior)`; the hangar landing hands off to first person (§8).

#### ENG-30 · Dominion interior encounters & core fight · `⬜` STRETCH
- **Interior:** 6 sections (hangar, a deflect-lesson corridor, barracks, a vertical shaft with
  sweeper hazards, a security checkpoint, then the Warden) leading to the core chamber.
- **Core fight, phase 1:** 4 conduits at 150 HP each; the core fires deflectable orbs.
- **Core fight, phase 2:** a sweeping laser; the heart is vulnerable whenever the rings align, every
  6 s.
- **Core fight, phase 3:** at 25% health, everything speeds up.

#### ENG-31 · Escape run & escape flight · `⬜` STRETCH
- **Escape run:** a 90 s run back to the hangar on a changed route.
- **Escape flight:** a 60 s flight out through the falling debris, then the breakup sequence and
  victory. Failing either restarts it as the next take.

---

## 6. 🟩 Nick: Art, Visuals & Player Experience

**Start here (role brief, Day 1 focus):** establish the art direction and the visual asset inventory,
make the ships readable, and do a minimal combat presentation pass alongside Ryan's playable
foundation.

**Deliverables:**
- a coherent, attractive visual style across all three ships and the combat spaces
- distinct visual and audio feedback for Rift, Lancer and Bastion
- a readable, functional combat HUD and ship-switch presentation
- polished effects and environment dressing that don't compromise the build

**Tools:** the `blender-*` skills (`.claude/skills/`; Codex: see the index in AGENTS.md §10) for
making, cleaning up and exporting 3D assets. The palettes and budgets are in
`blender-game-asset/references/art-direction.md`.

#### ART-01 · Art direction & style guide · `⬜`
- **Content:** a one-page style guide with palettes, the faction shape language, readability rules
  (player projectiles in squad colours, enemies in red and orange), lighting mood per level, and
  the **Director's Cut visual language** (cuts, letterbox, slate cards, grade; C-03).
- **Reference board:** inspiration only; no copying of existing IP.
- **Gate:** both humans agree on the style before mass production.

#### ART-02 · Visual asset inventory & sourcing plan · `⬜`
- **The list:** every asset the game needs (ships, enemies, Dominion pieces, props, environment sets,
  characters, VFX, UI and audio), each with its route: AI image-to-3D (Rodin first: credits expire
  Nov 20, and using it enters the prize pool), Fab or other licensed library, code-built Blender, or
  the template.
- **Status and credits:** track each asset's status here or in `ArtSource/inventory.md`, and log
  licences in `CREDITS.md`.

#### ART-03 · Ship visuals: Rift, Lancer, Bastion · `⬜`
| | Rift | Lancer | Bastion |
|---|---|---|---|
| Silhouette | Crescent wing + ring-shaped portal emitter on the nose | Long needle fuselage, forward-swept wings | Wide, chunky hull, twin missile pods, dome emitter |
| Colours | Violet `#6B2BD9` / magenta glow `#FF2BD6` | Ice white `#E8F4FF` / cyan `#12C8FF` | Amber gold `#F2A01E` / gunmetal |
| Part animations | Portal ring spin-up, gear, canopy | **Wing fold for the ram**, gear, canopy | **Missile pods open**, dome emitter rises, gear, canopy |

- Models via route A (concept → Rodin → cleanup) or the kits, passing the AAA rubric (the
  `blender-game-asset` skill). Sockets: muzzles, engines, cockpit camera.
- Delivered as `DA_<Ship>_Visuals` (`DA_Rift_Visuals`, `DA_Lancer_Visuals`, `DA_Bastion_Visuals`; exact
  names in `docs/visual/Presentation-Asset-Paths.md`, a differently named asset is silently ignored) (§3.5), with animations driven by the exposed state. **Readable as
  black silhouettes at 64 px.**

#### ART-04 · Enemy & Dominion visuals · `⬜`
- **Null Armada ships:** Shard Fighter, Hook Interceptor, Wall Gunship (shield emitter), Maul Bomber,
  Hive Carrier (animated bay doors).
- **Dominion exterior:** a modular Nanite hull with rotating rings, shield generators, turrets, the
  reactor trench (core ending) and the hangar mouth (stretch).
- **Look:** angular, red/black/white-eye shape language. Telegraph glows tied to the gameplay states.

#### ART-05 · Space environment & galaxy dressing · `⬜`
- **Space assets:**
  - `BP_GalaxySkybox`: layered nebula, stars, sun with lens flare.
  - `BP_PlanetVisual`: rings, atmosphere rim, Verdance and Cinder variants.
  - Nebula volumes, a speed-dust field, asteroid-field kits (`procedural_rocks.py`), station ruins.
- **Galaxy map:** dress `L_Galaxy` on your own actors (One File Per Actor): lighting, post-process
  and scale cues.

#### ART-06 · Planet environments: Verdance & Cinder (CORE) · `⬜`
- **Verdance:** bioluminescent jungle, giant mushrooms and ferns, waterfalls, mist, ringed moon,
  pollen, critters, wreck-site and relay-spire art.
- **Cinder:** basalt, magma rivers, cyan and amber crystal spires (crystal kits), falling ash, an
  outpost kit, the jammer tower and its destruction.
- **Across both:** terrain materials, foliage instancing, sky and atmosphere, lighting, and the
  atmospheric-descent vista (canyons and clouds the ship flies through).

#### ART-07 · Combat & travel VFX (Niagara) · `⬜`
- **Explosions** in 4 tiers; engine trails per squad colour; boost, drift and speed lines; warp-ins
  (squad white-blue, Armada red tear).
- **Every weapon's muzzle, tracer and impact** (hull, shield, rock); shield ripples; overheat vents;
  missiles and flak.
- **Powers:** portals (ring, swirl, transit flash), ram cone and sonic boom, missile swarm trails,
  shield dome (hex, ripples, shatter).
- **Abilities:** Gravity Well (lens distortion, accretion), Cloak (refraction shimmer), EMP (ring and
  arcs).
- **Travel:** hyperdrive (spool, tunnel, drop-out), atmospheric entry plasma, landing dust,
  tractor beam (stretch).
- **Readability rule:** nothing important hidden by bloom. Budgets: particle caps and significance
  (ART-13).

#### ART-08 · Ground visuals & animation · `⬜`
- **First-person arms:** idle, run, sprint, jump, slide, mantle and dash; blade ignite and retract;
  combo ×3, lunge, air slam, block, deflect flourish, guard break; interact and carry; climb into
  and out of the cockpit.
- **Energy blade:** emissive core and trail, scorch decals, the perfect-deflect flash.
- **(Nov 12)** The Null Armada trooper model (Trooper, then Grunt and Heavy variants) is built by engineering
  from Ryan's concept (`ArtSource/EngCharacters/NullTrooper/`, `MESSAGEBOARD.md` R-019); the rest of this item
  stays with Nick.
- **Crew models and animations:** idle, locomotion, telegraphed attacks, hit-reacts, stagger,
  deaths, ragdoll-ready; Fab or Mixamo retargets, credited. A visor glow shows alert state.

#### ART-09 · HUD & UI/UX · `⬜`
- **`WBP_SpaceHUD`** (via `ITDHUDData`):
  - **Squad panel:** three cards showing hull, shield, cooldown rings, ability padlocks and respawn
    timers; the **switch animation** ties into the "cut" (C-03 #1).
  - **Combat:** crosshair, target bracket with health, lead indicator, lock brackets, hit and kill
    markers; arrows for enemies, missiles and objectives.
  - **Status:** boost, heat and ammo, hyperdrive charge; banners for "WAVE X", "ABILITY READY",
    the boundary warning and Switch Strike.
  - **Atmosphere variant:** altitude and the landing prompt.
- **`WBP_GroundHUD`:** health, block stamina, perfect-deflect flash, dash charges, objective tracker,
  interact ring, damage direction, escape timer (stretch).
- **Menus and screens:**
  - Main menu (animated galaxy background), Pause, Settings (audio, sensitivity, invert, difficulty,
    subtitles, shake, graphics), Controls.
  - **(Nov 12, Ryan)** The Settings page also carries the on-foot **first / third person** choice (ENG-32).
    Engineering ships a code-built page; a styled one goes in `UTDUIStyle::SettingsWidgetClass`.
  - Galaxy map; `WBP_AbilityUnlocked` (the big reward moment); Defeat, Victory (stats, rank, takes)
    and Credits; Loading.
- **Onboarding:** `WBP_Tip` toasts from `DT_Tips` (on `OnTipRequested`), with glyphs that switch
  between keyboard/mouse and gamepad.
- **Director's Cut UI:** pilot name cards on switch (C-03 #1); scene slate cards and the "CUT!" /
  next-take restart (C-03 #3); takes per scene on the victory screen.

#### ART-10 · Audio & music · `⬜`
- **Space SFX:** engine loops per ship that rise in pitch with throttle, drift screech, every
  weapon's fire and impact (3+ variations each), overheat, lock tones, missile alarm, portals, ram,
  dome, abilities, explosion tiers, shield vs hull hits, low-hull alarm, the switch "cut" whoosh,
  the hyperdrive set, atmosphere wind, gear, canopy, the Dominion horn, turrets, generator kills,
  the breakup roar.
- **Ground SFX:** footsteps by surface, dash, slide, all blade sounds, crew barks, attacks and
  deaths, drones, doors, hazards, magma, crystals, alarms.
- **Music director:** stems that swell with `OnWaveStarted(Intensity01)` and calm on clear; planet
  exploration and combat layers; the Dominion and finale track; stingers (ability unlocked, reveal,
  generator down, reactor destroyed, "CUT!").
- **Mix:** sound classes and submixes (Master, Music, SFX, VO, UI) wired to Settings; music ducks
  under VO.
- **Sources:** licensed only (e.g. the WeLoveIndies jam pack: non-commercial), logged in CREDITS.md.

#### ART-11 · Narrative presentation: barks, VO, intro · `⬜`
- `DT_Barks` (about 80–120 lines: Wren, Kato, Moss and the Armada) played on
  `OnBarkRequested` through a priority queue, with radio-filtered VO (AI-voiced, credited) and
  subtitles.
- **Intro sequence (30–45 s, skippable):** the squad over Verdance's ring, then the Armada warps in.
  **(Nov 12, Ryan)** Engineering builds the intro as real-time shots in code (ENG-33); presentation supplies
  narration voice, music, the grade and optionally its own Level Sequence (`UTDUIStyle::IntroSequence`).

#### ART-12 · Cinematics & the Director's Cut presentation · `⬜`
- **Sequences:** the Dominion reveal, the reactor breakup and victory hero shot, landing and
  take-off shots, the planet descent breakthrough.
- **Theme hooks (C-03 #1–#3):**
  - the switch "cut" transition and name cards
  - director-cam rigs, framing and grade for the slow-motion highlight moments
  - scene slate cards and the "CUT!" restart
- **Trailer capture:** Movie Render Queue shots for ART-14.

#### ART-13 · Lighting, post-process & performance-minded art · `⬜`
- Lumen and Nanite settings per level, scalability presets wired to Settings, Niagara budgets,
  foliage culling.
- Hold the art budgets in `art-direction.md` and keep VFX and environment complexity within a steady
  60 fps on mid hardware. Profile with Ryan before the freeze.

#### ART-14 · Submission presentation · `⬜`
- **Gameplay video (30–60 s):** gameplay in the first 5 seconds. Dogfight → switch "cut" → portal
  missile return → ram → planet landing → blade deflect → Dominion reactor.
- **itch.io page:** cover art, 5+ screenshots and a GIF, the description, controls, credits and
  "Tools you used". Compile the **dev log** (Glass Box prize) from `DEVLOG.md` with Ryan.

### STRETCH: boarding finale visuals

#### ART-15 · Dominion interior kit & core visuals · `⬜` STRETCH
- **Interior kit:** modular pieces on a 400 cm grid (corridor straight, corner, T, doorway, shaft,
  catwalk, door) in gloss black with red strips; giant eye lenses that track the player.
- **Core chamber:** the white-light reactor with rotating rings, plus the collapse and alarm states
  for the escape run.

#### ART-16 · Boarding & escape cinematics/VFX · `⬜` STRETCH
Tractor-beam docking shot, hangar landing, core-destroyed sequence, escape debris and explosion
VFX, and the squad's launch from the hangar.

---

## 7. Landing: full sequence (CORE)

| # | Step | 🟦 Ryan (gameplay) | 🟩 Nick (presentation) |
|---|---|---|---|
| 1 | Marker appears (after waves 2 and 4) | Reveal marker; `ShowTip` / `RequestBark` | Marker visuals, bark VO, tip |
| 2 | Approach | Hyperdrive drops out in orbit; `ATDPlanetAtmosphere` trigger | Planet visual, atmosphere glow |
| 3 | Atmospheric entry | Trigger → `TravelTo(Verdance)` after 2.5 s | Heat plasma, shake, roar, white-out that hides the load |
| 4 | Breakthrough | Spawn in atmosphere flight mode at `PlayerStart_Descent`; wingmen in formation | Cloud-burst VFX, leftover plasma |
| 5 | Descent flight | Atmosphere flight, altitude data | Canyon and waterfall vista, wind audio |
| 6 | Land | Pad prompt → auto-land (gear and canopy state) | Gear and canopy animations, landing dust, thrusters |
| 7 | Exit ship | Hand-off to first person | Exit sequence, arms montage |
| 8 | Objectives | ENG-24–26 | ART-06/08/09, reward screen |
| 9 | Board ship | Allowed at any time (the player may fly around the planet); the board point is marked as the objective, and the way back to space opens, only once the planet's objectives are done | Enter sequence |
| 10 | Take-off and exit | Lift-off, ceiling → `TravelTo(Galaxy, "<Planet>_Orbit")` | Take-off shot, plasma burst |

## 8. Boarding: full sequence (STRETCH)

| # | Step | 🟦 Ryan | 🟩 Nick |
|---|---|---|---|
| 1 | Defenses down (ENG-18) | Docking bay opens instead of the space-only reactor ending | Bay-door animation, beacon light |
| 2 | Docking approach | Approach cone, tractor-beam control reduction | Tractor-beam VFX, cinematic camera |
| 3 | Hangar landing → step out | `TravelTo(DominionInterior)` → first-person hand-off | Hangar shot, doors seal, red alarm, bark |
| 4 | Interior → core | ENG-30 | ART-15 |
| 5 | Escape run → launch → escape flight | ENG-31 | ART-16 |
| 6 | Breakup → Victory | `TravelTo(Victory)` | Breakup sequence (shared with the core ending) |

---

## 9. Integration points

| # | Integration | 🟦 Ryan delivers | 🟩 Nick delivers |
|---|---|---|---|
| I-1 | Presentation hooks | Data-asset classes, spawn-at-event code, exposed animation state | Data-asset instances + VFX, SFX, meshes and animations |
| I-2 | HUD and menus | `ITDHUDData`, `ITDGameFlow`, events | All widgets |
| I-3 | Maps | Maps, gameplay actors, collision, spawns | Dressing, lighting, post-process, audio volumes (One File Per Actor) |
| I-4 | Waves → music and barks | `OnWaveStarted` / `OnWaveCleared`, `OnBarkRequested` | Music director, bark queue, VO |
| I-5 | Abilities | Unlock logic + `OnAbilityUnlocked` | Reward screen, ability VFX and SFX |
| I-6 | Travel and landing | Travel, atmosphere flight, auto-land | Loading screen, entry, landing and take-off presentation |
| I-7 | Director's Cut (C-03 #1–#3) | Switch-cut timing, highlight events + slow motion, scene/take restarts | Cut transitions and name cards, director cams, slate cards and "CUT!" |
| I-8 | Packaging | Build pipeline, performance profiling | Art budgets, scalability settings |
| I-9 | Submission | Packaged build, upload, form | Video, page, screenshots, dev log |

**Integration ritual** (whenever either of you lands something big): pull → play from the main menu
→ package → log bugs in §11 → each bug goes to the owner of the file it lives in.

---

## 10. Dependency order (no dates)

Both of you run in parallel. This shows **what unblocks what**, not a schedule.

```
C-01 Project ─► C-02 Contracts (+ presentation hooks) ─┬─► 🟦 ENG-01 Flight ─► ENG-04 Ships ─► ENG-05 Weapons
                                                       │      ─► ENG-07 Switching ─► ENG-08 Wingmen      (Ryan's start-here)
                                                       │   ENG-13 Enemies ─► ENG-14 Waves ─► ENG-18 Dominion finale
                                                       │   ENG-09/10/11 Signature powers ─► ENG-12 Abilities
                                                       │   ENG-19 Game flow ─► ENG-20 Landing ─► ENG-21/22/23/24 First person
                                                       │      ─► ENG-25 Verdance / ENG-26 Cinder
                                                       │   ENG-28 Packaging: early and continuous
                                                       │
                                                       └─► 🟩 ART-01 Style ─► ART-02 Inventory ─► ART-03 Ships (readability first)
                                                              ─► ART-07 Combat VFX + ART-10 combat SFX    (Nick's start-here)
                                                           ART-09 HUD (against the fake HUD data) · ART-05 Space environment
                                                           ART-06 Planets · ART-08 Ground visuals · ART-11 Barks
                                                           ART-12 Director's Cut presentation · ART-13 Performance art
C-03 Theme decision ─► ENG event hooks + ART-12        STRETCH (after core is solid): ENG-29–31 + ART-15–16
```

**Biggest risks (start early):**
- Ryan: ENG-01 flight feel, ENG-07 switching (protect it), ENG-09 portals.
- Shared: ENG-20 + ART-07 landing hand-off, ENG-22 deflect.
- Nick: ART-03 hero-ship quality.
- Both: packaging (ENG-28). Package early and often.

**Feature freeze: Nov 15, 12:00 PM EST.** Every major feature should be in by then; after that, only
fixes, polish, packaging and submission.

---

## 11. Bugs & requests (append-only)

Add new lines at the bottom only, to avoid merge conflicts. Format:
`[for Ryan|Nick] description — reporter, timestamp`

### Requests between Ryan and Nick
- _e.g. `[for Ryan] Add a "drift spark" VFX slot to UTDShipVisuals — Nick, Nov 13 14:20`_

### Bugs
- _e.g. `[for Nick] WBP_SpaceHUD: shield bar doesn't update after respawn — Ryan, Nov 13 18:05`_
