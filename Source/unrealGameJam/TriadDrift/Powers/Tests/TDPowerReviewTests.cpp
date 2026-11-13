// Re-check of the second Powers review (P2): what the fixes promised, tested directly. Teammate 2.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Powers.Review

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/StrongObjectPtr.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"

namespace TDPowerReviewTest
{
	const TCHAR* const EngineVFX = TEXT("/Niagara/DefaultAssets/DefaultSystem.DefaultSystem");

}

/** With Power2Active art the dome builds no placeholder lattice, and its art rides the dome (which lags Bastion). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerReviewDomeArt, "TriadDrift.Powers.Review.DomeArtSkipsLatticeAndRidesTheDome", TD_TEST_FLAGS)
bool FTDPowerReviewDomeArt::RunTest(const FString& Parameters)
{
	for (const bool bArt : { false, true })
	{
		const FString Tag = bArt ? TEXT(" (with art)") : TEXT(" (no art)");
		FTDTestWorld W;
		AActor* Bastion = TDPowerTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f));
		UTDShieldDomePower* Dome = TDPowerTest::AddPower<UTDShieldDomePower>(Bastion);
		const TStrongObjectPtr<UTDShipVisuals> Visuals(NewObject<UTDShipVisuals>());
		if (bArt)
		{
			Visuals->Power2Active.VFX = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TDPowerReviewTest::EngineVFX));
		}
		Dome->ShipVisuals = Visuals.Get();
		Dome->Slot = ETDPowerInput::Signature2;
		TestTrue(TEXT("dome up") + Tag, Dome->TryActivate());
		const ATDShieldDome* Shell = Dome->GetDome();
		if (!TestNotNull(TEXT("dome actor") + Tag, Shell))
		{
			return false;
		}
		if (bArt)
		{
			TestTrue(TEXT("Power2Active cue requested"), FTDPowerPresentation::GetCueRequests(TEXT("Ship.Power2Active")) >= 1);
			TestTrue(TEXT("the active cue attaches to the dome, not Bastion"), Dome->GetActiveCueAttachRoot() == Shell->GetRootComponent());
		}
		W.Tick();
		W.Tick();
		TestEqual(TEXT("placeholder lattice only without art") + Tag, Shell->IsShowingPlaceholder(), !bArt);
	}
	return true;
}

/** A captured ship's "GravityWell" override is re-asserted if anything else is pushed on top of it. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerReviewWellReassert, "TriadDrift.Powers.Review.GravityWellReassertsItsCapture", TD_TEST_FLAGS)
bool FTDPowerReviewWellReassert::RunTest(const FString& Parameters)
{
	for (const float Step : TDPowerTest::kPowerFrameSteps)
	{
		TDPowerTest::FScopedPowerFrameStep StepScope(Step);
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		AActor* Bastion = TDPowerTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 100000.f, 0.f);
		UTDGravityWellPower* Well = TDPowerTest::AddPower<UTDGravityWellPower>(Bastion);
		ATDEnemyShip* Fighter = TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(4000.f, 1500.f, 0.f));
		Fighter->GetHealth()->InitMaxValues(100000.f, 0.f, true);
		UTDShipMovementComponent* Move = Fighter->GetShipMovement();
		TestTrue(TEXT("well launched") + Fps, Well->TryActivate());
		W.Advance(1.2f, Step);
		TestEqual(TEXT("captured: the well's override is on top") + Fps, Move->GetActiveSpeedOverrideOwner(), FName(TEXT("GravityWell")));
		Move->SetSpeedOverrideFor(TEXT("SomethingElse"), 20000.f, 1.f);
		W.Tick(Step);
		W.Tick(Step);
		TestEqual(TEXT("re-asserted over a later override") + Fps, Move->GetActiveSpeedOverrideOwner(), FName(TEXT("GravityWell")));
		W.Advance(6.f, Step);
		TestFalse(TEXT("released when the vortex ends") + Fps, Move->HasSpeedOverrideFor(TEXT("GravityWell")));
	}
	return true;
}

/**
 * Review C2: the real Bastion, flown by a local player, painting through its real camera view. Turning the ship
 * (and its camera) ages the old locks out; the tap goes to the new group; a lock beyond the swarm's range drops.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerReviewRealSwarm, "TriadDrift.Powers.Review.RealBastionLocksFollowTheCamera", TD_TEST_FLAGS)
bool FTDPowerReviewRealSwarm::RunTest(const FString& Parameters)
{
	for (const float Step : TDPowerTest::kPowerFrameSteps)
	{
		TDPowerTest::FScopedPowerFrameStep StepScope(Step);
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		APlayerController* PC = W.SpawnLocalPlayerController();
		ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
		Squad->SetSquadPlayerController(PC);
		Squad->StartingShip = ETDShipId::Bastion;
		Squad->SpawnSquad(FTransform::Identity);
		W.Advance(0.5f, Step);
		ATDShip* Bastion = Squad->GetShip(ETDShipId::Bastion);
		if (!TestTrue(TEXT("flying Bastion") + Fps, Bastion && PC->GetPawn() == Bastion))
		{
			return false;
		}
		Squad->SetWingmenPassive(true); // only the player's guns and missiles in this test
		UTDTargetingComponent* Targeting = Bastion->FindComponentByClass<UTDTargetingComponent>();
		UTDMissileSwarmPower* Swarm = Cast<UTDMissileSwarmPower>(UTDPowerComponent::FindPower(Bastion, ETDPowerInput::Signature1));
		if (!TestNotNull(TEXT("targeting") + Fps, Targeting) || !TestNotNull(TEXT("swarm") + Fps, Swarm))
		{
			return false;
		}
		auto HoldStill = [](ATDEnemyShip* Ship)
		{
			if (UMovementComponent* Move = Ship ? Ship->FindComponentByClass<UMovementComponent>() : nullptr)
			{
				Move->StopMovementImmediately();
				Move->Deactivate();
			}
			return Ship;
		};
		const FVector Here = Bastion->GetActorLocation();
		const FRotator Facing = Bastion->GetActorRotation();
		TArray<TWeakObjectPtr<ATDEnemyShip>> OldGroup, NewGroup;
		for (int32 I = 0; I < 4; ++I)
		{
			OldGroup.Add(HoldStill(TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter, Here + Facing.RotateVector(FVector(20000.f, (I - 1.5f) * 700.f, 0.f)))));
			NewGroup.Add(HoldStill(TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter, Here + Facing.RotateVector(FVector(0.f, 30000.f, (I - 1.5f) * 700.f)))));
		}
		W.Advance(2.f, Step);
		TestTrue(TEXT("old group painted through the camera") + Fps, Targeting->GetLockedTargets().Num() >= 3);

		// The real ship cruises on: place both groups relative to it now. Old group dead ahead of the old heading,
		// new group 25 000 cm along the new one (the ship flies ~10 000 cm toward it in the next 2 s).
		const FRotator Turned = Facing + FRotator(0.f, 90.f, 0.f);
		const FVector Now = Bastion->GetActorLocation();
		for (int32 I = 0; I < 4; ++I)
		{
			if (ATDEnemyShip* Old = OldGroup[I].Get())
			{
				Old->SetActorLocation(Now + Facing.RotateVector(FVector(15000.f, (I - 1.5f) * 700.f, 0.f)));
			}
			if (ATDEnemyShip* New = NewGroup[I].Get())
			{
				New->SetActorLocation(Now + Turned.RotateVector(FVector(25000.f, (I - 1.5f) * 900.f, 0.f)));
			}
		}
		// Turn ship and camera 90 degrees to the new group; hold 2 s.
		Bastion->SetActorRotation(Turned);
		PC->SetControlRotation(Turned);
		W.Advance(2.f, Step);
		for (const TWeakObjectPtr<ATDEnemyShip>& Old : OldGroup)
		{
			TestFalse(TEXT("old lock aged out once out of view") + Fps, Targeting->GetLockedTargets().Contains(Old.Get()));
		}
		UTDPowerComponent::RoutePowerInput(Bastion, ETDPowerInput::Signature1, true);
		W.Tick(Step);
		UTDPowerComponent::RoutePowerInput(Bastion, ETDPowerInput::Signature1, false);
		W.Tick(Step);
		const TArray<TWeakObjectPtr<AActor>>& Targets = Swarm->GetLastTargets();
		TestTrue(TEXT("tap split across locks") + Fps, Targets.Num() > 0);
		for (const TWeakObjectPtr<AActor>& T : Targets)
		{
			TestTrue(TEXT("every missile to the new group") + Fps,
				NewGroup.ContainsByPredicate([&T](const TWeakObjectPtr<ATDEnemyShip>& N) { return N.Get() == T.Get(); }));
		}

		// Range: once the swarm is ready again, paint a fresh fighter just ahead, then move it beyond the swarm's
		// range: the lock drops.
		W.Advance(9.5f, Step);
		const FVector Ahead = Bastion->GetActorLocation() + Bastion->GetActorForwardVector() * 9000.f;
		ATDEnemyShip* Far = HoldStill(TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter, Ahead));
		W.Advance(1.f, Step);
		TestTrue(TEXT("fresh target painted") + Fps, Targeting->GetLockedTargets().Contains(Far));
		Far->SetActorLocation(Bastion->GetActorLocation() + Bastion->GetActorForwardVector() * (Swarm->MissileSpec.Range + 5000.f));
		W.Tick(Step);
		TestFalse(TEXT("a lock beyond the swarm's range drops") + Fps, Targeting->GetLockedTargets().Contains(Far));
	}
	return true;
}

/** Audit pass 2: spec sentences that had no test. Ram 7 s cooldown; swarm 200 cm splash; well stops fighters firing. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerAuditGaps, "TriadDrift.Powers.Review.SpecSentencesWithoutATest", TD_TEST_FLAGS)
bool FTDPowerAuditGaps::RunTest(const FString& Parameters)
{
	for (const float Step : TDPowerTest::kPowerFrameSteps)
	{
		TDPowerTest::FScopedPowerFrameStep StepScope(Step);
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		// Ram: 7 s cooldown, counted from the press (wind-up + dash 1.5 s included).
		{
			FTDTestWorld W;
			ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
			Lancer->SetTeam(ETDTeam::Player);
			UTDRamPower* Ram = TDPowerTest::AddPower<UTDRamPower>(Lancer);
			TestTrue(TEXT("ram starts") + Fps, Ram->TryActivate());
			W.Advance(6.8f, Step);
			TestFalse(TEXT("not ready at 6.8 s") + Fps, Ram->CanActivate());
			W.Advance(0.5f, Step);
			TestTrue(TEXT("ready by 7.3 s") + Fps, Ram->CanActivate());
		}
		// Swarm: 18 per missile with a 200 cm splash (the splash mechanics themselves: Combat.Projectile.SplashRadius).
		{
			FTDTestWorld W;
			AActor* Bastion = TDPowerTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
			const UTDMissileSwarmPower* Swarm = TDPowerTest::AddPower<UTDMissileSwarmPower>(Bastion);
			TestEqual(TEXT("swarm missile damage 18") + Fps, Swarm->MissileSpec.Damage, 18.f);
			TestEqual(TEXT("swarm splash radius 200 cm") + Fps, Swarm->MissileSpec.SplashRadius, 200.f);
			TestEqual(TEXT("swarm splash damage 18") + Fps, Swarm->MissileSpec.GetSplashDamage(), 18.f);
			TestEqual(TEXT("twelve missiles") + Fps, Swarm->MissileCount, 12);
		}
		// Gravity well: an enemy FIGHTER inside the vortex can't fire (turrets were already covered).
		{
			FTDTestWorld W;
			AActor* Bastion = TDPowerTest::SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(100.f), 100000.f, 0.f);
			UTDGravityWellPower* Well = TDPowerTest::AddPower<UTDGravityWellPower>(Bastion);
			ATDEnemyShip* Fighter = TDPowerTest::SpawnEnemyShip(W, TDEnemyIds::ShardFighter, FVector(4000.f, 1200.f, 0.f));
			Fighter->GetHealth()->InitMaxValues(100000.f, 0.f, true);
			TestTrue(TEXT("fighter can fire before") + Fps, Fighter->CanFire());
			TestTrue(TEXT("well launched") + Fps, Well->TryActivate());
			W.Advance(1.5f, Step);
			TestFalse(TEXT("fighter inside the vortex can't fire") + Fps, Fighter->CanFire());
			W.Advance(5.f, Step);
			TestTrue(TEXT("fighter can fire again after the vortex") + Fps, Fighter->CanFire());
		}
	}
	return true;
}

/** ENG-10: Lancer steers at 40% while dashing (T1's GetActiveSteeringScale), and full steering comes back after. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDPowerRamSteering, "TriadDrift.Powers.Review.RamSteersAtFortyPercent", TD_TEST_FLAGS)
bool FTDPowerRamSteering::RunTest(const FString& Parameters)
{
	for (const float Step : TDPowerTest::kPowerFrameSteps)
	{
		TDPowerTest::FScopedPowerFrameStep StepScope(Step);
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		ATDSpacecraft* Lancer = W.Spawn<ATDSpacecraft>(FVector::ZeroVector, FRotator::ZeroRotator);
		Lancer->SetTeam(ETDTeam::Player);
		UTDRamPower* Ram = TDPowerTest::AddPower<UTDRamPower>(Lancer);
		const UTDShipMovementComponent* Move = Lancer->GetShipMovement();
		TestEqual(TEXT("full steering before") + Fps, Move->GetActiveSteeringScale(), 1.f);
		TestTrue(TEXT("ram starts") + Fps, Ram->TryActivate());
		bool bDashed = false;
		float DashSteering = -1.f;
		for (float T = 0.f; T < 1.f && !bDashed; T += Step)
		{
			W.Tick(Step);
			bDashed = Ram->IsDashing();
		}
		DashSteering = Move->GetActiveSteeringScale();
		TestTrue(TEXT("dashing") + Fps, bDashed);
		TestEqual(TEXT("40% steering during the dash") + Fps, DashSteering, Ram->SteeringScale, 0.001f);
		TestEqual(TEXT("the spec's 40%") + Fps, Ram->SteeringScale, 0.4f, 0.001f);
		W.Advance(2.f, Step);
		TestFalse(TEXT("dash over") + Fps, Ram->IsPowerActive());
		TestEqual(TEXT("full steering back") + Fps, Move->GetActiveSteeringScale(), 1.f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
