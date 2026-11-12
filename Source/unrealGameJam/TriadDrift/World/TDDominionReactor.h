// Triad Drift - World (Teammate 4). ENG-18 Dominion reactor (core ending target).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TDDominionReactor.generated.h"

class UStaticMeshComponent;
class UTDHealthComponent;
class UTDDominionVisuals;
class ATDDominionReactor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDReactorPhaseSignature, ATDDominionReactor*, Reactor, int32, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDReactorDestroyedSignature, ATDDominionReactor*, Reactor, AActor*, Killer);

/**
 * The reactor at the bottom of the Dominion's trench. Immune (and untargetable) until exposed.
 * Three-phase health bar: each phase is PhaseHull hull; crossing into the next phase fires
 * OnPhaseChanged (the Dominion escalates turret fire and launches) and grants a short
 * invulnerability beat so the phase change reads.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDDominionReactor : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDDominionReactor();

	/** Shield up = no damage, not targetable. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Dominion")
	void SetExposed(bool bInExposed);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	bool IsExposed() const { return bExposed; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	bool IsAlive() const;

	/** 1..PhaseCount (PhaseCount + 1 once destroyed). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	int32 GetPhase() const { return Phase; }

	/** Health within the current phase, 1 -> 0, for the 3-segment boss bar. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	float GetPhaseHealth01() const;

	/** Whole reactor, 1 -> 0. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	float GetTotalHealth01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Dominion")
	UTDHealthComponent* GetHealth() const { return Health; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Dominion", meta = (ClampMin = "1"))
	int32 PhaseCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Dominion", meta = (ClampMin = "1"))
	float PhaseHull = 1200.f;

	/** Invulnerability after each phase change (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Dominion", meta = (ClampMin = "0"))
	float PhaseChangeInvulnerableSeconds = 1.5f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Dominion")
	FTDReactorPhaseSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Dominion")
	FTDReactorDestroyedSignature OnReactorDestroyed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDDominionVisuals> Visuals;

	virtual ETDTeam GetTeam() const override { return ETDTeam::Enemy; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	float FilterDamage(const FTDDamageInfo& Info);

	UFUNCTION()
	void HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

	UFUNCTION()
	void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Dominion")
	TObjectPtr<UTDHealthComponent> Health;

	int32 Phase = 1;
	bool bExposed = false;
	bool bDestroyed = false;
};
