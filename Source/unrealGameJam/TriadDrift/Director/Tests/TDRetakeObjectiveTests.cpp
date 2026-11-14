// Triad Drift - scenes and takes (C-03 #3): a retake never moves the objective on. Owner: Teammate 3.
// T7's play (main 50c4af7a): dying in the Warden fight came back at BREAK THE REACTOR CONDUITS with the Warden gone.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeKeepsTheObjective, "TriadDrift.Director.Takes.ARetakeNeverAdvancesTheObjective", TD_TEST_FLAGS)
bool FTDRetakeKeepsTheObjective::RunTest(const FString& Parameters)
{
	// For every scene of the Dominion interior (the course with the reported fault): start it, die in it, take 2. The
	// scene, its objective and the objective line are the same as before the death; nothing after it has started; a boss
	// that had not died is back.
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	for (int32 Index = 0; Index < Ids.Num(); ++Index)
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!TestNotNull(TEXT("GI"), GI))
		{
			return false;
		}
		GI->OpenLevelOverride = [](const FString&) {};
		GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationTest"));
		GI->NotifyLevelEntered(ETDDestination::DominionInterior);
		GI->GetMutableProgress().Phase = ETDGamePhase::DominionInterior;
		for (int32 i = 0; i < Index; ++i) { GI->GetMutableProgress().CompletedObjectives.Add(Ids[i]); }
		TDGroundTest::SpawnFloor(W);
		FActorSpawnParameters Params;
		Params.bDeferConstruction = true;
		ATDPlanetCourse* Course = W.GetWorld()->SpawnActor<ATDPlanetCourse>(ATDPlanetCourse::StaticClass(), FTransform::Identity, Params);
		if (!TestNotNull(TEXT("course"), Course))
		{
			return false;
		}
		Course->PlanetId = TEXT("DominionInterior");
		Course->bInteriorTravel = false;
		Course->FinishSpawning(FTransform::Identity);
		W.Advance(0.2f);
		ATDObjective* Obj = Course->FindObjective(Ids[Index]);
		if (!TestNotNull(TEXT("objective"), Obj) || !TestTrue(FString::Printf(TEXT("%s starts"), *Ids[Index].ToString()), Course->StartObjectiveAt(Ids[Index])))
		{
			continue;
		}
		ATDFirstPersonCharacter* Player = TDGroundTest::SpawnPlayer(W, Course->GetInteriorStart().GetLocation() + FVector(0.f, 0.f, 95.f));
		W.Advance(0.5f);
		// Into the scene's fight, as T7 was: the room's encounter arms around the player.
		for (TActorIterator<ATDEncounterVolume> It(W.GetWorld()); It && Player; ++It)
		{
			if (It->LinkedObjective == Obj)
			{
				Player->SetActorLocation(It->GetActorLocation() + FVector(0.f, 0.f, 95.f - It->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
				break;
			}
		}
		W.Advance(3.f);
		const FName SceneBefore = GI->GetCurrentScene();
		const int32 StepBefore = Obj->GetCurrentStepIndex();
		int32 BossesBefore = 0;
		for (TActorIterator<ATDCrewCharacter> It(W.GetWorld()); It; ++It) { BossesBefore += (!It->IsDead() && It->CrewType == ETDCrewType::Warden) ? 1 : 0; }
		if (!Player)
		{
			continue;
		}
		AddInfo(FString::Printf(TEXT("%s: scene %s, step %d, Wardens alive %d, crew alive %d"), *Ids[Index].ToString(), *SceneBefore.ToString(), StepBefore, BossesBefore,
			[&W]() { int32 N = 0; for (TActorIterator<ATDCrewCharacter> It(W.GetWorld()); It; ++It) { N += It->IsDead() ? 0 : 1; } return N; }()));
		Player->GetHealth()->Kill(nullptr, ETDDamageKind::Environmental); // the real CUT path
		GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
		W.Advance(1.f);
		const FString Name = Ids[Index].ToString();
		TestEqual(Name + TEXT(": the same scene after the retake"), GI->GetCurrentScene(), SceneBefore);
		TestFalse(Name + TEXT(": not completed by the death"), Obj->IsComplete());
		TestTrue(Name + TEXT(": running again"), Obj->IsActive());
		TestTrue(Name + TEXT(": its step is not ahead of where it was"), Obj->GetCurrentStepIndex() <= StepBefore);
		for (int32 j = Index + 1; j < Ids.Num(); ++j)
		{
			const ATDObjective* Later = Course->FindObjective(Ids[j]);
			TestFalse(FString::Printf(TEXT("%s: %s has not started"), *Name, *Ids[j].ToString()), Later && (Later->IsActive() || Later->IsComplete()));
		}
		int32 BossesAfter = 0;
		for (TActorIterator<ATDCrewCharacter> It(W.GetWorld()); It; ++It) { BossesAfter += (!It->IsDead() && It->CrewType == ETDCrewType::Warden) ? 1 : 0; }
		TestTrue(Name + TEXT(": a boss that was alive is alive again (or waits for the room)"), BossesAfter >= BossesBefore || BossesBefore == 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeWardenLeavesAlive, "TriadDrift.Director.Takes.AWardenLostAliveAfterTheCutNeverClearsItsScene", TD_TEST_FLAGS)
bool FTDRetakeWardenLeavesAlive::RunTest(const FString& Parameters)
{
	// T7's Warden skip, T5's cause: a Warden that leaves the world alive just after the CUT read as "room cleared" and
	// completed the scene. Now the scene stays the Warden's and a Warden stands again.
	const TArray<FName>& Ids = ATDPlanetCourse::InteriorObjectiveIds();
	const int32 Index = Ids.IndexOfByKey(FName(TEXT("Obj_DI_Warden")));
	if (!TestTrue(TEXT("the Warden scene exists"), Index != INDEX_NONE))
	{
		return false;
	}
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("GI"), GI)) { return false; }
	GI->OpenLevelOverride = [](const FString&) {};
	GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationTest"));
	GI->NotifyLevelEntered(ETDDestination::DominionInterior);
	GI->GetMutableProgress().Phase = ETDGamePhase::DominionInterior;
	for (int32 i = 0; i < Index; ++i) { GI->GetMutableProgress().CompletedObjectives.Add(Ids[i]); }
	TDGroundTest::SpawnFloor(W);
	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;
	ATDPlanetCourse* Course = W.GetWorld()->SpawnActor<ATDPlanetCourse>(ATDPlanetCourse::StaticClass(), FTransform::Identity, Params);
	if (!TestNotNull(TEXT("course"), Course)) { return false; }
	Course->PlanetId = TEXT("DominionInterior");
	Course->bInteriorTravel = false;
	Course->FinishSpawning(FTransform::Identity);
	W.Advance(0.2f);
	ATDObjective* Obj = Course->FindObjective(Ids[Index]);
	if (!TestNotNull(TEXT("Warden objective"), Obj) || !TestTrue(TEXT("starts"), Course->StartObjectiveAt(Ids[Index]))) { return false; }
	ATDFirstPersonCharacter* Player = TDGroundTest::SpawnPlayer(W, Course->GetInteriorStart().GetLocation() + FVector(0.f, 0.f, 95.f));
	for (TActorIterator<ATDEncounterVolume> It(W.GetWorld()); It && Player; ++It)
	{
		if (It->LinkedObjective == Obj)
		{
			Player->SetActorLocation(It->GetActorLocation() + FVector(0.f, 0.f, 95.f - It->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
			break;
		}
	}
	W.Advance(3.f);
	if (!TestNotNull(TEXT("player"), Player)) { return false; }
	Player->GetHealth()->Kill(nullptr, ETDDamageKind::Environmental);
	GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);
	W.Advance(0.5f);
	// The take-2 Warden leaves the world alive (a fall, a bad spawn spot).
	// Snapshot first: a returned Warden must not be dropped again by the same loop (one fall, not four).
	TArray<ATDCrewCharacter*> Wardens;
	for (TActorIterator<ATDCrewCharacter> It(W.GetWorld()); It; ++It)
	{
		if (!It->IsDead() && It->CrewType == ETDCrewType::Warden) { Wardens.Add(*It); }
	}
	const int32 Lost = Wardens.Num();
	for (ATDCrewCharacter* C : Wardens) { C->FellOutOfWorld(*GetDefault<UDamageType>()); } // kill-Z, as on a real map
	AddInfo(FString::Printf(TEXT("Wardens removed alive after the CUT: %d"), Lost));
	W.Advance(2.f);
	const ATDObjective* Core = Course->FindObjective(TEXT("Obj_DI_Core"));
	int32 Alive = 0;
	for (TActorIterator<ATDCrewCharacter> It(W.GetWorld()); It; ++It) { Alive += (!It->IsDead() && It->CrewType == ETDCrewType::Warden) ? 1 : 0; }
	TestEqual(TEXT("still the Warden's scene"), GI->GetCurrentScene(), FName(TEXT("Obj_DI_Warden")));
	TestFalse(TEXT("the Warden objective is not complete"), Obj->IsComplete());
	TestFalse(TEXT("the reactor has not started"), Core && (Core->IsActive() || Core->IsComplete()));
	TestTrue(TEXT("a Warden stands again"), Alive >= 1);
	return true;
}

#endif
