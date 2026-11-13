// Triad Drift - a stand-in for Nick's Director's Cut renderer widget in tests (N-038). Owner: Teammate 3.
// A UCLASS has to live in a header for UHT; it's empty and inert outside tests.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDDirectorTestWidget.generated.h"

UCLASS(Transient, NotBlueprintable, NotBlueprintType)
class UTDDirectorTestWidget : public UUserWidget
{
	GENERATED_BODY()
};
