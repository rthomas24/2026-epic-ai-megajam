// Triad Drift - Enemies & Waves (Teammate 4). Point-defense / Dominion turret (ENG-13, reused by ENG-18).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Enemies/TDEnemyTypes.h"
#include "TDEnemyTurret.generated.h"

class UTDHealthComponent;
class UTDWeaponComponent;
class UStaticMeshComponent;
class ATDEnemyTurret;
class UTDEnemyVisuals;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDTurretSignature, ATDEnemyTurret*, Turret);

/**
 * A destructible gun turret. Tracks the nearest squad ship in range (combat registry), turns at
 * TurnRateDeg, and fires Guns when lined up. If Guns.TelegraphSeconds > 0 every burst is telegraphed
 * (Dominion charge shots). Mounted on a carrier, or placed standalone.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDEnemyTurret : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDEnemyTurret();

	void Init(const FTDEnemyAttackSpec& InGuns, float Hull);

	/** Difficulty / power multiplier on the turret's outgoing damage. */
	void SetDamageMultiplier(float Multiplier);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Turret")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Turret")
	bool IsTelegraphing() const { return bTelegraphing; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Turret")
	AActor* GetTarget() const { return Target.Get(); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Turret")
	void ApplyStun(float Seconds);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Turret")
	bool IsStunned() const { return StunRemaining > 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Turret")
	UTDHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Turret")
	int32 GetBurstCount() const { return BurstCount; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Turret")
	FTDEnemyAttackSpec Guns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Turret", meta = (ClampMin = "0"))
	float TurnRateDeg = 120.f;

	/** Seconds the guns stay on per burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Turret", meta = (ClampMin = "0"))
	float BurstSeconds = 1.0f;	// ENG-27: carrier PD was the top damage source in wave 5 at 1.5 s

	/** Pause between bursts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Turret", meta = (ClampMin = "0"))
	float BurstCooldownSeconds = 2.0f;	// ENG-27: 1.0 made carrier point defense a death zone

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Turret", meta = (ClampMin = "0"))
	float StartHull = 60.f;

	/** TurretTelegraph (falls back to Telegraph), TurretDestroyed. Set by the owner, or per placed turret. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDEnemyVisuals> Visuals;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Turret")
	FTDTurretSignature OnTelegraphStarted;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Turret")
	FTDTurretSignature OnTurretDestroyed;

	// ITDTeam
	virtual ETDTeam GetTeam() const override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Turret")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Turret")
	TObjectPtr<UTDHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Turret")
	TObjectPtr<UTDWeaponComponent> Weapons;

	TWeakObjectPtr<AActor> Target;
	float RetargetTimer = 0.f;
	float CooldownRemaining = 0.f;
	float TelegraphRemaining = 0.f;
	float BurstRemaining = 0.f;
	float StunRemaining = 0.f;
	int32 BurstCount = 0;
	bool bTelegraphing = false;
	bool bInitialized = false;
	bool bDestroyed = false;
};
