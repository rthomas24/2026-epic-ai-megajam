// Triad Drift - World (environment builder, Nov 14). T2's play on main 9155a6dc: during the docking glide the Dominion's
// bay pocket (the structures builder's BP_DockingBay_Interior: white kit, ~30 lights of 6000 cd) blew out to near
// white and the ship barely read. Without editing their asset: the pocket's lights are scaled down at runtime and a
// post-process volume over the pocket holds a fixed exposure (no adaptation from black space), with less bloom.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDBayLook.generated.h"

UCLASS()
class UNREALGAMEJAM_API UTDBayLookSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static const FName DoneTag;	// TD.BayLook (on a dressed pocket)
	float LightScale = 0.3f;
	float ExposureBias = -0.5f;	// manual exposure in the pocket (EV, physical camera exposure off, like the interior)
	float BloomIntensity = 0.25f;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	/** Dress one bay pocket (idempotent; also for tests). Returns the number of lights scaled. */
	int32 Dress(AActor* Pocket);
	static bool IsBayPocket(const AActor* A);

private:
	void HandleSpawned(AActor* Actor);
	FDelegateHandle SpawnHandle;
};
