// Triad Drift - on-foot Armada crew (ENG-23). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace TDCrewImpl
{
	static float TargetRadius(const AActor* A)
	{
		FVector Origin, Extent;
		A->GetActorBounds(true, Origin, Extent);
		const float R = FVector2D(Extent.X, Extent.Y).Size();
		return R > 1.f ? FMath::Min(R, 150.f) : 40.f;
	}
}

ATDCrewCharacter::ATDCrewCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Body->SetStaticMesh(Cube.Object);
	}
	Eyes = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eyes"));
	Eyes->SetupAttachment(GetCapsuleComponent());
	Eyes->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Eyes->SetCanEverAffectNavigation(false);
	Eyes->SetCastShadow(false);
	if (Cube.Succeeded())
	{
		Eyes->SetStaticMesh(Cube.Object);
	}
	GetMesh()->SetVisibility(false);

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxShield = 0.f;
	Health->ShieldRegenFractionPerSecond = 0.f;
	Health->DefaultTeam = ETDTeam::Enemy;

	AIControllerClass = ATDCrewAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxAcceleration = 2400.f;
	GetCharacterMovement()->BrakingDecelerationFlying = 2400.f;
	Spec = FTDCrewLibrary::Get(CrewType);
}

void ATDCrewCharacter::InitFromType(ETDCrewType Type)
{
	CrewType = Type;
	Spec = FTDCrewLibrary::Get(Type);
	bSpecApplied = true;

	GetCapsuleComponent()->SetCapsuleSize(Spec.CapsuleRadius, Spec.CapsuleHalfHeight);
	if (Health)
	{
		Health->InitMaxValues(Spec.MaxHealth, 0.f, true);
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = Spec.MoveSpeed;
	Move->MaxFlySpeed = Spec.MoveSpeed;
	if (Spec.bFlying)
	{
		Move->GravityScale = 0.f;
		Move->SetMovementMode(MOVE_Flying);
	}
	// Placeholder body sized from the capsule (engine cube = 100 cm).
	Body->SetRelativeScale3D(Spec.BodyScale);
	// Eye slit on the front face, near the top of the body.
	Eyes->SetRelativeLocation(FVector(Spec.BodyScale.X * 50.f + 1.f, 0.f, Spec.BodyScale.Z * 50.f * 0.6f));
	Eyes->SetRelativeScale3D(FVector(0.02f, FMath::Max(Spec.BodyScale.Y * 0.6f, 0.12f), 0.06f));

	if (!Visuals)
	{
		LoadDefaultVisuals();
	}
	ApplyVisuals();
}

void ATDCrewCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (!bSpecApplied)
	{
		InitFromType(CrewType);
	}
	Health->OnDamaged.AddUniqueDynamic(this, &ATDCrewCharacter::HandleDamaged);
	Health->OnDeath.AddUniqueDynamic(this, &ATDCrewCharacter::HandleDeath);
	Health->DamageFilter.BindUObject(this, &ATDCrewCharacter::FilterDamage);
}

void ATDCrewCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	// Knocked or walked off the map: that's a death (encounters must see it), then clean up.
	if (Health && Health->IsAlive())
	{
		Health->Kill(nullptr, ETDDamageKind::Environmental);
	}
	Destroy();
}

bool ATDCrewCharacter::HasGroundAhead(const FVector& Direction, float Reach) const
{
	if (Spec.bFlying || Direction.IsNearlyZero())
	{
		return true;
	}
	const FVector Ahead = GetActorLocation() + Direction.GetSafeNormal2D() * (Spec.CapsuleRadius + Reach);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDCrewLedge), false, this);
	FHitResult Hit;
	// Floor within a step-down of 1.5 m counts; anything deeper is a ledge.
	return GetWorld()->LineTraceSingleByChannel(Hit, Ahead + FVector(0.f, 0.f, 50.f), Ahead - FVector(0.f, 0.f, Spec.CapsuleHalfHeight + 150.f), ECC_Visibility, Params);
}

void ATDCrewCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<ATDCrewCharacter>& S : Summons)
	{
		if (ATDCrewCharacter* Grunt = S.Get(); Grunt && !Grunt->IsActorBeingDestroyed())
		{
			Grunt->Destroy(); // no orphaned summons after the Warden is gone
		}
	}
	if (UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this))
	{
		Tokens->Release(this);
	}
	if (Health && Health->DamageFilter.IsBoundToObject(this))
	{
		Health->DamageFilter.Unbind();
	}
	Super::EndPlay(EndPlayReason);
}

bool ATDCrewCharacter::IsDead() const
{
	return Health && Health->IsDead();
}

// ------------------------------------------------------------------------------------------------
// Visuals
// ------------------------------------------------------------------------------------------------

void ATDCrewCharacter::LoadDefaultVisuals()
{
	// Core's one loader: quiet when Nick hasn't made the asset yet, cooked in packaged builds.
	Visuals = UTDStatics::LoadPresentation<UTDEnemyVisuals>(FString::Printf(TEXT("DA_Crew_%s_Visuals"), *Spec.VisualsId.ToString()));
}

void ATDCrewCharacter::ApplyVisuals()
{
	if (Visuals && !Visuals->SkeletalMesh.IsNull())
	{
		if (USkeletalMesh* SK = Visuals->SkeletalMesh.LoadSynchronous())
		{
			GetMesh()->SetSkeletalMesh(SK);
			if (!Visuals->AnimBPClass.IsNull())
			{
				if (UClass* AnimClass = Visuals->AnimBPClass.LoadSynchronous())
				{
					GetMesh()->SetAnimInstanceClass(AnimClass);
				}
			}
			GetMesh()->SetVisibility(true);
			GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -Spec.CapsuleHalfHeight));
			Body->SetVisibility(false);
			Eyes->SetVisibility(false);
			return;
		}
	}
	if (Visuals && !Visuals->Mesh.IsNull())
	{
		if (UStaticMesh* SM = Visuals->Mesh.LoadSynchronous())
		{
			Body->SetStaticMesh(SM);
			Body->SetRelativeScale3D(FVector::OneVector);
			for (int32 i = 0; i < Visuals->MaterialOverrides.Num(); ++i)
			{
				if (UMaterialInterface* Mat = Visuals->MaterialOverrides[i].LoadSynchronous())
				{
					Body->SetMaterial(i, Mat);
				}
			}
			Body->SetVisibility(true);
			Eyes->SetVisibility(false);
			return;
		}
	}
	// Placeholder: a solid (lit) Armada-coloured block with glowing white eyes, until Nick's crew art exists.
	// The engine's glow material is translucent, so it is only used for the eyes.
	Body->SetVisibility(true);
	Eyes->SetVisibility(true);
	UTDStatics::TintPlaceholder(Body, Spec.TeamColor, 0.f);
	UTDStatics::TintPlaceholder(Eyes, FLinearColor(1.f, 0.95f, 0.9f), 4.f);
}

void ATDCrewCharacter::PlayCrewCue(FName Name, const FTDFXCue& Cue, bool bAttached, const FVector& Location)
{
	OnCueRequested.Broadcast(this, Name);
	if (Cue.IsEmpty())
	{
		return;
	}
	if (bAttached)
	{
		UTDStatics::PlayCueAttached(Cue, GetRootComponent());
	}
	else
	{
		UTDStatics::PlayCue(this, Cue, Location, GetActorRotation());
	}
}

bool ATDCrewCharacter::IsPlaceholderVisible() const
{
	return Body && Body->IsVisible() && Body->GetStaticMesh() && Body->GetStaticMesh()->GetName() == TEXT("Cube");
}

// ------------------------------------------------------------------------------------------------
// Attacks
// ------------------------------------------------------------------------------------------------

