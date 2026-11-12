// Triad Drift - Powers (Teammate 2). Rift: Portal Blaster + Blink fallback.

#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"

static TAutoConsoleVariable<int32> CVarPortalBlink(
	TEXT("TD.PortalBlink"), 0,
	TEXT("1 = Rift's Q uses the Blink fallback (teleport + missile-reflect field) instead of portals."),
	ECVF_Default);

namespace
{
	/** What a portal or a blink can't go into. */
	FCollisionObjectQueryParams TDPortalBlockers()
	{
		FCollisionObjectQueryParams Params;
		Params.AddObjectTypesToQuery(ECC_WorldStatic);
		Params.AddObjectTypesToQuery(ECC_WorldDynamic);
		return Params;
	}

	float TDPortalTravellerRadius(const AActor* Actor)
	{
		FVector Origin, Extent;
		Actor->GetActorBounds(true, Origin, Extent, false);
		return Extent.IsNearlyZero() ? 100.f : (float)Extent.GetMax();
	}
}

// ================================================================================== portal actor

ATDPortal::ATDPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	// After movement and projectiles have stepped, so this frame's motion is complete.
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SetCanBeDamaged(false);
}

bool ATDPortal::HasClearance(const UWorld* World, const FVector& Location, float ClearanceRadius)
{
	if (!World || ClearanceRadius <= 0.f)
	{
		return true;
	}
	// Static AND dynamic geometry: the Dominion hull, trench, pylons, gate frames and planets are WorldDynamic.
	if (World->OverlapAnyTestByObjectType(Location, FQuat::Identity, TDPortalBlockers(), FCollisionShape::MakeSphere(ClearanceRadius),
		FCollisionQueryParams(SCENE_QUERY_STAT(TDPortalClearance), false)))
	{
		return false;
	}
	// T4's obstacle registry (asteroid fields etc. that may have no collision yet).
	if (const UTDWorldSubsystem* WorldSub = World->GetSubsystem<UTDWorldSubsystem>())
	{
		if (WorldSub->IsInsideObstacle(Location, ClearanceRadius))
		{
			return false;
		}
	}
	return true;
}

void ATDPortal::Init(AActor* InInstigator, float InRadius, const FTDPortalTransitParams& InParams, UTDTravelFX* InFX, float InExitClearance)
{
	PortalInstigator = InInstigator;
	Radius = InRadius;
	ExitClearance = InExitClearance;
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnSpacecraftTeleported.AddDynamic(this, &ATDPortal::HandleSpacecraftTeleported);
	}
	Params = InParams;
	FX = InFX;
	BroadcastState(ETDPortalState::Opening);
	if (FX)
	{
		UTDStatics::PlayCue(this, FX->PortalOpen, GetActorLocation(), GetActorRotation());
		IdleFX = UTDStatics::PlayCueAttached(FX->PortalIdle, GetRootComponent());
	}
	if (!FX || !FTDPowerPresentation::HasArt(FX->PortalIdle))
	{
		// Shipping-safe placeholder: a thin magenta disc (cylinder axis along the portal normal) with a bright rim.
		const FLinearColor Magenta = UTDStatics::ShipColor(ETDShipId::Rift);
		const float D = Radius * 2.f / 100.f;
		PlaceholderDisc = FTDPowerPresentation::AddShape(this, nullptr, FTDPowerPresentation::EShape::Cylinder, FVector(D, D, 0.04f),
			FRotator(90.f, 0.f, 0.f), Magenta * 0.5f, 2.f);
		if (UInstancedStaticMeshComponent* Rim = FTDPowerPresentation::AddRingLattice(this, nullptr, Radius, 1, 32, 50.f, Magenta, 8.f))
		{
			Rim->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		}
	}
	BroadcastState(ETDPortalState::Open);
}

void ATDPortal::BroadcastState(ETDPortalState State)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->BroadcastPortalState(this, State);
	}
}

