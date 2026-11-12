// Triad Drift - Combat (Teammate 2). Soft target, cycling, Bastion multi-lock, lead and HUD markers (ENG-06).

#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

UTDTargetingComponent::UTDTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

// ------------------------------------------------------------------------------------------ helpers

ETDTeam UTDTargetingComponent::GetOwnerTeam() const
{
	return UTDStatics::GetTeam(GetOwner());
}

void UTDTargetingComponent::SetViewOverride(const FVector& Location, const FRotator& Rotation)
{
	bHasViewOverride = true;
	ViewOverrideLocation = Location;
	ViewOverrideRotation = Rotation;
}

void UTDTargetingComponent::GetView(FVector& OutLocation, FRotator& OutRotation) const
{
	if (bHasViewOverride)
	{
		OutLocation = ViewOverrideLocation;
		OutRotation = ViewOverrideRotation;
		return;
	}
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		OutLocation = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->GetCameraViewPoint(OutLocation, OutRotation);
				return;
			}
		}
	}
	OutLocation = Owner->GetActorLocation();
	OutRotation = Owner->GetActorRotation();
}

float UTDTargetingComponent::GetAngleFromView(const FVector& Location) const
{
	FVector ViewLoc;
	FRotator ViewRot;
	GetView(ViewLoc, ViewRot);
	const FVector To = (Location - ViewLoc).GetSafeNormal();
	if (To.IsNearlyZero())
	{
		return 0.f;
	}
	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ViewRot.Vector(), To), -1.f, 1.f)));
}

bool UTDTargetingComponent::IsValidTarget(const AActor* Actor) const
{
	if (!IsValid(Actor) || Actor == GetOwner())
	{
		return false;
	}
	if (const UTDHealthComponent* Health = UTDStatics::GetHealth(Actor))
	{
		if (Health->IsDead() || !Health->bTargetable)
		{
			return false;
		}
	}
	if (!UTDStatics::AreTeamsHostile(GetOwnerTeam(), UTDStatics::GetTeam(Actor)))
	{
		return false;
	}
	const AActor* Owner = GetOwner();
	return !Owner || FVector::DistSquared(Owner->GetActorLocation(), Actor->GetActorLocation()) <= FMath::Square(MaxTargetRange);
}

float UTDTargetingComponent::GetLeadSpeed() const
{
	if (LeadProjectileSpeedOverride > 0.f)
	{
		return LeadProjectileSpeedOverride;
	}
	const AActor* Owner = GetOwner();
	if (const UTDWeaponComponent* Weapons = Owner ? Owner->FindComponentByClass<UTDWeaponComponent>() : nullptr)
	{
		const FTDWeaponSpec& Spec = Weapons->GetSpec(ETDWeaponSlot::Primary);
		if (Spec.IsValid())
		{
			return Spec.Projectile.Speed;
		}
	}
	return 0.f;
}

FVector UTDTargetingComponent::ComputeInterceptPoint(const FVector& ShooterLocation, const FVector& ShooterVelocity, const FVector& TargetLocation,
	const FVector& TargetVelocity, float ProjectileSpeed)
{
	if (ProjectileSpeed <= KINDA_SMALL_NUMBER)
	{
		return TargetLocation;
	}
	// The shot inherits the shooter's motion, so solve in the shooter's frame: |D + V t| = s t.
	const FVector D = TargetLocation - ShooterLocation;
	const FVector V = TargetVelocity - ShooterVelocity;
	const float A = FVector::DotProduct(V, V) - ProjectileSpeed * ProjectileSpeed;
	const float B = 2.f * FVector::DotProduct(D, V);
	const float C = FVector::DotProduct(D, D);

	float T = -1.f;
	if (FMath::Abs(A) < KINDA_SMALL_NUMBER)
	{
		if (FMath::Abs(B) > KINDA_SMALL_NUMBER)
		{
			T = -C / B;
		}
	}
	else
	{
		const float Disc = B * B - 4.f * A * C;
		if (Disc >= 0.f)
		{
			const float Sqrt = FMath::Sqrt(Disc);
			const float T1 = (-B - Sqrt) / (2.f * A);
			const float T2 = (-B + Sqrt) / (2.f * A);
			const float Lo = FMath::Min(T1, T2);
			const float Hi = FMath::Max(T1, T2);
			T = Lo > 0.f ? Lo : Hi;
		}
	}
	if (T <= 0.f || !FMath::IsFinite(T))
	{
		return TargetLocation;
	}
	return TargetLocation + V * T;
}

