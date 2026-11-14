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
#include "TriadDrift/Story/TDTutorialPhases.h"
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

/** Where the hint on screen is in its life: being shown, or just done by the player (a short tick-off beat). */
UENUM(BlueprintType)
enum class ETDHintState : uint8
{
	None,
	Showing,
	Done
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

	/** The hint showing now (None between hints, while suppressed or when disabled). During its tick-off beat it is
	 * still returned, with GetCurrentHintState() == Done. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints")
	ETDHint GetCurrentHint() const;

	/** Showing, or Done for DoneBeatSeconds after the player did it (the HUD ticks it off), or None. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints")
	ETDHintState GetCurrentHintState() const;

	/** Onboarding (T3): the four essentials (fly, fire, switch ship, power), how many are done of how many. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints")
	void GetEssentialsProgress(int32& OutDone, int32& OutTotal) const;

	/** Static HUD helpers: the first local player's hint state and essentials progress (0 / 0 when no hints). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints", meta = (WorldContext = "WorldContextObject"))
	static ETDHintState GetHintStateFor(const UObject* WorldContextObject);
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints", meta = (WorldContext = "WorldContextObject"))
	static void GetEssentialsProgressFor(const UObject* WorldContextObject, int32& OutDone, int32& OutTotal);

	/**
	 * T2's onboarding card (ENG-36 lower lane): the current hint split into its key glyph for the device ("LMB", "Q",
	 * "RB") and its action words ("FIRE"); the essentials step ("2 / 4"), false outside the four essentials; and the
	 * state (Done for DoneBeatSeconds after the player does it, for the tick).
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints", meta = (WorldContext = "WorldContextObject"))
	static FText GetHintKey(const UObject* WorldContextObject);
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints", meta = (WorldContext = "WorldContextObject"))
	static FText GetHintAction(const UObject* WorldContextObject);
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints", meta = (WorldContext = "WorldContextObject"))
	static bool GetHintStep(const UObject* WorldContextObject, int32& OutStep, int32& OutCount);
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Hints", meta = (WorldContext = "WorldContextObject"))
	static ETDHintState GetHintState(const UObject* WorldContextObject) { return GetHintStateFor(WorldContextObject); }

	/** The key glyph and the action words of a hint for a device (tests and docs). */
	static FText KeyFor(ETDHint Hint, bool bGamepad);
	static FText ActionFor(ETDHint Hint, ETDAbilityId Ability);

	/** Whether this hint was missed once already (a second timeout marks it seen). */
	bool WasMissed(ETDHint Hint) const { return Missed.Contains(Hint); }

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

	/** Dev (TD.Hints.Show / TD.Hints.Next, captures): this hint is the one on screen until cleared with None, whatever
	 *  the queue, the teach phase or the suppression say. Nothing is saved. */
	void DebugPinHint(ETDHint Hint) { DebugPinned = Hint; }
	ETDHint GetDebugPinnedHint() const { return DebugPinned; }

	/** The wording of a hint for a device (tests and docs). */
	static FText TextFor(ETDHint Hint, bool bGamepad, ETDAbilityId Ability);

	/** Seconds between two hints. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float GapSeconds = 1.2f;

	/** A hint the player ignores this long goes to the back of the queue once; the second time it counts as seen (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "1"))
	float TimeoutSeconds = 20.f;

	/** Seconds after wave 1 starts before the first hint (lets the slate card and the first enemies land). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float FirstHintDelay = 2.f;

	/** Seconds a done hint stays up as "done" before the next one (the HUD's tick-off). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float DoneBeatSeconds = 0.5f; // T2's card: a 0.4 s tick, then the fade

	/** Longest a waiting hint is held back by story transmissions (lines queued back to back), in seconds; then it
	 *  shows anyway. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float MaxStoryHoldSeconds = 8.f;

	/** Seconds of steering (aim input over 25%) that complete the Fly hint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Hints", meta = (ClampMin = "0"))
	float FlySecondsToComplete = 0.7f;
	ETDHint DebugPinned = ETDHint::None;

private:
	ATDSpacePlayerController* GetPC() const;
	ATDShip* GetFlownShip() const;
	bool IsSuppressed() const;
	void Complete(ETDHint Hint, bool bByPlayer = false);
	void MissCurrent();
	bool IsContinuedRun() const;
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
	TSet<ETDHint> Missed; // timed out once this session (not saved)
	ETDHint JustDone = ETDHint::None;
	float JustDoneLeft = 0.f;
	TArray<ETDHint> Queue;
public:
	/** T3's ENG-40 phases: a control lesson is shown only in its phase (Evade: Fly, Boost, Dodge; TargetAndFire: Fire,
	 *  TargetCycle; SquadSystems: Switch, Power). Other hints (drift, hyperdrive, abilities) any time; no phase = today's order. */
	static bool FitsTeachPhase(ETDHint Hint, ETDTeachPhase Phase);
	/** Tests: this phase instead of UTDTutorialPhases' (None = use the real one). */
	TOptional<ETDTeachPhase> TeachPhaseOverrideForTests;
	ETDTeachPhase CurrentTeachPhase() const;
private:
	ETDHint Current = ETDHint::None;
	float CurrentAge = 0.f;
	float Gap = 0.f;
	float StoryHeldSeconds = 0.f; // how long story lines have held the waiting hint (capped by MaxStoryHoldSeconds)
	float FlySeconds = 0.f;
	ETDAbilityId LastAbility = ETDAbilityId::None;
	bool bEnabled = true;
	bool bPersist = true;
	bool bBusBound = false;
};
