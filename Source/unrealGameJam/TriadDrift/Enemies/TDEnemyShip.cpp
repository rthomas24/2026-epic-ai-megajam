// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDEnemyShip.h"

#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Flight/TDFlightTypes.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/WorldSettings.h"

ATDEnemyShip::ATDEnemyShip()
{
	PrimaryActorTick.bCanEverTick = true;

	// Weapons come from ATDSpacecraft's "Weapons" subobject; EnsureWeapons() is a null-safe fallback.

	AIControllerClass = ATDEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Team = ETDTeam::Enemy;

	// Placeholder presentation (hidden once Nick's mesh is set). Glow comes from Core's always-cooked
	// M_TD_PlaceholderGlow through TintPlaceholder.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	auto MakePart = [this](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(HullPivot);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetCastShadow(false);
		C->CanCharacterStepUpOn = ECB_No;
		if (Mesh) { C->SetStaticMesh(Mesh); }
		C->SetRelativeLocationAndRotation(Loc, Rot);
		C->SetRelativeScale3D(Scale);
		return C;
	};
	UStaticMesh* Sphere = SphereMesh.Succeeded() ? SphereMesh.Object : nullptr;
	// One stretched glowing "visor" for the two eyes (wave 5 primitive count: one component, not two).
	PlaceholderEyeLeft = MakePart(TEXT("PlaceholderEyes"), Sphere, FVector(70.f, 0.f, 22.f), FRotator::ZeroRotator, FVector(0.38f, 1.05f, 0.3f));
	PlaceholderEyeRight = nullptr;
	// A thin disc across the nose: the gunship's frontal shield (shown only while it is up).
	PlaceholderShieldPlate = MakePart(TEXT("PlaceholderShieldPlate"), CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr,
		FVector(240.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(1.4f, 1.4f, 0.03f));	// live check: 2.6 hid the fight
	PlaceholderShieldPlate->SetVisibility(false);
}

// ------------------------------------------------------------------------------------------ setup

void ATDEnemyShip::InitFromSpec(FName EnemyId)
{
	const FTDEnemySpec& Found = UTDEnemyLibrary::Get(EnemyId);
	if (!Found.IsValid())
	{
		return;	// Get() already warned
	}
	InitFromSpecData(Found);
}

void ATDEnemyShip::InitFromSpecData(const FTDEnemySpec& InSpec)
{
	if (!InSpec.IsValid() || bDying)
	{
		return;
	}

	DestroyCarrierParts();
	Spec = InSpec;
	bSpecApplied = true;

	// ENG-27 difficulty: the applied spec is the scaled one (GetSpec() reports what's in play).
	const FTDDifficultyScale Difficulty = UTDDifficultySettings::GetScale();
	Spec.Hull *= Difficulty.EnemyHull;
	Spec.Shield *= Difficulty.EnemyHull;
	Spec.FrontalShield *= Difficulty.EnemyHull;
	Spec.BayHull *= Difficulty.EnemyHull;
	Spec.TurretHull *= Difficulty.EnemyHull;
	Spec.AimErrorDeg *= Difficulty.EnemyAimError;
	AimMaxMissCm = GunAimMaxMissCm * Difficulty.EnemyAimError;
	DifficultyDamage = Difficulty.EnemyDamage;

	SetTeam(ETDTeam::Enemy);
	if (Spec.Role == ETDEnemyRole::Carrier) { Tags.AddUnique(TDDirectorTags::Carrier); }
	else { Tags.Remove(TDDirectorTags::Carrier); }

	if (UTDHealthComponent* H = GetHealth())
	{
		H->DefaultTeam = ETDTeam::Enemy;
		H->InitMaxValues(Spec.Hull, Spec.Shield, true);
	}

	FTDFlightTuning Tuning;
	Tuning.CruiseSpeed = Spec.CruiseSpeed;
	Tuning.MinSpeed = Spec.CruiseSpeed * Spec.MinSpeedFraction;
	Tuning.BoostSpeed = Spec.CruiseSpeed * 1.6f;
	Tuning.TurnRate = Spec.TurnRateDeg;
	Tuning.RollRate = FMath::Max(60.f, Spec.TurnRateDeg * 1.5f);
	ApplyFlightTuning(Tuning);

	if (USphereComponent* Sphere = GetCollisionSphere())
	{
		Sphere->SetSphereRadius(Spec.Radius);
	}

	if (EnsureWeapons())
	{
		Weapons->SetOutgoingDamageMultiplier(TEXT("Difficulty"), DifficultyDamage);
		if (!Spec.Guns.WeaponId.IsNone())
		{
			Weapons->EquipWeapon(ETDWeaponSlot::Primary, Spec.Guns.WeaponId);
		}
		SecondaryWeaponId = NAME_None;
		if (!Spec.Heavy.WeaponId.IsNone())
		{
			Weapons->EquipWeapon(ETDWeaponSlot::Secondary, Spec.Heavy.WeaponId);
			SecondaryWeaponId = Spec.Heavy.WeaponId;
		}
	}

	FrontalShieldHP = Spec.FrontalShield;
	FrontalShieldDownRemaining = 0.f;
	FrontalShieldRecoverRemaining = 0.f;
	CooldownRemaining.Reset();
	FireCounts.Reset();
	CancelTelegraph();

	if (!bVisualsExplicit)
	{
		Visuals = ResolveVisuals(Spec);
	}
	UpdatePlaceholderLook();
	if (HasActorBegunPlay())
	{
		StartLoopCues();
		SpawnCarrierParts();
	}
}

UTDWeaponComponent* ATDEnemyShip::EnsureWeapons()
{
	if (!Weapons)
	{
		Weapons = FindComponentByClass<UTDWeaponComponent>();
	}
	if (!Weapons)
	{
		Weapons = NewObject<UTDWeaponComponent>(this, TEXT("EnemyWeapons"));
		if (Weapons)
		{
			AddInstanceComponent(Weapons);
			Weapons->RegisterComponent();
		}
	}
	return Weapons;
}

void ATDEnemyShip::JoinFormation(AActor* Leader, const FVector& LocalOffset)
{
	if (ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(GetController()))
	{
		AI->SetFormationLeader(Cast<ATDEnemyShip>(Leader), LocalOffset);
	}
}

