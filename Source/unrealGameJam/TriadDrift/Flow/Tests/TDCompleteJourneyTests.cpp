// Triad Drift - ENG-46: the complete-game journey, beat by beat (features.md "Complete-game checklist"). Owner: Teammate 7.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Journey.Complete
//
// One run of the whole game through the real flow: New Game, the tutorial and its completion, the skip path, Aftermath,
// the jump to Verdance, Verdance's objectives and the return, waves 3-4, Cinder, wave 5 and the approach rule, the
// assault, docking, the interior, the escape and victory, with save + Continue at every act boundary.
//
// Two kinds of step:
// - CHECK: behaviour that is built. A failure fails the test.
// - PENDING: a beat whose hook is not built yet. The step probes for it (by name, through reflection, or by a measured
//   rule) and reports "PENDING <beat> (owner): <what is missing>" as an info line; it does NOT fail. When the owner lands
//   the hook the probe turns into a pass on its own, and the summary at the end lists exactly what is still missing.
// Planet levels are not loaded (automation never opens a real map): their objectives go through the game instance's
// real CompleteObjective / NotifyLevelEntered, the same calls the planet game modes make. The planets' own suites test
// the on-foot play.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/Flow/TDSaveGame.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Profile/TDProfileSubsystem.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"

namespace TDCompleteJourney
{
	static const TCHAR* SaveSlot = TEXT("TriadDrift_AutomationCompleteJourney");
	static const TCHAR* ProfileSlot = TEXT("TD_Profile_AutomationCompleteJourney");

	/** The galaxy with the TD game instance, a real local player, isolated save and profile slots. */
	struct FRun
	{
		UTDFlowSettings* S = GetMutableDefault<UTDFlowSettings>();
		bool bWrite = S->bWriteSlotOnCheckpoint;
		FTDTestWorld W;
		UTDGameInstance* GI = nullptr;
		UTDProfileSubsystem* Profile = nullptr;
		TArray<FString> Pending;
		FAutomationTestBase& T;

		explicit FRun(FAutomationTestBase& InTest)
			: W(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass()), T(InTest)
		{
			S->bWriteSlotOnCheckpoint = true; // act-boundary saves are part of the journey
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			if (GI)
			{
				GI->SetSaveSlotOverride(SaveSlot);
				GI->DeleteSave();
				GI->OpenLevelOverride = [](const FString&) {}; // travel never loads a real map in automation
				Profile = GI->GetSubsystem<UTDProfileSubsystem>();
			}
			if (Profile)
			{
				Profile->SetSlotOverrideForTests(ProfileSlot);
				Profile->ResetProfile();
			}
			W.SpawnLocalPlayerController();
			if (ATDSpaceGameMode* GM = W.GetWorld()->GetAuthGameMode<ATDSpaceGameMode>())
			{
				GM->RestartScene();
			}
			W.Advance(0.5f);
		}
		~FRun()
		{
			if (GI) { GI->DeleteSave(); GI->SetSaveSlotOverride(FString()); GI->OpenLevelOverride = nullptr; }
			if (Profile) { Profile->ResetProfile(); Profile->SetSlotOverrideForTests(FString()); }
			S->bWriteSlotOnCheckpoint = bWrite;
		}

		void Pend(const TCHAR* Beat, const TCHAR* Owner, const FString& What)
		{
			const FString Line = FString::Printf(TEXT("PENDING %s (%s): %s"), Beat, Owner, *What);
			Pending.Add(Line);
			T.AddInfo(Line);
		}

		/** Save at an act boundary, read it back into a fresh apply, and check the phase and the waves survive. */
		void SaveAndContinue(const TCHAR* Boundary)
		{
			const FTDProgress Before = GI->GetProgressRef();
			GI->TakeCheckpoint(TEXT("Manual"));
			T.TestTrue(*FString::Printf(TEXT("%s: saved"), Boundary), GI->WriteSaveSlot(true));
			T.TestTrue(*FString::Printf(TEXT("%s: Continue offered"), Boundary), Before.Phase == ETDGamePhase::Victory || GI->HasContinue());
			UTDSaveGame* Save = nullptr;
			if (T.TestTrue(*FString::Printf(TEXT("%s: read back"), Boundary), GI->ReadSaveSlot(Save) && Save != nullptr) && Save)
			{
				T.TestTrue(*FString::Printf(TEXT("%s: applied"), Boundary), GI->ApplySave(*Save));
				T.TestEqual(*FString::Printf(TEXT("%s: phase kept"), Boundary), GI->GetProgressRef().Phase, Before.Phase);
				T.TestEqual(*FString::Printf(TEXT("%s: waves kept"), Boundary), GI->GetProgressRef().WavesCleared, Before.WavesCleared);
				T.TestEqual(*FString::Printf(TEXT("%s: unlocks kept"), Boundary), GI->GetProgressRef().UnlockedAbilities.Num(), Before.UnlockedAbilities.Num());
			}
		}

