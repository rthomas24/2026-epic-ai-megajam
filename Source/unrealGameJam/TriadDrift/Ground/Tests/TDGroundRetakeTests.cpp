// Triad Drift - retake / soft-lock integration tests (review B1, M3-M7). Owner: Teammate 5.
// Real CUT path: the player dies -> UTDGameInstance cuts -> real-time delay -> OnSceneRestart ->
// UTDGroundSceneSubsystem resets the scene and the ground mode respawns the player.

#include "Misc/AutomationTest.h"
#include "GameFramework/DamageType.h"

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
	Warden->SetOpeningGrace(0.f); // this test is about summons, not the opening
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
	Warden->SetOpeningGrace(0.f);
	Warden->StartAttack(ETDCrewAttack::Summon, P);
	W.Advance(Warden->Spec.BurstWindup + 0.1f);
	Room->ResetEncounter();
	W.Advance(0.1f);
	TestEqual(TEXT("no stray grunts after a reset"), TDRetakeTest::AliveCrew(W.GetWorld()), 0);
	return true;
}

// Director's mercy is a hook on any encounter: no tiers = the same fight every take; with tiers the highest
// reached applies to every member it spawns; a capped Warden stops summoning after its allowance.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeMercyHook, "TriadDrift.Ground.Retake.DirectorsMercyEasesByTake", TD_TEST_FLAGS)
bool FTDRetakeMercyHook::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-900.f, 0.f, 95.f));
	ATDEncounterVolume* Plain = TDRetakeTest::MakeRoom(W, FVector(0.f, 3000.f, 150.f), nullptr, INDEX_NONE, 2);
	ATDEncounterVolume* Room = TDRetakeTest::MakeRoom(W, FVector(0.f, 0.f, 150.f), nullptr, INDEX_NONE, 2);
	Room->CrewMix = { ETDCrewType::Warden, ETDCrewType::Grunt };
	FTDRetakeMercyTier T3; T3.FromTake = 3; T3.HealthScale = 0.75f; T3.MaxSummons = 1;
	FTDRetakeMercyTier T5; T5.FromTake = 5; T5.HealthScale = 0.5f; T5.MaxSummons = 0;
	Room->MercyTiers = { T5, T3 }; // order doesn't matter
	W.Advance(0.1f);
	const float WardenHp = FTDCrewLibrary::Get(ETDCrewType::Warden).MaxHealth;
	const float GruntHp = FTDCrewLibrary::Get(ETDCrewType::Grunt).MaxHealth;
	for (int32 Take = 1; Take <= 5; ++Take)
	{
		Plain->Activate();
		Room->Activate();
		const float Scale = Take >= 5 ? 0.5f : Take >= 3 ? 0.75f : 1.f;
		TestEqual(*FString::Printf(TEXT("take %d counted"), Take), Room->GetTake(), Take);
		TestTrue(*FString::Printf(TEXT("take %d: mercy only from take 3"), Take), (Room->GetActiveMercy() != nullptr) == (Take >= 3));
		TestNull(TEXT("no tiers, never eased"), Plain->GetActiveMercy());
		for (const TWeakObjectPtr<AActor>& M : Room->GetSpawned())
		{
			const ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get());
			if (!C) { continue; }
			const float Full = C->CrewType == ETDCrewType::Warden ? WardenHp : GruntHp;
			TestEqual(*FString::Printf(TEXT("take %d: %s health"), Take, *UEnum::GetValueAsString(C->CrewType)), C->GetHealth()->MaxHull, Full * Scale, 0.5f);
		}
		for (const TWeakObjectPtr<AActor>& M : Plain->GetSpawned())
		{
			if (const ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get())) { TestEqual(TEXT("plain room full health"), C->GetHealth()->MaxHull, GruntHp, 0.5f); }
		}
		if (Take == 3)
		{
			ATDCrewCharacter* Warden = Cast<ATDCrewCharacter>(Room->GetSpawned()[0].Get());
			if (AController* AI = Warden->GetController()) { AI->UnPossess(); }
			Warden->SetOpeningGrace(0.f);
			TestTrue(TEXT("take 3: one summon"), Warden->StartAttack(ETDCrewAttack::Summon, P));
			W.Advance(Warden->Spec.BurstWindup + 1.f);
			for (const TWeakObjectPtr<AActor>& M : Room->GetSpawned())
			{
				if (ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get()); C && C != Warden && C->CrewType == ETDCrewType::Grunt && C->GetHealth()) { C->GetHealth()->Kill(P, ETDDamageKind::Melee); }
			}
			W.Advance(Warden->GetCooldownLeft(ETDCrewAttack::Summon) + 0.5f);
			TestEqual(TEXT("take 3: allowance spent"), Warden->GetSummonsLeft(), 0);
			TestFalse(TEXT("take 3: no second summon"), Warden->StartAttack(ETDCrewAttack::Summon, P));
		}
		Plain->ResetEncounter();
		Room->ResetEncounter();
		W.Advance(0.1f);
	}
	return true;
}