float ATDCrewCharacter::AttackCooldown(ETDCrewAttack Attack) const
{
	const float PhaseScale = BossPhase >= 2 ? Spec.Phase2CooldownScale : 1.f;
	switch (Attack)
	{
	case ETDCrewAttack::Melee: return Spec.MeleeCooldown;
	case ETDCrewAttack::Burst: return Spec.BurstCooldown;
	case ETDCrewAttack::Shotgun: return Spec.ShotgunCooldown;
	case ETDCrewAttack::ChargeSlam: return Spec.BossAttackInterval * 2.f * PhaseScale;
	case ETDCrewAttack::BoltFan: return Spec.BossAttackInterval * 1.5f * PhaseScale;
	case ETDCrewAttack::Summon: return 12.f * PhaseScale;
	default: return 0.f;
	}
}

float ATDCrewCharacter::GetCooldownLeft(ETDCrewAttack Attack) const
{
	const float* Left = Cooldowns.Find(Attack);
	return Left ? FMath::Max(0.f, *Left) : 0.f;
}

void ATDCrewCharacter::ApplyMercy(float HealthScale, int32 MaxSummons)
{
	if (!bSpecApplied)
	{
		InitFromType(CrewType);
	}
	if (Health && HealthScale < 1.f)
	{
		Health->InitMaxValues(FMath::Max(1.f, Spec.MaxHealth * FMath::Clamp(HealthScale, 0.1f, 1.f)), 0.f, true);
	}
	SummonsLeft = MaxSummons;
}

int32 ATDCrewCharacter::GetAliveSummons() const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<ATDCrewCharacter>& S : Summons)
	{
		Alive += (S.IsValid() && !S->IsDead()) ? 1 : 0;
	}
	return Alive;
}

bool ATDCrewCharacter::StartAttack(ETDCrewAttack Attack, AActor* Target)
{
	if (Attack == ETDCrewAttack::None || IsDead() || IsStaggered() || IsAttacking() || GetCooldownLeft(Attack) > 0.f || OpeningGraceLeft > 0.f)
	{
		return false;
	}
	bool bCan = false;
	float Windup = Spec.BurstWindup;
	switch (Attack)
	{
	case ETDCrewAttack::Melee: bCan = Spec.MeleeDamage > 0.f && Target; Windup = Spec.MeleeWindup; break;
	case ETDCrewAttack::Burst: bCan = Spec.BurstCount > 0 && Target; break;
	case ETDCrewAttack::Shotgun: bCan = Spec.ShotgunPellets > 0 && Target; break;
	case ETDCrewAttack::ChargeSlam: bCan = Spec.ChargeSlamDamage > 0.f && Target; Windup = Spec.ChargeWindup; break;
	case ETDCrewAttack::BoltFan: bCan = Spec.FanBolts > 0 && Target; break;
	case ETDCrewAttack::Summon: bCan = Spec.SummonCount > 0 && SummonsLeft != 0 && GetAliveSummons() < Spec.SummonMaxAlive; break;
	default: break;
	}
	if (!bCan)
	{
		return false;
	}
	CurrentAttack = Attack;
	AttackTarget = Target;
	WindupLeft = FMath::Max(0.01f, Windup);
	RecoverLeft = 0.f;
	ShotsLeft = 0;
	PlayCrewCue(TEXT("Telegraph"), Visuals ? Visuals->Telegraph : FTDFXCue(), true, GetActorLocation());
	OnAttackStarted.Broadcast(this, Attack);
	return true;
}

