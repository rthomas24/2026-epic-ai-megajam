// Triad Drift - tutorial music (N-039, GitHub issue #24). Owner: Teammate 1.

#include "TriadDrift/Input/TDTutorialMusicSubsystem.h"
#include "TriadDrift/Input/TDMusicSettings.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Quartz/QuartzSubsystem.h"
#include "Sound/SoundBase.h"

void UTDTutorialMusicSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTDTutorialMusicSubsystem::Deinitialize()
{
	StopVoice();
	Super::Deinitialize();
}

UTDTutorialMusicSubsystem* UTDTutorialMusicSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UTDTutorialMusicSubsystem>() : nullptr;
}

void UTDTutorialMusicSubsystem::ArmForNewRun()
{
	StopVoice();
	State = ETDTutorialMusicState::Armed;
	Clock = 0.f;
	PreludeStartClock = LoopStartClock = LoopScheduledClock = ControlClock = PreludeIntroTime = -1.f;
	PreludeStartOffset = 0.f;
	FadeElapsed = 0.f;
	Volume = 1.f;
	bPreludeStarted = false;
	Intro.Reset();
	UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: armed for a new run"));
}

void UTDTutorialMusicSubsystem::Stop(const TCHAR* Why)
{
	if (State != ETDTutorialMusicState::Off)
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: stopped (%s)"), Why);
	}
	StopVoice();
	State = ETDTutorialMusicState::Off;
}

void UTDTutorialMusicSubsystem::SetSoundsForTests(USoundBase* Prelude, USoundBase* Loop, float InPreludeSeconds, float InFadeSeconds)
{
	bTestSounds = true;
	TestPrelude = Prelude;
	TestLoop = Loop;
	TestPreludeSeconds = InPreludeSeconds;
	TestFadeSeconds = InFadeSeconds;
}

bool UTDTutorialMusicSubsystem::OwnsOpeningMusic() const
{
	if (State != ETDTutorialMusicState::Armed && State != ETDTutorialMusicState::Prelude && State != ETDTutorialMusicState::Loop)
	{
		return false;
	}
	// Tests set their (possibly null) sounds explicitly: that counts as configured.
	return bTestSounds || PreludeSound() || LoopSound();
}

bool UTDTutorialMusicSubsystem::ShouldIntroPlayIntroMusic(const UObject* WorldContext)
{
	const UTDTutorialMusicSubsystem* T = Get(WorldContext);
	if (T && T->OwnsOpeningMusic())
	{
		UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: owns the opening; the intro's IntroMusic is not started"));
		return false;
	}
	return true;
}

USoundBase* UTDTutorialMusicSubsystem::PreludeSound() const
{
	if (bTestSounds) { return TestPrelude; }
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	return Style && !Style->TutorialPrelude.IsNull() ? Style->TutorialPrelude.LoadSynchronous() : nullptr;
}

USoundBase* UTDTutorialMusicSubsystem::LoopSound() const
{
	if (bTestSounds) { return TestLoop; }
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	return Style && !Style->TutorialLoop.IsNull() ? Style->TutorialLoop.LoadSynchronous() : nullptr;
}

float UTDTutorialMusicSubsystem::PreludeSeconds() const
{
	if (bTestSounds) { return TestPreludeSeconds; }
	const USoundBase* S = PreludeSound();
	return S ? S->GetDuration() : 0.f;
}

float UTDTutorialMusicSubsystem::FadeSeconds() const
{
	if (bTestSounds) { return TestFadeSeconds; }
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	return Style ? Style->TutorialFadeOutSeconds : 20.f;
}

