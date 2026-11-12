// Triad Drift - Powers (Teammate 2). World-level power bookkeeping: active shield domes, portal re-entry cooldowns.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDPowerSubsystem.generated.h"

class ATDShieldDome;

UCLASS()
class UNREALGAMEJAM_API UTDPowerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDPowerSubsystem* Get(const UObject* WorldContext);

	// --- Shield domes (ENG-11). T3's wingmen: move toward FindDomeCovering / GetNearestDome when hurt. ---
	void RegisterDome(ATDShieldDome* Dome);
	void UnregisterDome(ATDShieldDome* Dome);

	/** An active dome of Team whose sphere contains Location, or null. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	ATDShieldDome* FindDomeCovering(const FVector& Location, ETDTeam Team = ETDTeam::Player) const;

	/** Nearest active dome of Team, or null. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	ATDShieldDome* GetNearestDome(const FVector& Location, ETDTeam Team = ETDTeam::Player) const;

	const TArray<TObjectPtr<ATDShieldDome>>& GetActiveDomes() const { return Domes; }

	// --- Portal transit re-entry ---
	void MarkTransited(const AActor* Actor);
	bool IsInTransitCooldown(const AActor* Actor, float CooldownSeconds) const;

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDShieldDome>> Domes;

	TMap<TWeakObjectPtr<const AActor>, double> LastTransit;
};
