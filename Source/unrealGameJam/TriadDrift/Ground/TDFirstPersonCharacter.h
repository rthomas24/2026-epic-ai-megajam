// Triad Drift - first-person character for planets and the Dominion interior (ENG-21). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Ground/TDGroundTypes.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TDFirstPersonCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UTDHealthComponent;
class UTDEnergyBladeComponent;
class UTDThirdPersonCameraComponent;
class UAnimSequenceBase;
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

	/** Pad (L3 click): sprint on until the stick is released or eased under SprintToggleMinStick. Shift stays hold. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ground")
	void ToggleSprint() { bSprintToggled = !bSprintToggled; }
	bool IsSprintToggled() const { return bSprintToggled; }
	/** Stick deflection under which a toggled sprint ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ground")
	float SprintToggleMinStick = 0.3f;

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

	// ------------------------------------------------------------------ pilot body (N-021)
	/** The pilot's look. Empty: DA_Pilot_Visuals (Presentation, else Gameplay/Characters/**) at BeginPlay; none: no body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Body")
	TObjectPtr<class UTDPilotVisuals> PilotVisuals;

	/** In first person the body sits this far behind the camera so looking down never shows the inside of the chest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Body", meta = (Units = "cm"))
	float FirstPersonBodyBack = 18.f;

	/** Ground speeds the walk and run loops were authored for: playback rate follows the real speed (less foot sliding). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Body", meta = (Units = "cm/s"))
	float BodyWalkSpeed = 130.f; // measured from the planted foot (animation builder, Nov 14): Walk_v2 130, Run 487
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Body", meta = (Units = "cm/s"))
	float BodyRunSpeed = 487.f;

	/** Applies PilotVisuals (loads the default when empty). Public for tests. */
	void ApplyPilotVisuals();
	bool HasBody() const;
	/** Which loop a ground speed plays: 0 idle, 1 walk, 2 run. */
	static int32 ChooseBodyLoop(float GroundSpeed);
	/** First person: head hidden, body pushed back. Other views (director shots, death, boarding): the full body. */
	void SetBodyFirstPersonView(bool bFirstPerson);
	bool IsBodyFirstPersonView() const { return bBodyFirstPerson; }

	// ------------------------------------------------------------------ third person (ENG-32)
	/** Third person (true) or first person: the rig applies it now, or when a director shot / the death cam ends. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Camera")
	void SetThirdPerson(bool bWanted);
	/** The chase camera is the view (the body is shown whole, the blade is in the hand, facing follows the rule). */
	/**
	 * Who is on foot (Nicholas, issue #29): the pilot of the ship the player flew down / docked with, from
	 * Progress.LastShip (T1 records it at the entry commit; the planet hand-off writes the landed ship).
	 * Rift = Wren, Lancer = Kato, Bastion = Moss. For T3 (the on-foot speaker) and T2.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|OnFoot", meta = (WorldContext = "WorldContextObject"))
	static ETDShipId GetOnFootPilotShip(const UObject* WorldContextObject);
	/** "Wren" / "Kato" / "Moss" (the speaker names the bark and story systems use). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|OnFoot", meta = (WorldContext = "WorldContextObject"))
	static FName GetOnFootPilotName(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	bool IsThirdPersonView() const;
	UTDThirdPersonCameraComponent* GetThirdPersonRig() const { return ThirdPersonRig; }
	/** The camera aimed from now: the chase camera in third person, else the first-person one. */
	UCameraComponent* GetViewCamera() const;
	/** The hand blade shown in third person (hilt and beam), and how far the beam is out (0 retracted .. 1). */
	UStaticMeshComponent* GetHandBladeBeam() const { return HandBladeBeam; }
	UStaticMeshComponent* GetHandBladeHilt() const { return HandBladeHilt; }
	float GetHandBladeExtend01() const { return HandBladeExtend; }
	/** Turn rate toward the facing rule's yaw in third person (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Camera", meta = (Units = "deg"))
	float ThirdPersonTurnRate = 900.f;
	/** Motion blur on foot, both views (overrides the planet volume's 0.2: fast crew and drones smeared). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float OnFootMotionBlur = 0.05f;
	/** Beam length in the hand (the art is 100 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Body", meta = (Units = "cm"))
	float HandBladeLength = 95.f;
	/** Where the hilt sits on the hand bone when the mesh has no BladeSocket (set by eye on SK_Pilot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Body")
	FTransform HandBladeOffset;
	/** The pilot animation that plays for a blade state / move right now (tests, TD.Ground.Bones). */
	FName GetBodyActionName() const { return BodyActionName; }
	/**
	 * Look input shaped by the player's settings (Settings page rows, ENG-32 review): mouse sensitivity and invert Y,
	 * or for the stick its dead zone (radial, rescaled), sensitivity and invert Y. Returns (yaw, pitch) to add.
	 */
	static FVector2D ShapeLook(const FVector2D& Raw, bool bGamepad, const struct FTDPlayerSettings& Settings);
	/** Base stick turn per frame at full deflection (sensitivity 1). */
	static constexpr float StickLookScale = 3.f;
	/** Apply a look input as the player's device would (public for tests and agents). */
	void ApplyLookInput(const FVector2D& Raw, bool bGamepad);

	/** Eye point and aim: in third person the eye aims at what the crosshair (the chase camera's centre) is on. */
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	// ------------------------------------------------------------------ sound (ENG-35)
	/** Steps on soft ground (Verdance) and rock (Cinder, or any map named *Cinder*). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio") TSoftObjectPtr<class USoundBase> StepSoilSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio") TSoftObjectPtr<class USoundBase> StepRockSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio") TSoftObjectPtr<class USoundBase> DashSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio") TSoftObjectPtr<class USoundBase> JumpSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio") TSoftObjectPtr<class USoundBase> LandSound;
	/** 2D: the player's own hurt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio") TSoftObjectPtr<class USoundBase> HurtSound;
	/** Ground covered between two steps (run / sprint). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio", meta = (Units = "cm")) float StepStride = 170.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio", meta = (Units = "cm")) float SprintStepStride = 220.f;
	/** Rock steps instead of soil (set from the map at BeginPlay). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Audio") bool bRockyGround = false;
	/** The last sound events (StepSoil, StepRock, Dash, Jump, Land, Hurt): tests and TD.Ground.Sounds. */
	const TArray<FName>& GetSoundEvents() const { return SoundEvents; }

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
	/** Climb into the ship toward Hatch (true) or step down out of it where the pilot stands (false): plays the clip,
	 *  puts the blade away and moves the capsule; input waits until it ends. Returns the clip's length (0: none). */
	float StartShipTransition(bool bBoarding, const FVector& Hatch);
	bool IsInShipTransition() const { return bShipTransition; }
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
	/** A short tip on the on-foot prompt line (the guard's lunge hint), shown when nothing interactable is in view. */
	void ShowTip(const FText& Text, float Seconds);
	FText GetTip() const { return TipLeft > 0.f ? Tip : FText::GetEmpty(); }
	/** Blocked slashes on a guard this run (the lunge hint repeats every 5). Returns the new count. */
	int32 CountGuardedHit() { return ++GuardedHits; }
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
	void UpdateBody(float DeltaSeconds);
	void PlaceBody();
	bool bBodyFirstPerson = false;
	bool bBodyViewKnown = false;
	UPROPERTY(Transient) TObjectPtr<class UTDPilotVisuals> AppliedPilotVisuals;

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

	/** ENG-32 rig (T3): the chase camera, its collision and the director-safe switch. */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Camera")
	TObjectPtr<UTDThirdPersonCameraComponent> ThirdPersonRig;

	/** The blade in the body's right hand (third person and every view that shows the whole pilot). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Body")
	TObjectPtr<UStaticMeshComponent> HandBladeHilt;
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Body")
	TObjectPtr<UStaticMeshComponent> HandBladeBeam;

private:
	FText Tip;
	float TipLeft = 0.f;
	int32 GuardedHits = 0;
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
	// third person
	UFUNCTION()
	void HandleCameraModeChanged(ETDOnFootCamera NewMode);
	void UpdateFacing(float DeltaSeconds);
	void AttachHandBlade();
	void ApplyHandBladeVisuals();
	void UpdateHandBlade(float DeltaSeconds);
	void UpdateBodyActions();
	void PlayBodyAction(FName Anim, float Rate = 1.f, bool bUpperOnly = false);
	/** The first of these clips that exists (a newer version before its older one); true when one played. */
	bool PlayFirstBodyAction(std::initializer_list<const TCHAR*> Anims, float Rate = 1.f, bool bUpperOnly = false);
	UAnimSequenceBase* LoadPilotAnim(FName Anim) const;
	void HandleBladeDeflect(AActor* Projectile, bool bPerfect);
	void StartDeathRagdoll();
	FTimerHandle DeathRagdollTimer;
	// Climbing into / out of the ship (A_Pilot_Board_Ship 1.10 s, A_Pilot_Exit_Ship 0.63 s, in place: the capsule moves).
	bool bShipTransition = false;
	bool bShipBoarding = false;
	float ShipTransitionTime = 0.f;
	float ShipTransitionDuration = 0.f;
	FVector ShipTransitionFrom = FVector::ZeroVector;
	FVector ShipTransitionTo = FVector::ZeroVector;
	FVector ShipTransitionLast = FVector::ZeroVector;
	void TickShipTransition(float DeltaSeconds);
	/** The blade is put away while an interaction clip plays (it sweeps through the body), drawn again after. */
	float InteractRetractLeft = 0.f;
	float HandBladeExtend = 1.f;
	/** Standing still, the camera went > 60 deg off: keep turning until the body faces it (not just back under 60). */
	bool bIdleTurnToCamera = false;
	/** Attacking last frame (third person): a new attack snaps the body to the camera at once. */
	bool bFacingWasAttacking = false;
	TArray<FName> SoundEvents;
	float StepDistance = 0.f;
	float FallStartZ = 0.f;
	double LastHurtSoundTime = -10.0;
	void PlayFootSound(const TSoftObjectPtr<class USoundBase>& Sound, FName Event, bool b2D = false);
	void UpdateFootsteps(float DeltaSeconds);
	/** T3's melee framing: a live crewman this close (or the blade attacking / blocking) lifts the chase shoulder. */
	float CloseCombatRange = 400.f;
	float CloseCombatScan = 0.f;
	bool bEnemyClose = false;
	/** What the pilot last asked the rig for (only changes are sent: a test or the director may set it too). */
	bool bLastCloseCombat = false;
	void UpdateCloseCombat(float DeltaSeconds);
	/** Body, head, blades and turning for the current view (Tick, and at once on a mode change, even paused). */
	void ApplyViewState();
	ETDBladeState LastBodyBladeState = ETDBladeState::Idle;
	bool bLastBodyDashing = false;
	bool bLastBodySliding = false;
	bool bLastBodyMantling = false;
	FName BodyActionName;

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
	bool bSprintToggled = false;
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
	/** The right stick, apart from the mouse: its own sensitivity, dead zone and invert (the Settings page rows). */
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_LookStick;
	void OnLookStick(const FInputActionValue& Value);
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_SprintToggle;
	void OnSprintTogglePressed() { ToggleSprint(); }
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
