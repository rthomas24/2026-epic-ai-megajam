// Automation tests for flight & input (ENG-01, ENG-02). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDFlightSim.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/BoxComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"

namespace TDFlightTest
{
	/** A ship at the origin flying +X at cruise, with boundary off unless asked. */
	FTDFlightState CruisingState(const FTDFlightTuning& T)
	{
		FTDFlightState S;
		S.ForwardSpeed = T.CruiseSpeed;
		S.Velocity = FVector::ForwardVector * T.CruiseSpeed;
		return S;
	}

	FTDFlightContext NoBoundary()
	{
		FTDFlightContext C;
		C.bBoundaryEnabled = false;
		return C;
	}

	/** Runs Seconds of frames at Fps with a constant input. */
	void Run(FTDFlightState& S, const FTDFlightInput& In, const FTDFlightTuning& T, const FTDFlightContext& C, float Seconds, float Fps)
	{
		const int32 Frames = FMath::RoundToInt(Seconds * Fps);
		for (int32 i = 0; i < Frames; ++i)
		{
			TDFlightSim::Advance(S, In, T, C, 1.f / Fps);
		}
	}

	float HeadingDeg(const FVector& Dir)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
	}

	/** A scripted flight touching every system, used to compare frame rates. */
	FTDFlightState ScriptedFlight(float Fps)
	{
		const FTDFlightTuning T;
		FTDFlightContext C;
		C.BoundaryCenter = FVector::ZeroVector;
		FTDFlightTuning Tight = T;
		Tight.BoundaryRadius = 20000.f; // so the boundary engages during the script

		FTDFlightState S;
		FTDFlightInput In;
		In.Throttle = 1.f;
		Run(S, In, Tight, C, 1.f, Fps);           // accelerate from rest
		In.bBoost = true; In.Aim = FVector2D(0.6f, 0.2f);
		Run(S, In, Tight, C, 1.f, Fps);           // boosting turn
		In.bBoost = false; In.bDrift = true; In.Aim = FVector2D(1.f, 0.f);
		Run(S, In, Tight, C, 0.8f, Fps);          // drift swing
		In.bDrift = false; In.Aim = FVector2D(-0.3f, -0.5f); In.Roll = 0.7f; In.Strafe = FVector2D(1.f, 0.f);
		Run(S, In, Tight, C, 1.f, Fps);           // release burst, roll, strafe
		In = FTDFlightInput(); In.bBrake = true;
		Run(S, In, Tight, C, 1.2f, Fps);          // brake (boundary may be active)
		return S;
	}
}

using namespace TDFlightTest;

// --------------------------------------------------------------------------------------------- speed

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightCruiseSpeed, "TriadDrift.Flight.Speed.ReachesCruiseAndMin", TD_TEST_FLAGS)
bool FTDFlightCruiseSpeed::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	TestEqual(TEXT("default min speed (features.md)"), T.MinSpeed, 1500.f);
	TestEqual(TEXT("default cruise speed (features.md)"), T.CruiseSpeed, 6000.f);

	FTDFlightState S;
	FTDFlightInput In;
	In.Throttle = 1.f;
	Run(S, In, T, NoBoundary(), 3.f, 60.f);
	TestEqual(TEXT("throttle +1 reaches cruise 6000 cm/s"), (float)S.Velocity.Size(), 6000.f, 60.f);
	TestTrue(TEXT("travelling along the nose"), FVector::DotProduct(S.Velocity.GetSafeNormal(), S.Rotation.GetForwardVector()) > 0.999f);

	In.Throttle = -1.f;
	Run(S, In, T, NoBoundary(), 3.f, 60.f);
	TestEqual(TEXT("throttle -1 settles at min 1500 cm/s"), (float)S.Velocity.Size(), 1500.f, 15.f);

	In.Throttle = 0.f;
	Run(S, In, T, NoBoundary(), 3.f, 60.f);
	TestEqual(TEXT("throttle 0 is halfway (3750 cm/s)"), (float)S.Velocity.Size(), 3750.f, 40.f);

	// AI catch-up allowance: throttle +1 flies at the allowance instead of cruise.
	FTDFlightState Catch = CruisingState(T);
	FTDFlightContext CatchCtx = NoBoundary();
	CatchCtx.ThrottleTopSpeed = 7500.f;
	FTDFlightInput Full;
	Full.Throttle = 1.f;
	Run(Catch, Full, T, CatchCtx, 2.f, 60.f);
	TestEqual(TEXT("catch-up allowance raises throttle +1 to 7500"), (float)Catch.Velocity.Size(), 7500.f, 75.f);
	TestEqual(TEXT("no boost meter used"), Catch.Boost01, 1.f, 0.001f);
	CatchCtx.ThrottleTopSpeed = 0.f;
	Run(Catch, Full, T, CatchCtx, 2.f, 60.f);
	TestEqual(TEXT("cleared allowance returns to cruise"), (float)Catch.Velocity.Size(), T.CruiseSpeed, 60.f);

	// Brake from cruise is a hard stop to min speed.
	FTDFlightState B = CruisingState(T);
	FTDFlightInput Brake;
	Brake.Throttle = 1.f;
	Brake.bBrake = true;
	Run(B, Brake, T, NoBoundary(), 0.4f, 60.f);
	TestTrue(TEXT("brake sheds most speed within 0.4 s"), B.Velocity.Size() < 2200.f);
	Run(B, Brake, T, NoBoundary(), 0.35f, 60.f);
	TestEqual(TEXT("brake settles at min speed by 0.75 s"), (float)B.Velocity.Size(), 1500.f, 30.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightBoost, "TriadDrift.Flight.Boost.SpeedDrainAndRegen", TD_TEST_FLAGS)
