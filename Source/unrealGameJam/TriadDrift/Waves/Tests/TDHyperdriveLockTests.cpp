// Hyperdrive locks owned by the wave director ("Wave") and the Dominion ("Finale"): on while fighting,
// off in breathers / planet phases, and never stuck. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Waves.HyperdriveLock

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDHyperLockTest
{
	bool Locked(FTDTestWorld& W)
	{
		FText Reason;
		const UTDTravelSubsystem* T = UTDTravelSubsystem::Get(W.GetWorld());
		return T && T->IsHyperdriveLocked(Reason);
	}

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

	void KillAllEnemies(ATDWaveDirector* D)
	{
		for (AActor* A : D->GetLivingEnemies()) { if (UTDHealthComponent* H = UTDStatics::GetHealth(A)) { H->Kill(); } }
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHyperLockWave, "TriadDrift.Waves.HyperdriveLock.OnlyWhileAWaveRuns", TD_TEST_FLAGS)
bool FTDHyperLockWave::RunTest(const FString& Parameters)
{
	using namespace TDHyperLockTest;
	FTDTestWorld W;
	ATDWaveDirector* D = Director(W);
	TestFalse(TEXT("free before any wave"), Locked(W));

	D->StartWave(0);
	TestTrue(TEXT("locked while the wave warps in / fights"), Locked(W));
	W.Advance(D->WarpInSeconds + 2.5f);
	KillAllEnemies(D);
	W.Tick();
	TestTrue(TEXT("breather"), D->GetState() == ETDWaveState::Breather);
	TestFalse(TEXT("unlocked in the breather"), Locked(W));
	W.Advance(D->BreatherSeconds + 0.2f);
	TestTrue(TEXT("locked again when wave 2 starts"), Locked(W));

	// CUT retake: the retaken wave re-adds the lock (and never stacks a second one).
	UTDEventBus::Get(W.GetWorld())->BroadcastTakeStarted(D->GetWaveInfo().SceneId, 2);
	TestTrue(TEXT("retaken wave locks"), Locked(W));
	W.Advance(D->WarpInSeconds + 5.f);
	KillAllEnemies(D);
	W.Tick();
	TestTrue(TEXT("wave 2 ends the phase"), D->GetState() == ETDWaveState::PhaseComplete);
	TestFalse(TEXT("unlocked for the planet phase after a retake"), Locked(W));

	D->StartWave(2);
	D->StopAndClear();
	TestFalse(TEXT("StopAndClear releases"), Locked(W));

	D->StartWave(3);
	TestTrue(TEXT("locked"), Locked(W));
	D->Destroy();	// travel / world teardown
	W.Tick();
	TestFalse(TEXT("EndPlay releases: never stuck"), Locked(W));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHyperLockFinale, "TriadDrift.Waves.HyperdriveLock.FinaleUntilVictory", TD_TEST_FLAGS)
bool FTDHyperLockFinale::RunTest(const FString& Parameters)
{
	using namespace TDHyperLockTest;
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	ATDDominion* Dom = W.Spawn<ATDDominion>(FVector(100000.f, 0.f, 0.f));
	Dom->MaxLaunchedAlive = 0;
	TestFalse(TEXT("free while the Dominion is hidden"), Locked(W));
	GI->Progress.bDominionRevealed = true;
	W.Advance(Dom->ProgressPollSeconds + 0.1f);
	TestTrue(TEXT("locked from the warp-in"), Locked(W));
	W.Advance(Dom->WarpInSeconds + 0.2f);
	Dom->DebugDropShields();
	TestTrue(TEXT("still locked with the reactor exposed"), Locked(W));
	ATDDominionReactor* R = Dom->GetReactor();
	for (int32 i = 0; i < 3; ++i)
	{
		W.Advance(R->PhaseChangeInvulnerableSeconds + 0.1f);
		R->GetHealth()->ApplyDamage([] { FTDDamageInfo I; I.Amount = 99999.f; I.Kind = ETDDamageKind::Explosive; return I; }());
	}
	TestTrue(TEXT("still locked during the breakup"), Locked(W));
	W.Advance(Dom->BreakupSeconds + 2.5f);
	TestTrue(TEXT("victory"), GI->Progress.Phase == ETDGamePhase::Victory);
	TestFalse(TEXT("unlocked at victory"), Locked(W));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
