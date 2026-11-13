// Triad Drift - retake / soft-lock integration tests (review B1, M3-M7). Owner: Teammate 5.
// Real CUT path: the player dies -> UTDGameInstance cuts -> real-time delay -> OnSceneRestart ->
// UTDGroundSceneSubsystem resets the scene and the ground mode respawns the player.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Ground/TDGroundGameMode.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundScene.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"

namespace TDRetakeTest
{
	inline void Cut(UTDGameInstance* GI, ATDFirstPersonCharacter* Player)
	{
		Player->GetHealth()->Kill(nullptr, ETDDamageKind::Environmental); // -> ReportPlayerDefeated
		GI->TickFlow(UTDFlowSettings::Get()->RetakeDelaySeconds + 0.1f);  // the real-time CUT delay
	}

	inline int32 AliveCrew(UWorld* World)
	{
		int32 N = 0;
		for (TActorIterator<ATDCrewCharacter> It(World); It; ++It)
		{
			N += It->IsDead() ? 0 : 1;
		}
		return N;
	}

	inline ATDObjective* MakeObjective(FTDTestWorld& W, FName Id, int32 Steps)
	{
		ATDObjective* O = W.Spawn<ATDObjective>();
		O->ObjectiveId = Id;
		O->Steps.SetNum(Steps);
		return O;
	}

