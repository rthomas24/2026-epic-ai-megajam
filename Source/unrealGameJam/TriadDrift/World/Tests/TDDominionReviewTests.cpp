// Second World review (ENG-18): the finale after a jump in, a victory and a retake, with the real travel
// subsystem and real ships. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Finale

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Flight/TDSpacecraft.h"

namespace TDFinaleReview
{
	FString LockReason(FTDTestWorld& W)
	{
		FText Reason;
		const UTDTravelSubsystem* T = UTDTravelSubsystem::Get(W.GetWorld());
		return (T && T->IsHyperdriveLocked(Reason)) ? Reason.ToString() : FString();
	}
}

// MAJOR 2: jump from 500 000 cm, drop out at the Dominion's well, then the hyperdrive is refused.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFinaleLockAtTheWell, "TriadDrift.World.Finale.HyperdriveLocksOnlyAfterArrival", TD_TEST_FLAGS)
bool FTDFinaleLockAtTheWell::RunTest(const FString& Parameters)
{
	using namespace TDFinaleReview;
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		const FVector DomAt(500000.f, 0.f, 0.f);
		ATDDominion* Dom = W.Spawn<ATDDominion>(DomAt);
		Dom->MaxLaunchedAlive = 0;
		UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
		Travel->RegisterGravityWell(TEXT("Dominion"), DomAt, Dom->HyperdriveLockRadius, Dom);	// as the galaxy layout does

		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		Ship->SetPlayerControlled(true);
		FTDFlightInput In;
		In.Throttle = 1.f;
		Ship->SetFlightInput(In);
		if (UTDHealthComponent* H = Ship->GetHealth()) { H->SetInvulnerable(true); }

		GI->Progress.bDominionRevealed = true; GI->Progress.Phase = ETDGamePhase::DominionAssault;	// wave 5 cleared: the Dominion warps in
		W.Advance(Dom->ProgressPollSeconds + Dom->WarpInSeconds + 0.5f, 1.f / Fps);
		TestEqual(*FString::Printf(TEXT("@%.0f: shields up"), Fps), (int32)Dom->GetState(), (int32)ETDDominionState::ShieldsUp);
		TestTrue(*FString::Printf(TEXT("@%.0f: free to jump while 500 000 cm away"), Fps), LockReason(W).IsEmpty());

		UTDHyperdriveComponent* Drive = Ship->GetHyperdrive();
		TestTrue(*FString::Printf(TEXT("@%.0f: spool"), Fps), Drive && Drive->StartSpool());
		bool bJumped = false;
		for (int32 i = 0; i < FMath::RoundToInt(Fps * 30.f); ++i)
		{
			W.Tick(1.f / Fps);
			bJumped |= Drive->IsJumping();
			if (bJumped && Drive->GetState() == ETDHyperdriveState::Idle) { break; }
		}
		const float Dist = FVector::Dist(Ship->GetActorLocation(), DomAt);
		TestTrue(*FString::Printf(TEXT("@%.0f: jumped and dropped out at the well (%.0f cm from the Dominion)"), Fps, Dist), bJumped && Dist <= Dom->HyperdriveLockRadius + 5000.f);
		W.Advance(0.5f, 1.f / Fps);
		TestTrue(*FString::Printf(TEXT("@%.0f: then locked with the finale text (%s)"), Fps, *LockReason(W)), LockReason(W).Contains(TEXT("DOMINION")));
		TestFalse(*FString::Printf(TEXT("@%.0f: spool refused"), Fps), Drive->StartSpool());
	}
	return true;
}

namespace TDFinaleReview
{
	struct FAssault
	{
		FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
		UTDWaveTestGameInstance* GI = nullptr;
		ATDDominion* Dom = nullptr;
		AActor* Player = nullptr;

		FAssault()
		{
			GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
			Dom = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
			Dom->MaxLaunchedAlive = 8;
			ATDSpacecraft* Ship = W.Spawn<ATDSpacecraft>(FVector(150000.f, 0.f, 0.f));	// arrived, inside the well
			Ship->SetTeam(ETDTeam::Player);
			if (UTDHealthComponent* H = Ship->GetHealth()) { H->DefaultTeam = ETDTeam::Player; H->InitMaxValues(1e6f, 0.f, true); H->SetInvulnerable(true); }
			Player = Ship;
			GI->Progress.bDominionRevealed = true;
			GI->Progress.Phase = ETDGamePhase::DominionAssault;
			W.Advance(Dom->ProgressPollSeconds + Dom->WarpInSeconds + 0.5f);
		}
		float Hit(AActor* Target, float Amount)
		{
			return UTDStatics::ApplyTDDamage(Target, Amount, ETDDamageKind::Explosive, Player, Player);
		}
	};
}

