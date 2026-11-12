// Triad Drift - Combat (Teammate 2). Weapon slots: fire rate, heat, ammo, charge, salvos, spread, aim assist.

#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"

namespace
{
	const FTDWeaponSpec& InvalidSpec()
	{
		static const FTDWeaponSpec Invalid;
		return Invalid;
	}

	const FName& MuzzleSocketName(int32 Index)
	{
		static const FName Names[] = {
			FName(TEXT("Muzzle_0")), FName(TEXT("Muzzle_1")), FName(TEXT("Muzzle_2")), FName(TEXT("Muzzle_3")),
			FName(TEXT("Muzzle_4")), FName(TEXT("Muzzle_5")), FName(TEXT("Muzzle_6")), FName(TEXT("Muzzle_7")) };
		return Names[FMath::Clamp(Index, 0, 7)];
	}

	constexpr int32 MaxMuzzleSockets = 8;
	/** Safety cap on shots per slot per frame (a 20/s gun at a 0.25 s hitch fires 5). */
	constexpr int32 MaxShotsPerFrame = 12;
}

UTDWeaponComponent::UTDWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	MuzzleOffsets = { FVector(150.f, -60.f, 0.f), FVector(150.f, 60.f, 0.f) };
	Random.Initialize(0x7D2A);
}

void UTDWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	Random.GenerateNewSeed();
}

// ------------------------------------------------------------------------------------------ slots

FTDWeaponSlotState& UTDWeaponComponent::State(ETDWeaponSlot Slot)
{
	return Slot == ETDWeaponSlot::Primary ? PrimaryState : SecondaryState;
}

const FTDWeaponSlotState& UTDWeaponComponent::State(ETDWeaponSlot Slot) const
{
	return Slot == ETDWeaponSlot::Primary ? PrimaryState : SecondaryState;
}

const FTDWeaponSpec*& UTDWeaponComponent::SpecPtr(ETDWeaponSlot Slot)
{
	return Slot == ETDWeaponSlot::Primary ? PrimarySpec : SecondarySpec;
}

const FTDWeaponSpec* UTDWeaponComponent::SpecPtr(ETDWeaponSlot Slot) const
{
	return Slot == ETDWeaponSlot::Primary ? PrimarySpec : SecondarySpec;
}

void UTDWeaponComponent::EquipWeapon(ETDWeaponSlot Slot, FName WeaponId)
{
	StopChargeFX(Slot);
	const FTDWeaponSpec& Spec = UTDWeaponLibrary::Get(WeaponId);
	SpecPtr(Slot) = Spec.IsValid() ? &Spec : nullptr;

	FTDWeaponSlotState& S = State(Slot);
	const bool bWasHeld = S.bTriggerHeld;
	S = FTDWeaponSlotState();
	S.WeaponId = Spec.IsValid() ? WeaponId : NAME_None;
	S.Ammo = Spec.IsValid() ? Spec.MaxAmmo : 0;
	S.bTriggerHeld = bWasHeld && Spec.IsValid() && !AreWeaponsBlocked();
}

const FTDWeaponSpec& UTDWeaponComponent::GetSpec(ETDWeaponSlot Slot) const
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	return Spec ? *Spec : InvalidSpec();
}

const FTDWeaponSlotState& UTDWeaponComponent::GetSlotState(ETDWeaponSlot Slot) const
{
	return State(Slot);
}

FName UTDWeaponComponent::GetWeaponId(ETDWeaponSlot Slot) const
{
	return State(Slot).WeaponId;
}

// ------------------------------------------------------------------------------------------ input

void UTDWeaponComponent::SetTriggerHeld(ETDWeaponSlot Slot, bool bHeld)
{
	FTDWeaponSlotState& S = State(Slot);
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	const bool bNewHeld = bHeld && Spec && !AreWeaponsBlocked();
	if (S.bTriggerHeld == bNewHeld)
	{
		return;
	}
	S.bTriggerHeld = bNewHeld;

	if (!bNewHeld && Spec && Spec->FireMode == ETDFireMode::Charge)
	{
		// Charge weapons fire on release, only when fully charged.
		const bool bFire = S.bChargeReady && HasResourceToFire(*Spec, S) && !S.bOverheated && !AreWeaponsBlocked();
		S.ChargeHeld = 0.f;
		S.bChargeReady = false;
		StopChargeFX(Slot);
		if (bFire)
		{
			BeginShot(Slot, 0.f);
			S.RefireTimer = Spec->GetRefireSeconds();
		}
	}
}

