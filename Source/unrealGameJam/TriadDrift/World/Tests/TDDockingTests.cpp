// ENG-29 docking: the boarding finale opens the bay when the generators fall, the tractor pulls a ship in the cone
// (never locking it before the mouth), the glide hands off to the interior exactly once, and the space ending stays
// the fallback. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Docking

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDockingBay.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"

namespace TDDockingTest
{
	/** A revealed Dominion with its generators destroyed, in BoardingMode Mode. */
	ATDDominion* DefensesDown(FTDTestWorld& W, ETDBoardingMode Mode)
	{
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true;
		GI->Progress.Phase = ETDGamePhase::DominionAssault;
		ATDDominion* D = W.Spawn<ATDDominion>(FVector(400000.f, 0.f, 0.f));
		D->BoardingMode = Mode;
		AActor* Killer = W.Spawn<AActor>(FVector(300000.f, 0.f, 0.f));
		W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.5f);
		for (ATDShieldGenerator* G : D->GetGenerators()) { UTDStatics::ApplyTDDamage(G, 1000000.f, ETDDamageKind::Explosive, Killer, Killer); W.Advance(0.2f); }
		W.Advance(0.5f);
		return D;
	}

	/** A ship at Along cm out of the bay's mouth, Lateral cm off the axis, nose YawOffDeg off the mouth. */
	ATDShip* ShipInCone(FTDTestWorld& W, ATDDockingBay* Bay, float Along, float Lateral, float YawOffDeg)
	{
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Bay->GetOutward()).GetSafeNormal();
		const FVector Loc = Bay->GetMouth() + Bay->GetOutward() * Along + Right * Lateral;
		ATDShip* Ship = W.Spawn<ATDShip>(Loc);
		if (!Ship) { return nullptr; }
		const FRotator Face = (Bay->GetMouth() - Loc).Rotation() + FRotator(0.f, YawOffDeg, 0.f);
		Ship->SetActorRotation(Face);
		Ship->SetPlayerControlled(true);
		Bay->SetShipOverride(Ship);
		return Ship;
	}

	float LateralOf(const ATDDockingBay* Bay, const FVector& P)
	{
		const FVector D = P - Bay->GetMouth();
		return (D - FVector::DotProduct(D, Bay->GetOutward()) * Bay->GetOutward()).Size();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingFallback, "TriadDrift.World.Docking.SpaceEndingStaysTheFallback", TD_TEST_FLAGS)
