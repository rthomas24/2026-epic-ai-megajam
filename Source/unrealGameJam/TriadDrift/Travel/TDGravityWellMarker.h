// Triad Drift - placeable gravity well (hyperdrive drop-out point, ENG-15). Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDGravityWellMarker.generated.h"

/**
 * Drop a ship out of hyperdrive when it comes within Radius. Place one at each planet marker, at the
 * Dominion's approach, or attach one to a moving actor. Registers itself with UTDTravelSubsystem.
 * Code can skip the actor and call UTDTravelSubsystem::RegisterGravityWell directly.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDGravityWellMarker : public AActor
{
	GENERATED_BODY()

public:
	ATDGravityWellMarker();

	/** Unique id (e.g. "Verdance", "Dominion"). Defaults to the actor name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Travel")
	FName WellId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Travel", meta = (ClampMin = "0"))
	float Radius = 15000.f;

	/** Whether the well is active (e.g. a planet marker that hasn't been revealed yet can start off). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Travel")
	void SetWellEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Travel")
	bool bWellEnabled = true;

	FName GetResolvedId() const { return WellId != NAME_None ? WellId : GetFName(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
