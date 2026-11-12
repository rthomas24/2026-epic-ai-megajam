// Triad Drift - Powers (Teammate 2). Bastion's shield dome.

#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ATDShieldDome::ATDShieldDome()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SetCanBeDamaged(false);
}

ATDShieldDome* ATDShieldDome::SpawnDome(UWorld* World, AActor* InOwner, ETDTeam InTeam, float InRadius, float InLifetime, UTDAbilityFX* InFX)
{
	if (!World || !InOwner)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = InOwner;
	ATDShieldDome* Dome = World->SpawnActor<ATDShieldDome>(ATDShieldDome::StaticClass(), InOwner->GetActorLocation(), FRotator::ZeroRotator, Params);
	if (!Dome)
	{
		return nullptr;
	}
	Dome->FollowTarget = InOwner;
	Dome->Team = InTeam;
	Dome->Radius = InRadius;
	Dome->Lifetime = InLifetime;
	Dome->FX = InFX;
	Dome->bDomeActive = true;
	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(World))
	{
		Subsystem->RegisterDome(Dome);
	}
	if (InFX)
	{
		Dome->LoopFX = UTDStatics::PlayCueAttached(InFX->Loop, Dome->GetRootComponent());
	}
	return Dome;
}

bool ATDShieldDome::Covers(const FVector& Location) const
{
	return bDomeActive && FVector::DistSquared(GetActorLocation(), Location) <= FMath::Square(Radius);
}

void ATDShieldDome::Collapse()
{
	if (!bDomeActive)
	{
		return;
	}
	bDomeActive = false;
	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(this))
	{
		Subsystem->UnregisterDome(this);
	}
	if (LoopFX)
	{
		LoopFX->Deactivate();
		LoopFX = nullptr;
	}
	if (FX)
	{
		UTDStatics::PlayCue(this, FX->End, GetActorLocation());
	}
	Destroy();
}

void ATDShieldDome::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(this))
	{
		Subsystem->UnregisterDome(this);
	}
	bDomeActive = false;
	Super::EndPlay(EndPlayReason);
}

void ATDShieldDome::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bDomeActive)
	{
		return;
	}
	Age += DeltaSeconds;
	if (Age >= Lifetime)
	{
		Collapse();
		return;
	}

	// Drift after the owner, capped: a fast Bastion can outrun its own dome.
	if (const AActor* Target = FollowTarget.Get())
	{
		const FVector To = Target->GetActorLocation() - GetActorLocation();
		const float MaxStep = FollowSpeed * DeltaSeconds;
		SetActorLocation(GetActorLocation() + (To.SizeSquared() > MaxStep * MaxStep ? To.GetSafeNormal() * MaxStep : To));
	}

	if (bProtectSquad)
	{
		ProtectSquad(DeltaSeconds);
	}
	InterceptProjectiles();

	if (UTDStatics::ShouldDrawPlaceholders() && !LoopFX)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 24, FColor(255, 170, 30), false, -1.f, 0, 8.f);
	}
}

void ATDShieldDome::ProtectSquad(float DeltaSeconds)
{
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Registry)
	{
		return;
	}
	TArray<AActor*, TInlineAllocator<8>> Inside;
	{
		TArray<AActor*> Friendlies;
		Registry->GetActorsOfTeam(Team, Friendlies, true);
		for (AActor* A : Friendlies)
		{
			if (A && Covers(A->GetActorLocation()))
			{
				Inside.Add(A);
			}
		}
	}
	for (AActor* A : Inside)
	{
		UTDHealthComponent* Health = UTDStatics::GetHealth(A);
		if (!Health || Health->IsDead())
		{
			continue;
		}
		Health->SetInvulnerableFor(ProtectionRefreshSeconds);
		// 2x regen: add the ship's own rate again once its regen delay has passed.
		if (Health->GetTimeSinceLastDamage() >= Health->ShieldRegenDelay && Health->GetShield() < Health->MaxShield)
		{
			Health->RestoreShield(Health->MaxShield * Health->ShieldRegenFractionPerSecond * ExtraShieldRegenMultiplier * DeltaSeconds);
		}
	}
}

void ATDShieldDome::InterceptProjectiles()
{
	UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this);
	if (!Projectiles)
	{
		return;
	}
	TArray<ATDProjectile*, TInlineAllocator<32>> Hits;
	for (ATDProjectile* P : Projectiles->GetLiveProjectiles())
	{
		if (P && P->IsActive() && UTDStatics::AreTeamsHostile(Team, P->GetTeam()) && Covers(P->GetActorLocation())
			&& (bDestroyShots || P->IsMissile()))
		{
			Hits.Add(P);
		}
	}
	AActor* DomeOwner = GetOwner();
	for (ATDProjectile* P : Hits)
	{
		if (P->IsMissile() && P->GetSpec().bHoming)
		{
			const FVector Out = (P->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			P->Deflect(DomeOwner ? DomeOwner : this, Out.IsNearlyZero() ? -P->GetVelocity().GetSafeNormal() : Out, Team, 1.f);
			// Put it back on the shell so it doesn't count as inside next tick.
			P->TeleportProjectile(GetActorLocation() + Out * (Radius + 50.f), FVector::ZeroVector);
		}
		else if (bDestroyShots)
		{
			P->Kill(true);
		}
	}
}
