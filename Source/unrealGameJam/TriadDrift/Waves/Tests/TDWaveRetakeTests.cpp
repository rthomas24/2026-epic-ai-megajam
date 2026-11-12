// Review fixes around retakes: the CUT-delay soft-lock (minor 5) and hyperdrive locks that follow state
// rather than transitions (Travel review MAJOR). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Waves.Retake

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/TDWarpInMarker.h"
#include "EngineUtils.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDWaveRetakeTest
{
	ATDWaveDirector* Director(FTDTestWorld& W)
	{
		ATDWaveDirector* D = W.Spawn<ATDWaveDirector>();
		D->SetSpawnFactory([](UWorld* World, FName, const FTransform& T) -> AActor*
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<ATDWaveTestEnemy>(ATDWaveTestEnemy::StaticClass(), T, P);
		});
		return D;
	}

	void KillAll(ATDWaveDirector* D)
	{
		for (AActor* A : D->GetLivingEnemies()) { if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->Kill(); } }
	}

	FString LockReason(FTDTestWorld& W)
	{
		FText Reason;
		const UTDTravelSubsystem* T = UTDTravelSubsystem::Get(W.GetWorld());
		return (T && T->IsHyperdriveLocked(Reason)) ? Reason.ToString() : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeAfterVoidedClear, "TriadDrift.Waves.Retake.LastKillDuringCutRebuildsTheWave", TD_TEST_FLAGS)
