// Triad Drift - the music event bridge (N-082; T1).

#include "TriadDrift/Core/TDMusicEventBridge.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Story/TDStoryTypes.h"
#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

const FName UTDMusicEventBridge::VictoryBegan(TEXT("VictoryBegan"));
const FName UTDMusicEventBridge::VictoryStinger(TEXT("VictoryStinger"));
const FName UTDMusicEventBridge::VictoryEnded(TEXT("VictoryEnded"));
const FName UTDMusicEventBridge::PlanetRevealed(TEXT("PlanetRevealed"));
const FName UTDMusicEventBridge::Takeoff(TEXT("Takeoff"));
const FName UTDMusicEventBridge::PhaseChanged(TEXT("PhaseChanged"));
const FName UTDMusicEventBridge::TransmissionStarted(TEXT("TransmissionStarted"));
const FName UTDMusicEventBridge::TransmissionEnded(TEXT("TransmissionEnded"));

namespace TDMusicBridge
{
	FName EnumName(const UEnum* E, int64 V)
	{
		return E ? FName(*E->GetNameStringByValue(V)) : NAME_None;
	}
}

void UTDMusicEventBridge::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UTDDescentSubsystem>();
	Collection.InitializeDependency<UTDEventBus>();
	Super::Initialize(Collection);
	VictoryBeganHandle = ATDMenuBackdrop::OnVictoryBegan.AddUObject(this, &UTDMusicEventBridge::HandleVictoryBegan);
	StingerHandle = ATDMenuBackdrop::OnStinger.AddUObject(this, &UTDMusicEventBridge::HandleStinger);
	VictoryEndedHandle = ATDMenuBackdrop::OnVictoryEnded.AddUObject(this, &UTDMusicEventBridge::HandleVictoryEnded);
	if (UTDDescentSubsystem* Descent = GetGameInstance()->GetSubsystem<UTDDescentSubsystem>())
	{
		RevealedHandle = Descent->OnRevealed.AddUObject(this, &UTDMusicEventBridge::HandleRevealed);
	}
	if (UTDEventBus* Bus = GetGameInstance()->GetSubsystem<UTDEventBus>())
	{
		Bus->OnPhaseChanged.AddUniqueDynamic(this, &UTDMusicEventBridge::HandlePhase);
		bPhaseBound = true;
	}
}

void UTDMusicEventBridge::Deinitialize()
{
	ATDMenuBackdrop::OnVictoryBegan.Remove(VictoryBeganHandle);
	ATDMenuBackdrop::OnStinger.Remove(StingerHandle);
	ATDMenuBackdrop::OnVictoryEnded.Remove(VictoryEndedHandle);
	if (UTDDescentSubsystem* Descent = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTDDescentSubsystem>() : nullptr)
	{
		Descent->OnRevealed.Remove(RevealedHandle);
	}
	Super::Deinitialize();
}

UTDMusicEventBridge* UTDMusicEventBridge::GetMusicEventBridge(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UTDMusicEventBridge>() : nullptr;
}

UWorld* UTDMusicEventBridge::CurrentWorld() const
{
	return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
}

void UTDMusicEventBridge::Emit(FName EventId, UObject* WorldContext, FName DetailId, bool bFlag)
{
	UE_LOG(LogTriadDrift, Log, TEXT("TD music bridge: %s (%s, %d)"), *EventId.ToString(), *DetailId.ToString(), bFlag ? 1 : 0);
	OnMusicEvent.Broadcast(EventId, WorldContext, DetailId, bFlag);
}

FTDMusicBridgeState UTDMusicEventBridge::GetMusicState() const
{
	FTDMusicBridgeState S;
	if (const UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance()))
	{
		S.Phase = TDMusicBridge::EnumName(StaticEnum<ETDGamePhase>(), (int64)GI->GetProgressRef().Phase);
		S.Destination = TDMusicBridge::EnumName(StaticEnum<ETDDestination>(), (int64)GI->GetCurrentDestination());
	}
	S.bVictory = bVictory;
	S.bVictoryStingerPlayed = bStinger;
	S.bTransmissionActive = bTransmission;
	S.Transmission = Transmission;
	return S;
}

