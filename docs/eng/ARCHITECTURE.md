# Triad Drift engineering architecture (Ryan's side)

The contract for the six agents building Ryan's `ENG-xx` items in features.md. Read AGENTS.md and
features.md §0–§5 first. This file says **who owns what, how code plugs together, how we work in
parallel, and what "done" means**.

## 1. The team

| Agent | Area | Owns (under `Source/unrealGameJam/TriadDrift/`) | ENG items, in order |
|---|---|---|---|
| **Lead** (session "Game jam repository setup") | Architecture, integration, verification | `Core/`, `Tests/`, `Tools/`, `Config/`, `*.Build.cs`, `docs/eng/ARCHITECTURE.md` | C-01, C-02, ENG-27, ENG-28 |
| **Teammate 1** | Flight & input | `Flight/`, `Input/` | ENG-01, ENG-02 → ENG-15 → ENG-20 |
| **Teammate 2** | Combat | `Combat/`, `Powers/` | ENG-05, ENG-06 (+ ENG-03 respawn rules with T3) → ENG-09, 10, 11 → ENG-12 |
| **Teammate 3** | Squad & wingmen | `Squad/` | ENG-04, ENG-07, ENG-08 → C-03 hooks #1–#2 gameplay side |
| **Teammate 4** | Enemies & waves | `Enemies/`, `Waves/`, `World/` | ENG-13, ENG-14 → ENG-16, 17, 18 |
| **Teammate 5** | Game flow & ground | `Flow/`, `Ground/` | ENG-19 (+ C-03 #3 takes) → ENG-21–26 → stretch ENG-29–31 |

Six agents work at once. **Talk to each other.** Message the owner directly when you need an API
from their area, and copy the Lead on anything that changes a public signature. The Lead assigns
work, reviews, integrates and verifies; when you finish or block, tell the Lead immediately.

## 2. How we work in parallel (follows AGENTS.md §6.9)

- Each teammate has **its own git worktree and branch**, created by the Lead:
  `C:\Users\ryant\Documents\myRepos\game-jam\.wt\t<N>` on branch `eng/t<N>`. Work **only** there.
  Never edit files in the main checkout or another teammate's worktree.
- **Write only inside your owned folders** (§1) plus your own `docs/eng/<area>.md`. Anything else is
  read-only: ask the owner (or the Lead for `Core/`, `Build.cs`, `Config/`).
- **Commit small and often on your own branch** (`git add <paths>`; `git commit`). Message format
  `<area>: <what changed>`, ending with the `Co-Authored-By` line your session was given.
- **Never** `git push`, `pull`, `fetch`, `rebase`, `reset --hard`, switch branches, or touch
  worktrees. The Lead integrates: it merges `eng/t<N>` into `main`, builds, runs the tests and pushes.
- When the Lead announces "main is at `<sha>`, merge it", run `git merge main` in your worktree
  (local ref only), rebuild, and fix anything you broke.
- **Tell the Lead when you start a task and when you finish one** (item id, paths, commit hash).
  The Lead mirrors both on the `CLAIMS.md` board on `main`, which is how Nick's agents see what
  engineering is doing, what is coming and what is done (AGENTS.md §6.5 start / finish rule).
  Read `CLAIMS.md` before starting anything outside pure C++ in your own folders. Never edit it.
- No Unreal Editor GUI in worktrees. Build and test headless with the tools in §5.
- Binary assets (`.uasset`, `.umap`) are avoided on the engineering side for now: **gameplay must run
  with zero content assets**, using C++ defaults and engine basic shapes as placeholders. Nick's
  art arrives later through the presentation data assets (features.md §3.5).

## 3. Core: the shared foundation (Lead-owned, frozen once published)

`Source/unrealGameJam/TriadDrift/Core/`. Include as `"TriadDrift/Core/<File>.h"`.

| File | Contents |
|---|---|
| `TDTypes.h` | `LogTriadDrift`; enums `ETDShipId`, `ETDTeam`, `ETDAbilityId`, `ETDDestination`, `ETDDamageKind`, `ETDGamePhase`, `ETDHighlightKind`, `ETDHyperdriveState`, `ETDPortalState`, `ETDWeaponSlot {Primary, Secondary}`; structs `FTDRunStats`, `FTDProgress`, `FTDShipHUD`, `FTDWaveInfo`, `FTDFlightInput`; collision channels `TDCollision::Ship / Projectile / Interactable / Portal` |
| `TDInterfaces.h` | `ITDTeam`, `ITDDeflectable`, `ITDInteractable`, `ITDHUDData`, `ITDGameFlow` (C++ interfaces, Blueprint-callable) |
| `TDEventBus.h` | `UTDEventBus` (GameInstance subsystem): every event in features.md §3.4 as a `BlueprintAssignable` delegate plus a `Broadcast*` function. `UTDEventBus::Get(WorldContext)` |
| `TDDamageTypes.h` | `UTDDamageType` + one subclass per `ETDDamageKind`; `UTDDamageType::KindOf(DamageTypeClass)` |
| `TDHealthComponent.h` | `UTDHealthComponent`: hull + regenerating shield, invulnerability windows, damage-kind handling, `OnDamaged` / `OnShieldBroken` / `OnDeath`, hooks `AActor::TakeDamage` via `OnTakeAnyDamage`, registers its owner in the combat registry |
| `TDCombatRegistry.h` | `UTDCombatRegistry` (World subsystem): every live damageable actor by team. `GetActorsOfTeam`, `GetHostilesOf`, `FindNearestHostile`, `CountAlive` |
| `TDPresentation.h` | Presentation data assets (`UTDShipVisuals`, `UTDWeaponFX`, `UTDEnemyVisuals`, `UTDAbilityFX`, `UTDTravelFX`, `UTDBladeFX`, `UTDUIStyle`), all soft references, all optional |
| `TDStatics.h` | `UTDStatics`: `GetTeam`, `AreHostile`, `GetGameFlow`, null-safe `SpawnFX*` / `PlaySound*` / `PlayCameraShake` from soft references, `ApplyTDDamage` (team check + `ApplyDamage`) |
| `../Tests/TDTestWorld.h` | `FTDTestWorld`: creates a game world for automation tests, spawns actors, advances time deterministically |

Rules for using Core:
- **Damage** always goes through `UTDStatics::ApplyTDDamage` (friendly fire is off).
- **Events** always go through `UTDEventBus`. Never call another area's UI or presentation directly.
- **Presentation** is always a data-asset slot spawned with `UTDStatics::SpawnFX*`. An empty slot
  must do nothing. Until art exists, draw placeholders with engine basic shapes / debug draws.
- Need something added to Core? Message the Lead with the exact signature. Don't fork your own copy.

## 4. Cross-area class contracts

Each owner publishes these classes **API-first**: a compiling header with the public functions below
(stub bodies are fine) as your first commit, so the others can code against it. Keep these names and
signatures stable; extend freely.

### Teammate 1: flight (`Flight/`, `Input/`)
- `ATDSpacecraft : APawn, ITDTeam` — the flyable body shared by squad ships **and** enemy ships.
  - `void SetFlightInput(const FTDFlightInput& In)` / `const FTDFlightInput& GetFlightInput() const`
  - `UTDShipMovementComponent* GetShipMovement() const` · `UTDHealthComponent* GetHealth() const`
  - `void SetTeam(ETDTeam)` · `ETDTeam GetTeam() const override`
  - `float GetSpeed01() const` · `bool IsBoosting() const` · `bool IsDrifting() const` · `float GetBoost01() const`
  - `void ApplyFlightTuning(const FTDFlightTuning& Tuning)` (struct in your header: min/cruise/boost speed, turn rates, boost meter, drift values)
  - `void TeleportSpacecraft(const FVector& Location, const FRotator& Facing, bool bKeepSpeed)` (portals)
  - `void SetSpeedOverride(float SpeedCmPerSec, float SteeringScale)` / `ClearSpeedOverride()` (hyperdrive, ram)
  - `UStaticMeshComponent* GetHullMesh() const` (placeholder mesh; swapped by presentation later)
- `ATDShip : ATDSpacecraft` — a squad ship. `ETDShipId GetShipId() const` / `SetShipId`, chase camera
  (`USpringArmComponent`, `UCameraComponent`), `void SetPlayerControlled(bool)`.
- `ATDSpacePlayerController : APlayerController` — builds the Enhanced Input actions and mapping
  context **in C++** (no assets) per ENG-02 and routes them: flight to the possessed `ATDShip`,
  triggers to `UTDWeaponComponent` (T2), switch/command to `ATDSquadManager` (T3), power/ability to
  T2's power components.

### Teammate 2: combat (`Combat/`, `Powers/`)
- `UTDWeaponComponent : UActorComponent` — `void EquipWeapon(ETDWeaponSlot, FName WeaponId)` ·
  `void SetTriggerHeld(ETDWeaponSlot, bool bHeld)` · `void SetAimTarget(AActor* Target)` ·
  `void SetAimDirection(const FVector& WorldDir)` · `float GetHeat01(ETDWeaponSlot) const` ·
  `int32 GetAmmo(ETDWeaponSlot) const` · `bool IsOverheated(ETDWeaponSlot) const`
- `UTDWeaponLibrary` — `static const FTDWeaponSpec& Get(FName WeaponId)` with every weapon in
  features.md ENG-05 plus the enemy weapons of ENG-13 as C++ defaults.
- `ATDProjectile : AActor, ITDTeam, ITDDeflectable` — `static ATDProjectile* Spawn(UWorld*, const FTDProjectileSpec&, const FTransform&, AActor* Instigator, ETDTeam Team, AActor* HomingTarget = nullptr)`
- `UTDTargetingComponent : UActorComponent` — `AActor* GetCurrentTarget() const` · `void CycleTarget()` ·
  `const TArray<AActor*>& GetLockedTargets() const` · lead-indicator and off-screen data for the HUD.
- Later: `UTDPowerComponent` base (`bool TryActivate()`, `float GetCooldown01() const`,
  `FName GetPowerName() const`) with the three signature powers and three abilities.

### Teammate 3: squad (`Squad/`)
- `UTDShipLibrary` — `static const FTDShipSpec& Get(ETDShipId)`: ENG-04 stats, weapon ids, colours.
- `ATDSquadManager : AActor, ITDHUDData` — `static ATDSquadManager* Get(const UObject* WorldContext)` ·
  `void SpawnSquad(const FTransform& Origin)` · `ATDShip* GetShip(ETDShipId) const` ·
  `ATDShip* GetActiveShip() const` · `bool RequestSwitchTo(ETDShipId)` · `bool RequestSwitchNext()` /
  `RequestSwitchPrev()` · `TArray<ATDShip*> GetLivingShips() const` · `void IssueFocusCommand(AActor* Target)` ·
  `bool IsSwitchStrikeActive() const` · `void NotifyPowerUsed(ETDShipId)`
- `ATDWingmanAIController : AAIController` — flies an `ATDShip` through `SetFlightInput` and
  `UTDWeaponComponent` only (never powers).

### Teammate 4: enemies & waves (`Enemies/`, `Waves/`, `World/`)
- `ATDEnemyShip : ATDSpacecraft` — `void InitFromSpec(FName EnemyId)`; `UTDEnemyLibrary::Get(FName)`
  holds the five enemy types of ENG-13; telegraph state exposed (`bool IsTelegraphing() const`).
- `ATDEnemyAIController : AAIController` — approach / attack run / break off / re-approach.
- `ATDWaveDirector : AActor` — `static ATDWaveDirector* Get(const UObject*)` · `void StartWave(int32 Index)` ·
  `void StartFromProgress()` · `FTDWaveInfo GetWaveInfo() const` · `void SkipWave()`; wave table as
  C++ defaults (ENG-14); fires `OnWaveStarted` / `OnWaveCleared` / `OnTakeStarted`.

### Teammate 5: flow & ground (`Flow/`, `Ground/`)
- `UTDGameInstance : UGameInstance, ITDGameFlow` — progress, checkpoints, save slot, travel,
  scene/take restarts (C-03 #3), run stats.
- `ATDSpaceGameMode : AGameModeBase` — spawns the squad manager and wave director, uses
  `ATDSpacePlayerController`, and lights an empty map so the sandbox is playable with **no assets**:
  `UnrealEditor.exe <uproject> /Engine/Maps/Entry?game=/Script/unrealGameJam.TDSpaceGameMode -game -windowed`
- `UTDCheatManager` — console commands `TD.Phase`, `TD.Unlock`, `TD.God`, `TD.KillAll`, `TD.SkipWave`.
- Later: `ATDFirstPersonCharacter`, energy blade, crew AI, objectives, planet gameplay.

## 5. Build and verify

From your worktree root (PowerShell):

```powershell
.\Tools\td-build.ps1                      # compile the editor target for THIS worktree; exit 0 = clean
.\Tools\td-test.ps1 -Filter TriadDrift.Combat   # run automation tests headless; exit 0 = all passed
```

- Tests live next to the code they test in `<Area>/Tests/*.cpp`, named `TriadDrift.<Area>.<Thing>`,
  flags `EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter`, wrapped in
  `#if WITH_DEV_AUTOMATION_TESTS`. Use `FTDTestWorld` to spawn actors and advance time.
- Builds queue behind each other (`-WaitMutex`); a headless test run takes a minute or two.

## 6. Definition of done (every ENG item)

**Quality standard:** `/AAA.md` applies to all engineering work. Every report to the Lead names the stage
reached, in AAA.md's words: *prototype complete*, *technically verified* (builds, tests pass, real-path
tests at 30 / 60 / 120 fps), *integrated* (on `main`), *gameplay validated* (played in a running game,
by a human or by an agent through the MCP play tools, with captures). *Presentation approved* belongs to
Nick's side. Nothing is called done or final on tests and screenshots alone; after it works, find and fix
the most noticeable weakness a player would feel before adding anything new.

1. **Builds clean** in your worktree: exit 0, no new warnings.
2. **Behaviour is proven by automation tests** that assert the numbers in features.md (for example
   "shield starts regenerating 3 s after the last hit, at 25%/s"). No test, not done.
3. **Tunable, not hard-coded:** every number is an `EditAnywhere` `UPROPERTY` or a spec-struct field,
   defaulting to the features.md value.
4. **Wired in:** events on `UTDEventBus`, presentation slots null-safe, HUD data exposed.
5. **Robust:** null checks at boundaries, no per-frame allocations in hot paths, no `GetAllActorsOfClass`
   per tick (use the combat registry), works at 30 and 120 fps (frame-rate independent).
6. **Documented:** `docs/eng/<area>.md` says what exists, how to try it by hand, and known gaps.
7. **Reported to the Lead:** commit hash, files changed, tests added and their results, risks, and
   what you need from others. The Lead reviews, merges, re-runs the suite and only then ticks the item.

AAA bar: game feel first (responsive, readable, juicy), defensive code, zero crashes, and nothing
"temporary" left unexplained.
