// Triad Drift - Combat (Teammate 2). Weapon slots: fire rate, heat, ammo, charge, salvos, spread, aim assist.

#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Combat/TDCombatCueLog.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "GameFramework/MovementComponent.h"
#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"

namespace
{
	const FTDWeaponSpec& TDWeapInvalidSpec()
	{
		static const FTDWeaponSpec Invalid;
		return Invalid;
	}

	const FName& TDWeapMuzzleSocketName(int32 Index)
	{
		static const FName Names[] = {
			FName(TEXT("Muzzle_0")), FName(TEXT("Muzzle_1")), FName(TEXT("Muzzle_2")), FName(TEXT("Muzzle_3")),
			FName(TEXT("Muzzle_4")), FName(TEXT("Muzzle_5")), FName(TEXT("Muzzle_6")), FName(TEXT("Muzzle_7")) };
		return Names[FMath::Clamp(Index, 0, 7)];
	}

	constexpr int32 TDWeapMaxMuzzleSockets = 8;
	/** Safety cap on shots per slot per frame (a 20/s gun at a 0.25 s hitch fires 5). */
	constexpr int32 TDWeapMaxShotsPerFrame = 12;
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
	// Fire after the ship has moved this frame, so the muzzle is where the ship is (not a frame behind while boosting).
	if (AActor* Owner = GetOwner())
	{
		if (UMovementComponent* Move = Owner->FindComponentByClass<UMovementComponent>())
		{
			AddTickPrerequisiteComponent(Move);
		}
	}
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

	TObjectPtr<UTDWeaponFX>& SlotFX = Slot == ETDWeaponSlot::Primary ? PrimaryFX : SecondaryFX;
	bool& bAuto = Slot == ETDWeaponSlot::Primary ? bPrimaryFXAuto : bSecondaryFXAuto;
	if (!SlotFX || bAuto)
	{
		SlotFX = Spec.IsValid() ? FTDPowerPresentation::LoadWeaponFX(WeaponId) : nullptr;
		bAuto = true;
	}
	// Load the projectile art now, not on the first shot (a synchronous load mid-fight is a hitch).
	if (UTDWeaponFX* FX = SlotFX.Get())
	{
		FX->ProjectileMesh.LoadSynchronous();
		FX->Tracer.LoadSynchronous();
	}

	FTDWeaponSlotState& S = State(Slot);
	const bool bWasRequested = S.bTriggerRequested;
	S = FTDWeaponSlotState();
	S.WeaponId = Spec.IsValid() ? WeaponId : NAME_None;
	S.Ammo = Spec.IsValid() ? Spec.MaxAmmo : 0;
	S.bTriggerRequested = bWasRequested;
	S.bTriggerHeld = bWasRequested && Spec.IsValid() && !AreWeaponsBlocked();
}

const FTDWeaponSpec& UTDWeaponComponent::GetSpec(ETDWeaponSlot Slot) const
{
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	return Spec ? *Spec : TDWeapInvalidSpec();
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
	const bool bWasRequested = S.bTriggerRequested;
	S.bTriggerRequested = bHeld;
	UpdateEffectiveTrigger(Slot, bWasRequested && !bHeld);
}

