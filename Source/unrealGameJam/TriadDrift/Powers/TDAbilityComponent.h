// Triad Drift - Powers (Teammate 2). The unlockable-ability slot on each squad ship (ENG-12).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDAbilityComponent.generated.h"

class UTDPowerComponent;

/**
 * Holds which ability (Gravity Well / Cloak / EMP Pulse) this ship has and owns its power component
 * (Slot = Ability). Filled from ITDGameFlow::GetProgress().UnlockedAbilities on BeginPlay and from
 * UTDEventBus::OnAbilityUnlocked. HUD: GetAbility(), GetAbilityPower()->GetCooldown01().
 */
UCLASS(Blueprintable, ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDAbilityComponent();

	/** Installs Ability (None removes it). Replaces any previous ability power. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ability")
	void SetAbility(ETDAbilityId Ability);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ability")
	ETDAbilityId GetAbility() const { return Ability; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ability")
	UTDPowerComponent* GetAbilityPower() const { return Power; }

	/** HUD readiness: 0 while no ability is earned (locked), else the ability power's GetCooldown01 (1 = ready). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ability")
	float GetReadiness01() const;

	/** True once an ability has been earned and installed on this ship. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ability")
	bool HasAbility() const { return Ability != ETDAbilityId::None && Power != nullptr; }

	/** Re-read the unlocks from game flow (respawn, checkpoint restore). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ability")
	void RefreshFromProgress();

	/**
	 * The player pressed the ability input on this ship with nothing earned: Bark_AbilityLocked by the flown pilot, once,
	 * then again only after LockedBarkRearmSeconds of real time (a player mashing E hears it once).
	 */
	void NoteLockedPress();

	/** Bark_AbilityLocked lines asked for so far (tests). */
	int32 GetLockedBarks() const { return LockedBarks; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ability", meta = (Units = "s"))
	float LockedBarkRearmSeconds = 12.f;

	/** Which power class implements each ability. Defaults set in the constructor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ability")
	TMap<ETDAbilityId, TSubclassOf<UTDPowerComponent>> AbilityClasses;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleAbilityUnlocked(ETDAbilityId UnlockedAbility, ETDShipId Ship);

	bool GetOwnerShip(ETDShipId& OutShip) const;

	UPROPERTY(Transient)
	TObjectPtr<UTDPowerComponent> Power;

	ETDAbilityId Ability = ETDAbilityId::None;
	double LastLockedBark = -1e9;
	int32 LockedBarks = 0;
};