		/** A planet visit: enter, finish its objectives with their rewards (as the planet game mode does), back to the galaxy. */
		void VisitPlanet(ETDDestination Planet, const TArray<FName>& Objectives, const TArray<TPair<ETDAbilityId, ETDShipId>>& Rewards)
		{
			GI->NotifyLevelEntered(Planet);
			for (int32 i = 0; i < Objectives.Num(); ++i)
			{
				const TPair<ETDAbilityId, ETDShipId> R = Rewards.IsValidIndex(i) ? Rewards[i] : TPair<ETDAbilityId, ETDShipId>(ETDAbilityId::None, ETDShipId::Rift);
				GI->CompleteObjective(Objectives[i], R.Key, R.Value);
			}
			GI->NotifyLevelEntered(ETDDestination::Galaxy); // the return applies the story transition
		}
	};

	/** Does the class have a function or a property with this name (a hook another lane is building)? */
	static bool HasMember(const UClass* Class, const TCHAR* Name)
	{
		return Class && (Class->FindFunctionByName(FName(Name)) != nullptr || FindFProperty<FProperty>(Class, FName(Name)) != nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCompleteJourney, "TriadDrift.Journey.Complete.EveryBeatInPlayOrder", TD_TEST_FLAGS)
bool FTDCompleteJourney::RunTest(const FString& Parameters)
{
	using namespace TDCompleteJourney;
	FRun R(*this);
	if (!TestNotNull(TEXT("game instance"), R.GI) || !TestNotNull(TEXT("profile"), R.Profile)) { return false; }
	UTDGameInstance* GI = R.GI;
	const UTDFlowSettings* FS = UTDFlowSettings::Get();

	// ---- Front end: a first play has no skip offer; New Game arms the intro.
	TestFalse(TEXT("first play: no skip offer"), R.Profile->CanSkipTutorial());
	GI->StartNewRun();
	TestTrue(TEXT("New Game arms the intro"), GI->IsIntroPending());

	// ---- Opening -> tutorial battle: leaving the intro starts waves 1-2.
	TestFalse(TEXT("leaving the intro on a first play does not skip"), UTDProfileSubsystem::LeaveIntro(GI));
	TestEqual(TEXT("tutorial battle phase"), GI->GetProgressRef().Phase, ETDGamePhase::Waves1to2);

	// ---- 1 First Contact: the tutorial ends at the escape boundary (T4's event), else the fallback.
	if (!HasMember(ATDWaveDirector::StaticClass(), TEXT("OnTutorialEscaped")))
	{
		R.Pend(TEXT("Withdrawal / escape boundary"), TEXT("T4, ENG-40"), TEXT("ATDWaveDirector::OnTutorialEscaped does not exist; the tutorial ends on the wave-2 clear fallback"));
	}
	GI->ReportWaveCleared(1);
	TestEqual(TEXT("wave 1 alone ends nothing"), GI->GetProgressRef().Phase, ETDGamePhase::Waves1to2);
	TestFalse(TEXT("wave 1 alone does not complete the tutorial"), R.Profile->IsTutorialComplete());
	bool bAftermathFired = false;
	const FDelegateHandle AftermathProbe = R.Profile->OnAftermathBegins.AddLambda([&bAftermathFired](bool) { bAftermathFired = true; });
	GI->ReportWaveCleared(2);
	TestEqual(TEXT("after the tutorial: on to Verdance"), GI->GetProgressRef().Phase, ETDGamePhase::Verdance);
	TestTrue(TEXT("tutorial complete on the profile"), R.Profile->IsTutorialComplete());

	// ---- *Aftermath*: fired on the completion; the director (T3) must listen.
	TestTrue(TEXT("Aftermath fired on the tutorial's completion"), bAftermathFired);
	R.Profile->OnAftermathBegins.Remove(AftermathProbe);
	if (!R.Profile->OnAftermathBegins.IsBound())
	{
		R.Pend(TEXT("Aftermath"), TEXT("T3, ENG-42"), TEXT("nothing is bound to UTDProfileSubsystem::OnAftermathBegins: S2_* lines, tactical lanes and nav pulse do not play"));
	}
	R.SaveAndContinue(TEXT("after the tutorial"));

	// ---- 1 to 2: one hyperdrive jump reaches Verdance (T1's one-jump change), measured on today's tuning and layout.
	{
		const UTDHyperdriveComponent* HD = GetDefault<UTDHyperdriveComponent>();
		const float Reach = HD->Tuning.JumpSpeed*HD->Tuning.MaxJumpSeconds;
		const ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(R.W.GetWorld());
		FTransform Orbit;
		if (TestNotNull(TEXT("galaxy layout"), Layout) && Layout->FindReturnPoint(TDReturnPoints::VerdanceOrbit, Orbit))
		{
			const float Dist = (float)FVector::Dist(Layout->GetPlayerStart().GetLocation(), Orbit.GetLocation());
			if (Reach < Dist && !HasMember(UTDHyperdriveComponent::StaticClass(), TEXT("JumpToObjective")))
			{
				R.Pend(TEXT("One-jump arrival"), TEXT("T1, ENG-15"), FString::Printf(TEXT("a jump reaches %.1f km, Verdance orbit is %.1f km from the start"), Reach/100000.f, Dist/100000.f));
			}
		}
	}

	// ---- The skip path, in a second run on the same profile: the intro, then Aftermath with the tutorial state.
	{
		TestTrue(TEXT("a later New Game offers the skip"), R.Profile->CanSkipTutorial());
		const FTDProgress Run1 = GI->GetProgressRef();
		TestTrue(TEXT("skip accepted"), R.Profile->StartNewRunSkippingTutorial());
		TestTrue(TEXT("the skipped run still plays the intro"), GI->IsIntroPending());
		TestTrue(TEXT("the intro leads into Aftermath"), UTDProfileSubsystem::LeaveIntro(GI));
		TestEqual(TEXT("skip: phase after the tutorial"), GI->GetProgressRef().Phase, Run1.Phase);
		TestEqual(TEXT("skip: waves 1-2 cleared"), GI->GetProgressRef().WavesCleared, 2);
		TestEqual(TEXT("skip: nothing unlocked"), GI->GetProgressRef().UnlockedAbilities.Num(), 0);
		R.SaveAndContinue(TEXT("after a skip"));
	}

	// ---- 2 Verdance: its objectives and rewards, then the return starts waves 3-4.
	R.Pend(TEXT("Verdance transfer defence"), TEXT("T5, ENG-25"), FString::Printf(TEXT("the flow still lists %s; the device pickup + transfer defence (storyline 14-18) is not a flow objective yet"),
		*FString::JoinBy(FS->VerdanceObjectives, TEXT(", "), [](const FName& N) { return N.ToString(); })));
	R.VisitPlanet(ETDDestination::Verdance, FS->VerdanceObjectives,
		{ TPair<ETDAbilityId, ETDShipId>(ETDAbilityId::GravityWell, ETDShipId::Bastion), TPair<ETDAbilityId, ETDShipId>(ETDAbilityId::Cloak, ETDShipId::Rift) });
	TestEqual(TEXT("back from Verdance: waves 3-4"), GI->GetProgressRef().Phase, ETDGamePhase::Waves3to4);
	TestEqual(TEXT("Gravity Well on Bastion"), GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::GravityWell), ETDShipId::Bastion);
	TestEqual(TEXT("Cloak on Rift"), GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::Cloak), ETDShipId::Rift);
	R.SaveAndContinue(TEXT("after Verdance"));

