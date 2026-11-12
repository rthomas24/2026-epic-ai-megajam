// Automation tests added for the ENG-01/02 code review: drift and boost exploits, boundary,
// collisions and events, camera, input routing. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Flight

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDFlightSim.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "Camera/CameraActor.h"

/** Reaches the controller's private input state and handlers (tests only). */
struct FTDSpaceControllerTestAccess
{
	ATDSpacePlayerController* PC;
	void Throttle(float V) { PC->OnThrottle(FInputActionValue(V)); }
	void Mouse(const FVector2D& D) { PC->OnMouseAim(FInputActionValue(D)); }
	void Stick(const FVector2D& D) { PC->OnStickAim(FInputActionValue(D)); }
	void PowerDown() { PC->OnPowerPressed(FInputActionValue(true)); }
	void PowerUp() { PC->OnPowerReleased(FInputActionValue(false)); }
	bool UsingGamepad() const { return PC->bUsingGamepad; }
	FVector2D VirtualStick() const { return PC->VirtualStick; }
	float& HoldSeconds() { return PC->PowerHoldSeconds; }
};

namespace TDReviewTest
{
	FTDFlightState Cruising(const FTDFlightTuning& T)
	{
		FTDFlightState S;
		S.ForwardSpeed = T.CruiseSpeed;
		S.Velocity = FVector::ForwardVector * T.CruiseSpeed;
		return S;
	}

	FTDFlightContext Open()
	{
		FTDFlightContext C;
		C.bBoundaryEnabled = false;
		return C;
	}

	AActor* SpawnWall(FTDTestWorld& W, const FVector& Location)
	{
		AActor* Wall = W.Spawn<AActor>(Location);
		UBoxComponent* Box = NewObject<UBoxComponent>(Wall, TEXT("Box"));
		Box->SetBoxExtent(FVector(200.f, 40000.f, 40000.f));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Wall->SetRootComponent(Box);
		Box->RegisterComponent();
		Wall->SetActorLocation(Location);
		return Wall;
	}

	UTDPowerComponent* AddPower(AActor* Ship, ETDPowerInput Slot, FName Name)
	{
		UTDPowerComponent* Power = NewObject<UTDTestPowerComponent>(Ship, Name);
		Power->Slot = Slot;
		Power->PowerName = Name;
		Power->bRequirePlayerControl = false;
		Power->CooldownSeconds = 10.f;
		Power->RegisterComponent();
		return Power;
	}
}


// ------------------------------------------------------------------------------------------ drift

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewDriftSpam, "TriadDrift.Flight.Drift.TapSpamAndBurstCeiling", TD_TEST_FLAGS)
bool FTDReviewDriftSpam::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	const float Cap = T.BoostSpeed * (1.f + T.DriftReleaseBurst);

	// Tap drift ~6x per second for 3 s while turning: no burst is earned, speed never climbs.
	FTDFlightState S = TDReviewTest::Cruising(T);
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.Aim = FVector2D(0.4f, 0.f);
	float MaxSpeed = 0.f;
	int32 Bursts = 0;
	for (int32 Frame = 0; Frame < 360; ++Frame) // 3 s at 120 fps
	{
		In.bDrift = (Frame % 20) < 6; // 50 ms drift taps, 6 per second
		TDFlightSim::Advance(S, In, T, TDReviewTest::Open(), 1.f / 120.f);
		MaxSpeed = FMath::Max(MaxSpeed, (float)S.Velocity.Size());
		Bursts += S.bDriftBurstThisStep ? 1 : 0;
	}
	TestEqual(TEXT("tap-drifts earn no burst"), Bursts, 0);
	TestTrue(FString::Printf(TEXT("tap-spam can't pump speed (max %.0f)"), MaxSpeed), MaxSpeed <= T.CruiseSpeed * 1.02f);

	// Long drifts released back-to-back: each burst is earned, but speed stays under the ceiling.
	FTDFlightState L = TDReviewTest::Cruising(T);
	float MaxLong = 0.f;
	FTDFlightInput Long;
	Long.Throttle = 1.f;
	Long.bBoost = true;
	for (int32 Frame = 0; Frame < 1200; ++Frame) // 10 s
	{
		Long.bDrift = (Frame % 48) < 40; // 0.33 s drifts, released every 0.4 s
		TDFlightSim::Advance(L, Long, T, TDReviewTest::Open(), 1.f / 120.f);
		MaxLong = FMath::Max(MaxLong, (float)L.Velocity.Size());
	}
	TestTrue(FString::Printf(TEXT("repeated earned bursts never exceed boost x1.2 (max %.0f, cap %.0f)"), MaxLong, Cap), MaxLong <= Cap + 1.f);

	// A real drift still pays out the full +20%.
	FTDFlightState R = TDReviewTest::Cruising(T);
	FTDFlightInput Held;
	Held.Throttle = 1.f;
	Held.bDrift = true;
	for (int32 i = 0; i < 48; ++i) { TDFlightSim::Advance(R, Held, T, TDReviewTest::Open(), 1.f / 120.f); }
	const float Before = R.Velocity.Size();
	Held.bDrift = false;
	TDFlightSim::Advance(R, Held, T, TDReviewTest::Open(), 1.f / 120.f);
	TestTrue(TEXT("0.4 s drift earns the burst"), R.bDriftBurstThisStep);
	TestEqual(TEXT("+20%"), (float)R.Velocity.Size(), Before * 1.2f, Before * 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewDriftOverride, "TriadDrift.Flight.Drift.OverrideInteractions", TD_TEST_FLAGS)
