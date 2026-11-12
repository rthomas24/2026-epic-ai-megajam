// Triad Drift - Combat (Teammate 2). Swept, pooled projectile.

#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDShieldDome.h"
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
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

namespace
{
	// Scratch arrays reused across projectiles (game thread only) so hot paths never allocate.
	TArray<AActor*>& TDProjHostileScratch()
	{
		static TArray<AActor*> Scratch;
		return Scratch;
	}
	TArray<ATDProjectile*>& TDProjMissileScratch()
	{
		static TArray<ATDProjectile*> Scratch;
		return Scratch;
	}

	/** Conservative radius of an actor's bounds (largest half-extent). */
	float TDProjBoundsRadius(const AActor* Actor, FVector& OutCenter)
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

	float TDProjDistanceToBounds(const AActor* Actor, const FVector& Point)
	{
		FVector Origin, Extent;
		Actor->GetActorBounds(true, Origin, Extent);
		if (Extent.IsNearlyZero())
		{
			return FVector::Dist(Actor->GetActorLocation(), Point);
		}
		return FMath::Sqrt(FBox(Origin - Extent, Origin + Extent).ComputeSquaredDistanceToPoint(Point));
	}

	bool TDProjIsLiveTarget(const AActor* Actor)
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
		DefaultMesh = SphereMesh.Object;
		PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		DefaultMaterial = ShapeMaterial.Object;
	}
	PlaceholderHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderHead"));
	PlaceholderHead->SetupAttachment(Root);
	PlaceholderHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderHead->SetGenerateOverlapEvents(false);
	PlaceholderHead->SetCastShadow(false);
	PlaceholderHead->SetCanEverAffectNavigation(false);
	PlaceholderHead->bReceivesDecals = false;
	if (SphereMesh.Succeeded())
	{
		PlaceholderHead->SetStaticMesh(SphereMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (GlowFinder.Succeeded())
	{
		GlowMaterial = GlowFinder.Object;
	}

	// The actor stays visible (so a finishing tracer ribbon isn't cut off); the meshes are what hide.
	PlaceholderMesh->SetVisibility(false);
	PlaceholderHead->SetVisibility(false);
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
	static uint32 NextLaunchSerial = 1;
	LaunchSerial = NextLaunchSerial++;
	LastTransitTime = -1.0;
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
	PreviousLocation = Transform.GetLocation();
	bActive = true;
	bTrackingAbandoned = false;
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
	// Mesh: Nick's projectile mesh with its own materials, else the engine sphere tinted in squad / enemy colour.
	UStaticMesh* ArtMesh = (Spec.FX && !Spec.FX->ProjectileMesh.IsNull()) ? Spec.FX->ProjectileMesh.LoadSynchronous() : nullptr;
	UStaticMesh* WantMesh = ArtMesh ? ArtMesh : DefaultMesh.Get();
	if (WantMesh && PlaceholderMesh->GetStaticMesh() != WantMesh)
	{
		PlaceholderMesh->SetStaticMesh(WantMesh);
	}
	if (ArtMesh)
	{
		StreakLook = FTDStreakLook();
		PlaceholderMesh->EmptyOverrideMaterials();
		PlaceholderMesh->SetRelativeScale3D(FVector::OneVector);
		// Visual-only offset (centre-pivoted meshes, e.g. a rocket whose tail would clip into the pod).
		// Collision is a sweep from the actor origin, so gameplay never moves.
		PlaceholderMesh->SetRelativeLocation(Spec.FX->ProjectileMeshOffset);
	}
	else
	{
		// A glowing streak trailing the shot (its head at the actor origin, where the sweep is), sized from
		// the speed so it reads at gameplay distance. One material instance per pooled actor, re-coloured.
		const FLinearColor Color = (Spec.FX && !Spec.FX->Color.Equals(FLinearColor::White)) ? Spec.FX->Color : Spec.PlaceholderColor;
		const FTDStreakLook Look = FTDCombatPlaceholders::ComputeStreak(Spec);
		StreakLook = Look;
		UMaterialInterface* Base = GlowMaterial ? GlowMaterial.Get() : DefaultMaterial.Get();
		const FLinearColor Final = GlowMaterial ? Color * FTDCombatPlaceholders::StreakGlow : Color;
		if (!PlaceholderMID && Base)
		{
			PlaceholderMID = UMaterialInstanceDynamic::Create(Base, this);
		}
		if (PlaceholderMID)
		{
			if (PlaceholderMesh->GetMaterial(0) != PlaceholderMID)
			{
				PlaceholderMesh->SetMaterial(0, PlaceholderMID);
			}
			PlaceholderMID->SetVectorParameterValue(TEXT("Color"), Final);
			PlaceholderMID->SetVectorParameterValue(TEXT("EmissiveColor"), Final);
		}
		if (Look.bStreak)
		{
			PlaceholderMesh->SetRelativeScale3D(FVector(Look.Length, Look.Thickness, Look.Thickness) / 100.f);
			PlaceholderMesh->SetRelativeLocation(FVector(-0.5f * Look.Length + 0.5f * Look.Thickness, 0.f, 0.f));
		}
		else
		{
			PlaceholderMesh->SetRelativeScale3D(Spec.PlaceholderScale);
			PlaceholderMesh->SetRelativeLocation(FVector::ZeroVector);
		}
		if (Look.bHead)
		{
			if (!HeadMID && Base)
			{
				HeadMID = UMaterialInstanceDynamic::Create(Base, this);
				PlaceholderHead->SetMaterial(0, HeadMID);
			}
			if (HeadMID)
			{
				// Hot core: the team colour pushed towards white and brighter than the trail.
				const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.45f) * (GlowMaterial ? FTDCombatPlaceholders::StreakGlow * 1.6f : 1.f);
				HeadMID->SetVectorParameterValue(TEXT("Color"), Hot);
				HeadMID->SetVectorParameterValue(TEXT("EmissiveColor"), Hot);
			}
			PlaceholderHead->SetRelativeScale3D(FVector(Look.HeadDiameter / 100.f));
		}
	}

	// Tracer: one persistent Niagara component per pooled actor.
	UNiagaraSystem* Tracer = (Spec.FX && !Spec.FX->Tracer.IsNull()) ? Spec.FX->Tracer.LoadSynchronous() : nullptr;
	if (Tracer)
	{
		if (!TracerComp)
		{
			TracerComp = NewObject<UNiagaraComponent>(this, TEXT("Tracer"));
			TracerComp->SetAutoActivate(false);
			TracerComp->SetupAttachment(Root);
			TracerComp->RegisterComponent();
		}
		if (TracerComp->GetAsset() != Tracer)
		{
			TracerComp->SetAsset(Tracer);
		}
		TracerComp->Activate(true);
	}
	else if (TracerComp)
	{
		TracerComp->DeactivateImmediate();
	}

	// With a tracer and no art mesh, the trail is the look; otherwise show the mesh. The head is placeholder-only.
	PlaceholderMesh->SetVisibility(ArtMesh != nullptr || Tracer == nullptr);
	PlaceholderHead->SetVisibility(!ArtMesh && !Tracer && FTDCombatPlaceholders::ComputeStreak(Spec).bHead);
}

