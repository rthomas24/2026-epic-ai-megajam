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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnDodge, float, Direction);

/** Where the ship is relative to the play space (sphere in space, box in an atmosphere zone). */
UENUM(BlueprintType)
enum class ETDBoundaryState : uint8
{
	Inside,
	/** In the warning band just inside the edge (BoundaryWarnDistance). */
	Warning,
	/** Past the edge: the nose is being turned back and the ship pushed in. */
	Outside
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnBoundaryStateChanged, ETDBoundaryState, NewState);

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
	void SetBoxBoundary(const FBox& Box, float WallWarnDistance = -1.f);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearBoxBoundary();

	/** Atmosphere flight (ENG-20): gravity, speed capped at a fraction of space cruise, no boost. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetAtmosphereMode(bool bEnabled, float Gravity = 1200.f, float SpeedCapFraction = 0.6f);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsInAtmosphere() const { return Context.bAtmosphere; }

	/** Multiplier on turn and roll rates right now: 1 with no speed override, else the override's (the ram's 0.4). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetActiveSteeringScale() const { return Context.bSpeedOverride ? FMath::Max(0.f, Context.OverrideSteeringScale) : 1.f; }
	/** Plain top speed right now: cruise in space, cruise x the atmosphere cap fraction in atmosphere. */
	float GetCruiseCapSpeed() const { return Context.bAtmosphere ? Tuning.CruiseSpeed * Context.AtmosphereSpeedCapFraction : Tuning.CruiseSpeed; }

	/** Centre of the current spherical play space. */
	FVector GetBoundaryCenter() const { return Context.BoundaryCenter; }

	/** Re-orients the flight state after a teleport. bKeepSpeed keeps the speed along the new facing. */
	void NotifyTeleported(const FQuat& NewFacing, bool bKeepSpeed);

	// ---- Silent-move watchdog ----------------------------------------------------------------------
	/**
	 * Moves of the ship over SilentMoveDistance that are neither its own flight step nor an announced teleport
	 * (ATDSpacecraft::TeleportSpacecraft) are counted; for the player's ship a warning with the mover's callstack is
	 * logged (dev builds). Tests: TestEqual(TEXT("no silent teleport"), Ship->GetShipMovement()->GetSilentMoveCount(), 0).
	 */
	int32 GetSilentMoveCount() const { return SilentMoveCount; }
	float GetLargestSilentMove() const { return LargestSilentMove; }

	/** Wrap a deliberate placement (checkpoint snap) so the watchdog doesn't count it. */
	void SetMoveAnnounced(bool bAnnounced) { bMoveAnnounced = bAnnounced; if (!bAnnounced && UpdatedComponent) { WatchLastLocation = UpdatedComponent->GetComponentLocation(); } }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Flight", meta = (ClampMin = "0"))
	float SilentMoveDistance = 5000.f;

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
	/** Pickups (T2: Boost from special rocks): add to the boost meter, clamped 0..1. Returns the amount actually added.
	 *  The caller decides who gets it (the flown, living ship). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	float AddBoost01(float Amount) { const float Before = State.Boost01; State.Boost01 = FMath::Clamp(State.Boost01 + Amount, 0.f, 1.f); return State.Boost01 - Before; }

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

	/** A dodge started (Direction -1 left / +1 right): thruster burst cue, camera kick. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnDodge OnDodge;

	/**
	 * Lateral dodge burst (-1 left / +1 right, the ship's own right). Refused while on cooldown, parked, or under a
	 * speed override (ram, landing assist, atmosphere entry, hyperdrive jump). ATDShip::TryDodge adds the ship gates.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	bool RequestDodge(float Direction);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool CanDodge() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsDodging() const { return State.DodgeTimeLeft > 0.f; }

	/** 0 right after a dodge, 1 when the next one is ready (HUD pip). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetDodgeReady01() const { return Tuning.DodgeCooldown > 0.f ? FMath::Clamp(State.TimeSinceDodge / Tuning.DodgeCooldown, 0.f, 1.f) : 1.f; }

	/** Fires once per change of GetBoundaryState (presentation: warning sting, HUD flash). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnBoundaryStateChanged OnBoundaryStateChanged;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	ETDBoundaryState GetBoundaryState() const
	{
		return State.bOutOfBounds ? ETDBoundaryState::Outside : (State.bNearBoundary ? ETDBoundaryState::Warning : ETDBoundaryState::Inside);
	}

	/**
	 * Player-facing boundary text for the HUD (ITDHUDData): "EDGE OF THE COMBAT ZONE" in the warning band,
	 * "LEAVING THE COMBAT ZONE - TURN BACK" past the edge ("FLIGHT AREA" in an atmosphere zone), empty inside.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	FText GetBoundaryWarningText() const;

	/**
	 * Terrain for the ground floor, terrain look-ahead and landing paths: anything static, plus large movable
	 * geometry (a planet course built from movable BlockAll meshes is WorldDynamic). Small movers (projectiles,
	 * debris) and pawns never count.
	 */
	static FCollisionObjectQueryParams TerrainObjectTypes();
	static bool IsTerrainHit(const FHitResult& Hit);
	/** First terrain hit of a sphere sweep (Radius 0 = line), or false. */
	static bool SweepTerrain(const UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, float Radius, const FCollisionQueryParams& Params);

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
	/** In atmosphere: trace the ground below / ahead for the flight model's soft floor and terrain look-ahead. */
	void MeasureGroundClearance();
	void HandleTransformUpdated(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport);
	FVector WatchLastLocation = FVector::ZeroVector;
	bool bWatchReady = false;
	bool bInOwnMove = false;
	bool bMoveAnnounced = false;
	int32 SilentMoveCount = 0;
	float LargestSilentMove = 0.f;
	void ResolveMove(const FVector& Delta, float DeltaTime);
	void HandleShipImpact(const FHitResult& Hit, float DeltaTime);
	/** Resting contact: swing the nose along the surface and keep a minimum slide speed (never stuck at 0 m/s). */
	void SlideOffSurface(const FVector& Normal, float DeltaTime);
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