void ATDCrewCharacter::ReleaseAttack()
{
	const ETDCrewAttack Attack = CurrentAttack;
	Cooldowns.Add(Attack, AttackCooldown(Attack));
	OnAttackReleased.Broadcast(this, Attack);
	switch (Attack)
	{
	case ETDCrewAttack::Melee:
		DoMelee();
		RecoverLeft = 0.3f;
		break;
	case ETDCrewAttack::Burst:
		ShotsLeft = Spec.BurstCount;
		ShotTimer = 0.f;
		break;
	case ETDCrewAttack::Shotgun:
		DoShotgun();
		RecoverLeft = 0.4f;
		break;
	case ETDCrewAttack::ChargeSlam:
	{
		const AActor* T = AttackTarget.Get();
		FVector To = T ? (T->GetActorLocation() - GetActorLocation()) : GetActorForwardVector() * Spec.ChargeDistance;
		To.Z = 0.f;
		const float Dist = FMath::Min(To.Size(), Spec.ChargeDistance);
		ChargeVelocity = To.GetSafeNormal() * (Dist / FMath::Max(Spec.ChargeDuration, 0.05f));
		ChargeTimeLeft = Spec.ChargeDuration;
		break;
	}
	case ETDCrewAttack::BoltFan:
		DoBoltFan();
		RecoverLeft = 0.5f;
		break;
	case ETDCrewAttack::Summon:
		DoSummon();
		RecoverLeft = 0.5f;
		break;
	default:
		RecoverLeft = 0.1f;
		break;
	}
}

void ATDCrewCharacter::FinishAttack()
{
	const ETDCrewAttack Was = CurrentAttack;
	CurrentAttack = ETDCrewAttack::None;
	WindupLeft = RecoverLeft = 0.f;
	ShotsLeft = 0;
	OnAttackFinished.Broadcast(this, Was);
}

void ATDCrewCharacter::CancelAttack()
{
	ChargeTimeLeft = 0.f;
	if (CurrentAttack != ETDCrewAttack::None)
	{
		FinishAttack();
	}
}

FVector ATDCrewCharacter::AimAt(const AActor* At) const
{
	const FVector Muzzle = GetActorLocation() + FVector(0.f, 0.f, Spec.CapsuleHalfHeight * 0.4f);
	return At ? (At->GetActorLocation() - Muzzle).GetSafeNormal() : GetActorForwardVector();
}

void ATDCrewCharacter::DoMelee()
{
	AActor* T = AttackTarget.Get();
	const UTDHealthComponent* TH = T ? UTDStatics::GetHealth(T) : nullptr;
	if (!T || !TH || TH->IsDead())
	{
		return;
	}
	FVector To = T->GetActorLocation() - GetActorLocation();
	To.Z = 0.f;
	const float Reach = Spec.MeleeRange + TDCrewImpl::TargetRadius(T);
	const bool bInFront = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), To.GetSafeNormal()) >= 0.5f; // 120° swing
	if (To.Size() <= Reach && bInFront)
	{
		const FVector Dir = To.GetSafeNormal();
		UTDStatics::ApplyTDDamage(T, Spec.MeleeDamage, ETDDamageKind::Melee, this, this, T->GetActorLocation() - Dir * 40.f, Dir);
	}
}

void ATDCrewCharacter::FireBolt(const FVector& Direction, float Damage, float Speed, float Radius)
{
	UWorld* World = GetWorld();
	if (!World || Direction.IsNearlyZero())
	{
		return;
	}
	FTDProjectileSpec P;
	P.Damage = Damage;
	P.DamageKind = ETDDamageKind::Energy;
	P.Speed = Speed;
	P.Range = Spec.RangedMaxRange * 1.5f;
	P.CollisionRadius = Radius;
	P.bInheritShooterVelocity = false;
	const FVector Muzzle = GetActorLocation() + FVector(0.f, 0.f, Spec.CapsuleHalfHeight * 0.4f) + Direction * (Spec.CapsuleRadius + 35.f);
	ATDProjectile::Launch(World, P, FTransform(Direction.Rotation(), Muzzle), this, ETDTeam::Enemy, nullptr, FVector::ZeroVector, 1.f, false);
}

