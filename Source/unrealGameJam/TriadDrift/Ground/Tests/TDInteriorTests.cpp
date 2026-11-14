// Triad Drift - Dominion interior tests (ENG-29/30/31, step 1: the built layout, the room order, the hand-off to T4's
// escape flight, Continue mid-interior, the journey stages and the boarding flags). Owner: Teammate 5.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Planets/TDPlanetProps.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Ground/TDGroundBarks.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Squad/TDBarkLines.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "EngineUtils.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Squad/TDShipLibrary.h"

namespace TDInteriorTest
{
	struct FInterior
	{
		TDGroundTest::FNoSaveWrites NoWrites;
		FTDTestWorld W;
		UTDGameInstance* GI = nullptr;
		ATDPlanetCourse* Course = nullptr;

		/** Docked: the ship the pilot boarded from (Progress.LastShip), or null for the default. */
		explicit FInterior(const TArray<FName>& AlreadyDone = TArray<FName>(), const ETDShipId* Docked = nullptr)
			: W(UTDGameInstance::StaticClass())
		{
			GI = Cast<UTDGameInstance>(W.GetGameInstance());
			if (GI)
			{
				GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationTest"));
				GI->NotifyLevelEntered(ETDDestination::DominionInterior);
				GI->GetMutableProgress().Phase = ETDGamePhase::DominionInterior;
				for (const FName& Id : AlreadyDone) { GI->GetMutableProgress().CompletedObjectives.Add(Id); }
				if (Docked) { GI->GetMutableProgress().LastShip = *Docked; }
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
	TestTrue(*FString::Printf(TEXT("the door shows the playback ('%s')"), *Reactor->GetStatusText().ToString()), Reactor->GetStatusText().ToString().Contains(TEXT("PLAYBACK")));
	// A second press skips (the Lead's (b)); without a sequence the queue simply plays out.
	I.W.Advance(1.5f);
	if (!I.Course->IsFinalRecordDone()) { Record->Interact(P); }
	for (int32 i = 0; i < 200 && !I.Course->IsFinalRecordDone(); ++i) { I.W.Advance(0.5f); }
	TestTrue(TEXT("the record ends (or was skipped)"), I.Course->IsFinalRecordDone());
	TestTrue(TEXT("the playback line is cleared"), Reactor->GetStatusText().IsEmpty());
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
	const ETDShipId Docked = ATDFirstPersonCharacter::GetOnFootPilotShip(I.W.GetWorld()); // the ship parked when the interior loaded
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
		// Every visual part is parked with it (the Rift's portal ring), as the flying ship builds them.
		if (const UTDShipVisuals* Art = UTDShipLibrary::LoadVisuals(UTDShipLibrary::Get(Docked)))
		{
			int32 Want = 0;
			for (const FTDShipVisualPart& Spec : Art->Parts) { Want += Spec.Mesh.IsNull() || !Spec.Mesh.LoadSynchronous() ? 0 : 1; }
			TestEqual(TEXT("every visual part parked"), I.Course->GetParkedParts().Num(), Want);
			for (const UStaticMeshComponent* Part : I.Course->GetParkedParts())
			{
				TestTrue(TEXT("part near the ship"), Part && FVector::Dist(Part->Bounds.Origin, B.GetCenter()) < 2500.f);
			}
		}
	}
	else
	{
		AddInfo(TEXT("no ship art in this checkout: the pad stays empty"));
	}
	return true;
}

// Ryan (Nov 13): the Rift on the hangar pad had an empty portal ring. The parked ship is built like the flying one: the
// hull plus every visual part, for each of the three boarding ships.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorParkedParts, "TriadDrift.Ground.Interior.ParkedShipHasEveryVisualPart", TD_TEST_FLAGS)
bool FTDInteriorParkedParts::RunTest(const FString& Parameters)
{
	for (const ETDShipId Ship : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		TDInteriorTest::FInterior I(TArray<FName>(), &Ship);
		if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
		const UTDShipVisuals* Art = UTDShipLibrary::LoadVisuals(UTDShipLibrary::Get(Ship));
		if (!Art)
		{
			AddInfo(FString::Printf(TEXT("ship %d: no visuals in this checkout"), (int32)Ship));
			continue;
		}
		int32 Want = 0;
		for (const FTDShipVisualPart& Spec : Art->Parts) { Want += Spec.Mesh.IsNull() || !Spec.Mesh.LoadSynchronous() ? 0 : 1; }
		TestTrue(*FString::Printf(TEXT("ship %d parked"), (int32)Ship), I.Course->GetParkedShip() != nullptr);
		TestEqual(*FString::Printf(TEXT("ship %d: every visual part parked"), (int32)Ship), I.Course->GetParkedParts().Num(), Want);
		AddInfo(FString::Printf(TEXT("ship %d: %d parts parked"), (int32)Ship, I.Course->GetParkedParts().Num()));
	}
	return true;
}

// The environment builder's alarm (Lead, Nov 14): every kit piece with a strip slot is tagged TD.EmergencyStrip and has
// its own instance on that slot; nothing else carries the tag.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorStrips, "TriadDrift.Ground.Interior.EmergencyStripsAreTaggedWithTheirOwnInstance", TD_TEST_FLAGS)
bool FTDInteriorStrips::RunTest(const FString& Parameters)
{
	TDInteriorTest::FInterior I;
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	int32 Tagged = 0, StripSlots = 0;
	for (TActorIterator<AActor> It(I.Course->GetWorld()); It; ++It)
	{
		TArray<UStaticMeshComponent*> Comps;
		It->GetComponents(Comps);
		for (UStaticMeshComponent* C : Comps)
		{
			bool bStrip = false, bOwn = true;
			for (int32 i = 0; i < C->GetNumMaterials(); ++i)
			{
				UMaterialInterface* M = C->GetMaterial(i);
				const UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(M);
				const UMaterialInterface* Base = D ? D->Parent.Get() : M;
				if (Base && Base->GetName().StartsWith(TEXT("MI_KitD_Glow")))
				{
					bStrip = true;
					++StripSlots;
					if (!D) { AddInfo(FString::Printf(TEXT("%s (%s) slot %d: %s %s"), *C->GetName(), *GetNameSafe(C->GetStaticMesh()), i, *M->GetClass()->GetName(), *M->GetPathName())); }
					bOwn &= D != nullptr;
				}
			}
			const bool bTag = C->ComponentHasTag(ATDPlanetCourse::EmergencyStripTag);
			Tagged += bTag ? 1 : 0;
			if (bStrip || bTag)
			{
				TestEqual(*FString::Printf(TEXT("%s: tagged exactly when it has a strip"), *C->GetName()), bTag, bStrip);
				TestTrue(*FString::Printf(TEXT("%s: its strip slot has its own instance"), *C->GetName()), bOwn);
			}
		}
	}
	if (!FPackageName::DoesPackageExist(TEXT("/Game/TriadDrift/Gameplay/Props/Kit/Dominion/MI_KitD_Glow")))
	{
		AddInfo(TEXT("no kit strip material in this checkout"));
		return true;
	}
	AddInfo(FString::Printf(TEXT("%d strip pieces, %d strip slots"), Tagged, StripSlots));
	TestTrue(TEXT("the kit's strips are tagged"), Tagged > 0);
	return true;
}

// T1's music bridge (N-089): the interior fires its four moments once each, when they happen.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorMoments, "TriadDrift.Ground.Interior.MusicMomentsFireOnceWhenTheyHappen", TD_TEST_FLAGS)
bool FTDInteriorMoments::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	TDInteriorTest::FInterior I({ Ids[0], Ids[1], Ids[2] });
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	TArray<FName> Heard;
	const FDelegateHandle H = ATDPlanetCourse::OnInteriorMoment.AddLambda([&Heard](FName M, AActor*) { Heard.Add(M); });
	ATDFirstPersonCharacter* Pilot = TDGroundTest::SpawnPlayer(I.W, FVector(9000.f, 0.f, 95.f)); // the link before the chamber
	I.W.Advance(0.3f);
	TestFalse(TEXT("not in the chamber yet"), Heard.Contains(FName(TEXT("CoreChamberEntered"))));
	const FBox Box = I.Course->GetCoreChamberBox();
	if (Pilot && Box.IsValid)
	{
		Pilot->SetActorLocation(FVector(Box.Min.X + 600.f, 0.f, 95.f), false, nullptr, ETeleportType::TeleportPhysics);
		I.W.Advance(0.3f);
		I.W.Advance(0.3f);
		AddInfo(FString::Printf(TEXT("pilot %s at %s (controlled %d, dead %d), box %s"), *GetNameSafe(Pilot), *Pilot->GetActorLocation().ToCompactString(),
			Pilot->IsPlayerControlled(), Pilot->IsDead(), *Box.ToString()));
	}
	TestEqual(TEXT("CoreChamberEntered once on the crossing"), Heard.FilterByPredicate([](const FName& M) { return M == TEXT("CoreChamberEntered"); }).Num(), 1);
	TestTrue(TEXT("Core starts"), I.Course->StartObjectiveAt(Ids[3]));
	I.W.Advance(0.2f);
	TestTrue(TEXT("ReactorFightStarted"), Heard.Contains(FName(TEXT("ReactorFightStarted"))));
	TestFalse(TEXT("not ended yet"), Heard.Contains(FName(TEXT("ReactorFightEnded"))));
	I.Finish(I.Obj(3));
	TestEqual(TEXT("ReactorFightEnded once"), Heard.FilterByPredicate([](const FName& M) { return M == TEXT("ReactorFightEnded"); }).Num(), 1);
	I.Course->StartObjectiveAt(Ids[4]);
	I.W.Advance(0.2f);
	TestTrue(TEXT("EscapeStarted"), Heard.Contains(FName(TEXT("EscapeStarted"))));
	ATDPlanetCourse::OnInteriorMoment.Remove(H);
	AddInfo(FString::Printf(TEXT("heard: %s"), *FString::JoinBy(Heard, TEXT(", "), [](const FName& N) { return N.ToString(); })));
	return true;
}

// DA_ReactorCore_FX (the effects builder fills it): each slot fires at its event in the real reactor fight. Empty slots
// play nothing (today's behaviour); the course logs every slot either way.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorReactorFX, "TriadDrift.Ground.Interior.ReactorFXSlotsFireAtTheirEvents", TD_TEST_FLAGS)
bool FTDInteriorReactorFX::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	TDInteriorTest::FInterior I({ Ids[0], Ids[1], Ids[2] });
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	auto Count = [&I](const TCHAR* Slot) { return I.Course->GetReactorFXLog().FilterByPredicate([Slot](const FName& N) { return N == Slot; }).Num(); };
	TestEqual(TEXT("CoreIdle from the start"), Count(TEXT("CoreIdle")), 1);
	TestTrue(TEXT("Core starts"), I.Course->StartObjectiveAt(Ids[3]));
	I.W.Advance(0.2f);
	TArray<ATDShatterCrystal*> Breakables;
	for (TActorIterator<ATDShatterCrystal> It(I.W.GetWorld()); It; ++It) { if (It->LinkedObjective == I.Obj(3)) { Breakables.Add(*It); } }
	for (ATDShatterCrystal* C : Breakables) { if (C->LinkedStep == 0) { UTDStatics::ApplyTDDamage(C, 500.f, ETDDamageKind::Melee, nullptr, nullptr); } }
	I.W.Advance(0.3f);
	TestEqual(TEXT("ConduitBreak at each conduit"), Count(TEXT("ConduitBreak")), 4);
	TestEqual(TEXT("ShieldDrop once"), Count(TEXT("ShieldDrop")), 1);
	for (ATDShatterCrystal* C : Breakables) { if (C->LinkedStep == 1) { UTDStatics::ApplyTDDamage(C, 40.f, ETDDamageKind::Melee, nullptr, nullptr); } }
	I.W.Advance(0.1f);
	TestTrue(TEXT("HeartHit on a hit"), Count(TEXT("HeartHit")) >= 1);
	TestEqual(TEXT("not destroyed yet"), Count(TEXT("CoreDestroyed")), 0);
	for (ATDShatterCrystal* C : Breakables) { if (C->LinkedStep == 1) { UTDStatics::ApplyTDDamage(C, 500.f, ETDDamageKind::Melee, nullptr, nullptr); } }
	I.W.Advance(0.3f);
	TestEqual(TEXT("CoreDestroyed once"), Count(TEXT("CoreDestroyed")), 1);
	TestTrue(TEXT("Escape starts"), I.Course->StartObjectiveAt(Ids[4]));
	I.W.Advance(0.2f);
	TestEqual(TEXT("EscapeRunLoop once"), Count(TEXT("EscapeRunLoop")), 1);
	AddInfo(FString::Printf(TEXT("log: %s"), *FString::JoinBy(I.Course->GetReactorFXLog(), TEXT(", "), [](const FName& N) { return N.ToString(); })));
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

// T7's play (Nov 14, main 50c4af7a): dying in the Warden fight came back as take 2 with the objective on the reactor's
// conduits and no Warden. A retake restarts that scene: the Warden alive at full health and the objective unchanged.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorWardenRetake, "TriadDrift.Ground.Interior.DyingToTheWardenRetakesTheWardenFight", TD_TEST_FLAGS)
bool FTDInteriorWardenRetake::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	TDInteriorTest::FInterior I({ Ids[0], Ids[1] });
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(I.W, I.Course->GetInteriorStart().GetLocation() + FVector(0.f, 0.f, 95.f));
	ATDObjective* Gate = I.Obj(2);
	ATDObjective* Reactor = I.Obj(3);
	if (!TestNotNull(TEXT("pilot"), P) || !TestNotNull(TEXT("gate"), Gate) || !TestTrue(TEXT("the Warden starts"), I.Course->StartObjectiveAt(Ids[2]))) { return false; }
	I.W.Advance(0.5f);
	ATDEncounterVolume* Boss = nullptr;
	for (TActorIterator<ATDEncounterVolume> It(I.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Gate) { Boss = *It; } }
	if (!TestNotNull(TEXT("the Warden's room"), Boss)) { return false; }
	P->SetActorLocation(Boss->GetActorLocation() + FVector(-600.f, 0.f, -100.f));
	I.W.Advance(1.f);
	Boss->ActivateIfPlayerInside();
	ATDCrewCharacter* Warden = nullptr;
	for (const TWeakObjectPtr<AActor>& M : Boss->GetSpawned()) { if (ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get()); C && C->Spec.Type == ETDCrewType::Warden) { Warden = C; } }
	if (!TestNotNull(TEXT("the Warden is up"), Warden)) { return false; }
	Warden->SetOpeningGrace(0.f);
	Warden->StartAttack(ETDCrewAttack::Summon, P);
	I.W.Advance(Warden->Spec.BurstWindup + 0.3f);
	UTDStatics::ApplyTDDamage(Warden, Warden->GetHealth()->MaxHull * 0.4f, ETDDamageKind::Melee, P, P, Warden->GetActorLocation(), FVector(1.f, 0.f, 0.f));
	AddInfo(FString::Printf(TEXT("take 1: room alive %d, Warden hull %.0f"), Boss->GetAliveCount(), Warden->GetHealth()->GetHull()));

	// Die: CUT, take 2.
	P->GetHealth()->Kill(nullptr, ETDDamageKind::Environmental);
	I.GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
	I.W.Advance(2.f);
	ATDFirstPersonCharacter* P2 = nullptr;
	for (TActorIterator<ATDFirstPersonCharacter> It(I.W.GetWorld()); It; ++It) { if (!It->IsDead()) { P2 = *It; } }
	TestTrue(TEXT("the Warden objective is still the one on"), Gate->IsActive() && !Gate->IsComplete());
	TestEqual(TEXT("still DEFEAT THE WARDEN (step 0)"), Gate->GetCurrentStepIndex(), 0);
	TestFalse(TEXT("the reactor has not started"), Reactor && Reactor->IsActive());
	if (P2 && !Boss->IsActive()) { P2->SetActorLocation(Boss->GetActorLocation() + FVector(-600.f, 0.f, -100.f)); I.W.Advance(1.f); Boss->ActivateIfPlayerInside(); }
	ATDCrewCharacter* Again = nullptr;
	int32 Others = 0;
	for (TActorIterator<ATDCrewCharacter> It(I.W.GetWorld()); It; ++It)
	{
		if (It->IsDead()) { continue; }
		if (It->Spec.Type == ETDCrewType::Warden) { Again = *It; } else { ++Others; }
	}
	if (TestNotNull(TEXT("take 2: a Warden"), Again))
	{
		TestEqual(TEXT("take 2: at full health"), Again->GetHealth()->GetHull(), Again->GetHealth()->MaxHull);
		TestTrue(TEXT("take 2: it is a fresh one"), Again != Warden);
	}
	TestEqual(TEXT("take 2: no summons left from take 1"), Others, 0);
	return true;
}