void ATDEnemyShip::PlayWarpIn()
{
	if (Visuals)
	{
		UTDStatics::PlayCue(this, Visuals->WarpIn, GetActorLocation(), GetActorRotation());
	}
}

FString ATDEnemyShip::VisualsAssetName(const FTDEnemySpec& InSpec)
{
	// Names agreed with Nick's side (MESSAGEBOARD R-005): DA_Enemy_<Fighter|Interceptor|Gunship|Bomber|Carrier>_Visuals.
	return InSpec.VisualsAssetName.IsNone() ? FString() : InSpec.VisualsAssetName.ToString();
}

UTDEnemyVisuals* ATDEnemyShip::ResolveVisuals(const FTDEnemySpec& InSpec)
{
	if (!InSpec.Visuals.IsNull())
	{
		return InSpec.Visuals.LoadSynchronous();
	}
	const FString Name = VisualsAssetName(InSpec);
	// Core's loader: quiet when the asset doesn't exist yet, remembers misses, cooks with the Presentation folder.
	return Name.IsEmpty() ? nullptr : UTDStatics::LoadPresentation<UTDEnemyVisuals>(Name);
}

void ATDEnemyShip::StartLoopCues()
{
	if (Visuals && !EngineTrailFX)
	{
		EngineTrailFX = UTDStatics::PlayCueAttached(Visuals->EngineTrail, GetHullMesh());
	}
}

void ATDEnemyShip::HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float /*ShieldDamage*/, float HullDamage)
{
	if (!Visuals || bDying || HullDamage <= 0.f)
	{
		return;
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (Now - LastHullHitCueTime > 0.08)
	{
		LastHullHitCueTime = Now;
		UTDStatics::PlayCue(this, Visuals->HullHit, Info.HitLocation.IsNearlyZero() ? GetActorLocation() : Info.HitLocation);
	}
	if (!DamagedSmokeFX && InHealth && InHealth->GetHull01() < 0.5f)
	{
		DamagedSmokeFX = UTDStatics::PlayCueAttached(Visuals->DamagedSmoke, GetHullMesh());
	}
}

namespace TDEnemyShipSeeding
{
	// Per-world spawn order: the same world always plays the same way, whatever ran before it (a seed from
	// GetUniqueID depended on how many objects earlier tests had created: flaky in the full suite).
	TMap<TWeakObjectPtr<const UWorld>, int32>& Counters()
	{
		static TMap<TWeakObjectPtr<const UWorld>, int32> Map;
		return Map;
	}
}

int32& ATDEnemyShip::SpawnSeedSalt()
{
	static int32 Salt = 0;
	return Salt;
}

int32 ATDEnemyShip::GetSpawnSeed()
{
	if (SpawnSeed == INDEX_NONE)
	{
		TMap<TWeakObjectPtr<const UWorld>, int32>& Map = TDEnemyShipSeeding::Counters();
		for (auto It = Map.CreateIterator(); It; ++It) { if (!It.Key().IsValid()) { It.RemoveCurrent(); } }
		int32& Next = Map.FindOrAdd(GetWorld());
		SpawnSeed = 7919 * (++Next) + 104729 + 1000003 * SpawnSeedSalt();
	}
	return SpawnSeed;
}

void ATDEnemyShip::SetSpawnSeed(int32 Seed)
{
	SpawnSeed = Seed;
	AimRng.Initialize(Seed);
	if (ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(GetController()))
	{
		AI->ReseedRandom(Seed ^ 0x5bd1e995);
	}
}

void ATDEnemyShip::BeginPlay()
{
	Super::BeginPlay();
	AimRng.Initialize(GetSpawnSeed());	// every pilot misses differently, the same way every run
	EnsureWeapons();
	bVisualsExplicit = Visuals != nullptr && !bSpecApplied;	// set on a BP child / placed instance
	if (bSpecApplied) { StartLoopCues(); }
	if (UTDHealthComponent* H = GetHealth())
	{
		H->OnDamaged.AddUniqueDynamic(this, &ATDEnemyShip::HandleDamaged);
	}

	if (UTDHealthComponent* H = GetHealth())
	{
		H->DamageFilter.BindUObject(this, &ATDEnemyShip::FilterIncomingDamage);
		H->OnDeath.AddUniqueDynamic(this, &ATDEnemyShip::HandleHealthDeath);
	}

	if (bSpecApplied)
	{
		SpawnCarrierParts();
	}
}

void ATDEnemyShip::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTDHealthComponent* H = GetHealth())
	{
		H->DamageFilter.Unbind();
		H->OnDeath.RemoveDynamic(this, &ATDEnemyShip::HandleHealthDeath);
		H->OnDamaged.RemoveDynamic(this, &ATDEnemyShip::HandleDamaged);
	}
	DestroyCarrierParts();
	Super::EndPlay(EndPlayReason);
}