void ATDCrewCharacter::DoShotgun()
{
	const FVector Base = AimAt(AttackTarget.Get());
	const FRotator BaseRot = Base.Rotation();
	// Deterministic cone: one pellet on axis, the rest on a ring at the spread angle.
	for (int32 i = 0; i < Spec.ShotgunPellets; ++i)
	{
		FVector Dir = Base;
		if (i > 0)
		{
			const float Around = 360.f * (i - 1) / FMath::Max(1, Spec.ShotgunPellets - 1);
			const float Off = Spec.ShotgunSpreadDeg * 0.5f;
			Dir = (BaseRot + FRotator(FMath::Sin(FMath::DegreesToRadians(Around)) * Off, FMath::Cos(FMath::DegreesToRadians(Around)) * Off, 0.f)).Vector();
		}
		FireBolt(Dir, Spec.ShotgunPelletDamage, Spec.BoltSpeed, 16.f);
	}
}

void ATDCrewCharacter::DoBoltFan()
{
	const int32 N = BossPhase >= 2 && Spec.Phase2FanBolts > 0 ? Spec.Phase2FanBolts : Spec.FanBolts;
	const FRotator Base = AimAt(AttackTarget.Get()).Rotation();
	for (int32 i = 0; i < N; ++i)
	{
		const float Alpha = N > 1 ? (float)i / (N - 1) : 0.5f;
		const float Yaw = FMath::Lerp(-Spec.FanArcDeg * 0.5f, Spec.FanArcDeg * 0.5f, Alpha);
		FireBolt((Base + FRotator(0.f, Yaw, 0.f)).Vector(), Spec.BoltDamage, Spec.BoltSpeed, Spec.BoltRadius);
	}
}

void ATDCrewCharacter::DoSummon()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (SummonsLeft == 0)
	{
		return;
	}
	if (SummonsLeft > 0)
	{
		--SummonsLeft;
	}
	const int32 Want = BossPhase >= 2 && Spec.Phase2SummonCount > 0 ? Spec.Phase2SummonCount : Spec.SummonCount;
	const int32 Room = FMath::Max(0, Spec.SummonMaxAlive - GetAliveSummons());
	const int32 Count = FMath::Min(Want, Room);
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = 2.f * PI * (i + 0.5f) / FMath::Max(Count, 1);
		const FVector Loc = GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Spec.CapsuleRadius + 220.f);
		const FTransform Xf(GetActorRotation(), Loc);
		ATDCrewCharacter* Grunt = World->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Grunt)
		{
			Grunt->CrewType = ETDCrewType::Grunt;
			Grunt->FinishSpawning(Xf);
			Summons.Add(Grunt);
			if (ATDEncounterVolume* OwnerRoom = OwningEncounter.Get())
			{
				OwnerRoom->AdoptMember(Grunt); // counts toward the clear, removed by a reset
			}
		}
	}
	Summons.RemoveAll([](const TWeakObjectPtr<ATDCrewCharacter>& S) { return !S.IsValid(); });
}

void ATDCrewCharacter::TickCharge(float DeltaSeconds)
{
	if (ChargeTimeLeft <= 0.f)
	{
		return;
	}
	const float Step = FMath::Min(ChargeTimeLeft, DeltaSeconds);
	if (!HasGroundAhead(ChargeVelocity.GetSafeNormal2D(), ChargeVelocity.Size2D() * Step + Spec.CapsuleRadius))
	{
		EndCharge(); // slam at the edge rather than charging off it
		return;
	}
	FHitResult Hit;
	GetCharacterMovement()->SafeMoveUpdatedComponent(ChargeVelocity * Step, GetActorQuat(), true, Hit);
	ChargeTimeLeft -= DeltaSeconds;
	if (ChargeTimeLeft <= 0.f || (Hit.IsValidBlockingHit() && Cast<APawn>(Hit.GetActor())))
	{
		EndCharge();
	}
}

