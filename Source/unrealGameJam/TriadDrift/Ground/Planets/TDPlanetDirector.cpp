// Triad Drift - planet arrival and hand-off (ENG-20 ground half). Owner: Teammate 5.

#include "TriadDrift/Ground/Planets/TDPlanetDirector.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

// ================================================================================================

ATDBoardShipInteractable::ATDBoardShipInteractable()
{
	Prompt = NSLOCTEXT("TriadDrift", "BoardShip", "Board ship");
	ConsoleMesh = TDPropArt::Default(TEXT("SM_BoardPoint"));
	HoldTime = 0.5f;
	bSingleUse = false;
	SetActorScale3D(FVector(0.6f));
}

bool ATDBoardShipInteractable::CanInteract(AActor* Interactor) const
{
	const ATDShip* Ship = Director ? Director->GetPlayerShip() : nullptr;
	const UTDHealthComponent* H = Ship ? UTDStatics::GetHealth(Ship) : nullptr;
	return Director && Director->IsOnFoot() && bEnabled && Ship && (!H || H->IsAlive()) && Ship->GetLanding() && Ship->GetLanding()->IsLanded();
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
	if (bBuildCourse)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.bDeferConstruction = true;
		Course = GetWorld()->SpawnActor<ATDPlanetCourse>(ATDPlanetCourse::StaticClass(), PadTransform, Params);
		if (Course)
		{
			Course->PlanetId = PlanetId;
			Course->FinishSpawning(PadTransform);
		}
		// The course brings its own ground; until the rig takes the flag at spawn (T1), switch its slab off here.
		if (Rig)
		{
			TInlineComponentArray<UStaticMeshComponent*> Meshes(Rig);
			for (UStaticMeshComponent* M : Meshes)
			{
				if (M && M->GetFName() == TEXT("PlaceholderGround"))
				{
					M->SetVisibility(false);
					M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
			}
		}
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
		// Out of the hatch facing the course (the pad's nose, between the two trailheads), not the ship.
		FTransform Exit = Ship->GetLanding()->GetExitTransform();
		Exit.SetRotation(FRotator(0.f, PadTransform.Rotator().Yaw, 0.f).Quaternion());
		return Exit;
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

int32 ATDPlanetDirector::CompleteMissingObjectives()
{
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (!GI)
	{
		return 0;
	}
	struct FReward { const TCHAR* Id; ETDAbilityId Ability; ETDShipId Ship; };
	static const FReward Rewards[] = {
		{ TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion },
		{ TEXT("Obj_Relay"), ETDAbilityId::Cloak, ETDShipId::Rift },
		{ TEXT("Obj_Outpost"), ETDAbilityId::EMPPulse, ETDShipId::Lancer },
		{ TEXT("Obj_Jammer"), ETDAbilityId::None, ETDShipId::Rift },
	};
	int32 Fixed = 0;
	for (const FName& Id : RequiredObjectives)
	{
		if (GI->GetProgressRef().CompletedObjectives.Contains(Id))
		{
			continue;
		}
		bool bExists = false;
		for (TActorIterator<ATDObjective> It(GetWorld()); It; ++It)
		{
			bExists |= It->ObjectiveId == Id;
		}
		if (bExists)
		{
			continue;
		}
		ETDAbilityId Ability = ETDAbilityId::None;
		ETDShipId Ship = ETDShipId::Rift;
		for (const FReward& R : Rewards)
		{
			if (Id == FName(R.Id)) { Ability = R.Ability; Ship = R.Ship; }
		}
		UE_LOG(LogTriadDrift, Error, TEXT("Planet %s: required objective %s has no objective actor in the world; completing it so the run can't be trapped."),
			*PlanetId.ToString(), *Id.ToString());
		GI->CompleteObjective(Id, Ability, Ship);
		++Fixed;
	}
	return Fixed;
}

bool ATDPlanetDirector::HandOverToFoot(ATDShip* Ship)
{
	CompleteMissingObjectives();
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
	if (bPilotStowed)
	{
		// Back from the ship: hittable and targetable again.
		bPilotStowed = false;
		FirstPerson->GetHealth()->SetInvulnerable(false);
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			Registry->Register(FirstPerson);
		}
	}
	FirstPerson->RespawnAt(Exit);

	Ship->SetPlayerControlled(false);
	PC->Possess(FirstPerson);
	PC->SetControlRotation(Exit.Rotator());
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		Squad->SetWingmenPassive(true);
	}
	bOnFoot = true;
	if (UTDGameInstance* PilotGI = UTDGameInstance::Get(this); PilotGI && Ship)
	{
		PilotGI->GetMutableProgress().LastShip = Ship->GetShipId(); // the landed ship's pilot is the one on foot (issue #29)
	}
	if (PlanetId == TEXT("Cinder"))
	{
		UTDStorySubsystem::Fire(this, TEXT("G4_Moss_SolidGround")); // T3: on landing (once per run)
	}

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
		// Stowed in the ship: nothing on the ground may hurt or target the pilot (distance-based melee and
		// the Warden's slam ignore collision), or a CUT would fire in flight.
		FirstPerson->GetHealth()->SetInvulnerable(true);
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			Registry->Unregister(FirstPerson);
		}
		bPilotStowed = true;
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
		// CUT on foot: back at the pad, the ship parked beside it. If it was destroyed or knocked out of
		// Landed meanwhile, the squad is rebuilt and snapped down onto the pads again.
		ATDShip* Ship = PlayerShip.Get();
		const UTDHealthComponent* H = Ship ? UTDStatics::GetHealth(Ship) : nullptr;
		const bool bShipOk = Ship && (!H || H->IsAlive()) && Ship->GetLanding() && Ship->GetLanding()->IsLanded();
		if (!bShipOk)
		{
			if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
			{
				APlayerController* PC = GetPlayerController();
				Squad->SpawnSquad(Rig ? Rig->GetDescentStart() : GetActorTransform()); // possesses the lead ship...
				PlayerShip = Squad->GetActiveShip();
				SnapSquadLanded();
				if (PC && FirstPerson)
				{
					if (PlayerShip.IsValid()) { PlayerShip->SetPlayerControlled(false); }
					PC->Possess(FirstPerson); // ...so hand the player straight back to the pilot
				}
			}
		}
		// A scene past a checkpoint picks up at the checkpoint's spot (the Warden's door, the far bank...).
		FTransform Where = ExitTransformFor(PlayerShip.Get());
		if (Course)
		{
			if (const ATDObjective* Obj = Course->FindObjective(SceneId))
			{
				if (const AActor* Spot = Obj->GetCheckpointRespawn())
				{
					Where = Spot->GetActorTransform();
				}
			}
		}
		FirstPerson->RespawnAt(Where);
		return;
	}
	// CUT in the air (squad wiped): T3's SpawnSquad is the retake; descend again.
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		Squad->SpawnSquad(Rig ? Rig->GetDescentStart() : GetActorTransform());
		PlayerShip = Squad->GetActiveShip();
	}
}
