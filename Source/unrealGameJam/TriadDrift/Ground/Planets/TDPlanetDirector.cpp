// Triad Drift - planet arrival and hand-off (ENG-20 ground half). Owner: Teammate 5.

#include "TriadDrift/Ground/Planets/TDPlanetDirector.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

// ================================================================================================

ATDBoardShipInteractable::ATDBoardShipInteractable()
{
	Prompt = NSLOCTEXT("TriadDrift", "BoardShip", "Board ship");
	HoldTime = 0.5f;
	bSingleUse = false;
	SetActorScale3D(FVector(0.6f));
}

bool ATDBoardShipInteractable::CanInteract(AActor* Interactor) const
{
	return Director && Director->IsOnFoot() && bEnabled;
}

void ATDBoardShipInteractable::Interact(AActor* Interactor)
{
	if (CanInteract(Interactor))
	{
		OnInteracted.Broadcast(this, Interactor);
		Director->Reboard();
	}
}

// ================================================================================================

ATDPlanetDirector::ATDPlanetDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ATDPlanetDirector* ATDPlanetDirector::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	for (TActorIterator<ATDPlanetDirector> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

FName ATDPlanetDirector::GetPadReturnPoint() const
{
	return FName(*FString::Printf(TEXT("%s_Pad"), *PlanetId.ToString()));
}

void ATDPlanetDirector::BeginPlay()
{
	Super::BeginPlay();
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->OnShipLanded.AddUniqueDynamic(this, &ATDPlanetDirector::HandleShipLanded);
	}
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		RestartHandle = GI->OnSceneRestart.AddUObject(this, &ATDPlanetDirector::HandleSceneRestart);
	}
	if (bAutoStart)
	{
		StartVisit();
	}
}

void ATDPlanetDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->OnShipLanded.RemoveDynamic(this, &ATDPlanetDirector::HandleShipLanded);
	}
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->OnSceneRestart.Remove(RestartHandle);
	}
	Super::EndPlay(EndPlayReason);
}

APlayerController* ATDPlanetDirector::GetPlayerController() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

