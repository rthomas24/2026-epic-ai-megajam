// Triad Drift - Combat (Teammate 2). Helpers for TriadDrift.Combat.* automation tests.
// UCLASSes must live in a header for UHT; they are tiny and harmless in shipping builds.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TDCombatTestHelpers.generated.h"

/** Records UTDEventBus::OnHitConfirmed broadcasts. */
UCLASS(Transient)
class UNREALGAMEJAM_API UTDTestHitListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnHit(AActor* Target, bool bKill)
	{
		++Hits;
		if (bKill)
		{
			++Kills;
		}
	}

	int32 Hits = 0;
	int32 Kills = 0;
};
