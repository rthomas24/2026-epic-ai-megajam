// Triad Drift - Combat (Teammate 2). Swept, pooled projectile.

#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

namespace
{
	// Scratch arrays reused across projectiles (game thread only) so hot paths never allocate.
	TArray<AActor*>& HostileScratch()
	{
		static TArray<AActor*> Scratch;
		return Scratch;
	}
	TArray<ATDProjectile*>& MissileScratch()
	{
		static TArray<ATDProjectile*> Scratch;
		return Scratch;
	}

	/** Conservative radius of an actor's bounds (largest half-extent). */
	float BoundsRadius(const AActor* Actor, FVector& OutCenter)
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

	float DistanceToBounds(const AActor* Actor, const FVector& Point)
	{
		FVector Origin, Extent;
		Actor->GetActorBounds(true, Origin, Extent);
		if (Extent.IsNearlyZero())
		{
			return FVector::Dist(Actor->GetActorLocation(), Point);
		}
		return FMath::Sqrt(FBox(Origin - Extent, Origin + Extent).ComputeSquaredDistanceToPoint(Point));
	}

	bool IsLiveTarget(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return false;
		}
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Actor);
		return !Health || Health->IsAlive();
	}
}

ATDProjectile::ATDProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(Root);
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetGenerateOverlapEvents(false);
	PlaceholderMesh->SetCastShadow(false);
	PlaceholderMesh->SetCanEverAffectNavigation(false);
	PlaceholderMesh->bReceivesDecals = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (SphereMesh.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		PlaceholderMesh->SetMaterial(0, ShapeMaterial.Object);
	}

	SetActorHiddenInGame(true);
}

// ------------------------------------------------------------------------------------------ spawning

ATDProjectile* ATDProjectile::Spawn(UWorld* World, const FTDProjectileSpec& InSpec, const FTransform& Transform, AActor* InInstigator,
	ETDTeam InTeam, AActor* InHomingTarget)
{
	return Launch(World, InSpec, Transform, InInstigator, InTeam, InHomingTarget, FVector::ZeroVector, 1.f, false);
}

ATDProjectile* ATDProjectile::Launch(UWorld* World, const FTDProjectileSpec& InSpec, const FTransform& Transform, AActor* InInstigator,
	ETDTeam InTeam, AActor* InHomingTarget, const FVector& InheritedVelocity, float InDamageMultiplier, bool bInReportHits)
{
	UTDProjectileSubsystem* Subsystem = World ? World->GetSubsystem<UTDProjectileSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return nullptr;
	}
	ATDProjectile* P = Subsystem->Acquire();
	if (!P)
	{
		return nullptr;
	}
	P->Activate(InSpec, Transform, InInstigator, InTeam, InHomingTarget, InheritedVelocity, InDamageMultiplier, bInReportHits);
	return P;
}