bool FTDFlightBoost::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	TestEqual(TEXT("boost is cruise +80%"), T.BoostSpeed, T.CruiseSpeed * 1.8f, 1.f);
	TestEqual(TEXT("3 s meter"), T.BoostDuration, 3.f);
	TestEqual(TEXT("1.2 s regen delay"), T.BoostRegenDelay, 1.2f);

	const float Fps = 120.f;
	FTDFlightState S = CruisingState(T);
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;

	Run(S, In, T, NoBoundary(), 1.f, Fps);
	TestTrue(TEXT("boosting"), S.bBoosting);
	TestEqual(TEXT("boost speed = cruise x1.8"), (float)S.Velocity.Size(), T.CruiseSpeed * 1.8f, 0.01f * T.CruiseSpeed * 1.8f);
	TestEqual(TEXT("meter after 1 s of 3"), S.Boost01, 2.f / 3.f, 0.01f);

	Run(S, In, T, NoBoundary(), 1.95f, Fps);
	TestTrue(TEXT("still boosting at 2.95 s"), S.bBoosting);
	Run(S, In, T, NoBoundary(), 0.1f, Fps);
	TestFalse(TEXT("boost ends when the 3 s meter is empty"), S.bBoosting);
	TestEqual(TEXT("meter empty"), S.Boost01, 0.f, 0.001f);

	// Still holding boost on an empty meter: no re-boost, no regen (boost is still held but the meter
	// can't start); release and measure the regen delay.
	In.bBoost = false;
	Run(S, In, T, NoBoundary(), 1.1f, Fps);
	TestEqual(TEXT("no regen before 1.2 s"), S.Boost01, 0.f, 0.001f);
	Run(S, In, T, NoBoundary(), 0.2f, Fps);
	TestTrue(TEXT("regen started after 1.2 s"), S.Boost01 > 0.f);
	Run(S, In, T, NoBoundary(), T.BoostRegenTime, Fps);
	TestEqual(TEXT("full again 1.2 + 2.5 s after release"), S.Boost01, 1.f, 0.001f);

	Run(S, In, T, NoBoundary(), 2.f, Fps);
	TestEqual(TEXT("back to cruise after boost"), (float)S.Velocity.Size(), T.CruiseSpeed, 60.f);

	// A nearly empty meter can't start a boost.
	S.Boost01 = T.BoostMinToStart * 0.5f;
	S.TimeSinceBoost = 0.f;
	In.bBoost = true;
	TDFlightSim::Advance(S, In, T, NoBoundary(), 1.f / Fps);
	TestFalse(TEXT("needs BoostMinToStart to begin"), S.bBoosting);
	return true;
}

