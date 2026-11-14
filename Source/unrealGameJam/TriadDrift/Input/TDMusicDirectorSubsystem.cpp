// Triad Drift - combat music director (N-029). Owner: Teammate 1.

#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Input/TDMusicSettings.h"
#include "TriadDrift/Input/TDTutorialMusicSubsystem.h"
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
	if (AmbienceAudio)
	{
		AmbienceAudio->Stop();
		AmbienceAudio->DestroyComponent();
		AmbienceAudio = nullptr;
	}
	Super::Deinitialize();
}

USoundBase* UTDMusicDirectorSubsystem::ResolveMusic() const
{
	if (bUseOverride)
	{
		return MusicOverride;
	}
	// Review M10: found once and held (UPROPERTY); while missing, looked up again every 2 s, never every frame.
	if (CachedMusic || MusicRetry > 0.f)
	{
		return CachedMusic;
	}
	MusicRetry = 2.f;
	++MusicLookups;
	// UTDUIStyle::CombatMusic, read by name (the slot lands in Core with the N-029 push).
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	const FSoftObjectProperty* Prop = Style ? CastField<FSoftObjectProperty>(Style->GetClass()->FindPropertyByName(TEXT("CombatMusic"))) : nullptr;
	if (!Prop)
	{
		return nullptr;
	}
	const FSoftObjectPtr& Soft = Prop->GetPropertyValue_InContainer(Style);
	CachedMusic = Soft.IsNull() ? nullptr : Cast<USoundBase>(Soft.LoadSynchronous());
	return CachedMusic;
}

FName UTDMusicDirectorSubsystem::AmbienceSlotForMap(const FString& MapName)
{
	if (MapName.Contains(TEXT("Verdance"))) { return TEXT("AmbienceVerdance"); }
	if (MapName.Contains(TEXT("Cinder"))) { return TEXT("AmbienceCinder"); }
	if (MapName.Contains(TEXT("Galaxy"))) { return TEXT("AmbienceSpace"); }
	return NAME_None; // the menu, the Dominion interior, test worlds: no bed
}

USoundBase* UTDMusicDirectorSubsystem::ResolveAmbience() const
{
	if (bUseAmbienceOverride)
	{
		return AmbienceOverride;
	}
	const UWorld* World = GetWorld();
	const FName Slot = World ? AmbienceSlotForMap(World->GetMapName()) : NAME_None;
	const UTDUIStyle* Style = Slot.IsNone() ? nullptr : UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	const FSoftObjectProperty* Prop = Style ? CastField<FSoftObjectProperty>(Style->GetClass()->FindPropertyByName(Slot)) : nullptr;
	if (!Prop)
	{
		return nullptr;
	}
	const FSoftObjectPtr& Soft = Prop->GetPropertyValue_InContainer(Style);
	return Soft.IsNull() ? nullptr : Cast<USoundBase>(Soft.LoadSynchronous());
}

