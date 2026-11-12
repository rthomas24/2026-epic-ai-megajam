// Safety tests for travel: speed-override ownership, hyperdrive never ending inside geometry, wells /
// boundary / locks at several frame rates, entry and level-travel edge cases.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.Safety

#include "Misc/AutomationTest.h"

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
#include "TriadDrift/Tests/TDTestWorld.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
