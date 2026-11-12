// Triad Drift - Powers (Teammate 2). Lancer: Afterburner Ram.

#include "TriadDrift/Powers/TDRamPower.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDFlightTypes.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

namespace
{
	float TDRamBoundsRadius(const AActor* Actor, FVector& OutCenter)
	{
		FVector Extent;
		Actor->GetActorBounds(true, OutCenter, Extent, false);
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
	bEndOnOwnerDeath = true;
}

float UTDRamPower::GetWindup01() const
{
	if (!IsPowerActive() || WindupSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(ActiveElapsed / WindupSeconds, 0.f, 1.f);
}

float UTDRamPower::GetBoostSpeed() const
{
	if (const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner()))
	{
		if (const UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			return Move->GetFlightTuning().BoostSpeed;
		}
	}
	return FallbackBoostSpeed;
}

float UTDRamPower::GetDashSpeed() const
{
	return GetBoostSpeed() * BoostSpeedMultiplier;
}

bool UTDRamPower::IsHyperdriveBusy() const
{
	if (const ATDShip* Ship = Cast<ATDShip>(GetOwner()))
	{
		if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			return Hyper->GetState() != ETDHyperdriveState::Idle;
		}
	}
	return false;
}

bool UTDRamPower::CanActivate() const
{
	if (!Super::CanActivate() || IsHyperdriveBusy())
	{
		return false;
	}
	// Someone else (hyperdrive, atmosphere entry) owns the speed override: don't fight it.
	if (const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(GetOwner()))
	{
		if (const UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			if (Move->HasSpeedOverride())
			{
				return false;
			}
		}
	}
	return true;
}

void UTDRamPower::RefreshInvulnerability(float Seconds)
{
	// Timed and refreshed, never the untimed flag (TD.God and cinematics own that one).
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(GetOwner()))
	{
		Health->SetInvulnerableFor(Seconds);
	}
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
	DashDistance = 0.f;
	PostDashGrace = 0.f;
	RefreshInvulnerability(0.2f);
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->OnFlightImpact.AddUniqueDynamic(this, &UTDRamPower::HandleFlightImpact);
		}
	}
	if (WindupSeconds <= 0.f)
	{
		BeginDash();
	}
	return true;
}

void UTDRamPower::SetShipCollisionIgnored(bool bIgnore)
{
	// Lancer smashes THROUGH fighters: other ships stop blocking its movement for the dash.
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(GetOwner() ? GetOwner()->GetRootComponent() : nullptr))
	{
		Prim->SetCollisionResponseToChannel(TDCollision::Ship, bIgnore ? ECR_Ignore : ECR_Block);
		bIgnoringShips = bIgnore;
	}
}

void UTDRamPower::BeginDash()
{
	AActor* Owner = GetOwner();
	bDashStarted = true;
	LastLocation = Owner->GetActorLocation();
	DashDirection = Owner->GetActorForwardVector();
	SetShipCollisionIgnored(true);
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			// Order matters: SetSpeedOverride resets the override acceleration to its default.
			Move->SetSpeedOverride(GetDashSpeed(), SteeringScale);
			Move->SetOverrideAcceleration(DashAcceleration);
			// The flight model's velocity grip eases speed changes in over ~0.2 s; the ram is a kick.
			Move->Velocity = Owner->GetActorForwardVector() * GetDashSpeed();
			bOwnsOverride = true;
		}
	}
}

void UTDRamPower::OnTickActive(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || IsHyperdriveBusy())
	{
		EndPower(true);
		return;
	}
	RefreshInvulnerability(0.2f);
	if (!bDashStarted)
	{
		if (ActiveElapsed < WindupSeconds)
		{
			return;
		}
		BeginDash();
	}

	// Owners without the flight model (odd actors) are driven straight along the nose.
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
	if (!Now.Equals(LastLocation))
	{
		DashDirection = (Now - LastLocation).GetSafeNormal();
	}
	DashDistance += FVector::Dist(Now, LastLocation);
	SweepHits(LastLocation, Now);
	LastLocation = Owner->GetActorLocation();
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

	struct FTouch
	{
		TWeakObjectPtr<AActor> Actor;
		float Along = 0.f;
	};
	TArray<FTouch, TInlineAllocator<16>> Touched;
	{
		TArray<AActor*>& Hostiles = HostilesScratch;
		Hostiles.Reset();
		Registry->GetHostilesOf(GetOwnerTeam(), Hostiles, true);
		const FVector Seg = To - From;
		const double SegLenSq = FMath::Max(Seg.SizeSquared(), (double)KINDA_SMALL_NUMBER);
		for (AActor* Hostile : Hostiles)
		{
			if (!IsValid(Hostile) || HitThisDash.Contains(Hostile))
			{
				continue;
			}
			FVector Center;
			const float Radius = TDRamBoundsRadius(Hostile, Center);
			if (FMath::PointDistToSegment(Center, From, To) <= Reach + Radius)
			{
				Touched.Add({ Hostile, (float)(FVector::DotProduct(Center - From, Seg) / SegLenSq) });
			}
		}
	}
	// In path order, so a big ship stops the dash before anything behind it.
	Touched.Sort([](const FTouch& A, const FTouch& B) { return A.Along < B.Along; });
	for (const FTouch& T : Touched)
	{
		if (!IsPowerActive())
		{
			return;
		}
		if (AActor* Victim = T.Actor.Get())
		{
			HitActor(Victim, DashDirection);
		}
	}
}

