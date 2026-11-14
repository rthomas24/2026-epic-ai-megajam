// The tutorial's end on the objective line (T7's play, Nov 14). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flow.Objective

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flow/TDObjectiveText.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDObjectiveTutorialWithdrawal, "TriadDrift.Flow.Objective.TutorialWithdrawalThenJumpPoint", TD_TEST_FLAGS)
bool FTDObjectiveTutorialWithdrawal::RunTest(const FString& Parameters)
{
	// Same setup as T4's tutorial tests: wave 2 with the withdrawal forced on and stand-in enemies.
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("game instance"), GI)) { return false; }
	GI->Progress.Phase = ETDGamePhase::Waves1to2;
	AActor* Player = W.Spawn<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Player, TEXT("Root"));
	Player->SetRootComponent(Root);
	Root->RegisterComponent();
	UTDHealthComponent* Health = NewObject<UTDHealthComponent>(Player, TEXT("Health"));
	Health->MaxHull = 100.f;
	Health->MaxShield = 0.f;
	Health->DefaultTeam = ETDTeam::Player;
	Health->RegisterComponent();

	ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
	D->SpawnAnchorOverride = Player;
	D->SetSpawnFactory([](UWorld* World, FName, const FTransform& Transform) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ATDWaveTestEnemy>(ATDWaveTestEnemy::StaticClass(), Transform, Params);
	});
	D->TutorialMode = ETDTutorialMode::ForceOn;
	D->bWaitForLessons = false;	// this test is about the objective text; the ENG-40 lesson gate has its own test
	D->StartWave(1);
	for (float T = 0.f; D->GetPendingGroupCount() > 0 && T < 60.f; T += 1.f / 60.f) { W.Tick(1.f / 60.f); }

	FTDObjectiveLine L;
	TestTrue(TEXT("battle line"), FTDObjectiveText::Compute(W.GetWorld(), GI->Progress, ETDDestination::Galaxy, L));
	TestNotEqual(TEXT("not the withdrawal during the battle"), L.Id, FName(TEXT("Tutorial_Withdraw")));

	for (AActor* E : D->GetLivingEnemies()) { if (UTDHealthComponent* H = UTDStatics::GetHealth(E)) { H->Kill(); } }
	W.Tick();
	TestEqual(TEXT("withdrawal ordered"), (int32)D->GetTutorialPhase(), (int32)ETDTutorialPhase::Withdrawal);
	TestTrue(TEXT("withdrawal line"), FTDObjectiveText::Compute(W.GetWorld(), GI->Progress, ETDDestination::Galaxy, L));
	TestEqual(TEXT("WITHDRAW · FOLLOW THE MARKER"), L.Text.ToString(), FString(TEXT("WITHDRAW · FOLLOW THE MARKER")));

	FVector P, N;
	if (!TestTrue(TEXT("boundary up"), D->GetEscapeBoundary(P, N))) { return false; }
	Player->SetActorLocation(P + N * 2000.f);
	W.Tick();
	TestEqual(TEXT("escaped"), (int32)D->GetTutorialPhase(), (int32)ETDTutorialPhase::Escaped);
	TestTrue(TEXT("jump point line"), FTDObjectiveText::Compute(W.GetWorld(), GI->Progress, ETDDestination::Galaxy, L));
	TestEqual(TEXT("REACH THE JUMP POINT"), L.Text.ToString(), FString(TEXT("REACH THE JUMP POINT")));
	return true;
}

// T4's per-wave objective (126b238d): while wave 5 runs the line is its own ("BREAK THE DOMINION GUARD", placeholder
// words), every other wave keeps "WAVE n · CLEAR THE AREA".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDObjectiveWave5Text, "TriadDrift.Flow.Objective.Wave5UsesItsOwnObjectiveLine", TD_TEST_FLAGS)
bool FTDObjectiveWave5Text::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	if (!TestNotNull(TEXT("game instance"), GI)) { return false; }
	AActor* Player = W.Spawn<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Player, TEXT("Root"));
	Player->SetRootComponent(Root);
	Root->RegisterComponent();
	UTDHealthComponent* Health = NewObject<UTDHealthComponent>(Player, TEXT("Health"));
	Health->MaxHull = 100.f;
	Health->DefaultTeam = ETDTeam::Player;
	Health->RegisterComponent();
	ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
	D->SpawnAnchorOverride = Player;
	D->SetSpawnFactory([](UWorld* World, FName, const FTransform& Transform) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ATDWaveTestEnemy>(ATDWaveTestEnemy::StaticClass(), Transform, Params);
	});
	FTDObjectiveLine L;
	for (const int32 Index : { 2, 4 })
	{
		GI->Progress.Phase = Index == 4 ? ETDGamePhase::Wave5 : ETDGamePhase::Waves3to4;
		D->StartWave(Index);
		for (float T = 0.f; D->GetPendingGroupCount() > 0 && T < 60.f; T += 1.f / 60.f) { W.Tick(1.f / 60.f); }
		if (D->IsAwaitingCombatZone()) { AddInfo(TEXT("waiting for the combat zone; the line is the return line")); continue; }
		TestTrue(*FString::Printf(TEXT("wave %d has a line"), Index + 1), FTDObjectiveText::Compute(W.GetWorld(), GI->Progress, ETDDestination::Galaxy, L));
		const FText Own = D->GetWaveInfo().ObjectiveText;
		if (Index == 4)
		{
			TestFalse(TEXT("wave 5 has its own objective"), Own.IsEmpty());
			TestEqual(TEXT("wave 5's line is its own"), L.Text.ToString(), Own.ToString());
		}
		else
		{
			TestTrue(*FString::Printf(TEXT("wave %d: the generic line (%s)"), Index + 1, *L.Text.ToString()), L.Text.ToString().Contains(TEXT("CLEAR THE AREA")));
		}
		for (AActor* E : D->GetLivingEnemies()) { if (UTDHealthComponent* H = UTDStatics::GetHealth(E)) { H->Kill(); } }
		W.Tick();
	}
	return true;
}

#endif