void UTDWeaponComponent::SetAimTarget(AActor* Target)
{
	AimTarget = Target;
}

void UTDWeaponComponent::SetAimDirection(const FVector& WorldDir)
{
	AimDirection = WorldDir.GetSafeNormal();
}

FVector UTDWeaponComponent::GetAimDirection() const
{
	if (!AimDirection.IsNearlyZero())
	{
		return AimDirection;
	}
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
}

AActor* UTDWeaponComponent::GetAimTarget() const
{
	return AimTarget.Get();
}

// ------------------------------------------------------------------------------------------ queries

float UTDWeaponComponent::GetHeat01(ETDWeaponSlot Slot) const
{
	return FMath::Clamp(State(Slot).Heat, 0.f, 1.f);
}

int32 UTDWeaponComponent::GetAmmo(ETDWeaponSlot Slot) const
{
	return State(Slot).Ammo;
}

int32 UTDWeaponComponent::GetMaxAmmo(ETDWeaponSlot Slot) const
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	return Spec ? Spec->MaxAmmo : 0;
}

bool UTDWeaponComponent::IsOverheated(ETDWeaponSlot Slot) const
{
	return State(Slot).bOverheated;
}

float UTDWeaponComponent::GetCharge01(ETDWeaponSlot Slot) const
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	if (!Spec || Spec->FireMode != ETDFireMode::Charge || Spec->ChargeSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(State(Slot).ChargeHeld / Spec->ChargeSeconds, 0.f, 1.f);
}

bool UTDWeaponComponent::CanFire(ETDWeaponSlot Slot) const
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	const FTDWeaponSlotState& S = State(Slot);
	return Spec && !AreWeaponsBlocked() && !S.bOverheated && HasResourceToFire(*Spec, S) && S.RefireTimer <= 0.f;
}

bool UTDWeaponComponent::HasResourceToFire(const FTDWeaponSpec& Spec, const FTDWeaponSlotState& S) const
{
	switch (Spec.Resource)
	{
	case ETDWeaponResource::Ammo: return S.Ammo > 0;
	case ETDWeaponResource::Heat: return !S.bOverheated;
	default: return true;
	}
}

// ------------------------------------------------------------------------------------------ multipliers / blocking

void UTDWeaponComponent::SetOutgoingDamageMultiplier(FName SourceKey, float Multiplier)
{
	if (FMath::IsNearlyEqual(Multiplier, 1.f))
	{
		DamageMultipliers.Remove(SourceKey);
	}
	else
	{
		DamageMultipliers.Add(SourceKey, FMath::Max(Multiplier, 0.f));
	}
}

void UTDWeaponComponent::ClearOutgoingDamageMultiplier(FName SourceKey)
{
	DamageMultipliers.Remove(SourceKey);
}

void UTDWeaponComponent::SetNextShotDamageMultiplier(float Multiplier)
{
	NextShotMultiplier = FMath::Max(Multiplier, 0.f);
}

float UTDWeaponComponent::GetOutgoingDamageMultiplier() const
{
	float Product = 1.f;
	for (const TPair<FName, float>& Pair : DamageMultipliers)
	{
		Product *= Pair.Value;
	}
	return Product;
}

void UTDWeaponComponent::SetWeaponsBlocked(FName SourceKey, bool bBlocked)
{
	if (bBlocked)
	{
		BlockSources.Add(SourceKey);
		for (ETDWeaponSlot Slot : { ETDWeaponSlot::Primary, ETDWeaponSlot::Secondary })
		{
			FTDWeaponSlotState& S = State(Slot);
			S.bTriggerHeld = false;
			S.ChargeHeld = 0.f;
			S.bChargeReady = false;
			S.SalvoRemaining = 0;
			StopChargeFX(Slot);
		}
	}
	else
	{
		BlockSources.Remove(SourceKey);
	}
}