	// ---- Waves 3-4, then Cinder.
	GI->ReportWaveCleared(3);
	GI->ReportWaveCleared(4);
	TestEqual(TEXT("after wave 4: on to Cinder"), GI->GetProgressRef().Phase, ETDGamePhase::Cinder);
	R.VisitPlanet(ETDDestination::Cinder, FS->CinderObjectives, { TPair<ETDAbilityId, ETDShipId>(ETDAbilityId::EMPPulse, ETDShipId::Lancer) });
	TestEqual(TEXT("back from Cinder: wave 5"), GI->GetProgressRef().Phase, ETDGamePhase::Wave5);
	TestEqual(TEXT("EMP Pulse on Lancer"), GI->GetProgressRef().UnlockedAbilities.FindRef(ETDAbilityId::EMPPulse), ETDShipId::Lancer);
	R.SaveAndContinue(TEXT("after Cinder"));

	// ---- 4 Assault: the approach opens only when Cinder is done AND wave 5 is cleared.
	TestFalse(TEXT("wave 5 running: not yet the assault"), GI->GetProgressRef().Phase == ETDGamePhase::DominionAssault);
	GI->ReportWaveCleared(5);
	TestEqual(TEXT("wave 5 cleared: the assault"), GI->GetProgressRef().Phase, ETDGamePhase::DominionAssault);
	TestTrue(TEXT("assault stage in the world"), FTDJourney::Go(R.W.GetWorld(), ETDJourneyStage::DominionAssault));
	R.W.Advance(8.f, 1.f / 30.f);
	ATDDominion* Dom = ATDDominion::Get(R.W.GetWorld());
	if (!TestNotNull(TEXT("the Dominion"), Dom)) { return false; }
	TestTrue(TEXT("the Dominion is revealed"), Dom->GetState() != ETDDominionState::Hidden);
	TestEqual(TEXT("the assault is the Dominion scene"), GI->GetCurrentScene(), UTDGameInstance::DominionScene);
	R.SaveAndContinue(TEXT("the assault"));

