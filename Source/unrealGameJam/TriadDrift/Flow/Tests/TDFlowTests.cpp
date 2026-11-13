// Triad Drift - Flow automation tests (ENG-19, C-03 #3). Owner: Teammate 5.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flow

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDSkySphere.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDSaveGame.h"
#include "TriadDrift/Flow/Tests/TDFlowTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "Misc/ScopeExit.h"
#include "HAL/IConsoleManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SaveGame.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "AudioDevice.h"
#include "Sound/SoundWave.h"
#include "Components/AudioComponent.h"
#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/UI/TDPauseMenuWidget.h"
#include "Widgets/Input/SButton.h"
#include "Blueprint/UserWidget.h"

namespace TDFlowTest
{
	constexpr EAutomationTestFlags TestFlags = TD_TEST_FLAGS;
	static const TCHAR* TestSlot = TEXT("TriadDrift_AutomationTest");

	/** Swaps flow settings for the duration of a test and restores them after. */
	struct FScopedSettings
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		TMap<ETDDestination, FSoftObjectPath> Maps = S->DestinationMaps;
		FSoftObjectPath Transition = S->TransitionMap;
		float Fade = S->TravelFadeSeconds;
		float MinTransition = S->MinTransitionSeconds;
		float Timeout = S->TravelTimeoutSeconds;

		/**
		 * Every test starts with NO destination map and no transition map resolvable, whether or not
		 * the real maps are on disk; tests that need a successful travel point a destination at an
		 * engine map and fake OpenLevel (UTDGameInstance::OpenLevelOverride).
		 */
		FScopedSettings()
		{
			if (const UEnum* E = StaticEnum<ETDDestination>())
			{
				for (int32 i = 0; i < E->NumEnums() - 1; ++i)
				{
					const FString Bogus = FString::Printf(TEXT("/Game/TriadDrift/Tests/NoSuchMap_%s.NoSuchMap_%s"), *E->GetNameStringByIndex(i), *E->GetNameStringByIndex(i));
					S->DestinationMaps.Add(static_cast<ETDDestination>(E->GetValueByIndex(i)), FSoftObjectPath(Bogus));
				}
			}
			S->TransitionMap = FSoftObjectPath(TEXT("/Game/TriadDrift/Tests/NoSuchTransition.NoSuchTransition"));
		}

		~FScopedSettings()
		{
			S->DestinationMaps = Maps;
			S->TransitionMap = Transition;
			S->TravelFadeSeconds = Fade;
			S->MinTransitionSeconds = MinTransition;
			S->TravelTimeoutSeconds = Timeout;
		}
	};

	/** A real, initialized UTDGameInstance (with its event bus) on a throwaway world and test-only save slot. */
	struct FScopedGI
	{
		FScopedSettings NoRealMaps; // first member: built before, restored after, the GI
		UTDGameInstance* GI = nullptr;
		UTDFlowTestListener* Listener = nullptr;

		explicit FScopedGI(bool bFreshSlot = true)
		{
			GI = NewObject<UTDGameInstance>(GEngine);
			GI->AddToRoot();
			GI->SetSaveSlotOverride(TestSlot);
			static int32 WorldCounter = 0;
			GI->InitializeStandalone(FName(*FString::Printf(TEXT("TDFlowTestWorld_%d"), ++WorldCounter)));
			GI->SetSaveSlotOverride(TestSlot);
			if (bFreshSlot)
			{
				GI->DeleteSave();
				GI->StartNewRun(); // "New Game": this run may write the (test) slot
			}
			Listener = NewObject<UTDFlowTestListener>();
			Listener->AddToRoot();
			Listener->Bind(Bus());
		}

		bool bKeepSlot = false;

		~FScopedGI()
		{
			if (!bKeepSlot)
			{
				GI->DeleteSave();
			}
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

using TDFlowTest::FScopedGI;

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowProgressMutations, "TriadDrift.Flow.ProgressMutations", TDFlowTest::TestFlags)
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowAbilityUnlocks, "TriadDrift.Flow.AbilityUnlocks", TDFlowTest::TestFlags)
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowCheckpointRestore, "TriadDrift.Flow.CheckpointRestore", TDFlowTest::TestFlags)
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowSaveSlotRoundTrip, "TriadDrift.Flow.SaveSlotRoundTrip", TDFlowTest::TestFlags)
bool FTDFlowSaveSlotRoundTrip::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;

	TestFalse(TEXT("no Continue on an empty slot"), GI->HasContinue());

	GI->NotifyLevelEntered(ETDDestination::Verdance);
	GI->BeginScene(TEXT("Obj_WreckCore"));
	GI->RestartSceneNow(); // take 2 (restores the level-entry checkpoint, so progress is set after it)
	GI->SetPhase(ETDGamePhase::Verdance);
	GI->GetMutableProgress().WavesCleared = 2;
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
	{
		// Difficulty lives in the slot and comes back with Continue (ApplySave).
		const ETDDifficulty Was = UTDDifficultySettings::GetCurrent();
		GI->SetDifficulty(ETDDifficulty::Ace);
		UTDSaveGame* WithAce = nullptr;
		TestTrue(TEXT("re-read slot"), GI->ReadSaveSlot(WithAce));
		TestEqual(TEXT("difficulty saved"), WithAce ? WithAce->Difficulty : ETDDifficulty::Standard, ETDDifficulty::Ace);
		UTDDifficultySettings::SetCurrent(ETDDifficulty::Story);
		TestTrue(TEXT("apply"), WithAce && GI->ApplySave(*WithAce));
		TestEqual(TEXT("difficulty restored by the save"), GI->GetDifficulty(), ETDDifficulty::Ace);
		TestTrue(TEXT("cheat refuses nonsense"), !UTDCheatManager::DoDifficulty(GI->GetWorld(), TEXT("Nightmare")));
		UTDDifficultySettings::SetCurrent(Was);
	}
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
	GI->SetSaveSlotOverride(GI->GetSaveSlotName()); // written behind the cache's back: refresh it
	TestFalse(TEXT("incompatible save hides Continue"), GI->HasContinue());
	TestFalse(TEXT("ApplySave refuses incompatible save"), GI->ApplySave(*Old));

	GI->DeleteSave();
	TestFalse(TEXT("deleted"), GI->HasContinue());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowTakes, "TriadDrift.Flow.ScenesAndTakes", TDFlowTest::TestFlags)
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

	// CUT! with the configured delay (real time, via the flow clock): pending, nothing restored yet.
	const float Delay = UTDFlowSettings::Get()->RetakeDelaySeconds;
	TestTrue(TEXT("retake delay configured"), Delay > 0.5f);
	GI->ReportPlayerDefeated();
	TestTrue(TEXT("retake pending"), GI->IsRetakePending());
	TestEqual(TEXT("CUT! fired once"), S.Listener->Cuts.Num(), 1);
	if (S.Listener->Cuts.Num() == 1)
	{
		TestEqual(TEXT("cut scene"), S.Listener->Cuts[0].SceneId, Wave1);
		TestEqual(TEXT("cut next take"), S.Listener->Cuts[0].NextTake, 2);
		TestEqual(TEXT("cut delay"), S.Listener->Cuts[0].Delay, Delay, 1e-4f);
	}
	GI->ReportPlayerDefeated();
	TestEqual(TEXT("double defeat while cutting fires no second CUT"), S.Listener->Cuts.Num(), 1);
	GI->TickFlow(Delay * 0.5f);
	TestTrue(TEXT("still cutting halfway through the delay"), GI->IsRetakePending());
	TestEqual(TEXT("no restart yet"), Restarts.Num(), 0);
	TestEqual(TEXT("take unchanged during the delay"), GI->GetTakeNumber(Wave1), 1);
	GI->ReportWaveCleared(1);
	TestEqual(TEXT("a wave clear during the CUT delay is ignored"), S.P().WavesCleared, 0);
	GI->TickFlow(Delay * 0.6f);
	TestFalse(TEXT("no longer pending after the delay"), GI->IsRetakePending());
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowStats, "TriadDrift.Flow.StatsAndTimer", TDFlowTest::TestFlags)
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowEvents, "TriadDrift.Flow.Events", TDFlowTest::TestFlags)
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowTravelMissingMap, "TriadDrift.Flow.TravelMissingMapFailsSafely", TDFlowTest::TestFlags)
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

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowSandboxBoots, "TriadDrift.Flow.SandboxBootsOnEmptyWorld", TDFlowTest::TestFlags)
bool FTDFlowSandboxBoots::RunTest(const FString& Parameters)
{
	// Never write the player's real save from a test.
	UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
	const bool bSavedWrite = Settings->bWriteSlotOnCheckpoint;
	Settings->bWriteSlotOnCheckpoint = false;
	{
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		TestTrue(TEXT("world created"), W.IsValid());
		if (W.IsValid())
		{
			W.Advance(0.5f);
			UWorld* World = W.GetWorld();
			auto Count = [World](UClass* Class)
			{
				int32 N = 0;
				for (TActorIterator<AActor> It(World, Class); It; ++It) { ++N; }
				return N;
			};
			const ATDSpaceGameMode* GM = World->GetAuthGameMode<ATDSpaceGameMode>();
			TestNotNull(TEXT("space game mode active"), GM);
			TestEqual(TEXT("one placeholder sky (empty map)"), Count(ATDPlaceholderSky::StaticClass()), 1);
			for (TActorIterator<ATDPlaceholderSky> It(World); It; ++It)
			{
				TestTrue(TEXT("placeholder lights on an empty map"), It->HasLights());
				TestTrue(TEXT("placeholder exposure on an empty map"), It->HasExposure());
			}
			if (GM)
			{
				// T4's galaxy layout replaces the placeholder asteroids (and spawns the Dominion).
				TestEqual(TEXT("one galaxy layout"), Count(ATDGalaxyLayout::StaticClass()), 1);
				TestEqual(TEXT("no placeholder asteroids with the layout"), Count(AStaticMeshActor::StaticClass()), 0);
			}

			UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
			TestNotNull(TEXT("TD game instance"), GI);
			if (GI)
			{
				TestEqual(TEXT("entered the galaxy"), GI->GetCurrentDestination(), ETDDestination::Galaxy);
				TestTrue(TEXT("level-entry checkpoint"), GI->GetLastCheckpoint().bValid);
				TestEqual(TEXT("intro advances to waves 1-2"), GI->GetProgressRef().Phase, ETDGamePhase::Waves1to2);

				// A forced retake with no squad/director merged must not crash.
				GI->BeginScene(UTDGameInstance::WaveSceneId(1));
				GI->RestartSceneNow();
				W.Advance(0.1f);
				TestEqual(TEXT("retake in the sandbox"), GI->GetTakeNumber(UTDGameInstance::WaveSceneId(1)), 2);
			}
		}
	}
	Settings->bWriteSlotOnCheckpoint = bSavedWrite;
	return true;
}

