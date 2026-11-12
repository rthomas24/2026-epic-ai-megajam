// Triad Drift - Combat (Teammate 2). Every shot, rocket, missile, flak shell and mine (ENG-05, ENG-13).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TDProjectile.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UNiagaraComponent;
class UMaterialInstanceDynamic;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnProjectileDetonated, ATDProjectile*, Projectile, AActor*, DirectHit);

/**
 * A pooled, swept projectile. It has no collision of its own: each tick it sweeps a sphere along its
 * path on TDCollision::Projectile (so nothing tunnels at any speed or frame rate), skipping friendlies.
 * Damage goes only through UTDStatics::ApplyTDDamage. Spawn with ATDProjectile::Spawn, never SpawnActor.
 */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDProjectile : public AActor, public ITDTeam, public ITDDeflectable
{
	GENERATED_BODY()

public:
	ATDProjectile();

	/** Contract (ARCHITECTURE.md §4). Takes a projectile from the pool (or spawns one) and launches it along Transform's forward. */
	static ATDProjectile* Spawn(UWorld* World, const FTDProjectileSpec& Spec, const FTransform& Transform, AActor* Instigator, ETDTeam Team, AActor* HomingTarget = nullptr);

	/** Full launch: as Spawn, plus extra velocity (shooter's), a damage multiplier and hit reporting. */
	static ATDProjectile* Launch(UWorld* World, const FTDProjectileSpec& Spec, const FTransform& Transform, AActor* Instigator, ETDTeam Team,
		AActor* HomingTarget, const FVector& InheritedVelocity, float DamageMultiplier, bool bReportHits);

	// --- ITDTeam ---
	virtual ETDTeam GetTeam() const override { return Team; }

	// --- ITDDeflectable: flips team, redirects, re-homes on the original shooter. ---
	virtual void Deflect(AActor* NewInstigator, FVector NewDirection, ETDTeam NewTeam, float SpeedMultiplier) override;

	/** Destroys this projectile without damage (dome, EMP fizzle, point defense). bExplodeFX plays the impact FX. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Projectile")
	void Kill(bool bExplodeFX = true);

	/** Detonates here now: splash, shards, FX. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Projectile")
	void Detonate();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Projectile")
	void SetHomingTarget(AActor* NewTarget);

	/** Stop steering for Seconds (cloak, portal). Re-acquires afterwards only if bReacquire. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Projectile")
	void BreakTracking(float Seconds, bool bReacquire = false);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Projectile")
	void SetVelocity(const FVector& NewVelocity) { Velocity = NewVelocity; }

	/** Scales all damage this projectile (and its shards) deals from now on. Blade perfect deflect: x2. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Projectile")
	void SetDamageMultiplier(float NewMultiplier) { DamageMultiplier = FMath::Max(NewMultiplier, 0.f); }

	/** Stop homing entirely and fly straight (a normal deflect aimed at the crosshair: call right after Deflect). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Projectile")
	void ClearHoming() { Spec.bHoming = false; HomingTarget = nullptr; }

	/** Move without sweeping (portal transit). Keeps speed; NewDirection zero = keep direction. */
	void TeleportProjectile(const FVector& NewLocation, const FVector& NewDirection);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Projectile")
	bool IsActive() const { return bActive; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Projectile")
	bool IsMissile() const { return Spec.bIsMissile; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Projectile")
	bool WasDeflected() const { return DeflectCount > 0; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Projectile")
	AActor* GetHomingTarget() const { return HomingTarget.Get(); }

	/** Whoever fired it first (before any deflection). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Projectile")
	AActor* GetOriginalShooter() const { return OriginalShooter.Get(); }

	/** Current instigator (the deflector after a deflect). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Projectile")
	AActor* GetShooter() const { return Shooter.Get(); }

	virtual FVector GetVelocity() const override { return Velocity; }

	/**
	 * Where this projectile was at the start of its last step (its launch point before the first step, the
	 * arrival point right after a teleport). Portals and gates test Prev -> Cur against their plane with
	 * this, never Cur - Velocity * dt (a shot fired this frame hasn't flown a full step yet).
	 */
	FVector GetPreviousLocation() const { return PreviousLocation; }

	/** Unique per launch: pooled actors are reused, so anything keyed by actor must compare this too. */
	uint32 GetLaunchSerial() const { return LaunchSerial; }

	/** World time of this launch's last portal / gate transit (-1 = never). Reset on every launch. */
	double GetLastTransitTime() const { return LastTransitTime; }
	void SetLastTransitTime(double Time) { LastTransitTime = Time; }

	const FTDProjectileSpec& GetSpec() const { return Spec; }

	/** The placeholder streak (engine sphere stretched behind the shot) and the missile head; tests and diagnostics. */
	const UStaticMeshComponent* GetPlaceholderMesh() const { return PlaceholderMesh; }
	const UStaticMeshComponent* GetPlaceholderHead() const { return PlaceholderHead; }
	float GetDamageMultiplier() const { return DamageMultiplier; }
	float GetAge() const { return Age; }

	/** Fires when it detonates on impact, by fuse or at end of range (not on Kill). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Projectile")
	FTDOnProjectileDetonated OnDetonated;

	/** Advance by DeltaSeconds now (normally driven by Tick; tests and sub-frame catch-up call it directly). */
	void Step(float DeltaSeconds);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class UTDProjectileSubsystem;

	void Activate(const FTDProjectileSpec& InSpec, const FTransform& Transform, AActor* InInstigator, ETDTeam InTeam, AActor* InHomingTarget,
		const FVector& InheritedVelocity, float InDamageMultiplier, bool bInReportHits);
	void Deactivate();

	void UpdateHoming(float DeltaSeconds);
	void AcquireHomingTarget();
	bool ShootDownNearbyMissiles(float Radius);
	/** Sweep from Start to End; returns true and fills OutHit on the first valid hit (skipping friendlies). */
	bool SweepPath(const FVector& Start, const FVector& End, FHitResult& OutHit) const;
	void HandleImpact(const FHitResult& Hit);
	void DealDamage(AActor* Victim, float Amount, const FVector& FromLocation, const FVector& Direction);
	void ApplySplash(const FVector& Center, AActor* Exclude);
	void SpawnShards(const FVector& Origin, const FVector& Direction, AActor* Ignore);
	void ApplyKnockback(AActor* Victim) const;
	bool CanDamage(const AActor* Victim) const;
	void UpdatePlaceholderLook();
	/** Keeps the placeholder streak readable far from the camera (see FTDCombatPlaceholders::DistanceGrowth). */
	void UpdatePlaceholderDistanceScale();
	/** Placeholder impact flash, unless the cue the art would play there has content. */
	void PlayPlaceholderImpact(const FVector& Location, bool bCueHasArt) const;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USceneComponent> Root;

	/** Engine basic sphere, stretched along velocity. Hidden when Nick's tracer FX is set. */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlaceholderMID;

	/** Bright head on homing missiles (placeholder only). */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> PlaceholderHead;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeadMID;

	/** The streak in use (bStreak false when art replaces it). */
	FTDStreakLook StreakLook;
	/** Distance growth last applied (re-scaling is skipped while it changes by under 3%). */
	float AppliedGrowth = 1.f;

	/** Project glow material (UTDStatics::GetPlaceholderGlowMaterial, always cooked). Streaks fall back to the lit material without it. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowMaterial;

	/** One persistent tracer per pooled projectile: SetAsset + Activate on launch, Deactivate on death (ribbons finish). */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TracerComp;

	/** Engine sphere + basic material: restored on every launch so a reused actor never keeps another weapon's look. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	UPROPERTY(Transient)
	FTDProjectileSpec Spec;

	/** Turn rate a deflected shot gets while re-homing on its original shooter (returns must land). */
	static constexpr float DeflectHomingTurnRateDeg = 360.f;

	ETDTeam Team = ETDTeam::Neutral;
	uint32 LaunchSerial = 0;
	FVector PreviousLocation = FVector::ZeroVector;
	double LastTransitTime = -1.0;

	/** World time of the launch: the projectile's own Tick skips that frame (the weapon already stepped it). */
	double LaunchWorldTime = -1.0;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	float Lifetime = 1.f;
	float DamageMultiplier = 1.f;
	float TrackingBlockedFor = 0.f;
	bool bReacquireAfterBlock = false;
	/** BreakTracking(.., false): never pick a new target for the rest of this flight. */
	bool bTrackingAbandoned = false;
	bool bActive = false;
	bool bReportHits = false;
	int32 DeflectCount = 0;
	TWeakObjectPtr<AActor> Shooter;
	TWeakObjectPtr<AActor> OriginalShooter;
	TWeakObjectPtr<AActor> HomingTarget;
	/** Actor the shards of a split should not hit again. */
	TWeakObjectPtr<AActor> IgnoreActor;
};