void UTDTutorialMusicSubsystem::HandleIntroFinished(bool bPlayed, bool bSkipped)
{
	if (State != ETDTutorialMusicState::Armed && State != ETDTutorialMusicState::Prelude)
	{
		return;
	}
	ControlClock = Clock;
	if (State == ETDTutorialMusicState::Prelude && !bSkipped && bLoopQueued)
	{
		// Watched: the loop was queued for the prelude's last sample; it carries on, nothing restarts here.
		LoopStartClock = LoopScheduledClock;
		if (!bLoopOnAudioClock && Voice && !Voice->IsPlaying())
		{
			Voice->Play(0.f); // audio device without Quartz: the old game-frame start
		}
	}
	else
	{
		// Skipped, or no intro: a prelude still playing is cut; the loop's first beat is on the control frame.
		StartVoice(LoopSound(), 0.f, BoundWorld.Get());
		LoopStartClock = Clock;
	}
	State = ETDTutorialMusicState::Loop;
	UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: loop at control (%s)"), !bPlayed ? TEXT("no intro") : bSkipped ? TEXT("intro skipped") : TEXT("intro watched"));
}

void UTDTutorialMusicSubsystem::HandleWaveCleared(int32 WaveNumber)
{
	if (WaveNumber >= UTDMusicSettings::Get()->TutorialEndsAfterWave && IsHoldingCombatMusic())
	{
		BeginFade();
	}
}

void UTDTutorialMusicSubsystem::BeginFade()
{
	State = ETDTutorialMusicState::FadingOut;
	FadeElapsed = 0.f;
	UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: tutorial done, %.0f s fade"), FadeSeconds());
}

void UTDTutorialMusicSubsystem::Step(UWorld* World, float Dt, bool bMenuMap)
{
	if (State == ETDTutorialMusicState::Off || !World)
	{
		return;
	}
	Clock += Dt;
	if (bMenuMap)
	{
		// New Game arms it from the menu: only a tutorial that has started stops there (quit to menu).
		if (State != ETDTutorialMusicState::Armed)
		{
			Stop(TEXT("menu"));
		}
		return;
	}
	// Listen to this world's waves (the voice outlives worlds; the binding follows the current one).
	if (BoundWorld.Get() != World)
	{
		if (UTDEventBus* Old = BoundWorld.IsValid() ? UTDEventBus::Get(BoundWorld.Get()) : nullptr)
		{
			Old->OnWaveCleared.RemoveDynamic(this, &UTDTutorialMusicSubsystem::HandleWaveCleared);
		}
		if (UTDEventBus* Bus = UTDEventBus::Get(World))
		{
			Bus->OnWaveCleared.AddUniqueDynamic(this, &UTDTutorialMusicSubsystem::HandleWaveCleared);
		}
		if (UTDIntroDirector* Here = UTDIntroDirector::Get(World))
		{
			Here->OnIntroFinished.AddUniqueDynamic(this, &UTDTutorialMusicSubsystem::HandleIntroFinished);
		}
		BoundWorld = World;
	}

	if (State == ETDTutorialMusicState::Armed)
	{
		// The prelude ends on the control frame: it starts at (intro length - prelude length), 11.0 s into the 49 s
		// intro for a 38 s prelude, joining at the right offset if that point fell between two frames.
		UTDIntroDirector* Director = UTDIntroDirector::Get(World);
		const float Len = PreludeSeconds();
		if (Director && Director->IsPlaying() && !Director->WasSkipped() && Len > 0.f && !bPreludeStarted)
		{
			const float StartAt = Director->GetIntroLength() - Len;
			if (Director->GetIntroTime() >= StartAt)
			{
				bPreludeStarted = true;
				PreludeStartOffset = FMath::Max(0.f, Director->GetIntroTime() - StartAt);
				PreludeIntroTime = Director->GetIntroTime();
				PreludeStartClock = Clock;
				StartPreludeAndQueueLoop(World, PreludeStartOffset);
				State = ETDTutorialMusicState::Prelude;
			}
		}
	}

	// The prelude ends on its own on the loop's first sample: release it afterwards.
	if (PreludeVoice && !PreludeVoice->IsPlaying() && State != ETDTutorialMusicState::Prelude)
	{
		PreludeVoice->DestroyComponent();
		PreludeVoice = nullptr;
	}

	if (State == ETDTutorialMusicState::FadingOut)
	{
		FadeElapsed += Dt;
		const float Fade = FMath::Max(FadeSeconds(), 0.01f);
		Volume = FMath::Clamp(1.f - FadeElapsed / Fade, 0.f, 1.f);
		if (Voice)
		{
			Voice->SetVolumeMultiplier(FMath::Max(Volume, 0.001f));
		}
		if (Volume <= 0.f)
		{
			Stop(TEXT("fade complete"));
		}
	}
}

