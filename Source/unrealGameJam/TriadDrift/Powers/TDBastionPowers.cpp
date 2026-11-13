// Triad Drift - Powers (Teammate 2). Bastion: Missile Swarm and Shield Dome.

#include "TriadDrift/Powers/TDBastionPowers.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "Engine/World.h"

// ------------------------------------------------------------------------------------------ swarm

UTDMissileSwarmPower::UTDMissileSwarmPower()
{
	Slot = ETDPowerInput::Signature1;
	PowerName = TEXT("MissileSwarm");
	CooldownSeconds = 9.f;
	ActiveSeconds = 0.f;

	MissileSpec.Damage = 18.f;
	MissileSpec.DamageKind = ETDDamageKind::Explosive;
	MissileSpec.SplashRadius = 200.f;
	MissileSpec.SplashDamage = 18.f;
	MissileSpec.Speed = 14000.f;
	MissileSpec.Range = 14000.f * 4.f;
	MissileSpec.CollisionRadius = 60.f;
	MissileSpec.bHoming = true;
	MissileSpec.HomingTurnRateDeg = 240.f;
	MissileSpec.HomingAcquireConeDeg = 60.f;
	MissileSpec.HomingDelay = 0.15f;
	MissileSpec.bIsMissile = true;
	MissileSpec.bInheritShooterVelocity = true;
	MissileSpec.PlaceholderColor = FLinearColor(1.0f, 0.62f, 0.08f);
	MissileSpec.PlaceholderScale = FVector(0.8f, 0.25f, 0.25f);
}

void UTDMissileSwarmPower::BeginPlay()
{
	Super::BeginPlay();
	if (!MissileSpec.FX)
	{
		MissileSpec.FX = FTDPowerPresentation::LoadMissileSwarmFX();
	}
}

UTDTargetingComponent* UTDMissileSwarmPower::GetTargeting() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UTDTargetingComponent>() : nullptr;
}

void UTDMissileSwarmPower::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bAutoPaint)
	{
		return;
	}
	UTDTargetingComponent* Targeting = GetTargeting();
	if (!Targeting)
	{
		return;
	}
	// Locks beyond the swarm's reach are dropped (review: old locks behind the player or out of range).
	Targeting->LockMaxRange = MissileSpec.Range;
	const bool bShouldPaint = CooldownRemaining <= 0.f && (!bRequirePlayerControl || IsOwnerPlayerControlled());
	if (bShouldPaint && !Targeting->IsPainting())
	{
		Targeting->BeginLockPainting();
	}
	else if (!bShouldPaint && Targeting->IsPainting())
	{
		Targeting->EndLockPainting();
		Targeting->ClearLocks();
	}
}

bool UTDMissileSwarmPower::OnActivate()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || MissileCount <= 0)
	{
		return false;
	}

	TArray<AActor*> Targets;
	if (UTDTargetingComponent* Targeting = GetTargeting())
	{
		Targets = Targeting->IsPainting() ? Targeting->EndLockPainting() : Targeting->GetLockedTargets();
		if (Targets.Num() == 0 && Targeting->GetCurrentTarget())
		{
			Targets.Add(Targeting->GetCurrentTarget());
		}
		Targeting->ClearLocks();
	}
	LastTargets.Reset();
	for (AActor* T : Targets)
	{
		LastTargets.Add(T);
	}

	const FVector Origin = Owner->GetActorLocation();
	const FVector Fwd = Owner->GetActorForwardVector();
	FVector Up, Right;
	Fwd.FindBestAxisVectors(Up, Right);
	const float Cone = FMath::DegreesToRadians(LaunchConeDeg);
	const ETDTeam Team = GetOwnerTeam();
	// Switch Strike's +25% applies to the swarm too (second review minor).
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	const float Strike = Squad ? Squad->GetDamageMultiplier(Owner) : 1.f;

	for (int32 i = 0; i < MissileCount; ++i)
	{
		// Even ring fan so the swarm reads as a burst, then each missile curls onto its target.
		const float Around = (2.f * PI * i) / MissileCount;
		const FVector Dir = (Fwd * FMath::Cos(Cone) + (Up * FMath::Cos(Around) + Right * FMath::Sin(Around)) * FMath::Sin(Cone)).GetSafeNormal();
		AActor* Target = Targets.Num() > 0 ? Targets[i % Targets.Num()] : nullptr;
		ATDProjectile* P = ATDProjectile::Launch(World, MissileSpec, FTransform(Dir.Rotation(), Origin + Dir * 200.f), Owner, Team, Target,
			Owner->GetVelocity(), Strike, IsOwnerPlayerControlled());
		(void)P;
	}
	return true;
}

// ------------------------------------------------------------------------------------------ dome

UTDShieldDomePower::UTDShieldDomePower()
{
	Slot = ETDPowerInput::Signature2;
	PowerName = TEXT("ShieldDome");
	CooldownSeconds = 16.f;
	ActiveSeconds = 6.f;
	bCooldownStartsWhenPowerEnds = true;
	bEndOnOwnerDeath = false; // the dome keeps protecting the squad
}

bool UTDShieldDomePower::OnActivate()
{
	AActor* Owner = GetOwner();
	ActiveSeconds = DomeSeconds;
	ATDShieldDome* NewDome = ATDShieldDome::SpawnDome(GetWorld(), Owner, GetOwnerTeam(), DomeRadius, DomeSeconds + 0.1f, FX);
	Dome = NewDome;
	if (NewDome)
	{
		// Nick's Power2Active (or ability FX) is the dome's look: no placeholder lattice under it (review).
		NewDome->SetCoveredByArt(HasActiveArt());
	}
	return NewDome != nullptr;
}

USceneComponent* UTDShieldDomePower::GetActiveCueRoot() const
{
	if (const ATDShieldDome* D = Dome.Get())
	{
		return D->GetRootComponent(); // the dome lags Bastion by up to ~2000 cm: its art rides the dome
	}
	return Super::GetActiveCueRoot();
}

void UTDShieldDomePower::OnEnd(bool bInterrupted)
{
	if (ATDShieldDome* D = Dome.Get())
	{
		D->Collapse();
	}
	Dome = nullptr;
}
