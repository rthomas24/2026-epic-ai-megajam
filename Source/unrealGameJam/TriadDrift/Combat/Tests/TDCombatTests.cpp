// Triad Drift - Combat (Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/Tests/TDCombatTestHelpers.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"

#include "TriadDrift/Combat/Tests/TDCombatTestCommon.h"

namespace TDCombatTest
{


// ------------------------------------------------------------------------------------------ library data

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatLibraryNumbers, "TriadDrift.Combat.Library.WeaponNumbersMatchSpec", TD_TEST_FLAGS)
bool FTDCombatLibraryNumbers::RunTest(const FString& Parameters)
{
	for (const FName Id : UTDWeaponLibrary::GetAllIds())
	{
		TestTrue(*FString::Printf(TEXT("%s is valid"), *Id.ToString()), UTDWeaponLibrary::Get(Id).IsValid());
	}
	TestEqual(TEXT("12 weapons"), UTDWeaponLibrary::GetAllIds().Num(), 12);
	AddExpectedMessage(TEXT("unknown weapon id"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestFalse(TEXT("unknown id is invalid"), UTDWeaponLibrary::Get(TEXT("NoSuchGun")).IsValid());

	const FTDWeaponSpec& Phase = UTDWeaponLibrary::Get(TDWeaponIds::PhasePulseLasers);
	TestEqual(TEXT("Phase dmg"), Phase.Projectile.Damage, 8.f);
	TestEqual(TEXT("Phase rate"), Phase.FireRate, 8.f);
	TestEqual(TEXT("Phase speed"), Phase.Projectile.Speed, 40000.f);
	TestEqual(TEXT("Phase range"), Phase.Projectile.Range, 40000.f);
	TestEqual(TEXT("Phase heat"), Phase.HeatSecondsToOverheat, 4.f);
	TestEqual(TEXT("Phase lockout"), Phase.OverheatLockoutSeconds, 1.5f);
	TestEqual(TEXT("Phase aim assist"), Phase.AimAssistDeg, 3.f);
	TestTrue(TEXT("Phase alternates"), Phase.bAlternateMuzzles);

	const FTDWeaponSpec& Split = UTDWeaponLibrary::Get(TDWeaponIds::SplitterBolt);
	TestTrue(TEXT("Splitter is a charge shot"), Split.FireMode == ETDFireMode::Charge);
	TestEqual(TEXT("Splitter charge"), Split.ChargeSeconds, 0.8f);
	TestEqual(TEXT("Splitter dmg"), Split.Projectile.Damage, 40.f);
	TestEqual(TEXT("Splitter shards"), Split.Projectile.ShardCount, 5);
	TestEqual(TEXT("Splitter shard dmg"), Split.Projectile.ShardDamage, 12.f);
	TestEqual(TEXT("Splitter ammo"), Split.MaxAmmo, 6);
	TestEqual(TEXT("Splitter regen"), Split.AmmoRegenSeconds, 4.f);
	TestEqual(TEXT("Splitter speed"), Split.Projectile.Speed, 25000.f);

	const FTDWeaponSpec& Needle = UTDWeaponLibrary::Get(TDWeaponIds::NeedleCannons);
	TestEqual(TEXT("Needle dmg"), Needle.Projectile.Damage, 4.f);
	TestEqual(TEXT("Needle rate"), Needle.FireRate, 20.f);
	TestEqual(TEXT("Needle speed"), Needle.Projectile.Speed, 55000.f);
	TestEqual(TEXT("Needle range"), Needle.Projectile.Range, 30000.f);
	TestEqual(TEXT("Needle heat"), Needle.HeatSecondsToOverheat, 5.f);
	TestEqual(TEXT("Needle spread"), Needle.SpreadDeg, 1.2f);
	TestTrue(TEXT("Needle kinetic"), Needle.Projectile.DamageKind == ETDDamageKind::Kinetic);

	const FTDWeaponSpec& Spike = UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets);
	TestEqual(TEXT("Spike salvo"), Spike.ProjectilesPerShot, 4);
	TestEqual(TEXT("Spike dmg"), Spike.Projectile.Damage, 25.f);
	TestEqual(TEXT("Spike splash"), Spike.Projectile.SplashRadius, 300.f);
	TestEqual(TEXT("Spike refire"), Spike.GetRefireSeconds(), 1.5f, 0.001f);
	TestEqual(TEXT("Spike ammo"), Spike.MaxAmmo, 8);
	TestEqual(TEXT("Spike regen"), Spike.AmmoRegenSeconds, 5.f);

	const FTDWeaponSpec& Breaker = UTDWeaponLibrary::Get(TDWeaponIds::BreakerBlaster);
	TestEqual(TEXT("Breaker dmg"), Breaker.Projectile.Damage, 30.f);
	TestEqual(TEXT("Breaker splash"), Breaker.Projectile.SplashRadius, 250.f);
	TestEqual(TEXT("Breaker rate"), Breaker.FireRate, 2.5f);
	TestEqual(TEXT("Breaker heat"), Breaker.HeatSecondsToOverheat, 6.f);
	TestTrue(TEXT("Breaker knocks back"), Breaker.Projectile.KnockbackSpeed > 0.f);

	const FTDWeaponSpec& Flak = UTDWeaponLibrary::Get(TDWeaponIds::FlakBurst);
	TestEqual(TEXT("Flak dmg"), Flak.Projectile.Damage, 15.f);
	TestEqual(TEXT("Flak radius"), Flak.Projectile.SplashRadius, 800.f);
	TestEqual(TEXT("Flak refire"), Flak.GetRefireSeconds(), 1.2f, 0.001f);
	TestEqual(TEXT("Flak ammo"), Flak.MaxAmmo, 10);
	TestEqual(TEXT("Flak regen"), Flak.AmmoRegenSeconds, 3.f);
	TestTrue(TEXT("Flak shoots down missiles"), Flak.Projectile.bShootsDownMissiles);

	TestEqual(TEXT("Enemy pulse dmg"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyPulseGuns).Projectile.Damage, 5.f);
	TestEqual(TEXT("Enemy pulse rate"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyPulseGuns).FireRate, 4.f);
	TestEqual(TEXT("Enemy missile dmg"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile.Damage, 25.f);
	TestTrue(TEXT("Enemy missile homes"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile.bHoming);
	TestEqual(TEXT("Heavy bolt dmg"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile.Damage, 20.f);
	TestEqual(TEXT("Salvo count"), UTDWeaponLibrary::Get(TDWeaponIds::EnemySalvoMissile).ProjectilesPerShot, 3);
	TestEqual(TEXT("Salvo dmg"), UTDWeaponLibrary::Get(TDWeaponIds::EnemySalvoMissile).Projectile.Damage, 20.f);
	TestEqual(TEXT("Mine dmg"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyPlasmaMines).Projectile.Damage, 60.f);
	TestEqual(TEXT("Mine radius"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyPlasmaMines).Projectile.SplashRadius, 800.f);
	TestEqual(TEXT("PD dmg"), UTDWeaponLibrary::Get(TDWeaponIds::EnemyPointDefense).Projectile.Damage, 6.f);
	return true;
}

// ------------------------------------------------------------------------------------------ fire rate / heat / ammo

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatFireRate, "TriadDrift.Combat.Weapon.FireRateFrameRateIndependent", TD_TEST_FLAGS)
bool FTDCombatFireRate::RunTest(const FString& Parameters)
{
	// The first shot fires the moment the trigger goes down (t = 0), then one every 1/rate. Measure over
	// 31/30 s (a whole number of frames at 30/60/120 fps) so no shot sits on the window edge:
	// shots = floor(T * rate) + 1. 8/s -> 9, 20/s -> 21, 2.5/s -> 3.
	const float Window = 31.f / 30.f;
	struct FCase { FName Id; float Rate; };
	const FCase Cases[] = { { TDWeaponIds::PhasePulseLasers, 8.f }, { TDWeaponIds::NeedleCannons, 20.f }, { TDWeaponIds::BreakerBlaster, 2.5f } };
	for (const FCase& C : Cases)
	{
		const int32 Expected = FMath::FloorToInt(Window * C.Rate) + 1;
		for (const float Step : { 1.f / 30.f, 1.f / 60.f, 1.f / 120.f })
		{
			FTDTestWorld W;
			AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
			UTDWeaponComponent* Weapons = AddWeapons(Ship);
			Weapons->EquipWeapon(ETDWeaponSlot::Primary, C.Id);
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
			W.Advance(Window, Step);
			TestEqual(*FString::Printf(TEXT("%s: %d shots in %.3f s at %.0f fps"), *C.Id.ToString(), Expected, Window, 1.f / Step),
				Weapons->GetSlotState(ETDWeaponSlot::Primary).ShotsFired, Expected);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatOverheat, "TriadDrift.Combat.Weapon.OverheatAndLockoutTiming", TD_TEST_FLAGS)
bool FTDCombatOverheat::RunTest(const FString& Parameters)
{
	struct FCase { FName Id; float HeatSeconds; };
	const FCase Cases[] = { { TDWeaponIds::PhasePulseLasers, 4.f }, { TDWeaponIds::NeedleCannons, 5.f }, { TDWeaponIds::BreakerBlaster, 6.f } };
	for (const FCase& C : Cases)
	{
		FTDTestWorld W;
		AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = AddWeapons(Ship);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, C.Id);
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);

		const float OverheatAt = AdvanceUntil(W, [&] { return Weapons->IsOverheated(ETDWeaponSlot::Primary); }, 10.f);
		TestEqual(*FString::Printf(TEXT("%s overheats after %.0f s"), *C.Id.ToString(), C.HeatSeconds), OverheatAt, C.HeatSeconds, 0.1f);
		TestEqual(TEXT("heat is full"), Weapons->GetHeat01(ETDWeaponSlot::Primary), 1.f, 0.01f);

		const int32 ShotsAtOverheat = Weapons->GetSlotState(ETDWeaponSlot::Primary).ShotsFired;
		const float LockedFor = AdvanceUntil(W, [&] { return !Weapons->IsOverheated(ETDWeaponSlot::Primary); }, 5.f);
		TestEqual(TEXT("1.5 s lockout"), LockedFor, 1.5f, 0.05f);
		const int32 ShotsAfterLockout = Weapons->GetSlotState(ETDWeaponSlot::Primary).ShotsFired;
		TestTrue(TEXT("no shots during lockout (at most the one on release)"), ShotsAfterLockout - ShotsAtOverheat <= 1);
		W.Advance(0.5f);
		TestTrue(TEXT("fires again after lockout"), Weapons->GetSlotState(ETDWeaponSlot::Primary).ShotsFired > ShotsAfterLockout);
	}

	// Releasing the trigger cools the gun.
	{
		FTDTestWorld W;
		AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = AddWeapons(Ship);
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
		W.Advance(2.f);
		const float Hot = Weapons->GetHeat01(ETDWeaponSlot::Primary);
		TestTrue(TEXT("half heat after 2 s"), Hot > 0.4f && Hot < 0.6f);
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false);
		W.Advance(3.f);
		TestEqual(TEXT("cooled to zero"), Weapons->GetHeat01(ETDWeaponSlot::Primary), 0.f, 0.001f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatAmmo, "TriadDrift.Combat.Weapon.AmmoAndRegen", TD_TEST_FLAGS)
bool FTDCombatAmmo::RunTest(const FString& Parameters)
{
	struct FCase { FName Id; int32 Max; float Regen; };
	const FCase Cases[] = { { TDWeaponIds::SplitterBolt, 6, 4.f }, { TDWeaponIds::SpikeRockets, 8, 5.f }, { TDWeaponIds::FlakBurst, 10, 3.f } };
	for (const FCase& C : Cases)
	{
		FTDTestWorld W;
		AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = AddWeapons(Ship);
		Weapons->EquipWeapon(ETDWeaponSlot::Secondary, C.Id);
		TestEqual(*FString::Printf(TEXT("%s starts full"), *C.Id.ToString()), Weapons->GetAmmo(ETDWeaponSlot::Secondary), C.Max);

		int32 Fired = 0;
		while (Weapons->FireOnce(ETDWeaponSlot::Secondary) && Fired < 100)
		{
			++Fired;
		}
		TestEqual(TEXT("fires exactly Max rounds"), Fired, C.Max);
		TestEqual(TEXT("empty"), Weapons->GetAmmo(ETDWeaponSlot::Secondary), 0);
		TestFalse(TEXT("cannot fire empty"), Weapons->CanFire(ETDWeaponSlot::Secondary));

		W.Advance(C.Regen - 0.1f);
		TestEqual(TEXT("not yet regenerated"), Weapons->GetAmmo(ETDWeaponSlot::Secondary), 0);
		W.Advance(0.2f);
		TestEqual(TEXT("+1 after one regen period"), Weapons->GetAmmo(ETDWeaponSlot::Secondary), 1);
		W.Advance(C.Regen);
		TestEqual(TEXT("+2 after two"), Weapons->GetAmmo(ETDWeaponSlot::Secondary), 2);
	}

	// Spike Rockets: one salvo = 4 rockets, rippled.
	{
		FTDTestWorld W;
		AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
		UTDWeaponComponent* Weapons = AddWeapons(Ship);
		Weapons->EquipWeapon(ETDWeaponSlot::Secondary, TDWeaponIds::SpikeRockets);
		Weapons->FireOnce(ETDWeaponSlot::Secondary);
		W.Advance(0.3f);
		TestEqual(TEXT("salvo of 4 rockets in flight"), CountLive(W.GetWorld(), [](const ATDProjectile& P) { return P.GetSpec().Damage == 25.f; }), 4);
		TestEqual(TEXT("one salvo of ammo used"), Weapons->GetAmmo(ETDWeaponSlot::Secondary), 7);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatCharge, "TriadDrift.Combat.Weapon.ChargeShotNeedsFullCharge", TD_TEST_FLAGS)
bool FTDCombatCharge::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponComponent* Weapons = AddWeapons(Ship);
	Weapons->EquipWeapon(ETDWeaponSlot::Secondary, TDWeaponIds::SplitterBolt);

	Weapons->SetTriggerHeld(ETDWeaponSlot::Secondary, true);
	W.Advance(0.4f);
	TestEqual(TEXT("half charged"), Weapons->GetCharge01(ETDWeaponSlot::Secondary), 0.5f, 0.05f);
	W.Advance(0.1f);
	Weapons->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
	TestEqual(TEXT("early release does not fire"), Weapons->GetSlotState(ETDWeaponSlot::Secondary).ShotsFired, 0);
	TestEqual(TEXT("early release keeps ammo"), Weapons->GetAmmo(ETDWeaponSlot::Secondary), 6);
	TestEqual(TEXT("charge reset"), Weapons->GetCharge01(ETDWeaponSlot::Secondary), 0.f);

	Weapons->SetTriggerHeld(ETDWeaponSlot::Secondary, true);
	W.Advance(0.85f);
	TestEqual(TEXT("fully charged"), Weapons->GetCharge01(ETDWeaponSlot::Secondary), 1.f, 0.001f);
	TestEqual(TEXT("holding does not fire"), Weapons->GetSlotState(ETDWeaponSlot::Secondary).ShotsFired, 0);
	Weapons->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
	TestEqual(TEXT("release fires"), Weapons->GetSlotState(ETDWeaponSlot::Secondary).ShotsFired, 1);
	TestEqual(TEXT("one ammo used"), Weapons->GetAmmo(ETDWeaponSlot::Secondary), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatBlocked, "TriadDrift.Combat.Weapon.BlockedAndMultipliers", TD_TEST_FLAGS)
bool FTDCombatBlocked::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDWeaponComponent* Weapons = AddWeapons(Ship);
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);
	Weapons->SetWeaponsBlocked(TEXT("EMP"), true);
	Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
	W.Advance(0.5f);
	TestEqual(TEXT("blocked weapons do not fire"), Weapons->GetSlotState(ETDWeaponSlot::Primary).ShotsFired, 0);
	TestFalse(TEXT("FireOnce refused while blocked"), Weapons->FireOnce(ETDWeaponSlot::Primary));
	Weapons->SetWeaponsBlocked(TEXT("EMP"), false);

	Weapons->SetOutgoingDamageMultiplier(TEXT("SwitchStrike"), 1.25f);
	Weapons->SetOutgoingDamageMultiplier(TEXT("Test"), 2.f);
	TestEqual(TEXT("multipliers stack"), Weapons->GetOutgoingDamageMultiplier(), 2.5f, 0.001f);
	Weapons->ClearOutgoingDamageMultiplier(TEXT("Test"));
	TestEqual(TEXT("clear removes one source"), Weapons->GetOutgoingDamageMultiplier(), 1.25f, 0.001f);

	Weapons->SetNextShotDamageMultiplier(3.f);
	Weapons->FireOnce(ETDWeaponSlot::Primary);
	const ATDProjectile* First = Weapons->GetLastFiredProjectile(0);
	TestTrue(TEXT("next-shot multiplier applied"), First && FMath::IsNearlyEqual(First->GetDamageMultiplier(), 3.75f, 0.001f));
	W.Advance(0.2f);
	Weapons->FireOnce(ETDWeaponSlot::Primary);
	const ATDProjectile* Second = Weapons->GetLastFiredProjectile(0);
	TestTrue(TEXT("next-shot multiplier used once"), Second && FMath::IsNearlyEqual(Second->GetDamageMultiplier(), 1.25f, 0.001f));

	TestFalse(TEXT("empty slot cannot fire"), Weapons->FireOnce(ETDWeaponSlot::Secondary));
	return true;
}

// ------------------------------------------------------------------------------------------ projectiles

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatDamageFriendlyFire, "TriadDrift.Combat.Projectile.DamageAndNoFriendlyFire", TD_TEST_FLAGS)
bool FTDCombatDamageFriendlyFire::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	AActor* Wingman = SpawnBody(W, ETDTeam::Player, FVector(2000.f, 0.f, 0.f), FVector(100.f, 200.f, 200.f));
	AActor* Enemy = SpawnBody(W, ETDTeam::Enemy, FVector(4000.f, 0.f, 0.f), FVector(100.f, 200.f, 200.f));
	UTDWeaponComponent* Weapons = AddWeapons(Ship);
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::PhasePulseLasers);

	TestTrue(TEXT("fired"), Weapons->FireOnce(ETDWeaponSlot::Primary));
	W.Advance(0.3f);
	TestEqual(TEXT("wingman untouched (friendly fire off, shot passes through)"), Total(Wingman), 200.f);
	TestEqual(TEXT("enemy takes 8 to the shield"), Shield(Enemy), 92.f, 0.01f);
	TestEqual(TEXT("enemy hull untouched"), Hull(Enemy), 100.f);
	TestEqual(TEXT("projectile returned to the pool"), CountLive(W.GetWorld(), [](const ATDProjectile&) { return true; }), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatSplash, "TriadDrift.Combat.Projectile.SplashRadius", TD_TEST_FLAGS)
bool FTDCombatSplash::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Shooter = SpawnBody(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
	AActor* A = SpawnBody(W, ETDTeam::Enemy, FVector(1000.f, 0.f, 0.f), FVector(50.f), 100.f, 0.f);
	// Rocket stops with its 50 cm sphere touching A's face: detonation at x = 900.
	AActor* Inside = SpawnBody(W, ETDTeam::Enemy, FVector(900.f, 340.f, 0.f), FVector(50.f), 100.f, 0.f);  // bounds 290 cm away
	AActor* Outside = SpawnBody(W, ETDTeam::Enemy, FVector(900.f, -410.f, 0.f), FVector(50.f), 100.f, 0.f); // bounds 360 cm away
	AActor* Friendly = SpawnBody(W, ETDTeam::Player, FVector(900.f, 0.f, 200.f), FVector(50.f), 100.f, 0.f);

	const FTDProjectileSpec& Spec = UTDWeaponLibrary::Get(TDWeaponIds::SpikeRockets).Projectile;
	ATDProjectile::Spawn(W.GetWorld(), Spec, Facing(FVector::ZeroVector, FVector::ForwardVector), Shooter, ETDTeam::Player);
	W.Advance(0.2f);

	TestEqual(TEXT("direct hit 25"), Hull(A), 75.f, 0.01f);
	TestEqual(TEXT("inside 300 cm splash takes 25"), Hull(Inside), 75.f, 0.01f);
	TestEqual(TEXT("outside splash untouched"), Hull(Outside), 100.f);
	TestEqual(TEXT("friendly in splash untouched"), Hull(Friendly), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatShards, "TriadDrift.Combat.Projectile.SplitterSplitsIntoFiveShards", TD_TEST_FLAGS)
bool FTDCombatShards::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Shooter = SpawnBody(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
	AActor* A = SpawnBody(W, ETDTeam::Enemy, FVector(1500.f, 0.f, 0.f), FVector(50.f), 200.f, 0.f);
	// A swarm behind A for the shards to hit: a wide wall 1500 cm further on.
	AActor* Behind = SpawnBody(W, ETDTeam::Enemy, FVector(3000.f, 0.f, 0.f), FVector(50.f, 1500.f, 1500.f), 1000.f, 0.f);

	const FTDProjectileSpec& Spec = UTDWeaponLibrary::Get(TDWeaponIds::SplitterBolt).Projectile;
	ATDProjectile::Spawn(W.GetWorld(), Spec, Facing(FVector::ZeroVector, FVector::ForwardVector), Shooter, ETDTeam::Player);

	const float HitAt = AdvanceUntil(W, [&] { return Hull(A) < 200.f; }, 1.f);
	TestTrue(TEXT("bolt hit A"), HitAt > 0.f);
	TestEqual(TEXT("bolt deals 40"), Hull(A), 160.f, 0.01f);
	TestEqual(TEXT("5 shards spawned"), CountLive(W.GetWorld(), [](const ATDProjectile& P) { return P.GetSpec().Damage == 12.f; }), 5);

	W.Advance(0.5f);
	TestEqual(TEXT("shards do not re-hit A"), Hull(A), 160.f, 0.01f);
	TestEqual(TEXT("5 shards x 12 hit the swarm behind"), Hull(Behind), 1000.f - 60.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatFlakVsMissile, "TriadDrift.Combat.Projectile.FlakDestroysMissile", TD_TEST_FLAGS)
bool FTDCombatFlakVsMissile::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Bastion = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	AActor* Interceptor = SpawnBody(W, ETDTeam::Enemy, FVector(9000.f, 0.f, 0.f), FVector(50.f));

	ATDProjectile* Missile = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile,
		Facing(FVector(6000.f, 0.f, 300.f), -FVector::ForwardVector), Interceptor, ETDTeam::Enemy, Bastion);
	TestNotNull(TEXT("missile launched"), Missile);
	TestTrue(TEXT("missile homes on Bastion"), Missile && Missile->GetHomingTarget() == Bastion);

	UTDWeaponComponent* Weapons = AddWeapons(Bastion);
	Weapons->EquipWeapon(ETDWeaponSlot::Secondary, TDWeaponIds::FlakBurst);
	Weapons->SetAimDirection(FVector::ForwardVector);
	Weapons->FireOnce(ETDWeaponSlot::Secondary);

	W.Advance(0.6f);
	TestFalse(TEXT("missile shot down"), Missile && Missile->IsActive());
	TestEqual(TEXT("Bastion unharmed"), Total(Bastion), 200.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatHoming, "TriadDrift.Combat.Projectile.HomingAcquiresAndHits", TD_TEST_FLAGS)
bool FTDCombatHoming::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Interceptor = SpawnBody(W, ETDTeam::Enemy, FVector(-300.f, 0.f, 0.f), FVector(50.f));
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector(6000.f, 1000.f, 0.f), FVector(100.f)); // 9.5° off the nose
	AActor* Decoy = SpawnBody(W, ETDTeam::Player, FVector(-3000.f, 0.f, 0.f), FVector(100.f));  // behind: outside the cone

	ATDProjectile* Missile = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile,
		Facing(FVector::ZeroVector, FVector::ForwardVector), Interceptor, ETDTeam::Enemy);
	TestTrue(TEXT("acquired the target in its cone"), Missile && Missile->GetHomingTarget() == Rift);

	W.Advance(1.5f);
	TestEqual(TEXT("missile hit for 25"), Shield(Rift), 75.f, 0.01f);
	TestEqual(TEXT("decoy untouched"), Total(Decoy), 200.f);

	// Cloak: an untargetable target stops being tracked.
	ATDProjectile* Second = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile,
		Facing(FVector::ZeroVector, FVector::ForwardVector), Interceptor, ETDTeam::Enemy, Rift);
	UTDStatics::GetHealth(Rift)->SetTargetable(false);
	W.Advance(0.5f);
	TestTrue(TEXT("lost tracking on cloak"), Second && Second->GetHomingTarget() == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatDeflect, "TriadDrift.Combat.Projectile.DeflectReturnsToShooter", TD_TEST_FLAGS)
