// Triad Drift - Powers (Teammate 2). ENG-09 portal blaster + Blink. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDPortalTransit.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Powers/Tests/TDPowerTestHelpers.h"


namespace TDPowerTest
{
	/** Rift at the origin opens A straight ahead (+X, 12 000 cm) and B to its left (+Y, 12 000 cm). */
	UTDPortalBlasterPower* PortalTestOpenPair(FTDTestWorld& W, AActor*& OutRift)
	{
		OutRift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f), 100.f, 100.f);
		UTDPortalBlasterPower* Power = AddPower<UTDPortalBlasterPower>(OutRift);
		Power->HandleInputPressed();
		OutRift->SetActorRotation(FRotator(0.f, 90.f, 0.f));
		Power->HandleInputPressed();
		OutRift->SetActorRotation(FRotator::ZeroRotator);
		return Power;
	}

	int32 PortalTestTotalTransits(const UTDPortalBlasterPower* Power)
	{
		int32 N = 0;
		for (const TWeakObjectPtr<ATDPortal>& P : Power->GetPortals())
		{
			N += P.IsValid() ? P->GetTransitCount() : 0;
		}
		return N;
	}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalPlacement, "TriadDrift.Powers.Portal.PlacementPairingAndRecycle", TD_TEST_FLAGS)
