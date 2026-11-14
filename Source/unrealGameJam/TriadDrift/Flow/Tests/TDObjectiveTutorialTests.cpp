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

#endif