bool FTDCombatDeflect::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Gunship = SpawnBody(W, ETDTeam::Enemy, FVector::ZeroVector, FVector(150.f));
	AActor* Rift = SpawnBody(W, ETDTeam::Player, FVector(4000.f, 600.f, 0.f), FVector(100.f));

	ATDProjectile* Bolt = ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHeavyBolt).Projectile,
		Facing(FVector(300.f, 0.f, 0.f), FVector::ForwardVector), Gunship, ETDTeam::Enemy);
	W.Advance(0.2f);
	TestTrue(TEXT("bolt in flight"), Bolt && Bolt->IsActive());

	// Blade / portal sends it back slightly off-line: re-homing must still bring it to the gunship.
	Bolt->Deflect(Rift, FVector(-1.f, 0.3f, 0.f), ETDTeam::Player, 1.5f);
	TestTrue(TEXT("team flipped"), Bolt->GetTeam() == ETDTeam::Player);
	TestTrue(TEXT("re-homes on the original shooter"), Bolt->GetHomingTarget() == Gunship);
	TestTrue(TEXT("marked deflected"), Bolt->WasDeflected());

	W.Advance(2.f);
	TestEqual(TEXT("gunship hit by its own bolt"), Shield(Gunship), 80.f, 0.01f);
	TestEqual(TEXT("deflector untouched"), Total(Rift), 200.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatNoTunnelling, "TriadDrift.Combat.Projectile.NoTunnellingAtMaxSpeed", TD_TEST_FLAGS)
