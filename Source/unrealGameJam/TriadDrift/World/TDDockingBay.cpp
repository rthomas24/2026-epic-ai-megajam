// Triad Drift - World (Teammate 4). ENG-29 docking bay: approach cone, tractor-beam assist, hand-off to the interior.
#include "TriadDrift/World/TDDockingBay.h"

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "GameFramework/Controller.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Story/TDStorySubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

const FName ATDDockingBay::InteriorEntry(TEXT("Hangar"));
const FName ATDDockingBay::TractorOverride(TEXT("Tractor"));

ATDDockingBay::ATDDockingBay()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	UStaticMesh* CubeMesh = Cube.Succeeded() ? Cube.Object : nullptr;
	auto Part = [this, CubeMesh](const TCHAR* Name)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		if (CubeMesh) { C->SetStaticMesh(CubeMesh); }
		C->SetCollisionProfileName(TEXT("NoCollision"));	// visual only: the hull's collision is the Dominion's
		C->SetCastShadow(false);
		return C;
	};
	DoorLeft = Part(TEXT("DoorL"));
	DoorRight = Part(TEXT("DoorR"));
	Frame = Part(TEXT("Frame"));
	GuideLights = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GuideLights"));
	GuideLights->SetupAttachment(Root);
	if (Sphere.Succeeded()) { GuideLights->SetStaticMesh(Sphere.Object); }
	GuideLights->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GuideLights->SetCastShadow(false);
	GuideLights->SetVisibility(false);
}

void ATDDockingBay::BuildPlaceholder()
{
	// Doors on the bow face (+X out): two dark slabs that slide apart, a red-orange frame, white guide lights.
	const float H = DoorHalfWidth * 0.7f;
	DoorLeft->SetRelativeScale3D(FVector(300.f, DoorHalfWidth, H * 2.f) / 100.f);
	DoorRight->SetRelativeScale3D(FVector(300.f, DoorHalfWidth, H * 2.f) / 100.f);
	DoorLeft->SetRelativeLocation(FVector(0.f, -DoorHalfWidth * 0.5f, 0.f));
	DoorRight->SetRelativeLocation(FVector(0.f, DoorHalfWidth * 0.5f, 0.f));
	Frame->SetRelativeScale3D(FVector(150.f, DoorHalfWidth * 2.6f, H * 2.6f) / 100.f);
	Frame->SetRelativeLocation(FVector(-200.f, 0.f, 0.f));
	UTDStatics::TintPlaceholder(DoorLeft, FLinearColor(0.02f, 0.02f, 0.025f), 0.f);
	UTDStatics::TintPlaceholder(DoorRight, FLinearColor(0.02f, 0.02f, 0.025f), 0.f);
	UTDStatics::TintPlaceholder(Frame, FLinearColor(1.f, 0.3f, 0.05f), 4.f);
	UTDStatics::TintPlaceholder(GuideLights, FLinearColor(1.f, 1.f, 1.f), 12.f);
	GuideLights->ClearInstances();
	for (int32 i = 1; i <= 12; ++i)
	{
		const float X = i * (ConeDepth / 13.f);
		for (const float Side : { -1.f, 1.f })
		{
			GuideLights->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (DoorHalfWidth + X * 0.08f), -H), FVector(6.f)));
		}
	}
}

void ATDDockingBay::Open(ATDDominion* InDominion, bool bPlayCues)
{
	Dominion = InDominion;
	if (DockingState != ETDDockingState::Closed)
	{
		return;
	}
	BuildPlaceholder();
	DockingState = ETDDockingState::Open;
	DoorTime = bPlayCues ? 0.f : DoorOpenSeconds;	// a resumed save / retake finds the doors already open
	GuideLights->SetVisibility(true);
	if (bPlayCues && InDominion && InDominion->Visuals)
	{
		UTDStatics::PlayCue(this, InDominion->Visuals->DockingBayOpen, GetActorLocation(), GetActorRotation());
	}
	UE_LOG(LogTriadDrift, Log, TEXT("ATDDockingBay: open (cone %.0f deg, %.0f m)"), ConeHalfAngleDeg, ConeDepth / 100.f);
}

void ATDDockingBay::ResetForNewTake()
{
	if (DockingState == ETDDockingState::Docked)
	{
		return;	// the travel is already under way
	}
	ReleaseShip();
	ReleaseWingmen();
	if (DockingState == ETDDockingState::Gliding)
	{
		DockingState = ETDDockingState::Open;
	}
	GlideTime = 0.f;
}

