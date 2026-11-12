// Triad Drift - registry of every live damageable actor, by team. Lead-owned.
// Use this instead of GetAllActorsOfClass for targeting, AI and wave counting.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDCombatRegistry.generated.h"

UCLASS()
class UNREALGAMEJAM_API UTDCombatRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDCombatRegistry* Get(const UObject* WorldContextObject);

	/** Called by UTDHealthComponent. Safe to call twice. */
	void Register(AActor* Actor);
	void Unregister(AActor* Actor);

	/** Live (not dead) actors on Team. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Combat")
	void GetActorsOfTeam(ETDTeam Team, TArray<AActor*>& OutActors, bool bIncludeUntargetable = false) const;

	/** Live actors hostile to Team (Player <-> Enemy; Neutral is hostile to nobody). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Combat")
	void GetHostilesOf(ETDTeam Team, TArray<AActor*>& OutActors, bool bIncludeUntargetable = false) const;

	/** Nearest live, targetable hostile within MaxDistance (<= 0 = unlimited). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Combat")
	AActor* FindNearestHostile(ETDTeam Team, const FVector& Location, float MaxDistance = 0.f, const AActor* Ignore = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Combat")
	int32 CountAlive(ETDTeam Team) const;

	/** Every registered actor (alive or not), for iteration without allocation. */
	const TArray<TWeakObjectPtr<AActor>>& GetAll() const { return Actors; }

private:
	TArray<TWeakObjectPtr<AActor>> Actors;
};
