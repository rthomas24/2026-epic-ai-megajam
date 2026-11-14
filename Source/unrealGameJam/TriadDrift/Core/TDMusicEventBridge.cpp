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
#include "EngineUtils.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "UObject/UObjectGlobals.h"

const FName UTDMusicEventBridge::VictoryBegan(TEXT("VictoryBegan"));
const FName UTDMusicEventBridge::VictoryStinger(TEXT("VictoryStinger"));
const FName UTDMusicEventBridge::VictoryEnded(TEXT("VictoryEnded"));
const FName UTDMusicEventBridge::PlanetRevealed(TEXT("PlanetRevealed"));
const FName UTDMusicEventBridge::Takeoff(TEXT("Takeoff"));
const FName UTDMusicEventBridge::PhaseChanged(TEXT("PhaseChanged"));
const FName UTDMusicEventBridge::TransmissionStarted(TEXT("TransmissionStarted"));
const FName UTDMusicEventBridge::TransmissionEnded(TEXT("TransmissionEnded"));
const FName UTDMusicEventBridge::VictoryStingerDone(TEXT("VictoryStingerDone"));
const FName UTDMusicEventBridge::GroundCombat(TEXT("GroundCombat"));
const FName UTDMusicEventBridge::GroundAllClear(TEXT("GroundAllClear"));

const TArray<FName>& UTDMusicEventBridge::StingerLines()
{
	static const TArray<FName> Lines = { TEXT("A5_DistressSignal"), TEXT("C9_Kato_Identifiers"), TEXT("C9_Wren_Source"), TEXT("C9_Kato_Decades") };
	return Lines;
}

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
	PreLoadHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UTDMusicEventBridge::HandlePreLoadMap);
}

void UTDMusicEventBridge::HandlePreLoadMap(const FString& MapName)
{
	bGroundFight = false; // a new map starts in exploration (no event: PlanetRevealed / PhaseChanged follow)
	bRevealed = false; // the next map starts under its cover (or has none: the galaxy reveal sets it again)
	bParked = false;
}

void UTDMusicEventBridge::Deinitialize()
{
	ATDMenuBackdrop::OnVictoryBegan.Remove(VictoryBeganHandle);
	ATDMenuBackdrop::OnStinger.Remove(StingerHandle);
	ATDMenuBackdrop::OnVictoryEnded.Remove(VictoryEndedHandle);
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadHandle);
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
	S.bPlanetRevealed = bRevealed;
	S.bLanded = bParked;
	S.bVictoryStingerDone = bStingerDone;
	S.bGroundCombat = bGroundFight;
	return S;
}

void UTDMusicEventBridge::HandleVictoryBegan(ATDMenuBackdrop* Backdrop)
{
	bVictory = true;
	bStinger = false;
	bStingerDone = false;
	bStingerWaiting = false;
	StingerPending.Reset();
	Emit(VictoryBegan, Backdrop ? (UObject*)Backdrop->GetWorld() : (UObject*)CurrentWorld(), NAME_None, false);
}

void UTDMusicEventBridge::HandleStinger(ATDMenuBackdrop* Backdrop)
{
	bStinger = true;
	bStingerDone = false;
	StingerPending = StingerLines();
	bStingerWaiting = true; // the backdrop queues the lines right after this broadcast; settled from the next tick
	Emit(VictoryStinger, Backdrop ? (UObject*)Backdrop->GetWorld() : (UObject*)CurrentWorld(), NAME_None, false);
}

void UTDMusicEventBridge::HandleVictoryEnded(ATDMenuBackdrop* Backdrop)
{
	if (bStingerWaiting) { FinishStinger(false); } // the screen closed before the last line: done, cut short
	bVictory = false;
	Emit(VictoryEnded, Backdrop ? (UObject*)Backdrop->GetWorld() : (UObject*)CurrentWorld(), NAME_None, false);
}

void UTDMusicEventBridge::FinishStinger(bool bCompleted)
{
	bStingerWaiting = false;
	bStingerDone = true;
	StingerPending.Reset();
	Emit(VictoryStingerDone, CurrentWorld(), NAME_None, bCompleted);
}

void UTDMusicEventBridge::TickStinger(UWorld* World)
{
	if (!bStingerWaiting) { return; }
	// A line is over when it has aired and is not on air now; a line that never fires (heard in an earlier run) counts as
	// aired (UTDStorySubsystem::HasAired), so nothing waits for a line that won't come.
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(World);
	if (!Story) { FinishStinger(false); return; }
	const FTDTransmissionView Now = Story->GetCurrentTransmission();
	StingerPending.RemoveAll([&](const FName& Id) { return Story->HasAired(Id) && !(Now.bActive && Now.Id == Id); });
	if (StingerPending.Num() == 0) { FinishStinger(true); }
}

void UTDMusicEventBridge::TickGround(UWorld* World)
{
	// Read-only: any running crew encounter on this planet map (T5's ATDEncounterVolume) is on-foot combat.
	const UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance());
	const ETDDestination Dest = GI ? GI->GetCurrentDestination() : ETDDestination::Galaxy;
	const bool bPlanet = Dest == ETDDestination::Verdance || Dest == ETDDestination::Cinder;
	bool bFight = false;
	if (bPlanet)
	{
		for (TActorIterator<ATDEncounterVolume> It(World); It && !bFight; ++It) { bFight = It->IsActive(); }
	}
	if (bFight != bGroundFight)
	{
		bGroundFight = bFight;
		Emit(bFight ? GroundCombat : GroundAllClear, World, TDMusicBridge::EnumName(StaticEnum<ETDDestination>(), (int64)Dest), false);
	}
}

void UTDMusicEventBridge::HandleRevealed(ETDDestination Destination)
{
	bRevealed = true;
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
	if (!Landing) { return; }
	const ATDShip* Ship = Cast<ATDShip>(Landing->GetOwner());
	if (!Ship || !Ship->IsPlayerFlown()) { return; } // the local player's own landing and take-off only
	if (NewState == ETDLandingState::Landed) { bParked = true; }
	if (NewState == ETDLandingState::Flying || NewState == ETDLandingState::EnginesOn || NewState == ETDLandingState::Ascending) { bParked = false; }
	if (NewState != ETDLandingState::Ascending) { return; }
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
	TickStinger(World);
	TickGround(World);
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
