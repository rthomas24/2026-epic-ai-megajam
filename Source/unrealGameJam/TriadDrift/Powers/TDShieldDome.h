// Triad Drift - Powers (Teammate 2). Bastion's shield dome (ENG-11).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TDShieldDome.generated.h"

class UTDAbilityFX;
class ATDProjectile;
class UNiagaraComponent;

/**
 * A sphere that follows its owner slowly. Squad ships inside take no damage and regenerate shields
 * at 2x; hostile shots crossing it are destroyed, hostile missiles are deflected back at their shooter.
 * Query from anywhere with UTDPowerSubsystem::FindDomeCovering / GetNearestDome.
 */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDShieldDome : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDShieldDome();

	static ATDShieldDome* SpawnDome(UWorld* World, AActor* InOwner, ETDTeam InTeam, float InRadius, float InLifetime, UTDAbilityFX* InFX = nullptr);

	virtual ETDTeam GetTeam() const override { return Team; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dome")
	bool Covers(const FVector& Location) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dome")
	bool IsDomeActive() const { return bDomeActive; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dome")
	float GetRadius() const { return Radius; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dome")
	float GetRemaining01() const { return Lifetime > 0.f ? FMath::Clamp(1.f - Age / Lifetime, 0.f, 1.f) : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Dome")
	void Collapse();

	/** Squad inside take no damage and regen 2x (off for Blink's reflect field). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dome")
	bool bProtectSquad = true;

	/** Hostile non-missile shots crossing the dome are destroyed (off for Blink's reflect field). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dome")
	bool bDestroyShots = true;

	/**
	 * Follows its owner with an exponential lag (1/s): it trails a cruising Bastion (4800 cm/s) by ~1200 cm and a
	 * boosting one by ~2100 cm, so Bastion never leaves its own 3000 cm dome. Higher = tighter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dome")
	float FollowLagRate = 4.f;

	/** Shield shell hit by a hostile projectile at ShellPoint: destroy it there, or deflect a homing missile. */
	void ReceiveProjectile(ATDProjectile* Projectile, const FVector& ShellPoint);

	/** This dome stops (or reflects) Projectile: hostile, and a shot it destroys or a missile it deflects. */
	bool Intercepts(const ATDProjectile* Projectile) const;

	/** Extra shield regeneration inside, as a multiple of the ship's own rate (2x total = 1 extra). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dome")
	float ExtraShieldRegenMultiplier = 1.f;

	/** Invulnerability window refreshed every tick for ships inside (short, so leaving the dome ends it at once). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dome", meta = (Units = "s"))
	float ProtectionRefreshSeconds = 0.1f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ProtectSquad(float DeltaSeconds);
	void InterceptProjectiles();

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> LoopFX;

	UPROPERTY(Transient)
	TObjectPtr<UTDAbilityFX> FX;

	bool bPlaceholderBuilt = false;
	/** Reused every tick (no per-frame allocation). */
	TArray<AActor*> FriendlyScratch;
	TWeakObjectPtr<AActor> FollowTarget;
	ETDTeam Team = ETDTeam::Player;
	float Radius = 3000.f;
	float Lifetime = 6.f;
	float Age = 0.f;
	bool bDomeActive = false;
};
