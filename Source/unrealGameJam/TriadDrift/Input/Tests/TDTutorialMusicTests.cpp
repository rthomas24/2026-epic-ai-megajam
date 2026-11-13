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
		R.Run(0.2f, Dt); // binds this world's intro event
		R.Intro->NotifyRunStartedWithoutIntro(); // the galaxy entered without the intro: the control frame
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

		// Wave 1 cleared: the tutorial goes on (Nicholas: only the escape ends it).
		Bus->BroadcastWaveCleared(1);
		R.Run(2.f, Dt);
		TestEqual(*(Tag + TEXT("wave 1 clear alone: still the loop")), R.T->GetState(), ETDTutorialMusicState::Loop);
		// The escape: a 20 s fade, carried on across a level travel (another world steps it).
		R.T->NotifyTutorialEscaped();
		R.Run(10.f, Dt);
		TestEqual(*(Tag + TEXT("fading")), R.T->GetState(), ETDTutorialMusicState::FadingOut);
		TestEqual(*(Tag + TEXT("half way after 10 s")), R.T->GetFadeVolume(), 0.5f, 0.02f);
		R.Music->SetCombatOverrideForTests(true);
		R.Run(0.5f, Dt);
		TestEqual(*(Tag + TEXT("combat music still held under the fade (issue #24 finding 3)")), R.Music->GetMusicState(), ETDCombatMusicState::Silent);
		{
			FTDTestWorld Next; // the next level: the voice belongs to the game instance, the next world keeps it going
			for (float t = 0.f; t < 5.f; t += Dt) { R.T->Step(Next.GetWorld(), Dt, false); }
		}
		TestEqual(*(Tag + TEXT("the fade carried on across the travel (15.5 of 20 s)")), R.T->GetFadeVolume(), 1.f - 15.5f / FadeSeconds, 0.01f);
		R.Run(5.5f, Dt);
		TestEqual(*(Tag + TEXT("silent after the full 20 s")), R.T->GetState(), ETDTutorialMusicState::Off);
		TestEqual(*(Tag + TEXT("no voice left")), R.T->GetLiveVoiceCount(), 0);
		R.Run(1.f, Dt);
		TestTrue(*(Tag + TEXT("combat music comes in once the fade has finished")), R.Music->GetMusicState() != ETDCombatMusicState::Silent);
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
	// New Game arms it from the menu (live bug, Nov 13: the menu world stopped it before the travel): armed survives.
	R.GI->StartNewRun();
	R.T->Step(R.W.GetWorld(), 1.f / 60.f, /*bMenuMap*/ true);
	TestEqual(TEXT("armed on the menu: kept for the galaxy"), R.T->GetState(), ETDTutorialMusicState::Armed);
	// A started tutorial stops on a quit to the menu.
	R.Run(0.2f, 1.f / 60.f);
	R.Intro->NotifyRunStartedWithoutIntro();
	R.Run(0.2f, 1.f / 60.f);
	TestEqual(TEXT("playing"), R.T->GetState(), ETDTutorialMusicState::Loop);
	R.T->Step(R.W.GetWorld(), 1.f / 60.f, /*bMenuMap*/ true);
	TestEqual(TEXT("quit to menu: stopped"), R.T->GetState(), ETDTutorialMusicState::Off);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialNoIntroMusic, "TriadDrift.Input.TutorialMusic.IntroMusicOnlyWithoutTheTutorial", TD_TEST_FLAGS)