void ATDEnemyShip::UpdatePlaceholderLook()
{
	UStaticMeshComponent* Hull = GetHullMesh();
	if (!Hull)
	{
		return;
	}

	if (Visuals && !Visuals->Mesh.IsNull())
	{
		if (UStaticMesh* Mesh = Visuals->Mesh.LoadSynchronous())
		{
			Hull->SetStaticMesh(Mesh);
			Hull->SetRelativeRotation(FRotator::ZeroRotator);	// art is authored +X forward
			Hull->SetRelativeScale3D(FVector::OneVector);
			for (int32 i = 0; i < Visuals->MaterialOverrides.Num(); ++i)
			{
				if (UMaterialInterface* Mat = Visuals->MaterialOverrides[i].LoadSynchronous())
				{
					Hull->SetMaterial(i, Mat);
				}
			}
			// T1's placeholder wings and the white engine-glow cube would sit on top of the real mesh (the cube showed
			// between the fighter's nozzles): hide both and stop the throttle stretch; the art's own emissive engines glow.
			SetPlaceholderPartsVisible(false);
			// Real art: the placeholder parts go away entirely (no hidden primitives left per ship).
			for (UStaticMeshComponent* C : { PlaceholderEyeLeft.Get(), PlaceholderShieldPlate.Get() })	// wings are T1's: hidden above, not destroyed
			{
				if (C) { C->DestroyComponent(); }
			}
			PlaceholderEyeLeft = nullptr;
			PlaceholderShieldPlate = nullptr;
			if (HullPivot) { HullPivot->SetRelativeScale3D(FVector::OneVector); }
			BaseVisualScale = FVector::OneVector;
			EngineGlowMID = nullptr;
			ApplyVisualScale();
			bPlaceholderLook = false;
			return;
		}
	}

	// Placeholder: T1's cone + wings, scaled as one by the gameplay radius (220 cm = default) and tinted.
	SetPlaceholderPartsVisible(true);
	if (HullPivot)
	{
		// One silhouette per role, so types read apart before the art lands (playtest: all cones looked alike).
		FVector Shape(1.f);
		switch (Spec.Role)
		{
		case ETDEnemyRole::Interceptor: Shape = FVector(1.5f, 0.75f, 0.75f); break;	// long dart
		case ETDEnemyRole::Gunship:     Shape = FVector(0.8f, 1.9f, 0.8f); break;	// wide wall
		case ETDEnemyRole::Bomber:      Shape = FVector(1.1f, 1.5f, 1.5f); break;	// fat
		case ETDEnemyRole::Carrier:     Shape = FVector(2.0f, 1.2f, 0.7f); break;	// long slab
		default: break;
		}
		// Placeholder-only readability scale (collision keeps the gameplay radius).
		const float Readable = Spec.Size == ETDEnemySize::Small ? PlaceholderSmallShipScale
			: (Spec.Size == ETDEnemySize::Medium ? PlaceholderMediumShipScale : PlaceholderLargeShipScale);
		BaseVisualScale = Shape * FMath::Max(0.2f, Spec.Radius / 220.f) * Readable;
		ApplyVisualScale();
	}
	// The colour code on its own, on black space (Lead playtest 3: lit hulls were dark specks): a glowing
	// red-orange body that blooms, dark-red wings for the silhouette, two white glowing "eyes" at the nose,
	// a white-hot engine, and the gunship's shield as a cyan plate while it is up.
	UTDStatics::TintPlaceholder(Hull, Spec.PlaceholderColor, PlaceholderBodyGlow);
	UTDStatics::TintPlaceholder(PlaceholderWings, FLinearColor(0.35f, 0.03f, 0.02f), 0.6f);
	// Red-orange engine streak (the spacecraft stretches it with speed); white stays for the eyes.
	EngineGlowBase = FLinearColor(1.f, 0.32f, 0.06f) * PlaceholderEmissiveBoost;
	EngineGlowMID = UTDStatics::TintPlaceholder(GetEngineGlow(), FLinearColor(1.f, 0.32f, 0.06f), PlaceholderEmissiveBoost);
	for (UStaticMeshComponent* Eye : { PlaceholderEyeLeft.Get(), PlaceholderEyeRight.Get() })
	{
		if (Eye) { Eye->SetVisibility(true); UTDStatics::TintPlaceholder(Eye, FLinearColor::White, PlaceholderEyeGlow); }
	}
	bPlaceholderLook = true;
	if (PlaceholderShieldPlate && !HasFrontalShield())
	{
		PlaceholderShieldPlate->DestroyComponent();	// only gunships carry the plate
		PlaceholderShieldPlate = nullptr;
	}
	if (PlaceholderShieldPlate)
	{
		UTDStatics::TintPlaceholder(PlaceholderShieldPlate, FLinearColor(0.1f, 0.5f, 0.8f), 0.45f);
		PlaceholderShieldPlate->SetVisibility(HasFrontalShield() && IsFrontalShieldUp());
	}
}

// ---------------------------------------------------------------------------------------- attacks

const FTDEnemyAttackSpec* ATDEnemyShip::FindAttack(ETDEnemyAttack Attack) const
{
	if (Attack == ETDEnemyAttack::None) { return nullptr; }
	if (Spec.Guns.Kind == Attack) { return &Spec.Guns; }
	if (Spec.Heavy.Kind == Attack) { return &Spec.Heavy; }
	if (Spec.Heavy2.Kind == Attack) { return &Spec.Heavy2; }
	return nullptr;
}

bool ATDEnemyShip::CanFire() const
{
	return !bDying && StunRemaining <= 0.f && FireSuppressedRemaining <= 0.f && IsAliveEnemy();
}

bool ATDEnemyShip::IsAliveEnemy() const
{
	const UTDHealthComponent* H = GetHealth();
	return !bDying && (!H || H->IsAlive());
}

bool ATDEnemyShip::CanStartAttack(ETDEnemyAttack Attack) const
{
	const FTDEnemyAttackSpec* A = FindAttack(Attack);
	if (!A || Attack == ETDEnemyAttack::Guns || !CanFire())
	{
		return false;
	}
	if (IsTelegraphing() || BurstShotsRemaining > 0)
	{
		return false;
	}
	return GetAttackCooldownRemaining(Attack) <= 0.f;
}

bool ATDEnemyShip::TryStartAttack(ETDEnemyAttack Attack, AActor* Target)
{
	if (!CanStartAttack(Attack))
	{
		return false;
	}
	const FTDEnemyAttackSpec& A = *FindAttack(Attack);

	// Cooldown runs from the start of the attack, so "every 8 s" means start-to-start.
	CooldownRemaining.Add(Attack, A.Interval);

	if (A.IsTelegraphed())
	{
		TelegraphAttack = Attack;
		TelegraphElapsed = 0.f;
		TelegraphDuration = A.TelegraphSeconds;
		TelegraphTarget = Target;
		OnTelegraphStarted.Broadcast(this, Attack, TelegraphDuration);
		if (Visuals)
		{
			const FTDFXCue* Specific = nullptr;
			switch (Attack)
			{
			case ETDEnemyAttack::Missile:      Specific = &Visuals->TelegraphMissile; break;
			case ETDEnemyAttack::HeavyBolt:    Specific = &Visuals->TelegraphHeavyBolt; break;
			case ETDEnemyAttack::MissileSalvo: Specific = &Visuals->TelegraphSalvo; break;
			case ETDEnemyAttack::MineTrail:    Specific = &Visuals->TelegraphMineTrail; break;
			default: break;
			}
			// The specific cue wins only if it has VFX: a sound-only specific cue must not hide the generic flash.
			const bool bSpecificVFX = Specific && !Specific->VFX.IsNull();
			UTDStatics::PlayCueAttached(bSpecificVFX ? *Specific : Visuals->Telegraph, GetHullMesh());
			if (!bSpecificVFX && Specific && !Specific->IsEmpty()) { UTDStatics::PlayCueAttached(*Specific, GetHullMesh()); }
		}
	}
	else
	{
		ExecuteAttack(Attack, Target);
	}
	return true;
}

