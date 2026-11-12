# Triad Drift — Feature Breakdown & Work Split

> **Working title:** Triad Drift · **Jam:** 2026 Epic AI MegaJam · **Engine:** Unreal Engine (see AGENTS.md)
> **Deadline:** Nov 15, 2026 — **8:59 PM EST / 5:59 PM PST**. Late = disqualified.
> **Theme:** _TBD — fill in and adjust features marked 🎨 THEME to fit._

This file splits the whole game into features and gives each one to **Dev 1** or **Dev 2** so we can work at the
same time without editing the same files. Read **§1 to §3** first (the split, the folder ownership and the
shared contracts). Then go to your own section.

| | **Dev 1 — "Space"** | **Dev 2 — "Worlds & Presentation"** |
|---|---|---|
| **Who** | _fill in_ | _fill in_ |
| **Owns** | Everything that happens **in a ship**: flight, weapons, the three ships, switching, wingmen, signature powers, unlockable ship abilities, enemy fleets, waves, hyperdrive, portals, the galaxy map's gameplay, the mothership's exterior and the escape flight | Everything that happens **on foot or around the game**: the first-person character, the energy blade, crew enemies, planets and ground objectives, the mothership interior and core, game flow and progression, all UI, music, the space environment art, packaging and submission |
| **Feature IDs** | `S-xx` | `G-xx` |
| **Content folder** | `Content/TriadDrift/Space/` | `Content/TriadDrift/Ground/`, `UI/`, `Environment/`, `Audio/Music/`, `Flow/` |

**Shared work** is `C-xx`. It's done once in the first hour, then frozen (see §3).

**Priority key**
- **P0**: must ship. Without it there's no game.
- **P1**: should ship. This is what makes it good.
- **P2**: nice to have. It's the first thing we cut.
- ✂️ marks items on the official cut list.

**Status key** (edit inline as you go): `⬜ todo` · `🟨 in progress` · `✅ done` · `✂️ cut`

---

## 0. The game at a glance (what we're building)

A fast, bright, arcade space-combat game. You lead a wing of three ships (**Rift**, **Lancer** and **Bastion**) and
can switch between them at any moment. The two ships you aren't flying fight on their own. You land on planets
to earn new abilities, then board the enemy mothership on foot with an energy blade to end the war.

### Player journey (the full flow, start to finish)

```
Main Menu
  └─► Galaxy: Waves 1–2 (learn flight, shooting, switching)                    [Dev 1]
        └─► Planet A marker appears → hyperdrive → descend → land             [Dev 1 → Dev 2]
              └─► Planet A: on-foot objective → ability unlock → take off     [Dev 2]
        └─► Galaxy: Waves 3–4 (bigger, gunships, portals matter)               [Dev 1]
        └─► (P2 ✂️) Planet B: second objective + unlock                        [Dev 2]
        └─► Galaxy: Wave 5 (carriers) → MOTHERSHIP EXPOSED                     [Dev 1]
              └─► Break mothership defenses → dock                             [Dev 1]
                    └─► Mothership interior: corridors → core (energy blade)   [Dev 2]
                          └─► Core destroyed → run to ship (timer)             [Dev 2]
                                └─► (P2 ✂️) Escape flight as it breaks apart   [Dev 1]
                                      └─► Victory screen + credits             [Dev 2]
```

**Fallback if the escape flight is cut:** core destroyed → run to ship → cinematic fade → Victory screen.

---

## 1. Why the split is drawn this way

- **Different maps.** Dev 1 lives in `L_Galaxy`. Dev 2 lives in `L_Planet_*`, `L_Mothership_Interior` and `L_MainMenu`.
  Two people almost never need to open the same map.
- **Different pawns.** Dev 1 owns the ship pawns. Dev 2 owns the first-person character. The two meet only through
  the contracts in §3.
- **Different input contexts.** Dev 1 owns `IMC_Space`. Dev 2 owns `IMC_Ground` and `IMC_UI`.
- **One owner per screen.** Dev 2 owns **all widgets**. Dev 1 only *publishes data* for the HUD through an interface.
  This keeps Dev 1 focused on feel, the riskiest work, and keeps UI files on one side.
- **Environment as actors, not map edits.** Dev 2 builds the skybox, nebulae and planet visuals as **self-contained
  Blueprint actors**. Dev 1 drops them into `L_Galaxy`, so Dev 2 never edits Dev 1's map.

---

## 2. Folder & file ownership (the anti-conflict map)

