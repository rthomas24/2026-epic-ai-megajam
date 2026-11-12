// Triad Drift - Powers (Teammate 2). Which powers each squad ship carries (features.md ENG-04).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TDPowerLibrary.generated.h"

class UTDPowerComponent;

UCLASS()
class UNREALGAMEJAM_API UTDPowerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Signature power class for a ship's slot (null if none: only Bastion has Signature2). */
	static TSubclassOf<UTDPowerComponent> ClassFor(ETDShipId Ship, ETDPowerInput Slot);

	/**
	 * Adds the ship's signature powers and its UTDAbilityComponent (ability slot). Idempotent: slots that
	 * already have a power are left alone, so calling it again on respawn does nothing.
	 * Rift: Portal Blaster. Lancer: Afterburner Ram. Bastion: Missile Swarm + Shield Dome.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Power")
	static void InstallShipPowers(AActor* Ship, ETDShipId ShipId);
};
