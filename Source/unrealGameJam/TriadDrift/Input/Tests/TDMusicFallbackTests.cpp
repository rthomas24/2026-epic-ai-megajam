// R-047 the engineering music fallback. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.MusicFallback

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDMusicFallbackSubsystem.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Core/TDMusicEventBridge.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDSaveGame.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Components/AudioComponent.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"

namespace TDT1Fallback
{
	struct FRig
	{
		FTDTestWorld W{ UTDGameInstance::StaticClass() };
		UTDMusicFallbackSubsystem* F = nullptr;
		UTDMusicDirectorSubsystem* D = nullptr;
		UTDMusicEventBridge* B = nullptr;
		FRig()
		{
			F = UTDMusicFallbackSubsystem::Get(W.GetWorld());
			D = UTDMusicDirectorSubsystem::Get(W.GetWorld());
			B = UTDMusicEventBridge::GetMusicEventBridge(W.GetWorld());
			W.Advance(0.1f); // binds the bridge
		}
		void Emit(const TCHAR* Id) { B->Emit(FName(Id), W.GetWorld(), NAME_None, true); W.Advance(0.1f); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1FallbackAssets, "TriadDrift.Input.MusicFallback.EveryAssignedTrackLoads", TD_TEST_FLAGS)
bool FTDT1FallbackAssets::RunTest(const FString& Parameters)
{
	for (const ETDMusicPlacement P : { ETDMusicPlacement::Credits, ETDMusicPlacement::CinderExplore, ETDMusicPlacement::CinderCombat, ETDMusicPlacement::CoreChamber, ETDMusicPlacement::ReactorFight, ETDMusicPlacement::VerdanceExplore, ETDMusicPlacement::Escape })
	{
		const TArray<FSoftObjectPath>& Tracks = UTDMusicFallbackSubsystem::TracksFor(P);
		TestTrue(*FString::Printf(TEXT("%s has a track"), *UEnum::GetValueAsString(P)), Tracks.Num() > 0);
		for (const FSoftObjectPath& T : Tracks)
		{
			USoundBase* S = Cast<USoundBase>(T.TryLoad());
			TestNotNull(*FString::Printf(TEXT("%s loads"), *T.ToString()), S);
			if (S) { AddInfo(FString::Printf(TEXT("%s: %s, %.1f s%s"), *UEnum::GetValueAsString(P), *S->GetName(), S->GetDuration(), S->IsLooping() ? TEXT(", looping asset") : TEXT(", plays once"))); }
		}
	}
	for (const TCHAR* Path : { TEXT("/Game/TriadDrift/Audio/DominionAssault02/MUS_DominionAssault_PushTheAttack_Full_01.MUS_DominionAssault_PushTheAttack_Full_01"),
		TEXT("/Game/TriadDrift/Audio/IdleAmbient01/MUS_SpaceIdle_Ambient1_Full_01.MUS_SpaceIdle_Ambient1_Full_01"),
		TEXT("/Game/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.MUS_Combat_EtherealPulseRemix_01") })
	{
		TestNotNull(*FString::Printf(TEXT("director fallback %s loads"), Path), Cast<USoundBase>(FSoftObjectPath(Path).TryLoad()));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1FallbackCredits, "TriadDrift.Input.MusicFallback.CreditsAfterTheStingerInOrder", TD_TEST_FLAGS)
bool FTDT1FallbackCredits::RunTest(const FString& Parameters)
{
	TDT1Fallback::FRig R;
	if (!TestNotNull(TEXT("fallback"), R.F) || !TestNotNull(TEXT("bridge"), R.B) || !TestNotNull(TEXT("director"), R.D)) { return false; }
	R.Emit(TEXT("VictoryBegan"));
	R.Emit(TEXT("VictoryStinger"));
	TestEqual(TEXT("not during the stinger"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	R.Emit(TEXT("VictoryStingerDone"));
	TestEqual(TEXT("after the stinger: the credits"), R.F->GetPlayingPlacement(), ETDMusicPlacement::Credits);
	TestTrue(TEXT("... Cathedral Pulse (1) first"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_Credits_CathedralPulse_Full_01")));
	TestTrue(TEXT("the menu music yields to it"), R.D->ShouldMenuMusicYield());
	R.F->NotifyTrackFinishedForTests();
	TestTrue(TEXT("... then Ethereal Pulse (Remix)"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_Credits_EtherealPulseRemix_Full_02")));
	R.F->NotifyTrackFinishedForTests();
	R.W.Advance(0.3f);
	TestEqual(TEXT("both played once: nothing restarts"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	TestFalse(TEXT("the menu music may come back"), R.D->ShouldMenuMusicYield());
	R.Emit(TEXT("VictoryEnded"));
	R.Emit(TEXT("VictoryBegan"));
	R.Emit(TEXT("VictoryStingerDone"));
	R.Emit(TEXT("VictoryEnded"));
	TestEqual(TEXT("the screen closes: the credits stop"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1FallbackCinder, "TriadDrift.Input.MusicFallback.CinderExplorationAndCombat", TD_TEST_FLAGS)
bool FTDT1FallbackCinder::RunTest(const FString& Parameters)
{
	TDT1Fallback::FRig R;
	UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
	if (!R.F || !GI) { return false; }
	GI->NotifyLevelEntered(ETDDestination::Cinder);
	R.W.Advance(0.2f);
	TestEqual(TEXT("under the descent cover: nothing yet"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	if (UTDDescentSubsystem* Desc = UTDDescentSubsystem::Get(R.W.GetWorld())) { Desc->OnRevealed.Broadcast(ETDDestination::Cinder); }
	R.W.Advance(0.2f);
	TestEqual(TEXT("revealed: Cinder World (exploration)"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	TestTrue(TEXT("the director's beds hold meanwhile"), R.D->IsFallbackScorePlaying());

	// On foot: a crew encounter starts and clears (the bridge's GroundCombat / GroundAllClear from ATDEncounterVolume).
	R.B->Emit(UTDMusicEventBridge::GroundCombat, R.W.GetWorld(), TEXT("Cinder"), false);
	R.W.Advance(0.2f);
	TestEqual(TEXT("a crew encounter keeps the selected full World cue"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	TestTrue(TEXT("... MUS_Cinder_Exploration_World_Full_01"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_Cinder_Exploration_World_Full_01")));
	R.B->Emit(UTDMusicEventBridge::GroundAllClear, R.W.GetWorld(), TEXT("Cinder"), false);
	R.W.Advance(0.2f);
	TestEqual(TEXT("all clear keeps the same cue"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	R.Emit(TEXT("Takeoff"));
	TestEqual(TEXT("genuine takeoff ends the planet score"), R.F->GetWantedPlacement(), ETDMusicPlacement::None);
	TestEqual(TEXT("takeoff stops the voice"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);

	GI->NotifyLevelEntered(ETDDestination::Verdance);
	R.W.Advance(0.2f);
	// (a test world keeps the bridge's reveal flag across this level change: no map load resets it)
	TestTrue(TEXT("not Cinder any more: no Cinder track"), R.F->GetPlayingPlacement() != ETDMusicPlacement::CinderExplore && R.F->GetPlayingPlacement() != ETDMusicPlacement::CinderCombat);
	return true;
}

// Lead (Nov 14, from T1's played check: Verdance had ambience only): Nick's Verdance theme (N-059) after the reveal, as a
// marked fallback; his own Verdance player's claim stands it down at once.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1FallbackVerdance, "TriadDrift.Input.MusicFallback.VerdanceThemeAfterTheRevealUntilClaimed", TD_TEST_FLAGS)
bool FTDT1FallbackVerdance::RunTest(const FString& Parameters)
{
	TDT1Fallback::FRig R;
	UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
	if (!R.F || !R.D || !GI) { return false; }
	GI->NotifyLevelEntered(ETDDestination::Verdance);
	R.W.Advance(0.2f);
	TestEqual(TEXT("under the descent cover: nothing yet"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	if (UTDDescentSubsystem* Desc = UTDDescentSubsystem::Get(R.W.GetWorld())) { Desc->OnRevealed.Broadcast(ETDDestination::Verdance); }
	R.W.Advance(0.2f);
	TestEqual(TEXT("revealed: the Verdance theme"), R.F->GetPlayingPlacement(), ETDMusicPlacement::VerdanceExplore);
	TestTrue(TEXT("... MUS_Verdance_Main_Full_01"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_Verdance_Main_Full_01")));
	TestTrue(TEXT("the director's beds hold meanwhile"), R.D->IsFallbackScorePlaying());
	R.D->RequestExclusiveScore(TEXT("NickVerdance"));
	R.W.Advance(0.1f);
	TestEqual(TEXT("Nick's Verdance player claims: the fallback stands down"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	R.D->ReleaseExclusiveScore(TEXT("NickVerdance"), 0.f);
	return true;
}

// Lead (Nov 14): waves 3-5 played the waves 1-2 track. Waves 3-5 take the style's CombatMusicLate slot, else Nick's
// handed-off waves 3-5 track (N-074) as a marked fallback; waves 1-2 keep CombatMusic.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1LateWaveMusic, "TriadDrift.Input.MusicFallback.WavesThreeToFiveHaveTheirOwnTrack", TD_TEST_FLAGS)
bool FTDT1LateWaveMusic::RunTest(const FString& Parameters)
{
	for (const ETDGamePhase Phase : { ETDGamePhase::Waves3to4, ETDGamePhase::Wave5 })
	{
		TDT1Fallback::FRig R;
		UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
		if (!R.D || !GI) { return false; }
		GI->GetMutableProgress().Phase = Phase;
		USoundBase* Music = nullptr;
		for (int32 i = 0; i < 50 && !Music; ++i) { FlushAsyncLoading(); Music = R.D->GetResolvedCombatMusic(); R.W.Advance(0.05f); }
		const FString Name = UEnum::GetValueAsString(Phase);
		if (!TestNotNull(*(Name + TEXT(": a combat track resolves")), Music)) { continue; }
		const bool bFallback = R.D->IsLateWaveFallback();
		TestTrue(*FString::Printf(TEXT("%s: the waves 3-5 track (%s, %s)"), *Name, *Music->GetName(), bFallback ? TEXT("fallback") : TEXT("style slot")),
			!bFallback || Music->GetPathName() == UTDMusicDirectorSubsystem::LateWaveFallbackPath().ToString());
		AddInfo(FString::Printf(TEXT("%s -> %s%s"), *Name, *Music->GetName(), bFallback ? TEXT(" (FALLBACK, N-074)") : TEXT("")));
	}
	{
		TDT1Fallback::FRig R;
		UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
		if (!R.D || !GI) { return false; }
		GI->GetMutableProgress().Phase = ETDGamePhase::Waves1to2;
		R.W.Advance(0.1f);
		TestFalse(TEXT("waves 1-2: never the waves 3-5 fallback"), R.D->IsLateWaveFallback());
		USoundBase* Music = nullptr;
		for (int32 i = 0; i < 50 && !Music; ++i) { FlushAsyncLoading(); Music = R.D->GetResolvedCombatMusic(); R.W.Advance(0.05f); }
		if (Music) { AddInfo(FString::Printf(TEXT("Waves1to2 -> %s (style CombatMusic)"), *Music->GetName())); }
		const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
		TestTrue(TEXT("waves 1-2 resolve the assigned supplied style music, even when reused later"), Style && Music && Music == Style->CombatMusic.LoadSynchronous());
	}
	{
		// #139: a warm cache, then the switch goes off - the engineering fallback is no longer the track; on again, it is back.
		TDT1Fallback::FRig R;
		UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Music.Fallback"));
		if (!R.D || !GI || !CVar) { return false; }
		ON_SCOPE_EXIT { CVar->Set(1, ECVF_SetByCode); };
		GI->GetMutableProgress().Phase = ETDGamePhase::Waves3to4;
		USoundBase* Music = nullptr;
		for (int32 i = 0; i < 50 && !Music; ++i) { FlushAsyncLoading(); Music = R.D->GetResolvedCombatMusic(); R.W.Advance(0.05f); }
		if (Music && R.D->IsLateWaveFallback())
		{
			CVar->Set(0, ECVF_SetByCode);
			TestNull(TEXT("warm cache, fallback off: no engineering waves 3-5 track"), R.D->GetResolvedCombatMusic());
			CVar->Set(1, ECVF_SetByCode);
			TestTrue(TEXT("on again: back"), R.D->GetResolvedCombatMusic() == Music);
		}
		else if (Music)
		{
			// The control: an explicit CombatMusicLate style track is not a fallback - the switch does not touch it.
			CVar->Set(0, ECVF_SetByCode);
			TestTrue(TEXT("explicit style track: unaffected by the fallback switch"), R.D->GetResolvedCombatMusic() == Music);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1FallbackInterior, "TriadDrift.Input.MusicFallback.CoreChamberReactorAndEscape", TD_TEST_FLAGS)
bool FTDT1FallbackInterior::RunTest(const FString& Parameters)
{
	TDT1Fallback::FRig R;
	if (!R.F) { return false; }
	// T5's real hook: the interior course's moment reaches the bridge and the fallback.
	ATDPlanetCourse::OnInteriorMoment.Broadcast(UTDMusicEventBridge::CoreChamberEntered, nullptr);
	R.W.Advance(0.1f);
	TestEqual(TEXT("the core chamber (from ATDPlanetCourse::OnInteriorMoment): Cathedral Pulse"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CoreChamber);
	TestTrue(TEXT("... DominionCore01"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_DominionCore_CathedralPulse_Full_01")));
	R.Emit(TEXT("RevealEnded"));
	TestEqual(TEXT("the reveal plays inside the chamber's track"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CoreChamber);
	ATDPlanetCourse::OnInteriorMoment.Broadcast(UTDMusicEventBridge::ReactorFightStarted, nullptr);
	R.W.Advance(0.1f);
	TestEqual(TEXT("the reactor fight: Infinity Zone"), R.F->GetPlayingPlacement(), ETDMusicPlacement::ReactorFight);
	TestTrue(TEXT("... DominionAssault01"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_DominionAssault_InfinityZone_Full_01")));
	ATDPlanetCourse::OnInteriorMoment.Broadcast(UTDMusicEventBridge::ReactorFightEnded, nullptr);
	R.W.Advance(0.1f);
	TestEqual(TEXT("the heart is destroyed: the fight's track ends"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	ATDPlanetCourse::OnInteriorMoment.Broadcast(UTDMusicEventBridge::EscapeStarted, nullptr);
	R.W.Advance(0.1f);
	TestEqual(TEXT("the escape is wanted"), R.F->GetWantedPlacement(), ETDMusicPlacement::Escape);
	TestEqual(TEXT("Ritual owns the escape"), R.F->GetPlayingPlacement(), ETDMusicPlacement::Escape);
	TestTrue(TEXT("the supplied full Ritual master"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_DominionEscape_Ritual_Full_01")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1EscapeMusicRestore, "TriadDrift.Input.MusicFallback.EscapeRestoreReservesScoreBeforeTheFirstTick", TD_TEST_FLAGS)
bool FTDT1EscapeMusicRestore::RunTest(const FString& Parameters)
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Music.Fallback"));
	if (!TestNotNull(TEXT("fallback switch"), CVar)) { return false; }
	const int32 Was = CVar->GetInt();
	ON_SCOPE_EXIT { CVar->Set(Was, ECVF_SetByCode); };
	CVar->Set(1, ECVF_SetByCode);
	// Separate fresh worlds model late binding on a ground restore and the new galaxy after travel: neither emits
	// EscapeStarted. This verifies routing/ownership, not disk Continue, seamless audio travel or listening.
	for (const ETDDestination Destination : { ETDDestination::DominionInterior, ETDDestination::Galaxy })
	{
		TDT1Fallback::FRig R;
		UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
		if (!R.F || !R.D || !GI) { return false; }
		GI->NotifyLevelEntered(Destination);
		GI->GetMutableProgress().Phase = Destination == ETDDestination::Galaxy ? ETDGamePhase::Escape : ETDGamePhase::DominionInterior;
		GI->GetMutableProgress().CompletedObjectives.Add(FName(TEXT("Obj_DI_Core")));
		R.D->SetCombatOverrideForTests(true);
		R.D->SetMusicForTests(NewObject<USoundWave>());
		const int32 Before = R.D->GetStartCount();
		TestTrue(TEXT("escape reserves the bed before its fallback tick"), R.F->IsFallbackScorePlaying());
		R.D->Tick(0.05f); // deliberately tick the generic director first
		TestEqual(TEXT("no generic combat start in the restore gap"), R.D->GetStartCount(), Before);
		R.W.Advance(0.2f);
		TestEqual(TEXT("restored escape wanted without replaying its event"), R.F->GetWantedPlacement(), ETDMusicPlacement::Escape);
		TestEqual(TEXT("the imported Ritual route plays"), R.F->GetPlayingPlacement(), ETDMusicPlacement::Escape);
		TestTrue(TEXT("the correct full master route"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_DominionEscape_Ritual_Full_01")));
		R.Emit(TEXT("CoreChamberEntered"));
		R.Emit(TEXT("ReactorFightStarted"));
		TestEqual(TEXT("late earlier moments cannot downgrade restored escape"), R.F->GetWantedPlacement(), ETDMusicPlacement::Escape);
		TestEqual(TEXT("the generic bed never restarted"), R.D->GetStartCount(), Before);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1EscapeMusicOwnership, "TriadDrift.Input.MusicFallback.EscapeYieldsAndHandsOffToVictory", TD_TEST_FLAGS)
bool FTDT1EscapeMusicOwnership::RunTest(const FString& Parameters)
{
	TDT1Fallback::FRig R;
	UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Music.Fallback"));
	if (!R.F || !R.D || !GI || !CVar) { return false; }
	const int32 Was = CVar->GetInt();
	ON_SCOPE_EXIT { CVar->Set(Was, ECVF_SetByCode); };
	CVar->Set(1, ECVF_SetByCode);
	GI->GetMutableProgress().Phase = ETDGamePhase::Escape;
	R.W.Advance(0.1f);
	TestEqual(TEXT("escape starts"), R.F->GetPlayingPlacement(), ETDMusicPlacement::Escape);
	R.D->RequestExclusiveScore(TEXT("EscapeOwnerControl"));
	R.W.Advance(0.1f);
	TestEqual(TEXT("exclusive score still wins"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	R.D->ReleaseExclusiveScore(TEXT("EscapeOwnerControl"), 0.f);
	R.W.Advance(0.2f);
	TestEqual(TEXT("Ritual resumes after the owner releases"), R.F->GetPlayingPlacement(), ETDMusicPlacement::Escape);
	CVar->Set(0, ECVF_SetByCode);
	R.W.Advance(0.1f);
	TestEqual(TEXT("fallback switch still stops the voice"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	TestFalse(TEXT("disabled escape does not reserve the bed"), R.F->IsFallbackScorePlaying());
	CVar->Set(1, ECVF_SetByCode);
	R.W.Advance(0.1f);
	TestEqual(TEXT("enabled escape returns"), R.F->GetPlayingPlacement(), ETDMusicPlacement::Escape);
	R.Emit(TEXT("VictoryBegan")); // event can precede the phase snapshot
	R.Emit(TEXT("EscapeStarted")); // a late old-world moment must not restart the score over the stinger
	TestEqual(TEXT("no escape over the victory stinger"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	GI->GetMutableProgress().Phase = ETDGamePhase::Victory;
	R.Emit(TEXT("VictoryStingerDone"));
	TestEqual(TEXT("the existing credits playlist follows"), R.F->GetPlayingPlacement(), ETDMusicPlacement::Credits);
	R.Emit(TEXT("VictoryEnded"));
	TestEqual(TEXT("credits and old escape both end"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	TestFalse(TEXT("victory releases the escape bed reservation"), R.F->IsFallbackScorePlaying());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1CinderSavedPadMusic, "TriadDrift.Input.MusicFallback.CinderSavedPadRestoreWithoutReveal", TD_TEST_FLAGS)
bool FTDT1CinderSavedPadMusic::RunTest(const FString& Parameters)
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Music.Fallback"));
	if (!TestNotNull(TEXT("fallback switch"), CVar)) { return false; }
	const int32 Was = CVar->GetInt();
	ON_SCOPE_EXIT { CVar->Set(Was, ECVF_SetByCode); };
	CVar->Set(1, ECVF_SetByCode);
	TDT1Fallback::FRig R;
	UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
	if (!R.F || !R.B || !R.D || !GI) { return false; }
	UTDSaveGame* Save = NewObject<UTDSaveGame>();
	Save->Checkpoint.bValid = true;
	Save->Checkpoint.Destination = ETDDestination::Cinder;
	Save->Checkpoint.Progress.Phase = ETDGamePhase::Cinder;
	Save->Checkpoint.Progress.ReturnPoint = TEXT("Cinder_Pad");
	if (!TestTrue(TEXT("in-memory ground checkpoint applies"), GI->ApplySave(*Save))) { return false; }
	TestFalse(TEXT("no reveal event was replayed"), R.B->GetMusicState().bPlanetRevealed);
	R.W.Advance(0.2f);
	TestEqual(TEXT("saved pad wants exploration without reveal"), R.F->GetWantedPlacement(), ETDMusicPlacement::CinderExplore);
	TestEqual(TEXT("supplied Cinder exploration plays"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	R.Emit(TEXT("GroundCombat"));
	TestEqual(TEXT("restored encounter keeps the selected full cue"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	R.Emit(TEXT("GroundAllClear"));
	TestEqual(TEXT("all clear returns exploration"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	R.D->RequestExclusiveScore(TEXT("CinderRestoreOwner"));
	R.W.Advance(0.1f);
	TestEqual(TEXT("exclusive owner still stands fallback down"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	TestEqual(TEXT("saved placement still wanted during claim"), R.F->GetWantedPlacement(), ETDMusicPlacement::CinderExplore);
	R.D->ReleaseExclusiveScore(TEXT("CinderRestoreOwner"), 0.f);
	R.W.Advance(0.2f);
	TestEqual(TEXT("released owner returns restored score"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	CVar->Set(0, ECVF_SetByCode);
	R.W.Advance(0.1f);
	TestEqual(TEXT("switch still stops restored score"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	CVar->Set(1, ECVF_SetByCode);
	R.W.Advance(0.1f);
	TestEqual(TEXT("reenabled restored score returns"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);
	TestFalse(TEXT("restore correction never changes global reveal state"), R.B->GetMusicState().bPlanetRevealed);
	GI->GetMutableProgress().ReturnPoint = NAME_None; // the planet director clears this on reboarding
	R.W.Advance(0.1f);
	TestEqual(TEXT("cleared pad no longer bypasses reveal gate"), R.F->GetWantedPlacement(), ETDMusicPlacement::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1CinderSavedPadNegatives, "TriadDrift.Input.MusicFallback.CinderRestoreDoesNotBypassFreshArrivalGate", TD_TEST_FLAGS)
bool FTDT1CinderSavedPadNegatives::RunTest(const FString& Parameters)
{
	struct FCase { const TCHAR* Name; ETDDestination Destination; ETDGamePhase Phase; FName Pad; };
	const FCase Cases[] = {
		{ TEXT("fresh arrival"), ETDDestination::Cinder, ETDGamePhase::Cinder, NAME_None },
		{ TEXT("wrong pad"), ETDDestination::Cinder, ETDGamePhase::Cinder, TEXT("Verdance_Pad") },
		{ TEXT("wrong phase"), ETDDestination::Cinder, ETDGamePhase::Wave5, TEXT("Cinder_Pad") },
		{ TEXT("wrong destination"), ETDDestination::Galaxy, ETDGamePhase::Cinder, TEXT("Cinder_Pad") },
	};
	for (const FCase& C : Cases)
	{
		TDT1Fallback::FRig R;
		UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
		if (!R.F || !R.B || !GI) { return false; }
		UTDSaveGame* Save = NewObject<UTDSaveGame>();
		Save->Checkpoint.bValid = true;
		Save->Checkpoint.Destination = C.Destination;
		Save->Checkpoint.Progress.Phase = C.Phase;
		Save->Checkpoint.Progress.ReturnPoint = C.Pad;
		if (!TestTrue(FString(C.Name) + TEXT(": applies"), GI->ApplySave(*Save))) { return false; }
		R.W.Advance(0.2f);
		TestFalse(FString(C.Name) + TEXT(": no fabricated reveal"), R.B->GetMusicState().bPlanetRevealed);
		TestEqual(FString(C.Name) + TEXT(": no Cinder placement"), R.F->GetWantedPlacement(), ETDMusicPlacement::None);
		if (C.Pad.IsNone())
		{
			if (UTDDescentSubsystem* Desc = UTDDescentSubsystem::Get(R.W.GetWorld())) { Desc->OnRevealed.Broadcast(ETDDestination::Cinder); }
			R.W.Advance(0.2f);
			TestEqual(TEXT("fresh arrival still starts on its normal reveal"), R.F->GetWantedPlacement(), ETDMusicPlacement::CinderExplore);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1CinderCueContinuity, "TriadDrift.Input.MusicFallback.CinderCueSurvivesRoutineEventsWithoutRestart", TD_TEST_FLAGS)
bool FTDT1CinderCueContinuity::RunTest(const FString& Parameters)
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Music.Fallback"));
	if (!TestNotNull(TEXT("fallback switch"), CVar)) { return false; }
	const int32 Was = CVar->GetInt();
	ON_SCOPE_EXIT { CVar->Set(Was, ECVF_SetByCode); };
	CVar->Set(1, ECVF_SetByCode);
	for (const bool bInitiallyFighting : { false, true })
	{
		TDT1Fallback::FRig R;
		UTDGameInstance* GI = Cast<UTDGameInstance>(R.W.GetGameInstance());
		if (!R.F || !R.B || !R.D || !GI) { return false; }
		UTDSaveGame* Save = NewObject<UTDSaveGame>();
		Save->Checkpoint.bValid = true;
		Save->Checkpoint.Destination = ETDDestination::Cinder;
		Save->Checkpoint.Progress.Phase = ETDGamePhase::Cinder;
		Save->Checkpoint.Progress.ReturnPoint = TEXT("Cinder_Pad");
		if (!TestTrue(TEXT("Cinder checkpoint applies"), GI->ApplySave(*Save))) { return false; }
		TWeakObjectPtr<UAudioComponent> ObservedVoice;
		int32 PositionReports = 0;
		float LastPercent = -1.f;
		bool bPositionRegressed = false;
		FDelegateHandle PositionHandle;
		R.F->SetVoiceCreatedForTests([&](UAudioComponent* Component)
		{
			ObservedVoice = Component;
			PositionHandle = Component->OnAudioPlaybackPercentNative.AddLambda(
				[&](const UAudioComponent*, const USoundWave*, const float Percent)
				{
					// This sample is much shorter than either full cue: no natural wrap should occur here.
					if (PositionReports > 0 && Percent + 0.0001f < LastPercent) { bPositionRegressed = true; }
					LastPercent = Percent;
					++PositionReports;
				});
		});
		ON_SCOPE_EXIT
		{
			R.F->SetVoiceCreatedForTests(nullptr);
			if (ObservedVoice.IsValid()) { ObservedVoice->OnAudioPlaybackPercentNative.Remove(PositionHandle); }
		};
		if (bInitiallyFighting) { R.B->Emit(UTDMusicEventBridge::GroundCombat, R.W.GetWorld(), TEXT("Cinder"), false); }
		R.F->Tick(0.01f); // seed the initial cue before any routine encounter update
		const ETDMusicPlacement Selected = bInitiallyFighting ? ETDMusicPlacement::CinderCombat : ETDMusicPlacement::CinderExplore;
		TestEqual(TEXT("initial state selects its assigned cue"), R.F->GetPlayingPlacement(), Selected);
		const int32 Starts = R.F->GetTrackStartCountForTests();
		const FSoftObjectPath Track = R.F->GetPlayingTrack();
		USoundBase* Sound = Cast<USoundBase>(Track.TryLoad());
		if (!TestNotNull(TEXT("selected full cue loads"), Sound)) { return false; }
		TestTrue(TEXT("selected asset loops in place at its natural end"), Sound->IsLooping());
		const uint32 PlayOrder = ObservedVoice.IsValid() ? ObservedVoice->GetLastPlayOrder() : 0;
		for (int32 i = 0; i < 12; ++i)
		{
			R.B->Emit(i % 2 == 0 ? UTDMusicEventBridge::GroundCombat : UTDMusicEventBridge::GroundAllClear,
				R.W.GetWorld(), TEXT("Cinder"), false);
			R.B->Emit(UTDMusicEventBridge::PhaseChanged, R.W.GetWorld(), TEXT("Cinder"), false);
			R.B->Emit(UTDMusicEventBridge::TransmissionStarted, R.W.GetWorld(), TEXT("G5_Wren_ClearRoute"), false);
			R.B->Emit(UTDMusicEventBridge::TransmissionEnded, R.W.GetWorld(), TEXT("G5_Wren_ClearRoute"), true);
			R.W.Advance(0.5f);
			TestEqual(TEXT("routine events keep the selected placement"), R.F->GetPlayingPlacement(), Selected);
			TestTrue(TEXT("routine events keep the full track"), R.F->GetPlayingTrack() == Track);
			TestEqual(TEXT("routine events never call StartTrack again"), R.F->GetTrackStartCountForTests(), Starts);
			TestTrue(TEXT("routine events retain the same audio component"), R.F->GetVoiceForTests() == ObservedVoice.Get());
			if (ObservedVoice.IsValid())
			{
				TestEqual(TEXT("the component itself was not replayed"), ObservedVoice->GetLastPlayOrder(), PlayOrder);
			}
		}
		if (PositionReports >= 2) { TestFalse(TEXT("reported playback position never jumps back during routine events"), bPositionRegressed); }
		else { AddInfo(TEXT("Audio position callbacks unavailable in this test run; start/component continuity checked, actual playback position still needs audio-on native verification.")); }
		R.F->SetVoiceCreatedForTests(nullptr);
		if (ObservedVoice.IsValid()) { ObservedVoice->OnAudioPlaybackPercentNative.Remove(PositionHandle); }
		R.D->RequestExclusiveScore(TEXT("CinderContinuityOwner"));
		R.W.Advance(0.1f);
		TestEqual(TEXT("real score owner still interrupts"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
		TestEqual(TEXT("claim retains selection"), R.F->GetWantedPlacement(), Selected);
		R.D->ReleaseExclusiveScore(TEXT("CinderContinuityOwner"), 0.f);
		R.W.Advance(0.2f);
		TestEqual(TEXT("owner release restores the same selected cue"), R.F->GetPlayingPlacement(), Selected);
		R.Emit(TEXT("CoreChamberEntered"));
		TestEqual(TEXT("critical scene still replaces the planet cue"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CoreChamber);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1FallbackStandDown, "TriadDrift.Input.MusicFallback.StandsDownForAnyClaimAndTheSwitch", TD_TEST_FLAGS)
bool FTDT1FallbackStandDown::RunTest(const FString& Parameters)
{
	TDT1Fallback::FRig R;
	if (!R.F || !R.D) { return false; }
	R.Emit(TEXT("CoreChamberEntered"));
	TestEqual(TEXT("playing"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CoreChamber);
	R.D->RequestExclusiveScore(TEXT("NickAudio"));
	R.W.Advance(0.1f);
	TestEqual(TEXT("any claim: stands down at once"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	TestEqual(TEXT("... the moment still applies"), R.F->GetWantedPlacement(), ETDMusicPlacement::CoreChamber);
	R.D->ReleaseExclusiveScore(TEXT("NickAudio"), 0.f);
	R.W.Advance(0.2f);
	TestEqual(TEXT("released: back"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CoreChamber);

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Music.Fallback"));
	if (!TestNotNull(TEXT("td.Music.Fallback"), CVar)) { return false; }
	ON_SCOPE_EXIT { CVar->Set(1, ECVF_SetByCode); };
	CVar->Set(0, ECVF_SetByCode);
	R.W.Advance(0.1f);
	TestEqual(TEXT("td.Music.Fallback 0: off"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
	TestFalse(TEXT("... and the beds are free"), R.D->IsFallbackScorePlaying());
	CVar->Set(1, ECVF_SetByCode);
	R.W.Advance(0.1f);
	TestEqual(TEXT("on again: back"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CoreChamber);
	return true;
}

#endif