void ATDProjectile::Activate(const FTDProjectileSpec& InSpec, const FTransform& Transform, AActor* InInstigator, ETDTeam InTeam,
	AActor* InHomingTarget, const FVector& InheritedVelocity, float InDamageMultiplier, bool bInReportHits)
{
	Spec = InSpec;
	Team = InTeam;
	Shooter = InInstigator;
	OriginalShooter = InInstigator;
	HomingTarget = InHomingTarget;
	IgnoreActor = nullptr;
	SetInstigator(Cast<APawn>(InInstigator));
	DamageMultiplier = FMath::Max(InDamageMultiplier, 0.f);
	bReportHits = bInReportHits;
	DeflectCount = 0;
	TrackingBlockedFor = 0.f;
	bReacquireAfterBlock = false;
	Age = 0.f;
	Lifetime = FMath::Max(Spec.GetLifetime(), 0.05f);

	const FVector Dir = Transform.GetRotation().GetForwardVector();
	Velocity = Dir * Spec.Speed;
	if (Spec.bInheritShooterVelocity)
	{
		Velocity += Dir * FMath::Max(0.f, FVector::DotProduct(InheritedVelocity, Dir));
	}

	SetActorLocationAndRotation(Transform.GetLocation(), Dir.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	bActive = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
	{
		Subsystem->RegisterLive(this);
	}

	if (Spec.bHoming && !HomingTarget.IsValid())
	{
		AcquireHomingTarget();
	}

	UpdatePlaceholderLook();
}

void ATDProjectile::UpdatePlaceholderLook()
{
	bool bShowPlaceholder = true;

	if (TracerFX)
	{
		TracerFX->DestroyComponent();
		TracerFX = nullptr;
	}

	if (Spec.FX)
	{
		if (!Spec.FX->Tracer.IsNull())
		{
			FTDFXCue TracerCue;
			TracerCue.VFX = Spec.FX->Tracer;
			TracerFX = UTDStatics::PlayCueAttached(TracerCue, Root);
			bShowPlaceholder = TracerFX == nullptr;
		}
		if (!Spec.FX->ProjectileMesh.IsNull())
		{
			if (UStaticMesh* Mesh = Spec.FX->ProjectileMesh.LoadSynchronous())
			{
				PlaceholderMesh->SetStaticMesh(Mesh);
				bShowPlaceholder = true;
			}
		}
	}

	PlaceholderMesh->SetVisibility(bShowPlaceholder);
	if (bShowPlaceholder)
	{
		PlaceholderMesh->SetRelativeScale3D(Spec.PlaceholderScale);
		if (!PlaceholderMID)
		{
			PlaceholderMID = PlaceholderMesh->CreateDynamicMaterialInstance(0);
		}
		if (PlaceholderMID)
		{
			const FLinearColor Color = (Spec.FX && !Spec.FX->Color.Equals(FLinearColor::White)) ? Spec.FX->Color : Spec.PlaceholderColor;
			PlaceholderMID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}

void ATDProjectile::Deactivate()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	Velocity = FVector::ZeroVector;
	HomingTarget = nullptr;
	Shooter = nullptr;
	OriginalShooter = nullptr;
	IgnoreActor = nullptr;
	OnDetonated.Clear();
	if (TracerFX)
	{
		TracerFX->DestroyComponent();
		TracerFX = nullptr;
	}
	if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
	{
		Subsystem->Release(this);
	}
	else
	{
		Destroy();
	}
}

void ATDProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
	{
		Subsystem->UnregisterLive(this);
	}
	bActive = false;
	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------------------------------------ public API

void ATDProjectile::Deflect(AActor* NewInstigator, FVector NewDirection, ETDTeam NewTeam, float SpeedMultiplier)
{
	if (!bActive)
	{
		return;
	}
	const float Speed = FMath::Max(Velocity.Size(), Spec.Speed) * FMath::Max(SpeedMultiplier, 0.05f);
	const FVector Dir = NewDirection.IsNearlyZero() ? -Velocity.GetSafeNormal() : NewDirection.GetSafeNormal();

	Team = NewTeam;
	Shooter = NewInstigator;
	SetInstigator(Cast<APawn>(NewInstigator));
	IgnoreActor = NewInstigator;
	Velocity = Dir * Speed;
	++DeflectCount;
	bReportHits = (NewTeam == ETDTeam::Player);

	// Re-home on whoever fired it: a returned shot should come back to bite them.
	AActor* Original = OriginalShooter.Get();
	if (IsLiveTarget(Original) && UTDStatics::AreTeamsHostile(NewTeam, UTDStatics::GetTeam(Original)))
	{
		HomingTarget = Original;
		Spec.bHoming = true;
		Spec.HomingTurnRateDeg = FMath::Max(Spec.HomingTurnRateDeg, DeflectHomingTurnRateDeg);
		Spec.HomingDelay = 0.f;
	}
	else
	{
		HomingTarget = nullptr;
	}
	TrackingBlockedFor = 0.f;

	// A returned shot gets a fresh lifetime so it can make it home.
	Age = 0.f;
	Lifetime = FMath::Max(Lifetime, 4.f);
	SetActorRotation(Dir.Rotation());

	if (Spec.FX)
	{
		UTDStatics::PlayCue(this, Spec.FX->Deflected, GetActorLocation(), Dir.Rotation());
	}
}

void ATDProjectile::Kill(bool bExplodeFX)
{
	if (!bActive)
	{
		return;
	}
	if (bExplodeFX && Spec.FX)
	{
		UTDStatics::PlayCue(this, Spec.FX->Detonation, GetActorLocation(), GetActorRotation());
	}
	Deactivate();
}

void ATDProjectile::Detonate()
{
	if (!bActive)
	{
		return;
	}
	const FVector Here = GetActorLocation();
	if (Spec.SplashRadius > 0.f)
	{
		ApplySplash(Here, nullptr);
	}
	if (Spec.ShardCount > 0)
	{
		SpawnShards(Here, Velocity.GetSafeNormal(), IgnoreActor.Get());
	}
	if (Spec.FX)
	{
		UTDStatics::PlayCue(this, Spec.FX->Detonation, Here, GetActorRotation());
	}
	OnDetonated.Broadcast(this, nullptr);
	Deactivate();
}

void ATDProjectile::SetHomingTarget(AActor* NewTarget)
{
	HomingTarget = NewTarget;
	if (NewTarget)
	{
		Spec.bHoming = true;
	}
}

void ATDProjectile::BreakTracking(float Seconds, bool bReacquire)
{
	TrackingBlockedFor = FMath::Max(TrackingBlockedFor, Seconds);
	bReacquireAfterBlock = bReacquire;
	if (!bReacquire)
	{
		HomingTarget = nullptr;
	}
}

void ATDProjectile::TeleportProjectile(const FVector& NewLocation, const FVector& NewDirection)
{
	if (!NewDirection.IsNearlyZero())
	{
		Velocity = NewDirection.GetSafeNormal() * Velocity.Size();
	}
	SetActorLocationAndRotation(NewLocation, Velocity.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
}

// ------------------------------------------------------------------------------------------ simulation

void ATDProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Step(DeltaSeconds);
}

void ATDProjectile::Step(float DeltaSeconds)
{
	if (!bActive || DeltaSeconds <= 0.f)
	{
		return;
	}

	Age += DeltaSeconds;
	TrackingBlockedFor = FMath::Max(0.f, TrackingBlockedFor - DeltaSeconds);

	UpdateHoming(DeltaSeconds);

	const FVector Start = GetActorLocation();
	FVector End = Start + Velocity * DeltaSeconds;
	const bool bExpiring = Age >= Lifetime;
	if (bExpiring)
	{
		// Only fly the part of this step that was still inside the lifetime.
		const float Over = Age - Lifetime;
		End = Start + Velocity * FMath::Max(0.f, DeltaSeconds - Over);
	}

	// Point defense style: knock hostile missiles out of the air along the path.
	if (Spec.bShootsDownMissiles && Spec.ProximityFuseRadius <= 0.f)
	{
		UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this);
		if (Subsystem)
		{
			TArray<ATDProjectile*>& Missiles = MissileScratch();
			Missiles.Reset();
			const float SegLen = FVector::Dist(Start, End);
			Subsystem->FindHostileMissiles(Team, (Start + End) * 0.5f, SegLen * 0.5f + Spec.CollisionRadius + 150.f, Missiles);
			for (ATDProjectile* M : Missiles)
			{
				if (M != this && FMath::PointDistToSegment(M->GetActorLocation(), Start, End) <= Spec.CollisionRadius + M->GetSpec().CollisionRadius + 60.f)
				{
					M->Kill(true);
					SetActorLocation(M->GetActorLocation());
					OnDetonated.Broadcast(this, M);
					Deactivate();
					return;
				}
			}
		}
	}

	// Proximity fuse (flak, mines): detonate at the closest approach along this step's path.
	if (Spec.ProximityFuseRadius > 0.f && Age >= Spec.FuseArmDelay)
	{
		float BestT = 2.f;
		const FVector Seg = End - Start;
		const float SegLenSq = Seg.SizeSquared();
		auto ConsiderPoint = [&](const FVector& Point, float ExtraRadius)
		{
			const FVector Closest = FMath::ClosestPointOnSegment(Point, Start, End);
			if (FVector::Dist(Closest, Point) - ExtraRadius <= Spec.ProximityFuseRadius)
			{
				const float T = SegLenSq > KINDA_SMALL_NUMBER ? FVector::DotProduct(Closest - Start, Seg) / SegLenSq : 0.f;
				BestT = FMath::Min(BestT, T);
			}
		};

		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			TArray<AActor*>& Hostiles = HostileScratch();
			Hostiles.Reset();
			Registry->GetHostilesOf(Team, Hostiles, true);
			for (AActor* Hostile : Hostiles)
			{
				if (Hostile && Hostile != IgnoreActor.Get())
				{
					FVector Center;
					const float Radius = BoundsRadius(Hostile, Center);
					ConsiderPoint(Center, Radius);
				}
			}
		}
		if (Spec.bShootsDownMissiles)
		{
			if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
			{
				TArray<ATDProjectile*>& Missiles = MissileScratch();
				Missiles.Reset();
				Subsystem->FindHostileMissiles(Team, (Start + End) * 0.5f, FVector::Dist(Start, End) * 0.5f + Spec.ProximityFuseRadius + 200.f, Missiles);
				for (ATDProjectile* M : Missiles)
				{
					if (M != this)
					{
						ConsiderPoint(M->GetActorLocation(), M->GetSpec().CollisionRadius);
					}
				}
			}
		}
		if (BestT <= 1.f)
		{
			const FVector FuseLocation = Start + Seg * FMath::Clamp(BestT, 0.f, 1.f);
			// A direct hit on the way still counts first.
			FHitResult Hit;
			if (SweepPath(Start, FuseLocation, Hit))
			{
				SetActorLocation(Hit.Location);
				HandleImpact(Hit);
				return;
			}
			SetActorLocation(FuseLocation);
			Detonate();
			return;
		}
	}

	FHitResult Hit;
	if (SweepPath(Start, End, Hit))
	{
		SetActorLocation(Hit.Location);
		HandleImpact(Hit);
		return;
	}

	SetActorLocationAndRotation(End, Velocity.Rotation());

	if (bExpiring)
	{
		if (Spec.bDetonateOnExpire)
		{
			Detonate();
		}
		else
		{
			Kill(false);
		}
	}
}

