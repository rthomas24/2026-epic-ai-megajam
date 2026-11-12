// Triad Drift - first-person character for planets and the Dominion interior (ENG-21). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Ground/TDGroundTypes.h"
#include "TDFirstPersonCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UTDHealthComponent;
class UTDEnergyBladeComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnInteractCompleted, AActor* /*Target*/);

/**
 * Run 650 / sprint 950, jump Z 650 + double jump Z 500, dash (1200 cm in 0.18 s, 0.15 s i-frames,
 * 2 charges, 1.2 s regen), slide (Ctrl while sprinting, 0.7 s), auto-mantle up to 180 cm in 0.35 s,
 * interact (tap/hold, interrupted by damage), objective ping. 100 health regenerating 15/s after 4 s.
 * Death reports the player defeated to the game flow (CUT!, next take).
 *
 * Input is built in C++ (no assets) when a player possesses it. Every move also has a public function
 * so tests and AI-driven checks can drive it without input.
 */
UCLASS(Config = Game)
class UNREALGAMEJAM_API ATDFirstPersonCharacter : public ACharacter, public ITDTeam, public ITDHUDData
{
	GENERATED_BODY()

public:
	ATDFirstPersonCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ground")
	FTDGroundMoveTuning Tuning;

	// ------------------------------------------------------------------ moves
	/** X = right, Y = forward, each -1..1. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void SetMoveInput(FVector2D Input) { MoveInput = Input.GetClampedToMaxSize(1.f); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void SetSprintHeld(bool bHeld) { bSprintHeld = bHeld; }

	/** Jump from the ground, or double jump once in the air. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	bool TryJump();

	/** Dash along the move input (or facing). Spends a charge; grants i-frames. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	bool TryDash();

	/** Only while sprinting on the ground. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	bool TrySlide();

	/** Climb a ledge MantleMinHeight..MantleMaxHeight tall directly ahead. Also runs automatically. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	bool TryMantle();

	/** Ctrl: slide when sprinting on the ground, otherwise dash. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void CrouchAction();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void BeginInteract();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void EndInteract();

	/** Shows the active objective's waypoint for PingSeconds. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void PingObjective();

	/** Carrying an objective item: no sprint, no block, no attacks (ENG-25 wreck core). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void SetCarrying(bool bNewCarrying);

	/**
	 * Drive the body at a fixed velocity for Duration (dash, heavy lunge), ignoring gravity, then leave
	 * it at ExitSpeed along the same direction.
	 */
	void StartForcedMove(const FVector& Velocity, float Duration, float ExitSpeed);

	/** Back to full health and control at a spot (checkpoint respawn / new take). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void RespawnAt(const FTransform& Where);

	/** Put back on the last solid ground stood on (magma). Floors tagged TD.Unsafe never count as safe. */
	void ReturnToSafeGround();
	FVector GetLastSafeLocation() const { return LastSafeLocation; }