void UTDMusicDirectorSubsystem::TickAmbience(float Dt, bool bStop)
{
	UWorld* World = GetWorld();
	AmbienceRetry -= Dt;
	if (!AmbienceBed && !bStop && World && AmbienceRetry <= 0.f)
	{
		AmbienceRetry = 2.f; // the style asset may stream in later; never look it up every frame
		AmbienceBed = ResolveAmbience();
		if (AmbienceBed)
		{
			// The audio component may be absent (no audio device: headless, -nosound); the envelope still runs.
			AmbienceAudio = UGameplayStatics::CreateSound2D(World, AmbienceBed, 1.f, 1.f, 0.f, nullptr, false, false);
			if (AmbienceAudio)
			{
				AmbienceAudio->bIsUISound = true; // under the pause screen too, as the music
				AmbienceAudio->bAutoDestroy = false;
				AmbienceAudio->SetVolumeMultiplier(0.001f);
				AmbienceAudio->Play(0.f);
			}
			UE_LOG(LogTriadDrift, Log, TEXT("TD music: ambience bed %s"), *GetNameSafe(AmbienceBed));
		}
	}
	if (!AmbienceBed)
	{
		return;
	}
	AmbienceEnvelope = FMath::Clamp(AmbienceEnvelope + (bStop ? -Dt : Dt) / AmbienceFadeSeconds, 0.f, 1.f);
	if (AmbienceAudio)
	{
		AmbienceAudio->SetVolumeMultiplier(FMath::Max(AmbienceEnvelope, 0.001f));
		if (bStop && AmbienceEnvelope <= 0.f && AmbienceAudio->IsPlaying())
		{
			AmbienceAudio->Stop();
		}
	}
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
	IntensityGain = TargetIntensityGain(); // silent at the start anyway: begin at the right level
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

float UTDMusicDirectorSubsystem::TargetIntensityGain() const
{
	const UTDMusicSettings* MS = UTDMusicSettings::Get();
	float Intensity = 1.f;
	if (IntensityOverride.IsSet())
	{
		Intensity = IntensityOverride.GetValue();
	}
	else if (const UTDAudioStateSubsystem* AudioState = UTDAudioStateSubsystem::Get(GetWorld()))
	{
		Intensity = AudioState->GetMusicIntensity01();
	}
	return FMath::Lerp(MS->CalmGain, MS->FightGain, FMath::Clamp(Intensity, 0.f, 1.f));
}

void UTDMusicDirectorSubsystem::ApplyEnvelope()
{
	if (Audio)
	{
		Audio->SetVolumeMultiplier(FMath::Max(Envelope * IntensityGain, 0.001f));
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

USoundBase* UTDMusicDirectorSubsystem::ResolveAssaultMusic() const
{
	if (bAssaultOverride) { return AssaultOverride; }
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	return Style && !Style->AssaultMusic.IsNull() ? Style->AssaultMusic.LoadSynchronous() : nullptr;
}

void UTDMusicDirectorSubsystem::TickAssault(UWorld* World, bool bWanted)
{
	if (bWanted && !AssaultVoice && !bAssaultLogical)
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD music: the Dominion assault score starts"));
		AssaultVoice = UGameplayStatics::SpawnSound2D(World, ResolveAssaultMusic(), 1.f, 1.f, 0.f, nullptr, false, false);
		if (AssaultVoice) { AssaultVoice->FadeIn(2.f, 1.f); }
		else { bAssaultLogical = true; } // no audio device (tests): still hold the combat music
	}
	else if (!bWanted && (AssaultVoice || bAssaultLogical))
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD music: the Dominion assault score ends"));
		if (AssaultVoice) { AssaultVoice->FadeOut(3.f, 0.f); }
		AssaultVoice = nullptr;
		bAssaultLogical = false;
	}
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
	// N-039: the tutorial voice (game-instance level) steps with the world; while it plays, the combat music waits.
	UTDTutorialMusicSubsystem* Tutorial = UTDTutorialMusicSubsystem::Get(World);
	if (Tutorial)
	{
		// Issue #24: the tutorial voice is timed against the intro, which runs on unclamped real time; the world's delta
		// is clamped on hitch frames (MaxUndilatedFrameTime), so step it on the larger of the two.
		Tutorial->Step(World, FMath::Max(Dt, World->DeltaRealTimeSeconds), bMenuMap);
	}
	const bool bTutorialHolds = Tutorial && Tutorial->IsHoldingCombatMusic();
	// N-048: the Dominion assault's own score, from wave 5's clear to docking or the reactor's end; combat music waits.
	bool bAssaultPhase = false;
	if (const UTDGameInstance* GI = Cast<UTDGameInstance>(World->GetGameInstance()))
	{
		bAssaultPhase = GI->GetProgressRef().Phase == ETDGamePhase::DominionAssault;
	}
	TickAssault(World, bAssaultPhase && !bTravelling && !bVictory && !bMenuMap && ResolveAssaultMusic() != nullptr);
	const bool bAssaultHolds = IsAssaultMusicPlaying();
	const bool bBlocked = bTravelling || bVictory || bMenuMap || bTutorialHolds || bAssaultHolds || (MusicState == ETDCombatMusicState::Silent && !ResolveMusic());
	bool bCombat = !bBlocked && IsCombatNow();
	if (bWaitForTake && !bCombat)
	{
		bWaitForTake = false; // the old fight is over: the next one may start the music
	}
	bCombat = bCombat && !bWaitForTake;
	if (bBlocked)
	{
		BeginStop(bTravelling ? TEXT("travel") : bVictory ? TEXT("victory") : bTutorialHolds ? TEXT("tutorial music") : bAssaultHolds ? TEXT("assault music") : TEXT("menu"));
	}
	TickAmbience(Dt, bTravelling || bVictory || bMenuMap);

	MusicRetry = FMath::Max(0.f, MusicRetry - Dt);
	// N-037: the fight's intensity sets the music's level, rate-limited on top of the intensity's own smoothing.
	IntensityGain = FMath::FInterpConstantTo(IntensityGain, TargetIntensityGain(), Dt, UTDMusicSettings::Get()->MaxGainRatePerSecond);

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