bool FTDReviewDriftOverride::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	const float Cap = T.BoostSpeed * (1.f + T.DriftReleaseBurst);

	// Holding drift as a hyperdrive jump ends: the drift starts at 80,000 and must not keep it.
	FTDFlightState S = TDReviewTest::Cruising(T);
	FTDFlightContext Jump = TDReviewTest::Open();
	Jump.bSpeedOverride = true;
	Jump.OverrideSpeed = 80000.f;
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bDrift = true;
	for (int32 i = 0; i < 240; ++i) { TDFlightSim::Advance(S, In, T, Jump, 1.f / 120.f); }
	TestTrue(TEXT("no drift during an override"), !S.bDrifting);
	TestEqual(TEXT("at jump speed"), (float)S.Velocity.Size(), 80000.f, 800.f);
	for (int32 i = 0; i < 120 * 3; ++i) { TDFlightSim::Advance(S, In, T, TDReviewTest::Open(), 1.f / 120.f); }
	TestTrue(TEXT("drift engages once the override clears"), S.bDrifting);
	TestTrue(FString::Printf(TEXT("drift bleeds the jump speed down to the ceiling (%.0f)"), (float)S.Velocity.Size()), S.Velocity.Size() <= Cap + 1.f);

	// An override starting mid-drift ends the drift without a burst.
	FTDFlightState D = TDReviewTest::Cruising(T);
	for (int32 i = 0; i < 60; ++i) { TDFlightSim::Advance(D, In, T, TDReviewTest::Open(), 1.f / 120.f); }
	TestTrue(TEXT("drifting"), D.bDrifting);
	TDFlightSim::Advance(D, In, T, Jump, 1.f / 120.f);
	TestTrue(TEXT("drift ended by the override"), !D.bDrifting && D.bDriftEndedThisStep);
	TestFalse(TEXT("no spurious burst"), D.bDriftBurstThisStep);
	return true;
}

// ------------------------------------------------------------------------------------------ boost

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewBoostLockout, "TriadDrift.Flight.Boost.EmptyMeterLockout", TD_TEST_FLAGS)
bool FTDReviewBoostLockout::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	FTDFlightState S = TDReviewTest::Cruising(T);
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;
	int32 Starts = 0;
	bool bWas = false;
	for (int32 i = 0; i < 120 * 10; ++i) // hold boost for 10 s straight
	{
		TDFlightSim::Advance(S, In, T, TDReviewTest::Open(), 1.f / 120.f);
		Starts += (S.bBoosting && !bWas) ? 1 : 0;
		bWas = S.bBoosting;
	}
	TestEqual(TEXT("holding through an empty meter boosts once, no stutter"), Starts, 1);
	TestFalse(TEXT("locked out while still held"), S.bBoosting);
	TestTrue(TEXT("the meter refills meanwhile"), S.Boost01 > 0.9f);

	In.bBoost = false;
	TDFlightSim::Advance(S, In, T, TDReviewTest::Open(), 1.f / 120.f);
	In.bBoost = true;
	TDFlightSim::Advance(S, In, T, TDReviewTest::Open(), 1.f / 120.f);
	TestTrue(TEXT("release and press boosts again"), S.bBoosting);
	return true;
}

// ---------------------------------------------------------------------------------- strafe, boundary

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewStrafe, "TriadDrift.Flight.Strafe.LateralAndVertical", TD_TEST_FLAGS)
bool FTDReviewStrafe::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	TestEqual(TEXT("features.md strafe 2500"), T.StrafeSpeed, 2500.f);
	FTDFlightState S = TDReviewTest::Cruising(T);
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.Strafe = FVector2D(1.f, -1.f);
	for (int32 i = 0; i < 120 * 2; ++i) { TDFlightSim::Advance(S, In, T, TDReviewTest::Open(), 1.f / 120.f); }
	TestEqual(TEXT("right strafe 2500 cm/s"), (float)S.Velocity.Y, 2500.f, 30.f);
	TestEqual(TEXT("down strafe 2500 cm/s"), (float)S.Velocity.Z, -2500.f, 30.f);
	TestEqual(TEXT("forward speed unchanged"), (float)S.Velocity.X, T.CruiseSpeed, 60.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewBoundaryOutsteer, "TriadDrift.Flight.Boundary.CannotBeOutSteered", TD_TEST_FLAGS)
bool FTDReviewBoundaryOutsteer::RunTest(const FString& Parameters)
{
	FTDFlightTuning T;
	T.BoundaryRadius = 100000.f;
	FTDFlightContext C;

	// A pilot fighting the boundary: always aiming to turn away from the centre, boosting, and drifting
	// in bursts. They must still end up heading home.
	FTDFlightState S = TDReviewTest::Cruising(T);
	S.Location = FVector(T.BoundaryRadius + 1000.f, 0.f, 0.f);
	float MaxDepth = 0.f;
	for (int32 i = 0; i < 120 * 20; ++i)
	{
		FTDFlightInput In;
		In.Throttle = 1.f;
		In.bBoost = (i / 120) % 2 == 0;
		In.bDrift = (i % 120) > 80;
		const FVector Out = S.Location.GetSafeNormal();
		const FVector Right = S.Rotation.GetRightVector();
		In.Aim = FVector2D(FVector::DotProduct(Out, Right) >= 0.f ? 1.f : -1.f, 0.f); // yaw toward "out"
		TDFlightSim::Advance(S, In, T, C, 1.f / 120.f);
		MaxDepth = FMath::Max(MaxDepth, (float)S.Location.Size() - T.BoundaryRadius);
	}
	TestTrue(FString::Printf(TEXT("held near the edge despite the pilot (max %.0f cm out)"), MaxDepth), MaxDepth < 40000.f);
	TestTrue(TEXT("never escapes"), S.Location.Size() < T.BoundaryRadius + 40000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewTuningClampsSpeed, "TriadDrift.Flight.Speed.ApplyFlightTuningClampsCruise", TD_TEST_FLAGS)
bool FTDReviewTuningClampsSpeed::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	TestEqual(TEXT("spawns at the default cruise"), (float)Ship->GetVelocity().Size(), 6000.f, 1.f);

	// A slow spec (carrier: cruise 1500) applied right after spawn: no lurch.
	FTDFlightTuning Slow;
	Slow.MinSpeed = 600.f;
	Slow.CruiseSpeed = 1500.f;
	Slow.BoostSpeed = 2000.f;
	Ship->ApplyFlightTuning(Slow);
	TestEqual(TEXT("clamped to the new cruise at once"), (float)Ship->GetVelocity().Size(), 1500.f, 1.f);
	FTDFlightInput In;
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);
	float MaxSpeed = 0.f;
	for (int32 i = 0; i < 60; ++i) { W.Tick(); MaxSpeed = FMath::Max(MaxSpeed, (float)Ship->GetVelocity().Size()); }
	TestTrue(FString::Printf(TEXT("never above the new cruise (max %.0f)"), MaxSpeed), MaxSpeed <= 1501.f);

	// Intentional extra speed survives a re-applied spec.
	Ship->ApplyFlightTuning(FTDFlightTuning());
	Ship->SetSpeedOverride(30000.f, 0.5f);
	W.Advance(1.f);
	Ship->ApplyFlightTuning(FTDFlightTuning());
	TestTrue(TEXT("a speed override isn't clamped"), Ship->GetVelocity().Size() > 25000.f);
	Ship->ClearSpeedOverride();

	// Slower than the new cruise: never pushed up.
	Ship->GetShipMovement()->SetForwardSpeedImmediate(800.f);
	Ship->ApplyFlightTuning(FTDFlightTuning());
	TestEqual(TEXT("only clamps down"), (float)Ship->GetVelocity().Size(), 800.f, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewBoundarySurvivesTuning, "TriadDrift.Flight.Boundary.SurvivesApplyFlightTuning", TD_TEST_FLAGS)
