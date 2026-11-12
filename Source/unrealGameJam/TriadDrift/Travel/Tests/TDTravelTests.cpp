// Automation tests for travel (ENG-15 hyperdrive, ENG-20 atmosphere + landing). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDGravityWellMarker.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Flight/TDFlightSim.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Travel/Tests/TDTravelTestHelpers.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"

namespace TDTravelTest
{
	ATDShip* SpawnShip(FTDTestWorld& W, const FVector& Location, const FRotator& Facing = FRotator::ZeroRotator)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(Location, Facing);
		if (Ship)
		{
			FTDFlightInput In;
			In.Throttle = 1.f;
			Ship->SetFlightInput(In);
		}
		return Ship;
	}

	/** Ticks until Pred is true or Seconds pass; returns the time it took (or -1). */
	template <typename TPred>
	float TickUntil(FTDTestWorld& W, float Seconds, TPred Pred, float Dt = 1.f / 60.f)
	{
		for (float T = 0.f; T <= Seconds; T += Dt)
		{
			if (Pred())
			{
				return T;
			}
			W.Tick(Dt);
		}
		return Pred() ? Seconds : -1.f;
	}
}


// =================================================================================== hyperdrive

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelHyperSpool, "TriadDrift.Travel.Hyperdrive.SpoolJumpAndSteering", TD_TEST_FLAGS)
bool FTDTravelHyperSpool::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("hyperdrive"), Ship->GetHyperdrive())) { return false; }
	Ship->SetPlayerControlled(true);
	UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	W.Advance(0.5f);

	TestEqual(TEXT("features.md spool 1.2 s"), Hyper->Tuning.SpoolSeconds, 1.2f);
	TestEqual(TEXT("features.md jump 80 000 cm/s"), Hyper->Tuning.JumpSpeed, 80000.f);
	TestEqual(TEXT("features.md 10% steering"), Hyper->Tuning.JumpSteeringScale, 0.1f);

	TestTrue(TEXT("spool starts"), Hyper->StartSpool());
	TestFalse(TEXT("can't double-start"), Hyper->StartSpool());
	W.Advance(1.1f);
	TestEqual(TEXT("still spooling at 1.1 s"), Hyper->GetState(), ETDHyperdriveState::Spooling);
	TestEqual(TEXT("charge ~0.92 at 1.1 s"), Hyper->GetCharge01(), 1.1f / 1.2f, 0.03f);
	W.Advance(0.15f);
	TestEqual(TEXT("jumps after 1.2 s"), Hyper->GetState(), ETDHyperdriveState::Jumping);
	TestEqual(TEXT("charge full while jumping"), Hyper->GetCharge01(), 1.f);

	FTDFlightInput Turn;
	Turn.Throttle = 1.f;
	Turn.Aim = FVector2D(1.f, 0.f);
	Ship->SetFlightInput(Turn);
	W.Advance(1.f);
	TestEqual(TEXT("cruising at 80 000 cm/s"), (float)Ship->GetVelocity().Size(), 80000.f, 800.f);
	const float YawRate = Ship->GetShipMovement()->GetAngularRate().Z;
	TestTrue(FString::Printf(TEXT("10%% steering: yaw rate %.1f <= 11 deg/s"), YawRate), YawRate <= 11.f + 0.01f && YawRate > 9.f);

	// Too early for a manual drop-out, then accepted.
	Hyper->Tuning.MinJumpSeconds = 5.f;
	TestFalse(TEXT("manual drop-out refused before MinJumpSeconds"), Hyper->RequestDropOut());
	Hyper->Tuning.MinJumpSeconds = 0.5f;
	TestTrue(TEXT("manual drop-out accepted"), Hyper->RequestDropOut());
	TestEqual(TEXT("drop-out state"), Hyper->GetState(), ETDHyperdriveState::DropOut);
	W.Advance(0.5f);
	TestEqual(TEXT("drop-out burst speed"), (float)Ship->GetVelocity().Size(), Hyper->Tuning.DropOutSpeed, 1500.f);
	W.Advance(0.3f);
	TestEqual(TEXT("back to idle after the drop-out"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestFalse(TEXT("override released"), Ship->GetShipMovement()->HasSpeedOverride());
	Ship->SetFlightInput(FTDFlightInput());
	W.Advance(4.f);
	TestTrue(TEXT("burst bleeds back toward throttle speed"), Ship->GetVelocity().Size() < 6000.f);

	// Releasing during a spool cancels it.
	W.Advance(1.5f); // past the cooldown
	TestTrue(TEXT("spool again"), Hyper->StartSpool());
	W.Advance(0.5f);
	Hyper->StopSpool();
	TestEqual(TEXT("release cancels the spool"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestEqual(TEXT("charge reset"), Hyper->GetCharge01(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelHyperDamage, "TriadDrift.Travel.Hyperdrive.DamageBlocksAndCancels", TD_TEST_FLAGS)
bool FTDTravelHyperDamage::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector::ZeroVector);
	AActor* Shooter = W.Spawn<AActor>(FVector(5000.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->SetPlayerControlled(true);
	UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	W.Advance(0.5f);

	TestTrue(TEXT("spooling"), Hyper->StartSpool());
	W.Advance(0.5f);
	UTDStatics::ApplyTDDamage(Ship, 5.f, ETDDamageKind::Energy, Shooter, Shooter);
	TestEqual(TEXT("a hit cancels the spool"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestEqual(TEXT("blocked by recent damage"), Hyper->GetBlockReason(), ETDHyperdriveBlock::RecentDamage);
	TestFalse(TEXT("can't spool within 2 s of damage"), Hyper->StartSpool());
	W.Advance(1.8f);
	TestFalse(TEXT("still locked at 1.8 s"), Hyper->StartSpool());
	W.Advance(0.3f);
	TestTrue(TEXT("free again after 2 s"), Hyper->StartSpool());

	// Atmosphere blocks it entirely.
	Hyper->CancelSpool();
	Ship->GetShipMovement()->SetAtmosphereMode(true);
	TestEqual(TEXT("no hyperdrive in atmosphere"), Hyper->GetBlockReason(), ETDHyperdriveBlock::InAtmosphere);
	TestFalse(TEXT("spool refused in atmosphere"), Hyper->StartSpool());
	Ship->GetShipMovement()->SetAtmosphereMode(false);

	// Disabled (cutscene / entry).
	Hyper->SetHyperdriveEnabled(false);
	TestFalse(TEXT("spool refused while disabled"), Hyper->StartSpool());
	Hyper->SetHyperdriveEnabled(true);
	TestTrue(TEXT("re-enabled"), Hyper->CanSpool());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelHyperWell, "TriadDrift.Travel.Hyperdrive.DropsOutAtGravityWell", TD_TEST_FLAGS)
bool FTDTravelHyperWell::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("travel subsystem"), Travel)) { return false; }
	const FVector WellAt(600000.f, 0.f, 0.f);
	Travel->RegisterGravityWell(TEXT("Planet_Test"), WellAt, 15000.f);

	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->SetPlayerControlled(true);
	UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	W.Advance(0.2f);
	TestTrue(TEXT("spool"), Hyper->StartSpool());

	const float Took = TDTravelTest::TickUntil(W, 15.f, [Hyper]() { return Hyper->GetState() == ETDHyperdriveState::DropOut; });
	TestTrue(TEXT("dropped out"), Took > 0.f);
	TestEqual(TEXT("dropped out at the registered well"), Hyper->GetLastDropOutWell(), FName(TEXT("Planet_Test")));
	const float Dist = FVector::Dist(Ship->GetActorLocation(), WellAt);
	TestTrue(FString::Printf(TEXT("stopped at the well's edge (%.0f cm from centre, radius 15000)"), Dist), Dist <= 15000.f + 3000.f && Dist > 5000.f);

	// Inside a well: no spooling.
	W.Advance(2.f);
	Ship->TeleportSpacecraft(WellAt + FVector(2000.f, 0.f, 0.f), FRotator::ZeroRotator, false);
	TestEqual(TEXT("blocked inside the well"), Hyper->GetBlockReason(), ETDHyperdriveBlock::NearGravityWell);

	// A well that follows an actor (marker).
	ATDGravityWellMarker* Marker = W.Spawn<ATDGravityWellMarker>(FVector(0.f, 400000.f, 0.f));
	TestTrue(TEXT("marker registered itself"), Travel->GetGravityWells().ContainsByPredicate([Marker](const FTDTravelGravityWell& Well) { return Well.Owner.Get() == Marker; }));
	Marker->SetWellEnabled(false);
	TestFalse(TEXT("disabled marker unregistered"), Travel->GetGravityWells().ContainsByPredicate([Marker](const FTDTravelGravityWell& Well) { return Well.Owner.Get() == Marker; }));

	// The segment check catches a well the jump would skip in a single frame.
	FTDTravelGravityWell Found;
	TestTrue(TEXT("segment through a well"), Travel->FindWellOnSegment(FVector(500000.f, 0.f, 0.f), FVector(700000.f, 0.f, 0.f), Found));
	TestFalse(TEXT("segment far from any well"), Travel->FindWellOnSegment(FVector(0.f, -100000.f, 0.f), FVector(10000.f, -100000.f, 0.f), Found));
	return true;
}

// =================================================================================== atmosphere

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelAtmosphereSim, "TriadDrift.Travel.Atmosphere.SpeedCapAndGravity", TD_TEST_FLAGS)
bool FTDTravelAtmosphereSim::RunTest(const FString& Parameters)
{
	const FTDFlightTuning T;
	FTDFlightContext C;
	C.bBoundaryEnabled = false;
	C.bAtmosphere = true;

	FTDFlightState S;
	S.ForwardSpeed = T.CruiseSpeed;
	S.Velocity = FVector::ForwardVector * T.CruiseSpeed;
	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;
	for (int32 i = 0; i < 240; ++i) { TDFlightSim::Advance(S, In, T, C, 1.f / 60.f); }

	const float Horizontal = FVector(S.Velocity.X, S.Velocity.Y, 0.f).Size();
	TestEqual(TEXT("speed capped at 60% of space cruise (3600)"), Horizontal, 0.6f * T.CruiseSpeed, 60.f);
	TestFalse(TEXT("no boost in atmosphere"), S.bBoosting);
	TestEqual(TEXT("boost meter untouched"), S.Boost01, 1.f, 0.001f);
	TestTrue(FString::Printf(TEXT("gravity: the ship sinks (vz %.0f cm/s)"), (float)S.Velocity.Z), S.Velocity.Z < -50.f && S.Velocity.Z > -400.f);

	// Box boundary: above the ceiling the ship is turned and pushed back down.
	FTDFlightContext Box = C;
	Box.bBoxBoundary = true;
	Box.bBoundaryEnabled = true;
	Box.BoundaryBox = FBox(FVector(-100000.f), FVector(100000.f, 100000.f, 20000.f));
	FTDFlightState Up;
	Up.Location = FVector(0.f, 0.f, 24000.f);
	Up.Rotation = FRotator(60.f, 0.f, 0.f).Quaternion();
	Up.ForwardSpeed = 3600.f;
	Up.Velocity = Up.Rotation.GetForwardVector() * 3600.f;
	FTDFlightInput Level;
	Level.Throttle = 1.f;
	TDFlightSim::Advance(Up, Level, T, Box, 1.f / 60.f);
	TestTrue(TEXT("above the ceiling = out of bounds"), Up.bOutOfBounds);
	for (int32 i = 0; i < 60 * 8; ++i) { TDFlightSim::Advance(Up, Level, T, Box, 1.f / 60.f); }
	TestTrue(FString::Printf(TEXT("pushed back under the ceiling (z %.0f)"), (float)Up.Location.Z), Up.Location.Z < 21000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelAtmosphereZone, "TriadDrift.Travel.Atmosphere.ZoneModeAltitudeAndExit", TD_TEST_FLAGS)
bool FTDTravelAtmosphereZone::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDAtmosphereFlightZone* Zone = W.Spawn<ATDAtmosphereFlightZone>(FVector::ZeroVector);
	if (!TestNotNull(TEXT("zone"), Zone)) { return false; }
	const FBox Box = Zone->GetZoneBox();

	// A ground plane so altitude has something to measure against.
	ATDLandingPad* Ground = W.Spawn<ATDLandingPad>(FVector(0.f, 0.f, -2000.f));
	TestNotNull(TEXT("ground"), Ground);

	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector(0.f, 0.f, 3000.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	APlayerController* PC = W.SpawnPlayerController(Ship, ATDSpacePlayerController::StaticClass());
	Ship->SetPlayerControlled(true);
	W.Tick();
	W.Tick();

	TestTrue(TEXT("ship flies in atmosphere mode inside the zone"), Ship->GetShipMovement()->IsInAtmosphere());
	if (ATDSpacePlayerController* SPC = Cast<ATDSpacePlayerController>(PC))
	{
		TestEqual(TEXT("player input switched to the atmosphere layer"), SPC->GetFlightContext(), ETDFlightContext::Atmosphere);
	}
	const float Alt = Ship->GetAltitudeMeters();
	TestTrue(FString::Printf(TEXT("altitude measured (%.1f m)"), Alt), Alt > 40.f && Alt < 55.f);
	TestTrue(TEXT("hyperdrive blocked"), !Ship->GetHyperdrive()->CanSpool());
	W.Advance(3.f);
	TestEqual(TEXT("player ship capped at 60% cruise"), (float)Ship->GetVelocity().Size(), 3600.f, 200.f);

	// Exit closed: climbing past the ceiling doesn't leave.
	Zone->SetExitAllowed(false);
	Ship->TeleportSpacecraft(FVector(0.f, 0.f, Box.Max.Z - 200.f), FRotator(80.f, 0.f, 0.f), true);
	W.Advance(4.f);
	TestFalse(TEXT("exit stays shut until objectives are done"), Zone->IsExitTriggered());
	TestTrue(FString::Printf(TEXT("turned back by the ceiling (%.0f cm above it)"), (float)(Ship->GetActorLocation().Z - Box.Max.Z)),
		Ship->GetActorLocation().Z < Box.Max.Z + 12000.f && Ship->GetVelocity().Z < 0.f);

	// Exit open: climb through.
	Zone->SetExitAllowed(true);
	Ship->TeleportSpacecraft(FVector(0.f, 0.f, Box.Max.Z + 1000.f), FRotator(80.f, 0.f, 0.f), true);
	const float Took = TDTravelTest::TickUntil(W, 6.f, [Zone]() { return Zone->IsExitTriggered(); });
	TestTrue(TEXT("take-off exit through the ceiling once allowed"), Took >= 0.f);

	// Objectives gate: without a game flow the required list keeps the exit shut.
	ATDAtmosphereFlightZone* Gated = W.Spawn<ATDAtmosphereFlightZone>(FVector(0.f, 0.f, 0.f));
	Gated->RequiredObjectives = { TEXT("Obj_WreckCore") };
	TestFalse(TEXT("required objectives gate the exit"), Gated->IsExitAllowed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelAtmosphereEntry, "TriadDrift.Travel.Atmosphere.EntryCommitsAndHandsBack", TD_TEST_FLAGS)
bool FTDTravelAtmosphereEntry::RunTest(const FString& Parameters)
{
	FTDTestWorld W; // plain game instance: no ITDGameFlow, so travel can't happen
	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("entry component"), Ship->GetAtmosphereEntry())) { return false; }
	UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
	W.Advance(0.5f);

	TestEqual(TEXT("features.md 2.5 s entry"), Entry->EntrySeconds, 2.5f);
	TestTrue(TEXT("entry begins"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Verdance_Orbit"), FVector(-200000.f, 0.f, 0.f)));
	TestFalse(TEXT("no second entry"), Entry->BeginEntry(ETDDestination::Cinder, NAME_None, FVector::ZeroVector));
	TestTrue(TEXT("nose turned into the planet"), Ship->GetActorForwardVector().X < -0.99f);
	TestEqual(TEXT("hyperdrive off during entry"), Ship->GetHyperdrive()->GetBlockReason(), ETDHyperdriveBlock::Disabled);
	W.Advance(1.f);
	TestTrue(TEXT("entering"), Entry->IsEntering());
	TestEqual(TEXT("entry speed held"), (float)Ship->GetVelocity().Size(), Entry->EntrySpeed, 300.f);
	TestEqual(TEXT("Entry01 ~0.4 at 1 s"), Entry->GetEntry01(), 1.f / 2.5f, 0.05f);
	W.Advance(1.6f);
	TestFalse(TEXT("no flow: entry ends and hands control back"), Entry->IsEntering());
	TestFalse(TEXT("speed override released"), Ship->GetShipMovement()->HasSpeedOverride());
	TestTrue(TEXT("hyperdrive back on"), Ship->GetHyperdrive()->GetBlockReason() != ETDHyperdriveBlock::Disabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelPlayBounds, "TriadDrift.Travel.PlayBounds.AppliedToEveryShip", TD_TEST_FLAGS)
bool FTDTravelPlayBounds::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* A = TDTravelTest::SpawnShip(W, FVector(60000.f, 0.f, 0.f));
	W.Advance(0.2f);
	TestFalse(TEXT("default 15 km bounds: not out yet"), A->IsOutOfBounds());

	UTDTravelSubsystem::Get(W.GetWorld())->SetPlayBounds(FVector::ZeroVector, 50000.f);
	W.Tick();
	W.Tick();
	TestTrue(TEXT("existing ship picks up the layout's bounds"), A->IsOutOfBounds());

	ATDShip* B = TDTravelTest::SpawnShip(W, FVector(-70000.f, 0.f, 0.f));
	W.Tick();
	W.Tick();
	TestTrue(TEXT("ships spawned later pick them up too"), B->IsOutOfBounds());
	return true;
}

// ====================================================================================== landing

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelLanding, "TriadDrift.Travel.Landing.PromptSequenceWingmenTakeOff", TD_TEST_FLAGS)
bool FTDTravelLanding::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDLandingPad* Pad = W.Spawn<ATDLandingPad>(FVector::ZeroVector);
	ATDLandingPad* SidePad = W.Spawn<ATDLandingPad>(FVector(0.f, 3000.f, 0.f));
	if (!TestNotNull(TEXT("pads"), Pad) || !SidePad) { return false; }
	// Mark the second pad as a wingman pad through its editable property (no setter by design).
	if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(ATDLandingPad::StaticClass(), TEXT("bWingmanPad")))
	{
		Prop->SetPropertyValue_InContainer(SidePad, true);
	}
	TestTrue(TEXT("side pad is a wingman pad"), SidePad->IsWingmanPad());

	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector(-1500.f, 0.f, 1500.f));
	ATDShip* Mate = TDTravelTest::SpawnShip(W, FVector(-2000.f, 5000.f, 2500.f));
	if (!TestNotNull(TEXT("ships"), Ship) || !Mate) { return false; }
	Mate->SetShipId(ETDShipId::Bastion);
	Ship->SetPlayerControlled(true);
	UTDLandingComponent* Land = Ship->GetLanding();
	W.Tick();

	// Prompt: within 3000 cm and below 40% of cruise.
	Ship->GetShipMovement()->SetForwardSpeedImmediate(5000.f);
	TestFalse(TEXT("too fast to land (5000 > 40% of 6000)"), Land->CanLandNow());
	Ship->GetShipMovement()->SetForwardSpeedImmediate(2000.f);
	TestTrue(TEXT("slow and close: landing offered"), Land->CanLandNow());
	TestEqual(TEXT("prompt text"), Land->GetLandPrompt().ToString(), FString(TEXT("Hold F to land")));
	Ship->TeleportSpacecraft(FVector(-4000.f, 0.f, 1500.f), FRotator::ZeroRotator, false);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(2000.f);
	TestFalse(TEXT("out of range: no prompt"), Land->CanLandNow());
	Ship->TeleportSpacecraft(FVector(-1500.f, 0.f, 1500.f), FRotator::ZeroRotator, false);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(2000.f);

	TestTrue(TEXT("land"), Land->RequestLand());
	TestTrue(TEXT("pad reserved"), Pad->GetOccupant() == Ship);
	TestTrue(TEXT("wingman sent to the side pad"), Mate->GetLanding()->IsBusy() && SidePad->GetOccupant() == Mate);

	W.Advance(2.f); // align (1.5) + into gear
	TestTrue(TEXT("gear down mid-sequence"), Land->IsGearDown());
	TestFalse(TEXT("canopy still closed"), Land->IsCanopyOpen());
	const float Mid = Land->GetLanding01();
	TestTrue(FString::Printf(TEXT("Landing01 mid-way (%.2f)"), Mid), Mid > 0.2f && Mid < 0.8f);

	W.Advance(3.2f); // total 4.9 s
	TestTrue(TEXT("landed"), Land->IsLanded());
	TestTrue(TEXT("canopy open"), Land->IsCanopyOpen());
	TestFalse(TEXT("engines off"), Land->AreEnginesOn());
	TestEqual(TEXT("Landing01 = 1"), Land->GetLanding01(), 1.f);
	const FVector Touch = Pad->GetBeaconLocation();
	TestTrue(FString::Printf(TEXT("sitting on the pad (%s)"), *Ship->GetActorLocation().ToString()),
		FVector::Dist2D(Ship->GetActorLocation(), Touch) < 10.f && FMath::Abs(Ship->GetActorLocation().Z - Touch.Z - 225.f) < 10.f);
	TestTrue(TEXT("stationary"), Ship->GetVelocity().IsNearlyZero(1.f));
	TestTrue(TEXT("wingman landed beside"), Mate->GetLanding()->IsLanded());

	// Pilot exit point for the first-person hand-off: to the right, at capsule height above the deck.
	const FTransform Exit = Land->GetExitTransform();
	const FVector ToExit = Exit.GetLocation() - Ship->GetActorLocation();
	TestTrue(TEXT("exit is on the ship's right"), FVector::DotProduct(ToExit.GetSafeNormal2D(), Ship->GetActorRightVector()) > 0.99f);
	TestEqual(TEXT("exit offset"), (float)ToExit.Size2D(), Land->Tuning.ExitSideOffset, 1.f);
	TestEqual(TEXT("exit at capsule height above the deck"), (float)Exit.GetLocation().Z, (float)Touch.Z + Land->Tuning.ExitHeightAboveDeck, 1.f);
	TestTrue(TEXT("exit faces the ship's forward"), Exit.GetRotation().GetForwardVector().Equals(Ship->GetActorForwardVector(), 0.01f));

	// Flight input is ignored while landed.
	FTDFlightInput Full;
	Full.Throttle = 1.f;
	Full.bBoost = true;
	Ship->SetFlightInput(Full);
	const FVector Before = Ship->GetActorLocation();
	W.Advance(1.f);
	TestTrue(TEXT("stays put while landed"), Ship->GetActorLocation().Equals(Before, 1.f));

	// Take-off reverses it and lifts the wingman too.
	TestTrue(TEXT("take-off"), Land->RequestTakeOff());
	W.Advance(1.f);
	TestFalse(TEXT("canopy closes first"), Land->IsCanopyOpen());
	W.Advance(2.7f); // total 3.5 s + margin
	TestEqual(TEXT("flying again"), Land->GetLandingState(), ETDLandingState::Flying);
	TestFalse(TEXT("gear up"), Land->IsGearDown());
	TestTrue(TEXT("climbed off the pad"), Ship->GetActorLocation().Z > Touch.Z + 1500.f);
	TestTrue(TEXT("pad released"), !Pad->IsOccupied());
	TestTrue(TEXT("wingman took off too"), !Mate->GetLanding()->IsBusy() && !SidePad->IsOccupied());
	W.Tick();
	TestTrue(TEXT("flight resumes with forward speed"), Ship->GetVelocity().Size() > 1500.f);

	// Can't land twice / take off while flying.
	TestFalse(TEXT("no take-off while flying"), Land->RequestTakeOff());
	return true;
}

// ======================================================================================= camera

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelCockpit, "TriadDrift.Travel.Camera.CockpitToggle", TD_TEST_FLAGS)
bool FTDTravelCockpit::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector::ZeroVector);
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("controller"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	W.Tick();
	TestTrue(TEXT("chase camera by default"), Ship->GetChaseCamera()->IsActive() && !Ship->GetCockpitCamera()->IsActive());
	Ship->ToggleCockpitView();
	TestTrue(TEXT("cockpit camera after toggle"), Ship->IsCockpitView() && Ship->GetCockpitCamera()->IsActive() && !Ship->GetChaseCamera()->IsActive());
	Ship->SetPlayerControlled(false);
	TestFalse(TEXT("no camera active on a wingman"), Ship->GetCockpitCamera()->IsActive() || Ship->GetChaseCamera()->IsActive());
	Ship->SetPlayerControlled(true);
	Ship->ToggleCockpitView();
	TestTrue(TEXT("back to chase"), Ship->GetChaseCamera()->IsActive() && !Ship->GetCockpitCamera()->IsActive());

	bool bV = false;
	for (const FEnhancedActionKeyMapping& M : PC->GetCommonContext()->GetMappings())
	{
		bV |= (M.Action == PC->FindActionByName(TEXT("IA_CameraToggle")) && M.Key == EKeys::V);
	}
	TestTrue(TEXT("V toggles the camera"), bV);
	return true;
}