void ATDCrewCharacter::EndCharge()
{
	ChargeTimeLeft = 0.f;
	PlayCrewCue(TEXT("SlamImpact"), Visuals ? Visuals->Explosion : FTDFXCue(), false, GetActorLocation());
	UTDStatics::ApplyTDRadialDamage(this, GetActorLocation(), Spec.ChargeSlamRadius, Spec.ChargeSlamDamage, ETDDamageKind::Explosive,
		ETDTeam::Enemy, this, this);
	RecoverLeft = 0.6f;
}

// ------------------------------------------------------------------------------------------------
// Damage, stagger, death
// ------------------------------------------------------------------------------------------------

bool ATDCrewCharacter::IsInShieldArc(const FVector& WorldLocation) const
{
	FVector To = WorldLocation - GetActorLocation();
	To.Z = 0.f;
	if (To.IsNearlyZero())
	{
		return true;
	}
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(Spec.ShieldArcDeg * 0.5f));
	return FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), To.GetSafeNormal()) >= CosHalf;
}

float ATDCrewCharacter::FilterDamage(const FTDDamageInfo& Info)
{
	if (!Spec.bFrontalShield || IsGuardBroken() || Info.Kind != ETDDamageKind::Melee)
	{
		return Info.Amount;
	}
	const FVector From = Info.Instigator ? Info.Instigator->GetActorLocation() : (Info.HitLocation - Info.HitDirection * 100.f);
	if (!IsInShieldArc(From))
	{
		return Info.Amount;
	}
	PlayCrewCue(TEXT("ShieldHit"), Visuals ? Visuals->ShieldHit : FTDFXCue(), false, Info.HitLocation);
	return 0.f; // the shield takes the slash
}

void ATDCrewCharacter::HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	if (HullDamage > 0.f)
	{
		PlayCrewCue(TEXT("HullHit"), Visuals ? Visuals->HullHit : FTDFXCue(), false, Info.HitLocation);
	}
	if (Spec.Type == ETDCrewType::Warden && BossPhase == 1 && !IsDead() && Health->GetHull01() <= Spec.Phase2HealthFraction)
	{
		BossPhase = 2;
		Cooldowns.Reset(); // phase 2 opens with an attack
		OnBossPhase2.Broadcast(this);
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
		{
			Flow->RequestBark(TEXT("Bark_WardenPhase2"), TEXT("Armada"));
		}
	}
}

void ATDCrewCharacter::ApplyStagger(float Seconds, AActor* Source)
{
	if (IsDead())
	{
		return;
	}
	CancelAttack();
	const bool bWasStaggered = IsStaggered();
	StaggerLeft = FMath::Max(StaggerLeft, Seconds);
	if (!bWasStaggered)
	{
		// Entering the stagger state: Nick's Stagger cue (falls back to the older Stunned slot).
		const FTDFXCue* Cue = Visuals ? (Visuals->Stagger.IsEmpty() ? &Visuals->Stunned : &Visuals->Stagger) : nullptr;
		PlayCrewCue(TEXT("Stagger"), Cue ? *Cue : FTDFXCue(), true, GetActorLocation());
	}
}

void ATDCrewCharacter::BreakGuard(AActor* Source)
{
	if (Spec.bFrontalShield)
	{
		GuardBrokenLeft = Spec.GuardBrokenSeconds;
		ApplyStagger(0.5f, Source);
	}
}

