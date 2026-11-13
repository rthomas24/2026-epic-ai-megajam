# On foot (ENG-21, ENG-22, ENG-24): Teammate 5

Code: `Source/unrealGameJam/TriadDrift/Ground/`. Tests: `Ground/Tests/` (`TriadDrift.Ground.*`).

## What exists

| Class | What it does |
|---|---|
| `ATDFirstPersonCharacter` | The on-foot player (ENG-21). Enhanced Input is built in C++ with no assets. Also: `ITDTeam` (Player), the ground half of `ITDHUDData`, and animation state (`BladeState`, `bSprinting`, `bSliding`, `bDashing`, `bBlocking`, `bMantling`, `Speed01`). |
| `UTDEnergyBladeComponent` | The blade (ENG-22): combo, heavy, air slam, dash strike, block, deflect, parry, hit-stop. Presentation goes through `UTDBladeFX` cues (an empty slot does nothing). |
| `ATDObjective` + `UTDObjectiveSubsystem` | ENG-24: id, ordered steps (text + waypoint), and a reward. A scene per objective with retake reset; the waypoints are exposed for the HUD and the ping. |
| `ATDInteractableActor` | Tap or hold to use (`ITDInteractable`). It can complete an objective step. |
| `ATDEncounterVolume` | Spawns a crew group on entry and locks doors behind you. When the room is clear it opens the doors and completes a step. It resets on a retake. |
| `ATDDoor` | A sliding door (engine cube). |
| `ATDCrewDummy` | A stand-in crew enemy until ENG-23: health, Enemy team, `ITDStaggerable`, and a telegraphed melee swing. |
| `ATDDummyTurret` | Fires slow, deflectable 3-round bursts of T2's `ATDProjectile`. |
| `ATDGroundGameMode` | The zero-asset training ground (see below). |
| `ITDStaggerable` | The parry/guard-break contract that ENG-23 crew implement (`ApplyStagger`, `BreakGuard`). |

Controls:

| Action | Keys |
|---|---|
| Move / look | WASD / mouse |
| Jump, double jump | Space |
| Sprint | Shift |
| Dash | Q |
| Slide while sprinting, otherwise dash | Ctrl |
| Attack (hold for heavy) | LMB |
| Block | RMB |
| Interact (tap/hold) | E |
| Objective ping | Tab |

There are gamepad mappings too.

## How to try it
```
"C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor.exe" "<worktree>\unrealGameJam.uproject" /Engine/Maps/Entry?game=/Script/unrealGameJam.TDGroundGameMode -game -windowed
```
The training ground has:
- **Left:** a mantle lane of 100 / 150 / 180 / 250 cm ledges. The 250 one needs a double jump.
- **Right:** rising parkour stones.
- **Ahead:** two melee dummies that swing every 2.5 s, for parry practice, and a turret firing bolts for block and deflect practice.
- **Behind:** the sample objective. Hold E for 2 s at the console, then clear the vault of 3 crew.

Dying calls CUT! and you respawn as the next take. F10 opens the debug menu and `TD.*` cheats work.

## Feature coverage (features.md bullet → test)

### ENG-21 first-person character

| Bullet | Test | Notes |
|---|---|---|
| Run 650 / sprint 950 | `Ground.Move.RunAndSprintSpeeds` | At 30 and 120 fps; no sprinting backwards. |
| Jump Z 650 / double jump Z 500 | `Ground.Move.JumpAndDoubleJump` | No triple jump; resets on landing. |
| Dash: 1200 cm in 0.18 s, 0.15 s i-frames, 2 charges, 1.2 s regen | `Ground.Move.DashDistanceIFramesCharges` | At 30 and 120 fps. The dash moves the capsule with a sweep, so it's frame-rate exact. |
| Slide (Ctrl while sprinting), 0.7 s | `Ground.Move.Slide` | |
| Auto-mantle at ledges ≤ 180 cm, 0.35 s | `Ground.Move.MantleUpTo180` | 150 cm climbs in 0.35 s; 250 cm doesn't. |
| Attack / block / heavy inputs | `Ground.Blade.*` | Tested through `PressAttack`, `ReleaseAttack` and `SetBlockHeld`, the same functions the input bindings call. |
| Interact (tap/hold) via `ITDInteractable` | `Ground.InteractTapHoldAndInterrupt` | Includes the damage interrupt. |
| Objective ping (Tab) | **missing** | No test yet. |
| 100 health, regen 15/s after 4 s, death → checkpoint respawn | `Ground.HealthRegenAndDeathIsATake` | Death → CUT → `RespawnAt`. |
| Ground half of `ITDHUDData` | **missing** | Implemented, but not asserted. |

