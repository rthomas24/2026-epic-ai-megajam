// Triad Drift - Powers (Teammate 2). Base for signature powers and unlockable abilities (ENG-09..12).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TDPowerComponent.generated.h"

class UTDPowerComponent;
class UTDAbilityFX;

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
UCLASS(Abstract, ClassGroup = (TriadDrift))
class UNREALGAMEJAM_API UTDPowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDPowerComponent();

	// --- Contract ---
	/** Start the power if ready. Returns true if it started. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Power")
	bool TryActivate();

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

	/** Only fires while the player flies this ship (features.md: wingmen never use powers). Tests turn it off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	bool bRequirePlayerControl = true;

	/** Presentation (Activate / Loop / End cues). Null = silent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Power")
	TObjectPtr<UTDAbilityFX> FX = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Power")
	FTDOnPowerActivated OnPowerActivated;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Power")
	FTDOnPowerEnded OnPowerEnded;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/** Do the power. Return false to abort (no cooldown, no event). */
	virtual bool OnActivate() { return true; }
	virtual void OnTickActive(float DeltaTime) {}
	virtual void OnEnd(bool bInterrupted) {}

	bool IsOwnerPlayerControlled() const;
	/** Owner's ship id; false if the owner is not a squad ship. */
	bool GetOwnerShipId(ETDShipId& OutShip) const;
	ETDTeam GetOwnerTeam() const;
	void Highlight(ETDHighlightKind Kind, AActor* Subject, float Importance01) const;
	void AddStat(FName StatName, int32 Amount = 1) const;

	bool bPowerActive = false;
	float ActiveElapsed = 0.f;
	float CooldownRemaining = 0.f;
};