// T7 (Nov 14): 17-18 takes in the arena with no easing. Every arena wave eases from take 2.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorArenaMercy, "TriadDrift.Ground.Interior.ArenaWavesHaveDirectorsMercy", TD_TEST_FLAGS)
bool FTDInteriorArenaMercy::RunTest(const FString& Parameters)
{
	TDInteriorTest::FInterior I;
	ATDObjective* Arena = I.Obj(1);
	if (!TestNotNull(TEXT("arena"), Arena)) { return false; }
	int32 Waves = 0;
	for (TActorIterator<ATDEncounterVolume> It(I.W.GetWorld()); It; ++It)
	{
		if (It->LinkedObjective != Arena) { continue; }
		++Waves;
		int32 First = MAX_int32;
		for (const FTDRetakeMercyTier& T : It->MercyTiers) { First = FMath::Min(First, T.FromTake); }
		TestEqual(*FString::Printf(TEXT("%s eases from take 2"), *It->GetName()), First, 2);
	}
	TestEqual(TEXT("three waves"), Waves, 3);
	return true;
}

// T7 (Nov 14): the reactor's conduits carried no marker. While they are the step the marker is the nearest one standing.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorConduitMarkers, "TriadDrift.Ground.Interior.ConduitMarkerIsTheNearestStanding", TD_TEST_FLAGS)
bool FTDInteriorConduitMarkers::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	TDInteriorTest::FInterior I({ Ids[0], Ids[1], Ids[2] });
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(I.W, I.Course->GetInteriorStart().GetLocation() + FVector(0.f, 0.f, 95.f));
	ATDObjective* Reactor = I.Obj(3);
	if (!TestNotNull(TEXT("pilot"), P) || !TestNotNull(TEXT("reactor"), Reactor)) { return false; }
	I.Course->StartObjectiveAt(Ids[3]);
	if (!TestTrue(TEXT("the reactor is on"), Reactor->IsActive())) { return false; }
	TArray<ATDShatterCrystal*> Conduits;
	for (TActorIterator<ATDShatterCrystal> It(I.W.GetWorld()); It; ++It) { if (It->LinkedObjective == Reactor && It->LinkedStep == 0) { Conduits.Add(*It); } }
	TestEqual(TEXT("four conduits"), Conduits.Num(), 4);
	for (int32 Down = 0; Down < 3; ++Down)
	{
		I.W.Advance(0.3f); // the course ticks every 0.2 s
		AActor* M = Reactor->GetCurrentWaypoint();
		ATDShatterCrystal* C = Cast<ATDShatterCrystal>(M);
		if (!TestNotNull(*FString::Printf(TEXT("with %d down the marker is a conduit (%s)"), Down, *GetNameSafe(M)), C)) { return false; }
		TestTrue(TEXT("a standing one"), UTDStatics::GetHealth(C) && UTDStatics::GetHealth(C)->IsAlive());
		for (const ATDShatterCrystal* O : Conduits)
		{
			const UTDHealthComponent* H = UTDStatics::GetHealth(O);
			if (H && H->IsAlive()) { TestTrue(TEXT("the nearest"), FVector::Dist(P->GetActorLocation(), C->GetActorLocation()) <= FVector::Dist(P->GetActorLocation(), O->GetActorLocation()) + 1.f); }
		}
		UTDStatics::GetHealth(C)->Kill(P, ETDDamageKind::Melee);
	}
	return true;
}