// ================================================================================================
// Review round 1: save safety, Continue, travel, kills/deaths, story phases, patching, cheats
// ================================================================================================

namespace TDFlowTest
{
	static const TCHAR* ExistingMapA = TEXT("/Engine/Maps/Entry.Entry");
	static const TCHAR* ExistingMapB = TEXT("/Engine/Maps/Templates/Template_Default.Template_Default");

	static AActor* SpawnWithHealth(FTDTestWorld& W, UClass* Class, ETDTeam Team)
	{
		AActor* A = W.Spawn<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, Class);
		if (A)
		{
			UTDHealthComponent* H = NewObject<UTDHealthComponent>(A, TEXT("Health"));
			H->DefaultTeam = Team;
			H->MaxShield = 0.f;
			H->RegisterComponent();
		}
		return A;
	}

	static UTDSaveGame* ReadSlot(UTDGameInstance* GI)
	{
		UTDSaveGame* Save = nullptr;
		GI->ReadSaveSlot(Save);
		return Save;
	}
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowSaveSafety, "TriadDrift.Flow.SaveSafety", TDFlowTest::TestFlags)
bool FTDFlowSaveSafety::RunTest(const FString& Parameters)
{
	// A committed run with real progress in the galaxy.
	{
		FScopedGI A;
		A.bKeepSlot = true;
		UTDGameInstance* GI = A.GI;

		GI->NotifyLevelEntered(ETDDestination::MainMenu);
		TestFalse(TEXT("main menu never checkpoints"), GI->GetLastCheckpoint().bValid);
		TestFalse(TEXT("main menu never writes"), GI->HasContinue());

		GI->NotifyLevelEntered(ETDDestination::Galaxy);
		TestTrue(TEXT("galaxy entry checkpoints"), GI->GetLastCheckpoint().bValid);
		TestFalse(TEXT("a pristine run is never written"), GI->HasContinue());

		GI->SetPhase(ETDGamePhase::Waves1to2);
		GI->BeginScene(UTDGameInstance::WaveSceneId(1));
		GI->ReportWaveCleared(1);
		GI->BeginScene(UTDGameInstance::WaveSceneId(2));
		GI->ReportWaveCleared(2);
		TestTrue(TEXT("real progress is written"), GI->HasContinue());

		// Quitting to the menu must not repoint Continue at the menu.
		GI->NotifyLevelEntered(ETDDestination::MainMenu);
		const UTDSaveGame* Save = TDFlowTest::ReadSlot(GI);
		TestTrue(TEXT("still a save after returning to the menu"), Save != nullptr);
		if (Save)
		{
			TestEqual(TEXT("save still points at the galaxy"), Save->Checkpoint.Destination, ETDDestination::Galaxy);
			TestEqual(TEXT("save keeps 2 waves"), Save->Checkpoint.Progress.WavesCleared, 2);
		}
		TestTrue(TEXT("run clock stopped on the menu"), [&]() { const float T = GI->GetProgressRef().RunTimeSeconds; GI->AdvanceRunTime(5.f); return GI->GetProgressRef().RunTimeSeconds == T; }());
	}

	// A fresh boot straight into gameplay (no New Game, no Continue) must not wipe that save.
	{
		FScopedGI B(false);
		UTDGameInstance* GI = B.GI;
		TestFalse(TEXT("fresh boot is uncommitted"), GI->IsRunCommitted());
		TestTrue(TEXT("Continue is offered"), GI->HasContinue());
		GI->NotifyLevelEntered(ETDDestination::Galaxy);
		GI->SetPhase(ETDGamePhase::Waves1to2);
		GI->SetDominionState(true, false);
		const UTDSaveGame* Save = TDFlowTest::ReadSlot(GI);
		TestTrue(TEXT("save survives a fresh boot into gameplay"), Save && Save->Checkpoint.Progress.WavesCleared == 2);

		// Continue: progress comes back; with no map for the galaxy, travel fails and ContinueRun says so.
		TDFlowTest::FScopedSettings Scoped;
		Scoped.S->DestinationMaps.Remove(ETDDestination::Galaxy);
		AddExpectedMessage(TEXT("staying put"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
		AddExpectedMessage(TEXT("travel to the saved destination failed"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
		TestFalse(TEXT("ContinueRun false when travel fails"), GI->ContinueRun());
		TestEqual(TEXT("progress restored anyway"), GI->GetProgressRef().WavesCleared, 2);
		TestEqual(TEXT("phase restored"), GI->GetProgressRef().Phase, ETDGamePhase::Verdance);

		// With a real map it travels.
		Scoped.S->DestinationMaps.Add(ETDDestination::Galaxy, FSoftObjectPath(TDFlowTest::ExistingMapA));
		Scoped.S->TransitionMap.Reset();
		FString Opened;
		GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };
		TestTrue(TEXT("ContinueRun travels"), GI->ContinueRun());
		TestTrue(TEXT("travel pending"), GI->IsTravelPending());
		GI->TickFlow(0.1f);
		TestEqual(TEXT("opens the saved destination"), Opened, FString(TEXT("/Engine/Maps/Entry")));
		GI->OpenLevelOverride = nullptr;
	}

	// Foreign and corrupt saves are ignored.
	{
		FScopedGI C;
		UTDGameInstance* GI = C.GI;
		USaveGame* Foreign = UGameplayStatics::CreateSaveGameObject(USaveGame::StaticClass());
		UGameplayStatics::SaveGameToSlot(Foreign, GI->GetSaveSlotName(), UTDFlowSettings::Get()->SaveUserIndex);
		GI->SetSaveSlotOverride(GI->GetSaveSlotName());
		AddExpectedMessage(TEXT("corrupt or not a Triad Drift save"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0, false);
		TestFalse(TEXT("foreign save ignored"), GI->HasContinue());

		const FString Path = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (GI->GetSaveSlotName() + TEXT(".sav"));
		FFileHelper::SaveStringToFile(TEXT("this is not a save game"), *Path);
		GI->SetSaveSlotOverride(GI->GetSaveSlotName());
		TestFalse(TEXT("corrupt save ignored"), GI->HasContinue());
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowTravel, "TriadDrift.Flow.TravelSucceedsAndRecovers", TDFlowTest::TestFlags)
bool FTDFlowTravel::RunTest(const FString& Parameters)
{
	// T1, ENG-38: this test covers the fade + L_Transition route, which planet trips take only with td.Descent.Legacy 1
	// (TriadDrift.Travel.Descent covers the white-cover route).
	IConsoleVariable* Legacy = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Descent.Legacy"));
	if (Legacy) { Legacy->Set(1, ECVF_SetByCode); }
	ON_SCOPE_EXIT { if (Legacy) { Legacy->Set(0, ECVF_SetByCode); } };
	FScopedGI S;
	UTDGameInstance* GI = S.GI;
	TDFlowTest::FScopedSettings Scoped;
	Scoped.S->DestinationMaps.Add(ETDDestination::Galaxy, FSoftObjectPath(TDFlowTest::ExistingMapA));
	Scoped.S->DestinationMaps.Add(ETDDestination::Verdance, FSoftObjectPath(TDFlowTest::ExistingMapA));
	Scoped.S->TransitionMap = FSoftObjectPath(TDFlowTest::ExistingMapB);
	Scoped.S->MinTransitionSeconds = 1.f;
	Scoped.S->TravelTimeoutSeconds = 10.f;

	TArray<FString> Opened;
	GI->OpenLevelOverride = [&Opened](const FString& P) { Opened.Add(P); };

	GI->NotifyLevelEntered(ETDDestination::Verdance);
	GI->SetPhase(ETDGamePhase::Verdance);
	TestTrue(TEXT("travel starts"), GI->TravelTo(ETDDestination::Galaxy, TEXT("Verdance_Orbit")));
	TestEqual(TEXT("return point recorded"), S.P().ReturnPoint, FName(TEXT("Verdance_Orbit")));
	TestFalse(TEXT("second travel refused while pending"), GI->TravelTo(ETDDestination::Verdance, NAME_None));

	const float T0 = S.P().RunTimeSeconds;
	GI->AdvanceRunTime(3.f);
	TestEqual(TEXT("run clock paused during travel"), S.P().RunTimeSeconds, T0, 1e-4f);

	GI->TickFlow(0.05f);
	TestEqual(TEXT("loading screen opened first"), Opened.Num(), 1);
	if (Opened.Num() == 1)
	{
		TestEqual(TEXT("transition map"), Opened[0], FString(TEXT("/Engine/Maps/Templates/Template_Default")));
	}
	GI->NotifyMapLoaded(TEXT("/Engine/Maps/Templates/Template_Default"));
	GI->TickFlow(0.5f);
	TestEqual(TEXT("holds on the loading screen"), Opened.Num(), 1);
	GI->TickFlow(0.6f);
	TestEqual(TEXT("then opens the target"), Opened.Num(), 2);
	GI->NotifyMapLoaded(TEXT("/Engine/Maps/Entry"));
	TestFalse(TEXT("arrived"), GI->IsTravelPending());
	TestEqual(TEXT("destination updated"), GI->GetCurrentDestination(), ETDDestination::Galaxy);
	GI->AdvanceRunTime(1.f);
	TestEqual(TEXT("run clock resumes on arrival"), S.P().RunTimeSeconds, T0 + 1.f, 1e-4f);
	GI->NotifyMapLoaded(TEXT("/Engine/Maps/Entry"));
	GI->TickFlow(5.f);
	TestEqual(TEXT("a later map load never re-opens anything"), Opened.Num(), 2);

	// Superseded by another level change: travel is dropped, not stuck.
	AddExpectedMessage(TEXT("superseded"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	TestTrue(TEXT("travel again"), GI->TravelTo(ETDDestination::Verdance, NAME_None));
	GI->NotifyMapLoaded(TEXT("/Game/Somewhere/Else"));
	TestFalse(TEXT("superseded travel cleared"), GI->IsTravelPending());

	// A load that never arrives: the watchdog gives control back.
	AddExpectedMessage(TEXT("timed out"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	TestTrue(TEXT("travel once more"), GI->TravelTo(ETDDestination::Verdance, NAME_None));
	GI->TickFlow(0.05f);
	for (int32 i = 0; i < 12; ++i)
	{
		GI->TickFlow(1.f);
	}
	TestFalse(TEXT("watchdog cleared the stuck travel"), GI->IsTravelPending());
	GI->OpenLevelOverride = nullptr;
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowKillsDeaths, "TriadDrift.Flow.KillsAndDeathsFromBus", TDFlowTest::TestFlags)
bool FTDFlowKillsDeaths::RunTest(const FString& Parameters)
{
	FScopedGI S;
	FTDTestWorld W;
	AActor* Enemy = TDFlowTest::SpawnWithHealth(W, AActor::StaticClass(), ETDTeam::Enemy);
	APawn* PlayerPawn = Cast<APawn>(TDFlowTest::SpawnWithHealth(W, ADefaultPawn::StaticClass(), ETDTeam::Player));
	APawn* Wingman = Cast<APawn>(TDFlowTest::SpawnWithHealth(W, ADefaultPawn::StaticClass(), ETDTeam::Player));
	W.SpawnPlayerController(PlayerPawn);
	TestTrue(TEXT("player pawn is player-controlled"), PlayerPawn && PlayerPawn->IsPlayerControlled());

	S.GI->HandleActorKilled(Enemy, PlayerPawn, ETDDamageKind::Energy);
	TestEqual(TEXT("player kills an enemy: +1 kill"), S.P().Stats.Kills, 1);
	S.GI->HandleActorKilled(Enemy, nullptr, ETDDamageKind::Environmental);
	S.GI->HandleActorKilled(Enemy, Enemy, ETDDamageKind::Explosive);
	TestEqual(TEXT("environment / enemy kills don't count"), S.P().Stats.Kills, 1);
	S.GI->HandleActorKilled(Wingman, Enemy, ETDDamageKind::Energy);
	TestEqual(TEXT("wingman downed is not a death"), S.P().Stats.Deaths, 0);
	S.GI->HandleActorKilled(PlayerPawn, Enemy, ETDDamageKind::Energy);
	TestEqual(TEXT("piloted pawn killed: +1 death"), S.P().Stats.Deaths, 1);
	S.GI->HandleActorKilled(nullptr, PlayerPawn, ETDDamageKind::Energy);
	TestEqual(TEXT("null victim ignored"), S.P().Stats.Kills, 1);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowStoryPhases, "TriadDrift.Flow.StoryPhaseTransitions", TDFlowTest::TestFlags)
bool FTDFlowStoryPhases::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;
	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	GI->SetPhase(ETDGamePhase::Waves1to2);
	GI->ReportWaveCleared(1);
	TestEqual(TEXT("wave 1: still waves 1-2"), S.P().Phase, ETDGamePhase::Waves1to2);
	GI->ReportWaveCleared(2);
	TestEqual(TEXT("wave 2 -> Verdance"), S.P().Phase, ETDGamePhase::Verdance);

	// Verdance: both objectives, then back to the galaxy.
	GI->NotifyLevelEntered(ETDDestination::Verdance);
	TestEqual(TEXT("landing keeps the phase"), S.P().Phase, ETDGamePhase::Verdance);
	GI->CompleteObjective(TEXT("Obj_WreckCore"), ETDAbilityId::GravityWell, ETDShipId::Bastion);
	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	TestEqual(TEXT("one Verdance objective isn't enough"), S.P().Phase, ETDGamePhase::Verdance);
	GI->CompleteObjective(TEXT("Obj_Relay"), ETDAbilityId::Cloak, ETDShipId::Rift);
	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	TestEqual(TEXT("Verdance done -> waves 3-4"), S.P().Phase, ETDGamePhase::Waves3to4);
	TestEqual(TEXT("level-entry checkpoint holds the new phase"), GI->GetLastCheckpoint().Progress.Phase, ETDGamePhase::Waves3to4);

	GI->ReportWaveCleared(3);
	TestEqual(TEXT("wave 3: still waves 3-4"), S.P().Phase, ETDGamePhase::Waves3to4);
	GI->ReportWaveCleared(4);
	TestEqual(TEXT("wave 4 -> Cinder"), S.P().Phase, ETDGamePhase::Cinder);

	GI->NotifyLevelEntered(ETDDestination::Cinder);
	GI->CompleteObjective(TEXT("Obj_Outpost"), ETDAbilityId::EMPPulse, ETDShipId::Lancer);
	GI->CompleteObjective(TEXT("Obj_Jammer"), ETDAbilityId::None, ETDShipId::Rift);
	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	TestEqual(TEXT("Cinder done -> wave 5"), S.P().Phase, ETDGamePhase::Wave5);

	GI->ReportWaveCleared(5);
	TestEqual(TEXT("wave 5 -> Dominion assault"), S.P().Phase, ETDGamePhase::DominionAssault);
	TestTrue(TEXT("phase changes were broadcast"), S.Listener->Phases.Contains(ETDGamePhase::Wave5));
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowStoryStateSurvivesRetake, "TriadDrift.Flow.StoryStateSurvivesRetakeAndSave", TDFlowTest::TestFlags)
bool FTDFlowStoryStateSurvivesRetake::RunTest(const FString& Parameters)
{
	FScopedGI S;
	UTDGameInstance* GI = S.GI;
	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	GI->SetPhase(ETDGamePhase::Wave5);
	GI->BeginScene(UTDGameInstance::WaveSceneId(5));
	GI->ReportWaveCleared(5);                // checkpoint (revealed = false) ...
	GI->SetDominionState(true, false);       // ... then the director reveals the Dominion
	TestTrue(TEXT("checkpoint patched with the reveal"), GI->GetLastCheckpoint().Progress.bDominionRevealed);
	const UTDSaveGame* Save = TDFlowTest::ReadSlot(GI);
	TestTrue(TEXT("slot patched with the reveal"), Save && Save->Checkpoint.Progress.bDominionRevealed);

	GI->UnlockAbility(ETDAbilityId::EMPPulse, ETDShipId::Lancer);
	TestTrue(TEXT("checkpoint patched with the unlock"), GI->GetLastCheckpoint().Progress.UnlockedAbilities.Contains(ETDAbilityId::EMPPulse));

	GI->BeginScene(TEXT("Dominion_Generators"));
	GI->ReportPlayerDefeated();
	GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
	TestTrue(TEXT("reveal survives a retake"), S.P().bDominionRevealed);
	TestTrue(TEXT("unlock survives a retake"), GI->IsAbilityUnlocked(ETDAbilityId::EMPPulse));
	TestEqual(TEXT("phase survives a retake"), S.P().Phase, ETDGamePhase::DominionAssault);
	Save = TDFlowTest::ReadSlot(GI);
	TestTrue(TEXT("retake writes the takes to the slot"), Save && Save->Checkpoint.Progress.Takes.FindRef(TEXT("Dominion_Generators")) == 2);

	// A phase changed without going through SetPhase is rolled back, and the restore re-broadcasts it.
	const int32 PhaseEvents = S.Listener->Phases.Num();
	GI->GetMutableProgress().Phase = ETDGamePhase::Escape;
	GI->RestartSceneNow();
	TestEqual(TEXT("phase restored"), S.P().Phase, ETDGamePhase::DominionAssault);
	TestEqual(TEXT("restored phase re-broadcast"), S.Listener->Phases.Num(), PhaseEvents + 1);

	// Death outside any scene: CUT and a restart event still fire (scene None, take 0).
	GI->EndScene();
	const int32 Cuts = S.Listener->Cuts.Num();
	const int32 Takes = S.Listener->Takes.Num();
	GI->ReportPlayerDefeated();
	GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
	TestEqual(TEXT("CUT without a scene"), S.Listener->Cuts.Num(), Cuts + 1);
	TestEqual(TEXT("restart event without a scene"), S.Listener->Takes.Num(), Takes + 1);
	if (S.Listener->Takes.Num() == Takes + 1)
	{
		TestEqual(TEXT("no-scene restart id"), S.Listener->Takes.Last().Key, FName(NAME_None));
		TestEqual(TEXT("no-scene restart take"), S.Listener->Takes.Last().Value, 0);
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowCheats, "TriadDrift.Flow.Cheats", TDFlowTest::TestFlags)
bool FTDFlowCheats::RunTest(const FString& Parameters)
{
	TDFlowTest::FScopedSettings NoRealMaps;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	TestNotNull(TEXT("TD game instance"), GI);
	GI->OpenLevelOverride = [](const FString&) {}; // belt and braces: never a real OpenLevel in a test world
	if (!GI)
	{
		return false;
	}
	GI->SetSaveSlotOverride(TDFlowTest::TestSlot);
	UWorld* World = W.GetWorld();

	TestTrue(TEXT("TD.Phase Cinder"), UTDCheatManager::DoPhase(World, TEXT("Cinder")));
	TestEqual(TEXT("phase set"), GI->GetProgressRef().Phase, ETDGamePhase::Cinder);
	TestFalse(TEXT("TD.Phase bogus"), UTDCheatManager::DoPhase(World, TEXT("NotAPhase")));

	TestTrue(TEXT("TD.Unlock Cloak"), UTDCheatManager::DoUnlock(World, TEXT("Cloak"), FString()));
	TestEqual(TEXT("Cloak defaults to Rift"), GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);
	TestTrue(TEXT("TD.Unlock EMPPulse Bastion"), UTDCheatManager::DoUnlock(World, TEXT("EMPPulse"), TEXT("Bastion")));
	TestEqual(TEXT("explicit ship"), GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::EMPPulse), ETDShipId::Bastion);
	TestFalse(TEXT("TD.Unlock None"), UTDCheatManager::DoUnlock(World, TEXT("None"), FString()));

	APawn* Player = Cast<APawn>(TDFlowTest::SpawnWithHealth(W, ADefaultPawn::StaticClass(), ETDTeam::Player));
	APlayerController* PC = W.SpawnPlayerController(Player);
	UTDHealthComponent* PlayerHealth = UTDStatics::GetHealth(Player);
	if (UTDCheatManager::IsGodModeOn())
	{
		UTDCheatManager::DoGod(World); // start from off
	}
	TestTrue(TEXT("TD.God on"), UTDCheatManager::DoGod(World));
	TestTrue(TEXT("player invulnerable"), PlayerHealth && PlayerHealth->IsInvulnerable());
	TestFalse(TEXT("TD.God off"), UTDCheatManager::DoGod(World));
	TestTrue(TEXT("player vulnerable again"), PlayerHealth && !PlayerHealth->IsInvulnerable());

	AActor* E1 = TDFlowTest::SpawnWithHealth(W, AActor::StaticClass(), ETDTeam::Enemy);
	AActor* E2 = TDFlowTest::SpawnWithHealth(W, AActor::StaticClass(), ETDTeam::Enemy);
	W.Tick();
	TestEqual(TEXT("TD.KillAll kills both"), UTDCheatManager::DoKillAll(World), 2);
	TestTrue(TEXT("enemies dead"), UTDStatics::GetHealth(E1)->IsDead() && UTDStatics::GetHealth(E2)->IsDead());
	TestTrue(TEXT("player alive"), PlayerHealth && PlayerHealth->IsAlive());

	TestFalse(TEXT("TD.SkipWave without a director"), UTDCheatManager::DoSkipWave(World));

	AddExpectedMessage(TEXT("staying put"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	TestFalse(TEXT("TD.Travel bogus"), UTDCheatManager::DoTravel(World, TEXT("Narnia")));
	TestFalse(TEXT("TD.Travel to a map that doesn't exist yet"), UTDCheatManager::DoTravel(World, TEXT("Cinder")));

	GI->BeginScene(TEXT("Wave1"));
	TestTrue(TEXT("TD.Take"), UTDCheatManager::DoTake(World));
	TestTrue(TEXT("retake pending"), GI->IsRetakePending());
	GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
	TestEqual(TEXT("TD.Take -> take 2"), GI->GetTakeNumber(TEXT("Wave1")), 2);

	UTDCheatManager::InstallOn(PC);
	TestTrue(TEXT("TD cheat manager installed"), PC && PC->CheatManager && PC->CheatManager->IsA<UTDCheatManager>());
	UTDCheatManager::InstallOn(PC);
	TestTrue(TEXT("install is idempotent"), PC && PC->CheatManager && PC->CheatManager->IsA<UTDCheatManager>());
	GI->DeleteSave();
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowSandboxPlayer, "TriadDrift.Flow.SandboxPlayerHandOver", TDFlowTest::TestFlags)
bool FTDFlowSandboxPlayer::RunTest(const FString& Parameters)
{
	UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
	const bool bSavedWrite = Settings->bWriteSlotOnCheckpoint;
	Settings->bWriteSlotOnCheckpoint = false;
	{
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		ATDSpaceGameMode* GM = W.GetWorld() ? W.GetWorld()->GetAuthGameMode<ATDSpaceGameMode>() : nullptr;
		TestNotNull(TEXT("space mode"), GM);
		APlayerController* PC = W.SpawnPlayerController();
		W.Advance(0.2f);
		if (GM && PC)
		{
			GM->RestartScene(); // a take: SpawnSquad + hand-over
			W.Advance(0.2f);
			TestTrue(TEXT("TD cheat manager installed in the sandbox"), PC->CheatManager && PC->CheatManager->IsA<UTDCheatManager>());
			if (GM->GetSquadManager())
			{
				int32 Strays = 0;
				for (TActorIterator<ADefaultPawn> It(W.GetWorld()); It; ++It)
				{
					Strays += It->GetController() ? 0 : 1;
				}
				TestEqual(TEXT("no stray login pawn with a squad"), Strays, 0);
				TestFalse(TEXT("player flies a squad ship, not a free camera"), PC->GetPawn() && PC->GetPawn()->IsA<ADefaultPawn>());
			}
			else
			{
				AddInfo(TEXT("Squad manager not merged yet: stray-pawn check skipped (free-fly fallback)."));
			}
		}
	}
	Settings->bWriteSlotOnCheckpoint = bSavedWrite;
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowDirectorSceneIds, "TriadDrift.Flow.WaveSceneIdsComeFromTheDirector", TDFlowTest::TestFlags)
bool FTDFlowDirectorSceneIds::RunTest(const FString& Parameters)
{
	UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
	const bool bSavedWrite = Settings->bWriteSlotOnCheckpoint;
	Settings->bWriteSlotOnCheckpoint = false;
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		APawn* Player = Cast<APawn>(TDFlowTest::SpawnWithHealth(W, ADefaultPawn::StaticClass(), ETDTeam::Player));
		W.SpawnPlayerController(Player);
		ATDWaveDirector* Director = W.Spawn<ATDWaveDirector>();
		if (!TestNotNull(TEXT("GI"), GI) || !TestNotNull(TEXT("director"), Director))
		{
			return false;
		}
		TArray<FTDWaveDef> Table = Director->GetWaveTable();
		if (!TestTrue(TEXT("default wave table"), Table.Num() > 0))
		{
			return false;
		}
		const FName Custom(TEXT("Custom_Ambush"));
		Table[0].SceneId = Custom;
		Director->SetWaveTable(Table);
		GI->NotifyLevelEntered(ETDDestination::Galaxy);
		Director->StartWave(0);
		W.Advance(3.f);
		TestEqual(TEXT("scene id comes from the director's table"), GI->GetCurrentScene(), Custom);
		TestEqual(TEXT("take 1 of the custom scene"), GI->GetTakeNumber(Custom), 1);
		TestEqual(TEXT("no fallback scene opened"), GI->GetTakeNumber(TEXT("Wave1")), 0);

		GI->ReportPlayerDefeated();
		GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
		W.Advance(0.5f);
		TestEqual(TEXT("retake of the custom scene"), GI->GetTakeNumber(Custom), 2);
		TestEqual(TEXT("still in the custom scene after the director rebuilt it"), GI->GetCurrentScene(), Custom);
	}
	Settings->bWriteSlotOnCheckpoint = bSavedWrite;
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowResolveMap, "TriadDrift.Flow.ResolveDestinationMap", TDFlowTest::TestFlags)
bool FTDFlowResolveMap::RunTest(const FString& Parameters)
{
	TDFlowTest::FScopedSettings Scoped;
	FString Package;
	TestFalse(TEXT("bogus path does not resolve"), Scoped.S->ResolveDestinationMap(ETDDestination::Galaxy, Package));
	TestTrue(TEXT("no package for a bogus path"), Package.IsEmpty());
	Scoped.S->DestinationMaps.Add(ETDDestination::Galaxy, FSoftObjectPath(TDFlowTest::ExistingMapA));
	TestTrue(TEXT("existing map resolves"), Scoped.S->ResolveDestinationMap(ETDDestination::Galaxy, Package));
	TestEqual(TEXT("long package name"), Package, FString(TEXT("/Engine/Maps/Entry")));
	Scoped.S->DestinationMaps.Remove(ETDDestination::Galaxy);
	TestFalse(TEXT("unconfigured destination does not resolve"), Scoped.S->ResolveDestinationMap(ETDDestination::Galaxy, Package));
	Scoped.S->DestinationMaps.Add(ETDDestination::Galaxy, FSoftObjectPath());
	TestFalse(TEXT("null path does not resolve"), Scoped.S->ResolveDestinationMap(ETDDestination::Galaxy, Package));
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowMenuMode, "TriadDrift.Flow.MenuModeNeverCheckpoints", TDFlowTest::TestFlags)
bool FTDFlowMenuMode::RunTest(const FString& Parameters)
{
	TDFlowTest::FScopedSettings NoRealMaps;
	FTDTestWorld W(UTDGameInstance::StaticClass(), ATDMenuGameMode::StaticClass());
	W.Advance(0.2f);
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("GI"), GI)) { return false; }
	TestEqual(TEXT("menu destination"), GI->GetCurrentDestination(), ETDDestination::MainMenu);
	TestFalse(TEXT("no checkpoint on the menu"), GI->GetLastCheckpoint().bValid);
	int32 Pawns = 0;
	for (TActorIterator<APawn> It(W.GetWorld()); It; ++It) { ++Pawns; }
	TestEqual(TEXT("no pawn on the menu"), Pawns, 0);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowPlaceholderSky, "TriadDrift.Flow.PlaceholderSkyYieldsToTheMapsSky", TDFlowTest::TestFlags)
bool FTDFlowPlaceholderSky::RunTest(const FString& Parameters)
{
	auto AddComponent = [](AActor* A, UActorComponent* C)
	{
		if (USceneComponent* S = Cast<USceneComponent>(C))
		{
			if (!A->GetRootComponent()) { A->SetRootComponent(S); }
			else { S->SetupAttachment(A->GetRootComponent()); }
		}
		C->RegisterComponent();
		A->AddInstanceComponent(C);
	};
	{
		FTDTestWorld W;
		ATDPlaceholderSky* P = ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), ETDPlaceholderSkyStyle::Space);
		TestTrue(TEXT("empty map: full placeholder"), P && P->HasLights() && P->HasExposure());
	}
	{
		// (b) Nick's tag switches everything off.
		FTDTestWorld W;
		AActor* Sky = W.Spawn<AActor>();
		Sky->Tags.Add(ATDPlaceholderSky::PresentationSkyTag);
		TestNull(TEXT("tagged sky: no placeholder at all"), ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), ETDPlaceholderSkyStyle::Space));
	}
	{
		// (a) A generic actor carrying a directional light COMPONENT (like BP_GalaxySkybox) owns lighting.
		FTDTestWorld W;
		AActor* Rig = W.Spawn<AActor>();
		AddComponent(Rig, NewObject<UDirectionalLightComponent>(Rig, TEXT("Sun")));
		ATDPlaceholderSky* P = ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), ETDPlaceholderSkyStyle::Space);
		TestTrue(TEXT("light component: placeholder keeps only exposure"), P && !P->HasLights() && P->HasExposure());
	}
	{
		// (a) An unbound post-process COMPONENT owns exposure and bloom.
		FTDTestWorld W;
		AActor* Rig = W.Spawn<AActor>();
		UPostProcessComponent* PP = NewObject<UPostProcessComponent>(Rig, TEXT("PP"));
		PP->bUnbound = true;
		AddComponent(Rig, PP);
		ATDPlaceholderSky* P = ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), ETDPlaceholderSkyStyle::Space);
		TestTrue(TEXT("post-process component: placeholder keeps only lights"), P && P->HasLights() && !P->HasExposure());
	}
	{
		// Late sky: streamed in / spawned after BeginPlay -> the placeholder removes itself.
		FTDTestWorld W;
		TWeakObjectPtr<ATDPlaceholderSky> P = ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), ETDPlaceholderSkyStyle::Planet);
		TestTrue(TEXT("placeholder up"), P.IsValid());
		AActor* Sky = W.Spawn<AActor>();
		Sky->Tags.Add(ATDPlaceholderSky::PresentationSkyTag);
		W.Advance(1.f);
		TestFalse(TEXT("late tagged sky removes the placeholder"), P.IsValid() && !P->IsActorBeingDestroyed());
	}
	{
		// Late sky in the space mode: the already-spawned star backdrop goes too (N-004 review MINOR).
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		ATDSpaceGameMode* GM = W.GetWorld()->GetAuthGameMode<ATDSpaceGameMode>();
		TWeakObjectPtr<AActor> Stars = GM ? GM->GetStarBackdrop() : nullptr;
		if (ATDSkySphere::IsArtAvailable())
		{
			// The galaxy layout's nebula sky (T4, art audit) brings its own stars: no placeholder backdrop then.
			TestFalse(TEXT("the engineering nebula sky replaces the star backdrop"), Stars.IsValid());
			return true;
		}
		if (!TestTrue(TEXT("space mode spawned the star backdrop"), Stars.IsValid()))
		{
			return false;
		}
		// Stars read from every direction: unlit glow material, and instances all around the sky.
		TestNotNull(TEXT("glow material referenced on the mode (cooks)"), GM ? GM->GetStarMaterial() : nullptr);
		TInlineComponentArray<UInstancedStaticMeshComponent*> Layers(Stars.Get());
		TestTrue(TEXT("tinted star layers"), Layers.Num() >= 3);
		const FVector Axes[] = { FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector, FVector::UpVector, -FVector::UpVector };
		int32 PerAxis[6] = { 0, 0, 0, 0, 0, 0 };
		for (UInstancedStaticMeshComponent* ISM : Layers)
		{
			UMaterialInterface* M = ISM->GetMaterial(0);
			TestTrue(FString::Printf(TEXT("%s is unlit"), *ISM->GetName()), M && M->GetShadingModels().HasShadingModel(MSM_Unlit));
			TestTrue(FString::Printf(TEXT("%s uses the instanced-safe project glow"), *ISM->GetName()), M && UTDStatics::GetPlaceholderGlowMaterial() && M->GetMaterial() == UTDStatics::GetPlaceholderGlowMaterial()->GetMaterial());
			for (int32 i = 0; i < ISM->GetInstanceCount(); ++i)
			{
				FTransform X;
				ISM->GetInstanceTransform(i, X, true);
				const FVector Dir = (X.GetLocation() - Stars->GetActorLocation()).GetSafeNormal();
				for (int32 a = 0; a < 6; ++a) { if (FVector::DotProduct(Dir, Axes[a]) > 0.8) { ++PerAxis[a]; } }
			}
		}
		for (int32 a = 0; a < 6; ++a)
		{
			TestTrue(FString::Printf(TEXT("stars toward axis %d (%d)"), a, PerAxis[a]), PerAxis[a] > 20);
		}
		// The shell follows the camera, so the sky is full wherever the player flies (the Dominion is outside it).
		if (APlayerController* PC = W.SpawnLocalPlayerController())
		{
			W.Advance(0.2f);
			if (PC->PlayerCameraManager && TestTrue(TEXT("stars still there after a tick"), Stars.IsValid()))
			{
				TestTrue(TEXT("star shell centred on the camera"), FVector::Dist(Stars->GetActorLocation(), PC->PlayerCameraManager->GetCameraLocation()) < 100.f);
			}
		}
		W.Advance(1.f);
		TestTrue(TEXT("stars stay while there is no real sky"), Stars.IsValid() && !Stars->IsActorBeingDestroyed());
		AActor* Sky = W.Spawn<AActor>();
		Sky->Tags.Add(ATDPlaceholderSky::PresentationSkyTag);
		W.Advance(1.f);
		TestFalse(TEXT("late tagged sky removes the star backdrop"), Stars.IsValid() && !Stars->IsActorBeingDestroyed());
		TestNull(TEXT("mode forgets it"), GM ? GM->GetStarBackdrop() : nullptr);
	}
	return true;
}