	inline ATDEncounterVolume* MakeRoom(FTDTestWorld& W, const FVector& At, ATDObjective* Obj, int32 Step, int32 Count = 2)
	{
		ATDEncounterVolume* E = W.Spawn<ATDEncounterVolume>(At);
		E->Count = Count;
		E->RingRadius = 300.f;
		E->LinkedObjective = Obj;
		E->LinkedStep = Step;
		E->Box->SetBoxExtent(FVector(500.f, 500.f, 300.f));
		return E;
	}
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeSandbox, "TriadDrift.Ground.Retake.ConsoleVaultDeathRetakeComplete", TD_TEST_FLAGS)
bool FTDRetakeSandbox::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass(), ATDGroundGameMode::StaticClass());
	W.Advance(0.3f);
	ATDGroundGameMode* GM = W.GetWorld()->GetAuthGameMode<ATDGroundGameMode>();
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	ATDObjective* Obj = GM ? GM->GetSampleObjective() : nullptr;
	ATDEncounterVolume* Vault = GM ? GM->GetEncounter() : nullptr;
	if (!TestNotNull(TEXT("sandbox objective"), Obj) || !TestNotNull(TEXT("vault"), Vault) || !TestNotNull(TEXT("GI"), GI)) { return false; }
	if (GM->GetTurret()) { GM->GetTurret()->bActive = false; } // its bolts would interrupt the hold-interact

	// The player walks up to the console (hold E for 2 s), through the real character + local player.
	ATDFirstPersonCharacter* P = W.Spawn<ATDFirstPersonCharacter>(FVector(-850.f, 300.f, 95.f), FRotator(0.f, 180.f, 0.f));
	APlayerController* PC = W.SpawnLocalPlayerController(P);
	PC->SetControlRotation(FRotator(-25.f, 180.f, 0.f)); // look down at the console
	W.Advance(0.3f);
	auto UseConsole = [&]()
	{
		PC->SetControlRotation(FRotator(-25.f, 180.f, 0.f));
		W.Tick();
		P->BeginInteract();
		W.Advance(2.3f);
		P->EndInteract();
	};
	UseConsole();
	TestEqual(TEXT("console completes step 0"), Obj->GetCurrentStepIndex(), 1);
	const FVector Checkpoint = GM->GetRespawnTransform().GetLocation();
	TestTrue(TEXT("respawn point moved to the console (step checkpoint)"), FVector::Dist2D(Checkpoint, FVector(-850.f, 300.f, 0.f)) < 150.f);

	// Into the vault: the overlap starts the fight.
	P->SetActorLocation(FVector(-2300.f, 0.f, 95.f));
	W.Advance(0.3f);
	TestTrue(TEXT("vault fight started by walking in"), Vault->IsActive());
	TestEqual(TEXT("3 crew in the vault"), TDRetakeTest::AliveCrew(W.GetWorld()), 3);

	// CUT!
	TDRetakeTest::Cut(GI, P);
	W.Advance(0.3f);
	TestFalse(TEXT("player alive after the retake"), P->IsDead());
	TestTrue(TEXT("respawned at the checkpoint, not under the map"), FVector::Dist2D(P->GetActorLocation(), Checkpoint) < 150.f);
	TestEqual(TEXT("objective back at step 0"), Obj->GetCurrentStepIndex(), 0);
	TestFalse(TEXT("vault reset"), Vault->IsActive() || Vault->IsCleared());
	TestEqual(TEXT("vault crew removed"), TDRetakeTest::AliveCrew(W.GetWorld()), 0);

	// No soft-lock: the console works again, the vault fights again, and the objective completes.
	UseConsole();
	TestEqual(TEXT("console usable again after the retake (B1)"), Obj->GetCurrentStepIndex(), 1);
	P->SetActorLocation(FVector(-2300.f, 0.f, 95.f));
	W.Advance(0.3f);
	TestTrue(TEXT("vault arms again"), Vault->IsActive());
	for (TActorIterator<ATDCrewCharacter> It(W.GetWorld()); It; ++It)
	{
		It->GetHealth()->Kill(P, ETDDamageKind::Melee);
	}
	TestTrue(TEXT("vault cleared"), Vault->IsCleared());
	TestTrue(TEXT("objective complete on take 2"), Obj->IsComplete());
	TestEqual(TEXT("take 2 recorded"), GI->GetTakeNumber(TEXT("Obj_Sandbox")), 2);
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeAllRooms, "TriadDrift.Ground.Retake.ResetsEveryUnfinishedRoom", TD_TEST_FLAGS)
bool FTDRetakeAllRooms::RunTest(const FString& Parameters)
{
	TDGroundTest::FNoSaveWrites NoWrites;
	FTDTestWorld W(UTDGameInstance::StaticClass());
	TDGroundTest::SpawnFloor(W);
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	ATDObjective* A = TDRetakeTest::MakeObjective(W, TEXT("Obj_A"), 2);
	ATDObjective* B = TDRetakeTest::MakeObjective(W, TEXT("Obj_B"), 2);
	ATDEncounterVolume* RoomA = TDRetakeTest::MakeRoom(W, FVector(3000.f, 0.f, 150.f), A, 1);
	ATDEncounterVolume* Loose = TDRetakeTest::MakeRoom(W, FVector(-3000.f, 0.f, 150.f), nullptr, INDEX_NONE); // unlinked
	ATDFirstPersonCharacter* P = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f));
	W.SpawnLocalPlayerController(P);
	W.Advance(0.2f);
	GI->NotifyLevelEntered(ETDDestination::Verdance);
	B->StartObjective();
	A->StartObjective(); // A is the current scene
	A->CompleteStep(0);
	RoomA->Activate();
	Loose->Activate();
	B->CompleteStep(0);
	TestEqual(TEXT("both rooms fighting"), TDRetakeTest::AliveCrew(W.GetWorld()), 4);

	// Die inside the unlinked room while A is the current scene.
	P->SetActorLocation(FVector(-3000.f, 0.f, 95.f));
	TDRetakeTest::Cut(GI, P);
	TestFalse(TEXT("A's room reset"), RoomA->IsActive());
	TestFalse(TEXT("the unlinked room reset too (M6)"), Loose->IsActive());
	TestEqual(TEXT("no crew left behind"), TDRetakeTest::AliveCrew(W.GetWorld()), 0);
	TestEqual(TEXT("A back to step 0"), A->GetCurrentStepIndex(), 0);
	TestEqual(TEXT("B (another scene) back to step 0 too"), B->GetCurrentStepIndex(), 0);
	TestEqual(TEXT("A is still the current scene"), GI->GetCurrentScene(), FName(TEXT("Obj_A")));

	// A finished objective keeps its room.
	A->CompleteStep(0);
	RoomA->Activate();
	for (const TWeakObjectPtr<AActor>& M : RoomA->GetSpawned())
	{
		if (M.IsValid()) { UTDStatics::GetHealth(M.Get())->Kill(P, ETDDamageKind::Melee); }
	}
	TestTrue(TEXT("A complete"), A->IsComplete());
	TDRetakeTest::Cut(GI, P);
	TestTrue(TEXT("a completed objective stays complete"), A->IsComplete());
	TestTrue(TEXT("its room stays cleared"), RoomA->IsCleared());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeAlreadyInside, "TriadDrift.Ground.Retake.EncounterStartsForPlayerAlreadyInside", TD_TEST_FLAGS)
