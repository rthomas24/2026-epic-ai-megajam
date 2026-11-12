// Triad Drift - placeable gravity well (ENG-15). Owner: Teammate 1.

#include "TriadDrift/Travel/TDGravityWellMarker.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "Components/SceneComponent.h"

ATDGravityWellMarker::ATDGravityWellMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ATDGravityWellMarker::BeginPlay()
{
	Super::BeginPlay();
	SetWellEnabled(bWellEnabled);
}

void ATDGravityWellMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->UnregisterGravityWell(GetResolvedId());
	}
	Super::EndPlay(EndPlayReason);
}

void ATDGravityWellMarker::SetWellEnabled(bool bEnabled)
{
	bWellEnabled = bEnabled;
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		if (bEnabled)
		{
			Travel->RegisterGravityWell(GetResolvedId(), GetActorLocation(), Radius, this);
		}
		else
		{
			Travel->UnregisterGravityWell(GetResolvedId());
		}
	}
}