void UTDWeaponComponent::ResetWeapons()
{
	for (ETDWeaponSlot Slot : { ETDWeaponSlot::Primary, ETDWeaponSlot::Secondary })
	{
		const FTDWeaponSpec* Spec = SpecPtr(Slot);
		FTDWeaponSlotState& S = State(Slot);
		const bool bWasOverheated = S.bOverheated;
		S.Heat = 0.f;
		S.bOverheated = false;
		S.OverheatRemaining = 0.f;
		S.Ammo = Spec ? Spec->MaxAmmo : 0;
		S.AmmoRegenProgress = 0.f;
		S.ChargeHeld = 0.f;
		S.bChargeReady = false;
		S.SalvoRemaining = 0;
		S.RefireTimer = 0.f;
		StopChargeFX(Slot);
		if (bWasOverheated)
		{
			OnOverheatChanged.Broadcast(Slot, false);
		}
	}
}

// ------------------------------------------------------------------------------------------ tick

void UTDWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickSlot(ETDWeaponSlot::Primary, DeltaTime);
	TickSlot(ETDWeaponSlot::Secondary, DeltaTime);
}

void UTDWeaponComponent::TickSlot(ETDWeaponSlot Slot, float DeltaTime)
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	if (!Spec || DeltaTime <= 0.f)
	{
		return;
	}
	FTDWeaponSlotState& S = State(Slot);
	S.TimeSinceShot += DeltaTime;

	// Ammo regeneration.
	if (Spec->Resource == ETDWeaponResource::Ammo && Spec->AmmoRegenSeconds > 0.f)
	{
		if (S.Ammo < Spec->MaxAmmo)
		{
			S.AmmoRegenProgress += DeltaTime;
			while (S.AmmoRegenProgress >= Spec->AmmoRegenSeconds && S.Ammo < Spec->MaxAmmo)
			{
				S.AmmoRegenProgress -= Spec->AmmoRegenSeconds;
				++S.Ammo;
			}
		}
		if (S.Ammo >= Spec->MaxAmmo)
		{
			S.AmmoRegenProgress = 0.f;
		}
	}

	// Heat: lockout drains to zero; otherwise cool once the trigger is released.
	if (Spec->Resource == ETDWeaponResource::Heat)
	{
		if (S.bOverheated)
		{
			S.OverheatRemaining -= DeltaTime;
			const float Lockout = FMath::Max(Spec->OverheatLockoutSeconds, KINDA_SMALL_NUMBER);
			S.Heat = FMath::Clamp(S.OverheatRemaining / Lockout, 0.f, 1.f);
			if (S.OverheatRemaining <= 0.f)
			{
				S.bOverheated = false;
				S.OverheatRemaining = 0.f;
				S.Heat = 0.f;
				OnOverheatChanged.Broadcast(Slot, false);
			}
		}
		else if (!S.bTriggerHeld && S.TimeSinceShot >= Spec->HeatCoolDelay && S.Heat > 0.f)
		{
			S.Heat = FMath::Max(0.f, S.Heat - DeltaTime / FMath::Max(Spec->HeatSecondsToCool, KINDA_SMALL_NUMBER));
		}
	}

	// Remaining projectiles of a rippled salvo.
	if (S.SalvoRemaining > 0)
	{
		S.SalvoTimer -= DeltaTime;
		int32 Guard = 0;
		while (S.SalvoRemaining > 0 && S.SalvoTimer <= 0.f && Guard++ < MaxShotsPerFrame)
		{
			SpawnProjectile(Slot, -S.SalvoTimer);
			--S.SalvoRemaining;
			S.SalvoTimer += FMath::Max(Spec->SalvoInterval, KINDA_SMALL_NUMBER);
		}
	}

	S.RefireTimer -= DeltaTime;
	const bool bCanShoot = !AreWeaponsBlocked() && !S.bOverheated;

	if (Spec->FireMode == ETDFireMode::Automatic)
	{
		int32 Guard = 0;
		while (S.bTriggerHeld && bCanShoot && !S.bOverheated && S.RefireTimer <= 0.f && HasResourceToFire(*Spec, S) && Guard++ < MaxShotsPerFrame)
		{
			// -RefireTimer = how late this shot is inside the frame; the projectile is advanced by that much.
			BeginShot(Slot, FMath::Min(-S.RefireTimer, DeltaTime));
			S.RefireTimer += Spec->GetRefireSeconds();
		}
	}
	else if (Spec->FireMode == ETDFireMode::Charge)
	{
		if (S.bTriggerHeld && bCanShoot && S.RefireTimer <= 0.f && HasResourceToFire(*Spec, S))
		{
			if (S.ChargeHeld <= 0.f)
			{
				if (UTDWeaponFX* FX = GetFX(Slot))
				{
					if (AActor* Owner = GetOwner())
					{
						UNiagaraComponent* Loop = UTDStatics::PlayCueAttached(FX->ChargeLoop, Owner->GetRootComponent());
						(Slot == ETDWeaponSlot::Primary ? PrimaryChargeFX : SecondaryChargeFX) = Loop;
					}
				}
			}
			S.ChargeHeld += DeltaTime;
			S.bChargeReady = S.ChargeHeld >= Spec->ChargeSeconds;
		}
	}

	if (!S.bTriggerHeld || !bCanShoot)
	{
		S.RefireTimer = FMath::Max(S.RefireTimer, 0.f);
	}
}

