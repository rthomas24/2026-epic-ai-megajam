// Ship-feel A/B (T1's review, Nov 13): TD.Flight.ShipFeel gives each squad ship its own handling weight.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight.ShipFeel

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/Squad/TDSquadIntegration.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Misc/ScopeExit.h"

namespace TDShipFeelTest
{
	/** Seconds from min speed to 95% of cruise at full throttle. */
	float TimeToCruise(ETDShipId Id, float Dt = 1.f / 60.f)
	{
		FTDTestWorld W;
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
		if (!Ship) { return -1.f; }
		TDSquad::ApplySpec(Ship, UTDShipLibrary::Get(Id));
		const float Cruise = Ship->GetShipMovement()->GetFlightTuning().CruiseSpeed;
		FTDFlightInput In;
		In.Throttle = -1.f;
		Ship->SetFlightInput(In);
		W.Advance(4.f);
		In.Throttle = 1.f;
		Ship->SetFlightInput(In);
		for (float T = 0.f; T < 5.f; T += Dt)
		{
			W.Tick(Dt);
			if (Ship->GetVelocity().Size() >= Cruise * 0.95f) { return T; }
		}
		return 5.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDShipFeelPerShip, "TriadDrift.Flight.ShipFeel.PerShipWeightOnlyWhenSwitchedOn", TD_TEST_FLAGS)
bool FTDShipFeelPerShip::RunTest(const FString& Parameters)
{
	ON_SCOPE_EXIT { ATDShip::bPerShipFeel = false; };
	TestFalse(TEXT("off by default (until Ryan A/Bs it)"), ATDShip::bPerShipFeel);
	const FTDFlightTuning Shared;
	for (const ETDShipId Id : UTDShipLibrary::AllShips())
	{
		const FTDShipSpec& Spec = UTDShipLibrary::Get(Id);
		const FString Name = UEnum::GetValueAsString(Id);
		for (const bool bOn : { false, true })
		{
			ATDShip::bPerShipFeel = bOn;
			FTDTestWorld W;
			ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
			if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
			TDSquad::ApplySpec(Ship, Spec);
			const FTDFlightTuning& T = Ship->GetShipMovement()->GetFlightTuning();
			const FString Tag = FString::Printf(TEXT("%s, switch %d: "), *Name, bOn ? 1 : 0);
			TestEqual(*(Tag + TEXT("acceleration")), T.Acceleration, bOn ? Spec.FeelAcceleration : Shared.Acceleration);
			TestEqual(*(Tag + TEXT("nose response")), T.AngularResponse, bOn ? Spec.FeelAngularResponse : Shared.AngularResponse);
			TestEqual(*(Tag + TEXT("drift turn")), T.DriftTurnMultiplier, bOn ? Spec.FeelDriftTurnMultiplier : Shared.DriftTurnMultiplier);
			TestEqual(*(Tag + TEXT("dodge cooldown")), T.DodgeCooldown, bOn ? Spec.FeelDodgeCooldown : Shared.DodgeCooldown);
			TestEqual(*(Tag + TEXT("speeds unchanged")), T.CruiseSpeed, Spec.CruiseSpeed);
			TestEqual(*(Tag + TEXT("turn rate unchanged")), T.TurnRate, Spec.TurnRateDeg);
		}
	}

	// What the review found and the switch changes: the heavy Bastion reached cruise fastest.
	ATDShip::bPerShipFeel = false;
	const float OffRift = TDShipFeelTest::TimeToCruise(ETDShipId::Rift), OffBastion = TDShipFeelTest::TimeToCruise(ETDShipId::Bastion);
	ATDShip::bPerShipFeel = true;
	const float OnRift = TDShipFeelTest::TimeToCruise(ETDShipId::Rift), OnLancer = TDShipFeelTest::TimeToCruise(ETDShipId::Lancer), OnBastion = TDShipFeelTest::TimeToCruise(ETDShipId::Bastion);
	AddInfo(FString::Printf(TEXT("time to 95%% cruise: off Rift %.2f Bastion %.2f | on Rift %.2f Lancer %.2f Bastion %.2f s"), OffRift, OffBastion, OnRift, OnLancer, OnBastion));
	TestTrue(TEXT("off: the Bastion is quickest to cruise (the finding)"), OffBastion < OffRift);
	TestTrue(TEXT("on: the Bastion is the slowest to speed up"), OnBastion > OnRift && OnBastion > OnLancer);
	return true;
}

#endif
