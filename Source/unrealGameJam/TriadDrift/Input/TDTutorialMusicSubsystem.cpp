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
#include "Sound/SoundWave.h"

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
	PreludeStartClock = LoopStartClock = ControlClock = PreludeIntroTime = -1.f;
	EnteredAtSource = EnterClock = -1.f;
	EnteredFadeIn = 0.f;
	FadesBegun = 0;
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
	return bTestSounds || bTestSingle || SingleSound() || PreludeSound() || LoopSound();
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

void UTDTutorialMusicSubsystem::SetSingleVoiceForTests(USoundBase* Music, float DownbeatSeconds, float InFadeSeconds, float SkipFade)
{
	bTestSingle = true;
	bTestSounds = true;
	TestSingle = Music;
	TestDownbeat = DownbeatSeconds;
	TestFadeSeconds = InFadeSeconds;
	TestSkipFade = SkipFade;
}

#if !UE_BUILD_SHIPPING
static FString GTDTutorialMusicOverride; // TD.Tutorial.Music <asset path>: dev / recording checks without editing DA_UI_Style
#endif

USoundBase* UTDTutorialMusicSubsystem::SingleSound() const
{
	if (bTestSounds) { return bTestSingle ? TestSingle.Get() : nullptr; }
#if !UE_BUILD_SHIPPING
	if (!GTDTutorialMusicOverride.IsEmpty())
	{
		if (USoundBase* Override = LoadObject<USoundBase>(nullptr, *GTDTutorialMusicOverride)) { return Override; }
	}
#endif
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	return Style && !Style->TutorialMusic.IsNull() ? Style->TutorialMusic.LoadSynchronous() : nullptr;
}

bool UTDTutorialMusicSubsystem::IsSingleVoice() const
{
	return bTestSounds ? bTestSingle : SingleSound() != nullptr;
}

float UTDTutorialMusicSubsystem::SkipFadeIn() const
{
	if (bTestSounds) { return bTestSingle ? TestSkipFade : 0.15f; }
	const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	return Style ? Style->TutorialMusicSkipFadeInSeconds : 0.15f;
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
	if (bTestSingle) { return TestDownbeat; }
	if (bTestSounds) { return TestPreludeSeconds; }
	if (SingleSound())
	{
		const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
		return Style ? Style->TutorialMusicDownbeatSeconds : 38.f;
	}
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
	const bool bWatchedThrough = State == ETDTutorialMusicState::Prelude && !bSkipped;
	if (IsSingleVoice())
	{
		if (!bWatchedThrough)
		{
			// Skip / no intro: straight to the downbeat, with the short fade-in (a voice already in is replaced).
			StartSingle(PreludeSeconds(), SkipFadeIn(), BoundWorld.Get());
		}
		// Watched: the one voice reaches the downbeat on this frame by itself; nothing is touched here.
		LoopStartClock = Clock;
	}
	else
	{
		// Two voices (fallback): the loop's first beat on the control frame; a prelude still playing (a skip) is cut.
		StartVoice(LoopSound(), 0.f, BoundWorld.Get());
		if (!bWatchedThrough && Voice)
		{
			Voice->FadeIn(SkipFadeIn(), 1.f, 0.f);
		}
		LoopStartClock = Clock;
	}
	State = ETDTutorialMusicState::Loop;
	UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: control at clock %.4f s"), Clock);
	UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: loop at control (%s)"), !bPlayed ? TEXT("no intro") : bSkipped ? TEXT("intro skipped") : TEXT("intro watched"));
}

void UTDTutorialMusicSubsystem::HandleWaveCleared(int32 WaveNumber)
{
	// Idempotent (Audio's source finding): only the first qualifying clear starts the fade; a later one never restarts it.
	if (WaveNumber >= UTDMusicSettings::Get()->TutorialEndsAfterWave && (State == ETDTutorialMusicState::Prelude || State == ETDTutorialMusicState::Loop))
	{
		BeginFade();
	}
}

