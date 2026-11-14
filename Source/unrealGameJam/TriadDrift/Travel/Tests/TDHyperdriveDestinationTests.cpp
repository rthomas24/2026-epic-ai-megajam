// One jump to a marked destination (Ryan, Nov 13). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Hyperdrive.Destination

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"

namespace TDOneJumpTest
{
	struct FJump { bool bDest = false; ETDHyperdriveEndCause Cause = ETDHyperdriveEndCause::None; FName Well = NAME_None; float JumpSeconds = 0.f; float EndDistToWell = 0.f; };

	/** From Start, nose at Aim, one H; returns how the jump ended. Verdance and Cinder wells as in the galaxy (~1.3 km). */
	FJump Jump(const FVector& Start, const FVector& Aim, float Fps)
	{
		FJump J;
		FTDTestWorld W;
		UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
		if (!Travel) { return J; }
		Travel->SetPlayBounds(FVector::ZeroVector, 900000.f);
		const FVector Verdance(600000.f, 300000.f, 0.f), Cinder(-500000.f, -600000.f, 0.f);
		Travel->RegisterGravityWell(TEXT("Verdance"), Verdance, 130000.f);
		Travel->RegisterGravityWell(TEXT("Cinder"), Cinder, 120000.f);
		ATDShip* Ship = W.Spawn<ATDShip>(Start);
		if (!Ship) { return J; }
		Ship->SetPlayerControlled(true);
		Ship->TeleportSpacecraft(Start, (Aim - Start).Rotation(), false);
		W.Tick(1.f / Fps);
		UTDHyperdriveComponent* H = Ship->GetHyperdrive();
		if (!H || !H->StartSpool()) { return J; }
		float Jumping = 0.f;
		for (float T = 0.f; T < 20.f; T += 1.f / Fps)
		{
			W.Tick(1.f / Fps);
			if (H->GetState() == ETDHyperdriveState::Jumping) { Jumping += 1.f / Fps; }
			if (H->GetState() == ETDHyperdriveState::Idle && Jumping > 0.f) { break; }
		}
		J.bDest = H->HasDestination();
		J.Cause = H->GetLastEndCause();
		J.Well = H->GetLastDropOutWell();
		J.JumpSeconds = Jumping;
		J.EndDistToWell = FMath::Min(FVector::Dist(Ship->GetActorLocation(), Verdance) - 130000.f, FVector::Dist(Ship->GetActorLocation(), Cinder) - 120000.f);
		return J;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDOneJump, "TriadDrift.Travel.Hyperdrive.DestinationOneJumpFromTheFarEdge", TD_TEST_FLAGS)
bool FTDOneJump::RunTest(const FString& Parameters)
{
	using namespace TDOneJumpTest;
	const FVector Verdance(600000.f, 300000.f, 0.f), Cinder(-500000.f, -600000.f, 0.f);
	for (const float Fps : { 30.f, 120.f })
	{
		// The far edge of the play area, each planet roughly ahead (within the 20 deg cone).
		const FVector EdgeW(-850000.f, 0.f, 0.f), EdgeN(0.f, 850000.f, 0.f);
		const FJump V = Jump(EdgeW, Verdance + FVector(0.f, 150000.f, 0.f), Fps); // ~6 deg off
		const FJump C = Jump(EdgeN, Cinder, Fps);
		AddInfo(FString::Printf(TEXT("@%.0f fps: to Verdance %.2f s (%s), to Cinder %.2f s (%s)"), Fps, V.JumpSeconds, *V.Well.ToString(), C.JumpSeconds, *C.Well.ToString()));
		for (const TPair<const FJump*, const TCHAR*>& P : { TPair<const FJump*, const TCHAR*>(&V, TEXT("Verdance")), TPair<const FJump*, const TCHAR*>(&C, TEXT("Cinder")) })
		{
			const FString Tag = FString::Printf(TEXT("@%.0f fps, to %s: "), Fps, P.Value);
			TestTrue(*(Tag + TEXT("the planet ahead is the jump's destination")), P.Key->bDest);
			TestEqual(*(Tag + TEXT("one jump ends in its gravity well")), P.Key->Well, FName(P.Value));
			TestTrue(*(Tag + TEXT("... by the well, not a timeout")), P.Key->Cause == ETDHyperdriveEndCause::GravityWell);
			TestTrue(*(Tag + FString::Printf(TEXT("the tunnel lasts at most ~4 s (%.2f)"), P.Key->JumpSeconds)), P.Key->JumpSeconds <= 4.6f);
			TestTrue(*(Tag + FString::Printf(TEXT("drops out at the well's edge (%.0f m off)"), P.Key->EndDistToWell / 100.f)), P.Key->EndDistToWell < 60000.f);
		}
		// Nothing ahead: a free jump, as before (the time-limited tunnel at JumpSpeed).
		const FJump Free = Jump(FVector::ZeroVector, FVector(0.f, -900000.f, 0.f), Fps); // Cinder is ~40 deg off: outside the cone
		TestFalse(*FString::Printf(TEXT("@%.0f fps: nothing in the cone: a free jump"), Fps), Free.bDest);
	}
	return true;
}

#endif
