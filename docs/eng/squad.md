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
- **Formation:** a forward-swept wedge so the player can SEE their wing (live playtest finding):
  left slot `(600, -1500, 220)`, right slot `(1500, 1750, -160)` in a frame of the leader's heading
  and WORLD up (never its roll), 1600-2300 cm from the leader, ahead of the chase camera at different
  depths (they read as a V on screen), more than 30 degrees off the leader's nose and outside the
  central aim area. Assigned by switch order so wingmen don't swap sides. Squad ships never
  physically block each other. `GetFormationSlotLocation(Ship)`. Test
  `Wingmen.VisibleFromTheChaseCamera` projects both wingmen through the real chase camera.
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
  respawn seconds/player-controlled; `Ability` = the ability the ship has EARNED, `None` while locked,
  with `AbilityCooldown01` 0 while locked: N-004 contract, test `HUD.AbilityShownOnlyWhenEarned`), `GetCurrentTarget` / `GetLockedTargets` (active ship's
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

## Squad barks (ART-11 voice lines, `TDSquadBarks.h`)

Gameplay asks for a line through `ITDGameFlow::RequestBark(BarkId, Speaker)` (re-broadcast as
`UTDEventBus::OnBarkRequested`; with no game flow, straight to the bus). Speakers: `Wren` (Rift),
`Kato` (Lancer), `Moss` (Bastion). Nick's `DT_Barks` rows key on these ids:

| Bark id | When | Speaker | Priority / cooldown |
|---|---|---|---|
| `Bark_SquadSwitchIn` | a manual switch lands | the pilot you switched to | normal, 6 s |
| `Bark_SquadShipDownRift` / `...Lancer` / `...Bastion` | a ship goes down (not in a full wipe) | the healthiest survivor | high, 8 s per id, and at most one ship-down line (any ship) per 10 s |
| `Bark_SquadRespawn` | a downed ship warps back | that pilot | normal, 20 s |
| `Bark_SquadFocusAck` | "focus my target" | the first living wingman | normal, 5 s |
| `Bark_SquadWingmanKill` | a wingman kills a hostile | that wingman | normal, 12 s |
| `Bark_SquadLowHull` | a living ship's hull drops below 30 % (re-armed above 55 % or on respawn) | that pilot | normal, 20 s |
| `Bark_SquadWipe` | all three down | the last active pilot | critical, 5 s |
| `Bark_SquadEngage[High|Low](Ahead|Left|Right|Behind)` (12 ids, e.g. `Bark_SquadEngageHighLeft`) | a wingman leaves formation for a fight; the id says where IT is, seen from the player's camera ("Kato, engaging high left") | that wingman | normal, 8 s per id, and 10 s per wingman |

Limiter (`FTDBarkTuning` on `ATDSquadManager`, real time): a 2.5 s global gap between lines (High
and Critical skip it), at most 3 lines per 10 s (Critical skips it: the wipe happens once per take),
and every line respects its own cooldown. Tests: `Barks.LimiterRules`, `Barks.TriggersAndSpeakers`,
`Barks.NeverSpam` (20 s of Tab spam: 3-4 switch lines, ≤ 6 lines total).

## Wingmen in frame: regression number (`TDSquadFrame.Standard.Weak`)

`.\Tools\td-test.ps1 -Filter TDSquadFrame` (not in the default suite, about 2-5 min). It flies waves 1-5 with
T4's seeded `FTDBalancePilot` (Weak: the player is always Rift; Lancer and Bastion are AI) and, every
0.25 s, projects from the player's real chase camera (no window involved). It plays both
`td.Wingmen.InViewTargeting` modes. Per wave: **engaging** = samples with an enemy inside the frame;
**wing seen** = of those, at least one wingman inside the frame too; **aim line** = samples with a wingman
within about 8 degrees of the screen centre (must stay near 0).

Recorded 2026-11-12 on eng/t3 475a680 (main 29e091b + squad), three runs, wing seen % (engaging samples):

| Wave | Mode 0 (old targeting) | Mode 1 (default since 55abdff) |
|---|---|---|
| 1 | 48% (181), 50% (222), 49% (169) | 56% (223), 59% (140), 50% (213) |
| 2 | 35% (310), 19% (243), 37% (549) | 29% (491), 36% (355), 41% (386) |
| 3 | 36% (550), 36% (404), 30% (398) | 26% (398), 22% (527), 21% (495) |
| 4 | 12% (549), 19% (522), 14% (545) | 18% (516), 19% (513), 15% (544) |
| 5 | 19% (600), 31% (150), 7% (183) | 15% (292), 26% (320), 17% (430) |

Aim line: 0-3% of samples in every wave and mode.

Reading (Lead decision, 2026-11-12): in a 360-degree dogfight the chase camera sees roughly 90 degrees, and
target choice alone cannot hold the wing in frame (the "overtake into the shot" steering cost kills in the
balance run, so `InShotOvertakeWeight` is 0). The goal "I can tell where my wing is and that it fights
with me" is the HUD's job: `ITDHUDData::GetSquadMarkers()` drives named brackets in view and edge arrows
out of view. Mode 1 stays the default. Its balance check (T4's `TDBalance.Standard.Weak`, 5 seeds, kills
per minute per wingman vs mode 0 with bias 10): w1 L +28% B -24%, w3 L -2% B +18%, w5 L +42% B -10%
(guard rail: no wingman down by more than a third).

## How to play: squad and Director (player-facing text, for the pause screen / README)

```
YOUR SQUAD
You lead three ships: Rift, Lancer and Bastion. Switch between them at any moment with Tab, or 1, 2 and 3. The switch is instant, and the two ships you leave keep flying and fighting on their own.

SWITCH STRIKE
Use a ship's power (Q), then switch within a second and a half: the ship you switch to hits 25% harder for 4 seconds.

FOCUS FIRE
Press F and your wingmen go after your current target for 8 seconds.

DOWN, NOT OUT
Shields recover on their own; hull repairs between waves. If your ship goes down, you jump straight into the healthiest one and are protected for 2 seconds. A downed ship warps back in after 12 seconds.

SCENES AND TAKES
Every wave and objective is a scene. Lose all three ships and the director calls "CUT!": the scene starts again as the next take, with the whole squad back.
```
