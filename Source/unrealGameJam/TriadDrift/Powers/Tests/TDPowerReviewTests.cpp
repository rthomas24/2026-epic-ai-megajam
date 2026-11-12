// Re-check of the second Powers review (P2): what the fixes promised, tested directly. Teammate 2.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.Review

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/StrongObjectPtr.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"

namespace TDPowerReviewTest
{
	const TCHAR* const EngineVFX = TEXT("/Niagara/DefaultAssets/DefaultSystem.DefaultSystem");

}

/** With Power2Active art the dome builds no placeholder lattice, and its art rides the dome (which lags Bastion). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerReviewDomeArt, "TriadDrift.Powers.Review.DomeArtSkipsLatticeAndRidesTheDome", TD_TEST_FLAGS)
bool FTDPowerReviewDomeArt::RunTest(const FString& Parameters)
{
	for (const bool bArt : { false, true })
	{
		const FString Tag = bArt ? TEXT(" (with art)") : TEXT(" (no art)");
		FTDTestWorld W;
		AActor* Bastion = TDPowerTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f));
		UTDShieldDomePower* Dome = TDPowerTest::AddPower<UTDShieldDomePower>(Bastion);
		const TStrongObjectPtr<UTDShipVisuals> Visuals(NewObject<UTDShipVisuals>());
		if (bArt)
		{
			Visuals->Power2Active.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TDPowerReviewTest::EngineVFX));
		}
		Dome->ShipVisuals = Visuals.Get();
		Dome->Slot = ETDPowerInput::Signature2;
		TestTrue(TEXT("dome up") + Tag, Dome->TryActivate());
		const ATDShieldDome* Shell = Dome->GetDome();
		if (!TestNotNull(TEXT("dome actor") + Tag, Shell))
		{
			return false;
		}
		if (bArt)
		{
			TestTrue(TEXT("Power2Active cue requested"), FTDPowerPresentation::GetCueRequests(TEXT("Ship.Power2Active")) >= 1);
			TestTrue(TEXT("the active cue attaches to the dome, not Bastion"), Dome->GetActiveCueAttachRoot() == Shell->GetRootComponent());
		}
		W.Tick();
		W.Tick();
		TestEqual(TEXT("placeholder lattice only without art") + Tag, Shell->IsShowingPlaceholder(), !bArt);
	}
	return true;
}

/** A captured ship's "GravityWell" override is re-asserted if anything else is pushed on top of it. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerReviewWellReassert, "TriadDrift.Powers.Review.GravityWellReassertsItsCapture", TD_TEST_FLAGS)
bool FTDPowerReviewWellReassert::RunTest(const FString& Parameters)
{
	for (const float Step : TDPowerTest::kPowerFrameSteps)
	{
		TDPowerTest::FScopedPowerFrameStep StepScope(Step);
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		AActor* Bastion = TDPowerTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 100000.f, 0.f);
		UTDGravityWellPower* Well = TDPowerTest::AddPower<UTDGravityWellPower>(Bastion);
		ATDEnemyShip* Fighter = TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(4000.f, 1500.f, 0.f));
		Fighter->GetHealth()->InitMaxValues(100000.f, 0.f, true);
		UTDShipMovementComponent* Move = Fighter->GetShipMovement();
		TestTrue(TEXT("well launched") + Fps, Well->TryActivate());
		W.Advance(1.2f, Step);
		TestEqual(TEXT("captured: the well's override is on top") + Fps, Move->GetActiveSpeedOverrideOwner(), FName(TEXT("GravityWell")));
		Move->SetSpeedOverrideFor(TEXT("SomethingElse"), 20000.f, 1.f);
		W.Tick(Step);
		W.Tick(Step);
		TestEqual(TEXT("re-asserted over a later override") + Fps, Move->GetActiveSpeedOverrideOwner(), FName(TEXT("GravityWell")));
		W.Advance(6.f, Step);
		TestFalse(TEXT("released when the vortex ends") + Fps, Move->HasSpeedOverrideFor(TEXT("GravityWell")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
