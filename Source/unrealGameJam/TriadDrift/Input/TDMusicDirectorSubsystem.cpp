// Triad Drift - combat music director (N-029). Owner: Teammate 1.

#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Core/TDAudioState.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/UnrealType.h"

bool UTDMusicDirectorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

UTDMusicDirectorSubsystem* UTDMusicDirectorSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDMusicDirectorSubsystem>() : nullptr;
}

TStatId UTDMusicDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDMusicDirectorSubsystem, STATGROUP_Tickables);
}

void UTDMusicDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bMenuMap = Cast<ATDMenuGameMode>(InWorld.GetAuthGameMode()) != nullptr; // the menu keeps its own music, alone
	if (UTDEventBus* Bus = UTDEventBus::Get(&InWorld))
	{
		Bus->OnSceneCut.AddUniqueDynamic(this, &UTDMusicDirectorSubsystem::OnSceneCut);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDMusicDirectorSubsystem::OnTakeStarted);
	}
}

void UTDMusicDirectorSubsystem::Deinitialize()
{
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnSceneCut.RemoveDynamic(this, &UTDMusicDirectorSubsystem::OnSceneCut);
		Bus->OnTakeStarted.RemoveDynamic(this, &UTDMusicDirectorSubsystem::OnTakeStarted);
	}
	if (Audio)
	{
		Audio->Stop();
		Audio->DestroyComponent();
		Audio = nullptr;
	}
	Super::Deinitialize();
}

USoundBase* UTDMusicDirectorSubsystem::ResolveMusic() const
{
	if (bUseOverride)
	{
		return MusicOverride;
	}
	// UTDUIStyle::CombatMusic, read by name (the slot lands in Core with the N-029 push).
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	const FSoftObjectProperty* Prop = Style ? CastField<FSoftObjectProperty>(Style->GetClass()->FindPropertyByName(TEXT("CombatMusic"))) : nullptr;
	if (!Prop)
	{
		return nullptr;
	}
	const FSoftObjectPtr& Soft = Prop->GetPropertyValue_InContainer(Style);
	return Soft.IsNull() ? nullptr : Cast<USoundBase>(Soft.LoadSynchronous());
}

bool UTDMusicDirectorSubsystem::IsCombatNow() const
{
	if (CombatOverride.IsSet())
	{
		return CombatOverride.GetValue();
	}
	UWorld* World = GetWorld();
	if (const UTDAudioStateSubsystem* AudioState = UTDAudioStateSubsystem::Get(World))
	{
		const FTDAudioState S = AudioState->GetState();
		if (S.bWaveActive || S.Phase == ETDGamePhase::DominionAssault)
		{
			return true;
		}
	}
	if (World)
	{
		for (TActorIterator<ATDEncounterVolume> It(World); It; ++It)
		{
			if (It->IsActive())
			{
				return true;
			}
		}
	}
	return false;
}

void UTDMusicDirectorSubsystem::StartPlayback()
{
	USoundBase* Music = ResolveMusic();
	UWorld* World = GetWorld();
	if (!Music || !World)
	{
		return;
	}
	if (!Audio)
	{
		// One 2D UI sound (keeps playing in pause and slow motion); the sound's own class and volume apply.
		Audio = UGameplayStatics::CreateSound2D(World, Music, 1.f, 1.f, 0.f, nullptr, false, false);
		if (Audio)
		{
			Audio->bIsUISound = true;
			Audio->bAutoDestroy = false;
		}
	}
	else if (Audio->Sound != Music)
	{
		Audio->SetSound(Music);
	}
	Envelope = 0.f;
	ApplyEnvelope();
	if (Audio)
	{
		Audio->Play(0.f);
	}
	++StartCount;
	MusicState = ETDCombatMusicState::FadingIn;
	StateTime = 0.f;
	UE_LOG(LogTriadDrift, Log, TEXT("TD music: combat music starts (%s)"), *GetNameSafe(Music));
}

