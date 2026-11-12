// Triad Drift - Powers (Teammate 2). Gravity Well, Cloak, EMP Pulse.

#include "TriadDrift/Powers/TDAbilityPowers.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "NiagaraComponent.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "Components/InstancedStaticMeshComponent.h"
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
	if (!FX || !FTDPowerPresentation::HasArt(FX->Loop))
	{
		// Shipping-safe placeholder: a glowing violet core (the ring appears when the vortex opens).
		FTDPowerPresentation::AddShape(this, nullptr, FTDPowerPresentation::EShape::Sphere, FVector(6.f), FRotator::ZeroRotator,
			FLinearColor(0.55f, 0.2f, 1.f), 6.f);
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
		TArray<AActor*>& Hostiles = HostilesScratch;
		Hostiles.Reset();
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
			if (Dist > CoreRadius && CanBePulled(Hostile))
			{
				const float Alpha = 1.f - FMath::Clamp(Dist / Radius, 0.f, 1.f);
				const float Speed = FMath::Lerp(EdgePullSpeed, CorePullSpeed, Alpha);
				const float Step = FMath::Min(Speed * DeltaSeconds, Dist - CoreRadius);
				// Through the ship's own movement component: a swept move (never through rocks or hulls) plus
				// damping of its own velocity toward the pull, so a fighter can't simply out-fly the vortex.
				ATDSpacecraft* Craft = CastChecked<ATDSpacecraft>(Hostile);
				if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
				{
					// Caught: engines held at a crawl (unless something else owns the override), then dragged in
					// with a swept move, so a fighter can't out-fly the vortex or be pulled through a hull.
					if (!Captured.Contains(Craft) && !Move->HasSpeedOverride())
					{
						Move->SetSpeedOverride(CapturedSpeed, CapturedSteering);
						Captured.Add(Craft);
						if (FX)
						{
							UTDStatics::PlayCueAttached(FX->AffectedTarget, Craft->GetRootComponent());
						}
					}
					FHitResult Hit;
					Move->SafeMoveUpdatedComponent(To / Dist * Step, Hostile->GetActorQuat(), true, Hit);
				}
			}
			// Can't fire inside: ships suppress, turrets (carrier, Dominion) are held stunned.
			if (ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Hostile))
			{
				Enemy->SuppressFire(DamageTickSeconds + 0.1f);
			}
			else if (ATDEnemyTurret* Turret = Cast<ATDEnemyTurret>(Hostile))
			{
				Turret->ApplyStun(DamageTickSeconds + 0.1f);
			}
			if (bDamageTick)
			{
				UTDStatics::ApplyTDDamage(Hostile, DamagePerSecond * DamageTickSeconds, ETDDamageKind::Environmental, GetOwner(), this,
					Hostile->GetActorLocation(), To.GetSafeNormal());
			}
		}
	}

	ReleaseEscaped(Center, RadiusSq);

	// Hostile projectiles: bend toward the centre, get dragged in (no stable orbits), die in the core.
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
				const float Dist = FMath::Sqrt(DistSq);
				P->TeleportProjectile(P->GetActorLocation() + To / Dist * FMath::Min(ProjectilePullSpeed * DeltaSeconds, Dist), FVector::ZeroVector);
			}
		}
		for (ATDProjectile* P : Eaten)
		{
			P->Kill(true);
		}
	}

	if (!bRingBuilt && (!FX || !FTDPowerPresentation::HasArt(FX->Loop)))
	{
		bRingBuilt = true;
		FTDPowerPresentation::AddRingLattice(this, nullptr, Radius, 1, 48, 70.f, FLinearColor(0.55f, 0.2f, 1.f), 4.f);
	}
}

bool ATDGravityWell::CanBePulled(const AActor* Actor) const
{
	// Only free-flying ships, and only the small ones: generators, turrets, the reactor, carrier bays (all
	// attached or not spacecraft) and Large/Boss hulls stay where the level put them. They still take the damage.
	const ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Actor);
	if (!Craft || Craft->GetAttachParentActor() || !Craft->GetShipMovement())
	{
		return false;
	}
	if (const ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Craft))
	{
		return static_cast<uint8>(Enemy->GetSize()) <= static_cast<uint8>(MaxPulledSize);
	}
	return true;
}

void ATDGravityWell::ReleaseEscaped(const FVector& Center, float RadiusSq)
{
	for (auto It = Captured.CreateIterator(); It; ++It)
	{
		ATDSpacecraft* Craft = It->Get();
		const UTDHealthComponent* Health = Craft ? UTDStatics::GetHealth(Craft) : nullptr;
		if (!Craft || (Health && Health->IsDead()) || FVector::DistSquared(Craft->GetActorLocation(), Center) > RadiusSq)
		{
			if (Craft && Craft->GetShipMovement())
			{
				Craft->GetShipMovement()->ClearSpeedOverride();
			}
			It.RemoveCurrent();
		}
	}
}

void ATDGravityWell::ReleaseAll()
{
	for (const TWeakObjectPtr<ATDSpacecraft>& W : Captured)
	{
		if (ATDSpacecraft* Craft = W.Get())
		{
			if (Craft->GetShipMovement())
			{
				Craft->GetShipMovement()->ClearSpeedOverride();
			}
		}
	}
	Captured.Reset();
}

void ATDGravityWell::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAll(); // a retake / reset destroys the well mid-vortex: never leave ships crawling
	Super::EndPlay(EndPlayReason);
}

void ATDGravityWell::Implode()
{
	ReleaseAll();
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
	if (UTDPowerSubsystem* Subsystem = UTDPowerSubsystem::Get(this))
	{
		Subsystem->RegisterTransient(Well);
	}
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
	bEndedByShot = false;
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
	if (FX)
	{
		LoopFX = UTDStatics::PlayCueAttached(FX->Loop, Owner->GetRootComponent());
	}
	if (!FX || !FTDPowerPresentation::HasArt(FX->Loop))
	{
		FVector Origin, Extent;
		Owner->GetActorBounds(true, Origin, Extent, false);
		Shimmer = FTDPowerPresentation::AddRingLattice(Owner, nullptr, FMath::Max(150.f, (float)Extent.GetMax() * 1.4f), 2, 24, 15.f,
			UTDStatics::ShipColor(ETDShipId::Rift) * 0.6f, 1.5f);
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
	if (Shimmer)
	{
		Shimmer->DestroyComponent();
		Shimmer = nullptr;
	}
	if (LoopFX)
	{
		LoopFX->Deactivate();
		LoopFX = nullptr;
	}
	if (UTDWeaponComponent* Weapons = Owner->FindComponentByClass<UTDWeaponComponent>())
	{
		Weapons->OnWeaponFired.RemoveAll(this);
		if (!bEndedByShot)
		{
			// Expired, broken by another power, death, retake...: no banked 3x shot for later.
			Weapons->SetNextShotDamageMultiplier(1.f);
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
	bEndedByShot = true;
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
			if (FX)
			{
				UTDStatics::PlayCueAttached(FX->AffectedTarget, Hostile->GetRootComponent());
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
			else if (ATDEnemyTurret* Turret = Cast<ATDEnemyTurret>(Hostile))
			{
				Turret->ApplyStun(StunSeconds); // carrier point defense, Dominion guard turrets
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
	if (!FX || !FTDPowerPresentation::HasArt(FX->Activate))
	{
		ATDPlaceholderFlash::SpawnRingFlash(GetWorld(), Center, Radius, UTDStatics::ShipColor(ETDShipId::Lancer), 0.4f);
	}
	return true;
}