// ------------------------------------------------------------------------------------------ firing

bool UTDWeaponComponent::FireOnce(ETDWeaponSlot Slot)
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	FTDWeaponSlotState& S = State(Slot);
	if (!Spec || AreWeaponsBlocked() || S.bOverheated || !HasResourceToFire(*Spec, S))
	{
		return false;
	}
	BeginShot(Slot, 0.f);
	S.RefireTimer = FMath::Max(S.RefireTimer, Spec->GetRefireSeconds());
	return true;
}

void UTDWeaponComponent::BeginShot(ETDWeaponSlot Slot, float SubFrameAge)
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	if (!Spec)
	{
		return;
	}
	FTDWeaponSlotState& S = State(Slot);

	if (Spec->Resource == ETDWeaponResource::Ammo)
	{
		S.Ammo = FMath::Max(0, S.Ammo - 1);
	}
	S.TimeSinceShot = SubFrameAge;
	++S.ShotsFired;
	S.ShotMultiplier = NextShotMultiplier;
	NextShotMultiplier = 1.f;
	LastFired.Reset();

	const int32 Count = FMath::Max(1, Spec->ProjectilesPerShot);
	if (Spec->SalvoInterval <= 0.f || Count == 1)
	{
		for (int32 i = 0; i < Count; ++i)
		{
			SpawnProjectile(Slot, SubFrameAge);
		}
		S.SalvoRemaining = 0;
	}
	else
	{
		SpawnProjectile(Slot, SubFrameAge);
		S.SalvoRemaining = Count - 1;
		S.SalvoTimer = Spec->SalvoInterval - SubFrameAge;
	}

	if (Spec->Resource == ETDWeaponResource::Heat)
	{
		AddHeat(Slot, Spec->GetHeatPerShot());
	}

	OnWeaponFired.Broadcast(Slot, S.WeaponId);
}

void UTDWeaponComponent::AddHeat(ETDWeaponSlot Slot, float Amount)
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	FTDWeaponSlotState& S = State(Slot);
	if (!Spec || S.bOverheated)
	{
		return;
	}
	S.Heat += Amount;
	// Tolerance: N shots of 1/N must reach exactly full despite float error.
	if (S.Heat >= 1.f - 1e-4f)
	{
		S.Heat = 1.f;
		S.bOverheated = true;
		S.OverheatRemaining = Spec->OverheatLockoutSeconds;
		S.ChargeHeld = 0.f;
		S.bChargeReady = false;
		OnOverheatChanged.Broadcast(Slot, true);
		if (UTDWeaponFX* FX = GetFX(Slot))
		{
			if (AActor* Owner = GetOwner())
			{
				UTDStatics::PlayCue(this, FX->Overheat, Owner->GetActorLocation(), Owner->GetActorRotation());
			}
		}
	}
}

void UTDWeaponComponent::SpawnProjectile(ETDWeaponSlot Slot, float SubFrameAge)
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Spec || !Owner || !World)
	{
		return;
	}
	FTDWeaponSlotState& S = State(Slot);

	GetMuzzleTransforms(MuzzleScratch);
	FTransform Muzzle = Owner->GetActorTransform();
	if (MuzzleScratch.Num() > 0)
	{
		const int32 Index = Spec->bAlternateMuzzles ? (S.MuzzleCursor++ % MuzzleScratch.Num()) : 0;
		Muzzle = MuzzleScratch[Index];
	}
	const FVector MuzzleLocation = Muzzle.GetLocation();
	const FVector Dir = ComputeFireDirection(*Spec, MuzzleLocation);

	FTDProjectileSpec ProjSpec = Spec->Projectile;
	ProjSpec.FX = GetFX(Slot);

	AActor* HomingTarget = ProjSpec.bHoming ? ResolveAimTarget() : nullptr;
	const float Multiplier = GetOutgoingDamageMultiplier() * S.ShotMultiplier;

	ATDProjectile* P = ATDProjectile::Launch(World, ProjSpec, FTransform(Dir.Rotation(), MuzzleLocation), Owner, GetOwnerTeam(), HomingTarget,
		Owner->GetVelocity(), Multiplier, ShouldReportHits());
	if (!P)
	{
		return;
	}
	LastFired.Add(P);
	if (SubFrameAge > 0.f)
	{
		P->Step(SubFrameAge);
	}

	if (ProjSpec.FX)
	{
		UTDStatics::PlayCue(this, ProjSpec.FX->Muzzle, MuzzleLocation, Dir.Rotation());
	}
}

