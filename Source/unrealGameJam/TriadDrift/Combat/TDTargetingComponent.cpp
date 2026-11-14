// Triad Drift - Combat (Teammate 2). Soft target, cycling, Bastion multi-lock, lead and HUD markers (ENG-06).

#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDCombatPerf.h"
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

namespace
{
	/** Smallest positive t with |D + V t| = S t (constant-velocity target, straight shot at ground speed S). */
	bool TDTargetSolveIntercept(const FVector& D, const FVector& V, double S, double& OutT)
	{
		const double A = FVector::DotProduct(V, V) - S * S;
		const double B = 2.0 * FVector::DotProduct(D, V);
		const double C = FVector::DotProduct(D, D);
		double T = -1.0;
		if (FMath::Abs(A) < 1e-6)
		{
			if (FMath::Abs(B) > 1e-6)
			{
				T = -C / B;
			}
		}
		else
		{
			const double Disc = B * B - 4.0 * A * C;
			if (Disc >= 0.0)
			{
				const double Sqrt = FMath::Sqrt(Disc);
				const double T1 = (-B - Sqrt) / (2.0 * A);
				const double T2 = (-B + Sqrt) / (2.0 * A);
				const double Lo = FMath::Min(T1, T2);
				const double Hi = FMath::Max(T1, T2);
				T = Lo > 0.0 ? Lo : Hi;
			}
		}
		if (T <= 0.0 || !FMath::IsFinite(T))
		{
			return false;
		}
		OutT = T;
		return true;
	}
}

FVector UTDTargetingComponent::ComputeInterceptPoint(const FVector& ShooterLocation, const FVector& ShooterVelocity, const FVector& TargetLocation,
	const FVector& TargetVelocity, float ProjectileSpeed)
{
	if (ProjectileSpeed <= KINDA_SMALL_NUMBER)
	{
		return TargetLocation;
	}
	// Matches ATDProjectile: a shot fired along Dir flies at Speed + max(0, ShooterVel . Dir) along Dir (only the
	// forward part of the shooter's velocity is inherited; lateral drift velocity is not). Solve in the world frame
	// with that ground speed, re-evaluating the speed for the new aim direction (two refinement passes).
	const FVector D = TargetLocation - ShooterLocation;
	FVector Dir = D.GetSafeNormal();
	FVector Aim = TargetLocation;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		const double GroundSpeed = ProjectileSpeed + FMath::Max(0.0, FVector::DotProduct(ShooterVelocity, Dir));
		double T;
		if (!TDTargetSolveIntercept(D, TargetVelocity, GroundSpeed, T))
		{
			return TargetLocation;
		}
		Aim = TargetLocation + TargetVelocity * T;
		const FVector NewDir = (Aim - ShooterLocation).GetSafeNormal();
		if (NewDir.Equals(Dir, 1e-5))
		{
			break;
		}
		Dir = NewDir;
	}
	return Aim;
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
	bool bInherit = true;
	if (const UTDWeaponComponent* Weapons = Owner->FindComponentByClass<UTDWeaponComponent>())
	{
		const FTDWeaponSpec& Spec = Weapons->GetSpec(ETDWeaponSlot::Primary);
		bInherit = !Spec.IsValid() || Spec.Projectile.bInheritShooterVelocity;
	}
	OutWorldLocation = ComputeInterceptPoint(Owner->GetActorLocation(), bInherit ? Owner->GetVelocity() : FVector::ZeroVector,
		Target->GetActorLocation(), Target->GetVelocity(), Speed);
	return true;
}

// ------------------------------------------------------------------------------------------ tick

void UTDTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	TD_COMBAT_PERF(Targeting);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	PruneLocks(DeltaTime);

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
	else
	{
		// Selection runs at UpdateHz; brackets are re-projected every frame so they never lag the camera.
		Hostiles.RemoveAll([](const AActor* A) { return !IsValid(A); });
		FarHostiles.RemoveAll([](const TObjectPtr<AActor>& A) { return !IsValid(A); });
		MissileScratch.RemoveAll([](const ATDProjectile* P) { return !IsValid(P) || !P->IsActive(); });
		UpdateMarkers();
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
	// Out of targeting range but within marker range: kept aside for markers only.
	FarHostiles.Reset();
	const AActor* Me = GetOwner();
	if (Me && MarkerRange > MaxTargetRange)
	{
		const FVector Here = Me->GetActorLocation();
		for (AActor* A : Hostiles)
		{
			const double D2 = IsValid(A) ? FVector::DistSquared(A->GetActorLocation(), Here) : 0.0;
			if (D2 > FMath::Square(MaxTargetRange) && D2 <= FMath::Square(MarkerRange))
			{
				const UTDHealthComponent* Health = UTDStatics::GetHealth(A);
				if (!Health || (!Health->IsDead() && Health->bTargetable))
				{
					FarHostiles.Add(A);
				}
			}
		}
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
		FarHostiles.Sort([&Here](const AActor& A, const AActor& B)
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

void UTDTargetingComponent::SetManualTarget(AActor* Target)
{
	if (Target && IsValidTarget(Target))
	{
		SetCurrentTarget(Target);
		bManualTarget = true;
	}
	else
	{
		bManualTarget = false;
		UpdateSoftTarget();
	}
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
	UpdateMissileWarning();
	UpdateMarkers();
}

// ------------------------------------------------------------------------------------------ multi-lock

void UTDTargetingComponent::PruneLocks(float DeltaTime)
{
	const AActor* Owner = GetOwner();
	const float Range = LockMaxRange > 0.f ? LockMaxRange : MaxTargetRange;
	LockAwaySeconds.SetNum(LockedWeak.Num());
	bool bChanged = LockedTargets.Num() != LockedWeak.Num();
	for (int32 Index = LockedWeak.Num() - 1; Index >= 0; --Index)
	{
		const AActor* A = LockedWeak[Index].Get();
		bool bDrop = !IsValidTarget(A) || (Owner && FVector::DistSquared(A->GetActorLocation(), Owner->GetActorLocation()) > FMath::Square(Range));
		if (!bDrop)
		{
			// "Out of view" = outside the lock cone (plus a small margin), as features.md says: a lock 30-89 degrees off
			// the nose must age too, or a new group in the crosshair can never replace it (second Powers review).
			LockAwaySeconds[Index] = GetAngleFromView(A->GetActorLocation()) > LockConeDeg + LockStaleMarginDeg ? LockAwaySeconds[Index] + DeltaTime : 0.f;
			bDrop = LockAwaySeconds[Index] > LockStaleSeconds;
		}
		if (bDrop)
		{
			LockedWeak.RemoveAt(Index);
			LockAwaySeconds.RemoveAt(Index);
			bChanged = true;
		}
	}
	if (bChanged)
	{
		LockedTargets.Reset();
		for (const TWeakObjectPtr<AActor>& W : LockedWeak)
		{
			LockedTargets.Add(W.Get());
		}
	}
}

int32 UTDTargetingComponent::FindStalestLock() const
{
	int32 Stalest = INDEX_NONE;
	float Longest = 0.f;
	for (int32 Index = 0; Index < LockAwaySeconds.Num(); ++Index)
	{
		if (LockAwaySeconds[Index] > Longest)
		{
			Longest = LockAwaySeconds[Index];
			Stalest = Index;
		}
	}
	return Stalest;
}

void UTDTargetingComponent::AddLock(AActor* Target)
{
	LockedWeak.Add(Target);
	LockAwaySeconds.Add(0.f);
	LockedTargets.Add(Target);
}

void UTDTargetingComponent::RemoveLockAt(int32 Index)
{
	if (!LockedWeak.IsValidIndex(Index))
	{
		return;
	}
	LockedWeak.RemoveAt(Index);
	if (LockAwaySeconds.IsValidIndex(Index))
	{
		LockAwaySeconds.RemoveAt(Index);
	}
	if (LockedTargets.IsValidIndex(Index))
	{
		LockedTargets.RemoveAt(Index);
	}
}

void UTDTargetingComponent::BeginLockPainting()
{
	bPainting = true;
	LockedWeak.Reset();
	LockAwaySeconds.Reset();
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
	LockAwaySeconds.Reset();
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
	// Full: paint only if a lock has gone out of view, to replace the stalest one.
	const bool bFull = LockedWeak.Num() >= MaxLocks;
	if (bFull && FindStalestLock() == INDEX_NONE)
	{
		PaintCandidate = nullptr;
		PaintProgress = 0.f;
		return;
	}

	auto IsLocked = [this](const AActor* A) { return LockedWeak.ContainsByPredicate([A](const TWeakObjectPtr<AActor>& W) { return W.Get() == A; }); };

	// Prefer the soft target, else keep painting the one already being painted while it is still in the cone, else the
	// unlocked hostile nearest screen centre inside the lock cone. Keeping the candidate matters: with the Audition chase
	// camera's sustained shake (T1, Nov 13) two near-tied targets swapped "nearest" every frame and each swap restarted
	// the paint, so a group in the crosshair locked at half speed (gating run on lead/int 173a64f).
	AActor* Candidate = nullptr;
	AActor* Current = CurrentTarget.Get();
	AActor* Painting = PaintCandidate.Get();
	if (Current && !IsLocked(Current) && IsValidTarget(Current) && GetAngleFromView(Current->GetActorLocation()) <= LockConeDeg)
	{
		Candidate = Current;
	}
	else if (Painting && !IsLocked(Painting) && IsValidTarget(Painting) && GetAngleFromView(Painting->GetActorLocation()) <= LockConeDeg)
	{
		Candidate = Painting;
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

	// Switching to a different candidate restarts its paint; moving on after a lock keeps the carry-over
	// so the cadence stays exactly SecondsPerLock per target at any frame rate.
	if (Candidate != PaintCandidate.Get())
	{
		if (PaintCandidate.IsValid())
		{
			PaintProgress = 0.f;
		}
		PaintCandidate = Candidate;
	}
	if (!Candidate)
	{
		PaintProgress = 0.f;
		return;
	}

	PaintProgress += DeltaTime;
	if (PaintProgress + 1e-4f >= SecondsPerLock)
	{
		PaintProgress = FMath::Max(0.f, PaintProgress - SecondsPerLock);
		if (LockedWeak.Num() >= MaxLocks)
		{
			RemoveLockAt(FindStalestLock());
		}
		AddLock(Candidate);
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

	// The current and locked targets always get a marker, whatever their distance rank; then the nearest.
	const int32 ShipBudget = FMath::Max(0, MaxMarkers - MissileScratch.Num());
	TArray<AActor*, TInlineAllocator<8>> Priority;
	if (AActor* Current = CurrentTarget.Get(); IsValidTarget(Current))
	{
		Priority.Add(Current);
	}
	for (AActor* Locked : LockedTargets)
	{
		if (IsValidTarget(Locked))
		{
			Priority.AddUnique(Locked);
		}
	}
	for (AActor* A : Priority)
	{
		if (Markers.Num() < FMath::Max(ShipBudget, Priority.Num()))
		{
			AddMarker(A, A->GetActorLocation(), false);
		}
	}
	for (int32 i = 0; i < Hostiles.Num() && Markers.Num() < ShipBudget; ++i)
	{
		if (!Priority.Contains(Hostiles[i]))
		{
			AddMarker(Hostiles[i], Hostiles[i]->GetActorLocation(), false);
		}
	}
	// Then the nearest out-of-range hostiles (arrows to the fight), within what's left of the budget.
	for (int32 i = 0; i < FarHostiles.Num() && Markers.Num() < ShipBudget; ++i)
	{
		if (AActor* Far = FarHostiles[i].Get())
		{
			AddMarker(Far, Far->GetActorLocation(), false);
		}
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
