// Triad Drift - Dominion interior tests (ENG-29/30/31, step 1: the built layout, the room order, the hand-off to T4's
// escape flight, Continue mid-interior, the journey stages and the boarding flags). Owner: Teammate 5.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "EngineUtils.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "TriadDrift/Core/TDStatics.h"

namespace TDInteriorTest
{
	struct FInterior
	{
		TDGroundTest::FNoSaveWrites NoWrites;
		FTDTestWorld W;
		UTDGameInstance* GI = nullptr;
		ATDPlanetCourse* Course = nullptr;

		explicit FInterior(const TArray<FName>& AlreadyDone = TArray<FName>())
			: W(UTDGameInstance::StaticClass())
		{
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			if (GI)
			{
				GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationTest"));
				GI->NotifyLevelEntered(ETDDestination::DominionInterior);
				GI->GetMutableProgress().Phase = ETDGamePhase::DominionInterior;
				for (const FName& Id : AlreadyDone) { GI->GetMutableProgress().CompletedObjectives.Add(Id); }
			}
			FActorSpawnParameters Params;
			Params.bDeferConstruction = true;
			Course = W.GetWorld()->SpawnActor<ATDPlanetCourse>(ATDPlanetCourse::StaticClass(), FTransform::Identity, Params);
			if (Course)
			{
				Course->PlanetId = TEXT("DominionInterior");
				Course->bInteriorTravel = false;
				Course->FinishSpawning(FTransform::Identity);
			}
			W.Advance(0.2f);
		}

		ATDObjective* Obj(int32 Index) const
		{
			return Course ? Course->FindObjective(ATDPlanetCourse::InteriorObjectiveIds()[Index]) : nullptr;
		}

