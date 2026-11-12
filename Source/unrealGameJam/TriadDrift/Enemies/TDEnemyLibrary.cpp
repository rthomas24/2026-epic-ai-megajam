// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Core/TDTypes.h"

#define LOCTEXT_NAMESPACE "TDEnemyLibrary"

namespace
{
	FTDEnemyAttackSpec TDEnemyLib_MakeAttack(ETDEnemyAttack Kind, FName WeaponId, float Damage, int32 Count, float Interval,
		float Telegraph, float Range, float Cone, float Splash = 0.f)
	{
		FTDEnemyAttackSpec A;
		A.Kind = Kind;
		A.WeaponId = WeaponId;
		A.Damage = Damage;
		A.Count = Count;
		A.Interval = Interval;
		A.TelegraphSeconds = Telegraph;
		A.Range = Range;
		A.ConeHalfAngleDeg = Cone;
		A.SplashRadius = Splash;
		return A;
	}
}

TMap<FName, FTDEnemySpec>& UTDEnemyLibrary::Table()
{
	static TMap<FName, FTDEnemySpec> Specs;
	if (Specs.Num() == 0)
	{
		BuildDefaults(Specs);
	}
	return Specs;
}

void UTDEnemyLibrary::BuildDefaults(TMap<FName, FTDEnemySpec>& Out)
{
	Out.Reset();

	// Shard Fighter - swarm. 30 / 0, 7000 cm/s, pulse guns 5 dmg at 4/s, packs of 4-8, strafing runs.
	{
		FTDEnemySpec S;
		S.Id = TDEnemyIds::ShardFighter;
		S.VisualsAssetName = TEXT("DA_Enemy_Fighter_Visuals");
		S.DisplayName = LOCTEXT("ShardFighter", "Shard Fighter");
		S.Role = ETDEnemyRole::Fighter;
		S.Size = ETDEnemySize::Small;
		S.Hull = 30.f;
		S.Shield = 0.f;
		S.CruiseSpeed = 7000.f;
		S.TurnRateDeg = 110.f;
		S.Radius = 220.f;
		S.Guns = TDEnemyLib_MakeAttack(ETDEnemyAttack::Guns, TDEnemyWeaponIds::PulseGuns, 5.f, 1, 0.25f, 0.f, 9000.f, 10.f);
		S.EngageRange = 11000.f;
		S.BreakOffRange = 2200.f;
		S.BreakOffSeconds = 2.0f;
		S.AttackRunMaxSeconds = 3.5f;
		S.AimErrorDeg = 3.5f;
		S.PackMin = 4;
		S.PackMax = 8;
		S.DeathTumbleSeconds = 0.25f;
		S.KillImportance = 0.f;
		Out.Add(S.Id, S);
	}

	// Hook Interceptor - hunter. 50 / 20, 9000 cm/s, pulse guns + homing missile (25) every 8 s.
	{
		FTDEnemySpec S;
		S.Id = TDEnemyIds::HookInterceptor;
		S.VisualsAssetName = TEXT("DA_Enemy_Interceptor_Visuals");
		S.DisplayName = LOCTEXT("HookInterceptor", "Hook Interceptor");
		S.Role = ETDEnemyRole::Interceptor;
		S.Size = ETDEnemySize::Small;
		S.Hull = 50.f;
		S.Shield = 20.f;
		S.CruiseSpeed = 9000.f;
		S.TurnRateDeg = 125.f;
		S.Radius = 260.f;
		S.Guns = TDEnemyLib_MakeAttack(ETDEnemyAttack::Guns, TDEnemyWeaponIds::PulseGuns, 5.f, 1, 0.25f, 0.f, 9000.f, 10.f);
		S.Heavy = TDEnemyLib_MakeAttack(ETDEnemyAttack::Missile, TDEnemyWeaponIds::HomingMissile, 25.f, 1, 8.f, 0.8f, 16000.f, 35.f);
		S.EngageRange = 14000.f;
		S.BreakOffRange = 2500.f;
		S.BreakOffSeconds = 1.6f;
		S.AttackRunMaxSeconds = 4.f;
		S.AimErrorDeg = 2.5f;
		S.PackMin = 1;
		S.PackMax = 2;
		S.DeathTumbleSeconds = 0.35f;
		S.KillImportance = 0.f;
		Out.Add(S.Id, S);
	}

	// Wall Gunship - tank. 400 hull + 300 frontal shield, 3000 cm/s, heavy bolts (20) + missile salvo (3 x 20).
	{
		FTDEnemySpec S;
		S.Id = TDEnemyIds::WallGunship;
		S.VisualsAssetName = TEXT("DA_Enemy_Gunship_Visuals");
		S.DisplayName = LOCTEXT("WallGunship", "Wall Gunship");
		S.Role = ETDEnemyRole::Gunship;
		S.Size = ETDEnemySize::Large;
		S.Hull = 400.f;
		S.Shield = 0.f;
		S.FrontalShield = 300.f;
		S.FrontalShieldHalfAngleDeg = 70.f;
		S.FrontalShieldRecoverSeconds = 15.f;
		S.CruiseSpeed = 3000.f;
		S.TurnRateDeg = 45.f;
		S.MinSpeedFraction = 0.f;	// holds range, nose to the threat
		S.Radius = 700.f;
		S.Heavy = TDEnemyLib_MakeAttack(ETDEnemyAttack::HeavyBolt, TDEnemyWeaponIds::HeavyBolt, 20.f, 3, 4.5f, 0.6f, 18000.f, 15.f);	// ENG-27: 3.5 s made gunships the top damage source in waves 3-4
		// One FireOnce of Enemy_SalvoMissile = T2's 3-missile ripple.
		S.Heavy2 = TDEnemyLib_MakeAttack(ETDEnemyAttack::MissileSalvo, TDEnemyWeaponIds::SalvoMissile, 20.f, 3, 12.f, 1.2f, 20000.f, 40.f, 0.f);
		S.EngageRange = 15000.f;
		S.BreakOffRange = 4000.f;
		S.BreakOffSeconds = 0.f;		// gunships hold range instead of breaking off
		S.AttackRunMaxSeconds = 8.f;
		S.AimErrorDeg = 2.f;
		S.DeathTumbleSeconds = 1.2f;
		S.KillImportance = 0.5f;
		Out.Add(S.Id, S);
	}

	// Maul Bomber - area denial. 150 / 50, 4000 cm/s, plasma mines (60 dmg, 800 cm), lays mine trails.
	{
		FTDEnemySpec S;
		S.Id = TDEnemyIds::MaulBomber;
		S.VisualsAssetName = TEXT("DA_Enemy_Bomber_Visuals");
		S.DisplayName = LOCTEXT("MaulBomber", "Maul Bomber");
		S.Role = ETDEnemyRole::Bomber;
		S.Size = ETDEnemySize::Medium;
		S.Hull = 150.f;
		S.Shield = 50.f;
		S.CruiseSpeed = 4000.f;
		S.TurnRateDeg = 60.f;
		S.Radius = 500.f;
		// Range = how close to the squad's route point the bomber must be to start a trail.
		// Interval = cooldown between trails; Count = mines per trail (one FireOnce each).
		S.Heavy = TDEnemyLib_MakeAttack(ETDEnemyAttack::MineTrail, TDEnemyWeaponIds::PlasmaMines, 60.f, 4, 5.f, 0.5f, 7000.f, 180.f, 800.f);
		S.EngageRange = 9000.f;
		S.BreakOffRange = 0.f;
		S.BreakOffSeconds = 3.f;
		S.AttackRunMaxSeconds = 6.f;
		S.DeathTumbleSeconds = 0.8f;
		S.KillImportance = 0.2f;
		Out.Add(S.Id, S);
	}

	// Hive Carrier - spawner / mini-boss. 2000 hull + 3 bays x 300, 1500 cm/s, 4 PD turrets (6),
	// launches 3 fighters per bay every 12 s.
	{
		FTDEnemySpec S;
		S.Id = TDEnemyIds::HiveCarrier;
		S.VisualsAssetName = TEXT("DA_Enemy_Carrier_Visuals");
		S.DisplayName = LOCTEXT("HiveCarrier", "Hive Carrier");
		S.Role = ETDEnemyRole::Carrier;
		S.Size = ETDEnemySize::Boss;
		S.Hull = 1500.f;	// ENG-27: features.md 2000; the balance run (Standard, average pilot) put wave 5 at 267 s
		S.Shield = 0.f;
		S.CruiseSpeed = 1500.f;
		S.TurnRateDeg = 15.f;
		S.MinSpeedFraction = 0.f;
		S.Radius = 1800.f;
		S.Heavy = TDEnemyLib_MakeAttack(ETDEnemyAttack::LaunchFighters, NAME_None, 0.f, 3, 12.f, 1.5f, 60000.f, 180.f);
		S.BayCount = 3;
		S.MaxLaunchesPerBay = 3;	// 3 bays x 3 x 3 = at most 27 fighters per carrier
		S.BayHull = 300.f;
		S.LaunchedEnemyId = TDEnemyIds::ShardFighter;
		S.TurretCount = 4;
		S.TurretHull = 60.f;
		S.TurretGuns = TDEnemyLib_MakeAttack(ETDEnemyAttack::Guns, TDEnemyWeaponIds::PointDefense, 6.f, 1, 1.f / 6.f, 0.f, 8000.f, 6.f);
		S.EngageRange = 25000.f;
		S.BreakOffRange = 0.f;
		S.BreakOffSeconds = 0.f;
		S.AttackRunMaxSeconds = 0.f;
		S.AimErrorDeg = 0.f;
		S.DeathTumbleSeconds = 2.5f;
		S.KillImportance = 1.f;
		S.PlaceholderColor = FLinearColor(0.6f, 0.05f, 0.02f);
		Out.Add(S.Id, S);
	}
}

