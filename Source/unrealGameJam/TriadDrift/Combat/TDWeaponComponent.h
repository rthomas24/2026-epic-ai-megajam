// Triad Drift - Combat (Teammate 2). Two weapon slots on any actor (ENG-05).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TDWeaponComponent.generated.h"

class ATDProjectile;
class UTDWeaponFX;
class UNiagaraComponent;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnWeaponFired, ETDWeaponSlot, Slot, FName, WeaponId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnWeaponOverheat, ETDWeaponSlot, Slot, bool, bOverheated);

/** Runtime state of one weapon slot. Read-only for others; exposed for HUD / debugging. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDWeaponSlotState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponId = NAME_None;

	/** Effective trigger: requested && not blocked && owner alive. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bTriggerHeld = false;

	/** What the pilot asked for; survives blocks (EMP, Gravity Well) so fire resumes when they end. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bTriggerRequested = false;

	/** Shots (or salvos) begun since equip. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 ShotsFired = 0;

	/** 0..1 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Heat = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bOverheated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float OverheatRemaining = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 Ammo = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AmmoRegenProgress = 0.f;

	/** Seconds held on a Charge weapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ChargeHeld = 0.f;

	/** Time until the next shot may fire (can go negative within a frame: sub-frame catch-up). */
	float RefireTimer = 0.f;
	float TimeSinceShot = 1000.f;
	int32 MuzzleCursor = 0;
	int32 SalvoRemaining = 0;
	float SalvoTimer = 0.f;
	bool bChargeReady = false;
	/** One-shot multiplier captured when this shot/salvo began (Cloak's first shot). */
	float ShotMultiplier = 1.f;
	/** Aim target captured when the shot/salvo began: rippled salvos keep it even if the AI clears its target. */
	TWeakObjectPtr<AActor> SalvoTarget;
};