void ATDEnemyShip::CancelTelegraph()
{
	TelegraphAttack = ETDEnemyAttack::None;
	TelegraphElapsed = 0.f;
	TelegraphDuration = 0.f;
	TelegraphTarget.Reset();
	BurstAttack = ETDEnemyAttack::None;
	BurstShotsRemaining = 0;
	BurstTimer = 0.f;
	BurstTarget.Reset();
}

float ATDEnemyShip::GetTelegraph01() const
{
	if (!IsTelegraphing() || TelegraphDuration <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(TelegraphElapsed / TelegraphDuration, 0.f, 1.f);
}

float ATDEnemyShip::GetAttackCooldownRemaining(ETDEnemyAttack Attack) const
{
	const float* Found = CooldownRemaining.Find(Attack);
	return Found ? FMath::Max(0.f, *Found) : 0.f;
}

int32 ATDEnemyShip::GetFireCount(ETDEnemyAttack Attack) const
{
	return FireCounts.FindRef(Attack);
}

void ATDEnemyShip::ExecuteAttack(ETDEnemyAttack Attack, AActor* Target)
{
	const FTDEnemyAttackSpec* A = FindAttack(Attack);
	if (!A || !CanFire())
	{
		return;
	}

	switch (Attack)
	{
	case ETDEnemyAttack::Missile:
	case ETDEnemyAttack::MissileSalvo:
		// One FireOnce = one missile (Missile) or T2's 3-missile ripple (Salvo).
		FireSecondaryShot(A->WeaponId, Target);
		break;

	case ETDEnemyAttack::HeavyBolt:
	case ETDEnemyAttack::MineTrail:
		BurstAttack = Attack;
		BurstShotsRemaining = FMath::Max(1, A->Count);
		BurstTimer = 0.f;	// first shot this frame
		BurstTarget = Target;
		TickBurst(0.f);
		break;

	case ETDEnemyAttack::LaunchFighters:
		for (ATDCarrierBay* Bay : Bays)
		{
			if (Bay && Bay->IsAlive())
			{
				LaunchFightersFromBay(Bay, A->Count);
			}
		}
		break;

	default:
		break;
	}

	OnAttackFired.Broadcast(this, Attack);
}

void ATDEnemyShip::FireSecondaryShot(FName WeaponId, AActor* Target)
{
	if (WeaponId.IsNone() || !Weapons)
	{
		return;
	}
	if (SecondaryWeaponId != WeaponId)
	{
		Weapons->EquipWeapon(ETDWeaponSlot::Secondary, WeaponId);
		SecondaryWeaponId = WeaponId;
	}

	// Homing weapons lock onto the aim target at fire time; dumb-fire ones use the direction.
	const FVector Forward = GetActorForwardVector();
	const FVector AimDir = Target ? (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal() : Forward;
	Weapons->SetAimDirection(AimDir.IsNearlyZero() ? Forward : AimDir);
	Weapons->SetAimTarget(Target);
	const bool bFired = Weapons->FireOnce(ETDWeaponSlot::Secondary);
	Weapons->SetAimTarget(nullptr);
	if (!bFired)
	{
		return;	// only shots that really left the barrel count
	}
	for (const FTDEnemyAttackSpec* A : { &Spec.Heavy, &Spec.Heavy2 })
	{
		if (A->WeaponId == WeaponId)
		{
			FireCounts.FindOrAdd(A->Kind)++;
			break;
		}
	}
}

void ATDEnemyShip::TickBurst(float DeltaSeconds)
{
	if (BurstShotsRemaining <= 0)
	{
		return;
	}
	if (!CanFire())
	{
		BurstShotsRemaining = 0;
		BurstAttack = ETDEnemyAttack::None;
		return;
	}

	const FTDEnemyAttackSpec* A = FindAttack(BurstAttack);
	if (!A)
	{
		BurstShotsRemaining = 0;
		return;
	}
	const float Spacing = (BurstAttack == ETDEnemyAttack::MineTrail) ? MineSpacingSeconds : BurstShotSpacing;

	BurstTimer -= DeltaSeconds;
	// Frame-rate independent: a long frame may fire several queued shots.
	while (BurstShotsRemaining > 0 && BurstTimer <= 0.f)
	{
		FireSecondaryShot(A->WeaponId, BurstTarget.Get());
		--BurstShotsRemaining;
		BurstTimer += Spacing;
		if (Spacing <= 0.f) { BurstTimer = 0.f; }
	}
	if (BurstShotsRemaining <= 0)
	{
		BurstAttack = ETDEnemyAttack::None;
		BurstTarget.Reset();
	}
}

void ATDEnemyShip::SetGunsFiring(bool bFiring, AActor* Target)
{
	bGunsFiring = bFiring && Spec.Guns.IsValid();
	GunTarget = Target;
	ApplyPrimaryTrigger();
}

void ATDEnemyShip::ApplyPrimaryTrigger()
{
	const bool bHold = bGunsFiring && CanFire();
	if (Weapons && bHold != bPrimaryTriggerHeld)
	{
		Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, bHold);
	}
	bPrimaryTriggerHeld = bHold;
}

// ------------------------------------------------------------------------------------ status effects

void ATDEnemyShip::ApplyStun(float Seconds)
{
	if (Seconds <= 0.f || bDying)
	{
		return;
	}
	StunRemaining = FMath::Max(StunRemaining, Seconds);
	CancelTelegraph();
	ApplyPrimaryTrigger();
	SetBaysPaused(true);
	for (ATDEnemyTurret* Turret : Turrets)
	{
		if (Turret) { Turret->ApplyStun(Seconds); }
	}
	if (Visuals)
	{
		UTDStatics::PlayCueAttached(Visuals->Stunned, GetHullMesh());
	}
	OnStunned.Broadcast(this, Seconds);
}

void ATDEnemyShip::LoseTracking(float Seconds)
{
	if (Seconds <= 0.f || bDying)
	{
		return;
	}
	TrackingLostRemaining = FMath::Max(TrackingLostRemaining, Seconds);
	CancelTelegraph();
	bGunsFiring = false;
	ApplyPrimaryTrigger();
	if (ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(GetController()))
	{
		AI->ForgetTarget();
	}
	OnTrackingLost.Broadcast(this, Seconds);
}

void ATDEnemyShip::SuppressFire(float Seconds)
{
	if (Seconds <= 0.f)
	{
		return;
	}
	FireSuppressedRemaining = FMath::Max(FireSuppressedRemaining, Seconds);
	CancelTelegraph();
	ApplyPrimaryTrigger();
}

void ATDEnemyShip::SetBaysPaused(bool bPaused)
{
	for (ATDCarrierBay* Bay : Bays)
	{
		if (Bay) { Bay->SetPaused(bPaused); }
	}
}

// ------------------------------------------------------------------------------------ frontal shield

bool ATDEnemyShip::IsFrontalShieldUp() const
{
	return HasFrontalShield() && FrontalShieldHP > 0.f && FrontalShieldDownRemaining <= 0.f;
}

float ATDEnemyShip::GetFrontalShield01() const
{
	if (!HasFrontalShield() || FrontalShieldDownRemaining > 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(FrontalShieldHP / Spec.FrontalShield, 0.f, 1.f);
}

bool ATDEnemyShip::IsFrontalHit(const FVector& TravelDirection) const
{
	const FVector Travel = TravelDirection.GetSafeNormal();
	if (Travel.IsNearlyZero())
	{
		return false;
	}
	// A hit from in front travels against our forward vector.
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(Spec.FrontalShieldHalfAngleDeg));
	return FVector::DotProduct(-Travel, GetActorForwardVector()) >= CosHalf;
}

void ATDEnemyShip::DropFrontalShield(float Seconds)
{
	if (!HasFrontalShield() || Seconds <= 0.f)
	{
		return;
	}
	const bool bWasUp = IsFrontalShieldUp();
	FrontalShieldDownRemaining = FMath::Max(FrontalShieldDownRemaining, Seconds);
	if (bWasUp)
	{
		OnFrontalShieldBroken.Broadcast(this);
	}
}

void ATDEnemyShip::BreakFrontalShield()
{
	if (!HasFrontalShield() || FrontalShieldHP <= 0.f)
	{
		return;
	}
	FrontalShieldHP = 0.f;
	FrontalShieldRecoverRemaining = Spec.FrontalShieldRecoverSeconds;
	if (Visuals) { UTDStatics::PlayCueAttached(Visuals->FrontalShieldBreak, GetHullMesh()); }
	OnFrontalShieldBroken.Broadcast(this);
}

float ATDEnemyShip::FilterIncomingDamage(const FTDDamageInfo& Info)
{
	if (bDying)
	{
		return 0.f;
	}

	float Amount = Info.Amount;
	if (IsStunned())
	{
		Amount *= StunnedDamageMultiplier;
	}

	if (!HasFrontalShield())
	{
		return Amount;
	}

	// Lancer's ram smashes the frontal shield and hits the hull in full.
	if (Info.Kind == ETDDamageKind::Ram)
	{
		BreakFrontalShield();
		return Amount;
	}

	if (!IsFrontalShieldUp())
	{
		return Amount;
	}

	FVector Travel = Info.HitDirection;
	if (Travel.IsNearlyZero())
	{
		const AActor* Source = Info.Causer ? Info.Causer.Get() : Info.Instigator.Get();
		if (Source && Source != this)
		{
			Travel = GetActorLocation() - Source->GetActorLocation();
		}
	}
	if (!IsFrontalHit(Travel))
	{
		return Amount;	// flank / rear: straight to shield + hull
	}

	const float Absorbed = FMath::Min(Amount, FrontalShieldHP);
	FrontalShieldHP -= Absorbed;
	Amount -= Absorbed;
	if (FrontalShieldHP <= 0.f)
	{
		FrontalShieldHP = 0.f;
		FrontalShieldRecoverRemaining = Spec.FrontalShieldRecoverSeconds;
		OnFrontalShieldBroken.Broadcast(this);
	}
	if (Visuals && Absorbed > 0.f)
	{
		UTDStatics::PlayCue(this, Visuals->ShieldHit, Info.HitLocation.IsNearlyZero() ? GetActorLocation() : Info.HitLocation);
	}
	return Amount;
}

// ------------------------------------------------------------------------------------------ carrier

void ATDEnemyShip::MountPart(AActor* Part, const FVector& LocalDirection, float PartHalfExtent, float Up)
{
	if (!Part)
	{
		return;
	}
	bHasMountedParts = true;
	if (RangeScale != 1.f) { RangeScale = 1.f; ApplyVisualScale(); }
	// Snap (not KeepRelative) so a freshly spawned part doesn't inherit our world rotation twice.
	Part->AttachToActor(this, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true));
	// Outside our own blocking sphere, so shots and rams reach the part instead of the hull.
	const float Distance = Spec.Radius + PartHalfExtent + CarrierPartClearance;
	Part->SetActorRelativeLocation(LocalDirection.GetSafeNormal() * Distance + FVector(0.f, 0.f, Up));
	Part->SetActorRelativeRotation(LocalDirection.Rotation());	// part's +X faces outward

	// The carrier sweeps when it moves: never let it collide with its own parts (both ways).
	if (USphereComponent* Sphere = GetCollisionSphere())
	{
		Sphere->IgnoreActorWhenMoving(Part, true);
	}
	if (UPrimitiveComponent* PartRoot = Cast<UPrimitiveComponent>(Part->GetRootComponent()))
	{
		PartRoot->IgnoreActorWhenMoving(this, true);
	}
}