### ENG-22 energy blade

| Bullet | Test | Notes |
|---|---|---|
| Light ×3: 25 / 25 / 45, 180 cm, 100° arc, third knocks back, 0.45 s window | `Ground.Blade.LightCombo` | Also checks the arc, the reach, the buffered press and the window reset. |
| Heavy lunge: 70, 600 cm, breaks heavy guards, 2 s cooldown | `Ground.Blade.HeavyLungeAndCooldown` | Includes hold-to-heavy. |
| Air slam: 50 in 300 cm | `Ground.Blade.AirSlamAoE` | |
| Dash strike: 35 | `Ground.Blade.DashStrike` | |
| Block: 120° arc, 50% speed, 3 s stamina then guard break | `Ground.Blade.BlockStaminaGuardBreakParry` | |
| Perfect deflect ≤ 0.2 s: ×2, auto-aim at the shooter, slow-mo + `OnHighlightMoment` | `Ground.Blade.DeflectNormalAndPerfect` (real `ATDProjectile`) + `Ground.Blade.PerfectWindowAt30_60_120` | The slow-mo goes through T3's director, which sets its duration. |
| Normal deflect → crosshair | `Ground.Blade.DeflectNormalAndPerfect` | |
| Melee parry: 1.2 s stagger, next hit crits | `Ground.Blade.BlockStaminaGuardBreakParry` | |
| Hit-stop 50 ms on hit / 120 ms on kill | **missing** | Implemented (`UTDFeelSubsystem::RequestHitStop`), but not asserted. |

### ENG-24 objective framework

| Bullet | Test | Notes |
|---|---|---|
| `ATDObjective`: id, steps, waypoint, reward; `CompleteObjective` | `Ground.ObjectiveStepsRewardAndRetake` | Also covers the retake reset. |
| `OnObjectiveUpdated` payloads | **missing** | Fired, but not asserted. |
| Interactables: tap/hold, interrupted by damage | `Ground.InteractTapHoldAndInterrupt` | |
| Encounter volumes spawn crew; clearing a room opens its doors | `Ground.EncounterSpawnsClearsOpensDoors` | |
| Sandbox boots | `Ground.SandboxBoots` | |

## Crew enemies (ENG-23)

`Ground/Crew/`:
- `ATDCrewCharacter`: an ACharacter with `ITDTeam` (Enemy) and `ITDStaggerable`.
- `ATDCrewAIController`: a C++ state machine, Idle/Patrol → Alert → Engage → Stagger → Dead. It senses through the combat registry with line of sight, and steers with `AddMovementInput`, so no navmesh is needed.
- `UTDCrewTokenSubsystem`: at most 2 melee attackers at once.
- `FTDCrewLibrary`: the spec table.

| Type | HP | Attacks | Behaviour |
|---|---|---|---|
| Grunt | 60 | Baton 15 after a 0.6 s telegraph | Flanks off-axis. Needs a token; without one it circles at 330 cm. |
| Trooper | 50 | 3-round bursts of deflectable bolts (10) | Holds 900–1500 cm and strafes. |
| Heavy | 200 | Shield bash 25; shotgun 6 × 5 | Its frontal 120° shield blocks blade slashes until a heavy lunge breaks it (3 s). |
| Drone | 30 | Fast small bolts (5) | Flies about 320 cm up, at 600–950 cm. |
| Warden | 600 | Charge slam 40 (AoE), 7-bolt fan, summons 2 grunts | Phase 2 at 50%: cooldowns × 0.7, an 11-bolt fan, 3 summons. |