// ============================================================================ pilot loss / retakes

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelHyperPilotLoss, "TriadDrift.Travel.Hyperdrive.PilotLossDeathAndReset", TD_TEST_FLAGS)
bool FTDTravelHyperPilotLoss::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector::ZeroVector);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	UTDHyperdriveStateRecorder* Rec = NewObject<UTDHyperdriveStateRecorder>();
	Rec->AddToRoot();
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	if (!TestNotNull(TEXT("event bus"), Bus)) { Rec->RemoveFromRoot(); return false; }
	Bus->OnHyperdriveState.AddDynamic(Rec, &UTDHyperdriveStateRecorder::HandleState);
	W.Advance(0.3f);

	// AI / wingmen can't start a jump.
	TestFalse(TEXT("AI-flown ship can't spool"), Hyper->StartSpool());
	TestEqual(TEXT("blocked: not player flown"), Hyper->GetBlockReason(), ETDHyperdriveBlock::NotPlayerFlown);

	// Switch away mid-spool: Idle broadcast while still player-flown.
	Ship->SetPlayerControlled(true);
	TestTrue(TEXT("spool"), Hyper->StartSpool());
	W.Advance(0.5f);
	Ship->SetPlayerControlled(false);
	TestEqual(TEXT("spool cancelled by the switch"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestTrue(TEXT("Idle broadcast on the bus"), Rec->LastIs(ETDHyperdriveState::Idle));
	TestFalse(TEXT("no override left on the old ship"), Ship->GetShipMovement()->HasSpeedOverride());

	// Switch away mid-jump: the jump ends, no 80k AI ship.
	Ship->SetPlayerControlled(true);
	W.Advance(1.2f); // past the cooldown
	TestTrue(TEXT("spool again"), Hyper->StartSpool());
	W.Advance(1.6f);
	TestEqual(TEXT("jumping"), Hyper->GetState(), ETDHyperdriveState::Jumping);
	Rec->States.Reset();
	Ship->SetPlayerControlled(false);
	TestEqual(TEXT("jump ended by the switch"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestTrue(TEXT("Idle broadcast for the jump end"), Rec->LastIs(ETDHyperdriveState::Idle));
	TestFalse(TEXT("override cleared"), Ship->GetShipMovement()->HasSpeedOverride());
	W.Advance(3.f);
	TestTrue(FString::Printf(TEXT("the ex-jumper slows down (%.0f)"), (float)Ship->GetVelocity().Size()), Ship->GetVelocity().Size() < 15000.f);

	// Death mid-jump ends it too.
	Ship->SetPlayerControlled(true);
	W.Advance(1.2f);
	TestTrue(TEXT("spool for the death case"), Hyper->StartSpool());
	W.Advance(1.6f);
	TestEqual(TEXT("jumping again"), Hyper->GetState(), ETDHyperdriveState::Jumping);
	Ship->GetHealth()->Kill();
	TestEqual(TEXT("death ends the jump"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestFalse(TEXT("no override after death"), Ship->GetShipMovement()->HasSpeedOverride());

	// Retake reset: instant Idle, no drop-out, ready to spool at once.
	Ship->GetHealth()->Revive();
	W.Advance(0.2f);
	Hyper->ResetHyperdrive();
	TestTrue(TEXT("spool after reset"), Hyper->StartSpool());
	W.Advance(1.6f);
	Hyper->ResetHyperdrive();
	TestEqual(TEXT("reset mid-jump = Idle immediately"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestEqual(TEXT("charge 0"), Hyper->GetCharge01(), 0.f);
	TestFalse(TEXT("no override after reset"), Ship->GetShipMovement()->HasSpeedOverride());
	TestTrue(TEXT("no cooldown after reset"), Hyper->CanSpool());
	Rec->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSwitchLock, "TriadDrift.Travel.Landing.SwitchLockAndAbort", TD_TEST_FLAGS)
bool FTDTravelSwitchLock::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDLandingPad* Pad = W.Spawn<ATDLandingPad>(FVector::ZeroVector);
	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector(-1500.f, 0.f, 1500.f));
	if (!TestNotNull(TEXT("ship"), Ship) || !Pad) { return false; }
	Ship->SetPlayerControlled(true);
	W.Tick();
	FText Reason;
	TestFalse(TEXT("free to switch while flying"), Ship->IsSwitchLocked(Reason));

	TestTrue(TEXT("spool"), Ship->GetHyperdrive()->StartSpool());
	TestTrue(TEXT("locked while the hyperdrive is busy"), Ship->IsSwitchLocked(Reason) && !Reason.IsEmpty());
	Ship->GetHyperdrive()->CancelSpool();
	TestFalse(TEXT("unlocked after cancel"), Ship->IsSwitchLocked(Reason));

	Ship->GetShipMovement()->SetForwardSpeedImmediate(2000.f);
	TestTrue(TEXT("land"), Ship->GetLanding()->RequestLand());
	TestTrue(TEXT("locked while landing"), Ship->IsSwitchLocked(Reason));
	W.Advance(5.2f);
	TestTrue(TEXT("landed"), Ship->GetLanding()->IsLanded());
	TestTrue(TEXT("locked while landed"), Ship->IsSwitchLocked(Reason));

	// Retake abort: instantly flying, pad free, flight live.
	Ship->GetLanding()->AbortToFlying();
	TestEqual(TEXT("flying"), Ship->GetLanding()->GetLandingState(), ETDLandingState::Flying);
	TestFalse(TEXT("gear up"), Ship->GetLanding()->IsGearDown());
	TestFalse(TEXT("canopy closed"), Ship->GetLanding()->IsCanopyOpen());
	TestFalse(TEXT("pad released"), Pad->IsOccupied());
	TestFalse(TEXT("flight un-parked"), Ship->IsFlightParked());
	TestFalse(TEXT("switch unlocked"), Ship->IsSwitchLocked(Reason));
	Ship->GetLanding()->AbortToFlying(); // safe twice
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelPlanetRig, "TriadDrift.Travel.Planet.RigSpawnsZonePadsAndLands", TD_TEST_FLAGS)
bool FTDTravelPlanetRig::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDPlanetTravelRig* Rig = ATDPlanetTravelRig::SpawnForPlanet(W.GetWorld(), TEXT("Cinder"), FTransform(FRotator(0.f, 90.f, 0.f), FVector(5000.f, 0.f, 200.f)), { TEXT("Obj_Outpost") });
	if (!TestNotNull(TEXT("rig"), Rig)) { return false; }
	W.Tick();
	TestNotNull(TEXT("zone"), Rig->GetZone());
	TestNotNull(TEXT("player pad"), Rig->GetPlayerPad());
	TestEqual(TEXT("two wingman pads"), Rig->GetWingmanPads().Num(), 2);
	if (!Rig->GetZone() || !Rig->GetPlayerPad()) { return false; }
	TestEqual(TEXT("zone knows its planet"), Rig->GetZone()->PlanetId, FName(TEXT("Cinder")));
	TestFalse(TEXT("zone exit gated by the planet's objectives"), Rig->GetZone()->IsExitAllowed());
	TestFalse(TEXT("player pad isn't a wingman pad"), Rig->GetPlayerPad()->IsWingmanPad());
	TestEqual(TEXT("side pads linked"), Rig->GetPlayerPad()->GetSidePads().Num(), 2);
	TestTrue(TEXT("pads inside the zone"), Rig->GetZone()->GetZoneBox().IsInside(Rig->GetPlayerPad()->GetActorLocation()));

	const FTransform Descent = Rig->GetDescentStart();
	TestTrue(TEXT("descent start is high above the pad"), Descent.GetLocation().Z > Rig->GetPlayerPad()->GetActorLocation().Z + 20000.f);
	TestTrue(TEXT("descent start inside the zone"), Rig->GetZone()->GetZoneBox().IsInside(Descent.GetLocation()));
	const FVector ToPad = (Rig->GetPlayerPad()->GetActorLocation() - Descent.GetLocation()).GetSafeNormal();
	TestTrue(TEXT("descent start faces the pad"), FVector::DotProduct(Descent.GetRotation().GetForwardVector(), ToPad) > 0.95f);

	// A squad arriving at the descent start lands player + wingman on the rig's pads.
	ATDShip* Lead = TDTravelTest::SpawnShip(W, Rig->GetPlayerPad()->GetActorTransform().TransformPosition(FVector(-1500.f, 0.f, 1500.f)), FRotator(0.f, 90.f, 0.f));
	ATDShip* Mate = TDTravelTest::SpawnShip(W, Rig->GetPlayerPad()->GetActorTransform().TransformPosition(FVector(-3000.f, 2000.f, 2500.f)), FRotator(0.f, 90.f, 0.f));
	Lead->SetPlayerControlled(true);
	W.Tick();
	TestTrue(TEXT("in atmosphere inside the rig's zone"), Lead->GetShipMovement()->IsInAtmosphere());
	Lead->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
	TestTrue(TEXT("lands on the rig pad"), Lead->GetLanding()->RequestLand());
	W.Advance(5.5f);
	TestTrue(TEXT("player landed on the player pad"), Lead->GetLanding()->IsLanded() && Lead->GetLanding()->GetPad() == Rig->GetPlayerPad());
	TestTrue(TEXT("wingman landed on a wingman pad"), Mate->GetLanding()->IsLanded() && Mate->GetLanding()->GetPad() && Mate->GetLanding()->GetPad()->IsWingmanPad());
	const float Alt = Lead->GetAltitudeMeters();
	TestTrue(FString::Printf(TEXT("placeholder ground under the pad (altitude %.1f m)"), Alt), Alt >= 0.f && Alt < 5.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelCameraNow, "TriadDrift.Travel.Camera.ChaseTransformNowForAIShip", TD_TEST_FLAGS)
bool FTDTravelCameraNow::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDTravelTest::SpawnShip(W, FVector(1000.f, 2000.f, 3000.f), FRotator(0.f, 90.f, 0.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Tick();
	const FTransform Cam = Ship->GetChaseCameraTransformNow();
	const FVector Expected = Ship->GetActorLocation() - Ship->GetActorForwardVector() * 1400.f + FVector(0.f, 0.f, 280.f);
	TestTrue(FString::Printf(TEXT("behind and above an AI-flown ship (%s vs %s)"), *Cam.GetLocation().ToString(), *Expected.ToString()), Cam.GetLocation().Equals(Expected, 5.f));
	TestTrue(TEXT("looking along the ship's nose"), FVector::DotProduct(Cam.GetRotation().GetForwardVector(), Ship->GetActorForwardVector()) > 0.98f);
	TestEqual(TEXT("FOV for its speed"), Ship->GetChaseCameraFOVNow(), FMath::Lerp(90.f, 110.f, Ship->GetSpeed01()), 0.01f);

	// For the player-flown ship it's the live camera.
	Ship->SetPlayerControlled(true);
	W.Tick();
	TestTrue(TEXT("player ship: live camera transform"), Ship->GetChaseCameraTransformNow().Equals(Ship->GetChaseCamera()->GetComponentTransform(), 0.01f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