const FTDEnemySpec& UTDEnemyLibrary::Get(FName EnemyId)
{
	static const FTDEnemySpec Invalid;
	if (const FTDEnemySpec* Found = Table().Find(EnemyId))
	{
		return *Found;
	}
	UE_LOG(LogTriadDrift, Warning, TEXT("UTDEnemyLibrary: unknown enemy id '%s'"), *EnemyId.ToString());
	return Invalid;
}

FTDEnemySpec UTDEnemyLibrary::GetEnemySpec(FName EnemyId, bool& bFound)
{
	bFound = HasEnemy(EnemyId);
	return bFound ? Table().FindChecked(EnemyId) : FTDEnemySpec();
}

bool UTDEnemyLibrary::HasEnemy(FName EnemyId)
{
	return Table().Contains(EnemyId);
}

TArray<FName> UTDEnemyLibrary::GetAllEnemyIds()
{
	TArray<FName> Ids;
	Table().GetKeys(Ids);
	return Ids;
}

void UTDEnemyLibrary::OverrideSpec(const FTDEnemySpec& Spec)
{
	if (Spec.IsValid())
	{
		Table().Add(Spec.Id, Spec);
	}
}

void UTDEnemyLibrary::ResetToDefaults()
{
	BuildDefaults(Table());
}

#undef LOCTEXT_NAMESPACE