```
Content/TriadDrift/
├── Core/                  🔒 SHARED: frozen after C-02. Claim in CLAIMS.md + tell the other human before ANY edit
│   ├── Interfaces/        BPI_TD_GameFlow, BPI_TD_Team, BPI_TD_HUDData, BPI_TD_Deflectable
│   ├── Data/              E_TD_ShipId, E_TD_AbilityId, E_TD_Team, E_TD_Destination, S_TD_Progress, S_TD_ShipHUD
│   └── Projectiles/       BP_TD_ProjectileBase (owner: Dev 1)
├── Space/                 🟦 DEV 1 ONLY
│   ├── Ships/             BP_ShipBase, BP_Ship_Rift, BP_Ship_Lancer, BP_Ship_Bastion, DA_ShipStats_*
│   ├── Squad/             BP_SquadManager, BP_SpacePlayerController
│   ├── Weapons/           BP_Weapon_*, BP_Proj_*
│   ├── Powers/            BP_Power_Portal, BP_Power_Ram, BP_Power_MissileSwarm, BP_Power_ShieldDome
│   ├── Abilities/         BP_Ability_Cloak, BP_Ability_EMP, BP_Ability_GravityWell
│   ├── AI/                BT_/BB_ wingman + enemy behavior, BP_AIController_*
│   ├── Enemies/           BP_Enemy_Fighter, BP_Enemy_Gunship, BP_Enemy_Carrier
│   ├── Waves/             BP_WaveDirector, DT_Waves
│   ├── Travel/            BP_Hyperdrive, BP_PortalGate, BP_LandingZone, BP_DockingPort
│   ├── Mothership/        BP_Mothership_Exterior, BP_MS_Turret, BP_MS_ShieldGenerator
│   ├── VFX/  Audio/       space weapon trails, explosions, engine FX, ship SFX
│   ├── Input/             IMC_Space, IA_* (space)
│   └── Maps/              L_Galaxy (World Partition + One File Per Actor), L_Test_Space
├── Ground/                🟩 DEV 2 ONLY
│   ├── Character/         BP_FPCharacter, BP_GroundPlayerController, ABP_* arms
│   ├── Blade/             BP_EnergyBlade
│   ├── Enemies/           BP_Crew_Grunt, BP_Crew_Trooper, BP_Crew_Heavy, BP_Proj_CrewBlaster (child of Core projectile)
│   ├── Objectives/        BP_ObjectiveBase, BP_Obj_RecoverCore, BP_Obj_ClearOutpost, BP_Obj_DestroyCore
│   ├── Landing/           BP_LandedShip (non-flying prop + board/exit), LS_Descent, LS_Takeoff
│   ├── Planets/           planet terrain, materials, props
│   ├── Mothership/        interior kit, BP_MS_Core
│   ├── Input/             IMC_Ground, IA_* (ground)
│   └── Maps/              L_Planet_A, L_Planet_B, L_Mothership_Interior, L_Test_Ground
├── Flow/                  🟩 DEV 2: BP_TD_GameInstance, BP_GM_Menu, BP_GM_Ground, save/progress
├── UI/                    🟩 DEV 2: every WBP_*  (HUDs, menus, prompts, reward screens)
├── Environment/           🟩 DEV 2: BP_GalaxySkybox, BP_PlanetVisual, BP_NebulaVolume, BP_SpaceDust
├── Audio/Music/           🟩 DEV 2: music stems + BP_MusicDirector
├── Maps/                  🟩 DEV 2: L_MainMenu, L_Transition (loading)
└── Developers/<name>/     personal scratch. Never referenced by real content
```

**Rules**
- **Never edit the other dev's folders.** If you need a change there, ask your human to ask theirs, or open a
  request in the "Requests" section at the bottom of this file.
- **`Core/` is frozen** after C-02. Any change must be claimed in `CLAIMS.md`, both humans must agree, and it must
  be pushed on its own in a small commit. **Add new things rather than changing existing pins or fields.**
- **Space game mode:** `BP_GM_Space` lives in `Space/Squad/` (Dev 1). Ground and menu game modes live in `Flow/` (Dev 2).
- **Project settings** (`Config/*.ini`) are text, but still claim them before edits (input, collision channels,
  maps & modes). Use **only** these collision channels, which C-01 creates: `Ship`, `Projectile`, `Character`, `Interactable`.

---

## 3. Shared contracts (how the two halves talk)

These are created **once** in the first hour (C-02). After that, each side builds against them and can **stub**
the other side until it's ready. Nobody waits on anybody.

### 3.1 Enums & structs (`Core/Data/`)

| Asset | Values / fields |
|---|---|
| `E_TD_ShipId` | `Rift`, `Lancer`, `Bastion` |
| `E_TD_Team` | `Player`, `Enemy`, `Neutral` |
| `E_TD_AbilityId` | `None`, `Cloak`, `EMPPulse`, `GravityWell` (add new values at the **end** only) |
| `E_TD_Destination` | `MainMenu`, `Galaxy`, `PlanetA`, `PlanetB`, `MothershipInterior`, `Victory` |
| `S_TD_Progress` | `WavesCleared:int`, `CompletedObjectives:Set<Name>`, `UnlockedAbilities:Map<E_TD_AbilityId,E_TD_ShipId>`, `MothershipExposed:bool`, `MothershipDefensesDown:bool`, `ReturnPoint:Name`, `RunStartTime:float` |
| `S_TD_ShipHUD` | `Ship:E_TD_ShipId`, `Health01:float`, `Shield01:float`, `PowerCooldown01:float`, `AbilityCooldown01:float`, `Ability:E_TD_AbilityId`, `IsPlayerControlled:bool`, `IsAlive:bool` |

### 3.2 Interfaces (`Core/Interfaces/`)

| Interface | Implemented by | Called by | Functions |
|---|---|---|---|
| `BPI_TD_GameFlow` | **Dev 2**: `BP_TD_GameInstance` | Both | `TravelTo(E_TD_Destination, ReturnPoint:Name)` · `GetProgress() → S_TD_Progress` · `ReportWaveCleared(Index:int)` · `CompleteObjective(Id:Name, Reward:E_TD_AbilityId, ForShip:E_TD_ShipId)` · `SetMothershipState(Exposed:bool, DefensesDown:bool)` · `ReportPlayerDefeated()` |
| `BPI_TD_Team` | Every damageable actor | Weapons, AI | `GetTeam() → E_TD_Team` |
| `BPI_TD_HUDData` | **Dev 1**: `BP_SquadManager` | **Dev 2**: `WBP_SpaceHUD` | `GetSquadHUD() → Array<S_TD_ShipHUD>` · `GetCurrentTarget() → Actor` · `GetWaveInfo() → (Wave:int, EnemiesLeft:int)` · `GetHyperdriveCharge01() → float` · `GetObjectiveMarkers() → Array<Actor>` |
| `BPI_TD_Deflectable` | `BP_TD_ProjectileBase` | Blade (Dev 2), Rift portal (Dev 1), Bastion dome (Dev 1) | `Deflect(NewInstigator:Actor, NewDirection:Vector, NewTeam:E_TD_Team)` |

