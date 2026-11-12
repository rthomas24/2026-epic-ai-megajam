// Triad Drift - planet travel rig (ENG-20). Owner: Teammate 1.

#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ATDPlanetTravelRig::ATDPlanetTravelRig()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	GroundSlab = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderGround"));
	GroundSlab->SetupAttachment(RootComponent);
	if (Cube.Succeeded())
	{
		GroundSlab->SetStaticMesh(Cube.Object);
	}
	// 100 cm cube: 4 km x 4 km slab, 2 m thick, top flush with the actor origin (the pad's base).
	GroundSlab->SetRelativeScale3D(FVector(4000.f, 4000.f, 2.f));
	GroundSlab->SetRelativeLocation(FVector(0.f, 0.f, -100.f));
	GroundSlab->SetCollisionProfileName(TEXT("BlockAll"));
}

ATDPlanetTravelRig* ATDPlanetTravelRig::SpawnForPlanet(UWorld* World, FName InPlanetId, const FTransform& PadTransform,
	const TArray<FName>& InRequiredObjectives, bool bInPlaceholderGround)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	ATDPlanetTravelRig* Rig = World->SpawnActor<ATDPlanetTravelRig>(ATDPlanetTravelRig::StaticClass(), PadTransform, Params);
	if (Rig)
	{
		Rig->PlanetId = InPlanetId;
		Rig->RequiredObjectives = InRequiredObjectives;
		Rig->bPlaceholderGround = bInPlaceholderGround; // before BeginPlay (review minor 5)
		Rig->FinishSpawning(PadTransform);
	}
	return Rig;
}

void ATDPlanetTravelRig::BeginPlay()
{
	Super::BeginPlay();
	SetPlaceholderGround(bPlaceholderGround);
	BuildRig();
}

void ATDPlanetTravelRig::SetPlaceholderGround(bool bEnabled)
{
	bPlaceholderGround = bEnabled;
	if (GroundSlab)
	{
		GroundSlab->SetVisibility(bEnabled);
		GroundSlab->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

void ATDPlanetTravelRig::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Zone) { Zone->Destroy(); }
	if (PlayerPad) { PlayerPad->Destroy(); }
	for (ATDLandingPad* Pad : WingmanPads) { if (Pad) { Pad->Destroy(); } }
	Super::EndPlay(EndPlayReason);
}

void ATDPlanetTravelRig::BuildRig()
{
	UWorld* World = GetWorld();
	if (bBuilt || !World)
	{
		return;
	}
	bBuilt = true;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	const FTransform PadXf = GetActorTransform();

	// Wingman pads first so the player pad can link them as SidePads.
	for (const float Side : { -1.f, 1.f })
	{
		const FVector Offset(-400.f, Side * WingmanPadSpacing, 0.f);
		ATDLandingPad* Pad = World->SpawnActor<ATDLandingPad>(ATDLandingPad::StaticClass(), PadXf.TransformPosition(Offset), PadXf.Rotator(), Params);
		if (Pad)
		{
			Pad->SetWingmanPad(true);
			WingmanPads.Add(Pad);
			WingmanPadsRaw.Add(Pad);
		}
	}
	PlayerPad = World->SpawnActor<ATDLandingPad>(ATDLandingPad::StaticClass(), PadXf.GetLocation(), PadXf.Rotator(), Params);
	if (PlayerPad)
	{
		PlayerPad->SetSidePads(WingmanPadsRaw);
	}

	const FVector ZoneCenter = PadXf.GetLocation() + FVector(0.f, 0.f, ZoneExtent.Z - ZoneBelowPad);
	Zone = World->SpawnActor<ATDAtmosphereFlightZone>(ATDAtmosphereFlightZone::StaticClass(), ZoneCenter, FRotator::ZeroRotator, Params);
	if (Zone)
	{
		Zone->PlanetId = PlanetId;
		Zone->RequiredObjectives = RequiredObjectives;
		Zone->SetZoneExtent(ZoneExtent);
	}
}

FTransform ATDPlanetTravelRig::GetDescentStart() const
{
	const FTransform PadXf = GetActorTransform();
	const FVector Start = PadXf.TransformPosition(DescentStartOffset);
	const FVector ToPad = (PadXf.GetLocation() + FVector(0.f, 0.f, 3000.f) - Start).GetSafeNormal();
	return FTransform(ToPad.Rotation(), Start);
}
