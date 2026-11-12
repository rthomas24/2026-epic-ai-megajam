// Triad Drift - Powers (Teammate 2). Lancer: Afterburner Ram.

#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "Engine/World.h"

namespace
{
	float TDRamBoundsRadius(const AActor* Actor, FVector& OutCenter)
	{
		FVector Extent;
		Actor->GetActorBounds(true, OutCenter, Extent);
		if (Extent.IsNearlyZero())
		{
			OutCenter = Actor->GetActorLocation();
			return 50.f;
		}
		return Extent.GetMax();
	}
}

UTDRamPower::UTDRamPower()
{
	Slot = ETDPowerInput::Signature1;
	PowerName = TEXT("AfterburnerRam");
	CooldownSeconds = 7.f;
	bCooldownStartsWhenPowerEnds = false;
	ActiveSeconds = 0.3f + 1.2f;
}

float UTDRamPower::GetWindup01() const
{
	if (!IsPowerActive() || WindupSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(ActiveElapsed / WindupSeconds, 0.f, 1.f);
}

float UTDRamPower::GetDashSpeed() const
{
	float Boost = FallbackBoostSpeed;
	if (const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner()))
	{
		if (const UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Boost = Move->GetFlightTuning().BoostSpeed;
		}
	}
	return Boost * BoostSpeedMultiplier;
}

bool UTDRamPower::OnActivate()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	ActiveSeconds = WindupSeconds + DashSeconds;
	HitThisDash.Reset();
	bDashStarted = false;
	bStoppedByImpact = false;
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(Owner))
	{
		Health->SetInvulnerable(true);
	}
	if (WindupSeconds <= 0.f)
	{
		BeginDash();
	}
	return true;
}

void UTDRamPower::BeginDash()
{
	AActor* Owner = GetOwner();
	bDashStarted = true;
	LastLocation = Owner->GetActorLocation();
	DashDirection = Owner->GetActorForwardVector();
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		Craft->SetSpeedOverride(GetDashSpeed(), SteeringScale);
	}
}

void UTDRamPower::OnTickActive(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		EndPower(true);
		return;
	}
	if (!bDashStarted)
	{
		if (ActiveElapsed < WindupSeconds)
		{
			return;
		}
		BeginDash();
	}

	// Pawns without the flight model (tests, odd owners) are driven directly along the nose.
	if (!Owner->IsA<ATDSpacecraft>())
	{
		Owner->SetActorLocation(Owner->GetActorLocation() + Owner->GetActorForwardVector() * GetDashSpeed() * DeltaTime);
	}

	const FVector Now = Owner->GetActorLocation();
	// A portal / gate moved us this frame: restart the sweep at the new spot (and keep dashing).
	if (FVector::DistSquared(Now, LastLocation) > FMath::Square(GetDashSpeed() * DeltaTime * 3.f + 500.f))
	{
		LastLocation = Now;
		DashDirection = Owner->GetActorForwardVector();
		return;
	}
	DashDirection = (Now - LastLocation).GetSafeNormal();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = Owner->GetActorForwardVector();
	}
	SweepHits(LastLocation, Now);
	LastLocation = Now;
}

void UTDRamPower::SweepHits(const FVector& From, const FVector& To)
{
	AActor* Owner = GetOwner();
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Owner || !Registry)
	{
		return;
	}
	FVector OwnerCenter;
	const float Reach = TDRamBoundsRadius(Owner, OwnerCenter) + HitPadding;

	// Copy first: damage kills and unregisters actors.
	TArray<AActor*, TInlineAllocator<16>> Touched;
	{
		TArray<AActor*> Hostiles;
		Registry->GetHostilesOf(GetOwnerTeam(), Hostiles, true);
		for (AActor* Hostile : Hostiles)
		{
			if (!Hostile || HitThisDash.Contains(Hostile))
			{
				continue;
			}
			FVector Center;
			const float Radius = TDRamBoundsRadius(Hostile, Center);
			if (FMath::PointDistToSegment(Center, From, To) <= Reach + Radius)
			{
				Touched.Add(Hostile);
			}
		}
	}
	// Nearest along the path first, so a big ship stops the dash before anything behind it.
	Touched.Sort([&From](const AActor& A, const AActor& B) { return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From); });
	for (AActor* Victim : Touched)
	{
		if (!IsPowerActive())
		{
			return;
		}
		HitActor(Victim, DashDirection);
	}

	// World geometry (asteroids, the Dominion hull) also stops the dash.
	if (IsPowerActive())
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TDRamWorld), false, Owner);
		if (GetWorld()->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
			FCollisionShape::MakeSphere(Reach * 0.5f), Params))
		{
			bStoppedByImpact = true;
			Shockwave(Hit.Location);
			EndPower(false);
		}
	}
}

void UTDRamPower::HitActor(AActor* Victim, const FVector& Direction)
{
	AActor* Owner = GetOwner();
	HitThisDash.Add(Victim);

	UTDHealthComponent* Health = UTDStatics::GetHealth(Victim);
	const bool bWasAlive = !Health || Health->IsAlive();

	if (ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Victim))
	{
		if (Enemy->HasFrontalShield())
		{
			Enemy->BreakFrontalShield();
		}
	}
	if (ATDCarrierBay* Bay = Cast<ATDCarrierBay>(Victim))
	{
		Bay->BreakDoors();
	}

	UTDStatics::ApplyTDDamage(Victim, RamDamage, ETDDamageKind::Ram, Owner, Owner, Victim->GetActorLocation(), Direction, false);

	if (bWasAlive && Health && Health->IsDead())
	{
		// RamKill money shot: UTDDirectorSubsystem derives it from the Ram damage kind on OnActorKilled.
		if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this))
		{
			Feel->RequestHitStop();
		}
	}

	if (IsBig(Victim))
	{
		bStoppedByImpact = true;
		Shockwave(Owner->GetActorLocation() + Direction * 200.f);
		EndPower(false);
	}
}

bool UTDRamPower::IsBig(const AActor* Actor) const
{
	if (const ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Actor))
	{
		return static_cast<uint8>(Enemy->GetSize()) >= static_cast<uint8>(StopOnSize);
	}
	return false; // bays, turrets and unknown actors are smashed through
}

void UTDRamPower::Shockwave(const FVector& Where)
{
	AActor* Owner = GetOwner();
	UTDStatics::ApplyTDRadialDamage(this, Where, ShockwaveRadius, ShockwaveDamage, ETDDamageKind::Explosive, GetOwnerTeam(), Owner, Owner, false, nullptr);
}

void UTDRamPower::OnEnd(bool bInterrupted)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		Craft->ClearSpeedOverride();
	}
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(Owner))
	{
		Health->SetInvulnerable(false);
		Health->SetInvulnerableFor(PostDashInvulnerableSeconds);
	}
	bDashStarted = false;
}