bool FTDPortalPlacement::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rift = nullptr;
	UTDPortalBlasterPower* Power = PortalTestOpenPair(W, Rift);
	TestTrue(TEXT("power active"), Power->IsPowerActive());
	TestEqual(TEXT("two portals"), Power->GetPortals().Num(), 2);
	ATDPortal* A = Power->GetPortals()[0].Get();
	ATDPortal* B = Power->GetPortals()[1].Get();
	TestTrue(TEXT("A at max range ahead"), A && A->GetActorLocation().Equals(FVector(12000.f, 0.f, 0.f), 1.f));
	TestTrue(TEXT("B at max range left"), B && B->GetActorLocation().Equals(FVector(0.f, 12000.f, 0.f), 1.f));
	TestTrue(TEXT("A faces back toward Rift"), A && A->GetActorForwardVector().Equals(-FVector::ForwardVector, 0.01f));
	TestTrue(TEXT("paired"), A && B && A->GetPartner() == B && B->GetPartner() == A);

	// Third shot recycles the oldest (A).
	Rift->SetActorRotation(FRotator(0.f, -90.f, 0.f));
	Power->HandleInputPressed();
	W.Tick();
	TestEqual(TEXT("still two portals"), Power->GetPortals().Num(), 2);
	TestFalse(TEXT("A closed"), IsValid(A));
	ATDPortal* C = Power->GetPortals()[1].Get();
	TestTrue(TEXT("B now pairs with C"), B && C && B->GetPartner() == C && C->GetPartner() == B);

	// A wall: the portal opens 300 cm off its surface.
	FTDTestWorld W2;
	AActor* Rift2 = SpawnBody(W2, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	SpawnWall(W2, FVector(5100.f, 0.f, 0.f), FVector(100.f, 3000.f, 3000.f)); // face at x = 5000
	UTDPortalBlasterPower* Power2 = AddPower<UTDPortalBlasterPower>(Rift2);
	Power2->HandleInputPressed();
	ATDPortal* OnWall = Power2->GetPortals().Num() ? Power2->GetPortals()[0].Get() : nullptr;
	// 100 cm sweep sphere touches at x = 4900, minus the 300 cm offset.
	TestTrue(TEXT("placed off the wall"), OnWall && FMath::IsNearlyEqual(OnWall->GetActorLocation().X, 4600.f, 5.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalShipTransit, "TriadDrift.Powers.Portal.ShipKeepsSpeedTakesExitFacing", TD_TEST_FLAGS)
bool FTDPortalShipTransit::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rift = nullptr;
	UTDPortalBlasterPower* Power = PortalTestOpenPair(W, Rift);

	ATDSpacecraft* Ship = W.Spawn<ATDSpacecraft>(FVector(9000.f, 150.f, 0.f), FRotator::ZeroRotator);
	Ship->SetTeam(ETDTeam::Player);
	FTDFlightInput In;
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);

	float SpeedBefore = 0.f;
	const float At = AdvanceUntil(W, [&]
	{
		if (PortalTestTotalTransits(Power) == 0)
		{
			SpeedBefore = Ship->GetVelocity().Size();
			return false;
		}
		return true;
	}, 4.f);
	TestTrue(TEXT("ship went through A"), At > 0.f);
	TestTrue(TEXT("came out in front of B"), FVector::Dist2D(Ship->GetActorLocation(), FVector(0.f, 11600.f, 0.f)) < 800.f);
	TestTrue(TEXT("facing B's normal (-Y)"), FVector::DotProduct(Ship->GetActorForwardVector(), -FVector::RightVector) > 0.98f);
	TestEqual(TEXT("kept its speed"), (float)Ship->GetVelocity().Size(), SpeedBefore, SpeedBefore * 0.05f + 50.f);
	TestTrue(TEXT("lateral offset kept (150 cm right of centre)"), FMath::Abs(Ship->GetActorLocation().X - (-150.f)) < 200.f
		|| FMath::Abs(Ship->GetActorLocation().X - 150.f) < 200.f);
	TestTrue(TEXT("0.3 s invulnerable"), Ship->GetHealth() && Ship->GetHealth()->IsInvulnerable());
	W.Advance(0.4f);
	TestFalse(TEXT("invulnerability over"), Ship->GetHealth() && Ship->GetHealth()->IsInvulnerable());

	W.Advance(2.f);
	TestEqual(TEXT("exactly one transit (no bounce back)"), PortalTestTotalTransits(Power), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalMissileReturn, "TriadDrift.Powers.Portal.EnemyMissileReturnsToShooter", TD_TEST_FLAGS)
bool FTDPortalMissileReturn::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rift = nullptr;
	UTDPortalBlasterPower* Power = PortalTestOpenPair(W, Rift);
	AActor* Interceptor = SpawnBody(W, ETDTeam::Enemy, FVector(20000.f, 0.f, 0.f), FVector(150.f), 100.f, 100.f);

	UTDTestPowerListener* Listener = NewObject<UTDTestPowerListener>();
	Listener->AddToRoot();
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	if (Bus)
	{
		Bus->OnHighlightMoment.AddDynamic(Listener, &UTDTestPowerListener::OnHighlight);
	}

	// Fired at Rift from behind portal A: it flies into A on the way.
	ATDProjectile* Missile = LaunchMissile(W, FVector(15000.f, 0.f, 0.f), -FVector::ForwardVector, Interceptor, Rift);
	const float At = AdvanceUntil(W, [&] { return PortalTestTotalTransits(Power) > 0; }, 2.f);
	TestTrue(TEXT("missile entered the portal"), At > 0.f);
	TestTrue(TEXT("now on the Player team"), Missile && Missile->GetTeam() == ETDTeam::Player);
	TestTrue(TEXT("re-homing on the interceptor"), Missile && Missile->GetHomingTarget() == Interceptor);
	TestTrue(TEXT("MissileReturn highlight (accepted by the director)"), Listener->Highlights.Contains(ETDHighlightKind::MissileReturn));
	UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	TestTrue(TEXT("director money shot: slow-mo running"), Feel && Feel->IsSlowMoActive());

	// Assert at impact: the interceptor's shield regenerates 3 s later.
	const float HitAt = AdvanceUntil(W, [&] { return Shield(Interceptor) < 100.f; }, 6.f);
	TestTrue(TEXT("returned missile reached the interceptor"), HitAt > 0.f);
	TestEqual(TEXT("interceptor hit by its own missile"), Shield(Interceptor), 75.f, 0.01f);
	TestEqual(TEXT("Rift untouched"), Shield(Rift), 100.f);

	if (Bus)
	{
		Bus->OnHighlightMoment.RemoveAll(Listener);
	}
	Listener->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalTransitHelper, "TriadDrift.Powers.Portal.TransitHelperRules", TD_TEST_FLAGS)
