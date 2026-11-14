// Real-collision tests for the Hive Carrier's parts and the Dominion's generators (review B1).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies.Carrier

#include "Misc/AutomationTest.h"
#include "Components/SphereComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "Engine/World.h"

namespace
{
	ATDEnemyShip* TDCarrierTest_SpawnCarrier(FTDTestWorld& W, const FVector& Loc, bool bKeepAI)
	{
		ATDEnemyShip* C = W.Spawn<ATDEnemyShip>(Loc);
		if (!bKeepAI)
		{
			if (AController* Ctl = C->GetController()) { Ctl->UnPossess(); Ctl->Destroy(); }
		}
		C->InitFromSpec(TDEnemyIds::HiveCarrier);
		C->MaxLaunchedAlive = 0;
		return C;
	}

	/** A heavy player bolt from From at Target, led for the target's motion (a real ATDProjectile with its real sweep). */
	ATDProjectile* TDCarrierTest_FireAt(FTDTestWorld& W, const FVector& From, const AActor* Target, float Damage, ETDDamageKind Kind = ETDDamageKind::Explosive)
	{
		const AActor* Mover = Target->GetAttachParentActor() ? Target->GetAttachParentActor() : Target;
		const FVector To = Target->GetActorLocation() + Mover->GetVelocity() * (FVector::Dist(From, Target->GetActorLocation()) / 30000.f);
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile;
		Spec.Damage = Damage;
		Spec.DamageKind = Kind;
		Spec.SplashRadius = 0.f;
		Spec.bHoming = false;
		Spec.ProximityFuseRadius = 0.f;	// a plain direct-hit bolt (a fuse would trigger on the carrier's bounds)
		Spec.Speed = 30000.f;
		Spec.Range = 20000.f;
		Spec.bInheritShooterVelocity = false;
		return ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform((To - From).Rotation(), From), nullptr, ETDTeam::Player);
	}

	AActor* TDCarrierTest_SweepShip(FTDTestWorld& W, const FVector& From, const FVector& To)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TDTestShipSweep), false);
		const bool bHit = W.GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, TDCollision::Ship, FCollisionShape::MakeSphere(220.f), Params);
		return bHit ? Hit.GetActor() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCarrierRealShot, "TriadDrift.Enemies.Carrier.RealProjectileKillsBay", TD_TEST_FLAGS)
bool FTDCarrierRealShot::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDEnemyShip* C = TDCarrierTest_SpawnCarrier(W, FVector::ZeroVector, false);
	TArray<ATDCarrierBay*> Bays = C->GetBays();
	if (!TestEqual(TEXT("3 bays"), Bays.Num(), 3)) { return false; }
	ATDCarrierBay* Bay = Bays[0];

	// Sphere-mounted parts sit outside the blocking sphere, facing radially out. Socket-mounted parts (an art hull with
	// Bay_N / Turret_N) sit on the hull where the art says, facing the socket's +X: there "outside" means the part's
	// line of fire is clear of the hull's own collision.
	const bool bSockets = C->HasSocketMountedParts();
	const FVector Radial = (Bay->GetActorLocation() - C->GetActorLocation()).GetSafeNormal();
	const FVector Out = bSockets ? Bay->GetActorForwardVector() : Radial;
	if (bSockets)
	{
		FHitResult Block;
		TestFalse(TEXT("bay on the hull's surface, its line of fire clear of the hull"),
			C->GetHullMesh()->LineTraceComponent(Block, Bay->GetActorLocation() + Out * 6000.f, Bay->GetActorLocation(), FCollisionQueryParams(SCENE_QUERY_STAT(TDTestBayClear), false)));
		TestTrue(TEXT("bay faces away from the hull centre"), FVector::DotProduct(Bay->GetActorForwardVector(), Radial) > 0.f);
	}
	else
	{
		TestTrue(TEXT("bay mounted outside the hull sphere"), FVector::Dist(Bay->GetActorLocation(), C->GetActorLocation()) > C->GetSpec().Radius + Bay->GetHalfExtent());
		TestTrue(TEXT("bay faces outward (no double rotation)"), FVector::DotProduct(Bay->GetActorForwardVector(), Out) > 0.95f);
	}

	// Two real 200-damage bolts fired from outside straight at the bay.
	TDCarrierTest_FireAt(W, Bay->GetActorLocation() + Out * 6000.f, Bay, 200.f);
	W.Advance(0.5f);
	TestEqual(TEXT("first bolt hit the bay, not the hull"), Bay->GetHealth()->GetHull(), 100.f);
	TestEqual(TEXT("carrier hull untouched"), C->GetHealth()->GetHull(), C->GetHealth()->MaxHull);
	TDCarrierTest_FireAt(W, Bay->GetActorLocation() + Out * 6000.f, Bay, 200.f);
	W.Advance(0.5f);
	TestFalse(TEXT("real shots destroyed the bay"), Bay->IsAlive());
	TestEqual(TEXT("2 bays left"), C->GetLivingBayCount(), 2);

	// Turrets are reachable too.
	ATDEnemyTurret* T = C->GetTurrets()[0];
	const FVector TOut = bSockets ? T->GetActorForwardVector() : (T->GetActorLocation() - C->GetActorLocation()).GetSafeNormal();
	TDCarrierTest_FireAt(W, T->GetActorLocation() + TOut * 5000.f, T, 500.f);
	W.Advance(0.5f);
	TestFalse(TEXT("real shot destroyed a point-defense turret"), T->IsAlive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCarrierRam, "TriadDrift.Enemies.Carrier.RamPathReachesBayDoors", TD_TEST_FLAGS)