FVector UTDWeaponComponent::ComputeFireDirection(const FTDWeaponSpec& Spec, const FVector& MuzzleLocation)
{
	FVector Dir = GetAimDirection();

	if (Spec.AimAssistDeg > 0.f)
	{
		AActor* Target = ResolveAimTarget();
		const UTDHealthComponent* Health = Target ? UTDStatics::GetHealth(Target) : nullptr;
		if (Target && (!Health || Health->IsAlive()))
		{
			const AActor* Owner = GetOwner();
			const FVector ShooterVel = (Owner && Spec.Projectile.bInheritShooterVelocity) ? Owner->GetVelocity() : FVector::ZeroVector;
			const FVector Lead = UTDTargetingComponent::ComputeInterceptPoint(MuzzleLocation, ShooterVel, Target->GetActorLocation(),
				Target->GetVelocity(), Spec.Projectile.Speed);
			const FVector ToLead = (Lead - MuzzleLocation).GetSafeNormal();
			if (!ToLead.IsNearlyZero() && FVector::DotProduct(ToLead, Dir) >= FMath::Cos(FMath::DegreesToRadians(Spec.AimAssistDeg)))
			{
				Dir = ToLead;
			}
		}
	}

	if (Spec.SpreadDeg > 0.f)
	{
		Dir = Random.VRandCone(Dir, FMath::DegreesToRadians(Spec.SpreadDeg));
	}
	return Dir.GetSafeNormal();
}

AActor* UTDWeaponComponent::ResolveAimTarget() const
{
	if (AActor* Explicit = AimTarget.Get())
	{
		return Explicit;
	}
	const AActor* Owner = GetOwner();
	if (const UTDTargetingComponent* Targeting = Owner ? Owner->FindComponentByClass<UTDTargetingComponent>() : nullptr)
	{
		AActor* Soft = Targeting->GetCurrentTarget();
		if (Soft && FVector::DistSquared(Soft->GetActorLocation(), Owner->GetActorLocation()) <= FMath::Square(AimAssistMaxRange))
		{
			return Soft;
		}
	}
	return nullptr;
}

bool UTDWeaponComponent::ShouldReportHits() const
{
	if (bForceReportHits)
	{
		return true;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsPlayerControlled();
}

ETDTeam UTDWeaponComponent::GetOwnerTeam() const
{
	return UTDStatics::GetTeam(GetOwner());
}

void UTDWeaponComponent::GetMuzzleTransforms(TArray<FTransform>& Out) const
{
	Out.Reset();
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TInlineComponentArray<UStaticMeshComponent*> Meshes(Owner);
	for (const UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->DoesSocketExist(MuzzleSocketName(0)))
		{
			for (int32 i = 0; i < MaxMuzzleSockets && Mesh->DoesSocketExist(MuzzleSocketName(i)); ++i)
			{
				Out.Add(Mesh->GetSocketTransform(MuzzleSocketName(i)));
			}
			return;
		}
	}

	const FTransform OwnerXf = Owner->GetActorTransform();
	for (const FVector& Offset : MuzzleOffsets)
	{
		Out.Add(FTransform(OwnerXf.GetRotation(), OwnerXf.TransformPosition(Offset)));
	}
}

void UTDWeaponComponent::StopChargeFX(ETDWeaponSlot Slot)
{
	TObjectPtr<UNiagaraComponent>& Comp = Slot == ETDWeaponSlot::Primary ? PrimaryChargeFX : SecondaryChargeFX;
	if (Comp)
	{
		Comp->Deactivate();
		Comp = nullptr;
	}
}
