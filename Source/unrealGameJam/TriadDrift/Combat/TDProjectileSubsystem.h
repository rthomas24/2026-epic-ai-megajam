// Triad Drift - Combat (Teammate 2). Projectile pool + live-projectile index.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDProjectileSubsystem.generated.h"

class ATDProjectile;

/**
 * Owns the projectile pool and the list of live projectiles, so flak, point defense, the shield dome,
 * portals and incoming-missile warnings can query them without GetAllActorsOfClass.
 * (Kill hit-stop lives in Core: UTDFeelSubsystem.)
 */
UCLASS()
class UNREALGAMEJAM_API UTDProjectileSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDProjectileSubsystem* Get(const UObject* WorldContext);

	/** Pooled projectile ready to Activate (hidden, ticking disabled). */
	ATDProjectile* Acquire();
	void Release(ATDProjectile* Projectile);

	void RegisterLive(ATDProjectile* Projectile);
	void UnregisterLive(ATDProjectile* Projectile);

	/** Every live projectile (do not hold onto the reference across frames). */
	const TArray<TObjectPtr<ATDProjectile>>& GetLiveProjectiles() const { return Live; }

	/** Live missiles (bIsMissile) hostile to Team within Radius of Location. Appends to Out. */
	void FindHostileMissiles(ETDTeam Team, const FVector& Location, float Radius, TArray<ATDProjectile*>& Out) const;

	/** Live missiles currently homing on Target. Appends to Out. */
	void FindMissilesHomingOn(const AActor* Target, TArray<ATDProjectile*>& Out) const;

	int32 GetLiveCount() const { return Live.Num(); }
	int32 GetPooledCount() const { return Pool.Num(); }

	/** Pool size cap; beyond it, released projectiles are destroyed. */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Combat")
	int32 MaxPooled = 256;

	// UWorldSubsystem
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDProjectile>> Pool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDProjectile>> Live;
};
