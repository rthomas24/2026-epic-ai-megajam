// Triad Drift - the on-foot third-person camera (ENG-32). Owner: Teammate 3 (camera); T5 wires it into the pilot.
//
// One component makes the whole rig: attach it to the pilot's capsule (its location is the pivot) and it creates
// its own camera. In third person it shows an over-the-shoulder chase view (arm 320 cm, shoulder right 55 / up 65,
// location lag 12, rotation lag 18, FOV 85) that looks along the control rotation, so the crosshair stays centred
// and aims where it does in first person. Camera collision: a 12 cm probe swept from inside the capsule to the
// shoulder and then down the arm, against ECC_Camera and against other pawns (crew); the arm snaps in to the
// first hit, so no frame ever has the lens inside a wall, prop, tree or crew, and eases back out smoothly once
// clear. Closer than 60 cm to the pilot it hides the pilot's meshes (SetHiddenInGame, which is separate from the
// visibility T5's body code uses) and shows them again past 70 cm.
//
// Switching (SetThirdPerson) is instant (a cut, features.md ENG-32; BlendSeconds can add a blend) and swaps which
// camera is active on the pawn, even while paused. While the director owns the view (a director shot or the on-foot
// death cam, or anything else that took the view target), a change waits and applies when the view comes back.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TDThirdPersonCameraComponent.generated.h"

class UCameraComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDThirdPersonCameraComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UTDThirdPersonCameraComponent();

	// ---- The API for the pilot (T5) ----

	/** Ask for third person (true) or first person (false). Applies now, or when the director gives the view back. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Camera")
	void SetThirdPerson(bool bWanted);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Camera")
	void ToggleThirdPerson() { SetThirdPerson(!bWantThirdPerson); }

	/** The mode asked for (what the Settings page shows). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	bool WantsThirdPerson() const { return bWantThirdPerson; }

	/** True while the chase camera is the pawn's camera (including the blend in; false once blending back out). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	bool IsThirdPersonActive() const { return bThirdPersonActive; }

	/** The camera to aim from right now (melee targeting, interaction, the deflect return): ours in third person. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	UCameraComponent* GetViewCamera() const;

	/**
	 * The facing rule (ENG-32). MoveVector: the world-space move input or acceleration. Just running: face the
	 * movement. Attacking, blocking or aiming a deflect (bCombatAction): face the camera yaw (strafe). Standing
	 * still with the camera more than 60 deg off the body: turn to the camera yaw. Otherwise keep CurrentYaw.
	 * Returns a yaw in (-180, 180]; the caller turns toward it at its own rate.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	static float GetDesiredActorYaw(const FVector& MoveVector, float CameraYaw, bool bCombatAction, float CurrentYaw);

	/** Optional: the first-person camera to blend from and to (default: the owner's other camera component). */
	void SetFirstPersonCamera(UCameraComponent* InCamera);

	// ---- State (HUD, tests) ----

	UCameraComponent* GetThirdPersonCamera() const { return Camera; }
	bool IsChangePending() const { return bWantThirdPerson != bThirdPersonActive; }
	bool IsBlending() const { return BlendAlpha < 1.f; }
	bool IsBodyHiddenByCamera() const { return bBodyHidden; }
	/** The arm length in use now (cm), after collision. */
	float GetCurrentArmLength() const { return CurrentArm; }

	// ---- Tuning (ENG-32 values) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person", meta = (Units = "cm"))
	float ArmLength = 320.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person", meta = (Units = "cm"))
	float ShoulderRight = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person", meta = (Units = "cm"))
	float ShoulderUp = 65.f;
	/** Pivot location lag (interp speed); 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person")
	float LocationLagSpeed = 12.f;
	/** Rotation lag (interp speed); 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person")
	float RotationLagSpeed = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person", meta = (Units = "cm"))
	float ProbeRadius = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person", meta = (Units = "deg"))
	float FieldOfView = 85.f;
	/** Ease back out after an obstruction clears (interp speed). The pull-in is immediate (never inside geometry). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person")
	float EaseOutSpeed = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person", meta = (Units = "s"))
	float BlendSeconds = 0.f; // features.md ENG-32: the switch is instant (a cut)
	/** Hide the pilot when the lens is closer than this to the capsule (shown again 10 cm further out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person", meta = (Units = "cm"))
	float HideBodyDistance = 60.f;

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Where the chase lens goes for a pivot and a view rotation, collision-resolved (no lag). Tests and tools. */
	FVector ResolveChaseLocation(const FVector& Pivot, const FRotator& View, float& OutSafeArm) const;

private:
	/** True when nobody else (the director, the death cam, a cutscene) owns the view. */
	bool IsViewOurs() const;
	void ApplyMode(bool bThirdPerson);
	/** How far a ProbeRadius sphere travels from From toward To before it touches the level or a pawn (not the pilot). */
	float SweepProbe(const FVector& From, const FVector& To) const;
	FVector SafePoint(const FVector& From, const FVector& To) const;
	FVector ShoulderFor(const FVector& Anchor, const FRotator& View) const;
	/** True when nothing in the level is between the pilot's pivot and a lens position (crew may be). */
	bool HasLineToPilot(const FVector& Pivot, const FVector& Lens) const;
	/** The longest arm up to SafeArm that keeps a clear line from the lens back to the pilot. */
	float ArmWithLineOfSight(const FVector& Pivot, const FVector& Shoulder, const FVector& Back, float SafeArm) const;
	void UpdateChase(float DeltaTime, bool bSnap);
	void UpdateBodyHidden();
	void SetBodyHidden(bool bHide);
	FRotator GetViewRotation() const;
	UCameraComponent* FindFirstPersonCamera() const;

	UPROPERTY(Transient) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(Transient) TWeakObjectPtr<UCameraComponent> FirstPersonCamera;
	UPROPERTY(Transient) TArray<TWeakObjectPtr<UPrimitiveComponent>> HiddenByUs;

	bool bWantThirdPerson = false;
	bool bThirdPersonActive = false;
	/** Blending back to first person: our lens still shows, moving to the first-person eye. */
	bool bBlendingOut = false;
	bool bBodyHidden = false;
	bool bHaveChase = false;
	float BlendAlpha = 1.f;
	FTransform BlendFrom = FTransform::Identity;
	float BlendFromFOV = 90.f;
	FVector LaggedPivot = FVector::ZeroVector;
	FRotator LaggedRotation = FRotator::ZeroRotator;
	float CurrentArm = 0.f;
	FVector ChaseLocation = FVector::ZeroVector;
};
