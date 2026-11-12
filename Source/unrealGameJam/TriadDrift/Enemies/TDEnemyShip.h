// Triad Drift - Enemies & Waves (Teammate 4). ENG-13 enemy ship pawn.
#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Enemies/TDEnemyTypes.h"
#include "TriadDrift/Waves/TDWaveEnemy.h"
#include "TDEnemyShip.generated.h"

class UTDWeaponComponent;
class UTDEnemyVisuals;
class UTDHealthComponent;
class ATDCarrierBay;
class ATDEnemyTurret;
class ATDEnemyShip;
struct FTDDamageInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDEnemyTelegraphSignature, ATDEnemyShip*, Ship, ETDEnemyAttack, Attack, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyAttackSignature, ATDEnemyShip*, Ship, ETDEnemyAttack, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyDiedSignature, ATDEnemyShip*, Ship, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyLaunchedSignature, ATDEnemyShip*, Parent, ATDEnemyShip*, Launched);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDEnemyStateSignature, ATDEnemyShip*, Ship, float, Seconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDEnemyShieldSignature, ATDEnemyShip*, Ship);

/**
 * A Null Armada ship. Same flight model as the squad (T1's ATDSpacecraft, driven through
 * SetFlightInput), T2's UTDWeaponComponent for every weapon, and a telegraph -> fire attack
 * pipeline that the AI drives and Nick's VFX/audio read.
 *
 * Lifecycle: spawn -> InitFromSpec(Id) -> (AI possesses) -> ... -> death -> tumble -> explode -> destroy.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDEnemyShip : public ATDSpacecraft, public ITDWaveEnemy
{
	GENERATED_BODY()

public:
	ATDEnemyShip();

	// --- Setup ----------------------------------------------------------------------------------

	/** Apply an enemy type from UTDEnemyLibrary: health, flight tuning, weapons, carrier parts. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void InitFromSpec(FName EnemyId);

	/** Apply an explicit spec (tests, debug). */
	void InitFromSpecData(const FTDEnemySpec& InSpec);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	FName GetEnemyId() const { return Spec.Id; }

	const FTDEnemySpec& GetSpec() const { return Spec; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	ETDEnemyRole GetRole() const { return Spec.Role; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	ETDEnemySize GetSize() const { return Spec.Size; }

	// Weapons: ATDSpacecraft::GetWeapons() (T1's "Weapons" subobject).

	// ITDWaveEnemy
	virtual void InitWaveEnemy(FName EnemyId) override { InitFromSpec(EnemyId); }
	virtual void JoinFormation(AActor* Leader, const FVector& LocalOffset) override;
	virtual void PlayWarpIn() override;

	// --- Attacks (driven by ATDEnemyAIController) -----------------------------------------------

	/**
	 * Start an attack. Telegraphed attacks enter the telegraph state for the spec's TelegraphSeconds,
	 * then fire; others fire immediately. Returns false if it can't start (see CanStartAttack).
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	bool TryStartAttack(ETDEnemyAttack Attack, AActor* Target);

	/** Off cooldown, alive, able to fire, and no other telegraph or burst in progress. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool CanStartAttack(ETDEnemyAttack Attack) const;

	/** Hold or release the continuous guns. Suppressed while stunned / fire-suppressed / dying. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void SetGunsFiring(bool bFiring, AActor* Target);

	/** The AI wants the guns on. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool AreGunsFiring() const { return bGunsFiring; }

	/** The primary trigger is actually held on the weapon component (false while stunned). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsPrimaryTriggerHeld() const { return bPrimaryTriggerHeld; }

	/** Cancel an in-progress telegraph or burst (stun, death, lost tracking). Cooldown is kept. */
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

	/** How many shots each attack has actually fired (tests, stats). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	int32 GetFireCount(ETDEnemyAttack Attack) const;

	// --- Status effects (EMP, cloak, portals, gravity well) ------------------------------------

	/** Stun: dead stick, no firing, telegraphs cancelled, carrier bays paused. Takes the longer time. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void ApplyStun(float Seconds);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsStunned() const { return StunRemaining > 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetStunRemaining() const { return StunRemaining; }

	/** Lose tracking: the AI forgets its target and can't re-acquire one (cloak, portal transit). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void LoseTracking(float Seconds);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsTrackingLost() const { return TrackingLostRemaining > 0.f; }

	/** Suppress firing without a stun (Gravity Well: "can't fire" while inside). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Enemy")
	void SuppressFire(float Seconds);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool CanFire() const;

	/** Extra damage taken while stunned (EMP: +25%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Enemy", meta = (ClampMin = "1"))
	float StunnedDamageMultiplier = 1.25f;

	// --- Gunship frontal shield -------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool HasFrontalShield() const { return Spec.FrontalShield > 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsFrontalShieldUp() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetFrontalShield01() const;

	/** True if a hit travelling along TravelDirection (source -> ship) arrives inside the frontal arc. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsFrontalHit(const FVector& TravelDirection) const;

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

	/** Called by a bay on its launch cadence. Spawns Count fighters at the bay. */
	void LaunchFightersFromBay(ATDCarrierBay* Bay, int32 Count);

	/** Class used for fighters launched by carriers. Null = ATDEnemyShip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Enemy")
	TSubclassOf<ATDEnemyShip> LaunchedShipClass;

	/** Gap between the carrier's collision sphere and its mounted bays / turrets (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Enemy", meta = (ClampMin = "0"))
	float CarrierPartClearance = 150.f;

	/** Cap on living fighters a single carrier keeps in the air (perf + fairness). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Enemy", meta = (ClampMin = "0"))
	int32 MaxLaunchedAlive = 12;

	// --- Death --------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsDying() const { return bDying; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	bool IsAliveEnemy() const;

	/** Fraction of hull lost, 0 (pristine) .. 1 (dead). For damage-state visuals. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Enemy")
	float GetDamage01() const;

	// --- Events -------------------------------------------------------------------------------

	/** A telegraphed attack started winding up (VFX: charge glow, audio: wind-up). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Enemy")
	FTDEnemyTelegraphSignature OnTelegraphStarted;

	/** An attack fired (after its telegraph, if any). Once per attack, not per projectile. */
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

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Fires the attack for real (after the telegraph). */
	virtual void ExecuteAttack(ETDEnemyAttack Attack, AActor* Target);

	/** Health pre-filter: frontal shield + stun vulnerability. */
	float FilterIncomingDamage(const FTDDamageInfo& Info);

	UFUNCTION()
	void HandleHealthDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	UFUNCTION()
	void HandleLaunchedDied(ATDEnemyShip* Ship, AActor* Killer);

	void BeginDeath(AActor* Killer);
	void Explode();
	const FTDEnemyAttackSpec* FindAttack(ETDEnemyAttack Attack) const;
	void SpawnCarrierParts();
	void MountPart(AActor* Part, const FVector& LocalDirection, float PartHalfExtent, float Up);
	void DestroyCarrierParts();
	void FireSecondaryShot(FName WeaponId, AActor* Target);
	void TickTimers(float DeltaSeconds);
	void TickBurst(float DeltaSeconds);
	void TickDeath(float DeltaSeconds);
	void ApplyPrimaryTrigger();
	void UpdatePlaceholderLook();
	void SetBaysPaused(bool bPaused);
	UTDWeaponComponent* EnsureWeapons();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "TriadDrift|Enemy")
	FTDEnemySpec Spec;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDCarrierBay>> Bays;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDEnemyTurret>> Turrets;

	TArray<TWeakObjectPtr<ATDEnemyShip>> LaunchedAlive;

	TWeakObjectPtr<AActor> TelegraphTarget;
	TWeakObjectPtr<AActor> GunTarget;
	TMap<ETDEnemyAttack, float> CooldownRemaining;
	TMap<ETDEnemyAttack, int32> FireCounts;

	ETDEnemyAttack TelegraphAttack = ETDEnemyAttack::None;
	float TelegraphElapsed = 0.f;
	float TelegraphDuration = 0.f;

	// Multi-shot attack in progress (bolt volley, mine trail): one FireOnce per shot.
	ETDEnemyAttack BurstAttack = ETDEnemyAttack::None;
	int32 BurstShotsRemaining = 0;
	float BurstTimer = 0.f;
	TWeakObjectPtr<AActor> BurstTarget;

	/** Seconds between shots inside a volley (heavy bolts). */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Enemy", meta = (ClampMin = "0"))
	float BurstShotSpacing = 0.18f;

	/** Seconds between mines in a trail. */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Enemy", meta = (ClampMin = "0"))
	float MineSpacingSeconds = 0.45f;

	/** Projectile speed used to lead targets with the guns (cm/s). */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Enemy", meta = (ClampMin = "1000"))
	float GunLeadSpeed = 30000.f;

	FRandomStream AimRng{ 4242 };

	float StunRemaining = 0.f;
	float TrackingLostRemaining = 0.f;
	float FireSuppressedRemaining = 0.f;

	float FrontalShieldHP = 0.f;
	float FrontalShieldDownRemaining = 0.f;		// EMP drop (HP kept)
	float FrontalShieldRecoverRemaining = 0.f;	// broken, recovering

	float DeathTimer = 0.f;
	TWeakObjectPtr<AActor> LastKiller;
	FName SecondaryWeaponId = NAME_None;
	bool bGunsFiring = false;
	bool bPrimaryTriggerHeld = false;
	bool bDying = false;
	bool bExploded = false;
	bool bSpecApplied = false;
};
