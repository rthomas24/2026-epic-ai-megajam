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

## Planet arrival and hand-off (agreed with T1, ENG-20)

| Moment | Who | What |
|---|---|---|
| Galaxy → planet | T1 | After the 2.5 s entry, `UTDAtmosphereEntryComponent` calls `TravelTo(Verdance or Cinder, ReturnPoint)`. T4's planet atmosphere supplies the return point. |
| Fresh arrival on a planet map | T5 | The ground mode spawns the squad **in the air, flying**, at `PlayerStart_Descent`, so T1's `ATDAtmosphereFlightZone` takes over the descent. |
| Checkpoint or retake on a planet, after landing | T5 | The player respawns on foot at the last pad, and the ship is snapped down with `GetLanding()->SnapLanded(Pad)`. |
| Landed | T1 → T5 | `UTDTravelSubsystem::OnShipLanded(Ship, Pad)`. For the player-flown ship only (`Ship->IsPlayerFlown()`), T5 spawns the first-person character at the ship's side and possesses it. The ship stays parked. |
| Re-board | T5 → T1 | A board interactable on the parked ship possesses the ship again, then calls `Ship->GetLanding()->RequestTakeOff()`. T1 fires `OnShipTookOff`, and the squadmates lift with the player. |
| Planet → galaxy | T1 | Once `RequiredObjectives` are done and the player climbs out, the zone calls `TravelTo(Galaxy, "<Planet>_Orbit")`. |
| Galaxy arrival | T4 + T5 | T5's space mode places the squad at `ATDGalaxyLayout::FindReturnPoint(ReturnPoint)`. T4 positions that point outside the atmosphere trigger and inside T1's gravity well. |

Status: the agreement is recorded, but the arrival and hand-off code on T5's side isn't built yet. It comes with ENG-25/26, once T1's ENG-20 is on main.

## Known gaps
- **Input:** real key injection can't run headless. The harness's PlayerController has no local player, so the tests
  possess the character with an AIController and drive the same public move API that the input handlers call.
  Nobody has played the character windowed yet.
- **Perfect-deflect slow-mo:** features.md says 0.15 s, but T3's director plays 0.5–1.0 s for an accepted
  highlight. The blade falls back to its own 0.15 s only when the director refuses. T3 has been told.
- **Crew AI:** the crew are stand-ins (ENG-23 replaces them). They have no ragdoll, and the guard break is only a flag.
- **Carrying:** `SetCarrying` (no sprint/block while carrying) exists but isn't wired to an objective yet (ENG-25).
