// Triad Drift - World (environment builder, Nov 14). T7's play (21 deaths following the conduit marker): Cinder's magma
// read as thin cracks on foot. Every ATDMagmaVolume gets a molten glow light over its footprint (orange, shadowless,
// sized to the volume) so the pool lights its banks and reads as lethal from walking distance, without touching T5's
// files. The surface material itself is MI_Cind_MagmaSurface (ue_magma_lethal.py).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDMagmaLook.generated.h"

class ATDMagmaVolume;

UCLASS()
class UNREALGAMEJAM_API UTDMagmaLookSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static const FName GlowTag;	// TD.MagmaGlow (on the added light)
	FLinearColor GlowColor = FLinearColor(1.f, 0.32f, 0.06f);
	float CandelasPerSquareMetre = 9.f;	// the glow's intensity per m^2 of pool (capped)
	float MaxCandelas = 9000.f;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	/** Adds the glow to one volume (idempotent; also called by tests). */
	void Dress(ATDMagmaVolume* Volume) const;

private:
	void HandleSpawned(AActor* Actor);
	FDelegateHandle SpawnHandle;
};
