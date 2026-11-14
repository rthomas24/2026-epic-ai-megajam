// Triad Drift - profile tests: the first-play tutorial flag and "skip the tutorial". Owner: Teammate 7.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Profile

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Profile/TDProfileSubsystem.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"

namespace TDProfileTest
{
	static const TCHAR* Slot = TEXT("TD_Profile_AutomationTest");

	static void DeleteSlot()
	{
		const int32 User = UTDFlowSettings::Get()->SaveUserIndex;
		if (UGameplayStatics::DoesSaveGameExist(Slot, User))
		{
			UGameplayStatics::DeleteGameInSlot(Slot, User);
		}
	}

	/** No run save writes and a clean profile slot for the whole test; both restored after. */
	struct FScope
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		bool bWrite = S->bWriteSlotOnCheckpoint;
		FScope() { S->bWriteSlotOnCheckpoint = false; DeleteSlot(); }
		~FScope() { S->bWriteSlotOnCheckpoint = bWrite; DeleteSlot(); }
	};

	static UTDProfileSubsystem* Profile(FTDTestWorld& W)
	{
		UGameInstance* GI = W.GetGameInstance();
		UTDProfileSubsystem* P = GI ? GI->GetSubsystem<UTDProfileSubsystem>() : nullptr;
		if (P)
		{
			P->SetSlotOverrideForTests(Slot);
		}
		return P;
	}
}

// A fresh profile never sees the offer, and a skip is refused without changing the run.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDProfileFresh, "TriadDrift.Profile.FreshProfileNeverOffersSkip", TD_TEST_FLAGS)
bool FTDProfileFresh::RunTest(const FString& Parameters)
{
	TDProfileTest::FScope Scope;
	FTDTestWorld W(UTDGameInstance::StaticClass(), ATDMenuGameMode::StaticClass());
	UTDProfileSubsystem* P = TDProfileTest::Profile(W);
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	const ATDMenuGameMode* Menu = W.GetWorld() ? W.GetWorld()->GetAuthGameMode<ATDMenuGameMode>() : nullptr;
	if (!TestNotNull(TEXT("profile"), P) || !TestNotNull(TEXT("game instance"), GI) || !TestNotNull(TEXT("menu"), Menu))
	{
		return false;
	}
	TestFalse(TEXT("fresh: not complete"), P->IsTutorialComplete());
	TestFalse(TEXT("fresh: no skip offer"), P->CanSkipTutorial());
	TestFalse(TEXT("fresh: the menu does not offer it"), Menu->CanSkipTutorial());
	TestFalse(TEXT("fresh: skip refused"), P->StartNewRunSkippingTutorial());
	TestFalse(TEXT("refused: nothing armed"), P->IsSkipTutorialPending());
	TestFalse(TEXT("refused: no run started"), GI->IsRunCommitted());
	TestFalse(TEXT("refused: no intro armed"), GI->IsIntroPending());
	return true;
}

