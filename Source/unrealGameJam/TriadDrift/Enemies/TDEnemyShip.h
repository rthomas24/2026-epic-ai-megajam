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

	/** How many shots each attack has actually fired (UTDWeaponComponent::FireOnce succeeded). */
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
	int32 MaxLaunchedAlive = 8;

	// Lead (Nov 14, wave 5 played without god mode: 40 alive at 131 s and growing): a carrier's stream is finite and
	// pauses while the screen is full, so killing fighters always makes progress and killing a carrier ends it.
	/** Fighters one carrier launches in all (3 flights of 4); then it fights with its own guns. 0 = no limit. */
	int32 MaxTotalLaunched = 12;
	int32 GetTotalLaunched() const { return TotalLaunched; }
	bool IsLaunchSpent() const { return MaxTotalLaunched > 0 && TotalLaunched >= MaxTotalLaunched; }
	int32 GetLaunchedAliveCount() const;
	/** Carrier-launched fighters alive in the whole world (every carrier's). */
	static int32 CountCarrierFightersAlive(const UObject* WorldContext);

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

	/** Presentation asset name for a type (Nick fills these; no code or BP edits needed). */
	static FString VisualsAssetName(const FTDEnemySpec& InSpec);

	/** The visuals asset for a type: spec soft pointer, else the convention asset, else null. Cached. */
	static UTDEnemyVisuals* ResolveVisuals(const FTDEnemySpec& InSpec);

	/** Placeholder glow ("white eyes", engine) when no visuals asset supplies a mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0"))
	float PlaceholderEmissiveBoost = 8.f;

	/** Placeholder body glow: red-orange, strong enough to bloom on black space (playtest: dark specks). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0"))
	float PlaceholderBodyGlow = 2.5f;

	/** The white "eyes" (visor), brighter than the engine so they read against a bright nebula. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0"))
	float PlaceholderEyeGlow = 16.f;

	/** Placeholder-only visual scale by role (collision unchanged): readable at 100-400 m before the art lands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0.1"))
	float PlaceholderSmallShipScale = 2.75f;	// art audit: +25% (2.2 -> 2.75)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0.1"))
	float PlaceholderMediumShipScale = 2.f;	// +25%

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0.1"))
	float PlaceholderLargeShipScale = 1.75f;	// +25%

	/** Placeholder "eyes" (two white glowing spheres at the nose) and the gunship's frontal-shield plate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Presentation")
	TObjectPtr<UStaticMeshComponent> PlaceholderEyeLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Presentation")
	TObjectPtr<UStaticMeshComponent> PlaceholderEyeRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Presentation")
	TObjectPtr<UStaticMeshComponent> PlaceholderShieldPlate;

	bool bPlaceholderLook = false;

	// --- Death: the hulk drifts away from the camera and explodes in view ---
public:
	/** Longest death tumble (s): big ships used 1.2-2.5 s and drifted out of frame before exploding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0"))
	float MaxDeathTumbleSeconds = 1.f;

	/** The hulk eases away from the camera at this speed (cm/s), on top of its own velocity minus any part toward it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0"))
	float DeathDriftAwaySpeed = 800.f;

	FVector GetDeathDrift() const { return DeathDrift; }

	/**
	 * ENG-37: exhaust effects running now: one EngineTrail effect per hull nozzle socket ("Engine", "Engine_0".."Engine_8",
	 * ATDShip::GetNozzleSockets), or one at the hull origin when the mesh has none. 0 without EngineTrail VFX or once dead.
	 */
	int32 GetExhaustEffectCount() const;

	/** Exhausts this ship asked for: its nozzle sockets, or 1 at the hull origin (0 without EngineTrail VFX / once dead). */
	int32 GetExhaustNozzleCount() const { return ExhaustNozzleCount; }

	/** What the exhaust effects are fed as Niagara user floats "Throttle" (0..1, speed / cruise) and "Boost" (0/1). */
	float GetExhaustThrottle01() const { return ExhaustThrottle01; }
	bool GetExhaustBoost() const { return bExhaustBoost; }

protected:
	FVector DeathDrift = FVector::ZeroVector;
	FRotator DeathSpin = FRotator::ZeroRotator;

	// --- Range readability (Lead, art audit): the visual mesh grows with camera distance; collision never does ---
