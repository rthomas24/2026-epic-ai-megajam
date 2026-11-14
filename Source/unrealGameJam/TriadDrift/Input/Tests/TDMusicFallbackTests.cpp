// R-047 the engineering music fallback. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Input.MusicFallback

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Input/TDMusicFallbackSubsystem.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Core/TDMusicEventBridge.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
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
	for (const ETDMusicPlacement P : { ETDMusicPlacement::Credits, ETDMusicPlacement::CinderExplore, ETDMusicPlacement::CinderCombat, ETDMusicPlacement::CoreChamber, ETDMusicPlacement::ReactorFight })
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
	TestEqual(TEXT("the escape (Ritual) is not imported: empty, nothing plays"), UTDMusicFallbackSubsystem::TracksFor(ETDMusicPlacement::Escape).Num(), 0);
	for (const TCHAR* Path : { TEXT("/Game/TriadDrift/Audio/DominionAssault02/MUS_DominionAssault_PushTheAttack_Full_01.MUS_DominionAssault_PushTheAttack_Full_01"),
		TEXT("/Game/TriadDrift/Audio/IdleAmbient01/MUS_SpaceIdle_Ambient1_Full_01.MUS_SpaceIdle_Ambient1_Full_01") })
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
	TestEqual(TEXT("a crew encounter: Cinder Fall (combat)"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderCombat);
	TestTrue(TEXT("... MUS_Cinder_Combat_Fall_Full_01"), R.F->GetPlayingTrack().ToString().Contains(TEXT("MUS_Cinder_Combat_Fall_Full_01")));
	R.B->Emit(UTDMusicEventBridge::GroundAllClear, R.W.GetWorld(), TEXT("Cinder"), false);
	R.W.Advance(0.2f);
	TestEqual(TEXT("all clear: exploration again"), R.F->GetPlayingPlacement(), ETDMusicPlacement::CinderExplore);

	GI->NotifyLevelEntered(ETDDestination::Verdance);
	R.W.Advance(0.2f);
	TestEqual(TEXT("not Cinder: nothing (Verdance is Nick's own theme)"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
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
	TestEqual(TEXT("... but Ritual isn't imported: nothing plays (the beds carry on)"), R.F->GetPlayingPlacement(), ETDMusicPlacement::None);
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
