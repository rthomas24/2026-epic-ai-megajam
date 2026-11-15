// Triad Drift - Combat (Teammate 2). Swept, pooled projectile.

#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDAccuracyStats.h"
#include "TriadDrift/Flight/TDShip.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Combat/TDAimAssist.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Combat/TDCombatPerf.h"
#include "TriadDrift/Combat/TDCombatCueLog.h"
#include "TriadDrift/Combat/TDAudioHitTrace.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Powers/TDPowerSubsystem.h"
#include "TriadDrift/Powers/TDShieldDome.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
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
	TD_COMBAT_PERF(ProjectileLaunch);
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
	LaunchWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
	Spec = InSpec;
	Team = InTeam;
	Shooter = InInstigator;
	OriginalShooter = InInstigator;
	HomingTarget = InHomingTarget;
	bWarnedFlown = false;
	bReachedFlown = false;
	NoteHomingOnFlown();
	IgnoreActor = nullptr;
	SetInstigator(Cast<APawn>(InInstigator));
	DamageMultiplier = FMath::Max(InDamageMultiplier, 0.f);
	bReportHits = bInReportHits;
	DeflectCount = 0;
	TrackingBlockedFor = 0.f;
	bReacquireAfterBlock = false;
	Age = 0.f;
	const FVector Dir = Transform.GetRotation().GetForwardVector();
	Velocity = Dir * Spec.Speed;
	if (Spec.bInheritShooterVelocity)
	{
		Velocity += Dir * FMath::Max(0.f, FVector::DotProduct(InheritedVelocity, Dir));
	}
	// Range is measured in the shooter's frame (Range / Speed): inherited speed carries the shot further in the
	// world, which keeps chasing enemies able to hit a fleeing player (TriadDrift.Enemies.Play.WaveEngagesAFleeingPlayer).
	Lifetime = FMath::Max(Spec.GetLifetime(), 0.05f);

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
	StatShip = 255;
	StatSlot = 0;
	StatRoot = 0;
	AcquireAssistTarget();

	UpdatePlaceholderLook();
}

