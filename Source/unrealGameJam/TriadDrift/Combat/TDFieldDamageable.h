// Triad Drift - destructible space field contract (Teammate 2 for T6's World/SpaceField). One actor may hold thousands of
// instanced rocks; shots and Lancer's ram reach the right instance through this interface, not through ApplyTDDamage.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDFieldDamageable.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UTDFieldDamageable : public UInterface
{
	GENERATED_BODY()
};

class UNREALGAMEJAM_API ITDFieldDamageable
{
	GENERATED_BODY()

public:
	/**
	 * A projectile hit this actor (called instead of UTDStatics::ApplyTDDamage). Hit carries the component and Item (the
	 * instance index); Damage has every multiplier applied. Instigator is the shooter (null for a world-owned shot).
	 */
	virtual void ReceiveProjectileHit(const FHitResult& Hit, float Damage, AActor* InInstigator, ETDDamageKind Kind) = 0;

	/**
	 * Lancer's ram swept From -> To with this radius this frame. Break what it smashes (small and medium); return true and
	 * set OutBlock when something too big stops the dash there (the ram stops like it does at a carrier hull).
	 */
	virtual bool ReceiveRamSweep(const FVector& From, const FVector& To, float Radius, AActor* InInstigator, FVector& OutBlock) = 0;

	/** An explosive shot detonated (missiles, rockets, mines): damage the pieces inside Radius of Center with falloff. */
	virtual void ReceiveSplash(const FVector& Center, float Radius, float Damage, AActor* InInstigator) = 0;
};

/** Field actors register here so the ram reaches them without searching the world. */
struct UNREALGAMEJAM_API FTDFieldDamage
{
	static void Register(AActor* Field);
	static void Unregister(AActor* Field);
	/** Live registered fields in World (pruned as it goes). */
	static void GetFields(const UWorld* World, TArray<AActor*>& Out);

	/** A player shot hit a field (the flight HUD's quiet hit marker): the field actor and where. */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFieldHit, AActor*, const FVector&);
	static FOnFieldHit OnPlayerFieldHit;
};
