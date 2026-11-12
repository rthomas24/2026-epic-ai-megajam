// Test-only helper for TriadDrift.Travel tests: records UTDEventBus::OnHyperdriveState broadcasts.
// (UHT only parses headers, so the receiver UCLASS lives here.)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TDTravelTestHelpers.generated.h"

UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDHyperdriveStateRecorder : public UObject
{
	GENERATED_BODY()

public:
	TArray<ETDHyperdriveState> States;

	UFUNCTION()
	void HandleState(ETDHyperdriveState State) { States.Add(State); }

	bool LastIs(ETDHyperdriveState State) const { return States.Num() > 0 && States.Last() == State; }

	TArray<FText> RefusedTexts;
	UFUNCTION()
	void HandleRefused(UTDHyperdriveComponent* Hyperdrive, ETDHyperdriveBlock Reason, FText ReasonText) { RefusedTexts.Add(ReasonText); }
};
