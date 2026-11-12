// Triad Drift - on-foot scene registry (ENG-24, C-03 #3). Owner: Teammate 5.

#include "TriadDrift/Ground/TDGroundScene.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

UTDGroundSceneSubsystem* UTDGroundSceneSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDGroundSceneSubsystem>() : nullptr;
}

void UTDGroundSceneSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UTDGameInstance* GI = Cast<UTDGameInstance>(InWorld.GetGameInstance()))
	{
		RestartHandle = GI->OnSceneRestart.AddUObject(this, &UTDGroundSceneSubsystem::HandleSceneRestart);
	}
	InWorld.GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() { ApplyCompletedState(); }));
}

void UTDGroundSceneSubsystem::ApplyCompletedState()
{
	for (const TWeakObjectPtr<ATDEncounterVolume>& E : Encounters)
	{
		ATDEncounterVolume* Enc = E.Get();
		if (Enc && Enc->LinkedObjective && Enc->LinkedObjective->IsComplete())
		{
			Enc->MarkClearedOnLoad();
		}
	}
	for (const TWeakObjectPtr<ATDInteractableActor>& I : Interactables)
	{
		ATDInteractableActor* Act = I.Get();
		if (Act && Act->LinkedObjective && Act->LinkedObjective->IsComplete())
		{
			Act->bEnabled = false;
		}
	}
	OnCompletedOnLoad.Broadcast();
}

void UTDGroundSceneSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UTDGameInstance* GI = Cast<UTDGameInstance>(World->GetGameInstance()))
		{
			GI->OnSceneRestart.Remove(RestartHandle);
		}
	}
	Super::Deinitialize();
}

void UTDGroundSceneSubsystem::RegisterObjective(ATDObjective* Objective) { if (Objective) { Objectives.AddUnique(Objective); } }
void UTDGroundSceneSubsystem::RegisterEncounter(ATDEncounterVolume* Encounter) { if (Encounter) { Encounters.AddUnique(Encounter); } }
void UTDGroundSceneSubsystem::RegisterInteractable(ATDInteractableActor* Interactable) { if (Interactable) { Interactables.AddUnique(Interactable); } }

void UTDGroundSceneSubsystem::Unregister(AActor* Actor)
{
	Objectives.RemoveAll([Actor](const TWeakObjectPtr<ATDObjective>& O) { return !O.IsValid() || O.Get() == Actor; });
	Encounters.RemoveAll([Actor](const TWeakObjectPtr<ATDEncounterVolume>& E) { return !E.IsValid() || E.Get() == Actor; });
	Interactables.RemoveAll([Actor](const TWeakObjectPtr<ATDInteractableActor>& I) { return !I.IsValid() || I.Get() == Actor; });
}

void UTDGroundSceneSubsystem::NotifyStepStarted(ATDObjective* Objective, int32 StepIndex)
{
	for (const TWeakObjectPtr<ATDEncounterVolume>& E : Encounters)
	{
		ATDEncounterVolume* Enc = E.Get();
		if (Enc && Enc->LinkedObjective == Objective)
		{
			Enc->HandleStepStarted(Objective, StepIndex);
		}
	}
}

void UTDGroundSceneSubsystem::HandleSceneRestart(FName SceneId, int32 TakeNumber)
{
	ResetUnfinished(SceneId);
}

void UTDGroundSceneSubsystem::ResetUnfinished(FName CurrentScene)
{
	++ResetCount;

	// 1) Encounters first (they listen to nothing now: this is the one reset path).
	for (const TWeakObjectPtr<ATDEncounterVolume>& E : Encounters)
	{
		ATDEncounterVolume* Enc = E.Get();
		if (!Enc)
		{
			continue;
		}
		const bool bObjectiveDone = Enc->LinkedObjective && Enc->LinkedObjective->IsComplete();
		// A room of a step before the objective's checkpoint stays as it was (cleared, doors open).
		const bool bBeforeCheckpoint = Enc->LinkedObjective && Enc->LinkedStep != INDEX_NONE && Enc->LinkedStep < Enc->LinkedObjective->GetCheckpointStep();
		if (!bObjectiveDone && !bBeforeCheckpoint)
		{
			Enc->ResetEncounter();
		}
	}
	// 2) Interactables of unfinished objectives can be used again.
	for (const TWeakObjectPtr<ATDInteractableActor>& I : Interactables)
	{
		ATDInteractableActor* Int = I.Get();
		const bool bBeforeCheckpoint = Int && Int->LinkedObjective && Int->LinkedStep < Int->LinkedObjective->GetCheckpointStep();
		if (Int && !bBeforeCheckpoint && (!Int->LinkedObjective || !Int->LinkedObjective->IsComplete()))
		{
			Int->ResetUse();
		}
	}
	// 3) Objectives: reset every unfinished one; restart the ones that were running, current scene last.
	TArray<ATDObjective*> Restart;
	ATDObjective* Current = nullptr;
	for (const TWeakObjectPtr<ATDObjective>& O : Objectives)
	{
		ATDObjective* Obj = O.Get();
		if (!Obj || Obj->IsComplete())
		{
			continue;
		}
		const bool bWasRunning = Obj->IsActive() || Obj->bAutoStart;
		Obj->ResetObjective();
		if (bWasRunning)
		{
			if (!CurrentScene.IsNone() && Obj->ObjectiveId == CurrentScene)
			{
				Current = Obj;
			}
			else
			{
				Restart.Add(Obj);
			}
		}
	}
	// Planet props reset before the objectives start again (a step-start may re-arm them).
	OnResetUnfinished.Broadcast();
	for (ATDObjective* Obj : Restart)
	{
		Obj->StartObjective();
	}
	if (Current)
	{
		Current->StartObjective();
	}
	// 4) A player respawned inside an armed room starts that fight. Next tick: the game mode / planet
	//    director move the player back in their own OnSceneRestart handlers, after this one.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			for (const TWeakObjectPtr<ATDEncounterVolume>& E : Encounters)
			{
				if (ATDEncounterVolume* Enc = E.Get())
				{
					Enc->ActivateIfPlayerInside();
				}
			}
		}));
	}
}