bool FTDReviewBoundarySurvivesTuning::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(60000.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->GetShipMovement()->SetBoundary(FVector::ZeroVector, 50000.f);
	Ship->ApplyFlightTuning(FTDFlightTuning()); // a spec (ENG-04 / InitFromSpec) applied after the level set the bounds
	W.Tick();
	TestTrue(TEXT("boundary radius kept after ApplyFlightTuning"), Ship->IsOutOfBounds());
	return true;
}

// ------------------------------------------------------------------------------ collisions + events

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewCollisionEvents, "TriadDrift.Flight.World.CollisionDamageAngleCooldownAndEvents", TD_TEST_FLAGS)
bool FTDReviewCollisionEvents::RunTest(const FString& Parameters)
{
	auto Hit = [this](float YawIntoWall, float& OutDamage, bool& bOutHeadOn) -> bool
	{
		FTDTestWorld W;
		TDReviewTest::SpawnWall(W, FVector(3000.f, 0.f, 0.f));
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		UTDFlightEventCounter* Counter = NewObject<UTDFlightEventCounter>();
		Counter->AddToRoot();
		Counter->Bind(Ship->GetShipMovement());
		// Yaw measured from parallel to the wall (90 = straight in).
		Ship->TeleportSpacecraft(FVector(1500.f, 0.f, 0.f), FRotator(0.f, 90.f - YawIntoWall, 0.f), false);
		Ship->GetShipMovement()->SetForwardSpeedImmediate(6000.f);
		FTDFlightInput In;
		In.Throttle = 1.f;
		Ship->SetFlightInput(In);
		for (int32 i = 0; i < 90 && Counter->Impacts == 0; ++i) { W.Tick(); }
		OutDamage = Counter->LastImpactDamage;
		bOutHeadOn = Counter->bLastImpactHeadOn;
		const bool bHit = Counter->Impacts > 0;
		Counter->RemoveFromRoot();
		return bHit;
	};

	float D20 = 0.f, D40 = 0.f, D90 = 0.f;
	bool H20 = true, H40 = true, H90 = false;
	TestTrue(TEXT("20 deg hit"), Hit(20.f, D20, H20));
	TestTrue(TEXT("40 deg hit"), Hit(40.f, D40, H40));
	TestTrue(TEXT("head-on hit"), Hit(90.f, D90, H90));
	TestFalse(TEXT("20 deg is glancing"), H20);
	TestFalse(TEXT("40 deg is glancing"), H40);
	TestTrue(TEXT("90 deg is head-on"), H90);
	TestTrue(FString::Printf(TEXT("damage grows with angle (20: %.1f, 40: %.1f, 90: %.1f)"), D20, D40, D90), D20 < D40 && D40 < D90);
	TestEqual(TEXT("head-on at cruise = 60"), D90, 60.f, 6.f);

	// Cooldown + pinned contact: fly into the wall and keep pushing.
	{
		FTDTestWorld W;
		TDReviewTest::SpawnWall(W, FVector(3000.f, 0.f, 0.f));
		ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
		UTDFlightEventCounter* Counter = NewObject<UTDFlightEventCounter>();
		Counter->AddToRoot();
		Counter->Bind(Ship->GetShipMovement());
		Ship->TeleportSpacecraft(FVector(2300.f, 0.f, 0.f), FRotator::ZeroRotator, false);
		Ship->GetShipMovement()->SetForwardSpeedImmediate(6000.f);
		FTDFlightInput In;
		In.Throttle = 1.f;
		Ship->SetFlightInput(In);
		W.Advance(3.f);
		TestTrue(FString::Printf(TEXT("pinned against a wall: few damaging hits in 3 s (%d)"), Counter->DamagingImpacts), Counter->DamagingImpacts <= 2);
		const float Cooldown = Ship->GetShipMovement()->GetFlightTuning().CollisionDamageCooldown;
		TestTrue(TEXT("0.3 s damage cooldown bounds the hit rate"), Counter->DamagingImpacts <= FMath::CeilToInt(3.f / Cooldown) + 1);
		const int32 ImpactsSoFar = Counter->Impacts;
		// Resting contact at crawl speed raises no events at all.
		Ship->GetShipMovement()->SetForwardSpeedImmediate(150.f);
		FTDFlightInput Crawl;
		Crawl.Throttle = -1.f;
		Crawl.bBrake = true;
		Ship->GetShipMovement()->ApplyFlightTuning([&]() { FTDFlightTuning Slow = Ship->GetShipMovement()->GetFlightTuning(); Slow.MinSpeed = 150.f; return Slow; }());
		Ship->TeleportSpacecraft(FVector(2560.f, 0.f, 0.f), FRotator::ZeroRotator, false);
		Ship->GetShipMovement()->SetForwardSpeedImmediate(150.f);
		Ship->SetFlightInput(Crawl);
		const int32 Before = Counter->Impacts;
		W.Advance(1.f);
		TestEqual(TEXT("no impact events below MinImpactSpeed"), Counter->Impacts, Before);
		TestTrue(TEXT("events fired earlier"), ImpactsSoFar > 0);
		Counter->RemoveFromRoot();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewFlightEvents, "TriadDrift.Flight.World.BoostDriftEventsBalanced", TD_TEST_FLAGS)
bool FTDReviewFlightEvents::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	UTDFlightEventCounter* Counter = NewObject<UTDFlightEventCounter>();
	Counter->AddToRoot();
	Counter->Bind(Ship->GetShipMovement());

	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Advance(0.5f);
	In.bBoost = false;
	Ship->SetFlightInput(In);
	W.Advance(0.5f);
	TestEqual(TEXT("one boost start"), Counter->BoostStarted, 1);
	TestEqual(TEXT("one boost end"), Counter->BoostEnded, 1);

	In.bDrift = true;
	In.Aim = FVector2D(1.f, 0.f);
	Ship->SetFlightInput(In);
	W.Advance(0.5f);
	TestEqual(TEXT("one drift start"), Counter->DriftStarted, 1);

	// Teleport (portal) with drift held: the drift carries on, no silent end/restart.
	Ship->TeleportSpacecraft(FVector(0.f, 50000.f, 0.f), FRotator(0.f, -90.f, 0.f), true);
	W.Advance(0.3f);
	TestEqual(TEXT("teleport keeps the drift (no second start)"), Counter->DriftStarted, 1);
	TestEqual(TEXT("teleport doesn't end it either"), Counter->DriftEnded, 0);
	TestTrue(TEXT("drifting along the new facing"), Ship->IsDrifting() && Ship->GetVelocity().GetSafeNormal().Y < -0.99f);

	In.bDrift = false;
	Ship->SetFlightInput(In);
	W.Tick();
	TestEqual(TEXT("one drift end"), Counter->DriftEnded, 1);
	TestEqual(TEXT("with its burst"), Counter->DriftBursts, 1);

	// ResetFlightState closes open events and clears everything.
	In.bBoost = true;
	Ship->SetFlightInput(In);
	Ship->SetSpeedOverride(30000.f, 0.5f);
	Ship->GetShipMovement()->SetThrottleTopSpeed(9000.f);
	W.Advance(0.2f);
	Ship->ResetFlightState(false);
	TestFalse(TEXT("override cleared"), Ship->GetShipMovement()->HasSpeedOverride());
	TestEqual(TEXT("catch-up allowance cleared"), Ship->GetShipMovement()->GetThrottleTopSpeed(), Ship->GetShipMovement()->GetFlightTuning().CruiseSpeed);
	TestEqual(TEXT("meter full"), Ship->GetBoost01(), 1.f);
	TestFalse(TEXT("input cleared"), Ship->GetFlightInput().bBoost);
	TestTrue(TEXT("angular rate zeroed"), Ship->GetShipMovement()->GetAngularRate().IsNearlyZero());
	TestEqual(TEXT("starts again at cruise"), (float)Ship->GetVelocity().Size(), 6000.f, 1.f);
	TestEqual(TEXT("events balanced"), Counter->BoostStarted - Counter->BoostEnded, 0);

	Ship->SetFlightParked(true);
	W.Advance(1.f);
	TestTrue(TEXT("parked ships hold still"), Ship->GetVelocity().IsNearlyZero() && Ship->IsFlightParked());
	Ship->SetFlightParked(false);
	TestFalse(TEXT("unparked"), Ship->IsFlightParked());
	Counter->RemoveFromRoot();
	return true;
}