// T7 / the structures builder (Nov 14): the hangar start stood behind the parked Rift, and the pilot walking to the door
// stopped at x -484 on every step. For every ship that can be parked there (the one the pilot boarded from), the start
// is clear of the hull, and the pilot walks from it through the hangar door (x 1200). Staged starts and docking use the
// same start (GetInteriorStart).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorHangarStartClear, "TriadDrift.Ground.Interior.HangarStartHasAClearWayToTheDoor", TD_TEST_FLAGS)
bool FTDInteriorHangarStartClear::RunTest(const FString& Parameters)
{
	for (const ETDShipId Ship : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		TDInteriorTest::FInterior I(TArray<FName>(), &Ship);
		if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
		const FTransform Start = I.Course->GetInteriorStart();
		const FVector A = Start.GetLocation() + FVector(0.f, 0.f, 90.f);
		int32 Parts = 0;
		for (const TObjectPtr<UStaticMeshComponent>& Part : I.Course->GetParkedParts())
		{
			if (!Part) { continue; }
			++Parts;
			const FBox Box = Part->Bounds.GetBox().ExpandBy(50.f);
			TestFalse(*FString::Printf(TEXT("ship %d: %s clear of the start %s"), (int32)Ship, *Part->GetName(), *A.ToCompactString()), Box.IsInside(A));
		}
		// Walk it: toward the hangar door, from the start.
		ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(I.W, Start.GetLocation() + FVector(0.f, 0.f, 95.f));
		if (!TestNotNull(TEXT("pilot"), P)) { return false; }
		const FVector Door = I.Course->GetActorTransform().TransformPosition(FVector(1200.f, 0.f, 0.f));
		for (float T = 0.f; T < 12.f && FVector::Dist2D(P->GetActorLocation(), Door) > 150.f; T += 1.f / 30.f)
		{
			const FVector To = (Door - P->GetActorLocation()).GetSafeNormal2D();
			P->AddMovementInput(To, 1.f);
			I.W.Tick(1.f / 30.f);
		}
		const float Left = FVector::Dist2D(P->GetActorLocation(), Door);
		TestTrue(*FString::Printf(TEXT("ship %d: walked from %s to the door (%.1f m left, at %s; %d parts)"), (int32)Ship, *Start.GetLocation().ToCompactString(),
			Left / 100.f, *P->GetActorLocation().ToCompactString(), Parts), Left <= 200.f);
	}
	return true;
}