// ---------------------------------------------------------------------------------------------- drift

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightDrift, "TriadDrift.Flight.Drift.KeepsVelocityAndBursts", TD_TEST_FLAGS)
bool FTDFlightDrift::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	FTDFlightState S = CruisingState(T);
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bDrift = true;
	In.Aim = FVector2D(1.f, 0.f); // full yaw right

	Run(S, In, T, NoBoundary(), 1.f, 120.f);
	TestTrue(TEXT("drifting"), S.bDrifting);
	TestTrue(TEXT("velocity keeps its +X direction"), FVector::DotProduct(S.Velocity.GetSafeNormal(), FVector::ForwardVector) > 0.9999f);
	const float Turned = HeadingDeg(S.Rotation.GetForwardVector());
	TestTrue(FString::Printf(TEXT("nose swings at the 1.5x drift rate (turned %.1f deg in 1 s, normal cap is 110)"), Turned), Turned > 140.f && Turned <= 165.5f);
	TestTrue(TEXT("drift yaw rate is capped at 1.5 x 110"), S.AngularRate.Z <= 165.f + 0.01f);

	const float SpeedBefore = S.Velocity.Size();
	In.bDrift = false;
	In.Aim = FVector2D::ZeroVector;
	TDFlightSim::Advance(S, In, T, NoBoundary(), 1.f / 120.f);
	TestTrue(TEXT("drift released"), !S.bDrifting && S.bDriftBurstThisStep);
	TestTrue(TEXT("velocity snaps to the nose"), FVector::DotProduct(S.Velocity.GetSafeNormal(), S.Rotation.GetForwardVector()) > 0.999f);
	TestEqual(TEXT("release burst is +20%"), (float)S.Velocity.Size(), SpeedBefore * 1.2f, SpeedBefore * 0.01f);

	// The burst bleeds off back to cruise rather than vanishing instantly.
	Run(S, In, T, NoBoundary(), 0.25f, 120.f);
	TestTrue(TEXT("burst still felt after 0.25 s"), S.Velocity.Size() > T.CruiseSpeed * 1.05f);
	Run(S, In, T, NoBoundary(), 4.f, 120.f);
	TestEqual(TEXT("settles back to cruise"), (float)S.Velocity.Size(), T.CruiseSpeed, 60.f);
	return true;
}

// ------------------------------------------------------------------------------------------- rotation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightTurnRate, "TriadDrift.Flight.Rotation.TurnAndRollRateCaps", TD_TEST_FLAGS)
bool FTDFlightTurnRate::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	FTDFlightState S = CruisingState(T);
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.Aim = FVector2D(1.f, 0.f);

	Run(S, In, T, NoBoundary(), 1.f, 120.f); // settle on the cap
	const float H0 = HeadingDeg(S.Rotation.GetForwardVector());
	float MaxRate = 0.f;
	for (int32 i = 0; i < 60; ++i) // 0.5 s
	{
		TDFlightSim::Advance(S, In, T, NoBoundary(), 1.f / 120.f);
		MaxRate = FMath::Max(MaxRate, (float)S.AngularRate.Z);
	}
	const float H1 = HeadingDeg(S.Rotation.GetForwardVector());
	const float Rate = FMath::FindDeltaAngleDegrees(H0, H1) / 0.5f;
	TestEqual(TEXT("yaw rate at full aim = 110 deg/s"), Rate, 110.f, 1.5f);
	TestTrue(TEXT("yaw rate never exceeds the cap"), MaxRate <= 110.f + 0.01f);

	// Pitch: same cap.
	FTDFlightState P = CruisingState(T);
	FTDFlightInput Up;
	Up.Aim = FVector2D(0.f, 1.f);
	Run(P, Up, T, NoBoundary(), 1.f, 120.f);
	TestEqual(TEXT("pitch rate at full aim = 110 deg/s"), (float)P.AngularRate.Y, 110.f, 0.5f);
	TestTrue(TEXT("pitch up raises the nose"), P.Rotation.GetForwardVector().Z > 0.5f);

	// Roll: 180 deg/s, positive roll input rolls right (right wing down).
	FTDFlightState R = CruisingState(T);
	FTDFlightInput RollIn;
	RollIn.Roll = 1.f;
	Run(R, RollIn, T, NoBoundary(), 0.5f, 120.f);
	TestEqual(TEXT("roll rate = 180 deg/s"), (float)R.AngularRate.X, 180.f, 1.f);
	TestTrue(TEXT("roll right dips the right wing"), R.Rotation.GetRightVector().Z < -0.3f);

	// Auto-level: with no roll input the ship rolls back toward world up.
	FTDFlightInput None;
	None.Throttle = 1.f;
	Run(R, None, T, NoBoundary(), 6.f, 120.f);
	TestTrue(TEXT("auto-levels when roll is released"), FMath::Abs(R.Rotation.Rotator().Roll) < 3.f);

	// Visual bank leans into yaw without touching the flight frame.
	TestTrue(TEXT("hull banks right into a right turn"), S.VisualBank > T.AutoBankAngle * 0.8f);
	TestTrue(TEXT("flight frame stays level in a flat turn"), FMath::Abs(S.Rotation.Rotator().Roll) < 1.f);
	return true;
}

