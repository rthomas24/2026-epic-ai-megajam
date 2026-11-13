// Triad Drift - first-minute teaching: one control hint at a time, completed by doing it. Owner: Teammate 1 (Input).
//
// The core sequence starts with wave 1: aim / throttle, fire, boost, dodge, switch ship, signature power, target cycle.
// Later hints start the first time they apply: drift (the wave 2 tip), hyperdrive (the first planet marker), each
// unlocked ability. A hint completes when the player actually does it; any action the player already did before its
// hint came up is never shown. A hint that is ignored for TimeoutSeconds counts as seen. Each hint shows at most once
// per profile (save slot "TD_Hints"); SetHintsEnabled(false) (a menu setting) or TD.Hints 0 turns them off.
//
// The text reaches the screen through ITDHUDData::GetInteractPrompt: ATDSquadManager returns GetHintText() when no
// landing prompt or switch refusal is up, so a more important prompt always wins. Wording follows the last input
// device (keyboard or gamepad).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TDHintComponent.generated.h"

class ATDShip;

UENUM(BlueprintType)
enum class ETDHint : uint8
{
	None,
	Fly,
	Fire,
	Boost,
	Dodge,
	Switch,
	Power,
	TargetCycle,
	Drift,
	Hyperdrive,
	Ability
};

/** The hint profile (slot "TD_Hints"): which hints were seen, and the on/off setting. Separate from the run save. */
UCLASS()
class UNREALGAMEJAM_API UTDHintSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	TArray<ETDHint> Done;

	UPROPERTY(SaveGame)
	bool bEnabled = true;
};

UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDHintComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDHintComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The hint to show right now for the first local player, or empty. ATDSquadManager::GetInteractPrompt calls this last. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints", meta = (WorldContext = "WorldContextObject"))
	static FText GetHintText(const UObject* WorldContextObject);

	/** The hint showing now (None between hints, while suppressed or when disabled). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints")
	ETDHint GetCurrentHint() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints")
	FText GetCurrentText() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints")
	bool IsHintDone(ETDHint Hint) const { return Done.Contains(Hint); }

	/** Menu setting: hints on or off (saved with the hint profile). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hints")
	void SetHintsEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints")
	bool AreHintsEnabled() const { return bEnabled; }

	/** Forget every completed hint (TD.Hints.Reset, a new profile). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Hints")
	void ResetHints();

	/** Tests: start clean and never read or write the save slot. */
	void UseMemoryOnlyForTests();

	/** Queue a contextual hint now (also driven by the event bus: tips, ability unlocks, wave 1). */
	void Offer(ETDHint Hint);

	/** The wording of a hint for a device (tests and docs). */
	static FText TextFor(ETDHint Hint, bool bGamepad, ETDAbilityId Ability);

	/** Seconds between two hints. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float GapSeconds = 1.2f;

	/** A hint the player ignores this long counts as seen (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "1"))
	float TimeoutSeconds = 20.f;

	/** Seconds after wave 1 starts before the first hint (lets the slate card and the first enemies land). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float FirstHintDelay = 2.f;

	/** Seconds of steering (aim input over 25%) that complete the Fly hint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float FlySecondsToComplete = 1.f;

private:
	ATDSpacePlayerController* GetPC() const;
	ATDShip* GetFlownShip() const;
	bool IsSuppressed() const;
	void Complete(ETDHint Hint);
	void Observe(ETDHint Hint);
	void Load();
	void Save() const;
	void BindBus();

	UFUNCTION() void HandleWaveStarted(int32 WaveNumber, float Intensity01);
	UFUNCTION() void HandleShipSwitched(ETDShipId NewShip, ETDShipId OldShip);
	UFUNCTION() void HandleAbilityUnlocked(ETDAbilityId Ability, ETDShipId Ship);
	UFUNCTION() void HandleTip(FName TipId);
	UFUNCTION() void HandleWeaponInput(ETDWeaponSlot Slot, bool bHeld);
	UFUNCTION() void HandlePowerInput(ETDPowerInput Power);
	UFUNCTION() void HandleTargetCycle();

	TSet<ETDHint> Done;
	TArray<ETDHint> Queue;
	ETDHint Current = ETDHint::None;
	float CurrentAge = 0.f;
	float Gap = 0.f;
	float FlySeconds = 0.f;
	ETDAbilityId LastAbility = ETDAbilityId::None;
	bool bEnabled = true;
	bool bPersist = true;
	bool bBusBound = false;
};
