#include "TriadDrift/Tests/TDTestWorld.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
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
	if (LocalPlayer && GameInstance)
	{
		// Destroys the local player's controller and pawn and shuts its subsystems down.
		GameInstance->RemoveLocalPlayer(LocalPlayer);
		LocalPlayer = nullptr;
	}
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

APlayerController* FTDTestWorld::SpawnLocalPlayerController(APawn* PawnToPossess, TSubclassOf<APlayerController> ControllerClass)
{
	if (!World || !GameInstance || !GEngine)
	{
		return nullptr;
	}
	if (!LocalPlayer)
	{
		UClass* PlayerClass = GEngine->LocalPlayerClass ? GEngine->LocalPlayerClass.Get() : ULocalPlayer::StaticClass();
		LocalPlayer = NewObject<ULocalPlayer>(GEngine, PlayerClass);
		GameInstance->AddLocalPlayer(LocalPlayer, FPlatformMisc::GetPlatformUserForUserIndex(0));
	}
	else if (APlayerController* Existing = LocalPlayer->PlayerController)
	{
		// One local player per world: hand the pawn to the controller that already owns it.
		if (PawnToPossess)
		{
			Existing->Possess(PawnToPossess);
		}
		return Existing;
	}

	APlayerController* Controller = SpawnPlayerController(nullptr, ControllerClass);
	if (!Controller)
	{
		return nullptr;
	}
	Controller->SetPlayer(LocalPlayer); // makes it local, creates PlayerInput, starts the input subsystems
	if (PawnToPossess)
	{
		Controller->Possess(PawnToPossess);
	}
	return Controller;
}

void FTDTestWorld::InjectKey(APlayerController* Controller, const FKey& Key, bool bPressed)
{
	if (Controller)
	{
		Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f));
	}
}

void FTDTestWorld::InjectAxis(APlayerController* Controller, const FKey& Key, float Value)
{
	if (Controller)
	{
		Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Axis, Value, 1));
	}
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