void UTDTutorialMusicSubsystem::StartVoice(USoundBase* Sound, float StartSeconds, UWorld* World)
{
	StopVoice(); // never two voices
	++VoicesStarted;
	Volume = 1.f;
	if (!Sound || !World)
	{
		return; // no asset (or no audio device): the timing still runs
	}
	// Persists across level travel (owned by the audio device, not the world), kept alive by this UPROPERTY.
	Voice = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, StartSeconds, nullptr, /*bPersistAcrossLevelTransition*/ true, /*bAutoDestroy*/ false);
	if (Voice)
	{
		Voice->Play(StartSeconds);
	}
}

void UTDTutorialMusicSubsystem::StartPreludeAndQueueLoop(UWorld* World, float Offset)
{
	StopVoice();
	VoicesStarted += 2; // the prelude, and the loop queued behind it (it is not started again at control)
	Volume = 1.f;
	const float Remaining = FMath::Max(PreludeSeconds() - Offset, 0.01f);
	LoopScheduledClock = Clock + Remaining;
	bLoopQueued = true;
	bLoopOnAudioClock = false;
	USoundBase* Prelude = PreludeSound();
	USoundBase* Loop = LoopSound();
	if (!World || (!Prelude && !Loop))
	{
		return; // no asset (or tests): the timing still runs
	}
	// Both persist across level travel (owned by the audio device), kept alive by these UPROPERTYs.
	PreludeVoice = Prelude ? UGameplayStatics::CreateSound2D(World, Prelude, 1.f, 1.f, Offset, nullptr, true, false) : nullptr;
	Voice = Loop ? UGameplayStatics::CreateSound2D(World, Loop, 1.f, 1.f, 0.f, nullptr, true, false) : nullptr;
	if (!PreludeVoice && !Voice)
	{
		return; // no audio device (-nosound, tests)
	}

	// One audio clock whose single tick is the prelude's remaining length: the prelude fires on the clock's start, the
	// loop on its first tick, both on the audio render thread, so the loop's first sample follows the prelude's last.
	UQuartzSubsystem* Quartz = UQuartzSubsystem::Get(World);
	FQuartzClockSettings Settings;
	Settings.bIgnoreLevelChange = true;
	AudioClock = Quartz ? Quartz->CreateNewClock(World, TEXT("TDTutorialMusic"), Settings, /*bOverride*/ true, /*bUseAudioEngineClockManager*/ true) : nullptr;
	if (UQuartzClockHandle* Clk = AudioClock.Get())
	{
		const FQuartzQuantizationBoundary Now(EQuartzCommandQuantization::None);
		Clk->SetSecondsPerTick(World, Now, FOnQuartzCommandEventBP(), Clk, Remaining);
		if (PreludeVoice)
		{
			FQuartzQuantizationBoundary AtStart(EQuartzCommandQuantization::Tick, 1.f, EQuarztQuantizationReference::TransportRelative, /*bFireOnClockStart*/ true);
			PreludeVoice->PlayQuantized(World, Clk, AtStart, FOnQuartzCommandEventBP(), Offset);
		}
		if (Voice)
		{
			FQuartzQuantizationBoundary AtEnd(EQuartzCommandQuantization::Tick, 1.f, EQuarztQuantizationReference::TransportRelative, /*bFireOnClockStart*/ false);
			Voice->PlayQuantized(World, Clk, AtEnd, FOnQuartzCommandEventBP(), 0.f);
		}
		Clk->StartClock(World, Clk);
		bLoopOnAudioClock = Voice != nullptr;
		UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: prelude at %.2f s, loop queued on the audio clock in %.3f s"), Offset, Remaining);
	}
	else if (PreludeVoice)
	{
		PreludeVoice->Play(Offset); // no Quartz: the loop starts on the control frame (HandleIntroFinished)
	}
}