	// ------------------------------------------------------------------ state (HUD / anim)
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsSprinting() const { return bSprinting; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsSliding() const { return bSliding; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsDashing() const { return bDashing; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsMantling() const { return bMantling; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsBlocking() const { return bBlocking; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsCarrying() const { return bCarrying; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsDead() const;
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") int32 GetJumpsUsed() const { return JumpsUsed; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") float GetDashRecharge01() const;
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") float GetInteractProgress01() const;
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") AActor* GetInteractTarget() const { return InteractTarget.Get(); }
	/** The interactable being held right now (null when not holding). */
	AActor* GetHoldTarget() const { return HoldTarget.Get(); }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") bool IsObjectivePinged() const { return PingTimeLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ground") ETDBladeState GetBladeState() const;

	FVector GetDashDirection() const { return ForcedDirection; }
	float GetDashRemainingDistance() const { return bDashing ? ForcedVelocity.Size() * ForcedTimeLeft : 0.f; }

	UCameraComponent* GetCamera() const { return Camera; }
	UTDHealthComponent* GetHealth() const { return Health; }
	UTDEnergyBladeComponent* GetBlade() const { return Blade; }
	UStaticMeshComponent* GetBladeMesh() const { return BladePlaceholder; }
	USceneComponent* GetBladeGrip() const { return BladeGrip; }

	/** Applies the blade's UTDBladeFX: BladeMesh if set, otherwise the placeholder tinted with BladeColor. */
	void ApplyBladeVisuals();
	bool IsBladeMeshFromArt() const { return bBladeMeshFromArt; }

	FTDOnInteractCompleted OnInteractCompleted;

	/** The C++-built ground mapping context (tests check every action has its keys). */
	UInputMappingContext* GetGroundInputContext() const { return InputContext; }

	// Animation state for Nick's AnimBP (features.md §3.5), mirrored every tick.
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TriadDrift|Anim") ETDBladeState BladeState = ETDBladeState::Idle;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TriadDrift|Anim") bool bSprinting = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TriadDrift|Anim") bool bSliding = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TriadDrift|Anim") bool bDashing = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TriadDrift|Anim") bool bBlocking = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TriadDrift|Anim") bool bMantling = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TriadDrift|Anim") float Speed01 = 0.f;

	// ITDTeam
	virtual ETDTeam GetTeam() const override { return ETDTeam::Player; }

	// ITDHUDData (ground half)
	virtual float GetHealth01() const override;
	virtual float GetBlockStamina01() const override;
	virtual int32 GetDashCharges() const override { return DashCharges; }
	virtual FText GetInteractPrompt() const override;
	virtual TArray<AActor*> GetObjectiveMarkers() const override;
	/** The current objective line (the flow keeps the last one anyone broadcast). */
	virtual FText GetObjectiveText() const override;
	virtual float GetSpeed01() const override { return Speed01; }

	// ACharacter
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void UnPossessed() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Ground")
	TObjectPtr<UCameraComponent> Camera;

	/** Placeholder blade (engine cube) until Nick's blade mesh arrives. */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Ground")
	TObjectPtr<UStaticMeshComponent> BladePlaceholder;

	/** The hand: posed each frame; the blade and hilt hang off it (blade along +Z). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Ground")
	TObjectPtr<USceneComponent> BladeGrip;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Ground")
	TObjectPtr<UStaticMeshComponent> BladeHilt;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Ground")
	TObjectPtr<UTDHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Ground")
	TObjectPtr<UTDEnergyBladeComponent> Blade;

private:
	void UpdateMoveSpeed();
	void TickDash(float DeltaSeconds);
	void TickForcedMove(float DeltaSeconds);
	void TickSlide(float DeltaSeconds);
	void TickMantle(float DeltaSeconds);
	void TickInteract(float DeltaSeconds);
	bool FindMantleTarget(FVector& OutTarget) const;
	AActor* TraceInteractable() const;
	void CancelInteract();
	void BuildInput();
	void HandleBladeRetract(bool bRetracted);
	void UpdateBladePose(float DeltaSeconds);
	bool bBladeMeshFromArt = false;

	UFUNCTION()
	void HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

	UFUNCTION()
	void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	// Input handlers
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnMoveReleased(const FInputActionValue& Value) { SetMoveInput(FVector2D::ZeroVector); }
	void OnJumpPressed() { TryJump(); }
	void OnSprintPressed() { SetSprintHeld(true); }
	void OnSprintReleased() { SetSprintHeld(false); }
	void OnDashPressed() { TryDash(); }
	void OnCrouchPressed() { CrouchAction(); }
	void OnAttackPressed();
	void OnAttackReleased();
	void OnBlockPressed();
	void OnBlockReleased();
	void OnInteractPressed() { BeginInteract(); }
	void OnInteractReleased() { EndInteract(); }
public:
	UInputAction* GetPauseAction() const { return IA_Pause; }
protected:
	void OnPingPressed() { PingObjective(); }

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bSprintHeld = false;
	bool bCarrying = false;
	FVector LastSafeLocation = FVector::ZeroVector;
	float SafeSampleTimer = 0.f;
	void TickSafeGround(float DeltaSeconds);
	int32 JumpsUsed = 0;

	int32 DashCharges = 2;
	float DashRechargeTime = 0.f;

	bool bForcedMove = false;
	FVector ForcedVelocity = FVector::ZeroVector;
	FVector ForcedDirection = FVector::ForwardVector;
	float ForcedTimeLeft = 0.f;
	float ForcedExitSpeed = 0.f;

	float SlideTimeLeft = 0.f;
	FVector SlideDirection = FVector::ForwardVector;

	FVector MantleStart = FVector::ZeroVector;
	FVector MantleEnd = FVector::ZeroVector;
	float MantleTime = 0.f;
	float MantleCooldown = 0.f;

	TWeakObjectPtr<AActor> InteractTarget;
	TWeakObjectPtr<AActor> HoldTarget;
	float HoldProgress = 0.f;
	float HoldRequired = 0.f;

	float PingTimeLeft = 0.f;

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> InputContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Dash;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Crouch;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Attack;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Block;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Interact;
	/** Escape / Start on foot: forwarded to the controller's OnPauseInput (IMC_SpaceCommon is off on foot). */
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Pause;
	void OnPausePressed();
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Ping;
};
