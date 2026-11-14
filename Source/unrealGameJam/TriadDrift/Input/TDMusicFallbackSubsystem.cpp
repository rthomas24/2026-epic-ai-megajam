// Triad Drift - the engineering music fallback (R-047; T1). See the header for the placements and the rules.

#include "TriadDrift/Input/TDMusicFallbackSubsystem.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Core/TDMusicEventBridge.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

static TAutoConsoleVariable<int32> CVarTDMusicFallback(TEXT("td.Music.Fallback"), 1,
	TEXT("R-047: 1 = the engineering music fallback plays Nicholas's assigned placements (credits, Cinder, core chamber, reactor fight, and the assault / idle beds when DA_UI_Style leaves them empty); 0 = all of it off."));

namespace TDMusicFallback
{
	FSoftObjectPath P(const TCHAR* Folder, const TCHAR* Asset)
	{
		return FSoftObjectPath(FString::Printf(TEXT("/Game/TriadDrift/Audio/%s/%s.%s"), Folder, Asset, Asset));
	}
}

bool UTDMusicFallbackSubsystem::IsEnabled() { return CVarTDMusicFallback.GetValueOnGameThread() != 0; }

const TArray<FSoftObjectPath>& UTDMusicFallbackSubsystem::TracksFor(ETDMusicPlacement Placement)
{
	using namespace TDMusicFallback;
	static const TArray<FSoftObjectPath> Credits = { P(TEXT("CreditsMusic01"), TEXT("MUS_Credits_CathedralPulse_Full_01")), P(TEXT("CreditsMusic01"), TEXT("MUS_Credits_EtherealPulseRemix_Full_02")) };
	static const TArray<FSoftObjectPath> CinderExplore = { P(TEXT("CinderMusic01"), TEXT("MUS_Cinder_Exploration_World_Full_01")) };
	static const TArray<FSoftObjectPath> CinderCombat = { P(TEXT("CinderMusic01"), TEXT("MUS_Cinder_Combat_Fall_Full_01")) };
	static const TArray<FSoftObjectPath> Core = { P(TEXT("DominionCore01"), TEXT("MUS_DominionCore_CathedralPulse_Full_01")) };
	static const TArray<FSoftObjectPath> Reactor = { P(TEXT("DominionAssault01"), TEXT("MUS_DominionAssault_InfinityZone_Full_01")) };
	static const TArray<FSoftObjectPath> Verdance = { P(TEXT("VerdanceTheme01"), TEXT("MUS_Verdance_Main_Full_01")) };
	static const TArray<FSoftObjectPath> None; // the escape's Ritual is not imported yet
	switch (Placement)
	{
	case ETDMusicPlacement::Credits: return Credits;
	case ETDMusicPlacement::CinderExplore: return CinderExplore;
	case ETDMusicPlacement::CinderCombat: return CinderCombat;
	case ETDMusicPlacement::CoreChamber: return Core;
	case ETDMusicPlacement::ReactorFight: return Reactor;
	case ETDMusicPlacement::VerdanceExplore: return Verdance;
	default: return None;
	}
}

bool UTDMusicFallbackSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

UTDMusicFallbackSubsystem* UTDMusicFallbackSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDMusicFallbackSubsystem>() : nullptr;
}

TStatId UTDMusicFallbackSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDMusicFallbackSubsystem, STATGROUP_Tickables); }

void UTDMusicFallbackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTDMusicFallbackSubsystem::Deinitialize()
{
	if (UTDMusicEventBridge* Bridge = UTDMusicEventBridge::GetMusicEventBridge(GetWorld()))
	{
		Bridge->OnMusicEvent.RemoveDynamic(this, &UTDMusicFallbackSubsystem::HandleBridgeEvent);
	}
	StopVoice(0.f);
	Super::Deinitialize();
}

FSoftObjectPath UTDMusicFallbackSubsystem::GetPlayingTrack() const
{
	const TArray<FSoftObjectPath>& Tracks = TracksFor(Playing);
	return Tracks.IsValidIndex(TrackIndex) ? Tracks[TrackIndex] : FSoftObjectPath();
}

void UTDMusicFallbackSubsystem::HandleBridgeEvent(FName EventId, UObject* WorldContext, FName DetailId, bool bFlag)
{
	const UWorld* From = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (From && From != GetWorld()) { return; } // another world's moment
	if (EventId == UTDMusicEventBridge::VictoryBegan) { EventPlacement = ETDMusicPlacement::None; }
	else if (EventId == UTDMusicEventBridge::VictoryStingerDone) { EventPlacement = ETDMusicPlacement::Credits; }
	else if (EventId == UTDMusicEventBridge::VictoryEnded) { if (EventPlacement == ETDMusicPlacement::Credits) { EventPlacement = ETDMusicPlacement::None; } }
	else if (EventId == UTDMusicEventBridge::GroundCombat) { bGroundFight = true; }
	else if (EventId == UTDMusicEventBridge::Takeoff) { bTakenOff = true; }
	else if (EventId == UTDMusicEventBridge::GroundAllClear) { bGroundFight = false; }
	else if (EventId == UTDMusicEventBridge::CoreChamberEntered) { EventPlacement = ETDMusicPlacement::CoreChamber; }
	else if (EventId == UTDMusicEventBridge::ReactorFightStarted) { EventPlacement = ETDMusicPlacement::ReactorFight; }
	else if (EventId == UTDMusicEventBridge::ReactorFightEnded) { if (EventPlacement == ETDMusicPlacement::ReactorFight) { EventPlacement = ETDMusicPlacement::None; } }
	else if (EventId == UTDMusicEventBridge::EscapeStarted) { EventPlacement = ETDMusicPlacement::Escape; }
}