void ATDPortal::Close()
{
	if (bClosed)
	{
		return;
	}
	bClosed = true;
	BroadcastState(ETDPortalState::Closing);
	if (FX)
	{
		UTDStatics::PlayCue(this, FX->PortalClose, GetActorLocation(), GetActorRotation());
	}
	if (IdleFX)
	{
		IdleFX->Deactivate();
		IdleFX = nullptr;
	}
	if (ATDPortal* P = Partner.Get())
	{
		if (P->GetPartner() == this)
		{
			P->SetPartner(nullptr);
		}
	}
	BroadcastState(ETDPortalState::Closed);
	Destroy();
}

void ATDPortal::HandleSpacecraftTeleported(AActor* Spacecraft, FVector From, FVector To)
{
	if (Spacecraft)
	{
		LastPositions.Add(Spacecraft, To);
	}
}

void ATDPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(this))
	{
		Bus->OnSpacecraftTeleported.RemoveAll(this);
	}
	bClosed = true;
	Super::EndPlay(EndPlayReason);
}

void ATDPortal::NotifyArrived(const AActor* Traveller, const FVector& Location)
{
	LastPositions.Add(Traveller, Location);
}

void ATDPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bClosed)
	{
		return;
	}
	if (!Partner.IsValid() || DeltaSeconds <= 0.f)
	{
		return;
	}

	// Spacecraft (any team): track their positions frame to frame.
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*, TInlineAllocator<64>> Craft;
		for (const TWeakObjectPtr<AActor>& W : Registry->GetAll())
		{
			AActor* A = W.Get();
			if (A && A->IsA<ATDSpacecraft>())
			{
				Craft.Add(A);
			}
		}
		for (AActor* A : Craft)
		{
			const FVector Cur = A->GetActorLocation();
			FVector* Prev = LastPositions.Find(A);
			if (!Prev)
			{
				LastPositions.Add(A, Cur);
				continue;
			}
			const FVector From = *Prev;
			*Prev = Cur;
			if (TryCross(A, From, Cur))
			{
				LastPositions.Add(A, A->GetActorLocation());
			}
		}
	}

	// Projectiles: their previous position is one step back along their velocity.
	if (UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
	{
		TArray<ATDProjectile*, TInlineAllocator<64>> Live;
		for (ATDProjectile* P : Projectiles->GetLiveProjectiles())
		{
			if (P && P->IsActive())
			{
				Live.Add(P);
			}
		}
		for (ATDProjectile* P : Live)
		{
			const FVector Cur = P->GetActorLocation();
			TryCross(P, P->GetPreviousLocation(), Cur);
		}
	}

	// Forget actors that are gone.
	if (LastPositions.Num() > 64)
	{
		for (auto It = LastPositions.CreateIterator(); It; ++It)
		{
			if (!It->Key.IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}
}

bool ATDPortal::TryCross(AActor* Traveller, const FVector& Prev, const FVector& Cur)
{
	ATDPortal* Exit = Partner.Get();
	if (!Exit)
	{
		return false;
	}
	const FVector P = GetActorLocation();
	const FVector N = GetActorForwardVector();
	const double D0 = FVector::DotProduct(Prev - P, N);
	const double D1 = FVector::DotProduct(Cur - P, N);
	if (D0 * D1 > 0.0 || FMath::IsNearlyEqual(D0, D1))
	{
		return false; // didn't cross the plane this frame
	}
	const double T = D0 / (D0 - D1);
	const FVector Hit = Prev + (Cur - Prev) * T;
	if (FVector::DistSquared(Hit, P) > FMath::Square(Radius))
	{
		return false; // crossed the plane outside the disc
	}

	// Door semantics: the normal faces the viewer. In through the front (against the normal) -> out of the
	// partner's front (along its normal); in through the back -> out of the partner's back.
	const bool bFrontEntry = D1 < D0;
	const FQuat Flip(GetActorUpVector(), PI);
	const FQuat ExitFlip(Exit->GetActorUpVector(), PI);
	const FQuat EntryRot = bFrontEntry ? Flip * GetActorQuat() : GetActorQuat();
	const FQuat ExitRot = bFrontEntry ? Exit->GetActorQuat() : ExitFlip * Exit->GetActorQuat();
	const FTransform Entry(EntryRot, P);
	const FTransform ExitXf(ExitRot, Exit->GetActorLocation());

	// Never put a ship inside geometry, and never squeeze something bigger than the disc through it.
	if (Traveller->IsA<ATDSpacecraft>())
	{
		const float TravellerRadius = TDPortalTravellerRadius(Traveller);
		if (TravellerRadius > Radius)
		{
			return false;
		}
		const FVector Out = UTDPortalTransit::MapLocation(Cur, Entry, ExitXf, Params.ExitForwardOffset);
		if (!HasClearance(GetWorld(), Out, FMath::Max(ExitClearance, TravellerRadius)))
		{
			return false;
		}
	}

	if (!UTDPortalTransit::TransitActor(Traveller, Entry, ExitXf, PortalInstigator.Get(), Params))
	{
		return false;
	}
	++TransitCount;
	Exit->NotifyArrived(Traveller, Traveller->GetActorLocation());
	BroadcastState(ETDPortalState::Transit);
	if (FX)
	{
		UTDStatics::PlayCue(this, FX->PortalTransit, Hit, GetActorRotation());
		UTDStatics::PlayCue(this, FX->PortalTransit, Traveller->GetActorLocation(), Exit->GetActorRotation());
	}
	return true;
}

// ================================================================================== power

UTDPortalBlasterPower::UTDPortalBlasterPower()
{
	Slot = ETDPowerInput::Signature1;
	PowerName = TEXT("PortalBlaster");
	CooldownSeconds = 8.f;
	bCooldownStartsWhenPowerEnds = true;
	bEndsManually = true;
	bEndOnOwnerDeath = false; // open portals stay usable by the wingmen
}

void UTDPortalBlasterPower::BeginPlay()
{
	Super::BeginPlay();
	if (!TravelFX)
	{
		TravelFX = FTDPowerPresentation::LoadTravelFX();
	}
}

bool UTDPortalBlasterPower::IsUsingBlink() const
{
	return bUseBlinkFallback || CVarPortalBlink.GetValueOnGameThread() != 0;
}

void UTDPortalBlasterPower::HandleInputPressed()
{
	if (IsPowerActive() && !IsUsingBlink())
	{
		PlacePortal();
		return;
	}
	TryActivate();
}

FVector UTDPortalBlasterPower::ComputePlacement(FVector& OutFacing) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		OutFacing = FVector::ForwardVector;
		return FVector::ZeroVector;
	}
	FVector Aim = Owner->GetActorForwardVector();
	if (const UTDWeaponComponent* Weapons = Owner->FindComponentByClass<UTDWeaponComponent>())
	{
		Aim = Weapons->GetAimDirection();
	}
	OutFacing = Aim;
	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + Aim * MaxRange;
	const float Offset = IsUsingBlink() ? BlinkSurfaceOffset : SurfaceOffset;

	FHitResult Hit;
	FCollisionQueryParams QParams(SCENE_QUERY_STAT(TDPortalPlace), false, Owner);
	if (GetWorld() && GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, TDPortalBlockers(),
		FCollisionShape::MakeSphere(100.f), QParams))
	{
		const float Dist = FMath::Max(0.f, (Hit.Location - Start).Size() - Offset);
		return Start + Aim * Dist;
	}
	return End;
}

