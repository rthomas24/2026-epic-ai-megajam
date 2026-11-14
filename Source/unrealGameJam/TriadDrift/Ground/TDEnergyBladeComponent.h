// Triad Drift - energy blade (ENG-22). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/TDGroundTypes.h"
#include "TDEnergyBladeComponent.generated.h"

class UTDBladeFX;
class UTDHealthComponent;
class ACharacter;

DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnBladeSwing, ETDBladeState /*Move*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTDOnBladeHit, AActor* /*Victim*/, float /*Damage*/, bool /*bKill*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTDOnBladeDeflect, AActor* /*Projectile*/, bool /*bPerfect*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnBladeParry, AActor* /*Attacker*/);
DECLARE_MULTICAST_DELEGATE(FTDOnGuardBroken);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnBladeCue, FName /*CueName*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnBladeRetract, bool /*bRetracted*/);

/**
 * The on-foot melee weapon. Put it on an ACharacter that has a UTDHealthComponent.
 *
 * Attacks: three-hit light combo (25/25/45, 180 cm, 100° arc, 0.45 s window, the third knocks back),
 * heavy lunge (70, 600 cm dash-stab, breaks guards, 2 s cooldown), air slam (50 in 300 cm on
 * landing), dash strike (35 along a dash).
 *
 * Block (120° front arc, 50% move speed, 3 s of stamina, then guard break):
 *  - shots reaching the front arc are deflected before they hit: toward the crosshair, or, within 0.2 s of
 *    raising the block (PERFECT), back at the shooter at x2 damage with a 0.15 s slow-mo and
 *    OnHighlightMoment(PerfectDeflect)
 *  - melee hits are blocked; a perfect-timed block parries: the attacker staggers for 1.2 s and the next
 *    blade hit crits
 * Hit-stop: 50 ms on hit, 120 ms on kill.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDEnergyBladeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDEnergyBladeComponent();

	// ------------------------------------------------------------------ tuning (features.md ENG-22)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Light")
	TArray<FTDBladeSwing> LightCombo;

	/** After a light swing finishes, the next press within this window continues the combo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Light", meta = (Units = "s"))
	float ComboWindow = 0.45f;

	/** Holding attack this long turns it into a heavy lunge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Heavy", meta = (Units = "s"))
	float HeavyHoldSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Heavy")
	float HeavyDamage = 70.f;

	/** A light swing's hit lands this long after the press, with the hand (the pilot clips reach the target 0.10-0.13 s
	 *  in; the animation audit). 0 = on the press. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Light", meta = (Units = "s", ClampMin = "0"))
	float HitDelay = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Heavy", meta = (Units = "cm"))
	float HeavyLungeDistance = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Heavy", meta = (Units = "s"))
	float HeavyLungeDuration = 0.2f;

	/** The lunge's stab lands at the clip's thrust (A_Pilot_Heavy_v4: full extension 0.13 s at x1.4), not on its first frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Heavy", meta = (Units = "s", ClampMin = "0"))
	float HeavyHitDelay = 0.13f;

	/** Sweep radius of the lunge's stab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Heavy", meta = (Units = "cm"))
	float HeavyHitRadius = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Heavy", meta = (Units = "s"))
	float HeavyCooldown = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|AirSlam")
	float AirSlamDamage = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|AirSlam", meta = (Units = "cm"))
	float AirSlamRadius = 300.f;
	/** Where the last air slam met the floor and its normal (the effect plays there, lying on it). */
	FVector GetLastSlamImpact() const { return LastSlamImpact; }
	FVector GetLastSlamNormal() const { return LastSlamNormal; }

	/** Downward speed of the plunge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|AirSlam", meta = (Units = "cm/s"))
	float AirSlamSpeed = 3000.f;

	/** An air slam that never touches down ends after this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|AirSlam", meta = (Units = "s"))
	float AirSlamTimeout = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|DashStrike")
	float DashStrikeDamage = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|DashStrike", meta = (Units = "cm"))
	float DashStrikeRadius = 120.f;

	/** Full width of the guarded front arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Block", meta = (Units = "deg"))
	float BlockArcDegrees = 120.f;

	/** Move speed multiplier while blocking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Block")
	float BlockMoveSpeedScale = 0.5f;

	/** Seconds of continuous blocking before the guard breaks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Block", meta = (Units = "s"))
	float BlockStaminaSeconds = 3.f;

	/** Extra stamina (seconds) each blocked hit costs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Block", meta = (Units = "s"))
	float BlockHitStaminaCost = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Block", meta = (Units = "s"))
	float StaminaRegenDelay = 0.6f;

	/** Stamina seconds regained per second when not blocking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Block")
	float StaminaRegenRate = 1.5f;

	/** How long a guard break stops you blocking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Block", meta = (Units = "s"))
	float GuardBreakSeconds = 1.2f;

	/** A block raised this recently is perfect: deflects home on the shooter, melee is parried. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect", meta = (Units = "s"))
	float PerfectWindow = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect")
	float PerfectDeflectDamageMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect")
	float NormalDeflectDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect")
	float DeflectSpeedMultiplier = 1.25f;

	/** Shots are caught when they come this close (plus one frame of travel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect", meta = (Units = "cm"))
	float DeflectRadius = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect", meta = (Units = "s"))
	float PerfectDeflectSlowMoSeconds = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect")
	float PerfectDeflectSlowMoDilation = 0.25f;

	/** Importance passed to the director for a perfect deflect (docs/eng/director.md: 0.6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Deflect", meta = (ClampMin = "0", ClampMax = "1"))
	float PerfectDeflectImportance = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Parry", meta = (Units = "s"))
	float ParryStaggerSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Parry")
	float CritMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Feel", meta = (Units = "s"))
	float HitStopOnHit = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Feel", meta = (Units = "s"))
	float HitStopOnKill = 0.12f;

	/** Nick's blade presentation (optional; an empty slot does nothing). Loads /Game/TriadDrift/Presentation/DA_Blade_FX when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blade|Presentation")
	TObjectPtr<UTDBladeFX> BladeFX;

	// ------------------------------------------------------------------ input (the character routes these)
	/** Attack button down: the light / air-slam / dash-strike lands on the press; holding 0.35 s with no hit becomes a heavy lunge. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	void PressAttack();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	void ReleaseAttack();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	void SetBlockHeld(bool bHeld);

	// ------------------------------------------------------------------ moves (direct, for AI-free tests and combos)
	/** Next light swing (or queues it if a swing is still playing). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	bool LightAttack();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	bool HeavyLunge();

	/** Only while airborne. Damage lands when the owner touches down (NotifyOwnerLanded). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	bool AirSlam();

	/** Strike along a dash path from the owner's position (called by the character mid-dash). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	bool DashStrike(FVector Direction, float Distance);

	/** The character calls this from Landed(). */
	void NotifyOwnerLanded();

	// ------------------------------------------------------------------ state (HUD / anim)
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	ETDBladeState GetBladeState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	bool IsBlocking() const { return State == ETDBladeState::Blocking; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	bool IsGuardBroken() const { return State == ETDBladeState::GuardBroken; }

	/** True during a swing, lunge or slam: the character shouldn't sprint or slide. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	bool IsAttacking() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	bool IsInPerfectWindow() const { return IsBlocking() && BlockTime <= PerfectWindow; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	float GetBlockStamina01() const { return BlockStaminaSeconds > 0.f ? Stamina / BlockStaminaSeconds : 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	float GetHeavyCooldown01() const { return HeavyCooldown > 0.f ? 1.f - FMath::Clamp(HeavyCooldownLeft / HeavyCooldown, 0.f, 1.f) : 1.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	int32 GetComboIndex() const { return ComboIndex; }

	/** 0..1 through the current swing / lunge (0 when idle): drives the blade pose and Nick's AnimBP. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	float GetMoveProgress01() const { return (IsAttacking() && MoveDuration > 0.f) ? FMath::Clamp(1.f - SwingTimeLeft / MoveDuration, 0.f, 1.f) : 0.f; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	bool HasCritReady() const { return bCritReady; }

	/** Move-speed multiplier the character applies (block slows you). */
	float GetMoveSpeedScale() const { return IsBlocking() ? BlockMoveSpeedScale : 1.f; }

	/** Is World location inside the guarded front arc? */
	bool IsInFrontArc(const FVector& WorldLocation) const;

	/** Tries to catch one projectile now (used by the per-tick scan; public for tests). */
	bool TryDeflect(AActor* Projectile);

	/** Put the blade away (death, boarding the ship): plays Retract. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	void Retract();

	/** Draw it again (respawn, leaving the ship): plays Ignite. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	void Ignite();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Blade")
	bool IsRetracted() const { return bRetracted; }

	/** Fresh blade: full stamina, no cooldowns, no crit, no guard break (respawn / new take). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Blade")
	void ResetState();

	/** Interrupts everything (death, carrying an objective). */
	void CancelAll();

	FTDOnBladeSwing OnSwing;
	FTDOnBladeHit OnBladeHit;
	FTDOnBladeDeflect OnDeflect;
	FTDOnBladeParry OnParry;
	FTDOnGuardBroken OnGuardBroken;
	/** Every presentation cue request by name, even when the slot is empty (tests, debugging). */
	FTDOnBladeCue OnCueRequested;
	FTDOnBladeRetract OnRetractChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartSwing(int32 Index);
	int32 HitArc(float Range, float ArcDegrees, float Damage, float Knockback);
	bool HitActor(AActor* Victim, float Damage, const FVector& HitLocation, const FVector& HitDir, float Knockback);
	void ScanForProjectiles(float DeltaTime);
	void BreakGuard();
	void PlayBladeCue(FName Name, const struct FTDFXCue* Cue, bool bAttached, const FVector& Location, const FRotator* Rotation = nullptr);
	float FilterIncomingDamage(const FTDDamageInfo& Info);
	void GetView(FVector& OutLocation, FVector& OutForward) const;
	ETDTeam GetOwnerTeam() const;
	ACharacter* GetOwnerCharacter() const;

	ETDBladeState State = ETDBladeState::Idle;
	int32 ComboIndex = 0;           // swings landed in the current combo (0 = none)
	float SwingTimeLeft = 0.f;      // lockout of the current move
	float ComboTimeLeft = 0.f;      // window to continue the combo after a swing
	bool bLightQueued = false;
	// A light swing's hit, waiting for its active frame (HitDelay).
	float PendingHitLeft = -1.f;
	float PendingRange = 0.f, PendingArc = 0.f, PendingDamage = 0.f, PendingKnockback = 0.f;
	void PlaySwingTrail();
	bool bAttackHeld = false;
	float AttackHeldTime = 0.f;
	bool bHeavyFiredThisPress = false;
	/** A press from rest waits: released before HeavyHoldSeconds it is a light swing, held it goes straight to the heavy. */
	bool bLightDeferred = false;
	/** The heavy's chosen victim, stabbed when HeavyHitDelay runs out (if the lunge is still on). */
	TWeakObjectPtr<AActor> PendingHeavyTarget;
	float PendingHeavyLeft = -1.f;
	FVector PendingHeavyDir = FVector::ForwardVector;
	void ApplyHeavyHit(AActor* Target, const FVector& Dir);
	bool bBlockHeld = false;
	float BlockTime = 0.f;
	float Stamina = 3.f;
	float TimeSinceBlock = 100.f;
	float GuardBrokenLeft = 0.f;
	float HeavyCooldownLeft = 0.f;
	float LungeTimeLeft = 0.f;
	FVector LungeVelocity = FVector::ZeroVector;
	bool bCritReady = false;
	bool bRetracted = false;
	float MoveDuration = 0.f;
	bool bPressSwingLanded = false;
	int32 HitCounter = 0;

	UPROPERTY(Transient)
	TObjectPtr<UTDHealthComponent> OwnerHealth;

private:
	FVector LastSlamImpact = FVector::ZeroVector;
	FVector LastSlamNormal = FVector::UpVector;
};
