// Triad Drift - landing pad (ENG-20). Owner: Teammate 1.

#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "TDLandingPad"

ATDLandingPad::ATDLandingPad()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(Root);
	if (Cylinder.Succeeded())
	{
		PadMesh->SetStaticMesh(Cylinder.Object);
		// 100 cm cylinder centred on its origin: 1000 cm wide, 40 cm thick deck sitting on the origin.
		PadMesh->SetRelativeScale3D(FVector(10.f, 10.f, 0.4f));
		PadMesh->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	}
	PadMesh->SetCollisionProfileName(TEXT("BlockAll"));

	PadArtMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadArtMesh"));
	PadArtMesh->SetupAttachment(Root);
	PadArtMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PadArtMesh->SetGenerateOverlapEvents(false);
	PadArtMesh->SetVisibility(false);

	PromptText = LOCTEXT("HoldToLand", "Hold F to land");
}

void ATDLandingPad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPadArt();
}

void ATDLandingPad::ApplyPadArt()
{
	UStaticMesh* Art = nullptr;
	if (!PadArt.IsNull())
	{
		Art = PadArt.Get();
		if (!Art && FPackageName::DoesPackageExist(PadArt.ToSoftObjectPath().GetLongPackageName()))
		{
			Art = PadArt.LoadSynchronous();
		}
	}
	if (PadArtMesh)
	{
		PadArtMesh->SetStaticMesh(Art);
		PadArtMesh->SetVisibility(Art != nullptr);
	}
	if (PadMesh)
	{
		// The cylinder keeps colliding either way; it only stops drawing once real art shows.
		PadMesh->SetVisibility(Art == nullptr);
	}
}

void ATDLandingPad::BeginPlay()
{
	Super::BeginPlay();
	ApplyPadArt();
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->RegisterPad(this);
	}
}

void ATDLandingPad::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(this))
	{
		Travel->UnregisterPad(this);
	}
	Super::EndPlay(EndPlayReason);
}

FVector ATDLandingPad::GetBeaconLocation() const
{
	return GetActorLocation() + GetActorUpVector() * DeckHeight;
}

bool ATDLandingPad::Reserve(ATDShip* Ship)
{
	if (!Ship || (Occupant.IsValid() && Occupant.Get() != Ship))
	{
		return false;
	}
	Occupant = Ship;
	return true;
}

void ATDLandingPad::Release(ATDShip* Ship)
{
	if (Occupant.Get() == Ship)
	{
		Occupant.Reset();
	}
}

#undef LOCTEXT_NAMESPACE
