# Triad Drift — Full Feature Spec & Work Split

> **Working title:** Triad Drift · **Jam:** 2026 Epic AI MegaJam · **Engine:** Unreal Engine (see AGENTS.md)
> **Hard deadline:** Nov 15, 2026 — **8:59 PM EST / 5:59 PM PST**. Late = disqualified.
> **Theme:** _TBD — fill in, then adjust anything marked 🎨 THEME._

**Scope: everything.** We build every feature in this file as fast as possible. Work isn't split by day. Each dev
owns a **lane** and works down it continuously, in whatever order the dependencies allow (see §10).

All numbers are **starting tuning values** (Unreal units: cm, cm/s, seconds). Change them freely, but keep them in
data assets or tables, not hard-coded in graphs.

**Status key** (edit inline as you go): `⬜ todo` · `🟨 in progress` · `✅ done` · `🐞 has bugs`

### Contents
0. [The game](#0-the-game) · 1. [The split](#1-the-split) · 2. [Folder ownership](#2-folder--file-ownership) ·
3. [Shared contracts](#3-shared-contracts) · 4. [Shared setup](#4-shared-setup-c-xx) ·
5. [🟦 Dev 1 — Space](#5--dev-1--space) · 6. [🟩 Dev 2 — Worlds & Presentation](#6--dev-2--worlds--presentation) ·
7. [Landing — full sequence](#7-landing--full-sequence-both-devs) · 8. [Boarding — full sequence](#8-boarding--full-sequence-both-devs) ·
9. [Integration points](#9-integration-points) · 10. [Dependency order](#10-dependency-order-no-dates) ·
11. [Bugs & requests](#11-bugs--requests-append-only)

---

## 0. The game

A fast, bright, arcade space-combat game. You lead a wing of three ships (**Rift**, **Lancer** and **Bastion**) and
switch between them at any moment. The two ships you aren't flying fight on their own. You land on two worlds and
complete ground objectives on foot to unlock new ship abilities. Then you break the enemy mothership's defenses, dock,
and fight through it in first person with an **energy blade** to destroy its core. Last, you escape as it breaks apart.

### Fiction (original, kept light) 🎨 THEME
- **The squad:** three pilots, one per ship.
  - **Wren** flies **Rift**: playful and sly.
  - **Kato** flies **Lancer**: cocky and fast-talking.
  - **Moss** flies **Bastion**: calm, protective veteran.
  - The player is the **wing lead**, who can take any cockpit. "Switching" means the lead takes over a ship by link.
- **The enemy:** the **Null Armada**, a faction of red-and-black angular ships with hollow white "eye" lights.
  Their mothership is the **Dominion**.
- **The worlds:**
  - **Verdance:** a bioluminescent jungle planet with giant flora, waterfalls and a ringed moon in the sky.
  - **Cinder:** a crystal and volcanic world with glowing magma rivers, crystal spires and ash storms.

### Full player journey

```
Main Menu ─► Intro cinematic (squad briefing over radio, Armada arrives)
  └─► GALAXY · Waves 1–2  (fighters; teaches flight, shooting, switching, each signature power)      [Dev 1]
        └─► Verdance marker → hyperdrive → atmospheric entry → fly down → land                       [Dev 1 → Dev 2]
              └─► VERDANCE · Obj 1 "Recover the Wreck Core"  → unlock GRAVITY WELL (Bastion)          [Dev 2]
              └─► VERDANCE · Obj 2 "Wake the Ancient Relay"  → unlock CLOAK (Rift)                    [Dev 2]
              └─► Board ship → take off → back to orbit                                               [Dev 2 → Dev 1]
  └─► GALAXY · Waves 3–4  (gunships, mixed fleets, portal gates matter)                               [Dev 1]
        └─► Cinder marker → land                                                                      [Dev 1 → Dev 2]
              └─► CINDER · Obj 3 "Clear the Armada Outpost" → unlock EMP PULSE (Lancer)               [Dev 2]
              └─► CINDER · Obj 4 "Destroy the Signal Jammer" → REVEALS THE MOTHERSHIP                 [Dev 2]
  └─► GALAXY · Wave 5 "Dominion's Guard" (carriers) → Dominion warps in (cinematic)                   [Dev 1]
        └─► Destroy 4 shield generators + turrets → docking bay opens → assisted docking              [Dev 1]
              └─► DOMINION INTERIOR · hangar → corridors → core chamber (energy blade)                [Dev 2]
                    └─► Core boss → destroyed → 90 s escape run to hangar → launch                    [Dev 2]
                          └─► ESCAPE FLIGHT through the breaking Dominion → breakup cinematic         [Dev 1]
                                └─► Victory screen → stats → credits                                  [Dev 2]
```

Target length: **20–30 minutes** for a first playthrough.

---

## 1. The split

| | **🟦 Dev 1 — "Space"** | **🟩 Dev 2 — "Worlds & Presentation"** |
|---|---|---|
| **Who** | _fill in_ | _fill in_ |
| **Owns** | Everything **in a ship**: flight (space and atmosphere), all ship weapons, signature powers, unlockable abilities, switching, wingman AI, enemy ships, waves, hyperdrive, portals, the galaxy map's gameplay, the Dominion's exterior, docking flight, the escape flight, ship animations, space VFX and SFX, and the space cinematics (Dominion reveal and breakup) | Everything **on foot or around the game**: game flow and progression, the first-person character and its animations, the energy blade, crew enemies and their animations, both planets and all objectives, landing-pad scenes, the Dominion interior and core, all UI, narrative and radio barks, music and mix, space environment art, the intro cinematic, packaging and submission |
| **Feature IDs** | `S-xx` | `G-xx` |
| **Lives in maps** | `L_Galaxy`, `L_Test_Space` | `L_MainMenu`, `L_Planet_Verdance`, `L_Planet_Cinder`, `L_Dominion_Interior`, `L_Transition`, `L_Test_Ground` |

**The golden rule:** a map's owner is the only person who edits that map. If a feature needs one dev's **class**
inside the other dev's **map**, the class owner builds it in their own folder, and the map owner places it. For
example, Dev 1 builds `BP_LandingPad` in `Space/Travel/` and Dev 2 drops it into `L_Planet_Verdance`.

---

## 2. Folder & file ownership

```
Content/TriadDrift/
├── Core/                  🔒 SHARED — frozen after C-02. Claim in CLAIMS.md + both humans agree before ANY edit
│   ├── Interfaces/        BPI_TD_GameFlow, BPI_TD_Team, BPI_TD_HUDData, BPI_TD_Deflectable, BPI_TD_Interactable
│   ├── Data/              E_TD_* enums, S_TD_* structs
│   └── Projectiles/       BP_TD_ProjectileBase (owner: Dev 1)
├── Space/                 🟦 DEV 1 ONLY
│   ├── Ships/             BP_ShipBase, BP_Ship_Rift/Lancer/Bastion, DA_ShipStats_*, ship meshes/materials/anims
│   ├── Squad/             BP_SquadManager, BP_SpacePlayerController, BP_GM_Space
│   ├── Weapons/           BP_Weapon_*, BP_Proj_*, DA_Weapon_*
│   ├── Powers/            BP_Power_Portal, BP_Portal, BP_Power_Ram, BP_Power_MissileSwarm, BP_Power_ShieldDome
│   ├── Abilities/         BP_Ability_Cloak, BP_Ability_EMP, BP_Ability_GravityWell
│   ├── AI/                wingman + enemy AI controllers, behavior/state trees
│   ├── Enemies/           BP_EnemyShipBase, BP_Enemy_Fighter/Interceptor/Gunship/Bomber/Carrier
│   ├── Waves/             BP_WaveDirector, DT_Waves
│   ├── Travel/            BP_Hyperdrive, BP_PortalGate, BP_PlanetAtmosphere, BP_LandingPad, BP_DockingBay, BP_AtmosphereFlightZone
│   ├── Dominion/          BP_Dominion_Exterior, BP_DOM_Turret, BP_DOM_ShieldGenerator, BP_DOM_EscapeDebris
│   ├── Cinematics/        LS_DominionReveal, LS_DominionBreakup, LS_ShipLaunch
│   ├── VFX/  Audio/       all space VFX + ship/weapon/space SFX
│   ├── Input/             IMC_Space, IMC_Atmosphere, IA_* (space)
│   └── Maps/              L_Galaxy (World Partition + One File Per Actor), L_Test_Space
├── Ground/                🟩 DEV 2 ONLY
│   ├── Character/         BP_FPCharacter, BP_GroundPlayerController, BP_GM_Ground, arms mesh + ABP_FPArms + montages
│   ├── Blade/             BP_EnergyBlade, DA_BladeCombo
│   ├── Enemies/           BP_Crew_Base, BP_Crew_Grunt/Trooper/Heavy/Sentry/Warden, BP_Proj_Crew*, anims
│   ├── Objectives/        BP_ObjectiveBase, BP_Obj_* , BP_Interactable_*
│   ├── Landing/           BP_LandedShip, LS_ExitShip, LS_EnterShip, BP_PadScene
│   ├── Planets/Verdance/  terrain, flora, props, materials
│   ├── Planets/Cinder/    terrain, crystals, magma, props, materials
│   ├── Dominion/          interior modular kit, BP_DOM_Core, BP_DOM_Door, BP_DOM_Hazard_*
│   ├── Cinematics/        LS_Intro, LS_CoreDestroyed
│   ├── Input/             IMC_Ground, IA_* (ground)
│   └── Maps/              L_Planet_Verdance, L_Planet_Cinder, L_Dominion_Interior, L_Test_Ground
├── Flow/                  🟩 DEV 2 — BP_TD_GameInstance, BP_TD_SaveGame, BP_GM_Menu, DT_Barks, DT_Tips
├── UI/                    🟩 DEV 2 — every WBP_*, icons, fonts, IMC_UI
├── Environment/           🟩 DEV 2 — BP_GalaxySkybox, BP_PlanetVisual, BP_NebulaVolume, BP_SpaceDust, BP_AsteroidField, BP_Sun
├── Audio/Music/  Audio/VO/  Audio/Mix/   🟩 DEV 2 — music stems, BP_MusicDirector, voice lines, sound classes/mixes
├── Maps/                  🟩 DEV 2 — L_MainMenu, L_Transition
└── Developers/<name>/     personal scratch; never referenced by real content
```

**Art sources (Blender files, AI originals, exports) live outside Unreal's Content/, with the same lanes:**

```
ArtSource/
├── Space/         🟦 DEV 1: player ships, enemy ships, Dominion exterior, ship weapons/FX meshes
├── Ground/        🟩 DEV 2: first-person arms/blade, crew, Dominion interior kit, objective props
├── Environment/   🟩 DEV 2: asteroids, rocks, crystals, flora, planet set pieces, skybox meshes
└── Shared/        🔒 claim first: palettes, trim sheets, decal atlases, calibration asset
    per asset: <Category>/<Asset>/{brief.md, refs/, source/, <Asset>_vNN.blend, review/, export/}
```
How to make the assets: the `blender-*` skills in `.claude/skills/` (index in AGENTS.md §10).

**Rules**
- **Never edit the other dev's folders or maps.** Ask through your humans, or add a line to §11 "Requests".
- **`Core/` is frozen** after C-02. To change it: claim it, get both humans' OK, and push the change on its own.
  **Add new things; never rename or remove existing enum values, struct fields, or interface pins.**
- **Project settings / `Config/*.ini`**: claim before editing (input, collision, maps & modes, GameInstance).
- **Collision channels** (created in C-01, use only these): `Ship`, `Projectile`, `Character`, `Interactable`, `Portal`.
- **Cross-lane assets are read-only references.** For example, Dev 2's `BP_LandedShip` references Dev 1's ship
  meshes but never edits them.

---

## 3. Shared contracts

These are created once in C-02. Both sides build against them and **stub** the other side until it's real.

### 3.1 Enums (`Core/Data/`)

| Enum | Values (append new values at the **end** only) |
|---|---|
| `E_TD_ShipId` | `Rift`, `Lancer`, `Bastion` |
| `E_TD_Team` | `Player`, `Enemy`, `Neutral` |
| `E_TD_AbilityId` | `None`, `GravityWell`, `Cloak`, `EMPPulse` |
| `E_TD_Destination` | `MainMenu`, `Galaxy`, `Verdance`, `Cinder`, `DominionInterior`, `Victory` |
| `E_TD_DamageKind` | `Energy`, `Kinetic`, `Explosive`, `Melee`, `Ram`, `Environmental` |
| `E_TD_GamePhase` | `Intro`, `Waves1to2`, `Verdance`, `Waves3to4`, `Cinder`, `Wave5`, `DominionAssault`, `DominionInterior`, `Escape`, `Victory` |

### 3.2 Structs

| Struct | Fields |
|---|---|
| `S_TD_Progress` | `Phase:E_TD_GamePhase` · `WavesCleared:int` · `CompletedObjectives:Set<Name>` · `UnlockedAbilities:Map<E_TD_AbilityId,E_TD_ShipId>` · `DominionRevealed:bool` · `DominionDefensesDown:bool` · `ReturnPoint:Name` · `LastShip:E_TD_ShipId` · `RunTimeSeconds:float` · `Stats:S_TD_RunStats` |
| `S_TD_RunStats` | `Kills:int` · `ShipSwitches:int` · `PortalsUsed:int` · `ShotsDeflected:int` · `Deaths:int` · `MissilesReturned:int` |
| `S_TD_ShipHUD` | `Ship` · `Health01` · `Shield01` · `Boost01` · `Heat01` · `SecondaryAmmo:int` · `PowerCooldown01` · `Power2Cooldown01` · `AbilityCooldown01` · `Ability:E_TD_AbilityId` · `IsPlayerControlled:bool` · `IsAlive:bool` · `RespawnSeconds:float` |

### 3.3 Interfaces

| Interface | Implemented by | Called by | Functions |
|---|---|---|---|
| `BPI_TD_GameFlow` | 🟩 `BP_TD_GameInstance` | Both | `TravelTo(Dest, ReturnPoint)` · `GetProgress()` · `SetPhase(Phase)` · `ReportWaveCleared(Index)` · `CompleteObjective(Id, Reward, ForShip)` · `SetDominionState(Revealed, DefensesDown)` · `ReportPlayerDefeated()` · `AddStat(Name, Amount)` · `RequestBark(BarkId:Name, Speaker:E_TD_ShipId)` · `ShowTip(TipId:Name)` · `SaveCheckpoint()` |
| `BPI_TD_Team` | Everything damageable | Weapons, AI | `GetTeam()` |
| `BPI_TD_HUDData` | 🟦 `BP_SquadManager` | 🟩 `WBP_SpaceHUD` | `GetSquadHUD()` · `GetCurrentTarget()` · `GetLockedTargets()` · `GetWaveInfo()` · `GetHyperdriveCharge01()` · `GetObjectiveMarkers()` · `GetSpeed01()` · `IsOutOfBounds()` · `GetAltitude()` (atmosphere only) |
| `BPI_TD_Deflectable` | `BP_TD_ProjectileBase` | 🟩 blade · 🟦 portal, dome | `Deflect(NewInstigator, NewDirection, NewTeam, SpeedMultiplier)` |
| `BPI_TD_Interactable` | Any interactable | 🟩 FP character | `GetPrompt() → Text` · `CanInteract(Instigator)` · `Interact(Instigator)` · `GetHoldTime() → float` |

### 3.4 Damage
- Use Unreal's **built-in `Apply Damage` / `Event AnyDamage`**. Pass `E_TD_DamageKind` through a custom
  `DamageType` class per kind (`DT_TD_Energy` etc., in `Core/Data/`).
- Friendly fire is **off**: always check `GetTeam()` before applying damage.

### 3.5 Event dispatchers both sides use

| Dispatcher | Lives on | Fires when | Listeners |
|---|---|---|---|
| `OnWaveStarted(Index, Intensity01)` | 🟦 `BP_WaveDirector` | A wave begins | 🟩 Music, HUD, barks |
| `OnWaveCleared(Index)` | 🟦 `BP_WaveDirector` | A wave ends | 🟩 Music, HUD, GameFlow |
| `OnShipSwitched(NewShip, OldShip)` | 🟦 `BP_SquadManager` | The player swaps ships | 🟩 HUD, barks, music sting |
| `OnShipDowned(Ship)` / `OnShipRespawned(Ship)` | 🟦 `BP_SquadManager` | A ship dies or respawns | 🟩 HUD, barks |
| `OnSquadWiped()` | 🟦 `BP_SquadManager` | All 3 ships are down | 🟩 GameFlow → defeat |
| `OnPowerUsed(Ship, PowerName)` | 🟦 `BP_ShipBase` | A signature power or ability fires | 🟩 barks, stats |
| `OnAbilityUnlocked(Ability, Ship)` | 🟩 `BP_TD_GameInstance` | An objective is completed | 🟦 ships refresh their ability slot |
| `OnPhaseChanged(Phase)` | 🟩 `BP_TD_GameInstance` | The story phase changes | 🟦 wave director, 🟩 music |

### 3.6 Stubs (so nobody is ever blocked)
- 🟦 `BP_Stub_GameFlow` (in `Space/`): fakes the GameInstance and prints calls.
- 🟩 `BP_Stub_HUDData` (in `UI/`): fakes squad data with sliders for testing widgets.
- 🟩 Uses a static mesh stand-in for `BP_LandedShip` until Dev 1's ship meshes exist.
- 🟦 Uses a placeholder sphere/cube skybox until Dev 2's environment actors exist.

---

## 4. Shared setup (`C-xx`)

### C-01 · Project bootstrap · 🟦 Dev 1 drives · `🟨`
- [x] Create the UE project and record the exact engine version in AGENTS.md. **Both devs must be on the same version.** Done Nov 12: `unrealGameJam`, First Person C++ template, UE 5.9.0 MegaJam Preview, at the repo root.
- [x] Add the Unreal `.gitignore` (+ `*.blend1`, `__pycache__/`) and `.gitattributes` with Git LFS (`*.uasset *.umap *.blend` + images, audio, meshes). Not marked `lockable`: we coordinate through CLAIMS.md instead, so files aren't read-only.
- [ ] Create the full folder tree from §2.
- [ ] Create the collision channels and object types (`Ship`, `Projectile`, `Character`, `Interactable`, `Portal`).
- [ ] Create empty maps for every map in §1. `L_Galaxy` uses World Partition with **One File Per Actor**, and big interior maps should too.
- [ ] Enable plugins: Enhanced Input, Niagara, (optional) State Tree, Common UI if used, Movie Render Queue for trailer capture.
- [ ] Create `CLAIMS.md`, `CREDITS.md` and `DEVLOG.md`. Push.
- [ ] Dev 2 clones, opens, and confirms there are no errors.

### C-02 · Contracts · 🟩 Dev 2 drives, Dev 1 reviews · `⬜`
- [ ] Create every enum, struct, interface and DamageType in §3 exactly as written.
- [ ] 🟦 Dev 1 creates `BP_TD_ProjectileBase`: speed, damage, kind, team, lifetime, homing target (optional), impact VFX/SFX slots. It implements `BPI_TD_Deflectable` (flip team, new direction, re-target the original instigator if homing) and `BPI_TD_Team`.
- [ ] Push. **`Core/` is now frozen.**

### C-03 · Theme pass · both · `⬜` 🎨 THEME
- [ ] Write the theme at the top of this file and in AGENTS.md. Decide how the squad, worlds and finale express it in **gameplay**.

---

## 5. 🟦 DEV 1 — Space

### S-01 · Space flight model · `⬜`
Arcade 6DOF-lite flight. It's fast, readable, and has a signature **drift**.

| Control | Behavior | Start values |
|---|---|---|
| Throttle | Analog forward speed between min and cruise | min 1500, cruise 6000 cm/s (per-ship multipliers in S-04) |
| Aim (mouse / right stick) | The ship turns toward an aim reticle (mouse = virtual joystick with a center deadzone) | max turn 110°/s pitch-yaw |
| Roll | Manual roll. Auto-banks into turns when there's no roll input | 180°/s |
| Strafe (optional) | Lateral/vertical thrusters | 2500 cm/s |
| **Boost** | Hold for a big speed increase; drains the boost meter | +80% speed, 3 s full meter, regens 1.2 s after release |
| **Drift** | Hold: velocity keeps its old direction while the nose rotates freely (slide sideways and shoot behind you). Release: snaps velocity to the nose with a burst | 1.5× turn rate while drifting; 20% speed burst on release |
| Brake | Hard decel to min speed | |

- [ ] `BP_ShipBase` (Pawn + FloatingPawnMovement or custom velocity integration), with all values in `DA_ShipStats`.
- [ ] **Camera:** spring-arm chase cam with rotation lag 8 and location lag 12. FOV 90 → 110 with speed. Camera roll follows bank at 30%. A subtle look-ahead offset toward the aim direction.
- [ ] Optional cockpit/close camera toggle (`IA_CameraToggle`).
- [ ] **Speed feel:** speed lines (Niagara screen-space), wind/engine audio pitch, motion blur on boost, and camera shake on boost start.
- [ ] **Collision:** glancing hits on asteroids bounce and scrape (sparks + damage proportional to impact angle). Head-on hits deal heavy damage.
- [ ] **Play-space boundary:** a soft sphere. Past it, a warning on the HUD (`IsOutOfBounds`), then you're gently steered back.
- [ ] Gamepad and mouse + keyboard are both first-class.

**Done when:** flying through an asteroid field in `L_Test_Space` is fun by itself, and drifting around a rock while
shooting backwards feels great.

### S-02 · Space input map · `⬜`
`IMC_Space` (and `IMC_Atmosphere`, which adds Land/Gear and drops Hyperdrive):

| Action | KB/M | Gamepad |
|---|---|---|
| Throttle up/down | W / S | RT / LT (analog) |
| Aim | Mouse | Right stick |
| Roll | A / D | Left stick X |
| Strafe vertical | Space / Ctrl | Left stick Y (optional mode) |
| Fire primary | LMB | R1 / RB |
| Fire secondary | RMB | L1 / LB |
| Boost | Shift | Left stick click |
| Drift | Alt or C | B / Circle (hold) |
| Signature power 1 | Q | X / Square |
| Signature power 2 (Bastion only) | Hold Q | Hold X / Square |
| Unlocked ability | E | Y / Triangle |
| Switch next / prev | Tab / Shift+Tab or Mouse wheel | D-pad right / left |
| Switch direct | 1 / 2 / 3 | D-pad up (hold → radial) |
| Target cycle | T / MMB | Right stick click |
| Hyperdrive | H (hold) | D-pad down (hold) |
| Squad command: focus my target | F | A / Cross (tap) |
| Land (atmosphere) / Interact | F | A / Cross (hold) |
| Pause / Galaxy map | Esc / M | Start / Select |

### S-03 · Ship health, shields, death & respawn · `⬜`
- [ ] **Shield** absorbs damage first and regenerates after 3 s without damage, at 25%/s. **Hull** doesn't regen in combat but slowly repairs between waves.
- [ ] **Hit feedback:** shield-hit ripple shader on the hull, hull-hit sparks and smoke, a low-hull fire trail below 30%, and a cockpit alarm SFX for the controlled ship.
- [ ] **Death:** the ship tumbles for 0.6 s, then a big explosion (Niagara + light flash + debris meshes + shockwave), and the camera pulls back. The player auto-switches to the healthiest living ship with a quick flash transition.
- [ ] **Respawn:** a downed ship warps back in near the squad after 12 s (warp-in VFX). The HUD card shows a countdown.
- [ ] **Squad wipe:** if all 3 ships are down at once, fire `OnSquadWiped`, then `ReportPlayerDefeated()`.
- [ ] Brief invulnerability (2 s) after respawn and after switching into a ship.

### S-04 · The three ships · `⬜`

| | **Rift** (Trickster) | **Lancer** (Interceptor) | **Bastion** (Heavy) |
|---|---|---|---|
| Pilot | Wren | Kato | Moss |
| Color | Magenta / violet engines | Cyan / white engines | Amber / gold engines |
| Silhouette | Crescent wing, ring-shaped portal emitter on the nose | Long needle fuselage, swept forward wings | Wide, chunky hull, two missile pods, dome emitter on top |
| Hull / Shield | 100 / 100 | 70 / 80 | 180 / 160 |
| Cruise / Boost speed | 6000 / 11000 | 7500 / 15000 | 4800 / 8500 |
| Turn rate | 120°/s (best) | 100°/s | 75°/s |
| Boost meter | 3 s | 4 s, fast regen | 2.5 s |
| Primary | Phase Pulse Lasers | Needle Cannons | Breaker Blaster |
| Secondary | Splitter Bolt | Spike Rockets | Flak Burst |
| Signature | **Portal Blaster** | **Afterburner Ram** | **Missile Swarm** + **Shield Dome** |
| Unlockable ability | **Cloak** (Verdance Obj 2) | **EMP Pulse** (Cinder Obj 3) | **Gravity Well** (Verdance Obj 1) |

- [ ] Child BPs `BP_Ship_Rift/Lancer/Bastion` + `DA_ShipStats_*`.
- [ ] Meshes: AI-generated (Rodin/Tripo/Meshy) or Fab, kitbashed to the silhouettes above. Every asset goes in `CREDITS.md`.
- [ ] Emissive engine and trim materials driven by a squad color parameter, plus engine-glow intensity tied to throttle.

### S-05 · Ship weapons (every weapon) · `⬜`
All weapons are `BP_Weapon_Base` components configured by a `DA_Weapon_*`. All projectiles are children of
`BP_TD_ProjectileBase`. Player projectiles use **squad colors**. Enemy projectiles are always **red/orange**.

| Weapon | Ship | Type | Damage | Rate | Proj. speed | Range | Resource | Notes |
|---|---|---|---|---|---|---|---|---|
| **Phase Pulse Lasers** | Rift | Twin alternating bolts | 8 each | 8/s | 40 000 | 40 000 | Heat: 4 s to overheat, 1.5 s cooldown lockout | Slight aim assist (3° cone) |
| **Splitter Bolt** | Rift | Charge shot (hold 0.8 s) | 40 core + 5×12 shards | — | 25 000 | 35 000 | 6 ammo, 1 per 4 s regen | Splits into 5 shards on impact or at max range; great vs swarms |
| **Needle Cannons** | Lancer | Rapid kinetic | 4 | 20/s | 55 000 | 30 000 | Heat: 5 s | Small spread (1.2°), fast tracers |
| **Spike Rockets** | Lancer | Dumb-fire salvo of 4 | 25 each, 300 cm splash | 1 salvo/1.5 s | 30 000 | 40 000 | 8 salvos, regen 1 per 5 s | Straight-line, high burst |
| **Breaker Blaster** | Bastion | Heavy plasma | 30, 250 cm splash | 2.5/s | 28 000 | 35 000 | Heat: 6 s | Knockback on fighters |
| **Flak Burst** | Bastion | Proximity flak | 15 per fragment, 800 cm burst | 1/1.2 s | 22 000 | 20 000 | 10 shells, regen 1 per 3 s | Detonates near enemies or projectiles; **shoots down missiles** |

- [ ] Each weapon has a muzzle flash, a tracer/trail, an impact VFX on hull vs shield vs rock, fire SFX (with variations), and an impact SFX.
- [ ] Heat or ammo state is exposed to the HUD. Overheat shows a vent steam VFX and a hiss.
- [ ] **Lead indicator** for the current target (HUD data: predicted intercept point).
- [ ] **Hit-confirm:** hit-marker event, kill-confirm event (bigger marker + sound), and 40 ms hit-stop on kills.

### S-06 · Targeting & lock-on · `⬜`
- [ ] Soft target = the enemy closest to screen center within 25°. `IA_TargetCycle` cycles by distance.
- [ ] Missile lock (Bastion Missile Swarm): hold the power while sweeping the reticle over enemies to paint up to 6 locks (0.25 s per lock). Each lock plays a tone and draws a bracket. Release to fire.
- [ ] Off-screen enemy, incoming-missile and objective direction data for HUD arrows.
- [ ] **Incoming missile warning:** tracks enemy homing missiles targeting the controlled ship (beeping that speeds up).

### S-07 · Squad manager & ship switching · `⬜`
- [ ] `BP_SquadManager` spawns, tracks and respawns all 3 ships. It owns which ship the player controls and implements `BPI_TD_HUDData`.
- [ ] **Switch:** `IA_SwitchNext/Prev/Direct` → possess the target ship instantly.
  - **Camera:** 0.2 s blend with a chromatic streak along the line between the ships (Niagara beam from old to new ship) and a squad-color flash.
  - **SFX:** a "link" whoosh, plus a short radio bark from the pilot you're taking over (via `RequestBark`).
  - The old ship hands off to its AI controller on the same frame and keeps its velocity.
  - 0.4 s switch cooldown. You can't switch to a downed ship (the HUD shows it greyed out).
- [ ] **Switch Strike** (combo mechanic): switching within 1.5 s of using a signature power gives the new ship +25% damage for 4 s, with a glowing engine trail. This rewards chaining powers across ships.
- [ ] **Power persistence:** a power started on one ship finishes even after you switch away (the portal stays open, the dome stays up).
- [ ] Fires `OnShipSwitched`, `OnShipDowned`, `OnShipRespawned` and `OnSquadWiped`.

### S-08 · Wingman AI · `⬜`
- [ ] **States:** Formation → Engage → Evade → Regroup → Downed.
  - **Formation:** a loose V behind and to the side of the player (offsets 1500–2500 cm) with gentle drift so they look alive. They match the player's speed and use boost to catch up past 8000 cm.
  - **Engage:** picks the enemy threatening the player or itself (priority: attacking the player > nearest > lowest health). Makes attack runs with **primary and secondary weapons only**, then breaks off.
  - **Evade:** when its shield is broken or a missile is incoming, it barrel-rolls and boosts away for 2 s.
  - **Regroup:** returns to formation when no enemy is within 20 000 cm.
- [ ] **Never uses signature powers or unlocked abilities.** Those only fire when the player is in the cockpit.
- [ ] Follows the player through portals and portal gates and **jumps with the player** on hyperdrive.
- [ ] **Command** `IA_SquadCommand`: both wingmen focus the player's target for 8 s (bark: "On it, lead!").
- [ ] Simple avoidance (sphere traces ahead) for asteroids, the player and each other.
- [ ] **Shield Dome awareness:** wingmen move toward an active Bastion dome when they're damaged.
- [ ] Wingmen are slightly less accurate than the player (aim error 2–4°) so the player stays the hero.

### S-09 · Signature: Rift — Portal Blaster · `⬜` ⚠️ highest-risk feature
- [ ] **Fire 1** shoots a portal projectile that opens **Portal A** where it hits, or at max range (12 000 cm). **Fire 2** opens **Portal B**. A third shot recycles the oldest portal. A pair stays open for 12 s, and the power's cooldown is 8 s after the pair closes.
- [ ] **Ship transit:** any squad ship flying into one portal exits the other, keeping its speed and adopting the exit portal's forward direction. 0.3 s invulnerability on exit.
  - **Camera:** a quick white-violet flash and FOV punch, with no hard snap.
  - **SFX:** "whomp-shhh".
- [ ] **Missile return:** enemy projectiles (especially homing missiles) entering a portal exit the other, get `Deflect()`'d to the Player team, and **re-home on the enemy that fired them**. This counts toward `Stats.MissilesReturned`.
- [ ] **Enemy transit (fun bonus):** enemy fighters that fly into a portal come out the other side and lose tracking for 2 s.
- [ ] **Portal visuals:** a glowing magenta ring, a swirling interior shader showing a render-target view of the other portal (fallback: animated swirl), crackle particles, an open/close animation (scale 0 → 1 over 0.25 s with elastic overshoot), and a hum SFX with attenuation.
- [ ] **Tactics enabled:** a portal behind an enemy fleet, a portal next to a gunship's unshielded back, or a portal in front of incoming missiles.
- [ ] **Fallback if it's still buggy late:** "Blink": one shot teleports Rift to the impact point, and a separate 3 s missile-reflect field around the ship.

### S-10 · Signature: Lancer — Afterburner Ram · `⬜`
- [ ] Press: 0.3 s wind-up, during which the wings fold back (ship animation), the engines flare white, and you hear a rising whine.
- [ ] Then a 1.2 s dash at 3× boost speed. The ship is invulnerable and its collision switches to ram mode.
  - **Ram damage:** 300 to anything it hits. It **passes through** fighters (they explode) and **breaks gunship shields** and carrier bay doors.
  - **VFX:** a cone shockwave in front, a long cyan streak trail, and a sonic-boom ring at the start.
  - **Camera:** a heavy FOV punch and radial blur.
- [ ] Hitting something big (a carrier hull, the Dominion, an asteroid) stops the dash with a shockwave of 150 damage in 1500 cm and a bounce-back.
- [ ] Cooldown 7 s. Can be aimed mid-dash with 40% steering.

### S-11 · Signature: Bastion — Missile Swarm & Shield Dome · `⬜`
- [ ] **Missile Swarm (tap Q):** fires 12 micro-missiles split across painted locks, or the soft target if nothing is painted (lock painting is in S-06).
  - Each missile does 18 damage with a 200 cm splash.
  - **Flight:** missiles wobble out of the pods (the pods open as a ship animation) with spiral trails, then curve onto their targets.
  - Cooldown 9 s.
- [ ] **Shield Dome (hold Q 0.5 s):** a 3000 cm-radius hex-pattern amber bubble centered on Bastion, lasting 6 s. Cooldown 16 s.
  - Squad ships inside take **no damage** and regenerate shield 2× faster.
  - Enemy projectiles that hit the dome from outside are destroyed. **Missiles are deflected back.**
  - The dome follows Bastion slowly, so Bastion should slow down while it's up.
  - **Visuals:** a ripple VFX at each impact point, an expanding open animation and a shattering close animation.

### S-12 · Unlockable ship abilities · `⬜`
Each ship has one ability slot (`IA_Ability`), filled from `GetProgress().UnlockedAbilities` on spawn and on
`OnAbilityUnlocked`. A locked slot shows a padlock on the HUD.

| Ability | Ship | Unlocked by | Behavior | Cooldown | VFX / SFX |
|---|---|---|---|---|---|
| **Gravity Well** | Bastion | Verdance Obj 1 | Launches a slow orb (12 000 cm/s) that detonates on impact or after 1.5 s. It becomes a 5000 cm vortex for 5 s that **pulls enemies and enemy projectiles to its center**. Enemies inside take 10 dps and can't fire. It collapses in a 120-damage implosion. Combos with Missile Swarm and Flak. | 18 s | Dark purple swirling lens-distortion sphere, accretion-disc particles, deep bass hum, implosion boom |
| **Cloak** | Rift | Verdance Obj 2 | 6 s of invisibility. Enemies lose lock, missiles lose tracking, and AI ignores Rift. The first shot out of cloak deals **3× damage**. Firing or ramming breaks it early. The wingmen get a 1 s shimmer when it activates. | 15 s | Refraction shimmer shader, engine glow dims, soft "fade" whoosh; heartbeat audio while cloaked |
| **EMP Pulse** | Lancer | Cinder Obj 3 | An instant 6000 cm radial pulse. Enemies inside are **stunned for 3 s** (they drift and can't shoot), **gunship shields drop for 5 s**, and enemy missiles in range fizzle. Stunned enemies take +25% damage. | 14 s | Expanding cyan electric ring, arcs jumping between enemies, sparking stunned enemies, a crackling "ZZHHM" |

- [ ] Shared `BP_AbilityBase` (activate, cooldown, `OnPowerUsed` event, HUD data) with three children.
- [ ] Abilities only fire when the player is in that cockpit. The slot is visible on every ship's HUD card.
- [ ] On first unlock, the first time you're in that ship, a tip prompt fires (`ShowTip("Ability_GravityWell")`).

### S-13 · Enemy ships · `⬜`
`BP_EnemyShipBase`: team Enemy, health/shield, death explosion scaled by size, the Null Armada look (angular
red/black, glowing white "eye" lights), and it targets **any** squad ship.

| Enemy | Role | HP / Shield | Speed | Weapons | Behavior | Counter |
|---|---|---|---|---|---|---|
| **Shard Fighter** | Swarm | 30 / 0 | 7000 | Red pulse guns (5 dmg, 4/s) | Packs of 4–8, strafing runs, scatters when hit | Needle Cannons, Splitter Bolt, Flak |
| **Hook Interceptor** | Hunter | 50 / 20 | 9000 | Pulse guns + 1 homing missile (25 dmg) every 8 s | Chases the player's ship specifically | Portal missile return, Drift + shoot back |
| **Wall Gunship** | Tank | 400 / **frontal shield 300** | 3000 | Heavy bolts (20 dmg, slow and dodgeable) + homing missile salvo (3×20) | Faces the nearest ship; the shield blocks frontal shots | Flank, portal behind it, Lancer Ram, EMP |
| **Maul Bomber** | Area denial | 150 / 50 | 4000 | Drops slow plasma mines (60 dmg, 800 cm) | Flies through the squad's path laying mine trails | Flak shoots mines; Gravity Well pulls mines into enemies |
| **Hive Carrier** | Spawner / mini-boss | 2000 hull, 3 bays × 300 | 1500 | 4 point-defense turrets (6 dmg) | Launches 3 fighters per bay every 12 s until that bay dies; bays open with an animation before launch | Destroy bays (Ram breaks bay doors), Missile Swarm on turrets |

- [ ] Enemy AI: Approach → Attack Run → Break Off → Re-approach, plus formation flying for fighters (leader + followers).
- [ ] Enemies warp in with a red-tear warp VFX + SFX (shared with the Dominion's arrival).
- [ ] Every enemy has a visible **telegraph** before big attacks (a glow charge-up and a sound cue).
- [ ] Death variety: fighters pop with a spinning wreck, gunships break in half, carriers chain-explode over 3 s.

### S-14 · Wave director · `⬜`
- [ ] `BP_WaveDirector` + `DT_Waves` (rows: wave index, groups → enemy type, count, spawn zone, delay, intensity 0–1, bark on start).
- [ ] Spawn zones sit around the combat area, often **behind** or **flanking** the player, so portals, hyperdrive and drift matter.

| Wave | Phase | Composition | Intensity |
|---|---|---|---|
| 1 | Waves1to2 | 2 groups × 4 Shard Fighters | 0.2 |
| 2 | Waves1to2 | 3 × 5 Fighters + 2 Hook Interceptors | 0.35 |
| — | → Verdance | Verdance marker appears (bark: "Moss: Reading a wreck on Verdance. Could be useful.") | — |
| 3 | Waves3to4 | 2 Wall Gunships + 3 × 5 Fighters | 0.55 |
| 4 | Waves3to4 | 2 Gunships, 2 Maul Bombers, 4 Interceptors, fighters | 0.7 |
| — | → Cinder | Cinder marker appears | — |
| 5 | Wave5 | "Dominion's Guard": 2 Hive Carriers + escorts + bombers | 0.9 |
| — | → DominionAssault | `LS_DominionReveal`, Dominion assault (S-18) | 1.0 |

- [ ] Reads `GetProgress()` on load, so the right phase and wave resume after coming back from a planet.
- [ ] Between waves: a 6 s breather, hull repair, then a "WAVE X INCOMING" event. Fires `OnWaveStarted` / `OnWaveCleared` and implements `GetWaveInfo()`.
- [ ] Early-wave tutorial beats: `ShowTip` for switching (wave 1), each signature power (first time in each ship), drift (wave 1) and hyperdrive (first marker).

### S-15 · Hyperdrive · `⬜`
- [ ] **Hold H for 1.2 s:** an engine wind-up (rising SFX), stars stretching, the camera pulling back, and a hyperdrive bar on the HUD. It can't charge if the ship took damage in the last 2 s, and taking a hit cancels it.
- [ ] **Jump:** 80 000 cm/s straight line with 10% steering. Tunnel VFX (streaking stars, a nebula-color tube) and a wind roar. Lasts until release, reaching the target or hitting the boundary.
  - Auto-drops out near planet markers and the Dominion (a 15 000 cm "gravity well" radius).
- [ ] **Drop-out:** a flash, an FOV slam back, a boom SFX, and a brief speed carry-over.
- [ ] **Wingmen jump with you** and drop out in formation.
- [ ] Implements `GetHyperdriveCharge01()`. Hyperdriving to a marker sets the bark/tip context.

### S-16 · Portal Gates (galaxy shortcuts) · `⬜`
- [ ] 3 permanent `BP_PortalGate` pairs in `L_Galaxy`: Combat zone ↔ Verdance orbit, Combat zone ↔ Cinder orbit, and a "flank" pair used in waves 3–5 that opens behind the enemy spawn zone.
- [ ] Each one is a huge, ancient ring structure (much bigger than Rift's portals) with an idle glow, and it activates as the squad approaches. It reuses S-09's transit logic. Wingmen follow.

### S-17 · Galaxy map (`L_Galaxy`) layout · `⬜`
- [ ] **Zones:**
  - The central combat zone, with an asteroid field and the ruins of a station for cover.
  - Verdance orbit (a giant planet with a ring), Cinder orbit, and the Dominion approach (deep space, nebula).
  - All of it is reachable via hyperdrive in seconds.
- [ ] Places Dev 2's `BP_GalaxySkybox`, `BP_PlanetVisual` ×2 (+ background planets), `BP_NebulaVolume`s, `BP_SpaceDust` and `BP_AsteroidField`s.
- [ ] **`BP_PlanetAtmosphere`** (Dev 1 class) around each landable planet. Entering it while the marker is active triggers atmospheric entry (see §7).
- [ ] **Return points** (named): `Verdance_Orbit`, `Cinder_Orbit`, `Dominion_Hangar_Exit` (escape start). The squad spawns there when returning.
- [ ] Scale tricks: huge planets far away, a parallax skybox, and small ships against giant structures.

### S-18 · Dominion exterior, assault & docking · `⬜`
- [ ] **`LS_DominionReveal` (cinematic, ~8 s, skippable):** space tears red, the colossal Dominion slides out of warp, its lights snap on, and a deep horn sounds. A bark follows ("Kato: …that's a lot of ship.").
- [ ] **`BP_Dominion_Exterior`:** a massive layered silhouette (≥ 50× the size of Bastion) with glowing white "eye" lights, a red energy shield bubble and animated rotating rings.
- [ ] **Assault:**
  - 4 `BP_DOM_ShieldGenerator`s on the hull (glowing pylons, 800 HP each, protected by local turrets).
  - 8–12 `BP_DOM_Turret`s (rotating barrels, telegraphed charge shots).
  - Continuous fighter launches from hangars (reuses wave groups).
  - Each generator destroyed gives a big explosion, a section of the shield bubble flickering out, and a bark.
  - All 4 down: the shield collapses (cinematic VFX), `SetDominionState(DefensesDown=true)`, and the docking bay doors grind open (animation + SFX).
- [ ] **Docking (see §8):** the `BP_DockingBay` approach.

### S-19 · Atmosphere flight & landing (ship side) · `⬜`
See §7 for the full sequence. Dev 1 builds the ship-side pieces:
- [ ] **Atmosphere flight mode** on `BP_ShipBase` (`IMC_Atmosphere`): gravity pulls the ship down gently, top speed is capped (60% of space cruise), there's no hyperdrive, and the camera has more horizon. Wind buffeting, cloud particles and an altitude readout (`GetAltitude`).
- [ ] **`BP_AtmosphereFlightZone`:** bounds for flying on a planet (soft walls) plus a ceiling. Flying above the ceiling with the objectives done means take-off.
- [ ] **`BP_LandingPad`:** a glowing beacon column visible from far away. When the ship is within 3000 cm and below 40% speed, a "Hold F to land" prompt appears (via HUD data).
- [ ] **Auto-landing:** control is taken. The ship aligns to the pad, the **landing gear deploys** (animation), the ship descends with thruster dust kicked up (Niagara reading the surface), it touches down with a suspension squash, the engines spool down and the **canopy opens** (animation). Then it calls the hand-off (`OnShipLanded` dispatcher → Dev 2's exit sequence).
- [ ] **Take-off:** the reverse. Canopy closes, engines spool, gear retracts, a vertical lift-off, then control returns at 30 m altitude.
- [ ] **Wingmen** auto-land on the pad's two side spots, or hover overhead if there's no room.

### S-20 · Escape flight · `⬜`
- [ ] Starts at `Dominion_Hangar_Exit` in `L_Galaxy`. `LS_ShipLaunch`: the squad blasts out of the hangar mouth with fire behind them.
- [ ] **60 s timer.** Fly through the breaking Dominion:
  - The hull splits, giant debris tumbles (`BP_DOM_EscapeDebris`, physics-lite: scripted spline tumbles), explosions chain along the hull, and electric arcs act as hazards.
  - A few desperate fighters chase you.
  - Boost and hyperdrive (re-enabled at the halfway point) matter.
- [ ] **Reaching the safe radius:** `LS_DominionBreakup`. The reactor goes supernova-white, a shockwave ring expands, and the squad rides the shockwave out (a hero shot of all three ships in formation). Then `TravelTo(Victory)`.
- [ ] **Failure:** caught by the explosion means restarting the escape from its checkpoint.

### S-21 · Ship animations · `⬜`
Every ship is rigged (skeletal mesh, or rigid parts animated with timelines/procedural transforms). Keep it fast:

| Animation | Rift | Lancer | Bastion |
|---|---|---|---|
| Landing gear deploy/retract | ✔ | ✔ | ✔ |
| Canopy open/close | ✔ | ✔ | ✔ |
| Thruster gimbal (procedural, follows input) | ✔ | ✔ | ✔ |
| Control surfaces / flaps bank with turns (procedural) | ✔ | ✔ | ✔ |
| Boost: engine nozzles flare and wings spread/flex | ✔ | ✔ | ✔ |
| Signature wind-up | Portal ring emitter spins up | **Wings fold back** for the ram | **Missile pods open**; dome emitter rises |
| Ability cast | Hull shimmer pulse | Capacitor fins glow and snap | Gravity orb chamber opens |
| Damaged state | Sparks, flicker, smoke | ✔ | ✔ |
| Idle hover (landed, on pad) | Gentle bob + engine idle glow | ✔ | ✔ |

- [ ] Enemy ships: carrier bay doors open/close, gunship shield emitter rotation, turret aim/recoil, fighter wing flaps.
- [ ] Dominion: rotating rings, opening hangar doors, shield generator pulse, breakup (scripted pieces).

### S-22 · Space VFX · `⬜`
- [ ] Explosions in 4 tiers (small/medium/large/capital), each with a flash light, debris, smoke and a shockwave.
- [ ] Engine trails per ship color (ribbon), speed lines, warp-in tears (squad = white-blue, Armada = red).
- [ ] Every weapon's muzzle, tracer and impact. Shield ripples. Portals, dome, ram cone, missile trails, flak bursts, Gravity Well, Cloak shimmer and EMP ring.
- [ ] Hyperdrive tunnel, atmospheric entry plasma (with §7), landing dust.
- [ ] **Readability rule:** player = squad colors, enemy = red/orange, and nothing important is hidden by bloom.

### S-23 · Space SFX · `⬜`
- [ ] Engine loops per ship that pitch and layer with throttle and boost, a drift screech and release burst.
- [ ] Every weapon's fire/impact (3+ variations each), overheat vent, missile lock tones, incoming missile alarm.
- [ ] Portal open/hum/transit/close, ram wind-up and sonic boom, dome up/hit/shatter, Gravity Well, Cloak, EMP.
- [ ] Explosions per tier, shield hit vs hull hit, low-hull alarm, switch whoosh, hyperdrive wind-up/tunnel/drop-out.
- [ ] Atmosphere wind and buffeting, landing gear clunk, thrusters, canopy hiss.
- [ ] Dominion horn, turret charge, generator destruction, hull groans and breakup roar.
- [ ] Use attenuation and concurrency limits so big fights don't clip. Route through Dev 2's sound classes.

### S-24 · Space game feel & balance pass · `⬜`
- [ ] Camera shake tiers, hit-stop, controller rumble (per weapon, damage, ram and explosion).
- [ ] Play the full space path start → finish at least twice. Tune all the numbers in this section.
- [ ] Difficulty: a single "Pilot / Ace" toggle that scales enemy damage and accuracy (read from settings).

---

## 6. 🟩 DEV 2 — Worlds & Presentation

### G-01 · Game instance, progression, saves & level travel · `⬜`
- [ ] `BP_TD_GameInstance` implements `BPI_TD_GameFlow` and holds `S_TD_Progress`. It fires `OnAbilityUnlocked` and `OnPhaseChanged`.
- [ ] **`TravelTo()`:** fade to black (0.4 s) → `L_Transition`, a loading screen with a tip, a planet/ship art card and a spinning squad emblem → open the target → fade in. Optional per-destination stinger (e.g. atmospheric entry, see §7).
- [ ] **Checkpoints:** `SaveCheckpoint()` snapshots progress on every level entry, wave clear and objective complete. `BP_TD_SaveGame` saves to slot (continue from the main menu).
- [ ] **Defeat:** `WBP_Defeat` with **Retry from checkpoint** and **Quit to menu**. Track `Stats.Deaths`.
- [ ] **Settings save** (volume, sensitivity, invert, difficulty, subtitles, graphics preset).
- [ ] **Debug menu** (F10, dev builds only): jump to any destination or phase, unlock any ability, god mode, kill all enemies, skip wave. Both devs need this constantly.
- [ ] Run timer + `AddStat` for the victory screen.

### G-02 · First-person character · `⬜`
`BP_FPCharacter` must be fast and agile, matching the arcade pace of the space game.

| Move | Input (KB/M · Pad) | Behavior | Start values |
|---|---|---|---|
| Walk / run | WASD · Left stick | Default run (no slow walk) | 650 cm/s |
| Sprint | Shift · L3 | Faster, can't block while sprinting | 950 cm/s, FOV +8 |
| Jump | Space · A | Responsive, with air control 0.6 | Jump Z 650 |
| Double jump | Space in air · A | A small jet-boot burst | Z 500, 1 per airtime |
| **Dash** | Ctrl or Q · B | A short burst in the input direction with i-frames 0.15 s. Works in air | 1200 cm in 0.18 s, 2 charges, 1.2 s regen |
| Slide | Ctrl while sprinting · B while sprinting | Low slide, keeps momentum, can attack out of it | 0.7 s |
| Mantle | Auto when jumping at a ledge ≤ 180 cm | Quick vault-up | 0.35 s |
| Look | Mouse · Right stick | | Sensitivity in settings |
| Attack | LMB · RT | Blade combo (G-03) | |
| Block / Deflect | RMB · LT (hold) | G-03 | |
| Heavy / Lunge | Hold LMB · Hold RT | G-03 | |
| Interact | E · X (hold where needed) | `BPI_TD_Interactable` | |
| Objective ping | Tab · D-pad up | Highlights the objective waypoint | |

- [ ] **Health** 100. Regenerates after 4 s without damage at 15/s. Damage shows a red vignette, directional indicators and a camera kick.
- [ ] **Death:** a slow-motion fall and fade, then a checkpoint respawn in the same level (G-01).
- [ ] **Camera:** head bob (subtle, can be turned off), landing dip, dash FOV punch, slide camera lowering.
- [ ] **Footsteps** by surface (jungle moss, crystal, metal deck, ash).
- [ ] `IMC_Ground` with all of the above.

### G-03 · Energy blade · `⬜`
A short bright plasma blade (squad-colored, the color of the last ship you piloted 🎨).

| Move | Input | Damage | Behavior |
|---|---|---|---|
| **Light combo** (3 hits) | Tap attack | 25 / 25 / 45 | Slash right → slash left → overhead. Each swing has a wide sweep trace (180 cm range, 100° arc) and a small forward step. The 3rd hit knocks back. Combo window 0.45 s. |
| **Heavy lunge** | Hold attack 0.4 s, release | 70 | Dashes 600 cm forward, stabbing. Breaks Heavy guards. 2 s cooldown |
| **Air slam** | Attack while airborne + looking down | 50, 300 cm AoE | Plunges down with a shockwave on landing |
| **Dash strike** | Attack during dash | 35 | Slash through the enemy while dashing past |
| **Block** | Hold block | — | Blocks melee and projectiles in a 120° front arc. Moves at 50% speed. Stamina: 3 s continuous, then the guard breaks (1 s stun) |
| **Perfect deflect** | Press block ≤ 0.2 s before a projectile hits | Projectile's damage × 2 | The projectile is `Deflect()`'d **straight back at its shooter** (auto-aim), with a slow-mo flash (0.15 s at 30% time), a sharp *ting* and a big spark. It counts toward `Stats.ShotsDeflected` |
| **Normal deflect** | Block while a projectile hits | Projectile's damage × 1 | The projectile reflects toward the crosshair direction |
| **Melee parry** | Perfect block vs a melee attack | — | Staggers the attacker for 1.2 s; the next hit is a guaranteed critical |

- [ ] `BP_EnergyBlade` actor attached to the FP arms. All values in `DA_BladeCombo`.
- [ ] **Visuals:** an emissive core + bloom, a swing ribbon trail, a light that follows the blade, hit sparks, a glowing scorch decal on surfaces, and enemy hit flashes.
- [ ] **Audio:** an idle hum that changes pitch with swing velocity, swing whooshes (light/heavy), flesh/armor hit, clash, deflect *ting*, perfect deflect bell-ring, and ignite/retract.
- [ ] **Hit-stop:** 50 ms on hit, 120 ms on kill, and a slow-mo flash on perfect deflect.

### G-04 · First-person animations · `⬜`
FP arms mesh + `ABP_FPArms` + montages. Use a Fab or marketplace arms pack, or retarget mannequin arms. Credit it.
- [ ] Locomotion: idle (blade held low, gentle sway), run bob, sprint (blade held back), jump/land, double-jump kick, slide, mantle (hands grab ledge), dash (arm swept back).
- [ ] **Blade:** ignite (blade extends from the hilt), retract, light combo ×3, heavy wind-up + lunge, air slam, dash strike, block pose, block-hit flinch, perfect deflect flourish, guard break stagger, melee parry.
- [ ] **Interaction:** hold-interact (hand reaching out to a console), pick up/carry the Wreck Core (G-11), place charge.
- [ ] **Ship transitions** (see §7 and §8): climb out of the cockpit (a camera-driven montage: hands push the canopy frame, jump down), and climb into the cockpit.
- [ ] **Hit-reacts:** small flinch, big knockback, death fall.
- [ ] **Third-person body** for cinematics only (exit/enter ship shots): a simple pilot model with matching anims (or keep the camera first-person for all of them, which is acceptable).

### G-05 · Crew enemies (on foot) · `⬜`
Null Armada crew: armored humanoids or drones with glowing white visors. Original designs only.

| Enemy | Role | HP | Attacks | Behavior | Counter |
|---|---|---|---|---|---|
| **Grunt** | Melee rusher | 60 | Shock baton swing (15), telegraphed by a 0.5 s visor glow | Charges in groups of 3–5, flanks | Combos, parry |
| **Trooper** | Ranged | 50 | Blaster bolts (10 dmg, **slow bright red, deflectable**), 3-round bursts | Keeps distance, uses cover, strafes | Deflect bolts back, dash in |
| **Heavy** | Shielded bruiser | 200 | Shield bash (25), shotgun spread (5 × 6, deflectable) | Advances behind a frontal shield that blocks slashes | Heavy lunge breaks the guard, deflected shotgun, flank with dash |
| **Sentry Drone** | Flying harasser | 30 | Fast small bolts (5) | Hovers out of melee range, swoops | Deflect, air slam, jump + slash |
| **Warden** | Mini-boss (Cinder outpost + Dominion core door) | 600 | Combo of the above: charge slam (40, AoE), bolt fan (deflectable), summons 2 grunts | Phase 2 at 50%: faster, red glow | Parry the charge, deflect the fan |

- [ ] `BP_Crew_Base` with AI (Behavior Tree or State Tree): Idle/Patrol → Alert (bark + visor turns red, alerts nearby crew) → Engage → Stagger → Dead.
- [ ] **Animations** per enemy (Fab/Mixamo-style packs retargeted, credited):
  - Idle, walk, run, strafe.
  - Attack anims with clear telegraphs, hit-react light/heavy, stagger, death.
  - **Ragdoll on death**, blown back if killed by a heavy or deflect.
- [ ] Projectiles are children of `BP_TD_ProjectileBase`, so blade deflection just works.
- [ ] Attack tokens: at most 2 grunts attack at once and the rest circle, so fights stay readable and fair.
- [ ] Death: visor flickers out, body dissolve shader after 4 s (keeps performance in check).

### G-06 · Interaction & objective framework · `⬜`
- [ ] `BP_ObjectiveBase`: `ObjectiveId`, display name, ordered **steps** (each with text + waypoint actor), `RewardAbility`, `RewardShip`, and an `OnCompleted` event.
- [ ] Waypoint system: a world-space marker + distance, clamped to the screen edge when off-screen, plus the objective ping highlight.
- [ ] Interactables (implement `BPI_TD_Interactable`): tap or hold with a radial progress fill, interruptible by damage (hold only).
- [ ] **On objective complete:** `CompleteObjective(...)` → `WBP_AbilityUnlocked` (G-12) → the next objective or the "Return to your ship" step.
- [ ] Encounter triggers: a volume spawns crew groups (with spawn VFX: drop pods / teleport-in), and "room clear" opens doors.

### G-07 · Planet: Verdance · `⬜` 🎨 THEME
**Look:** a bioluminescent jungle under a purple-teal sky. Giant glowing mushrooms and fern trees, waterfalls into
mist, floating pollen, and a ringed moon dominating the sky. It's dim enough that glows pop.

- [ ] `L_Planet_Verdance`: a compact landscape (≈ 1 km²) with clear landmarks (a giant fallen tree, an ancient relay spire, the crash site smoke column), and one **landing pad** (place Dev 1's `BP_LandingPad`) next to an Armada scout camp.
- [ ] Flight zone for the descent (place Dev 1's `BP_AtmosphereFlightZone`): canyons and a waterfall to fly past on the way down.
- [ ] **Obj 1: "Recover the Wreck Core"** (→ **Gravity Well, Bastion**)
  1. Follow the crash beacon through the jungle, with a Trooper patrol ambush on the way.
  2. Reach a crashed squad-style ship (an old wreck) and fight Grunts + Troopers.
  3. Hold-interact (4 s) to extract the glowing core. Two waves of crew drop in (drop-pod VFX).
  4. **Carry the core** back toward the pad. You can't sprint or block while carrying, but you can drop it to fight and pick it up again. A shortcut gate opens.
  5. Slot it into Bastion on the pad → unlock.
- [ ] **Obj 2: "Wake the Ancient Relay"** (→ **Cloak, Rift**)
  1. Climb the relay spire: light platforming using double jump, dash and mantle, with Sentry Drones harassing.
  2. Activate 3 resonance nodes, each defended by crew. The nodes hum louder and the relay lights progressively.
  3. At the top, the relay pulses with a sky-wide shimmer → unlock. You get a fast zipline/glide back down (fun bonus) or a shortcut path.
- [ ] Ambient life: glowing insects, critters that scatter (simple), wind through foliage, distant creature calls.
- [ ] Performance: use foliage instancing, plus Nanite where available and Lumen.

### G-08 · Planet: Cinder · `⬜` 🎨 THEME
**Look:** black volcanic rock, huge cyan and amber crystal spires, glowing magma rivers, ash falling, and an
orange-red sky with a massive ringed planet on the horizon. It's dangerous and dramatic.

- [ ] `L_Planet_Cinder`: a compact map with a landing pad on a crystal plateau, an **Armada outpost** in a caldera, and a **signal jammer** tower on a magma island.
- [ ] Hazards: magma (instant heavy damage + respawn at the last safe spot), periodic geyser vents (telegraphed), crystal shards that shatter when slashed (cover that breaks).
- [ ] **Obj 3: "Clear the Armada Outpost"** (→ **EMP Pulse, Lancer**)
  1. Breach the outpost gate: slash 2 power conduits to open it.
  2. Three areas of escalating crew (Grunts/Troopers → Heavies → Drones).
  3. **Warden mini-boss** in the command yard.
  4. Hold-interact the captured EMP prototype → unlock.
- [ ] **Obj 4: "Destroy the Signal Jammer"** (→ **reveals the Dominion**, sets `DominionRevealed`)
  1. Cross the magma island via collapsing crystal bridges (timed platforming).
  2. Plant 3 charges on the jammer's legs (hold-interact) while crew attacks.
  3. Run! The tower collapses behind you (scripted destruction sequence + camera shake).
  4. A radio bark: "Wren: Jammer's down… oh. Oh, that's where they were hiding it." Then return to the ship.
- [ ] Ambient: an ash particle system, crystal hum audio, magma bubbling, distant eruptions.

### G-09 · Landing-pad scenes (character side of landing) · `⬜`
See §7. Dev 2 builds the on-foot half:
- [ ] `BP_LandedShip`: the landed squad ships as interactable props (Dev 1's meshes, read-only), canopy open, idle engine glow. Interacting with the one you flew ("Board ship") is only allowed when the planet's objectives are complete (otherwise it's greyed out with "Objectives remaining").
- [ ] `LS_ExitShip` (FP camera): the canopy opens → you push up out of the cockpit (FP arms montage) → hop down onto the pad → control.
- [ ] `LS_EnterShip`: walk-up → climb-in montage → canopy closes → hand back to Dev 1's take-off.
- [ ] Wingmen pilots stay with their landed ships (simple idle pilot models) and bark encouragement over radio during objectives.
- [ ] Reward install moment: the carried Wreck Core slots into Bastion with a glow-up, and the EMP prototype is loaded into Lancer (short animated beats on the pad).

### G-10 · Dominion interior · `⬜`
**Look:** the Armada's style inside. Black glossy metal, red light strips, giant white "eye" lenses watching from
the walls, ribbed corridors and vast vertical shafts. Brutal and imposing next to the bright galaxy.

- [ ] Modular kit: corridor straight/corner/T, door (animated split open), shaft, catwalk, railings, consoles, pipes, light strips, eye lenses (they track the player, which is a cheap, creepy touch).
- [ ] `L_Dominion_Interior` layout (linear, 6–10 minutes):
  1. **Hangar** (arrival, see §8). The squad's 3 ships land. A crew squad charges as the hangar doors seal.
  2. **Corridor A:** Troopers in a long hallway, teaching **deflect** (a tip prompt).
  3. **Barracks hall:** a big mixed fight, with a door that opens on clear.
  4. **Vertical shaft:** descend via catwalks and drop-downs, with Sentry Drones and moving hazard arms (timed sweeping energy beams, `BP_DOM_Hazard_Sweeper`).
  5. **Security checkpoint:** Heavies + Troopers behind barriers. Slash 2 shield emitters to drop the barrier.
  6. **Core antechamber:** **Warden** guarding the core door.
  7. **Core chamber** (G-11).
- [ ] Ambience: deep machine hum, distant announcements in an alien language (processed AI VO), alarms during the escape.

### G-11 · Core encounter & escape run · `⬜`
- [ ] **`BP_DOM_Core`:** a towering spherical reactor of white light inside rotating red rings.
  - **Phase 1:** 4 exposed **conduits** around the base (slash to destroy, 150 HP each). The core fires **deflectable energy orbs** (perfect deflect damages the core directly), and grunts spawn.
  - **Phase 2 (all conduits down):** the rings spin faster, the core opens and exposes its heart. A sweeping laser forces you to jump or dash. Troopers and drones join.
    - Deflected orbs and blade hits on the heart (when the rings align, every 6 s) damage it.
  - **Phase 3 (25%):** frantic, with everything faster. Kill shot = a final perfect deflect or blade strike → `LS_CoreDestroyed`.
- [ ] **`LS_CoreDestroyed`:** slow motion, the core cracks white, a shockwave knocks the player back, alarms sound, red lighting turns to strobe, then control returns.
- [ ] **Escape run (90 s timer):** the path back through the level is **changed**.
  - Collapsing sections, fires, falling debris, the sweeper hazards gone haywire, and a few panicked crew.
  - Shortcut doors open, making it a fast sprint/dash route.
  - The timer is on the HUD.
  - Reaching the hangar plays `LS_EnterShip`, then `TravelTo(Galaxy, "Dominion_Hangar_Exit")`, which starts Dev 1's escape flight (S-20).
- [ ] **Timer runs out:** the screen goes white and you restart the escape run from its start.

### G-12 · UI — all widgets · `⬜`
Style: bold, clean sci-fi with big readable numbers, squad colors (magenta/cyan/amber) on dark translucent panels,
and a distinctive display font (credited). Animate everything a little (slide/scale/pulse).

- [ ] **`WBP_SpaceHUD`** (built on `BPI_TD_HUDData`, stub first):
  - **Squad panel:** 3 ship cards (icon, hull, shield, power + ability cooldown rings, padlock if locked, respawn countdown). The active card is enlarged. Animates on switch.
  - **Center:** crosshair, aim reticle (mouse virtual stick), target bracket + health + lead indicator, missile lock brackets, hit/kill markers.
  - **Edges:** off-screen enemy arrows, incoming missile arrows (red, pulsing), objective arrow + distance.
  - **Bars:** boost, heat/ammo, hyperdrive charge, speed.
  - **Banners:** "WAVE X", enemies left, "ABILITY READY", boundary warning, Switch Strike active.
  - **Atmosphere variant:** altitude, landing prompt.
- [ ] **`WBP_GroundHUD`:** health, block stamina, perfect-deflect flash, dash charges, objective tracker + waypoint, interact prompt with hold ring, damage direction, escape timer.
- [ ] **`WBP_MainMenu`:** title logo, Continue / New Game / Controls / Settings / Credits / Quit. The background is a slow camera fly-by in a galaxy scene (G-15 actors), with music.
- [ ] **`WBP_Pause`**, **`WBP_Settings`** (audio sliders, sensitivity, invert Y, difficulty Pilot/Ace, subtitles on/off + size, head bob, screen shake intensity, graphics preset, key rebinding if time), **`WBP_Controls`** (KB/M + gamepad diagrams for space and ground).
- [ ] **`WBP_GalaxyMap`** (M): a stylized map of the zones, planets, the Dominion and the current objective.
- [ ] **`WBP_AbilityUnlocked`:** the big reward moment. The screen dims, the ability icon assembles, the name and ship portrait appear, a "how to use" line and the input glyph show, a sting plays, and you press to continue.
- [ ] **`WBP_Tip`:** contextual toasts from `DT_Tips` (with the input glyph for KB/M vs pad, auto-detected).
- [ ] **`WBP_Subtitles` / radio:** pilot portrait + name + subtitle line for each bark.
- [ ] **`WBP_Defeat`, `WBP_Victory`** (run time, kills, switches, portals used, shots deflected, missiles returned, deaths, and a rank S/A/B/C), and **`WBP_Credits`** (team, tools, every third-party asset from `CREDITS.md`).
- [ ] **`WBP_Loading`** (`L_Transition`): an art card + tip + spinner.
- [ ] Input-device detection to swap all glyphs between KB/M and gamepad.

### G-13 · Narrative, radio barks & VO · `⬜`
- [ ] `DT_Barks`: Id, speaker (Wren/Kato/Moss/Armada), line text, audio asset, priority, cooldown.
- [ ] `RequestBark()` plays the VO with radio filter processing + the subtitle widget. A priority queue, so nothing overlaps or spams.
- [ ] **Bark categories** (write ~80–120 lines total, AI-voiced with distinct voices per pilot, credited):
  - Taking over each ship (×3 pilots, several variants).
  - Using each signature/ability, kills, multi-kills, ship downed / respawned, low hull, Switch Strike.
  - Wave start/clear, marker reveals, landing/take-off, objective start/complete for all 4 objectives.
  - Dominion reveal / generators / docking, core fight, escape, victory banter.
  - Armada announcements (alien language plus subtitles that taunt).
- [ ] **Intro briefing** (`LS_Intro`, ~30–45 s, skippable): the squad in formation over Verdance's ring, banter establishing each pilot's personality, then the Armada warps in → gameplay.
- [ ] Ending: victory banter over the breakup hero shot, then credits.

### G-14 · Music & audio mix · `⬜`
- [ ] **`BP_MusicDirector`:** a layered stem soundtrack.
  - **Space:** calm → combat layers swell with `Intensity01` from `OnWaveStarted`, dropping to calm on `OnWaveCleared`.
  - **Planets:** an exploration track per planet (Verdance airy/organic, Cinder percussive/ominous), plus combat layers during encounters.
  - **Dominion:** a dark industrial track, the core boss track (3 intensity phases) and an escape track (fast, heroic).
  - **Menu theme, victory theme,** and a defeat sting.
  - Stingers: ability unlocked, Dominion reveal, generator down, core destroyed.
- [ ] Sound classes + submixes: Master / Music / SFX / VO / UI, hooked to the settings. Sidechain-duck music under VO.
- [ ] **Ground SFX:** footsteps per surface, jump/land, dash whoosh, slide, mantle grunt, all blade sounds (G-03), crew barks/attacks/deaths, drone buzz, doors, consoles, hazards, magma, crystals, alarms, collapse rumble, carrying-the-core hum.
- [ ] **UI SFX:** hover, click, back, tip pop, unlock sting.
- [ ] Sources: WeLoveIndies jam pack, AI audio tools, licensed packs. **Log every one in `CREDITS.md`.**

### G-15 · Space environment art (delivered as actors) · `⬜`
Bright, saturated sci-fi, original in look (no copied assets or logos).
- [ ] **`BP_GalaxySkybox`:** a layered nebula sky (pink/teal/violet, painted or AI-generated HDRI + cubemap), star field with twinkle, a distant galaxy band and a sun (`BP_Sun` with a lens flare and directional light).
- [ ] **`BP_PlanetVisual`:** a sphere with a surface material (parameters for Verdance green-teal and Cinder black-orange), atmosphere rim glow, **rings** (a translucent banded mesh), slow rotation, and moons.
- [ ] **`BP_NebulaVolume`:** colored volumetric fog clouds you can fly through (reduced visibility, a good spot to ambush).
- [ ] **`BP_SpaceDust`:** a camera-relative particle field that sells speed.
- [ ] **`BP_AsteroidField`:** an instanced asteroid scatter with size/density parameters and a few big hero rocks. Collision on medium and large rocks only.
- [ ] **Station ruins** set dressing for the combat zone (a modular, broken ring station).
- [ ] Handed to Dev 1 as ready-to-drop actors. **Dev 2 never edits `L_Galaxy`.**

### G-16 · Intro cinematic & menu scene · `⬜`
- [ ] `LS_Intro` (see G-13): camera work over the planet's rings, the squad in formation, and the Armada's red warp tears.
- [ ] The main-menu background level/scene uses the G-15 actors (separate from `L_Galaxy`, so no conflict).
- [ ] **Trailer capture:** Movie Render Queue shots for the gameplay video (G-18).

### G-17 · Packaging, performance & build pipeline · `⬜`
- [ ] Packaging profile: Windows, Shipping, with all maps in "Maps to Package". Startup map `L_MainMenu`, GameInstance set, debug menu stripped from Shipping.
- [ ] **Package often** (any time a major feature lands) into `Builds/` (git-ignored) and smoke-test the packaged exe start → finish.
- [ ] Performance: a 60 fps target on mid hardware.
  - **Scalability presets** wired to settings.
  - **Niagara:** particle budgets and significance.
  - **Lumen/Nanite** settings, foliage culling distances, and the ragdoll/body cleanup timer.
- [ ] **Final zip** named **`TeamName_GameName`**, < 1 GB (otherwise use butler).

### G-18 · Submission package · `⬜`
- [ ] **itch.io page:**
  - Title art, 5+ screenshots and a GIF, and a description with the hook in the first line.
  - Controls, team members, tools used (list **Rodin/Tripo/Meshy/fal/Claude/etc.** for partner prizes), every third-party asset, pre-jam declarations.
- [ ] **30–60 s gameplay video:** gameplay in the first 5 seconds. Dogfight → switch → portal missile return → ram → landing → blade deflect → Dominion core → breakup.
- [ ] **Dev log** (Glass Box prize) compiled from `DEVLOG.md`, with screenshots and the AI workflow story.
- [ ] Submit with a **comfortable buffer before 8:59 PM EST**. Both humans verify the submission form fields.

---

## 7. Landing — full sequence (both devs)

| # | Step | Owner | Details |
|---|---|---|---|
| 1 | Marker appears | 🟦 | After waves 2 and 4, the wave director reveals the planet marker. A bark and tip follow ("Hyperdrive to Verdance") |
| 2 | Approach | 🟦 | Hyperdrive auto-drops out in orbit. The squad flies toward `BP_PlanetAtmosphere` |
| 3 | **Atmospheric entry** | 🟦 | Entering the atmosphere trigger: the ship nose glows with heat plasma (Niagara + hull heat shader), heavy shake, roar SFX, the screen whitens over 2.5 s. Calls `TravelTo(Verdance)` |
| 4 | Transition | 🟩 | The fade/plasma white hides the load. `L_Transition` is skipped or very short (pre-stream if possible) |
| 5 | Breakthrough | 🟦 class / 🟩 map | The ship spawns high in the planet sky (a `PlayerStart_Descent` placed by Dev 2) **in atmosphere flight mode**, bursting out of a cloud layer with leftover plasma VFX. Wingmen spawn in formation |
| 6 | Descent flight | 🟦 | The player flies down through Dev 2's scenery (canyons, waterfalls) toward the pad's beacon column. Altitude is on the HUD |
| 7 | Land | 🟦 | "Hold F to land" near the pad → auto-landing (gear, dust, touchdown, canopy). Wingmen land beside it. Fires `OnShipLanded` |
| 8 | Exit ship | 🟩 | `LS_ExitShip` (FP), then the FP character spawns on the pad. The objective tracker appears |
| 9 | Objectives | 🟩 | Objectives, unlocks and reward screens. Wingmen radio barks |
| 10 | Board ship | 🟩 | Interact with the ship (allowed once all of the planet's objectives are done) → `LS_EnterShip` |
| 11 | Take-off | 🟦 | Canopy closes, lift-off, control returns. Fly up past the zone ceiling |
| 12 | Exit atmosphere | 🟦 / 🟩 | Reverse plasma burst → `TravelTo(Galaxy, "Verdance_Orbit")` → the squad spawns in orbit facing the combat zone. The next waves begin |

---

## 8. Boarding — full sequence (both devs)

| # | Step | Owner | Details |
|---|---|---|---|
| 1 | Reveal | 🟦 | Cinder Obj 4 sets `DominionRevealed` → after wave 5, `LS_DominionReveal` plays |
| 2 | Assault | 🟦 | Destroy 4 shield generators and turrets while fighters keep launching |
| 3 | Shields down | 🟦 | Shield collapse VFX → `SetDominionState(DefensesDown)` → the hangar doors open with a beacon light |
| 4 | **Docking approach** | 🟦 | Fly into `BP_DockingBay`'s approach cone. Turret fire intensifies. Inside the cone, a **tractor beam** (blue light tunnel) catches the squad and pulls them in. Control is reduced and the camera goes cinematic |
| 5 | Transition | 🟦 → 🟩 | Entering the hangar mouth fades → `TravelTo(DominionInterior)` |
| 6 | Hangar landing | 🟩 | `L_Dominion_Interior` opens on a short shot: the 3 ships (`BP_LandedShip` + a simple landing animation or a matinee-style sequence) touch down in the hangar. Doors seal behind them with a red alarm |
| 7 | Step out | 🟩 | `LS_ExitShip` → FP control. A bark: "Moss: We'll hold the hangar. Go end this, lead." |
| 8 | Fight to the core | 🟩 | G-10 layout, then the G-11 core boss |
| 9 | Escape run | 🟩 | 90 s run back to the hangar |
| 10 | Launch | 🟩 → 🟦 | `LS_EnterShip` → `TravelTo(Galaxy, "Dominion_Hangar_Exit")` → `LS_ShipLaunch` → the S-20 escape flight → `LS_DominionBreakup` → `TravelTo(Victory)` |

---

## 9. Integration points

| # | Integration | 🟦 Dev 1 delivers | 🟩 Dev 2 delivers |
|---|---|---|---|
| I-1 | Level travel | Calls `TravelTo` from the atmosphere, docking bay and escape | `BP_TD_GameInstance` travel, fades and loading |
| I-2 | Space HUD | `BPI_TD_HUDData` on `BP_SquadManager` | `WBP_SpaceHUD` |
| I-3 | Environment art | Places the actors in `L_Galaxy` | Skybox, planets, nebulae, dust, asteroids, station ruins |
| I-4 | Waves → music and barks | `OnWaveStarted/Cleared`, `OnShipSwitched`, `OnPowerUsed` | Music director + bark system bindings |
| I-5 | Progress resume | Wave director reads `GetProgress()` and `OnPhaseChanged` | Progress persisted across levels + checkpoints |
| I-6 | Ability unlocks | Ability slot reads `UnlockedAbilities` + `OnAbilityUnlocked` | `CompleteObjective` + reward screen |
| I-7 | Landing | `BP_PlanetAtmosphere`, atmosphere flight, `BP_LandingPad`, `BP_AtmosphereFlightZone`, auto-land/take-off, `OnShipLanded` | Places the pads, flight zones and descent start in the planet maps. `BP_LandedShip`, exit/enter sequences |
| I-8 | Boarding | Dominion exterior, docking tractor beam, escape flight, breakup | Interior, hangar landing shot, core, escape run |
| I-9 | Ship meshes for landed ships | Ship meshes/materials (stable asset paths) | `BP_LandedShip` references them read-only |
| I-10 | Tips and barks | Calls `ShowTip` / `RequestBark` at the right moments | `DT_Tips`, `DT_Barks`, widgets, VO |
| I-11 | Sound mix | Space SFX routed to the shared sound classes | Sound classes, submixes and settings |

**Integration ritual** (whenever either dev lands something big): pull → play the full game from the main menu
→ package → log bugs in §11 → each bug goes to the owner of the file it lives in.

---

## 10. Dependency order (no dates)

You both run full speed in parallel. This is only **what unblocks what**, so you can pick the next item sensibly.

```
C-01 Project ─► C-02 Contracts ─┬─► 🟦 S-01 Flight ─► S-04 Ships ─► S-05 Weapons ─► S-07 Switching ─► S-08 Wingmen
                                │        └─► S-19 Atmosphere flight/landing (needs S-01)
                                │   S-13 Enemies ─► S-14 Waves ─► S-18 Dominion ─► S-20 Escape
                                │   S-09/S-10/S-11 Signatures (need S-04) ─► S-12 Abilities (needs G-01 unlock data)
                                │
                                └─► 🟩 G-01 GameFlow ─► G-06 Objectives ─► G-07 Verdance / G-08 Cinder
                                         G-02 FP Character ─► G-03 Blade ─► G-04 FP anims ─► G-05 Crew
                                         G-10 Dominion interior ─► G-11 Core + escape run
                                         G-12 UI (stubs first) · G-15 Environment (early: Dev 1 needs it for S-17)
                                         G-13 Barks / G-14 Music (anytime once events exist) · G-17/G-18 continuous
```

**Biggest risks (start early):** S-01 flight feel, S-09 portals, S-19 + G-09 landing hand-off, G-03 deflect,
G-11 core boss, and G-17 packaging (package early and often so it doesn't surprise us at the end).

---

## 11. Bugs & requests (append-only)

Add new lines at the bottom only, to avoid merge conflicts. Format: `[for Dev X] description — reporter, timestamp`

### Requests between devs
- _e.g. `[for Dev 1] Expose ship mesh soft refs on DA_ShipStats for BP_LandedShip — Dev 2, Nov 13 14:20`_

### Bugs
- _e.g. `[Dev 2] WBP_SpaceHUD: shield bar doesn't update after respawn — found by Dev 1, Nov 13 18:05`_
