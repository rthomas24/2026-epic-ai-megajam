// Triad Drift - Enemies & Waves (Teammate 4). Maul Bomber plasma mine (ENG-13).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TDPlasmaMine.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTDHealthComponent;
class ATDPlasmaMine;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDMineSignature, ATDPlasmaMine*, Mine);

/**
 * A drifting plasma mine. Arms after ArmSeconds, detonates when a hostile ship enters TriggerRadius
 * (Damage over SplashRadius, explosive), fizzles after Lifetime. Shootable (1 HP): Flak clears
 * mine fields. Gravity Well may move it (it simulates no physics; it just has a velocity).
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDPlasmaMine : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDPlasmaMine();

	void Init(AActor* InOwnerShip, ETDTeam InTeam, float InDamage, float InSplashRadius, float InTriggerRadius, float InArmSeconds, float InLifetime, const FVector& InVelocity);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Mine")
	bool IsArmed() const { return bArmed; }

	/** Detonate now (proximity, shot, or forced). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Mine")
	void Detonate();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Mine")
	void SetDriftVelocity(const FVector& InVelocity) { Velocity = InVelocity; }

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Mine")
	FTDMineSignature OnArmed;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Mine")
	FTDMineSignature OnDetonated;

	// ITDTeam
	virtual ETDTeam GetTeam() const override { return Team; }

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Mine")
	TObjectPtr<UStaticMeshComponent> Mesh;

	TWeakObjectPtr<AActor> OwnerShip;
	ETDTeam Team = ETDTeam::Enemy;
	FVector Velocity = FVector::ZeroVector;
	float Damage = 60.f;
	float SplashRadius = 800.f;
	float TriggerRadius = 450.f;
	float ArmRemaining = 0.75f;
	float LifeRemaining = 20.f;
	float ProximityCheckTimer = 0.f;
	bool bArmed = false;
	bool bDetonated = false;
};
