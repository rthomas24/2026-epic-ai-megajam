// Test-only helpers for TriadDrift.Input tests. (UHT only parses headers, so receiver UCLASSes live here.)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TDInputTestHelpers.generated.h"

/** Counts UTDSettingsSubsystem::OnOnFootCameraModeChanged broadcasts. Root it while bound (AddToRoot / ON_SCOPE_EXIT). */
UCLASS(Transient, NotBlueprintable)
class UNREALGAMEJAM_API UTDSettingsEventCounter : public UObject
{
	GENERATED_BODY()

public:
	int32 CameraChanges = 0;
	ETDOnFootCamera LastMode = ETDOnFootCamera::FirstPerson;

	UFUNCTION()
	void HandleCameraMode(ETDOnFootCamera NewMode) { ++CameraChanges; LastMode = NewMode; }
};