namespace
{
	/** The local player is on foot (first-person camera), or with no player pawn, a character (crew) fired it. */
	bool TDProjIsOnFootShot(const UWorld* World, const AActor* Shooter)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (const APawn* Pawn = PC ? PC->GetPawn() : nullptr)
		{
			return !Pawn->IsA<ATDSpacecraft>();
		}
		return Shooter && Shooter->IsA<ACharacter>();
	}
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
		// Colour code: squad shots in ship colour, enemy shots Armada red-orange, deflected shots in the deflector's
		// colour. On foot (first-person camera close to the shot) bolts use the small bolt look, not the space streak.
		const FLinearColor Color = FTDCombatPlaceholders::ProjectileColor(Spec, Team, Shooter.Get(), DeflectCount > 0);
		LookColor = Color;
		bOnFootLook = TDProjIsOnFootShot(GetWorld(), Shooter.Get());
		const FTDStreakLook Look = bOnFootLook ? FTDCombatPlaceholders::ComputeOnFootStreak(Spec)
			: Team == ETDTeam::Player ? FTDCombatPlaceholders::ComputeSquadStreak(Spec) : FTDCombatPlaceholders::ComputeStreak(Spec);
		StreakLook = Look;
		bTracerTip = !bOnFootLook && Team == ETDTeam::Player && Look.bStreak && !Look.bHead;
		AppliedLength = 0.f;
		AppliedGrowth = 1.f;
		bAppliedIncoming = false;
		if (!GlowMaterial)
		{
			GlowMaterial = UTDStatics::GetPlaceholderGlowMaterial(); // project M_TD_PlaceholderGlow, always cooked
		}
		UMaterialInterface* Base = GlowMaterial ? GlowMaterial.Get() : DefaultMaterial.Get();
		const float GlowScale = bOnFootLook ? FTDCombatPlaceholders::OnFootGlow : FTDCombatPlaceholders::StreakGlow;
		const FLinearColor Final = GlowMaterial ? Color * GlowScale : Color;
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
			ApplyStreakTransform(1.f, false);
		}
		else
		{
			PlaceholderMesh->SetRelativeScale3D(Spec.PlaceholderScale);
			PlaceholderMesh->SetRelativeLocation(FVector::ZeroVector);
		}
		if (Look.bHead || Team == ETDTeam::Enemy || bTracerTip)
		{
			if (!HeadMID && Base)
			{
				HeadMID = UMaterialInstanceDynamic::Create(Base, this);
				PlaceholderHead->SetMaterial(0, HeadMID);
			}
			if (HeadMID)
			{
				// Hot core: the team colour pushed towards white and brighter than the trail.
				// Stays in squad / enemy colour (playtest: 0.45 white read as a white ball); incoming enemy fire burns hotter.
				// The squad's heavy heads glow softer (stacked over the reticle they bloomed into a pale ball); the squad
				// tracer tip is near-white so the shot never vanishes against a sky of its own colour.
				const float HeadGlow = bOnFootLook ? 1.f : Team == ETDTeam::Enemy ? FTDCombatPlaceholders::EnemyHeadGlow : bTracerTip ? 0.9f : 0.6f;
				// Enemy and on-foot heads stay pure team colour: any white tips red-orange to pale pink (live, both).
				const float White = (bOnFootLook || Team == ETDTeam::Enemy) ? 0.f : bTracerTip ? FTDCombatPlaceholders::TipWhite : 0.15f;
				const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, White) * (GlowMaterial ? GlowScale * HeadGlow : 1.f);
				HeadMID->SetVectorParameterValue(TEXT("Color"), Hot);
				HeadMID->SetVectorParameterValue(TEXT("EmissiveColor"), Hot);
			}
			if (!bTracerTip)
			{
				PlaceholderHead->SetRelativeScale3D(FVector(Look.HeadDiameter / 100.f));
				PlaceholderHead->SetRelativeLocation(FVector::ZeroVector);
			}
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
	PlaceholderHead->SetVisibility(!ArtMesh && !Tracer && (StreakLook.bHead || bTracerTip));
}

void ATDProjectile::Deactivate()
{
	if (!bActive)
	{
		return;
	}
	NoteMissileEnded();
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
	bAssistOn = false;
	StatShip = 255;
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
		FTDCombatCueLog::Play(this, Spec.FX->Deflected, ETDCombatCueKind::Deflected, Spec.FX, GetActorLocation(), Dir.Rotation());
	}
	UpdatePlaceholderLook(); // re-tint in the deflector's colour
}