bool FTDTutorialNoIntroMusic::RunTest(const FString& Parameters)
{
	// Issue #24 finding 1: a New Game with tutorial music never starts the intro's legacy IntroMusic.
	using namespace TDTutorialMusicTest;
	{
		FRun R;
		if (!TestTrue(TEXT("set up"), R.Init(true))) { return false; }
		TestTrue(TEXT("armed: owns the opening"), R.T->OwnsOpeningMusic());
		if (!TestTrue(TEXT("intro plays"), R.Intro->PlayForTests(FSimpleDelegate()))) { return false; }
		TestTrue(TEXT("tutorial armed: the intro did not start IntroMusic"), R.Intro->WasIntroMusicSuppressed());
		R.Run(15.f, 1.f / 60.f);
		TestEqual(TEXT("prelude playing, IntroMusic still not started"), R.T->GetState(), ETDTutorialMusicState::Prelude);
		TestTrue(TEXT("... still suppressed"), R.Intro->WasIntroMusicSuppressed());
	}
	{
		FRun R;
		if (!TestTrue(TEXT("set up 2"), R.Init(true))) { return false; }
		R.T->Stop(TEXT("test: a run without tutorial music"));
		TestFalse(TEXT("off: does not own the opening"), R.T->OwnsOpeningMusic());
		if (!TestTrue(TEXT("intro plays 2"), R.Intro->PlayForTests(FSimpleDelegate()))) { return false; }
		TestFalse(TEXT("no tutorial music: IntroMusic allowed as before"), R.Intro->WasIntroMusicSuppressed());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialSingleVoice, "TriadDrift.Input.TutorialMusic.SingleVoiceDownbeatOnControl", TD_TEST_FLAGS)
bool FTDTutorialSingleVoice::RunTest(const FString& Parameters)
{
	// Issue #24 v2: one continuous source. Watched: entered once at source ~0 so source 0:38 is on the control frame,
	// nothing at control. Skipped: re-entered at 0:38 on the control frame with a 150 ms fade-in.
	using namespace TDTutorialMusicTest;
	for (const bool bSkip : { false, true })
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const float Dt = 1.f / Fps;
		const FString Tag = FString::Printf(TEXT("%s @%.0f fps: "), bSkip ? TEXT("skipped at 20 s") : TEXT("watched"), Fps);
		FRun R;
		if (!TestTrue(*(Tag + TEXT("set up")), R.Init(true))) { return false; }
		R.T->SetSingleVoiceForTests(nullptr, 38.f, FadeSeconds, 0.15f);
		int32 Finished = 0;
		if (!TestTrue(*(Tag + TEXT("intro plays")), R.Intro->PlayForTests(FSimpleDelegate::CreateLambda([&Finished]() { ++Finished; })))) { return false; }
		const float Total = R.Intro->GetTotalDuration();
		if (bSkip)
		{
			R.Run(20.f, Dt);
			TestEqual(*(Tag + TEXT("in before the skip")), R.T->GetState(), ETDTutorialMusicState::Prelude);
			TestEqual(*(Tag + TEXT("one voice so far")), R.T->GetVoicesStarted(), 1);
			R.Intro->Skip();
		}
		for (float t = 0.f; t < Total + 5.f && Finished == 0; t += Dt) { R.W.Tick(Dt); }
		TestEqual(*(Tag + TEXT("intro finished")), Finished, 1);
		TestEqual(*(Tag + TEXT("playing")), R.T->GetState(), ETDTutorialMusicState::Loop);
		if (bSkip)
		{
			TestEqual(*(Tag + TEXT("skip: re-entered at the downbeat")), R.T->GetEnteredAtSourceTime(), 38.f, 0.001f);
			TestEqual(*(Tag + TEXT("skip: 150 ms fade-in")), R.T->GetEnteredFadeIn(), 0.15f, 0.001f);
			TestEqual(*(Tag + TEXT("skip: entered on the control frame")), R.T->GetSourceTimeAtControl(), 38.f, 0.001f);
		}
		else
		{
			TestEqual(*(Tag + TEXT("watched: entered once, at about source 0")), R.T->GetEnteredAtSourceTime(), 0.f, Dt + 0.001f);
			TestEqual(*(Tag + TEXT("watched: no fade-in")), R.T->GetEnteredFadeIn(), 0.f);
			TestEqual(*(Tag + FString::Printf(TEXT("watched: source 0:38 on the control frame (source %.3f there)"), R.T->GetSourceTimeAtControl())),
				R.T->GetSourceTimeAtControl(), 38.f, Dt * 1.5f);
			TestEqual(*(Tag + TEXT("watched: one voice in all (nothing restarted at control)")), R.T->GetVoicesStarted(), 1);
		}
		TestEqual(*(Tag + TEXT("never two at once")), R.T->GetLiveVoiceCount(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialFadeIdempotent, "TriadDrift.Input.TutorialMusic.SecondWaveClearNeverRestartsTheFade", TD_TEST_FLAGS)
bool FTDTutorialFadeIdempotent::RunTest(const FString& Parameters)
{
	using namespace TDTutorialMusicTest;
	FRun R;
	if (!TestTrue(TEXT("set up"), R.Init(false))) { return false; }
	R.Run(0.2f, 1.f / 60.f);
	R.Intro->NotifyRunStartedWithoutIntro();
	R.Run(0.2f, 1.f / 60.f);
	UTDEventBus* Bus = UTDEventBus::Get(R.W.GetWorld());
	if (!TestNotNull(TEXT("bus"), Bus)) { return false; }
	R.T->NotifyTutorialEscaped();
	R.Run(10.f, 1.f / 60.f);
	R.T->NotifyTutorialEscaped(TEXT("test: again")); // a second escape event 10 s into the fade
	Bus->BroadcastWaveCleared(2);
	Bus->BroadcastWaveCleared(1);
	R.Run(0.5f, 1.f / 60.f);
	TestEqual(TEXT("one fade only"), R.T->GetFadesBegun(), 1);
	TestEqual(TEXT("still 10.5 of 20 s in (not restarted)"), R.T->GetFadeVolume(), 1.f - 10.5f / FadeSeconds, 0.02f);
	R.Run(10.f, 1.f / 60.f);
	TestEqual(TEXT("silent at 20 s, as before"), R.T->GetState(), ETDTutorialMusicState::Off);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTutorialEndsOnEscape, "TriadDrift.Input.TutorialMusic.OnlyTheEscapeEndsIt", TD_TEST_FLAGS)
bool FTDTutorialEndsOnEscape::RunTest(const FString& Parameters)
{
	// Nicholas (issue #24): the tutorial battle ends when the player crosses the escape boundary during the ordered
	// withdrawal, gunship alive or destroyed. Wave 1's clear or the gunship's destruction alone never fade the loop.
	using namespace TDTutorialMusicTest;
	FRun R;
	if (!TestTrue(TEXT("set up"), R.Init(false))) { return false; }
	R.GI->SetPhase(ETDGamePhase::Waves1to2);
	R.Run(0.2f, 1.f / 60.f);
	R.Intro->NotifyRunStartedWithoutIntro();
	R.Run(0.2f, 1.f / 60.f);
	TestEqual(TEXT("loop"), R.T->GetState(), ETDTutorialMusicState::Loop);
	UTDEventBus* Bus = UTDEventBus::Get(R.W.GetWorld());
	if (!TestNotNull(TEXT("bus"), Bus)) { return false; }
	Bus->BroadcastWaveCleared(1);
	R.Run(3.f, 1.f / 60.f);
	TestEqual(TEXT("wave 1 cleared: the loop goes on"), R.T->GetState(), ETDTutorialMusicState::Loop);
	AActor* Gunship = R.W.Spawn<AActor>();
	Bus->BroadcastActorKilled(Gunship, nullptr, ETDDamageKind::Energy); // the gunship destroyed
	R.Run(3.f, 1.f / 60.f);
	TestEqual(TEXT("gunship destroyed: the loop goes on"), R.T->GetState(), ETDTutorialMusicState::Loop);
	TestEqual(TEXT("no fade yet"), R.T->GetFadesBegun(), 0);
	R.T->NotifyTutorialEscaped();
	R.Run(0.5f, 1.f / 60.f);
	TestEqual(TEXT("the escape: the 20 s fade"), R.T->GetState(), ETDTutorialMusicState::FadingOut);

	// Fallback until T4's event is bound: the run moving on to Verdance ends it too.
	FRun R2;
	if (!TestTrue(TEXT("set up 2"), R2.Init(false))) { return false; }
	R2.GI->SetPhase(ETDGamePhase::Waves1to2);
	R2.Run(0.2f, 1.f / 60.f);
	R2.Intro->NotifyRunStartedWithoutIntro();
	R2.Run(0.2f, 1.f / 60.f);
	R2.GI->SetPhase(ETDGamePhase::Verdance);
	R2.Run(0.5f, 1.f / 60.f);
	TestEqual(TEXT("fallback: the run moved past the tutorial battle"), R2.T->GetState(), ETDTutorialMusicState::FadingOut);
	return true;
}

#endif
