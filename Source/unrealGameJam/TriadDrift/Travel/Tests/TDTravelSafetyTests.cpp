// Safety tests for travel: speed-override ownership, hyperdrive never ending inside geometry, wells /
// boundary / locks at several frame rates, entry and level-travel edge cases.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Safety

#include "Misc/AutomationTest.h"
#include "GameFramework/WorldSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Travel/Tests/TDTravelTestHelpers.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"

namespace TDTravelSafetyTest
{
	ATDShip* SpawnPlayerShip(FTDTestWorld& W, const FVector& At = FVector::ZeroVector)
	{
		ATDShip* Ship = W.Spawn<ATDShip>(At);
		if (Ship)
		{
			Ship->SetPlayerControlled(true);
			FTDFlightInput In;
			In.Throttle = 1.f;
			Ship->SetFlightInput(In);
		}
		return Ship;
	}

	AActor* SpawnBlocker(FTDTestWorld& W, const FVector& At, UPrimitiveComponent* (*Make)(AActor*))
	{
		AActor* A = W.Spawn<AActor>(At);
		UPrimitiveComponent* Prim = Make(A);
		Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Prim->SetCollisionObjectType(ECC_WorldStatic);
		Prim->SetCollisionResponseToAllChannels(ECR_Block);
		A->SetRootComponent(Prim);
		Prim->RegisterComponent();
		A->SetActorLocation(At);
		return A;
	}

	UPrimitiveComponent* MakeWall(AActor* Owner)
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(Owner, TEXT("Wall"));
		Box->SetBoxExtent(FVector(500.f, 60000.f, 60000.f));
		return Box;
	}

	UPrimitiveComponent* MakePlanet(AActor* Owner)
	{
		USphereComponent* Sphere = NewObject<USphereComponent>(Owner, TEXT("Planet"));
		Sphere->InitSphereRadius(60000.f);
		return Sphere;
	}

	/** Spools and jumps, then ticks at Fps until the jump ends (or Seconds pass). Calls OnFrame every frame. */
	template <typename TFrame>
	bool JumpUntilEnd(FTDTestWorld& W, ATDShip* Ship, float Fps, float Seconds, TFrame OnFrame)
	{
		UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
		if (!Hyper->StartSpool())
		{
			return false;
		}
		const float Dt = 1.f / Fps;
		bool bJumped = false;
		for (float T = 0.f; T < Seconds; T += Dt)
		{
			W.Tick(Dt);
			OnFrame();
			bJumped |= Hyper->GetState() == ETDHyperdriveState::Jumping;
			if (bJumped && Hyper->GetState() != ETDHyperdriveState::Jumping)
			{
				return true;
			}
		}
		return false;
	}

	const float FrameRates[] = { 30.f, 60.f, 120.f };
}

// ================================================================================ override ownership

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSafetyOverrides, "TriadDrift.Travel.Safety.SpeedOverrideOwnershipPairs", TD_TEST_FLAGS)
bool FTDTravelSafetyOverrides::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	const FName Hyper = UTDHyperdriveComponent::OverrideOwner;
	const FName Entry = UTDAtmosphereEntryComponent::OverrideOwner;
	const FName Ram(TEXT("Ram"));

	// Hyperdrive under a ram: the ram wins while active, the jump is restored when the ram ends.
	Move->SetSpeedOverrideFor(Hyper, 80000.f, 0.1f);
	Move->SetSpeedOverrideFor(Ram, 33000.f, 0.4f);
	TestEqual(TEXT("most recent request flown (ram)"), Move->GetActiveSpeedOverrideOwner(), Ram);
	Move->ClearSpeedOverrideFor(Ram);
	TestEqual(TEXT("ram ends: hyperdrive restored, not cleared"), Move->GetActiveSpeedOverrideOwner(), Hyper);
	TestTrue(TEXT("still overridden"), Move->HasSpeedOverride());
	W.Advance(1.5f);
	TestEqual(TEXT("flying the restored 80000"), (float)Ship->GetVelocity().Size(), 80000.f, 800.f);
	Move->ClearSpeedOverrideFor(Hyper);
	TestFalse(TEXT("all clear"), Move->HasSpeedOverride());

	// Entry under hyperdrive: the jump ending doesn't drop the committed entry.
	Move->SetSpeedOverrideFor(Entry, 9000.f, 0.35f);
	Move->SetSpeedOverrideFor(Hyper, 80000.f, 0.1f);
	Move->ClearSpeedOverrideFor(Hyper);
	TestEqual(TEXT("entry kept when the jump ends"), Move->GetActiveSpeedOverrideOwner(), Entry);

	// Releasing the lower request keeps the upper one.
	Move->SetSpeedOverrideFor(Ram, 33000.f, 0.4f);
	Move->ClearSpeedOverrideFor(Entry);
	TestEqual(TEXT("clearing the older owner leaves the newer"), Move->GetActiveSpeedOverrideOwner(), Ram);
	Move->ClearSpeedOverrideFor(Ram);

	// Legacy unkeyed callers (T4's enemy AI) only clear their own request.
	Move->SetSpeedOverrideFor(Hyper, 80000.f, 0.1f);
	Move->SetSpeedOverride(5000.f, 1.f);
	TestEqual(TEXT("unkeyed request on top"), Move->GetActiveSpeedOverrideOwner(), FName(NAME_None));
	Move->ClearSpeedOverride();
	TestTrue(TEXT("unkeyed clear leaves the keyed jump"), Move->HasSpeedOverrideFor(Hyper));
	TestEqual(TEXT("re-setting an owner moves it to the top"), (Move->SetSpeedOverrideFor(Ram, 1.f), Move->SetSpeedOverrideFor(Hyper, 2.f), Move->GetActiveSpeedOverrideOwner()), Hyper);

	// Reset drops every owner.
	Ship->ResetFlightState(false);
	TestFalse(TEXT("reset clears every owner"), Move->HasSpeedOverride());

	// Real components: a ram during a real jump, then the jump's own drop-out only clears its request.
	W.Advance(1.2f);
	UTDHyperdriveComponent* Drive = Ship->GetHyperdrive();
	TestTrue(TEXT("spool"), Drive->StartSpool());
	W.Advance(2.f); // past the 0.5 s minimum jump time
	TestEqual(TEXT("jumping"), Drive->GetState(), ETDHyperdriveState::Jumping);
	Ship->SetSpeedOverrideFor(Ram, 33000.f, 0.4f);
	TestTrue(TEXT("manual drop-out"), Drive->RequestDropOut());
	TestEqual(TEXT("the drop-out burst sits on top of the ram"), Move->GetActiveSpeedOverrideOwner(), Hyper);
	W.Advance(1.f);
	TestEqual(TEXT("hyperdrive done"), Drive->GetState(), ETDHyperdriveState::Idle);
	TestEqual(TEXT("the ram is still there afterwards"), Move->GetActiveSpeedOverrideOwner(), Ram);
	Ship->ClearSpeedOverrideFor(Ram);
	return true;
}

