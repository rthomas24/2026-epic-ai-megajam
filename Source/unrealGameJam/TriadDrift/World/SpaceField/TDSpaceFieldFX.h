// Triad Drift - space field break cues (Teammate 6). The effects builder fills a DA_SpaceField_FX in
// /Game/TriadDrift/Presentation; empty cues fall back to the engineering placeholder burst.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TDSpaceFieldFX.generated.h"

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDSpaceFieldFX : public UDataAsset
{
	GENERATED_BODY()

public:
	/** A small piece breaks (1-10 m: pebble, panel, pod). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|SpaceField") FTDFXCue BreakSmall;
	/** A medium rock splits into smalls (10-40 m). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|SpaceField") FTDFXCue BreakMedium;
	/** A large rock or a wreck splits into mediums (40 m and up). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|SpaceField") FTDFXCue BreakLarge;
	/** A special (crystal-veined) asteroid breaks and drops its reward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|SpaceField") FTDFXCue BreakSpecial;
};
