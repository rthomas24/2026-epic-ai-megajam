// Triad Drift - Waves (Teammate 4). Placeholder warp-in telegraph: a red ring that grows at a spawn point
// for the warp-in time, so the player sees where a wave arrives before Nick's EnemyWarpIn VFX exists.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDWarpInMarker.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDWarpInMarker : public AActor
{
	GENERATED_BODY()

public:
	ATDWarpInMarker();

	/** Starts the telegraph: grows from 20% to full Size (cm) over Seconds, facing FaceToward, then removes itself. */
	void Begin(float Seconds, float Size, const FVector& FaceToward);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Waves")
	TObjectPtr<UStaticMeshComponent> Ring;

	/** Hard references so the cook ships what the placeholder uses. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

private:
	float Duration = 1.5f;
	float Elapsed = 0.f;
	float FullSize = 3000.f;
};
