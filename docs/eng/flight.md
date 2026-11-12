# Flight & input (ENG-01, ENG-02)

Owner: Teammate 1. Code: `Source/unrealGameJam/TriadDrift/Flight/`, `Source/unrealGameJam/TriadDrift/Input/`.

## What exists

| Class | What it does |
|---|---|
| `FTDFlightTuning` (`TDFlightTypes.h`) | Every flight number, defaulting to the features.md ENG-01 value. Per-ship values (ENG-04) arrive through `ATDSpacecraft::ApplyFlightTuning`. |
| `TDFlightSim` (`TDFlightSim.h`) | The flight model as pure functions on plain data (`FTDFlightState`), with no world. Each frame is split into equal sub-steps of at most 1/120 s, so 30, 60 and 120 fps give the same flight. |
| `UTDShipMovementComponent` | Runs the sim every tick, then sweeps the move. Resolves collisions as glancing or head-on, and enforces the soft play-space boundary. Also handles impulses, teleports and speed overrides. |
| `ATDSpacecraft` | The flyable pawn shared by squad and enemy ships:<br>• a collision sphere (`TDCollision::Ship`)<br>• a placeholder hull: an engine cone plus cube wings<br>• Health (Core) and Weapons (T2)<br>• collision damage (`Environmental`, via `ApplyTDDamage`)<br>• `TeleportSpacecraft` (fires `OnTeleported` and the event bus)<br>• a visual bank on the hull |
| `ATDShip` | A squad ship: `ShipId`, Targeting (T2) and the chase camera. |
| `ATDSpacePlayerController` | Builds every input action and three mapping contexts in C++ (`IMC_SpaceCommon`, `IMC_Space`, `IMC_Atmosphere`). Flies the possessed ship and routes fire and target input to T2, and switching and squad commands to T3. Broadcasts a delegate for every other input. |

### Flight model (defaults)
- **Throttle:** follows the Core contract, so -1 is min speed (1500) and +1 is cruise (6000), linear in between. The player controller flies at cruise with no throttle input; S / LT slow down.
- **Aim-to-turn:** pitch and yaw rates follow the stick with response 12/s, capped at 110°/s. The angular rate is capped hard.
- **Roll:** 180°/s. With no roll input, the ship auto-levels toward world up at up to 40°/s. Set `AutoLevelRate = 0` for pure 6DOF.
- **Auto-bank:** visual only. The hull mesh banks up to 40° into yaw, while the flight frame stays predictable.
- **Strafe:** 2500 cm/s on the arrow keys and left stick Y. It's optional; features.md doesn't bind it.
- **Boost:**
  - speed is `BoostSpeed` (default 10800 = cruise +80%)
  - the meter lasts 3 s, starts regenerating 1.2 s after boost ends, and refills in 2.5 s
  - a boost needs 10% of the meter to start
  - no boosting while drifting
- **Drift:**
  - while held, velocity keeps its direction and the nose turns at 1.5× (165°/s)
  - on release, velocity snaps to the nose with a +20% burst, which bleeds off at 2500 cm/s²
- **Brake:** decelerates at 18000 cm/s² down to min speed. Space bar, or LT pressed past 92%.
- **Grip:** velocity re-aligns with the nose at 10/s, so knockback and scrapes slide a little and then recover.
- **Collisions:**
  - A hit counts as head-on when its "head-on-ness" is at least 0.7. Head-on hits bounce back at 35% restitution, halve the speed and deal 60 damage at cruise, scaled by impact speed.
  - Anything shallower is a scrape: it keeps 85% of the tangential speed, pops 400 cm/s off the surface and turns the nose halfway along it. A scrape deals up to 12 damage at cruise, scaled by angle and speed.
  - Collision damage has a 0.3 s cooldown. Bumping a squadmate does no damage (friendly fire is off).
- **Boundary:**
  - a sphere of radius 15 km (`BoundaryRadius`; levels call `SetBoundary(Center, Radius)`)
  - `IsNearBoundary` is the 500 m warning band inside the edge; `IsOutOfBounds` is past the edge
  - outside, the nose is steered back toward the centre at 22–90°/s, plus a push-back of up to 8000 cm/s², both growing with depth
- **Overrides:** `SetSpeedOverride(Speed, SteeringScale)` is for hyperdrive and the ram. After `ClearSpeedOverride`, speed above boost speed drops at 30000 cm/s².

### Chase camera (`FTDChaseCameraTuning` on `ATDShip`)
- arm 1400 cm, height 280 cm
- rotation lag 8, location lag 12 (max lag distance 1800 cm)
- FOV 90 → 110 with `Speed01`
- follows 30% of the ship's roll plus the visual bank
- leans up to 7° into turns (look-ahead)
- `SnapCamera()` makes a hard cut with no lag. It runs automatically on switch-in (`SetPlayerControlled(true)`) and on teleport.

### State for presentation and the HUD
On the spacecraft and its movement component:
- `GetSpeed01()` (min → boost), `IsBoosting()`, `IsDrifting()`, `GetBoost01()`
- `IsOutOfBounds()`, `IsNearBoundary()`
- `GetAngularRate()`, `GetVisualBank()`

Movement-component delegates: `OnBoostStarted` / `OnBoostEnded`, `OnDriftStarted` / `OnDriftReleased` (the burst moment) and `OnFlightImpact`.

