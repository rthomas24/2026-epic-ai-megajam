# Squad & wingmen (Teammate 3)

ENG-04 (ship stats), ENG-07 (squad manager & instant switching), ENG-08 (wingman AI), plus the
ENG-03 death/respawn rules for squad ships. Code: `Source/unrealGameJam/TriadDrift/Squad/`.

## What exists

| File | What it does |
|---|---|
| `TDShipLibrary.h/.cpp` | `FTDShipSpec` + `UTDShipLibrary::Get(ETDShipId)`: the ENG-04 table (hull/shield, min/cruise/boost, turn/roll, boost meter and regen, weapon ids, power name, unlockable, squad colour, ship/pilot/role names for the C-03 #1 name card). `ApplySpecToShip` pushes a spec onto an `ATDShip`. |
| `TDSquadManager.h/.cpp` | `ATDSquadManager` (implements `ITDHUDData`). Spawns and tracks the three ships, owns who the player flies, death → tumble → auto-switch → respawn, squad wipe, Switch Strike, focus command, formation slots. |
| `TDWingmanAIController.h/.cpp` | `ATDWingmanAIController`: Formation → Engage → Evade → Regroup → Downed. Flies only through `SetFlightInput` and pulls only the primary/secondary triggers. |
| `TDSquadIntegration.h/.cpp` | **The only file that calls other areas** (Core health/bus/registry/flow, T1 flight, T2 weapons/targeting/projectiles, T4 wave info). A cross-area signature change is a one-file fix. |
| `Tests/TDSquadTests.cpp` | `TriadDrift.Squad.*` automation tests (below). `Tests/TDSquadTestListener.h` records bus events. |

### Rules as built (all tunable `EditAnywhere` on `ATDSquadManager` / `ATDWingmanAIController`)

- **Switching** (`RequestSwitchTo/Next/Prev`): same call → the wingman releases the target, the
  player controller possesses it, the old ship goes to its wingman. Both ships' velocities are
  captured and re-applied (`SetVelocityImmediate`), so nothing stalls. `SwitchCooldown` 0.4 s;
  downed ships are skipped; next/prev wrap. The new ship gets `SwitchInInvulnerability` 2 s
  (ENG-03, M4): an auto-switch (death) grants 2 s; a manual switch 0.35 s, re-armed per ship at most
  every 6 s, so Tab spam can't farm invulnerability. Cooldown runs on real time (a money shot never
  eats Tab presses); a press during the cooldown is buffered and runs when it ends (double-tap
  reaches the third ship). A manual switch is refused (with `OnSwitchRefused(Reason)` and
  `GetLastRefusalText()` for the HUD) while T1's `ATDShip::IsSwitchLocked` says so (hyperdrive not
  Idle, landing, atmospheric entry). The ship you leave gets its held power / trigger inputs
  released and any charge cancelled; a manually cycled target carries over. Fires
  `OnShipSwitched(New, Old)` and `AddStat("ShipSwitches")`. The chase camera snaps
  (`SnapCamera`) so the switch reads as a hard cut (C-03 #1).
- **Death:** the squad manager polls `UTDHealthComponent::IsDead` each tick (no delegate coupling).
  At death: neutral input, triggers released, guns blocked, aim cleared, hyperdrive stopped, flight
  parked, and Squad drives a 0.6 s tumble (spin + drift, HullHit cue). Then Explosion cue, hidden,
  collision off, and if it was the player's ship a forced switch to the **healthiest** living ship.
  `OnShipDowned` fires at death. Respawn and retake share one restore path: unpark, teleport to a
  safe spot, `ResetFlightState`, full health, weapons unblocked + reset, powers `ResetPower`,
  hyperdrive back on; a retake also `ResetHyperdrive` + `AbortToFlying`.
- **Respawn:** `RespawnDelay` 12 s from death; warps in behind its formation slot at the leader's
  velocity, full health, `RespawnInvulnerability` 2 s, back to its wingman. `OnShipRespawned`.
- **Between waves (ENG-03):** hull repairs at 8/s on living ships from `OnWaveCleared` until the
  next `OnWaveStarted`; never during a wave.
- **Presentation:** `FTDShipSpec::Visuals` (default `/Game/TriadDrift/Presentation/DA_<Ship>_Visuals`,
  loaded only if the asset exists, misses cached) is handed to T1's `ATDShip::SetShipVisuals`. Squad
  plays SwitchIn / SwitchOut exactly once per switch (gameplay side only), HullHit at death,
  Explosion at the end of the tumble, RespawnWarpIn on respawn. Squad colour comes from
  `UTDStatics::ShipColor` unless the visuals asset sets one.
- **Squad wipe:** all three down → `OnSquadWiped` once → `ITDGameFlow::ReportPlayerDefeated()`. If a
  game flow took it, respawn timers stop and the flow restarts the scene by calling `SpawnSquad`
  again (that call fully resets the squad). With no game flow (sandbox) timers keep running.
- **Switch Strike:** a power (`UTDEventBus::OnPowerUsed`, broadcast by T2) within
  `SwitchStrikeWindow` 1.5 s before a switch gives the new ship `+25%` for 4 s through
  `UTDWeaponComponent::SetOutgoingDamageMultiplier("SwitchStrike", 1.25)`; cleared on expiry, on
  the next switch, on death and on EndPlay. One power arms one switch.
- **Powers:** the squad installs T2's power components on each ship once, from the spec
  (`FTDShipSpec::Signature1PowerClass` / `Signature2PowerClass` / `AbilityComponentClass`; defaults:
  Rift portal blaster, Lancer ram, Bastion missile swarm + shield dome on hold-Q, and an ability slot
  on all three). Never re-created; cooldowns reset on respawn and retake. AI-flown ships can't fire
  them (T2's player-control guard, tested). A power started on one ship keeps running after a switch
  (dome stays up, tested).
- **Formation:** loose V. Left slot `(-1600, -1600, 150)`, right slot `(-1600, 1600, -150)` in a
  frame of the leader's heading and WORLD up (never its roll, so a barrel roll doesn't swing the
  slots), about 2300 cm from the leader, assigned by switch order so wingmen don't swap sides.
  Squad ships never physically block each other. `GetFormationSlotLocation(Ship)`.
- **Wingman AI:**
  - Formation: flies parallel to the leader, steers out the sideways error and fixes the
    along-track error with speed only (an overshoot eases off, never U-turns). A catch-up allowance
    lets the throttle reach 1.25x the leader's speed (T1 `SetThrottleTopSpeed`, no boost meter), so
    Bastion keeps up with a cruising Lancer; boost is only a burst when more than 8000 cm behind.
  - Target priority: focus order → whoever threatens the player (`GetLastAttacker` on the player's
    ship, or within 12 000 cm and pointed at it) → nearest (2000 cm bands) → lowest health; the two
    wingmen prefer different targets; slight stickiness.
  - Engage: pursues the lead point, breaks off inside 1800 cm, fires the primary inside a 6° nose
    cone and 20 000 cm, secondary every 3 s (+0-1.5 s) held 1 s so charge weapons fire, never
    shoots through the player, 2-4° aim error re-rolled every 0.3-0.6 s, 35 000 cm leash.
  - Evade: shield break or a homing missile on it (`UTDProjectileSubsystem::FindMissilesHomingOn`)
    → 1.8 s break-and-jink at boost, 3 s cooldown; then Regroup.
  - Hurt (< 40%) and a player shield dome within 15 000 cm (`UTDPowerSubsystem::GetNearestDome`) →
    heads into it.
  - Hyperdrive spool (`OnHyperdriveState` Spooling): break off and form up; Idle (cancel or
    arrival) resumes fighting.
  - `ATDSquadManager::SetWingmenPassive(bool)`: landing/cinematics; no input, triggers or targets.
  - Avoidance, every 0.15 s along the velocity (1.2 s look-ahead): **T4's obstacle registry first**
    (`UTDWorldSubsystem::FindAvoidance`: asteroids, station ruins, planets, Dominion hull), and only
    if it reports nothing, a **fallback sphere sweep** against WorldStatic / WorldDynamic for what the
    registry doesn't hold (portal gate frames, Dominion pylons, moving ships). Plus 900 cm
    separation from squadmates (1500 cm, which also adjusts speed so an along-track stack opens up;
    collision spheres are 220 cm).
  - Follows the leader: `UTDEventBus::OnSpacecraftTeleported` (portals, gates) warps it to its slot;
    during hyperdrive (leader above 60 000 cm/s, well over ram speed) it rides in its slot; if more than
    60 000 cm from the leader for any reason it warps back.
  - **Never** calls a power or ability (there is no code path to one).
- **HUD** (`ITDHUDData`): `GetSquadHUD` (per ship hull/shield/boost/heat/secondary ammo/alive/
  respawn seconds/player-controlled/ability), `GetCurrentTarget` / `GetLockedTargets` (active ship's
  `UTDTargetingComponent`), power/ability cooldowns (`UTDPowerComponent::FindPower`), `GetWaveInfo` (T4 wave director), `GetSpeed01`, `IsOutOfBounds`.

## How to try it

- Tests: `.\Tools\td-test.ps1 -Filter TriadDrift.Squad`
- By hand: the sandbox game mode (T5) spawns `ATDSquadManager` and calls `SpawnSquad`. Tab /
  Shift+Tab / 1-2-3 switch (T1's controller calls `RequestSwitchNext/Prev/To`), F = focus.

| Test | Proves |
|---|---|
| `ShipLibrary.SpecsMatchFeatures` | every ENG-04 number and weapon id |
| `ShipLibrary.AppliedToSpawnedShips` | spec reaches health, flight tuning and weapons |
| `Switch.SameFrameAndKeepsVelocity` | possession + AI handover inside the call, both velocities kept, event |
| `Switch.Cooldown` | 0.4 s cooldown, next/prev wrap, no-op on active |
| `Switch.DownedShipUnselectable` | downed ship can't be chosen, cycling skips it |
| `Switch.SwitchInInvulnerability` | 2 s grace on switch-in |
| `Death.AutoSwitchToHealthiest` | tumble then switch to the healthiest ship |
| `Death.RespawnAfter12sWith2sInvulnerability` | 12 s respawn, full health, 2 s invulnerable |
| `Death.SquadWipeEvent` | wipe fires once; `SpawnSquad` restores the squad |
| `SwitchStrike.WindowAndMultiplier` | 1.5 s window, ×1.25 on the new ship for 4 s, cleared, one power = one switch |
| `HUD.SquadCards` | squad cards content and order (wave info comes from T4's `ATDWaveDirector`) |
| `Wingmen.HoldFormation` | wingmen within 1500 cm of their slots after 8 s |
| `Wingmen.EngageWithoutPowers` | both engage, deal damage, zero `OnPowerUsed` |
| `Wingmen.KeepFightingAfterSwitch` | the ship you left keeps fighting under AI |
| `Wingmen.HoldFormationBehindFasterLeader` | Rift and Bastion hold their slots behind a full-throttle Lancer (30/120 fps) |
| `ShipLibrary.PowersInstalledOnce` / `PowersComeFromTheSpec` | right power classes per ship, no duplicates after respawn + retake, overrides swap them, HUD cooldowns are real |
| `Wingmen.CannotFirePowersEvenIfAsked` | power input pushed at AI ships is refused |
| `Switch.PowerKeepsRunningAfterSwitch` | Bastion's dome stays up after switching away and ends on its own timer |
| `Switch.ThroughPlayerInput` | real Tab key through `ATDSpacePlayerController` switches ships |
| `Switch.PowersThroughPlayerInput` | real 1/2/3, Q and E keys fire each ship's power and ability (**red until T1's controller routes power input to `RoutePowerInput`**) |
| `Death.RespawnResetsWeapons` | ammo and heat restored on respawn |
| `HUD.TargetingForwardsActiveShip` / `HUD.WaveInfoCachedLookup` | targeting HUD follows the active ship; cached wave-director lookup |
| `Wingmen.TargetPriorityThreatToPlayerFirst` / `...LowestHealthBreaksTies` | threat to the player beats nearest; equal distance → lower health |
| `Wingmen.EvadeOnIncomingMissile` | a real enemy homing missile on a wingman → Evade, 30/60/120 |
| `Wingmen.EvadeOnShieldBreak` | real hit breaks the shield → Evade (boost) for 1.8 s, 30/60/120 fps |
| `Wingmen.FollowLeaderThroughPortal` | leader teleport → wingmen arrive in their slots |
| `Wingmen.AvoidRegisteredObstacle` | wingman steers round a registry asteroid on its line, then rejoins |
| `Wingmen.KeepApartFromSquadmates` | two wingmen forced onto one slot never come within 600 cm |
| `Wingmen.HurtWingmanSeeksShieldDome` | a hurt wingman flies into a player dome; a healthy one doesn't |
| `Wingmen.AimErrorTwoToFourDegrees` | measured shot direction error stays 2-4°, re-rolled |
| `Wingmen.BoostToCatchUpBeyond8000` | boost when > 8000 cm behind, none once back in the slot |
| `Death.HullRepairsBetweenWavesOnly` | no hull regen in a wave; ~8/s between waves |
| `Wingmen.FocusCommand` | focus overrides priority, expires at 8 s |
| `Wingmen.FollowHyperdriveNotRam` | a 45 000 cm/s dash does not break engagement; an 80 000 cm/s jump carries wingmen in formation |
| `Death.SpawnSquadIsARetake` | `SpawnSquad` again revives, repositions and resets everything (T5 retake) |

## Known gaps

- **Portal following** is a warp to a point AHEAD of the leader's exit (never inside the gate disc),
  not a fly-through. Tested with T4's real gate pair.
- Wingmen don't roll to match the leader; they rely on T1's auto-level.
- Bastion's flak isn't yet aimed at incoming missiles specifically.
- Director's Cut switch-cut payload and money shots: see `docs/eng/director.md`.
- HUD `GetHyperdriveCharge01` returns 0 until T1's hyperdrive component is on main.