bool FTDCombatNoTunnelling::RunTest(const FString& Parameters)
{
	for (const float Step : { 1.f / 30.f, 1.f / 120.f })
	{
		FTDTestWorld W;
		AActor* Shooter = SpawnBody(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
		// 4 cm thick plate; a 55 000 cm/s needle moves 1833 cm per frame at 30 fps.
		AActor* Plate = SpawnBody(W, ETDTeam::Enemy, FVector(17777.f, 0.f, 0.f), FVector(2.f, 200.f, 200.f), 100.f, 0.f);
		const FTDProjectileSpec& Spec = UTDWeaponLibrary::Get(TDWeaponIds::NeedleCannons).Projectile;
		ATDProjectile::Spawn(W.GetWorld(), Spec, Facing(FVector::ZeroVector, FVector::ForwardVector), Shooter, ETDTeam::Player);
		W.Advance(0.6f, Step);
		TestEqual(*FString::Printf(TEXT("thin plate hit at %.0f fps"), 1.f / Step), Hull(Plate), 96.f, 0.01f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatHitConfirm, "TriadDrift.Combat.Projectile.HitConfirmAndKillHitStop", TD_TEST_FLAGS)
bool FTDCombatHitConfirm::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	AActor* Fighter = SpawnBody(W, ETDTeam::Enemy, FVector(2000.f, 0.f, 0.f), FVector(150.f), 10.f, 0.f);
	UTDWeaponComponent* Weapons = AddWeapons(Ship);
	Weapons->bForceReportHits = true;
	Weapons->EquipWeapon(ETDWeaponSlot::Primary, TDWeaponIds::BreakerBlaster);

	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	TestNotNull(TEXT("event bus"), Bus);
	UTDTestHitListener* Listener = NewObject<UTDTestHitListener>();
	Listener->AddToRoot();
	if (Bus)
	{
		Bus->OnHitConfirmed.AddDynamic(Listener, &UTDTestHitListener::OnHit);
	}

	Weapons->FireOnce(ETDWeaponSlot::Primary);
	UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	const float KilledAt = AdvanceUntil(W, [&] { return UTDStatics::GetHealth(Fighter)->IsDead(); }, 1.f);
	TestTrue(TEXT("fighter killed"), KilledAt > 0.f);
	TestEqual(TEXT("one hit confirmed"), Listener->Hits, 1);
	TestEqual(TEXT("one kill confirmed"), Listener->Kills, 1);
	TestTrue(TEXT("kill triggered hit-stop"), Feel && Feel->GetCurrentDilation() < 1.f);

	if (Bus)
	{
		Bus->OnHitConfirmed.RemoveAll(Listener);
	}
	Listener->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatKnockback, "TriadDrift.Combat.Projectile.BreakerKnocksBackFighters", TD_TEST_FLAGS)
bool FTDCombatKnockback::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Shooter = SpawnBody(W, ETDTeam::Player, FVector(-500.f, 0.f, 0.f), FVector(50.f));
	AActor* Fighter = SpawnBody(W, ETDTeam::Enemy, FVector(1500.f, 0.f, 0.f), FVector(100.f), 500.f, 0.f);
	UProjectileMovementComponent* Move = NewObject<UProjectileMovementComponent>(Fighter, TEXT("Move"));
	Move->ProjectileGravityScale = 0.f;
	Move->bAutoActivate = true;
	Move->RegisterComponent();
	Move->Velocity = FVector::ZeroVector;
	AActor* Gunship = SpawnBody(W, ETDTeam::Enemy, FVector(1500.f, 3000.f, 0.f), FVector(800.f), 500.f, 0.f);
	UProjectileMovementComponent* BigMove = NewObject<UProjectileMovementComponent>(Gunship, TEXT("Move"));
	BigMove->ProjectileGravityScale = 0.f;
	BigMove->RegisterComponent();
	BigMove->Velocity = FVector::ZeroVector;

	ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::BreakerBlaster).Projectile,
		Facing(FVector::ZeroVector, FVector::ForwardVector), Shooter, ETDTeam::Player);
	W.Advance(0.1f);
	TestEqual(TEXT("fighter pushed 1500 cm/s along the shot"), (float)Move->Velocity.X, 1500.f, 1.f);

	ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::BreakerBlaster).Projectile,
		Facing(FVector(0.f, 3000.f, 0.f), FVector::ForwardVector), Shooter, ETDTeam::Player);
	W.Advance(0.1f);
	TestTrue(TEXT("gunship was hit"), Hull(Gunship) < 500.f);
	TestEqual(TEXT("big ships are not knocked back"), (float)BigMove->Velocity.Size(), 0.f, 0.01f);
	return true;
}