void ATDProjectile::UpdateHoming(float DeltaSeconds)
{
	if (!Spec.bHoming || Age < Spec.HomingDelay || TrackingBlockedFor > 0.f)
	{
		return;
	}

	AActor* Target = HomingTarget.Get();
	if (!IsLiveTarget(Target))
	{
		HomingTarget = nullptr;
		if (DeflectCount == 0 || bReacquireAfterBlock)
		{
			AcquireHomingTarget();
		}
		Target = HomingTarget.Get();
		if (!Target)
		{
			return;
		}
	}
	// Cloak: a target that stopped being targetable is no longer tracked (deflected returns keep tracking).
	if (DeflectCount == 0)
	{
		if (const UTDHealthComponent* Health = UTDStatics::GetHealth(Target))
		{
			if (!Health->bTargetable)
			{
				HomingTarget = nullptr;
				return;
			}
		}
	}

	const float Speed = Velocity.Size();
	if (Speed <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector CurrentDir = Velocity / Speed;
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	// Mild lead: aim at where the target will be when we close the current distance.
	const float TimeToTarget = ToTarget.Size() / Speed;
	const FVector Predicted = Target->GetActorLocation() + Target->GetVelocity() * FMath::Min(TimeToTarget, 1.5f) * 0.5f;
	const FVector DesiredDir = (Predicted - GetActorLocation()).GetSafeNormal();
	if (DesiredDir.IsNearlyZero())
	{
		return;
	}

	const float AngleRad = FMath::Acos(FMath::Clamp(FVector::DotProduct(CurrentDir, DesiredDir), -1.f, 1.f));
	const float MaxStepRad = FMath::DegreesToRadians(Spec.HomingTurnRateDeg) * DeltaSeconds;
	FVector NewDir = DesiredDir;
	if (AngleRad > MaxStepRad && AngleRad > KINDA_SMALL_NUMBER)
	{
		const FQuat Full = FQuat::FindBetweenNormals(CurrentDir, DesiredDir);
		NewDir = FQuat::Slerp(FQuat::Identity, Full, MaxStepRad / AngleRad).RotateVector(CurrentDir).GetSafeNormal();
	}
	Velocity = NewDir * Speed;
}

void ATDProjectile::AcquireHomingTarget()
{
	UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Registry)
	{
		return;
	}
	TArray<AActor*>& Hostiles = HostileScratch();
	Hostiles.Reset();
	Registry->GetHostilesOf(Team, Hostiles, false);

	const FVector Here = GetActorLocation();
	const FVector Dir = Velocity.GetSafeNormal();
	const float CosCone = FMath::Cos(FMath::DegreesToRadians(Spec.HomingAcquireConeDeg));
	const float MaxDist = FMath::Max(Spec.Speed * (Lifetime - Age), 1000.f);

	AActor* Best = nullptr;
	float BestDistSq = MaxDist * MaxDist;
	for (AActor* Hostile : Hostiles)
	{
		if (!Hostile || Hostile == IgnoreActor.Get())
		{
			continue;
		}
		const FVector To = Hostile->GetActorLocation() - Here;
		const float DistSq = To.SizeSquared();
		if (DistSq < BestDistSq && FVector::DotProduct(To.GetSafeNormal(), Dir) >= CosCone)
		{
			Best = Hostile;
			BestDistSq = DistSq;
		}
	}
	HomingTarget = Best;
}