// ------------------------------------------------------------------------------------------------
// The front end: New Game / Continue / Quit from the menu mode (Nick's widget or the placeholder), the win
// travelling to the victory screen, and the victory screen returning to the menu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowMenuJourney, "TriadDrift.Flow.MenuNewGameContinueVictoryAndBack", TDFlowTest::TestFlags)
bool FTDFlowMenuJourney::RunTest(const FString& Parameters)
{
	UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
	const bool bWrite = S->bWriteSlotOnCheckpoint;
	S->bWriteSlotOnCheckpoint = false;
	ON_SCOPE_EXIT { S->bWriteSlotOnCheckpoint = bWrite; };
	{
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDMenuGameMode::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		ATDMenuGameMode* Menu = W.GetWorld()->GetAuthGameMode<ATDMenuGameMode>();
		if (!TestNotNull(TEXT("GI"), GI) || !TestNotNull(TEXT("menu mode"), Menu)) { return false; }
		GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationMenu"));
		GI->DeleteSave();
		ON_SCOPE_EXIT { GI->DeleteSave(); GI->SetSaveSlotOverride(FString()); };
		{
			UTDGameInstance* Fresh = NewObject<UTDGameInstance>(GetTransientPackage());
			TestTrue(TEXT("tests never use the player's save slot"), Fresh->GetSaveSlotName() != UTDFlowSettings::Get()->SaveSlotName
				&& Fresh->GetSaveSlotName().EndsWith(TEXT("_Automation")));
		}
		TestTrue(TEXT("no in-game HUD on the menu (the dev HUD only replaces a stock AHUD)"), Menu->HUDClass == ATDFrontEndHUD::StaticClass()
			&& GetDefault<ATDTransitionGameMode>()->HUDClass == ATDFrontEndHUD::StaticClass());
		TestFalse(TEXT("the menu, not the victory screen"), Menu->IsShowingVictory());
		TestFalse(TEXT("no Continue without a save"), Menu->CanContinue());
		TestFalse(TEXT("Continue refused"), Menu->ContinueGame());

		FString Opened;
		GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };
		GI->GetMutableProgress().WavesCleared = 3; // leftovers from an earlier run
		TestTrue(TEXT("New Game travels"), Menu->NewGame());
		TestEqual(TEXT("a fresh run"), GI->GetProgressRef().WavesCleared, 0);
		GI->TickFlow(UTDFlowSettings::Get()->TravelFadeSeconds + UTDFlowSettings::Get()->MinTransitionSeconds + 0.5f);
		GI->TickFlow(UTDFlowSettings::Get()->MinTransitionSeconds + 0.5f);
		TestTrue(FString::Printf(TEXT("to the galaxy (%s)"), *Opened), Opened.Contains(TEXT("L_Galaxy")) || Opened.Contains(TEXT("L_Transition")));
		GI->OpenLevelOverride = nullptr;
	}
	{
		// Winning in the galaxy: the victory screen follows after the hold.
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!TestNotNull(TEXT("GI"), GI)) { return false; }
		GI->NotifyLevelEntered(ETDDestination::Galaxy);
		FString Opened;
		GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };
		GI->SetPhase(ETDGamePhase::Victory);
		GI->TickFlow(1.f);
		TestFalse(TEXT("the outro plays first"), GI->IsTravelPending());
		GI->TickFlow(UTDFlowSettings::Get()->VictoryHoldSeconds);
		TestTrue(TEXT("then to the victory screen"), GI->IsTravelPending());
		for (int32 i = 0; i < 20 && !Opened.Contains(TEXT("L_MainMenu")); ++i) { GI->TickFlow(0.5f); if (Opened.Contains(TEXT("L_Transition"))) { GI->NotifyMapLoaded(TEXT("/Game/TriadDrift/Maps/L_Transition")); Opened.Reset(); } }
		TestTrue(FString::Printf(TEXT("the victory screen is the menu map (%s)"), *Opened), Opened.Contains(TEXT("L_MainMenu")));
		GI->OpenLevelOverride = nullptr;
	}
	{
		// The menu map after a win shows the victory screen, then returns to the menu.
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationMenuWon"));
		GI->DeleteSave(); // a won run's slot holds the Victory phase: no Continue either way
		ON_SCOPE_EXIT { GI->SetSaveSlotOverride(FString()); };
		GI->GetMutableProgress().Phase = ETDGamePhase::Victory;
		ATDMenuGameMode* Menu = W.Spawn<ATDMenuGameMode>();
		Menu->StartPlay();
		TestTrue(TEXT("victory screen first"), Menu->IsShowingVictory());
		TestFalse(TEXT("no Continue into a won run"), Menu->CanContinue());
		Menu->ReturnToMenu();
		// Leaving before the ending stinger plays it first (T3, Nov 13), then the menu.
		for (int32 i = 0; i < 600 && Menu->IsShowingVictory(); ++i) { W.Tick(1.f / 30.f); }
		TestFalse(TEXT("then the menu"), Menu->IsShowingVictory());
	}
	{
		// T1's pad pass: closing Settings returns focus to the button that opened it (SETTINGS), not to NEW GAME; a
		// button that can no longer take focus hands it to the first.
		TSharedRef<SButton> NewGame = SNew(SButton);
		TSharedRef<SButton> Settings = SNew(SButton);
		TestTrue(TEXT("back to SETTINGS"), ATDMenuGameMode::ChooseRefocus(Settings, NewGame) == Settings);
		TestTrue(TEXT("nothing remembered: the first button"), ATDMenuGameMode::ChooseRefocus(nullptr, NewGame) == NewGame);
		Settings->SetEnabled(false);
		TestTrue(TEXT("a disabled one: the first button"), ATDMenuGameMode::ChooseRefocus(Settings, NewGame) == NewGame);
	}
	{
		// N-033: with Nick's widgets assigned, a return to the menu after a win brings his MAIN MENU widget back
		// (it used to rebuild the code-built menu instead).
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationMenuWon2"));
		GI->DeleteSave();
		ON_SCOPE_EXIT { GI->SetSaveSlotOverride(FString()); };
		GI->GetMutableProgress().Phase = ETDGamePhase::Victory;
		UTDUIStyle* Style = NewObject<UTDUIStyle>(GetTransientPackage());
		Style->MainMenuWidgetClass = UUserWidget::StaticClass(); // stands in for WBP_MainMenu
		Style->VictoryWidgetClass = UTDPauseMenuWidget::StaticClass();
		ATDMenuGameMode* Menu = W.GetWorld()->SpawnActorDeferred<ATDMenuGameMode>(ATDMenuGameMode::StaticClass(), FTransform::Identity);
		Menu->StyleOverride = Style;
		Menu->FinishSpawning(FTransform::Identity);
		Menu->StartPlay();
		TestTrue(TEXT("Nick's victory widget after a win"), Menu->GetShownWidgetClass() == UTDPauseMenuWidget::StaticClass());
		Menu->ReturnToMenu();
		for (int32 i = 0; i < 600 && Menu->IsShowingVictory(); ++i) { W.Tick(1.f / 30.f); }
		TestTrue(TEXT("RETURN TO MENU: Nick's main-menu widget again"), Menu->GetShownWidgetClass() == UUserWidget::StaticClass());
		TestFalse(TEXT("not the code-built menu"), Menu->UsesPlaceholderMenu());
	}
	return true;
}


