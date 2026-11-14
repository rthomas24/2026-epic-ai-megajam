// Triad Drift - World (Teammate 4). ENG-29 docking bay: approach cone, tractor-beam assist, hand-off to the interior.
#include "TriadDrift/World/TDDockingBay.h"
#include "NiagaraComponent.h"
#include "TriadDrift/World/TDWorldBarks.h"

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
	FrameBars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FrameBars"));
	FrameBars->SetupAttachment(Root);
	if (CubeMesh) { FrameBars->SetStaticMesh(CubeMesh); }
	FrameBars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FrameBars->SetCastShadow(false);
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
	// T1 (Nov 14, docking2/H2_10): the frame was one solid lit slab across the mouth, and the glide flew the camera through
	// it (a frame of solid yellow). Now it is four edge bars (Frame is the top one) that leave the opening clear.
	const float BarT = DoorHalfWidth * 0.3f;
	const FVector Outer(150.f, DoorHalfWidth * 2.6f, H * 2.6f);
	Frame->SetRelativeScale3D(FVector(Outer.X, Outer.Y, BarT) / 100.f);
	Frame->SetRelativeLocation(FVector(-200.f, 0.f, (Outer.Z - BarT) * 0.5f));
	FrameBars->ClearInstances();
	FrameBars->AddInstance(FTransform(FRotator::ZeroRotator, FVector(-200.f, 0.f, -(Outer.Z - BarT) * 0.5f), FVector(Outer.X, Outer.Y, BarT) / 100.f));
	for (const float Side : { -1.f, 1.f })
	{
		FrameBars->AddInstance(FTransform(FRotator::ZeroRotator, FVector(-200.f, Side * (Outer.Y - BarT) * 0.5f, 0.f), FVector(Outer.X, BarT, Outer.Z) / 100.f));
	}
	UTDStatics::TintPlaceholder(DoorLeft, FLinearColor(0.02f, 0.02f, 0.025f), 0.f);
	UTDStatics::TintPlaceholder(DoorRight, FLinearColor(0.02f, 0.02f, 0.025f), 0.f);
	UTDStatics::TintPlaceholder(Frame, FLinearColor(1.f, 0.3f, 0.05f), 4.f);
	UTDStatics::TintPlaceholder(FrameBars, FLinearColor(1.f, 0.3f, 0.05f), 4.f);
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
	ShowApproachMarker(true);
	if (bPlayCues && InDominion && InDominion->Visuals)
	{
		UTDStatics::PlayCue(this, InDominion->Visuals->DockingBayOpen, GetActorLocation(), GetActorRotation());
	}
	UE_LOG(LogTriadDrift, Log, TEXT("ATDDockingBay: open (cone %.0f deg, %.0f m)"), ConeHalfAngleDeg, ConeDepth / 100.f);
}

void ATDDockingBay::ResetForNewTake()
{
	bTractorSaid = false;
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
	const float MaxAngle = D.Size() <= CaptureRadius ? FMath::Max(ConeHalfAngleDeg, CaptureHalfAngleDeg) : ConeHalfAngleDeg;
	if (FMath::RadiansToDegrees(FMath::Atan2(Lateral, Along)) > MaxAngle)
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
	bBeamOn = false;
	if (BeamFX) { BeamFX->Deactivate(); }
	if (bTractorShielding)
	{
		bTractorShielding = false;
		if (ATDShip* Was = GetShip()) { if (UTDHealthComponent* H = UTDStatics::GetHealth(Was)) { H->SetInvulnerable(false); } }
	}
}

void ATDDockingBay::UpdateBeam(const ATDShip* Ship, float Pull)
{
	// ENG-37: the beam loop from the mouth to the held ship; parameters every frame. Null-safe without the art.
	bBeamOn = true;
	const ATDDominion* D = Dominion.Get();
	if (!BeamFX && D && D->Visuals && !D->Visuals->TractorBeam.IsEmpty())
	{
		BeamFX = UTDStatics::PlayCueAttached(D->Visuals->TractorBeam, GetRootComponent());
	}
	if (BeamFX)
	{
		if (!BeamFX->IsActive()) { BeamFX->Activate(true); }
		BeamFX->SetVariableFloat(TEXT("User.Length"), Ship ? (float)FVector::Dist(Ship->GetActorLocation(), GetMouth()) : 0.f);
		BeamFX->SetVariableFloat(TEXT("User.Intensity"), FMath::Clamp(Pull, 0.f, 1.f));
	}
}

