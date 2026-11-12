// Triad Drift - planet arrival, landing hand-off, re-board and on-foot retakes (ENG-20 ground half, with T1).
// Owner: Teammate 5. Contract: docs/eng/ground.md "Planet arrival and hand-off" and docs/eng/travel.md.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TDPlanetDirector.generated.h"

class ATDPlanetTravelRig;
class ATDSquadManager;
class ATDShip;
class ATDLandingPad;
class ATDFirstPersonCharacter;
class ATDPlanetDirector;

/** The "Board ship" prompt standing at the landed ship's exit point. */
UCLASS()
class UNREALGAMEJAM_API ATDBoardShipInteractable : public ATDInteractableActor
{
	GENERATED_BODY()

public:
	ATDBoardShipInteractable();

	UPROPERTY(Transient)
	TObjectPtr<ATDPlanetDirector> Director;

	virtual bool CanInteract(AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;
};

/**
 * Runs a planet visit from the ground side:
 * - **Fresh arrival:** T1's ATDPlanetTravelRig plus T3's squad, flying in at GetDescentStart() under the
 *   space player controller. T1's flight zone owns atmosphere flight and landing.
 * - **Landed** (UTDTravelSubsystem::OnShipLanded for the player's ship): the first-person character
 *   is spawned at GetExitTransform() and possessed; wingmen go passive; the run checkpoints with
 *   ReturnPoint "<Planet>_Pad".
 * - **Re-board** (the board interactable): possess the ship, SetPlayerControlled(true), RequestTakeOff().
 * - **CUT! after landing:** back on foot at the pad, with the ship still parked. Before landing: the
 *   squad respawns at the descent start.
 * - **Loading a "<Planet>_Pad" checkpoint:** every ship is SnapLanded and the player starts on foot.
 * - **Leaving:** T1's ceiling exit calls TravelTo(Galaxy, "<Planet>_Orbit") once RequiredObjectives are done.
 */
UCLASS()
class UNREALGAMEJAM_API ATDPlanetDirector : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetDirector();

	/** "Verdance" / "Cinder". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FName PlanetId = TEXT("Verdance");

	/** Where the player pad goes (+X = landing facing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FTransform PadTransform;

	/** Objectives that open the ceiling exit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TArray<FName> RequiredObjectives;

	/** Keep T1's placeholder ground slab (off once the planet layout provides ground). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	bool bRigPlaceholderGround = true;

	/** Start the visit on BeginPlay (tests can set things up first and call StartVisit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	bool bAutoStart = true;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Planet")
	void StartVisit();

	/** Hand control from the landed ship to the first-person character. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Planet")
	bool HandOverToFoot(ATDShip* Ship);

	/** Back into the ship and take off. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Planet")
	bool Reboard();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet") bool IsOnFoot() const { return bOnFoot; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet") ATDPlanetTravelRig* GetRig() const { return Rig; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet") ATDFirstPersonCharacter* GetFirstPerson() const { return FirstPerson; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet") ATDShip* GetPlayerShip() const { return PlayerShip.Get(); }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet") ATDBoardShipInteractable* GetBoardPoint() const { return BoardPoint; }

	/** ReturnPoint saved while the player is on foot here: "<Planet>_Pad". */
	FName GetPadReturnPoint() const;

	static ATDPlanetDirector* Get(const UObject* WorldContext);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleShipLanded(ATDShip* Ship, ATDLandingPad* Pad);

	void HandleSceneRestart(FName SceneId, int32 TakeNumber);
	APlayerController* GetPlayerController() const;
	ATDSquadManager* EnsureSquad();
	void SnapSquadLanded();
	FTransform ExitTransformFor(ATDShip* Ship) const;

	UPROPERTY(Transient) TObjectPtr<ATDPlanetTravelRig> Rig;
	UPROPERTY(Transient) TObjectPtr<ATDFirstPersonCharacter> FirstPerson;
	UPROPERTY(Transient) TObjectPtr<ATDBoardShipInteractable> BoardPoint;
	TWeakObjectPtr<ATDShip> PlayerShip;
	bool bOnFoot = false;
	bool bStarted = false;
	FDelegateHandle RestartHandle;
};
