# Flight & input (ENG-01, ENG-02)

Owner: Teammate 1. Code: `Source/unrealGameJam/TriadDrift/Flight/`, `Source/unrealGameJam/TriadDrift/Input/`.
Hyperdrive, atmosphere and landing are in [travel.md](travel.md).

## What exists

| Class | What it does |
|---|---|
| `FTDFlightTuning` (`TDFlightTypes.h`) | Every flight number, including feel constants, as an editable field defaulting to the features.md ENG-01 value. Per-ship values (ENG-04) arrive through `ATDSpacecraft::ApplyFlightTuning`. |
| `TDFlightSim` (`TDFlightSim.h`) | The flight model as pure functions on plain data (`FTDFlightState`, `FTDFlightContext`), with no world. Each frame is split into equal sub-steps of at most 1/120 s, so 30, 60 and 120 fps give the same flight. |
| `UTDShipMovementComponent` | Runs the sim every tick, then sweeps the move. Resolves glancing and head-on collisions and applies world play bounds. Also handles impulses, teleports, speed overrides, the catch-up allowance and resets. |
| `ATDSpacecraft` | The flyable pawn shared by squad and enemy ships. See the component list below. |
| `ATDShip` | A squad ship: `ShipId`, Targeting (T2), Hyperdrive, Landing and AtmosphereEntry (travel.md), a `UTDShipVisuals` slot, and the chase and cockpit cameras. |
| `ATDSpacePlayerController` | Builds every input action and three mapping contexts in C++ (`IMC_SpaceCommon`, `IMC_Space`, `IMC_Atmosphere`). Flies the possessed ship and routes input to T2 (weapons, targeting, powers), T3 (switching, squad command) and travel (hyperdrive, landing, camera). |

`ATDSpacecraft` components and behaviour:
- a collision sphere on the `TDCollision::Ship` channel
- a placeholder hull: an engine cone plus cube wings, with a glowing engine block that stretches with boost
- Health (Core) and Weapons (T2)
- collision damage (`Environmental`) through `ApplyTDDamage`
- `TeleportSpacecraft`, which fires `OnTeleported` and the event bus
- `ResetFlightState` / `SetFlightParked`
- a visual bank on the hull

### Flight model (defaults)
- **Throttle:** follows the Core contract, so -1 is min speed (1500) and +1 is cruise (6000), linear in between.
  - The player controller flies at cruise with no input. S / LT slow down.
  - W / RT raise the top speed by up to +20% of cruise (`ThrottleUpBonus`, through the catch-up allowance below).
- **Aim-to-turn:** pitch and yaw rates follow the stick with response 12/s, hard-capped at 110°/s.
- **Roll:** 180°/s. With no roll input the ship auto-levels toward world up (gain 2.5, max 40°/s). Auto-level pauses while the nose is near vertical (|fwd.Z| > 0.9). Set `AutoLevelRate = 0` for pure 6DOF.
- **Auto-bank:** visual only. The hull mesh banks up to 40° into yaw.
- **Strafe:** 2500 cm/s on the arrow keys and left stick Y.
- **Boost:**
  - speed is `BoostSpeed` (default 10800 = cruise +80%)
  - the meter lasts 3 s, starts regenerating 1.2 s after boost ends, and refills in 2.5 s
  - a boost needs 10% of the meter to start
  - **an empty meter locks boost out until the button is released**, so there's no stutter-boosting
  - no boost while drifting or in atmosphere
- **Drift:**
  - while held, velocity keeps its direction and the nose turns at 1.5× (165°/s)
  - on release, velocity snaps to the nose
  - the +20% burst is granted only after 0.25 s of drift or 20° of nose swing (`DriftBurstMinSeconds` / `DriftBurstMinAngle`), and never above `BoostSpeed × 1.2`
  - anything above that ceiling, such as drifting out of a hyperdrive jump or a ram, drops at 30000 cm/s². The burst itself bleeds off at 2500 cm/s².
  - a speed override starting mid-drift ends the drift without a burst
  - a teleport keeps a held drift going along the new facing
- **Brake:** decelerates at 18000 cm/s² down to min speed. Space bar, or LT pressed past 92%.
- **Grip:** velocity re-aligns with the nose at 10/s, so knockback and scrapes slide a little and then recover.
- **Collisions:**
  - An impact counts as head-on when its "head-on-ness" (into-speed / speed) is at least 0.7.
  - Head-on hits bounce at 35% restitution and keep 50% of the speed. They deal 60 damage at cruise, scaled by impact speed.
  - Anything shallower is a scrape: it keeps 85% of the tangential speed, pops 400 cm/s off the surface and turns the nose 50% along the surface. A scrape deals up to 12 damage at cruise, scaled by angle and speed.
  - Damage has a 0.3 s cooldown.
  - Contact below 300 cm/s into the surface (`MinImpactSpeed`) is resolved silently: no event and no damage, so resting against a rock doesn't reset shield regen.
  - Friendly fire is off: bumping a squadmate does no damage.