void ATDProjectile::Deactivate()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	SetActorTickEnabled(false);
	PlaceholderMesh->SetVisibility(false);
	PlaceholderHead->SetVisibility(false);
	Velocity = FVector::ZeroVector;
	HomingTarget = nullptr;
	Shooter = nullptr;
	OriginalShooter = nullptr;
	IgnoreActor = nullptr;
	OnDetonated.Clear();
	if (TracerComp)
	{
		TracerComp->Deactivate(); // let the ribbon finish
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
	if (TDProjIsLiveTarget(Original) && UTDStatics::AreTeamsHostile(NewTeam, UTDStatics::GetTeam(Original)))
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
	bTrackingAbandoned = false;

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
	if (bExplodeFX)
	{
		PlayPlaceholderImpact(GetActorLocation(), Spec.FX && FTDCombatPlaceholders::HasVisualArt(Spec.FX->Detonation));
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
	PlayPlaceholderImpact(Here, Spec.FX && FTDCombatPlaceholders::HasVisualArt(Spec.FX->Detonation));
	OnDetonated.Broadcast(this, nullptr);
	Deactivate();
}

void ATDProjectile::SetHomingTarget(AActor* NewTarget)
{
	HomingTarget = NewTarget;
	bTrackingAbandoned = false;
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
		bTrackingAbandoned = true;
	}
}