bool FTDRetakeAlreadyInside::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDObjective* O = TDRetakeTest::MakeObjective(W, TEXT("Obj_Inside"), 2);
	ATDEncounterVolume* Room = TDRetakeTest::MakeRoom(W, FVector(0.f, 0.f, 150.f), O, 1);
	ATDFirstPersonCharacter* P = W.Spawn<ATDFirstPersonCharacter>(FVector(0.f, 0.f, 95.f)); // standing inside already
	W.SpawnLocalPlayerController(P);
	W.Advance(0.2f);
	TestFalse(TEXT("not before its step"), Room->IsActive());
	O->StartObjective();
	O->CompleteStep(0); // the console was inside the room
	TestTrue(TEXT("starts the moment its step begins, without re-entering (M4)"), Room->IsActive());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeFallOff, "TriadDrift.Ground.Retake.CrewFallingOffTheMapStillClears", TD_TEST_FLAGS)
bool FTDRetakeFallOff::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDEncounterVolume* Room = TDRetakeTest::MakeRoom(W, FVector(0.f, 0.f, 150.f), nullptr, INDEX_NONE, 2);
	W.Advance(0.1f);
	Room->Activate();
	TArray<AActor*> Members;
	for (const TWeakObjectPtr<AActor>& M : Room->GetSpawned()) { Members.Add(M.Get()); }
	if (!TestEqual(TEXT("two crew"), Members.Num(), 2)) { return false; }
	UTDStatics::GetHealth(Members[0])->Kill(nullptr, ETDDamageKind::Melee);
	Members[1]->FellOutOfWorld(*GetDefault<UDamageType>()); // knocked off a ledge
	W.Tick();
	TestTrue(TEXT("room clears when the last one falls off the map (M3)"), Room->IsCleared());

	// And a member simply destroyed (no death at all) also counts.
	ATDEncounterVolume* Room2 = TDRetakeTest::MakeRoom(W, FVector(4000.f, 0.f, 150.f), nullptr, INDEX_NONE, 1);
	W.Tick();
	Room2->Activate();
	AActor* Only = Room2->GetSpawned().Num() ? Room2->GetSpawned()[0].Get() : nullptr;
	if (Only) { Only->Destroy(); }
	TestTrue(TEXT("destroyed member counts as gone"), Room2->IsCleared());
	return true;
}

// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeSummons, "TriadDrift.Ground.Retake.WardenSummonsBelongToTheRoom", TD_TEST_FLAGS)
bool FTDRetakeSummons::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-900.f, 0.f, 95.f));
	ATDEncounterVolume* Room = TDRetakeTest::MakeRoom(W, FVector(0.f, 0.f, 150.f), nullptr, INDEX_NONE, 1);
	Room->CrewMix = { ETDCrewType::Warden };
	W.Advance(0.1f);
	Room->Activate();
	ATDCrewCharacter* Warden = Room->GetSpawned().Num() ? Cast<ATDCrewCharacter>(Room->GetSpawned()[0].Get()) : nullptr;
	if (!TestNotNull(TEXT("warden"), Warden)) { return false; }
	if (AController* AI = Warden->GetController()) { AI->UnPossess(); }
	TestTrue(TEXT("summon"), Warden->StartAttack(ETDCrewAttack::Summon, P));
	W.Advance(Warden->Spec.BurstWindup + 0.1f);
	TestEqual(TEXT("summons join the room (M7)"), Room->GetAliveCount(), 1 + Warden->Spec.SummonCount);
	Warden->GetHealth()->Kill(P, ETDDamageKind::Melee);
	TestEqual(TEXT("summons die with the Warden"), Room->GetAliveCount(), 0);
	TestTrue(TEXT("room cleared"), Room->IsCleared());

	// Reset mid-fight: nothing is left behind.
	Room->ResetEncounter();
	W.Advance(0.1f);
	Room->Activate();
	Warden = Cast<ATDCrewCharacter>(Room->GetSpawned()[0].Get());
	if (AController* AI = Warden->GetController()) { AI->UnPossess(); }
	Warden->StartAttack(ETDCrewAttack::Summon, P);
	W.Advance(Warden->Spec.BurstWindup + 0.1f);
	Room->ResetEncounter();
	W.Advance(0.1f);
	TestEqual(TEXT("no stray grunts after a reset"), TDRetakeTest::AliveCrew(W.GetWorld()), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