// N-093 (Nicholas's approved script): at 30 s left of the escape run Moss says "Thirty seconds. Keep going."
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorEscape30, "TriadDrift.Ground.Interior.EscapeThirtySecondsIsMossKeepGoing", TD_TEST_FLAGS)
bool FTDInteriorEscape30::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	TDInteriorTest::FInterior I({ Ids[0], Ids[1], Ids[2], Ids[3] });
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(I.W, I.Course->GetInteriorStart().GetLocation() + FVector(0.f, 0.f, 95.f));
	if (!TestNotNull(TEXT("pilot"), P)) { return false; }
	P->GetHealth()->SetInvulnerable(true);
	UTDGroundBarks* B = UTDGroundBarks::Get(I.W.GetWorld());
	if (!TestNotNull(TEXT("barks"), B)) { return false; }
	B->ResetForTests();
	TestTrue(TEXT("the escape starts"), I.Course->StartObjectiveAt(Ids[4]));
	for (float T = 0.f; T < 120.f && I.Course->GetEscapeSecondsLeft() > 28.f; T += 0.5f) { I.W.Advance(0.5f); }
	I.W.Advance(1.f);
	TestTrue(*FString::Printf(TEXT("30 s left (%.1f)"), I.Course->GetEscapeSecondsLeft()), I.Course->GetEscapeSecondsLeft() <= 30.f);
	TestTrue(TEXT("Bark_Dom_Escape_30 said"), B->GetSaid().Contains(FName(TEXT("Bark_Dom_Escape_30"))));
	TestEqual(TEXT("by Moss"), B->GetLastSpeaker(TEXT("Bark_Dom_Escape_30")), FName(TEXT("Moss")));
	FTDBarkLines::ResetForTests();
	FText Line;
	if (!FTDBarkLines::FindRow(TEXT("Bark_Dom_Escape_30"), TEXT("Moss")))
	{
		AddWarning(TEXT("DT_BarksFallback is not imported in this checkout"));
		return true;
	}
	TestTrue(TEXT("a line"), FTDBarkLines::PickLine(I.W.GetWorld(), TEXT("Bark_Dom_Escape_30"), TEXT("Moss"), Line));
	TestEqual(TEXT("Nicholas's words"), Line.ToString(), FString(TEXT("Thirty seconds. Keep going.")));
	return true;
}