// ------------------------------------------------------------------------------------------ targeting

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatTargetCone, "TriadDrift.Combat.Targeting.SoftTargetCone", TD_TEST_FLAGS)
bool FTDCombatTargetCone::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = AddTargeting(Ship);

	auto AtAngle = [](float Deg, float Dist) { return FVector(FMath::Cos(FMath::DegreesToRadians(Deg)), FMath::Sin(FMath::DegreesToRadians(Deg)), 0.f) * Dist; };
	AActor* At20 = SpawnBody(W, ETDTeam::Enemy, AtAngle(20.f, 10000.f));
	AActor* At30 = SpawnBody(W, ETDTeam::Enemy, AtAngle(30.f, 5000.f));
	SpawnBody(W, ETDTeam::Player, AtAngle(2.f, 6000.f)); // friendly dead centre: never a target

	Targeting->RefreshNow();
	TestTrue(TEXT("20° enemy is the soft target"), Targeting->GetCurrentTarget() == At20);

	AActor* At5 = SpawnBody(W, ETDTeam::Enemy, AtAngle(5.f, 20000.f));
	AActor* At15Near = SpawnBody(W, ETDTeam::Enemy, AtAngle(15.f, 3000.f));
	UTDStatics::GetHealth(At20)->Kill();
	Targeting->RefreshNow();
	TestTrue(TEXT("closest to screen centre wins over nearest"), Targeting->GetCurrentTarget() == At5);

	UTDStatics::GetHealth(At5)->Kill();
	UTDStatics::GetHealth(At15Near)->Kill();
	Targeting->RefreshNow();
	TestNull(TEXT("30° is outside the 25° cone"), Targeting->GetCurrentTarget());
	TestNotNull(TEXT("still marked on the HUD"), Targeting->GetMarkers().FindByPredicate([At30](const FTDTargetMarker& M) { return M.Actor.Get() == At30; }));

	SpawnBody(W, ETDTeam::Enemy, AtAngle(0.f, 60000.f));
	Targeting->RefreshNow();
	TestNull(TEXT("beyond max range is ignored"), Targeting->GetCurrentTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatTargetCycle, "TriadDrift.Combat.Targeting.CycleByDistance", TD_TEST_FLAGS)