bool ATDProjectile::SweepPath(const FVector& Start, const FVector& End, FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	if (!World || Start.Equals(End, 0.01f))
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDProjectileSweep), false, this);
	if (AActor* S = Shooter.Get())
	{
		Params.AddIgnoredActor(S);
	}
	if (AActor* I = IgnoreActor.Get())
	{
		Params.AddIgnoredActor(I);
	}
	const FCollisionShape Shape = FCollisionShape::MakeSphere(FMath::Max(Spec.CollisionRadius, 1.f));

	// Friendlies block the channel too (they're ships), so skip them and sweep again. Bounded retries.
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		if (!World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, TDCollision::Projectile, Shape, Params))
		{
			return false;
		}
		AActor* HitActor = OutHit.GetActor();
		if (!HitActor)
		{
			return true; // BSP / world geometry
		}
		const bool bFriendly = Team != ETDTeam::Neutral && UTDStatics::GetTeam(HitActor) == Team;
		const bool bOtherProjectile = HitActor->IsA<ATDProjectile>();
		const bool bDeadShip = !IsLiveTarget(HitActor);
		if (!bFriendly && !bOtherProjectile && !bDeadShip)
		{
			return true;
		}
		Params.AddIgnoredActor(HitActor);
	}
	return false;
}

bool ATDProjectile::CanDamage(const AActor* Victim) const
{
	return IsValid(Victim) && Victim != this && UTDStatics::GetTeam(Victim) != Team;
}

