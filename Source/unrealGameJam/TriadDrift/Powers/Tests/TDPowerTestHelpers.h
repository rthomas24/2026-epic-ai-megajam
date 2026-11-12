// Triad Drift - Powers (Teammate 2). Helpers for TriadDrift.Powers.* automation tests (UHT needs a header).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDPowerTestHelpers.generated.h"

/** Records UTDEventBus highlight and power-used broadcasts. */
UCLASS(Transient)
class UNREALGAMEJAM_API UTDTestPowerListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnHighlight(ETDHighlightKind Kind, AActor* Subject, float Importance01)
	{
		Highlights.Add(Kind);
	}

	UFUNCTION()
	void OnPowerUsed(ETDShipId Ship, FName PowerName)
	{
		PowersUsed.Add(PowerName);
	}

	TArray<ETDHighlightKind> Highlights;
	TArray<FName> PowersUsed;
};