		/** Completes every remaining step of an objective (the fights themselves are played elsewhere). */
		void Finish(ATDObjective* O)
		{
			for (int32 i = 0; O && O->IsActive() && i < 10; ++i)
			{
				O->AdvanceStep();
				W.Advance(0.05f);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorBuilt, "TriadDrift.Ground.Interior.BuildsTheRoomsDoorsAndConsoles", TD_TEST_FLAGS)
bool FTDInteriorBuilt::RunTest(const FString& Parameters)
{
	TDInteriorTest::FInterior I;
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	TestTrue(TEXT("is the interior"), I.Course->IsInterior());
	for (int32 i = 0; i < ATDPlanetCourse::InteriorObjectiveIds().Num(); ++i)
	{
		TestNotNull(*FString::Printf(TEXT("objective %s"), *ATDPlanetCourse::InteriorObjectiveIds()[i].ToString()), I.Obj(i));
	}
	// Every door knows the pilot (storyline.md act 4).
	int32 Doors = 0, Known = 0;
	for (TActorIterator<ATDDoor> It(I.W.GetWorld()); It; ++It) { ++Doors; Known += It->bRecognisesPilot ? 1 : 0; }
	TestEqual(TEXT("six doors"), Doors, 6);
	TestEqual(TEXT("all recognise the pilot"), Known, Doors);
	// One recording console per room, each with its own id; using one asks for its beat.
	const TMap<TObjectPtr<AActor>, FName>& Consoles = I.Course->GetStoryConsoles();
	TestEqual(TEXT("four story consoles (T3's ids)"), Consoles.Num(), 4);
	TSet<FName> Ids;
	for (const TPair<TObjectPtr<AActor>, FName>& C : Consoles) { Ids.Add(C.Value); }
	TestEqual(TEXT("distinct ids"), Ids.Num(), 4);
	TestTrue(TEXT("hangar recording id"), Ids.Contains(TEXT("A4_Recording_Hangar")));
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(I.W, I.Course->GetInteriorStart().GetLocation());
	for (const TPair<TObjectPtr<AActor>, FName>& C : Consoles)
	{
		if (C.Value == TEXT("A4_Recording_Hangar"))
		{
			Cast<ATDInteractableActor>(C.Key.Get())->Interact(P);
		}
	}
	TestEqual(TEXT("the console asked for its recording"), I.Course->GetLastStoryBeat(), FName(TEXT("A4_Recording_Hangar")));
	// The pilot starts on the hangar floor, not in a wall or in the void.
	if (TestNotNull(TEXT("pilot"), P))
	{
		I.W.Advance(1.f);
		TestTrue(*FString::Printf(TEXT("standing in the hangar (z %.0f)"), P->GetActorLocation().Z), P->GetActorLocation().Z > 50.f && P->GetActorLocation().Z < 200.f
			&& FMath::Abs(P->GetActorLocation().X) < 1200.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorChain, "TriadDrift.Ground.Interior.RoomsInOrderThenCoreThenEscapeHandsOver", TD_TEST_FLAGS)
bool FTDInteriorChain::RunTest(const FString& Parameters)
{
	TDInteriorTest::FInterior I;
	if (!TestNotNull(TEXT("interior"), I.Course) || !TestNotNull(TEXT("game instance"), I.GI)) { return false; }
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	// Rooms come in order: the arena cannot start before the corridor.
	TestFalse(TEXT("arena before the corridor: no"), I.Course->StartObjectiveAt(Ids[1]));
	for (int32 i = 0; i < 4; ++i)
	{
		TestTrue(*FString::Printf(TEXT("%s starts"), *Ids[i].ToString()), I.Course->StartObjectiveAt(Ids[i]));
		if (i == 3)
		{
			// The core for real: break the four conduits and the heart with damage (live run: the heart's death
			// completed nothing), and the shield drops between them.
			TArray<ATDShatterCrystal*> Breakables;
			for (TActorIterator<ATDShatterCrystal> It(I.W.GetWorld()); It; ++It) { if (It->LinkedObjective == I.Obj(3)) { Breakables.Add(*It); } }
			TestEqual(TEXT("four conduits and the heart"), Breakables.Num(), 5);
			TestTrue(TEXT("shield up while the conduits stand"), I.Course->GetCoreShield() && I.Course->GetCoreShield()->IsVisible());
			for (ATDShatterCrystal* C : Breakables) { if (C->LinkedStep == 0) { UTDStatics::ApplyTDDamage(C, 500.f, ETDDamageKind::Melee, nullptr, nullptr); } }
			I.W.Advance(0.3f);
			TestEqual(TEXT("conduits down -> the heart"), I.Obj(3)->GetCurrentStepIndex(), 1);
			TestFalse(TEXT("shield dropped"), I.Course->GetCoreShield() && I.Course->GetCoreShield()->IsVisible());
			for (ATDShatterCrystal* C : Breakables) { if (C->LinkedStep == 1) { UTDStatics::ApplyTDDamage(C, 500.f, ETDDamageKind::Melee, nullptr, nullptr); } }
			I.W.Advance(0.3f);
			TestTrue(TEXT("the heart's death destroys the core"), I.Obj(3)->IsComplete());
		}
		if (i + 1 < 4)
		{
			TestFalse(TEXT("one room at a time"), I.Course->StartObjectiveAt(Ids[i + 1]));
		}
		I.Finish(I.Obj(i));
		TestTrue(*FString::Printf(TEXT("%s done"), *Ids[i].ToString()), I.Obj(i)->IsComplete());
		TestTrue(TEXT("saved as done"), I.GI->GetProgressRef().CompletedObjectives.Contains(Ids[i]));
	}
	TestTrue(TEXT("the core is dead in progress"), I.GI->GetProgressRef().bReactorDestroyed);
	// The escape run starts by itself after the core's moment, with its timer.
	I.W.Advance(3.f);
	ATDObjective* Run = I.Obj(4);
	TestTrue(TEXT("the escape run is on"), Run && Run->IsActive());
	bool bClock = false;
	for (TActorIterator<ATDCollapseRun> It(I.W.GetWorld()); It; ++It) { bClock |= It->IsRunning() && It->GetTimeLeft() > 80.f && It->GetTimeLeft() <= 90.f; }
	TestTrue(TEXT("the 90 s clock runs"), bClock);
	// Every door is open for the way back.
	int32 Closed = 0;
	for (TActorIterator<ATDDoor> It(I.W.GetWorld()); It; ++It) { Closed += It->IsOpen() ? 0 : 1; }
	TestEqual(TEXT("doors open for the run"), Closed, 0);
	// Reaching the ship hands over to T4's escape flight (the contract): Phase Escape at the hangar mouth.
	I.Finish(Run);
	TestTrue(TEXT("handed over"), I.Course->HasHandedOff());
	TestEqual(TEXT("phase Escape"), I.GI->GetProgressRef().Phase, ETDGamePhase::Escape);
	TestTrue(TEXT("boarded"), I.GI->GetProgressRef().bDominionBoarded);
	TestEqual(TEXT("return point: the hangar mouth"), I.GI->GetProgressRef().ReturnPoint, TDReturnPoints::DominionHangarExit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorContinue, "TriadDrift.Ground.Interior.ContinueStartsAtTheNextRoom", TD_TEST_FLAGS)
bool FTDInteriorContinue::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	FTransform Fresh, Mid;
	{
		TDInteriorTest::FInterior I;
		if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
		Fresh = I.Course->GetInteriorStart();
	}
	TDInteriorTest::FInterior I({ Ids[0], Ids[1] });
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	Mid = I.Course->GetInteriorStart();
	TestTrue(TEXT("fresh: in the hangar"), Fresh.GetLocation().X < 0.f);
	TestTrue(*FString::Printf(TEXT("after the arena: at the Warden's gate (x %.0f)"), Mid.GetLocation().X), Mid.GetLocation().X > 7600.f && Mid.GetLocation().X < 8400.f);
	TestTrue(TEXT("corridor and arena stay done"), I.Obj(0)->IsComplete() && I.Obj(1)->IsComplete());
	TestTrue(TEXT("the Warden can start"), I.Course->StartObjectiveAt(Ids[2]));
	return true;
}

// Story 07-09: the final record is mandatory and gates the reactor: after the Warden the reactor door stays shut until
// the record (and its queued lines) has played; then it opens. Story consoles answer only out of combat.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorFinalRecord, "TriadDrift.Ground.Interior.FinalRecordGatesTheReactorDoor", TD_TEST_FLAGS)
bool FTDInteriorFinalRecord::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	TDInteriorTest::FInterior I({ Ids[0], Ids[1], Ids[2] }); // through the Warden
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	ATDDoor* Reactor = nullptr;
	for (TActorIterator<ATDDoor> It(I.W.GetWorld()); It; ++It) { if (FMath::Abs(It->GetActorLocation().X - 11200.f) < 50.f) { Reactor = *It; } }
	if (!TestNotNull(TEXT("reactor door"), Reactor)) { return false; }
	I.W.Advance(2.f);
	TestFalse(TEXT("after the Warden the reactor door is still shut"), Reactor->IsOpen());
	ATDSurveyTerminal* Record = Cast<ATDSurveyTerminal>(I.Course->GetFinalRecordConsole());
	if (!TestNotNull(TEXT("final record console"), Record)) { return false; }
	TestTrue(TEXT("outside the reactor room (the Warden room's side)"), Record->GetActorLocation().X < 10400.f);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(I.W, Record->GetActorLocation() + FVector(-200.f, 0.f, 50.f));
	// A crewman near it: the record waits (recordings never compete with a fight).
	ATDCrewCharacter* Crew = I.W.Spawn<ATDCrewCharacter>(Record->GetActorLocation() + FVector(-600.f, 400.f, 50.f));
	I.W.Advance(0.2f);
	TestFalse(TEXT("no record while a hostile is near"), Record->CanInteract(P));
	if (Crew) { Crew->Destroy(); }
	I.W.Advance(0.2f);
	int32 Started = 0;
	const FDelegateHandle H = ATDPlanetCourse::OnFinalRecordStarted.AddLambda([&Started](AActor*) { ++Started; });
	Record->Interact(P);
	ATDPlanetCourse::OnFinalRecordStarted.Remove(H);
	TestEqual(TEXT("T3's hook once"), Started, 1);
	I.W.Advance(0.3f);
	TestFalse(TEXT("the door waits while the record plays"), Reactor->IsOpen() && !I.Course->IsFinalRecordDone());
	for (int32 i = 0; i < 200 && !I.Course->IsFinalRecordDone(); ++i) { I.W.Advance(0.5f); }
	TestTrue(TEXT("the record ends"), I.Course->IsFinalRecordDone());
	I.W.Advance(1.f);
	TestTrue(TEXT("then the reactor door opens"), Reactor->IsOpen());
	return true;
}

// Nicholas (issue #29): the pilot on foot is the flown ship's pilot; the interior parks that ship on the hangar pad.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorBoardingPilot, "TriadDrift.Ground.Interior.BoardingPilotIsTheFlownShips", TD_TEST_FLAGS)
bool FTDInteriorBoardingPilot::RunTest(const FString& Parameters)
{
	TDInteriorTest::FInterior I;
	if (!TestNotNull(TEXT("interior"), I.Course) || !TestNotNull(TEXT("game instance"), I.GI)) { return false; }
	for (const ETDShipId Ship : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		I.GI->GetMutableProgress().LastShip = Ship;
		const FName Name = ATDFirstPersonCharacter::GetOnFootPilotName(I.W.GetWorld());
		TestEqual(TEXT("pilot ship"), ATDFirstPersonCharacter::GetOnFootPilotShip(I.W.GetWorld()), Ship);
		TestEqual(TEXT("pilot name"), Name, Ship == ETDShipId::Lancer ? FName(TEXT("Kato")) : Ship == ETDShipId::Bastion ? FName(TEXT("Moss")) : FName(TEXT("Wren")));
	}
	if (UStaticMeshComponent* Parked = I.Course->GetParkedShip())
	{
		const FBox B = Parked->Bounds.GetBox();
		TestTrue(*FString::Printf(TEXT("the ship stands on the pad (bottom z %.0f)"), B.Min.Z), B.Min.Z > -5.f && B.Min.Z < 20.f);
		TestTrue(TEXT("over the pad"), FMath::Abs(B.GetCenter().X + 200.f) < 400.f && FMath::Abs(B.GetCenter().Y) < 400.f);
	}
	else
	{
		AddInfo(TEXT("no ship art in this checkout: the pad stays empty"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorFlags, "TriadDrift.Flow.Boarding.PhaseSetsFlagsAndStagesExist", TD_TEST_FLAGS)
bool FTDInteriorFlags::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("game instance"), GI)) { return false; }
	GI->GetMutableProgress().Phase = ETDGamePhase::DominionAssault;
	GI->SetPhase(ETDGamePhase::DominionInterior); // T4's docking
	TestTrue(TEXT("interior: boarded"), GI->GetProgressRef().bDominionBoarded);
	TestFalse(TEXT("interior: core not yet dead"), GI->GetProgressRef().bReactorDestroyed);
	GI->SetPhase(ETDGamePhase::Escape);
	TestTrue(TEXT("escape: the core is dead"), GI->GetProgressRef().bReactorDestroyed);

	const FTDProgress Keep;
	const FTDProgress Dock = FTDJourney::MakeProgress(ETDJourneyStage::DominionDocking, Keep);
	TestTrue(TEXT("docking: defences down"), Dock.bDominionDefensesDown && Dock.Phase == ETDGamePhase::DominionAssault && !Dock.bDominionBoarded);
	TestEqual(TEXT("docking: at the approach"), FTDJourney::ReturnPointOf(ETDJourneyStage::DominionDocking), FTDJourney::DominionApproach);
	const FTDProgress In = FTDJourney::MakeProgress(ETDJourneyStage::DominionInterior, Keep);
	TestTrue(TEXT("interior stage"), In.Phase == ETDGamePhase::DominionInterior && In.bDominionBoarded && !In.bReactorDestroyed);
	TestEqual(TEXT("interior: the map"), FTDJourney::DestinationOf(ETDJourneyStage::DominionInterior), ETDDestination::DominionInterior);
	TestEqual(TEXT("interior: the hangar"), FTDJourney::ReturnPointOf(ETDJourneyStage::DominionInterior), FName(TEXT("Hangar")));
	const FTDProgress Out = FTDJourney::MakeProgress(ETDJourneyStage::EscapeFlight, Keep);
	TestTrue(TEXT("escape stage"), Out.Phase == ETDGamePhase::Escape && Out.bReactorDestroyed && Out.bDominionBoarded);
	TestEqual(TEXT("escape: the hangar mouth"), FTDJourney::ReturnPointOf(ETDJourneyStage::EscapeFlight), TDReturnPoints::DominionHangarExit);
	ETDJourneyStage Parsed;
	TestTrue(TEXT("TD.Stage DominionInterior parses"), FTDJourney::Parse(TEXT("DominionInterior"), Parsed) && Parsed == ETDJourneyStage::DominionInterior);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