- **Boundary:**
  - **World play bounds come from `UTDTravelSubsystem::SetPlayBounds(Center, Radius)`.** Every movement component picks them up on its next tick. They live in `FTDFlightContext`, so `ApplyFlightTuning` can't wipe them.
  - `SetBoundary` on a single component overrides them. With neither set, the radius is 15 km.
  - `IsNearBoundary` is the 500 m warning band; `IsOutOfBounds` is past the edge.
  - Outside, the nose is steered home at up to 220°/s, which is faster than any ship can turn, so the pilot can't out-steer it. There is also a push-back of up to 8000 cm/s². Both grow with depth.
- **Overrides:**
  - `SetSpeedOverride(Speed, SteeringScale)` is for the hyperdrive and the ram. It disables boost and drift.
  - `SetOverrideAcceleration` sets how fast the override speed is reached; the default is 60000 cm/s².
  - After `ClearSpeedOverride`, speed above boost speed drops at 30000 cm/s².
- **Catch-up allowance:** `SetThrottleTopSpeed(Speed)` / `ClearThrottleTopSpeed()` / `GetThrottleTopSpeed()`. While it's set, Throttle +1 flies at that speed with normal acceleration and no boost meter. T3's wingmen use it to hold formation; the player controller uses it for W.
- **Reset and park:**
  - `ResetFlightState(bKeepVelocity)` clears input, the meter (and its lockout), drift, boost, overrides, the allowance and the rates. It fires the matching end events.
  - `SetFlightParked(true)` stops simulation at zero velocity. Use it for downed ships and landing.

### Chase camera (`FTDChaseCameraTuning` on `ATDShip`)
- arm 1400 cm, height 280 cm
- rotation lag 8, location lag 12 (max lag distance 1800 cm)
- FOV 90 → 110 with `Speed01`
- **Roll follow is 30% of the bank, measured against a reference "up".**
  - The reference tracks world up at no more than 120°/s and holds while the nose is near vertical.
  - The follow eases back to 0 past 90° of bank.
  - So barrel rolls and loops never whip the camera; the tests check this.
- leans up to 7° into turns (look-ahead)
- `SnapCamera()` makes a hard cut. It runs automatically on switch-in and teleport.
- Cockpit camera: V toggles `SetCockpitView`. The placeholder hull is hidden from the pilot's own view.

### State for presentation and the HUD
On the spacecraft and its movement component:
- `GetSpeed01()` (min → boost), `IsBoosting()`, `IsDrifting()`, `GetBoost01()`
- `IsOutOfBounds()`, `IsNearBoundary()`
- `GetAngularRate()`, `GetVisualBank()`, `GetAltitudeMeters()`

Movement-component delegates:
- `OnBoostStarted` / `OnBoostEnded`
- `OnDriftStarted` / `OnDriftReleased(bBurst)`: every start pairs with exactly one end
- `OnFlightImpact`

On the controller:
- `GetVirtualStick()`: where the HUD draws the mouse reticle
- `IsUsingGamepad()`
- `IsSwitchRadialOpen()` / `GetRadialSelection()`
- `GetAimWorldDirection()`