void ATDEnemyShip::SpawnCarrierParts()
{
	UWorld* World = GetWorld();
	if (!World || Spec.Role != ETDEnemyRole::Carrier || Bays.Num() > 0 || Turrets.Num() > 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Bays: port, starboard and dorsal. First launches are staggered so they don't all pop at once.
	const FVector BayDirs[] = { FVector(0.f, -1.f, 0.f), FVector(0.f, 1.f, 0.f), FVector(-0.3f, 0.f, 1.f) };
	for (int32 i = 0; i < Spec.BayCount; ++i)
	{
		ATDCarrierBay* Bay = World->SpawnActor<ATDCarrierBay>(ATDCarrierBay::StaticClass(), FTransform::Identity, Params);
		if (!Bay) { continue; }
		const FVector Dir = BayDirs[i % UE_ARRAY_COUNT(BayDirs)].RotateAngleAxis(25.f * (i / 3), FVector::UpVector);
		MountPart(Bay, Dir, Bay->GetHalfExtent(), 0.f);
		const float FirstDelay = Spec.Heavy.Interval * 0.5f + i * 2.f;
		Bay->Init(this, Spec.BayHull, Spec.Heavy.Count, Spec.Heavy.Interval, Spec.Heavy.TelegraphSeconds, FirstDelay);
		Bay->MaxLaunches = Spec.MaxLaunchesPerBay;
		Bay->Visuals = Visuals;
		Bays.Add(Bay);
	}

	// Point-defense turrets on the four upper quarters.
	for (int32 i = 0; i < Spec.TurretCount; ++i)
	{
		ATDEnemyTurret* Turret = World->SpawnActor<ATDEnemyTurret>(ATDEnemyTurret::StaticClass(), FTransform::Identity, Params);
		if (!Turret) { continue; }
		const float Angle = (PI * 0.5f) * i + PI * 0.25f;
		MountPart(Turret, FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.6f), 120.f, 0.f);
		Turret->Init(Spec.TurretGuns, Spec.TurretHull);
		Turret->SetDamageMultiplier(DifficultyDamage);
		Turret->Visuals = Visuals;
		Turrets.Add(Turret);
	}
}

