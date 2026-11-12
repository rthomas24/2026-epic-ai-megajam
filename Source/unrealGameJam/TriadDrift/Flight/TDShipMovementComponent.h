// Triad Drift - ship movement component (ENG-01). Owner: Teammate 1 (Flight & Input).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flight/TDFlightTypes.h"
#include "TriadDrift/Flight/TDFlightSim.h"
#include "TDShipMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnFlightImpact, const FTDFlightImpact&, Impact);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnFlightEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnDriftEnded, bool, bBurst);

/**
 * Arcade 6DOF-lite flight: custom velocity integration (TDFlightSim), swept movement with glancing /
 * head-on collision response, soft spherical play-space boundary. Driven only by SetFlightInput, so
 * the player controller and AI controllers fly ships the same way. Velocity is never reset by
 * possession changes.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDShipMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UTDShipMovementComponent();

	// ---- Input & tuning -------------------------------------------------------------------------

	/** Latches the input: it stays applied until replaced (call every tick or on change, both work). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetFlightInput(const FTDFlightInput& In);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	const FTDFlightInput& GetFlightInput() const { return Input; }

	/**
	 * New tuning (per-ship spec). If the ship is cruising faster than the new cruise speed (e.g. a fresh
	 * spawn at the default 6000 handed a slow carrier spec) its speed is clamped down at once, so it doesn't
	 * lurch. Boost, drift bursts, speed overrides and the catch-up allowance are left alone.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ApplyFlightTuning(const FTDFlightTuning& NewTuning);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	const FTDFlightTuning& GetFlightTuning() const { return Tuning; }

	// ---- Gameplay overrides -----------------------------------------------------------------------

	/**
	 * Keyed speed override (hyperdrive "Hyperdrive", atmospheric entry "AtmosphereEntry", T2's ram "Ram",
	 * enemy AI...). Requests stack: the most recent request is the one flown; setting an existing owner
	 * updates it and moves it to the top. Clearing an owner removes only its request, so the previous
	 * owner's override is restored rather than everything being cleared.
	 * Steering multiplies turn/roll rates; Acceleration < 0 uses the default (60000 cm/s^2).
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetSpeedOverrideFor(FName Owner, float SpeedCmPerSec, float SteeringScale = 1.f, float AccelerationCmPerSec2 = -1.f);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearSpeedOverrideFor(FName Owner);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool HasSpeedOverrideFor(FName Owner) const;

	/** Owner of the override currently flown (NAME_None = the unkeyed legacy request, or none at all). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	FName GetActiveSpeedOverrideOwner() const { return OverrideStack.Num() > 0 ? OverrideStack.Last().Owner : NAME_None; }

	/** Legacy unkeyed override: the request owned by NAME_None in the stack. Prefer the keyed calls. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetSpeedOverride(float SpeedCmPerSec, float SteeringScale = 1.f);

	/** Clears only the unkeyed (NAME_None) request; keyed owners keep theirs. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearSpeedOverride();

	/** Acceleration for the unkeyed request (now and future); negative restores the default (60000 cm/s^2). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetOverrideAcceleration(float AccelerationCmPerSec2);

	/** Any override active (any owner). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool HasSpeedOverride() const { return OverrideStack.Num() > 0; }

	/** Ends a boost and a drift now, with their end events (parking, death). Velocity untouched. */
	void EndBoostAndDrift();

	/** Drops every override request (reset / respawn). */
	void ClearAllSpeedOverrides();

	/**
	 * AI catch-up allowance (wingman formation): Throttle +1 flies at TopSpeed instead of cruise, with
	 * normal acceleration and no boost meter. Values <= CruiseSpeed clear it. Player ships shouldn't use it.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetThrottleTopSpeed(float TopSpeedCmPerSec) { Context.ThrottleTopSpeed = FMath::Max(0.f, TopSpeedCmPerSec); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearThrottleTopSpeed() { Context.ThrottleTopSpeed = 0.f; }

	/** Speed Throttle +1 currently flies at (cruise, or the catch-up allowance). Map Throttle over MinSpeed..this. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetThrottleTopSpeed() const { return FMath::Max(Tuning.CruiseSpeed, Context.ThrottleTopSpeed); }

	/** Centre of the soft spherical play space (radius is FTDFlightTuning::BoundaryRadius). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetBoundary(const FVector& Center, float Radius, bool bEnabled = true);

	/** Box play space (atmosphere zones): soft walls and ceiling. Clear returns to the sphere. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetBoxBoundary(const FBox& Box);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearBoxBoundary();

	/** Atmosphere flight (ENG-20): gravity, speed capped at a fraction of space cruise, no boost. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetAtmosphereMode(bool bEnabled, float Gravity = 1200.f, float SpeedCapFraction = 0.6f);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsInAtmosphere() const { return Context.bAtmosphere; }

	/** Centre of the current spherical play space. */
	FVector GetBoundaryCenter() const { return Context.BoundaryCenter; }

	/** Re-orients the flight state after a teleport. bKeepSpeed keeps the speed along the new facing. */
	void NotifyTeleported(const FQuat& NewFacing, bool bKeepSpeed);

	/**
	 * Adds an instant velocity change (knockback, explosions). The flight model's grip pulls the ship
	 * back onto its nose afterwards. Writing UMovementComponent::Velocity directly works too: external
	 * changes are picked up at the start of the next tick.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void AddImpulseVelocity(const FVector& DeltaVelocity);

	/**
	 * Back to a clean flight state: input cleared, boost meter full, no drift / boost / override / catch-up
	 * allowance, angular rates zeroed. Velocity is kept or reset to StartSpeedFraction * cruise along the nose.
	 * Call on ship down and respawn (T3). Fires the matching end events if boost or drift were active.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ResetFlightState(bool bKeepVelocity = false);

	/** Sets the full velocity instantly (switch hand-offs, scripted moves). Engine speed follows its length. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetVelocityImmediate(const FVector& NewVelocity);

	/** Sets speed instantly along the current nose (spawns, respawns, cutscene hand-offs). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetForwardSpeedImmediate(float SpeedCmPerSec);

	// ---- State for gameplay, HUD and presentation --------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetSpeed() const { return Velocity.Size(); }

	/** Speed mapped MinSpeed..BoostSpeed to 0..1 (engine audio, FOV, trails). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetSpeed01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetBoost01() const { return State.Boost01; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsBoosting() const { return State.bBoosting; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsDrifting() const { return State.bDrifting; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsOutOfBounds() const { return State.bOutOfBounds; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsNearBoundary() const { return State.bNearBoundary; }

	/** Body rates in deg/s (X roll, Y pitch, Z yaw). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	FVector GetAngularRate() const { return State.AngularRate; }

	/** Visual-only bank angle (deg) the hull and chase camera use. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetVisualBank() const { return State.VisualBank; }

	const FTDFlightState& GetFlightState() const { return State; }

	// ---- Events ---------------------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnFlightImpact OnFlightImpact;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnFlightEvent OnBoostStarted;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnFlightEvent OnBoostEnded;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnFlightEvent OnDriftStarted;

	/**
	 * Every drift end, so it always pairs with OnDriftStarted. bBurst = the pilot earned the +20% release
	 * burst (false for tap-drifts, or when a speed override cut the drift short).
	 */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnDriftEnded OnDriftReleased;

	// ---- UMovementComponent --------------------------------------------------------------------------
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual float GetMaxSpeed() const override;
	virtual void StopMovementImmediately() override;