// ======================================================================== hyperdrive vs geometry

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSafetyObstacles, "TriadDrift.Travel.Safety.HyperdriveNeverEndsInsideGeometry", TD_TEST_FLAGS)
bool FTDTravelSafetyObstacles::RunTest(const FString& Parameters)
{
	struct FCase { const TCHAR* Name; float Distance; int32 Kind; }; // 0 wall, 1 planet body, 2 ship
	const FCase Cases[] = { { TEXT("asteroid wall"), 250000.f, 0 }, { TEXT("planet body (no well)"), 400000.f, 1 }, { TEXT("another ship"), 300000.f, 2 } };

	for (const FCase& Case : Cases)
	{
		for (const float Fps : TDTravelSafetyTest::FrameRates)
		{
			FTDTestWorld W;
			ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
			AActor* Blocker = nullptr;
			float FaceX = 0.f; // nearest x the ship's centre may reach before touching
			const float ShipRadius = Ship->GetCollisionSphere()->GetScaledSphereRadius();
			if (Case.Kind == 0)
			{
				Blocker = TDTravelSafetyTest::SpawnBlocker(W, FVector(Case.Distance, 0.f, 0.f), &TDTravelSafetyTest::MakeWall);
				FaceX = Case.Distance - 500.f - ShipRadius;
			}
			else if (Case.Kind == 1)
			{
				Blocker = TDTravelSafetyTest::SpawnBlocker(W, FVector(Case.Distance, 0.f, 0.f), &TDTravelSafetyTest::MakePlanet);
				FaceX = Case.Distance - 60000.f - ShipRadius;
			}
			else
			{
				ATDSpacecraft* Other = W.Spawn<ATDSpacecraft>(FVector(Case.Distance, 0.f, 0.f), FRotator(0.f, 180.f, 0.f));
				Other->SetTeam(ETDTeam::Enemy);
				Other->SetFlightParked(true);
				Blocker = Other;
				FaceX = Case.Distance - 2.f * ShipRadius;
			}
			W.Tick();
			const float HP0 = Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield();
			float MaxX = -1.f;
			const bool bEnded = TDTravelSafetyTest::JumpUntilEnd(W, Ship, Fps, 15.f, [&]() { MaxX = FMath::Max(MaxX, (float)Ship->GetActorLocation().X); });
			// Let the drop-out finish too.
			for (int32 i = 0; i < FMath::RoundToInt(Fps); ++i) { W.Tick(1.f / Fps); MaxX = FMath::Max(MaxX, (float)Ship->GetActorLocation().X); }

			const FString Tag = FString::Printf(TEXT("%s @ %.0f fps"), Case.Name, Fps);
			TestTrue(*FString::Printf(TEXT("%s: jump ended"), *Tag), bEnded);
			TestEqual(*FString::Printf(TEXT("%s: ended by the look-ahead"), *Tag), Ship->GetHyperdrive()->GetLastEndCause(), ETDHyperdriveEndCause::Obstacle);
			TestTrue(*FString::Printf(TEXT("%s: never reached it (max x %.0f, face %.0f)"), *Tag, MaxX, FaceX), MaxX < FaceX);
			TestEqual(*FString::Printf(TEXT("%s: no collision damage"), *Tag), Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield(), HP0, 0.01f);
			TestTrue(*FString::Printf(TEXT("%s: back to cruise or below (%.0f)"), *Tag, (float)Ship->GetVelocity().Size()), Ship->GetVelocity().Size() <= 6300.f);
		}
	}

	// A spool with something solid right ahead is refused outright.
	FTDTestWorld W;
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
	TDTravelSafetyTest::SpawnBlocker(W, FVector(12000.f, 0.f, 0.f), &TDTravelSafetyTest::MakeWall);
	W.Tick();
	TestEqual(TEXT("obstacle within 200 m blocks the spool"), Ship->GetHyperdrive()->GetBlockReason(), ETDHyperdriveBlock::ObstacleAhead);
	TestFalse(TEXT("spool refused"), Ship->GetHyperdrive()->StartSpool());
	Ship->TeleportSpacecraft(FVector::ZeroVector, FRotator(0.f, 180.f, 0.f), false);
	W.Tick();
	TestTrue(TEXT("facing away it may spool"), Ship->GetHyperdrive()->CanSpool());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSafetyWellsBounds, "TriadDrift.Travel.Safety.HyperdriveWellsBoundaryAndLocksAtFrameRates", TD_TEST_FLAGS)
