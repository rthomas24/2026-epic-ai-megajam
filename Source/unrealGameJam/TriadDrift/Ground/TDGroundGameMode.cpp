// Triad Drift - zero-asset ground sandbox (ENG-21/22/24). Owner: Teammate 5.

#include "TriadDrift/Ground/TDGroundGameMode.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TriadDrift/Ground/Planets/TDPlanetDirector.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"

namespace TDGroundSandbox
{
	static const TCHAR* CubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");
	static const TCHAR* ShapeMaterial = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	template <typename T>
	bool WorldHas(UWorld* World)
	{
		for (TActorIterator<T> It(World); It; ++It)
		{
			return true;
		}
		return false;
	}
}

ATDGroundGameMode::ATDGroundGameMode()
{
	DefaultPawnClass = ATDFirstPersonCharacter::StaticClass();
}

void ATDGroundGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	// One game mode for every on-foot map (DefaultEngine.ini GameModeMapPrefixes "L_Planet").
	const FString Short = FPackageName::GetShortName(MapName);
	if (Short.Contains(TEXT("Verdance")))
	{
		Destination = ETDDestination::Verdance;
	}
	else if (Short.Contains(TEXT("Cinder")))
	{
		Destination = ETDDestination::Cinder;
	}
	else if (Short.Contains(TEXT("Dominion")))
	{
		Destination = ETDDestination::DominionInterior;
	}
	// A real planet map (L_Planet_*) is a full visit: the squad flies in, lands, and the pilot gets out
	// (ATDPlanetDirector). Anything else (Entry, test maps) stays the on-foot training ground.
	bPlanetMode = Short.StartsWith(TEXT("L_Planet")) && (Destination == ETDDestination::Verdance || Destination == ETDDestination::Cinder);
	if (bPlanetMode)
	{
		DefaultPawnClass = nullptr; // the squad possesses its lead ship; the pilot appears on landing
		PlayerControllerClass = ATDSpacePlayerController::StaticClass();
		bSpawnTrainingGround = false;
	}
	Super::InitGame(MapName, Options, ErrorMessage);
}

bool ATDGroundGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	// Never spawn the pilot before the ground it stands on exists (it would fall, die and start on take 2).
	return bWorldBuilt && Super::PlayerCanRestart_Implementation(Player);
}

AActor* ATDGroundGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (AActor* Found = Super::ChoosePlayerStart_Implementation(Player))
	{
		RespawnTransform = Found->GetActorTransform(); // retakes come back to the start actually used
		bHasRespawnTransform = true;
		return Found;
	}
	// Empty map: make a start where the training ground expects the player.
	if (!SpawnPoint)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnPoint = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), PlayerSpawn, Params);
	}
	if (SpawnPoint && !bHasRespawnTransform)
	{
		RespawnTransform = SpawnPoint->GetActorTransform();
		bHasRespawnTransform = true;
	}
	return SpawnPoint;
}

void ATDGroundGameMode::StartPlay()
{
	if (bSpawnEnvironment)
	{
		SpawnEnvironment();
	}
	if (bSpawnTrainingGround)
	{
		SpawnTrainingGround();
	}
	if (bPlanetMode)
	{
		SpawnPlanetVisit();
	}
	bWorldBuilt = true;
	Super::StartPlay();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		UTDCheatManager::InstallOn(PC);
		if (PC && PC->IsLocalController())
		{
			PC->SetInputMode(FInputModeGameOnly()); // never inherit the menu's UI-only input
			PC->SetShowMouseCursor(false);
		}

		if (PC && !PC->GetPawn())
		{
			RestartPlayer(PC); // logged in before the floor existed
		}
	}

	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->NotifyLevelEntered(Destination);
		if (!bPlanetMode) // on a planet the director owns retakes (on foot at the pad, or a fresh descent)
		{
			SceneRestartHandle = GI->OnSceneRestart.AddUObject(this, &ATDGroundGameMode::HandleSceneRestart);
			if (UTDEventBus* Bus = UTDEventBus::Get(this))
			{
				Bus->OnObjectiveUpdated.AddUniqueDynamic(this, &ATDGroundGameMode::HandleObjectiveUpdated);
			}
		}
	}
}

void ATDGroundGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->OnSceneRestart.Remove(SceneRestartHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDGroundGameMode::SpawnPlanetVisit()
{
	const UTDFlowSettings* Settings = UTDFlowSettings::Get();
	const FName Planet = Destination == ETDDestination::Cinder ? FName(TEXT("Cinder")) : FName(TEXT("Verdance"));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform Xf = PlanetPadTransform;
	PlanetDirector = GetWorld()->SpawnActorDeferred<ATDPlanetDirector>(ATDPlanetDirector::StaticClass(), Xf);
	if (PlanetDirector)
	{
		PlanetDirector->PlanetId = Planet;
		PlanetDirector->PadTransform = Xf;
		PlanetDirector->RequiredObjectives = Destination == ETDDestination::Cinder ? Settings->CinderObjectives : Settings->VerdanceObjectives;
		PlanetDirector->FinishSpawning(Xf);
	}
}

void ATDGroundGameMode::HandleConsoleUsed(AActor* Interactable, AActor* Interactor)
{
	if (RoomDoor)
	{
		RoomDoor->Open();
	}
}

void ATDGroundGameMode::HandleSceneRestart(FName SceneId, int32 TakeNumber)
{
	RespawnPlayer();
}

void ATDGroundGameMode::HandleObjectiveUpdated(FName ObjectiveId, int32 Step, FText Text)
{
	// Progress checkpoint: a completed step moves the retake spot to where the player stands, if that is
	// solid ground (never mid-jump, never mid-mantle).
	if (Step <= 0)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const ATDFirstPersonCharacter* Char = Cast<ATDFirstPersonCharacter>(It->Get() ? It->Get()->GetPawn() : nullptr);
		if (Char && !Char->IsDead() && Char->GetCharacterMovement()->IsMovingOnGround())
		{
			RespawnTransform = FTransform(FRotator(0.f, Char->GetControlRotation().Yaw, 0.f), Char->GetActorLocation());
			bHasRespawnTransform = true;
		}
	}
}

void ATDGroundGameMode::RespawnPlayer()
{
	const FTransform Where = bHasRespawnTransform ? RespawnTransform : (SpawnPoint ? SpawnPoint->GetActorTransform() : PlayerSpawn);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ATDFirstPersonCharacter* Char = Cast<ATDFirstPersonCharacter>(It->Get() ? It->Get()->GetPawn() : nullptr))
		{
			Char->RespawnAt(Where);
		}
	}
}

// ------------------------------------------------------------------------------------------------

AActor* ATDGroundGameMode::SpawnBlock(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, const TCHAR* Label)
{
	static UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TDGroundSandbox::CubeMesh);
	static UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TDGroundSandbox::ShapeMaterial);
	if (!Cube)
	{
		return nullptr;
	}
	const FTransform Xf(FRotator::ZeroRotator, Center, SizeCm / 100.f); // engine cube is 100 cm
	AStaticMeshActor* Block = GetWorld()->SpawnActorDeferred<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Xf);
	if (!Block)
	{
		return nullptr;
	}
	UStaticMeshComponent* SMC = Block->GetStaticMeshComponent();
	SMC->SetMobility(EComponentMobility::Movable);
	SMC->SetStaticMesh(Cube);
	SMC->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Block->FinishSpawning(Xf);
	UTDStatics::TintPlaceholder(SMC, Color, 0.f);
#if WITH_EDITOR
	Block->SetActorLabel(Label);
#endif
	return Block;
}

void ATDGroundGameMode::SpawnEnvironment()
{
	// Daylight placeholder only where the planet map has no sky of its own (N-004 contract).
	ATDPlaceholderSky::SpawnIfNeeded(GetWorld(), ETDPlaceholderSkyStyle::Planet);
}

