// Triad Drift - Squad tests (Teammate 3). Records UTDEventBus squad events for automation tests.
// A UCLASS can't live inside #if WITH_DEV_AUTOMATION_TESTS, so this tiny class is always compiled;
// nothing in the game creates it.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDSquadTestListener.generated.h"

class UTDEventBus;

UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDSquadTestListener : public UObject
{
	GENERATED_BODY()

public:
	void Listen(UTDEventBus* Bus);
	void StopListening();

	UFUNCTION()
	void OnSwitched(ETDShipId NewShip, ETDShipId OldShip) { ++Switches; LastNew = NewShip; LastOld = OldShip; }

	UFUNCTION()
	void OnDowned(ETDShipId Ship) { ++Downed; LastDowned = Ship; }

	UFUNCTION()
	void OnRespawned(ETDShipId Ship) { ++Respawned; LastRespawned = Ship; }

	UFUNCTION()
	void OnWiped() { ++Wipes; }

	UFUNCTION()
	void OnPowerUsed(ETDShipId Ship, FName PowerName) { ++PowersUsed; }

	int32 Switches = 0;
	int32 Downed = 0;
	int32 Respawned = 0;
	int32 Wipes = 0;
	int32 PowersUsed = 0;
	ETDShipId LastNew = ETDShipId::Rift;
	ETDShipId LastOld = ETDShipId::Rift;
	ETDShipId LastDowned = ETDShipId::Rift;
	ETDShipId LastRespawned = ETDShipId::Rift;

private:
	TWeakObjectPtr<UTDEventBus> BoundBus;
};
