// Triad Drift - Squad (Teammate 3). ENG-04: gameplay stats of the three squad ships.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDShipLibrary.generated.h"

class ATDShip;
class UTDPowerComponent;
class UTDAbilityComponent;
class UTDShipVisuals;

/**
 * Gameplay spec for one squad ship (features.md ENG-04). Every number defaults to the features.md
 * value and can be overridden per ship on ATDSquadManager (ShipSpecOverrides) for tuning.
 */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDShipSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	ETDShipId Ship = ETDShipId::Rift;

	/** "RIFT" - used by the switch-cut name card (C-03 #1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	FText ShipName;

	/** "WREN" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	FText PilotName;

	/** "Trickster", "Interceptor", "Heavy" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	FText RoleName;

	// --- Survivability (ENG-03 / ENG-04) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHull = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float MaxShield = 100.f;

	/** Seconds without damage before the shield regenerates (balance sweep, T4: 3-6 s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0", Units = "s"))
	float ShieldRegenDelay = 3.f;

	/** Fraction of MaxShield regenerated per second (0.25 = 25%/s; T4 sweeps 0.08-0.25). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float ShieldRegenFractionPerSecond = 0.25f;

	// --- Flight (ENG-01 base values x ENG-04 per-ship) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "cm/s"))
	float MinSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "cm/s"))
	float CruiseSpeed = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "cm/s"))
	float BoostSpeed = 11000.f;

	/** Pitch and yaw rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "deg/s"))
	float TurnRateDeg = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "deg/s"))
	float RollRateDeg = 180.f;

	/** Seconds of continuous boost a full meter gives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "s"))
	float BoostMeterSeconds = 3.f;

	/** Delay after boosting stops before the meter regenerates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "s"))
	float BoostRegenDelay = 1.2f;

	/** Seconds to refill an empty meter once regen starts. Lancer: fast regen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight", meta = (Units = "s"))
	float BoostRegenSeconds = 2.5f;

	// --- Weapons (ids resolve through T2's UTDWeaponLibrary) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	FName PrimaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	FName SecondaryWeapon;

	/** Display/HUD name of the signature power (player-only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Powers")
	FName SignaturePower;

	/** Signature power on Q (T2 component class; the squad installs it once per ship). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Powers")
	TSubclassOf<UTDPowerComponent> Signature1PowerClass;

	/** Second signature power on hold-Q (Bastion's shield dome). None for the others. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Powers")
	TSubclassOf<UTDPowerComponent> Signature2PowerClass;

	/** The unlockable-ability slot component (installs whatever progress has unlocked, ENG-12). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Powers")
	TSubclassOf<UTDAbilityComponent> AbilityComponentClass;

	/** Ability this ship can unlock (ENG-12). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Powers")
	ETDAbilityId UnlockableAbility = ETDAbilityId::None;

	/** Squad colour: projectiles, HUD, placeholder hull tint. Defaults to UTDStatics::ShipColor; a
	 *  visuals asset with its own SquadColor overrides it (see GetSquadColor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FLinearColor SquadColor = FLinearColor::White;

	/** Nick's per-ship presentation asset. Convention:
	 *  /Game/TriadDrift/Presentation/DA_<Ship>_Visuals (flat folder, agreed with Nick); loaded if it
	 *  exists, else placeholder. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UTDShipVisuals> Visuals;
};

/**
 * Static access to the three ship specs (ENG-04) and helpers to apply them to an ATDShip.
 * Get() never fails: an invalid id returns the Rift spec.
 */
UCLASS()
class UNREALGAMEJAM_API UTDShipLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** C++ access by reference (no copy). */
	static const FTDShipSpec& Get(ETDShipId Ship);

	/** Blueprint access. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Squad", meta = (DisplayName = "Get Ship Spec"))
	static FTDShipSpec GetShipSpec(ETDShipId Ship);

	/** All three ids in switch order (Rift, Lancer, Bastion). */
	static const TArray<ETDShipId>& AllShips();

	/**
	 * Apply a spec to a squad ship: ship id, flight tuning (T1 ATDSpacecraft::ApplyFlightTuning),
	 * hull/shield (Core UTDHealthComponent) and weapons (T2 UTDWeaponComponent::EquipWeapon).
	 * Null-safe; missing components are skipped with a warning.
	 */
	static void ApplySpecToShip(ATDShip* Ship, const FTDShipSpec& Spec);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Squad")
	static void ApplyDefaultSpecToShip(ATDShip* Ship, ETDShipId ShipId);

	/** The visuals asset for a spec if it exists on disk (null otherwise; never logs an error). */
	static UTDShipVisuals* LoadVisuals(const FTDShipSpec& Spec);

	/** The one squad colour: the visuals asset's, if assigned and not white; else the spec's. */
	static FLinearColor GetSquadColor(const FTDShipSpec& Spec);
};