void ATDProjectile::HandleImpact(const FHitResult& Hit)
{
	AActor* Victim = Hit.GetActor();
	const FVector Here = GetActorLocation();
	const FVector Dir = Velocity.GetSafeNormal();

	const UTDHealthComponent* VictimHealth = Victim ? UTDStatics::GetHealth(Victim) : nullptr;
	if (Spec.FX)
	{
		const FTDFXCue& Cue = !VictimHealth ? Spec.FX->ImpactWorld : (VictimHealth->IsShieldUp() ? Spec.FX->ImpactShield : Spec.FX->ImpactHull);
		UTDStatics::PlayCue(this, Cue, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	if (Victim && CanDamage(Victim))
	{
		DealDamage(Victim, Spec.Damage, Hit.ImpactPoint);
		ApplyKnockback(Victim);
	}
	if (Spec.SplashRadius > 0.f)
	{
		ApplySplash(Here, Victim);
	}
	if (Spec.ShardCount > 0)
	{
		SpawnShards(Here, Dir, Victim);
	}
	if (Spec.SplashRadius > 0.f && Spec.FX)
	{
		UTDStatics::PlayCue(this, Spec.FX->Detonation, Here, Dir.Rotation());
	}
	OnDetonated.Broadcast(this, Victim);
	Deactivate();
}

void ATDProjectile::DealDamage(AActor* Victim, float Amount, const FVector& FromLocation)
{
	UTDHealthComponent* Health = UTDStatics::GetHealth(Victim);
	const bool bWasAlive = Health && Health->IsAlive();
	const float Dealt = UTDStatics::ApplyTDDamage(Victim, Amount * DamageMultiplier, Spec.DamageKind, Shooter.Get(), this, FromLocation,
		Velocity.GetSafeNormal());

	if (bReportHits && Dealt > 0.f)
	{
		const bool bKill = bWasAlive && Health && Health->IsDead();
		if (UTDEventBus* Bus = UTDEventBus::Get(this))
		{
			Bus->BroadcastHitConfirmed(Victim, bKill);
		}
		if (bKill)
		{
			if (UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this))
			{
				Feel->RequestHitStop();
			}
		}
	}
}

void ATDProjectile::ApplySplash(const FVector& Center, AActor* Exclude)
{
	const float SplashAmount = Spec.GetSplashDamage();
	if (SplashAmount > 0.f)
	{
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this))
		{
			// Copy: damage can kill and unregister actors while we iterate.
			TArray<AActor*, TInlineAllocator<32>> Victims;
			{
				TArray<AActor*>& Hostiles = HostileScratch();
				Hostiles.Reset();
				Registry->GetHostilesOf(Team, Hostiles, true);
				for (AActor* Hostile : Hostiles)
				{
					if (Hostile && Hostile != Exclude && DistanceToBounds(Hostile, Center) <= Spec.SplashRadius)
					{
						Victims.Add(Hostile);
					}
				}
			}
			for (AActor* V : Victims)
			{
				if (IsValid(V))
				{
					DealDamage(V, SplashAmount, Center);
				}
			}
		}
	}

	if (Spec.bShootsDownMissiles)
	{
		ShootDownNearbyMissiles(Spec.SplashRadius);
	}
}