bool FTDRetakeAfterVoidedClear::RunTest(const FString& Parameters)
{
	using namespace TDWaveRetakeTest;
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	ATDWaveDirector* D = Director(W);
	GI->Progress.WavesCleared = 1;
	D->StartWave(1);	// wave 2
	W.Advance(D->WarpInSeconds + 6.f);
	const FName Scene = D->GetWaveInfo().SceneId;

	// The squad died ("CUT!"); during the delay the last enemy dies too, so the wave finishes...
	KillAll(D);
	W.Tick();
	TestTrue(TEXT("wave 2 finished"), D->GetState() != ETDWaveState::Active && D->GetState() != ETDWaveState::WarpingIn);
	// ...and the flow voids that clear because the scene is restarting.
	GI->Progress.WavesCleared = 1;
	UTDEventBus::Get(W.GetWorld())->BroadcastTakeStarted(Scene, 2);
	TestTrue(TEXT("the retake runs wave 2 again"), D->GetState() == ETDWaveState::WarpingIn || D->GetState() == ETDWaveState::Active);
	TestEqual(TEXT("still wave 2"), D->GetWaveInfo().WaveNumber, 2);
	W.Advance(D->WarpInSeconds + 6.f);
	TestTrue(*FString::Printf(TEXT("with enemies alive (%d)"), D->GetLivingEnemies().Num()), D->GetLivingEnemies().Num() > 0);
	TestFalse(TEXT("and the hyperdrive locked"), LockReason(W).IsEmpty());

	// A clear that stands is not replayed by a late take.
	KillAll(D);
	W.Tick();
	const ETDWaveState After = D->GetState();
	UTDEventBus::Get(W.GetWorld())->BroadcastTakeStarted(Scene, 3);	// WavesCleared is now 2 (fake flow counts it)
	TestEqual(TEXT("a standing clear is not replayed"), (int32)D->GetState(), (int32)After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRetakeLocksFollowState, "TriadDrift.Waves.Retake.HyperdriveLocksFollowState", TD_TEST_FLAGS)
bool FTDRetakeLocksFollowState::RunTest(const FString& Parameters)
{
	using namespace TDWaveRetakeTest;
	// Wave: a retake that clears every lock (whatever order the handlers run in) can't open the hyperdrive.
	{
		FTDTestWorld W;
		ATDWaveDirector* D = Director(W);
		D->StartWave(0);
		W.Advance(D->WarpInSeconds + 1.f);
		UTDTravelSubsystem::Get(W.GetWorld())->RemoveHyperdriveLock(TDHyperdriveLocks::Wave);	// a clear-all
		W.Advance(0.3f);
		TestFalse(TEXT("wave lock is back within 0.3 s"), LockReason(W).IsEmpty());
	}
	// Finale: die in the assault, retake (locks cleared), the hyperdrive still refuses with the finale's text.
	{
		FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		ATDDominion* Dom = W.Spawn<ATDDominion>(FVector(100000.f, 0.f, 0.f));
		Dom->MaxLaunchedAlive = 0;
		GI->Progress.bDominionRevealed = true;
		W.Advance(Dom->ProgressPollSeconds + Dom->WarpInSeconds + 0.5f);
		TestTrue(TEXT("finale locked"), LockReason(W).Contains(TEXT("DOMINION")));
		UTDTravelSubsystem::Get(W.GetWorld())->RemoveHyperdriveLock(TDHyperdriveLocks::Finale);	// the retake's clear-all
		UTDEventBus::Get(W.GetWorld())->BroadcastTakeStarted(TEXT("Dominion"), 2);
		TestTrue(TEXT("re-locked by the retake itself"), LockReason(W).Contains(TEXT("DOMINION")));
		UTDTravelSubsystem::Get(W.GetWorld())->RemoveHyperdriveLock(TDHyperdriveLocks::Finale);	// cleared after our handler
		W.Advance(0.3f);
		TestTrue(TEXT("and re-asserted within 0.3 s whatever the order"), LockReason(W).Contains(TEXT("DOMINION")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWaveInfoAfterLastWave, "TriadDrift.Waves.NoWaveInfoAfterTheLastWave", TD_TEST_FLAGS)
bool FTDWaveInfoAfterLastWave::RunTest(const FString& Parameters)
{
	using namespace TDWaveRetakeTest;
	// Lead playtest 3: the HUD read "WAVE 05 / 00 HOSTILES" through the Dominion assault.
	FTDTestWorld W;
	ATDWaveDirector* D = Director(W);
	D->StartWave(D->GetWaveCount() - 1);
	W.Advance(D->WarpInSeconds + 8.f);
	TestTrue(TEXT("wave 5 active"), D->GetWaveInfo().bWaveActive && D->GetWaveInfo().WaveNumber == D->GetWaveCount());
	for (int32 i = 0; i < 20 && D->GetState() != ETDWaveState::AllComplete; ++i) { KillAll(D); W.Advance(1.f); }
	const FTDWaveInfo Info = D->GetWaveInfo();
	TestTrue(TEXT("all complete"), D->GetState() == ETDWaveState::AllComplete);
	TestFalse(TEXT("no wave active"), Info.bWaveActive || Info.bInBreather);
	TestEqual(TEXT("no wave number"), Info.WaveNumber, 0);
	TestEqual(TEXT("nothing to count"), Info.EnemiesAlive + Info.EnemiesTotal, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWarpInTelegraphShows, "TriadDrift.Waves.WarpInTelegraphIsVisible", TD_TEST_FLAGS)
bool FTDWarpInTelegraphShows::RunTest(const FString& Parameters)
{
	using namespace TDWaveRetakeTest;
	// Review minor 8: with no EnemyWarpIn VFX authored, each warp-in point still shows a placeholder ring.
	FTDTestWorld W;
	ATDWaveDirector* D = Director(W);
	D->TravelFX = nullptr;
	D->StartWave(0);
	W.Advance(0.1f);
	TArray<TWeakObjectPtr<ATDWarpInMarker>> First;
	for (TActorIterator<ATDWarpInMarker> It(W.GetWorld()); It; ++It) { First.Add(*It); }
	TestTrue(*FString::Printf(TEXT("a ring at each announced warp-in point (%d)"), First.Num()), First.Num() >= 1);
	W.Advance(D->WarpInSeconds + 0.5f);
	int32 Left = 0;
	for (const TWeakObjectPtr<ATDWarpInMarker>& M : First) { Left += (M.IsValid() && !M->IsActorBeingDestroyed()) ? 1 : 0; }
	TestEqual(TEXT("those rings are gone once their ships arrived"), Left, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