// An encounter's crew show themselves before they hurt (opening grace), and a DropCrew tier fields fewer of them
// on later takes (the Verdance wreck ambush: take 3 one grunt fewer, take 5 the trooper alone).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeOpeningGrace, "TriadDrift.Ground.Retake.OpeningGraceAndFewerCrew", TD_TEST_FLAGS)
bool FTDRetakeOpeningGrace::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-1500.f, 0.f, 95.f)); // outside the box: Activate() is the only trigger
	ATDEncounterVolume* Room = TDRetakeTest::MakeRoom(W, FVector(0.f, 0.f, 150.f), nullptr, INDEX_NONE, 3);
	Room->CrewMix = { ETDCrewType::Trooper, ETDCrewType::Grunt, ETDCrewType::Grunt };
	FTDRetakeMercyTier T3; T3.FromTake = 3; T3.DropCrew = 1;
	FTDRetakeMercyTier T5; T5.FromTake = 5; T5.DropCrew = 2;
	Room->MercyTiers = { T3, T5 };
	W.Advance(0.1f);
	const int32 Expect[] = { 3, 3, 2, 2, 1, 1 };
	for (int32 Take = 1; Take <= 6; ++Take)
	{
		Room->Activate();
		TestEqual(*FString::Printf(TEXT("take %d crew"), Take), Room->GetSpawned().Num(), Expect[Take - 1]);
		ATDCrewCharacter* Trooper = nullptr;
		for (const TWeakObjectPtr<AActor>& M : Room->GetSpawned()) { if (ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get()); C && C->CrewType == ETDCrewType::Trooper) { Trooper = C; } }
		if (!TestNotNull(*FString::Printf(TEXT("take %d: the trooper always fights"), Take), Trooper)) { return false; }
		if (Take == 1)
		{
			if (AController* AI = Trooper->GetController()) { AI->UnPossess(); }
			TestFalse(TEXT("no shot in the opening grace"), Trooper->StartAttack(ETDCrewAttack::Burst, P));
			W.Advance(Room->OpeningGraceSeconds * 0.5f);
			TestFalse(TEXT("still holding fire half-way"), Trooper->StartAttack(ETDCrewAttack::Burst, P));
			W.Advance(Room->OpeningGraceSeconds * 0.5f + 0.1f);
			TestTrue(TEXT("fires once the grace is over"), Trooper->StartAttack(ETDCrewAttack::Burst, P));
		}
		Room->ResetEncounter();
		W.Advance(0.1f);
	}
	return true;
}

// T7 (Nov 14, with T3's lead): after a CUT the Warden's room came back cleared and the scene complete, no Warden.
// A boss destroyed while alive (a fall, kill-Z) is put back, so its room can only clear by killing it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeLostWarden, "TriadDrift.Ground.Retake.AWardenThatLeavesTheWorldAliveIsPutBack", TD_TEST_FLAGS)
bool FTDRetakeLostWarden::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	TDGroundTest::SpawnFloor(W);
	ATDFirstPersonCharacter* P = TDGroundTest::SpawnPlayer(W, FVector(-900.f, 0.f, 95.f));
	ATDObjective* O = TDRetakeTest::MakeObjective(W, TEXT("Obj_Gate"), 2);
	ATDEncounterVolume* Room = TDRetakeTest::MakeRoom(W, FVector(0.f, 0.f, 150.f), O, 0, 1);
	Room->CrewMix = { ETDCrewType::Warden };
	O->StartObjective();
	W.Advance(0.1f);
	Room->Activate();
	auto WardenOf = [Room]() -> ATDCrewCharacter*
	{
		for (const TWeakObjectPtr<AActor>& M : Room->GetSpawned())
		{
			ATDCrewCharacter* C = Cast<ATDCrewCharacter>(M.Get());
			if (C && !C->IsActorBeingDestroyed() && C->Spec.Type == ETDCrewType::Warden && !C->IsDead()) { return C; }
		}
		return nullptr;
	};
	for (int32 i = 0; i < ATDEncounterVolume::MaxBossReturns; ++i)
	{
		ATDCrewCharacter* Warden = WardenOf();
		if (!TestNotNull(*FString::Printf(TEXT("a Warden (%d)"), i), Warden)) { return false; }
		if (i % 2 == 0) { Warden->FellOutOfWorld(*GetDefault<UDamageType>()); } // through the kill-Z (T3's path)
		else { Warden->Destroy(); } // removed alive
		W.Advance(0.1f);
		TestFalse(*FString::Printf(TEXT("the room is not cleared (%d)"), i), Room->IsCleared());
		TestEqual(*FString::Printf(TEXT("the objective stays on step 0 (%d)"), i), O->GetCurrentStepIndex(), 0);
		ATDCrewCharacter* Back = WardenOf();
		if (TestNotNull(*FString::Printf(TEXT("put back (%d)"), i), Back))
		{
			TestEqual(TEXT("at full health"), Back->GetHealth()->GetHull(), Back->GetHealth()->MaxHull);
		}
	}
	// Killing it clears the room as ever.
	if (ATDCrewCharacter* Warden = WardenOf()) { Warden->GetHealth()->Kill(P, ETDDamageKind::Melee); }
	W.Advance(0.1f);
	TestTrue(TEXT("killed: the room clears"), Room->IsCleared());
	TestEqual(TEXT("killed: the step completes"), O->GetCurrentStepIndex(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