bool ATDProjectile::ShootDownNearbyMissiles(float Radius)
{
	UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this);
	if (!Subsystem || Radius <= 0.f)
	{
		return false;
	}
	TArray<ATDProjectile*>& Missiles = MissileScratch();
	Missiles.Reset();
	Subsystem->FindHostileMissiles(Team, GetActorLocation(), Radius, Missiles);
	// Kill() releases into the pool and edits the live list, so work from a copy.
	TArray<ATDProjectile*, TInlineAllocator<16>> ToKill(Missiles);
	for (ATDProjectile* M : ToKill)
	{
		if (M != this && IsValid(M))
		{
			M->Kill(true);
		}
	}
	return ToKill.Num() > 0;
}

void ATDProjectile::SpawnShards(const FVector& Origin, const FVector& Direction, AActor* Ignore)
{
	UWorld* World = GetWorld();
	if (!World || Spec.ShardCount <= 0)
	{
		return;
	}
	FTDProjectileSpec ShardSpec = Spec;
	ShardSpec.Damage = Spec.ShardDamage;
	ShardSpec.ShardCount = 0;
	ShardSpec.SplashRadius = 0.f;
	ShardSpec.Speed = Spec.ShardSpeed;
	ShardSpec.Range = Spec.ShardRange;
	ShardSpec.LifetimeOverride = 0.f;
	ShardSpec.bDetonateOnExpire = false;
	ShardSpec.bInheritShooterVelocity = false;
	ShardSpec.CollisionRadius = FMath::Max(Spec.CollisionRadius * 0.6f, 20.f);
	ShardSpec.PlaceholderScale = Spec.PlaceholderScale * 0.5f;

	const FVector Fwd = Direction.IsNearlyZero() ? GetActorForwardVector() : Direction.GetSafeNormal();
	FVector Up, Right;
	Fwd.FindBestAxisVectors(Up, Right);
	const float Cone = FMath::DegreesToRadians(Spec.ShardConeDeg);

	// Even ring around the flight direction: predictable, readable spread.
	for (int32 i = 0; i < Spec.ShardCount; ++i)
	{
		const float Around = (2.f * PI * i) / Spec.ShardCount;
		const FVector Dir = (Fwd * FMath::Cos(Cone) + (Up * FMath::Cos(Around) + Right * FMath::Sin(Around)) * FMath::Sin(Cone)).GetSafeNormal();
		ATDProjectile* Shard = Launch(World, ShardSpec, FTransform(Dir.Rotation(), Origin), Shooter.Get(), Team, nullptr, FVector::ZeroVector,
			DamageMultiplier, bReportHits);
		if (Shard)
		{
			Shard->IgnoreActor = Ignore;
			Shard->OriginalShooter = OriginalShooter;
		}
	}
}

void ATDProjectile::ApplyKnockback(AActor* Victim) const
{
	if (Spec.KnockbackSpeed <= 0.f || !IsValid(Victim))
	{
		return;
	}
	FVector Center;
	if (BoundsRadius(Victim, Center) > Spec.KnockbackMaxTargetRadius)
	{
		return;
	}
	const FVector Push = Velocity.GetSafeNormal() * Spec.KnockbackSpeed;
	if (UMovementComponent* Move = Victim->FindComponentByClass<UMovementComponent>())
	{
		Move->Velocity += Push;
		return;
	}
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Victim->GetRootComponent()))
	{
		if (Prim->IsSimulatingPhysics())
		{
			Prim->AddImpulse(Push, NAME_None, true);
		}
	}
}