### 3.3 Damage
- Use **Unreal's built-in `Apply Damage` / `Event AnyDamage`** everywhere. Don't invent a custom damage system.
- Before applying damage, check `GetTeam()` so nothing hits its own team. Friendly fire is **off**.

### 3.4 Events both sides listen to
Use Event Dispatchers on these actors (bind with "Get Actor of Class" or have the GameInstance forward them):

| Dispatcher | Lives on | Fired when | Listened to by |
|---|---|---|---|
| `OnWaveStarted(Index, Intensity01)` | `BP_WaveDirector` (Dev 1) | A wave begins | `BP_MusicDirector` (Dev 2), HUD |
| `OnWaveCleared(Index)` | `BP_WaveDirector` | A wave ends | Music, HUD, GameFlow |
| `OnShipSwitched(NewShip)` | `BP_SquadManager` (Dev 1) | The player swaps ships | HUD, music sting |
| `OnSquadWiped()` | `BP_SquadManager` | All 3 ships are destroyed | GameFlow → defeat screen |
| `OnAbilityUnlocked(Ability, Ship)` | `BP_TD_GameInstance` (Dev 2) | An objective is completed | Dev 1's ships (refresh loadout) |

### 3.5 Stubs (so nobody is blocked)
- **Dev 1** keeps a `BP_Stub_GameFlow` in `Space/` that fakes the GameInstance (prints `TravelTo`) until G-02 lands.
- **Dev 2** builds HUD widgets against a `BP_Stub_HUDData` with fake numbers until S-16 lands.
- **Dev 2** can test the landing sequence with a static ship mesh before Dev 1's ships exist.

---

## 4. Shared setup (first hour of Nov 12, done together on a call)

### C-01 · Project bootstrap — **Dev 1 drives, Dev 2 watches** · P0 · `⬜`
Only one person creates the project. The other clones it afterwards.
- [ ] Create the UE project (record the exact engine version in AGENTS.md; both devs must match).
- [ ] Add the Unreal `.gitignore` and `.gitattributes` with Git LFS (`*.uasset`, `*.umap`, images, audio, meshes; mark them `lockable`).
- [ ] Create the full folder tree from §2, with empty `.gitkeep`-style placeholder assets where needed.
- [ ] Create the collision channels (`Ship`, `Projectile`, `Character`, `Interactable`) and the default maps in Project Settings.
- [ ] Create empty maps: `L_Galaxy` (World Partition and **One File Per Actor on**), `L_MainMenu`, `L_Planet_A`, `L_Mothership_Interior`, `L_Transition`, and each dev's test map.
- [ ] Create `CLAIMS.md`, `CREDITS.md` and `DEVLOG.md`.
- [ ] Push. **Dev 2 clones and opens the project** to confirm it works on their machine.

**Done when:** both machines open the same project, on the same engine version, with no errors.

### C-02 · Contracts — **Dev 2 drives, Dev 1 reviews** · P0 · `⬜`
- [ ] Create every enum, struct and interface in §3.1 and §3.2 exactly as written.
- [ ] Dev 1 creates `BP_TD_ProjectileBase` (speed, damage, team, lifetime, implements `BPI_TD_Deflectable` and `BPI_TD_Team`).
- [ ] Push. **From here on `Core/` is frozen.**

**Done when:** both devs can reference every contract asset from their own folders.

### C-03 · Theme pass — **both** · P0 · `⬜` 🎨 THEME
- [ ] Write the theme at the top of this file and in AGENTS.md.
- [ ] Spend 15 minutes deciding how Triad Drift expresses the theme. The theme should be visible in gameplay, not just in the title.
- [ ] Tag the affected features below with a short note.

---

## 5. 🟦 DEV 1 — Space

> Build order is riskiest-feel-first. **Flight feel is the whole game. Get it fun before anything else.**

### Day 1 (Nov 12): fly, shoot, switch, wingmen

#### S-01 · Arcade flight model · P0 · `⬜`
**What:** Fast, readable, arcade 6DOF-lite flight. It should feel great with **gamepad and mouse + keyboard**.
- [ ] `BP_ShipBase` pawn: forward thrust (throttle up/down), pitch/yaw toward aim, roll, strafe (optional).
- [ ] Boost (hold) drains a boost meter. Brake/drift (hold) lets the ship slide while turning: the "drift" in Triad Drift.
- [ ] Speed-based camera: spring-arm chase cam, FOV widens with speed, slight lag, roll tilt.
- [ ] Enhanced Input: `IMC_Space` with `IA_Throttle`, `IA_Look`, `IA_Roll`, `IA_Boost`, `IA_Drift`, `IA_Fire`, `IA_Power`, `IA_Ability`, `IA_SwitchNext`, `IA_SwitchPrev`, `IA_Hyperdrive`, `IA_Interact`.
- [ ] Soft play-space boundary (push back + warning flag for the HUD) so players can't fly into nothing.
- [ ] All tuning values exposed in `DA_ShipStats` (no magic numbers in graphs).

**Done when:** flying around an empty `L_Test_Space` with a few asteroids is fun for 2 minutes on its own.