bool FTDDockingFallback::RunTest(const FString&)
{
	for (const ETDBoardingMode Mode : { ETDBoardingMode::Auto, ETDBoardingMode::ForceOff })
	{
		// Auto in a test world (/Temp) and ForceOff (td.Finale.Boarding 0) both keep today's reactor ending.
		FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
		ATDDominion* D = TDDockingTest::DefensesDown(W, Mode);
		TestFalse(TEXT("no boarding"), D->UsesBoardingFinale());
		TestEqual(TEXT("reactor exposed"), (int32)D->GetState(), (int32)ETDDominionState::ReactorExposed);
		TestNull(TEXT("no docking bay"), D->GetDockingBay());
		TestTrue(TEXT("the reactor is open to fire"), D->GetReactor() && D->GetReactor()->IsExposed());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingOpens, "TriadDrift.World.Docking.GeneratorsDownOpenTheBay", TD_TEST_FLAGS)
bool FTDDockingOpens::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	TestEqual(TEXT("docking open"), (int32)D->GetState(), (int32)ETDDominionState::DockingOpen);
	ATDDockingBay* Bay = D->GetDockingBay();
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	TestTrue(TEXT("bay open"), Bay->IsOpen());
	TestTrue(TEXT("the reactor stays shielded (the core fight is inside)"), !D->GetReactor() || !D->GetReactor()->IsExposed());
	// The mouth is on the bow face, pointing out along the Dominion's nose.
	const FVector Local = D->GetActorTransform().InverseTransformPosition(Bay->GetMouth());
	TestTrue(*FString::Printf(TEXT("mouth on the bow (x %.0f vs hull max %.0f)"), Local.X, D->GetLocalHullBounds().Max.X), FMath::IsNearlyEqual(Local.X, D->GetLocalHullBounds().Max.X, 10.f));
	TestTrue(TEXT("faces out of the bow"), FVector::DotProduct(Bay->GetOutward(), D->GetActorForwardVector()) > 0.99f);
	// The approach stays a fight: launches and the hyperdrive lock carry on.
	TestTrue(TEXT("still a hot approach (stage 1)"), D->GetStage() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingTractor, "TriadDrift.World.Docking.TractorPullsButNeverLocksBeforeTheMouth", TD_TEST_FLAGS)
bool FTDDockingTractor::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	TestEqual(TEXT("outside the cone: no pull"), Bay->GetTractorStrength(Bay->GetMouth() + Bay->GetOutward() * 100000.f + FVector(0.f, 0.f, 200000.f)), 0.f);
	TestEqual(TEXT("behind the mouth: no pull"), Bay->GetTractorStrength(Bay->GetMouth() - Bay->GetOutward() * 1000.f), 0.f);

	// 1.8 km out, 250 m off the axis, nose 20 degrees off: the tractor slows, softens and lines the ship up.
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 180000.f, 25000.f, 20.f);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	const float Lat0 = TDDockingTest::LateralOf(Bay, Ship->GetActorLocation());
	W.Advance(1.f);
	TestTrue(*FString::Printf(TEXT("held (pull %.2f)"), Bay->GetCurrentPull()), Bay->GetCurrentPull() > 0.2f);
	TestTrue(TEXT("speed override on"), Ship->GetShipMovement() && Ship->GetShipMovement()->HasSpeedOverrideFor(ATDDockingBay::TractorOverride));
	TestFalse(TEXT("never locked before the mouth"), Ship->IsFlightParked());
	W.Advance(2.f);
	TestTrue(*FString::Printf(TEXT("lined up (%.0f -> %.0f cm off the axis)"), Lat0, TDDockingTest::LateralOf(Bay, Ship->GetActorLocation())), TDDockingTest::LateralOf(Bay, Ship->GetActorLocation()) < Lat0 * 0.9f);

	// Abort: the pilot flies out of the cone and gets full control back.
	Ship->SetActorLocation(Bay->GetMouth() + Bay->GetOutward() * 150000.f + FVector(0.f, 0.f, 250000.f));
	W.Advance(0.2f);
	TestEqual(TEXT("out of the cone: released"), Bay->GetCurrentPull(), 0.f);
	TestFalse(TEXT("override cleared"), Ship->GetShipMovement()->HasSpeedOverrideFor(ATDDockingBay::TractorOverride));
	TestEqual(TEXT("still open, nobody docked"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Open);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingHandOff, "TriadDrift.World.Docking.GlideHandsOffToTheInteriorOnce", TD_TEST_FLAGS)
bool FTDDockingHandOff::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 20000.f, 1000.f, 0.f);	// 200 m out, near the axis
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Advance(0.1f);
	TestEqual(TEXT("gliding"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Gliding);
	TestTrue(TEXT("controls locked for the glide"), Ship->IsFlightParked());
	const UTDHealthComponent* H = UTDStatics::GetHealth(Ship);
	TestTrue(TEXT("invulnerable in the glide"), H && H->IsInvulnerable());
	W.Advance(Bay->GlideSeconds + 1.f);
	TestEqual(TEXT("docked"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Docked);
	TestEqual(TEXT("exactly one travel request"), Bay->GetTravelRequests(), 1);
	TestTrue(TEXT("travels to the interior"), GI->Travels.Num() == 1 && GI->Travels[0] == ETDDestination::DominionInterior);
	TestTrue(TEXT("phase DominionInterior"), GI->Progress.Phase == ETDGamePhase::DominionInterior);
	W.Advance(2.f);
	TestEqual(TEXT("still one travel"), GI->Travels.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDockingRetake, "TriadDrift.World.Docking.RetakeKeepsTheBayOpenAndLetsGo", TD_TEST_FLAGS)
bool FTDDockingRetake::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDDominion* D = TDDockingTest::DefensesDown(W, ETDBoardingMode::ForceOn);
	ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
	if (!TestNotNull(TEXT("bay"), Bay)) { return false; }
	ATDShip* Ship = TDDockingTest::ShipInCone(W, Bay, 20000.f, 1000.f, 0.f);
	W.Advance(0.1f);
	TestEqual(TEXT("gliding"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Gliding);
	Bay->ResetForNewTake();	// a CUT lands mid-glide (the take restart)
	TestEqual(TEXT("back to open"), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Open);
	TestFalse(TEXT("controls back"), Ship && Ship->IsFlightParked());
	TestEqual(TEXT("no travel"), Bay->GetTravelRequests(), 0);
	TestEqual(TEXT("still the boarding state"), (int32)D->GetState(), (int32)ETDDominionState::DockingOpen);
	return true;
}

#endif