// -------------------------------------------------------------------------------------------- camera

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewCamera, "TriadDrift.Flight.Camera.LagFovRollContinuitySnap", TD_TEST_FLAGS)
bool FTDReviewCamera::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->SetPlayerControlled(true);
	USpringArmComponent* Arm = Ship->GetSpringArm();
	W.Tick();

	// Snap on switch-in: lag off for the cut frame, back on afterwards.
	TestFalse(TEXT("switch-in is a hard cut (no location lag)"), Arm->bEnableCameraLag);
	W.Tick();
	W.Tick();
	TestTrue(TEXT("lag back on after the cut"), Arm->bEnableCameraLag && Arm->bEnableCameraRotationLag);
	TestEqual(TEXT("rotation lag 8"), Arm->CameraRotationLagSpeed, 8.f);
	TestEqual(TEXT("location lag 12"), Arm->CameraLagSpeed, 12.f);

	// FOV: 90 at min speed, 110 at boost.
	FTDFlightInput Slow;
	Slow.Throttle = -1.f;
	Ship->SetFlightInput(Slow);
	W.Advance(4.f);
	TestEqual(TEXT("FOV 90 at min speed"), Ship->GetChaseCamera()->FieldOfView, 90.f, 1.f);
	FTDFlightInput Fast;
	Fast.Throttle = 1.f;
	Fast.bBoost = true;
	Ship->SetFlightInput(Fast);
	W.Advance(2.f);
	TestEqual(TEXT("FOV ~110 at boost speed"), Ship->GetChaseCamera()->FieldOfView, 110.f, 2.5f);

	// Roll follow: 30% of the bank, continuous through a full barrel roll.
	auto MaxStepDegPerSec = [&W, Ship](const FTDFlightInput& In, float Seconds, float& OutMaxRoll)
	{
		Ship->SetFlightInput(In);
		FQuat Prev = Ship->GetChaseTargetQuat();
		float MaxRate = 0.f;
		OutMaxRoll = 0.f;
		const float Dt = 1.f / 60.f;
		for (float T = 0.f; T < Seconds; T += Dt)
		{
			W.Tick(Dt);
			const FQuat Cur = Ship->GetChaseTargetQuat();
			MaxRate = FMath::Max(MaxRate, FMath::RadiansToDegrees(Cur.AngularDistance(Prev)) / Dt);
			OutMaxRoll = FMath::Max(OutMaxRoll, FMath::Abs(Ship->GetCameraRoll()));
			Prev = Cur;
		}
		return MaxRate;
	};
	Ship->ResetFlightState(false);
	Ship->GetShipMovement()->ApplyFlightTuning([&]() { FTDFlightTuning N = Ship->GetShipMovement()->GetFlightTuning(); N.AutoLevelRate = 0.f; return N; }());
	W.Advance(0.5f);
	FTDFlightInput Roll;
	Roll.Throttle = 1.f;
	Roll.Roll = 1.f;
	float MaxRoll = 0.f;
	const float RollRate = MaxStepDegPerSec(Roll, 2.5f, MaxRoll); // > 360 deg of roll at 180 deg/s
	TestTrue(FString::Printf(TEXT("barrel roll: camera target turns smoothly (max %.0f deg/s; a whip is 360+)"), RollRate), RollRate < 140.f);
	TestTrue(FString::Printf(TEXT("roll follow peaks near 30%% of 90 deg (%.1f)"), MaxRoll), MaxRoll > 24.f && MaxRoll < 31.f);

	// Loop: full pitch for 4 s (> 360 deg at 110 deg/s), through both vertical points.
	FTDFlightInput Loop;
	Loop.Throttle = 1.f;
	Loop.Aim = FVector2D(0.f, 1.f);
	float LoopRoll = 0.f;
	const float LoopRate = MaxStepDegPerSec(Loop, 4.f, LoopRoll);
	TestTrue(FString::Printf(TEXT("loop: camera target stays continuous (max %.0f deg/s)"), LoopRate), LoopRate < 260.f);

	// Teleport snaps the camera too.
	Ship->TeleportSpacecraft(FVector(500000.f, 0.f, 0.f), FRotator::ZeroRotator, true);
	TestFalse(TEXT("teleport is a hard cut"), Arm->bEnableCameraLag);
	return true;
}