// T7 (Nov 14): after each arena retake the pilot died to Trooper bolts within 6-7 s (38 takes): a wave's ring put crew
// 1-4 m from the retake spot. Every arena spawn point is at least 14 m from the arena's retake spots.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorArenaSpawnsFar, "TriadDrift.Ground.Interior.ArenaWavesSpawnFarFromTheRetakeSpot", TD_TEST_FLAGS)
bool FTDInteriorArenaSpawnsFar::RunTest(const FString& Parameters)
{
	TDInteriorTest::FInterior I;
	ATDObjective* Arena = I.Obj(1);
	if (!TestNotNull(TEXT("arena"), Arena)) { return false; }
	TArray<FVector> Spots;
	for (const FTDObjectiveStep& S : Arena->Steps) { if (S.bCheckpoint && S.RespawnAt) { Spots.Add(S.RespawnAt->GetActorLocation()); } }
	TestTrue(TEXT("the arena has retake spots"), Spots.Num() >= 1);
	int32 Points = 0;
	for (TActorIterator<ATDEncounterVolume> It(I.W.GetWorld()); It; ++It)
	{
		if (It->LinkedObjective != Arena) { continue; }
		TestTrue(*FString::Printf(TEXT("%s has explicit spawn points"), *It->GetName()), It->SpawnPoints.Num() > 0);
		for (const FVector& P : It->SpawnPoints)
		{
			++Points;
			const FVector At = It->GetActorTransform().TransformPosition(P);
			for (const FVector& Spot : Spots)
			{
				TestTrue(*FString::Printf(TEXT("%s spawn %s is %.1f m from the retake spot"), *It->GetName(), *At.ToCompactString(), FVector::Dist2D(At, Spot) / 100.f),
					FVector::Dist2D(At, Spot) >= 1400.f);
			}
		}
	}
	TestEqual(TEXT("nine spawn points"), Points, 9);
	return true;
}

