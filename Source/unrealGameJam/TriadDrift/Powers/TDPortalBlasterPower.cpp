// Triad Drift - Powers (Teammate 2). Rift: Portal Blaster + Blink fallback.

#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

static TAutoConsoleVariable<int32> CVarPortalBlink(
	TEXT("TD.PortalBlink"), 0,
	TEXT("1 = Rift's Q uses the Blink fallback (teleport + missile-reflect field) instead of portals."),
	ECVF_Default);

// ================================================================================== portal actor

ATDPortal::ATDPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	// After movement and projectiles have stepped, so this frame's motion is complete.
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SetCanBeDamaged(false);
}

void ATDPortal::Init(AActor* InInstigator, float InRadius, const FTDPortalTransitParams& InParams, UTDTravelFX* InFX)
{
	PortalInstigator = InInstigator;
	Radius = InRadius;
	Params = InParams;
	FX = InFX;
	BroadcastState(ETDPortalState::Opening);
	if (FX)
	{
		UTDStatics::PlayCue(this, FX->PortalOpen, GetActorLocation(), GetActorRotation());
		IdleFX = UTDStatics::PlayCueAttached(FX->PortalIdle, GetRootComponent());
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

void ATDPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
	if (UTDStatics::ShouldDrawPlaceholders() && !IdleFX)
	{
		DrawDebugCircle(GetWorld(), GetActorLocation(), Radius, 32, Partner.IsValid() ? FColor(255, 40, 220) : FColor(120, 40, 110), false, -1.f, 0, 12.f,
			GetActorRightVector(), GetActorUpVector(), false);
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
			TryCross(P, Cur - P->GetVelocity() * DeltaSeconds, Cur);
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

	// Two-sided: entering along the facing exits along the partner's facing, and vice versa.
	const bool bFront = D1 > D0;
	const FQuat EntryRot = bFront ? GetActorQuat() : FQuat(GetActorUpVector(), PI) * GetActorQuat();
	const FQuat ExitRot = bFront ? Exit->GetActorQuat() : FQuat(Exit->GetActorUpVector(), PI) * Exit->GetActorQuat();
	const FTransform Entry(EntryRot, P);
	const FTransform ExitXf(ExitRot, Exit->GetActorLocation());

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
	if (GetWorld() && GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
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
	if (Portals.Num() >= 2)
	{
		if (ATDPortal* Oldest = Portals[0].Get())
		{
			Oldest->Close();
		}
		Portals.RemoveAt(0);
	}

	FVector Facing;
	const FVector Where = ComputePlacement(Facing);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPortal* Portal = World->SpawnActor<ATDPortal>(ATDPortal::StaticClass(), Where, Facing.Rotation(), Params);
	if (!Portal)
	{
		return nullptr;
	}
	Portal->Init(Owner, PortalRadius, TransitParams, TravelFX);
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
	const FVector Where = ComputePlacement(Facing);
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
