---
name: ue-cpp-gameplay
description: How to write, build and verify Unreal Engine 5.9 C++ gameplay code for Triad Drift without opening the editor - the Core contracts (event bus, health, teams, presentation cues), headless builds with Tools/td-build.ps1, automation tests with FTDTestWorld and Tools/td-test.ps1, worktree rules for parallel agents, and the UE 5.9 pitfalls that cost time. Use whenever a task touches Source/unrealGameJam, any ENG-xx item in features.md, a C++ compile or link error, a failing or missing automation test, or "is this gameplay feature actually working".
compatibility: Unreal Engine 5.9 (MegaJam Preview), Visual Studio 2026 with MSVC 14.50, Windows. Tools are PowerShell scripts in Tools/.
---

# Unreal C++ gameplay for Triad Drift

Read `docs/eng/ARCHITECTURE.md` first: it holds the ownership table, the Core API and the definition of
done. This skill is the how-to that goes with it.

## The loop

1. **Work in your worktree** (`.wt/t<N>`, branch `eng/t<N>`) and only in the folders you own.
2. **Write the code and its tests together.** Tests live in `<Area>/Tests/*.cpp`.
3. **Build:** `.\Tools\td-build.ps1` (from the worktree root). Exit 0 and `errors=0 warnings=0`.
4. **Test:** `.\Tools\td-test.ps1 -Filter TriadDrift.<Area>`. Exit 0 and every line `[SUCCESS]`.
   A run takes 15–40 s. The full suite is `.\Tools\td-test.ps1`.
5. **Commit** (`git add <paths>`; never push, pull, rebase or switch branches), then report to the
   Lead: commit hash, files, tests and their results, risks.

Builds queue behind each other across worktrees, so a build can sit waiting for a minute; that is
normal. Never run the editor GUI from a worktree.

## Code conventions

- **C++ first.** Classes are `ATD*` / `UTD*` / `FTD*` / `ETD*` / `ITD*` under
  `Source/unrealGameJam/TriadDrift/<Area>/`. Include with the full path:
  `#include "TriadDrift/Core/TDTypes.h"`. Export with `UNREALGAMEJAM_API`.
- **No content assets needed to run.** Defaults live in C++ (spec structs and libraries). Placeholder
  visuals use engine meshes, e.g. `/Engine/BasicShapes/Cone.Cone`, loaded with
  `ConstructorHelpers::FObjectFinder` or `LoadObject`. Art arrives later through the presentation
  data assets in `Core/TDPresentation.h`; an empty slot must do nothing.
- **Every tunable is a `UPROPERTY(EditAnywhere)`** or a spec-struct field whose default is the
  features.md number. No magic numbers in logic.
- **Frame-rate independent.** Integrate with `DeltaTime`; for stiff maths use fixed sub-steps. Tests
  assert the same result at 30 and 120 fps.
- **No per-tick `GetAllActorsOfClass`**, no per-tick allocations in hot paths. Use
  `UTDCombatRegistry` for "who is out there".

## Core in one screen

| Need | Use |
|---|---|
| Deal damage | `UTDStatics::ApplyTDDamage(Target, Amount, Kind, Instigator, Causer, HitLoc, HitDir)`; splash: `ApplyTDRadialDamage`. Friendly fire is off; the Causer's team wins |
| Health | `UTDHealthComponent`: `InitMaxValues`, `GetHull01`, `IsAlive`, `SetInvulnerableFor`, `SuppressShield`, `Revive`, `Kill`, `OnDamaged`, `OnShieldBroken`, `OnDeath`, `DamageFilter` (C++ pre-filter) |
| Teams | Implement `ITDTeam::GetTeam() const` (plain C++ virtual), or set `UTDHealthComponent::DefaultTeam`. Query with `UTDStatics::GetTeam / AreHostile` |
| Find targets | `UTDCombatRegistry::Get(this)`: `GetHostilesOf`, `FindNearestHostile`, `CountAlive` |
| Fire an event | `if (UTDEventBus* Bus = UTDEventBus::Get(this)) { Bus->BroadcastX(...); }` |
| Game flow | `if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this)) { ... }` (null in plain test worlds) |
| VFX / sound / shake | `UTDStatics::PlayCue(this, Cue, Location, Rotation)` / `PlayCueAttached(Cue, Component)` with an `FTDFXCue` from a presentation data asset |
| Hit-stop / slow motion | `UTDFeelSubsystem::Get(this)->RequestHitStop()` / `RequestSlowMo(Seconds, Dilation)`. Never set global time dilation yourself |

Damage does **not** go through `AActor::TakeDamage`: `ApplyTDDamage` calls the health component
directly, so overriding `TakeDamage` will not intercept gameplay damage. Use `DamageFilter`.

## Writing a test

```cpp
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightCruise, "TriadDrift.Flight.ReachesCruiseSpeed", TD_TEST_FLAGS)
bool FTDFlightCruise::RunTest(const FString& Parameters)
{
    FTDTestWorld W;                                   // game world + game mode; actors BeginPlay and Tick
    ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
    FTDFlightInput In; In.Throttle = 1.f;
    Ship->SetFlightInput(In);
    W.Advance(5.f);                                   // 5 s at 60 fps; W.Advance(5.f, 1.f / 30.f) for 30 fps
    TestEqual(TEXT("cruise speed"), Ship->GetVelocity().Size(), 6000.0, 60.0);
    return true;
}
#endif
```

