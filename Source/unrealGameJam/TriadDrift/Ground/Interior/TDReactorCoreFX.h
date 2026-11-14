// Triad Drift - the reactor fight's and the escape's effect slots (the effects builder fills them). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TDReactorCoreFX.generated.h"

class USoundBase;

/**
 * DA_ReactorCore_FX: found through UTDStatics::LoadPresentation (Presentation/ first, else the engineering copy anywhere
 * under Gameplay/Characters, e.g. Gameplay/Characters/FX/DA_ReactorCore_FX). Every slot is optional: an empty one plays
 * nothing, as before.
 */
UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDReactorCoreFX : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Loop attached at the reactor heart (about 4 m), from the interior's start until the core is destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor") FTDFXCue CoreIdle;
	/** One-shot at each conduit as it breaks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor") FTDFXCue ConduitBreak;
	/** One-shot at the core when its shield drops (the four conduits down). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor") FTDFXCue ShieldDrop;
	/** Loop attached to the sweeping laser's emitter, +X along the beam (the laser itself is still to come). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor") FTDFXCue LaserSweep;
	/** One-shot at the hit point on the heart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor") FTDFXCue HeartHit;
	/** One-shot at the heart when it is destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor") FTDFXCue CoreDestroyed;
	/** Loop attached to the on-foot pawn for the escape run (sparks, embers, falling debris). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape") FTDFXCue EscapeRunLoop;
	/** Loop attached to the flown ship during the escape flight (the flight's owner plays it; read from here). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape") FTDFXCue EscapeFlightLoop;
	/** 2D alarm loop for the escape run when EscapeRunLoop has no sound (engineering fallback SW_Dominion_EscapeAlarm_01). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape") TSoftObjectPtr<USoundBase> EscapeAlarm;

	/** The asset in use (Presentation wins), or null. */
	static const UTDReactorCoreFX* Get();
};
