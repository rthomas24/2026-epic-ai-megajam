// Triad Drift - on-foot scene registry: what a "CUT!" resets on a planet (ENG-24, C-03 #3). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDGroundScene.generated.h"

class ATDObjective;
class ATDEncounterVolume;
class ATDInteractableActor;

/**
 * Every objective, encounter and interactable registers here. On every scene restart (UTDGameInstance
 * OnSceneRestart) everything not finished goes back to its starting state, not only the scene that
 * was running:
 * - incomplete objectives reset, and the ones that were running start again (the current scene's last,
 *   so it stays the current scene)
 * - encounters reset unless their objective is complete (crew and summons removed, doors restored)
 * - single-use interactables of incomplete objectives can be used again
 * - encounters re-check whether the player is already standing inside them
 * Completed objectives and their rooms stay done.
 */
UCLASS()
class UNREALGAMEJAM_API UTDGroundSceneSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDGroundSceneSubsystem* Get(const UObject* WorldContext);

	void RegisterObjective(ATDObjective* Objective);
	void RegisterEncounter(ATDEncounterVolume* Encounter);
	void RegisterInteractable(ATDInteractableActor* Interactable);
	void Unregister(AActor* Actor);

	/** An objective step began: encounters linked to it check for a player already inside. */
	void NotifyStepStarted(ATDObjective* Objective, int32 StepIndex);

	/** Reset everything unfinished (called on a retake; public for tests and cheats). */
	void ResetUnfinished(FName CurrentScene);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	int32 GetResetCount() const { return ResetCount; }

	/** After every ResetUnfinished: planet props (counters, bridges, hazards, carried items) reset themselves. */
	DECLARE_MULTICAST_DELEGATE(FTDOnGroundReset);
	FTDOnGroundReset OnResetUnfinished;

private:
	void HandleSceneRestart(FName SceneId, int32 TakeNumber);

	TArray<TWeakObjectPtr<ATDObjective>> Objectives;
	TArray<TWeakObjectPtr<ATDEncounterVolume>> Encounters;
	TArray<TWeakObjectPtr<ATDInteractableActor>> Interactables;
	FDelegateHandle RestartHandle;
	int32 ResetCount = 0;
};