void UTDTutorialMusicSubsystem::StopVoice()
{
	for (TObjectPtr<UAudioComponent>* Slot : { &Voice, &PreludeVoice })
	{
		if (UAudioComponent* C = Slot->Get())
		{
			C->Stop();
			C->DestroyComponent();
		}
		*Slot = nullptr;
	}
	if (UQuartzClockHandle* Clk = AudioClock.Get())
	{
		if (UWorld* World = BoundWorld.Get())
		{
			Clk->StopClock(World, /*CancelPendingEvents*/ true, Clk);
			if (UQuartzSubsystem* Quartz = UQuartzSubsystem::Get(World))
			{
				Quartz->DeleteClockByHandle(World, Clk);
			}
		}
		AudioClock = nullptr;
	}
	bLoopQueued = false;
	bLoopOnAudioClock = false;
}

int32 UTDTutorialMusicSubsystem::GetLiveVoiceCount() const
{
	const int32 Playing = (Voice && Voice->IsPlaying() ? 1 : 0) + (PreludeVoice && PreludeVoice->IsPlaying() ? 1 : 0);
	if (Playing > 0)
	{
		return Playing;
	}
	// No audio device (tests, -nosound): the logical voice.
	return (State == ETDTutorialMusicState::Prelude || State == ETDTutorialMusicState::Loop || State == ETDTutorialMusicState::FadingOut) ? 1 : 0;
}

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Flow/TDGameInstance.h"

namespace TDTutorialMusicDebug
{
	// Audio recording (N-039): a New Game straight into the intro from any map, as the menu's NEW GAME does.
	static FAutoConsoleCommandWithWorldAndArgs GNewGame(TEXT("TD.Tutorial.NewGame"),
		TEXT("Start a New Game into the intro (as the menu's NEW GAME): arms the tutorial music."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			UTDGameInstance* GI = UTDGameInstance::Get(World);
			if (!GI) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Tutorial.NewGame: no Triad Drift game instance")); return; }
			GI->StartNewRun();
			const bool bOk = GI->TravelTo(ETDDestination::Galaxy, NAME_None);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Tutorial.NewGame: %s"), bOk ? TEXT("travelling to the galaxy, intro next") : TEXT("travel refused"));
		}));

	static FAutoConsoleCommandWithWorldAndArgs GStatus(TEXT("TD.Tutorial.Status"),
		TEXT("Log the tutorial music: state, prelude / control / loop times, fade volume, voices."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			const UTDTutorialMusicSubsystem* T = UTDTutorialMusicSubsystem::Get(World);
			if (!T) { UE_LOG(LogTriadDrift, Log, TEXT("TD.Tutorial.Status: no subsystem")); return; }
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Tutorial.Status: %s clock %.2f | prelude at intro %.2f s (clock %.2f, offset %.2f) | control %.2f loop %.2f (queued for %.2f, audio clock %d) | volume %.2f | voices started %d live %d"),
				*UEnum::GetValueAsString(T->GetState()), T->GetClock(), T->GetPreludeIntroTime(), T->GetPreludeStartClock(), T->GetPreludeStartOffset(),
				T->GetControlClock(), T->GetLoopStartClock(), T->GetLoopScheduledClock(), T->IsLoopOnAudioClock() ? 1 : 0, T->GetFadeVolume(), T->GetVoicesStarted(), T->GetLiveVoiceCount());
		}));
}
#endif