// Finishing the tutorial sets the flag once, fires Aftermath once, and the flag survives a new session (a new game instance).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDProfilePersists, "TriadDrift.Profile.CompletionPersistsAcrossSessions", TD_TEST_FLAGS)
bool FTDProfilePersists::RunTest(const FString& Parameters)
{
	TDProfileTest::FScope Scope;
	{
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		W.Advance(0.2f);
		UTDProfileSubsystem* P = TDProfileTest::Profile(W);
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!TestNotNull(TEXT("profile"), P) || !TestNotNull(TEXT("game instance"), GI))
		{
			return false;
		}
		int32 Fired = 0;
		bool bSkippedArg = true;
		P->OnAftermathBegins.AddLambda([&Fired, &bSkippedArg](bool bSkipped) { ++Fired; bSkippedArg = bSkipped; });

		GI->StartNewRun();
		GI->SetPhase(ETDGamePhase::Waves1to2);
		TestFalse(TEXT("during the tutorial: not complete"), P->IsTutorialComplete());
		GI->ReportWaveCleared(1);
		TestFalse(TEXT("wave 1 cleared alone ends nothing"), P->IsTutorialComplete());

		GI->SetPhase(ETDGamePhase::Verdance); // the fallback: the run leaves the tutorial battle
		TestTrue(TEXT("complete after leaving the tutorial"), P->IsTutorialComplete());
		TestEqual(TEXT("Aftermath fired once"), Fired, 1);
		TestFalse(TEXT("Aftermath: not a skip"), bSkippedArg);
		// Exactly once: a story director alive in this world (T3's UTDStorySubsystem binds OnAftermathBegins) takes it
		// as it fires, so nothing stays pending; with no director yet, it waits for a late one (T3's story test covers
		// the late binder playing it from IsAftermathPending).
		if (UTDStorySubsystem::Get(W.GetWorld()))
		{
			TestFalse(TEXT("the live story director took Aftermath (nothing left pending)"), P->IsAftermathPending());
		}
		else
		{
			TestTrue(TEXT("no director yet: Aftermath pending for a late one"), P->ConsumeAftermathPending());
		}
		TestFalse(TEXT("taken once: nothing left"), P->ConsumeAftermathPending());

		P->NotifyTutorialCompleted(TEXT("T4's escape event, later the same run"));
		TestEqual(TEXT("once per run"), Fired, 1);
	}
	{
		FTDTestWorld W2(UTDGameInstance::StaticClass(), ATDMenuGameMode::StaticClass());
		UTDProfileSubsystem* P2 = TDProfileTest::Profile(W2); // loads the slot written above
		const ATDMenuGameMode* Menu = W2.GetWorld() ? W2.GetWorld()->GetAuthGameMode<ATDMenuGameMode>() : nullptr;
		if (!TestNotNull(TEXT("profile (session 2)"), P2) || !TestNotNull(TEXT("menu (session 2)"), Menu))
		{
			return false;
		}
		TestTrue(TEXT("session 2: still complete"), P2->IsTutorialComplete());
		TestTrue(TEXT("session 2: the menu offers the skip"), Menu->CanSkipTutorial());

		P2->ResetProfile();
		P2->Load();
		TestFalse(TEXT("reset is written too"), P2->IsTutorialComplete());
	}
	return true;
}

// What counts as finishing it: leaving Waves1to2 in play. A cheat stage (a new run first), Intro -> later, and a corrupt
// slot never set it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDProfileRules, "TriadDrift.Profile.OnlyTheRealTutorialCounts", TD_TEST_FLAGS)
bool FTDProfileRules::RunTest(const FString& Parameters)
{
	TDProfileTest::FScope Scope;
	FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
	W.Advance(0.2f);
	UTDProfileSubsystem* P = TDProfileTest::Profile(W);
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("profile"), P) || !TestNotNull(TEXT("game instance"), GI))
	{
		return false;
	}
	GI->SetPhase(ETDGamePhase::Waves1to2);
	GI->StartNewRun(); // TD.Stage does this before it builds a later stage
	GI->SetPhase(ETDGamePhase::Cinder);
	TestFalse(TEXT("cheat stage after a new run: not complete"), P->IsTutorialComplete());

	GI->StartNewRun();
	GI->SetPhase(ETDGamePhase::Verdance); // Intro -> Verdance (a skip's shape)
	TestFalse(TEXT("Intro -> later: not complete"), P->IsTutorialComplete());

	// A corrupt profile file loads as a fresh profile, without asserting.
	TArray<uint8> Junk; Junk.Init(0x5A, 128);
	UGameplayStatics::SaveDataToSlot(Junk, TDProfileTest::Slot, UTDFlowSettings::Get()->SaveUserIndex);
	AddExpectedError(TEXT("is corrupt"), EAutomationExpectedErrorFlags::Contains, 0);
	P->Load();
	TestFalse(TEXT("corrupt slot = fresh profile"), P->IsTutorialComplete());
	return true;
}