/**
 * Two weapon slots (Primary, Secondary) fed from UTDWeaponLibrary.
 * Drive with SetTriggerHeld + SetAimDirection / SetAimTarget. Frame-rate independent: shots that fall
 * inside a long frame are all fired (with their sub-frame offset applied to their start position).
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDWeaponComponent();

	// --- Contract (ARCHITECTURE.md §4) ---
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void EquipWeapon(ETDWeaponSlot Slot, FName WeaponId);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void SetTriggerHeld(ETDWeaponSlot Slot, bool bHeld);

	/** Aim-assist / homing target. Null = fall back to the owner's UTDTargetingComponent current target. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void SetAimTarget(AActor* Target);

	/** World-space aim direction. Zero vector = owner forward. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void SetAimDirection(const FVector& WorldDir);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	float GetHeat01(ETDWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	int32 GetAmmo(ETDWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	bool IsOverheated(ETDWeaponSlot Slot) const;

	// --- Extras ---
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	int32 GetMaxAmmo(ETDWeaponSlot Slot) const;

	/** 0..1 charge progress on a Charge weapon (0 for others). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	float GetCharge01(ETDWeaponSlot Slot) const;

	/** Ready to fire right now (equipped, not locked out, has ammo, refire elapsed). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	bool CanFire(ETDWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	FName GetWeaponId(ETDWeaponSlot Slot) const;

	/** The equipped spec (invalid spec if empty). */
	const FTDWeaponSpec& GetSpec(ETDWeaponSlot Slot) const;

	const FTDWeaponSlotState& GetSlotState(ETDWeaponSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	FVector GetAimDirection() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	AActor* GetAimTarget() const;

	/**
	 * Outgoing damage multipliers by source ("SwitchStrike", "Cloak", ...). The product of all is applied
	 * to every projectile fired. Multiplier 1 or Clear removes the source.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void SetOutgoingDamageMultiplier(FName SourceKey, float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void ClearOutgoingDamageMultiplier(FName SourceKey);

	/** Applies to the next shot (or salvo) only, then clears itself (Cloak's 3x first shot). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void SetNextShotDamageMultiplier(float Multiplier);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	float GetOutgoingDamageMultiplier() const;

	/** Refill ammo and clear heat on both slots (respawn, cheats). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void ResetWeapons();

	/** Disable all firing (stunned by EMP, inside a Gravity Well, docked). Releases triggers. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void SetWeaponsBlocked(FName SourceKey, bool bBlocked);

	/** Blocked by any source, or the owner is dead (death tumble, downed). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Weapon")
	bool AreWeaponsBlocked() const;

	/** Drop a charge without firing (ship switched away, unpossessed, cutscene). Also releases the request. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	void CancelCharge(ETDWeaponSlot Slot);

	/** World transforms of the muzzles (sockets "Muzzle_N" on the first static mesh with them, else MuzzleOffsets). */
	void GetMuzzleTransforms(TArray<FTransform>& Out) const;

	/** Fires one shot (or one salvo) immediately, ignoring the trigger but respecting resources. Returns true if it fired. AI / tests. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Weapon")
	bool FireOnce(ETDWeaponSlot Slot);

	/** Number of projectiles the last shot / salvo spawned so far. */
	int32 GetLastFiredCount() const { return LastFired.Num(); }

	/** Projectile Index of the last shot, or null once it has expired (pooled actors are reused, so this checks the launch serial). */
	ATDProjectile* GetLastFiredProjectile(int32 Index = 0) const;

	// --- Tuning ---
	/** Local-space muzzle offsets used when the owner has no "Muzzle_N" sockets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Weapon")
	TArray<FVector> MuzzleOffsets;

	/**
	 * Presentation per slot. Left empty, EquipWeapon loads /Game/TriadDrift/Presentation/DA_<WeaponId>_FX if
	 * it exists. Null = placeholder visuals, silence.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Weapon")
	TObjectPtr<UTDWeaponFX> PrimaryFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Weapon")
	TObjectPtr<UTDWeaponFX> SecondaryFX = nullptr;

	/** Fire HUD hit markers and kill hit-stop for this shooter. Default: true when the owner is a locally player-controlled pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Weapon")
	bool bForceReportHits = false;

	/** Max range for the targeting-component fallback aim target used by aim assist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Weapon", meta = (Units = "cm"))
	float AimAssistMaxRange = 40000.f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Weapon")
	FTDOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Weapon")
	FTDOnWeaponOverheat OnOverheatChanged;

	/** Seeds the spread RNG (tests). */
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	FTDWeaponSlotState& State(ETDWeaponSlot Slot);
	const FTDWeaponSlotState& State(ETDWeaponSlot Slot) const;
	const FTDWeaponSpec*& SpecPtr(ETDWeaponSlot Slot);
	const FTDWeaponSpec* SpecPtr(ETDWeaponSlot Slot) const;

	void TickSlot(ETDWeaponSlot Slot, float DeltaTime);
	/** Recompute the effective trigger. A pilot release (not a block) fires a full charge. */
	void UpdateEffectiveTrigger(ETDWeaponSlot Slot, bool bPilotRelease);
	bool HasResourceToFire(const FTDWeaponSpec& Spec, const FTDWeaponSlotState& S) const;
	/** Consumes resource and starts the shot/salvo. SubFrameAge = seconds this shot is "late" within the frame. */
	void BeginShot(ETDWeaponSlot Slot, float SubFrameAge);
	void SpawnProjectile(ETDWeaponSlot Slot, float SubFrameAge);
	void AddHeat(ETDWeaponSlot Slot, float Amount);
	FVector ComputeFireDirection(const FTDWeaponSpec& Spec, const FVector& MuzzleLocation);
	AActor* ResolveAimTarget() const;
	bool ShouldReportHits() const;
	ETDTeam GetOwnerTeam() const;

	FTDWeaponSlotState PrimaryState;
	FTDWeaponSlotState SecondaryState;
	const FTDWeaponSpec* PrimarySpec = nullptr;
	const FTDWeaponSpec* SecondarySpec = nullptr;

	FVector AimDirection = FVector::ZeroVector;
	TWeakObjectPtr<AActor> AimTarget;

	TMap<FName, float> DamageMultipliers;
	float NextShotMultiplier = 1.f;
	TSet<FName> BlockSources;

	void StopChargeFX(ETDWeaponSlot Slot);
	UTDWeaponFX* GetFX(ETDWeaponSlot Slot) const { return Slot == ETDWeaponSlot::Primary ? PrimaryFX.Get() : SecondaryFX.Get(); }

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> PrimaryChargeFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SecondaryChargeFX;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> PrimaryChargeAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> SecondaryChargeAudio;

	struct FFiredShot
	{
		TWeakObjectPtr<ATDProjectile> Projectile;
		uint32 Serial = 0;
	};
	TArray<FFiredShot> LastFired;
	/** The slot's FX came from the path convention (so re-equipping may replace it); designer-set FX is never replaced. */
	bool bPrimaryFXAuto = false;
	bool bSecondaryFXAuto = false;
	TArray<FTransform> MuzzleScratch;
	FRandomStream Random;
};