void ATDEnemyShip::DestroyCarrierParts()
{
	for (ATDCarrierBay* Bay : Bays)
	{
		if (IsValid(Bay)) { Bay->Destroy(); }
	}
	for (ATDEnemyTurret* Turret : Turrets)
	{
		if (IsValid(Turret)) { Turret->Destroy(); }
	}
	Bays.Reset();
	Turrets.Reset();
}

int32 ATDEnemyShip::GetLivingBayCount() const
{
	int32 N = 0;
	for (const ATDCarrierBay* Bay : Bays)
	{
		if (Bay && Bay->IsAlive()) { ++N; }
	}
	return N;
}

TArray<ATDCarrierBay*> ATDEnemyShip::GetBays() const
{
	TArray<ATDCarrierBay*> Out;
	for (ATDCarrierBay* Bay : Bays) { if (Bay) { Out.Add(Bay); } }
	return Out;
}

TArray<ATDEnemyTurret*> ATDEnemyShip::GetTurrets() const
{
	TArray<ATDEnemyTurret*> Out;
	for (ATDEnemyTurret* T : Turrets) { if (T) { Out.Add(T); } }
	return Out;
}

void ATDEnemyShip::LaunchFightersFromBay(ATDCarrierBay* Bay, int32 Count)
{
	UWorld* World = GetWorld();
	if (!World || !Bay || !Bay->IsAlive() || bDying || Spec.LaunchedEnemyId.IsNone())
	{
		return;
	}

	LaunchedAlive.RemoveAll([](const TWeakObjectPtr<ATDEnemyShip>& S) { return !S.IsValid() || S->IsDying(); });

	TSubclassOf<ATDEnemyShip> Class = LaunchedShipClass ? LaunchedShipClass : TSubclassOf<ATDEnemyShip>(ATDEnemyShip::StaticClass());
	const FVector Out = (Bay->GetActorLocation() - GetActorLocation()).GetSafeNormal(UE_SMALL_NUMBER, GetActorRightVector());
	const FRotator Facing = (Out + GetActorForwardVector() * 0.5f).Rotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATDEnemyShip* Leader = nullptr;
	for (int32 i = 0; i < Count; ++i)
	{
		if (MaxLaunchedAlive > 0 && LaunchedAlive.Num() >= MaxLaunchedAlive)
		{
			break;
		}
		const FVector Loc = Bay->GetActorLocation() + Out * (400.f + 350.f * i) + GetActorUpVector() * (i - (Count - 1) * 0.5f) * 300.f;
		ATDEnemyShip* Ship = World->SpawnActor<ATDEnemyShip>(Class, Loc, Facing, Params);
		if (!Ship) { continue; }
		Ship->InitFromSpec(Spec.LaunchedEnemyId);
		Ship->OnEnemyDied.AddUniqueDynamic(this, &ATDEnemyShip::HandleLaunchedDied);
		LaunchedAlive.Add(Ship);

		if (!Leader)
		{
			Leader = Ship;
		}
		else
		{
			Ship->JoinFormation(Leader, FVector(-500.f * i, (i % 2 ? -1.f : 1.f) * 600.f * ((i + 1) / 2), 0.f));
		}
		if (ATDWaveDirector* Director = ATDWaveDirector::Get(this))
		{
			Director->RegisterEnemy(Ship);	// launched fighters count toward the wave
		}
		OnEnemyLaunched.Broadcast(this, Ship);
	}
}

void ATDEnemyShip::HandleLaunchedDied(ATDEnemyShip* Ship, AActor* /*Killer*/)
{
	LaunchedAlive.Remove(Ship);
}

// -------------------------------------------------------------------------------------------- death

void ATDEnemyShip::HandleHealthDeath(UTDHealthComponent* /*InHealth*/, const FTDDamageInfo& KillingBlow)
{
	BeginDeath(KillingBlow.Instigator);
}