bool FTDTravelSafetyWellsBounds::RunTest(const FString& Parameters)
{
	for (const float Fps : TDTravelSafetyTest::FrameRates)
	{
		// Gravity well.
		{
			FTDTestWorld W;
			UTDTravelSubsystem::Get(W.GetWorld())->RegisterGravityWell(TEXT("Well"), FVector(500000.f, 0.f, 0.f), 15000.f);
			ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
			W.Tick();
			const bool bEnded = TDTravelSafetyTest::JumpUntilEnd(W, Ship, Fps, 15.f, [] {});
			const float Dist = FVector::Dist(Ship->GetActorLocation(), FVector(500000.f, 0.f, 0.f));
			TestTrue(*FString::Printf(TEXT("well @ %.0f fps: ended"), Fps), bEnded);
			TestEqual(*FString::Printf(TEXT("well @ %.0f fps: cause"), Fps), Ship->GetHyperdrive()->GetLastEndCause(), ETDHyperdriveEndCause::GravityWell);
			TestTrue(*FString::Printf(TEXT("well @ %.0f fps: stopped at the edge (%.0f)"), Fps, Dist), Dist <= 15000.f + 3000.f);
		}
		// Play boundary.
		{
			FTDTestWorld W;
			UTDTravelSubsystem::Get(W.GetWorld())->SetPlayBounds(FVector::ZeroVector, 400000.f);
			ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
			W.Tick();
			const bool bEnded = TDTravelSafetyTest::JumpUntilEnd(W, Ship, Fps, 15.f, [] {});
			TestTrue(*FString::Printf(TEXT("boundary @ %.0f fps: ended"), Fps), bEnded);
			TestEqual(*FString::Printf(TEXT("boundary @ %.0f fps: cause"), Fps), Ship->GetHyperdrive()->GetLastEndCause(), ETDHyperdriveEndCause::Boundary);
			TestTrue(*FString::Printf(TEXT("boundary @ %.0f fps: inside the play space (%.0f)"), Fps, (float)Ship->GetActorLocation().Size()), Ship->GetActorLocation().Size() < 400000.f);
		}
		// Timing: the spool takes 1.2 s at every frame rate.
		{
			FTDTestWorld W;
			ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
			W.Tick();
			UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
			Hyper->StartSpool();
			float T = 0.f;
			const float Dt = 1.f / Fps;
			while (Hyper->GetState() == ETDHyperdriveState::Spooling && T < 3.f) { W.Tick(Dt); T += Dt; }
			TestEqual(*FString::Printf(TEXT("spool 1.2 s @ %.0f fps"), Fps), T, 1.2f, Dt + 0.001f);
		}
	}

	// Locks (an active wave / the finale): no spool, and a jump in progress drops out.
	FTDTestWorld W;
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
	W.Tick();
	Travel->AddHyperdriveLock(TEXT("Wave"), FText::FromString(TEXT("Clear the wave first")));
	TestEqual(TEXT("locked"), Ship->GetHyperdrive()->GetBlockReason(), ETDHyperdriveBlock::Locked);
	TestFalse(TEXT("no spool while locked"), Ship->GetHyperdrive()->StartSpool());
	Travel->RemoveHyperdriveLock(TEXT("Wave"));
	TestTrue(TEXT("spool once unlocked"), Ship->GetHyperdrive()->StartSpool());
	W.Advance(1.6f);
	TestEqual(TEXT("jumping"), Ship->GetHyperdrive()->GetState(), ETDHyperdriveState::Jumping);
	Travel->AddHyperdriveLock(TEXT("Finale"), FText::FromString(TEXT("The Dominion is here")));
	W.Tick();
	TestEqual(TEXT("a lock mid-jump drops out"), Ship->GetHyperdrive()->GetLastEndCause(), ETDHyperdriveEndCause::Locked);
	FText Reason;
	TestTrue(TEXT("lock reason exposed"), Travel->IsHyperdriveLocked(Reason) && !Reason.IsEmpty());
	return true;
}

