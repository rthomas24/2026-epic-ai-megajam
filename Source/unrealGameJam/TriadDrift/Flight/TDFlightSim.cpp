// Triad Drift - pure arcade flight simulation (ENG-01). Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDFlightSim.h"

namespace
{
	/** Frame-rate independent exponential blend factor. */
	FORCEINLINE float TDFlightSim_ExpAlpha(float Rate, float DeltaSeconds)
	{
		return Rate > 0.f ? 1.f - FMath::Exp(-Rate * DeltaSeconds) : 0.f;
	}

	FORCEINLINE float TDFlightSim_SafeAxis(float V)
	{
		return FMath::IsFinite(V) ? FMath::Clamp(V, -1.f, 1.f) : 0.f;
	}

	/** Longest frame we integrate; longer hitches are clamped so the ship can't tunnel or explode. */
	constexpr float TDFlightSim_MaxFrameSeconds = 0.25f;
}

FTDFlightInput TDFlightSim::Sanitize(const FTDFlightInput& In)
{
	FTDFlightInput Out = In;
	Out.Throttle = TDFlightSim_SafeAxis(In.Throttle);
	Out.Aim = FVector2D(TDFlightSim_SafeAxis(In.Aim.X), TDFlightSim_SafeAxis(In.Aim.Y));
	Out.Roll = TDFlightSim_SafeAxis(In.Roll);
	Out.Strafe = FVector2D(TDFlightSim_SafeAxis(In.Strafe.X), TDFlightSim_SafeAxis(In.Strafe.Y));
	return Out;
}

float TDFlightSim::ComputeTargetSpeed(const FTDFlightState& State, const FTDFlightInput& In, const FTDFlightTuning& Tuning, const FTDFlightContext& Context)
{
	if (Context.bSpeedOverride)
	{
		return FMath::Max(0.f, Context.OverrideSpeed);
	}
	float Target;
	if (State.bBoosting)
	{
		Target = Tuning.BoostSpeed;
	}
	else if (In.bBrake)
	{
		Target = Tuning.MinSpeed;
	}
	else
	{
		const float Throttle01 = (TDFlightSim_SafeAxis(In.Throttle) + 1.f) * 0.5f;
		const float Top = FMath::Max(Tuning.CruiseSpeed, Context.ThrottleTopSpeed);
		Target = FMath::Lerp(Tuning.MinSpeed, Top, Throttle01);
	}
	if (Context.bAtmosphere)
	{
		const float Cap = Tuning.CruiseSpeed * Context.AtmosphereSpeedCapFraction;
		Target = FMath::Min(Target, Cap);
	}
	return Target;
}