void ATDCrewCharacter::HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow)
{
	CancelAttack();
	if (UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this))
	{
		Tokens->Release(this);
	}
	OnCrewDied.Broadcast(this);
	// The Warden's summons fall with it.
	for (const TWeakObjectPtr<ATDCrewCharacter>& S : Summons)
	{
		if (ATDCrewCharacter* Grunt = S.Get(); Grunt && !Grunt->IsDead() && Grunt->GetHealth())
		{
			Grunt->GetHealth()->Kill(KillingBlow.Instigator, ETDDamageKind::Environmental);
		}
	}
	PlayCrewCue(TEXT("Death"), Visuals ? Visuals->Death : FTDFXCue(), false, GetActorLocation());

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// Ragdoll: the skeletal mesh if Nick's art has a physics asset, otherwise the placeholder block tumbles.
	const FVector Push = (KillingBlow.HitDirection.IsNearlyZero() ? -GetActorForwardVector() : KillingBlow.HitDirection.GetSafeNormal()) * 450.f + FVector(0.f, 0.f, 250.f);
	if (GetMesh()->IsVisible() && GetMesh()->GetPhysicsAsset())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->AddImpulse(Push, NAME_None, true);
	}
	else if (Body->IsVisible())
	{
		Body->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Body->SetSimulatePhysics(true);
		Eyes->SetVisibility(false); // the eyes go dark when it dies
		Body->AddImpulse(Push, NAME_None, true);
	}
	DetachFromControllerPendingDestroy();
	SetLifeSpan(FMath::Max(0.1f, CorpseSeconds));
}

// ------------------------------------------------------------------------------------------------

void ATDCrewCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	OpeningGraceLeft = FMath::Max(0.f, OpeningGraceLeft - DeltaSeconds);
	if (DeltaSeconds <= 0.f || IsDead())
	{
		return;
	}
	StaggerLeft = FMath::Max(0.f, StaggerLeft - DeltaSeconds);
	GuardBrokenLeft = FMath::Max(0.f, GuardBrokenLeft - DeltaSeconds);
	for (TPair<ETDCrewAttack, float>& C : Cooldowns)
	{
		C.Value = FMath::Max(0.f, C.Value - DeltaSeconds);
	}
	TickCharge(DeltaSeconds);

	if (CurrentAttack == ETDCrewAttack::None)
	{
		return;
	}
	if (WindupLeft > 0.f)
	{
		WindupLeft -= DeltaSeconds;
		if (WindupLeft <= 0.f)
		{
			WindupLeft = 0.f;
			ReleaseAttack();
		}
		return;
	}
	if (ShotsLeft > 0)
	{
		ShotTimer -= DeltaSeconds;
		if (ShotTimer <= 0.f)
		{
			FireBolt(AimAt(AttackTarget.Get()), Spec.BoltDamage, Spec.BoltSpeed, Spec.BoltRadius);
			--ShotsLeft;
			ShotTimer += Spec.BurstSpacing;
			if (ShotsLeft == 0)
			{
				RecoverLeft = 0.25f;
			}
		}
		return;
	}
	if (ChargeTimeLeft > 0.f)
	{
		return;
	}
	RecoverLeft -= DeltaSeconds;
	if (RecoverLeft <= 0.f)
	{
		FinishAttack();
	}
}

// ================================================================================================

UTDCrewTokenSubsystem* UTDCrewTokenSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDCrewTokenSubsystem>() : nullptr;
}

bool UTDCrewTokenSubsystem::TryAcquire(AActor* Attacker)
{
	if (!Attacker)
	{
		return false;
	}
	Holders.RemoveAll([](const TWeakObjectPtr<AActor>& H) { return !H.IsValid(); });
	if (Holds(Attacker))
	{
		return true;
	}
	if (Holders.Num() >= MaxMeleeAttackers)
	{
		return false;
	}
	Holders.Add(Attacker);
	return true;
}

void UTDCrewTokenSubsystem::Release(AActor* Attacker)
{
	Holders.RemoveAll([Attacker](const TWeakObjectPtr<AActor>& H) { return !H.IsValid() || H.Get() == Attacker; });
}

bool UTDCrewTokenSubsystem::Holds(const AActor* Attacker) const
{
	for (const TWeakObjectPtr<AActor>& H : Holders)
	{
		if (H.Get() == Attacker)
		{
			return true;
		}
	}
	return false;
}

int32 UTDCrewTokenSubsystem::GetActiveCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<AActor>& H : Holders)
	{
		N += H.IsValid() ? 1 : 0;
	}
	return N;
}