// ------------------------------------------------------------------------------------------------
// N-015: the menu music comes from DA_UI_Style.MenuMusic (2D, UI sound), fades as New Game travels, and
// nothing is created when the style has none.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowMenuMusic, "TriadDrift.Flow.MenuMusicFromTheStyleFadesOnNewGame", TDFlowTest::TestFlags)
bool FTDFlowMenuMusic::RunTest(const FString& Parameters)
{
	// Any engine sound stands in for Nick's theme (never referenced by path: only through the style).
	USoundBase* TestSound = LoadObject<USoundBase>(nullptr, TEXT("/Engine/EngineSounds/WhiteNoise.WhiteNoise"));
	if (!TestSound) { TestSound = NewObject<USoundWave>(GetTransientPackage()); }
	for (const bool bWithMusic : { true, false })
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		UTDUIStyle* Style = NewObject<UTDUIStyle>(GetTransientPackage());
		Style->MenuMusic = bWithMusic ? TestSound : nullptr; // Nick's DA_UI_Style: MenuMusic only, every widget class empty
		ATDMenuGameMode* Menu = W.GetWorld()->SpawnActorDeferred<ATDMenuGameMode>(ATDMenuGameMode::StaticClass(), FTransform::Identity);
		Menu->StyleOverride = Style;
		Menu->FinishSpawning(FTransform::Identity);
		Menu->StartPlay();
		if (!bWithMusic)
		{
			TestNull(TEXT("no music asset: nothing created"), Menu->GetMusic());
			continue;
		}
		TestTrue(TEXT("a style with only MenuMusic still gets the placeholder menu"), Menu->UsesPlaceholderMenu());
		UAudioComponent* M = Menu->GetMusic();
		if (!TestNotNull(TEXT("music component created"), M)) { return false; }
		TestTrue(TEXT("the style's sound"), M->Sound == TestSound);
		TestTrue(TEXT("2D UI sound"), M->bIsUISound && !M->bAllowSpatialization);
		TestTrue(TEXT("started"), Menu->WasMusicStarted());
		if (GEngine && GEngine->GetMainAudioDevice())
		{
			TestTrue(TEXT("playing (audio device present)"), M->IsPlaying());
		}
		else
		{
			AddInfo(TEXT("no audio device in this run (-nosound): playback itself is checked in the running game"));
		}
		// The track is not a looping asset: when it ends, the menu plays it again from the top.
		M->OnAudioFinished.Broadcast();
		TestEqual(TEXT("the track ended: it starts again"), Menu->GetMusicLoops(), 1);
		GI->OpenLevelOverride = [](const FString&) {};
		TestTrue(TEXT("New Game"), Menu->NewGame());
		TestTrue(TEXT("the music fades as the travel starts"), Menu->IsMusicFadingOut());
		M->OnAudioFinished.Broadcast();
		TestEqual(TEXT("fading out for the travel: not restarted"), Menu->GetMusicLoops(), 1);
		GI->OpenLevelOverride = nullptr;
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
// Second review, Flow minors (Lead's decisions; T3): RestartSceneNow has ReportPlayerDefeated's travel guard; a failed
// victory travel is retried, then falls back to the menu; a Continue into the assault opens with its slate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlowReviewMinors, "TriadDrift.Flow.Review.RestartGuardVictoryRetryAndAssaultSlate", TDFlowTest::TestFlags)
bool FTDFlowReviewMinors::RunTest(const FString& Parameters)
{
	UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
	const bool bWrite = S->bWriteSlotOnCheckpoint;
	S->bWriteSlotOnCheckpoint = false;
	ON_SCOPE_EXIT { S->bWriteSlotOnCheckpoint = bWrite; };
	{
		// 1. A restart asked for while leaving the level is ignored (no restore into a world about to unload).
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!TestNotNull(TEXT("GI"), GI)) { return false; }
		GI->NotifyLevelEntered(ETDDestination::Galaxy); // checkpoint: 0 waves cleared
		GI->GetMutableProgress().WavesCleared = 4;
		FString Opened;
		GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };
		TestTrue(TEXT("travel to Verdance starts"), GI->TravelTo(ETDDestination::Verdance, NAME_None));
		GI->RestartSceneNow();
		TestEqual(TEXT("no checkpoint restore mid-travel"), GI->GetProgressRef().WavesCleared, 4);
		TestTrue(TEXT("still travelling"), GI->IsTravelPending());
		GI->OpenLevelOverride = nullptr;
	}
	{
		// 2. The victory map can't be opened: retried twice, then the menu.
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!TestNotNull(TEXT("GI"), GI)) { return false; }
		GI->NotifyLevelEntered(ETDDestination::Galaxy);
		const FSoftObjectPath* Was = S->DestinationMaps.Find(ETDDestination::Victory);
		const FSoftObjectPath Saved = Was ? *Was : FSoftObjectPath();
		const bool bHad = Was != nullptr;
		S->DestinationMaps.Add(ETDDestination::Victory, FSoftObjectPath(TEXT("/Game/TriadDrift/Maps/L_DoesNotExist.L_DoesNotExist")));
		ON_SCOPE_EXIT { if (bHad) { S->DestinationMaps.Add(ETDDestination::Victory, Saved); } else { S->DestinationMaps.Remove(ETDDestination::Victory); } };
		FString Opened;
		GI->OpenLevelOverride = [&Opened](const FString& P) { Opened = P; };
		GI->SetPhase(ETDGamePhase::Victory);
		GI->TickFlow(S->VictoryHoldSeconds + 0.1f);
		TestFalse(TEXT("first try fails (no victory map)"), GI->IsTravelPending());
		GI->TickFlow(0.6f);
		TestFalse(TEXT("second try fails"), GI->IsTravelPending());
		GI->TickFlow(0.6f);
		TestTrue(TEXT("third failure: falls back to the menu"), GI->IsTravelPending());
		for (int32 i = 0; i < 20 && !Opened.Contains(TEXT("L_MainMenu")); ++i)
		{
			GI->TickFlow(0.5f);
			if (Opened.Contains(TEXT("L_Transition"))) { GI->NotifyMapLoaded(TEXT("/Game/TriadDrift/Maps/L_Transition")); Opened.Reset(); }
		}
		TestTrue(FString::Printf(TEXT("the menu map opens (%s)"), *Opened), Opened.Contains(TEXT("L_MainMenu")));
		GI->OpenLevelOverride = nullptr;
	}
	{
		// 3. Continue into the assault: the restored scene id is Dominion, and the slate still plays on entry.
		FTDTestWorld W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!TestNotNull(TEXT("GI"), GI)) { return false; }
		UTDFlowTestListener* Listener = NewObject<UTDFlowTestListener>();
		Listener->AddToRoot();
		ON_SCOPE_EXIT { Listener->RemoveFromRoot(); };
		Listener->Bind(UTDEventBus::Get(W.GetWorld()));
		UTDSaveGame* Save = NewObject<UTDSaveGame>();
		Save->Checkpoint.bValid = true;
		Save->Checkpoint.Destination = ETDDestination::Galaxy;
		Save->Checkpoint.SceneId = UTDGameInstance::DominionScene;
		Save->Checkpoint.Progress.Phase = ETDGamePhase::DominionAssault;
		Save->Checkpoint.Progress.Takes.Add(UTDGameInstance::DominionScene, 2);
		TestTrue(TEXT("save applies"), GI->ApplySave(*Save));
		GI->NotifyLevelEntered(ETDDestination::Galaxy);
		const bool bSlate = Listener->Takes.ContainsByPredicate([](const TPair<FName, int32>& T) { return T.Key == UTDGameInstance::DominionScene; });
		TestTrue(TEXT("the assault's slate plays after a Continue"), bSlate);
		const int32 Before = Listener->Takes.Num();
		GI->BeginDominionSceneIfAssault();
		TestEqual(TEXT("and only once"), Listener->Takes.Num(), Before);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
