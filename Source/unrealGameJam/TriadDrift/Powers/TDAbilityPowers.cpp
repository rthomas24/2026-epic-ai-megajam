// Triad Drift - Powers (Teammate 2). Gravity Well, Cloak, EMP Pulse.

#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// ================================================================================== Gravity Well actor

ATDGravityWell::ATDGravityWell()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SetCanBeDamaged(false);
}

void ATDGravityWell::Launch(AActor* InOwner, ETDTeam InTeam, const FVector& InDirection, UTDAbilityFX* InFX)
{
	SetOwner(InOwner);
	Team = InTeam;
	Direction = InDirection.GetSafeNormal();
	FX = InFX;
	bLaunched = true;
	Age = 0.f;
	if (FX)
	{
		LoopFX = UTDStatics::PlayCueAttached(FX->Loop, GetRootComponent());
	}
	if (OrbSeconds <= 0.f)
	{
		bVortex = true;
	}
}

void ATDGravityWell::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLaunched)
	{
		return;
	}
	Age += DeltaSeconds;
	if (!bVortex)
	{
		// Fly exactly OrbSpeed x OrbSeconds, whatever the frame rate, then open; carry the leftover time into the vortex.
		const float Flown = FMath::Min(DeltaSeconds, FMath::Max(0.f, OrbSeconds - (Age - DeltaSeconds)));
		SetActorLocation(GetActorLocation() + Direction * OrbSpeed * Flown);
		if (Age >= OrbSeconds)
		{
			bVortex = true;
			Age -= OrbSeconds;
		}
		return;
	}
	TickVortex(DeltaSeconds);
	if (Age >= VortexSeconds)
	{
		Implode();
	}
}

void ATDGravityWell::TickVortex(float DeltaSeconds)
{
	const FVector Center = GetActorLocation();
	const float RadiusSq = Radius * Radius;

	// Enemies: pulled in (moved directly so it works whatever their flight model does), can't fire, 10 dps.
	DamageAccumulator += DeltaSeconds;
	const bool bDamageTick = DamageAccumulator >= DamageTickSeconds;
	if (bDamageTick)
	{
		DamageAccumulator -= DamageTickSeconds;
	}
	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Hostiles;
		Registry->GetHostilesOf(Team, Hostiles, true);
		for (AActor* Hostile : Hostiles)
		{
			if (!IsValid(Hostile))
			{
				continue;
			}
			const FVector To = Center - Hostile->GetActorLocation();
			const float DistSq = To.SizeSquared();
			if (DistSq > RadiusSq)
			{
				continue;
			}
			const float Dist = FMath::Sqrt(DistSq);
			if (Dist > CoreRadius)
			{
				const float Alpha = 1.f - FMath::Clamp(Dist / Radius, 0.f, 1.f);
				const float Speed = FMath::Lerp(EdgePullSpeed, CorePullSpeed, Alpha);
				const float Step = FMath::Min(Speed * DeltaSeconds, Dist - CoreRadius);
				Hostile->SetActorLocation(Hostile->GetActorLocation() + To / Dist * Step);
			}
			if (ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Hostile))
			{
				Enemy->SuppressFire(DamageTickSeconds + 0.1f);
			}
			if (bDamageTick)
			{
				UTDStatics::ApplyTDDamage(Hostile, DamagePerSecond * DamageTickSeconds, ETDDamageKind::Environmental, GetOwner(), this,
					Hostile->GetActorLocation(), To.GetSafeNormal());
			}
		}
	}

	// Hostile projectiles: bend toward the centre and die in the core.
	if (UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
	{
		TArray<ATDProjectile*, TInlineAllocator<32>> Eaten;
		for (ATDProjectile* P : Projectiles->GetLiveProjectiles())
		{
			if (!P || !P->IsActive() || !UTDStatics::AreTeamsHostile(Team, P->GetTeam()))
			{
				continue;
			}
			const FVector To = Center - P->GetActorLocation();
			const float DistSq = To.SizeSquared();
			if (DistSq > RadiusSq)
			{
				continue;
			}
			if (DistSq <= CoreRadius * CoreRadius)
			{
				Eaten.Add(P);
				continue;
			}
			const FVector Vel = P->GetVelocity();
			const float Speed = Vel.Size();
			if (Speed > KINDA_SMALL_NUMBER)
			{
				const FVector Cur = Vel / Speed;
				const FVector Want = To.GetSafeNormal();
				const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Cur, Want), -1.f, 1.f));
				const float MaxStep = FMath::DegreesToRadians(ProjectileBendDegPerSecond) * DeltaSeconds;
				const FVector NewDir = Angle <= MaxStep ? Want
					: FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Cur, Want), MaxStep / Angle).RotateVector(Cur);
				P->SetVelocity(NewDir.GetSafeNormal() * Speed);
				P->BreakTracking(0.2f, false);
			}
		}
		for (ATDProjectile* P : Eaten)
		{
			P->Kill(true);
		}
	}

	if (UTDStatics::ShouldDrawPlaceholders() && !LoopFX)
	{
		DrawDebugSphere(GetWorld(), Center, Radius, 20, FColor(160, 60, 255), false, -1.f, 0, 6.f);
	}
}

void ATDGravityWell::Implode()
{
	UTDStatics::ApplyTDRadialDamage(this, GetActorLocation(), Radius, ImplosionDamage, ETDDamageKind::Explosive, Team, GetOwner(), this, false, nullptr);
	if (FX)
	{
		UTDStatics::PlayCue(this, FX->End, GetActorLocation());
	}
	if (LoopFX)
	{
		LoopFX->Deactivate();
	}
	bLaunched = false;
	bVortex = false;
	Destroy();
}

