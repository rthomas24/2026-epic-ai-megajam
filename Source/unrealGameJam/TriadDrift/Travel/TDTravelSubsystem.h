// Triad Drift - travel registry: gravity wells (hyperdrive drop-out) and landing pads (ENG-15, ENG-20).
// Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDTravelSubsystem.generated.h"

class ATDShip;
class ATDLandingPad;

/** A point that pulls ships out of hyperdrive: planet markers, the Dominion, portal gates... */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDGravityWell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Travel")
	FName Id = NAME_None;

	/** If set, the well follows this actor (a moving Dominion); otherwise Location is used. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Travel")
	TWeakObjectPtr<AActor> Owner;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Travel")
	FVector Location = FVector::ZeroVector;

	/** Ships in hyperdrive drop out once inside this radius (cm). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Travel")
	float Radius = 15000.f;

	FVector GetLocation() const { return Owner.IsValid() ? Owner->GetActorLocation() : Location; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnShipLandingEvent, ATDShip*, Ship, ATDLandingPad*, Pad);

/**
 * World subsystem for travel: who can pull a ship out of hyperdrive, which landing pads exist, and
 * the landed / took-off events T5's first-person hand-off listens to.
 * T4's galaxy layout registers wells with RegisterGravityWell (or places ATDGravityWellMarker actors).
 */
UCLASS()
class UNREALGAMEJAM_API UTDTravelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDTravelSubsystem* Get(const UObject* WorldContextObject);

	// ---- Gravity wells --------------------------------------------------------------------------
	/** Adds or replaces (same Id) a well. Owner may be null for a static point. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Travel")
	void RegisterGravityWell(FName Id, FVector Location, float Radius = 15000.f, AActor* Owner = nullptr);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Travel")
	void UnregisterGravityWell(FName Id);

	/** The nearest well whose radius contains Location. */
	bool FindWellContaining(const FVector& Location, FTDGravityWell& OutWell) const;

	/** The first well the segment From->To passes within radius of (for fast jumps that could skip a well in one frame). */
	bool FindWellOnSegment(const FVector& From, const FVector& To, FTDGravityWell& OutWell) const;

	const TArray<FTDGravityWell>& GetGravityWells() const { return Wells; }

	// ---- Play space ----------------------------------------------------------------------------
	/** The galaxy's soft spherical play space (T4's layout sets it). Every spacecraft picks it up automatically. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Travel")
	void SetPlayBounds(FVector Center, float Radius);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Travel")
	void ClearPlayBounds();

	/** False when no bounds were set (ships keep their own default). */
	bool GetPlayBounds(FVector& OutCenter, float& OutRadius) const;

	/** Bumped on every Set/Clear so movement components re-apply cheaply. */
	int32 GetPlayBoundsVersion() const { return PlayBoundsVersion; }

	// ---- Landing pads ---------------------------------------------------------------------------
	void RegisterPad(ATDLandingPad* Pad);
	void UnregisterPad(ATDLandingPad* Pad);

	/** Nearest free pad for the player (non-wingman pads) within MaxDistance of Location. */
	ATDLandingPad* FindPlayerPad(const FVector& Location, float MaxDistance) const;

	/** Free wingman pads around a player pad, nearest first. */
	void FindWingmanPads(const ATDLandingPad* PlayerPad, int32 Count, TArray<ATDLandingPad*>& OutPads) const;

	const TArray<TWeakObjectPtr<ATDLandingPad>>& GetPads() const { return Pads; }

	// ---- Events (T5's first-person hand-off binds these) ------------------------------------------------
	/** A ship finished its landing sequence (canopy open). Fired for wingmen too: check Ship->IsPlayerFlown(). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Travel")
	FTDOnShipLandingEvent OnShipLanded;

	/** A ship finished taking off and is flying again. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Travel")
	FTDOnShipLandingEvent OnShipTookOff;

private:
	UPROPERTY()
	TArray<FTDGravityWell> Wells;

	TArray<TWeakObjectPtr<ATDLandingPad>> Pads;

	bool bHasPlayBounds = false;
	FVector PlayBoundsCenter = FVector::ZeroVector;
	float PlayBoundsRadius = 0.f;
	int32 PlayBoundsVersion = 0;
};
