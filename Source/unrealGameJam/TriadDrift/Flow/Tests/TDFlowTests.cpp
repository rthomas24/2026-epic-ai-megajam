// Triad Drift - Flow automation tests (ENG-19, C-03 #3). Owner: Teammate 5.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flow

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDSaveGame.h"
#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace TDFlowTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	static const TCHAR* TestSlot = TEXT("TriadDrift_AutomationTest");

	/** A real, initialized UTDGameInstance (with its event bus) on a throwaway world and test-only save slot. */
	struct FScopedGI
	{
		UTDGameInstance* GI = nullptr;
		UTDFlowTestListener* Listener = nullptr;

		FScopedGI()
		{
			GI = NewObject<UTDGameInstance>(GEngine);
			GI->AddToRoot();
			GI->SetSaveSlotOverride(TestSlot);
			GI->InitializeStandalone(TEXT("TDFlowTestWorld"));
			GI->SetSaveSlotOverride(TestSlot);
			GI->DeleteSave();
			Listener = NewObject<UTDFlowTestListener>();
			Listener->AddToRoot();
			Listener->Bind(Bus());
		}

		~FScopedGI()
		{
			GI->DeleteSave();
			UWorld* World = GI->GetWorld();
			GI->Shutdown();
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			Listener->RemoveFromRoot();
			GI->RemoveFromRoot();
		}

		UTDEventBus* Bus() const { return GI->GetSubsystem<UTDEventBus>(); }
		const FTDProgress& P() const { return GI->GetProgressRef(); }
	};
}

using namespace TDFlowTest;

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowProgressMutations, "TriadDrift.Flow.ProgressMutations", TDFlowTest::Flags)
bool FTDFlowProgressMutations::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;

	TestEqual(TEXT("new run starts in Intro"), S.P().Phase, ETDGamePhase::Intro);
	GI->SetPhase(ETDGamePhase::Waves1to2);
	TestEqual(TEXT("SetPhase"), S.P().Phase, ETDGamePhase::Waves1to2);

	GI->ReportWaveCleared(1);
	TestEqual(TEXT("wave 1 cleared"), S.P().WavesCleared, 1);
	GI->ReportWaveCleared(1);
	TestEqual(TEXT("ReportWaveCleared is idempotent"), S.P().WavesCleared, 1);
	GI->ReportWaveCleared(0);
	GI->ReportWaveCleared(-3);
	TestEqual(TEXT("invalid wave numbers ignored"), S.P().WavesCleared, 1);

	GI->ReportWaveCleared(2);
	TestEqual(TEXT("wave 2 cleared"), S.P().WavesCleared, 2);
	TestEqual(TEXT("clearing wave 2 enters Verdance"), S.P().Phase, ETDGamePhase::Verdance);

	GI->SetDominionState(true, false);
	TestTrue(TEXT("Dominion revealed"), S.P().bDominionRevealed);
	TestFalse(TEXT("defenses still up"), S.P().bDominionDefensesDown);
	GI->SetDominionState(true, true);
	TestTrue(TEXT("defenses down"), S.P().bDominionDefensesDown);

	GI->CompleteObjective(TEXT("Obj_Jammer"), ETDAbilityId::None, ETDShipId::Rift);
	TestTrue(TEXT("objective recorded"), S.P().CompletedObjectives.Contains(TEXT("Obj_Jammer")));
	TestEqual(TEXT("no ability for a no-reward objective"), S.P().UnlockedAbilities.Num(), 0);
	GI->CompleteObjective(NAME_None, ETDAbilityId::Cloak, ETDShipId::Rift);
	TestEqual(TEXT("unnamed objective ignored"), S.P().CompletedObjectives.Num(), 1);

	GI->SetLastShip(ETDShipId::Bastion);
	TestEqual(TEXT("last ship"), S.P().LastShip, ETDShipId::Bastion);

	GI->StartNewRun();
	TestEqual(TEXT("new run resets waves"), S.P().WavesCleared, 0);
	TestEqual(TEXT("new run resets phase"), S.P().Phase, ETDGamePhase::Intro);
	TestFalse(TEXT("new run clears the checkpoint"), GI->GetLastCheckpoint().bValid);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowAbilityUnlocks, "TriadDrift.Flow.AbilityUnlocks", TDFlowTest::Flags)