void UTDMusicEventBridge::HandleVictoryBegan(ATDMenuBackdrop* Backdrop)
{
	bVictory = true;
	bStinger = false;
	Emit(VictoryBegan, Backdrop ? (UObject*)Backdrop->GetWorld() : (UObject*)CurrentWorld(), NAME_None, false);
}

void UTDMusicEventBridge::HandleStinger(ATDMenuBackdrop* Backdrop)
{
	bStinger = true;
	Emit(VictoryStinger, Backdrop ? (UObject*)Backdrop->GetWorld() : (UObject*)CurrentWorld(), NAME_None, false);
}

void UTDMusicEventBridge::HandleVictoryEnded(ATDMenuBackdrop* Backdrop)
{
	bVictory = false;
	Emit(VictoryEnded, Backdrop ? (UObject*)Backdrop->GetWorld() : (UObject*)CurrentWorld(), NAME_None, false);
}

void UTDMusicEventBridge::HandleRevealed(ETDDestination Destination)
{
	Emit(PlanetRevealed, CurrentWorld(), TDMusicBridge::EnumName(StaticEnum<ETDDestination>(), (int64)Destination), false);
}

void UTDMusicEventBridge::HandlePhase(ETDGamePhase Phase)
{
	Emit(PhaseChanged, CurrentWorld(), TDMusicBridge::EnumName(StaticEnum<ETDGamePhase>(), (int64)Phase), false);
}

void UTDMusicEventBridge::HandleTransmission(const FTDTransmissionView& View)
{
	bTransmission = true;
	Transmission = View.Id;
	Emit(TransmissionStarted, BoundWorld.Get(), View.Id, false);
}

void UTDMusicEventBridge::HandleTransmissionEnded(FName Id, bool bCompleted)
{
	if (Transmission == Id) { bTransmission = false; Transmission = NAME_None; }
	Emit(TransmissionEnded, BoundWorld.Get(), Id, bCompleted);
}

void UTDMusicEventBridge::HandleLandingState(UTDLandingComponent* Landing, ETDLandingState NewState)
{
	if (NewState != ETDLandingState::Ascending || !Landing) { return; }
	const ATDShip* Ship = Cast<ATDShip>(Landing->GetOwner());
	if (!Ship || !Ship->IsPlayerFlown()) { return; } // the local player's own take-off only
	Emit(Takeoff, Landing->GetWorld(), TDMusicBridge::EnumName(StaticEnum<ETDShipId>(), (int64)Ship->GetShipId()), false);
}

void UTDMusicEventBridge::Tick(float DeltaTime)
{
	UWorld* World = CurrentWorld();
	if (!World) { return; }
	// The story subsystem lives in the world: follow it across level changes (bind once per world, never twice).
	if (BoundWorld.Get() != World)
	{
		if (UTDStorySubsystem* Old = BoundStory.Get())
		{
			Old->OnTransmission.RemoveDynamic(this, &UTDMusicEventBridge::HandleTransmission);
			Old->OnTransmissionEnded.RemoveDynamic(this, &UTDMusicEventBridge::HandleTransmissionEnded);
		}
		BoundStory.Reset();
		BoundWorld = World;
		bTransmission = false;
		Transmission = NAME_None;
	}
	if (!BoundStory.IsValid())
	{
		if (UTDStorySubsystem* Story = UTDStorySubsystem::Get(World))
		{
			Story->OnTransmission.AddUniqueDynamic(this, &UTDMusicEventBridge::HandleTransmission);
			Story->OnTransmissionEnded.AddUniqueDynamic(this, &UTDMusicEventBridge::HandleTransmissionEnded);
			BoundStory = Story;
		}
	}
	// The flown ship's landing component (the flown ship changes on a switch, a respawn, a new map).
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	UTDLandingComponent* Landing = Ship ? Ship->GetLanding() : nullptr;
	if (Landing != BoundLanding.Get())
	{
		if (UTDLandingComponent* Old = BoundLanding.Get()) { Old->OnLandingStateChanged.RemoveDynamic(this, &UTDMusicEventBridge::HandleLandingState); }
		if (Landing) { Landing->OnLandingStateChanged.AddUniqueDynamic(this, &UTDMusicEventBridge::HandleLandingState); }
		BoundLanding = Landing;
	}
}
