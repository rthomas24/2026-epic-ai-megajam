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
#include "TriadDrift/Powers/TDPowerPresentation.h"
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

	// Follow the owner with a lag (frame-rate independent exponential approach).
	if (const AActor* Target = FollowTarget.Get())
	{
		const float Alpha = 1.f - FMath::Exp(-FollowLagRate * DeltaSeconds);
		SetActorLocation(FMath::Lerp(GetActorLocation(), Target->GetActorLocation(), Alpha));
	}

	if (bProtectSquad)
	{
		ProtectSquad(DeltaSeconds);
	}
	InterceptProjectiles();

	if (!bPlaceholderBuilt)
	{
		// Built on the first tick, once the spawner has set the mode (dome vs Blink's reflect field).
		bPlaceholderBuilt = true;
		if (!FX || !FTDPowerPresentation::HasArt(FX->Loop))
		{
			const FLinearColor Color = bProtectSquad ? UTDStatics::ShipColor(ETDShipId::Bastion) : UTDStatics::ShipColor(ETDShipId::Rift);
			FTDPowerPresentation::AddRingLattice(this, nullptr, Radius, 3, 48, 60.f, Color, 4.f);
		}
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
		TArray<AActor*>& Friendlies = FriendlyScratch;
		Friendlies.Reset();
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
	// Projectiles stop themselves on the shell during their own swept step (ATDProjectile asks
	// UTDPowerSubsystem); this catches the rest: shots that were already inside (fired from inside, or
	// the dome just moved / opened over them).
	TArray<ATDProjectile*, TInlineAllocator<32>> Hits;
	for (ATDProjectile* P : Projectiles->GetLiveProjectiles())
	{
		if (P && P->IsActive() && Intercepts(P) && Covers(P->GetActorLocation()))
		{
			Hits.Add(P);
		}
	}
	for (ATDProjectile* P : Hits)
	{
		const FVector Out = (P->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		ReceiveProjectile(P, GetActorLocation() + (Out.IsNearlyZero() ? FVector::UpVector : Out) * Radius);
	}
}

bool ATDShieldDome::Intercepts(const ATDProjectile* P) const
{
	return bDomeActive && P && P->IsActive() && UTDStatics::AreTeamsHostile(Team, P->GetTeam())
		&& (bDestroyShots || (P->IsMissile() && P->GetSpec().bHoming));
}

void ATDShieldDome::ReceiveProjectile(ATDProjectile* P, const FVector& ShellPoint)
{
	if (!Intercepts(P))
	{
		return;
	}
	if (P->IsMissile() && P->GetSpec().bHoming)
	{
		const FVector Out = (ShellPoint - GetActorLocation()).GetSafeNormal();
		AActor* DomeOwner = GetOwner();
		P->Deflect(DomeOwner ? DomeOwner : this, Out.IsNearlyZero() ? -P->GetVelocity().GetSafeNormal() : Out, Team, 1.f);
		// Just outside the shell so it doesn't count as inside next tick.
		P->TeleportProjectile(GetActorLocation() + Out * (Radius + 50.f), FVector::ZeroVector);
	}
	else if (bDestroyShots)
	{
		P->TeleportProjectile(ShellPoint, FVector::ZeroVector); // impact FX on the shell, not inside it
		P->Kill(true);
	}
}