void UTDMusicDirectorSubsystem::BeginStop(const TCHAR* Why)
{
	if (MusicState == ETDCombatMusicState::Silent || MusicState == ETDCombatMusicState::Stopping)
	{
		return;
	}
	StopFrom = FMath::Max(Envelope, 0.01f);
	MusicState = ETDCombatMusicState::Stopping;
	StateTime = 0.f;
	UE_LOG(LogTriadDrift, Log, TEXT("TD music: 1 s stop (%s)"), Why);
}

void UTDMusicDirectorSubsystem::ApplyEnvelope()
{
	if (Audio)
	{
		Audio->SetVolumeMultiplier(FMath::Max(Envelope, 0.001f));
	}
}

void UTDMusicDirectorSubsystem::OnSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds) { HandleSceneCut(); }
void UTDMusicDirectorSubsystem::OnTakeStarted(FName SceneId, int32 TakeNumber) { HandleTakeStarted(); }

void UTDMusicDirectorSubsystem::HandleSceneCut()
{
	bWaitForTake = true; // the retake's own fight starts it again, from the top
	BeginStop(TEXT("CUT"));
}

void UTDMusicDirectorSubsystem::HandleTakeStarted()
{
	bWaitForTake = false;
}

void UTDMusicDirectorSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Real seconds: fades keep their length in slow motion.
	const AWorldSettings* WS = World->GetWorldSettings();
	const float Dilation = WS ? FMath::Max(WS->GetEffectiveTimeDilation(), 0.01f) : 1.f;
	const float Dt = DeltaTime / Dilation;

	if (const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(World))
	{
		bTravelling = bTravelling || Travel->HasRequestedLevelTravel();
	}
	if (const UTDGameInstance* GI = Cast<UTDGameInstance>(World->GetGameInstance()))
	{
		bTravelling = bTravelling || GI->IsTravelPending(); // any flow travel: planets, return, quit to menu, victory
	}
	bool bVictory = false;
	if (const UTDAudioStateSubsystem* AudioState = UTDAudioStateSubsystem::Get(World))
	{
		bVictory = AudioState->GetState().Phase == ETDGamePhase::Victory;
	}
	const bool bBlocked = bTravelling || bVictory || bMenuMap || (MusicState == ETDCombatMusicState::Silent && !ResolveMusic());
	bool bCombat = !bBlocked && IsCombatNow();
	if (bWaitForTake && !bCombat)
	{
		bWaitForTake = false; // the old fight is over: the next one may start the music
	}
	bCombat = bCombat && !bWaitForTake;
	if (bBlocked)
	{
		BeginStop(bTravelling ? TEXT("travel") : bVictory ? TEXT("victory") : TEXT("menu"));
	}

	StateTime += Dt;
	switch (MusicState)
	{
	case ETDCombatMusicState::Silent:
		if (bCombat) { StartPlayback(); }
		break;
	case ETDCombatMusicState::FadingIn:
		Envelope = FMath::Min(1.f, Envelope + Dt / FadeInSeconds);
		if (Envelope >= 1.f) { MusicState = ETDCombatMusicState::Playing; StateTime = 0.f; }
		break;
	case ETDCombatMusicState::Playing:
		if (!bCombat) { MusicState = ETDCombatMusicState::Holding; StateTime = 0.f; }
		break;
	case ETDCombatMusicState::Holding:
		if (bCombat) { MusicState = ETDCombatMusicState::Playing; StateTime = 0.f; } // a new fight: carry on
		else if (StateTime >= HoldSeconds) { MusicState = ETDCombatMusicState::FadingOut; StateTime = 0.f; }
		break;
	case ETDCombatMusicState::FadingOut:
		if (bCombat) { MusicState = ETDCombatMusicState::FadingIn; StateTime = 0.f; break; } // swell back up, no restart
		Envelope = FMath::Max(0.f, Envelope - Dt / FadeOutSeconds);
		if (Envelope <= 0.f)
		{
			if (Audio) { Audio->Stop(); }
			MusicState = ETDCombatMusicState::Silent;
		}
		break;
	case ETDCombatMusicState::Stopping:
		Envelope = FMath::Max(0.f, Envelope - Dt * StopFrom / StopSeconds);
		if (Envelope <= 0.f)
		{
			if (Audio) { Audio->Stop(); }
			MusicState = ETDCombatMusicState::Silent;
		}
		break;
	}
	ApplyEnvelope();
}