bool UTDPortalBlasterPower::OnActivate()
{
	if (IsUsingBlink())
	{
		bEndsManually = false;
		return DoBlink();
	}
	bEndsManually = true;
	Portals.Reset();
	return PlacePortal() != nullptr;
}

ATDPortal* UTDPortalBlasterPower::PlacePortal()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return nullptr;
	}
	Portals.RemoveAll([](const TWeakObjectPtr<ATDPortal>& P) { return !P.IsValid(); });

	FVector Aim;
	FVector Where = ComputePlacement(Aim);
	const FVector Normal = -Aim; // faces back toward Rift, like a door

	// Bounded push-out: the front exit must have room for a ship, else refuse this placement.
	bool bClear = false;
	for (int32 Step = 0; Step <= PushOutSteps; ++Step)
	{
		const FVector Candidate = Where + Normal * (PushOutStep * Step);
		if (ATDPortal::HasClearance(World, Candidate + Normal * TransitParams.ExitForwardOffset, ExitClearanceRadius))
		{
			Where = Candidate;
			bClear = true;
			break;
		}
	}
	if (!bClear)
	{
		++RefusedPlacements;
		if (TravelFX)
		{
			UTDStatics::PlayCue(this, TravelFX->PortalClose, Where, Normal.Rotation()); // "can't place" until a dedicated cue exists
		}
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPortal* Portal = World->SpawnActor<ATDPortal>(ATDPortal::StaticClass(), Where, Normal.Rotation(), Params);
	if (!Portal)
	{
		return nullptr;
	}
	Portal->Init(Owner, PortalRadius, TransitParams, TravelFX, ExitClearanceRadius);
	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(this))
	{
		Subsystem->RegisterTransient(Portal);
	}
	if (Portals.Num() >= 2)
	{
		// A third shot recycles the oldest.
		if (ATDPortal* Oldest = Portals[0].Get())
		{
			Oldest->Close();
		}
		Portals.RemoveAt(0);
	}
	Portals.Add(Portal);

	if (Portals.Num() == 2)
	{
		ATDPortal* A = Portals[0].Get();
		ATDPortal* B = Portals[1].Get();
		if (A && B)
		{
			A->SetPartner(B);
			B->SetPartner(A);
		}
	}
	return Portal;
}