// ================================================================================== Gravity Well power

UTDGravityWellPower::UTDGravityWellPower()
{
	Slot = ETDPowerInput::Ability;
	PowerName = TEXT("GravityWell");
	CooldownSeconds = 18.f;
}

bool UTDGravityWellPower::OnActivate()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}
	FVector Aim = Owner->GetActorForwardVector();
	if (const UTDWeaponComponent* Weapons = Owner->FindComponentByClass<UTDWeaponComponent>())
	{
		Aim = Weapons->GetAimDirection();
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDGravityWell* Well = World->SpawnActor<ATDGravityWell>(ATDGravityWell::StaticClass(), Owner->GetActorLocation() + Aim * 400.f, Aim.Rotation(), Params);
	if (!Well)
	{
		return false;
	}
	Well->OrbSpeed = OrbSpeed;
	Well->OrbSeconds = OrbSeconds;
	Well->Radius = Radius;
	Well->VortexSeconds = VortexSeconds;
	Well->DamagePerSecond = DamagePerSecond;
	Well->ImplosionDamage = ImplosionDamage;
	Well->Launch(Owner, GetOwnerTeam(), Aim, FX);
	LastWell = Well;
	return true;
}

// ================================================================================== Cloak

UTDCloakPower::UTDCloakPower()
{
	Slot = ETDPowerInput::Ability;
	PowerName = TEXT("Cloak");
	CooldownSeconds = 15.f;
	ActiveSeconds = 6.f;
	bCooldownStartsWhenPowerEnds = true;
}

bool UTDCloakPower::OnActivate()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(Owner))
	{
		Health->SetTargetable(false);
	}
	if (UTDWeaponComponent* Weapons = Owner->FindComponentByClass<UTDWeaponComponent>())
	{
		Weapons->SetNextShotDamageMultiplier(FirstShotMultiplier);
		Weapons->OnWeaponFired.AddDynamic(this, &UTDCloakPower::HandleWeaponFired);
	}
	TInlineComponentArray<UTDPowerComponent*> Powers(Owner);
	for (UTDPowerComponent* Other : Powers)
	{
		if (Other && Other != this)
		{
			Other->OnPowerActivated.AddDynamic(this, &UTDCloakPower::HandleOtherPower);
		}
	}
	// Missiles already in the air lose their lock.
	if (UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
	{
		TArray<ATDProjectile*> Homing;
		Projectiles->FindMissilesHomingOn(Owner, Homing);
		for (ATDProjectile* P : Homing)
		{
			P->BreakTracking(ActiveSeconds, false);
		}
	}
	return true;
}

void UTDCloakPower::OnEnd(bool bInterrupted)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (UTDHealthComponent* Health = UTDStatics::GetHealth(Owner))
	{
		Health->SetTargetable(true);
	}
	if (UTDWeaponComponent* Weapons = Owner->FindComponentByClass<UTDWeaponComponent>())
	{
		Weapons->OnWeaponFired.RemoveAll(this);
		if (!bInterrupted)
		{
			Weapons->SetNextShotDamageMultiplier(1.f); // expired without firing: no banked 3x shot
		}
	}
	TInlineComponentArray<UTDPowerComponent*> Powers(Owner);
	for (UTDPowerComponent* Other : Powers)
	{
		if (Other && Other != this)
		{
			Other->OnPowerActivated.RemoveAll(this);
		}
	}
}

void UTDCloakPower::HandleWeaponFired(ETDWeaponSlot WeaponSlot, FName WeaponId)
{
	EndPower(true);
}

void UTDCloakPower::HandleOtherPower(UTDPowerComponent* Power)
{
	EndPower(true);
}

// ================================================================================== EMP Pulse

UTDEMPPower::UTDEMPPower()
{
	Slot = ETDPowerInput::Ability;
	PowerName = TEXT("EMPPulse");
	CooldownSeconds = 14.f;
}

bool UTDEMPPower::OnActivate()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	const FVector Center = Owner->GetActorLocation();
	const ETDTeam Team = GetOwnerTeam();
	LastAffected = 0;

	if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
	{
		TArray<AActor*> Hostiles;
		Registry->GetHostilesOf(Team, Hostiles, true);
		for (AActor* Hostile : Hostiles)
		{
			if (!IsValid(Hostile) || FVector::DistSquared(Hostile->GetActorLocation(), Center) > FMath::Square(Radius))
			{
				continue;
			}
			if (ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Hostile))
			{
				Enemy->ApplyStun(StunSeconds); // T4: stunned enemies take +25% (StunnedDamageMultiplier)
				if (Enemy->HasFrontalShield())
				{
					Enemy->DropFrontalShield(ShieldDropSeconds);
				}
				++LastAffected;
			}
		}
	}
	if (UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
	{
		TArray<ATDProjectile*> Missiles;
		Projectiles->FindHostileMissiles(Team, Center, Radius, Missiles);
		for (ATDProjectile* M : Missiles)
		{
			M->Kill(true);
		}
	}
	if (UTDStatics::ShouldDrawPlaceholders())
	{
		DrawDebugSphere(GetWorld(), Center, Radius, 24, FColor(80, 220, 255), false, 0.4f, 0, 10.f);
	}
	return true;
}
