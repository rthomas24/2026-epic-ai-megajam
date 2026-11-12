// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDEnemyShip.h"

#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Flight/TDFlightTypes.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

ATDEnemyShip::ATDEnemyShip()
{
	PrimaryActorTick.bCanEverTick = true;

	// Weapons come from ATDSpacecraft's "Weapons" subobject; EnsureWeapons() is a null-safe fallback.

	AIControllerClass = ATDEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Team = ETDTeam::Enemy;
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

	SetTeam(ETDTeam::Enemy);

	if (UTDHealthComponent* H = GetHealth())
	{
		H->DefaultTeam = ETDTeam::Enemy;
		H->InitMaxValues(Spec.Hull, Spec.Shield, true);
	}

	FTDFlightTuning Tuning;
	Tuning.CruiseSpeed = Spec.CruiseSpeed;
	Tuning.MinSpeed = Spec.CruiseSpeed * 0.3f;
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

	UpdatePlaceholderLook();

	if (HasActorBegunPlay())
	{
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

void ATDEnemyShip::BeginPlay()
{
	Super::BeginPlay();
	EnsureWeapons();

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
			if (PlaceholderWings) { PlaceholderWings->SetVisibility(false); }
			if (HullPivot) { HullPivot->SetRelativeScale3D(FVector::OneVector); }
			return;
		}
	}

	// Placeholder: T1's cone + wings, scaled as one by the gameplay radius (220 cm = default) and tinted.
	if (PlaceholderWings) { PlaceholderWings->SetVisibility(true); }
	if (HullPivot)
	{
		HullPivot->SetRelativeScale3D(FVector(FMath::Max(0.2f, Spec.Radius / 220.f)));
	}
	for (UStaticMeshComponent* Part : { Hull, PlaceholderWings.Get() })
	{
		if (UMaterialInstanceDynamic* MID = Part ? Part->CreateDynamicMaterialInstance(0) : nullptr)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Spec.PlaceholderColor);
		}
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
			UTDStatics::PlayCueAttached(Visuals->Telegraph, GetHullMesh());
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
	Weapons->FireOnce(ETDWeaponSlot::Secondary);
	Weapons->SetAimTarget(nullptr);

	// Counted per attempt, so tests don't depend on weapon internals.
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
	CancelTelegraph();
	bGunsFiring = false;
	ApplyPrimaryTrigger();
	if (Weapons)
	{
		Weapons->SetTriggerHeld(ETDWeaponSlot::Secondary, false);
	}
	DestroyCarrierParts();
	DeathTimer = Spec.DeathTumbleSeconds;

	OnEnemyDied.Broadcast(this, Killer);

	if (Spec.Role == ETDEnemyRole::Carrier && Spec.KillImportance > 0.f)
	{
		if (UTDEventBus* Bus = UTDEventBus::Get(this))
		{
			Bus->BroadcastHighlightMoment(ETDHighlightKind::CarrierKill, this, Spec.KillImportance);
		}
	}

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
	OnEnemyExploded.Broadcast(this, LastKiller.Get());
	Destroy();
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
			if (IsFrontalShieldUp()) { OnFrontalShieldRestored.Broadcast(this); }
		}
	}
	if (FrontalShieldRecoverRemaining > 0.f)
	{
		FrontalShieldRecoverRemaining -= DeltaSeconds;
		if (FrontalShieldRecoverRemaining <= 0.f)
		{
			FrontalShieldRecoverRemaining = 0.f;
			FrontalShieldHP = Spec.FrontalShield;
			if (IsFrontalShieldUp()) { OnFrontalShieldRestored.Broadcast(this); }
		}
	}
}

void ATDEnemyShip::Tick(float DeltaSeconds)
{
	if (bDying)
	{
		// Tumble: spin out, bleed speed.
		FTDFlightInput Tumble;
		Tumble.Throttle = -1.f;
		Tumble.Aim = FVector2D(0.35f, 0.8f);
		Tumble.Roll = 1.f;
		SetFlightInput(Tumble);
		Super::Tick(DeltaSeconds);
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
			Dir = AimRng.VRandCone(Dir, FMath::DegreesToRadians(Spec.AimErrorDeg));
		}
		Weapons->SetAimDirection(Dir);
	}
}