#### S-02 · Primary weapons & projectiles · P0 · `⬜`
- [ ] `BP_Weapon_Base` component: fire rate, spread, projectile class, muzzle sockets, heat or ammo (pick one, keep it simple).
- [ ] `BP_Proj_Laser` (child of `BP_TD_ProjectileBase`): bright trail, hit spark, damage via `Apply Damage`.
- [ ] Light aim assist: bullets bend slightly toward the targeted enemy within a small cone.
- [ ] Hit feedback: hit-marker event (for the HUD), a small camera shake, and an impact sound.

**Done when:** you can shoot and destroy a static target dummy and it *feels* punchy.

#### S-03 · Ship health, shields, death & respawn · P0 · `⬜`
- [ ] Health + regenerating shield. Shield absorbs first and regens after N seconds without damage.
- [ ] Death: explosion VFX, ship disabled. **Respawn rule:** a downed ship respawns near the player after 10 s (tune this).
- [ ] If all 3 are down at once, fire `OnSquadWiped` and call `GameFlow.ReportPlayerDefeated()`.
- [ ] If the ship you're flying dies, auto-switch to a living wingman.

**Done when:** you can die, auto-switch, see the downed ship respawn, and trigger a squad wipe.

#### S-04 · The three ships (variants) · P0 · `⬜`
| Ship | Feel | Primary weapon | Stats direction |
|---|---|---|---|
| **Rift** (Trickster) | Agile, mid speed | Twin pulse lasers | Mid HP, mid speed, best turn |
| **Lancer** (Interceptor) | Fastest | Rapid cannons, high fire rate | Low HP, top speed, quick boost regen |
| **Bastion** (Heavy) | Slow, tanky | Heavy blaster | High HP + shield, slowest |
- [ ] Child BPs + `DA_ShipStats_Rift/Lancer/Bastion`.
- [ ] **Distinct silhouettes and engine-glow colors** so they read at a glance (Rift = purple/magenta, Lancer = cyan, Bastion = orange/gold).
- [ ] Placeholder meshes are fine on Day 1. Use AI-generated or Fab meshes later (log them in `CREDITS.md`).

**Done when:** each ship clearly flies and shoots differently.

#### S-05 · Squad manager & instant switching · P0 · `⬜`
- [ ] `BP_SquadManager` spawns and tracks all 3 ships and owns who the player is in.
- [ ] `IA_SwitchNext` / `IA_SwitchPrev` (and optional direct keys 1/2/3 or d-pad) **instantly possess** the target ship. Quick camera blend (≤0.25 s), whoosh SFX, screen flash.
- [ ] The ship you leave hands off to its AI controller with no hitch.
- [ ] Short switch cooldown (~0.5 s) so it can't be spammed.
- [ ] Fires `OnShipSwitched`. Implements `BPI_TD_HUDData.GetSquadHUD()`.

**Done when:** you can switch mid-dogfight with zero hitch and immediately keep fighting.

#### S-06 · Wingman AI · P0 · `⬜`
- [ ] Follow: holds a loose V-formation behind and to the side of the player and catches up with boost if far away.
- [ ] Engage: picks the nearest enemy threatening the player (or itself), attacks with its **primary weapon only**, then breaks off and returns to formation.
- [ ] **Never uses signature powers.** Powers only fire when the player is in the cockpit.
- [ ] Avoids ramming the player and asteroids (simple avoidance traces).
- [ ] Optional P1: a "focus my target" command (`IA_Command`) that wingmen obey for 8 s.

**Done when:** with the player idle, the wingmen visibly fight enemies on their own and stay close.

### Day 2 (Nov 13): enemies, waves, powers, hyperdrive

#### S-07 · Enemy base + light fighter swarm · P0 · `⬜`
- [ ] `BP_EnemyShipBase`: team = Enemy, health, death explosion, drops nothing (keep it simple).
- [ ] `BP_Enemy_Fighter`: weak, fast, comes in packs of 4–8, makes strafing runs on the nearest player ship.
- [ ] Enemies target **any** squad ship, not just the player's, so the wingmen matter.

#### S-08 · Shielded gunship · P0 · `⬜`
- [ ] Slow, tanky, with a **frontal shield** that blocks shots. You flank it, or use powers (portal behind it, or Lancer ram).
- [ ] Fires slow, visible, dodgeable heavy bolts and a **homing missile** (it's what Rift's portal sends back).

#### S-09 · Carrier · P1 · `⬜`
- [ ] Big target with weak points (2–3 launch bays) that spawns fighters periodically until its bays are destroyed.
- [ ] Spawns a gunship at most once.

#### S-10 · Wave director · P0 · `⬜`
- [ ] `BP_WaveDirector` + `DT_Waves` (data table: wave index, enemy counts by type, spawn points, intensity).
- [ ] Spawns waves at spawn points around the play space, often **behind** the player, so portals and hyperdrive matter.
- [ ] Waves 1–2 are fighters only. Wave 3 adds gunships. Wave 4 is mixed. Wave 5 adds a carrier. After wave 5, call `SetMothershipState(Exposed=true)`.
- [ ] Reads `GetProgress().WavesCleared` on level load so returning from a planet **resumes** the right wave.
- [ ] After wave 2 (and 4 if Planet B is in), stops spawning and reveals the planet landing marker.
- [ ] Fires `OnWaveStarted` / `OnWaveCleared`. Implements `GetWaveInfo()`.
- [ ] Short breather between waves (5–10 s) with a "WAVE X INCOMING" event for the HUD.

#### S-11 · Lancer signature: afterburner ram · P0 · `⬜`
- [ ] Press Power: huge burst of speed for ~1 s. The ship becomes invulnerable and **rams through** enemies, dealing heavy damage and shredding fighters.
- [ ] Breaks gunship shields. Cooldown ~6 s. Big motion-blur/streak VFX and a sonic boom.