**Placeholder look:**
- The hull is tinted with `UTDStatics::ShipColor` (squad) or `TeamColor` (others). The wings are a darker shade.
- The engine block glows (emissive ×8) and stretches with speed and boost.
- `ATDShip::SetShipVisuals(UTDShipVisuals*)` with a mesh replaces the hull and hides the placeholder parts. The mesh should be exported +X forward.
- `ATDSpacecraft::SetPlaceholderPartsVisible(false)` hides them for other art paths (T4's enemies).

### Input map (ENG-02)

| Action | KB/M | Gamepad | Routed to |
|---|---|---|---|
| Throttle | W / S | RT / LT | flight (W/RT = +20% top speed) |
| Aim | Mouse (virtual joystick, deadzone 0.06, exponent 1.3) | Right stick (deadzone 0.15, exponent 1.6) | flight; `SetAimDirection` |
| Roll | A / D | Left stick X | flight |
| Strafe | Arrow keys | Left stick Y (vertical) | flight |
| Boost / Drift / Brake | Shift / C / Space | L3 / B / LT fully pressed | flight |
| Fire primary / secondary | LMB / RMB | RB / LB | `UTDWeaponComponent::SetTriggerHeld` |
| Power | Q | X | `UTDPowerComponent::RoutePowerInput` press + release |
| Ability | E | Y | `RoutePowerInput(Ability)` press + release |
| Switch next / prev / direct | Tab / Shift+Tab / 1-2-3 | D-pad right / left; hold up = radial (left Rift, up Lancer, right Bastion); tap up = next | `ATDSquadManager` |
| Target cycle | T / MMB | R3 | `UTDTargetingComponent::CycleTarget` |
| Hyperdrive | H (hold to spool, press again to drop out) | D-pad down | `UTDHyperdriveComponent` (IMC_Space only) |
| Squad command / Land | F | A | `IssueFocusCommand` in Space; hold 0.5 s to land in Atmosphere |
| Camera | V | none | chase / cockpit |
| Pause / Galaxy map | Esc / M | Start / Select | `OnPauseInput` / `OnGalaxyMapInput`; these work while paused |

How the Q and X power buttons behave:
- **Ships without a Signature2 power** (Rift, Lancer): Signature1 fires on press, and its release edge is sent too.
- **Two-power ships** (Bastion):
  - tap = Signature1 (press + release on key-up)
  - holding past 0.5 s (`PowerHoldSeconds`) = Signature2 press at the threshold, then its release on key-up
- Every power press is also broadcast on `OnPowerInput`.

Other controller behaviour:
- LeftAlt isn't bound, because Alt+Tab would leave the game.
- **Aim device:** moving the gamepad stick takes over aim and recentres the mouse reticle. The mouse takes aim back only after a real movement (3 or more counts).
- **Contexts:** `SetFlightContext(Space | Atmosphere)` swaps the IMC_Space and IMC_Atmosphere layers.
- **Disabling:** `SetSpaceInputEnabled(false)` releases held buttons and pushes a neutral cruise input once, then leaves the ship's input alone for cutscenes and autopilots.
- **Ship switches:** held fire buttons carry across. The triggers are released on the old ship and re-pressed on the new one. The W top-speed bonus is cleared from the old ship.

## How to try it

Tests:
```powershell
.\Tools\td-build.ps1
.\Tools\td-test.ps1 -Filter TriadDrift.Flight
```

By hand:
```powershell
.\Tools\td-play.ps1
```
To capture screenshots:
```powershell
.\Tools\td-shot.ps1 -Shots "4,9"
```

| Test | Proves |
|---|---|
| `Speed.ReachesCruiseAndMin` | cruise 6000, min 1500, throttle 0 = 3750, brake, catch-up allowance |
| `Boost.SpeedDrainAndRegen` / `Boost.EmptyMeterLockout` | ×1.8 speed, 3 s meter, 1.2 s regen delay, 2.5 s refill; holding through empty boosts once, no stutter |
| `Drift.KeepsVelocityAndBursts` / `Drift.TapSpamAndBurstCeiling` / `Drift.OverrideInteractions` | direction hold, 1.5× nose, +20% when earned; tap-spam can't pump speed; repeated bursts stay ≤ boost×1.2; drifting out of an 80k jump bleeds down; an override mid-drift gives no burst |
| `Rotation.TurnAndRollRateCaps` | 110 / 110 / 180°/s, roll direction, auto-level, visual bank |
| `Strafe.LateralAndVertical` | 2500 cm/s on both axes |
| `Boundary.SteersBack` / `Boundary.CannotBeOutSteered` / `Boundary.SurvivesApplyFlightTuning` | warn band, steer and push; a pilot fighting it (aim out, boost, drift) is held within 400 m; bounds survive a later tuning |
| `FrameRate.SameAt30And120` | the same scripted flight at 30, 60 and 120 fps is within 5 cm; 37 fps is within 0.5% |
| `Robustness.BadInputAndTime` | zero, negative and NaN dt; NaN and infinite inputs; hitch clamp |
| `World.ShipFliesAndKeepsVelocityAcrossPossession` | in-world cruise, velocity survives possess and unpossess (`APawn::Restart` override), impulses |
| `World.TeleportAndOverride` | teleport keeps speed along the new facing; override and recovery |
| `World.GlancingVsHeadOnCollision` / `World.CollisionDamageAngleCooldownAndEvents` | bounce vs scrape; damage grows with angle (20° < 40° < 90°, head-on ≈ 60); cooldown bounds pinned contact; no events below `MinImpactSpeed` |
| `World.BoostDriftEventsBalanced` | one start per end; teleport keeps a drift; `ResetFlightState` and `SetFlightParked` |
| `Camera.LagFovRollContinuitySnap` | lag 8/12, FOV 90 → 110, roll follow ≈30% (peaks at 90° of bank), no whip through a barrel roll or a loop, snap on switch and teleport |
| `Input.RuntimeMappingContexts` | every ENG-02 binding in the right context |
| `Input.InjectedActionsAndPowerRouting` | actions injected through `UEnhancedPlayerInput` reach the ship; stick shaping; LT brake threshold; W bonus; device-switch hysteresis; Q tap/hold against real power components; E ability; disabled input leaves scripted input alone; Pause works while paused |

## Known gaps
- **Needs a hand check:**
  - Shift+Tab, since headless tests have no key state for `IsInputKeyDown`.
  - The overall feel (mouse sensitivity, camera, drift and boost numbers). The tests prove the numbers, not the fun.
- **Mapping contexts** are added through the local-player subsystem, which headless tests don't have. The injection tests drive actions directly.
- **No presentation calls yet.** Camera shakes and boost, drift and impact FX from `UTDShipVisuals` are left to whoever binds the delegates above.
- **Rebinding and invert options** are fields on the controller (`bInvertMousePitch`, `bInvertStickPitch`, sensitivities), but there's no settings UI.