// N-052 (T3's G8 sequences): the reactor room starts with Face to Face for the boarding pilot, and overcoming the room's
// defenders plays its "overcome" half.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorFaceToFace, "TriadDrift.Ground.Interior.FaceToFaceAtTheReactorAndWhenItsDefendersFall", TD_TEST_FLAGS)
bool FTDInteriorFaceToFace::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	TDInteriorTest::FInterior I({ Ids[0], Ids[1], Ids[2] });
	if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(I.W, I.Course->GetInteriorStart().GetLocation() + FVector(0.f, 0.f, 95.f));
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(I.W.GetWorld());
	if (!TestNotNull(TEXT("pilot"), P) || !TestNotNull(TEXT("story"), Story)) { return false; }
	P->GetHealth()->SetInvulnerable(true);
	const ETDShipId Boarder = ATDFirstPersonCharacter::GetOnFootPilotShip(I.W.GetWorld());
	const FName Entry = UTDStorySubsystem::FaceToFaceSequence(Boarder, false);
	const FName Over = UTDStorySubsystem::FaceToFaceSequence(Boarder, true);
	TestTrue(TEXT("the reactor starts"), I.Course->StartObjectiveAt(Ids[3]));
	I.W.Advance(0.2f);
	TestTrue(*FString::Printf(TEXT("%s plays as the room starts"), *Entry.ToString()), Story->IsSequencePlaying(Entry));
	Story->SkipSequence();
	I.W.Advance(0.2f);
	// The room's defenders (the drones today): beat them.
	ATDObjective* Reactor = I.Obj(3);
	for (TActorIterator<ATDEncounterVolume> It(I.W.GetWorld()); It; ++It)
	{
		if (It->LinkedObjective != Reactor) { continue; }
		It->Activate();
		for (const TWeakObjectPtr<AActor>& M : It->GetSpawned()) { if (UTDHealthComponent* H = M.IsValid() ? UTDStatics::GetHealth(M.Get()) : nullptr) { H->Kill(P, ETDDamageKind::Melee); } }
		I.W.Advance(0.2f);
		TestTrue(TEXT("the defenders are overcome"), It->IsCleared());
	}
	TestTrue(*FString::Printf(TEXT("%s plays when they fall"), *Over.ToString()), Story->IsSequencePlaying(Over));
	return true;
}