#### S-12 · Bastion signature: missile swarm + shield dome · P0 · `⬜`
- [ ] **Missile swarm** (Power tap): fires 8–12 homing micro-missiles at up to N locked targets.
- [ ] **Shield dome** (Power hold, or a second input): a bubble around Bastion for ~5 s. Wingmen inside take no damage, and enemy projectiles hitting it are destroyed (P1: deflected via `BPI_TD_Deflectable`).
- [ ] Separate cooldowns. Pull wingmen toward the dome while it's up so it's useful.

#### S-13 · Rift signature: portal blaster · P0 · ⚠️ highest-risk feature · `⬜`
- [ ] Fire 1 places **Portal A** at the impact point or max range. Fire 2 places **Portal B**. A third shot recycles A.
- [ ] Flying into one portal teleports the ship out of the other, **keeping speed and exit-facing direction**. The camera must not snap badly (use a fade/streak on transition).
- [ ] **Projectiles** (especially enemy homing missiles) that enter a portal exit the other and get `Deflect()`'d to Player team, so missiles can be sent back.
- [ ] Wingmen can follow you through an open portal (P1).
- [ ] Portals last ~10 s. Readable, glowing ring VFX in Rift's color.
- [ ] **Fallback if it's buggy by end of Day 2:** single-portal "blink" (fire → instantly teleport to the impact point) and a separate missile-reflect pulse. Same fantasy, far less risk.

#### S-14 · Hyperdrive · P0 · `⬜`
- [ ] Hold `IA_Hyperdrive` to spool up (~1.5 s wind-up SFX and VFX), then a very fast straight-line burst across open space for a few seconds. Steering is limited and it's interrupted by collision.
- [ ] Cannot engage while taking heavy fire (P1). Wingmen jump with you and arrive in formation.
- [ ] Used to reach planet markers and the mothership quickly. Implements `GetHyperdriveCharge01()`.

#### S-15 · Portal gates (travel shortcuts) · P1 · `⬜`
- [ ] 2–3 permanent `BP_PortalGate` pairs in `L_Galaxy` that link distant zones, so the squad can appear behind an enemy fleet.
- [ ] Reuses S-13's teleport logic.

#### S-16 · Targeting & HUD data · P0 · `⬜`
- [ ] Target selection: auto-target the enemy nearest the screen center, with a manual cycle button.
- [ ] Implements every function of `BPI_TD_HUDData` (squad status, target, wave info, hyperdrive, objective markers).
- [ ] Off-screen enemy and objective direction data for HUD arrows. Dev 2 draws them.
- [ ] Until Dev 2's HUD lands, a quick debug print/Widget in `Developers/` is fine.

### Day 3 (Nov 14): the galaxy map, unlockable abilities, mothership exterior

#### S-17 · Unlockable ship abilities · P0 (one ability) / P1 (all three) · `⬜`
- [ ] Ability framework on `BP_ShipBase`: one ability slot (`IA_Ability`), filled from `GetProgress().UnlockedAbilities` on spawn and on `OnAbilityUnlocked`.
- [ ] **Gravity Well** (P0, Bastion by default): throws a sphere that pulls nearby enemies together for 4 s. Combos with the missile swarm.
- [ ] **EMP Pulse** (P1, Lancer): a radial blast that stuns enemies and strips gunship shields for 3 s.
- [ ] **Cloak** (P1, Rift): 5 s invisible to enemy AI, and firing breaks it. The first shot from cloak deals bonus damage.
- [ ] The ship that receives each ability is decided by the objective's `ForShip` (set by Dev 2 in the objective).

#### S-18 · Galaxy map (`L_Galaxy`) gameplay layout · P0 · `⬜`
- [ ] A compact but huge-feeling play space: a central combat zone, Planet A off to one side, (Planet B) on the other, and the mothership far out.
- [ ] Place Dev 2's `BP_GalaxySkybox`, `BP_PlanetVisual` and `BP_NebulaVolume` actors when they're ready (use placeholders until then).
- [ ] `BP_LandingZone` at each planet: flying into the atmosphere trigger while the marker is active calls `GameFlow.TravelTo(PlanetA, ReturnPoint="PlanetA_Orbit")`.
- [ ] Return points: when you come back from a planet, the squad spawns at the matching `ReturnPoint` in orbit.
- [ ] Asteroid fields and debris as cover and set dressing (Dev 1 places them; Dev 2 can supply meshes).

#### S-19 · Mothership exterior & docking · P0 · `⬜`
- [ ] `BP_Mothership_Exterior`: massive, bold silhouette, appears or warps in when `MothershipExposed` becomes true.
- [ ] Defenses: 3–4 `BP_MS_ShieldGenerator` weak points + some `BP_MS_Turret`s. Destroying all generators drops the shield, which calls `SetMothershipState(DefensesDown=true)`.
- [ ] Fighters keep launching during the attack (reuse waves).
- [ ] `BP_DockingPort` opens when the defenses are down. Flying in calls `TravelTo(MothershipInterior)`.

### Day 4 (Nov 15): polish & cut-list items

#### S-20 · Escape flight · P2 ✂️ (cut #3) · `⬜`
- [ ] After the core is destroyed (Dev 2 calls `TravelTo(Galaxy, ReturnPoint="Escape")`), the squad launches from the mothership with a 30 s timer through falling debris and explosions.
- [ ] Reaching safe distance plays the mothership breakup and calls `TravelTo(Victory)`.

