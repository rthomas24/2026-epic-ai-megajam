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

	/** Advance one frame of DeltaSeconds (real, undilated seconds). */
	void Tick(float DeltaSeconds = 1.f / 60.f);

	/** Advance Seconds of real time in fixed steps (default 60 fps). */
	void Advance(float Seconds, float StepSeconds = 1.f / 60.f);

	/** World (dilated) seconds elapsed. */
	float GetTimeSeconds() const;

private:
	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
};
