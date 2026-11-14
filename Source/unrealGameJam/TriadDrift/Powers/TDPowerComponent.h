// Triad Drift - Powers (Teammate 2). Base for signature powers and unlockable abilities (ENG-09..12).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TDPowerComponent.generated.h"

class UTDPowerComponent;
class UTDAbilityFX;
class UTDShipVisuals;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnPowerActivated, UTDPowerComponent*, Power);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnPowerEnded, UTDPowerComponent*, Power, bool, bInterrupted);

/**
 * One power slot on a ship (Signature1 = Q tap, Signature2 = Bastion hold-Q, Ability = E).
 * Lifecycle: TryActivate -> (optional wind-up) -> active for ActiveSeconds (0 = instant) -> end -> cooldown.
 * Keeps running after the player switches away (ENG-07). Wingmen never call it.
 *
 * T1's controller: UTDPowerComponent::RoutePowerInput(PossessedShip, ETDPowerInput, bPressed).
 * T3's HUD: FindPower(Ship, Slot)->GetCooldown01() / GetPowerName(); Switch Strike listens to
 * UTDEventBus::OnPowerUsed, which every successful activation broadcasts.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (TriadDrift))
class UNREALGAMEJAM_API UTDPowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDPowerComponent();

	// --- Contract ---
	/** Start the power if ready. Returns true if it started. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Power")
	bool TryActivate();

	// --- #201 WINGMAN AI ENTRY (wingman builder): the squad's wingman AI calls this, nothing else does ---
	/** TryActivate for an AI pilot: every rule of TryActivate except bRequirePlayerControl. */
	bool TryActivateFromAI();
	/** The current (or last) activation was started by a wingman's AI. */
	bool WasStartedByAI() const { return bStartedByAI; }
	// --- end #201 ---

	/** 1 = ready, 0 = just used (stays 0 while active when the cooldown starts at the end). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	float GetCooldown01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	float GetCooldownRemaining() const { return CooldownRemaining; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	FName GetPowerName() const { return PowerName; }

	/** The power is running (UActorComponent::IsActive is component activation, so this is named differently). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	bool IsPowerActive() const { return bPowerActive; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	virtual bool CanActivate() const;

	/** Wind-up progress for animation (Lancer wing fold). 0 when not winding up. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	virtual float GetWindup01() const { return 0.f; }

	/** Seconds the power has been active. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	float GetActiveElapsed() const { return ActiveElapsed; }

	/** End early (cancel, death). Cooldown still applies. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Power")
	void EndPower(bool bInterrupted = true);

	/** End any active phase safely and make the power ready (death, respawn, retake). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Power")
	void ResetPower() { EndPower(true); CooldownRemaining = 0.f; }

	/** Reset cooldown to ready (respawn, cheats). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Power")
	void ResetCooldown() { CooldownRemaining = 0.f; }

	/** Raw input edges. Default: press = TryActivate. Hold-style powers override. */
	virtual void HandleInputPressed();
	virtual void HandleInputReleased() {}

	// --- Lookup / routing ---
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Power")
	static UTDPowerComponent* FindPower(const AActor* Ship, ETDPowerInput Slot);

	/** Route one controller input edge to the ship's power in that slot. Returns true if a power handled it. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Power")
	static bool RoutePowerInput(AActor* Ship, ETDPowerInput Slot, bool bPressed);

	/** Where this power's active cue attaches (tests and diagnostics). */
	USceneComponent* GetActiveCueAttachRoot() const { return GetActiveCueRoot(); }

	// --- Tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	ETDPowerInput Slot = ETDPowerInput::Signature1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	FName PowerName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power", meta = (Units = "s"))
	float CooldownSeconds = 8.f;

	/** True: cooldown counts down only after the power ends (portals, dome). False: from activation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	bool bCooldownStartsWhenPowerEnds = false;

	/** Stays active until EndPower (portals: until both close). Overrides ActiveSeconds = 0 meaning instant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	bool bEndsManually = false;

	/** 0 = instant power (unless bEndsManually). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power", meta = (Units = "s"))
	float ActiveSeconds = 0.f;

	/** End the power when its ship dies (ram, cloak). Dome and portals keep running for the rest of the squad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	bool bEndOnOwnerDeath = true;

	/** Only fires while the player flies this ship (features.md: wingmen never use powers). Tests turn it off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	bool bRequirePlayerControl = true;

	/**
	 * Presentation. Abilities: UTDAbilityFX (Activate / Loop / End / AffectedTarget), auto-loaded from
	 * DA_Ability_<Id>_FX when left empty. Signature powers play the ship's UTDShipVisuals cues
	 * (PowerWindup / PowerActive / Power2Active / PowerEnd) from DA_<Ship>_Visuals. All optional.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	TObjectPtr<UTDAbilityFX> FX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	TObjectPtr<UTDShipVisuals> ShipVisuals = nullptr;

	/** Which ability this power is (None for signature powers): picks DA_Ability_<Id>_FX. */
	virtual ETDAbilityId GetAbilityId() const { return ETDAbilityId::None; }

	/**
	 * dialogs.md: the pilot's line when the player starts this power (None = no line). Said once per activation, by the
	 * flown pilot, through the squad's bark limiter (FTDCombatBarks). Priority is uint8 ETDBarkPriority.
	 */
	virtual FName GetActivateBark(uint8& OutPriority) const { return NAME_None; }

	/** Bark_PowerNotReady for a press on cooldown: once per cooldown (tests read it). */
	bool HasSaidNotReady() const { return bSaidNotReady; }

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Power")
	FTDOnPowerActivated OnPowerActivated;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Power")
	FTDOnPowerEnded OnPowerEnded;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** Scene cut / new take: end now and be ready. */
	void HandlePowersReset();
	FDelegateHandle ResetHandle;

	/** Do the power. Return false to abort (no cooldown, no event). */
	virtual bool OnActivate() { return true; }
	virtual void OnTickActive(float DeltaTime) {}
	virtual void OnEnd(bool bInterrupted) {}
	/**
	 * The player switched away from this ship while the power was running (it is now AI-flown). Default: keep
	 * running on its own timer (cloak, dome, a portal pair, a well). Powers that must not run under AI end here.
	 */
	virtual void OnLostPlayerControl() {}
	/** #201: the player switched INTO this ship while a power its wingman AI started was running. Default: keep
	 *  running on its own timer (dome, cloak, a well, portals). Powers that would surprise the player end here. */
	virtual void OnGainedPlayerControl() {}

	bool IsOwnerPlayerControlled() const;
	/** Owner's ship id; false if the owner is not a squad ship. */
	bool GetOwnerShipId(ETDShipId& OutShip) const;
	ETDTeam GetOwnerTeam() const;
	void Highlight(ETDHighlightKind Kind, AActor* Subject, float Importance01) const;
	void AddStat(FName StatName, int32 Amount = 1) const;

	/** Ship cue for this slot at activation (wind-up powers override to play PowerWindup first). */
	virtual void PlayActivationCues();
	void PlayShipCue(int32 Which); // 0 wind-up, 1 active (slot-aware), 2 end
	/** Where the active cue attaches (the ship by default; the dome power uses its dome, which lags behind Bastion). */
	virtual USceneComponent* GetActiveCueRoot() const;
	/** Placeholder art is used only when no art is assigned for this moment. */
	bool HasActiveArt() const;

	bool bPowerActive = false;
	/** Player-controlled on the previous tick (to detect a switch away mid-power). */
	bool bWasPlayerControlled = false;
	float ActiveElapsed = 0.f;
	float CooldownRemaining = 0.f;
	/** Barks: Bark_PowerNotReady said this cooldown; Bark_PowerReady alternates (dialogs.md: 1 in 2). */
	bool bSaidNotReady = false;
	int32 ReadyCount = 0;
	/** #201: inside TryActivateFromAI (CanActivate skips the player-control rule), and who started the activation. */
	bool bAIActivating = false;
	bool bStartedByAI = false;
};
