// Triad Drift - Combat (Teammate 2). Weapon and projectile data (ENG-05).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDWeaponTypes.generated.h"

class UTDWeaponFX;

/** How a weapon turns a held trigger into shots. */
UENUM(BlueprintType)
enum class ETDFireMode : uint8
{
	/** Fires at FireRate while the trigger is held. */
	Automatic,
	/** Hold to charge for ChargeSeconds, release to fire. Releasing early cancels. */
	Charge,
};

/** What limits sustained fire. */
UENUM(BlueprintType)
enum class ETDWeaponResource : uint8
{
	/** Unlimited (enemy weapons: the AI paces them). */
	None,
	/** Builds heat while firing; overheats and locks out. */
	Heat,
	/** Finite ammo that regenerates over time. */
	Ammo,
};

/** Everything a projectile needs to fly and hurt. Plain data: copied into every projectile. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDProjectileSpec
{
	GENERATED_BODY()

	// --- Damage ---
	/** Direct-hit damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	ETDDamageKind DamageKind = ETDDamageKind::Energy;

	/** 0 = no splash. Hostiles whose bounds are within this radius of the detonation take SplashDamage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (Units = "cm"))
	float SplashRadius = 0.f;

	/** Splash damage to everything in SplashRadius other than the direct-hit victim. < 0 = use Damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float SplashDamage = -1.f;

	/** Velocity change applied to small hostiles hit directly (Breaker Blaster). 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (Units = "CentimetersPerSecond"))
	float KnockbackSpeed = 0.f;

	/** Knockback only applies to targets whose bounding-sphere radius is at most this (fighters, not gunships). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (Units = "cm"))
	float KnockbackMaxTargetRadius = 600.f;

	// --- Flight ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (Units = "CentimetersPerSecond"))
	float Speed = 30000.f;

	/** Max distance flown before expiring (or detonating, if it has splash). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (Units = "cm"))
	float Range = 30000.f;

	/** > 0 overrides Range/Speed as the lifetime (mines). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (Units = "s"))
	float LifetimeOverride = 0.f;

	/** Radius of the swept collision sphere. Generous radii make arcade hits feel fair. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (Units = "cm"))
	float CollisionRadius = 40.f;

	/** Add the shooter's forward velocity so shots never get outrun by a boosting ship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	bool bInheritShooterVelocity = true;

	/** Detonate at end of range/lifetime (splash weapons) instead of fizzling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	bool bDetonateOnExpire = false;

	// --- Homing ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
	bool bHoming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing", meta = (Units = "DegreesPerSecond"))
	float HomingTurnRateDeg = 90.f;

	/** If spawned without a target, acquire the nearest hostile within this cone of the flight direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing", meta = (Units = "Degrees"))
	float HomingAcquireConeDeg = 30.f;

	/** Seconds after launch before steering starts (lets salvos fan out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing", meta = (Units = "s"))
	float HomingDelay = 0.f;

	// --- Aim assist (issue #192: the Rift's and the Lancer's regular attacks; not a guided missile) ---
	/** > 0: at launch the shot picks the hostile nearest its initial line within this half-angle (and AssistRange), then
	 *  turns toward it at AssistTurnRateDeg, never more than AssistMaxTurnDeg in all; it flies straight if the target dies
	 *  or is lost (no re-acquire). Only the player's own ship; wingmen's shots are unchanged. 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim assist", meta = (ClampMin = "0", ClampMax = "30"))
	float AssistConeDeg = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim assist", meta = (ClampMin = "0"))
	float AssistTurnRateDeg = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim assist", meta = (ClampMin = "0", ClampMax = "90"))
	float AssistMaxTurnDeg = 40.f;
	/** Acquisition range (cm); 0 = the shot's own Range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim assist", meta = (ClampMin = "0"))
	float AssistRange = 0.f;

	// --- Proximity fuse (flak, mines) ---
	/** > 0: detonates when a hostile ship (or a hostile missile, if bShootsDownMissiles) comes this close. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fuse", meta = (Units = "cm"))
	float ProximityFuseRadius = 0.f;

	/** Seconds after launch before the proximity fuse arms (so flak doesn't pop on the muzzle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fuse", meta = (Units = "s"))
	float FuseArmDelay = 0.f;

	/** Destroys hostile missiles: in its splash on detonation, and by proximity-fusing on them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fuse")
	bool bShootsDownMissiles = false;

	/** This projectile is a missile / mine: flak and point defense can shoot it down, and it triggers incoming-missile warnings when homing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fuse")
	bool bIsMissile = false;

	// --- Split (Splitter Bolt) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split")
	int32 ShardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split")
	float ShardDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split", meta = (Units = "CentimetersPerSecond"))
	float ShardSpeed = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split", meta = (Units = "cm"))
	float ShardRange = 6000.f;

	/** Half-angle of the cone shards fan out in, around the parent's flight direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Split", meta = (Units = "Degrees"))
	float ShardConeDeg = 22.f;

	// --- Placeholder look (used until Nick's tracer FX exists) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
	FLinearColor PlaceholderColor = FLinearColor(1.f, 0.2f, 0.9f);

	/** Scale of the engine basic sphere: X is the stretch along the flight direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
	FVector PlaceholderScale = FVector(1.2f, 0.25f, 0.25f);

	/** Presentation slots (muzzle/tracer/impact/sounds). Null = placeholder only. Set at fire time, never in C++ defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "Look")
	TObjectPtr<UTDWeaponFX> FX = nullptr;

	float GetLifetime() const { return LifetimeOverride > 0.f ? LifetimeOverride : (Speed > KINDA_SMALL_NUMBER ? Range / Speed : 5.f); }
	float GetSplashDamage() const { return SplashDamage < 0.f ? Damage : SplashDamage; }
};

/** A weapon: trigger behaviour, resource, and the projectile it fires. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDWeaponSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	ETDFireMode FireMode = ETDFireMode::Automatic;

	/** Shots (or salvos) per second while the trigger is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.01"))
	float FireRate = 5.f;

	/** Projectiles per shot. > 1 with SalvoInterval > 0 fires them in a quick ripple (Spike Rockets). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 ProjectilesPerShot = 1;

	/** Seconds between projectiles inside one salvo. 0 = all at once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (Units = "s"))
	float SalvoInterval = 0.f;

	/** Random cone half-angle per projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (Units = "Degrees"))
	float SpreadDeg = 0.f;

	/** If the aim target (or soft target) is within this angle of the aim direction, shots aim at its lead point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (Units = "Degrees"))
	float AimAssistDeg = 0.f;

	/** Cycle through muzzles shot by shot (twin alternating lasers) instead of firing from all at once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bAlternateMuzzles = true;

	/** Hold time to fire a Charge weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (Units = "s"))
	float ChargeSeconds = 0.f;

	// --- Resource ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	ETDWeaponResource Resource = ETDWeaponResource::None;

	/** Heat: seconds of continuous fire from cold to overheat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Units = "s"))
	float HeatSecondsToOverheat = 4.f;

	/** Heat: lockout after overheating. Heat drains to zero over the lockout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Units = "s"))
	float OverheatLockoutSeconds = 1.5f;

	/** Heat: seconds without firing before cooling starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Units = "s"))
	float HeatCoolDelay = 0.25f;

	/** Heat: seconds to cool from full to zero once cooling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Units = "s"))
	float HeatSecondsToCool = 2.f;

	/** Ammo: capacity (shots or salvos). Starts full. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 MaxAmmo = 0;

	/** Ammo: seconds to regenerate one round while below max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Units = "s"))
	float AmmoRegenSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FTDProjectileSpec Projectile;

	bool IsValid() const { return !WeaponId.IsNone(); }
	float GetRefireSeconds() const { return 1.f / FMath::Max(FireRate, 0.01f); }
	/** Heat added per shot so that holding the trigger from cold overheats exactly HeatSecondsToOverheat later (the first shot is at t = 0). */
	float GetHeatPerShot() const { return 1.f / FMath::Max(HeatSecondsToOverheat * FireRate + 1.f, 1.f); }
};

/** Stable weapon ids for UTDWeaponLibrary::Get. */
namespace TDWeaponIds
{
	// Squad (ENG-05)
	inline const FName PhasePulseLasers(TEXT("PhasePulseLasers"));
	inline const FName SplitterBolt(TEXT("SplitterBolt"));
	inline const FName NeedleCannons(TEXT("NeedleCannons"));
	inline const FName SpikeRockets(TEXT("SpikeRockets"));
	inline const FName BreakerBlaster(TEXT("BreakerBlaster"));
	inline const FName FlakBurst(TEXT("FlakBurst"));
	// Null Armada (ENG-13)
	inline const FName EnemyPulseGuns(TEXT("Enemy_PulseGuns"));
	inline const FName EnemyHomingMissile(TEXT("Enemy_HomingMissile"));
	inline const FName EnemyHeavyBolt(TEXT("Enemy_HeavyBolt"));
	/** One trigger pull / FireOnce = a ripple of 3 homing missiles. */
	inline const FName EnemySalvoMissile(TEXT("Enemy_SalvoMissile"));
	inline const FName EnemyPlasmaMines(TEXT("Enemy_PlasmaMines"));
	inline const FName EnemyPointDefense(TEXT("Enemy_PointDefense"));
}