bool FTDCarrierRam::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDEnemyShip* C = TDCarrierTest_SpawnCarrier(W, FVector::ZeroVector, false);
	ATDCarrierBay* Bay = C->GetBays()[1];
	const FVector Out = (Bay->GetActorLocation() - C->GetActorLocation()).GetSafeNormal();

	// The ram (ENG-10) moves a ship along its path on the Ship channel and damages whatever it hits
	// with ETDDamageKind::Ram. Prove the same sweep reaches the bay before the hull.
	AActor* HitActor = TDCarrierTest_SweepShip(W, Bay->GetActorLocation() + Out * 5000.f, C->GetActorLocation());
	TestTrue(TEXT("a ship flying at the bay hits the bay first"), HitActor == Bay);

	UTDStatics::ApplyTDDamage(HitActor, 100.f, ETDDamageKind::Ram, nullptr, nullptr, Bay->GetActorLocation(), -Out);
	TestTrue(TEXT("ram breaks the doors"), Bay->AreDoorsBroken());
	TestEqual(TEXT("ram lands doubled on the open bay"), Bay->GetHealth()->GetHull(), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCarrierMoves, "TriadDrift.Enemies.Carrier.ClosesDistanceWithPartsAttached", TD_TEST_FLAGS)
bool FTDCarrierMoves::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Target = W.Spawn<AActor>(FVector(45000.f, 0.f, 0.f));
	{
		USceneComponent* Root = NewObject<USceneComponent>(Target, TEXT("Root"));
		Target->SetRootComponent(Root);
		Root->RegisterComponent();
		Target->SetActorLocation(FVector(45000.f, 0.f, 0.f));
		UTDHealthComponent* H = NewObject<UTDHealthComponent>(Target, TEXT("Health"));
		H->DefaultTeam = ETDTeam::Player;
		H->RegisterComponent();
	}
	ATDEnemyShip* C = TDCarrierTest_SpawnCarrier(W, FVector::ZeroVector, /*bKeepAI*/ true);
	TestNotNull(TEXT("carrier is piloted"), Cast<ATDEnemyAIController>(C->GetController()));
	const float Start = FVector::Dist(C->GetActorLocation(), Target->GetActorLocation());
	W.Advance(8.f);
	const float End = FVector::Dist(C->GetActorLocation(), Target->GetActorLocation());
	TestTrue(*FString::Printf(TEXT("carrier closes distance (%.0f -> %.0f)"), Start, End), End < Start - 5000.f);
	TestEqual(TEXT("parts came along"), C->GetBays().Num(), 3);
	if (C->HasSocketMountedParts())
	{
		// On the hull: a bay rides its socket (same offset from the hull after the move) and its line of fire stays clear.
		const ATDCarrierBay* Bay = C->GetBays()[0];
		FHitResult Block;
		TestFalse(TEXT("bays still on the hull's surface after moving"),
			C->GetHullMesh()->LineTraceComponent(Block, Bay->GetActorLocation() + Bay->GetActorForwardVector() * 6000.f, Bay->GetActorLocation(), FCollisionQueryParams(SCENE_QUERY_STAT(TDTestBayClear), false)));
	}
	else
	{
		TestTrue(TEXT("bays still outside the hull after moving"), FVector::Dist(C->GetBays()[0]->GetActorLocation(), C->GetActorLocation()) > C->GetSpec().Radius);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDominionRealShot, "TriadDrift.World.Dominion.RealProjectileReachesGenerator", TD_TEST_FLAGS)
bool FTDDominionRealShot::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	Cast<UTDWaveTestGameInstance>(W.GetGameInstance())->Progress.bDominionRevealed = true; Cast<UTDWaveTestGameInstance>(W.GetGameInstance())->Progress.Phase = ETDGamePhase::DominionAssault;
	ATDDominion* D = W.Spawn<ATDDominion>(FVector::ZeroVector);
	D->MaxLaunchedAlive = 0;
	W.Advance(D->WarpInSeconds + 0.2f);
	ATDShieldGenerator* G = D->GetGenerators()[0];
	const FVector Out = (G->GetActorLocation() - D->GetActorLocation()).GetSafeNormal();
	TDCarrierTest_FireAt(W, G->GetActorLocation() + Out * 6000.f, G, 300.f);
	W.Advance(0.5f);
	TestEqual(TEXT("real shot damaged the generator"), G->GetHealth()->GetHull(), 500.f);
	TestTrue(TEXT("a ship flying at the generator hits it"), TDCarrierTest_SweepShip(W, G->GetActorLocation() + Out * 5000.f, G->GetActorLocation()) == G);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