void TDFlightSim::Step(FTDFlightState& S, const FTDFlightInput& In, const FTDFlightTuning& T, const FTDFlightContext& C, float Dt)
{
	if (Dt <= 0.f)
	{
		return;
	}

	S.bDriftBurstThisStep = false;
	S.bDriftEndedThisStep = false;
	const float SteerScale = C.bSpeedOverride ? FMath::Max(0.f, C.OverrideSteeringScale) : 1.f;

	// ---- Drift start (before rotation so the locked direction is the pre-turn travel direction) ---
	const bool bWantDrift = In.bDrift && !C.bSpeedOverride;
	if (bWantDrift && !S.bDrifting)
	{
		S.bDrifting = true;
		S.DriftDirection = S.Velocity.SizeSquared() > 1.f ? S.Velocity.GetSafeNormal() : S.Rotation.GetForwardVector();
		S.DriftStartForward = S.Rotation.GetForwardVector();
		S.DriftTime = 0.f;
	}
	if (S.bDrifting)
	{
		S.DriftTime += Dt;
	}

	// ---- Boost meter ---------------------------------------------------------------------------------
	if (!In.bBoost)
	{
		S.bBoostLockout = false; // released: an empty meter may start again once it refills
	}
	const bool bWantBoost = In.bBoost && !S.bDrifting && !C.bSpeedOverride && !C.bAtmosphere;
	const bool bCanBoost = !S.bBoostLockout && S.Boost01 > 0.f && (S.bBoosting || S.Boost01 >= T.BoostMinToStart);
	S.bBoosting = bWantBoost && bCanBoost;
	if (S.bBoosting)
	{
		S.Boost01 = FMath::Max(0.f, S.Boost01 - Dt / FMath::Max(T.BoostDuration, KINDA_SMALL_NUMBER));
		S.TimeSinceBoost = 0.f;
		if (S.Boost01 <= 0.f)
		{
			S.bBoosting = false; // meter empty: must release and wait for BoostMinToStart
			S.bBoostLockout = In.bBoost;
		}
	}
	else
	{
		S.TimeSinceBoost += Dt;
		if (S.TimeSinceBoost >= T.BoostRegenDelay)
		{
			S.Boost01 = FMath::Min(1.f, S.Boost01 + Dt / FMath::Max(T.BoostRegenTime, KINDA_SMALL_NUMBER));
		}
	}

	// ---- Rotation --------------------------------------------------------------------------------
	const float TurnCap = T.TurnRate * (S.bDrifting ? T.DriftTurnMultiplier : 1.f) * SteerScale;
	const float RollCap = T.RollRate * SteerScale;

	FVector Forward = S.Rotation.GetForwardVector();
	const bool bNoseVertical = FMath::Abs(Forward.Z) > T.VerticalNoseLimit;

	float TargetRoll = In.Roll * RollCap;
	if (FMath::Abs(In.Roll) < 0.05f && T.AutoLevelRate > 0.f && !bNoseVertical)
	{
		const float Bank = S.Rotation.Rotator().Roll; // + = rolled right
		TargetRoll = FMath::Clamp(-Bank * T.AutoLevelGain, -T.AutoLevelRate, T.AutoLevelRate);
	}

	const FVector TargetRates(TargetRoll, In.Aim.Y * TurnCap, In.Aim.X * TurnCap);
	const float AngAlpha = TDFlightSim_ExpAlpha(T.AngularResponse, Dt);
	S.AngularRate = FMath::Lerp(S.AngularRate, TargetRates, AngAlpha);
	// Hard caps (drift release or steering-scale changes can leave the rate above the new cap).
	S.AngularRate.X = FMath::Clamp(S.AngularRate.X, -RollCap, RollCap);
	S.AngularRate.Y = FMath::Clamp(S.AngularRate.Y, -TurnCap, TurnCap);
	S.AngularRate.Z = FMath::Clamp(S.AngularRate.Z, -TurnCap, TurnCap);

	const FRotator LocalDelta(S.AngularRate.Y * Dt, S.AngularRate.Z * Dt, S.AngularRate.X * Dt);
	S.Rotation = (S.Rotation * FQuat(LocalDelta)).GetNormalized();

	// ---- Boundary: steer the nose back toward the centre --------------------------------------------
	float BoundaryStrength = 0.f;
	FVector ToCenter = FVector::ZeroVector;
	S.bOutOfBounds = false;
	S.bNearBoundary = false;
	if (C.bBoundaryEnabled && C.bBoxBoundary && C.BoundaryBox.IsValid)
	{
		const FVector Closest = C.BoundaryBox.GetClosestPointTo(S.Location);
		const float Outside = FVector::Dist(Closest, S.Location);
		S.bNearBoundary = Outside > 0.f || !C.BoundaryBox.ExpandBy(-T.BoundaryWarnDistance).IsInsideOrOn(S.Location);
		if (Outside > KINDA_SMALL_NUMBER)
		{
			S.bOutOfBounds = true;
			ToCenter = (Closest - S.Location) / Outside;
			const float Depth01 = FMath::Clamp(Outside / T.BoundaryFullStrengthDepth, 0.f, 1.f);
			BoundaryStrength = 0.25f + 0.75f * Depth01;
		}
	}
	else if (C.bBoundaryEnabled && (C.BoundaryRadius > 0.f ? C.BoundaryRadius : T.BoundaryRadius) > 0.f)
	{
		const float Radius = C.BoundaryRadius > 0.f ? C.BoundaryRadius : T.BoundaryRadius;
		const FVector Offset = S.Location - C.BoundaryCenter;
		const float Dist = Offset.Size();
		S.bNearBoundary = Dist > Radius - T.BoundaryWarnDistance;
		if (Dist > Radius)
		{
			S.bOutOfBounds = true;
			ToCenter = -Offset / Dist;
			const float Depth01 = FMath::Clamp((Dist - Radius) / T.BoundaryFullStrengthDepth, 0.f, 1.f);
			BoundaryStrength = 0.25f + 0.75f * Depth01; // gentle at the edge, firm deeper out
		}
	}
	if (BoundaryStrength > 0.f)
	{
		Forward = S.Rotation.GetForwardVector();
		const float CosAngle = FMath::Clamp(FVector::DotProduct(Forward, ToCenter), -1.f, 1.f);
		const float Angle = FMath::Acos(CosAngle);
		if (Angle > KINDA_SMALL_NUMBER)
		{
			FVector Axis = FVector::CrossProduct(Forward, ToCenter);
			if (!Axis.Normalize())
			{
				Axis = S.Rotation.GetUpVector(); // pointing straight out: turn around the up axis
			}
			const float MaxStep = FMath::DegreesToRadians(T.BoundarySteerRate * BoundaryStrength) * Dt;
			S.Rotation = (FQuat(Axis, FMath::Min(Angle, MaxStep)) * S.Rotation).GetNormalized();
		}
	}

	Forward = S.Rotation.GetForwardVector();
	const FVector Right = S.Rotation.GetRightVector();
	const FVector Up = S.Rotation.GetUpVector();

	// ---- Drift release: snap travel to the nose with a burst ------------------------------------------
	// The burst never takes the ship past boost speed + burst, and needs a real drift (time or swing).
	const float BurstCap = T.BoostSpeed * (1.f + T.DriftReleaseBurst);
	if (!bWantDrift && S.bDrifting)
	{
		S.bDrifting = false;
		S.bDriftEndedThisStep = true;
		const float Swing = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(S.DriftStartForward, Forward), -1.f, 1.f)));
		const bool bPilotReleased = !In.bDrift; // an override starting mid-drift ends it without a burst
		const bool bEarned = S.DriftTime >= T.DriftBurstMinSeconds || Swing >= T.DriftBurstMinAngle;
		const float DriftSpeed = S.Velocity.Size();
		S.StrafeVelocity = FVector2D::ZeroVector;
		if (bPilotReleased && bEarned)
		{
			S.bDriftBurstThisStep = true;
			const float BurstSpeed = FMath::Min(DriftSpeed * (1.f + T.DriftReleaseBurst), FMath::Max(BurstCap, DriftSpeed));
			S.ForwardSpeed = BurstSpeed;
			S.bBurstOverspeed = true;
			S.Velocity = Forward * BurstSpeed;
		}
		else
		{
			// No burst: travel still snaps to the nose (that's the drift's exit), at the same speed.
			S.ForwardSpeed = DriftSpeed;
			S.Velocity = Forward * DriftSpeed;
		}
	}

	// ---- Speed and velocity -------------------------------------------------------------------------
	if (S.bDrifting)
	{
		// Velocity keeps its direction; only drag and the brake change its length.
		float DriftSpeed = S.Velocity.Size();
		if (DriftSpeed > BurstCap)
		{
			// Drifting out of a hyperdrive jump or ram: bleed the excess hard, like any other overspeed.
			DriftSpeed = FMath::FInterpConstantTo(DriftSpeed, BurstCap, Dt, T.HighOverspeedDeceleration);
		}
		else
		{
			DriftSpeed = FMath::Max(0.f, DriftSpeed - T.DriftDrag * Dt);
		}
		if (In.bBrake)
		{
			DriftSpeed = FMath::FInterpConstantTo(DriftSpeed, T.MinSpeed, Dt, T.BrakeDeceleration);
		}
		S.Velocity = S.DriftDirection * DriftSpeed;
		S.ForwardSpeed = DriftSpeed;
		S.StrafeVelocity = FVector2D::ZeroVector;
	}
	else
	{
		const float Target = ComputeTargetSpeed(S, In, T, C);
		float Rate;
		if (Target > S.ForwardSpeed)
		{
			Rate = C.bSpeedOverride ? C.OverrideAcceleration : (S.bBoosting ? T.BoostAcceleration : T.Acceleration);
		}
		else if (C.bSpeedOverride)
		{
			Rate = C.OverrideAcceleration;
		}
		else if (In.bBrake)
		{
			Rate = T.BrakeDeceleration;
		}
		else
		{
			// After a drift burst the extra speed bleeds off slowly so the burst is felt; anything above
			// the burst cap (or above boost speed otherwise) drops fast.
			if (S.ForwardSpeed > BurstCap)
			{
				Rate = T.HighOverspeedDeceleration;
			}
			else if (S.bBurstOverspeed)
			{
				Rate = T.OverspeedDeceleration;
			}
			else
			{
				Rate = S.ForwardSpeed > T.BoostSpeed ? T.HighOverspeedDeceleration : T.Deceleration;
			}
		}
		S.ForwardSpeed = FMath::FInterpConstantTo(S.ForwardSpeed, Target, Dt, Rate);
		if (S.ForwardSpeed <= Target + 1.f || S.bBoosting || In.bBrake)
		{
			S.bBurstOverspeed = false;
		}

		const FVector2D StrafeTarget = In.Strafe * T.StrafeSpeed;
		S.StrafeVelocity = FMath::Vector2DInterpConstantTo(S.StrafeVelocity, StrafeTarget, Dt, T.StrafeAcceleration);

		const FVector Desired = Forward * S.ForwardSpeed + Right * S.StrafeVelocity.X + Up * S.StrafeVelocity.Y;
		S.Velocity = FMath::Lerp(S.Velocity, Desired, TDFlightSim_ExpAlpha(T.VelocityGrip, Dt));
	}

	// Atmosphere: gravity pulls down (grip mostly counters it, so the ship sags rather than falls).
	if (C.bAtmosphere && !C.bSpeedOverride)
	{
		S.Velocity.Z -= C.AtmosphereGravity * Dt;
	}

	// Boundary push-back on top of whatever the pilot is doing.
	if (BoundaryStrength > 0.f)
	{
		S.Velocity += ToCenter * (T.BoundaryPushAcceleration * BoundaryStrength * Dt);
		if (S.bDrifting)
		{
			S.DriftDirection = S.Velocity.GetSafeNormal(UE_SMALL_NUMBER, S.DriftDirection);
		}
	}

	// ---- Visual bank (presentation only) ------------------------------------------------------------
	const float Yaw01 = T.TurnRate > 0.f ? FMath::Clamp(S.AngularRate.Z / T.TurnRate, -1.f, 1.f) : 0.f;
	S.VisualBank = FMath::Lerp(S.VisualBank, Yaw01 * T.AutoBankAngle, TDFlightSim_ExpAlpha(T.AutoBankResponse, Dt));

	// ---- Integrate position ---------------------------------------------------------------------------
	S.Location += S.Velocity * Dt;
}

int32 TDFlightSim::Advance(FTDFlightState& State, const FTDFlightInput& Input, const FTDFlightTuning& Tuning, const FTDFlightContext& Context, float DeltaSeconds)
{
	if (!(DeltaSeconds > 0.f) || !FMath::IsFinite(DeltaSeconds))
	{
		return 0;
	}
	const float Frame = FMath::Min(DeltaSeconds, TDFlightSim_MaxFrameSeconds);
	const float MaxStep = FMath::Clamp(Tuning.MaxSubstepSeconds, 0.001f, 0.05f);
	const int32 Steps = FMath::Max(1, FMath::CeilToInt((Frame / MaxStep) - 1.e-4f));
	const float StepSeconds = Frame / Steps;

	const FTDFlightInput Clean = Sanitize(Input);
	bool bBurst = false;
	bool bEnded = false;
	for (int32 i = 0; i < Steps; ++i)
	{
		Step(State, Clean, Tuning, Context, StepSeconds);
		bBurst |= State.bDriftBurstThisStep;
		bEnded |= State.bDriftEndedThisStep;
	}
	State.bDriftBurstThisStep = bBurst;
	State.bDriftEndedThisStep = bEnded;
	return Steps;
}