void UTDWeaponComponent::UpdateEffectiveTrigger(ETDWeaponSlot Slot, bool bPilotRelease)
{
	FTDWeaponSlotState& S = State(Slot);
	const FTDWeaponSpec* Spec = SpecPtr(Slot);
	const bool bBlocked = AreWeaponsBlocked();
	const bool bNewHeld = S.bTriggerRequested && Spec && !bBlocked;
	if (S.bTriggerHeld == bNewHeld && !bPilotRelease)
	{
		return;
	}
	const bool bWasHeld = S.bTriggerHeld;
	S.bTriggerHeld = bNewHeld;

	if (bWasHeld && !bNewHeld && Spec && Spec->FireMode == ETDFireMode::Charge)
	{
		// Charge weapons fire when the PILOT releases a full charge; a block or death just drops it.
		const bool bFire = bPilotRelease && !bBlocked && S.bChargeReady && HasResourceToFire(*Spec, S) && !S.bOverheated;
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

void UTDWeaponComponent::CancelCharge(ETDWeaponSlot Slot)
{
	FTDWeaponSlotState& S = State(Slot);
	S.ChargeHeld = 0.f;
	S.bChargeReady = false;
	S.bTriggerRequested = false;
	S.bTriggerHeld = false;
	StopChargeFX(Slot);
}

bool UTDWeaponComponent::AreWeaponsBlocked() const
{
	if (BlockSources.Num() > 0)
	{
		return true;
	}
	const UTDHealthComponent* Health = UTDStatics::GetHealth(GetOwner());
	return Health && Health->IsDead();
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
			S.SalvoRemaining = 0;
			UpdateEffectiveTrigger(Slot, false); // drops a charge, keeps the request
			S.ChargeHeld = 0.f;
			S.bChargeReady = false;
			StopChargeFX(Slot);
		}
	}
	else
	{
		BlockSources.Remove(SourceKey);
		UpdateEffectiveTrigger(ETDWeaponSlot::Primary, false);
		UpdateEffectiveTrigger(ETDWeaponSlot::Secondary, false);
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

	// Death, blocks ending, etc. (never fires a charge: only a pilot release does).
	UpdateEffectiveTrigger(Slot, false);

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
		// Cooling waits at least 1.5 refire intervals, so tap-firing at the max rate can't dodge heat.
		else if (!S.bTriggerHeld && S.TimeSinceShot >= FMath::Max(Spec->HeatCoolDelay, 1.5f * Spec->GetRefireSeconds()) && S.Heat > 0.f)
		{
			S.Heat = FMath::Max(0.f, S.Heat - DeltaTime / FMath::Max(Spec->HeatSecondsToCool, KINDA_SMALL_NUMBER));
		}
	}

	// Remaining projectiles of a rippled salvo. A dead or blocked shooter launches nothing more from its wreck.
	if (S.SalvoRemaining > 0 && AreWeaponsBlocked())
	{
		S.SalvoRemaining = 0;
	}
	if (S.SalvoRemaining > 0)
	{
		S.SalvoTimer -= DeltaTime;
		int32 Guard = 0;
		while (S.SalvoRemaining > 0 && S.SalvoTimer <= 0.f && Guard++ < TDWeapMaxShotsPerFrame)
		{
			SpawnProjectile(Slot, -S.SalvoTimer);
			--S.SalvoRemaining;
			S.SalvoTimer += FMath::Max(Spec->SalvoInterval, KINDA_SMALL_NUMBER);
		}
	}

	// A weapon that was ready at the start of the frame fires "now" on the press: no sub-frame head start.
	// (Otherwise the press-frame shot is stepped a whole frame: 1333+ cm ahead at 30 fps, 50 at 120.)
	const bool bReadyAtFrameStart = S.RefireTimer <= 0.f;
	S.RefireTimer -= DeltaTime;
	if (bReadyAtFrameStart)
	{
		S.RefireTimer = 0.f;
	}
	const bool bCanShoot = !AreWeaponsBlocked() && !S.bOverheated;

	if (Spec->FireMode == ETDFireMode::Automatic)
	{
		int32 Guard = 0;
		while (S.bTriggerHeld && bCanShoot && !S.bOverheated && S.RefireTimer <= 0.f && HasResourceToFire(*Spec, S) && Guard++ < TDWeapMaxShotsPerFrame)
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
						StopChargeFX(Slot);
						UAudioComponent* Audio = nullptr;
						UNiagaraComponent* Loop = UTDStatics::PlayCueAttachedLoop(FX->ChargeLoop, Owner->GetRootComponent(), NAME_None, Audio);
						(Slot == ETDWeaponSlot::Primary ? PrimaryChargeFX : SecondaryChargeFX) = Loop;
						(Slot == ETDWeaponSlot::Primary ? PrimaryChargeAudio : SecondaryChargeAudio) = Audio;
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
		// dialogs.md Bark_AmmoLow / Bark_AmmoOut: the shot that takes the rack to 25% / empty, flown pilot only. Each is
		// asked once, then re-armed only when the rack is back above half (a regen trickle fired off stays quiet).
		if (S.Ammo * 2 > Spec->MaxAmmo)
		{
			S.bSaidAmmoLow = S.bSaidAmmoOut = false;
		}
		else if (FTDCombatBarks::IsPlayerFlown(GetOwner()))
		{
			if (S.Ammo == 0 && !S.bSaidAmmoOut)
			{
				S.bSaidAmmoOut = S.bSaidAmmoLow = true;
				FTDCombatBarks::SayAsPilot(GetOwner(), TDBarkIdsT2::AmmoOut, ETDBarkPriority::High);
			}
			else if (S.Ammo > 0 && S.Ammo <= FMath::FloorToInt(Spec->MaxAmmo * .25f) && !S.bSaidAmmoLow)
			{
				S.bSaidAmmoLow = true;
				FTDCombatBarks::SayAsPilot(GetOwner(), TDBarkIdsT2::AmmoLow, ETDBarkPriority::Normal);
			}
		}
	}
	S.TimeSinceShot = SubFrameAge;
	++S.ShotsFired;
	S.ShotMultiplier = NextShotMultiplier;
	NextShotMultiplier = 1.f;
	S.SalvoTarget = ResolveAimTarget();
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
		// dialogs.md Bark_WeaponHot: once per overheat (this branch runs only on the edge into lockout).
		FTDCombatBarks::SayAsPilot(GetOwner(), TDBarkIdsT2::WeaponHot, ETDBarkPriority::Normal);
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

	GetMuzzleTransforms(MuzzleScratch, Slot);
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

	AActor* HomingTarget = ProjSpec.bHoming ? (S.SalvoTarget.IsValid() ? S.SalvoTarget.Get() : ResolveAimTarget()) : nullptr;
	const float Multiplier = GetOutgoingDamageMultiplier() * S.ShotMultiplier;

	ATDProjectile* P = ATDProjectile::Launch(World, ProjSpec, FTransform(Dir.Rotation(), MuzzleLocation), Owner, GetOwnerTeam(), HomingTarget,
		Owner->GetVelocity(), Multiplier, ShouldReportHits());
	if (!P)
	{
		return;
	}
	LastFired.Add({ P, P->GetLaunchSerial() });
	if (SubFrameAge > 0.f)
	{
		P->Step(SubFrameAge);
	}

	if (ProjSpec.FX)
	{
		FTDCombatCueLog::Play(this, ProjSpec.FX->Muzzle, ETDCombatCueKind::Muzzle, ProjSpec.FX, MuzzleLocation, Dir.Rotation());
	}
	if (!ProjSpec.FX || !FTDCombatPlaceholders::HasVisualArt(ProjSpec.FX->Muzzle))
	{
		// Placeholder muzzle flash, riding on the ship so it stays at the barrel.
		if (UTDProjectileSubsystem* Projectiles = UTDProjectileSubsystem::Get(this))
		{
			// Squad-coloured, and never bigger than about a third of the hull (playtest 3: Bastion's white flash hid the ship).
			const FLinearColor Color = FTDCombatPlaceholders::ProjectileColor(ProjSpec, GetOwnerTeam(), Owner, false);
			const float Radius = FTDCombatPlaceholders::CapMuzzleFlashRadius(FTDCombatPlaceholders::MuzzleFlashRadius(ProjSpec), Owner->GetSimpleCollisionRadius());
			Projectiles->PlayPlaceholderFlash(MuzzleLocation, Radius, FMath::Lerp(Color, FLinearColor::White, 0.12f), 0.06f, Owner->GetRootComponent());
		}
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

void UTDWeaponComponent::GetMuzzleTransforms(TArray<FTransform>& Out, ETDWeaponSlot Slot) const
{
	// The secondary slot leaves from "<SecondaryMuzzlePrefix>N" sockets when the ship has them (the Rodin interceptor's
	// Missile_0/1 under the wings, T4 / the Lead); otherwise it shares the guns' muzzles as before.
	if (Slot == ETDWeaponSlot::Secondary && !SecondaryMuzzlePrefix.IsNone())
	{
		if (SecondarySocketNames.Num() == 0 || SecondarySocketPrefixBuilt != SecondaryMuzzlePrefix)
		{
			SecondarySocketNames.Reset();
			for (int32 i = 0; i < TDWeapMaxMuzzleSockets; ++i)
			{
				SecondarySocketNames.Add(FName(*FString::Printf(TEXT("%s%d"), *SecondaryMuzzlePrefix.ToString(), i)));
			}
			SecondarySocketPrefixBuilt = SecondaryMuzzlePrefix;
		}
		Out.Reset();
		if (const AActor* Owner = GetOwner())
		{
			TInlineComponentArray<UStaticMeshComponent*> Meshes(Owner);
			for (const UStaticMeshComponent* Mesh : Meshes)
			{
				if (Mesh && Mesh->DoesSocketExist(SecondarySocketNames[0]))
				{
					for (int32 i = 0; i < SecondarySocketNames.Num() && Mesh->DoesSocketExist(SecondarySocketNames[i]); ++i)
					{
						Out.Add(Mesh->GetSocketTransform(SecondarySocketNames[i]));
					}
					return;
				}
			}
		}
	}
	GetMuzzleTransforms(Out);
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
		if (Mesh && Mesh->DoesSocketExist(TDWeapMuzzleSocketName(0)))
		{
			for (int32 i = 0; i < TDWeapMaxMuzzleSockets && Mesh->DoesSocketExist(TDWeapMuzzleSocketName(i)); ++i)
			{
				Out.Add(Mesh->GetSocketTransform(TDWeapMuzzleSocketName(i)));
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

ATDProjectile* UTDWeaponComponent::GetLastFiredProjectile(int32 Index) const
{
	if (!LastFired.IsValidIndex(Index))
	{
		return nullptr;
	}
	ATDProjectile* P = LastFired[Index].Projectile.Get();
	return (P && P->IsActive() && P->GetLaunchSerial() == LastFired[Index].Serial) ? P : nullptr;
}

void UTDWeaponComponent::StopChargeFX(ETDWeaponSlot Slot)
{
	TObjectPtr<UNiagaraComponent>& Comp = Slot == ETDWeaponSlot::Primary ? PrimaryChargeFX : SecondaryChargeFX;
	if (Comp)
	{
		Comp->Deactivate();
		Comp = nullptr;
	}
	TObjectPtr<UAudioComponent>& Audio = Slot == ETDWeaponSlot::Primary ? PrimaryChargeAudio : SecondaryChargeAudio;
	if (Audio)
	{
		Audio->FadeOut(0.1f, 0.f);
		Audio = nullptr;
	}
}