// MAJOR 3: in the Victory phase the Dominion is gone: no weapons, no launches, no lock.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFinaleVictoryPhase, "TriadDrift.World.Finale.NoFightUnderTheVictoryPhase", TD_TEST_FLAGS)
bool FTDFinaleVictoryPhase::RunTest(const FString& Parameters)
{
	using namespace TDFinaleReview;
	// A won save loaded (or TD.Stage Victory) while the assault is running.
	{
		FAssault A;
		TestEqual(TEXT("assault running"), (int32)A.Dom->GetState(), (int32)ETDDominionState::ShieldsUp);
		TestTrue(TEXT("locked during the fight"), LockReason(A.W).Contains(TEXT("DOMINION")));
		A.GI->Progress.Phase = ETDGamePhase::Victory;
		A.GI->Progress.bDominionDefensesDown = true;
		A.W.Advance(A.Dom->ProgressPollSeconds + 0.5f);
		TestEqual(TEXT("finished"), (int32)A.Dom->GetState(), (int32)ETDDominionState::Finished);
		TestTrue(TEXT("no lock"), LockReason(A.W).IsEmpty());
		TestTrue(TEXT("reactor gone"), !A.Dom->GetReactor() || A.Dom->GetReactor()->IsHidden());
		for (ATDEnemyTurret* T : A.Dom->GetTurrets()) { TestTrue(TEXT("turret silent"), !T || !T->IsActorTickEnabled()); }
		const int32 Before = A.Dom->GetLaunchedAlive();
		A.W.Advance(20.f);
		TestEqual(TEXT("no launches"), A.Dom->GetLaunchedAlive(), 0);
		(void)Before;
	}
	// Level entry with a won save: never shows up.
	{
		FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true;
		GI->Progress.bDominionDefensesDown = true;
		GI->Progress.Phase = ETDGamePhase::Victory;
		ATDDominion* Dom = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
		W.Advance(Dom->ProgressPollSeconds + 0.5f);
		TestEqual(TEXT("a won save: finished"), (int32)Dom->GetState(), (int32)ETDDominionState::Finished);
		TestTrue(TEXT("a won save: no lock"), LockReason(W).IsEmpty());
	}
	// Minor 4: the reveal flag alone (a voided wave-5 clear) doesn't start the finale.
	{
		FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true;
		GI->Progress.Phase = ETDGamePhase::Wave5;
		ATDDominion* Dom = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
		W.Advance(Dom->ProgressPollSeconds + 0.5f);
		TestEqual(TEXT("wave 5 replayed: still hidden"), (int32)Dom->GetState(), (int32)ETDDominionState::Hidden);
	}
	return true;
}

// MAJOR 1 (Dominion half): what a new take of the "Dominion" scene resets.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFinaleRetakeReset, "TriadDrift.World.Finale.RetakeResetsTheRightThings", TD_TEST_FLAGS)
bool FTDFinaleRetakeReset::RunTest(const FString& Parameters)
{
	using namespace TDFinaleReview;
	FAssault A;
	TArray<ATDShieldGenerator*> Gens = A.Dom->GetGenerators();
	if (!TestEqual(TEXT("four generators"), Gens.Num(), 4)) { return false; }
	A.Hit(Gens[0], 1e6f);
	A.Hit(Gens[1], 1e6f);
	A.Hit(Gens[2], 50.f);	// damaged, alive
	A.W.Advance(20.f);	// fighters launched
	TestTrue(TEXT("fighters out"), A.Dom->GetLaunchedAlive() > 0);
	const float GenMax = UTDStatics::GetHealth(Gens[3])->GetHull();
	UTDEventBus::Get(A.W.GetWorld())->BroadcastTakeStarted(TEXT("Dominion"), 2);
	A.W.Tick();
	TestEqual(TEXT("destroyed generators stay destroyed"), A.Dom->GetGeneratorsRemaining(), 2);
	TestEqual(TEXT("a damaged generator is back to full"), UTDStatics::GetHealth(Gens[2])->GetHull(), GenMax, 1.f);
	TestEqual(TEXT("launched fighters cleared"), A.Dom->GetLaunchedAlive(), 0);
	TestTrue(TEXT("still locked"), LockReason(A.W).Contains(TEXT("DOMINION")));

	// Reactor: phase 1 passed, phase 2 damaged; a retake restarts phase 2 at full, phase 1 stays passed.
	A.Hit(Gens[2], 1e6f);
	A.Hit(Gens[3], 1e6f);
	A.W.Advance(0.5f);
	ATDDominionReactor* R = A.Dom->GetReactor();
	if (!TestNotNull(TEXT("reactor"), R) || !TestTrue(TEXT("exposed"), R->IsExposed())) { return false; }
	A.Hit(R, R->PhaseHull + 10.f);
	A.W.Advance(R->PhaseChangeInvulnerableSeconds + 0.2f);
	TestEqual(TEXT("phase 2"), R->GetPhase(), 2);
	A.Hit(R, 500.f);
	UTDEventBus::Get(A.W.GetWorld())->BroadcastTakeStarted(TEXT("Dominion"), 3);
	A.W.Tick();
	TestEqual(TEXT("still phase 2"), R->GetPhase(), 2);
	TestEqual(TEXT("phase 2 back at full"), R->GetHealth()->GetHull(), R->PhaseHull * (R->PhaseCount - 1), 1.f);
	TestEqual(TEXT("still exposed: generators stay down"), (int32)A.Dom->GetState(), (int32)ETDDominionState::ReactorExposed);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
