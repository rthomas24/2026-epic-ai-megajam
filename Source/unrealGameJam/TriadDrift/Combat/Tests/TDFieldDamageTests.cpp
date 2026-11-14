// T6's destructible field contract (ITDFieldDamageable), engineering side (Teammate 2).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Field

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/ScopeExit.h"
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/Tests/TDFieldTestActor.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

// A shot into a field reaches it with the real hit (its component) and the shot's damage; a player shot raises the quiet
// field-hit notice for the HUD; nothing goes through ApplyTDDamage (no OnHitConfirmed).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFieldProjectile, "TriadDrift.Combat.Field.ProjectileReachesTheInstance", TD_TEST_FLAGS)
bool FTDFieldProjectile::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDFieldTestActor* Field = W.Spawn<ATDFieldTestActor>(FVector(3000.f, 0.f, 0.f));
	AActor* Shooter = W.Spawn<AActor>(FVector::ZeroVector);
	int32 Notices = 0;
	const FDelegateHandle H = FTDFieldDamage::OnPlayerFieldHit.AddLambda([&Notices](AActor*, const FVector&) { ++Notices; });
	ON_SCOPE_EXIT { FTDFieldDamage::OnPlayerFieldHit.Remove(H); };
	const FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
	ATDProjectile::Launch(W.GetWorld(), Spec, FTransform(FRotator::ZeroRotator, FVector(500.f, 0.f, 0.f)), Shooter, ETDTeam::Player, nullptr,
		FVector::ZeroVector, 1.f, true);
	W.Advance(1.f);
	TestEqual(TEXT("the field got the hit"), Field->ProjectileHits, 1);
	TestTrue(TEXT("with the shot's damage"), Field->LastDamage > 0.f);
	TestTrue(TEXT("and the hit component"), Field->LastHitComponent.Get() == Field->Box);
	TestEqual(TEXT("one quiet HUD notice for a player shot"), Notices, 1);

	// An explosive shot (a rocket) detonating next to a registered field splashes it.
	FTDFieldDamage::Register(Field);
	ON_SCOPE_EXIT { FTDFieldDamage::Unregister(Field); };
	const FTDProjectileSpec Rocket = UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets).Projectile;
	TestTrue(TEXT("rockets splash"), Rocket.SplashRadius > 0.f);
	ATDProjectile* R = ATDProjectile::Launch(W.GetWorld(), Rocket, FTransform(FRotator::ZeroRotator, Field->GetActorLocation() + FVector(0.f, 0.f, 2000.f)),
		Shooter, ETDTeam::Player, nullptr, FVector::ZeroVector, 1.f, true);
	if (R)
	{
		R->TeleportProjectile(Field->GetActorLocation() + FVector(0.f, 0.f, 500.f), FVector::ZeroVector);
		R->Detonate();
	}
	TestTrue(TEXT("a detonation next to the field splashes it"), Field->Splashes > 0);
	return true;
}

// Lancer's dash reaches a registered field: a small rock breaks and the dash goes on; a large one stops it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDFieldRam, "TriadDrift.Combat.Field.RamBreaksOrStops", TD_TEST_FLAGS)
bool FTDFieldRam::RunTest(const FString& Parameters)
{
	for (const bool bBlock : { false, true })
	{
		FTDTestWorld W;
		ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
		ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
		Squad->SetSquadPlayerController(PC);
		Squad->StartingShip = ETDShipId::Lancer;
		Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20000.f)));
		W.Advance(.3f);
		ATDShip* Lancer = Squad->GetActiveShip();
		UTDRamPower* Ram = Lancer ? Cast<UTDRamPower>(UTDPowerComponent::FindPower(Lancer, ETDPowerInput::Signature1)) : nullptr;
		if (!TestNotNull(TEXT("Lancer's ram"), Ram))
		{
			return false;
		}
		ATDFieldTestActor* Field = W.Spawn<ATDFieldTestActor>(Lancer->GetActorLocation() + Lancer->GetActorForwardVector() * 6000.f);
		Field->Box->SetCollisionResponseToAllChannels(ECR_Ignore); // the sweep, not the flight model, reaches it here
		Field->bBlockRam = bBlock;
		FTDFieldDamage::Register(Field);
		ON_SCOPE_EXIT { FTDFieldDamage::Unregister(Field); };
		const FString Tag = bBlock ? TEXT(" (large)") : TEXT(" (small)");
		TestTrue(TEXT("ram") + Tag, UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, true));
		UTDPowerComponent::RoutePowerInput(Lancer, ETDPowerInput::Signature1, false);
		for (float T = 0.f; T < 3.f && Field->RamSweeps == 0; T += 1.f / 60.f)
		{
			W.Tick(1.f / 60.f);
		}
		TestTrue(TEXT("the dash reached the field") + Tag, Field->RamSweeps > 0);
		if (bBlock)
		{
			TestTrue(TEXT("a large rock stops the dash"), !Ram->IsDashing() && Ram->WasStoppedByImpact());
		}
		else
		{
			TestFalse(TEXT("a small rock does not stop it"), Ram->WasStoppedByImpact());
		}
	}
	return true;
}

#endif
