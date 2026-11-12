// Triad Drift - atmosphere entry and atmosphere flight zone (ENG-20). Owner: Teammate 1.

#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/BoxComponent.h"

namespace
{
	/** Every live squad ship (player team), without GetAllActorsOfClass. */
	void GetSquadShips(const UObject* Context, TArray<ATDShip*>& Out)
	{
		Out.Reset();
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(Context))
		{
			TArray<AActor*> Actors;
			Registry->GetActorsOfTeam(ETDTeam::Player, Actors, true);
			for (AActor* Actor : Actors)
			{
				if (ATDShip* Ship = Cast<ATDShip>(Actor))
				{
					Out.Add(Ship);
				}
			}
		}
	}
}

// ====================================================================== UTDAtmosphereEntryComponent

UTDAtmosphereEntryComponent::UTDAtmosphereEntryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

float UTDAtmosphereEntryComponent::GetEntry01() const
{
	if (EntryState == ETDAtmosphereEntryState::Idle)
	{
		return 0.f;
	}
	return EntrySeconds > 0.f ? FMath::Clamp(EntryTime / EntrySeconds, 0.f, 1.f) : 1.f;
}

bool UTDAtmosphereEntryComponent::BeginEntry(ETDDestination InDestination, FName InReturnPoint, FVector PlanetCenter)
{
	ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner());
	if (!Craft || EntryState != ETDAtmosphereEntryState::Idle)
	{
		return false;
	}
	EntryState = ETDAtmosphereEntryState::Entering;
	EntryTime = 0.f;
	Destination = InDestination;
	ReturnPoint = InReturnPoint;

	if (ATDShip* Ship = Cast<ATDShip>(Craft))
	{
		if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			Hyper->SetHyperdriveEnabled(false);
		}
	}
	// Committed: point into the planet if we're grazing it backwards, then hold speed with reduced steering.
	const FVector ToPlanet = (PlanetCenter - Craft->GetActorLocation()).GetSafeNormal();
	if (!ToPlanet.IsNearlyZero() && FVector::DotProduct(ToPlanet, Craft->GetActorForwardVector()) < 0.f)
	{
		Craft->TeleportSpacecraft(Craft->GetActorLocation(), ToPlanet.Rotation(), true);
	}
	Craft->SetSpeedOverride(EntrySpeed, EntrySteeringScale);
	if (TravelFX)
	{
		UTDStatics::PlayCueAttached(TravelFX->AtmosphericEntry, Craft->GetRootComponent());
	}
	OnEntryStarted.Broadcast(this);
	if (EntrySeconds <= 0.f)
	{
		FinishEntry();
	}
	return true;
}

void UTDAtmosphereEntryComponent::FinishEntry()
{
	EntryState = ETDAtmosphereEntryState::Travelling;
	OnEntryCompleted.Broadcast(this);

	bool bTravelled = false;
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		bTravelled = Flow->TravelTo(Destination, ReturnPoint);
	}
	if (!bTravelled)
	{
		// No flow (tests, sandbox) or the planet map is missing: hand the ship back.
		UE_LOG(LogTriadDrift, Warning, TEXT("Atmosphere entry: TravelTo(%s) did not happen; returning control."),
			*StaticEnum<ETDDestination>()->GetNameStringByValue((int64)Destination));
		if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner()))
		{
			Craft->ClearSpeedOverride();
			if (ATDShip* Ship = Cast<ATDShip>(Craft))
			{
				if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
				{
					Hyper->SetHyperdriveEnabled(true);
				}
			}
		}
		EntryState = ETDAtmosphereEntryState::Idle;
	}
}

void UTDAtmosphereEntryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (EntryState == ETDAtmosphereEntryState::Entering)
	{
		EntryTime += DeltaTime;
		if (EntryTime >= EntrySeconds)
		{
			FinishEntry();
		}
	}
}

// ========================================================================== ATDAtmosphereFlightZone

ATDAtmosphereFlightZone::ATDAtmosphereFlightZone()
{
	PrimaryActorTick.bCanEverTick = true;
	ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
	ZoneBox->SetBoxExtent(FVector(300000.f, 300000.f, 80000.f));
	ZoneBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // the flight model does the walls
	ZoneBox->ShapeColor = FColor(120, 255, 140);
	RootComponent = ZoneBox;
}

FBox ATDAtmosphereFlightZone::GetZoneBox() const
{
	const FVector Extent = ZoneBox ? ZoneBox->GetScaledBoxExtent() : FVector::ZeroVector;
	const FVector Center = GetActorLocation();
	return FBox(Center - Extent, Center + Extent);
}

bool ATDAtmosphereFlightZone::IsShipInZone(const ATDShip* Ship) const
{
	return Ship && GetZoneBox().ExpandBy(FVector(0.f, 0.f, ExitHeightAboveCeiling)).IsInsideOrOn(Ship->GetActorLocation());
}

bool ATDAtmosphereFlightZone::IsExitAllowed() const
{
	if (bExitOverride)
	{
		return bExitAllowedOverride;
	}
	if (RequiredObjectives.Num() == 0)
	{
		return true;
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		const FTDProgress Progress = Flow->GetProgress();
		for (const FName& Id : RequiredObjectives)
		{
			if (!Progress.CompletedObjectives.Contains(Id))
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

void ATDAtmosphereFlightZone::ApplyToShip(ATDShip* Ship, bool bExitOpen)
{
	UTDShipMovementComponent* Move = Ship->GetShipMovement();
	if (!Move)
	{
		return;
	}
	FBox Box = GetZoneBox();
	if (bExitOpen)
	{
		Box.Max.Z += ExitHeightAboveCeiling * 4.f; // let the player climb through the ceiling
	}
	if (!Move->IsInAtmosphere())
	{
		Move->SetAtmosphereMode(true, Gravity, SpeedCapFraction);
		if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			Hyper->CancelSpool();
		}
	}
	Move->SetBoxBoundary(Box);
	if (ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(Ship->GetController()))
	{
		PC->SetFlightContext(ETDFlightContext::Atmosphere);
	}
	ManagedShips.AddUnique(Ship);
}

void ATDAtmosphereFlightZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bExitOpen = IsExitAllowed();
	const FBox Box = GetZoneBox();

	TArray<ATDShip*> Ships;
	GetSquadShips(this, Ships);
	for (ATDShip* Ship : Ships)
	{
		if (!IsShipInZone(Ship))
		{
			continue;
		}
		ApplyToShip(Ship, bExitOpen);

		if (!Ship->IsPlayerFlown() || bExitTriggered)
		{
			continue;
		}
		const bool bLanding = Ship->GetLanding() && Ship->GetLanding()->IsBusy();
		const float Z = Ship->GetActorLocation().Z;
		if (bExitOpen && !bLanding && Z >= Box.Max.Z + ExitHeightAboveCeiling)
		{
			bExitTriggered = true;
			OnExitTriggered.Broadcast(Ship);
			if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
			{
				Flow->TravelTo(ETDDestination::Galaxy, FName(*FString::Printf(TEXT("%s_Orbit"), *PlanetId.ToString())));
			}
		}
		else if (!bExitOpen && Z >= Box.Max.Z - 500.f)
		{
			if (!bTipShown && CeilingBlockedTip != NAME_None)
			{
				bTipShown = true;
				if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
				{
					Flow->ShowTip(CeilingBlockedTip);
				}
			}
		}
		else if (Z < Box.Max.Z - 5000.f)
		{
			bTipShown = false; // show it again on the next approach
		}
	}
}
