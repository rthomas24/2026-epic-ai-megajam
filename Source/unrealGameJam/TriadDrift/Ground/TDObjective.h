// Triad Drift - objective framework (ENG-24). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDObjective.generated.h"

class ATDObjective;

USTRUCT(BlueprintType)
struct FTDObjectiveStep
{
	GENERATED_BODY()

	/** HUD text for this step ("Clear the wreck site"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText Text;

	/** Where the marker points while this step is current (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TObjectPtr<AActor> Waypoint;

	/** Reaching this step is a checkpoint: a retake restarts the scene here, not at step 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	bool bCheckpoint = false;

	/** Where the pilot comes back for a retake from this checkpoint (optional; else the pad). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TObjectPtr<AActor> RespawnAt;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnObjectiveStep, ATDObjective*, Objective, int32, StepIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnObjectiveDone, ATDObjective*, Objective);

/**
 * One objective: an id, ordered steps (text + waypoint) and a reward. Starting it opens a Director's Cut
 * scene (the objective id) and fires OnObjectiveUpdated(Id, 0, Text). Each completed step fires
 * OnObjectiveUpdated(Id, Step, Text); the last calls ITDGameFlow::CompleteObjective(Id, Reward, Ship),
 * which unlocks the ability and brings up Nick's reward screen.
 *
 * On any retake, if incomplete, it resets to step 0 (and restarts if it was running): UTDGroundSceneSubsystem.
 */
UCLASS()
class UNREALGAMEJAM_API ATDObjective : public AActor
{
	GENERATED_BODY()

public:
	ATDObjective();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName ObjectiveId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TArray<FTDObjectiveStep> Steps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	ETDAbilityId RewardAbility = ETDAbilityId::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	ETDShipId RewardShip = ETDShipId::Rift;

	/** Start on BeginPlay (otherwise call StartObjective, e.g. from a trigger). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	bool bAutoStart = false;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Objective")
	void StartObjective();

	/** Completes StepIndex if it is the current step (out-of-order calls are ignored). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Objective")
	bool CompleteStep(int32 StepIndex);

	/** Completes the current step. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Objective")
	bool AdvanceStep();

	/** Re-announces the current step with new text ("WAKE THE RESONANCE NODES 2 / 3"). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Objective")
	void UpdateStepText(const FText& Text);

	/** Already done in this run (loading a checkpoint): complete, inactive, never broadcasts or rewards again. */
	void MarkAlreadyComplete() { bActive = false; bComplete = true; }

	/** Back to not-started (retakes). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Objective")
	void ResetObjective();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") bool IsActive() const { return bActive; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") bool IsComplete() const { return bComplete; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") int32 GetCurrentStepIndex() const { return bActive ? CurrentStep : INDEX_NONE; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") FText GetCurrentStepText() const;
	/** The step a retake restarts from (the last checkpoint step reached; 0 when none). */
	int32 GetCheckpointStep() const { return CheckpointStep; }
	/** Where the pilot should come back for a retake, if the last checkpoint has a spot. */
	AActor* GetCheckpointRespawn() const { return Steps.IsValidIndex(CheckpointStep) ? Steps[CheckpointStep].RespawnAt.Get() : nullptr; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") AActor* GetCurrentWaypoint() const;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Objective")
	FTDOnObjectiveStep OnStepStarted;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Objective")
	FTDOnObjectiveDone OnCompleted;

	/** Native: fired when a retake resets this objective (encounters reset with it). */
	DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnObjectiveReset, ATDObjective*);
	FTDOnObjectiveReset OnReset;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Broadcast(int32 Step, const FText& Text) const;
	void Finish();

	bool bActive = false;
	bool bComplete = false;
	int32 CurrentStep = 0;
	int32 CheckpointStep = 0;
};

/** Tracks active objectives so the HUD and the ping can find their markers without actor iteration. */
UCLASS()
class UNREALGAMEJAM_API UTDObjectiveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDObjectiveSubsystem* Get(const UObject* WorldContext);

	void Register(ATDObjective* Objective);
	void Unregister(ATDObjective* Objective);

	void GetActiveObjectives(TArray<ATDObjective*>& Out) const;
	/** The markers every on-foot drawer points at (T2's HUD, the dev HUD). Where the straight line from the pilot to a
	 *  step's waypoint crosses a registered hazard (Cinder's magma), the next waypoint of that crossing's safe chain is
	 *  given instead, until the last leg (T7's play, Nov 14: 21 CUTs following a marker straight over the river). */
	void GetActiveWaypoints(TArray<AActor*>& Out) const;

	/** A hazard the marker must route around while Objective is on Step (INDEX_NONE: any step): Chain is the safe way
	 *  across, in order from either side (bank, stones, far bank). */
	void AddCrossing(const FBox& Hazard, ATDObjective* Objective, int32 Step, const TArray<AActor*>& Chain);
	/** The marker for a pilot at From heading to Target on Objective's current step: Target, or a chain waypoint. */
	AActor* RouteWaypoint(const FVector& From, const ATDObjective* Objective, AActor* Target) const;
	/** The same for a given step (tests walk every step). */
	AActor* RouteWaypointForStep(const FVector& From, const ATDObjective* Objective, int32 Step, AActor* Target) const;
	/** The flat line A-B passes over the hazard's footprint (grown by Margin). */
	static bool SegmentCrosses(const FBox& Hazard, const FVector& A, const FVector& B, float Margin = 50.f);
	/** Within this of a chain waypoint, the marker moves on to the next one. */
	static constexpr float CrossingReach = 350.f;

private:
	TArray<TWeakObjectPtr<ATDObjective>> Active;
	struct FCrossing
	{
		FBox Hazard;
		TWeakObjectPtr<ATDObjective> Objective;
		int32 Step = INDEX_NONE;
		TArray<TWeakObjectPtr<AActor>> Chain;
	};
	TArray<FCrossing> Crossings;
};
