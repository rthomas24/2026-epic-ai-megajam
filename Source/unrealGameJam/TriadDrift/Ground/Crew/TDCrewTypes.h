// Triad Drift - crew enemy specs (ENG-23). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDCrewTypes.generated.h"

UENUM(BlueprintType)
enum class ETDCrewType : uint8
{
	Grunt,
	Trooper,
	Heavy,
	Drone,
	Warden
};

/** AI state (features.md ENG-23: Idle/Patrol -> Alert -> Engage -> Stagger -> Dead). */
UENUM(BlueprintType)
enum class ETDCrewState : uint8
{
	Idle,
	Patrol,
	Alert,
	Engage,
	Stagger,
	Dead
};

/** What the crew member is doing right now (read by Nick's AnimBP and telegraph FX). */
UENUM(BlueprintType)
enum class ETDCrewAttack : uint8
{
	None,
	Melee,        // grunt baton, heavy shield bash
	Burst,        // trooper / drone bolts
	Shotgun,      // heavy
	ChargeSlam,   // warden
	BoltFan,      // warden
	Summon        // warden
};

/** One crew type's numbers. Defaults live in UTDCrewLibrary (features.md ENG-23 table). */
USTRUCT(BlueprintType)
struct FTDCrewSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew") ETDCrewType Type = ETDCrewType::Grunt;
	/** Presentation id: /Game/TriadDrift/Presentation/Crew/DA_Crew_<VisualsId>_Visuals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew") FName VisualsId = TEXT("Grunt");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body") float MaxHealth = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body", meta = (Units = "cm")) float CapsuleRadius = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body", meta = (Units = "cm")) float CapsuleHalfHeight = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body", meta = (Units = "cm/s")) float MoveSpeed = 520.f;
	/** Drones fly; everything else walks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body") bool bFlying = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body", meta = (Units = "cm")) float HoverHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense", meta = (Units = "cm")) float SightRange = 3500.f;
	/** Seconds between spotting the player and engaging (readability). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense", meta = (Units = "s")) float AlertSeconds = 0.6f;

	/** Engage distance band: back off inside Min, close in beyond Max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engage", meta = (Units = "cm")) float PreferredRangeMin = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engage", meta = (Units = "cm")) float PreferredRangeMax = 140.f;
	/** Grunts approach off-axis to flank. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engage", meta = (Units = "deg")) float FlankAngle = 0.f;
	/** Troopers: hide behind registered cover, peek out to fire, relocate when flanked/approached/cover lost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover") bool bUsesCover = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (Units = "s")) float CoverHideSeconds = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (Units = "cm")) float CoverMaxTravel = 1600.f;
	/** Leave cover when the player gets this close. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (Units = "cm")) float CoverAbandonDistance = 500.f;

	/** Ranged crew sidestep while firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engage") float StrafeScale = 0.f;

	// Melee (grunt baton, heavy bash)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee") float MeleeDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee", meta = (Units = "cm")) float MeleeRange = 170.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee", meta = (Units = "s")) float MeleeWindup = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee", meta = (Units = "s")) float MeleeCooldown = 1.4f;
	/** Melee attackers need an attack token (at most N grunts swing at once). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee") bool bNeedsAttackToken = false;

	// Bolts (trooper, drone, warden fan)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged") float BoltDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (Units = "cm/s")) float BoltSpeed = 2200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (Units = "cm")) float BoltRadius = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged") int32 BurstCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (Units = "s")) float BurstSpacing = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (Units = "s")) float BurstWindup = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (Units = "s")) float BurstCooldown = 2.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (Units = "cm")) float RangedMaxRange = 2500.f;

	// Shotgun (heavy)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shotgun") int32 ShotgunPellets = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shotgun") float ShotgunPelletDamage = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shotgun", meta = (Units = "deg")) float ShotgunSpreadDeg = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shotgun", meta = (Units = "cm")) float ShotgunMinRange = 260.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shotgun", meta = (Units = "cm")) float ShotgunMaxRange = 1100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shotgun", meta = (Units = "s")) float ShotgunCooldown = 2.6f;

	// Frontal shield (heavy): blocks blade slashes from the front until a heavy lunge breaks it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield") bool bFrontalShield = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (Units = "deg")) float ShieldArcDeg = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shield", meta = (Units = "s")) float GuardBrokenSeconds = 3.f;

	// Warden (mini-boss)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") float ChargeSlamDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden", meta = (Units = "cm")) float ChargeSlamRadius = 320.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden", meta = (Units = "cm")) float ChargeDistance = 1200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden", meta = (Units = "s")) float ChargeDuration = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden", meta = (Units = "s")) float ChargeWindup = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") int32 FanBolts = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden", meta = (Units = "deg")) float FanArcDeg = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") int32 SummonCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") int32 SummonMaxAlive = 4;
	/** Seconds between Warden attacks (scaled by Phase2CooldownScale in phase 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden", meta = (Units = "s")) float BossAttackInterval = 2.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") float Phase2HealthFraction = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") float Phase2CooldownScale = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") int32 Phase2FanBolts = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warden") int32 Phase2SummonCount = 0;

	/** Placeholder tint (Armada red / orange with black armour). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look") FLinearColor TeamColor = FLinearColor(0.85f, 0.08f, 0.04f);
	/** Placeholder body size relative to the capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look") FVector BodyScale = FVector(0.7f, 0.7f, 1.8f);
};

/** Crew spec table (C++ defaults; features.md ENG-23). */
class UNREALGAMEJAM_API FTDCrewLibrary
{
public:
	static const FTDCrewSpec& Get(ETDCrewType Type);
	static FName TypeName(ETDCrewType Type);
};
