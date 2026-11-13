// Triad Drift - World (Teammate 4). The approach to Verdance: convoys with scanning gear and a flickering transponder.
#include "TriadDrift/World/TDVerdanceApproach.h"

#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The run is on its way to Verdance (the phase the galaxy plays between waves 1-2 and the landing). */
	bool TDVerdanceApproach_IsOnTheWay(const UObject* Context)
	{
		const ITDGameFlow* Flow = UTDStatics::GetGameFlow(Context);
		return Flow && Flow->GetProgress().Phase == ETDGamePhase::Verdance;
	}

	const AActor* TDVerdanceApproach_Flown(const UObject* Context)
	{
		const ATDSquadManager* Squad = ATDSquadManager::Get(Context);
		return Squad ? Squad->GetActiveShip() : nullptr;
	}
}

// ------------------------------------------------------------------------------------------------ convoy

ATDArmadaConvoy::ATDArmadaConvoy()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CubeMesh = Cube.Succeeded() ? Cube.Object : nullptr;
	SphereMesh = Sphere.Succeeded() ? Sphere.Object : nullptr;
	CylinderMesh = Cylinder.Succeeded() ? Cylinder.Object : nullptr;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SetActorHiddenInGame(true);
}

void ATDArmadaConvoy::SetupLane(const FVector& A, const FVector& B, int32 Count, bool bInScannable)
{
	LaneA = A;
	LaneB = B;
	bScannable = bInScannable;
	auto Part = [this](UStaticMesh* Mesh, const FVector& Scale, const FLinearColor& Color, float Glow)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// scenery: ships fly past, nothing to shoot
		C->SetupAttachment(Root);
		C->RegisterComponent();
		C->SetWorldScale3D(Scale);
		UTDStatics::TintPlaceholder(C, Color, Glow);
		Parts.Add(C);
		return C;
	};
	const FLinearColor Armour(0.0103f, 0.0109f, 0.0129f);
	const FLinearColor Gear(0.75f, 0.85f, 0.9f);
	for (int32 i = 0; i < Count; ++i)
	{
		FHauler H;
		// 40 x 13 x 11 m: a freighter, readable at 1-2 km (live look, Nov 13: 18 m read as specks at 600 m).
		H.Body = Part(CubeMesh, FVector(40.f, 13.f, 11.f), Armour, 0.f);
		H.PodL = Part(SphereMesh, FVector(7.f), Gear, 6.f);			// sensor pods: the gear it is ferrying, lit
		H.PodR = Part(SphereMesh, FVector(7.f), Gear, 6.f);
		H.Ring = Part(CylinderMesh, FVector(16.f, 16.f, 0.6f), FLinearColor(1.f, 0.35f, 0.08f), 6.f);	// scanner ring, Armada orange
		H.Along = -i * Spacing;
		Haulers.Add(H);
	}
}

FVector ATDArmadaConvoy::GetHaulerLocation(int32 Index) const
{
	return Haulers.IsValidIndex(Index) && Haulers[Index].Body ? Haulers[Index].Body->GetComponentLocation() : FVector::ZeroVector;
}

