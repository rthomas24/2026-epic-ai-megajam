// Combat music director (N-029). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.Music

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "TriadDrift/Flow/TDGameInstance.h"

#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Input/TDMusicSettings.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicIntensityGain, "TriadDrift.Input.Music.IntensityGainFollowsTheFight", TD_TEST_FLAGS)
bool FTDMusicIntensityGain::RunTest(const FString& Parameters)
{
	// N-037: the combat music's level follows the fight's smoothed intensity: CalmGain (0.55) when quiet, the
	// authored volume (1.0) in a full fight, rate-limited (no clicks); gate, hold, CUT and travel unchanged.
	using namespace TDMusicTest;
	const UTDMusicSettings* MS = UTDMusicSettings::Get();
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const float Dt = 1.f / Fps;
		const FString Tag = FString::Printf(TEXT("%.0f fps: "), Fps);
		FWorld T;
		if (!TestTrue(*(Tag + TEXT("director")), T.Init())) { return false; }
		auto Run = [&](float Seconds, float& MaxStep)
		{
			float Prev = T.M->GetAppliedVolume();
			for (float t = 0.f; t < Seconds; t += Dt)
			{
				T.W.Tick(Dt);
				MaxStep = FMath::Max(MaxStep, FMath::Abs(T.M->GetAppliedVolume() - Prev) / Dt);
				Prev = T.M->GetAppliedVolume();
			}
		};
		float MaxStep = 0.f;
		T.M->SetIntensityForTests(0.f);
		T.Combat(true);
		Run(3.f, MaxStep);
		TestEqual(*(Tag + TEXT("full envelope after the fade in")), T.M->GetEnvelope(), 1.f);
		TestEqual(*(Tag + TEXT("quiet fight: the calm gain")), T.M->GetIntensityGain(), MS->CalmGain, 0.01f);
		TestEqual(*(Tag + TEXT("... is what plays")), T.M->GetAppliedVolume(), MS->CalmGain, 0.01f);

		T.M->SetIntensityForTests(1.f);
		Run(0.5f, MaxStep);
		TestTrue(*(Tag + TEXT("rate-limited: not there yet after 0.5 s")), T.M->GetIntensityGain() < MS->FightGain - 0.05f);
		Run(1.f, MaxStep);
		TestEqual(*(Tag + TEXT("full fight: the authored volume")), T.M->GetAppliedVolume(), MS->FightGain, 0.01f);
		TestTrue(*(Tag + FString::Printf(TEXT("no clicks: volume never moves faster than the fades (%.2f /s)"), MaxStep)), MaxStep <= 0.55f);

		// A CUT and a travel still stop it within 1 s, at any gain.
		UTDEventBus* Bus = UTDEventBus::Get(T.W.GetWorld());
		if (Bus) { Bus->OnSceneCut.Broadcast(TEXT("Wave_1"), 2, 1.f); }
		Run(1.1f, MaxStep);
		TestEqual(*(Tag + TEXT("CUT: silent within 1 s")), T.M->GetAppliedVolume(), 0.f, 0.001f);
		T.M->SetTravellingForTests();
		Run(0.5f, MaxStep);
		TestEqual(*(Tag + TEXT("travel: stays silent")), T.M->GetMusicState(), S::Silent);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicNoPerFrameLookup, "TriadDrift.Input.Music.NoStyleLookupEveryFrameWhileSilent", TD_TEST_FLAGS)
bool FTDMusicNoPerFrameLookup::RunTest(const FString& Parameters)
{
	// Review M10: while the music was silent Tick resolved the track every frame (LoadPresentation + LoadSynchronous).
	FTDTestWorld W;
	UTDMusicDirectorSubsystem* M = UTDMusicDirectorSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("director"), M)) { return false; }
	M->SetCombatOverrideForTests(false);
	const int32 Before = M->GetMusicLookupsForTests();
	for (int32 i = 0; i < 120; ++i) { W.Tick(1.f / 60.f); } // 2 s of silence
	const int32 Lookups = M->GetMusicLookupsForTests() - Before;
	TestTrue(FString::Printf(TEXT("at most one lookup per 2 s while silent (%d in 120 frames)"), Lookups), Lookups <= 2);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicAssault, "TriadDrift.Input.Music.AssaultScoreHoldsCombatMusicUntilDocking", TD_TEST_FLAGS)
bool FTDMusicAssault::RunTest(const FString& Parameters)
{
	// N-048: the Dominion assault's own score plays from wave 5's clear (phase DominionAssault) through the generators and
	// the reactor, combat music held; docking (DominionInterior) or the end ends it and lets combat music back.
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDMusicDirectorSubsystem* M = UTDMusicDirectorSubsystem::Get(W.GetWorld());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("music"), M) || !TestNotNull(TEXT("gi"), GI)) { return false; }
	USoundWave* Combat = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD_TestAssaultCombat"));
	Combat->Duration = 60.f;
	USoundWave* Assault = NewObject<USoundWave>(GetTransientPackage(), TEXT("TD_TestAssaultScore"));
	Assault->Duration = 139.f;
	M->SetMusicForTests(Combat);
	M->SetAssaultMusicForTests(Assault);
	GI->SetPhase(ETDGamePhase::Wave5);
	M->SetCombatOverrideForTests(true);
	W.Advance(3.f);
	TestFalse(TEXT("wave 5: no assault score"), M->IsAssaultMusicPlaying());
	TestTrue(TEXT("wave 5: combat music"), M->GetMusicState() != ETDCombatMusicState::Silent);
	GI->SetPhase(ETDGamePhase::DominionAssault); // wave 5 cleared
	W.Advance(3.f);
	TestTrue(TEXT("the assault: its own score"), M->IsAssaultMusicPlaying());
	TestEqual(TEXT("... and combat music held, even in a fight"), M->GetMusicState(), ETDCombatMusicState::Silent);
	GI->SetPhase(ETDGamePhase::DominionInterior); // docked
	W.Advance(3.f);
	TestFalse(TEXT("docked: the assault score ends"), M->IsAssaultMusicPlaying());
	TestTrue(TEXT("... and combat music may play again"), M->GetMusicState() != ETDCombatMusicState::Silent);
	return true;
}

#endif