// ------------------------------------------------------------------------------------------- boundary

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightBoundary, "TriadDrift.Flight.Boundary.SteersBack", TD_TEST_FLAGS)
bool FTDFlightBoundary::RunTest(const FString& Parameters)
{
	FTDFlightTuning T;
	T.BoundaryRadius = 100000.f;
	FTDFlightContext C; // boundary on, centre at origin

	// Inside, but in the warning band.
	FTDFlightState Warn = CruisingState(T);
	Warn.Location = FVector(T.BoundaryRadius - T.BoundaryWarnDistance * 0.5f, 0.f, 0.f);
	Warn.Velocity = FVector::ZeroVector; Warn.ForwardSpeed = 0.f;
	TDFlightSim::Advance(Warn, FTDFlightInput(), T, C, 1.f / 60.f);
	TestTrue(TEXT("warning band sets IsNearBoundary"), Warn.bNearBoundary);
	TestFalse(TEXT("but not out of bounds"), Warn.bOutOfBounds);

	// Outside, flying straight out with no input.
	FTDFlightState S = CruisingState(T);
	S.Location = FVector(T.BoundaryRadius + 2000.f, 0.f, 0.f);
	FTDFlightInput In;
	In.Throttle = 1.f;
	TDFlightSim::Advance(S, In, T, C, 1.f / 60.f);
	TestTrue(TEXT("out of bounds flagged"), S.bOutOfBounds);
	const float StartDist = S.Location.Size();

	float MaxDist = StartDist;
	for (int32 i = 0; i < 60 * 12; ++i)
	{
		TDFlightSim::Advance(S, In, T, C, 1.f / 60.f);
		MaxDist = FMath::Max(MaxDist, (float)S.Location.Size());
	}
	const FVector ToCenter = (-S.Location).GetSafeNormal();
	TestTrue(TEXT("nose turned back toward the centre"), FVector::DotProduct(S.Rotation.GetForwardVector(), ToCenter) > 0.7f);
	TestTrue(TEXT("pushed back inside (or heading in)"), S.Location.Size() < StartDist);
	TestTrue(FString::Printf(TEXT("overshoot stays bounded (max %.0f cm past the edge)"), MaxDist - T.BoundaryRadius), MaxDist - T.BoundaryRadius < 60000.f);

	// Disabled boundary does nothing.
	FTDFlightState Off = CruisingState(T);
	Off.Location = FVector(T.BoundaryRadius * 2.f, 0.f, 0.f);
	Run(Off, In, T, NoBoundary(), 2.f, 60.f);
	TestFalse(TEXT("no boundary when disabled"), Off.bOutOfBounds);
	TestTrue(TEXT("keeps flying out when disabled"), FVector::DotProduct(Off.Rotation.GetForwardVector(), FVector::ForwardVector) > 0.999f);
	return true;
}