ATDSquadManager* ATDPlanetDirector::EnsureSquad()
{
	if (ATDSquadManager* Existing = ATDSquadManager::Get(this))
	{
		return Existing;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<ATDSquadManager>(ATDSquadManager::StaticClass(), FTransform::Identity, Params);
}

void ATDPlanetDirector::StartVisit()
{
	if (bStarted)
	{
		return;
	}
	bStarted = true;
	Rig = ATDPlanetTravelRig::SpawnForPlanet(GetWorld(), PlanetId, PadTransform, RequiredObjectives);
	if (Rig)
	{
		Rig->bPlaceholderGround = bRigPlaceholderGround;
	}

	ATDSquadManager* Squad = EnsureSquad();
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (Squad)
	{
		if (GI)
		{
			Squad->StartingShip = GI->GetProgressRef().LastShip;
		}
		// Fresh arrival: the squad flies in high and behind the pad; T1's zone takes over the descent.
		Squad->SpawnSquad(Rig ? Rig->GetDescentStart() : GetActorTransform());
		PlayerShip = Squad->GetActiveShip();
	}

	// Loading a checkpoint taken on foot here: everyone is already down; the pilot starts on foot.
	const bool bLandedStart = GI && GI->GetProgressRef().ReturnPoint == GetPadReturnPoint();
	if (bLandedStart && Rig && PlayerShip.IsValid())
	{
		SnapSquadLanded();
		HandOverToFoot(PlayerShip.Get());
	}
}

void ATDPlanetDirector::SnapSquadLanded()
{
	ATDSquadManager* Squad = ATDSquadManager::Get(this);
	if (!Squad || !Rig)
	{
		return;
	}
	if (ATDShip* Lead = PlayerShip.Get())
	{
		if (UTDLandingComponent* L = Lead->GetLanding())
		{
			L->SnapLanded(Rig->GetPlayerPad());
		}
	}
	int32 PadIndex = 0;
	const TArray<ATDLandingPad*>& Side = Rig->GetWingmanPads();
	for (ATDShip* Ship : Squad->GetLivingShips())
	{
		if (Ship && Ship != PlayerShip.Get() && Side.IsValidIndex(PadIndex))
		{
			if (UTDLandingComponent* L = Ship->GetLanding())
			{
				L->SnapLanded(Side[PadIndex++]);
			}
		}
	}
	Squad->SetWingmenPassive(true);
}

FTransform ATDPlanetDirector::ExitTransformFor(ATDShip* Ship) const
{
	if (Ship && Ship->GetLanding())
	{
		return Ship->GetLanding()->GetExitTransform();
	}
	return Rig ? Rig->GetActorTransform() : GetActorTransform();
}

void ATDPlanetDirector::HandleShipLanded(ATDShip* Ship, ATDLandingPad* Pad)
{
	if (Ship && Ship->IsPlayerFlown() && !bOnFoot)
	{
		HandOverToFoot(Ship);
	}
}

bool ATDPlanetDirector::HandOverToFoot(ATDShip* Ship)
{
	APlayerController* PC = GetPlayerController();
	if (!Ship || !PC)
	{
		return false;
	}
	PlayerShip = Ship;
	const FTransform Exit = ExitTransformFor(Ship);

	if (!FirstPerson)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		FirstPerson = GetWorld()->SpawnActor<ATDFirstPersonCharacter>(ATDFirstPersonCharacter::StaticClass(), Exit, Params);
	}
	if (!FirstPerson)
	{
		return false;
	}
	FirstPerson->SetActorHiddenInGame(false);
	FirstPerson->SetActorEnableCollision(true);
	FirstPerson->SetActorTickEnabled(true);
	FirstPerson->RespawnAt(Exit);

	Ship->SetPlayerControlled(false);
	PC->Possess(FirstPerson);
	PC->SetControlRotation(Exit.Rotator());
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		Squad->SetWingmenPassive(true);
	}
	bOnFoot = true;

	// The re-board prompt stands where the pilot climbed out.
	if (!BoardPoint)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		BoardPoint = GetWorld()->SpawnActor<ATDBoardShipInteractable>(ATDBoardShipInteractable::StaticClass(),
			Exit.GetLocation() - Exit.GetRotation().GetForwardVector() * 150.f, Exit.Rotator(), Params);
		if (BoardPoint)
		{
			BoardPoint->Director = this;
		}
	}
	if (BoardPoint)
	{
		BoardPoint->bEnabled = true;
		BoardPoint->SetActorHiddenInGame(false);
		BoardPoint->SetActorEnableCollision(true);
	}

	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->GetMutableProgress().ReturnPoint = GetPadReturnPoint();
		GI->SetLastShip(Ship->GetShipId());
		GI->SaveCheckpoint(); // a later Continue / CUT restarts on foot at this pad
	}
	return true;
}

bool ATDPlanetDirector::Reboard()
{
	APlayerController* PC = GetPlayerController();
	ATDShip* Ship = PlayerShip.Get();
	if (!bOnFoot || !PC || !Ship || !Ship->GetLanding())
	{
		return false;
	}
	bOnFoot = false;
	if (FirstPerson)
	{
		if (FirstPerson->GetBlade())
		{
			FirstPerson->GetBlade()->Retract();
		}
		FirstPerson->SetActorHiddenInGame(true);
		FirstPerson->SetActorEnableCollision(false);
		FirstPerson->SetActorTickEnabled(false);
	}
	if (BoardPoint)
	{
		BoardPoint->bEnabled = false;
		BoardPoint->SetActorHiddenInGame(true);
		BoardPoint->SetActorEnableCollision(false);
	}
	PC->Possess(Ship);
	Ship->SetPlayerControlled(true);
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		Squad->SetWingmenPassive(false);
	}
	Ship->GetLanding()->RequestTakeOff();
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->GetMutableProgress().ReturnPoint = NAME_None; // airborne: a reload starts with a fresh descent
	}
	return true;
}

void ATDPlanetDirector::HandleSceneRestart(FName SceneId, int32 TakeNumber)
{
	if (bOnFoot && FirstPerson)
	{
		// CUT on foot: back at the pad, the ship still parked beside it.
		FirstPerson->RespawnAt(ExitTransformFor(PlayerShip.Get()));
		return;
	}
	// CUT in the air (squad wiped): T3's SpawnSquad is the retake; descend again.
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		Squad->SpawnSquad(Rig ? Rig->GetDescentStart() : GetActorTransform());
		PlayerShip = Squad->GetActiveShip();
	}
}