void ATDEnemyShip::BeginDeath(AActor* Killer)
{
	if (bDying)
	{
		return;
	}
	bDying = true;
	LastKiller = Killer;
	// Weapons first: a wreck launches nothing (a gunship killed after missile 1 of a salvo fired 2 and 3).
	if (Weapons) { Weapons->SetWeaponsBlocked(TEXT("Death"), true); }
	BurstShotsRemaining = 0;
	CancelTelegraph();
	bGunsFiring = false;
	ApplyPrimaryTrigger();
	if (Weapons)
	{
		Weapons->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
	}
	DestroyCarrierParts();
	// Capped so a big hulk explodes in view (T2's death captures: carriers tumbled 2.5 s past the camera).
	DeathTimer = FMath::Min(Spec.DeathTumbleSeconds, MaxDeathTumbleSeconds);
	// The hulk drifts on its own velocity, never toward the player's view: the flight model lets go of it.
	{
		DeathDrift = GetVelocity();
		if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager)
		{
			const FVector ToCam = (PC->PlayerCameraManager->GetCameraLocation() - GetActorLocation()).GetSafeNormal();
			const float Toward = FVector::DotProduct(DeathDrift, ToCam);
			if (Toward > 0.f) { DeathDrift -= ToCam * Toward; }	// strip the part heading at the camera
			DeathDrift -= ToCam * DeathDriftAwaySpeed;				// and ease away from it
		}
		DeathDrift = DeathDrift.GetClampedToMaxSize(6000.f);
		DeathSpin = FRotator(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(90.f, 160.f) * (FMath::RandBool() ? 1.f : -1.f));
		if (UTDShipMovementComponent* Move = GetShipMovement()) { Move->SetComponentTickEnabled(false); }
	}

	// The tumbling wreck is scenery: ships fly through it (no 60-hull ram from a dead fighter).
	if (USphereComponent* Sphere = GetCollisionSphere())
	{
		Sphere->SetCollisionResponseToChannel(TDCollision::Ship, ECR_Ignore);
	}
	ClearSpeedOverrideFor(TEXT("EnemyAI"));
	if (EngineTrailFX) { EngineTrailFX->Deactivate(); EngineTrailFX = nullptr; }
	if (Visuals)
	{
		UTDStatics::PlayCueAttached(Visuals->DeathTumble, GetHullMesh());
	}

	OnEnemyDied.Broadcast(this, Killer);

	// CarrierKill money shot: UTDDirectorSubsystem picks it up from the carrier tag.

	if (DeathTimer <= 0.f)
	{
		Explode();
	}
}

void ATDEnemyShip::TickDeath(float DeltaSeconds)
{
	if (bExploded)
	{
		return;
	}
	DeathTimer -= DeltaSeconds;
	if (DeathTimer <= 0.f)
	{
		Explode();
	}
}

void ATDEnemyShip::Explode()
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;
	if (Visuals)
	{
		UTDStatics::PlayCue(this, Visuals->Explosion, GetActorLocation(), GetActorRotation());
	}
	// No explosion art yet: T2's pooled placeholder burst, sized to the visible hull (enemies must not vanish).
	{
		FVector Origin, Extent;
		GetActorBounds(true, Origin, Extent);
		const float VisibleRadius = FMath::Max(Spec.Radius, Extent.GetMax());
		const bool bBig = Spec.Size == ETDEnemySize::Large || Spec.Size == ETDEnemySize::Boss;
		if (!Visuals || !FTDCombatPlaceholders::HasVisualArt(Visuals->Explosion))
	{
		if (UTDProjectileSubsystem* Booms = UTDProjectileSubsystem::Get(this))
		{
			Booms->PlayPlaceholderExplosion(GetActorLocation(), FMath::Max(400.f, VisibleRadius * 2.5f), Spec.PlaceholderColor, bBig ? 14 : 8);
		}
	}
	}
	OnEnemyExploded.Broadcast(this, LastKiller.Get());
	Destroy();
}

float ATDEnemyShip::GetGunFireRange(const AActor* Target) const
{
	if (!Spec.Guns.IsValid())
	{
		return 0.f;
	}
	float Closing = 0.f;
	if (Target)
	{
		const FVector Dir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		Closing = FMath::Max(0.f, FVector::DotProduct(GetVelocity() - Target->GetVelocity(), Dir));	// both ships' closing speed
	}
	// A target closing at V meets the bolts (about 30 000 cm/s, 20 000 cm range) well before gun range, and a
	// head-on pass at 12 000-16 000 cm/s otherwise leaves room for one shot: open fire earlier the faster it closes.
	return FMath::Min(Spec.Guns.Range * (1.f + 2.f * Closing / 30000.f), Spec.Guns.Range * 2.2f);	// bolts still reach at 2.2x
}

void ATDEnemyShip::GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const
{
	CollisionRadius = Spec.Radius;
	if (Spec.Role == ETDEnemyRole::Carrier && (Bays.Num() > 0 || Turrets.Num() > 0))
	{
		CollisionRadius = Spec.Radius + CarrierPartClearance + 2.f * 300.f;	// parts + their extent
	}
	CollisionHalfHeight = CollisionRadius;
}

float ATDEnemyShip::GetDamage01() const
{
	const UTDHealthComponent* H = GetHealth();
	return H ? 1.f - H->GetHull01() : 0.f;
}

// --------------------------------------------------------------------------------------------- tick

void ATDEnemyShip::TickTimers(float DeltaSeconds)
{
	for (TPair<ETDEnemyAttack, float>& Pair : CooldownRemaining)
	{
		Pair.Value = FMath::Max(0.f, Pair.Value - DeltaSeconds);
	}

	if (StunRemaining > 0.f)
	{
		StunRemaining -= DeltaSeconds;
		if (StunRemaining <= 0.f)
		{
			StunRemaining = 0.f;
			SetBaysPaused(false);
		}
	}
	TrackingLostRemaining = FMath::Max(0.f, TrackingLostRemaining - DeltaSeconds);
	FireSuppressedRemaining = FMath::Max(0.f, FireSuppressedRemaining - DeltaSeconds);

	if (FrontalShieldDownRemaining > 0.f)
	{
		FrontalShieldDownRemaining -= DeltaSeconds;
		if (FrontalShieldDownRemaining <= 0.f)
		{
			FrontalShieldDownRemaining = 0.f;
			if (IsFrontalShieldUp())
			{
				if (Visuals) { UTDStatics::PlayCueAttached(Visuals->FrontalShieldRestore, GetHullMesh()); }
				OnFrontalShieldRestored.Broadcast(this);
			}
		}
	}
	if (FrontalShieldRecoverRemaining > 0.f)
	{
		FrontalShieldRecoverRemaining -= DeltaSeconds;
		if (FrontalShieldRecoverRemaining <= 0.f)
		{
			FrontalShieldRecoverRemaining = 0.f;
			FrontalShieldHP = Spec.FrontalShield;
			if (IsFrontalShieldUp())
			{
				if (Visuals) { UTDStatics::PlayCueAttached(Visuals->FrontalShieldRestore, GetHullMesh()); }
				OnFrontalShieldRestored.Broadcast(this);
			}
		}
	}
}