bool UTDTargetingComponent::GetLeadIndicator(FVector& OutWorldLocation) const
{
	const AActor* Target = CurrentTarget.Get();
	const AActor* Owner = GetOwner();
	const float Speed = GetLeadSpeed();
	if (!Target || !Owner || Speed <= 0.f)
	{
		return false;
	}
	OutWorldLocation = ComputeInterceptPoint(Owner->GetActorLocation(), Owner->GetVelocity(), Target->GetActorLocation(), Target->GetVelocity(), Speed);
	return true;
}

// ------------------------------------------------------------------------------------------ tick

void UTDTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	PruneLocks();

	UpdateAccumulator += DeltaTime;
	const float Interval = UpdateHz > 0.f ? 1.f / UpdateHz : 0.f;
	if (bPainting || UpdateAccumulator >= Interval)
	{
		UpdateAccumulator = Interval > 0.f ? FMath::Fmod(UpdateAccumulator, Interval) : 0.f;
		Refresh();
	}
	if (bPainting)
	{
		UpdatePainting(DeltaTime);
	}
}

void UTDTargetingComponent::RefreshNow()
{
	PruneLocks();
	Refresh();
}

void UTDTargetingComponent::Refresh()
{
	GatherHostiles();
	UpdateSoftTarget();
	UpdateMissileWarning();
	UpdateMarkers();
}

void UTDTargetingComponent::GatherHostiles()
{
	Hostiles.Reset();
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		Registry->GetHostilesOf(GetOwnerTeam(), Hostiles, false);
	}
	Hostiles.RemoveAllSwap([this](const AActor* A) { return !IsValidTarget(A); }, EAllowShrinking::No);

	// Nearest first: cycling and markers rely on it.
	if (const AActor* Owner = GetOwner())
	{
		const FVector Here = Owner->GetActorLocation();
		Hostiles.Sort([&Here](const AActor& A, const AActor& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), Here) < FVector::DistSquared(B.GetActorLocation(), Here);
		});
	}
}

void UTDTargetingComponent::SetCurrentTarget(AActor* NewTarget)
{
	AActor* Old = CurrentTarget.Get();
	if (Old == NewTarget)
	{
		return;
	}
	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(NewTarget, Old);
}

void UTDTargetingComponent::UpdateSoftTarget()
{
	AActor* Current = CurrentTarget.Get();
	const bool bCurrentValid = IsValidTarget(Current);

	if (bManualTarget)
	{
		if (bCurrentValid)
		{
			return;
		}
		bManualTarget = false;
	}

	AActor* Best = nullptr;
	float BestAngle = SoftTargetConeDeg;
	for (AActor* Hostile : Hostiles)
	{
		const float Angle = GetAngleFromView(Hostile->GetActorLocation());
		if (Angle <= BestAngle)
		{
			Best = Hostile;
			BestAngle = Angle;
		}
	}

	if (bCurrentValid)
	{
		const float CurrentAngle = GetAngleFromView(Current->GetActorLocation());
		if (CurrentAngle <= SoftTargetKeepConeDeg && (!Best || Best == Current || BestAngle + SoftTargetSwitchMarginDeg >= CurrentAngle))
		{
			return; // sticky: avoid flicker between two close candidates
		}
	}
	SetCurrentTarget(Best);
}

void UTDTargetingComponent::CycleTarget()
{
	PruneLocks();
	GatherHostiles();

	// Hostiles is sorted nearest first; keep those inside the cycle cone.
	TArray<AActor*, TInlineAllocator<32>> Candidates;
	for (AActor* Hostile : Hostiles)
	{
		if (GetAngleFromView(Hostile->GetActorLocation()) <= CycleConeDeg)
		{
			Candidates.Add(Hostile);
		}
	}
	if (Candidates.Num() == 0)
	{
		return;
	}
	const int32 CurrentIndex = Candidates.IndexOfByKey(CurrentTarget.Get());
	const int32 NextIndex = (CurrentIndex == INDEX_NONE) ? 0 : (CurrentIndex + 1) % Candidates.Num();
	SetCurrentTarget(Candidates[NextIndex]);
	bManualTarget = true;
	UpdateMarkers();
}

