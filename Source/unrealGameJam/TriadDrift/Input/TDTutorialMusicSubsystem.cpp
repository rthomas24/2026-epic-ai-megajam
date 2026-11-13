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
#include "Sound/SoundBase.h"

void UTDTutorialMusicSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	IntroStartedHandle = UTDIntroDirector::OnAnyIntroStarted.AddUObject(this, &UTDTutorialMusicSubsystem::HandleIntroStarted);
	IntroFinishedHandle = UTDIntroDirector::OnAnyIntroFinished.AddUObject(this, &UTDTutorialMusicSubsystem::HandleIntroFinished);
}

void UTDTutorialMusicSubsystem::Deinitialize()
{
	UTDIntroDirector::OnAnyIntroStarted.Remove(IntroStartedHandle);
	UTDIntroDirector::OnAnyIntroFinished.Remove(IntroFinishedHandle);
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
	PreludeStartClock = LoopStartClock = ControlClock = PreludeIntroTime = -1.f;
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

void UTDTutorialMusicSubsystem::HandleIntroStarted(UTDIntroDirector* InIntro)
{
	if (State == ETDTutorialMusicState::Armed && InIntro && InIntro->GetWorld() && InIntro->GetWorld()->GetGameInstance() == GetGameInstance())
	{
		Intro = InIntro;
	}
}

void UTDTutorialMusicSubsystem::HandleIntroFinished(UTDIntroDirector* InIntro, bool bSkipped)
{
	if (!InIntro || !InIntro->GetWorld() || InIntro->GetWorld()->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	if (State != ETDTutorialMusicState::Armed && State != ETDTutorialMusicState::Prelude)
	{
		return;
	}
	// The control frame: the loop's first beat. A prelude still playing (a skip) is cut.
	ControlClock = Clock;
	StartVoice(LoopSound(), 0.f, InIntro->GetWorld());
	LoopStartClock = Clock;
	State = ETDTutorialMusicState::Loop;
	UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: loop at control (%s intro)"), bSkipped ? TEXT("skipped") : TEXT("watched"));
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
		Stop(TEXT("menu"));
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
		BoundWorld = World;
	}

	if (State == ETDTutorialMusicState::Armed || State == ETDTutorialMusicState::Prelude)
	{
		UTDIntroDirector* Director = Intro.Get();
		if (!Director)
		{
			UTDIntroDirector* Here = UTDIntroDirector::Get(World);
			Director = Here && Here->IsPlaying() ? Here : nullptr;
			Intro = Director;
		}
		if (Director && Director->IsPlaying())
		{
			// The prelude ends on the control frame: it starts at (intro length - prelude length), joining at the
			// right offset if that point fell between two frames (or before the intro began).
			const float Len = PreludeSeconds();
			// Control returns after the shots AND the hand-off blend (48 + 1 = 49 s today): the prelude starts that
			// long before it (11.0 s into the intro for a 38 s prelude).
			const float StartAt = Director->GetTotalDuration() + UTDIntroDirector::HandOffSeconds - Len;
			if (!bPreludeStarted && Len > 0.f && !Director->WasSkipped() && Director->GetElapsed() >= StartAt)
			{
				bPreludeStarted = true;
				PreludeStartOffset = FMath::Max(0.f, Director->GetElapsed() - StartAt);
				PreludeIntroTime = Director->GetElapsed();
				PreludeStartClock = Clock;
				StartVoice(PreludeSound(), PreludeStartOffset, World);
				State = ETDTutorialMusicState::Prelude;
			}
		}
		else if (State == ETDTutorialMusicState::Armed && !Director)
		{
			// A New Game with no intro (td.Intro 0, headless): the control frame is the run's first playable frame.
			// Wait for the galaxy's level entry (the run leaves the Intro phase there).
			if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(World))
			{
				if (Flow->GetProgress().Phase != ETDGamePhase::Intro)
				{
					ControlClock = Clock;
					StartVoice(LoopSound(), 0.f, World);
					LoopStartClock = Clock;
					State = ETDTutorialMusicState::Loop;
					UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: loop at control (no intro)"));
				}
			}
		}
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

void UTDTutorialMusicSubsystem::StopVoice()
{
	if (Voice)
	{
		Voice->Stop();
		Voice->DestroyComponent();
		Voice = nullptr;
	}
}

int32 UTDTutorialMusicSubsystem::GetLiveVoiceCount() const
{
	if (Voice && Voice->IsPlaying())
	{
		return 1;
	}
	// No audio device (tests, -nosound): the logical voice.
	return (State == ETDTutorialMusicState::Prelude || State == ETDTutorialMusicState::Loop || State == ETDTutorialMusicState::FadingOut) ? 1 : 0;
}
