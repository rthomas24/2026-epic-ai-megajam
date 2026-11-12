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
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnObjectiveStep, ATDObjective*, Objective, int32, StepIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnObjectiveDone, ATDObjective*, Objective);

/**
 * One objective: an id, ordered steps (text + waypoint) and a reward. Starting it opens a Director's Cut
 * scene (the objective id) and fires OnObjectiveUpdated(Id, 0, Text). Each completed step fires
 * OnObjectiveUpdated(Id, Step, Text); the last calls ITDGameFlow::CompleteObjective(Id, Reward, Ship),
 * which unlocks the ability and brings up Nick's reward screen.
 *
 * On a retake of its scene it resets to step 0 and restarts.
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

	/** Back to not-started (retakes). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Objective")
	void ResetObjective();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") bool IsActive() const { return bActive; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") bool IsComplete() const { return bComplete; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") int32 GetCurrentStepIndex() const { return bActive ? CurrentStep : INDEX_NONE; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Objective") FText GetCurrentStepText() const;
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
	void HandleSceneRestart(FName SceneId, int32 TakeNumber);

	bool bActive = false;
	bool bComplete = false;
	int32 CurrentStep = 0;
	FDelegateHandle RestartHandle;
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
	void GetActiveWaypoints(TArray<AActor*>& Out) const;

private:
	TArray<TWeakObjectPtr<ATDObjective>> Active;
};