// --------------------------------------------------------------------------------------------- input

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewInputRouting, "TriadDrift.Flight.Input.InjectedActionsAndPowerRouting", TD_TEST_FLAGS)
bool FTDReviewInputRouting::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector::ZeroVector);
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->SetPlayerControlled(true);
	W.Tick();
	UEnhancedPlayerInput* EPI = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	if (!TestNotNull(TEXT("enhanced player input"), EPI)) { return false; }

	// Inject through Enhanced Input (real trigger processing and our bindings).
	EPI->InjectInputForAction(PC->FindActionByName(TEXT("IA_Boost")), FInputActionValue(true));
	W.Tick();
	TestTrue(TEXT("injected Boost reaches the ship"), Ship->GetFlightInput().bBoost);
	W.Tick(); // no injection this frame = released
	W.Tick();
	TestFalse(TEXT("Boost released"), Ship->GetFlightInput().bBoost);

	EPI->InjectInputForAction(PC->FindActionByName(TEXT("IA_Roll")), FInputActionValue(-1.f));
	W.Tick();
	TestEqual(TEXT("injected Roll axis"), Ship->GetFlightInput().Roll, -1.f, 0.01f);

	FTDSpaceControllerTestAccess Access{ PC };

	// Stick shaping.
	TestTrue(TEXT("inside the deadzone = 0"), ATDSpacePlayerController::ShapeStick(FVector2D(0.1f, 0.f), 0.15f, 1.6f).IsNearlyZero());
	TestEqual(TEXT("full = 1"), (float)ATDSpacePlayerController::ShapeStick(FVector2D(1.f, 0.f), 0.15f, 1.6f).X, 1.f, 0.001f);
	TestEqual(TEXT("mid remapped and curved"), (float)ATDSpacePlayerController::ShapeStick(FVector2D(0.575f, 0.f), 0.15f, 2.f).X, 0.25f, 0.01f);
	TestEqual(TEXT("over-range clamped"), (float)ATDSpacePlayerController::ShapeStick(FVector2D(3.f, 0.f), 0.15f, 1.6f).Size(), 1.f, 0.001f);

	// LT brake threshold (gamepad) and throttle mapping.
	Access.Stick(FVector2D(0.5f, 0.f));
	Access.Throttle(-0.95f);
	W.Tick();
	TestTrue(TEXT("LT past 92% brakes"), Ship->GetFlightInput().bBrake);
	Access.Throttle(-0.5f);
	W.Tick();
	TestFalse(TEXT("half LT doesn't brake"), Ship->GetFlightInput().bBrake);
	TestEqual(TEXT("half LT = Throttle 0"), Ship->GetFlightInput().Throttle, 0.f, 0.01f);
	Access.Throttle(1.f);
	W.Tick();
	TestEqual(TEXT("W / RT raises top speed by 20%"), Ship->GetShipMovement()->GetThrottleTopSpeed(), 7200.f, 1.f);
	Access.Throttle(0.f);
	W.Tick();
	TestEqual(TEXT("released: back to cruise top speed"), Ship->GetShipMovement()->GetThrottleTopSpeed(), 6000.f, 1.f);

	// Device switching.
	TestTrue(TEXT("gamepad after stick input"), Access.UsingGamepad());
	TestTrue(TEXT("virtual stick zeroed on the switch"), Access.VirtualStick().IsNearlyZero());
	Access.Mouse(FVector2D(1.f, 0.f));
	TestTrue(TEXT("tiny mouse nudge doesn't steal aim"), Access.UsingGamepad());
	Access.Mouse(FVector2D(20.f, 0.f));
	TestFalse(TEXT("real mouse movement takes over"), Access.UsingGamepad());

	// Powers: one-power ship fires on press; two-power ship splits tap / hold.
	UTDPowerComponent* Sig1 = TDReviewTest::AddPower(Ship, ETDPowerInput::Signature1, TEXT("TestSig1"));
	TestEqual(TEXT("dome hold is 0.5 s"), Access.HoldSeconds(), 0.5f);
	Access.PowerDown();
	TestTrue(TEXT("no Signature2: Signature1 fires on press"), Sig1->GetCooldown01() < 1.f);
	Access.PowerUp();
	Sig1->ResetCooldown();

	UTDPowerComponent* Sig2 = TDReviewTest::AddPower(Ship, ETDPowerInput::Signature2, TEXT("TestSig2"));
	Access.PowerDown();
	TestEqual(TEXT("two powers: nothing on press"), Sig1->GetCooldown01(), 1.f);
	W.Advance(0.1f);
	Access.PowerUp();
	TestTrue(TEXT("tap = Signature1"), Sig1->GetCooldown01() < 1.f);
	TestEqual(TEXT("tap leaves Signature2 ready"), Sig2->GetCooldown01(), 1.f);
	Sig1->ResetCooldown();
	Access.PowerDown();
	W.Advance(0.6f);
	TestTrue(TEXT("hold 0.5 s = Signature2"), Sig2->GetCooldown01() < 1.f);
	Access.PowerUp();
	TestEqual(TEXT("hold doesn't also fire Signature1"), Sig1->GetCooldown01(), 1.f);

	// Hold Q on Bastion, then switch away: the old ship gets the Signature2 release, and the key-up
	// that arrives after the switch is swallowed (it must not fire the new ship's Signature1).
	Sig1->ResetCooldown();
	Sig2->ResetCooldown();
	UTDTestPowerComponent* Sig1Counter = Cast<UTDTestPowerComponent>(Sig1);
	UTDTestPowerComponent* Sig2Counter = Cast<UTDTestPowerComponent>(Sig2);
	const int32 ReleasesBefore = Sig2Counter ? Sig2Counter->Releases : 0;
	Access.PowerDown();
	W.Advance(0.6f);
	PC->UnPossess();
	TestEqual(TEXT("switching away mid-hold sends the Signature2 release"), Sig2Counter ? Sig2Counter->Releases : -1, ReleasesBefore + 1);
	PC->Possess(Ship);
	Ship->SetPlayerControlled(true);
	W.Tick();
	const int32 Sig1PressesBefore = Sig1Counter ? Sig1Counter->Presses : 0;
	Access.PowerUp(); // the Q key-up after the switch
	TestEqual(TEXT("late key-up after a switch is swallowed"), Sig1Counter ? Sig1Counter->Presses : -1, Sig1PressesBefore);

	UTDPowerComponent* Ability = TDReviewTest::AddPower(Ship, ETDPowerInput::Ability, TEXT("TestAbility"));
	EPI->InjectInputForAction(PC->FindActionByName(TEXT("IA_Ability")), FInputActionValue(true));
	W.Tick();
	TestTrue(TEXT("injected E fires the ability"), Ability->GetCooldown01() < 1.f);

	// Disabled input leaves an autopilot's input alone.
	PC->SetSpaceInputEnabled(false);
	FTDFlightInput Auto;
	Auto.Throttle = -1.f;
	Auto.Aim = FVector2D(0.3f, 0.f);
	Ship->SetFlightInput(Auto);
	W.Advance(0.5f);
	TestEqual(TEXT("disabled controller doesn't overwrite scripted input"), Ship->GetFlightInput().Throttle, -1.f);
	TestEqual(TEXT("scripted aim kept"), (float)Ship->GetFlightInput().Aim.X, 0.3f, 0.001f);
	TestTrue(TEXT("Pause works while paused"), PC->FindActionByName(TEXT("IA_Pause"))->bTriggerWhenPaused);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewAimRay, "TriadDrift.Flight.Input.AimRayIgnoresViewTarget", TD_TEST_FLAGS)
