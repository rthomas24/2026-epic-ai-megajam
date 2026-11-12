// Triad Drift - presentation hooks (features.md §3.5). Lead-owned class definitions; Nick owns the
// asset instances. Every slot is optional: an empty slot must do nothing.
// Play a cue with UTDStatics::PlayCue / PlayCueAttached.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDPresentation.generated.h"

class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;
class UStaticMesh;
class USkeletalMesh;
class UMaterialInterface;
class UTexture2D;

/** One presentation moment: an effect, a sound and a camera shake, each optional. */
USTRUCT(BlueprintType)
struct FTDFXCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	TSoftObjectPtr<UNiagaraSystem> VFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	TSoftClassPtr<UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX", meta = (ClampMin = "0"))
	float ShakeScale = 1.f;

	/** World-space radius within which the local player's camera shakes; 0 = always. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX", meta = (ClampMin = "0"))
	float ShakeRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|FX")
	FVector VFXScale = FVector::OneVector;

	bool IsEmpty() const { return VFX.IsNull() && Sound.IsNull() && Shake.IsNull(); }
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDShipVisuals : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FLinearColor SquadColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UTexture2D> HUDIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue EngineTrail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue BoostStart;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue BoostLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue DriftLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue DriftRelease;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	TSoftObjectPtr<USoundBase> EngineLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	FTDFXCue CollisionScrape;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch")
	FTDFXCue SwitchIn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch")
	FTDFXCue SwitchOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue PowerWindup;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue PowerActive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue PowerEnd;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	FTDFXCue Power2Active;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue ShieldHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue ShieldBreak;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue HullHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue LowHullLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue Explosion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	FTDFXCue RespawnWarpIn;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDWeaponFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FTDFXCue Muzzle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FTDFXCue ChargeLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire")
	FTDFXCue Overheat;

	/** Attached to the projectile for its lifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSoftObjectPtr<UNiagaraSystem> Tracer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue ImpactHull;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue ImpactShield;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue ImpactWorld;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue Detonation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impact")
	FTDFXCue Deflected;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDEnemyVisuals : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue EngineTrail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue WarpIn;

	/** Plays for the telegraph duration before a big attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Telegraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue ShieldHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue HullHit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Stunned;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Explosion;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDAbilityFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Activate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Loop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue End;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue AffectedTarget;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDTravelFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveSpool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveTunnel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hyperdrive")
	FTDFXCue HyperdriveDropOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalOpen;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalIdle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalTransit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FTDFXCue PortalClose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue AtmosphericEntry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue LandingDust;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atmosphere")
	FTDFXCue TakeOff;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp")
	FTDFXCue EnemyWarpIn;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDBladeFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FLinearColor BladeColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Ignite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Retract;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue SwingLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue SwingHeavy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Hit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Block;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue Deflect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue PerfectDeflect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue GuardBreak;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	FTDFXCue AirSlam;
};

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDUIStyle : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
	TMap<ETDShipId, FLinearColor> SquadColors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
	FLinearColor EnemyColor = FLinearColor(1.f, 0.12f, 0.24f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icons")
	TMap<ETDAbilityId, TSoftObjectPtr<UTexture2D>> AbilityIcons;
};