- `FTDTestWorld W(UTDGameInstance::StaticClass(), AMyGameMode::StaticClass());` picks the game
  instance and game mode. `W.SpawnPlayerController(Pawn)` possesses a pawn.
- Assert the **numbers in features.md** with a tolerance. Test the unhappy paths too: dead targets,
  null references, zero delta time, the same call twice.
- A test that needs helper `UCLASS`es must declare them in a header (UHT does not parse `.cpp`).
- Keep pure maths (flight integration, formation slots, target scoring) in plain functions or
  structs so tests can call them without a world.

## UE 5.9 pitfalls we already hit

| Symptom | Cause and fix |
|---|---|
| Pawn takes no damage from `UGameplayStatics::ApplyDamage` in a test | `APawn::ShouldTakeDamage` needs an authoritative game mode. Use `ApplyTDDamage` (direct) and `FTDTestWorld` (has a game mode) |
| Spawned actors never tick or `BeginPlay` in a hand-made world | The world has no game mode, so play never starts. Use `FTDTestWorld` |
| `UWorld::CleanupWorld called on a world that has begun play` warning fails the test | Call `World->EndPlay(...)` before destroying it. `FTDTestWorld` does |
| Radial queries miss actors with no primitive components | `GetActorBounds` returns zero bounds at the origin; fall back to `GetActorLocation()` |
| `EAutomationTestFlags` compile errors | Use the `TD_TEST_FLAGS` macro from `TDTestWorld.h` |
| Interface function will not override | Core interfaces are plain C++ virtuals: `virtual X Foo() const override;`, not `_Implementation` |
| `use_nodes` / `GENERATED_BODY` in a `.cpp`-only class | UHT only reads headers. Put `UCLASS`, `USTRUCT`, `UENUM`, `UINTERFACE` in `.h` files |
| Unresolved external for a new engine type | Add the module to `unrealGameJam.Build.cs` (Lead-owned: ask the Lead). Already there: Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, StateTreeModule, GameplayStateTreeModule, UMG, Slate, SlateCore, Niagara, DeveloperSettings, PhysicsCore, NavigationSystem |
| Enhanced Input without assets | `NewObject<UInputAction>(this)` and `NewObject<UInputMappingContext>(this)`, then `MapKey(Action, Key)`; add modifiers and triggers with `NewObject` too |
| A trigger / sensor volume acts as an invisible wall for ships | `OverlapAllDynamic` still BLOCKS our custom channels (`TDCollision::Ship`, `Projectile`, ...), because their default response in `DefaultEngine.ini` is Block. Call `SetCollisionResponseToAllChannels(ECR_Overlap)` first, then set the specific responses |
| Parts attached to a ship cannot be shot or rammed | A child actor mounted inside the parent's blocking sphere is never reached by a projectile sweep. Mount it outside the sphere and `IgnoreActorWhenMoving` both ways. Prove it with a test that fires a real projectile, not `ApplyTDDamage` |
| Pooled actor reused while something still points at it | Projectiles are pooled LIFO: never key a cooldown or a highlight on the actor pointer alone, use the launch serial as well |
| FX component held after `UTDStatics::PlayCue` | One-shot cues come from the Niagara pool (auto-release). Use the return value this frame only. For loops use `PlayCueAttachedLoop` and stop both halves yourself |
| Engine `LoadGameFromSlot` asserts on a corrupt `.sav` | Read the bytes, check the header, then `LoadGameFromMemory` (see `Flow/TDGameInstance.cpp`) |
| A test passes but the feature is broken in play | The test called an internal directly (`Kill()`, `ApplyTDDamage`, a counter of attempts). Drive the real path: real projectile, real possession, real input, at 30 / 60 / 120 fps |
| Build breaks only after a merge: `ambiguous call to overloaded function`, or C4459 `hides global declaration` in someone else's file | Unity builds glue several `.cpp` files together, and the grouping changes when other branches land. In any `.cpp`: no generic names at namespace scope (`Steps`, `Spawn`, `Advance`), no file-scope `using namespace`, and helpers used by two files live in ONE header as `inline` functions in an area namespace. Before reporting done, merge `main` and build |
| `/Game/...` argument turns into `C:/Program Files/Git/Game/...` | Git Bash rewrites leading-slash arguments. `export MSYS_NO_PATHCONV=1` before calling `Tools/td-shot.ps1 -Map /Game/...`, or run it from PowerShell |
| A test depends on whether a real map exists on disk | Point the settings at a bogus path (or `/Engine/Maps/Entry`) for the test and restore them; never `OpenLevel` a real map inside an automation world |
| Build fails only in another agent's file | You merged `main` mid-change or touched a folder you do not own. Message the owner; do not "fix" their file |

## Try it by hand (human or agent with a screen)

```powershell
& "C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor.exe" "<worktree>\unrealGameJam.uproject" /Engine/Maps/Entry?game=/Script/unrealGameJam.TDSpaceGameMode -game -windowed -ResX=1600 -ResY=900
```

`TD.*` console commands (`TD.God`, `TD.SkipWave`, `TD.KillAll`, `TD.Phase <name>`, ...) come from
`UTDCheatManager`.
