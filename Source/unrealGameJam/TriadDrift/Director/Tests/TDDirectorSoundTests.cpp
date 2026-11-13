// Triad Drift - the Director's Cut sounds (ENG-35): slate clap, CUT! sting, slow motion in and out. Owner: Teammate 3.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Director/TDDirectorSounds.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDDirectorSoundTest
{
	static int32 Count(ETDDirectorSound Which)
	{
		int32 N = 0;
		for (const ETDDirectorSound S : FTDDirectorSounds::PlayedForTests)
		{
			N += S == Which ? 1 : 0;
		}
		return N;
	}

	/** A UI style with every Director sound set (any engine sound stands in for the real ones). */
	static UTDUIStyle* MakeStyle(bool bWithSounds)
	{
		UTDUIStyle* Style = NewObject<UTDUIStyle>(GetTransientPackage());
		if (bWithSounds)
		{
			const FSoftObjectPath Noise(TEXT("/Engine/EngineSounds/WhiteNoise.WhiteNoise"));
			for (const ETDDirectorSound S : { ETDDirectorSound::SlateClap, ETDDirectorSound::CutSting, ETDDirectorSound::SlowMoIn, ETDDirectorSound::SlowMoOut })
			{
				const_cast<FTDFXCue*>(FTDDirectorSounds::GetCue(Style, S))->Sound = TSoftObjectPtr<USoundBase>(Noise);
			}
		}
		return Style;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDirectorSounds_Hooks, "TriadDrift.Director.Sounds.SlateCutAndSlowMoInOut", TD_TEST_FLAGS)
bool FTDDirectorSounds_Hooks::RunTest(const FString& Parameters)
{
	using namespace TDDirectorSoundTest;
	if (!LoadObject<USoundBase>(nullptr, TEXT("/Engine/EngineSounds/WhiteNoise.WhiteNoise")))
	{
		AddWarning(TEXT("engine stand-in sound missing; nothing to check"));
		return true;
	}
	ON_SCOPE_EXIT
	{
		FTDDirectorSounds::StyleOverrideForTests = nullptr;
		FTDDirectorSounds::PlayedForTests.Reset();
	};
	for (const bool bWithSounds : { true, false })
	{
		const FString Tag = bWithSounds ? TEXT(" (sounds set)") : TEXT(" (style without sounds: silent)");
		UTDUIStyle* Style = MakeStyle(bWithSounds);
		Style->AddToRoot();
		ON_SCOPE_EXIT { Style->RemoveFromRoot(); };
		FTDDirectorSounds::StyleOverrideForTests = Style;
		FTDDirectorSounds::PlayedForTests.Reset();

		FTDTestWorld W;
		UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(W.GetWorld());
		UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
		AActor* Subject = W.Spawn<AActor>(FVector(5000.f, 0.f, 0.f));
		if (!TestNotNull(TEXT("director"), Director) || !TestNotNull(TEXT("bus"), Bus) || !TestNotNull(TEXT("subject"), Subject))
		{
			return false;
		}
		W.Tick();
		const int32 Want = bWithSounds ? 1 : 0;

		// The slate comes up: the clap.
		Bus->BroadcastTakeStarted(TEXT("Wave1"), 1);
		W.Tick();
		TestEqual(TEXT("slate clap") + Tag, Count(ETDDirectorSound::SlateClap), Want);

		// A money shot: the whoosh in once, the release once it ends, a replacing shot doesn't whoosh again.
		TestTrue(TEXT("a ram kill beat") + Tag, Director->ReportHighlight(ETDHighlightKind::RamKill, Subject, 0.6f));
		TestEqual(TEXT("slow-mo in") + Tag, Count(ETDDirectorSound::SlowMoIn), Want);
		TestTrue(TEXT("a reactor kill takes over") + Tag, Director->ReportHighlight(ETDHighlightKind::ReactorKill, Subject, 1.f));
		TestEqual(TEXT("still one slow-mo in (the slow motion never stopped)") + Tag, Count(ETDDirectorSound::SlowMoIn), Want);
		TestEqual(TEXT("no slow-mo out while it runs") + Tag, Count(ETDDirectorSound::SlowMoOut), 0);
		W.Advance(6.f, 1.f / 30.f);
		TestFalse(TEXT("the beat is over") + Tag, Director->IsShotActive());
		TestEqual(TEXT("slow-mo out once") + Tag, Count(ETDDirectorSound::SlowMoOut), Want);

		// CUT!: the sting, and a cancelled beat still releases.
		W.Advance(3.f, 1.f / 30.f);
		TestTrue(TEXT("another beat") + Tag, Director->ReportHighlight(ETDHighlightKind::ReactorKill, Subject, 1.f));
		Bus->BroadcastSceneCut(TEXT("Wave1"), 2, 1.5f);
		W.Tick();
		TestEqual(TEXT("CUT! sting") + Tag, Count(ETDDirectorSound::CutSting), Want);
		TestEqual(TEXT("the cut beat released its slow motion") + Tag, Count(ETDDirectorSound::SlowMoOut), 2 * Want);
		Bus->BroadcastTakeStarted(TEXT("Wave1"), 2);
		W.Tick();
		TestEqual(TEXT("the retake's slate claps again") + Tag, Count(ETDDirectorSound::SlateClap), 2 * Want);
	}
	return true;
}

#endif