On the controller:
- `GetVirtualStick()`: where the HUD draws the mouse reticle
- `IsUsingGamepad()`
- `IsSwitchRadialOpen()` / `GetRadialSelection()`
- `GetAimWorldDirection()`

### Input map (ENG-02)

| Action | KB/M | Gamepad | Routed to |
|---|---|---|---|
| Throttle | W / S | RT / LT | flight |
| Aim | Mouse (virtual joystick, deadzone 0.06, exponent 1.3) | Right stick (deadzone 0.15, exponent 1.6) | flight; `SetAimDirection` |
| Roll | A / D | Left stick X | flight |
| Strafe | Arrow keys | Left stick Y (vertical) | flight |
| Boost / Drift / Brake | Shift / Alt or C / Space | L3 / B / LT fully pressed | flight |
| Fire primary / secondary | LMB / RMB | RB / LB | `UTDWeaponComponent::SetTriggerHeld` |
| Power | Q tap = Signature1, hold 0.35 s = Signature2 | X | `OnPowerInput` delegate (T2 binds) |
| Ability | E | Y | `OnPowerInput(Ability)` |
| Switch next / prev / direct | Tab / Shift+Tab / 1-2-3 | D-pad right / left; hold up = radial (left Rift, up Lancer, right Bastion); tap up = next | `ATDSquadManager` |
| Target cycle | T / MMB | R3 | `UTDTargetingComponent::CycleTarget` |
| Hyperdrive (hold) | H | D-pad down | `OnHyperdriveInput(bHeld)` (IMC_Space only) |
| Squad command / Land | F | A | `IssueFocusCommand(current target)` in Space, `OnLandInput` in Atmosphere |
| Pause / Galaxy map | Esc / M | Start / Select | `OnPauseInput` / `OnGalaxyMapInput` |

`SetFlightContext(Space | Atmosphere)` swaps the IMC_Space and IMC_Atmosphere layers. `SetSpaceInputEnabled(false)` removes all space input and releases the triggers, for cutscenes and ground sections. Held fire buttons carry across a ship switch: the triggers are released on the old ship and re-pressed on the new one.

## How to try it

Tests:
```powershell
.\Tools\td-build.ps1
.\Tools\td-test.ps1 -Filter TriadDrift.Flight
```

By hand: run the sandbox command in ARCHITECTURE.md §4, which needs T5's `ATDSpaceGameMode` using `ATDSpacePlayerController`.

| Test | Proves |
|---|---|
| `Speed.ReachesCruiseAndMin` | cruise 6000, min 1500, throttle 0 = 3750, brake to min |
| `Boost.SpeedDrainAndRegen` | boost = cruise ×1.8; the 3 s meter empties on time; no regen before 1.2 s; full 2.5 s after that; needs `BoostMinToStart` |
| `Drift.KeepsVelocityAndBursts` | velocity direction held while the nose turns at the 1.5× rate (capped at 165°/s); release snaps to the nose with +20%; the burst bleeds back to cruise |
| `Rotation.TurnAndRollRateCaps` | yaw and pitch 110°/s, roll 180°/s, roll direction, auto-level, visual bank without frame roll |
| `Boundary.SteersBack` | warning band, out-of-bounds flag, steered and pushed back, overshoot bounded, disabled boundary is inert |
| `FrameRate.SameAt30And120` | the same scripted flight at 30, 60 and 120 fps lands within 5 cm; 37 fps is within 0.5% |
| `Robustness.BadInputAndTime` | zero, negative and NaN dt; NaN and infinite inputs; hitch clamp |
| `World.ShipFliesAndKeepsVelocityAcrossPossession` | in-world cruise, `Speed01`, `IsBoosting`, velocity survives possess and unpossess, impulse then grip recovery |
| `World.TeleportAndOverride` | teleport keeps speed along the new facing; speed override and recovery |
| `World.GlancingVsHeadOnCollision` | head-on bounces with ≥40 damage; a 15° scrape keeps more than 4000 cm/s and takes under 20 damage |
| `Input.RuntimeMappingContexts` | every ENG-02 key and pad binding exists in the right context; the Atmosphere layer has Land and no Hyperdrive; the player flies at cruise with no input |

## Known gaps
- **No hand-flown PIE session yet:** game feel (camera, mouse sensitivity, drift and boost numbers) is tuned on paper and proven by tests only. It needs a human play pass once T5's sandbox game mode lands.
- **Routing isn't tested with simulated key presses.** The tests check that the bindings exist; routing into T2 and T3 is plain calls in `Route*`.
- **Powers** only broadcast `OnPowerInput` for now. They get wired to T2's `UTDPowerComponent` when it lands.
- **`APawn::Restart` is overridden** on `ATDSpacecraft` so possession doesn't call `StopMovementImmediately`. This is deliberate, for ship switching.
- **Cockpit camera** (optional in ENG-01) isn't built.
- **No presentation calls yet.** Camera shakes and boost, drift and impact FX from `UTDShipVisuals` are left to whoever binds the delegates above (Nick's side or a presentation component).
- **Rebinding and invert options** are fields on the controller (`bInvertMousePitch`, `bInvertStickPitch`, sensitivities), but there's no settings UI.
