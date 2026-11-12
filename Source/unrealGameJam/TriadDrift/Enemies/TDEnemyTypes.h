// Triad Drift - Enemies & Waves (Teammate 4). ENG-13 enemy data types.
#pragma once

#include "CoreMinimal.h"
#include "TDEnemyTypes.generated.h"

/** What an enemy is for. Drives the AI's behaviour profile. */
UENUM(BlueprintType)
enum class ETDEnemyRole : uint8
{
	Fighter,		// Shard Fighter: swarm, packs, strafing runs
	Interceptor,	// Hook Interceptor: hunts the player's active ship
	Gunship,		// Wall Gunship: faces its target, frontal shield
	Bomber,			// Maul Bomber: lays plasma mine trails along the squad's route
	Carrier			// Hive Carrier: spawner / mini-boss with bays and point defense
};

/** Size class. Drives death variety (tumble time, explosion tier) and the director-cam importance. */
UENUM(BlueprintType)
enum class ETDEnemySize : uint8
{
	Small,
	Medium,
	Large,
	Boss
};

/** Attack slots an enemy can use. "Heavy" attacks are always telegraphed. */
UENUM(BlueprintType)
enum class ETDEnemyAttack : uint8
{
	None,
	Guns,			// continuous primary fire (no telegraph)
	Missile,		// interceptor homing missile
	HeavyBolt,		// gunship slow heavy bolts
	MissileSalvo,	// gunship 3 x missiles
	MineTrail,		// bomber plasma mine drop
	LaunchFighters	// carrier bay launch
};

/** One attack an enemy can perform. Numbers default to features.md ENG-13. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDEnemyAttackSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	ETDEnemyAttack Kind = ETDEnemyAttack::None;

	/** Weapon id in T2's UTDWeaponLibrary. NAME_None = handled by enemy code (fighter launches). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName WeaponId = NAME_None;

	/** Damage per projectile / mine (reference value; T2's weapon spec is authoritative for WeaponId attacks). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Damage = 0.f;

	/** Projectiles (or fighters) per use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	int32 Count = 1;

	/** Seconds between uses (cooldown). For Guns this is 1 / fire rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Interval = 1.f;

	/** Telegraph wind-up before the attack fires. 0 = no telegraph. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float TelegraphSeconds = 0.f;

	/** Max range at which the AI will start this attack (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Range = 20000.f;

	/** Splash radius (cm), for mines and salvos. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float SplashRadius = 0.f;

	/** Firing cone half-angle: the AI only starts the attack when the target is inside it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0", ClampMax = "180"))
	float ConeHalfAngleDeg = 12.f;

	bool IsValid() const { return Kind != ETDEnemyAttack::None; }
	bool IsTelegraphed() const { return TelegraphSeconds > 0.f; }
};

/** Everything that defines one enemy type. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDEnemySpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	ETDEnemyRole Role = ETDEnemyRole::Fighter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	ETDEnemySize Size = ETDEnemySize::Small;

	// --- Survivability -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float Hull = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float Shield = 0.f;

	/** Gunship frontal shield HP. 0 = none. Blocks damage arriving inside FrontalShieldHalfAngleDeg of the nose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float FrontalShield = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0", ClampMax = "180"))
	float FrontalShieldHalfAngleDeg = 70.f;

	/** Seconds after the frontal shield breaks before it comes back at full strength. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float FrontalShieldRecoverSeconds = 15.f;

	// --- Flight ---------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0"))
	float CruiseSpeed = 7000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "0"))
	float TurnRateDeg = 100.f;

	/** Placeholder collision / visual radius (cm) until Nick's meshes arrive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (ClampMin = "10"))
	float Radius = 250.f;

	// --- Weapons --------------------------------------------------------------------------------

	/** Continuous guns (Primary slot). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	FTDEnemyAttackSpec Guns;

	/** The big, telegraphed attack (Secondary slot, mines or launches). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	FTDEnemyAttackSpec Heavy;

	/** Optional second heavy attack (gunship has bolts + salvo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	FTDEnemyAttackSpec Heavy2;

	// --- Behaviour ------------------------------------------------------------------------------

	/** Distance the AI tries to reach before starting an attack run (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "0"))
	float EngageRange = 12000.f;

	/** Break off the attack run when closer than this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "0"))
	float BreakOffRange = 2500.f;

	/** Seconds spent breaking away before re-approaching. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "0"))
	float BreakOffSeconds = 2.5f;

	/** Max attack-run duration before forcing a break-off (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "0"))
	float AttackRunMaxSeconds = 4.f;

	/** Aim error in degrees (applied to the guns' aim direction). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "0"))
	float AimErrorDeg = 3.f;

	/** Pack size range for fighters spawned as a group. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "1"))
	int32 PackMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "1"))
	int32 PackMax = 1;

	// --- Carrier --------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier", meta = (ClampMin = "0"))
	int32 BayCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier", meta = (ClampMin = "0"))
	float BayHull = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier")
	FName LaunchedEnemyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier", meta = (ClampMin = "0"))
	int32 TurretCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier")
	FTDEnemyAttackSpec TurretGuns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carrier", meta = (ClampMin = "0"))
	float TurretHull = 60.f;

	// --- Presentation / death -------------------------------------------------------------------

	/** Seconds of tumble before the explosion. Varies by size. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0"))
	float DeathTumbleSeconds = 0.3f;

	/** Importance passed to OnHighlightMoment when this enemy dies (0 = never a highlight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0", ClampMax = "1"))
	float KillImportance = 0.f;

	/** Placeholder tint used until UTDEnemyVisuals is assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FLinearColor PlaceholderColor = FLinearColor(1.f, 0.15f, 0.05f);

	bool IsValid() const { return !Id.IsNone(); }
};