bool FTDCombatTargetCycle::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = AddTargeting(Ship);
	AActor* Far = SpawnBody(W, ETDTeam::Enemy, FVector(9000.f, 0.f, 0.f));
	AActor* Near = SpawnBody(W, ETDTeam::Enemy, FVector(2000.f, 1500.f, 0.f));
	AActor* Mid = SpawnBody(W, ETDTeam::Enemy, FVector(5000.f, -2000.f, 0.f));
	SpawnBody(W, ETDTeam::Enemy, FVector(-4000.f, 0.f, 0.f)); // behind: outside the cycle cone

	Targeting->CycleTarget();
	TestTrue(TEXT("cycle 1 = nearest"), Targeting->GetCurrentTarget() == Near);
	Targeting->CycleTarget();
	TestTrue(TEXT("cycle 2 = next by distance"), Targeting->GetCurrentTarget() == Mid);
	Targeting->CycleTarget();
	TestTrue(TEXT("cycle 3 = farthest"), Targeting->GetCurrentTarget() == Far);
	Targeting->CycleTarget();
	TestTrue(TEXT("wraps to nearest"), Targeting->GetCurrentTarget() == Near);

	W.Advance(0.5f);
	TestTrue(TEXT("manual target is held over the soft target"), Targeting->GetCurrentTarget() == Near);
	UTDStatics::GetHealth(Near)->Kill();
	W.Advance(0.1f);
	TestTrue(TEXT("dead manual target falls back to the soft target"), Targeting->GetCurrentTarget() == Far);
	return true;
}

