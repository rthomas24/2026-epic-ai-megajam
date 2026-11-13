// Triad Drift - Director's Cut sounds (ENG-35). Owner: Teammate 3.

#include "TriadDrift/Director/TDDirectorSounds.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#if WITH_DEV_AUTOMATION_TESTS
TWeakObjectPtr<const UTDUIStyle> FTDDirectorSounds::StyleOverrideForTests;
TArray<ETDDirectorSound> FTDDirectorSounds::PlayedForTests;
#endif

const TCHAR* FTDDirectorSounds::Name(ETDDirectorSound Which)
{
	switch (Which)
	{
	case ETDDirectorSound::SlateClap: return TEXT("SlateClap");
	case ETDDirectorSound::CutSting: return TEXT("CutSting");
	case ETDDirectorSound::SlowMoIn: return TEXT("SlowMoIn");
	case ETDDirectorSound::SlowMoOut: return TEXT("SlowMoOut");
	default: return TEXT("?");
	}
}

const FTDFXCue* FTDDirectorSounds::GetCue(const UTDUIStyle* Style, ETDDirectorSound Which)
{
	if (!Style)
	{
		return nullptr;
	}
	switch (Which)
	{
	case ETDDirectorSound::SlateClap: return &Style->SlateClap;
	case ETDDirectorSound::CutSting: return &Style->CutSting;
	case ETDDirectorSound::SlowMoIn: return &Style->SlowMoIn;
	case ETDDirectorSound::SlowMoOut: return &Style->SlowMoOut;
	default: return nullptr;
	}
}

bool FTDDirectorSounds::Play(const UObject* WorldContext, ETDDirectorSound Which)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld())
	{
		return false;
	}
	const UTDUIStyle* Style = nullptr;
#if WITH_DEV_AUTOMATION_TESTS
	Style = StyleOverrideForTests.Get();
#endif
	if (!Style)
	{
		Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	}
	const FTDFXCue* Cue = GetCue(Style, Which);
	if (!Cue || Cue->Sound.IsNull())
	{
		return false; // optional: silent until a sound is set
	}
	USoundBase* Sound = Cue->Sound.LoadSynchronous();
	if (!Sound)
	{
		return false;
	}
#if WITH_DEV_AUTOMATION_TESTS
	PlayedForTests.Add(Which);
#endif
	// 2D UI sound: no position, unaffected by global time dilation, plays while paused. The asset carries its own
	// volume, class and concurrency (audio standard): no overrides here.
	UGameplayStatics::PlaySound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, nullptr, /*bIsUISound*/ true);
	UE_LOG(LogTriadDrift, Verbose, TEXT("Director sound: %s"), Name(Which));
	return true;
}
