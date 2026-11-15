// Triad Drift - Combat (Teammate 2). Weapon table: features.md ENG-05 (squad) and ENG-13 (Null Armada).

#include "TriadDrift/Combat/TDWeaponLibrary.h"

#define LOCTEXT_NAMESPACE "TDWeapons"

namespace
{
	// Squad colours (AGENTS.md §2): Rift magenta, Lancer cyan, Bastion amber. Enemies red/orange.
	const FLinearColor TDWLRiftMagenta(1.0f, 0.12f, 0.85f);
	const FLinearColor TDWLLancerCyan(0.1f, 0.9f, 1.0f);
	const FLinearColor TDWLBastionAmber(1.0f, 0.62f, 0.08f);
	const FLinearColor TDWLEnemyRed(1.0f, 0.12f, 0.04f);
	const FLinearColor TDWLEnemyOrange(1.0f, 0.42f, 0.02f);

	FTDWeaponSpec TDWLMakeBase(FName Id, const FText& Name)
	{
		FTDWeaponSpec W;
		W.WeaponId = Id;
		W.DisplayName = Name;
		return W;
	}

	TMap<FName, FTDWeaponSpec> TDWLBuildTable()
	{
		TMap<FName, FTDWeaponSpec> T;

		// ----------------------------------------------------------------- Rift
		{
			// Twin alternating bolts, 8 dmg each, 8/s, 40 000 cm/s, 40 000 cm, heat 4 s, 1.5 s lockout, 3° aim assist.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::PhasePulseLasers, LOCTEXT("PhasePulse", "Phase Pulse Lasers"));
			W.FireMode = ETDFireMode::Automatic;
			W.FireRate = 8.f;
			W.bAlternateMuzzles = true;
			W.AimAssistDeg = 3.f;
			W.Resource = ETDWeaponResource::Heat;
			W.HeatSecondsToOverheat = 4.f;
			W.OverheatLockoutSeconds = 1.5f;
			W.Projectile.Damage = 8.f;
			W.Projectile.DamageKind = ETDDamageKind::Energy;
			W.Projectile.Speed = 40000.f;
			W.Projectile.Range = 40000.f;
			W.Projectile.CollisionRadius = 45.f;
			W.Projectile.PlaceholderColor = TDWLRiftMagenta;
			W.Projectile.PlaceholderScale = FVector(1.6f, 0.22f, 0.22f);
			// #192: mild aim assist (Rift primary).
			W.Projectile.AssistConeDeg = 9.f;
			W.Projectile.AssistTurnRateDeg = 40.f;
			W.Projectile.AssistMaxTurnDeg = 40.f;
			T.Add(W.WeaponId, W);
		}
		{
			// Charge 0.8 s, 40 + 5 x 12 shards, 25 000 cm/s, 35 000 cm, 6 ammo +1 per 4 s, splits on impact.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::SplitterBolt, LOCTEXT("Splitter", "Splitter Bolt"));
			W.FireMode = ETDFireMode::Charge;
			W.ChargeSeconds = 0.8f;
			W.FireRate = 2.f; // refire after a released shot
			W.bAlternateMuzzles = true;
			W.AimAssistDeg = 3.f;
			W.Resource = ETDWeaponResource::Ammo;
			W.MaxAmmo = 6;
			W.AmmoRegenSeconds = 4.f;
			W.Projectile.Damage = 40.f;
			W.Projectile.DamageKind = ETDDamageKind::Energy;
			W.Projectile.Speed = 25000.f;
			W.Projectile.Range = 35000.f;
			W.Projectile.CollisionRadius = 70.f;
			W.Projectile.ShardCount = 5;
			W.Projectile.ShardDamage = 12.f;
			W.Projectile.ShardSpeed = 25000.f;
			W.Projectile.ShardRange = 6000.f;
			W.Projectile.ShardConeDeg = 22.f;
			W.Projectile.bDetonateOnExpire = true; // splits at max range too
			W.Projectile.PlaceholderColor = TDWLRiftMagenta;
			W.Projectile.PlaceholderScale = FVector(1.4f, 0.6f, 0.6f);
			// #192: mild aim assist (Rift secondary; its shards fly straight).
			W.Projectile.AssistConeDeg = 10.f;
			W.Projectile.AssistTurnRateDeg = 45.f;
			W.Projectile.AssistMaxTurnDeg = 40.f;
			T.Add(W.WeaponId, W);
		}

		// ----------------------------------------------------------------- Lancer
		{
			// Rapid kinetic, 4 dmg, 20/s, 55 000 cm/s, 30 000 cm, heat 5 s, 1.2° spread.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::NeedleCannons, LOCTEXT("Needle", "Needle Cannons"));
			W.FireMode = ETDFireMode::Automatic;
			W.FireRate = 20.f;
			W.SpreadDeg = 1.2f;
			W.bAlternateMuzzles = true;
			W.Resource = ETDWeaponResource::Heat;
			W.HeatSecondsToOverheat = 5.f;
			W.OverheatLockoutSeconds = 1.5f;
			W.Projectile.Damage = 4.f;
			W.Projectile.DamageKind = ETDDamageKind::Kinetic;
			W.Projectile.Speed = 55000.f;
			W.Projectile.Range = 30000.f;
			W.Projectile.CollisionRadius = 35.f;
			W.Projectile.PlaceholderColor = TDWLLancerCyan;
			W.Projectile.PlaceholderScale = FVector(2.0f, 0.12f, 0.12f);
			// #192: mild aim assist (Lancer primary, fast cannons).
			W.Projectile.AssistConeDeg = 8.f;
			W.Projectile.AssistTurnRateDeg = 30.f;
			W.Projectile.AssistMaxTurnDeg = 40.f;
			T.Add(W.WeaponId, W);
		}
		{
			// Dumb-fire salvo of 4, 25 each with 300 cm splash, 1 per 1.5 s, 30 000 cm/s, 40 000 cm, 8 salvos +1 per 5 s.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::SpikeRockets, LOCTEXT("Spike", "Spike Rockets"));
			W.FireMode = ETDFireMode::Automatic;
			W.FireRate = 1.f / 1.5f;
			W.ProjectilesPerShot = 4;
			W.SalvoInterval = 0.07f;
			W.SpreadDeg = 1.5f;
			W.bAlternateMuzzles = true;
			W.Resource = ETDWeaponResource::Ammo;
			W.MaxAmmo = 8;
			W.AmmoRegenSeconds = 5.f;
			W.Projectile.Damage = 25.f;
			W.Projectile.DamageKind = ETDDamageKind::Explosive;
			W.Projectile.SplashRadius = 300.f;
			W.Projectile.SplashDamage = 25.f;
			W.Projectile.Speed = 30000.f;
			W.Projectile.Range = 40000.f;
			W.Projectile.CollisionRadius = 50.f;
			W.Projectile.bDetonateOnExpire = true;
			W.Projectile.PlaceholderColor = TDWLLancerCyan;
			W.Projectile.PlaceholderScale = FVector(1.2f, 0.35f, 0.35f);
			// #192: mild aim assist (Lancer secondary).
			W.Projectile.AssistConeDeg = 9.f;
			W.Projectile.AssistTurnRateDeg = 35.f;
			W.Projectile.AssistMaxTurnDeg = 40.f;
			T.Add(W.WeaponId, W);
		}

		// ----------------------------------------------------------------- Bastion
		{
			// Heavy plasma, 30 with 250 cm splash, 2.5/s, 28 000 cm/s, 35 000 cm, heat 6 s, knocks back fighters.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::BreakerBlaster, LOCTEXT("Breaker", "Breaker Blaster"));
			W.FireMode = ETDFireMode::Automatic;
			W.FireRate = 2.5f;
			W.bAlternateMuzzles = true;
			W.Resource = ETDWeaponResource::Heat;
			W.HeatSecondsToOverheat = 6.f;
			W.OverheatLockoutSeconds = 1.5f;
			W.Projectile.Damage = 30.f;
			W.Projectile.DamageKind = ETDDamageKind::Energy;
			W.Projectile.SplashRadius = 250.f;
			W.Projectile.SplashDamage = 30.f;
			W.Projectile.KnockbackSpeed = 1500.f;
			W.Projectile.KnockbackMaxTargetRadius = 600.f;
			W.Projectile.Speed = 28000.f;
			W.Projectile.Range = 35000.f;
			W.Projectile.CollisionRadius = 80.f;
			W.Projectile.PlaceholderColor = TDWLBastionAmber;
			W.Projectile.PlaceholderScale = FVector(1.3f, 0.8f, 0.8f);
			T.Add(W.WeaponId, W);
		}
		{
			// Proximity flak, 15 per fragment over 800 cm, 1 per 1.2 s, 22 000 cm/s, 20 000 cm, 10 shells +1 per 3 s, shoots down missiles.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::FlakBurst, LOCTEXT("Flak", "Flak Burst"));
			W.FireMode = ETDFireMode::Automatic;
			W.FireRate = 1.f / 1.2f;
			W.bAlternateMuzzles = true;
			W.Resource = ETDWeaponResource::Ammo;
			W.MaxAmmo = 10;
			W.AmmoRegenSeconds = 3.f;
			W.Projectile.Damage = 15.f;
			W.Projectile.DamageKind = ETDDamageKind::Explosive;
			W.Projectile.SplashRadius = 800.f;
			W.Projectile.SplashDamage = 15.f;
			W.Projectile.ProximityFuseRadius = 500.f;
			W.Projectile.FuseArmDelay = 0.12f;
			W.Projectile.bShootsDownMissiles = true;
			W.Projectile.bDetonateOnExpire = true;
			W.Projectile.Speed = 22000.f;
			W.Projectile.Range = 20000.f;
			W.Projectile.CollisionRadius = 80.f;
			W.Projectile.PlaceholderColor = TDWLBastionAmber;
			W.Projectile.PlaceholderScale = FVector(0.7f, 0.7f, 0.7f);
			T.Add(W.WeaponId, W);
		}

		// ----------------------------------------------------------------- Null Armada (ENG-13; ids agreed with T4)
		{
			// Shard Fighter / Hook Interceptor: pulse guns 5 dmg, 4/s.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::EnemyPulseGuns, LOCTEXT("EPulse", "Pulse Guns"));
			W.FireRate = 4.f;
			W.bAlternateMuzzles = true;
			W.Projectile.Damage = 5.f;
			W.Projectile.DamageKind = ETDDamageKind::Energy;
			W.Projectile.Speed = 30000.f;
			W.Projectile.Range = 20000.f;
			W.Projectile.CollisionRadius = 45.f;
			W.Projectile.PlaceholderColor = TDWLEnemyRed;
			W.Projectile.PlaceholderScale = FVector(1.4f, 0.22f, 0.22f);
			T.Add(W.WeaponId, W);
		}
		{
			// Hook Interceptor: homing missile 25, every 8 s (AI paces it). Deflectable; flak can shoot it down.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::EnemyHomingMissile, LOCTEXT("EMissile", "Homing Missile"));
			W.FireRate = 1.f / 8.f;
			W.bAlternateMuzzles = true;
			W.Projectile.Damage = 25.f;
			W.Projectile.DamageKind = ETDDamageKind::Explosive;
			W.Projectile.Speed = 9000.f;
			W.Projectile.Range = 9000.f * 7.f; // ~7 s of flight
			W.Projectile.CollisionRadius = 90.f;
			W.Projectile.bHoming = true;
			W.Projectile.HomingTurnRateDeg = 100.f;
			W.Projectile.HomingAcquireConeDeg = 45.f;
			W.Projectile.HomingDelay = 0.2f;
			W.Projectile.bIsMissile = true;
			W.Projectile.PlaceholderColor = TDWLEnemyOrange;
			W.Projectile.PlaceholderScale = FVector(1.0f, 0.45f, 0.45f);
			T.Add(W.WeaponId, W);
		}
		{
			// Wall Gunship: slow heavy bolts, 20.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::EnemyHeavyBolt, LOCTEXT("EHeavy", "Heavy Bolt"));
			W.FireRate = 1.f;
			W.bAlternateMuzzles = true;
			W.Projectile.Damage = 20.f;
			W.Projectile.DamageKind = ETDDamageKind::Energy;
			W.Projectile.Speed = 12000.f;
			W.Projectile.Range = 30000.f;
			W.Projectile.CollisionRadius = 110.f;
			W.Projectile.PlaceholderColor = TDWLEnemyOrange;
			W.Projectile.PlaceholderScale = FVector(1.4f, 1.0f, 1.0f);
			T.Add(W.WeaponId, W);
		}
		{
			// Wall Gunship: missile salvo 3 x 20 homing, rippled 0.15 s apart, fanned out before steering.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::EnemySalvoMissile, LOCTEXT("ESalvo", "Missile Salvo"));
			W.FireRate = 1.f / 10.f;
			W.ProjectilesPerShot = 3;
			W.SalvoInterval = 0.15f;
			W.SpreadDeg = 10.f;
			W.bAlternateMuzzles = true;
			W.Projectile.Damage = 20.f;
			W.Projectile.DamageKind = ETDDamageKind::Explosive;
			W.Projectile.Speed = 9500.f;
			W.Projectile.Range = 9500.f * 7.f;
			W.Projectile.CollisionRadius = 90.f;
			W.Projectile.bHoming = true;
			W.Projectile.HomingTurnRateDeg = 90.f;
			W.Projectile.HomingAcquireConeDeg = 45.f;
			W.Projectile.HomingDelay = 0.35f;
			W.Projectile.bIsMissile = true;
			W.Projectile.PlaceholderColor = TDWLEnemyOrange;
			W.Projectile.PlaceholderScale = FVector(1.0f, 0.45f, 0.45f);
			T.Add(W.WeaponId, W);
		}
		{
			// Maul Bomber: plasma mines 60 over 800 cm. Proximity-fused, drift slowly, flak and Gravity Well counter them.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::EnemyPlasmaMines, LOCTEXT("EMine", "Plasma Mine"));
			W.FireRate = 0.5f;
			W.bAlternateMuzzles = false;
			W.Projectile.Damage = 60.f;
			W.Projectile.DamageKind = ETDDamageKind::Explosive;
			W.Projectile.SplashRadius = 800.f;
			W.Projectile.SplashDamage = 60.f;
			W.Projectile.ProximityFuseRadius = 500.f;
			W.Projectile.FuseArmDelay = 0.75f;
			W.Projectile.Speed = 300.f;
			W.Projectile.bInheritShooterVelocity = false;
			W.Projectile.LifetimeOverride = 20.f;
			W.Projectile.Range = 300.f * 20.f;
			W.Projectile.CollisionRadius = 140.f;
			W.Projectile.bIsMissile = true; // shoot-downable
			W.Projectile.PlaceholderColor = FLinearColor(1.f, 0.05f, 0.35f);
			W.Projectile.PlaceholderScale = FVector(1.6f, 1.6f, 1.6f);
			T.Add(W.WeaponId, W);
		}
		{
			// Hive Carrier / Dominion turrets: point defense 6 dmg, 6/s, short range; also shoots down squad missiles.
			FTDWeaponSpec W = TDWLMakeBase(TDWeaponIds::EnemyPointDefense, LOCTEXT("EPD", "Point Defense"));
			W.FireRate = 6.f;
			W.SpreadDeg = 1.5f;
			W.bAlternateMuzzles = true;
			W.Projectile.Damage = 6.f;
			W.Projectile.DamageKind = ETDDamageKind::Kinetic;
			W.Projectile.Speed = 30000.f;
			W.Projectile.Range = 8000.f;
			W.Projectile.CollisionRadius = 40.f;
			W.Projectile.bShootsDownMissiles = true;
			W.Projectile.PlaceholderColor = TDWLEnemyRed;
			W.Projectile.PlaceholderScale = FVector(1.2f, 0.18f, 0.18f);
			T.Add(W.WeaponId, W);
		}

		return T;
	}