protected:
	virtual void BeginPlay() override;

	/** Initial tuning (features.md ENG-01 defaults). Per-ship values arrive through ApplyFlightTuning. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Flight")
	FTDFlightTuning Tuning;

	/** Speed the ship starts at on BeginPlay (fraction of cruise). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Flight", meta = (ClampMin = "0"))
	float StartSpeedFraction = 1.f;

private:
	void ResolveMove(const FVector& Delta, float DeltaTime);
	void HandleShipImpact(const FHitResult& Hit, float DeltaTime);
	void SyncStateFromComponent();
	void BroadcastStateChanges(const FTDFlightState& Before);

	FTDFlightInput Input;
	FTDFlightState State;
	FTDFlightContext Context;
	/** Velocity we wrote last tick; a mismatch means someone outside changed Velocity (knockback). */
	FVector LastWrittenVelocity = FVector::ZeroVector;
	double LastCollisionDamageTime = -1000.0;
	TWeakObjectPtr<AActor> LastImpactActor;
	double LastImpactTime = -1000.0;
	struct FSpeedOverrideRequest
	{
		FName Owner;
		float Speed = 0.f;
		float Steering = 1.f;
		float Acceleration = -1.f;
	};
	/** Override requests, oldest first; the last one is flown. */
	TArray<FSpeedOverrideRequest> OverrideStack;
	float LegacyOverrideAcceleration = -1.f;
	void ApplyOverrideStack();

	int32 AppliedPlayBoundsVersion = -1;
	bool bWorldBoundsApplied = false;
	void ApplyWorldPlayBounds();
	bool bStateInitialised = false;
};
