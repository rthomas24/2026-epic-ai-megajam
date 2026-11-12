// Triad Drift - atmosphere entry (galaxy) and atmosphere flight zone (planets), ENG-20. Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDAtmosphereZones.generated.h"

class UBoxComponent;
class USphereComponent;
class ATDShip;
class UTDTravelFX;
class UTDAtmosphereEntryComponent;

UENUM(BlueprintType)
enum class ETDAtmosphereEntryState : uint8
{
	Idle,
	Entering,  // 2.5 s of heat and shake; the ship is committed
	Travelling // TravelTo was called
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnAtmosphereEntry, UTDAtmosphereEntryComponent*, Entry);

/**
 * Atmospheric entry on a squad ship (features.md section 7 steps 2-3). T4's galaxy ATDPlanetAtmosphere
 * detects the player ship entering and calls BeginEntry. The ship is then committed: speed is held,
 * steering is reduced, hyperdrive is shut off and the AtmosphericEntry cue plays. After EntrySeconds,
 * ITDGameFlow::TravelTo(Destination, ReturnPoint) is called. If there's no flow, or travel fails,
 * control is handed back.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDAtmosphereEntryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDAtmosphereEntryComponent();

	/** Start the entry. Returns false if one is already running or the owner isn't a spacecraft. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Atmosphere")
	bool BeginEntry(ETDDestination InDestination, FName InReturnPoint, FVector PlanetCenter);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	ETDAtmosphereEntryState GetEntryState() const { return EntryState; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsEntering() const { return EntryState != ETDAtmosphereEntryState::Idle; }

	/** 0..1 through the entry (heat / shake intensity for presentation). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	float GetEntry01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	ETDDestination GetDestination() const { return Destination; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float EntrySeconds = 2.5f;

	/** Speed held during entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float EntrySpeed = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0", ClampMax = "1"))
	float EntrySteeringScale = 0.35f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Atmosphere")
	FTDOnAtmosphereEntry OnEntryStarted;

	/** TravelTo was called (fires in worlds without a game flow too). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Atmosphere")
	FTDOnAtmosphereEntry OnEntryCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	TObjectPtr<UTDTravelFX> TravelFX;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void FinishEntry();

	ETDAtmosphereEntryState EntryState = ETDAtmosphereEntryState::Idle;
	ETDDestination Destination = ETDDestination::Verdance;
	FName ReturnPoint = NAME_None;
	float EntryTime = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnAtmosphereExit, ATDShip*, Ship);

/**
 * Planet side (features.md §7 steps 4–5 and 10). Every squad ship inside the box flies in
 * atmosphere mode: gravity, speed capped at 60% of space cruise, no hyperdrive, Land on F/A. The
 * box's walls and ceiling are soft boundaries. Once the planet's objectives are done (or
 * SetExitAllowed is called), flying out through the ceiling calls
 * TravelTo(Galaxy, "<PlanetId>_Orbit").
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDAtmosphereFlightZone : public AActor
{
	GENERATED_BODY()

public:
	ATDAtmosphereFlightZone();

	/** "Verdance" / "Cinder": used for the exit return point "<PlanetId>_Orbit". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	FName PlanetId = TEXT("Verdance");

	/** Exit is allowed once all of these are in FTDProgress::CompletedObjectives (empty = always). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	TArray<FName> RequiredObjectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float Gravity = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0.05", ClampMax = "1"))
	float SpeedCapFraction = 0.6f;

	/** How far above the ceiling the player must climb to leave once the exit is open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere", meta = (ClampMin = "0"))
	float ExitHeightAboveCeiling = 3000.f;

	/** Tip shown (once per approach) when the player hits the ceiling before the objectives are done. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Atmosphere")
	FName CeilingBlockedTip = TEXT("Tip_FinishObjectivesFirst");

	/** Force the exit open or closed regardless of objectives (scripted sequences, cheats). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Atmosphere")
	void SetExitAllowed(bool bAllowed) { bExitOverride = true; bExitAllowedOverride = bAllowed; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsExitAllowed() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	FBox GetZoneBox() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Atmosphere")
	bool IsShipInZone(const ATDShip* Ship) const;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Atmosphere")
	FTDOnAtmosphereExit OnExitTriggered;

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Atmosphere")
	TObjectPtr<UBoxComponent> ZoneBox;

private:
	void ApplyToShip(ATDShip* Ship, bool bExitOpen);

	bool bExitOverride = false;
	bool bExitAllowedOverride = false;
	bool bExitTriggered = false;
	bool bTipShown = false;
	TArray<TWeakObjectPtr<ATDShip>> ManagedShips;
};