void UTDTutorialMusicSubsystem::BeginFade()
{
	State = ETDTutorialMusicState::FadingOut;
	FadeElapsed = 0.f;
	++FadesBegun;
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
			const float StartAt = Director->GetIntroLength() - Len; // < 0: an intro shorter than the prelude
			if (Director->GetIntroTime() >= FMath::Max(StartAt, 0.f))
			{
				bPreludeStarted = true;
				PreludeStartOffset = FMath::Max(0.f, Director->GetIntroTime() - StartAt);
				PreludeIntroTime = Director->GetIntroTime();
				PreludeStartClock = Clock;
				if (IsSingleVoice())
				{
					StartSingle(PreludeStartOffset, 0.f, World); // source 0 (or later) so the downbeat lands on control
				}
				else
				{
					StartVoice(PreludeSound(), PreludeStartOffset, World);
				}
				State = ETDTutorialMusicState::Prelude;
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

void UTDTutorialMusicSubsystem::StartSingle(float SourceTime, float FadeIn, UWorld* World)
{
	StopVoice(); // never two voices
	++VoicesStarted;
	Volume = 1.f;
	EnteredAtSource = SourceTime;
	EnteredFadeIn = FadeIn;
	EnterClock = Clock;
	USoundBase* Sound = SingleSound();
	if (!Sound || !World)
	{
		return; // no asset (tests): the timing still runs
	}
	Voice = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition*/ true, /*bAutoDestroy*/ false);
	if (!Voice)
	{
		return; // no audio device
	}
	// The MetaSound reads its entry point from "StartTime" (Audio's graph feeds it to the Wave Player). A plain wave
	// (no MetaSound) is seeked by the component instead; a MetaSound is not, so the offset is never applied twice.
	Voice->SetFloatParameter(TEXT("StartTime"), SourceTime);
	const float Seek = Cast<USoundWave>(Sound) ? SourceTime : 0.f;
	if (FadeIn > 0.f) { Voice->FadeIn(FadeIn, 1.f, Seek); }
	else { Voice->Play(Seek); }
	UE_LOG(LogTriadDrift, Log, TEXT("TD tutorial music: one voice in at source %.3f s (fade-in %.2f s) at clock %.4f s"), SourceTime, FadeIn, Clock);
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

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Flow/TDGameInstance.h"

#include "AudioMixerBlueprintLibrary.h"

namespace TDTutorialMusicDebug
{
	static FAutoConsoleCommandWithWorldAndArgs GMusic(TEXT("TD.Tutorial.Music"),
		TEXT("Dev: use this asset as UTDUIStyle::TutorialMusic for the session (empty = the style's). TD.Tutorial.Music /Game/Path/Asset.Asset"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			GTDTutorialMusicOverride = Args.Num() ? Args[0] : FString();
			const bool bOk = GTDTutorialMusicOverride.IsEmpty() || LoadObject<USoundBase>(nullptr, *GTDTutorialMusicOverride) != nullptr;
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Tutorial.Music: %s (%s)"), GTDTutorialMusicOverride.IsEmpty() ? TEXT("the style's") : *GTDTutorialMusicOverride, bOk ? TEXT("loaded") : TEXT("NOT FOUND"));
		}));

	// Records the real output (the main submix) to Saved/BouncedWavFiles/<name>.wav; logs the tutorial clock at the start
	// so a recording position can be matched to logged events (control, entries).
	static FAutoConsoleCommandWithWorldAndArgs GRecord(TEXT("TD.Audio.Record"),
		TEXT("Dev: TD.Audio.Record start [seconds] | stop <name>: record the game's audio output to Saved/BouncedWavFiles/<name>.wav"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UTDTutorialMusicSubsystem* T = UTDTutorialMusicSubsystem::Get(World);
			if (Args.Num() && Args[0].Equals(TEXT("start"), ESearchCase::IgnoreCase))
			{
				UAudioMixerBlueprintLibrary::StartRecordingOutput(World, Args.Num() > 1 ? FCString::Atof(*Args[1]) : 120.f);
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Audio.Record: started at tutorial clock %.4f s"), T ? T->GetClock() : -1.f);
			}
			else if (Args.Num() > 1 && Args[0].Equals(TEXT("stop"), ESearchCase::IgnoreCase))
			{
				UAudioMixerBlueprintLibrary::StopRecordingOutput(World, EAudioRecordingExportType::WavFile, Args[1], FString());
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Audio.Record: stopped at tutorial clock %.4f s -> Saved/BouncedWavFiles/%s.wav"), T ? T->GetClock() : -1.f, *Args[1]);
			}
		}));

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
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Tutorial.Status: %s clock %.2f | prelude at intro %.2f s (clock %.2f, offset %.2f) | control %.2f loop %.2f | single %d entered at source %.2f, source now %.2f | volume %.2f | voices started %d live %d"),
				*UEnum::GetValueAsString(T->GetState()), T->GetClock(), T->GetPreludeIntroTime(), T->GetPreludeStartClock(), T->GetPreludeStartOffset(),
				T->GetControlClock(), T->GetLoopStartClock(), T->IsSingleVoice() ? 1 : 0, T->GetEnteredAtSourceTime(), T->GetSourceTimeNow(), T->GetFadeVolume(), T->GetVoicesStarted(), T->GetLiveVoiceCount());
		}));
}
#endif
