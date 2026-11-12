// Triad Drift - planet travel rig: the gameplay pieces a planet map needs for ENG-20, in one actor.
// Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDPlanetTravelRig.generated.h"

class ATDAtmosphereFlightZone;
class ATDLandingPad;
class UStaticMeshComponent;

/**
 * One actor that sets up a planet map for flight, landing and take-off: an ATDAtmosphereFlightZone
 * (soft walls and ceiling; exit once RequiredObjectives are done), the player's landing pad with two
 * wingman side pads, and optionally a placeholder ground slab so altitude and pads have something
 * under them before Nick's environment exists. Place it in the map, or have the ground game mode
 * call SpawnForPlanet. The rig's transform is the player pad: +X is the landing facing.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDPlanetTravelRig : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetTravelRig();

	/**
	 * Spawns and configures a rig (ground game mode). Returns null if the world is missing. bPlaceholderGround is
	 * applied before BeginPlay: pass false when the level has real terrain (Nick's environment).
	 */
	static ATDPlanetTravelRig* SpawnForPlanet(UWorld* World, FName PlanetId, const FTransform& PadTransform,
		const TArray<FName>& RequiredObjectives, bool bInPlaceholderGround = true);

	/** Shows / hides the placeholder ground slab and its collision (any time, also after BeginPlay). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Planet")
	void SetPlaceholderGround(bool bEnabled);

	/** "Verdance" / "Cinder". The zone's exit return point is "<PlanetId>_Orbit". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	FName PlanetId = TEXT("Verdance");

	/** Objectives that must be complete before the ceiling lets the squad leave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	TArray<FName> RequiredObjectives;

	/** Half-size of the flight zone (cm), centred on the pad horizontally; its floor sits ZoneBelowPad under the pad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	FVector ZoneExtent = FVector(250000.f, 250000.f, 60000.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "0"))
	float ZoneBelowPad = 5000.f;

	/** Wingman pads sit this far to each side of the player pad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "0"))
	float WingmanPadSpacing = 2200.f;

	/** Where a fresh arrival starts its descent, relative to the pad (behind it and high up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	FVector DescentStartOffset = FVector(-60000.f, 0.f, 30000.f);

	/** Spawn a flat placeholder ground slab under the pads (dev; turn off once the environment has ground). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	bool bPlaceholderGround = true;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	ATDAtmosphereFlightZone* GetZone() const { return Zone; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	ATDLandingPad* GetPlayerPad() const { return PlayerPad; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	const TArray<ATDLandingPad*>& GetWingmanPads() const { return WingmanPadsRaw; }

	/** Fresh-arrival spawn for the squad leader (descending toward the pad), if no PlayerStart_Descent is placed. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	FTransform GetDescentStart() const;

	/** Spawns the zone and pads (BeginPlay does this; safe to call once). */
	void BuildRig();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Planet")
	TObjectPtr<UStaticMeshComponent> GroundSlab;

private:
	UPROPERTY(Transient) TObjectPtr<ATDAtmosphereFlightZone> Zone;
	UPROPERTY(Transient) TObjectPtr<ATDLandingPad> PlayerPad;
	UPROPERTY(Transient) TArray<TObjectPtr<ATDLandingPad>> WingmanPads;
	TArray<ATDLandingPad*> WingmanPadsRaw;
	bool bBuilt = false;
};