void UTDRamPower::HitActor(AActor* Victim, const FVector& Direction)
{
	if (!IsValid(Victim))
	{
		return;
	}
	AActor* Owner = GetOwner();
	HitThisDash.Add(Victim);

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

	const UTDHealthComponent* Health = UTDStatics::GetHealth(Victim);
	const bool bWasAlive = !Health || Health->IsAlive();
	UTDStatics::ApplyTDDamage(Victim, RamDamage, ETDDamageKind::Ram, Owner, Owner, Victim->GetActorLocation(), Direction, false);
	// RamKill money shot: UTDDirectorSubsystem derives it from the Ram damage kind on OnActorKilled.
	if (bWasAlive && Health && Health->IsDead())
	{
		if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this))
		{
			Feel->RequestHitStop();
		}
	}

	if (IsBig(Victim))
	{
		// Stop at first contact, not wherever the frame ended (we may be inside the hull by now).
		FVector OwnerCenter, VictimCenter;
		const float Reach = TDRamBoundsRadius(Owner, OwnerCenter) + TDRamBoundsRadius(Victim, VictimCenter);
		const float Back = FMath::Max(0.f, Reach - (float)FVector::Dist(Owner->GetActorLocation(), VictimCenter));
		if (Back > 0.f)
		{
			Owner->SetActorLocation(Owner->GetActorLocation() - Direction * Back);
		}
		StopDash(Owner->GetActorLocation() + Direction * 200.f, Victim);
	}
}

void UTDRamPower::HandleFlightImpact(const FTDFlightImpact& Impact)
{
	if (!IsDashing())
	{
		return;
	}
	// Ships are ignored during the dash, so anything the flight model still hits is world geometry.
	AActor* Other = Impact.OtherActor.Get();
	if (Other && Other->IsA<ATDSpacecraft>())
	{
		return;
	}
	StopDash(Impact.ImpactPoint, Other);
}

void UTDRamPower::StopDash(const FVector& Where, AActor* Stopper)
{
	bStoppedByImpact = true;
	AActor* Owner = GetOwner();
	// The shockwave spares whatever stopped us: it already took the full 300.
	UTDStatics::ApplyTDRadialDamage(this, Where, ShockwaveRadius, ShockwaveDamage, ETDDamageKind::Explosive, GetOwnerTeam(), Owner, Owner, false, Stopper);
	EndPower(false);
}

bool UTDRamPower::IsBig(const AActor* Actor) const
{
	if (const ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Actor))
	{
		return static_cast<uint8>(Enemy->GetSize()) >= static_cast<uint8>(StopOnSize);
	}
	return false; // bays, turrets and unknown actors are smashed through
}

void UTDRamPower::OnEnd(bool bInterrupted)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (bIgnoringShips)
	{
		SetShipCollisionIgnored(false);
	}
	if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Owner))
	{
		if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
		{
			Move->OnFlightImpact.RemoveAll(this);
			if (bOwnsOverride)
			{
				Move->ClearSpeedOverride();
				Move->SetOverrideAcceleration(-1.f);
			}
		}
	}
	bOwnsOverride = false;
	bDashStarted = false;
	// Stay safe while the speed bleeds off (head-on hits at 45 000 cm/s are lethal).
	PostDashGrace = MaxPostDashInvulnerableSeconds;
	RefreshInvulnerability(0.2f);
}

void UTDRamPower::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsPowerActive() && PostDashGrace > 0.f)
	{
		PostDashGrace -= DeltaTime;
		const AActor* Owner = GetOwner();
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Owner);
		if (!Owner || (Health && Health->IsDead()) || Owner->GetVelocity().Size() <= GetBoostSpeed() * 1.02f)
		{
			PostDashGrace = 0.f;
			return;
		}
		RefreshInvulnerability(0.1f);
	}
}
