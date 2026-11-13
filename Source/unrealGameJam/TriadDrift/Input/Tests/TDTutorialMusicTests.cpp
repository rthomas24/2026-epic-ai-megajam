// Tutorial music (N-039, GitHub issue #24). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.TutorialMusic

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDTutorialMusicSubsystem.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Sound/SoundWave.h"

namespace TDTutorialMusicTest
{
	constexpr float PreludeSeconds = 38.f;
	constexpr float FadeSeconds = 20.f;

	struct FRun
	{
		FTDTestWorld W{ UTDGameInstance::StaticClass() };
		UTDTutorialMusicSubsystem* T = nullptr;
		UTDIntroDirector* Intro = nullptr;
		UTDMusicDirectorSubsystem* Music = nullptr;
		UTDGameInstance* GI = nullptr;
		USoundWave* CombatTrack = nullptr;

		bool Init(bool bWithIntroWorld)
		{
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			T = UTDTutorialMusicSubsystem::Get(W.GetWorld());
			Music = UTDMusicDirectorSubsystem::Get(W.GetWorld());
			Intro = UTDIntroDirector::Get(W.GetWorld());
			if (!GI || !T || !Music || !Intro) { return false; }
			T->SetSoundsForTests(nullptr, nullptr, PreludeSeconds, FadeSeconds);
			CombatTrack = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD_TestTutorialCombat"));
			CombatTrack->Duration = 120.f;
			Music->SetMusicForTests(CombatTrack);
			Music->SetCombatOverrideForTests(false);
			if (bWithIntroWorld)
			{
				ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
				APlayerController* PC = W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass());
				if (!Squad || !PC) { return false; }
				Squad->SetSquadPlayerController(PC);
				Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(-25000.f, 0.f, 0.f)));
				W.Advance(0.5f);
			}
			GI->StartNewRun(); // arms the tutorial music
			return T->GetState() == ETDTutorialMusicState::Armed;
		}

		void Run(float Seconds, float Dt) { for (float t = 0.f; t < Seconds; t += Dt) { W.Tick(Dt); } }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialBeatAtControl, "TriadDrift.Input.TutorialMusic.BeatAtControlWatchedAndSkipped", TD_TEST_FLAGS)
