// Triad Drift - pure arcade flight simulation (ENG-01). Owner: Teammate 1 (Flight & Input).
//
// No UObjects, no world: the movement component feeds it input and delta time, and it returns the
// new orientation and velocity. That keeps the flight model deterministic and unit-testable
// (TriadDrift.Flight.* tests step it directly at any frame rate).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flight/TDFlightTypes.h"

/** Everything the flight model integrates. Plain data, copyable. */
struct UNREALGAMEJAM_API FTDFlightState
{
	FVector Location = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	FVector Velocity = FVector::ZeroVector;

	/** Engine speed along the nose (cm/s). Velocity converges to Nose * ForwardSpeed + strafe. */
	float ForwardSpeed = 0.f;

	/** Current local strafe velocity: X = right, Y = up (cm/s). */
	FVector2D StrafeVelocity = FVector2D::ZeroVector;

	/** Current body rates (deg/s): X = roll, Y = pitch (+ = nose up), Z = yaw (+ = nose right). */
	FVector AngularRate = FVector::ZeroVector;

	/** Boost meter 0..1. */
	float Boost01 = 1.f;
	/** Seconds since boost last drained (drives the regen delay). */
	float TimeSinceBoost = 1000.f;
	bool bBoosting = false;

	bool bDrifting = false;
	/** Direction of travel locked when the drift started. */
	FVector DriftDirection = FVector::ForwardVector;

	bool bOutOfBounds = false;
	bool bNearBoundary = false;

	/** Visual-only hull bank (deg), for the mesh and the camera. */
	float VisualBank = 0.f;

	/** Speed above target came from a drift burst (bleeds off at OverspeedDeceleration). */
	bool bBurstOverspeed = false;

	/** Set for one step when a drift is released (presentation burst hook). */
	bool bDriftBurstThisStep = false;
};

/** External modifiers set by gameplay (hyperdrive, ram, levels). */
struct UNREALGAMEJAM_API FTDFlightContext
{
	FVector BoundaryCenter = FVector::ZeroVector;
	bool bBoundaryEnabled = true;

	/** Box play space instead of the sphere (atmosphere zones: soft walls + ceiling). */
	bool bBoxBoundary = false;
	FBox BoundaryBox = FBox(ForceInit);

	/** Atmosphere flight (ENG-20): gravity, capped speed, no boost. */
	bool bAtmosphere = false;
	/** Downward pull in atmosphere (cm/s^2). Grip counters most of it, so the nose sags gently. */
	float AtmosphereGravity = 1200.f;
	/** Max speed in atmosphere as a fraction of the space CruiseSpeed. */
	float AtmosphereSpeedCapFraction = 0.6f;

	bool bSpeedOverride = false;
	float OverrideSpeed = 0.f;
	/** Multiplier on turn and roll rates while the override is active. */
	float OverrideSteeringScale = 1.f;
	/**
	 * AI catch-up allowance: when > CruiseSpeed, Throttle +1 maps to this speed instead of cruise
	 * (normal acceleration, no boost meter). 0 = off. Wingmen use it to hold formation on a faster leader.
	 */
	float ThrottleTopSpeed = 0.f;

	/** Acceleration used to reach the override speed (cm/s^2). */
	float OverrideAcceleration = 60000.f;
};

namespace TDFlightSim
{
	/** Speed the engine is heading for this step, given input and modifiers (ignores drift). */
	UNREALGAMEJAM_API float ComputeTargetSpeed(const FTDFlightState& State, const FTDFlightInput& Input, const FTDFlightTuning& Tuning, const FTDFlightContext& Context);

	/** Advances one step of exactly DeltaSeconds (no sub-stepping). */
	UNREALGAMEJAM_API void Step(FTDFlightState& State, const FTDFlightInput& Input, const FTDFlightTuning& Tuning, const FTDFlightContext& Context, float DeltaSeconds);

	/**
	 * Advances a whole frame, split into equal sub-steps no longer than Tuning.MaxSubstepSeconds, so
	 * 30 fps and 120 fps produce the same motion. Moves State.Location by the integrated velocity
	 * (the movement component then sweeps to resolve collisions). Returns the number of sub-steps.
	 */
	UNREALGAMEJAM_API int32 Advance(FTDFlightState& State, const FTDFlightInput& Input, const FTDFlightTuning& Tuning, const FTDFlightContext& Context, float DeltaSeconds);

	/** Clamps and sanitises an input (NaN-safe, axes to -1..1). */
	UNREALGAMEJAM_API FTDFlightInput Sanitize(const FTDFlightInput& Input);
}
