// Test-only helper for TriadDrift.Flight tests.

#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"

void UTDFlightEventCounter::Bind(UTDShipMovementComponent* Movement)
{
	if (!Movement)
	{
		return;
	}
	Movement->OnBoostStarted.AddDynamic(this, &UTDFlightEventCounter::HandleBoostStarted);
	Movement->OnBoostEnded.AddDynamic(this, &UTDFlightEventCounter::HandleBoostEnded);
	Movement->OnDriftStarted.AddDynamic(this, &UTDFlightEventCounter::HandleDriftStarted);
	Movement->OnDriftReleased.AddDynamic(this, &UTDFlightEventCounter::HandleDriftEnded);
	Movement->OnFlightImpact.AddDynamic(this, &UTDFlightEventCounter::HandleImpact);
}

void UTDFlightEventCounter::HandleImpact(const FTDFlightImpact& Impact)
{
	++Impacts;
	LastImpactDamage = Impact.Damage;
	TotalImpactDamage += Impact.Damage;
	bLastImpactHeadOn = Impact.bHeadOn;
	if (Impact.Damage > 0.f)
	{
		++DamagingImpacts;
	}
}