// A shaking view (the Audition chase camera's sustained shake) over two targets tied for "nearest the centre" must not
// restart the paint every frame: four targets in the crosshair still lock at the 0.25 s cadence (gating run, lead/int
// 173a64f: 2 locks in 2 s instead of 3+).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatPaintUnderShake, "TriadDrift.Combat.Targeting.PaintKeepsItsCandidateUnderShake", TD_TEST_FLAGS)
bool FTDCombatPaintUnderShake::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = AddTargeting(Ship);
	for (int32 i = 0; i < 4; ++i)
	{
		SpawnBody(W, ETDTeam::Enemy, FVector(20000.f, (i - 1.5f) * 700.f, 0.f)); // the two middle ones tie at the centre
	}
	Targeting->BeginLockPainting();
	// The view jitters a tenth of a degree left and right every frame: the nearest of the tied pair alternates.
	for (int32 Frame = 0; Frame < 30; ++Frame) // 1 s at 30 fps
	{
		Targeting->SetViewOverride(FVector::ZeroVector, FRotator(0.f, (Frame % 2) ? .1f : -.1f, 0.f));
		W.Tick(1.f / 30.f);
	}
	TestTrue(FString::Printf(TEXT("4 locks in 1 s under shake (got %d)"), Targeting->GetLockedTargets().Num()), Targeting->GetLockedTargets().Num() >= 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatMultiLock, "TriadDrift.Combat.Targeting.MultiLockSixAtQuarterSecond", TD_TEST_FLAGS)
