// Triad Drift - hull + regenerating shield (features.md ENG-03). Lead-owned.
// Put one on anything that can be damaged. It registers its owner with UTDCombatRegistry.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDHealthComponent.generated.h"

class UTDHealthComponent;
class UDamageType;
class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FTDOnDamaged, UTDHealthComponent*, Health, const FTDDamageInfo&, Info, float, ShieldDamage, float, HullDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnShieldBroken, UTDHealthComponent*, Health, const FTDDamageInfo&, Info);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnDeath, UTDHealthComponent*, Health, const FTDDamageInfo&, KillingBlow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnRevived, UTDHealthComponent*, Health);

/**
 * Optional C++ pre-filter, run before shield and hull. Return the damage that gets through
 * (0 = fully blocked). Example: the Wall Gunship's frontal shield.
 */
DECLARE_DELEGATE_RetVal_OneParam(float, FTDDamageFilter, const FTDDamageInfo& /*Info*/);

UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDHealthComponent();

	// ------------------------------------------------------------------ tuning
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Health", meta = (ClampMin = "1"))
	float MaxHull = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Health", meta = (ClampMin = "0"))
	float MaxShield = 100.f;

	/** Seconds without damage before the shield starts regenerating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health", meta = (ClampMin = "0"))
	float ShieldRegenDelay = 3.f;

	/** Fraction of MaxShield regenerated per second (0.25 = 25%/s). 0 disables regeneration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health", meta = (ClampMin = "0"))
	float ShieldRegenFractionPerSecond = 0.25f;

	/** Seconds without damage before the hull regenerates (only if HullRegenPerSecond > 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health", meta = (ClampMin = "0"))
	float HullRegenDelay = 4.f;

	/** Hull points per second; 0 = no passive hull regeneration (ships). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health", meta = (ClampMin = "0"))
	float HullRegenPerSecond = 0.f;

	/** All incoming damage is multiplied by this (difficulty, stun vulnerability...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health", meta = (ClampMin = "0"))
	float IncomingDamageMultiplier = 1.f;

	/** Team used when the owner does not implement ITDTeam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health")
	ETDTeam DefaultTeam = ETDTeam::Neutral;

	/** False hides the owner from targeting queries (cloak). It can still be damaged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health")
	bool bTargetable = true;

	/** Destroy the owning actor this many seconds after death; < 0 = never (owner handles it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Health")
	float DestroyOwnerAfterDeathSeconds = -1.f;

	// ------------------------------------------------------------------ events
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Health")
	FTDOnDamaged OnDamaged;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Health")
	FTDOnShieldBroken OnShieldBroken;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Health")
	FTDOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Health")
	FTDOnRevived OnRevived;

	/** C++ only. Bind to veto or scale damage before it is applied. */
	FTDDamageFilter DamageFilter;

	// ------------------------------------------------------------------ damage
	/**
	 * Apply one hit. Returns the damage actually dealt (shield + hull). No team check here:
	 * use UTDStatics::ApplyTDDamage from gameplay code.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	float ApplyDamage(const FTDDamageInfo& Info);

	/** Kill outright (ignores invulnerability and shields). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void Kill(AActor* Killer = nullptr, ETDDamageKind Kind = ETDDamageKind::Environmental);

	/** Bring back from death (or top up) to these fractions and clear timers. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void Revive(float HullFraction = 1.f, float ShieldFraction = 1.f);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void RepairHull(float Amount);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void RestoreShield(float Amount);

	/**
	 * Zero this component's shield and hold it down for Seconds. A gunship's FRONTAL shield is not
	 * this shield: EMP must also call ATDEnemyShip::DropFrontalShield / ApplyStun.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void SuppressShield(float Seconds);

	/** Set new maximums. bRefill also restores current values to full. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void InitMaxValues(float NewMaxHull, float NewMaxShield, bool bRefill = true);

	/** Timed invulnerability; extends, never shortens. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void SetInvulnerableFor(float Seconds);

	/** Ends a timed invulnerability now (a power that granted it was interrupted). The untimed flag is untouched. */
	void ClearTimedInvulnerability() { InvulnerableTimeRemaining = 0.f; }

	/** Untimed invulnerability flag (ram dash, cinematics, god mode). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void SetInvulnerable(bool bNewInvulnerable) { bInvulnerable = bNewInvulnerable; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Health")
	void SetTargetable(bool bNewTargetable) { bTargetable = bNewTargetable; }

	// ------------------------------------------------------------------ queries
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	bool IsAlive() const { return !bDead; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	bool IsInvulnerable() const { return bInvulnerable || InvulnerableTimeRemaining > 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	float GetHull() const { return Hull; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	float GetShield() const { return Shield; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	float GetHull01() const { return MaxHull > 0.f ? Hull / MaxHull : 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	float GetShield01() const { return MaxShield > 0.f ? Shield / MaxShield : 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	bool IsShieldUp() const { return Shield > 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	float GetTimeSinceLastDamage() const { return TimeSinceDamage; }

	/** The last actor that damaged this one (for "who threatens the player" AI queries). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	AActor* GetLastAttacker() const { return LastAttacker.Get(); }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	void Die(const FTDDamageInfo& KillingBlow);

	float Hull = 100.f;
	float Shield = 100.f;
	float TimeSinceDamage = 1000.f;
	float InvulnerableTimeRemaining = 0.f;
	float ShieldSuppressedRemaining = 0.f;
	bool bInvulnerable = false;
	bool bDead = false;
	TWeakObjectPtr<AActor> LastAttacker;
};