float ATDDockingBay::GetTractorStrength(const FVector& WorldPoint) const
{
	if (DockingState == ETDDockingState::Closed || ConeDepth <= 0.f)
	{
		return 0.f;
	}
	const FVector D = WorldPoint - GetMouth();
	const float Along = FVector::DotProduct(D, GetOutward());
	if (Along <= 0.f || Along > ConeDepth)
	{
		return 0.f;
	}
	const float Lateral = (D - Along * GetOutward()).Size();
	if (FMath::RadiansToDegrees(FMath::Atan2(Lateral, Along)) > ConeHalfAngleDeg)
	{
		return 0.f;
	}
	// Grows toward the mouth; softly zero at the cone's far end so entering it never jerks the ship.
	return FMath::Clamp(1.f - Along / ConeDepth, 0.f, 1.f);
}

void ATDDockingBay::SetShipOverride(ATDShip* Ship)
{
	ShipOverride = Ship;
}

ATDShip* ATDDockingBay::GetShip() const
{
	if (ShipOverride.IsValid())
	{
		return ShipOverride.Get();
	}
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	return Squad ? Squad->GetActiveShip() : nullptr;
}

void ATDDockingBay::ReleaseShip()
{
	if (ATDShip* Ship = Held.Get())
	{
		Ship->ClearSpeedOverrideFor(TractorOverride);
		if (Ship->IsFlightParked() && DockingState == ETDDockingState::Gliding)
		{
			Ship->SetFlightParked(false);
			if (UTDHealthComponent* H = UTDStatics::GetHealth(Ship)) { H->SetInvulnerable(false); }
		}
	}
	Held.Reset();
	CurrentPull = 0.f;
}

void ATDDockingBay::BeginGlide(ATDShip* Ship)
{
	DockingState = ETDDockingState::Gliding;
	GlideTime = 0.f;
	GlideFrom = Ship->GetActorLocation();
	GlideFromRot = Ship->GetActorQuat();
	Ship->ClearSpeedOverrideFor(TractorOverride);
	Ship->SetFlightParked(true);	// controls locked: the bay has the ship now
	UTDStorySubsystem::Fire(this, TEXT("G6_Wren_CoverTheBay"));	// the docking commit (once per run)
	if (UTDHealthComponent* H = UTDStatics::GetHealth(Ship)) { H->SetInvulnerable(true); }
	Held = Ship;
	HoldWingmen(Ship);
}

void ATDDockingBay::HoldWingmen(const ATDShip* Docking)
{
	// Gameplay 06: the wingmen break off and hold a short orbit outside the bay, so the player goes in alone. Their AI
	// pauses (it would follow its leader into the mouth) and the bay flies them; a retake hands them back.
	ReleaseWingmen();
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	if (!Squad)
	{
		return;
	}
	int32 k = 0;
	for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		ATDShip* W = Squad->GetShip(Id);
		const UTDHealthComponent* H = W ? UTDStatics::GetHealth(W) : nullptr;
		if (!W || W == Docking || W->IsHidden() || (H && H->IsDead()))
		{
			continue;
		}
		if (AController* C = W->GetController()) { C->SetActorTickEnabled(false); }
		W->SetFlightParked(true);
		if (UTDHealthComponent* WH = UTDStatics::GetHealth(W)) { WH->SetInvulnerable(true); }
		FHeldWingman Held1;
		Held1.Ship = W;
		Held1.Angle = 90.f + 180.f * k++;	// opposite sides of the orbit
		HeldWingmen.Add(Held1);
	}
	TickWingmen(0.f);
}

void ATDDockingBay::ReleaseWingmen()
{
	for (const FHeldWingman& W : HeldWingmen)
	{
		ATDShip* Ship = W.Ship.Get();
		if (!Ship) { continue; }
		Ship->SetFlightParked(false);
		if (AController* C = Ship->GetController()) { C->SetActorTickEnabled(true); }
		if (UTDHealthComponent* H = UTDStatics::GetHealth(Ship)) { H->SetInvulnerable(false); }
	}
	HeldWingmen.Reset();
}

void ATDDockingBay::TickWingmen(float DeltaSeconds)
{
	const FVector Centre = GetMouth() + GetOutward() * WingmanOrbitOut;
	const FVector Side = FVector::CrossProduct(FVector::UpVector, GetOutward()).GetSafeNormal();
	const FVector Up = FVector::CrossProduct(GetOutward(), Side).GetSafeNormal();
	for (FHeldWingman& W : HeldWingmen)
	{
		ATDShip* Ship = W.Ship.Get();
		if (!Ship) { continue; }
		W.Angle += WingmanOrbitDegPerSec * DeltaSeconds;
		const float A = FMath::DegreesToRadians(W.Angle);
		const FVector Radial = Side * FMath::Cos(A) + Up * FMath::Sin(A);
		const FVector Tangent = -Side * FMath::Sin(A) + Up * FMath::Cos(A);
		const FVector Target = Centre + Radial * WingmanOrbitRadius;
		// Ease onto the orbit (they may be anywhere around the mouth when docking commits), nose along it.
		const FVector Now = Ship->GetActorLocation();
		const FVector Next = DeltaSeconds > 0.f ? FMath::VInterpTo(Now, Target, DeltaSeconds, 1.5f) : Now;
		Ship->SetActorLocationAndRotation(Next, FMath::QInterpTo(Ship->GetActorQuat(), Tangent.ToOrientationQuat(), DeltaSeconds, 3.f));
	}
}

