// Triad Drift - objective framework (ENG-24). Owner: Teammate 5.

#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Ground/TDGroundScene.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ATDObjective::ATDObjective()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ATDObjective::BeginPlay()
{
	Super::BeginPlay();
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->RegisterObjective(this); // retakes reset us through the scene registry
	}
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		// Already done in this run (Continue / retake after completion): stay complete.
		if (!ObjectiveId.IsNone() && GI->GetProgressRef().CompletedObjectives.Contains(ObjectiveId))
		{
			bComplete = true;
			return;
		}
	}
	if (bAutoStart)
	{
		StartObjective();
	}
}

void ATDObjective::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDGroundSceneSubsystem* Scene = UTDGroundSceneSubsystem::Get(this))
	{
		Scene->Unregister(this);
	}
	if (UTDObjectiveSubsystem* Sub = UTDObjectiveSubsystem::Get(this))
	{
		Sub->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ATDObjective::StartObjective()
{
	if (bActive || bComplete)
	{
		return;
	}
	if (Steps.Num() == 0)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("Objective %s has no steps; completing immediately."), *ObjectiveId.ToString());
	}
	bActive = true;
	CurrentStep = 0;
	if (UTDObjectiveSubsystem* Sub = UTDObjectiveSubsystem::Get(this))
	{
		Sub->Register(this);
	}
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->BeginScene(ObjectiveId); // slate: "SCENE n · <objective> · TAKE t"
	}
	if (Steps.Num() == 0)
	{
		Finish();
		return;
	}
	Broadcast(0, Steps[0].Text);
	OnStepStarted.Broadcast(this, 0);
}

bool ATDObjective::CompleteStep(int32 StepIndex)
{
	if (!bActive || StepIndex != CurrentStep)
	{
		return false;
	}
	++CurrentStep;
	if (CurrentStep >= Steps.Num())
	{
		Finish();
		return true;
	}
	Broadcast(CurrentStep, Steps[CurrentStep].Text);
	OnStepStarted.Broadcast(this, CurrentStep);
	return true;
}

bool ATDObjective::AdvanceStep()
{
	return CompleteStep(CurrentStep);
}

void ATDObjective::Finish()
{
	bActive = false;
	bComplete = true;
	if (UTDObjectiveSubsystem* Sub = UTDObjectiveSubsystem::Get(this))
	{
		Sub->Unregister(this);
	}
	Broadcast(Steps.Num(), NSLOCTEXT("TriadDrift", "ObjectiveComplete", "Objective complete"));
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->CompleteObjective(ObjectiveId, RewardAbility, RewardShip); // unlock + reward screen + checkpoint
	}
	OnCompleted.Broadcast(this);
}

void ATDObjective::ResetObjective()
{
	bActive = false;
	bComplete = false;
	CurrentStep = 0;
	if (UTDObjectiveSubsystem* Sub = UTDObjectiveSubsystem::Get(this))
	{
		Sub->Unregister(this);
	}
	OnReset.Broadcast(this);
}

FText ATDObjective::GetCurrentStepText() const
{
	return (bActive && Steps.IsValidIndex(CurrentStep)) ? Steps[CurrentStep].Text : FText::GetEmpty();
}

AActor* ATDObjective::GetCurrentWaypoint() const
{
	return (bActive && Steps.IsValidIndex(CurrentStep)) ? Steps[CurrentStep].Waypoint.Get() : nullptr;
}

void ATDObjective::Broadcast(int32 Step, const FText& Text) const
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastObjectiveUpdated(ObjectiveId, Step, Text);
	}
}

// ------------------------------------------------------------------------------------------------

UTDObjectiveSubsystem* UTDObjectiveSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDObjectiveSubsystem>() : nullptr;
}

void UTDObjectiveSubsystem::Register(ATDObjective* Objective)
{
	if (Objective)
	{
		Active.AddUnique(Objective);
	}
}

void UTDObjectiveSubsystem::Unregister(ATDObjective* Objective)
{
	Active.RemoveAll([Objective](const TWeakObjectPtr<ATDObjective>& O) { return !O.IsValid() || O.Get() == Objective; });
}

void UTDObjectiveSubsystem::GetActiveObjectives(TArray<ATDObjective*>& Out) const
{
	for (const TWeakObjectPtr<ATDObjective>& O : Active)
	{
		if (ATDObjective* Obj = O.Get(); Obj && Obj->IsActive())
		{
			Out.Add(Obj);
		}
	}
}

void UTDObjectiveSubsystem::GetActiveWaypoints(TArray<AActor*>& Out) const
{
	for (const TWeakObjectPtr<ATDObjective>& O : Active)
	{
		if (const ATDObjective* Obj = O.Get())
		{
			if (AActor* W = Obj->GetCurrentWaypoint())
			{
				Out.Add(W);
			}
		}
	}
}