bool FTDCombatMultiLock::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	UTDTargetingComponent* Targeting = AddTargeting(Ship);
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnBody(W, ETDTeam::Enemy, FVector(8000.f, -2400.f + i * 700.f, (i % 2) * 400.f));
	}

	Targeting->BeginLockPainting();
	W.Advance(0.3f);
	TestEqual(TEXT("1 lock after 0.3 s"), Targeting->GetLockedTargets().Num(), 1);
	W.Advance(0.25f);
	TestEqual(TEXT("2 locks after 0.55 s"), Targeting->GetLockedTargets().Num(), 2);
	W.Advance(1.0f);
	TestEqual(TEXT("6 locks after 1.55 s"), Targeting->GetLockedTargets().Num(), 6);
	W.Advance(1.0f);
	TestEqual(TEXT("capped at 6"), Targeting->GetLockedTargets().Num(), 6);

	const TArray<AActor*> Locks = Targeting->EndLockPainting();
	TSet<AActor*> Unique(Locks);
	TestEqual(TEXT("6 distinct targets"), Unique.Num(), 6);

	UTDStatics::GetHealth(Locks[0])->Kill();
	W.Tick();
	TestEqual(TEXT("dead locks are dropped"), Targeting->GetLockedTargets().Num(), 5);
	Targeting->ClearLocks();
	TestEqual(TEXT("cleared"), Targeting->GetLockedTargets().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCombatLead, "TriadDrift.Combat.Targeting.LeadInterceptAndMissileWarning", TD_TEST_FLAGS)
