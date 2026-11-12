// Triad Drift - Combat (Teammate 2). Projectile pool + live-projectile index + kill hit-stop.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDProjectileSubsystem.generated.h"

class ATDProjectile;

/**
 * Owns the projectile pool and the list of live projectiles, so flak, point defense, the shield dome,
 * portals and incoming-missile warnings can query them without GetAllActorsOfClass.
 * Also runs the 40 ms kill hit-stop.
 */
UCLASS()
class UNREALGAMEJAM_API UTDProjectileSubsystem : public UTickableWorldSubsystem
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

	/** Freeze the game for Seconds of real time (kill confirm). Ignored while another time dilation is active. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Combat")
	void RequestHitStop(float Seconds = 0.04f);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Combat")
	bool IsHitStopActive() const { return bHitStopActive; }

	/** Count of hit-stops requested (tests). */
	int32 GetHitStopRequests() const { return HitStopRequests; }

	/** Global switch (tests and accessibility). */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Combat")
	bool bHitStopEnabled = true;

	/** Time dilation used while hit-stopped (not 0: keeps tick deltas sane). */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Combat")
	float HitStopDilation = 0.02f;

	/** Pool size cap; beyond it, released projectiles are destroyed. */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Combat")
	int32 MaxPooled = 256;

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void EndHitStop();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDProjectile>> Pool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDProjectile>> Live;

	bool bHitStopActive = false;
	double HitStopEndRealTime = 0.0;
	int32 HitStopRequests = 0;
};
