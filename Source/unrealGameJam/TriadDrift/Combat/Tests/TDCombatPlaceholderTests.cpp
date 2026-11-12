// Placeholder streaks and flashes (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.Placeholder

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/Tests/TDCombatTestCommon.h"

namespace TDCombatPlaceholderTest
{
	const FName PlayerWeaponIds[] = { TDWeaponIds::PhasePulseLasers, TDWeaponIds::SplitterBolt, TDWeaponIds::NeedleCannons,
		TDWeaponIds::SpikeRockets, TDWeaponIds::BreakerBlaster, TDWeaponIds::FlakBurst };
	const FName EnemyWeaponIds[] = { TDWeaponIds::EnemyPulseGuns, TDWeaponIds::EnemyHomingMissile, TDWeaponIds::EnemyHeavyBolt,
		TDWeaponIds::EnemySalvoMissile, TDWeaponIds::EnemyPlasmaMines, TDWeaponIds::EnemyPointDefense };
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderStreakSizes, "TriadDrift.Combat.Placeholder.StreakSizes", TD_TEST_FLAGS)
bool FTDCombatPlaceholderStreakSizes::RunTest(const FString& Parameters)
{
	for (const FName Id : TDCombatPlaceholderTest::PlayerWeaponIds)
	{
		const FTDProjectileSpec& Spec = UTDWeaponLibrary::Get(Id).Projectile;
		const FTDStreakLook Look = FTDCombatPlaceholders::ComputeStreak(Spec);
		const FString Name = Id.ToString();
		TestTrue(Name + TEXT(" is a streak"), Look.bStreak);
		TestTrue(Name + TEXT(" length 250-800 cm"), Look.Length >= 250.f && Look.Length <= 800.f);
		TestTrue(Name + TEXT(" thick enough to read (>= 35 cm)"), Look.Thickness >= 35.f);
		if (!Look.bHeavy)
		{
			TestTrue(Name + TEXT(" laser/needle is 300-800 cm"), Look.Length >= 300.f && Look.Length <= 800.f);
		}
	}
	const FTDStreakLook Laser = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile);
	const FTDStreakLook Rocket = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets).Projectile);
	TestTrue(TEXT("rockets are fatter than lasers"), Rocket.bHeavy && Rocket.Thickness > Laser.Thickness);

	const FTDStreakLook Missile = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile);
	TestTrue(TEXT("homing missile has a glowing head bigger than its trail"), Missile.bHead && Missile.HeadDiameter > Missile.Thickness);
	TestFalse(TEXT("lasers have no head"), Laser.bHead);
	const FTDStreakLook Mine = FTDCombatPlaceholders::ComputeStreak(UTDWeaponLibrary::Get(TDWeaponIds::EnemyPlasmaMines).Projectile);
	TestFalse(TEXT("slow mines keep their sphere"), Mine.bStreak);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderStreakOnProjectile, "TriadDrift.Combat.Placeholder.StreakOnProjectile", TD_TEST_FLAGS)