	const TArray<FName>& TDWLOrderedIds()
	{
		static const TArray<FName> Ids = {
			TDWeaponIds::PhasePulseLasers, TDWeaponIds::SplitterBolt, TDWeaponIds::NeedleCannons, TDWeaponIds::SpikeRockets,
			TDWeaponIds::BreakerBlaster, TDWeaponIds::FlakBurst, TDWeaponIds::EnemyPulseGuns, TDWeaponIds::EnemyHomingMissile,
			TDWeaponIds::EnemyHeavyBolt, TDWeaponIds::EnemySalvoMissile, TDWeaponIds::EnemyPlasmaMines, TDWeaponIds::EnemyPointDefense };
		return Ids;
	}
}

const TMap<FName, FTDWeaponSpec>& UTDWeaponLibrary::GetTable()
{
	static const TMap<FName, FTDWeaponSpec> Table = TDWLBuildTable();
	return Table;
}

const FTDWeaponSpec& UTDWeaponLibrary::Get(FName WeaponId)
{
	static const FTDWeaponSpec Invalid;
	if (WeaponId.IsNone())
	{
		return Invalid;
	}
	if (const FTDWeaponSpec* Found = GetTable().Find(WeaponId))
	{
		return *Found;
	}
	UE_LOG(LogTriadDrift, Warning, TEXT("UTDWeaponLibrary: unknown weapon id '%s'"), *WeaponId.ToString());
	return Invalid;
}

bool UTDWeaponLibrary::Has(FName WeaponId)
{
	return GetTable().Contains(WeaponId);
}

TArray<FName> UTDWeaponLibrary::GetAllIds()
{
	return TDWLOrderedIds();
}

#undef LOCTEXT_NAMESPACE