void ATDDockingBay::FinishDocking()
{
	if (DockingState == ETDDockingState::Docked)
	{
		return;
	}
	DockingState = ETDDockingState::Docked;
	++TravelRequests;
	// T5's contract: SetPhase(DominionInterior) marks the run boarded (Continue goes to the interior).
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->SetPhase(ETDGamePhase::DominionInterior);
		Flow->SaveCheckpoint();
		if (!Flow->TravelTo(ETDDestination::DominionInterior, InteriorEntry))
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("ATDDockingBay: the interior map is missing - travel refused"));
		}
	}
}

void ATDDockingBay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DockingState == ETDDockingState::Closed || DockingState == ETDDockingState::Docked)
	{
		return;
	}
	// Doors slide apart.
	DoorTime = FMath::Min(DoorTime + DeltaSeconds, DoorOpenSeconds);
	const float Open01 = FMath::InterpEaseInOut(0.f, 1.f, DoorTime / FMath::Max(DoorOpenSeconds, 0.01f), 2.f);
	DoorLeft->SetRelativeLocation(FVector(0.f, -DoorHalfWidth * (0.5f + Open01), 0.f));
	DoorRight->SetRelativeLocation(FVector(0.f, DoorHalfWidth * (0.5f + Open01), 0.f));

	if (DockingState == ETDDockingState::Gliding)
	{
		ATDShip* Ship = Held.Get();
		if (!Ship)
		{
			DockingState = ETDDockingState::Open;
			return;
		}
		GlideTime += DeltaSeconds;
		TickWingmen(DeltaSeconds);
		const float T = FMath::Clamp(GlideTime / GlideSeconds, 0.f, 1.f);
		const float E = FMath::InterpEaseInOut(0.f, 1.f, T, 2.f);
		const FVector To = GetMouth() - GetOutward() * GlideDepth;
		Ship->SetActorLocationAndRotation(FMath::Lerp(GlideFrom, To, E), FQuat::Slerp(GlideFromRot, (-GetOutward()).ToOrientationQuat(), E));
		if (T >= 1.f)
		{
			FinishDocking();
		}
		return;
	}

	ATDShip* Ship = GetShip();
	if (Held.IsValid() && Held.Get() != Ship)
	{
		ReleaseShip();	// switched ships: the tractor follows the one the player flies
	}
	if (!Ship || Ship->IsHidden())
	{
		return;
	}
	const UTDHealthComponent* Health = UTDStatics::GetHealth(Ship);
	if (Health && Health->IsDead())
	{
		ReleaseShip();
		return;
	}
	const float Pull = GetTractorStrength(Ship->GetActorLocation());
	CurrentPull = Pull;
	if (Pull <= 0.f)
	{
		if (Held.IsValid()) { ReleaseShip(); }
		return;
	}
	Held = Ship;
	const FVector Loc = Ship->GetActorLocation();
	const FVector D = Loc - GetMouth();
	const float Along = FVector::DotProduct(D, GetOutward());
	const FVector LateralVec = D - Along * GetOutward();
	if (Along <= MouthDistance && LateralVec.Size() <= MouthRadius)
	{
		BeginGlide(Ship);
		return;
	}
	// Speed eases toward the tractor speed and steering softens with depth; it never reaches zero.
	const UTDShipMovementComponent* Move = Ship->GetShipMovement();
	const float Cruise = Move ? Move->GetCruiseCapSpeed() : TractorSpeed;
	Ship->SetSpeedOverrideFor(TractorOverride, FMath::Lerp(Cruise, TractorSpeed, Pull), FMath::Lerp(1.f, MinSteeringScale, Pull));
	// Heading: turn toward the mouth at up to HeadingPullDegPerSec, slower than the pilot's softened turn rate.
	const FQuat Want = (GetMouth() - Loc).GetSafeNormal().ToOrientationQuat();
	const FQuat Now = Ship->GetActorQuat();
	const float Angle = FMath::RadiansToDegrees(Now.AngularDistance(Want));
	if (Angle > KINDA_SMALL_NUMBER)
	{
		const float Step = FMath::Min(1.f, HeadingPullDegPerSec * Pull * DeltaSeconds / Angle);
		Ship->SetActorRotation(FQuat::Slerp(Now, Want, Step));
	}
	// Line: ease onto the axis.
	const float Keep = FMath::Exp(-LineupPerSecond * Pull * DeltaSeconds);
	Ship->SetActorLocation(Loc - LateralVec * (1.f - Keep), true);
}

void ATDDockingBay::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DockingState != ETDDockingState::Docked)
	{
		ReleaseShip();
	}
	Super::EndPlay(EndPlayReason);
}
