// Combat music director (N-029). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Music

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Sound/SoundWave.h"

namespace TDMusicTest
{
	using S = ETDCombatMusicState;

	struct FWorld
	{
		FTDTestWorld W;
		UTDMusicDirectorSubsystem* M = nullptr;
		bool Init(bool bWithTrack = true)
		{
			M = UTDMusicDirectorSubsystem::Get(W.GetWorld());
			if (!M) { return false; }
			USoundWave* Track = nullptr;
			if (bWithTrack)
			{
				Track = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD_TestCombatTrack"));
				Track->Duration = 120.f;
				Track->bLooping = true;
			}
			M->SetMusicForTests(Track);
			M->SetCombatOverrideForTests(false);
			W.Tick();
			return true;
		}
		void Combat(bool b) { M->SetCombatOverrideForTests(b); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicFadeHoldOut, "TriadDrift.Input.Music.FadeInHoldAndFadeOut", TD_TEST_FLAGS)
bool FTDMusicFadeHoldOut::RunTest(const FString& Parameters)
{
	using namespace TDMusicTest;
	FWorld T;
	if (!TestTrue(TEXT("director"), T.Init())) { return false; }
	TestEqual(TEXT("no fight: silent"), T.M->GetMusicState(), S::Silent);
	T.Combat(true);
	T.W.Advance(1.f);
	TestEqual(TEXT("fight: fading in"), T.M->GetMusicState(), S::FadingIn);
	TestEqual(TEXT("half way after 1 s (2 s fade)"), T.M->GetEnvelope(), 0.5f, 0.06f);
	T.W.Advance(1.2f);
	TestEqual(TEXT("full after 2 s"), T.M->GetMusicState(), S::Playing);
	TestEqual(TEXT("full volume"), T.M->GetEnvelope(), 1.f);
	TestEqual(TEXT("started once"), T.M->GetStartCount(), 1);

	T.Combat(false);
	T.W.Advance(5.8f);
	TestEqual(TEXT("fight over: holds at full volume for 6 s"), T.M->GetMusicState(), S::Holding);
	TestEqual(TEXT("... still full"), T.M->GetEnvelope(), 1.f);
	T.W.Advance(0.4f);
	TestEqual(TEXT("after the hold: fading out"), T.M->GetMusicState(), S::FadingOut);
	T.W.Advance(2.f);
	TestEqual(TEXT("half way through the 4 s fade"), T.M->GetEnvelope(), 0.5f, 0.08f);
	T.W.Advance(2.2f);
	TestEqual(TEXT("then silent"), T.M->GetMusicState(), S::Silent);
	TestEqual(TEXT("... at zero"), T.M->GetEnvelope(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicContinue, "TriadDrift.Input.Music.NewFightInHoldOrFadeCarriesOn", TD_TEST_FLAGS)
bool FTDMusicContinue::RunTest(const FString& Parameters)
{
	using namespace TDMusicTest;
	FWorld T;
	if (!TestTrue(TEXT("director"), T.Init())) { return false; }
	T.Combat(true);
	T.W.Advance(2.5f);
	T.Combat(false);
	T.W.Advance(3.f);
	TestEqual(TEXT("holding"), T.M->GetMusicState(), S::Holding);
	T.Combat(true);
	T.W.Tick();
	TestEqual(TEXT("a new fight in the hold: playing on"), T.M->GetMusicState(), S::Playing);
	TestEqual(TEXT("... without restarting the track"), T.M->GetStartCount(), 1);

	T.Combat(false);
	T.W.Advance(6.2f + 2.f);
	TestEqual(TEXT("fading out"), T.M->GetMusicState(), S::FadingOut);
	T.Combat(true);
	T.W.Advance(0.5f);
	TestEqual(TEXT("a new fight in the fade out: swells back up"), T.M->GetMusicState(), S::FadingIn);
	T.W.Advance(1.5f);
	TestEqual(TEXT("back to full"), T.M->GetMusicState(), S::Playing);
	TestEqual(TEXT("still the same playback"), T.M->GetStartCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicStops, "TriadDrift.Input.Music.OneSecondStopOnCutTravelVictory", TD_TEST_FLAGS)
bool FTDMusicStops::RunTest(const FString& Parameters)
{
	using namespace TDMusicTest;
	{
		FWorld T;
		if (!TestTrue(TEXT("director"), T.Init())) { return false; }
		UTDEventBus* Bus = UTDEventBus::Get(T.W.GetWorld());
		if (!TestNotNull(TEXT("bus"), Bus)) { return false; }
		T.Combat(true);
		T.W.Advance(2.5f);
		Bus->BroadcastSceneCut(TEXT("Wave1"), 2, 1.5f);
		T.W.Tick();
		TestEqual(TEXT("CUT: stopping"), T.M->GetMusicState(), S::Stopping);
		T.W.Advance(1.1f);
		TestEqual(TEXT("CUT: silent after 1 s"), T.M->GetMusicState(), S::Silent);
		T.W.Advance(1.f);
		TestEqual(TEXT("CUT: stays silent until the retake starts"), T.M->GetMusicState(), S::Silent);
		Bus->BroadcastTakeStarted(TEXT("Wave1"), 2);
		T.W.Tick();
		TestEqual(TEXT("the retake's fight starts it again"), T.M->GetMusicState(), S::FadingIn);
		TestEqual(TEXT("... from the top"), T.M->GetStartCount(), 2);

		T.W.Advance(2.5f);
		Bus->BroadcastPhaseChanged(ETDGamePhase::Victory);
		T.W.Tick();
		TestEqual(TEXT("victory: stopping"), T.M->GetMusicState(), S::Stopping);
		T.W.Advance(1.1f);
		TestEqual(TEXT("victory: silent, even with the fight signal on"), T.M->GetMusicState(), S::Silent);
	}
	{
		FWorld T;
		if (!TestTrue(TEXT("director"), T.Init())) { return false; }
		T.Combat(true);
		T.W.Advance(2.5f);
		T.M->SetTravellingForTests(); // a planet trip, a return, or quit to menu
		T.W.Tick();
		TestEqual(TEXT("travel: stopping"), T.M->GetMusicState(), S::Stopping);
		T.W.Advance(1.1f);
		TestEqual(TEXT("travel: silent within 1 s"), T.M->GetMusicState(), S::Silent);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicTriggers, "TriadDrift.Input.Music.TriggersAndEmptySlot", TD_TEST_FLAGS)
bool FTDMusicTriggers::RunTest(const FString& Parameters)
{
	using namespace TDMusicTest;
	{
		FWorld T;
		if (!TestTrue(TEXT("director"), T.Init(false))) { return false; }
		T.Combat(true);
		T.W.Advance(3.f);
		TestEqual(TEXT("empty slot: silent through a fight"), T.M->GetMusicState(), S::Silent);
		TestEqual(TEXT("... never started"), T.M->GetStartCount(), 0);
	}
	{
		FWorld T;
		if (!TestTrue(TEXT("director"), T.Init())) { return false; }
		T.M->SetCombatOverrideForTests(TOptional<bool>()); // the real signals from here
		TestFalse(TEXT("calm: no fight"), T.M->IsCombatNow());
		UTDEventBus::Get(T.W.GetWorld())->BroadcastPhaseChanged(ETDGamePhase::DominionAssault);
		T.W.Tick();
		TestTrue(TEXT("the Dominion assault is a fight"), T.M->IsCombatNow());
		UTDEventBus::Get(T.W.GetWorld())->BroadcastPhaseChanged(ETDGamePhase::Cinder);
		T.W.Tick();
		TestFalse(TEXT("a calm phase is not"), T.M->IsCombatNow());
		ATDEncounterVolume* Room = T.W.Spawn<ATDEncounterVolume>(FVector(0.f, 0.f, 0.f));
		if (!TestNotNull(TEXT("encounter"), Room)) { return false; }
		Room->Activate();
		T.W.Tick();
		TestTrue(TEXT("an active encounter on foot is a fight"), T.M->IsCombatNow());
		T.W.Advance(2.5f);
		TestEqual(TEXT("... and the music plays"), T.M->GetMusicState(), S::Playing);
	}
	return true;
}

#endif