#### S-21 · Space combat juice & VFX · P1 · `⬜`
- [ ] Big, clear, colorful explosions (Niagara), weapon trails, engine glows, speed lines, hit flashes.
- [ ] Camera shake tiers (hit / boost / ram / big explosion). Brief hit-stop on kills.
- [ ] Readability first: enemy projectiles are a **different color family** from player ones (enemy = red/orange, player = squad colors).

#### S-22 · Space SFX hookup · P1 · `⬜`
- [ ] Engine roar that pitches with speed, weapon SFX per ship, explosions, hyperdrive wind-up, portal open/through, switch whoosh, shield hit, low-health warning.
- [ ] Sounds live in `Space/Audio/`. Use attenuation so distant fights are quieter.

#### S-23 · Space balance & tuning pass · P1 · `⬜`
- [ ] A full playthrough of all waves at least twice. Tune enemy counts, HP and cooldowns. Target 10–15 minutes for the full game.

---

## 6. 🟩 DEV 2 — Worlds & Presentation

> Build order: game flow + first-person character + blade first (they're shared by planets **and** the mothership),
> then the planet, then the mothership interior.

### Day 1 (Nov 12): game flow, first-person character, blade, environment art

#### G-01 · Game instance, progression & level travel · P0 · `⬜`
- [ ] `BP_TD_GameInstance` implements `BPI_TD_GameFlow` and holds `S_TD_Progress` for the whole run.
- [ ] `TravelTo()` → fade out → `L_Transition` (loading with a tip/art card) → open the target level → fade in.
- [ ] `CompleteObjective()` adds the objective to `CompletedObjectives`, writes `UnlockedAbilities` and fires `OnAbilityUnlocked`.
- [ ] `ReportPlayerDefeated()` shows the defeat screen with **Retry from last checkpoint** (restore the progress snapshot taken on level entry) and Quit to menu.
- [ ] A **debug menu** (cheat key, dev builds only) to jump to any destination with any progress state. This is critical for both devs' testing.
- [ ] Set as the project's Game Instance class (this is a Project Settings edit, so claim it).

**Done when:** you can go Menu → Galaxy → Planet A → Galaxy → Mothership → Victory using stubs and the debug menu.

#### G-02 · First-person character · P0 · `⬜`
- [ ] `BP_FPCharacter`: walk, sprint, jump, dash (short burst; feels fast like the space game), mouse look, gamepad support.
- [ ] `IMC_Ground`: `IA_Move`, `IA_Look`, `IA_Jump`, `IA_Sprint`, `IA_Dash`, `IA_Attack`, `IA_Block`, `IA_Interact`.
- [ ] Health with a brief regen delay. On death, call `ReportPlayerDefeated()` (or a local checkpoint respawn, P1).
- [ ] Visible first-person arms or hands holding the blade (placeholder mannequin arms are fine).
- [ ] Interact system: a trace for `Interactable` objects with a prompt event for the HUD.

#### G-03 · Energy blade · P0 · `⬜`
- [ ] **Attack:** a 2–3 hit light combo with sweep traces in front of the player and generous range (arcade, not realistic). A heavy/lunge attack is P1.
- [ ] **Block/Deflect (hold):** projectiles that hit the blade arc while blocking are `Deflect()`'d **back toward whoever fired them**. A perfect-timing window (~0.2 s after pressing) auto-aims the deflection at the shooter, while a late block just blocks.
- [ ] Juice: bright emissive blade with a trail, hum SFX that changes with swing speed, clash sparks, and hit-stop on kill.
- [ ] Ground enemy projectiles must be children of `BP_TD_ProjectileBase` so deflection just works.

**Done when:** in `L_Test_Ground`, you can kill a dummy and deflect a turret's shots back to kill it.

#### G-04 · Space environment art (as actors) · P0 · `⬜` 🎨 bright, saturated sci-fi
- [ ] `BP_GalaxySkybox`: a saturated nebula skybox (pink, teal, violet), a star field and a distant sun.
- [ ] `BP_PlanetVisual`: a big planet sphere with an atmosphere glow and **rings**, with color parameters. Used for Planet A, Planet B and background planets.
- [ ] `BP_NebulaVolume` (colored fog volumes the squad flies through) and `BP_SpaceDust` (particles that sell speed).
- [ ] Hand them to Dev 1 as ready-to-drop actors. **Don't edit `L_Galaxy` yourself.**
- [ ] Original look only. "In the spirit of" bright sci-fi, but no copied assets, logos or names from existing games.

### Day 2 (Nov 13): HUD, menus, crew enemies, objective framework

#### G-05 · Space HUD · P0 · `⬜`
- [ ] `WBP_SpaceHUD`, built against `BPI_TD_HUDData` (use `BP_Stub_HUDData` until Dev 1's S-16 lands).
- [ ] Squad panel: 3 ship cards (icon, health, shield, power cooldown ring, ability icon, alive/respawn timer) with the active ship highlighted. Animate on `OnShipSwitched`.
- [ ] Center: crosshair, target bracket with target health, lead indicator (P1), hit markers.
- [ ] Edges: off-screen enemy arrows and an objective marker arrow with distance.
- [ ] Top: "WAVE X" banner, enemies-left count. A hyperdrive charge bar. Boundary warning.
- [ ] Readable at a glance, big and bold, in squad colors.

#### G-06 · Menus & meta screens · P0 · `⬜`
- [ ] `L_MainMenu` with `WBP_MainMenu`: title, Play, Controls, Settings, Credits, Quit. Animated background (a slow camera drift past planets using G-04 actors).
- [ ] `WBP_Pause` (resume, controls, restart checkpoint, quit to menu).
- [ ] `WBP_Settings`: master/music/SFX volume, mouse sensitivity, invert Y, and (P1) a graphics quality preset.
- [ ] `WBP_Controls`: KB/M + gamepad layout image.
- [ ] `WBP_Defeat` and `WBP_Victory` (run time, waves cleared, abilities unlocked) + `WBP_Credits` (reads from `CREDITS.md` content, team + tools + assets).

#### G-07 · Onboarding prompts · P1 · `⬜`
- [ ] `WBP_Tip` toast system: contextual one-line prompts ("Press LB/Q to switch ships", "Rift: fire twice to open portals").
- [ ] Triggered by a simple `ShowTip(Id)` call on the GameInstance. Dev 1 calls it at the right moments in waves 1–2.

#### G-08 · Crew enemies (on foot) · P0 · `⬜`
- [ ] `BP_Crew_Grunt`: melee rusher. Telegraphs its attack (wind-up glow), dies in 2–3 blade hits.
- [ ] `BP_Crew_Trooper`: ranged. Fires **slow, bright, deflectable** blaster bolts and keeps distance. Teaches deflect.
- [ ] `BP_Crew_Heavy` (P1): shielded and blocks frontal slashes. You kill it with its own deflected shots, or by flanking with a dash.
- [ ] Simple AI (Behavior Tree or State Tree): patrol/idle → alert → engage. Stays readable, with no instant hits.
- [ ] Reuse the same enemies on planets (themed outpost guards) and on the mothership.

#### G-09 · Objective framework · P0 · `⬜`
- [ ] `BP_ObjectiveBase`: `ObjectiveId:Name`, `RewardAbility:E_TD_AbilityId`, `RewardShip:E_TD_ShipId`, steps, and an `OnCompleted` event.
- [ ] Objective tracker widget (top-left text + waypoint marker).
- [ ] On complete: calls `GameFlow.CompleteObjective(...)`, then shows the reward screen (G-11).

#### G-10 · Ground HUD · P0 · `⬜`
- [ ] `WBP_GroundHUD`: health, crosshair/dot, blade-block state + perfect-deflect flash, interact prompt, objective tracker, damage direction indicators.

### Day 3 (Nov 14): Planet A, landing, mothership interior, core

#### G-11 · Ability unlock reward screen · P0 · `⬜`
- [ ] `WBP_AbilityUnlocked`: a big, satisfying moment: ability icon, name, which ship got it, a one-line "how to use", and a short animation + sting SFX.

#### G-12 · Landing & take-off sequence · P0 · `⬜`
- [ ] **Descent (P0, cinematic):** on arriving in `L_Planet_A`, play `LS_Descent`: the squad punches through the atmosphere (fire/heat VFX, clouds, rumble) and lands on a pad. Then the player **steps out** in first person next to `BP_LandedShip` (a non-flying prop of the ship they were piloting).
- [ ] **Take-off:** interact with `BP_LandedShip` after the objective is complete → `LS_Takeoff` → `TravelTo(Galaxy, ReturnPoint="PlanetA_Orbit")`.
- [ ] P2 ✂️: player-controlled atmospheric descent using Dev 1's ship pawn (only if both devs are ahead).

#### G-13 · Planet A level + objective · P0 · `⬜` 🎨 THEME
- [ ] `L_Planet_A`: one compact, gorgeous biome (for example glowing alien jungle or crystal desert) with a ringed planet in the sky. One landing pad and a clear path to the objective (~3–5 min to complete).
- [ ] **Objective: "Recover the wrecked ship's core."** Follow the wreck's beacon → fight off a crew patrol (grunts + troopers) → interact with the core (hold) while 2 waves of enemies attack → carry or auto-transfer it back to the ship.
- [ ] Reward: **Gravity Well → Bastion** (matches S-17 P0).
- [ ] Environmental storytelling: wreckage, the enemy faction's banners, a sense of scale (huge rock arches, giant flora).

#### G-14 · Mothership interior · P0 · `⬜`
- [ ] `L_Mothership_Interior`: arrive at a docking bay → 3–4 corridor/room sections → reactor core chamber. Linear, ~4–6 minutes.
- [ ] A modular kit (walls, floors, doors, pipes, glowing panels) in the enemy faction's colors (red/black contrast against the bright galaxy).
- [ ] Encounters escalate: grunts → troopers in a hallway (deflect lesson) → mixed room → heavy at the core door.
- [ ] Locked doors open when a room is cleared (simple "all enemies dead" trigger).

#### G-15 · Core encounter & escape run · P0 (core) / P1 (run) · `⬜`
- [ ] `BP_MS_Core`: a big glowing reactor with 3 exposed weak points (destroyed with blade hits) and turrets that shoot deflectable bolts. Phase 2 spawns crew reinforcements.
- [ ] On destroy: alarms, red lighting, shaking, debris, and a **countdown timer** (~60 s) to run back to the docking bay (a shortcut door opens).
- [ ] Reaching the ship → `TravelTo(Galaxy, "Escape")` if S-20 exists, otherwise `TravelTo(Victory)` with a cinematic fade.

### Day 4 (Nov 15): Planet B (if ahead), audio, packaging, submission

#### G-16 · Planet B + second objective · P2 ✂️ (cut #1) · `⬜`
- [ ] `L_Planet_B`: a second biome that reuses as much of Planet A as possible (new palette and props).
- [ ] **Objective: "Clear the enemy outpost."** Kill all guards, destroy the signal tower. Reward: **EMP Pulse → Lancer**.
- [ ] Only start if Planet A, the mothership and the core are done by Nov 14 night.

#### G-17 · Music & audio mix · P1 · `⬜`
- [ ] `BP_MusicDirector`: layered/stem soundtrack that **swells with wave intensity** (binds to `OnWaveStarted(Intensity01)`), calmer between waves, a distinct ground/planet track, and an urgent mothership track + escape stinger.
- [ ] Sound classes/mix: Master, Music, SFX, UI (hooked to the settings sliders).
- [ ] Ground SFX: footsteps, blade hum/swing/clash/deflect, crew barks, doors, core alarms, UI clicks.
- [ ] Sources: WeLoveIndies jam pack, AI audio tools, etc. **Log every sound/music source in `CREDITS.md`.**

#### G-18 · Packaging & build pipeline · P0 · `⬜`
- [ ] Set up the packaging profile (Windows, Shipping). Build **every evening** starting Nov 13 into `Builds/` (git-ignored) and smoke-test the packaged exe start → finish.
- [ ] Check that every level is listed in "Maps to Package". Startup map = `L_MainMenu`. Default GameInstance is set.
- [ ] Final zip named **`TeamName_GameName`**, < 1 GB (otherwise use butler).
- [ ] Performance pass: target a steady 60 fps on mid hardware (scalability defaults, Niagara particle caps, Lumen settings).

#### G-19 · Submission package · P0 · `⬜`
- [ ] itch.io project page: title, cover art, screenshots, description, controls, team, credits, tools used, third-party assets, pre-jam declarations.
- [ ] **30–60 s gameplay video** with gameplay in the first 5 seconds (dogfight → switch → portal → planet → blade).
- [ ] Dev log (Glass Box prize) compiled from `DEVLOG.md` with screenshots.
- [ ] Submit to the jam with **a buffer of at least 2–3 hours before 8:59 PM EST**. Both humans double-check the form.

---

## 7. Integration points (where the halves meet)

| # | Integration | Dev 1 delivers | Dev 2 delivers | Target |
|---|---|---|---|---|
| I-1 | Level travel | Calls `TravelTo` from `BP_LandingZone` / `BP_DockingPort` | `BP_TD_GameInstance` travel + fades | Nov 13 PM |
| I-2 | Space HUD | `BPI_TD_HUDData` on `BP_SquadManager` | `WBP_SpaceHUD` | Nov 13 PM |
| I-3 | Environment art | Places the actors in `L_Galaxy` | Skybox/planet/nebula actors | Nov 13 PM |
| I-4 | Wave → music | `OnWaveStarted/Cleared` dispatchers | `BP_MusicDirector` binds | Nov 14 PM |
| I-5 | Progress resume | Wave director reads `GetProgress()` | Progress persisted across levels | Nov 14 AM |
| I-6 | Ability unlock | Ability slot reads `UnlockedAbilities` | `CompleteObjective` + reward screen | Nov 14 PM |
| I-7 | Mothership | Exterior defenses + dock → `TravelTo(MothershipInterior)` | Interior + core → `TravelTo(Galaxy/Victory)` | Nov 14 PM |
| I-8 | Landed ship visual | Ship meshes/materials (share the mesh asset path) | `BP_LandedShip` references Dev 1's mesh (read-only) | Nov 14 AM |
| I-9 | Tips | Calls `ShowTip(Id)` in early waves | `WBP_Tip` system | Nov 14 |

**Daily integration check (every evening, both devs):** pull → full playthrough from Main Menu → package →
note bugs in §9 → assign each bug to the owner of the file it lives in.

---

## 8. Schedule at a glance

| Day | Dev 1 (Space) | Dev 2 (Worlds & Presentation) | End-of-day playable |
|---|---|---|---|
| **Nov 12** | C-01, S-01 → S-06 | C-02, C-03, G-01 → G-04 | Fly 3 switchable ships with fighting wingmen · FP character + blade test map · travel works with stubs |
| **Nov 13** | S-07, S-08, S-10, S-11, S-12, S-13, S-14, S-16 (S-09 if time) | G-05 → G-10 · **first packaged build (G-18)** | Waves with powers + hyperdrive, real HUD and menus, crew enemies in the test map · packaged |
| **Nov 14** | S-17 (Gravity Well), S-18, S-19, S-15 | G-11 → G-15 | **Full game start → finish:** waves → Planet A → unlock → waves → mothership → core → victory · packaged |
| **Nov 15** | S-21, S-22, S-23, S-17 extra abilities, S-20 if ahead | G-17, G-18, G-19, G-16 if ahead | **Final package + submission well before 8:59 PM EST** |

**Feature freeze:** Nov 15, 12:00 PM EST. After that, only bug fixes, audio, polish and the submission.

### Cut order (when time runs short, cut in exactly this order)
1. ✂️ **Planet B** (G-16) and its EMP-on-Lancer unlock
2. ✂️ **Extra unlockable abilities** (Cloak, EMP), keeping only Gravity Well
3. ✂️ **Escape flight** (S-20). Use the victory fade instead.
4. Then: Carrier (S-09) → portal gates (S-15) → heavy crew (G-08 heavy) → onboarding tips (G-07)
5. Last resort: Rift portal → fallback blink (see S-13)

**Never cut:** flight feel, ship switching, wingmen, the 3 signature powers, waves, one planet + one unlock,
the mothership boarding with the blade, audio, and a packaged build.

---

## 9. Bugs & requests (append-only: add new lines at the bottom of each list)

Writing to the bottom only avoids merge conflicts. Each line should say who it's for, then a description and who reported it.

### Requests between devs
- _e.g. `[for Dev 1] Expose ship mesh soft ref on DA_ShipStats for BP_LandedShip — Dev 2, Nov 13`_

### Bugs
- _e.g. `[Dev 2] WBP_SpaceHUD: shield bar doesn't update after respawn — found by Dev 1, Nov 13`_
