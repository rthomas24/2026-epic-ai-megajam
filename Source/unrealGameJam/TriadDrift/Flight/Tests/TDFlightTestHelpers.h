// Test-only helper for TriadDrift.Flight tests: counts the movement component's dynamic events.
// (UHT only parses headers, so the receiver UCLASS lives here.)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Flight/TDFlightTypes.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TDFlightTestHelpers.generated.h"

class UTDShipMovementComponent;

/** Concrete do-nothing power (the base class is abstract) for controller routing tests. */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class UNREALGAMEJAM_API UTDTestPowerComponent : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	int32 Presses = 0;
	int32 Releases = 0;
	virtual void HandleInputPressed() override { ++Presses; Super::HandleInputPressed(); }
	virtual void HandleInputReleased() override { ++Releases; Super::HandleInputReleased(); }
};

UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDFlightEventCounter : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UTDShipMovementComponent* Movement);

	int32 BoostStarted = 0;
	int32 BoostEnded = 0;
	int32 DriftStarted = 0;
	int32 DriftEnded = 0;
	int32 DriftBursts = 0;
	int32 Impacts = 0;
	int32 DamagingImpacts = 0;
	float LastImpactDamage = 0.f;
	float TotalImpactDamage = 0.f;
	bool bLastImpactHeadOn = false;

	UFUNCTION() void HandleBoostStarted() { ++BoostStarted; }
	UFUNCTION() void HandleBoostEnded() { ++BoostEnded; }
	UFUNCTION() void HandleDriftStarted() { ++DriftStarted; }
	UFUNCTION() void HandleDriftEnded(bool bBurst) { ++DriftEnded; DriftBursts += bBurst ? 1 : 0; }
	UFUNCTION() void HandleImpact(const FTDFlightImpact& Impact);

	/** Damage taken by kind (index = ETDDamageKind) and hit count, from UTDHealthComponent::OnDamaged. */
	float DamageByKind[6] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
	int32 DamageHits = 0;
	UFUNCTION() void HandleDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
	{
		++DamageHits;
		const int32 K = FMath::Clamp((int32)Info.Kind, 0, 5);
		DamageByKind[K] += ShieldDamage + HullDamage;
	}

	int32 SimpleEvents = 0;
	UFUNCTION() void HandleSimple() { ++SimpleEvents; }

	int32 Dodges = 0;
	float LastDodgeDir = 0.f;
	UFUNCTION() void HandleDodge(float Direction) { ++Dodges; LastDodgeDir = Direction; }

	int32 PauseChanges = 0;
	bool bLastPaused = false;
	UFUNCTION() void HandlePause(bool bPaused) { ++PauseChanges; bLastPaused = bPaused; }

	TArray<ETDBoundaryState> BoundaryStates;
	UFUNCTION() void HandleBoundary(ETDBoundaryState NewState) { BoundaryStates.Add(NewState); }
};
