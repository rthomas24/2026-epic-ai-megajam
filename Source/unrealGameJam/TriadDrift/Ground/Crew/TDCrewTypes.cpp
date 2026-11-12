// Triad Drift - crew enemy specs (ENG-23). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDCrewTypes.h"

namespace TDCrewSpecs
{
	static TArray<FTDCrewSpec> MakeTable()
	{
		TArray<FTDCrewSpec> T;
		T.SetNum(5);

		// Grunt: 60 HP, telegraphed baton (15), groups of 3-5, flanks.
		FTDCrewSpec& G = T[(int32)ETDCrewType::Grunt];
		G.Type = ETDCrewType::Grunt; G.VisualsId = TEXT("Grunt");
		G.MaxHealth = 60.f; G.MoveSpeed = 560.f;
		G.PreferredRangeMin = 0.f; G.PreferredRangeMax = 140.f; G.FlankAngle = 40.f;
		G.MeleeDamage = 15.f; G.MeleeRange = 170.f; G.MeleeWindup = 0.6f; G.MeleeCooldown = 1.4f; G.bNeedsAttackToken = true;

		// Trooper: 50 HP, slow bright deflectable bolts (10, 3-round bursts), keeps distance.
		FTDCrewSpec& Tr = T[(int32)ETDCrewType::Trooper];
		Tr.Type = ETDCrewType::Trooper; Tr.VisualsId = TEXT("Trooper");
		Tr.MaxHealth = 50.f; Tr.MoveSpeed = 480.f;
		Tr.PreferredRangeMin = 900.f; Tr.PreferredRangeMax = 1500.f; Tr.StrafeScale = 0.5f;
		Tr.BoltDamage = 10.f; Tr.BoltSpeed = 2200.f; Tr.BurstCount = 3; Tr.BurstSpacing = 0.18f; Tr.BurstWindup = 0.4f; Tr.BurstCooldown = 2.2f;
		Tr.TeamColor = FLinearColor(0.95f, 0.35f, 0.05f);
		Tr.bUsesCover = true;

		// Heavy: 200 HP, shield bash (25), deflectable shotgun (5 x 6), frontal shield blocks slashes.
		FTDCrewSpec& H = T[(int32)ETDCrewType::Heavy];
		H.Type = ETDCrewType::Heavy; H.VisualsId = TEXT("Heavy");
		H.MaxHealth = 200.f; H.MoveSpeed = 330.f; H.CapsuleRadius = 55.f; H.CapsuleHalfHeight = 105.f;
		H.PreferredRangeMin = 0.f; H.PreferredRangeMax = 220.f;
		H.MeleeDamage = 25.f; H.MeleeRange = 220.f; H.MeleeWindup = 0.7f; H.MeleeCooldown = 1.8f;
		H.ShotgunPellets = 6; H.ShotgunPelletDamage = 5.f; H.BoltSpeed = 2600.f;
		H.bFrontalShield = true; H.ShieldArcDeg = 120.f;
		H.BodyScale = FVector(1.1f, 1.1f, 2.1f);
		H.TeamColor = FLinearColor(0.45f, 0.05f, 0.03f);

		// Sentry drone: 30 HP, fast small bolts (5), hovers out of melee range.
		FTDCrewSpec& D = T[(int32)ETDCrewType::Drone];
		D.Type = ETDCrewType::Drone; D.VisualsId = TEXT("Drone");
		D.MaxHealth = 30.f; D.MoveSpeed = 600.f; D.bFlying = true; D.HoverHeight = 220.f; // within a jump-slash (machine playtest: at 320 the blade could never reach one)
		D.CapsuleRadius = 35.f; D.CapsuleHalfHeight = 35.f;
		D.PreferredRangeMin = 350.f; D.PreferredRangeMax = 600.f; D.StrafeScale = 0.8f;
		D.BoltDamage = 5.f; D.BoltSpeed = 4000.f; D.BoltRadius = 14.f; D.BurstCount = 1; D.BurstWindup = 0.25f; D.BurstCooldown = 0.9f;
		D.BodyScale = FVector(0.6f, 0.6f, 0.5f);
		D.TeamColor = FLinearColor(1.f, 0.55f, 0.1f);

		// Warden (mini-boss): 600 HP, charge slam (40 AoE), bolt fan, summons grunts; phase 2 at 50%.
		FTDCrewSpec& W = T[(int32)ETDCrewType::Warden];
		W.Type = ETDCrewType::Warden; W.VisualsId = TEXT("Warden");
		W.MaxHealth = 400.f; W.MoveSpeed = 380.f; // (machine playtest: 600 HP + 40 slam + 7x10 fans beat the pilot every take) W.CapsuleRadius = 70.f; W.CapsuleHalfHeight = 130.f;
		W.SightRange = 5000.f; W.AlertSeconds = 1.0f;
		W.PreferredRangeMin = 400.f; W.PreferredRangeMax = 1100.f;
		W.ChargeSlamDamage = 30.f; W.ChargeSlamRadius = 320.f; W.ChargeDistance = 1200.f; W.ChargeDuration = 0.5f; W.ChargeWindup = 0.8f;
		W.BoltDamage = 7.f; W.BoltSpeed = 2000.f; W.FanBolts = 7; W.FanArcDeg = 60.f; W.Phase2FanBolts = 11;
		W.SummonCount = 1; W.Phase2SummonCount = 2; W.SummonMaxAlive = 4;
		W.BossAttackInterval = 2.4f; W.Phase2HealthFraction = 0.5f; W.Phase2CooldownScale = 0.7f;
		W.BurstWindup = 0.5f;
		W.BodyScale = FVector(1.5f, 1.5f, 2.6f);
		W.TeamColor = FLinearColor(0.6f, 0.02f, 0.02f);
		return T;
	}
}

const FTDCrewSpec& FTDCrewLibrary::Get(ETDCrewType Type)
{
	static const TArray<FTDCrewSpec> Table = TDCrewSpecs::MakeTable();
	const int32 Index = FMath::Clamp((int32)Type, 0, Table.Num() - 1);
	return Table[Index];
}

FName FTDCrewLibrary::TypeName(ETDCrewType Type)
{
	return Get(Type).VisualsId;
}
