// Automation tests for the enemy table (ENG-13). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEnemiesStats, "TriadDrift.Enemies.Library.StatsMatchFeatures", TD_TEST_FLAGS)
bool FTDEnemiesStats::RunTest(const FString& Parameters)
{
	using namespace TDEnemyIds;
	TestEqual(TEXT("five enemy types"), UTDEnemyLibrary::GetAllEnemyIds().Num(), 5);

	const FTDEnemySpec& F = UTDEnemyLibrary::Get(ShardFighter);
	TestEqual(TEXT("Fighter hull 30"), F.Hull, 30.f);
	TestEqual(TEXT("Fighter shield 0"), F.Shield, 0.f);
	TestEqual(TEXT("Fighter speed 7000"), F.CruiseSpeed, 7000.f);
	TestEqual(TEXT("Fighter pulse guns 5 dmg"), F.Guns.Damage, 5.f);
	TestEqual(TEXT("Fighter pulse guns 4/s"), 1.f / F.Guns.Interval, 4.f, 0.01f);
	TestTrue(TEXT("Fighter packs 4-8"), F.PackMin == 4 && F.PackMax == 8);
	TestEqual(TEXT("Fighter weapon id"), F.Guns.WeaponId, TDEnemyWeaponIds::PulseGuns);

	const FTDEnemySpec& I = UTDEnemyLibrary::Get(HookInterceptor);
	TestEqual(TEXT("Interceptor hull 50"), I.Hull, 50.f);
	TestEqual(TEXT("Interceptor shield 20"), I.Shield, 20.f);
	TestEqual(TEXT("Interceptor speed 9000"), I.CruiseSpeed, 9000.f);
	TestTrue(TEXT("Interceptor homing missile"), I.Heavy.Kind == ETDEnemyAttack::Missile && I.Heavy.WeaponId == TDEnemyWeaponIds::HomingMissile);
	TestEqual(TEXT("Interceptor missile 25 dmg"), I.Heavy.Damage, 25.f);
	TestEqual(TEXT("Interceptor missile every 8 s"), I.Heavy.Interval, 8.f);
	TestTrue(TEXT("Interceptor missile is telegraphed"), I.Heavy.IsTelegraphed());
	TestTrue(TEXT("Interceptor role hunts"), I.Role == ETDEnemyRole::Interceptor);

	const FTDEnemySpec& G = UTDEnemyLibrary::Get(WallGunship);
	TestEqual(TEXT("Gunship hull 400"), G.Hull, 400.f);
	TestEqual(TEXT("Gunship frontal shield 300"), G.FrontalShield, 300.f);
	TestEqual(TEXT("Gunship speed 3000"), G.CruiseSpeed, 3000.f);
	TestTrue(TEXT("Gunship heavy bolts 20"), G.Heavy.Kind == ETDEnemyAttack::HeavyBolt && G.Heavy.Damage == 20.f);
	TestTrue(TEXT("Gunship salvo 3 x 20"), G.Heavy2.Kind == ETDEnemyAttack::MissileSalvo && G.Heavy2.Count == 3 && G.Heavy2.Damage == 20.f);
	TestTrue(TEXT("Gunship attacks telegraphed"), G.Heavy.IsTelegraphed() && G.Heavy2.IsTelegraphed());

	const FTDEnemySpec& B = UTDEnemyLibrary::Get(MaulBomber);
	TestEqual(TEXT("Bomber hull 150"), B.Hull, 150.f);
	TestEqual(TEXT("Bomber shield 50"), B.Shield, 50.f);
	TestEqual(TEXT("Bomber speed 4000"), B.CruiseSpeed, 4000.f);
	TestTrue(TEXT("Bomber plasma mines"), B.Heavy.Kind == ETDEnemyAttack::MineTrail && B.Heavy.WeaponId == TDEnemyWeaponIds::PlasmaMines);
	TestEqual(TEXT("Bomber mine 60 dmg"), B.Heavy.Damage, 60.f);
	TestEqual(TEXT("Bomber mine 800 cm"), B.Heavy.SplashRadius, 800.f);

	const FTDEnemySpec& C = UTDEnemyLibrary::Get(HiveCarrier);
	TestEqual(TEXT("Carrier hull 2000"), C.Hull, 2000.f);
	TestEqual(TEXT("Carrier 3 bays"), C.BayCount, 3);
	TestEqual(TEXT("Carrier bay 300 HP"), C.BayHull, 300.f);
	TestEqual(TEXT("Carrier speed 1500"), C.CruiseSpeed, 1500.f);
	TestEqual(TEXT("Carrier 4 turrets"), C.TurretCount, 4);
	TestEqual(TEXT("Carrier turret 6 dmg"), C.TurretGuns.Damage, 6.f);
	TestEqual(TEXT("Carrier launches 3 per bay"), C.Heavy.Count, 3);
	TestEqual(TEXT("Carrier launches every 12 s"), C.Heavy.Interval, 12.f);
	TestEqual(TEXT("Carrier launches fighters"), C.LaunchedEnemyId, ShardFighter);
	TestTrue(TEXT("Carrier is a boss"), C.Size == ETDEnemySize::Boss && C.KillImportance > 0.f);

	// Death variety scales with size.
	TestTrue(TEXT("tumble grows with size"), F.DeathTumbleSeconds < B.DeathTumbleSeconds && B.DeathTumbleSeconds < G.DeathTumbleSeconds && G.DeathTumbleSeconds < C.DeathTumbleSeconds);

	// Unknown ids are safe.
	AddExpectedError(TEXT("unknown enemy id"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("unknown id -> invalid spec"), UTDEnemyLibrary::Get(TEXT("NoSuchShip")).IsValid());

	// Overrides and reset.
	FTDEnemySpec Tuned = F;
	Tuned.Hull = 45.f;
	UTDEnemyLibrary::OverrideSpec(Tuned);
	TestEqual(TEXT("override applies"), UTDEnemyLibrary::Get(ShardFighter).Hull, 45.f);
	UTDEnemyLibrary::ResetToDefaults();
	TestEqual(TEXT("reset restores"), UTDEnemyLibrary::Get(ShardFighter).Hull, 30.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