// -------------------------------------------------------------------------------- frame-rate independence

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightFrameRate, "TriadDrift.Flight.FrameRate.SameAt30And120", TD_TEST_FLAGS)
bool FTDFlightFrameRate::RunTest(const FString& Parameters)
{
	const FTDFlightState A = ScriptedFlight(30.f);
	const FTDFlightState B = ScriptedFlight(120.f);
	const FTDFlightState C = ScriptedFlight(60.f);

	const float PosErr = (A.Location - B.Location).Size();
	const float VelErr = (A.Velocity - B.Velocity).Size();
	const float RotErr = FMath::RadiansToDegrees(A.Rotation.AngularDistance(B.Rotation));
	TestTrue(FString::Printf(TEXT("30 vs 120 fps position within 5 cm (%.3f cm over %.0f cm flown)"), PosErr, (float)B.Location.Size()), PosErr < 5.f);
	TestTrue(FString::Printf(TEXT("30 vs 120 fps velocity within 2 cm/s (%.3f)"), VelErr), VelErr < 2.f);
	TestTrue(FString::Printf(TEXT("30 vs 120 fps rotation within 0.1 deg (%.4f)"), RotErr), RotErr < 0.1f);
	TestEqual(TEXT("boost meter matches"), A.Boost01, B.Boost01, 0.002f);

	const float PosErr60 = (C.Location - B.Location).Size();
	TestTrue(FString::Printf(TEXT("60 vs 120 fps position within 5 cm (%.3f cm)"), PosErr60), PosErr60 < 5.f);

	// An odd frame rate (37 fps) uses shorter sub-steps; one constant-input second must still match.
	FTDFlightState Odd, Ref;
	FTDFlightInput In;
	In.Throttle = 1.f; In.bBoost = true; In.Aim = FVector2D(0.7f, -0.3f);
	const FTDFlightTuning T;
	FTDFlightContext NoB; NoB.bBoundaryEnabled = false;
	for (int32 i = 0; i < 37; ++i) { TDFlightSim::Advance(Odd, In, T, NoB, 1.f / 37.f); }
	for (int32 i = 0; i < 120; ++i) { TDFlightSim::Advance(Ref, In, T, NoB, 1.f / 120.f); }
	const float OddErr = (Odd.Location - Ref.Location).Size();
	TestTrue(FString::Printf(TEXT("37 fps within 0.5%% of 120 fps over 1 s (%.1f cm of %.0f)"), OddErr, (float)Ref.Location.Size()), OddErr < 0.005f * Ref.Location.Size());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightRobustness, "TriadDrift.Flight.Robustness.BadInputAndTime", TD_TEST_FLAGS)
bool FTDFlightRobustness::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	FTDFlightState S = CruisingState(T);
	const FTDFlightState Before = S;

	TestEqual(TEXT("zero dt does nothing"), TDFlightSim::Advance(S, FTDFlightInput(), T, NoBoundary(), 0.f), 0);
	TestEqual(TEXT("negative dt does nothing"), TDFlightSim::Advance(S, FTDFlightInput(), T, NoBoundary(), -1.f), 0);
	TestEqual(TEXT("NaN dt does nothing"), TDFlightSim::Advance(S, FTDFlightInput(), T, NoBoundary(), NAN), 0);
	TestTrue(TEXT("state untouched"), S.Location.Equals(Before.Location) && S.Velocity.Equals(Before.Velocity));

	FTDFlightInput Bad;
	Bad.Throttle = NAN;
	Bad.Aim = FVector2D(50.f, -NAN);
	Bad.Roll = INFINITY;
	Bad.Strafe = FVector2D(-7.f, 3.f);
	Run(S, Bad, T, NoBoundary(), 1.f, 60.f);
	TestFalse(TEXT("no NaN in location"), S.Location.ContainsNaN());
	TestFalse(TEXT("no NaN in rotation"), S.Rotation.ContainsNaN());
	TestTrue(TEXT("aim clamped to the turn cap"), FMath::Abs(S.AngularRate.Z) <= T.TurnRate + 0.01f);

	// A huge hitch is clamped (no tunnelling across the map in one frame).
	FTDFlightState H = CruisingState(T);
	TDFlightSim::Advance(H, FTDFlightInput(), T, NoBoundary(), 5.f);
	TestTrue(TEXT("hitch clamped to 0.25 s of travel"), H.Location.Size() < T.CruiseSpeed * 0.26f);
	return true;
}

