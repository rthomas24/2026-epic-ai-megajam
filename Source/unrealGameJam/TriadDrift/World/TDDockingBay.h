// Copyright Triad Drift team. ENG-29: the Dominion's docking bay, the approach cone and its tractor-beam assist.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDDockingBay.generated.h"

class ATDDominion;
class ATDShip;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

UENUM(BlueprintType)
enum class ETDDockingState : uint8
{
	Closed,		// doors shut, no tractor
	Open,		// doors open (or opening), the cone pulls a ship that enters it
	Gliding,	// the ship crossed the mouth: controls locked, it glides inside
	Docked		// travel to the interior requested (exactly once)
};

/**
 * ENG-29. Opens on the Dominion's bow (at its hangar point) when the four generators are down and the boarding finale
 * is on. A cone opens outward from the mouth (+X of this actor = out of the bay):
 * - Inside the cone, the tractor assist grows with depth: the ship's speed eases toward TractorSpeed, its steering
 *   drops to MinSteeringScale, and its heading and line are pulled toward the bay axis. It never fully locks: the
 *   pilot can still fly out of the cone, and leaving the cone gives full control back.
 * - Within MouthDistance of the mouth (and inside MouthRadius of the axis) the controls lock and the ship glides in
 *   over GlideSeconds, invulnerable. Then SetPhase(DominionInterior), SaveCheckpoint() and
 *   TravelTo(DominionInterior, "Hangar"), exactly once.
 * The player's active ship only; wingmen wait outside.
 */
UCLASS()
class UNREALGAMEJAM_API ATDDockingBay : public AActor
{
	GENERATED_BODY()

public:
	ATDDockingBay();

	/** The interior's start point name (T5's ATDDominionInterior "Hangar"). */
	static const FName InteriorEntry;

	/** The speed-override owner the tractor uses on the ship. */
	static const FName TractorOverride;

	/** Opens the doors (bPlayCues: the DockingBayOpen cue; false when resuming a save or a retake). */
	void Open(ATDDominion* InDominion, bool bPlayCues);

	/** A new take of the assault: the tractor lets go and a glide in progress is cancelled (the bay stays open). */
	void ResetForNewTake();

	ETDDockingState GetDockingState() const { return DockingState; }
	bool IsOpen() const { return DockingState != ETDDockingState::Closed; }

	/** World position of the mouth and the outward bay axis. */
	FVector GetMouth() const { return GetActorLocation(); }
	FVector GetOutward() const { return GetActorForwardVector(); }

	/** 0 outside the cone, rising to 1 at the mouth (how strongly the tractor holds the ship). */
	float GetTractorStrength(const FVector& WorldPoint) const;

	/** Tractor strength on the ship last frame (tests, HUD). */
	float GetCurrentPull() const { return CurrentPull; }

	/** How many times this bay asked for the interior travel (tests: exactly one). */
	int32 GetTravelRequests() const { return TravelRequests; }

	/** Wingmen held in the orbit outside the bay while the player docks (Gameplay 06: the player enters alone). */
	int32 GetHeldWingmenCount() const { return HeldWingmen.Num(); }

	/** The orbit the wingmen hold (radius and how far out of the mouth its centre is, cm), and their speed (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float WingmanOrbitRadius = 15000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float WingmanOrbitOut = 40000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float WingmanOrbitDegPerSec = 25.f;

	/** Tests / dev: the ship the tractor acts on (default: the squad's active ship). */
	void SetShipOverride(ATDShip* Ship);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "1", ClampMax = "80"))
	float ConeHalfAngleDeg = 35.f;

	/**
	 * Near the mouth the tractor takes a ship coming in from the side too: within CaptureRadius (cm) of the mouth and in
	 * front of the bow, up to CaptureHalfAngleDeg off the axis (T7's playtest, Nov 14: a human approach came in outside
	 * the 35 degree cone and never docked). The pull then lines it up on the axis.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float CaptureRadius = 40000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "1", ClampMax = "89"))
	float CaptureHalfAngleDeg = 80.f;

	/** The HUD waypoint that leads into the cone: this far (cm) out on the bay axis while the bay is open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float ApproachMarkerDistance = 80000.f;

	/** How far out (cm) the cone reaches from the mouth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float ConeDepth = 300000.f;

	/** The ship eases toward this speed (cm/s) as the pull grows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float TractorSpeed = 1500.f;

	/** Steering scale at full pull (the ship still answers, at about a third). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0", ClampMax = "1"))
	float MinSteeringScale = 0.35f;

	/** Heading pull toward the bay axis at full strength (deg/s); below the pilot's reduced turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float HeadingPullDegPerSec = 18.f;

	/** Lateral pull toward the axis at full strength (fraction of the offset per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float LineupPerSecond = 0.6f;

	/** Within this distance (cm) of the mouth, and MouthRadius of the axis, the controls lock and the glide starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float MouthDistance = 30000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float MouthRadius = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0.1"))
	float GlideSeconds = 2.f;

	/** How far inside the mouth (cm) the glide ends: over the pad, 20 m short of the 60 m pocket's back wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float GlideDepth = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Docking", meta = (ClampMin = "0"))
	float DoorOpenSeconds = 2.5f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	ATDShip* GetShip() const;
	void ReleaseShip();
	void BeginGlide(ATDShip* Ship);
	void FinishDocking();
	void BuildPlaceholder();
	void HoldWingmen(const ATDShip* Docking);
	void ReleaseWingmen();
	void TickWingmen(float DeltaSeconds);

	struct FHeldWingman
	{
		TWeakObjectPtr<ATDShip> Ship;
		float Angle = 0.f;	// deg around the orbit
	};
	TArray<FHeldWingman> HeldWingmen;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Docking")
	TObjectPtr<USceneComponent> Root;

	/** Placeholder doors (two slabs that slide apart) and the frame around the mouth. */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Docking")
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Docking")
	TObjectPtr<UStaticMeshComponent> DoorRight;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Docking")
	TObjectPtr<UStaticMeshComponent> Frame;

	/** Guide lights down the approach axis (shown while open). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Docking")
	TObjectPtr<UInstancedStaticMeshComponent> GuideLights;

	TWeakObjectPtr<ATDDominion> Dominion;
	TWeakObjectPtr<ATDShip> ShipOverride;
	TWeakObjectPtr<ATDShip> Held;	// the ship the tractor holds now
	ETDDockingState DockingState = ETDDockingState::Closed;
	float DoorTime = 0.f;
	float GlideTime = 0.f;
	FVector GlideFrom = FVector::ZeroVector;
	FQuat GlideFromRot = FQuat::Identity;
	float CurrentPull = 0.f;
	int32 TravelRequests = 0;
	float DoorHalfWidth = 2500.f;

	/** The approach waypoint (TDTags::ObjectiveMarker) on the axis while the bay is open. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> ApproachMarker;
	void ShowApproachMarker(bool bShow);
};
