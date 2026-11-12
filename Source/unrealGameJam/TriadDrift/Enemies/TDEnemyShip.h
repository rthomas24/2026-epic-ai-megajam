// Triad Drift - Enemies & Waves (Teammate 4). ENG-13 enemy ship pawn.
#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Enemies/TDEnemyTypes.h"
#include "TDEnemyShip.generated.h"

class UTDWeaponComponent;
class UTDEnemyVisuals;
class ATDCarrierBay;
class ATDEnemyTurret;
class ATDEnemyShip;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDEnemyTelegraphSignature, ATDEnemyShip*, Ship, ETDEnemyAttack, Attack, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyAttackSignature, ATDEnemyShip*, Ship, ETDEnemyAttack, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyDiedSignature, ATDEnemyShip*, Ship, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyLaunchedSignature, ATDEnemyShip*, Parent, ATDEnemyShip*, Launched);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyStateSignature, ATDEnemyShip*, Ship, float, Seconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDEnemyShieldSignature, ATDEnemyShip*, Ship);

/**
 * A Null Armada ship. Same flight model as the squad (T1's ATDSpacecraft, driven through
 * SetFlightInput), T2's UTDWeaponComponent for guns, and a telegraph -> fire attack pipeline that
 * the AI drives and Nick's VFX/audio read.
 *
 * Lifecycle: spawn -> InitFromSpec(Id) -> (AI possesses) -> ... -> Die() -> tumble -> explode -> destroy.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDEnemyShip : public ATDSpacecraft
{
	GENERATED_BODY()

public:
	ATDEnemyShip(const FObjectInitializer& ObjectInitializer);

	// --- Setup ----------------------------------------------------------------------------------

	/** Apply an enemy type from UTDEnemyLibrary: health, flight tuning, weapons, carrier parts. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void InitFromSpec(FName EnemyId);

	/** Apply an explicit spec (tests, debug). */
	void InitFromSpecData(const FTDEnemySpec& Spec);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	FName GetEnemyId() const { return Spec.Id; }

	const FTDEnemySpec& GetSpec() const { return Spec; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	ETDEnemyRole GetRole() const { return Spec.Role; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	ETDEnemySize GetSize() const { return Spec.Size; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	UTDWeaponComponent* GetWeapons() const { return Weapons; }

	// --- Attacks (driven by ATDEnemyAIController) -----------------------------------------------

	/**
	 * Start an attack. Telegraphed attacks enter the telegraph state for the spec's TelegraphSeconds,
	 * then fire; others fire immediately. Returns false if stunned, dead, on cooldown or already busy.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	bool TryStartAttack(ETDEnemyAttack Attack, AActor* Target);

	/** True while the attack is off cooldown and nothing else is telegraphing. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool CanStartAttack(ETDEnemyAttack Attack) const;

	/** Hold or release the continuous guns. Ignored while stunned. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void SetGunsFiring(bool bFiring, AActor* Target);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool AreGunsFiring() const { return bGunsFiring; }

	/** Cancel an in-progress telegraph (stun, death, lost tracking). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void CancelTelegraph();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsTelegraphing() const { return TelegraphAttack != ETDEnemyAttack::None; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	ETDEnemyAttack GetTelegraphAttack() const { return TelegraphAttack; }

	/** 0 at telegraph start -> 1 when it fires. 0 when not telegraphing. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetTelegraph01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetTelegraphDuration() const { return TelegraphDuration; }

	/** Seconds until the attack can be used again. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetAttackCooldownRemaining(ETDEnemyAttack Attack) const;

	/** How many times each attack has actually fired (tests, stats). */
	int32 GetFireCount(ETDEnemyAttack Attack) const;

	// --- Status effects (EMP, cloak, portals, gravity well) ------------------------------------

	/** Stun: no steering input, no firing, telegraphs cancelled. Stacks by taking the longer time. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void ApplyStun(float Seconds);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsStunned() const { return StunRemaining > 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetStunRemaining() const { return StunRemaining; }

	/** Lose tracking: the AI forgets its target and cannot re-acquire one (cloak, portal transit). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void LoseTracking(float Seconds);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsTrackingLost() const { return TrackingLostRemaining > 0.f; }

	/** Suppress firing without a stun (Gravity Well: "can't fire" while inside). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void SuppressFire(float Seconds);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool CanFire() const;

	// --- Gunship frontal shield -------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool HasFrontalShield() const { return Spec.FrontalShield > 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsFrontalShieldUp() const { return HasFrontalShield() && FrontalShieldHP > 0.f && FrontalShieldDownRemaining <= 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetFrontalShield01() const;

	/** True if damage travelling along WorldDirection (from source toward this ship) hits the frontal arc. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsFrontalHit(const FVector& DamageTravelDirection) const;

	/** Drop the frontal shield for Seconds without draining it (EMP: 5 s). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void DropFrontalShield(float Seconds);

	/** Break the frontal shield outright (Lancer ram). It recovers after FrontalShieldRecoverSeconds. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void BreakFrontalShield();

	// --- Carrier ----------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	int32 GetLivingBayCount() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	TArray<ATDCarrierBay*> GetBays() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	TArray<ATDEnemyTurret*> GetTurrets() const;

	/** Called by a bay on its launch cadence. Spawns the launched fighters at the bay. */
	void LaunchFightersFromBay(ATDCarrierBay* Bay, int32 Count);

	/** Class used for fighters launched by carriers (and by the wave director by default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Enemy")
	TSubclassOf<ATDEnemyShip> LaunchedShipClass;

	// --- Death --------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsDying() const { return bDying; }

	/** Fraction of hull lost, 0 (pristine) .. 1 (dead). For damage-state visuals. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetDamage01() const;

	// --- Events -------------------------------------------------------------------------------

	/** A telegraphed attack started winding up (VFX: charge glow, audio: wind-up). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyTelegraphSignature OnTelegraphStarted;

	/** An attack fired (after its telegraph, if any). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyAttackSignature OnAttackFired;

	/** The ship died (start of the tumble). Killer may be null. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyDiedSignature OnEnemyDied;

	/** The ship exploded (end of the tumble), just before it is destroyed. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyDiedSignature OnEnemyExploded;

	/** A carrier launched a fighter. The wave director listens so launched fighters count for the wave. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyLaunchedSignature OnEnemyLaunched;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyStateSignature OnStunned;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyStateSignature OnTrackingLost;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyShieldSignature OnFrontalShieldBroken;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyShieldSignature OnFrontalShieldRestored;

	// --- Presentation -------------------------------------------------------------------------

	/** Nick's presentation data asset (features.md 3.5). Empty = placeholder shapes only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDEnemyVisuals> Visuals;

	// --- AActor -------------------------------------------------------------------------------
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Fires the attack for real. Override in Blueprint children for custom patterns. */
	virtual void ExecuteAttack(ETDEnemyAttack Attack, AActor* Target);

	/** Start the death sequence. Safe to call more than once. */
	virtual void Die(AActor* Killer);
	virtual void Explode();

	const FTDEnemyAttackSpec* FindAttack(ETDEnemyAttack Attack) const;

	UFUNCTION()
	void HandleHealthDeath(AActor* Killer);

	void SpawnCarrierParts();
	void DestroyCarrierParts();
	void DropMine(const FTDEnemyAttackSpec& Attack);
	void FireMissileSalvo(const FTDEnemyAttackSpec& Attack, AActor* Target);
	void UpdatePlaceholderLook();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Enemy")
	TObjectPtr<UTDWeaponComponent> Weapons;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "TriadDrift|Enemy")
	FTDEnemySpec Spec;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDCarrierBay>> Bays;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDEnemyTurret>> Turrets;

	TWeakObjectPtr<AActor> TelegraphTarget;
	TWeakObjectPtr<AActor> GunTarget;
	TMap<ETDEnemyAttack, float> CooldownRemaining;
	TMap<ETDEnemyAttack, int32> FireCounts;

	ETDEnemyAttack TelegraphAttack = ETDEnemyAttack::None;
	float TelegraphElapsed = 0.f;
	float TelegraphDuration = 0.f;

	float StunRemaining = 0.f;
	float TrackingLostRemaining = 0.f;
	float FireSuppressedRemaining = 0.f;

	float FrontalShieldHP = 0.f;
	float FrontalShieldDownRemaining = 0.f;	// EMP drop (shield HP kept)
	float FrontalShieldRecoverRemaining = 0.f;	// broken, recovering

	float DeathTimer = 0.f;
	TWeakObjectPtr<AActor> LastKiller;
	bool bGunsFiring = false;
	bool bDying = false;
	bool bExploded = false;
};
