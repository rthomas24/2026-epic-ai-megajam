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
- **Strafe:** 4000 cm/s on the arrow keys and left stick Y. It's for fine positioning; the dodge is the evasive move.
- **Dodge (player only, Lead decision Nov 12):** a lateral burst of `DodgeSpeed` 100 m/s for `DodgeSeconds` 0.25 s,
  so the hull moves about 25 m sideways. `DodgeCooldown` is 1.5 s, and it works while boosting and drifting.
  - No invulnerability: it dodges by moving the hull.
  - Refused during the ram or any speed override, landing, atmosphere entry, hyperdrive spool and jump, a director
    slow-motion shot, the cooldown, or when the ship isn't player-flown (wingmen and enemies never dodge).
  - API: `ATDShip::TryDodge(±1)`, `UTDShipMovementComponent::RequestDodge` / `CanDodge` / `IsDodging` /
    `GetDodgeReady01` (HUD pip), `OnDodge(Direction)`, and the cue request `"Dodge"`.
  - Measured against three real fighters (`Flight.Feel.EvasiveFlyingReducesHits`, 6 seeds, fully deterministic
    with T4's spawn seeds), against flying straight:
    - weaving alone: 51%
    - weaving with well-timed dodges: 25%, which meets the Lead's 35% target and is asserted
    - drifting: 121%, observation only
    - strafing: 144%, observation only
  - At the 60 m/s first spec the dodge gave no measurable gain, so `DodgeSpeed` is 100 m/s; a sweep is in
    `TDExplore.Flight.DodgeSweep`.
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

- **Atmosphere ground floor:** in atmosphere the movement component traces the static ground below the hull.
  Inside `GroundAvoidHeight` (30 m) the sink is cancelled and the nose eases toward level
  (`GroundPullUpRate` 120°/s, scaled by depth). A hands-off or diving ship skims the ground and never grinds
  into it.

### Chase camera (`FTDChaseCameraTuning` on `ATDShip`)
- arm 1400 cm, height 280 cm
- rotation lag 8, location lag 18 (max lag distance 1000 cm); flight-feel pass below
- FOV 90 → 110 with `Speed01`
- **Roll follow is 30% of the bank, measured against a reference "up".**
  - The reference tracks world up at no more than 120°/s and holds while the nose is near vertical.
  - The follow eases back to 0 past 90° of bank.
  - So barrel rolls and loops never whip the camera; the tests check this.
- leans up to 7° into yaw turns (look-ahead), and 20% of that (1.4°) into pitch (`LookAheadPitchScale`)
- `SnapCamera()` makes a hard cut. It runs automatically on switch-in and teleport.
- Cockpit camera: V toggles `SetCockpitView`. The placeholder hull is hidden from the pilot's own view.
- Motion blur is off in space by default (`MotionBlurAmount` / `MotionBlurMax` = 0), because any
  camera-rotation blur turns stars into dashes.

### Flight-feel pass (Nov 13, T1)
How it was judged: a 60 s chase-cam run per ship (cruise, boost, a hard right turn, a boosted hard left turn,
a drifting turn, a full pull-up, barrel roll, then a switch cut), offscreen 1920x1080 on `L_Galaxy`.
- Captures were taken as fast as the MCP allows (about one every 0.8 s).
- `TD.Flight.Cam rec 60` logged the camera numbers every 0.1 s.
- The same script ran with the old values and the new values in one session.
- Before/after strips: `.wt/t1/Saved/TDShots/feel_sheets/strip_*.png` (local, not in git).

| Measured | Old (lag 12 / max 1800 / pitch lead 7°) | New (lag 18 / max 1000 / pitch lead 1.4°) |
|---|---|---|
| Ship centre in a 110-120°/s pull-up (screen height, 0 = top) | 0.71-0.73: on the HUD cards | 0.64-0.65: above the HUD band |
| Camera distance at cruise (Rift / Lancer / Bastion, cm) | 1900 / 2160 / 1870 | 1780 / 1880 / 1710 |
| Camera distance at boost, peak (cm) | 2390 / 2730 / 2170: the Lancer shrank to a speck | 2010 / 2210 / 1880 |
| Furthest off-centre in a turn (x, 0.5 = centre) | 0.68 for the Lancer: it left the screen during a high-speed drift spin | 0.26 |

Kept as they were:
- FOV 90 → 110: the speed read is good.
- Boost still pulls the camera back about 2 m.
- Yaw look-ahead of 7°: the ship sits 10-14% off-centre on hard turns, which reads as leaning in.
- Roll follow 30%: about 12° of horizon tilt in a banked turn, and up to about 27° in a barrel roll. That is lively but readable.
- Dust density: all 120 motes at every speed above the minimum.
- Switch cut: a hard cut. The new ship is framed on the first frame, with no lag swing.

Open points:
- A long drift spin at boost speed (150°/s) still swings the ship to about a quarter of the screen width, and briefly into the HUD band. Fixing it properly means aiming the camera along the velocity while drifting, which moves the aim reticle, so it is a design call, not a tuning value.
- In the test script, `TD.KillAll` right after a switch triggers the director's multi-kill shot (FOV 65, long lens). That made two switch frames look like a broken cut. It is a script artefact, not the switch.

Dev tool: `TD.Flight.Cam [tag]` prints what the player sees: ship screen position, FOV, camera and ship roll, the camera's
offset in ship space, speed, boost and drift, turn rates, and dust.
- `TD.Flight.Cam rec <s> [tag]` logs it every 0.1 s.
- `TD.Flight.Cam set <Cam|FB> <Field> <Value>` changes a chase-camera or feedback tuning value on every squad ship live, so you can tune without rebuilding.
- It is not in Shipping builds.

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
- `GetVirtualStick()`: where the HUD draws the mouse reticle. It recentres at `MouseRecenterRate` (3/s) when
  the mouse is still, and resets to centre on a ship switch, so the new ship starts straight.
- `IsUsingGamepad()`
- `IsSwitchRadialOpen()` / `GetRadialSelection()`
- `GetAimWorldDirection()`

**Placeholder look:**
- The hull is tinted with `UTDStatics::ShipColor` (squad) or `TeamColor` (others). The wings are a darker shade.
- The engine block glows (emissive ×8) and stretches with speed and boost.
- `ATDShip::SetShipVisuals(UTDShipVisuals*)` with a mesh replaces the hull and hides the placeholder parts. The mesh should be exported +X forward.
- `ATDSpacecraft::SetPlaceholderPartsVisible(false)` hides them for other art paths (T4's enemies).

**Presentation (`UTDShipVisuals`, Nick's `/Game/TriadDrift/Presentation/DA_<Ship>_Visuals`; T3 assigns it through `ATDShip::SetShipVisuals`):**
- **Assembly:** `Mesh` alone, `Mesh` + `Parts`, or `Parts` only.
  - Each part is a visual-only `UStaticMeshComponent`: no collision, no tick.
  - A part attaches to `ParentSocket` on the primary mesh (the hull mesh, or the first part when there are only parts), or to the ship root.
  - `FindVisualPart(Name)` returns a part for gameplay hooks.
- **Placeholders** (hull cone, wings, engine block) hide as soon as any mesh or part is set.
- **Teardown:** `SetShipVisuals(nullptr)` or a new asset tears the parts down and restores the placeholder. Projectiles still only hit the hull sphere.
- **Cues:**
  - `EngineTrail` runs as a loop while visuals are assigned.
  - `BoostStart` plays at boost start, with `BoostLoop` looping while boosting.
  - `DriftLoop` loops while drifting; `DriftRelease` plays on an earned burst.
  - `CollisionScrape` plays on glancing impacts.
  - Engine cues attach to a part named `Engine`, else the hull's `Engine` socket, else the hull.
  - `GetCueRequestCount(Slot)` exposes the requests for tests.
- **Engine audio parameters (frozen contract, Nov 12).** Every frame, both engine loops (the
  `UTDShipVisuals::EngineLoop` sound and the `EngineTrail` cue's sound) receive three float parameters. Player
  ship and wingmen alike, surviving switches and respawns:

  | Parameter | Range | Meaning |
  |---|---|---|
  | `Speed01` | 0..1 | `GetSpeed01()`: min speed → boost speed |
  | `Boost01` | 0..1 | 1 while boosting, eased over 0.2 s |
  | `Drift01` | 0..1 | 1 while drifting, eased over 0.2 s |

  A MetaSound source reads these as float inputs with exactly these names. Any other sound (a plain
  `USoundWave` / cue) is also pitched by speed: `0.8 + 0.6 × Speed01`. A MetaSound is not, so it is never
  pitched twice. Names are constants on `ATDShip` (`AudioParamSpeed01` / `Boost01` / `Drift01`). Test:
  `TriadDrift.Flight.Visuals.EngineLoopAudioParameters`.

### Input map (ENG-02)

| Action | KB/M | Gamepad | Routed to |
|---|---|---|---|
| Throttle | W / S | RT / LT | flight (W/RT = +20% top speed) |
| Aim | Mouse (virtual joystick, deadzone 0.06, exponent 1.3) | Right stick (deadzone 0.15, exponent 1.6) | flight; `SetAimDirection` |
| Roll | A / D | Left stick X | flight |
| Strafe | Arrow keys | Left stick Y (vertical) | flight |
| Dodge | LeftAlt (toward the held roll A / D, else the steering side, else right), or double-tap A / D within 0.3 s | double flick of the left stick X (both flicks past 85%) | `ATDShip::TryDodge` |
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
- **On foot:** while the possessed pawn isn't a spacecraft (T5's first-person character), all three space mapping contexts are removed, and they come back on re-boarding.
- **Disabling:** `SetSpaceInputEnabled(false)` releases held buttons and pushes a neutral cruise input once, then leaves the ship's input alone for cutscenes and autopilots.
- **Ship switches:** held fire buttons carry across. The triggers are released on the old ship and re-pressed on the new one. The W top-speed bonus is cleared from the old ship.

## Feedback with no art (`UTDFlightFeedbackComponent` on `ATDShip`)

Nick's cue slots (`UTDShipVisuals`, `UTDTravelFX`) may stay empty, and flight must still read and feel good without them. So for
the player's ship, every flight and travel moment has a cheap placeholder that steps aside by itself when art is assigned:
- **Camera placeholder** (a kick: roll / pitch / FOV punch with a fast attack and an ease-out, plus procedural sine shake; no
  camera-shake asset). It only plays while the cue's `Shake` is empty.
- **Glow placeholder** (short bursts of the Core placeholder glow in the squad colour on the hull). It only shows while the
  cue's `VFX` is empty.
- `TD.Flight.Placeholders 0` turns both off. `Tuning.CameraScale` scales every kick.

The audit below says what the player saw and felt with empty cues before this change (main 46422f9), and what they get now.
Captures are in `Saved/TDShots/feedback/` (live game on port 8011, Nov 12).

| Moment (cue slot) | Before, with empty cues | Placeholder now | Capture |
|---|---|---|---|
| Dodge (`Dodge`) | the hull jumps sideways; nothing else | thruster burst in squad colour on the side **away** from the move, white core; camera rolls 6° into the move, FOV +4, small shake (0.45 s) | `03_dodge_burst`, `04_dodge_left` |
| Boost start (`BoostStart`, `BoostLoop`) | FOV eases 90 → 110 with speed; the placeholder engine glow stretches (placeholder hull only) | FOV punch +6 and shake; twin flares beside the engine (kept off the crosshair); the dust streaks lengthen | `02_boost_dust` |
| Drift (`DriftLoop`) / release (`DriftRelease`) | the ship slides; nothing on release | 0.12° rumble while drifting; on an earned burst, twin flares, FOV +5, a dip | `06_drift_release` |
| Brake (no slot) | FOV eases back with speed | retro flares at the nose, view dips 1.5° and FOV −3 | `05_brake` |
| Hyperdrive spool (`HyperdriveSpool`) | nothing but the HUD charge bar | the view tightens (FOV −7) and shakes harder as the charge builds | `07_spool` |
| Jump (`HyperdriveTunnel`) | FOV at max (speed) | FOV +12 punch, held +10 in the tunnel; side flares; dust becomes a streaking tunnel | `08_jump` |
| Drop-out (`HyperdriveDropOut`) | a cut to the new spot | FOV −8 snap, 1.2° shake, a flash | (log: `HyperdriveDropOut=1`) |
| Portal gate / Rift portal transit (`PortalTransit`) | a camera snap only | FOV +12, roll flick, shake, a violet flash. It fires only when the ship left from inside a gate's or portal's bounds, never for arrivals, squad placement, retakes or dev teleports | `20`–`25_gate_*`: a live transit of Gate_Verdance_Zone both ways (A→B and back), `PortalTransit` counted each time, wingmen on screen in formation after each (`TD.Squad.Frame`) |
| Atmosphere entry (`AtmosphericEntry`) | nothing until the map loads | heat shake and FOV swell on a bell over the entry, an orange re-entry glow on the nose | `10_entry` shows the entry haze only; the glow flashes fired (log `AtmosphereEntry=9`) but none was in frame at capture |
| Landing touchdown (`LandingDust`) | the ship settles | 2° dip, shake, a flat dust disc under the hull | `11_touchdown` |
| Take-off (`TakeOff`) | the ship rises | lift shake, FOV +3, a flat squad-colour thrust disc under the hull | `12_takeoff` |

**Sense of speed (near-field dust).** Cruise read as slow because the stars are effectively at infinity: there was no
parallax. Now 120 small glow motes sit still in the world inside a 90 m box that travels 25 m ahead of the ship and wraps
them around. Each mote is drawn as a streak along the velocity, `speed × 0.02 s` long (max 22 m). Thickness ramps in from
15 m/s to 200 m/s, and motes within 12 m of the camera shrink away, so none smears across the view. Only the player's flown
ship draws dust, and none when landed. `TD.Flight.SpeedDust 0/1` toggles it.
- **Frame cost, measured live (Nov 12):**
  - Game thread: about 0.006–0.012 ms per frame for the whole component (`TD.Flight.Feedback`), against 0.005 ms with dust off.
  - Frame time: no measurable difference, averaged over six alternating samples (dust on 4.6 ms, off 4.8 ms).
  - Render: one instanced draw of 120 cubes. Instances update incrementally; the first version re-created the render state
    every frame and cost about 3 ms of frame time, which is why `bMarkRenderStateDirty` is false.

Readout: `TD.Flight.Feedback` logs the tick cost, the frame time, the motes shown, the active bursts, the current camera kick
and how many times each placeholder fired.

| Test | Proves |
|---|---|
| `Feedback.DodgeBurstAndKickWithNoArt` | empty Dodge cue: a burst glows, the camera rolls into the move (and the other way for a left dodge), FOV punch, all gone after 0.8 s |
| `Feedback.ArtReplacesThePlaceholder` | a cue with VFX gets no glow; a cue with a Shake gets no placeholder kick (in the ship, too) |
| `Feedback.BoostBrakeHyperdriveRead` | boost start and brake fire once with a FOV punch / dip; the spool tightens the view; the jump kicks |
| `Feedback.PortalTransitKickOnlyFromAPortal` | a placement away from portals gives no kick; a transit out of a gate does |
| `Feedback.SpeedDustOnlyForThePlayerAtSpeed` | 120 motes for the flown ship, none for wingmen or when not flown; tick under 0.3 ms |

## First-minute control hints (`UTDHintComponent` on `ATDSpacePlayerController`)

The hints teach one control at a time, each finished by doing it. The text goes through `ITDHUDData::GetInteractPrompt`
(`ATDSquadManager` returns `UTDHintComponent::GetHintText` last, after the land prompt and the switch refusal; T3 wires it
in once 8aa5c70 is on main).
- **Core, from wave 1** (first hint 2 s after `OnWaveStarted(1)`, 1.2 s between hints):
  1. aim/throttle (1 s of steering)
  2. fire (LMB / RB)
  3. boost
  4. dodge
  5. switch ship (`OnShipSwitched`)
  6. signature power (Q / X)
  7. target cycle
- **Later, when they first apply:**
  - drift: T4's `Tip_Drift`, or wave 2+
  - hyperdrive: `Tip_Hyperdrive`, the first planet marker; a spool completes it
  - each unlocked ability: `OnAbilityUnlocked`, named; E / Y completes it
  - landing already has its own prompt.
- **Rules:**
  - Anything the player already did is never shown, so an experienced pilot sees nothing.
  - A hint ignored for 20 s counts as seen.
  - Each hint shows once per profile: slot `TD_Hints`, separate from the run save; tests never touch it.
  - Nothing shows (and the clock stops) while paused, on foot, landing or with the land prompt up, during atmosphere entry,
    or with the hyperdrive running.
  - Wording follows the last input device.
  - Off with `SetHintsEnabled(false)` (a menu setting, saved) or `TD.Hints 0`. `TD.Hints.Reset` and `TD.Hints.Status` are
    dev commands.
- **Wave-1 grace (T4):** enemies deal ×0.5 for the first 15 s of wave 1 whatever the player does, so fire, boost and
  dodge can be tried safely.
- **Tests:** `Input.Hints.CoreSequenceCompletesByDoingIt` (real keys and mouse through the squad: every core step),
  `Input.Hints.LaterHintsWhenTheyApply` (experienced pilot sees none; drift, hyperdrive and ability by real input),
  `Input.Hints.OncePriorityTimeoutDeviceAndSetting`.

## Player options (`UTDSettingsSubsystem`, a game-instance subsystem)

There is one object for every menu: Get Game Instance → Get Subsystem (TDSettingsSubsystem), or
`UTDSettingsSubsystem::GetTDSettings(this)` in C++. Each setter applies at once and saves to slot `TD_Settings`
(separate from the run save and the hint profile).

| Option | Call | Range | Takes effect |
|---|---|---|---|
| Mouse sensitivity | `SetMouseSensitivity` | ×0.25–3 of the class default | `MouseSensitivity` on every space controller |
| Invert mouse Y | `SetInvertMouseY` | bool | `bInvertMousePitch` |
| Gamepad sensitivity | `SetGamepadSensitivity` | ×0.25–3 | shaped right-stick aim × this, clamped to full deflection |
| Gamepad dead zone | `SetGamepadDeadzone` | 0–0.5 (default 0.15) | `StickDeadzone` |
| Invert gamepad Y | `SetInvertGamepadY` | bool | `bInvertStickPitch` |
| Hints | `SetHintsEnabled` | bool | `UTDHintComponent` |
| Master volume | `SetMasterVolume` | 0–1 | the main audio device's transient primary volume |
| Music / effects volume | `SetMusicVolume` / `SetEffectsVolume` | 0–1 | a runtime sound mix over `SC_TD_Music` / `SC_TD_SFX`, once Nick's classes exist (kept until then) |
| Window mode / resolution | `SetWindowMode` / `SetResolution` (0,0 = desktop), `GetSupportedResolutions` | | `UGameUserSettings`, saved |

- **Start-up:** controls apply when each `ATDSpacePlayerController` begins play, and audio applies when the game instance
  starts.
- **Video at start-up:** video applies only once the player has chosen it (`bVideoChosen`). A first start keeps the engine's
  window, and `-windowed` / `-ResX` on the command line always win at start-up. The first version forced windowed
  fullscreen on every start and broke agent sessions launched with `-ResX`.
- **Never touched:** the editor window, headless runs and tests (tests use their own slot).
- **Other calls:** `ResetToDefaults`, `ApplyAll(FTDPlayerSettings)`.
- **Dev command:** `TD.Settings [key value]`, with keys MouseSens, InvertMouse, PadSens, PadDeadzone, InvertPad, Hints,
  Master, Music, Effects, `Window 0-2` and `Res W H`.
- **Played live (Nov 12, -game):** windowed 1600×900 and 1280×720 resized the window, and windowed fullscreen filled the
  desktop. T5 builds the OPTIONS page.
- **Tests:** `Input.Settings.PersistAndApplyAtStartup`, `Input.Settings.EachControlTakesEffect` (mouse and stick aim
  through the controller's handlers: sensitivity, invert, dead zone).

## Shipping builds (what `!UE_BUILD_SHIPPING` removes in Flight / Input / Travel)

Audit of Nov 12. The Shipping game target (`unrealGameJam Win64 Shipping`) compiles. Only these are compiled out, and
none of them is needed by a player:
- `Flight/TDFlightDebug.cpp`: the `TD.Flight.Threats` and `TD.Flight.Feedback` readouts.
- `Flight/TDShipMovementComponent.cpp`: the silent-move warning log and callstack. The counting itself stays.
- `Flight/TDSpacecraft.cpp`: the long-teleport log and callstack.
- `Input/TDInputDebug.cpp`: the `TD.Input.*` agent commands. `TDActivateInputToolset` has an empty Shipping body, so the
  controller still links.
- `Travel/TDTravelDebug.cpp`: the `TD.Travel.*` commands.

Gameplay code in this lane has no `check()` / `verify()` with side effects, no `WITH_EDITOR` paths and no editor-only
calls. The hints, the no-art feedback, the options, the approach assist and every prompt are compiled in every
configuration.

## On-foot camera setting and the Settings page (ENG-32, engineering side)

- **Camera setting:** `UTDSettingsSubsystem` holds `OnFootCameraMode` (`ETDOnFootCamera` FirstPerson / ThirdPerson,
  default first person), saved with the other options.
  - `GetOnFootCameraMode`, `SetOnFootCameraMode` and `ToggleOnFootCameraMode` are Blueprint-callable.
  - `OnOnFootCameraModeChanged` fires on every real change, including from Reset and ApplyAll.
  - The ground character (T5) binds it, reads the mode at BeginPlay, and applies it once a director shot or the
    death cam ends.
- **Keys:** `IMC_OnFoot` exists only while the possessed pawn isn't a spacecraft. In it, V and Gamepad_Special_Left
  (View) toggle the mode. In space, V stays the cockpit camera. `IMC_Global` (always on) maps O and pad Y to open
  the Settings page, but only while the game is paused.
- **Settings page:** `UTDSettingsSubsystem::OpenSettingsPage(this)`, plus `CloseSettingsPage` and
  `IsSettingsPageOpen`.
  - If `UTDUIStyle::SettingsWidgetClass` is set in DA_UI_Style, that widget opens instead. It is read by name, so it
    works however Core declares it.
  - Otherwise the code-built `STDSettingsPage` (Input/TDSettingsPage.h) opens. Its rows: on-foot camera, mouse
    sensitivity, invert mouse Y, gamepad sensitivity, gamepad dead zone, invert gamepad Y, control hints, master /
    music / effects volume, window mode, resolution, Reset to defaults, Back.
  - Keys: Up / Down (W / S, D-pad, left stick) select; Left / Right (A / D, D-pad, stick) change; Enter / Space / pad A
    activate; Esc / Backspace / pad B close. The mouse clicks rows and their < > arrows.
  - Every change applies at once and is saved.
  - The page keeps keyboard and pad focus while it is open and closes on any map change. Esc through the game's
    pause input closes the page before it unpauses.
- **Hooks in other files:**
  - Main menu (T5, `ATDMenuGameMode`): a SETTINGS button between CONTINUE and QUIT, plus `KeepMenuFocus` leaves the
    page alone.
  - Pause line (Lead, TDDebugHUD): "O / Y SETTINGS".
- **Tests:** `Input.SettingsPage.CameraModePersistsAndBroadcasts`,
  `Input.SettingsPage.NavigatesWithKeysAndPadAndEveryRowChanges`, `Input.SettingsPage.SettingsWidgetClassOverride`,
  `Input.SettingsPage.VAndViewToggleOnFootOnly` (real keys: V and View toggle on foot, never in space, and not again
  after re-boarding).
- **Played (Nov 13, about 00:30, agent):**
  - Pause, O, the page opened; keys changed rows (camera, mouse, master); Reset worked; the page closed.
  - On Verdance, V and View toggled the mode (log).
  - No screenshots: the display was off overnight, so the viewport had no drawable area.

## Landing pad art

`ATDLandingPad::PadArt` (soft) defaults to `/Game/TriadDrift/Gameplay/Props/Planet/SM_LandingPad`. While the asset is
missing, the cylinder shows.
- **Footprint:** a disc 1000 cm across (500 cm radius), centred on the actor, with its walkable face on top (+Z) at
  `DeckHeight` = 40 cm above the actor origin. Pivot at the bottom centre; +X is the landing facing.
- **Collision is unchanged:** the cylinder keeps colliding (hidden) and the art mesh never collides, so the art's top
  must sit at 40 cm.
- **Cooking:** pads are spawned at run time by the planet rig, so nothing on a map references the mesh. The folder
  must be in the cook list (Lead) or the packaged game shows the cylinder.
- **Test:** `Travel.Landing.PadArtSwapsTheLookNotTheCollision`.

## Combat music director (N-029, `UTDMusicDirectorSubsystem`, a world subsystem in Input/)

The director plays `UTDUIStyle::CombatMusic` (Nick's slot, read by name) through one 2D UI-sound audio component per
map, using the sound's own class and volume. The options' Music slider reaches it through `SC_MenuTheme_Music`
(or `SC_TD_Music` once that exists).
- **When it plays:** while a fight is on, meaning any of `FTDAudioState::bWaveActive`, the DominionAssault phase, or
  any `ATDEncounterVolume::IsActive()`.
- **Fades:** a 2 s fade in. When the fight ends, a 6 s hold at full volume, then a 4 s fade out. A new fight during the
  hold carries on; during the fade out the music swells back up without restarting.
- **1 s stop:** on a level travel (any flow travel, quit to menu included), on the Victory phase and on a CUT. After a
  CUT it stays silent until the retake starts (`OnTakeStarted`), then plays from the top.
- **Never plays** with an empty slot or on the menu map. The component belongs to the map, so the menu's own music
  can never overlap it.
- **Why a world subsystem:** it ticks with the world (also when paused, in real seconds), its one component dies with
  the map, and tests drive it with a test world.
- **Tests (test sound):** `Input.Music.FadeInHoldAndFadeOut`, `Input.Music.NewFightInHoldOrFadeCarriesOn`,
  `Input.Music.OneSecondStopOnCutTravelVictory`, `Input.Music.TriggersAndEmptySlot` (real Dominion phase and a real
  encounter volume).
- **Not yet:** heard in the game. The track isn't in the slot on this branch, and nobody has listened; Nicholas
  approves by ear.

## Placeholder engine trail

Every squad ship draws a short ribbon of glow segments in its squad colour behind the exhaust
(`UTDFlightFeedbackComponent`, `Tuning.TrailSegments` = 10 samples, 0.03 s apart).
- The segments follow the exhaust's actual path, so the ribbon bends through turns. It tapers from 55 cm to nothing,
  and boost makes it 40 % thicker and longer (it is speed × 0.3 s long: about 18 m at cruise).
- It never draws across a teleport, and it hides when the ship is hidden, parked, landed with engines off, or when
  `UTDShipVisuals::EngineTrail` has a VFX.
- Exhaust point: the art `Engine` socket or part, else the rear of the hull or the placeholder glow
  (`ATDShip::GetEngineExhaustLocation`).
- **Test:** `Flight.Feedback.EngineTrailRibbonWithNoArt` (19 m at 60 m/s, longer on boost, no streak across a
  400 m jump).
- **Seen live:** only from the chase camera, where the player's own trail is end-on and hidden behind the hull. The
  side view is for T3's intro capture.

Settings page captures (offscreen, Nov 13): `Saved/TDShots/settings/{720p,1080p}_0N_*.png`.

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
| `Strafe.LateralAndVertical` | `StrafeSpeed` (4000 cm/s) on both axes |
| `Review.DodgeMovesTheHullAndRespectsGates` | ~speed × time sideways at 30/60/120 fps, also boosting and drifting; no invulnerability; cooldown; refused for wingmen, the ram, hyperdrive spool and slow motion |
| `Feel.EvasiveFlyingReducesHits` | three real fighters, 6 seeds, deterministic: weave + timed dodge ≤ 35% of straight-line damage (asserted); weave, drift and strafe printed only |
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
- **No-art feedback:** the placeholders are "technically verified" only; Nicholas decides whether they stay as they are until
  art lands. The gate captures were taken about 0.5 s after the transit, so the 0.6 s kick has mostly decayed in them.
- **Gate arrival block (for testers):** `TD.World.Goto Gate` puts the ship 91 m from the gate at speed, inside the gate's 5 s
  arrival block, so it passes straight through without a transit. Fly away and come back, as `gate_live2.py` does. In play,
  the return points sit 700 m from the gates (T4), about 11 s at cruise, so a player never meets the block.
- **Needs a hand check:**
  - Shift+Tab, since headless tests have no key state for `IsInputKeyDown`.
  - The overall feel (mouse sensitivity, camera, drift and boost numbers). The tests prove the numbers, not the fun.
- **Mapping contexts** are added through the local-player subsystem, which headless tests don't have. The injection tests drive actions directly.
- **No presentation calls yet.** Camera shakes and boost, drift and impact FX from `UTDShipVisuals` are left to whoever binds the delegates above.
- **Rebinding and invert options** are fields on the controller (`bInvertMousePitch`, `bInvertStickPitch`, sensitivities), but there's no settings UI.