// ------------------------------------------------------------------------------------------ multi-lock

void UTDTargetingComponent::PruneLocks()
{
	const int32 Before = LockedWeak.Num();
	LockedWeak.RemoveAll([this](const TWeakObjectPtr<AActor>& W)
	{
		const AActor* A = W.Get();
		if (!IsValid(A))
		{
			return true;
		}
		const UTDHealthComponent* Health = UTDStatics::GetHealth(A);
		return Health && Health->IsDead();
	});
	if (Before != LockedWeak.Num() || LockedTargets.Num() != LockedWeak.Num())
	{
		LockedTargets.Reset();
		for (const TWeakObjectPtr<AActor>& W : LockedWeak)
		{
			LockedTargets.Add(W.Get());
		}
	}
}

void UTDTargetingComponent::BeginLockPainting()
{
	bPainting = true;
	LockedWeak.Reset();
	LockedTargets.Reset();
	PaintCandidate = nullptr;
	PaintProgress = 0.f;
}

TArray<AActor*> UTDTargetingComponent::EndLockPainting()
{
	bPainting = false;
	PaintCandidate = nullptr;
	PaintProgress = 0.f;
	PruneLocks();
	return LockedTargets;
}

void UTDTargetingComponent::ClearLocks()
{
	LockedWeak.Reset();
	LockedTargets.Reset();
	PaintCandidate = nullptr;
	PaintProgress = 0.f;
}

float UTDTargetingComponent::GetPaintProgress01() const
{
	return SecondsPerLock > 0.f ? FMath::Clamp(PaintProgress / SecondsPerLock, 0.f, 1.f) : 0.f;
}

void UTDTargetingComponent::UpdatePainting(float DeltaTime)
{
	if (LockedWeak.Num() >= MaxLocks)
	{
		PaintCandidate = nullptr;
		PaintProgress = 0.f;
		return;
	}

	auto IsLocked = [this](const AActor* A) { return LockedWeak.ContainsByPredicate([A](const TWeakObjectPtr<AActor>& W) { return W.Get() == A; }); };

	// Prefer the soft target, else the unlocked hostile nearest screen centre inside the lock cone.
	AActor* Candidate = nullptr;
	AActor* Current = CurrentTarget.Get();
	if (Current && !IsLocked(Current) && IsValidTarget(Current) && GetAngleFromView(Current->GetActorLocation()) <= LockConeDeg)
	{
		Candidate = Current;
	}
	else
	{
		float BestAngle = LockConeDeg;
		for (AActor* Hostile : Hostiles)
		{
			if (IsLocked(Hostile))
			{
				continue;
			}
			const float Angle = GetAngleFromView(Hostile->GetActorLocation());
			if (Angle <= BestAngle)
			{
				Candidate = Hostile;
				BestAngle = Angle;
			}
		}
	}

	if (Candidate != PaintCandidate.Get())
	{
		PaintCandidate = Candidate;
		PaintProgress = 0.f;
	}
	if (!Candidate)
	{
		return;
	}

	PaintProgress += DeltaTime;
	if (PaintProgress >= SecondsPerLock)
	{
		PaintProgress -= SecondsPerLock;
		LockedWeak.Add(Candidate);
		LockedTargets.Add(Candidate);
		PaintCandidate = nullptr;
		OnTargetLocked.Broadcast(Candidate, LockedWeak.Num());
	}
}

// ------------------------------------------------------------------------------------------ warnings and markers