void ATDProjectile::Kill(bool bExplodeFX)
{
	if (!bActive)
	{
		return;
	}
	if (bExplodeFX && Spec.FX)
	{
		FTDCombatCueLog::Play(this, Spec.FX->Detonation, ETDCombatCueKind::Detonation, Spec.FX, GetActorLocation(), GetActorRotation());
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
		if (bWarnedFlown)
		{
			const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
			const APawn* Flown = PC ? PC->GetPawn() : nullptr;
			bReachedFlown |= Flown && FVector::Dist(Flown->GetActorLocation(), Here) <= Spec.SplashRadius;
		}
		ApplySplash(Here, nullptr);
	}
	if (Spec.ShardCount > 0)
	{
		SpawnShards(Here, Velocity.GetSafeNormal(), IgnoreActor.Get());
	}
	if (Spec.FX)
	{
		FTDCombatCueLog::Play(this, Spec.FX->Detonation, ETDCombatCueKind::Detonation, Spec.FX, Here, GetActorRotation());
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
	NoteHomingOnFlown();
}

void ATDProjectile::NoteHomingOnFlown()
{
	if (bWarnedFlown || !Spec.bIsMissile || !Spec.bHoming || !UTDStatics::AreTeamsHostile(Team, ETDTeam::Player))
	{
		return;
	}
	const AActor* Target = HomingTarget.Get();
	if (Target && FTDCombatBarks::IsPlayerFlown(Target))
	{
		bWarnedFlown = true;
		FTDCombatBarks::SayAsWingman(this, TDBarkIdsT2::MissileIncoming, ETDBarkPriority::Critical);
	}
}

void ATDProjectile::NoteMissileEnded()
{
	if (!bWarnedFlown || bReachedFlown)
	{
		return;
	}
	bWarnedFlown = false;
	const UWorld* World = GetWorld();
	const APlayerController* PC = World && !World->bIsTearingDown ? World->GetFirstPlayerController() : nullptr;
	const AActor* Flown = PC ? PC->GetPawn() : nullptr;
	const UTDHealthComponent* H = Flown ? UTDStatics::GetHealth(Flown) : nullptr;
	if (Flown && H && H->IsAlive() && FTDCombatBarks::IsPlayerFlown(Flown))
	{
		FTDCombatBarks::SayAsPilot(Flown, TDBarkIdsT2::MissileEvaded, ETDBarkPriority::Flavour);
	}
}

void ATDProjectile::NoteShieldedHit(AActor* Victim) const
{
	// dialogs.md Bark_TargetShielded: the player keeps shooting into a gunship's raised frontal shield. Three blocked hits
	// on the same ship within 2 s ask once; that ship can't ask again until its shield has gone down (or 20 s pass).
	const ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Victim);
	if (!Enemy || !Enemy->HasFrontalShield() || !FTDCombatBarks::IsPlayerFlown(Shooter.Get()))
	{
		return;
	}
	struct FState { TWeakObjectPtr<const AActor> Ship; int32 Hits = 0; double Last = -1e9; double Said = -1e9; };
	static FState S;
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (!Enemy->IsFrontalShieldUp())
	{
		if (S.Ship == Enemy)
		{
			S = FState(); // broken: the next raised shield may be called again
		}
		return;
	}
	const FVector Into = Velocity.GetSafeNormal();
	const float Cos = FMath::Cos(FMath::DegreesToRadians(Enemy->GetSpec().FrontalShieldHalfAngleDeg));
	if (FVector::DotProduct(Enemy->GetActorForwardVector(), -Into) < Cos)
	{
		return; // from the side or behind: the shield doesn't block it
	}
	if (S.Ship != Enemy || Now - S.Last > 2.0)
	{
		const double Said = S.Ship == Enemy ? S.Said : -1e9;
		S = FState();
		S.Ship = Enemy;
		S.Said = Said;
	}
	S.Last = Now;
	if (++S.Hits == 3 && Now - S.Said > 20.0)
	{
		S.Said = Now;
		FTDCombatBarks::SayAsWingman(this, TDBarkIdsT2::TargetShielded, ETDBarkPriority::Normal, static_cast<int32>(ETDShipId::Lancer));
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
	TD_COMBAT_PERF(Projectiles);
	Super::Tick(DeltaSeconds);
	// Launched earlier in this same frame: the weapon placed it (and stepped its sub-frame age) already.
	const UWorld* World = GetWorld();
	if (!(World && World->GetTimeSeconds() == LaunchWorldTime))
	{
		Step(DeltaSeconds);
	}
	UpdatePlaceholderDistanceScale();
}

void ATDProjectile::UpdatePlaceholderDistanceScale()
{
	if (!bActive || !StreakLook.bStreak || !PlaceholderMesh->IsVisible())
	{
		return;
	}
	float Grow = AppliedGrowth;
	bool bIncoming = bAppliedIncoming;
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (PC && PC->PlayerCameraManager)
	{
		const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
		const float Distance = FVector::Dist(Camera, GetActorLocation());
		CameraDistance = Distance;
		bIncoming = !bOnFootLook && Team == ETDTeam::Enemy && FTDCombatPlaceholders::IsIncoming(Velocity, GetActorLocation(), Camera);
		Grow = bOnFootLook ? FTDCombatPlaceholders::OnFootDistanceGrowth(StreakLook, Distance) : bIncoming
			? FTDCombatPlaceholders::DistanceGrowth(StreakLook, Distance, FTDCombatPlaceholders::MinIncomingStreakPixels)
			: FTDCombatPlaceholders::DistanceGrowth(StreakLook, Distance);
	}
	// A streak still growing out of the muzzle is updated every tick; after that only visible changes are.
	const bool bGrowing = AppliedLength < StreakLook.Length - 1.f;
	// Close to the camera the head cap changes with every metre: keep updating there.
	const bool bNearCamera = CameraDistance > 0.f && CameraDistance < 5000.f && (StreakLook.bHead || bIncoming);
	if (!bGrowing && !bNearCamera && bIncoming == bAppliedIncoming && FMath::Abs(Grow - AppliedGrowth) < 0.03f * AppliedGrowth)
	{
		return; // cheap at 150+ live shots: no transform update for a change nobody can see
	}
	ApplyStreakTransform(Grow, bIncoming);
}

void ATDProjectile::ApplyStreakTransform(float Grow, bool bIncoming)
{
	AppliedGrowth = Grow;
	bAppliedIncoming = bIncoming;
	const float Thickness = StreakLook.Thickness * Grow;
	// Only the distance flown is drawn: a long tracer never reaches back through the ship or past the camera.
	const float Flown = Age * FMath::Max(Velocity.Size(), Spec.Speed);
	AppliedLength = FTDCombatPlaceholders::VisibleLength(StreakLook, Flown);
	const float Length = FMath::Max(AppliedLength, Thickness);
	PlaceholderMesh->SetRelativeScale3D(FVector(Length, Thickness, Thickness) / 100.f);
	PlaceholderMesh->SetRelativeLocation(FVector(-0.5f * Length + 0.5f * Thickness, 0.f, 0.f));
	if (bTracerTip)
	{
		const float TipThick = Thickness * FTDCombatPlaceholders::TipThickness;
		const float Tip = FMath::Max(FTDCombatPlaceholders::TipLength(StreakLook, AppliedLength) * Grow, TipThick);
		PlaceholderHead->SetVisibility(HeadMID != nullptr);
		PlaceholderHead->SetRelativeScale3D(FVector(Tip, TipThick, TipThick) / 100.f);
		PlaceholderHead->SetRelativeLocation(FVector(-0.5f * Tip + 0.5f * TipThick, 0.f, 0.f));
		return;
	}
	const bool bShowHead = StreakLook.bHead || bIncoming;
	PlaceholderHead->SetVisibility(bShowHead && HeadMID != nullptr);
	if (bShowHead)
	{
		float Head = FMath::Max(StreakLook.HeadDiameter, StreakLook.Thickness * (bIncoming ? FTDCombatPlaceholders::IncomingHeadScale : 0.f)) * Grow;
		if (CameraDistance > 0.f)
		{
			Head = FMath::Min(Head, FMath::Max(CameraDistance * FTDCombatPlaceholders::MaxHeadPerCameraDistance, 1.f));
		}
		PlaceholderHead->SetRelativeScale3D(FVector(Head / 100.f));
		PlaceholderHead->SetRelativeLocation(FVector::ZeroVector);
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
	UpdateAssist(DeltaSeconds);

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

	// Open portals: if this step crosses a portal disc, fly only to the plane and transit there (a 30 fps
	// laser step is 1300+ cm and must not hit a ship behind the portal instead).
	if (UTDPowerSubsystem* PortalPowers = UTDPowerSubsystem::Get(this))
	{
		float BestT = 2.f;
		ATDPortal* Crossed = nullptr;
		for (const TWeakObjectPtr<ATDPortal>& Weak : PortalPowers->GetActivePortals())
		{
			float T = 0.f;
			ATDPortal* Portal = Weak.Get();
			if (Portal && Portal->FindDiscCrossing(Start, End, T) && T < BestT)
			{
				BestT = T;
				Crossed = Portal;
			}
		}
		if (Crossed)
		{
			const FVector Dir = (End - Start).GetSafeNormal();
			const FVector OnPlane = Start + (End - Start) * BestT;
			FHitResult Hit;
			if (SweepPath(Start, OnPlane, Hit))
			{
				SetActorLocation(Hit.Location);
				HandleImpact(Hit);
				return;
			}
			const FVector JustPast = OnPlane + Dir * 1.f;
			SetActorLocation(JustPast);
			Crossed->TransitAlong(this, Start, JustPast);
			// Carry the rest of this step on from wherever the shot now is (the exit, or just past the plane if the
			// transit was refused): a 30 fps shot doesn't lose up to 1333 cm of travel at a portal (review).
			const float SegmentSpeed = Velocity.Size();
			const float Remaining = SegmentSpeed > KINDA_SMALL_NUMBER
				? FMath::Max(0.f, (float)FVector::Dist(JustPast, End) / SegmentSpeed) : 0.f;
			if (bActive && Remaining > KINDA_SMALL_NUMBER && PortalCarryDepth < 2)
			{
				++PortalCarryDepth;
				Age -= Remaining; // Step adds it back: no extra ageing
				Step(Remaining);
				--PortalCarryDepth;
			}
			return;
		}
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

void ATDProjectile::AcquireAssistTarget()
{
	bAssistOn = false;
	AssistTarget = nullptr;
	AssistTurnedDeg = 0.f;
	// #192 / #201: the Rift's and the Lancer's regular shots (weapon data with an assist cone) home gently on the player's
	// soft lock. No lock, no homing; wingmen's shots and the Bastion's (no assist data) fly straight.
	if (Spec.AssistConeDeg <= 0.f || Spec.bHoming || !FTDAimAssist::IsEnabled())
	{
		return;
	}
	AActor* Ship = Shooter.Get();
	if (!FTDAimAssist::IsPlayerShip(Ship))
	{
		return;
	}
	const UTDTargetingComponent* Targeting = Ship->FindComponentByClass<UTDTargetingComponent>();
	AActor* Lock = Targeting ? Targeting->GetCurrentTarget() : nullptr;
	const float Speed = Velocity.Size();
	if (!TDProjIsLiveTarget(Lock) || Speed <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector To = Lock->GetActorLocation() - GetActorLocation();
	const float Dist = To.Size();
	const float Range = Spec.AssistRange > 0.f ? Spec.AssistRange : Spec.Range;
	// The lock must be roughly where the shot is going (twice the weapon's cone, scaled): never a U-turn.
	const float Cone = FMath::Min(Spec.AssistConeDeg * 2.f * FMath::Max(FTDAimAssist::Strength(Ship), .5f), 45.f);
	if (Dist <= KINDA_SMALL_NUMBER || Dist > Range || FVector::DotProduct(To / Dist, Velocity / Speed) < FMath::Cos(FMath::DegreesToRadians(Cone)))
	{
		return;
	}
	AssistTurnRateDeg = Spec.AssistTurnRateDeg * FTDAimAssist::HomingTurnScale(Ship);
	AssistMaxTurnDeg = FMath::Min(Spec.AssistMaxTurnDeg, FTDAimAssist::HomingMaxTurnDeg());
	AssistTarget = Lock;
	bAssistOn = AssistTurnRateDeg > 0.f && AssistMaxTurnDeg > 0.f;
}

void ATDProjectile::UpdateAssist(float DeltaSeconds)
{
	if (!bAssistOn)
	{
		return;
	}
	AActor* Target = AssistTarget.Get();
	const UTDHealthComponent* Health = Target ? UTDStatics::GetHealth(Target) : nullptr;
	// Dead, gone, cloaked or turned as far as allowed: straight from here on (no re-acquire, no orbiting).
	if (!TDProjIsLiveTarget(Target) || (Health && !Health->bTargetable) || AssistTurnedDeg >= AssistMaxTurnDeg)
	{
		bAssistOn = false;
		return;
	}
	const float Speed = Velocity.Size();
	if (Speed <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector CurrentDir = Velocity / Speed;
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	if (FVector::DotProduct(ToTarget, CurrentDir) <= 0.f)
	{
		bAssistOn = false; // flew past it: never turn back
		return;
	}
	const float TimeToTarget = ToTarget.Size() / Speed;
	const FVector Predicted = Target->GetActorLocation() + Target->GetVelocity() * FMath::Min(TimeToTarget, 1.f) * .5f;
	const FVector DesiredDir = (Predicted - GetActorLocation()).GetSafeNormal();
	if (DesiredDir.IsNearlyZero())
	{
		return;
	}
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(CurrentDir, DesiredDir), -1.f, 1.f)));
	const float StepDeg = FMath::Min3(AngleDeg, AssistTurnRateDeg * DeltaSeconds, AssistMaxTurnDeg - AssistTurnedDeg);
	if (StepDeg <= KINDA_SMALL_NUMBER || AngleDeg <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FQuat Full = FQuat::FindBetweenNormals(CurrentDir, DesiredDir);
	const FVector NewDir = FQuat::Slerp(FQuat::Identity, Full, StepDeg / AngleDeg).RotateVector(CurrentDir).GetSafeNormal();
	Velocity = NewDir * Speed; // same speed, same look: only the heading bends
	AssistTurnedDeg += StepDeg;
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
	NoteHomingOnFlown();
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
	bReachedFlown |= bWarnedFlown && Victim && FTDCombatBarks::IsPlayerFlown(Victim);
	if (bWarnedFlown && Spec.SplashRadius > 0.f && !bReachedFlown)
	{
		const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const APawn* Flown = PC ? PC->GetPawn() : nullptr;
		bReachedFlown = Flown && FVector::Dist(Flown->GetActorLocation(), Hit.ImpactPoint) <= Spec.SplashRadius;
	}
	bool bImpactArt = false;
	if (VictimHealth && FTDAudioHitTrace::IsOn() && !Victim->Implements<UTDFieldDamageable>() && CanDamage(Victim))
	{
		// Board N-069: mark the real hit for recorded audio, at the cue choice and before the damage.
		const bool bShield = VictimHealth->IsShieldUp();
		FTDAudioHitTrace::Trace(this, Spec.FX ? (bShield ? &Spec.FX->ImpactShield : &Spec.FX->ImpactHull) : nullptr,
			bShield ? ETDCombatCueKind::ImpactShield : ETDCombatCueKind::ImpactHull, Shooter.Get(), Victim, VictimHealth);
	}
	if (Spec.FX)
	{
		const FTDFXCue& Cue = !VictimHealth ? Spec.FX->ImpactWorld : (VictimHealth->IsShieldUp() ? Spec.FX->ImpactShield : Spec.FX->ImpactHull);
		FTDCombatCueLog::Play(this, Cue, !VictimHealth ? ETDCombatCueKind::ImpactWorld : (VictimHealth->IsShieldUp() ? ETDCombatCueKind::ImpactShield : ETDCombatCueKind::ImpactHull), Spec.FX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		bImpactArt = FTDCombatPlaceholders::HasVisualArt(Cue) || (Spec.SplashRadius > 0.f && FTDCombatPlaceholders::HasVisualArt(Spec.FX->Detonation));
	}
	PlayPlaceholderImpact(Hit.ImpactPoint, bImpactArt, VictimHealth, Hit.ImpactNormal);

	if (Victim && Victim->Implements<UTDFieldDamageable>())
	{
		// T6's destructible field: the instance hit, the shot's real damage. Player shots tell the HUD (a quiet marker),
		// never the bus's OnHitConfirmed (no hit / kill sound, no director moment for a rock).
		Cast<ITDFieldDamageable>(Victim)->ReceiveProjectileHit(Hit, Spec.Damage * DamageMultiplier, Shooter.Get(), Spec.DamageKind);
		if (bReportHits)
		{
			FTDFieldDamage::OnPlayerFieldHit.Broadcast(Victim, Hit.ImpactPoint);
		}
	}
	else if (Victim && CanDamage(Victim))
	{
		NoteShieldedHit(Victim); // before the damage: the shield's state as the shot arrived
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
		FTDCombatCueLog::Play(this, Spec.FX->Detonation, ETDCombatCueKind::Detonation, Spec.FX, Here, Dir.Rotation());
	}
	OnDetonated.Broadcast(this, Victim);
	Deactivate();
}

void ATDProjectile::PlayPlaceholderImpact(const FVector& Location, bool bCueHasArt, const UTDHealthComponent* VictimHealth, const FVector& Normal) const
{
	// Something with health, hit by a light shot in space: shield puff vs hull sparks. Explosive shots and on-foot bolts
	// keep their own looks.
	if (!bCueHasArt && VictimHealth && !bOnFootLook && Spec.SplashRadius <= 0.f && !Spec.bIsMissile)
	{
		if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
		{
			Subsystem->PlayPlaceholderHit(Location, FTDCombatPlaceholders::ImpactFlashRadius(Spec), VictimHealth->IsShieldUp(), Normal);
			return;
		}
	}
	PlayPlaceholderImpact(Location, bCueHasArt);
}

void ATDProjectile::PlayPlaceholderImpact(const FVector& Location, bool bCueHasArt) const
{
	if (bCueHasArt)
	{
		return;
	}
	if (UTDProjectileSubsystem* Subsystem = UTDProjectileSubsystem::Get(this))
	{
		const FLinearColor Color = LookColor;
		if (bOnFootLook)
		{
			// On foot the hit is often a metre from the camera (a pellet on the player): a small spark at the on-foot
			// glow, not the 140 cm space pop at glow 14 that hid the Heavy in the live check.
			const float Dim = FTDCombatPlaceholders::OnFootGlow / FTDCombatPlaceholders::FlashGlow;
			Subsystem->PlayPlaceholderFlash(Location, FTDCombatPlaceholders::OnFootImpactRadius(StreakLook), Color * Dim, 0.1f);
			return;
		}
		if (Spec.SplashRadius > 0.f || Spec.bIsMissile)
		{
			// Rockets, missiles, flak and mines: a real explosion read (core, shell, debris), not a pop.
			Subsystem->PlayPlaceholderExplosion(Location, FTDCombatPlaceholders::ImpactFlashRadius(Spec), Color, 6);
		}
		else
		{
			// Near the camera (a hit on a carrier hull a few metres away) a hit flash is capped like an explosion shell.
			if (ATDCombatFlash* Flash = Subsystem->PlayPlaceholderFlash(Location, FTDCombatPlaceholders::ImpactFlashRadius(Spec), FMath::Lerp(Color, FLinearColor::White, 0.3f), 0.14f))
			{
				Flash->SetCameraCap(ATDCombatFlash::MaxShellRadiusPerCameraDistance);
			}
		}
	}
}

void ATDProjectile::DealDamage(AActor* Victim, float Amount, const FVector& FromLocation, const FVector& Direction)
{
	UTDHealthComponent* Health = UTDStatics::GetHealth(Victim);
	const bool bWasAlive = Health && Health->IsAlive();
	const float Dealt = UTDStatics::ApplyTDDamage(Victim, Amount * DamageMultiplier, Spec.DamageKind, Shooter.Get(), this, FromLocation,
		Direction);

	// Only things with health confirm a hit: a rock or a neutral prop never lights the hit marker.
	if (bReportHits && Health && Dealt > 0.f)
	{
		const bool bKill = bWasAlive && Health->IsDead();
		if (StatShip < 3)
		{
			FTDAccuracyStats::NoteHit(StatShip, StatSlot, StatRoot != 0 ? StatRoot : LaunchSerial, bKill);
		}
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
		// T6's destructible field: rubble inside the blast breaks too (each field checks its own pieces against the radius).
		{
			TArray<AActor*> Fields;
			FTDFieldDamage::GetFields(GetWorld(), Fields);
			for (AActor* Field : Fields)
			{
				Cast<ITDFieldDamageable>(Field)->ReceiveSplash(Center, Spec.SplashRadius, SplashAmount * DamageMultiplier, Shooter.Get());
			}
		}
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
	ShardSpec.AssistConeDeg = 0.f; // shards fly straight
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
			Shard->StatShip = StatShip;
			Shard->StatSlot = StatSlot;
			Shard->StatRoot = StatRoot != 0 ? StatRoot : LaunchSerial;
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