bool FTDCombatPlaceholderStreakOnProjectile::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	auto Check = [&](FName Id, ETDTeam Team)
	{
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(Id).Projectile;
		Spec.FX = nullptr; // no art: placeholder only
		ATDProjectile* P = ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, Team);
		if (!TestNotNull(Id.ToString() + TEXT(" spawned"), P))
		{
			return;
		}
		const FTDStreakLook Look = FTDCombatPlaceholders::ComputeStreak(Spec);
		const UStaticMeshComponent* Mesh = P->GetPlaceholderMesh();
		TestTrue(Id.ToString() + TEXT(" streak visible"), Mesh && Mesh->IsVisible());
		if (Mesh && Look.bStreak)
		{
			TestTrue(Id.ToString() + TEXT(" streak length matches"), FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X * 100.f, Look.Length, 1.f));
			TestTrue(Id.ToString() + TEXT(" streak trails behind the shot"), Mesh->GetRelativeLocation().X < 0.f);
		}
		const UStaticMeshComponent* Head = P->GetPlaceholderHead();
		TestEqual(Id.ToString() + TEXT(" head only on missiles"), Head && Head->IsVisible(), Look.bHead);
		P->Kill(false);
		TestFalse(Id.ToString() + TEXT(" streak hidden when pooled"), Mesh && Mesh->IsVisible());
		TestFalse(Id.ToString() + TEXT(" head hidden when pooled"), Head && Head->IsVisible());
	};
	for (const FName Id : TDCombatPlaceholderTest::PlayerWeaponIds)
	{
		Check(Id, ETDTeam::Player);
	}
	for (const FName Id : TDCombatPlaceholderTest::EnemyWeaponIds)
	{
		Check(Id, ETDTeam::Enemy);
	}

	// A pooled actor reused by a laser after a missile drops the missile's head.
	FTDProjectileSpec MissileSpec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile;
	MissileSpec.FX = nullptr;
	ATDProjectile* M = ATDProjectile::Spawn(W.GetWorld(), MissileSpec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, ETDTeam::Enemy);
	if (M)
	{
		M->Kill(false);
	}
	FTDProjectileSpec LaserSpec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
	LaserSpec.FX = nullptr;
	ATDProjectile* L = ATDProjectile::Spawn(W.GetWorld(), LaserSpec, FTransform(FVector(0.f, 0.f, 1000.f)), nullptr, ETDTeam::Player);
	TestTrue(TEXT("reused actor: no stale missile head"), L && L == M && !L->GetPlaceholderHead()->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPlaceholderFlashes, "TriadDrift.Combat.Placeholder.MuzzleAndImpactFlashes", TD_TEST_FLAGS)
bool FTDCombatPlaceholderFlashes::RunTest(const FString& Parameters)
{
	for (const float Step : TDCombatTest::kCombatFrameSteps)
	{
		const FString Fps = FString::Printf(TEXT(" @ %.0f fps"), 1.f / Step);
		FTDTestWorld W;
		UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
		if (!TestNotNull(TEXT("projectile subsystem"), Projectiles))
		{
			return false;
		}

		// Impact: a laser into a wall, no art.
		AActor* Wall = TDCombatTest::SpawnBody(W, ETDTeam::Enemy, FVector(1500.f, 0.f, 0.f), FVector(100.f, 600.f, 600.f), 1000.f, 0.f);
		FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers).Projectile;
		Spec.FX = nullptr;
		ATDProjectile::Spawn(W.GetWorld(), Spec, FTransform(FVector::ZeroVector), nullptr, ETDTeam::Player);
		bool bFlashed = false;
		for (float T = 0.f; T < 0.3f && !bFlashed; T += Step)
		{
			W.Tick(Step);
			bFlashed = Projectiles->GetPlayingFlashCount() > 0;
		}
		TestTrue(TEXT("impact flash plays") + Fps, bFlashed && Wall != nullptr);
		W.Advance(0.5f, Step);
		TestEqual(TEXT("impact flash is short and goes away") + Fps, Projectiles->GetPlayingFlashCount(), 0);

		// Muzzle: a ship-less shooter with no weapon art.
		AActor* Shooter = TDCombatTest::SpawnBody(W, ETDTeam::Player, FVector(0.f, 0.f, 5000.f), FVector(50.f));
		UTDWeaponComponent* Weapons = TDCombatTest::AddWeapons(Shooter);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::NeedleCannons);
		const UTDWeaponFX* FX = Weapons->PrimaryFX.Get();
		if (!FX || FX->Muzzle.IsEmpty())
		{
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
			bool bMuzzle = false;
			for (float T = 0.f; T < 0.2f && !bMuzzle; T += Step)
			{
				W.Tick(Step);
				bMuzzle = Projectiles->GetPlayingFlashCount() > 0;
			}
			TestTrue(TEXT("muzzle flash plays") + Fps, bMuzzle);
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false);
		}
	}

	// The pool is capped: a storm of flashes reuses actors instead of spawning without bound.
	FTDTestWorld W;
	UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(W.GetWorld());
	for (int32 I = 0; I < 200; ++I)
	{
		Projectiles->PlayPlaceholderFlash(FVector(I * 10.f, 0.f, 0.f), 100.f, FLinearColor::Red, 1.f);
	}
	TestTrue(TEXT("flash pool capped"), Projectiles->GetPlayingFlashCount() <= Projectiles->MaxFlashes);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