void UTDTargetingComponent::UpdateMissileWarning()
{
	MissileScratch.Reset();
	AActor* Owner = GetOwner();
	if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
	{
		Subsystem->FindMissilesHomingOn(Owner, MissileScratch);
	}
	const ETDTeam MyTeam = GetOwnerTeam();
	MissileScratch.RemoveAllSwap([MyTeam](const ATDProjectile* P) { return !UTDStatics::AreTeamsHostile(MyTeam, P->GetTeam()); }, EAllowShrinking::No);

	IncomingMissileCount = MissileScratch.Num();
	NearestMissileSeconds = 1000.f;
	NearestMissileDirection = FVector::ZeroVector;

	if (Owner)
	{
		const FVector Here = Owner->GetActorLocation();
		for (const ATDProjectile* P : MissileScratch)
		{
			const FVector To = Here - P->GetActorLocation();
			const float Dist = To.Size();
			const float Closing = FMath::Max(FVector::DotProduct(P->GetVelocity() - Owner->GetVelocity(), To.GetSafeNormal()), P->GetVelocity().Size() * 0.25f);
			const float Seconds = Closing > KINDA_SMALL_NUMBER ? Dist / Closing : 1000.f;
			if (Seconds < NearestMissileSeconds)
			{
				NearestMissileSeconds = Seconds;
				NearestMissileDirection = (-To).GetSafeNormal();
			}
		}
	}

	ETDMissileWarning NewWarning = ETDMissileWarning::None;
	if (IncomingMissileCount > 0)
	{
		NewWarning = NearestMissileSeconds <= MissileImminentSeconds ? ETDMissileWarning::Imminent : ETDMissileWarning::Tracking;
	}
	if (NewWarning != MissileWarning)
	{
		MissileWarning = NewWarning;
		OnMissileWarningChanged.Broadcast(NewWarning);
	}
}

void UTDTargetingComponent::UpdateMarkers()
{
	Markers.Reset();
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	GetView(ViewLoc, ViewRot);
	const FRotationMatrix ViewBasis(ViewRot);
	const FVector Fwd = ViewBasis.GetUnitAxis(EAxis::X);
	const FVector Right = ViewBasis.GetUnitAxis(EAxis::Y);
	const FVector Up = ViewBasis.GetUnitAxis(EAxis::Z);

	const APawn* Pawn = Cast<APawn>(Owner);
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	int32 ViewW = 0, ViewH = 0;
	if (PC)
	{
		PC->GetViewportSize(ViewW, ViewH);
	}

	auto AddMarker = [&](AActor* Actor, const FVector& Location, bool bMissile)
	{
		FTDTargetMarker& M = Markers.AddDefaulted_GetRef();
		M.Actor = Actor;
		M.WorldLocation = Location;
		M.Distance = FVector::Dist(Owner->GetActorLocation(), Location);
		M.bIsCurrentTarget = !bMissile && Actor == CurrentTarget.Get();
		M.bLocked = !bMissile && LockedTargets.Contains(Actor);
		M.bIncomingMissile = bMissile;

		const FVector Rel = Location - ViewLoc;
		const FVector2D Dir2D(FVector::DotProduct(Rel, Right), -FVector::DotProduct(Rel, Up));
		M.ScreenDirection = Dir2D.GetSafeNormal();

		if (PC && ViewW > 0 && ViewH > 0)
		{
			FVector2D Screen;
			if (PC->ProjectWorldLocationToScreen(Location, Screen, true) && FVector::DotProduct(Rel, Fwd) > 0.f)
			{
				M.ScreenPosition01 = FVector2D(Screen.X / ViewW, Screen.Y / ViewH);
				M.bOnScreen = M.ScreenPosition01.X >= 0.f && M.ScreenPosition01.X <= 1.f && M.ScreenPosition01.Y >= 0.f && M.ScreenPosition01.Y <= 1.f;
			}
		}
		else
		{
			// No viewport (AI, tests): treat a 90° horizontal FOV cone as on screen.
			const float Forward = FVector::DotProduct(Rel, Fwd);
			if (Forward > 0.f)
			{
				const float X = FVector::DotProduct(Rel, Right) / Forward;
				const float Y = -FVector::DotProduct(Rel, Up) / Forward;
				M.ScreenPosition01 = FVector2D(0.5f + X * 0.5f, 0.5f + Y * 0.5f);
				M.bOnScreen = FMath::Abs(X) <= 1.f && FMath::Abs(Y) <= 1.f;
			}
		}
	};

	const int32 ShipBudget = FMath::Max(0, MaxMarkers - MissileScratch.Num());
	for (int32 i = 0; i < Hostiles.Num() && i < ShipBudget; ++i)
	{
		AddMarker(Hostiles[i], Hostiles[i]->GetActorLocation(), false);
	}
	for (ATDProjectile* P : MissileScratch)
	{
		if (Markers.Num() >= MaxMarkers)
		{
			break;
		}
		AddMarker(P, P->GetActorLocation(), true);
	}
}