public:
	/** Visual scale is 1 inside this camera distance (cm)... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0"))
	float RangeScaleNearCm = 6000.f;

	/** ...rising linearly to RangeScaleMax at this distance, and capped there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "1"))
	float RangeScaleFarCm = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "1"))
	float RangeScaleMax = 2.5f;

	/** Placeholder engine glow multiplier at RangeScaleFarCm (1 up close): a red-orange spark at range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "1"))
	float RangeEngineGlowMax = 2.f;

	/** Smoothing rate (1/s, real time) so the scale never pops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation", meta = (ClampMin = "0.1"))
	float RangeScaleSmoothing = 4.f;

	/** The curve: 1 at Near, MaxScale at Far, linear between, clamped. */
	static float RangeScaleFor(float DistanceCm, float NearCm, float FarCm, float MaxScale);

	/** Current (smoothed) range multiplier on the visual mesh. */
	float GetRangeScale() const { return RangeScale; }

	/** One step of the range scale toward a view location; bAllowed=false eases back to 1 (director shots, intro). */
	void UpdateRangeScale(const FVector& ViewLocation, bool bAllowed, float RealDeltaSeconds);

protected:
	void TickRangeScale(float DeltaSeconds);
	void ApplyVisualScale();

	FVector BaseVisualScale = FVector::OneVector;
	float RangeScale = 1.f;
	float AppliedRangeScale = 1.f;
	bool bHasMountedParts = false;	// carriers / gunships with hit parts keep true scale (parts sit on the hull)

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> EngineGlowMID;
	FLinearColor EngineGlowBase = FLinearColor::Black;

public:
	/** Gun aim error is an angle (spec AimErrorDeg) capped so a bolt misses by at most this (cm, x difficulty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Enemy", meta = (ClampMin = "0"))
	float GunAimMaxMissCm = 260.f;

	/** Effective gun range against a target closing on us: bolts meet it, so fire opens earlier (cm). */
	float GetGunFireRange(const AActor* Target) const;

	/** Deterministic per-world spawn-order seed for this pilot's random draws (aim error, AI choices). */
	int32 GetSpawnSeed();

	/** Added to every spawn seed (0 = default). Balance distribution runs vary it to sample many fights. */
	static int32& SpawnSeedSalt();

	/** Fix this pilot's random draws to Seed (tests: N seeded passes of the same scenario). */
	void SetSpawnSeed(int32 Seed);

	/** GunAimMaxMissCm scaled by the difficulty in play (set in InitFromSpecData). */
	float AimMaxMissCm = 260.f;
	int32 SpawnSeed = INDEX_NONE;

	/**
	 * Nick's presentation data asset (features.md 3.5). Set explicitly to override; otherwise filled
	 * from ResolveVisuals in InitFromSpec. Empty = placeholder shapes only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDEnemyVisuals> Visuals;

	virtual void Tick(float DeltaSeconds) override;

	/** True hull size including carrier parts mounted outside the sphere (break-off and avoidance). */
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const override;

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

	UFUNCTION()
	void HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

	void StartLoopCues();
	void StopExhaust();
	void UpdateExhaustParams();

	/** The first exhaust (it carries the EngineTrail sound). */
	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> EngineTrailFX;

	/** The other nozzles' exhausts (VFX only). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UNiagaraComponent>> ExtraExhaustFX;

	float ExhaustThrottle01 = 0.f;
	int32 ExhaustNozzleCount = 0;
	bool bExhaustBoost = false;

	UPROPERTY(Transient)
	TObjectPtr<class UNiagaraComponent> DamagedSmokeFX;

	bool bVisualsExplicit = false;
	float DifficultyDamage = 1.f;
	double LastHullHitCueTime = -1.0;

	void BeginDeath(AActor* Killer);
	void Explode();
	const FTDEnemyAttackSpec* FindAttack(ETDEnemyAttack Attack) const;
	void SpawnCarrierParts();
	/** Mounts a carrier part: at the hull mesh's Socket when it has one (+X out), else outside the collision sphere. */
	void MountPart(AActor* Part, const FVector& LocalDirection, float PartHalfExtent, float Up, FName Socket = NAME_None);
	/** Socket-mounted parts sit on the hull itself: the blocking sphere shrinks to the hull's thinnest half-extent so
	 *  shots and rams reach them, and the hull mesh blocks projectiles so its ends stay hittable. Once. */
	void FitCollisionToSocketedHull();
	bool bSocketMounts = false;
	bool bFittedToSockets = false;

public:
	/** Carrier parts mounted at the hull's own sockets (Bay_0..2, Turret_0..3), not around the sphere. */
	bool HasSocketMountedParts() const { return bSocketMounts; }
protected:
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
	int32 TotalLaunched = 0;

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