void ATDArmadaConvoy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PollTimer -= DeltaSeconds;
	if (PollTimer <= 0.f)
	{
		PollTimer = 0.5f;
		const bool bWant = ActiveOverride >= 0 ? ActiveOverride > 0 : TDVerdanceApproach_IsOnTheWay(this);
		if (bWant != bActive)
		{
			bActive = bWant;
			SetActorHiddenInGame(!bActive);
		}
	}
	if (!bActive || Haulers.Num() == 0)
	{
		return;
	}
	const FVector Lane = LaneB - LaneA;
	const float Length = FMath::Max(1.f, (float)Lane.Size());
	const FVector Dir = Lane / Length;
	const FQuat Facing = Dir.ToOrientationQuat();
	RingSpin += DeltaSeconds * 40.f;
	// The column loops along the lane (a hauler past B comes back in at A, out of sight that far off).
	for (FHauler& H : Haulers)
	{
		H.Along += Speed * DeltaSeconds;
		if (H.Along > Length) { H.Along -= Length + Spacing; }
		const FVector P = LaneA + Dir * FMath::Max(0.f, H.Along);
		const bool bOnLane = H.Along >= 0.f;
		for (UStaticMeshComponent* C : { H.Body.Get(), H.PodL.Get(), H.PodR.Get(), H.Ring.Get() }) { if (C) { C->SetVisibility(bOnLane); } }
		H.Body->SetWorldLocationAndRotation(P, Facing);
		H.PodL->SetWorldLocation(P + Facing.RotateVector(FVector(-700.f, -1100.f, 700.f)));
		H.PodR->SetWorldLocation(P + Facing.RotateVector(FVector(-700.f, 1100.f, 700.f)));
		H.Ring->SetWorldLocationAndRotation(P + Facing.RotateVector(FVector(900.f, 0.f, 1000.f)), Facing * FQuat(FRotator(0.f, RingSpin, 0.f)));
	}

	// The scannable hauler (the column's lead): hold close to read what it carries.
	if (!bScannable || bScanned)
	{
		return;
	}
	const FHauler& Lead = Haulers[0];
	const AActor* Ship = TDVerdanceApproach_Flown(this);
	const bool bClose = Ship && Lead.Along >= 0.f && FVector::Dist(Ship->GetActorLocation(), Lead.Body->GetComponentLocation()) <= ScanRadius;
	ScanTime = bClose ? ScanTime + DeltaSeconds : 0.f;	// a hold: drifting out starts it over
	if (bClose != bRingHot)
	{
		// Being read: the ring flares (the only feedback; no HUD mode).
		bRingHot = bClose;
		UTDStatics::TintPlaceholder(Lead.Ring, FLinearColor(1.f, 0.35f, 0.08f), bRingHot ? 20.f : 6.f);
	}
	if (ScanTime >= ScanSeconds)
	{
		bScanned = true;
		UTDStatics::TintPlaceholder(Lead.Ring, FLinearColor(0.6f, 0.9f, 1.f), 6.f);	// read: the ring settles to the gear's cold white
		UTDStorySubsystem::Fire(this, TEXT("A2_ConvoyScanned"));
	}
}

// ------------------------------------------------------------------------------------------------ transponder

ATDTransponderBeacon::ATDTransponderBeacon()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Mast = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mast"));
	SetRootComponent(Mast);
	if (Cylinder.Succeeded()) { Mast->SetStaticMesh(Cylinder.Object); }
	Mast->SetRelativeScale3D(FVector(3.f, 3.f, 120.f));	// a 120 m mast and a 40 m light: findable from ~2 km
	Light = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Light"));
	Light->SetupAttachment(Mast);
	if (Sphere.Succeeded()) { Light->SetStaticMesh(Sphere.Object); }
	Light->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
	Light->SetRelativeScale3D(FVector(13.f, 13.f, 0.35f));	// relative to the mast: ~40 m across
	for (UStaticMeshComponent* C : { Mast.Get(), Light.Get() })
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
	}
	SetActorHiddenInGame(true);
}

void ATDTransponderBeacon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PollTimer -= DeltaSeconds;
	if (PollTimer <= 0.f)
	{
		PollTimer = 0.5f;
		const bool bWant = ActiveOverride >= 0 ? ActiveOverride > 0 : TDVerdanceApproach_IsOnTheWay(this);
		if (bWant != bActive)
		{
			bActive = bWant;
			SetActorHiddenInGame(!bActive);
			if (bActive)
			{
				UTDStatics::TintPlaceholder(Mast, FLinearColor(0.03f, 0.03f, 0.035f), 0.f);
			}
		}
	}
	if (!bActive)
	{
		return;
	}
	// Irregular flicker: a broken rhythm (three overlapping beats), so it reads as failing, not as a beacon.
	Time += DeltaSeconds;
	const float Beat = FMath::Sin(Time * 7.3f) + FMath::Sin(Time * 2.1f + 1.3f) * 0.8f + FMath::Sin(Time * 13.7f + 0.4f) * 0.5f;
	const bool bOn = Beat > 0.35f;
	if (bOn != bLit)
	{
		bLit = bOn;
		UTDStatics::TintPlaceholder(Light, bLit ? FLinearColor(0.35f, 1.f, 0.85f) : FLinearColor(0.02f, 0.05f, 0.05f), bLit ? 40.f : 0.f);
	}
	if (!bSighted)
	{
		const AActor* Ship = TDVerdanceApproach_Flown(this);
		if (Ship && FVector::Dist(Ship->GetActorLocation(), GetActorLocation()) <= NoticeRadius)
		{
			bSighted = true;
			UTDStorySubsystem::Fire(this, TEXT("A2_TransponderSighted"));
		}
	}
}
