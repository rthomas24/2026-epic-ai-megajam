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

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ApplyFlightTuning(const FTDFlightTuning& NewTuning);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	const FTDFlightTuning& GetFlightTuning() const { return Tuning; }

	// ---- Gameplay overrides -----------------------------------------------------------------------

	/** Forces a speed (hyperdrive, ram). SteeringScale multiplies turn/roll rates while active. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetSpeedOverride(float SpeedCmPerSec, float SteeringScale = 1.f);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearSpeedOverride();

	/** Acceleration used to reach the override speed; negative restores the default (60000 cm/s^2). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetOverrideAcceleration(float AccelerationCmPerSec2);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool HasSpeedOverride() const { return Context.bSpeedOverride; }

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

	/** Re-orients the flight state after a teleport. bKeepSpeed keeps the speed along the new facing. */
	void NotifyTeleported(const FQuat& NewFacing, bool bKeepSpeed);

	/**
	 * Adds an instant velocity change (knockback, explosions). The flight model's grip pulls the ship
	 * back onto its nose afterwards. Writing UMovementComponent::Velocity directly works too: external
	 * changes are picked up at the start of the next tick.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void AddImpulseVelocity(const FVector& DeltaVelocity);

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

	/** Drift released: velocity snapped to the nose with the burst. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnFlightEvent OnDriftReleased;

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
	int32 AppliedPlayBoundsVersion = -1;
	void ApplyWorldPlayBounds();
	bool bStateInitialised = false;
};
