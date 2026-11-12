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
  (ENG-03). Fires `OnShipSwitched(New, Old)` and `AddStat("ShipSwitches")`. The chase camera snaps
  (`SnapCamera`) so the switch reads as a hard cut (C-03 #1).
- **Death:** the squad manager polls `UTDHealthComponent::IsDead` each tick (no delegate coupling).
  Dying 0.6 s (`DeathTumbleSeconds`) → hidden, collision off → if it was the player's ship, a forced
  switch to the **healthiest** living ship (hull+shield fraction). `OnShipDowned` fires at death.
- **Respawn:** `RespawnDelay` 12 s from death; warps in behind its formation slot at the leader's
  velocity, full health, `RespawnInvulnerability` 2 s, back to its wingman. `OnShipRespawned`.
- **Squad wipe:** all three down → `OnSquadWiped` once → `ITDGameFlow::ReportPlayerDefeated()`. If a
  game flow took it, respawn timers stop and the flow restarts the scene by calling `SpawnSquad`
  again (that call fully resets the squad). With no game flow (sandbox) timers keep running.
- **Switch Strike:** a power (`UTDEventBus::OnPowerUsed`, broadcast by T2) within
  `SwitchStrikeWindow` 1.5 s before a switch gives the new ship `+25%` for 4 s through
  `UTDWeaponComponent::SetOutgoingDamageMultiplier("SwitchStrike", 1.25)`; cleared on expiry, on
  the next switch, on death and on EndPlay. One power arms one switch.
- **Powers keep running after a switch:** Squad never cancels anything on the ship you leave.
  Powers are T2's components and keep ticking on their ship.
- **Formation:** loose V. Left slot `(-1600, -1600, 150)`, right slot `(-1600, 1600, -150)` in
  leader space (about 2300 cm from the leader), assigned by switch order so wingmen don't
  swap sides. `GetFormationSlotLocation(Ship)`.
- **Wingman AI:**
  - Formation: matches the leader's speed, corrects toward the slot, boosts beyond 8000 cm.
  - Target priority: focus order → whoever threatens the player (`GetLastAttacker` on the player's
    ship, or within 12 000 cm and pointed at it) → nearest (2000 cm bands) → lowest health; the two
    wingmen prefer different targets; slight stickiness.
  - Engage: pursues the lead point, breaks off inside 1800 cm, fires the primary inside a 6° nose
    cone and 20 000 cm, secondary every 3 s (+0-1.5 s) held 1 s so charge weapons fire, never
    shoots through the player, 2-4° aim error re-rolled every 0.3-0.6 s, 35 000 cm leash.
  - Evade: shield break or a homing missile on it (`UTDProjectileSubsystem::FindMissilesHomingOn`)
    → 1.8 s break-and-jink at boost, 3 s cooldown; then Regroup.
  - Hurt (< 40%) + an active shield dome → drifts toward the dome (hook ready; see gaps).
  - Avoidance: sphere sweep along velocity (1.2 s look-ahead, 0.15 s cadence) against WorldStatic /
    WorldDynamic, plus 900 cm separation from squadmates.
  - Follows the leader: `UTDEventBus::OnSpacecraftTeleported` (portals, gates) warps it to its slot;
    during hyperdrive (leader above 60 000 cm/s, well over ram speed) it rides in its slot; if more than
    60 000 cm from the leader for any reason it warps back.
  - **Never** calls a power or ability (there is no code path to one).
- **HUD** (`ITDHUDData`): `GetSquadHUD` (per ship hull/shield/boost/heat/secondary ammo/alive/
  respawn seconds/player-controlled/ability), `GetCurrentTarget` / `GetLockedTargets` (active ship's
  `UTDTargetingComponent`), `GetWaveInfo` (T4 wave director), `GetSpeed01`, `IsOutOfBounds`.

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
| `HUD.SquadCards` | squad cards content and order |
| `Wingmen.HoldFormation` | wingmen within 1500 cm of their slots after 8 s |
| `Wingmen.EngageWithoutPowers` | both engage, deal damage, zero `OnPowerUsed` |
| `Wingmen.KeepFightingAfterSwitch` | the ship you left keeps fighting under AI |
| `Wingmen.FocusCommand` | focus overrides priority, expires at 8 s |
| `Wingmen.FollowHyperdriveNotRam` | a 45 000 cm/s dash does not break engagement; an 80 000 cm/s jump carries wingmen in formation |
| `Death.SpawnSquadIsARetake` | `SpawnSquad` again revives, repositions and resets everything (T5 retake) |

## Known gaps

- **Shield dome drift**: `TDSquad::GetActiveShieldDome` returns false until T2 publishes the dome
  (ENG-11). One function to fill in.
- **Power cooldowns on the HUD cards** stay at "ready" until T2's `UTDPowerComponent` exists.
- **Portal following** is a warp to the slot when the leader teleports, not a fly-through.
- Wingmen don't roll to match the leader; they rely on T1's auto-level.
- Bastion's flak isn't yet aimed at incoming missiles specifically.
- C-03 #1 timing data and #2 (`OnHighlightMoment` + slow-mo) are the next task.