bool FTDPortalTransitHelper::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	const FTransform Entry(FRotator::ZeroRotator, FVector(1000.f, 0.f, 0.f));
	const FTransform Exit(FRotator(0.f, 90.f, 0.f), FVector(0.f, 5000.f, 0.f));
	FTDPortalTransitParams Params;

	TestTrue(TEXT("mapped direction"), UTDPortalTransit::MapDirection(FVector::ForwardVector, Entry, Exit).Equals(FVector::RightVector, 0.001f));
	TestTrue(TEXT("mapped location keeps lateral offset"),
		UTDPortalTransit::MapLocation(FVector(1000.f, 100.f, 50.f), Entry, Exit, 400.f).Equals(FVector(-100.f, 5400.f, 50.f), 0.5f));

	ATDEnemyShip* Fighter = SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(1000.f, 0.f, 0.f));
	TestNotNull(TEXT("fighter"), Fighter);
	TestTrue(TEXT("fighter transits"), UTDPortalTransit::TransitActor(Fighter, Entry, Exit, nullptr, Params));
	TestTrue(TEXT("fighter loses tracking"), Fighter && Fighter->IsTrackingLost());
	TestTrue(TEXT("fighter takes the exit facing"), Fighter && Fighter->GetActorForwardVector().Equals(FVector::RightVector, 0.01f));
	TestFalse(TEXT("re-entry cooldown blocks an instant bounce"), UTDPortalTransit::TransitActor(Fighter, Exit, Entry, nullptr, Params));
	W.Advance(2.1f);
	TestFalse(TEXT("tracking back after 2 s"), Fighter && Fighter->IsTrackingLost());
	TestTrue(TEXT("transits again after the cooldown"), UTDPortalTransit::TransitActor(Fighter, Exit, Entry, nullptr, Params));

	AActor* NotATraveller = SpawnBody(W, ETDTeam::Enemy, FVector(1000.f, 0.f, 0.f));
	TestFalse(TEXT("plain actors are refused"), UTDPortalTransit::TransitActor(NotATraveller, Entry, Exit, nullptr, Params));
	TestFalse(TEXT("null is refused"), UTDPortalTransit::TransitActor(nullptr, Entry, Exit, nullptr, Params));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalWindow, "TriadDrift.Powers.Portal.OpenFor12sThen8sCooldown", TD_TEST_FLAGS)
bool FTDPortalWindow::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Rift = nullptr;
	UTDPortalBlasterPower* Power = PortalTestOpenPair(W, Rift);
	W.Advance(11.8f);
	TestTrue(TEXT("still open at 11.8 s"), Power->IsPowerActive() && Power->GetPortals().Num() == 2);
	TestEqual(TEXT("cooldown frozen while open"), Power->GetCooldown01(), 0.f);
	W.Advance(0.3f);
	TestFalse(TEXT("closed after 12 s"), Power->IsPowerActive());
	TestEqual(TEXT("portals gone"), Power->GetPortals().Num(), 0);
	TestFalse(TEXT("on cooldown"), Power->CanActivate());
	W.Advance(7.8f);
	TestFalse(TEXT("not ready at 7.8 s"), Power->CanActivate());
	W.Advance(0.3f);
	TestTrue(TEXT("ready after 8 s"), Power->CanActivate());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalClearance, "TriadDrift.Powers.Portal.NeverExitsIntoGeometry", TD_TEST_FLAGS)
