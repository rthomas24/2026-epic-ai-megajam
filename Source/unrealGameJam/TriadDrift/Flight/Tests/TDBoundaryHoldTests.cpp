// The edge nag is silent while a sequence owns the ship (the Lead, Nov 13: "LEAVING THE FLIGHT AREA - TURN BACK" over the
// climb-out director shot). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.BoundaryHold

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1BoundaryHold, "TriadDrift.Flight.BoundaryHold.SilentWhileASequenceOwnsTheShip", TD_TEST_FLAGS)
bool FTDT1BoundaryHold::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	UTDFlightEventCounter* Counter = NewObject<UTDFlightEventCounter>();
	Counter->AddToRoot();
	Counter->Bind(Move);
	Move->OnBoundaryStateChanged.AddDynamic(Counter, &UTDFlightEventCounter::HandleBoundary); // Bind leaves this one out
	// A 1 km box whose wall is 100 m away: the ship is in the 1 km warning band.
	auto Edge = [&]() { Move->SetBoxBoundary(FBox(FVector(-90000.f, -90000.f, 0.f), FVector(10000.f, 90000.f, 100000.f)), 100000.f); };
	Edge();
	for (int32 i = 0; i < 10; ++i) { Edge(); W.Tick(1.f / 60.f); }
	TestTrue(TEXT("in the band: the warning shows"), !Move->GetBoundaryWarningText().IsEmpty());
	TestTrue(TEXT("in the band: one state event"), Counter->BoundaryStates.Num() >= 1);

	for (const FName Owner : { FName(TEXT("TDExitClimb")), FName(TEXT("AtmosphereEntry")), FName(TEXT("Hyperdrive")), FName(TEXT("LandingAssist")) })
	{
		Move->SetSpeedOverrideFor(Owner, 3000.f, 0.3f);
		TestTrue(*FString::Printf(TEXT("%s: held"), *Owner.ToString()), Move->IsBoundaryWarningHeld());
		TestTrue(*FString::Printf(TEXT("%s: no warning text"), *Owner.ToString()), Move->GetBoundaryWarningText().IsEmpty());
		Move->ClearSpeedOverrideFor(Owner);
	}
	// A gameplay override (the ram) is not a sequence.
	Move->SetSpeedOverrideFor(TEXT("Ram"), 3000.f, 1.f);
	TestFalse(TEXT("the ram doesn't hold it"), Move->IsBoundaryWarningHeld());
	Move->ClearSpeedOverrideFor(TEXT("Ram"));

	// An explicit hold (T4's withdrawal): no events while held, one on release (still in the band).
	Move->SetBoundaryWarningHoldFor(TEXT("Withdrawal"), true);
	W.Tick(1.f / 60.f);
	const int32 Before = Counter->BoundaryStates.Num();
	// Push it outside meanwhile: still silent.
	Move->SetBoxBoundary(FBox(FVector(-90000.f, -90000.f, 0.f), FVector(-5000.f, 90000.f, 100000.f)), 100000.f);
	for (int32 i = 0; i < 5; ++i) { W.Tick(1.f / 60.f); }
	TestEqual(TEXT("held: no state events"), Counter->BoundaryStates.Num(), Before);
	TestTrue(TEXT("held: no text"), Move->GetBoundaryWarningText().IsEmpty());
	Move->SetBoundaryWarningHoldFor(TEXT("Withdrawal"), false);
	W.Tick(1.f / 60.f);
	TestEqual(TEXT("released near the edge: exactly one event"), Counter->BoundaryStates.Num(), Before + 1);
	TestTrue(TEXT("released: the warning is back"), !Move->GetBoundaryWarningText().IsEmpty());
	Counter->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1BoundaryRelease, "TriadDrift.Flight.BoundaryHold.ReleaseLetsACorridorRunPastTheEdge", TD_TEST_FLAGS)
bool FTDT1BoundaryRelease::RunTest(const FString& Parameters)
{
	// T4's ordered withdrawal (T7's playtest): the escape line runs past the combat zone's edge.
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	Ship->TeleportSpacecraft(FVector(0.f, 0.f, 20000.f), FRotator::ZeroRotator, false); // flying +X, out of the box
	Move->SetBoxBoundary(FBox(FVector(-90000.f, -90000.f, 0.f), FVector(-5000.f, 90000.f, 100000.f)), 100000.f);
	Move->SetBoundaryReleaseFor(TEXT("Withdrawal"), true);
	for (int32 i = 0; i < 60; ++i) { W.Tick(1.f / 60.f); }
	TestTrue(TEXT("released: no warning"), Move->GetBoundaryWarningText().IsEmpty() && Move->IsBoundaryWarningHeld());
	TestFalse(TEXT("released: not out of bounds"), Move->IsOutOfBounds());
	TestTrue(TEXT("released: the nose is not turned back"), Ship->GetActorForwardVector().X > 0.99f);
	Move->SetBoundaryReleaseFor(TEXT("Withdrawal"), false);
	for (int32 i = 0; i < 60; ++i) { W.Tick(1.f / 60.f); }
	TestTrue(TEXT("released no more: out of bounds again, with the warning"), Move->IsOutOfBounds() && !Move->GetBoundaryWarningText().IsEmpty());
	TestTrue(TEXT("... and the nose turns back"), Ship->GetActorForwardVector().X < 0.99f);
	return true;
}

#endif