// ------------------------------------------------------------------------------------------ in a world

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightShipInWorld, "TriadDrift.Flight.World.ShipFliesAndKeepsVelocityAcrossPossession", TD_TEST_FLAGS)
bool FTDFlightShipInWorld::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship spawned"), Ship)) { return false; }
	TestNotNull(TEXT("movement"), Ship->GetShipMovement());
	TestNotNull(TEXT("health"), Ship->GetHealth());
	TestTrue(TEXT("placeholder hull mesh"), Ship->GetHullMesh() && Ship->GetHullMesh()->GetStaticMesh() != nullptr);
	TestEqual(TEXT("squad ships are on the player team"), Ship->GetTeam(), ETDTeam::Player);

	FTDFlightInput In;
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);
	W.Advance(3.f);
	TestEqual(TEXT("cruises at 6000 in a world"), (float)Ship->GetVelocity().Size(), 6000.f, 60.f);
	TestTrue(TEXT("actually moved"), Ship->GetActorLocation().X > 10000.f);
	TestTrue(TEXT("Speed01 between min and boost"), Ship->GetSpeed01() > 0.4f && Ship->GetSpeed01() < 0.6f);

	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Advance(0.5f);
	TestTrue(TEXT("IsBoosting exposed"), Ship->IsBoosting());
	In.bBoost = false;
	Ship->SetFlightInput(In);
	W.Advance(2.f);

	// Possession changes never reset velocity (T3 switching contract).
	const FVector V0 = Ship->GetVelocity();
	APlayerController* PC = W.SpawnPlayerController(Ship, ATDSpacePlayerController::StaticClass());
	TestNotNull(TEXT("space player controller"), PC);
	Ship->SetPlayerControlled(true);
	W.Tick();
	TestTrue(TEXT("velocity survives possess"), Ship->GetVelocity().Equals(V0, 200.f));
	if (PC) { PC->UnPossess(); }
	Ship->SetPlayerControlled(false);
	W.Tick();
	TestTrue(TEXT("velocity survives unpossess"), Ship->GetVelocity().Size() > 5000.f);

	// Knockback through Velocity or AddImpulseVelocity is honoured, then grip recovers.
	Ship->GetShipMovement()->AddImpulseVelocity(FVector(0.f, 3000.f, 0.f));
	TestTrue(TEXT("impulse applied"), Ship->GetVelocity().Y > 2500.f);
	W.Advance(1.f);
	TestTrue(TEXT("grip pulls travel back onto the nose"), FMath::Abs(FVector::DotProduct(Ship->GetVelocity().GetSafeNormal(), Ship->GetActorForwardVector())) > 0.98f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightTeleport, "TriadDrift.Flight.World.TeleportAndOverride", TD_TEST_FLAGS)
bool FTDFlightTeleport::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Advance(1.f);

	const float Speed = Ship->GetVelocity().Size();
	Ship->TeleportSpacecraft(FVector(50000.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f), true);
	TestTrue(TEXT("moved"), Ship->GetActorLocation().Equals(FVector(50000.f, 0.f, 0.f), 1.f));
	TestTrue(TEXT("faces the new direction"), Ship->GetActorForwardVector().Equals(FVector(0.f, 1.f, 0.f), 0.01f));
	TestTrue(TEXT("keeps speed along the new facing"), Ship->GetVelocity().GetSafeNormal().Equals(FVector(0.f, 1.f, 0.f), 0.01f)
		&& FMath::IsNearlyEqual((float)Ship->GetVelocity().Size(), Speed, 50.f));

	Ship->SetSpeedOverride(40000.f, 0.25f);
	W.Advance(1.f);
	TestEqual(TEXT("speed override (hyperdrive / ram)"), (float)Ship->GetVelocity().Size(), 40000.f, 400.f);
	Ship->ClearSpeedOverride();
	W.Advance(6.f);
	TestEqual(TEXT("back to throttle speed after the override"), (float)Ship->GetVelocity().Size(), 3750.f, 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightCollision, "TriadDrift.Flight.World.GlancingVsHeadOnCollision", TD_TEST_FLAGS)