void UTDPortalBlasterPower::OnTickActive(float DeltaTime)
{
	if (ActiveElapsed >= WindowSeconds)
	{
		EndPower(false);
	}
}

void UTDPortalBlasterPower::OnEnd(bool bInterrupted)
{
	for (const TWeakObjectPtr<ATDPortal>& P : Portals)
	{
		if (ATDPortal* Portal = P.Get())
		{
			Portal->Close();
		}
	}
	Portals.Reset();
}

bool UTDPortalBlasterPower::DoBlink()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}
	FVector Facing;
	FVector Where = ComputePlacement(Facing);
	const float Clearance = FMath::Max(ExitClearanceRadius, TDPortalTravellerRadius(Owner));
	bool bClear = false;
	for (int32 Step = 0; Step <= PushOutSteps; ++Step)
	{
		const FVector Candidate = Where - Facing * (PushOutStep * Step);
		if (ATDPortal::HasClearance(World, Candidate, Clearance))
		{
			Where = Candidate;
			bClear = true;
			break;
		}
	}
	if (!bClear)
	{
		++RefusedPlacements;
		if (TravelFX)
		{
			UTDStatics::PlayCue(this, TravelFX->PortalClose, Where, Facing.Rotation());
		}
		return false;
	}
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		Craft->TeleportSpacecraft(Where, Owner->GetActorRotation(), true);
	}
	else
	{
		const FVector From = Owner->GetActorLocation();
		Owner->SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
		if (UTDEventBus* Bus = UTDEventBus::Get(this))
		{
			Bus->BroadcastSpacecraftTeleported(Owner, From, Where);
		}
	}
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(Owner))
	{
		Health->SetInvulnerableFor(TransitParams.InvulnerableSeconds);
	}
	if (ATDShieldDome* Field = ATDShieldDome::SpawnDome(World, Owner, GetOwnerTeam(), BlinkReflectRadius, BlinkReflectSeconds, FX))
	{
		Field->bProtectSquad = false;
		Field->bDestroyShots = false;
		Field->FollowSpeed = 100000.f; // rides with Rift
	}
	return true;
}