float ATDEnemyShip::RangeScaleFor(float DistanceCm, float NearCm, float FarCm, float MaxScale)
{
	if (FarCm <= NearCm) { return DistanceCm > NearCm ? MaxScale : 1.f; }
	const float T = FMath::Clamp((DistanceCm - NearCm) / (FarCm - NearCm), 0.f, 1.f);
	return FMath::Lerp(1.f, FMath::Max(1.f, MaxScale), T);
}

void ATDEnemyShip::ApplyVisualScale()
{
	// HullPivot sits at the actor centre and carries only visuals (no collision): scaling it scales about the
	// aim point and never touches the collision sphere or the mounted hit parts.
	if (HullPivot) { HullPivot->SetRelativeScale3D(BaseVisualScale * RangeScale); }
	AppliedRangeScale = RangeScale;
	if (EngineGlowMID && bPlaceholderLook)
	{
		const float T = RangeScaleMax > 1.f ? (RangeScale - 1.f) / (RangeScaleMax - 1.f) : 0.f;
		const FLinearColor Glow = EngineGlowBase * FMath::Lerp(1.f, RangeEngineGlowMax, FMath::Clamp(T, 0.f, 1.f));
		EngineGlowMID->SetVectorParameterValue(TEXT("Color"), Glow);
		EngineGlowMID->SetVectorParameterValue(TEXT("EmissiveColor"), Glow);
	}
}

void ATDEnemyShip::UpdateRangeScale(const FVector& ViewLocation, bool bAllowed, float RealDeltaSeconds)
{
	const float Target = (bAllowed && !bHasMountedParts)
		? RangeScaleFor(FVector::Dist(ViewLocation, GetActorLocation()), RangeScaleNearCm, RangeScaleFarCm, RangeScaleMax) : 1.f;
	RangeScale = FMath::FInterpTo(RangeScale, Target, FMath::Max(0.f, RealDeltaSeconds), RangeScaleSmoothing);
	if (FMath::Abs(RangeScale - AppliedRangeScale) > 0.004f || (RangeScale == 1.f && AppliedRangeScale != 1.f))
	{
		ApplyVisualScale();
	}
}

void ATDEnemyShip::TickRangeScale(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) { return; }
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager) { return; }
	// True scale whenever the camera is not the player's own chase cam (T3's intro and showcase cameras) or a
	// director money shot is running.
	const UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(this);
	const bool bAllowed = PC->GetViewTarget() == PC->GetPawn() && !(Director && Director->IsShotActive());
	const float Dilation = FMath::Max(0.01f, World->GetWorldSettings() ? World->GetWorldSettings()->GetEffectiveTimeDilation() : 1.f);
	UpdateRangeScale(PC->PlayerCameraManager->GetCameraLocation(), bAllowed, DeltaSeconds / Dilation);
}

void ATDEnemyShip::Tick(float DeltaSeconds)
{
	if (bDying)
	{
		// The hulk drifts and spins on its own (the flight model is off): never toward the player's view.
		Super::Tick(DeltaSeconds);
		DeathDrift *= FMath::Exp(-0.6f * DeltaSeconds);	// bleed speed
		AddActorWorldOffset(DeathDrift * DeltaSeconds, false);
		AddActorLocalRotation(DeathSpin * DeltaSeconds);
		// Too close to (or behind) the camera to read: explode now, in view.
		if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager && !bExploded)
		{
			const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
			const FVector Fwd = PC->PlayerCameraManager->GetCameraRotation().Vector();
			const FVector ToHulk = GetActorLocation() - Cam;
			if (ToHulk.Size() < FMath::Max(4000.f, Spec.Radius * 3.f) || FVector::DotProduct(ToHulk, Fwd) < 0.f) { DeathTimer = 0.f; }
		}
		TickDeath(DeltaSeconds);
		return;
	}

	if (IsStunned())
	{
		// Dead stick: coast, no steering. The ship is authoritative over the AI here.
		FTDFlightInput Dead;
		Dead.Throttle = -0.5f;
		SetFlightInput(Dead);
	}

	Super::Tick(DeltaSeconds);

	TickTimers(DeltaSeconds);
	TickRangeScale(DeltaSeconds);

	if (bPlaceholderLook && PlaceholderShieldPlate && HasFrontalShield())
	{
		const bool bUp = IsFrontalShieldUp();
		if (PlaceholderShieldPlate->IsVisible() != bUp) { PlaceholderShieldPlate->SetVisibility(bUp); }
	}

	if (IsTelegraphing())
	{
		if (!CanFire())
		{
			CancelTelegraph();
		}
		else
		{
			TelegraphElapsed += DeltaSeconds;
			if (TelegraphElapsed >= TelegraphDuration)
			{
				const ETDEnemyAttack Attack = TelegraphAttack;
				AActor* Target = TelegraphTarget.Get();
				TelegraphAttack = ETDEnemyAttack::None;
				TelegraphElapsed = 0.f;
				TelegraphTarget.Reset();
				ExecuteAttack(Attack, Target);
			}
		}
	}

	TickBurst(DeltaSeconds);
	ApplyPrimaryTrigger();

	if (bPrimaryTriggerHeld && Weapons)
	{
		FVector Dir = GetActorForwardVector();
		if (const AActor* Target = GunTarget.Get())
		{
			// Lead the target, then add the spec's aim error so enemies are beatable.
			const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
			const float Time = ToTarget.Size() / FMath::Max(1000.f, GunLeadSpeed);
			const FVector Aim = (ToTarget + Target->GetVelocity() * Time).GetSafeNormal();
			if (!Aim.IsNearlyZero()) { Dir = Aim; }
		}
		if (Spec.AimErrorDeg > 0.f)
		{
			// The error is an angle, capped as a miss distance: far shots spread, a pass at close range
			// still lands some bolts (head-on passes were a coin flip between 1 hit and none).
			float ErrorDeg = Spec.AimErrorDeg;
			if (const AActor* Target = GunTarget.Get())
			{
				const float Dist = FMath::Max(100.f, FVector::Dist(Target->GetActorLocation(), GetActorLocation()));
				ErrorDeg = FMath::Min(ErrorDeg, FMath::RadiansToDegrees(FMath::Atan(AimMaxMissCm / Dist)));
			}
			Dir = AimRng.VRandCone(Dir, FMath::DegreesToRadians(ErrorDeg));
		}
		Weapons->SetAimDirection(Dir);
	}
}