- **Projectiles:** bolts and pellets are T2's `ATDProjectile`, so the blade deflects them.
- **Telegraphs:** every telegraph plays `Visuals->Telegraph`, and `IsTelegraphing()` / `GetCurrentAttack()` are exposed for animation.
- **Death:** a placeholder ragdoll (the body block simulates), capsule collision off, cleaned up after 4 s. Kills reach the flow through the bus.
- **Visuals:** `Visuals` (`UTDEnemyVisuals`), or `/Game/TriadDrift/Presentation/Crew/DA_Crew_<Grunt|Trooper|Heavy|Drone|Warden>_Visuals` when it exists. A skeletal or static mesh hides the red placeholder block.
- **Encounters:** `ATDEncounterVolume` spawns `ATDCrewCharacter` with a `CrewMix` and alerts them to the player.

Tests (`TriadDrift.Ground.Crew.*`, all through the real AI, projectiles and blade):

| Behaviour | Test |
|---|---|
| Spec table | `SpecTable` |
| Grunt telegraph and 15 hit | `GruntApproachesTelegraphsAndHits` (30, 60 and 120 fps) |
| Attack tokens | `AtMostTwoGruntsAttackAtOnce` |
| Trooper distance and bursts | `TrooperKeepsDistanceAndBursts` |
| Real trooper bolt perfect-deflected back | `PerfectDeflectReturnsTrooperBolt` |
| Heavy shield, guard break, bash, 6 pellets | `HeavyShieldBashAndShotgun` |
| Drone hover and reach | `DroneHoversOutOfMeleeReach` |
| Parry staggers a grunt | `ParryStaggersGrunt` |
| Warden slam 40, fan 7/11, phase 2, summons 3 | `WardenPhasesSummonsFanAndSlam` |
| Warden AI cycles fan, slam and summon | `WardenAICyclesItsAttacks` |
| Blade kill → ragdoll → Kills stat | `BladeKillRagdollAndKillStat` |
| Visuals asset replaces the placeholder | `VisualsReplacePlaceholder` |

Gaps:
- **Cover:** troopers use `UTDCrewCoverSubsystem`, with four points per registered block (encounter `CoverActors`). They move to a point the player can't see, hide for 1.2 s, peek out to the side with line of sight, burst, then duck back. They relocate when flanked, when the player comes within 5 m, or when the cover is destroyed. Test: `Crew.TrooperUsesCoverPeeksAndRelocates`.
- The crew use C++ AI, not the template's StateTree.
- No skeletal art yet, so the ragdoll is the placeholder block.
- **Presentation:**
  - Visuals load from `/Game/TriadDrift/Presentation/DA_Crew_<Id>_Visuals` (flat folder).
  - `AnimBPClass` is applied together with `SkeletalMesh`.
  - Cues: Telegraph, Stagger (on entering stagger), Death, ShieldHit, HullHit, SlamImpact. They're observable through `OnCueRequested`; test: `Crew.PresentationCuesAndAnimBP`.
