// Triad Drift - World (Teammate 4).
#include "TriadDrift/World/TDPlanetAtmosphere.h"

#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATDPlanetAtmosphere::ATDPlanetAtmosphere()
{
	PrimaryActorTick.bCanEverTick = true;

	Atmosphere = CreateDefaultSubobject<USphereComponent>(TEXT("Atmosphere"));
	SetRootComponent(Atmosphere);
	// Overlap EVERYTHING, including the custom Ship / Projectile channels (whose default response
	// is Block, which would make the atmosphere an invisible wall).
	Atmosphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Atmosphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	Atmosphere->SetGenerateOverlapEvents(true);
	Atmosphere->SetSphereRadius(AtmosphereRadius);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	PlanetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Planet"));
	PlanetMesh->SetupAttachment(Atmosphere);
	PlanetMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (Sphere.Succeeded()) { PlanetMesh->SetStaticMesh(Sphere.Object); }
}

ETDGamePhase ATDPlanetAtmosphere::PhaseFor(ETDDestination InDestination)
{
	switch (InDestination)
	{
	case ETDDestination::Verdance: return ETDGamePhase::Verdance;
	case ETDDestination::Cinder:   return ETDGamePhase::Cinder;
	default:                       return ETDGamePhase::Victory;	// never matches a space phase
	}
}

void ATDPlanetAtmosphere::Configure(ETDDestination InDestination, FName InReturnPoint, float InPlanetRadius, float InAtmosphereRadius, const FLinearColor& InColor)
{
	Destination = InDestination;
	ReturnPoint = InReturnPoint;
	PlanetRadius = FMath::Max(100.f, InPlanetRadius);
	AtmosphereRadius = FMath::Max(PlanetRadius + 1000.f, InAtmosphereRadius);
	Color = InColor;
	Atmosphere->SetSphereRadius(AtmosphereRadius);
	PlanetMesh->SetRelativeScale3D(FVector(PlanetRadius * 2.f / 100.f));
	UTDStatics::TintPlaceholder(PlanetMesh, Color, 0.f);
}

void ATDPlanetAtmosphere::BeginPlay()
{
	Super::BeginPlay();
	Configure(Destination, ReturnPoint, PlanetRadius, AtmosphereRadius, Color);
	Atmosphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ATDPlanetAtmosphere::HandleOverlap);
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnMarkerRevealed.AddUniqueDynamic(this, &ATDPlanetAtmosphere::HandleMarkerRevealed);
	}
	if (UTDWorldSubsystem* WorldSys = UTDWorldSubsystem::Get(this))
	{
		WorldSys->AddObstacle(GetActorLocation(), PlanetRadius);
	}
	// Hyperdrive drops out in orbit, outside the entry trigger (T1's gravity wells).
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->RegisterGravityWell(GetWellId(), GetActorLocation(), AtmosphereRadius * GravityWellRadiusScale, this);
	}
	SyncWithProgress();
}

FName ATDPlanetAtmosphere::GetWellId() const
{
	const UEnum* E = StaticEnum<ETDDestination>();
	return FName(*FString::Printf(TEXT("Planet_%s"), E ? *E->GetNameStringByValue(static_cast<int64>(Destination)) : TEXT("Unknown")));
}

void ATDPlanetAtmosphere::SetPlaceholderVisible(bool bVisible)
{
	if (PlanetMesh) { PlanetMesh->SetVisibility(bVisible); }
}

void ATDPlanetAtmosphere::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnMarkerRevealed.RemoveDynamic(this, &ATDPlanetAtmosphere::HandleMarkerRevealed);
	}
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->UnregisterGravityWell(GetWellId());
	}
	Super::EndPlay(EndPlayReason);
}

void ATDPlanetAtmosphere::SetOpen(bool bInOpen)
{
	if (bOpen == bInOpen)
	{
		return;
	}
	bOpen = bInOpen;
	if (!bOpen)
	{
		bEntered = false;
		PendingTravel = -1.f;
	}
	OnOpenChanged.Broadcast(this, bOpen);

	// A ship already sitting inside when the marker opens still counts.
	if (bOpen)
	{
		TArray<AActor*> Inside;
		Atmosphere->GetOverlappingActors(Inside, ATDSpacecraft::StaticClass());
		for (AActor* A : Inside) { TryEnter(Cast<ATDSpacecraft>(A)); }
	}
}

void ATDPlanetAtmosphere::SyncWithProgress()
{
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		// Open in the planet's phase; closed once the story has moved past it. Earlier phases leave
		// it alone, so a marker opened by OnMarkerRevealed isn't shut before the flow updates.
		const ETDGamePhase Phase = Flow->GetProgress().Phase;
		const ETDGamePhase Mine = PhaseFor(Destination);
		if (Phase == Mine)
		{
			SetOpen(true);
		}
		else if (static_cast<uint8>(Phase) > static_cast<uint8>(Mine))
		{
			SetOpen(false);
		}
	}
}

void ATDPlanetAtmosphere::HandleMarkerRevealed(ETDDestination InDestination)
{
	if (InDestination == Destination)
	{
		SetOpen(true);
	}
}

void ATDPlanetAtmosphere::HandleOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	TryEnter(Cast<ATDSpacecraft>(OtherActor));
}

void ATDPlanetAtmosphere::TryEnter(ATDSpacecraft* Ship)
{
	// Only the ship the player is flying takes the squad down.
	if (!Ship || !bOpen || bEntered || !Ship->IsPlayerControlled())
	{
		return;
	}
	bEntered = true;
	OnShipEnteredAtmosphere.Broadcast(this, Ship);

	// T1's ENG-20 entry sequence takes the squad down and travels itself.
	if (ATDShip* SquadShip = Cast<ATDShip>(Ship))
	{
		if (UTDAtmosphereEntryComponent* Entry = SquadShip->GetAtmosphereEntry())
		{
			if (Entry->BeginEntry(Destination, ReturnPoint, GetActorLocation(), PlanetRadius))	// radius: the dive stops above the surface
			{
				return;
			}
		}
	}
	// Fallback (non-squad pawns, tests): travel directly after a beat.
	if (bAutoTravel)
	{
		PendingTravel = EntryDelaySeconds;
	}
}

void ATDPlanetAtmosphere::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Cheap progress poll so the atmosphere opens/closes with the phase (resume, cheats, retakes).
	PollTimer -= DeltaSeconds;
	if (PollTimer <= 0.f)
	{
		PollTimer = 1.f;
		SyncWithProgress();
	}

	// Fallback to overlap events: the player's ship inside the atmosphere radius enters it.
	if (bOpen && !bEntered)
	{
		if (ATDSpacecraft* Player = Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			if (FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= FMath::Square(AtmosphereRadius))
			{
				TryEnter(Player);
			}
		}
	}

	if (PendingTravel >= 0.f)
	{
		PendingTravel -= DeltaSeconds;
		if (PendingTravel < 0.f && bAutoTravel)
		{
			if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
			{
				Flow->TravelTo(Destination, ReturnPoint);
			}
		}
	}
}