void ATDProjectile::TeleportProjectile(const FVector& NewLocation, const FVector& NewDirection)
{
	if (!NewDirection.IsNearlyZero())
	{
		Velocity = NewDirection.GetSafeNormal() * Velocity.Size();
	}
	SetActorLocationAndRotation(NewLocation, Velocity.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	PreviousLocation = NewLocation; // a jump is not motion: nothing should see it as crossing a plane
}

// ------------------------------------------------------------------------------------------ simulation

void ATDProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Step(DeltaSeconds);
	UpdatePlaceholderDistanceScale();
}

void ATDProjectile::UpdatePlaceholderDistanceScale()
{
	if (!bActive || !StreakLook.bStreak || !PlaceholderMesh->IsVisible())
	{
		return;
	}
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const float Distance = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation());
	const float Grow = FTDCombatPlaceholders::DistanceGrowth(StreakLook, Distance);
	const float Thickness = StreakLook.Thickness * Grow;
	PlaceholderMesh->SetRelativeScale3D(FVector(StreakLook.Length, Thickness, Thickness) / 100.f);
	PlaceholderMesh->SetRelativeLocation(FVector(-0.5f * StreakLook.Length + 0.5f * Thickness, 0.f, 0.f));
	if (StreakLook.bHead)
	{
		PlaceholderHead->SetRelativeScale3D(FVector(StreakLook.HeadDiameter * Grow / 100.f));
	}
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
	PreviousLocation = Start;
	FVector End = Start + Velocity * DeltaSeconds;
	const bool bExpiring = Age >= Lifetime;
	if (bExpiring)
	{
		// Only fly the part of this step that was still inside the lifetime.
		const float Over = Age - Lifetime;
		End = Start + Velocity * FMath::Max(0.f, DeltaSeconds - Over);
	}

	// Hostile shield domes: stop exactly on the shell if this step enters one (a fast shot can't skip
	// through the shell and hit a ship inside in the same frame).
	if (UTDPowerSubsystem* Powers = UTDPowerSubsystem::Get(this))
	{
		float BestT = 2.f;
		ATDShieldDome* HitDome = nullptr;
		for (ATDShieldDome* Dome : Powers->GetActiveDomes())
		{
			if (!IsValid(Dome) || !Dome->Intercepts(this))
			{
				continue;
			}
			const FVector C = Dome->GetActorLocation();
			const float R = Dome->GetRadius();
			if (FVector::DistSquared(Start, C) <= R * R)
			{
				continue; // already inside: the dome's own tick handles it
			}
			// First entry of the segment into the sphere.
			const FVector D = End - Start;
			const double A = FVector::DotProduct(D, D);
			const double B = 2.0 * FVector::DotProduct(Start - C, D);
			const double Cc = FVector::DotProduct(Start - C, Start - C) - (double)R * R;
			const double Disc = B * B - 4.0 * A * Cc;
			if (A > KINDA_SMALL_NUMBER && Disc >= 0.0)
			{
				const double T = (-B - FMath::Sqrt(Disc)) / (2.0 * A);
				if (T >= 0.0 && T <= 1.0 && T < BestT)
				{
					BestT = (float)T;
					HitDome = Dome;
				}
			}
		}
		if (HitDome)
		{
			const FVector ShellPoint = Start + (End - Start) * BestT;
			// A ship (or rock) before the shell still takes the hit first.
			FHitResult Hit;
			if (SweepPath(Start, ShellPoint, Hit))
			{
				SetActorLocation(Hit.Location);
				HandleImpact(Hit);
				return;
			}
			SetActorLocation(ShellPoint);
			HitDome->ReceiveProjectile(this, ShellPoint);
			return;
		}
	}

	// Point defense style: knock hostile missiles out of the air along the path.
	if (Spec.bShootsDownMissiles && Spec.ProximityFuseRadius <= 0.f)
	{
		UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this);
		if (Subsystem)
		{
			TArray<ATDProjectile*>& Missiles = TDProjMissileScratch();
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
			TArray<AActor*>& Hostiles = TDProjHostileScratch();
			Hostiles.Reset();
			Registry->GetHostilesOf(Team, Hostiles, true);
			for (AActor* Hostile : Hostiles)
			{
				if (Hostile && Hostile != IgnoreActor.Get())
				{
					// Distance to the actor's own colliding box (not a bounding sphere, not child actors): a carrier's
					// bays and turrets fuse where they are, not 2000 cm out on the carrier's bounding sphere.
					FVector Origin, Extent;
					Hostile->GetActorBounds(true, Origin, Extent, false);
					if (Extent.IsNearlyZero())
					{
						ConsiderPoint(Hostile->GetActorLocation(), 50.f);
						continue;
					}
					const FBox Box(Origin - Extent, Origin + Extent);
					// Closest approach of the segment to the box: sample the segment's closest point to the centre,
					// then refine once against the box surface.
					FVector OnSeg = FMath::ClosestPointOnSegment(Origin, Start, End);
					const FVector OnBox = Box.GetClosestPointTo(OnSeg);
					OnSeg = FMath::ClosestPointOnSegment(OnBox, Start, End);
					ConsiderPoint(Box.GetClosestPointTo(OnSeg), 0.f);
				}
			}
		}
		if (Spec.bShootsDownMissiles)
		{
			if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
			{
				TArray<ATDProjectile*>& Missiles = TDProjMissileScratch();
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
	if (!TDProjIsLiveTarget(Target))
	{
		HomingTarget = nullptr;
		if (!bTrackingAbandoned && (DeflectCount == 0 || bReacquireAfterBlock))
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
	TArray<AActor*>& Hostiles = TDProjHostileScratch();
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
		const bool bDeadShip = !TDProjIsLiveTarget(HitActor);
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
	bool bImpactArt = false;
	if (Spec.FX)
	{
		const FTDFXCue& Cue = !VictimHealth ? Spec.FX->ImpactWorld : (VictimHealth->IsShieldUp() ? Spec.FX->ImpactShield : Spec.FX->ImpactHull);
		UTDStatics::PlayCue(this, Cue, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		bImpactArt = FTDCombatPlaceholders::HasVisualArt(Cue) || (Spec.SplashRadius > 0.f && FTDCombatPlaceholders::HasVisualArt(Spec.FX->Detonation));
	}
	PlayPlaceholderImpact(Hit.ImpactPoint, bImpactArt);

	if (Victim && CanDamage(Victim))
	{
		DealDamage(Victim, Spec.Damage, Hit.ImpactPoint, Dir);
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

void ATDProjectile::PlayPlaceholderImpact(const FVector& Location, bool bCueHasArt) const
{
	if (bCueHasArt)
	{
		return;
	}
	if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
	{
		const FLinearColor Color = (Spec.FX && !Spec.FX->Color.Equals(FLinearColor::White)) ? Spec.FX->Color : Spec.PlaceholderColor;
		const float Seconds = Spec.SplashRadius > 0.f ? 0.3f : 0.14f;
		Subsystem->PlayPlaceholderFlash(Location, FTDCombatPlaceholders::ImpactFlashRadius(Spec), FMath::Lerp(Color, FLinearColor::White, 0.3f), Seconds);
	}
}

void ATDProjectile::DealDamage(AActor* Victim, float Amount, const FVector& FromLocation, const FVector& Direction)
{
	UTDHealthComponent* Health = UTDStatics::GetHealth(Victim);
	const bool bWasAlive = Health && Health->IsAlive();
	const float Dealt = UTDStatics::ApplyTDDamage(Victim, Amount * DamageMultiplier, Spec.DamageKind, Shooter.Get(), this, FromLocation,
		Direction);

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
				TArray<AActor*>& Hostiles = TDProjHostileScratch();
				Hostiles.Reset();
				Registry->GetHostilesOf(Team, Hostiles, true);
				for (AActor* Hostile : Hostiles)
				{
					if (Hostile && Hostile != Exclude && TDProjDistanceToBounds(Hostile, Center) <= Spec.SplashRadius)
					{
						Victims.Add(Hostile);
					}
				}
			}
			for (AActor* V : Victims)
			{
				if (IsValid(V))
				{
					// Blast pushes outward from its centre: the gunship's frontal shield judges by this, not by the shell's heading.
					const FVector Out = (V->GetActorLocation() - Center).GetSafeNormal();
					DealDamage(V, SplashAmount, Center, Out.IsNearlyZero() ? Velocity.GetSafeNormal() : Out);
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
	TArray<ATDProjectile*>& Missiles = TDProjMissileScratch();
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
	if (TDProjBoundsRadius(Victim, Center) > Spec.KnockbackMaxTargetRadius)
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
