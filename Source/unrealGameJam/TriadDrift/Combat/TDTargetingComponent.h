// Triad Drift - Combat (Teammate 2). Soft target, cycling, Bastion multi-lock, lead and off-screen data (ENG-06).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDTargetingComponent.generated.h"

class ATDProjectile;

// FTDTargetMarker and ETDMissileWarning live in Core/TDTypes.h (ITDHUDData exposes them).

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnTargetChanged, AActor*, NewTarget, AActor*, OldTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnTargetLocked, AActor*, Target, int32, LockCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnMissileWarningChanged, ETDMissileWarning, Warning);

/**
 * Lives on squad ships (works on any actor). Updates at UpdateHz from the combat registry: never iterates
 * the world. View = the player camera when player-controlled, else the owner's transform.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDTargetingComponent();

	// --- Contract (ARCHITECTURE.md §4) ---
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** Lock onto Target as if the player had cycled to it (ship switch carries the pick across the cut). Null clears the manual pick. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Targeting")
	void SetManualTarget(AActor* Target);

	/** True when the current target was picked by CycleTarget / SetManualTarget rather than the soft-target cone. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	bool IsManualTarget() const { return bManualTarget && CurrentTarget.IsValid(); }

	/** Next hostile by distance (nearest first, wrapping). Holds that target until it dies or leaves range. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Targeting")
	void CycleTarget();

	const TArray<AActor*>& GetLockedTargets() const { return LockedTargets; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting", meta = (DisplayName = "Get Locked Targets"))
	TArray<AActor*> K2_GetLockedTargets() const { return LockedTargets; }

	// --- Lead indicator ---
	/** Intercept point for the current target at ProjectileSpeed (primary weapon's by default). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	bool GetLeadIndicator(FVector& OutWorldLocation) const;

	/**
	 * Where to aim from ShooterLocation to hit a constant-velocity target. ShooterVelocity is the shooter's velocity when
	 * the shot inherits it (only its forward component along the aim adds to the shot's speed, as in ATDProjectile);
	 * pass zero otherwise. Falls back to the target's location when no intercept exists.
	 */
	static FVector ComputeInterceptPoint(const FVector& ShooterLocation, const FVector& ShooterVelocity, const FVector& TargetLocation,
		const FVector& TargetVelocity, float ProjectileSpeed);

	// --- HUD data ---
	const TArray<FTDTargetMarker>& GetMarkers() const { return Markers; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting", meta = (DisplayName = "Get Markers"))
	TArray<FTDTargetMarker> K2_GetMarkers() const { return Markers; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	ETDMissileWarning GetMissileWarning() const { return MissileWarning; }

	/** Seconds until the nearest homing missile reaches the owner (large if none). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	float GetNearestMissileSeconds() const { return NearestMissileSeconds; }

	/** World direction from the owner to the nearest incoming missile (zero if none). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	FVector GetNearestMissileDirection() const { return NearestMissileDirection; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	int32 GetIncomingMissileCount() const { return IncomingMissileCount; }

	// --- Bastion multi-lock (driven by the Missile Swarm power) ---
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Targeting")
	void BeginLockPainting();

	/** Stops painting; locks stay until ClearLocks (the swarm consumes them). Returns the locks. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Targeting")
	TArray<AActor*> EndLockPainting();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Targeting")
	void ClearLocks();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	bool IsPainting() const { return bPainting; }

	/** 0..1 progress on the target currently being painted. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	float GetPaintProgress01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Targeting")
	AActor* GetPaintCandidate() const { return PaintCandidate.Get(); }

	/** Force an immediate re-evaluation (tests; after a ship switch). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Targeting")
	void RefreshNow();

	/** Override the view (tests, cinematic cameras). Cleared with ClearViewOverride. */
	void SetViewOverride(const FVector& Location, const FRotator& Rotation);
	void ClearViewOverride() { bHasViewOverride = false; }

	/** Angle (degrees) between the view forward and the direction to Location. */
	float GetAngleFromView(const FVector& Location) const;

	// --- Tuning (ENG-06 values) ---
	/** Soft target: closest to screen centre within this cone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "Degrees"))
	float SoftTargetConeDeg = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "cm"))
	float MaxTargetRange = 40000.f;

	/** CycleTarget picks among hostiles in this cone (front hemisphere by default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "Degrees"))
	float CycleConeDeg = 90.f;

	/** Keep the soft target while it stays inside this wider cone (less flicker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "Degrees"))
	float SoftTargetKeepConeDeg = 30.f;

	/** A new candidate must be this many degrees closer to centre to steal the soft target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "Degrees"))
	float SoftTargetSwitchMarginDeg = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting")
	int32 MaxLocks = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "s"))
	float SecondsPerLock = 0.25f;

	/** Cone the paint candidate must stay inside. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "Degrees"))
	float LockConeDeg = 25.f;

	/** Projectile speed for the lead indicator. 0 = use the owner's primary weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "CentimetersPerSecond"))
	float LeadProjectileSpeedOverride = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "s"))
	float MissileImminentSeconds = 1.5f;

	/** Max HUD markers (nearest first). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting")
	int32 MaxMarkers = 24;

	/** Scan rate for target selection and markers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Targeting", meta = (Units = "Hertz"))
	float UpdateHz = 30.f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Targeting")
	FTDOnTargetChanged OnTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Targeting")
	FTDOnTargetLocked OnTargetLocked;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Targeting")
	FTDOnMissileWarningChanged OnMissileWarningChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void GatherHostiles();
	void UpdateSoftTarget();
	void UpdatePainting(float DeltaTime);
	void UpdateMissileWarning();
	void UpdateMarkers();
	void SetCurrentTarget(AActor* NewTarget);
	void PruneLocks();
	void Refresh();
	bool IsValidTarget(const AActor* Actor) const;
	void GetView(FVector& OutLocation, FRotator& OutRotation) const;
	float GetLeadSpeed() const;
	ETDTeam GetOwnerTeam() const;

	TWeakObjectPtr<AActor> CurrentTarget;
	bool bManualTarget = false;

	/** UPROPERTY so GC nulls destroyed actors between refreshes. */
	UPROPERTY(Transient)
	TArray<AActor*> Hostiles;
	/** Mirror of LockedWeak, rebuilt every tick after pruning dead targets (safe to hand out by reference). */
	UPROPERTY(Transient)
	TArray<AActor*> LockedTargets;
	TArray<TWeakObjectPtr<AActor>> LockedWeak;
	TArray<FTDTargetMarker> Markers;
	UPROPERTY(Transient)
	TArray<ATDProjectile*> MissileScratch;

	bool bPainting = false;
	TWeakObjectPtr<AActor> PaintCandidate;
	float PaintProgress = 0.f;

	ETDMissileWarning MissileWarning = ETDMissileWarning::None;
	float NearestMissileSeconds = 1000.f;
	FVector NearestMissileDirection = FVector::ZeroVector;
	int32 IncomingMissileCount = 0;

	float UpdateAccumulator = 0.f;

	bool bHasViewOverride = false;
	FVector ViewOverrideLocation = FVector::ZeroVector;
	FRotator ViewOverrideRotation = FRotator::ZeroRotator;
};
