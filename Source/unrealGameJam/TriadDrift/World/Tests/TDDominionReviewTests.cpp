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

		GI->Progress.bDominionRevealed = true;	// wave 5 cleared: the Dominion warps in
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

#endif // WITH_DEV_AUTOMATION_TESTS
