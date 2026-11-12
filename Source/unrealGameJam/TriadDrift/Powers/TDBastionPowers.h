// Triad Drift - Powers (Teammate 2). Bastion: Missile Swarm (tap Q) and Shield Dome (hold Q) (ENG-11).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TDBastionPowers.generated.h"

class ATDShieldDome;
class UTDTargetingComponent;

/**
 * 12 micro-missiles split across the targets painted by the ship's UTDTargetingComponent (or the soft
 * target), 18 damage + 200 cm splash each, 9 s cooldown. While the player flies Bastion and the swarm
 * is ready, lock painting runs automatically (hold-Q is the dome, so painting can't need a held button).
 */
UCLASS(Blueprintable, ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDMissileSwarmPower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDMissileSwarmPower();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Swarm")
	int32 MissileCount = 12;

	/** Half-angle the swarm fans out in before the missiles turn onto their targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Swarm", meta = (Units = "Degrees"))
	float LaunchConeDeg = 35.f;

	/** Paint automatically while ready and player-flown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Swarm")
	bool bAutoPaint = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Swarm")
	FTDProjectileSpec MissileSpec;

	/** Targets the last swarm was split across (tests, HUD). */
	const TArray<TWeakObjectPtr<AActor>>& GetLastTargets() const { return LastTargets; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivate() override;

private:
	UTDTargetingComponent* GetTargeting() const;
	TArray<TWeakObjectPtr<AActor>> LastTargets;
};

/** Hold Q: a 3000 cm dome for 6 s, 16 s cooldown (counted from when it collapses). */
UCLASS(Blueprintable, ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDShieldDomePower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDShieldDomePower();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dome", meta = (Units = "cm"))
	float DomeRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dome", meta = (Units = "s"))
	float DomeSeconds = 6.f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dome")
	ATDShieldDome* GetDome() const { return Dome.Get(); }

protected:
	virtual bool OnActivate() override;
	virtual void OnEnd(bool bInterrupted) override;

private:
	TWeakObjectPtr<ATDShieldDome> Dome;
};