bool FTDPortalClearance::RunTest(const FString& Parameters)
{
	// Placement: boxed in front and behind -> bounded push-out fails -> refused.
	{
		FTDTestWorld W;
		AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		SpawnWall(W, FVector(2700.f, 0.f, 0.f), FVector(2000.f));  // face at x = 700
		SpawnWall(W, FVector(-1300.f, 0.f, 0.f), FVector(1000.f)); // face at x = -300
		UTDPortalBlasterPower* Power = AddPower<UTDPortalBlasterPower>(Rift);
		Power->HandleInputPressed();
		TestEqual(TEXT("placement refused"), Power->GetRefusedPlacements(), 1);
		TestEqual(TEXT("no portal"), Power->GetPortals().Num(), 0);
		TestFalse(TEXT("refused shot doesn't start the power"), Power->IsPowerActive());
	}
	// Transit: geometry dropped in front of B after placement -> the ship is not sent through.
	{
		FTDTestWorld W;
		AActor* Rift = nullptr;
		UTDPortalBlasterPower* Power = PortalTestOpenPair(W, Rift);
		SpawnWall(W, FVector(0.f, 11300.f, 0.f), FVector(600.f, 200.f, 600.f));
		ATDSpacecraft* Ship = W.Spawn<ATDSpacecraft>(FVector(9000.f, 0.f, 0.f), FRotator::ZeroRotator);
		Ship->SetTeam(ETDTeam::Player);
		FTDFlightInput In;
		In.Throttle = 1.f;
		Ship->SetFlightInput(In);
		W.Advance(2.f);
		TestEqual(TEXT("blocked exit: no transit"), PortalTestTotalTransits(Power), 0);
		TestTrue(TEXT("ship flew straight on past A"), Ship->GetActorLocation().X > 12000.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPortalBlink, "TriadDrift.Powers.Portal.BlinkFallback", TD_TEST_FLAGS)
bool FTDPortalBlink::RunTest(const FString& Parameters)
{
	for (const bool bViaCVar : { false, true })
	{
		FTDTestWorld W;
		AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f), 100.f, 100.f);
		AActor* Interceptor = SpawnBody(W, ETDTeam::Enemy, FVector(20000.f, 3000.f, 0.f), FVector(150.f), 100.f, 100.f);
		UTDPortalBlasterPower* Power = AddPower<UTDPortalBlasterPower>(Rift);
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("TD.PortalBlink"));
		if (bViaCVar)
		{
			TestNotNull(TEXT("TD.PortalBlink exists"), CVar);
			if (CVar)
			{
				CVar->Set(1);
			}
		}
		else
		{
			Power->bUseBlinkFallback = true;
		}
		TestTrue(TEXT("blink mode"), Power->IsUsingBlink());

		TestTrue(TEXT("blink fired"), Power->TryActivate());
		TestTrue(TEXT("teleported to max range"), Rift->GetActorLocation().Equals(FVector(12000.f, 0.f, 0.f), 1.f));
		TestTrue(TEXT("invulnerable after blink"), UTDStatics::GetHealth(Rift)->IsInvulnerable());
		TestEqual(TEXT("no portals in blink mode"), Power->GetPortals().Num(), 0);
		TestFalse(TEXT("blink is instant"), Power->IsPowerActive());
		TestNull(TEXT("reflect field is not a squad dome"), UTDPowerSubsystem::Get(W.GetWorld())->FindDomeCovering(Rift->GetActorLocation()));

		ATDProjectile* Missile = LaunchMissile(W, FVector(14000.f, 0.f, 0.f), -FVector::ForwardVector, Interceptor, Rift);
		W.Advance(0.3f);
		TestTrue(TEXT("reflect field turned the missile"), Missile && Missile->GetTeam() == ETDTeam::Player);
		W.Advance(3.f);
		TestEqual(TEXT("Rift unharmed"), Shield(Rift), 100.f);

		TestFalse(TEXT("8 s cooldown"), Power->CanActivate());
		W.Advance(5.f);
		TestTrue(TEXT("ready after 8 s"), Power->CanActivate());
		if (CVar)
		{
			CVar->Set(0);
		}
	}
	return true;
}

} // namespace TDPowerTest

#endif // WITH_DEV_AUTOMATION_TESTS
