// Automation tests for portal gates (ENG-16). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Gate

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace
{
	struct FGateRig
	{
		FTDTestWorld W;
		ATDPortalGate* A = nullptr;
		ATDPortalGate* B = nullptr;

		FGateRig()
		{
			A = W.Spawn<ATDPortalGate>(FVector::ZeroVector, FRotator::ZeroRotator);			// travel +X into A
			B = W.Spawn<ATDPortalGate>(FVector(100000.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f));	// come out along +Y
			ATDPortalGate::LinkGates(A, B);
		}

		ATDSpacecraft* SpawnShip(const FVector& Loc, const FRotator& Facing, ETDTeam Team)
		{
			ATDSpacecraft* S = W.Spawn<ATDSpacecraft>(Loc, Facing);
			S->SetTeam(Team);
			if (UTDHealthComponent* H = S->GetHealth()) { H->DefaultTeam = Team; }
			S->SetSpeedOverride(6000.f, 1.f);
			return S;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldGateMaths, "TriadDrift.World.Gate.CrossingMaths", TD_TEST_FLAGS)
bool FTDWorldGateMaths::RunTest(const FString& Parameters)
{
	const FTransform Gate = FTransform::Identity;
	bool bFwd = false;
	TestTrue(TEXT("crosses forward"), ATDPortalGate::SegmentCrossesGate(Gate, 1000.f, FVector(-100, 0, 0), FVector(100, 0, 0), bFwd) && bFwd);
	TestTrue(TEXT("crosses backward"), ATDPortalGate::SegmentCrossesGate(Gate, 1000.f, FVector(100, 50, 0), FVector(-100, 50, 0), bFwd) && !bFwd);
	TestFalse(TEXT("outside the disc"), ATDPortalGate::SegmentCrossesGate(Gate, 1000.f, FVector(-100, 1500, 0), FVector(100, 1500, 0), bFwd));
	TestFalse(TEXT("same side"), ATDPortalGate::SegmentCrossesGate(Gate, 1000.f, FVector(10, 0, 0), FVector(300, 0, 0), bFwd));
	TestTrue(TEXT("fast mover (hyperdrive step) still crosses"), ATDPortalGate::SegmentCrossesGate(Gate, 1000.f, FVector(-1400, 0, 0), FVector(1400, 0, 0), bFwd));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldGateShip, "TriadDrift.World.Gate.ShipTransitsToPartner", TD_TEST_FLAGS)
bool FTDWorldGateShip::RunTest(const FString& Parameters)
{
	FGateRig R;
	ATDSpacecraft* Ship = R.SpawnShip(FVector(-800.f, 300.f, 0.f), FRotator::ZeroRotator, ETDTeam::Player);
	R.W.Advance(0.6f);

	TestEqual(TEXT("one transit"), R.A->GetTransitCount(), 1);
	const FVector Rel = Ship->GetActorLocation() - R.B->GetActorLocation();
	TestTrue(TEXT("came out at the partner gate"), Rel.Size() < 4000.f);
	TestTrue(TEXT("travelling out along the partner's forward"), FVector::DotProduct(Ship->GetActorForwardVector(), R.B->GetActorForwardVector()) > 0.95f);
	TestTrue(TEXT("kept its speed"), Ship->GetVelocity().Size() > 4000.f);

	R.W.Advance(2.f);
	TestEqual(TEXT("no bounce back through the pair"), R.A->GetTransitCount() + R.B->GetTransitCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldGateBack, "TriadDrift.World.Gate.TwoSidedAndInactive", TD_TEST_FLAGS)
bool FTDWorldGateBack::RunTest(const FString& Parameters)
{
	FGateRig R;
	// From the back: travelling -X into A -> out of B's back (-Y).
	ATDSpacecraft* Back = R.SpawnShip(FVector(800.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), ETDTeam::Player);
	R.W.Advance(0.6f);
	TestEqual(TEXT("back-side transit"), R.A->GetTransitCount(), 1);
	TestTrue(TEXT("exits the partner's back"), FVector::DotProduct(Back->GetActorForwardVector(), -R.B->GetActorForwardVector()) > 0.95f);

	// Closed gates do nothing.
	R.A->SetGateActive(false);
	ATDSpacecraft* Blocked = R.SpawnShip(FVector(-800.f, 0.f, 500.f), FRotator::ZeroRotator, ETDTeam::Player);
	R.W.Advance(0.6f);
	TestEqual(TEXT("inactive gate: no transit"), R.A->GetTransitCount(), 1);
	TestTrue(TEXT("ship flew straight through the closed frame"), Blocked->GetActorLocation().X > 0.f && FVector::Dist(Blocked->GetActorLocation(), R.B->GetActorLocation()) > 50000.f);

	// Unlinked gates do nothing either.
	ATDPortalGate* Lonely = R.W.Spawn<ATDPortalGate>(FVector(0.f, 50000.f, 0.f));
	TestFalse(TEXT("no partner -> inactive"), Lonely->IsGateActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldGateEnemy, "TriadDrift.World.Gate.EnemiesLoseTracking", TD_TEST_FLAGS)
bool FTDWorldGateEnemy::RunTest(const FString& Parameters)
{
	FGateRig R;
	ATDEnemyShip* E = R.W.Spawn<ATDEnemyShip>(FVector(-800.f, 0.f, 0.f));
	if (AController* C = E->GetController()) { C->UnPossess(); C->Destroy(); }
	E->InitFromSpec(TDEnemyIds::ShardFighter);
	E->SetSpeedOverride(6000.f, 1.f);
	R.W.Advance(0.6f);
	TestEqual(TEXT("enemy transits"), R.A->GetTransitCount(), 1);
	TestTrue(TEXT("enemy lost tracking (2 s)"), E->IsTrackingLost());
	R.W.Advance(2.f);
	TestFalse(TEXT("tracking returns"), E->IsTrackingLost());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldGateProjectile, "TriadDrift.World.Gate.EnemyShotsAreReturned", TD_TEST_FLAGS)
bool FTDWorldGateProjectile::RunTest(const FString& Parameters)
{
	FGateRig R;
	ATDSpacecraft* Shooter = R.SpawnShip(FVector(-30000.f, 0.f, 0.f), FRotator::ZeroRotator, ETDTeam::Enemy);
	Shooter->SetSpeedOverride(0.f, 0.f);
	const FTDWeaponSpec& Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyPulseGuns);
	ATDProjectile* Shot = ATDProjectile::Spawn(R.W.GetWorld(), Spec.Projectile, FTransform(FRotator::ZeroRotator, FVector(-1500.f, 0.f, 0.f)), Shooter, ETDTeam::Enemy);
	if (!TestNotNull(TEXT("projectile spawned"), Shot)) { return false; }
	R.W.Advance(0.2f);
	TestEqual(TEXT("projectile transited"), R.A->GetTransitCount(), 1);
	TestTrue(TEXT("enemy shot returned to the Player team"), Shot->GetTeam() == ETDTeam::Player);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