bool ATDDockingBay::IsBeamActive() const
{
	return bBeamOn;	// the cue's state (its Niagara component exists only where something renders)
}

void ATDDockingBay::ShowApproachMarker(bool bShow)
{
	if (!bShow)
	{
		if (IsValid(ApproachMarker)) { ApproachMarker->Destroy(); }
		ApproachMarker = nullptr;
		return;
	}
	if (IsValid(ApproachMarker) || !GetWorld() || ApproachMarkerDistance <= 0.f)
	{
		return;
	}
	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector At = GetMouth() + GetOutward() * ApproachMarkerDistance;
	if (AActor* M = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), At, (-GetOutward()).Rotation(), P))
	{
		USceneComponent* MRoot = NewObject<USceneComponent>(M, TEXT("Root"));
		MRoot->SetMobility(EComponentMobility::Movable);
		M->SetRootComponent(MRoot);
		MRoot->RegisterComponent();
		M->SetActorLocationAndRotation(At, (-GetOutward()).Rotation());
		M->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);	// rides the Dominion
		M->Tags.Add(TDTags::ObjectiveMarker);
		M->Tags.Add(TEXT("TD.Docking.Approach"));
		ApproachMarker = M;
	}
}

void ATDDockingBay::BeginGlide(ATDShip* Ship)
{
	DockingState = ETDDockingState::Gliding;
	GlideTime = 0.f;
	HoldTime = 0.f;
	GlideFrom = Ship->GetActorLocation();
	GlideFromRot = Ship->GetActorQuat();
	// The glide carries on at the arrival speed and settles over the pad, never faster than the ship's boost (T1, live: a
	// fixed 2 s ease-in-out over 340 m peaked near 340 m/s; Lead, Nov 14). A cubic Hermite with the arrival speed as its
	// start slope: with slope 1 (duration = distance / start speed) it peaks at 4/3 of the start speed, a third of the way in.
	{
		const float Dist = FVector::Dist(GlideFrom, GetMouth() - GetOutward() * GlideDepth);
		const UTDShipMovementComponent* Move = Ship->GetShipMovement();
		const float Boost = Move ? Move->GetFlightTuning().BoostSpeed : 10800.f;
		const float Start = FMath::Clamp((float)Ship->GetVelocity().Size(), 1000.f, Boost * 0.7f);	// peak 0.93 x boost
		GlideDuration = FMath::Clamp(Dist / Start, 1.5f, FMath::Max(GlideSeconds, 1.5f));
		GlideStartSlope = Start * GlideDuration / FMath::Max(Dist, 1.f);
	}
	Ship->ClearSpeedOverrideFor(TractorOverride);
	Ship->SetFlightParked(true);	// controls locked: the bay has the ship now
	ShowApproachMarker(false);
	if (BeamFX) { BeamFX->SetVariableFloat(TEXT("User.Intensity"), 1.f); }	// the glide: full beam until the travel
	UTDStorySubsystem::PlaySequence(this, TEXT("Docking"));	// G6_Wren_CoverTheBay, then C7_Moss_WayOut (storyline 26)	// the docking commit (once per run)
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
		const float T = FMath::Clamp(GlideTime / FMath::Max(GlideDuration, 0.1f), 0.f, 1.f);
		// Hermite: position 0 -> 1, start slope GlideStartSlope (the arrival speed), end slope 0 (settled over the pad).
		const float E = (T * T * T - 2.f * T * T + T) * GlideStartSlope + (3.f * T * T - 2.f * T * T * T);
		const FVector To = GetMouth() - GetOutward() * GlideDepth;
		Ship->SetActorLocationAndRotation(FMath::Lerp(GlideFrom, To, E), FQuat::Slerp(GlideFromRot, (-GetOutward()).ToOrientationQuat(), E));
		if (T >= 1.f)
		{
			// T1 (live, Nov 14): commit to load is now ~5 s, which cut the Docking lines (G6 cover-the-bay, C7 way-out).
			// The ship holds over the pad until the sequence has finished (or is skipped), at most DockingLinesHoldSeconds.
			HoldTime += DeltaSeconds;
			const UTDStorySubsystem* Story = UTDStorySubsystem::Get(this);
			const bool bLinesPlaying = Story && Story->IsSequencePlaying(TEXT("Docking"));
			if (!bLinesPlaying || HoldTime >= DockingLinesHoldSeconds)
			{
				FinishDocking();
			}
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
	if (!Held.IsValid() && !bTractorSaid)
	{
		bTractorSaid = true;	// dialogs.md Bark_Dom_Tractor: the tractor assist takes hold (the flown pilot)
		FTDWorldBarks::Say(this, TDWorldBarkIds::DomTractor, ETDBarkPriority::High, FTDWorldBarks::EWho::FlownPilot, 0.f, Ship);
	}
	Held = Ship;
	UpdateBeam(Ship, Pull);
	const FVector Loc = Ship->GetActorLocation();
	const FVector D = Loc - GetMouth();
	const float Along = FVector::DotProduct(D, GetOutward());
	const FVector LateralVec = D - Along * GetOutward();
	if (Along <= MouthDistance && LateralVec.Size() <= MouthRadius)
	{
		BeginGlide(Ship);
		return;
	}
	// Steering softens with depth all the way; the speed only eases to TractorSpeed in the last SlowZone before the mouth
	// (T1, live, Nov 14: slowing over the whole 3 km cone made a 40 s crawl under fire; the approach should take ~10-20 s).
	const UTDShipMovementComponent* Move = Ship->GetShipMovement();
	const float Cruise = Move ? Move->GetCruiseCapSpeed() : TractorSpeed;
	// Lead (Nov 14): cone edge to commit in about 12 s flown straight. A ship lined up on the axis is reeled in at
	// ReelSpeed; from TakeOverDistance the tractor takes over, slows it to TractorSpeed at the mouth and shields it.
	const float Aligned = FMath::GetMappedRangeValueClamped(FVector2D(0.9f, 0.97f), FVector2D(0.f, 1.f),
		(float)FVector::DotProduct(Ship->GetActorForwardVector(), -GetOutward()));
	const float Reel = FMath::Lerp(Cruise, FMath::Max(Cruise, ReelSpeed), Aligned);
	const float TakeOver = 1.f - FMath::Clamp((Along - MouthDistance) / FMath::Max(TakeOverDistance - MouthDistance, 1.f), 0.f, 1.f);
	Ship->SetSpeedOverrideFor(TractorOverride, FMath::Lerp(Reel, TractorSpeed, FMath::SmoothStep(0.f, 1.f, TakeOver)), FMath::Lerp(1.f, MinSteeringScale, Pull), 30000.f);
	if (UTDHealthComponent* H = UTDStatics::GetHealth(Ship))
	{
		const bool bShield = Along <= TakeOverDistance;	// the tractor has the ship: enemy fire no longer lands
		if (bShield != bTractorShielding) { bTractorShielding = bShield; H->SetInvulnerable(bShield); }
	}
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

bool ATDDockingBay::IsMouthClear(float HalfClearance) const
{
	// The glide's path through the frame's depth, in the bay's space (+X out of the mouth).
	const FBox Path(FVector(-1000.f, -HalfClearance, -HalfClearance), FVector(1000.f, HalfClearance, HalfClearance));
	const FBox Unit(FVector(-50.f), FVector(50.f));	// /Engine/BasicShapes/Cube
	auto Crosses = [&Path, &Unit](const FTransform& Rel) { return Unit.TransformBy(Rel).Intersect(Path); };
	if (Frame && Frame->IsVisible() && Crosses(Frame->GetRelativeTransform())) { return false; }
	if (FrameBars && FrameBars->IsVisible())
	{
		for (int32 i = 0; i < FrameBars->GetInstanceCount(); ++i)
		{
			FTransform T;
			if (FrameBars->GetInstanceTransform(i, T, false) && Crosses(T)) { return false; }
		}
	}
	return true;
}