bool FTDTutorialBeatAtControl::RunTest(const FString& Parameters)
{
	// Watched: the prelude starts at (intro length - 38 s) so its end lands on the control frame, where the loop
	// starts at 0:00. Skipped: the prelude is cut and the loop starts at 0:00 on the control frame.
	using namespace TDTutorialMusicTest;
	for (const bool bSkip : { false, true })
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const float Dt = 1.f / Fps;
		const FString Tag = FString::Printf(TEXT("%s @%.0f fps: "), bSkip ? TEXT("skipped at 20 s") : TEXT("watched"), Fps);
		FRun R;
		if (!TestTrue(*(Tag + TEXT("set up")), R.Init(true))) { return false; }
		int32 Finished = 0;
		if (!TestTrue(*(Tag + TEXT("intro plays")), R.Intro->PlayForTests(FSimpleDelegate::CreateLambda([&Finished]() { ++Finished; })))) { return false; }
		const float Total = R.Intro->GetTotalDuration();
		if (bSkip)
		{
			R.Run(20.f, Dt);
			TestTrue(*(Tag + TEXT("prelude started before the skip (it starts at intro length - 38 s)")), R.T->GetState() == ETDTutorialMusicState::Prelude);
			R.Intro->Skip();
		}
		for (float t = 0.f; t < Total + 5.f && Finished == 0; t += Dt) { R.W.Tick(Dt); }
		TestEqual(*(Tag + TEXT("intro finished")), Finished, 1);
		TestEqual(*(Tag + TEXT("tutorial loop playing")), R.T->GetState(), ETDTutorialMusicState::Loop);
		TestEqual(*(Tag + TEXT("the loop's first beat is on the control frame")), R.T->GetLoopStartClock(), R.T->GetControlClock(), 0.001f);
		if (!bSkip)
		{
			TestEqual(*(Tag + FString::Printf(TEXT("prelude started at intro time %.2f (shots %.1f + 1 s hand-off - 38)"), R.T->GetPreludeIntroTime(), Total)),
				R.T->GetPreludeIntroTime(), Total + UTDIntroDirector::HandOffSeconds - PreludeSeconds, Dt + 0.001f);
			const float PreludeEnd = R.T->GetPreludeStartClock() + (PreludeSeconds - R.T->GetPreludeStartOffset());
			AddInfo(FString::Printf(TEXT("%sprelude end %.3f, control %.3f, gap %.3f s (intro total %.2f, elapsed at finish %.2f)"), *Tag, PreludeEnd, R.T->GetControlClock(), R.T->GetControlClock() - PreludeEnd, Total, R.Intro->GetElapsed()));
			TestEqual(*(Tag + FString::Printf(TEXT("prelude ends on the control frame (%.3f vs %.3f s)"), PreludeEnd, R.T->GetControlClock())),
				PreludeEnd, R.T->GetControlClock(), Dt * 1.5f);
			TestEqual(*(Tag + TEXT("two voices in all: prelude, then loop")), R.T->GetVoicesStarted(), 2);
		}
		TestEqual(*(Tag + TEXT("never two at once")), R.T->GetLiveVoiceCount(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialLoopAndFade, "TriadDrift.Input.TutorialMusic.LoopSurvivesRetakeAndGapThenFadesAcrossTravel", TD_TEST_FLAGS)
bool FTDTutorialLoopAndFade::RunTest(const FString& Parameters)
{
	using namespace TDTutorialMusicTest;
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const float Dt = 1.f / Fps;
		const FString Tag = FString::Printf(TEXT("@%.0f fps: "), Fps);
		FRun R;
		if (!TestTrue(*(Tag + TEXT("set up")), R.Init(false))) { return false; }
		// No intro: the loop starts on the run's first playable frame (the run leaves the Intro phase).
		R.GI->SetPhase(ETDGamePhase::Waves1to2);
		R.Run(0.2f, Dt);
		TestEqual(*(Tag + TEXT("no intro: loop at control")), R.T->GetState(), ETDTutorialMusicState::Loop);
		const int32 Voices = R.T->GetVoicesStarted();

		// A fight: the combat music waits while the tutorial plays (the gate doesn't touch the loop).
		R.Music->SetCombatOverrideForTests(true);
		R.Run(3.f, Dt);
		TestEqual(*(Tag + TEXT("combat music held during the tutorial")), R.Music->GetMusicState(), ETDCombatMusicState::Silent);
		// CUT! and a retake, then a quiet gap: the loop carries on, not restarted.
		UTDEventBus* Bus = UTDEventBus::Get(R.W.GetWorld());
		if (!TestNotNull(*(Tag + TEXT("bus")), Bus)) { return false; }
		Bus->BroadcastSceneCut(TEXT("Wave_1"), 2, 1.f);
		R.Run(1.5f, Dt);
		Bus->BroadcastTakeStarted(TEXT("Wave_1"), 2);
		R.Music->SetCombatOverrideForTests(false);
		R.Run(20.f, Dt);
		TestEqual(*(Tag + TEXT("still the loop after a retake and a gap")), R.T->GetState(), ETDTutorialMusicState::Loop);
		TestEqual(*(Tag + TEXT("not restarted")), R.T->GetVoicesStarted(), Voices);

		// Wave 1 cleared: a 20 s fade, carried on across a level travel (another world steps it).
		Bus->BroadcastWaveCleared(1);
		R.Run(10.f, Dt);
		TestEqual(*(Tag + TEXT("fading")), R.T->GetState(), ETDTutorialMusicState::FadingOut);
		TestEqual(*(Tag + TEXT("half way after 10 s")), R.T->GetFadeVolume(), 0.5f, 0.02f);
		R.Music->SetCombatOverrideForTests(true);
		R.Run(0.5f, Dt);
		TestTrue(*(Tag + TEXT("combat music takes over under the fade")), R.Music->GetMusicState() != ETDCombatMusicState::Silent);
		{
			FTDTestWorld Next; // the next level: the voice belongs to the game instance, the next world keeps it going
			for (float t = 0.f; t < 5.f; t += Dt) { R.T->Step(Next.GetWorld(), Dt, false); }
		}
		TestEqual(*(Tag + TEXT("the fade carried on across the travel (15.5 of 20 s)")), R.T->GetFadeVolume(), 1.f - 15.5f / FadeSeconds, 0.01f);
		R.Run(5.5f, Dt);
		TestEqual(*(Tag + TEXT("silent after the full 20 s")), R.T->GetState(), ETDTutorialMusicState::Off);
		TestEqual(*(Tag + TEXT("no voice left")), R.T->GetLiveVoiceCount(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialContinue, "TriadDrift.Input.TutorialMusic.ContinueAndMenuNeverPlayIt", TD_TEST_FLAGS)
bool FTDTutorialContinue::RunTest(const FString& Parameters)
{
	using namespace TDTutorialMusicTest;
	FRun R;
	if (!TestTrue(TEXT("set up"), R.Init(false))) { return false; }
	R.T->Stop(TEXT("test reset"));
	// A Continue (ApplySave) never arms it: only StartNewRun does.
	R.GI->SetPhase(ETDGamePhase::Waves1to2);
	R.Run(1.f, 1.f / 60.f);
	TestEqual(TEXT("continue: no tutorial music"), R.T->GetState(), ETDTutorialMusicState::Off);
	// The menu stops a running tutorial.
	R.GI->StartNewRun();
	R.Run(0.2f, 1.f / 60.f);
	R.T->Step(R.W.GetWorld(), 1.f / 60.f, /*bMenuMap*/ true);
	TestEqual(TEXT("menu: stopped"), R.T->GetState(), ETDTutorialMusicState::Off);
	return true;
}

#endif
