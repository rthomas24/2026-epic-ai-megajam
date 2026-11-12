// Triad Drift - a throwaway game world for automation tests. Lead-owned.
//
//   FTDTestWorld W;                                   // plain UGameInstance + AGameModeBase
//   ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
//   W.Advance(3.f);                                   // 3 s at 60 fps, deterministic
//   TestEqual(TEXT("..."), ...);
//
// Actors get BeginPlay and Tick; timers, subsystems and time dilation work. No rendering.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Templates/SubclassOf.h"

class UGameInstance;
class AGameModeBase;
class APlayerController;
class APawn;
class ULocalPlayer;
struct FKey;

/** Flags every Triad Drift test uses, so `td-test.ps1` finds them. */
#define TD_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

class UNREALGAMEJAM_API FTDTestWorld
{
public:
	explicit FTDTestWorld(TSubclassOf<UGameInstance> GameInstanceClass = nullptr, TSubclassOf<AGameModeBase> GameModeClass = nullptr);
	~FTDTestWorld();

	FTDTestWorld(const FTDTestWorld&) = delete;
	FTDTestWorld& operator=(const FTDTestWorld&) = delete;

	UWorld* GetWorld() const { return World; }
	UGameInstance* GetGameInstance() const { return GameInstance; }
	bool IsValid() const { return World != nullptr; }

	/** Spawn an actor (always spawns, ignoring collisions). */
	template <typename T>
	T* Spawn(const FVector& Location = FVector::ZeroVector, const FRotator& Rotation = FRotator::ZeroRotator, UClass* Class = nullptr)
	{
		if (!World)
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<T>(Class ? Class : T::StaticClass(), Location, Rotation, Params);
	}

	/** Spawn a player controller (optionally of a specific class) and possess Pawn with it. */
	APlayerController* SpawnPlayerController(APawn* PawnToPossess = nullptr, TSubclassOf<APlayerController> ControllerClass = nullptr);

	/**
	 * Spawn a LOCAL player controller: creates a ULocalPlayer on the test game instance (no viewport),
	 * so IsLocalController() is true, the Enhanced Input local-player subsystem exists, input
	 * components are processed and CharacterMovement simulates. Use this, not SpawnPlayerController,
	 * for anything driven by real key input. One local player per test world; removed on teardown.
	 */
	APlayerController* SpawnLocalPlayerController(APawn* PawnToPossess = nullptr, TSubclassOf<APlayerController> ControllerClass = nullptr);

	/** Press or release a key on a local controller through APlayerController::InputKey (the real path). */
	static void InjectKey(APlayerController* Controller, const FKey& Key, bool bPressed);

	/** Feed an analog value (mouse axis, stick axis, trigger) for one frame. */
	static void InjectAxis(APlayerController* Controller, const FKey& Key, float Value);

	/** Advance one frame of DeltaSeconds (real, undilated seconds). */
	void Tick(float DeltaSeconds = 1.f / 60.f);

	/** Advance Seconds of real time in fixed steps (default 60 fps). */
	void Advance(float Seconds, float StepSeconds = 1.f / 60.f);

	/** World (dilated) seconds elapsed. */
	float GetTimeSeconds() const;

private:
	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
	ULocalPlayer* LocalPlayer = nullptr;
};