void ATDGroundGameMode::SpawnTrainingGround()
{
	// Verdance-ish palette: teal ground, violet ledges, warm-white parkour.
	const FLinearColor Ground(0.05f, 0.22f, 0.2f);
	const FLinearColor Ledge(0.35f, 0.15f, 0.55f);
	const FLinearColor Parkour(0.85f, 0.8f, 0.7f);

	SpawnBlock(FVector(0.f, 0.f, -50.f), FVector(12000.f, 12000.f, 100.f), Ground, TEXT("TD_Ground_Floor"));

	// Mantle lane to the left: 100 / 150 / 180 cm (mantle) and 250 cm (too tall: double jump).
	const float Heights[] = { 100.f, 150.f, 180.f, 250.f };
	for (int32 i = 0; i < 4; ++i)
	{
		const float H = Heights[i];
		SpawnBlock(FVector(600.f + i * 500.f, -900.f, H * 0.5f), FVector(300.f, 300.f, H), Ledge, *FString::Printf(TEXT("TD_Ground_Ledge_%dcm"), int32(H)));
	}
	// Parkour stepping stones to the right, rising.
	for (int32 i = 0; i < 5; ++i)
	{
		SpawnBlock(FVector(500.f + i * 450.f, 900.f, 80.f + i * 90.f), FVector(220.f, 220.f, 60.f), Parkour, *FString::Printf(TEXT("TD_Ground_Stone_%d"), i));
	}

	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Turret straight ahead behind a low wall: block (RMB) its bolts, time it for a perfect deflect.
	Turret = World->SpawnActor<ATDDummyTurret>(ATDDummyTurret::StaticClass(), FVector(2600.f, 0.f, 60.f), FRotator(0.f, 180.f, 0.f), Params);
	if (Turret)
	{
		Turret->BoltDamage = 4.f;      // deflect practice, not a threat
		Turret->BurstInterval = 3.5f;
		Turret->MaxRange = 3000.f;     // only engages once you walk toward it
	}

	// Two melee dummies that swing every 2.5 s with a 0.5 s telegraph: parry practice.
	for (int32 i = 0; i < 2; ++i)
	{
		if (ATDCrewDummy* Dummy = World->SpawnActor<ATDCrewDummy>(ATDCrewDummy::StaticClass(), FVector(900.f, -250.f + i * 500.f, 100.f), FRotator(0.f, 180.f, 0.f), Params))
		{
			Dummy->MaxHealth = 200.f;
			Dummy->MeleeInterval = 3.f;
			Dummy->MeleeDamage = 6.f; // practice partner: telegraphed, light, parryable
			if (Dummy->GetHealth())
			{
				Dummy->GetHealth()->InitMaxValues(200.f, 0.f, true);
				Dummy->GetHealth()->DestroyOwnerAfterDeathSeconds = 3.f;
			}
		}
	}

	// Sample objective: console (hold 2 s) -> clear the room behind the door.
	const FVector RoomCenter(-2500.f, 0.f, 0.f);
	SpawnBlock(RoomCenter + FVector(0.f, 900.f, 200.f), FVector(1800.f, 60.f, 400.f), Ledge, TEXT("TD_Ground_Room_WallN"));
	SpawnBlock(RoomCenter + FVector(0.f, -900.f, 200.f), FVector(1800.f, 60.f, 400.f), Ledge, TEXT("TD_Ground_Room_WallS"));
	SpawnBlock(RoomCenter + FVector(-900.f, 0.f, 200.f), FVector(60.f, 1800.f, 400.f), Ledge, TEXT("TD_Ground_Room_WallW"));
	SpawnBlock(RoomCenter + FVector(900.f, 600.f, 200.f), FVector(60.f, 600.f, 400.f), Ledge, TEXT("TD_Ground_Room_WallE1"));
	SpawnBlock(RoomCenter + FVector(900.f, -600.f, 200.f), FVector(60.f, 600.f, 400.f), Ledge, TEXT("TD_Ground_Room_WallE2"));

	SampleObjective = World->SpawnActor<ATDObjective>(ATDObjective::StaticClass(), RoomCenter, FRotator::ZeroRotator, Params);
	Console = World->SpawnActor<ATDInteractableActor>(ATDInteractableActor::StaticClass(), FVector(-1000.f, 300.f, 60.f), FRotator::ZeroRotator, Params);
	RoomDoor = World->SpawnActor<ATDDoor>(ATDDoor::StaticClass(), RoomCenter + FVector(900.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	Encounter = World->SpawnActor<ATDEncounterVolume>(ATDEncounterVolume::StaticClass(), RoomCenter + FVector(0.f, 0.f, 150.f), FRotator::ZeroRotator, Params);

	if (SampleObjective && Console && RoomDoor && Encounter)
	{
		SampleObjective->ObjectiveId = TEXT("Obj_Sandbox");
		SampleObjective->Title = NSLOCTEXT("TriadDrift", "SandboxObj", "Training: open the vault");
		FTDObjectiveStep S0; S0.Text = NSLOCTEXT("TriadDrift", "SandboxStep0", "Hold E at the console"); S0.Waypoint = Console;
		FTDObjectiveStep S1; S1.Text = NSLOCTEXT("TriadDrift", "SandboxStep1", "Clear the vault"); S1.Waypoint = Encounter;
		SampleObjective->Steps = { S0, S1 };

		Console->Prompt = NSLOCTEXT("TriadDrift", "SandboxConsole", "Hold E: open the vault");
		Console->HoldTime = 2.f;
		Console->LinkedObjective = SampleObjective;
		Console->LinkedStep = 0;
		// The console opens the door; the room seals behind you and reopens when it's clear.
		Console->OnInteracted.AddDynamic(this, &ATDGroundGameMode::HandleConsoleUsed);

		Encounter->LinkedObjective = SampleObjective;
		Encounter->LinkedStep = 1;
		Encounter->Count = 3;
		Encounter->RingRadius = 450.f;
		Encounter->CrewMix = { ETDCrewType::Grunt, ETDCrewType::Trooper, ETDCrewType::Grunt };
		Encounter->CoverActors = {
			SpawnBlock(RoomCenter + FVector(-350.f, 300.f, 75.f), FVector(160.f, 260.f, 150.f), Parkour, TEXT("TD_Ground_Room_Cover1")),
			SpawnBlock(RoomCenter + FVector(-350.f, -300.f, 75.f), FVector(160.f, 260.f, 150.f), Parkour, TEXT("TD_Ground_Room_Cover2")),
		};
		Encounter->DoorsToLock = { RoomDoor };
		Encounter->Box->SetBoxExtent(FVector(800.f, 800.f, 300.f));

		SampleObjective->StartObjective();
	}
}
