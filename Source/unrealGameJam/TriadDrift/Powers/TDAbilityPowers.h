// Triad Drift - Powers (Teammate 2). Unlockable abilities (ENG-12): Gravity Well, Cloak, EMP Pulse.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TDAbilityPowers.generated.h"

class UTDAbilityFX;
class UNiagaraComponent;

/** The Gravity Well orb / vortex. Spawned by UTDGravityWellPower. */
UCLASS(NotBlueprintable)
class UNREALGAMEJAM_API ATDGravityWell : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDGravityWell();

	virtual ETDTeam GetTeam() const override { return Team; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|GravityWell")
	bool IsVortexActive() const { return bVortex; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|GravityWell")
	float GetRadius() const { return Radius; }

	// --- Tuning (set by the power before BeginPlay-time Launch) ---
	float OrbSpeed = 5000.f;
	float OrbSeconds = 0.8f;
	float Radius = 5000.f;
	float VortexSeconds = 5.f;
	float DamagePerSecond = 10.f;
	float DamageTickSeconds = 0.25f;
	float ImplosionDamage = 120.f;
	/** Pull speed at the edge and at the centre (cm/s); enemies stop being pulled inside CoreRadius. */
	float EdgePullSpeed = 1500.f;
	float CorePullSpeed = 3500.f;
	float CoreRadius = 400.f;
	/** How fast hostile projectiles' velocity bends toward the centre. */
	float ProjectileBendDegPerSecond = 270.f;

	void Launch(AActor* InOwner, ETDTeam InTeam, const FVector& Direction, UTDAbilityFX* InFX);

	virtual void Tick(float DeltaSeconds) override;

private:
	void TickVortex(float DeltaSeconds);
	void Implode();

	UPROPERTY(Transient)
	TObjectPtr<UTDAbilityFX> FX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> LoopFX;

	ETDTeam Team = ETDTeam::Player;
	FVector Direction = FVector::ForwardVector;
	float Age = 0.f;
	float DamageAccumulator = 0.f;
	bool bVortex = false;
	bool bLaunched = false;
};

/** Bastion (Verdance Obj 1): slow orb -> 5000 cm vortex for 5 s, 10 dps + no firing inside, 120 implosion. 18 s. */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDGravityWellPower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDGravityWellPower();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|GravityWell", meta = (Units = "cm"))
	float Radius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|GravityWell", meta = (Units = "s"))
	float VortexSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|GravityWell")
	float DamagePerSecond = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|GravityWell")
	float ImplosionDamage = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|GravityWell", meta = (Units = "CentimetersPerSecond"))
	float OrbSpeed = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|GravityWell", meta = (Units = "s"))
	float OrbSeconds = 0.8f;

	ATDGravityWell* GetLastWell() const { return LastWell.Get(); }

protected:
	virtual bool OnActivate() override;

private:
	TWeakObjectPtr<ATDGravityWell> LastWell;
};

/** Rift (Verdance Obj 2): 6 s untargetable; first shot out does 3x; firing or using another power breaks it. 15 s. */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDCloakPower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDCloakPower();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Cloak")
	bool IsCloaked() const { return IsPowerActive(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cloak")
	float FirstShotMultiplier = 3.f;

protected:
	virtual bool OnActivate() override;
	virtual void OnEnd(bool bInterrupted) override;

private:
	UFUNCTION()
	void HandleWeaponFired(ETDWeaponSlot WeaponSlot, FName WeaponId);

	UFUNCTION()
	void HandleOtherPower(UTDPowerComponent* Power);
};

/** Lancer (Cinder Obj 3): 6000 cm pulse; stun 3 s, gunship shields down 5 s, missiles fizzle. 14 s. */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDEMPPower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDEMPPower();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EMP", meta = (Units = "cm"))
	float Radius = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EMP", meta = (Units = "s"))
	float StunSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|EMP", meta = (Units = "s"))
	float ShieldDropSeconds = 5.f;

	/** Enemies affected by the last pulse (tests). */
	int32 GetLastAffectedCount() const { return LastAffected; }

protected:
	virtual bool OnActivate() override;

private:
	int32 LastAffected = 0;
};