	// ---- 4 Docking (boarding finale) or the reactor from space (fallback ending).
	TestTrue(TEXT("docking stage"), FTDJourney::Go(R.W.GetWorld(), ETDJourneyStage::DominionDocking));
	R.W.Advance(3.f, 1.f / 30.f);
	if (Dom->UsesBoardingFinale())
	{
		TestEqual(TEXT("boarding: the bay is open"), Dom->GetState(), ETDDominionState::DockingOpen);
		TestNotNull(TEXT("boarding: a docking bay"), Dom->GetDockingBay());
	}
	else
	{
		TestEqual(TEXT("fallback ending: the reactor is exposed"), Dom->GetState(), ETDDominionState::ReactorExposed);
	}

	// ---- 4 Interior: the reveal gate at the reactor threshold (Cinematic 08, F_* lines).
	{
		GI->SetPhase(ETDGamePhase::DominionInterior);
		TestTrue(TEXT("interior marks the Dominion boarded"), GI->GetProgressRef().bDominionBoarded);
		R.SaveAndContinue(TEXT("the interior"));
		R.Pend(TEXT("The reveal (final record gate)"), TEXT("T5 gate, T3 queue, ENG-30/42"), TEXT("no mandatory final record before the reactor door; nothing fires F_* (features.md checklist)"));
	}

	// ---- 5 The escape: the heart is dead, the flight out through debris, the radius.
	GI->SetPhase(ETDGamePhase::Escape);
	TestTrue(TEXT("escape implies the reactor destroyed"), GI->GetProgressRef().bReactorDestroyed);
	R.SaveAndContinue(TEXT("the escape"));
	// The escape flight needs a fresh galaxy (the Dominion enters it only from Hidden, as after the interior's travel):
	// Continue the escape save into a second, fresh galaxy world, as a player resuming the escape would.
	{
		FTDTestWorld W2(UTDGameInstance::StaticClass(), ATDSpaceGameMode::StaticClass());
		UTDGameInstance* GI2 = Cast<UTDGameInstance>(W2.GetGameInstance());
		if (TestNotNull(TEXT("fresh galaxy for the escape"), GI2))
		{
			GI2->SetSaveSlotOverride(SaveSlot);
			GI2->OpenLevelOverride = [](const FString&) {};
			UTDSaveGame* Save = nullptr;
			TestTrue(TEXT("Continue from the escape"), GI2->ReadSaveSlot(Save) && Save && GI2->ApplySave(*Save));
			W2.Advance(1.5f, 1.f / 30.f); // the Dominion polls the progress
			ATDDominion* EscDom = ATDDominion::Get(W2.GetWorld());
			if (TestNotNull(TEXT("the Dominion for the escape"), EscDom))
			{
				TestEqual(TEXT("the escape flight runs"), EscDom->GetState(), ETDDominionState::EscapeFlight);
				TestTrue(TEXT("the escape has a clock"), EscDom->GetEscapeTimeLeft() > 0.f);
				TestTrue(TEXT("the escape radius is real"), EscDom->GetEscapeExitRadius() > 0.f);
			}
			GI2->OpenLevelOverride = nullptr;
			GI2->SetSaveSlotOverride(FString());
		}
	}

	// ---- Ending: victory, no Continue into a won run.
	TestTrue(TEXT("victory stage"), FTDJourney::Go(R.W.GetWorld(), ETDJourneyStage::Victory));
	R.W.Advance(0.5f, 1.f / 30.f);
	TestEqual(TEXT("victory"), GI->GetProgressRef().Phase, ETDGamePhase::Victory);
	GI->TakeCheckpoint(TEXT("Manual"));
	GI->WriteSaveSlot(true);
	TestFalse(TEXT("no Continue after victory"), GI->HasContinue());

	// ---- The summary: what still stands between today and a complete game, by owner.
	AddInfo(FString::Printf(TEXT("ENG-46 complete journey: %d beat(s) pending%s"), R.Pending.Num(), R.Pending.Num() ? TEXT(":") : TEXT(" - every beat built")));
	for (const FString& P : R.Pending)
	{
		AddInfo(TEXT("  ") + P);
	}
	return true;
}

#endif
