#include "TriadDrift/Tests/TDTestWorld.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "TriadDrift/Core/TDTypes.h"

FTDTestWorld::FTDTestWorld(TSubclassOf<UGameInstance> GameInstanceClass, TSubclassOf<AGameModeBase> GameModeClass)
{
	if (!GEngine)
	{
		UE_LOG(LogTriadDrift, Error, TEXT("FTDTestWorld: no GEngine"));
		return;
	}

	UClass* InstanceClass = GameInstanceClass ? GameInstanceClass.Get() : UGameInstance::StaticClass();
	GameInstance = NewObject<UGameInstance>(GEngine, InstanceClass);
	GameInstance->AddToRoot();

	// Creates a world context and an empty game world, then Init() (which starts the subsystems).
	GameInstance->InitializeStandalone();
	World = GameInstance->GetWorld();
	if (!World)
	{
		UE_LOG(LogTriadDrift, Error, TEXT("FTDTestWorld: failed to create a world"));
		return;
	}

	if (AWorldSettings* Settings = World->GetWorldSettings())
	{
		Settings->DefaultGameMode = GameModeClass ? GameModeClass.Get() : AGameModeBase::StaticClass();
	}

	const FURL URL;
	World->SetGameMode(URL);
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();
}

FTDTestWorld::~FTDTestWorld()
{
	if (World)
	{
		World->BeginTearingDown();
		World->EndPlay(EEndPlayReason::LevelTransition);
		if (GEngine)
		{
			GEngine->ShutdownWorldNetDriver(World);
		}
		World->DestroyWorld(false);
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World = nullptr;
	}
	if (GameInstance)
	{
		GameInstance->Shutdown();
		GameInstance->RemoveFromRoot();
		GameInstance = nullptr;
	}
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
}

APlayerController* FTDTestWorld::SpawnPlayerController(APawn* PawnToPossess, TSubclassOf<APlayerController> ControllerClass)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* Class = ControllerClass ? ControllerClass.Get() : APlayerController::StaticClass();
	APlayerController* Controller = World->SpawnActor<APlayerController>(Class, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (Controller && PawnToPossess)
	{
		Controller->Possess(PawnToPossess);
	}
	return Controller;
}

void FTDTestWorld::Tick(float DeltaSeconds)
{
	if (!World)
	{
		return;
	}
	++GFrameCounter;
	World->Tick(ELevelTick::LEVELTICK_All, DeltaSeconds);
}

void FTDTestWorld::Advance(float Seconds, float StepSeconds)
{
	if (StepSeconds <= 0.f)
	{
		return;
	}
	// Whole steps only, so results are identical run to run.
	const int32 Steps = FMath::RoundToInt(Seconds / StepSeconds);
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		Tick(StepSeconds);
	}
}

float FTDTestWorld::GetTimeSeconds() const
{
	return World ? World->GetTimeSeconds() : 0.f;
}
