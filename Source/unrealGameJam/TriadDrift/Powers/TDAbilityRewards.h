// Triad Drift - the reward moment of an ability unlock (board N-070, Nick's UI; Teammate 2).
//
// When an objective awards an ability (Gravity Well on Verdance, for Bastion), the UI shows a reward screen at once,
// and a flight hint later, the next time the player flies the ship that owns it. This subsystem holds that state, one
// entry per award, for the whole session (a GameInstance subsystem: it crosses the planet -> galaxy travel). It never
// touches progression or checkpoints; it only listens to UTDEventBus::OnAbilityUnlocked.
//
//   UI reward screen:  GetPendingAbilityReward(Ability, Ship, bCanUseNow) -> show -> MarkAbilityRewardPresented(Ability)
//   flight hint:       GetAbilityFlightHint(Ability) while the owning ship is flown -> show -> MarkAbilityFlightHintShown(Ability)
// Each award has its own presented flag and its own hint flag: a later unlock is never suppressed by an earlier one.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDAbilityRewards.generated.h"

USTRUCT(BlueprintType)
struct FTDAbilityReward
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Abilities") ETDAbilityId Ability = ETDAbilityId::None;
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Abilities") ETDShipId Ship = ETDShipId::Rift;
	/** The reward screen has been shown for this award. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Abilities") bool bPresented = false;
	/** The flight hint has been shown for this award. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Abilities") bool bFlightHintShown = false;
};

UCLASS()
class UNREALGAMEJAM_API UTDAbilityRewardSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UTDAbilityRewardSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * The oldest award whose reward screen has not been shown. bCanUseNow: the player is flying Ship right now (on foot,
	 * or in another ship, it is false). WorldContext picks the world whose player is asked (null: the game instance's).
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Abilities", meta = (WorldContext = "WorldContext"))
	bool GetPendingAbilityReward(ETDAbilityId& Ability, ETDShipId& Ship, bool& bCanUseNow, const UObject* WorldContext = nullptr) const;

	/** The reward screen for this award has been shown (once per award). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Abilities")
	void MarkAbilityRewardPresented(ETDAbilityId Ability);

	/** An award whose flight hint is due: its owning ship is the one flown now, and its hint has not been shown. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Abilities", meta = (WorldContext = "WorldContext"))
	bool GetAbilityFlightHint(ETDAbilityId& Ability, const UObject* WorldContext = nullptr) const;

	/** The flight hint for this award has been shown (its own flag; other awards keep theirs). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Abilities")
	void MarkAbilityFlightHintShown(ETDAbilityId Ability);

	const TArray<FTDAbilityReward>& GetRewards() const { return Rewards; }
	/** Tests and a new run: forget every award. */
	void ResetRewards() { Rewards.Reset(); }

	/** The ship the player flies now in that world's first player controller, or false on foot / no pawn. */
	static bool GetFlownShip(const UWorld* World, ETDShipId& OutShip);

private:
	UFUNCTION() void HandleAbilityUnlocked(ETDAbilityId Ability, ETDShipId Ship);
	const UWorld* ResolveWorld(const UObject* WorldContext) const;

	UPROPERTY() TArray<FTDAbilityReward> Rewards;
};