ETDMusicPlacement UTDMusicFallbackSubsystem::ComputeWanted() const
{
	if (EventPlacement != ETDMusicPlacement::None) { return EventPlacement; }
	// Cinder, from the bridge's snapshot: after the reveal, exploration; a crew encounter running, combat.
	if (const UTDMusicEventBridge* Bridge = UTDMusicEventBridge::GetMusicEventBridge(GetWorld()))
	{
		const FTDMusicBridgeState S = Bridge->GetMusicState();
		if (S.Destination == FName(TEXT("Cinder")) && S.bPlanetRevealed)
		{
			return (S.bGroundCombat || bGroundFight) ? ETDMusicPlacement::CinderCombat : ETDMusicPlacement::CinderExplore;
		}
		// Verdance, after the reveal: Nick's theme (N-059) as a fallback; his own Verdance player's claim stands it down.
		if (S.Destination == FName(TEXT("Verdance")) && S.bPlanetRevealed && !bTakenOff)
		{
			return ETDMusicPlacement::VerdanceExplore;
		}
	}
	return ETDMusicPlacement::None;
}

void UTDMusicFallbackSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) { return; }
	if (!bBound)
	{
		if (UTDMusicEventBridge* Bridge = UTDMusicEventBridge::GetMusicEventBridge(World))
		{
			Bridge->OnMusicEvent.AddUniqueDynamic(this, &UTDMusicFallbackSubsystem::HandleBridgeEvent);
			bBound = true;
		}
	}
	const ETDMusicPlacement Was = Wanted;
	Wanted = ComputeWanted();
	if (Wanted != Was && FinishedPlacement != Wanted) { FinishedPlacement = ETDMusicPlacement::None; }

	const UTDMusicDirectorSubsystem* Director = UTDMusicDirectorSubsystem::Get(World);
	const bool bClaimed = Director && Director->IsExclusiveScoreHeld();
	bool bTravelling = false;
	if (const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(World)) { bTravelling = Travel->HasRequestedLevelTravel(); }
	if (const UTDGameInstance* GI = Cast<UTDGameInstance>(World->GetGameInstance())) { bTravelling = bTravelling || GI->IsTravelPending(); }
	const bool bAllowed = IsEnabled() && !bClaimed && !bTravelling && Wanted != ETDMusicPlacement::None && Wanted != FinishedPlacement
		&& TracksFor(Wanted).Num() > 0;
	const ETDMusicPlacement Target = bAllowed ? Wanted : ETDMusicPlacement::None;
	if (Target != Playing)
	{
		if (Playing != ETDMusicPlacement::None)
		{
			const bool bStandDown = bClaimed || bTravelling || !IsEnabled();
			UE_LOG(LogTriadDrift, Log, TEXT("TD music fallback: %s ends (%s)"), *UEnum::GetValueAsString(Playing),
				bClaimed ? TEXT("another score claims the music") : bTravelling ? TEXT("travel") : !IsEnabled() ? TEXT("td.Music.Fallback 0") : TEXT("the moment is over"));
			StopVoice(bStandDown ? StandDownSeconds : FadeOutSeconds);
			Playing = ETDMusicPlacement::None;
		}
		if (Target != ETDMusicPlacement::None) { StartTrack(Target, 0); }
	}
}

void UTDMusicFallbackSubsystem::StartTrack(ETDMusicPlacement Placement, int32 Index)
{
	const TArray<FSoftObjectPath>& Tracks = TracksFor(Placement);
	if (!Tracks.IsValidIndex(Index)) { return; }
	USoundBase* Sound = Cast<USoundBase>(Tracks[Index].TryLoad());
	if (!Sound)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("TD music fallback: %s is missing (%s): nothing plays"), *Tracks[Index].ToString(), *UEnum::GetValueAsString(Placement));
		FinishedPlacement = Placement;
		return;
	}
	StopVoice(0.f);
	Playing = Placement;
	TrackIndex = Index;
	// Unity gain, the asset's own class and looping; a plain fade in.
	Voice = UGameplayStatics::SpawnSound2D(GetWorld(), Sound, 1.f, 1.f, 0.f, nullptr, false, false);
	if (Voice)
	{
		Voice->OnAudioFinished.AddUniqueDynamic(this, &UTDMusicFallbackSubsystem::HandleFinished);
		Voice->FadeIn(Index == 0 ? FadeInSeconds : 0.f, 1.f);
	}
	UE_LOG(LogTriadDrift, Log, TEXT("TD music fallback: %s plays %s%s"), *UEnum::GetValueAsString(Placement), *Sound->GetName(), Voice ? TEXT("") : TEXT(" (no audio device: logical)"));
}

void UTDMusicFallbackSubsystem::HandleFinished()
{
	if (Playing == ETDMusicPlacement::None) { return; }
	const ETDMusicPlacement Done = Playing;
	if (TracksFor(Done).IsValidIndex(TrackIndex + 1))
	{
		StartTrack(Done, TrackIndex + 1); // the next track of a sequence (the credits' second)
		return;
	}
	// Played once to its end (not a looping asset): the existing bed returns; it doesn't start again for this moment.
	UE_LOG(LogTriadDrift, Log, TEXT("TD music fallback: %s finished"), *UEnum::GetValueAsString(Done));
	StopVoice(0.f);
	FinishedPlacement = Done;
	Playing = ETDMusicPlacement::None;
}

void UTDMusicFallbackSubsystem::StopVoice(float FadeSeconds)
{
	if (!Voice) { return; }
	Voice->OnAudioFinished.RemoveAll(this); // a fade-out's end is not the track's end
	if (FadeSeconds > 0.f) { Voice->FadeOut(FadeSeconds, 0.f); }
	else { Voice->Stop(); }
	Voice = nullptr;
}