- **Blade:**
  - The blade loads `/Game/TriadDrift/Presentation/DA_Blade_FX`. `BladeMesh` replaces the placeholder; without it, the placeholder is tinted with `BladeColor`.
  - Retract plays on death (the blade is hidden and can't attack) and Ignite on respawn. Every cue goes through `OnCueRequested`; test: `Blade.PresentationCuesMeshAndRetract`.
## Planet arrival and hand-off (agreed with T1, ENG-20)

| Moment | Who | What |
|---|---|---|
| Galaxy → planet | T1 | After the 2.5 s entry, `UTDAtmosphereEntryComponent` calls `TravelTo(Verdance or Cinder, ReturnPoint)`. T4's planet atmosphere supplies the return point. |
| Fresh arrival on a planet map | T5 | The ground mode spawns the squad **in the air, flying**, at `PlayerStart_Descent`, so T1's `ATDAtmosphereFlightZone` takes over the descent. |
| Checkpoint or retake on a planet, after landing | T5 | The player respawns on foot at the last pad, and the ship is snapped down with `GetLanding()->SnapLanded(Pad)`. |
| Landed | T1 → T5 | `UTDTravelSubsystem::OnShipLanded(Ship, Pad)`. For the player-flown ship only (`Ship->IsPlayerFlown()`), T5 spawns the first-person character at ``Ship->GetLanding()->GetExitTransform()`` (320 cm to the ship's right, on the deck) and possesses it. Bind ``UTDTravelSubsystem::OnShipLanded`` / ``OnShipTookOff`` (dynamic delegates) from the ground mode. The ship stays parked. |
| Re-board | T5 → T1 | A board interactable at the same exit transform possesses the ship again, then calls `Ship->GetLanding()->RequestTakeOff()`. T1 fires `OnShipTookOff`, and the squadmates lift with the player. |
| Planet → galaxy | T1 | Once `RequiredObjectives` are done and the player climbs out, the zone calls `TravelTo(Galaxy, "<Planet>_Orbit")`. |
| Galaxy arrival | T4 + T5 | T5's space mode places the squad at `ATDGalaxyLayout::FindReturnPoint(ReturnPoint)`. T4 positions that point outside the atmosphere trigger and inside T1's gravity well. |

Status: the agreement is recorded, but the arrival and hand-off code on T5's side isn't built yet. It comes with ENG-25/26, once T1's ENG-20 is on main.

## Director's mercy (retakes of one fight, C-03 #3)

The rule: **a fight the player keeps losing eases, one notch at a time, and only for them.**
- **Takes:** every `ATDEncounterVolume` counts its own takes. Take 1 is the first time it starts, and each
  retake that resets and restarts it adds one. Deaths elsewhere don't count.
- **Tiers:** `MercyTiers` lists the steps (`FTDRetakeMercyTier`: `FromTake`, `HealthScale`, `MaxSummons`).
  From `FromTake` on, the highest tier reached applies to every member the room spawns: health is scaled,
  and a boss may summon at most `MaxSummons` times that take (-1 = no limit, 0 = never).
- **Logging:** each eased start logs `Director's mercy: <room> take N eases the fight (health x%, summons n)`.
- **Opt-in:** a room with no tiers is the same fight every take. Any on-foot fight can opt in.
- **Fewer crew:** a tier can also set `DropCrew`, which leaves members out from the end of the mix. At least one
  always fights.
- **Opening grace (every encounter):** crew spawned by an encounter hold fire for `OpeningGraceSeconds` (1.2 s).
  Before this, the agent journey lost 16 takes at the Verdance wreck ambush: the trooper's first burst landed 0.5 s
  after the crew appeared, about 20 damage per second in all, and a pilot who didn't react was dead in 5.1 s.
  Test: `Ground.Retake.OpeningGraceAndFewerCrew`.
- **Wreck ambush (Verdance):** from take 3 one grunt fewer; from take 5 the trooper alone.
- **Where it's used today:** only the Cinder Warden arena. Take 3 onward: 75% health and one summon. Take 5
  onward: 50% health and no summons. This came from the machine-played tests, which lost up to 9 takes in a
  row with the Warden nearly dead.
- Nobody who wins first time ever sees it. There is no on-screen notice (a slate tag could be added by
  presentation later).

| Test | What it proves |
|---|---|
| `Ground.Retake.DirectorsMercyEasesByTake` | Takes 1 to 5 on a room with tiers next to a plain room. It checks the health per take for every member, that the plain room never eases, and that the take-3 summon allowance is spent after one summon. |
| `Ground.Course.WardenEasesOnRetakes` | The real Cinder arena through real CUT retakes. Takes 1 and 2 are the full Warden. Takes 3 and 4 are at 75% with one summon. Take 5 is at 50% with none, and a summon is refused. |
| `Ground.Played.Cinder` | The machine-played run asserts only the outcome (outpost cleared, Warden down). Mercy is what keeps a losing streak from running out its tries. |

## Fight sizes (take 1) and standing-still damage

Rule: no on-foot fight's take 1 is a spike. A pilot who stands still takes about 25 damage per second at most, and every
fight has a checkpoint that refills the pilot when reached. Mercy tiers (above) are for a player who keeps losing,
not the baseline.
- **Model:** every attacker fires on its cooldown, at most 2 grunts swing at once (attack tokens), every shot hits.
- **Per crew:** grunt 15 / 2.0 s = 7.5 dps; trooper 3 x 10 / about 3.0 s = 10 dps; heavy about 10 dps (shotgun 6 x 5 at
  2.6–11 m, or bash 25); drone 5 / 1.15 s = 4.3 dps; Warden about 20 dps (bolt fan, slam 30).
- **Pilot:** 100 HP; regen 15/s after 4 s without damage. Reaching a checkpoint restores full health.
- **Note:** because of the token cap, a third grunt adds time to kill (60 HP) and another body flanking, not damage
  per second.

| Planet | Fight | Crew (take 1) | Standing-still dps | Checkpoint | Mercy |
|---|---|---|---|---|---|
| Verdance | wreck trail ambush | Trooper, Grunt, Grunt | about 25 (measured: 100 to 0 in 5.1 s) | none (step 0) | take 3: one fewer, take 5: two fewer |
| Verdance | wreck site | Grunt, Grunt, Heavy (was one more Grunt) | about 25 | site edge (new) | take 3: no Heavy, take 5: one Grunt |
| Verdance | extraction waves | Grunt, Grunt, then Trooper + Drone | about 15, then about 14 | at the wreck | none |
| Verdance | relay drones | 3 Drones | about 13 | plateau | none |
| Cinder | outpost gate guards | Grunt, Trooper, Grunt | about 25 | none (step 0) | none |
| Cinder | yard / barracks / command | G G T / H G G / T D T | about 25 / 25 / 24 | each door | none |
| Cinder | Warden | Warden (+1 summon) | about 20 | arena door | take 3: 75% and one summon, take 5: 50% and none |
| Cinder | jammer defence | Trooper, Grunt, Grunt (was + Drone, about 29) | about 25 | far bank | take 3: one fewer, take 5: two fewer |

Every encounter also has the 1.2 s opening grace and the wind-up tell (below), so damage starts about 1.6 s after
the crew appear.

## Reading an enemy shot (the deflect skill)

Deflecting is the core on-foot skill, so every shot is announced and slow enough to time a block against:
- **Opening grace:** encounter crew hold fire 1.2 s after they appear.
- **Wind-up tell:** for every crew attack, an Armada red-orange glow at the muzzle grows over the wind-up (35 cm to
  90 cm). Ranged attacks also draw a thin red-orange aim line to the target.
  - This is placeholder only: it's hidden when the crew's `DA_Crew_*_Visuals` supply a `Telegraph` cue, and gone when
    the attack releases.
  - Test: `Ground.Crew.PlaceholderTellBeforeEveryShot`.
  - Captures: `Saved/TDShots/mcp/journey/TELL_trooper_windup.png`, `TELL_bolt_in_flight.png`.
- **Trooper burst:** wind-up 0.4 s, then 3 bolts 0.18 s apart, 10 damage each. Bolt speed 2200 cm/s: 0.55 s to cross
  12 m, 0.68 s for 15 m.
- **Block:** a 120° frontal arc, 3 s of stamina.
- **Perfect deflect:** a block started up to 0.2 s before the hit (`PerfectWindow`) returns the bolt at the shooter
  for double damage, with a director's slow-motion beat.
- So from the crew appearing, the player has 1.2 s of grace, then a 0.4 s tell, then about 0.55 s of bolt flight:
  about 2 s to turn to the shooter and block.

## Third person on foot (ENG-32): T3's rig wired into the pilot

The pilot owns T3's `UTDThirdPersonCameraComponent` ("ThirdPersonCamera", on the capsule). See `director.md` for the rig itself.

- **Switching:** the saved choice (`UTDSettingsSubsystem::OnFootCameraMode`) is read at BeginPlay, so it applies at start-up and after every level travel. The pilot also binds `OnOnFootCameraModeChanged`, so V / View (T1's controller) and the Settings page switch it live. The rig holds a change while a director shot or the death cam owns the view.
- **Body:** in third person the whole pilot shows, head included. The camera-rig blade is hidden. The blade in the body's right hand (`HandBladeHilt` + `HandBladeBeam`) shows whenever the whole pilot is on screen: third person, director shots and the death view. It never shows in first person.
- **Hand blade:**
  - Meshes: `SM_TD_BladeHilt` and `SM_TD_BladeBeam`, with the beam on the hilt's `BladeEmitter` socket and `BladeColor` set on a dynamic `MI_TD_BladeBeam`. `UTDBladeFX::BladeMesh` wins if set.
  - Placement: a mesh socket `BladeSocket` if one exists, else `hand_r` with `HandBladeOffset` (-9, -3, 0 cm; pitch -90). That offset was set by eye with `TD.Ground.HandBlade`.
  - Retract scales the beam along its length over 0.15 s.
- **Facing:** `bUseControllerRotationYaw` is off in third person. The body turns at 900 deg/s (twice that in combat) toward `GetDesiredActorYaw`. Once standing still with the camera more than 60 deg off, it keeps turning until it faces the camera (the rule alone would stop at 60).
- **Aim:** `GetActorEyesViewPoint` keeps the eye position. In third person it aims at whatever the chase lens's centre ray hits within 50 m (behind the pilot doesn't count), so melee targeting, the deflect return and interaction go where the crosshair is.
- **Moves on the body** (`Pilot/Anims`, through `UTDLoopBlendAnimInstance`):

  | Move | Animation |
  |---|---|
  | Light 1 / 2 / 3 | `A_Pilot_Light1` / `2` / `3` (rate 1.5-1.6) |
  | Heavy lunge, dash strike | `A_Pilot_Heavy` |
  | Air slam | `A_Pilot_Light3` |
  | Block | upper-body loop `A_Pilot_Block` |
  | Guard break | `A_Pilot_HitReact_Front` (upper body) |
  | Dash, slide | `A_Pilot_Dash` |
  | Mantle | `A_Pilot_JumpStart` |
  | Falling | `A_Pilot_FallLoop` |
  | Hits | `A_Pilot_HitReact_Front` / `_Back` by direction (upper body) |
  | Death | ragdoll on `PA_Pilot`, as before |

  While moving faster than 150 cm/s, swings play from `spine_01` up, so the legs keep running.
- **Dev commands:** `TD.Ground.ThirdPerson [0|1]` and `TD.Ground.HandBlade [X Y Z P Y R]`. `TD.Ground.Bones` now also prints the loop, the one-shot, the upper loop and the procedural flag.
- **Tests:** `TriadDrift.Ground.ThirdPerson.*`:
  - PilotFollowsTheSavedChoice
  - PilotFacingRule
  - MeleeAndAimThroughTheCrosshair
  - EveryMoveReadsOnTheBody
  - HandBladeRetractsOnDeathAndReturns
  - T3's rig tests, which now run on the pilot's own rig

## Known gaps
- **Input:** real key injection can't run headless. The harness's PlayerController has no local player, so the tests
  possess the character with an AIController and drive the same public move API that the input handlers call.
  Nobody has played the character windowed yet.
- **Perfect-deflect slow-mo:** settled with T3. The director plays a 0.15 s beat for PerfectDeflect (`TriadDrift.Director.PerfectDeflectIsAShortBeat`, on eng/t3). The blade plays its own 0.15 s only when the director refuses the highlight; neither plays during a CUT or travel.
- **Crew AI:** the crew are stand-ins (ENG-23 replaces them). They have no ragdoll, and the guard break is only a flag.
- **Carrying:** `SetCarrying` (no sprint/block while carrying) exists but isn't wired to an objective yet (ENG-25).