bool FTDFlightCollision::RunTest(const FString& Parameters)
{
	auto SpawnWall = [](FTDTestWorld& W, const FVector& Location)
	{
		AActor* Wall = W.Spawn<AActor>(Location);
		UBoxComponent* Box = NewObject<UBoxComponent>(Wall, TEXT("Box"));
		Box->SetBoxExtent(FVector(200.f, 20000.f, 20000.f));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Wall->SetRootComponent(Box);
		Box->RegisterComponent();
		Wall->SetActorLocation(Location);
		return Wall;
	};

	auto Fly = [](FTDTestWorld& W, ATDShip* Ship, float Yaw)
	{
		Ship->TeleportSpacecraft(FVector::ZeroVector, FRotator(0.f, Yaw, 0.f), false);
		Ship->GetShipMovement()->SetForwardSpeedImmediate(6000.f);
		FTDFlightInput In;
		In.Throttle = 1.f;
		Ship->SetFlightInput(In);
	};

	// Head-on: wall 3000 cm ahead.
	{
		FTDTestWorld W;
		SpawnWall(W, FVector(3000.f, 0.f, 0.f));
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
		Fly(W, Ship, 0.f);
		const float HP0 = Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield();
		bool bHit = false;
		FTDFlightImpact Impact;
		for (int32 i = 0; i < 60 && !bHit; ++i)
		{
			W.Tick();
			bHit = Ship->GetVelocity().X < 0.f;
		}
		TestTrue(TEXT("head-on bounces back"), bHit);
		TestTrue(TEXT("did not pass through the wall"), Ship->GetActorLocation().X < 3000.f);
		const float Lost = HP0 - (Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield());
		TestTrue(FString::Printf(TEXT("head-on deals heavy damage (%.1f)"), Lost), Lost >= 40.f);
	}

	// Glancing: wall the same, approach at 75 degrees off its normal.
	{
		FTDTestWorld W;
		SpawnWall(W, FVector(3000.f, 0.f, 0.f));
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
		Fly(W, Ship, 75.f);
		Ship->TeleportSpacecraft(FVector(2000.f, 0.f, 0.f), FRotator(0.f, 75.f, 0.f), false); // 15 deg into the wall (75 off its normal)
		Ship->GetShipMovement()->SetForwardSpeedImmediate(6000.f);
		const float HP0 = Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield();
		W.Advance(1.f);
		const float Lost = HP0 - (Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield());
		TestTrue(TEXT("scrape keeps most speed"), Ship->GetVelocity().Size() > 4000.f);
		TestTrue(TEXT("slides along the wall, not through it"), Ship->GetActorLocation().X < 3000.f && Ship->GetActorLocation().Y > 1000.f);
		TestTrue(FString::Printf(TEXT("glancing damage is light (%.1f)"), Lost), Lost > 0.f && Lost < 20.f);
	}
	return true;
}