// T7 (Nov 14): a death between rooms ("CUT! Scene None") came back at the hangar start, 104 m from the reactor. In every
// gap between interior rooms the retake spot is the next room's door, ahead of the last room done.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDInteriorRetakeBetweenRooms, "TriadDrift.Ground.Interior.ADeathBetweenRoomsComesBackAtTheNextDoor", TD_TEST_FLAGS)
bool FTDInteriorRetakeBetweenRooms::RunTest(const FString& Parameters)
{
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	float LastX = -1e9f;
	for (int32 Done = 1; Done < Ids.Num(); ++Done)
	{
		TArray<FName> Before;
		for (int32 i = 0; i < Done; ++i) { Before.Add(Ids[i]); }
		TDInteriorTest::FInterior I(Before);
		if (!TestNotNull(TEXT("interior"), I.Course)) { return false; }
		const FVector Spot = I.Course->GetInteriorRetakeSpot(NAME_None).GetLocation();
		const FVector Hangar = TDInteriorTest::FInterior().Course->GetInteriorRetakeSpot(NAME_None).GetLocation();
		AddInfo(FString::Printf(TEXT("after %s: retake at %s"), *Ids[Done - 1].ToString(), *Spot.ToCompactString()));
		TestTrue(*FString::Printf(TEXT("after %s: not the hangar"), *Ids[Done - 1].ToString()), FVector::Dist2D(Spot, Hangar) > 1000.f);
		TestTrue(*FString::Printf(TEXT("after %s: further in than the last gap"), *Ids[Done - 1].ToString()), Spot.X >= LastX);
		LastX = Spot.X;
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