// Skipping lands at Aftermath with the state a finished tutorial leaves; a later plain New Game plays the tutorial again.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDProfileSkip, "TriadDrift.Profile.SkipLandsInAftermath", TD_TEST_FLAGS)
bool FTDProfileSkip::RunTest(const FString& Parameters)
{
	TDProfileTest::FScope Scope;
	FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
	W.Advance(0.2f);
	UTDProfileSubsystem* P = TDProfileTest::Profile(W);
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("profile"), P) || !TestNotNull(TEXT("game instance"), GI))
	{
		return false;
	}
	TestEqual(TEXT("in the galaxy"), GI->GetCurrentDestination(), ETDDestination::Galaxy);
	P->NotifyTutorialCompleted(TEXT("test: an earlier run"));
	TestTrue(TEXT("offer now"), P->CanSkipTutorial());

	int32 Fired = 0;
	bool bSkippedArg = false;
	P->OnAftermathBegins.AddLambda([&Fired, &bSkippedArg](bool bSkipped) { ++Fired; bSkippedArg = bSkipped; });

	TestTrue(TEXT("skip accepted"), P->StartNewRunSkippingTutorial());
	TestTrue(TEXT("a New Game: the intro still plays"), GI->IsIntroPending());
	TestTrue(TEXT("skip armed"), P->IsSkipTutorialPending());
	TestEqual(TEXT("still Intro until the intro ends"), GI->GetProgressRef().Phase, ETDGamePhase::Intro);

	// The intro ends (watched or skipped): the game mode leaves it through LeaveIntro.
	TestTrue(TEXT("LeaveIntro applied the skip"), UTDProfileSubsystem::LeaveIntro(GI));
	const FTDProgress& Pr = GI->GetProgressRef();
	TestEqual(TEXT("phase after the tutorial"), Pr.Phase, ETDGamePhase::Verdance);
	TestEqual(TEXT("waves 1-2 cleared"), Pr.WavesCleared, 2);
	TestEqual(TEXT("nothing unlocked (earned on the planets)"), Pr.UnlockedAbilities.Num(), 0);
	TestEqual(TEXT("no objectives done"), Pr.CompletedObjectives.Num(), 0);
	TestFalse(TEXT("Dominion not revealed"), Pr.bDominionRevealed);
	TestTrue(TEXT("checkpoint taken"), GI->GetLastCheckpoint().bValid);
	TestEqual(TEXT("checkpoint phase"), GI->GetLastCheckpoint().Progress.Phase, ETDGamePhase::Verdance);
	TestEqual(TEXT("checkpoint waves"), GI->GetLastCheckpoint().Progress.WavesCleared, 2);
	TestEqual(TEXT("checkpoint in the galaxy"), GI->GetLastCheckpoint().Destination, ETDDestination::Galaxy);
	TestEqual(TEXT("Aftermath fired once"), Fired, 1);
	TestTrue(TEXT("Aftermath: a skip"), bSkippedArg);
	TestFalse(TEXT("skip consumed"), P->IsSkipTutorialPending());
	TestTrue(TEXT("this run skipped"), P->DidRunSkipTutorial());

	// The wave director in the skipped state starts no tutorial wave.
	if (ATDWaveDirector* Director = ATDWaveDirector::Get(W.GetWorld()))
	{
		Director->StopAndClear();
		Director->StartFromProgress();
		W.Advance(3.f);
		TestEqual(TEXT("no wave runs at Aftermath"), Director->GetCurrentWaveIndex(), (int32)INDEX_NONE);
	}

	// A skipped run is not a completion.
	P->NotifyTutorialCompleted(TEXT("test: should be ignored"));
	TestEqual(TEXT("no Aftermath again from a skipped run"), Fired, 1);

	// A later plain New Game plays the tutorial: nothing armed.
	GI->StartNewRun();
	TestFalse(TEXT("plain New Game: no skip"), P->IsSkipTutorialPending());
	TestFalse(TEXT("plain New Game: not a skipped run"), P->DidRunSkipTutorial());
	TestFalse(TEXT("plain New Game: LeaveIntro does not skip"), UTDProfileSubsystem::LeaveIntro(GI));
	TestEqual(TEXT("plain New Game: the tutorial battle"), GI->GetProgressRef().Phase, ETDGamePhase::Waves1to2);
	TestEqual(TEXT("plain New Game: no waves cleared"), GI->GetProgressRef().WavesCleared, 0);
	return true;
}

#endif