// ----------------------------------------------------------------------------------------------- input

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFlightInputMap, "TriadDrift.Flight.Input.RuntimeMappingContexts", TD_TEST_FLAGS)
bool FTDFlightInputMap::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("controller"), PC)) { return false; }
	W.Tick();

	UInputMappingContext* Common = PC->GetCommonContext();
	UInputMappingContext* Space = PC->GetSpaceContext();
	UInputMappingContext* Atmo = PC->GetAtmosphereContext();
	if (!TestTrue(TEXT("contexts built in C++"), Common && Space && Atmo)) { return false; }

	auto Has = [](const UInputMappingContext* Ctx, const UInputAction* Action, const FKey& Key)
	{
		for (const FEnhancedActionKeyMapping& M : Ctx->GetMappings())
		{
			if (M.Action == Action && M.Key == Key) { return true; }
		}
		return false;
	};
	auto A = [PC](const TCHAR* Name) { return PC->FindActionByName(FName(Name)); };

	struct FExpect { const TCHAR* Action; FKey Key; };
	const FExpect CommonExpect[] = {
		{ TEXT("IA_Throttle"), EKeys::W }, { TEXT("IA_Throttle"), EKeys::S },
		{ TEXT("IA_Throttle"), EKeys::Gamepad_RightTriggerAxis }, { TEXT("IA_Throttle"), EKeys::Gamepad_LeftTriggerAxis },
		{ TEXT("IA_AimMouse"), EKeys::Mouse2D }, { TEXT("IA_AimStick"), EKeys::Gamepad_Right2D },
		{ TEXT("IA_Roll"), EKeys::A }, { TEXT("IA_Roll"), EKeys::D }, { TEXT("IA_Roll"), EKeys::Gamepad_LeftX },
		{ TEXT("IA_FirePrimary"), EKeys::LeftMouseButton }, { TEXT("IA_FirePrimary"), EKeys::Gamepad_RightShoulder },
		{ TEXT("IA_FireSecondary"), EKeys::RightMouseButton }, { TEXT("IA_FireSecondary"), EKeys::Gamepad_LeftShoulder },
		{ TEXT("IA_Boost"), EKeys::LeftShift }, { TEXT("IA_Boost"), EKeys::Gamepad_LeftThumbstick },
		{ TEXT("IA_Drift"), EKeys::LeftAlt }, { TEXT("IA_Drift"), EKeys::C }, { TEXT("IA_Drift"), EKeys::Gamepad_FaceButton_Right },
		{ TEXT("IA_Power"), EKeys::Q }, { TEXT("IA_Power"), EKeys::Gamepad_FaceButton_Left },
		{ TEXT("IA_Ability"), EKeys::E }, { TEXT("IA_Ability"), EKeys::Gamepad_FaceButton_Top },
		{ TEXT("IA_SwitchNext"), EKeys::Tab }, { TEXT("IA_SwitchNext"), EKeys::Gamepad_DPad_Right },
		{ TEXT("IA_SwitchPrev"), EKeys::Gamepad_DPad_Left },
		{ TEXT("IA_Switch1"), EKeys::One }, { TEXT("IA_Switch2"), EKeys::Two }, { TEXT("IA_Switch3"), EKeys::Three },
		{ TEXT("IA_SwitchRadial"), EKeys::Gamepad_DPad_Up },
		{ TEXT("IA_TargetCycle"), EKeys::T }, { TEXT("IA_TargetCycle"), EKeys::MiddleMouseButton }, { TEXT("IA_TargetCycle"), EKeys::Gamepad_RightThumbstick },
		{ TEXT("IA_Pause"), EKeys::Escape }, { TEXT("IA_Pause"), EKeys::Gamepad_Special_Right },
		{ TEXT("IA_GalaxyMap"), EKeys::M }, { TEXT("IA_GalaxyMap"), EKeys::Gamepad_Special_Left },
	};
	for (const FExpect& E : CommonExpect)
	{
		const UInputAction* Action = A(E.Action);
		TestTrue(FString::Printf(TEXT("%s on %s"), E.Action, *E.Key.ToString()), Action && Has(Common, Action, E.Key));
	}

	TestTrue(TEXT("IMC_Space: Hyperdrive on H"), Has(Space, A(TEXT("IA_Hyperdrive")), EKeys::H));
	TestTrue(TEXT("IMC_Space: Hyperdrive on D-pad down"), Has(Space, A(TEXT("IA_Hyperdrive")), EKeys::Gamepad_DPad_Down));
	TestTrue(TEXT("IMC_Space: squad command on F"), Has(Space, A(TEXT("IA_SquadCommand")), EKeys::F));
	TestTrue(TEXT("IMC_Atmosphere: Land on F"), Has(Atmo, A(TEXT("IA_Land")), EKeys::F));
	TestTrue(TEXT("IMC_Atmosphere: Land on A"), Has(Atmo, A(TEXT("IA_Land")), EKeys::Gamepad_FaceButton_Bottom));
	TestFalse(TEXT("IMC_Atmosphere has no Hyperdrive"), Has(Atmo, A(TEXT("IA_Hyperdrive")), EKeys::H));

	PC->SetFlightContext(ETDFlightContext::Atmosphere);
	TestEqual(TEXT("context switch"), PC->GetFlightContext(), ETDFlightContext::Atmosphere);

	// With no throttle pressed the controller flies the ship at cruise.
	W.Advance(3.f);
	TestEqual(TEXT("player ship cruises with no input"), (float)Ship->GetVelocity().Size(), 6000.f, 80.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