bool FTDCombatLead::RunTest(const FString& Parameters)
{
	// Target 10 000 cm ahead crossing at 1000 cm/s; shot at 10 000 cm/s. t = sqrt(1e8 / 0.99e8).
	const FVector Lead = UTDTargetingComponent::ComputeInterceptPoint(FVector::ZeroVector, FVector::ZeroVector, FVector(10000.f, 0.f, 0.f),
		FVector(0.f, 1000.f, 0.f), 10000.f);
	const float T = FMath::Sqrt(1.f / 0.99f);
	TestEqual(TEXT("lead X"), (float)Lead.X, 10000.f, 1.f);
	TestEqual(TEXT("lead Y"), (float)Lead.Y, 1000.f * T, 1.f);
	TestEqual(TEXT("projectile reaches the lead point at t"), (float)(Lead.Size() / 10000.0), T, 0.001f);
	TestEqual(TEXT("unreachable target falls back to its location"),
		UTDTargetingComponent::ComputeInterceptPoint(FVector::ZeroVector, FVector::ZeroVector, FVector(1000.f, 0.f, 0.f), FVector(20000.f, 0.f, 0.f), 10000.f),
		FVector(1000.f, 0.f, 0.f));

	// Incoming-missile warning.
	FTDTestWorld W;
	AActor* Ship = SpawnBody(W, ETDTeam::Player, FVector::ZeroVector, FVector(50.f));
	AActor* Interceptor = SpawnBody(W, ETDTeam::Enemy, FVector(30000.f, 0.f, 0.f), FVector(50.f));
	UTDTargetingComponent* Targeting = AddTargeting(Ship);
	Targeting->RefreshNow();
	TestTrue(TEXT("no warning"), Targeting->GetMissileWarning() == ETDMissileWarning::None);

	ATDProjectile::Spawn(W.GetWorld(), UTDWeaponLibrary::Get(TDWeaponIds::EnemyHomingMissile).Projectile,
		Facing(FVector(25000.f, 0.f, 0.f), -FVector::ForwardVector), Interceptor, ETDTeam::Enemy, Ship);
	Targeting->RefreshNow();
	TestTrue(TEXT("tracking warning"), Targeting->GetMissileWarning() == ETDMissileWarning::Tracking);
	TestEqual(TEXT("one incoming"), Targeting->GetIncomingMissileCount(), 1);
	TestTrue(TEXT("points at the missile"), FVector::DotProduct(Targeting->GetNearestMissileDirection(), FVector::ForwardVector) > 0.99f);

	const float ImminentAt = AdvanceUntil(W, [&] { return Targeting->GetMissileWarning() == ETDMissileWarning::Imminent; }, 4.f);
	TestTrue(TEXT("becomes imminent before impact"), ImminentAt > 0.f && UTDStatics::GetHealth(Ship)->GetShield() == 100.f);
	return true;
}

} // namespace TDCombatTest

#endif // WITH_DEV_AUTOMATION_TESTS