// ============================================================================ entry / level travel

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSafetyEntry, "TriadDrift.Travel.Safety.EntryWingmanDeathRetakeAndOnce", TD_TEST_FLAGS)
bool FTDTravelSafetyEntry::RunTest(const FString& Parameters)
{
	// Wingman: never takes the squad down.
	{
		FTDTestWorld W;
		ATDShip* Wingman = W.Spawn<ATDShip>(FVector::ZeroVector);
		W.Tick();
		TestFalse(TEXT("a wingman can't start an entry"), Wingman->GetAtmosphereEntry()->BeginEntry(ETDDestination::Verdance, TEXT("Descent"), FVector(200000.f, 0.f, 0.f), 60000.f));
	}
	// Death during entry: clean state, no travel.
	{
		FTDTestWorld W;
		ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
		W.Tick();
		UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
		TestTrue(TEXT("entry begins"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Descent"), FVector(200000.f, 0.f, 0.f), 60000.f));
		TestTrue(TEXT("invulnerable while committed"), Ship->GetHealth()->IsInvulnerable());
		W.Advance(0.5f);
		Ship->GetHealth()->Kill();
		TestFalse(TEXT("death aborts the entry"), Entry->IsEntering());
		TestFalse(TEXT("entry override released"), Ship->GetShipMovement()->HasSpeedOverrideFor(UTDAtmosphereEntryComponent::OverrideOwner));
		TestFalse(TEXT("invulnerability released"), Ship->GetHealth()->IsInvulnerable());
		TestFalse(TEXT("no travel requested"), UTDTravelSubsystem::Get(W.GetWorld())->HasRequestedLevelTravel());
		TestFalse(TEXT("a dead ship can't start an entry"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Descent"), FVector(200000.f, 0.f, 0.f), 60000.f));
	}
	// Twice: no second entry while one runs; the dive is speed-limited above the surface.
	{
		FTDTestWorld W;
		ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
		W.Tick();
		UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
		const FVector Planet(70000.f, 0.f, 0.f); // surface 10000 cm ahead
		TestTrue(TEXT("first entry"), Entry->BeginEntry(ETDDestination::Cinder, TEXT("Descent"), Planet, 60000.f));
		TestFalse(TEXT("second entry refused"), Entry->BeginEntry(ETDDestination::Verdance, TEXT("Descent"), Planet, 60000.f));
		float MinToSurface = TNumericLimits<float>::Max();
		for (int32 i = 0; i < 150; ++i)
		{
			W.Tick();
			MinToSurface = FMath::Min(MinToSurface, (float)FVector::Dist(Ship->GetActorLocation(), Planet) - 60000.f);
		}
		TestTrue(FString::Printf(TEXT("dive never reaches the surface (closest %.0f cm)"), MinToSurface), MinToSurface > 1000.f);
	}
	// A retake pending: no entry, no level travel.
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetWorld()->GetGameInstance());
		if (TestNotNull(TEXT("flow game instance"), GI))
		{
			ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
			W.Tick();
			GI->NotifyLevelEntered(ETDDestination::Galaxy);
			GI->BeginScene(TEXT("Wave1"));
			GI->ReportPlayerDefeated();
			TestTrue(TEXT("retake pending"), GI->IsRetakePending());
			UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
			TestFalse(TEXT("no level travel over a pending retake"), Travel->CanRequestLevelTravel());
			TestFalse(TEXT("no entry over a pending retake"), Ship->GetAtmosphereEntry()->BeginEntry(ETDDestination::Verdance, TEXT("Descent"), FVector(200000.f, 0.f, 0.f), 60000.f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSafetyFrameRates, "TriadDrift.Travel.Safety.LandingAndEntryTimingAtFrameRates", TD_TEST_FLAGS)
bool FTDTravelSafetyFrameRates::RunTest(const FString& Parameters)
{
	for (const float Fps : TDTravelSafetyTest::FrameRates)
	{
		const float Dt = 1.f / Fps;
		// Landing: 4.9 s from request to Landed; take-off: 3.5 s.
		{
			FTDTestWorld W;
			ATDLandingPad* Pad = W.Spawn<ATDLandingPad>(FVector::ZeroVector);
			ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W, FVector(-1500.f, 0.f, 1500.f));
			W.Tick(Dt);
			Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
			UTDLandingComponent* Land = Ship->GetLanding();
			TestTrue(TEXT("land"), Pad && Land->RequestLand());
			float T = 0.f;
			while (!Land->IsLanded() && T < 10.f) { W.Tick(Dt); T += Dt; }
			TestEqual(*FString::Printf(TEXT("landing 4.9 s @ %.0f fps"), Fps), T, 4.9f, 6.f * Dt + 0.01f);
			Land->RequestTakeOff();
			T = 0.f;
			while (Land->IsBusy() && T < 10.f) { W.Tick(Dt); T += Dt; }
			TestEqual(*FString::Printf(TEXT("take-off 3.5 s @ %.0f fps"), Fps), T, 3.5f, 5.f * Dt + 0.01f);
		}
		// Entry hands back after 2.5 s (no flow in a plain test world).
		{
			FTDTestWorld W;
			ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
			W.Tick(Dt);
			UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
			Entry->BeginEntry(ETDDestination::Verdance, TEXT("Descent"), FVector(500000.f, 0.f, 0.f), 60000.f);
			float T = 0.f;
			while (Entry->IsEntering() && T < 5.f) { W.Tick(Dt); T += Dt; }
			TestEqual(*FString::Printf(TEXT("entry 2.5 s @ %.0f fps"), Fps), T, 2.5f, Dt + 0.01f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSafetyHoldF, "TriadDrift.Travel.Safety.HoldFLandsAndTakesOffThroughTheController", TD_TEST_FLAGS)
bool FTDTravelSafetyHoldF::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDLandingPad* Pad = W.Spawn<ATDLandingPad>(FVector::ZeroVector);
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(-1500.f, 0.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("pad"), Pad) || !TestNotNull(TEXT("local space controller"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	PC->SetFlightContext(ETDFlightContext::Atmosphere); // F / A = Land
	W.Tick();
	W.Tick();
	FTDTestWorld::InjectKey(PC, EKeys::SpaceBar, true); // the pilot brakes to land, as a player would
	W.Tick();
	TestTrue(TEXT("Space brake reaches the ship"), PC->GetCurrentFlightInput().bBrake);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
	UTDLandingComponent* Land = Ship->GetLanding();

	auto HoldF = [&W, PC](float Seconds)
	{
		FTDTestWorld::InjectKey(PC, EKeys::F, true);
		W.Advance(Seconds);
		FTDTestWorld::InjectKey(PC, EKeys::F, false);
		W.Tick();
	};

	// A tap does nothing (hold to land).
	HoldF(0.1f);
	TestFalse(TEXT("a tap of F doesn't land"), Land->IsBusy());

	Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
	FTDTestWorld::InjectKey(PC, EKeys::F, true);
	W.Advance(0.6f);
	FTDTestWorld::InjectKey(PC, EKeys::F, false);
	W.Tick();
	TestTrue(TEXT("hold F lands (real key through Enhanced Input)"), Land->IsBusy());
	W.Advance(5.5f);
	TestTrue(TEXT("landed"), Land->IsLanded());

	// No on-foot hand-off took the pilot out: hold F again takes off (no soft-lock in any build).
	HoldF(0.7f);
	TestTrue(TEXT("hold F in a landed ship takes off"), Land->IsBusy() && !Land->IsLanded());
	W.Advance(4.f);
	TestEqual(TEXT("flying again"), Land->GetLandingState(), ETDLandingState::Flying);
	TestFalse(TEXT("pad released"), Pad->IsOccupied());
	FTDTestWorld::InjectKey(PC, EKeys::SpaceBar, false);

	// Too fast near a pad: holding F engages the approach assist (Lead decision, Nov 12), which flies in and lands.
	Ship->TeleportSpacecraft(FVector(-1500.f, 0.f, 1500.f), FRotator::ZeroRotator, false);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(5000.f); // too fast for a direct landing
	TestFalse(TEXT("too fast: no direct landing"), Land->CanLandNow());
	FTDTestWorld::InjectKey(PC, EKeys::F, true);
	W.Advance(1.5f);
	FTDTestWorld::InjectKey(PC, EKeys::F, false);
	TestTrue(TEXT("too fast but holding F: the assist brings it in to land"), Land->IsBusy());

	// Out of the assist range, holding F keeps retrying while held: it lands once the ship is in range and slow.
	W.Advance(6.f);
	HoldF(0.7f); // take off again
	W.Advance(4.f);
	Ship->TeleportSpacecraft(FVector(-60000.f, 0.f, 1500.f), FRotator::ZeroRotator, false);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
	FTDTestWorld::InjectKey(PC, EKeys::F, true);
	W.Advance(0.6f);
	TestFalse(TEXT("600 m out: nothing to land on yet"), Land->IsBusy() || Land->IsApproachAssisting());
	Ship->TeleportSpacecraft(FVector(-1500.f, 0.f, 1500.f), FRotator::ZeroRotator, false);
	Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
	FTDTestWorld::InjectKey(PC, EKeys::SpaceBar, true);
	W.Advance(0.6f);
	FTDTestWorld::InjectKey(PC, EKeys::F, false);
	FTDTestWorld::InjectKey(PC, EKeys::SpaceBar, false);
	TestTrue(TEXT("still holding F: lands once it's possible (no release needed)"), Land->IsBusy());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelStaleLandHold, "TriadDrift.Travel.Safety.HeldFNeverCarriesThroughAHandOver", TD_TEST_FLAGS)
bool FTDTravelStaleLandHold::RunTest(const FString& Parameters)
{
	// Review minor 7: F pressed again during the landing sequence and held through the on-foot hand-over must not
	// take off at touchdown, nor start the approach assist after re-boarding and taking off.
	FTDTestWorld W;
	ATDLandingPad* Pad = W.Spawn<ATDLandingPad>(FVector::ZeroVector);
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(-1500.f, 0.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("local space controller"), PC) || !Pad) { return false; }
	Ship->SetPlayerControlled(true);
	PC->SetFlightContext(ETDFlightContext::Atmosphere); // F / A = Land
	W.Tick();
	W.Tick();
	UTDLandingComponent* Land = Ship->GetLanding();
	Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);

	FTDTestWorld::InjectKey(PC, EKeys::F, true);
	W.Advance(0.6f);
	FTDTestWorld::InjectKey(PC, EKeys::F, false);
	W.Tick();
	TestTrue(TEXT("landing"), Land->IsBusy());
	W.Advance(0.5f);
	FTDTestWorld::InjectKey(PC, EKeys::F, true); // pressed again mid-sequence and held
	W.Advance(6.f);
	TestTrue(TEXT("held F through touchdown: still landed (no instant take-off)"), Land->IsLanded());

	// On-foot hand-over while F is still held: the controller leaves the ship, then re-boards it.
	APawn* Pilot = W.Spawn<APawn>(FVector(0.f, 500.f, 200.f));
	Ship->SetPlayerControlled(false);
	PC->Possess(Pilot);
	W.Advance(0.5f);
	PC->Possess(Ship);
	Ship->SetPlayerControlled(true);
	PC->SetFlightContext(ETDFlightContext::Atmosphere);
	TestTrue(TEXT("re-board: take off"), Land->RequestTakeOff());
	W.Advance(5.f);
	TestEqual(TEXT("flying after take-off"), Land->GetLandingState(), ETDLandingState::Flying);
	W.Advance(2.f);
	TestFalse(TEXT("the stale hold doesn't start the assist"), Land->IsApproachAssisting());
	TestFalse(TEXT("the stale hold doesn't land the ship again"), Land->IsBusy());
	FTDTestWorld::InjectKey(PC, EKeys::F, false);
	W.Tick();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelVoidedTravel, "TriadDrift.Travel.Safety.VoidedOrTimedOutTravelHandsBackAndRetries", TD_TEST_FLAGS)
bool FTDTravelVoidedTravel::RunTest(const FString& Parameters)
{
	// Review minor 6: a load voided by another level, or a travel that timed out, must hand the entering ship back
	// (no override, no invulnerability, hyperdrive on) and allow a second attempt.
	UTDFlowSettings* Settings = GetMutableDefault<UTDFlowSettings>();
	const TMap<ETDDestination, FSoftObjectPath> SavedMaps = Settings->DestinationMaps;
	const FSoftObjectPath SavedTransition = Settings->TransitionMap;
	const float SavedTimeout = Settings->TravelTimeoutSeconds;
	ON_SCOPE_EXIT
	{
		Settings->DestinationMaps = SavedMaps;
		Settings->TransitionMap = SavedTransition;
		Settings->TravelTimeoutSeconds = SavedTimeout;
	};
	Settings->DestinationMaps.Add(ETDDestination::Verdance, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")));
	Settings->TransitionMap = FSoftObjectPath();
	Settings->TravelTimeoutSeconds = 2.f;

	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetWorld()->GetGameInstance());
	if (!TestNotNull(TEXT("flow"), GI)) { return false; }
	GI->OpenLevelOverride = [](const FString&) {}; // never actually leave the test world
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
	W.Tick();
	UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	const FVector Planet(400000.f, 0.f, 0.f);

	auto Enter = [&]()
	{
		const bool bBegan = Entry->BeginEntry(ETDDestination::Verdance, TEXT("Descent"), Planet, 60000.f);
		W.Advance(Entry->EntrySeconds + 0.1f);
		return bBegan && Entry->GetEntryState() == ETDAtmosphereEntryState::Travelling;
	};
	auto HandedBack = [&](const TCHAR* Why)
	{
		W.Tick();
		TestEqual(*FString::Printf(TEXT("%s: entry idle"), Why), Entry->GetEntryState(), ETDAtmosphereEntryState::Idle);
		TestFalse(*FString::Printf(TEXT("%s: entry override released"), Why), Move->HasSpeedOverrideFor(UTDAtmosphereEntryComponent::OverrideOwner));
		TestFalse(*FString::Printf(TEXT("%s: not invulnerable"), Why), Ship->GetHealth()->IsInvulnerable());
		TestTrue(*FString::Printf(TEXT("%s: a new travel may be requested"), Why), Travel->CanRequestLevelTravel());
	};

	TestTrue(TEXT("first entry travels"), Enter());
	TestTrue(TEXT("travel under way"), Travel->HasRequestedLevelTravel() && GI->IsTravelPending());
	TestFalse(TEXT("while under way: no second request"), Travel->CanRequestLevelTravel());
	GI->NotifyMapLoaded(TEXT("/Game/Somewhere/Else")); // another system loaded a level: our travel is void
	HandedBack(TEXT("voided"));

	TestTrue(TEXT("second attempt travels"), Enter());
	GI->TickFlow(3.f); // past TravelTimeoutSeconds
	HandedBack(TEXT("timed out"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSwitchLatches, "TriadDrift.Travel.Safety.SwitchLeavesNoLatchedInputOrHeldHyperdrive", TD_TEST_FLAGS)
bool FTDTravelSwitchLatches::RunTest(const FString& Parameters)
{
	// Squad review questions: (1) the ship the player leaves must not keep flying the player's last input until its
	// AI writes one; (2) a hyperdrive key held through a switch must not spool the new ship.
	FTDTestWorld W;
	ATDShip* A = W.Spawn<ATDShip>(FVector(0.f, 0.f, 1500.f));
	ATDShip* B = W.Spawn<ATDShip>(FVector(0.f, 8000.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(A, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("local space controller"), PC)) { return false; }
	A->SetPlayerControlled(true);
	W.Tick();
	W.Tick();

	// (1) Boost + roll + drift held on A, then switch to B with no AI on A.
	FTDTestWorld::InjectKey(PC, EKeys::LeftShift, true);
	FTDTestWorld::InjectKey(PC, EKeys::D, true);
	W.Advance(0.3f);
	TestTrue(TEXT("A boosting and rolling under the player"), A->GetFlightInput().bBoost && A->GetFlightInput().Roll > 0.9f);
	A->SetPlayerControlled(false);
	PC->Possess(B);
	B->SetPlayerControlled(true);
	W.Tick();
	TestFalse(TEXT("left ship: no latched boost"), A->GetFlightInput().bBoost);
	TestEqual(TEXT("left ship: no latched roll"), A->GetFlightInput().Roll, 0.f, 0.01f);
	TestTrue(TEXT("left ship: no latched turn"), A->GetFlightInput().Aim.IsNearlyZero());
	TestEqual(TEXT("left ship: cruise throttle"), A->GetFlightInput().Throttle, 1.f, 0.01f);
	W.Advance(0.5f);
	TestFalse(TEXT("left ship stops boosting"), A->IsBoosting());
	TestTrue(TEXT("held keys carry to the new ship (by design)"), B->GetFlightInput().bBoost);
	FTDTestWorld::InjectKey(PC, EKeys::LeftShift, false);
	FTDTestWorld::InjectKey(PC, EKeys::D, false);
	W.Advance(3.f); // boost cooldown, hyperdrive cooldowns

	// (2) H held on B (spooling), switch back to A while still holding H.
	FTDTestWorld::InjectKey(PC, EKeys::H, true);
	W.Advance(0.3f);
	TestEqual(TEXT("B spooling under the held key"), B->GetHyperdrive()->GetState(), ETDHyperdriveState::Spooling);
	B->SetPlayerControlled(false);
	PC->Possess(A);
	A->SetPlayerControlled(true);
	W.Advance(2.5f); // longer than a full spool
	TestEqual(TEXT("left ship's spool ended"), B->GetHyperdrive()->GetState(), ETDHyperdriveState::Idle);
	TestEqual(TEXT("the held key doesn't spool the new ship"), A->GetHyperdrive()->GetState(), ETDHyperdriveState::Idle);
	FTDTestWorld::InjectKey(PC, EKeys::H, false);
	W.Tick();
	TestEqual(TEXT("release: still idle"), A->GetHyperdrive()->GetState(), ETDHyperdriveState::Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelJumpSlowMo, "TriadDrift.Travel.Safety.JumpUnderSlowMotion", TD_TEST_FLAGS)
bool FTDTravelJumpSlowMo::RunTest(const FString& Parameters)
{
	// Review minor 9: a jump while the director's slow motion runs spools, jumps and drops out on game time.
	FTDTestWorld W;
	W.GetWorld()->GetWorldSettings()->SetTimeDilation(0.25f);
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
	W.Tick();
	UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	TestTrue(TEXT("spool starts"), Hyper->RequestSpoolFromPilot());
	const float Spool = Hyper->Tuning.SpoolSeconds;
	W.Advance(Spool * 0.5f / 0.25f);
	TestEqual(TEXT("half the spool in game time: still spooling"), Hyper->GetState(), ETDHyperdriveState::Spooling);
	W.Advance(Spool * 0.7f / 0.25f);
	TestEqual(TEXT("spool finished on game time: jumping"), Hyper->GetState(), ETDHyperdriveState::Jumping);
	const FVector Before = Ship->GetActorLocation();
	W.Advance(0.6f / 0.25f); // past MinJumpSeconds in game time
	const float Moved = FVector::Dist(Before, Ship->GetActorLocation());
	TestTrue(*FString::Printf(TEXT("jump speed scales with game time (%.0f cm in 0.6 game s)"), Moved), Moved > Hyper->Tuning.JumpSpeed * 0.6f * 0.5f && Moved < Hyper->Tuning.JumpSpeed * 0.6f * 1.5f);
	TestTrue(TEXT("drop out on request"), Hyper->RequestDropOut());
	W.Advance(3.f / 0.25f);
	TestEqual(TEXT("idle again"), Hyper->GetState(), ETDHyperdriveState::Idle);
	TestTrue(*FString::Printf(TEXT("back to flight speed (%.0f)"), Ship->GetVelocity().Size()), Ship->GetVelocity().Size() < Ship->GetShipMovement()->GetFlightTuning().BoostSpeed * 1.05f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelSafetyKeys, "TriadDrift.Travel.Safety.RealKeysReachTheShipAndSurviveASwitch", TD_TEST_FLAGS)
bool FTDTravelSafetyKeys::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(-1500.f, 0.f, 1500.f));
	ATDShip* Other = W.Spawn<ATDShip>(FVector(0.f, 5000.f, 1500.f));
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(Ship, ATDSpacePlayerController::StaticClass()));
	if (!TestNotNull(TEXT("local space controller"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	W.Tick();

	auto Held = [&W, PC](const FKey& Key, TFunctionRef<bool()> Check)
	{
		FTDTestWorld::InjectKey(PC, Key, true);
		W.Tick();
		W.Tick();
		const bool bOk = Check();
		FTDTestWorld::InjectKey(PC, Key, false);
		W.Tick();
		return bOk;
	};
	TestTrue(TEXT("Space brakes"), Held(EKeys::SpaceBar, [PC] { return PC->GetCurrentFlightInput().bBrake; }));
	TestTrue(TEXT("Shift boosts"), Held(EKeys::LeftShift, [PC] { return PC->GetCurrentFlightInput().bBoost; }));
	TestTrue(TEXT("C drifts"), Held(EKeys::C, [PC] { return PC->GetCurrentFlightInput().bDrift; }));
	TestTrue(TEXT("S throttles back"), Held(EKeys::S, [PC] { return PC->GetCurrentFlightInput().Throttle < -0.9f; }));
	TestTrue(TEXT("D rolls"), Held(EKeys::D, [PC] { return PC->GetCurrentFlightInput().Roll > 0.9f; }));
	TestTrue(TEXT("H spools the hyperdrive"), Held(EKeys::H, [Ship] { return Ship->GetHyperdrive()->GetState() == ETDHyperdriveState::Spooling; }));
	W.Tick();

	// Held boost carries straight through a ship switch (no "ignore until released" on re-possess).
	FTDTestWorld::InjectKey(PC, EKeys::LeftShift, true);
	W.Tick();
	Ship->SetPlayerControlled(false);
	PC->Possess(Other);
	Other->SetPlayerControlled(true);
	W.Tick();
	W.Tick();
	TestTrue(TEXT("boost still held on the new ship"), PC->GetCurrentFlightInput().bBoost && Other->GetFlightInput().bBoost);
	FTDTestWorld::InjectKey(PC, EKeys::LeftShift, false);
	W.Tick();
	TestFalse(TEXT("release reaches the new ship"), Other->GetFlightInput().bBoost);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelReviewImpactCap, "TriadDrift.Travel.Safety.NoImpactOneShotsAShip", TD_TEST_FLAGS)
bool FTDTravelReviewImpactCap::RunTest(const FString& Parameters)
{
	for (const float Speed : { 15000.f, 80000.f })
	{
		FTDTestWorld W;
		TDTravelSafetyTest::SpawnBlocker(W, FVector(6000.f, 0.f, 0.f), &TDTravelSafetyTest::MakeWall);
		ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
		W.Tick();
		Ship->SetSpeedOverrideFor(TEXT("Test"), Speed, 1.f, 1000000.f); // slam into the wall at jump / burst speed
		Ship->GetShipMovement()->SetForwardSpeedImmediate(Speed);
		const float HP0 = Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield();
		W.Advance(1.f);
		const float Lost = HP0 - (Ship->GetHealth()->GetHull() + Ship->GetHealth()->GetShield());
		TestTrue(FString::Printf(TEXT("head-on at %.0f cm/s: alive, at most 60 per impact (lost %.1f)"), Speed, Lost), Ship->GetHealth()->IsAlive() && Lost > 0.f && Lost <= 60.f * 2.f + 0.01f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelReviewLandingDeath, "TriadDrift.Travel.Safety.LandingInvulnerableAndDeathAborts", TD_TEST_FLAGS)
bool FTDTravelReviewLandingDeath::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDLandingPad* Pad = W.Spawn<ATDLandingPad>(FVector::ZeroVector);
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W, FVector(-1500.f, 0.f, 1500.f));
	AActor* Shooter = W.Spawn<AActor>(FVector(5000.f, 0.f, 0.f));
	W.Tick();
	Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
	TestTrue(TEXT("land"), Ship->GetLanding()->RequestLand());
	W.Advance(1.f);
	TestEqual(TEXT("invulnerable while landing"), UTDStatics::ApplyTDDamage(Ship, 50.f, ETDDamageKind::Energy, Shooter, Shooter), 0.f);
	W.Advance(4.5f);
	TestTrue(TEXT("landed"), Ship->GetLanding()->IsLanded());
	TestEqual(TEXT("invulnerable while landed"), UTDStatics::ApplyTDDamage(Ship, 50.f, ETDDamageKind::Energy, Shooter, Shooter), 0.f);

	Ship->GetHealth()->Kill(); // scripted / cheat kill bypasses invulnerability
	TestEqual(TEXT("death aborts the landing"), Ship->GetLanding()->GetLandingState(), ETDLandingState::Flying);
	TestFalse(TEXT("pad released for others"), Pad->IsOccupied());
	TestFalse(TEXT("gear up, canopy shut"), Ship->GetLanding()->IsGearDown() || Ship->GetLanding()->IsCanopyOpen());
	Ship->GetHealth()->Revive();
	TestFalse(TEXT("revived ship isn't still invulnerable from the landing"), Ship->GetHealth()->IsInvulnerable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelReviewLocks, "TriadDrift.Travel.Safety.LockReasonRefusedEventOwnedLocks", TD_TEST_FLAGS)
bool FTDTravelReviewLocks::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetWorld()->GetGameInstance());
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
	if (!TestNotNull(TEXT("flow"), GI) || !TestNotNull(TEXT("ship"), Ship)) { return false; }
	W.Tick();
	UTDHyperdriveStateRecorder* Rec = NewObject<UTDHyperdriveStateRecorder>();
	Rec->AddToRoot();
	Ship->GetHyperdrive()->OnSpoolRefused.AddDynamic(Rec, &UTDHyperdriveStateRecorder::HandleRefused);

	const FText LockText = FText::FromString(TEXT("HYPERDRIVE LOCKED - ENEMIES IN RANGE"));
	Travel->AddHyperdriveLock(TEXT("Wave"), LockText);
	TestTrue(TEXT("player-facing reason"), Ship->GetHyperdrive()->GetBlockReasonText().EqualTo(LockText));
	TestTrue(TEXT("HUD line"), Travel->GetHyperdriveLockText().EqualTo(LockText));
	TestFalse(TEXT("pilot press refused"), Ship->GetHyperdrive()->RequestSpoolFromPilot());
	TestTrue(TEXT("refused event carries the reason"), Rec->RefusedTexts.Num() == 1 && Rec->RefusedTexts[0].EqualTo(LockText));

	// A CUT retake does NOT clear locks (review MAJOR): an active wave and the finale keep theirs through it.
	AActor* WaveOwner = W.Spawn<AActor>(FVector::ZeroVector);
	AActor* FinaleOwner = W.Spawn<AActor>(FVector::ZeroVector);
	const FText FinaleText = FText::FromString(TEXT("HYPERDRIVE LOCKED - DOMINION ASSAULT"));
	Travel->AddHyperdriveLock(TEXT("Wave"), LockText, WaveOwner);
	Travel->AddHyperdriveLock(TEXT("Finale"), FinaleText, FinaleOwner);
	GI->OnSceneRestart.Broadcast(TEXT("Wave1"), 2);
	W.Tick();
	FText Any;
	TestTrue(TEXT("retake: still locked"), Travel->IsHyperdriveLocked(Any));
	TestTrue(TEXT("retake: the finale lock is the active reason"), Any.EqualTo(FinaleText));
	TestFalse(TEXT("retake: pilot press still refused"), Ship->GetHyperdrive()->RequestSpoolFromPilot());
	Travel->RemoveHyperdriveLock(TEXT("Finale"));
	TestTrue(TEXT("finale released: the wave's lock remains"), Travel->IsHyperdriveLocked(Any) && Any.EqualTo(LockText));

	// A lock whose owner was destroyed is gone (no orphan can block the hyperdrive forever).
	WaveOwner->Destroy();
	W.Tick();
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	TestFalse(TEXT("owner destroyed: lock gone"), Travel->IsHyperdriveLocked(Any));
	TestTrue(TEXT("HUD line empty"), Travel->GetHyperdriveLockText().IsEmpty());
	TestTrue(TEXT("pilot press spools again"), Ship->GetHyperdrive()->RequestSpoolFromPilot());

	// Level travel: locks live per world, so a new level starts unlocked.
	Travel->AddHyperdriveLock(TEXT("Finale"), LockText, FinaleOwner);
	FTDTestWorld Next;
	TestFalse(TEXT("new level: no locks"), UTDTravelSubsystem::Get(Next.GetWorld())->IsHyperdriveLocked(Any));
	Rec->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelReviewLoops, "TriadDrift.Travel.Safety.CueLoopsStopOnParkDeathAndLanding", TD_TEST_FLAGS)
bool FTDTravelReviewLoops::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDShip* Ship = TDTravelSafetyTest::SpawnPlayerShip(W);
	Ship->SetShipVisuals(NewObject<UTDShipVisuals>());
	W.Tick();
	TestTrue(TEXT("engine trail + hum while flying"), Ship->IsCueLoopActive(TEXT("EngineTrail")) && Ship->IsCueLoopActive(TEXT("EngineLoop")));

	FTDFlightInput In;
	In.Throttle = 1.f;
	In.bBoost = true;
	Ship->SetFlightInput(In);
	W.Advance(0.3f);
	TestTrue(TEXT("boost loop running"), Ship->IsCueLoopActive(TEXT("BoostLoop")));
	Ship->SetFlightParked(true);
	W.Tick();
	TestFalse(TEXT("park ends the boost loop"), Ship->IsCueLoopActive(TEXT("BoostLoop")));
	TestFalse(TEXT("park stops the engine trail"), Ship->IsCueLoopActive(TEXT("EngineTrail")));
	TestFalse(TEXT("park stops the engine hum"), Ship->IsCueLoopActive(TEXT("EngineLoop")));
	Ship->SetFlightParked(false);
	W.Tick();
	TestTrue(TEXT("unpark relights the engine"), Ship->IsCueLoopActive(TEXT("EngineTrail")));

	In.bBoost = false;
	In.bDrift = true;
	Ship->SetFlightInput(In);
	W.Advance(0.3f);
	TestTrue(TEXT("drift loop running"), Ship->IsCueLoopActive(TEXT("DriftLoop")));
	Ship->GetHealth()->Kill();
	Ship->SetFlightParked(true); // what the squad does at the start of the tumble
	W.Tick();
	TestFalse(TEXT("death ends the drift loop"), Ship->IsCueLoopActive(TEXT("DriftLoop")));
	TestFalse(TEXT("dead: no engine trail"), Ship->IsCueLoopActive(TEXT("EngineTrail")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTravelReviewBoundaryInward, "TriadDrift.Travel.Safety.JumpInwardFromTheEdgeDoesNotFizzle", TD_TEST_FLAGS)
bool FTDTravelReviewBoundaryInward::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDTravelSubsystem::Get(W.GetWorld())->SetPlayBounds(FVector::ZeroVector, 400000.f);
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(380000.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f)); // in the warning band, nose home
	Ship->SetPlayerControlled(true);
	W.Tick();
	TestTrue(TEXT("in the warning band"), Ship->GetShipMovement()->IsNearBoundary());
	TestTrue(TEXT("spool"), Ship->GetHyperdrive()->StartSpool());
	W.Advance(2.f);
	TestEqual(TEXT("jumping inward from the edge keeps going"), Ship->GetHyperdrive()->GetState(), ETDHyperdriveState::Jumping);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