bool FTDReviewAimRay::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(1000.f, -2000.f, 500.f), FRotator(0.f, 30.f, 0.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("controller"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	Ship->SetFlightParked(true); // hold still so directions compare exactly
	W.Tick();
	W.Tick();

	const FVector Centre = PC->GetAimWorldDirectionForViewport(1920, 1080);
	TestTrue(TEXT("centred reticle aims along the nose"), FVector::DotProduct(Centre, Ship->GetActorForwardVector()) > 0.999f);

	FTDSpaceControllerTestAccess Access{ PC };
	Access.Mouse(FVector2D(100.f, 0.f)); // reticle to the right
	const FVector Right = PC->GetAimWorldDirectionForViewport(1920, 1080);
	TestTrue(TEXT("reticle right aims right of the nose"), FVector::DotProduct(Right, Ship->GetActorRightVector()) > 0.05f);

	// A director shot takes the view target: the aim must not move.
	ACameraActor* Director = W.Spawn<ACameraActor>(FVector(50000.f, 40000.f, 9000.f), FRotator(-20.f, -120.f, 15.f));
	PC->SetViewTarget(Director);
	W.Tick();
	W.Tick();
	TestTrue(TEXT("view target really switched"), PC->GetViewTarget() == Director);
	const FVector DuringShot = PC->GetAimWorldDirectionForViewport(1920, 1080);
	TestTrue(FString::Printf(TEXT("aim unchanged by the director camera (dot %.5f)"), FVector::DotProduct(DuringShot, Right)), FVector::DotProduct(DuringShot, Right) > 0.9999f);
	TestTrue(TEXT("headless default viewport works"), !PC->GetAimWorldDirection().IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewDriftPumping, "TriadDrift.Flight.Drift.PumpingNeverExceedsLegalSpeed", TD_TEST_FLAGS)
bool FTDReviewDriftPumping::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	for (const bool bAtmosphere : { false, true })
	{
		FTDFlightContext C;
		C.bBoundaryEnabled = false;
		C.bAtmosphere = bAtmosphere;
		const float Legal = bAtmosphere ? T.CruiseSpeed * C.AtmosphereSpeedCapFraction : T.CruiseSpeed;
		const float Ceiling = Legal * (1.f + T.DriftReleaseBurst);

		FTDFlightState S;
		S.ForwardSpeed = Legal;
		S.Velocity = FVector::ForwardVector * Legal;
		FTDFlightInput In;
		In.Throttle = 1.f;
		In.Aim = FVector2D(0.3f, 0.f);
		float MaxSpeed = 0.f;
		float SustainedAbove = 0.f;
		int32 Bursts = 0;
		for (int32 Frame = 0; Frame < 1200; ++Frame) // 10 s at 120 fps
		{
			In.bDrift = (Frame % 48) < 40; // 0.33 s on, 0.07 s off: the pumping pattern from the review
			TDFlightSim::Advance(S, In, T, C, 1.f / 120.f);
			const float Horizontal = bAtmosphere ? (float)FVector(S.Velocity.X, S.Velocity.Y, 0.f).Size() : (float)S.Velocity.Size();
			MaxSpeed = FMath::Max(MaxSpeed, Horizontal);
			SustainedAbove += (Horizontal > Legal * 1.02f) ? 1.f / 120.f : 0.f;
			Bursts += S.bDriftBurstThisStep ? 1 : 0;
		}
		const TCHAR* Where = bAtmosphere ? TEXT("atmosphere") : TEXT("space");
		TestTrue(FString::Printf(TEXT("%s: pumping never exceeds legal x1.2 (max %.0f, ceiling %.0f)"), Where, MaxSpeed, Ceiling), MaxSpeed <= Ceiling + 1.f);
		TestTrue(FString::Printf(TEXT("%s: the burst cooldown limits bursts (%d in 10 s)"), Where, Bursts), Bursts <= 7);
		TestTrue(FString::Printf(TEXT("%s: no sustained speed above legal (%.1f s of 10)"), Where, SustainedAbove), SustainedAbove < 6.f);
		if (bAtmosphere)
		{
			TestTrue(FString::Printf(TEXT("atmosphere: gravity still wins over pumping (vz %.0f)"), (float)S.Velocity.Z), S.Location.Z < 0.f);
		}
	}

	// Boosting into a drift still earns boost x1.2 (legal: the meter was used).
	FTDFlightState B;
	B.ForwardSpeed = T.BoostSpeed;
	B.Velocity = FVector::ForwardVector * T.BoostSpeed;
	FTDFlightContext Open;
	Open.bBoundaryEnabled = false;
	FTDFlightInput Boost;
	Boost.Throttle = 1.f;
	Boost.bBoost = true;
	for (int32 i = 0; i < 60; ++i) { TDFlightSim::Advance(B, Boost, T, Open, 1.f / 120.f); }
	Boost.bBoost = false;
	Boost.bDrift = true;
	for (int32 i = 0; i < 48; ++i) { TDFlightSim::Advance(B, Boost, T, Open, 1.f / 120.f); }
	const float Before = B.Velocity.Size();
	Boost.bDrift = false;
	TDFlightSim::Advance(B, Boost, T, Open, 1.f / 120.f);
	TestTrue(TEXT("boost -> drift -> release earns the burst"), B.bDriftBurstThisStep);
	TestTrue(FString::Printf(TEXT("burst above cruise x1.2 after a real boost (%.0f)"), (float)B.Velocity.Size()), B.Velocity.Size() > T.CruiseSpeed * 1.2f && B.Velocity.Size() <= Before * 1.2f + 1.f);

	// Braking is always at least as strong as not braking, even far above the burst cap.
	FTDFlightState Fast, FastBrake;
	Fast.ForwardSpeed = FastBrake.ForwardSpeed = 40000.f;
	Fast.Velocity = FastBrake.Velocity = FVector::ForwardVector * 40000.f;
	FTDFlightInput Coast, Brake;
	Coast.Throttle = 1.f;
	Brake.Throttle = 1.f;
	Brake.bBrake = true;
	for (int32 i = 0; i < 60; ++i)
	{
		TDFlightSim::Advance(Fast, Coast, T, Open, 1.f / 120.f);
		TDFlightSim::Advance(FastBrake, Brake, T, Open, 1.f / 120.f);
	}
	TestTrue(TEXT("braking sheds overspeed at least as fast as coasting"), FastBrake.Velocity.Size() <= Fast.Velocity.Size() + 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewMouseSettles, "TriadDrift.Flight.Review.MouseTurnSettlesWingsLevel", TD_TEST_FLAGS)
bool FTDReviewMouseSettles::RunTest(const FString& Parameters)
{
	// Lead playtest: after a mouse turn the ship stayed banked ~30 deg for 10+ s with no input.
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("local space controller"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	W.Tick();

	// A hard mouse turn: 0.5 s of mouse movement every frame (the controller's Mouse2D handler; the headless
	// world has no viewport to pair MouseX/MouseY into Mouse2D).
	FTDSpaceControllerTestAccess Access{ PC };
	for (int32 i = 0; i < 30; ++i)
	{
		Access.Mouse(FVector2D(20.f, 0.f));
		W.Tick();
	}
	const float TurningBank = FMath::Abs(Ship->GetShipMovement()->GetVisualBank());
	TestTrue(*FString::Printf(TEXT("mouse turn banks the hull (%.1f deg)"), TurningBank), TurningBank > 10.f);
	TestTrue(TEXT("mouse turn steers"), FMath::Abs(PC->GetCurrentFlightInput().Aim.X) > 0.3f);

	// Mouse still: the stick recentres and the hull is wings-level within ~1.5 s.
	W.Advance(1.5f);
	TestTrue(*FString::Printf(TEXT("aim back to zero (%.2f)"), PC->GetCurrentFlightInput().Aim.Size()), PC->GetCurrentFlightInput().Aim.IsNearlyZero(0.01f));
	TestTrue(*FString::Printf(TEXT("visual bank settled (%.1f deg)"), Ship->GetShipMovement()->GetVisualBank()), FMath::Abs(Ship->GetShipMovement()->GetVisualBank()) < 3.f);
	TestTrue(*FString::Printf(TEXT("flight frame wings-level (%.1f deg)"), Ship->GetActorRotation().Roll), FMath::Abs(Ship->GetActorRotation().Roll) < 3.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewGroundFloorSim, "TriadDrift.Flight.Review.AtmosphereGroundFloorSim", TD_TEST_FLAGS)
bool FTDReviewGroundFloorSim::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	FTDFlightContext C = TDReviewTest::Open();
	C.bAtmosphere = true;
	FTDFlightInput In;
	for (const float Fps : { 30.f, 60.f, 120.f })
	{
		const FString Tag = FString::Printf(TEXT("%.0f fps: "), Fps);
		FTDFlightState S = TDReviewTest::Cruising(T);
		S.Rotation = FRotator(-30.f, 0.f, 0.f).Quaternion();
		S.Velocity = S.Rotation.GetForwardVector() * S.ForwardSpeed;
		C.GroundClearance = 300.f; // 3 m above the ground, diving 30 deg
		for (int32 i = 0; i < FMath::RoundToInt(Fps); ++i)
		{
			TDFlightSim::Advance(S, In, T, C, 1.f / Fps);
		}
		TestTrue(*(Tag + FString::Printf(TEXT("nose pulled to level (pitch %.1f)"), S.Rotation.Rotator().Pitch)), S.Rotation.Rotator().Pitch > -2.f);
		TestTrue(*(Tag + FString::Printf(TEXT("sink cancelled (vz %.0f)"), S.Velocity.Z)), S.Velocity.Z > -T.CruiseSpeed * 0.06f);
		TestTrue(*(Tag + TEXT("no pull-up past level")), S.Rotation.Rotator().Pitch < 2.f);

		// No ground in range: the dive is untouched by the floor.
		FTDFlightState Free = TDReviewTest::Cruising(T);
		Free.Rotation = FRotator(-30.f, 0.f, 0.f).Quaternion();
		C.GroundClearance = -1.f;
		TDFlightSim::Advance(Free, In, T, C, 0.5f);
		TestTrue(*(Tag + TEXT("high up: dive kept")), Free.Rotation.Rotator().Pitch < -25.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewGroundFloorWorld, "TriadDrift.Flight.Review.AtmosphereHandsOffNeverHitsTheGround", TD_TEST_FLAGS)
bool FTDReviewGroundFloorWorld::RunTest(const FString& Parameters)
{
	// Live playtest (Nov 12): arriving nose-down on Verdance, a hands-off ship flew into the plain and skidded.
	FTDTestWorld W;
	AActor* Ground = TDReviewTest::SpawnWall(W, FVector(0.f, 0.f, -200.f));
	Cast<UBoxComponent>(Ground->GetRootComponent())->SetBoxExtent(FVector(400000.f, 400000.f, 200.f)); // top at Z = 0
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 4000.f), FRotator(-30.f, 0.f, 0.f));
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	Move->SetAtmosphereMode(true, 1200.f, 0.6f);
	W.Tick();
	const float Radius = Ship->GetRootComponent()->Bounds.SphereRadius;
	float MinGap = TNumericLimits<float>::Max();
	for (int32 i = 0; i < 360; ++i) // 6 s hands-off
	{
		W.Tick();
		MinGap = FMath::Min(MinGap, (float)Ship->GetActorLocation().Z - Radius);
	}
	TestTrue(*FString::Printf(TEXT("hull stays clear of the ground (min gap %.0f cm)"), MinGap), MinGap > 50.f);
	TestTrue(*FString::Printf(TEXT("levelled off (pitch %.1f)"), Ship->GetActorRotation().Pitch), FMath::Abs(Ship->GetActorRotation().Pitch) < 5.f);
	TestTrue(*FString::Printf(TEXT("still flying (speed %.0f)"), Ship->GetVelocity().Size()), Ship->GetVelocity().Size() > 1500.f);
	TestFalse(TEXT("no damage taken"), Ship->GetHealth() && Ship->GetHealth()->GetHull01() < 0.999f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewSwitchResetsMouseStick, "TriadDrift.Flight.Review.SwitchStartsStraight", TD_TEST_FLAGS)
bool FTDReviewSwitchResetsMouseStick::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 1500.f));
	ATDShip* Other = W.Spawn<ATDShip>(FVector(0.f, 5000.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("local space controller"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	W.Tick();
	FTDSpaceControllerTestAccess Access{ PC };
	Access.Mouse(FVector2D(150.f, 0.f));
	W.Tick();
	TestTrue(TEXT("mouse offset steers the first ship"), PC->GetCurrentFlightInput().Aim.X > 0.3f);

	Ship->SetPlayerControlled(false);
	PC->Possess(Other);
	Other->SetPlayerControlled(true);
	TestTrue(TEXT("stick recentred on the switch"), Access.VirtualStick().IsNearlyZero());
	TestTrue(TEXT("the new ship gets no turn on its first frame"), Other->GetFlightInput().Aim.IsNearlyZero());
	W.Tick();
	TestTrue(TEXT("still straight after a frame"), PC->GetCurrentFlightInput().Aim.IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDReviewBoundaryExperience, "TriadDrift.Flight.Review.BoundaryWarnsTurnsBackNeverTeleports", TD_TEST_FLAGS)
bool FTDReviewBoundaryExperience::RunTest(const FString& Parameters)
{
	// The edge of the galaxy: warning text and one event per state change, a gentle turn-back, never a wall or a
	// teleport, and a pilot holding the stick outward can't escape.
	for (const bool bPilotFightsIt : { false, true })
	{
		const FString Tag = bPilotFightsIt ? TEXT("pilot steering out: ") : TEXT("hands-off: ");
		FTDTestWorld W;
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 0.f));
		UTDShipMovementComponent* Move = Ship->GetShipMovement();
		const float Radius = 200000.f;
		Move->SetBoundary(FVector::ZeroVector, Radius, true);
		Ship->TeleportSpacecraft(FVector(Radius - 60000.f, 0.f, 0.f), FRotator::ZeroRotator, false); // nose out, 600 m inside
		UTDFlightEventCounter* Counter = NewObject<UTDFlightEventCounter>();
		Move->OnBoundaryStateChanged.AddDynamic(Counter, &UTDFlightEventCounter::HandleBoundary);
		W.Tick();
		TestEqual(*(Tag + TEXT("starts inside")), Move->GetBoundaryState(), ETDBoundaryState::Inside);
		TestTrue(*(Tag + TEXT("no text inside")), Move->GetBoundaryWarningText().IsEmpty());

		float MaxDist = 0.f;
		float MaxStep = 0.f;
		bool bSawWarning = false, bSawOutside = false;
		FVector Last = Ship->GetActorLocation();
		for (int32 i = 0; i < 60 * 40; ++i)
		{
			FTDFlightInput In;
			In.Throttle = 1.f;
			if (bPilotFightsIt)
			{
				// Always steer the nose away from the centre.
				const FVector Out = Ship->GetActorLocation().GetSafeNormal();
				In.Aim.X = FVector::DotProduct(Ship->GetActorRightVector(), Out) >= 0.f ? 1.f : -1.f;
			}
			Ship->SetFlightInput(In);
			W.Tick();
			const FVector Now = Ship->GetActorLocation();
			MaxStep = FMath::Max(MaxStep, (float)FVector::Dist(Now, Last));
			Last = Now;
			MaxDist = FMath::Max(MaxDist, (float)Now.Size());
			if (Move->GetBoundaryState() == ETDBoundaryState::Warning)
			{
				bSawWarning = true;
				TestEqual(*(Tag + TEXT("warning text")), Move->GetBoundaryWarningText().ToString(), FString(TEXT("EDGE OF THE COMBAT ZONE")));
			}
			if (Move->GetBoundaryState() == ETDBoundaryState::Outside && !bSawOutside)
			{
				bSawOutside = true;
				TestEqual(*(Tag + TEXT("outside text")), Move->GetBoundaryWarningText().ToString(), FString(TEXT("LEAVING THE COMBAT ZONE - TURN BACK")));
			}
		}
		TestTrue(*(Tag + TEXT("warned before the edge")), bSawWarning);
		TestTrue(*(Tag + TEXT("crossed the edge")), bSawOutside);
		TestTrue(*(Tag + FString::Printf(TEXT("turned back within 300 m past the edge (max %.0f m past)"), (MaxDist - Radius) / 100.f)), MaxDist < Radius + 30000.f);
		const float SpeedCap = Move->GetFlightTuning().BoostSpeed;
		TestTrue(*(Tag + FString::Printf(TEXT("no teleport: largest frame step %.0f cm"), MaxStep)), MaxStep < SpeedCap / 60.f * 1.5f);
		if (!bPilotFightsIt)
		{
			TestTrue(*(Tag + TEXT("ends back inside")), Ship->GetActorLocation().Size() < Radius);
			TestTrue(*(Tag + TEXT("events: inside -> warning -> outside, then back")), Counter->BoundaryStates.Num() >= 4
				&& Counter->BoundaryStates[0] == ETDBoundaryState::Warning && Counter->BoundaryStates[1] == ETDBoundaryState::Outside);
			for (int32 i = 1; i < Counter->BoundaryStates.Num(); ++i)
			{
				TestTrue(*(Tag + TEXT("one event per change")), Counter->BoundaryStates[i] != Counter->BoundaryStates[i - 1]);
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