bool FTDFlowAbilityUnlocks::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;

	// Default owners match features.md §0.
	const TMap<ETDAbilityId, ETDShipId>& Owners = UTDFlowSettings::Get()->AbilityOwners;
	TestEqual(TEXT("Gravity Well belongs to Bastion"), Owners.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	TestEqual(TEXT("Cloak belongs to Rift"), Owners.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);
	TestEqual(TEXT("EMP belongs to Lancer"), Owners.FindRef(ETDAbilityId::EMPPulse), ETDShipId::Lancer);

	TestFalse(TEXT("nothing unlocked yet"), GI->IsAbilityUnlocked(ETDAbilityId::GravityWell));
	GI->CompleteObjective(TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestTrue(TEXT("Gravity Well unlocked"), GI->IsAbilityUnlocked(ETDAbilityId::GravityWell));
	TestEqual(TEXT("for Bastion"), S.P().UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	TestEqual(TEXT("OnAbilityUnlocked fired once"), S.Listener->Unlocks.Num(), 1);

	// Completing the same objective again doesn't re-unlock or re-fire.
	GI->CompleteObjective(TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestEqual(TEXT("no duplicate unlock event"), S.Listener->Unlocks.Num(), 1);

	GI->CompleteObjective(TEXT("Obj_Relay"), ETDAbilityId::Cloak, ETDShipId::Rift);
	GI->CompleteObjective(TEXT("Obj_Outpost"), ETDAbilityId::EMPPulse, ETDShipId::Lancer);
	TestEqual(TEXT("three abilities unlocked"), S.P().UnlockedAbilities.Num(), 3);
	TestEqual(TEXT("three objectives recorded"), S.P().CompletedObjectives.Num(), 3);

	GI->UnlockAbility(ETDAbilityId::None, ETDShipId::Rift);
	TestFalse(TEXT("None is never unlocked"), GI->IsAbilityUnlocked(ETDAbilityId::None));
	TestEqual(TEXT("None adds nothing"), S.P().UnlockedAbilities.Num(), 3);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowCheckpointRestore, "TriadDrift.Flow.CheckpointRestore", TDFlowTest::Flags)
bool FTDFlowCheckpointRestore::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;

	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	TestTrue(TEXT("level entry takes a checkpoint"), GI->GetLastCheckpoint().bValid);
	TestEqual(TEXT("level entry reason"), GI->GetLastCheckpoint().Reason, FName(TEXT("LevelEntry")));

	GI->SetPhase(ETDGamePhase::Waves1to2);
	GI->ReportWaveCleared(1);
	TestEqual(TEXT("wave clear takes a checkpoint"), GI->GetLastCheckpoint().Reason, FName(TEXT("WaveCleared")));
	TestEqual(TEXT("checkpoint holds the cleared wave"), GI->GetLastCheckpoint().Progress.WavesCleared, 1);
	TestEqual(TEXT("checkpoint destination"), GI->GetLastCheckpoint().Destination, ETDDestination::Galaxy);

	// Play on past the checkpoint, then fail.
	GI->BeginScene(UTDGameInstance::WaveSceneId(2));
	GI->GetMutableProgress().WavesCleared = 7;
	GI->GetMutableProgress().bDominionRevealed = true;
	GI->AddStat(TEXT("Kills"), 5);
	GI->AdvanceRunTime(12.f);

	GI->RestartSceneNow();
	TestEqual(TEXT("progress rolled back"), S.P().WavesCleared, 1);
	TestFalse(TEXT("dominion flag rolled back"), S.P().bDominionRevealed);
	TestEqual(TEXT("phase kept from checkpoint"), S.P().Phase, ETDGamePhase::Waves1to2);
	TestEqual(TEXT("stats survive a retake"), S.P().Stats.Kills, 5);
	TestTrue(TEXT("run time survives a retake"), S.P().RunTimeSeconds >= 12.f);

	GI->CompleteObjective(TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestEqual(TEXT("objective takes a checkpoint"), GI->GetLastCheckpoint().Reason, FName(TEXT("ObjectiveComplete")));
	TestTrue(TEXT("checkpoint holds the unlock"), GI->GetLastCheckpoint().Progress.UnlockedAbilities.Contains(ETDAbilityId::GravityWell));

	GI->SaveCheckpoint();
	TestEqual(TEXT("manual checkpoint"), GI->GetLastCheckpoint().Reason, FName(TEXT("Manual")));
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowSaveSlotRoundTrip, "TriadDrift.Flow.SaveSlotRoundTrip", TDFlowTest::Flags)
bool FTDFlowSaveSlotRoundTrip::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;

	TestFalse(TEXT("no Continue on an empty slot"), GI->HasContinue());

	GI->NotifyLevelEntered(ETDDestination::Verdance);
	GI->SetPhase(ETDGamePhase::Verdance);
	GI->GetMutableProgress().WavesCleared = 2;
	GI->BeginScene(TEXT("Obj_WreckCore"));
	GI->RestartSceneNow(); // take 2
	GI->CompleteObjective(TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion);
	GI->AddStat(TEXT("Kills"), 9);
	GI->AddStat(TEXT("ShipSwitches"), 4);
	TestTrue(TEXT("write slot"), GI->WriteSaveSlot());
	TestTrue(TEXT("Continue available"), GI->HasContinue());

	UTDSaveGame* Save = nullptr;
	TestTrue(TEXT("read slot"), GI->ReadSaveSlot(Save));
	if (!Save)
	{
		return false;
	}
	TestEqual(TEXT("version field"), Save->Version, UTDSaveGame::CurrentVersion);
	TestEqual(TEXT("saved destination"), Save->Checkpoint.Destination, ETDDestination::Verdance);
	TestEqual(TEXT("saved phase"), Save->Checkpoint.Progress.Phase, ETDGamePhase::Verdance);
	TestEqual(TEXT("saved waves"), Save->Checkpoint.Progress.WavesCleared, 2);
	TestTrue(TEXT("saved objective"), Save->Checkpoint.Progress.CompletedObjectives.Contains(TEXT("Obj_WreckCore")));
	TestEqual(TEXT("saved unlock"), Save->Checkpoint.Progress.UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	TestEqual(TEXT("saved takes"), Save->Checkpoint.Progress.Takes.FindRef(TEXT("Obj_WreckCore")), 2);
	TestEqual(TEXT("saved latest kills"), Save->Checkpoint.Progress.Stats.Kills, 9);
	TestEqual(TEXT("saved scene order"), Save->SceneOrder.Num(), 1);

	// Apply into a fresh run.
	GI->StartNewRun();
	TestEqual(TEXT("fresh run"), S.P().WavesCleared, 0);
	TestTrue(TEXT("apply save"), GI->ApplySave(*Save));
	TestEqual(TEXT("restored phase"), S.P().Phase, ETDGamePhase::Verdance);
	TestEqual(TEXT("restored waves"), S.P().WavesCleared, 2);
	TestTrue(TEXT("restored ability"), GI->IsAbilityUnlocked(ETDAbilityId::GravityWell));
	TestEqual(TEXT("restored switches"), S.P().Stats.ShipSwitches, 4);
	TestEqual(TEXT("restored destination"), GI->GetCurrentDestination(), ETDDestination::Verdance);
	TestEqual(TEXT("restored scene takes"), GI->GetSceneTakes().Num(), 1);

	// A save from another version is refused, never half-loaded.
	UTDSaveGame* Old = Cast<UTDSaveGame>(UGameplayStatics::CreateSaveGameObject(UTDSaveGame::StaticClass()));
	Old->Version = UTDSaveGame::CurrentVersion + 1;
	Old->Checkpoint = Save->Checkpoint;
	TestTrue(TEXT("write incompatible save"), UGameplayStatics::SaveGameToSlot(Old, GI->GetSaveSlotName(), UTDFlowSettings::Get()->SaveUserIndex));
	TestFalse(TEXT("incompatible save hides Continue"), GI->HasContinue());
	TestFalse(TEXT("ApplySave refuses incompatible save"), GI->ApplySave(*Old));

	GI->DeleteSave();
	TestFalse(TEXT("deleted"), GI->HasContinue());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowTakes, "TriadDrift.Flow.ScenesAndTakes", TDFlowTest::Flags)
bool FTDFlowTakes::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;
	const FName Wave1 = UTDGameInstance::WaveSceneId(1);
	TestEqual(TEXT("wave scene id"), Wave1, FName(TEXT("Wave1")));

	TArray<TPair<FName, int32>> Restarts;
	GI->OnSceneRestart.AddLambda([&Restarts](FName Scene, int32 Take) { Restarts.Emplace(Scene, Take); });

	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	GI->BeginScene(Wave1);
	TestEqual(TEXT("first take is 1"), GI->GetTakeNumber(Wave1), 1);
	TestEqual(TEXT("scene number 1"), GI->GetSceneNumber(Wave1), 1);
	TestEqual(TEXT("slate fired"), S.Listener->Takes.Num(), 1);
	GI->BeginScene(Wave1);
	TestEqual(TEXT("re-opening the same take doesn't re-fire the slate"), S.Listener->Takes.Num(), 1);

	// CUT! with the configured delay: pending, nothing restored yet.
	GI->ReportPlayerDefeated();
	TestTrue(TEXT("retake pending"), GI->IsRetakePending());
	GI->ReportPlayerDefeated();
	TestEqual(TEXT("double defeat ignored while cutting"), GI->GetTakeNumber(Wave1), 1);

	GI->RestartSceneNow();
	TestFalse(TEXT("no longer pending"), GI->IsRetakePending());
	TestEqual(TEXT("take 2"), GI->GetTakeNumber(Wave1), 2);
	TestEqual(TEXT("scene still current"), GI->GetCurrentScene(), Wave1);
	TestEqual(TEXT("OnSceneRestart fired"), Restarts.Num(), 1);
	if (Restarts.Num() == 1)
	{
		TestEqual(TEXT("restart scene"), Restarts[0].Key, Wave1);
		TestEqual(TEXT("restart take"), Restarts[0].Value, 2);
	}
	TestEqual(TEXT("OnTakeStarted fired for take 2"), S.Listener->Takes.Num(), 2);
	if (S.Listener->Takes.Num() == 2)
	{
		TestEqual(TEXT("take payload scene"), S.Listener->Takes[1].Key, Wave1);
		TestEqual(TEXT("take payload number"), S.Listener->Takes[1].Value, 2);
	}

	// The wave director restarts the wave and calls BeginScene again: no duplicate slate.
	GI->BeginScene(Wave1);
	TestEqual(TEXT("no duplicate slate after retake"), S.Listener->Takes.Num(), 2);
	TestEqual(TEXT("take stays 2"), GI->GetTakeNumber(Wave1), 2);

	GI->RestartSceneNow();
	TestEqual(TEXT("take 3"), GI->GetTakeNumber(Wave1), 3);

	GI->ReportWaveCleared(1);
	TestEqual(TEXT("scene closed on clear"), GI->GetCurrentScene(), NAME_None);

	const FName Wave2 = UTDGameInstance::WaveSceneId(2);
	GI->BeginScene(Wave2);
	TestEqual(TEXT("next scene starts at take 1"), GI->GetTakeNumber(Wave2), 1);
	TestEqual(TEXT("scene number 2"), GI->GetSceneNumber(Wave2), 2);

	const TArray<FTDSceneTakes> Victory = GI->GetSceneTakes();
	TestEqual(TEXT("victory rows"), Victory.Num(), 2);
	if (Victory.Num() == 2)
	{
		TestEqual(TEXT("row 1 scene"), Victory[0].SceneId, Wave1);
		TestEqual(TEXT("row 1 takes"), Victory[0].Takes, 3);
		TestEqual(TEXT("row 2 number"), Victory[1].SceneNumber, 2);
		TestEqual(TEXT("row 2 takes"), Victory[1].Takes, 1);
	}

	// Retake restores the wave-1-clear checkpoint (wave 2 progress is discarded) but keeps takes.
	GI->GetMutableProgress().WavesCleared = 4;
	GI->RestartSceneNow();
	TestEqual(TEXT("retake restores checkpoint"), S.P().WavesCleared, 1);
	TestEqual(TEXT("wave 1 takes kept"), GI->GetTakeNumber(Wave1), 3);
	TestEqual(TEXT("wave 2 take 2"), GI->GetTakeNumber(Wave2), 2);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowStats, "TriadDrift.Flow.StatsAndTimer", TDFlowTest::Flags)
bool FTDFlowStats::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;

	GI->AddStat(TEXT("Kills"), 3);
	GI->AddStat(TEXT("Kills"), 2);
	GI->AddStat(TEXT("ShipSwitches"), 1);
	GI->AddStat(TEXT("PortalsUsed"), 2);
	GI->AddStat(TEXT("ShotsDeflected"), 6);
	GI->AddStat(TEXT("Deaths"), 1);
	GI->AddStat(TEXT("MissilesReturned"), 1);
	GI->AddStat(TEXT("HighlightMoments"), 4);
	AddExpectedMessage(TEXT("FTDRunStats has no field"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	GI->AddStat(TEXT("NotAStat"), 99);
	GI->AddStat(NAME_None, 99);

	const FTDRunStats& St = S.P().Stats;
	TestEqual(TEXT("kills accumulate"), St.Kills, 5);
	TestEqual(TEXT("switches"), St.ShipSwitches, 1);
	TestEqual(TEXT("portals"), St.PortalsUsed, 2);
	TestEqual(TEXT("deflects"), St.ShotsDeflected, 6);
	TestEqual(TEXT("deaths"), St.Deaths, 1);
	TestEqual(TEXT("missiles returned"), St.MissilesReturned, 1);
	TestEqual(TEXT("highlights"), St.HighlightMoments, 4);

	const float Start = S.P().RunTimeSeconds;
	GI->AdvanceRunTime(1.5f);
	TestEqual(TEXT("run timer advances"), S.P().RunTimeSeconds, Start + 1.5f, 1e-4f);
	GI->SetRunTimerPaused(true);
	GI->AdvanceRunTime(10.f);
	TestEqual(TEXT("paused timer holds"), S.P().RunTimeSeconds, Start + 1.5f, 1e-4f);
	GI->SetRunTimerPaused(false);
	GI->SetPhase(ETDGamePhase::Victory);
	GI->AdvanceRunTime(10.f);
	TestEqual(TEXT("victory stops the clock"), S.P().RunTimeSeconds, Start + 1.5f, 1e-4f);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowEvents, "TriadDrift.Flow.Events", TDFlowTest::Flags)
bool FTDFlowEvents::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;
	UTDEventBus* Bus = S.Bus();
	TestNotNull(TEXT("event bus exists"), Bus);
	if (!Bus)
	{
		return false;
	}

	GI->SetPhase(ETDGamePhase::Waves1to2);
	GI->SetPhase(ETDGamePhase::Waves1to2);
	TestEqual(TEXT("OnPhaseChanged fires once per change"), S.Listener->Phases.Num(), 1);
	if (S.Listener->Phases.Num() == 1)
	{
		TestEqual(TEXT("phase payload"), S.Listener->Phases[0], ETDGamePhase::Waves1to2);
	}

	GI->UnlockAbility(ETDAbilityId::EMPPulse, ETDShipId::Lancer);
	TestEqual(TEXT("unlock event"), S.Listener->Unlocks.Num(), 1);
	if (S.Listener->Unlocks.Num() == 1)
	{
		TestEqual(TEXT("unlock ability"), S.Listener->Unlocks[0].Key, ETDAbilityId::EMPPulse);
		TestEqual(TEXT("unlock ship"), S.Listener->Unlocks[0].Value, ETDShipId::Lancer);
	}

	GI->ShowTip(TEXT("Tip_Switch"));
	TestEqual(TEXT("tip forwarded"), S.Listener->Tips.Num(), 1);
	if (S.Listener->Tips.Num() == 1)
	{
		TestEqual(TEXT("tip payload"), S.Listener->Tips[0], FName(TEXT("Tip_Switch")));
	}

	GI->RequestBark(TEXT("Bark_WaveCleared"), TEXT("Kato"));
	TestEqual(TEXT("bark forwarded"), S.Listener->Barks.Num(), 1);
	if (S.Listener->Barks.Num() == 1)
	{
		TestEqual(TEXT("bark id"), S.Listener->Barks[0].Key, FName(TEXT("Bark_WaveCleared")));
		TestEqual(TEXT("bark speaker"), S.Listener->Barks[0].Value, FName(TEXT("Kato")));
	}

	// The wave director's bus events drive scenes: OnWaveStarted opens "Wave1" take 1.
	Bus->BroadcastWaveStarted(1, 0.2f);
	TestEqual(TEXT("wave start opens the scene"), GI->GetCurrentScene(), FName(TEXT("Wave1")));
	TestEqual(TEXT("take started"), S.Listener->Takes.Num(), 1);
	if (S.Listener->Takes.Num() == 1)
	{
		TestEqual(TEXT("take scene"), S.Listener->Takes[0].Key, FName(TEXT("Wave1")));
		TestEqual(TEXT("take number"), S.Listener->Takes[0].Value, 1);
	}
	Bus->BroadcastWaveCleared(1);
	TestEqual(TEXT("wave clear via bus"), S.P().WavesCleared, 1);
	TestEqual(TEXT("scene closed via bus"), GI->GetCurrentScene(), NAME_None);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowTravelMissingMap, "TriadDrift.Flow.TravelMissingMapFailsSafely", TDFlowTest::Flags)
bool FTDFlowTravelMissingMap::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;

	UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
	const TMap<ETDDestination, FSoftObjectPath> SavedMaps = Settings->DestinationMaps;
	Settings->DestinationMaps.Add(ETDDestination::Cinder, FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_DoesNotExist.L_DoesNotExist")));
	Settings->DestinationMaps.Remove(ETDDestination::Victory);

	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	GI->GetMutableProgress().ReturnPoint = TEXT("Before");

	AddExpectedMessage(TEXT("staying put"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2, false);
	TestFalse(TEXT("missing map fails"), GI->TravelTo(ETDDestination::Cinder, TEXT("CinderPad")));
	TestFalse(TEXT("unconfigured destination fails"), GI->TravelTo(ETDDestination::Victory, NAME_None));
	TestEqual(TEXT("return point untouched on failure"), S.P().ReturnPoint, FName(TEXT("Before")));
	TestEqual(TEXT("destination unchanged"), GI->GetCurrentDestination(), ETDDestination::Galaxy);

	FString Package;
	TestFalse(TEXT("resolve rejects missing package"), Settings->ResolveDestinationMap(ETDDestination::Cinder, Package));
	TestTrue(TEXT("no package name returned"), Package.IsEmpty());

	Settings->DestinationMaps = SavedMaps;
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
