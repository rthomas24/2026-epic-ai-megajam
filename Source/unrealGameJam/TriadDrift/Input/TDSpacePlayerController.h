// Triad Drift - space player controller and runtime Enhanced Input setup (ENG-02). Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDSpacePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class ATDShip;
class ATDSpacecraft;
struct FInputActionInstance;

/** Which input layer is active on top of the shared space controls. */
UENUM(BlueprintType)
enum class ETDFlightContext : uint8
{
	Space,      // IMC_Space: Hyperdrive + Squad command
	Atmosphere  // IMC_Atmosphere: Land instead of Hyperdrive; F/A = Land
};

/** Index of a power input: signature power 1, signature power 2 (Bastion hold-Q), unlocked ability. */
UENUM(BlueprintType)
enum class ETDPowerInput : uint8
{
	Signature1,
	Signature2,
	Ability
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnWeaponInput, ETDWeaponSlot, Slot, bool, bHeld);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnPowerInput, ETDPowerInput, Power);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnHoldInput, bool, bHeld);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnPauseChanged, bool, bPaused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnSwitchDirectInput, ETDShipId, Ship);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnSwitchStepInput, int32, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnSimpleInput);

/**
 * Builds every space input action and mapping context in C++ at runtime (no input assets), for
 * keyboard + mouse and gamepad (features.md ENG-02), and routes them:
 *  - flight      -> possessed ATDSpacecraft::SetFlightInput (mouse is a virtual joystick)
 *  - triggers    -> T2's UTDWeaponComponent on the pawn
 *  - switching   -> T3's ATDSquadManager
 *  - target      -> T2's UTDTargetingComponent
 * Every routed input is also broadcast on a delegate below, so systems that don't exist yet
 * (powers, hyperdrive, pause, galaxy map, land) can bind without touching this class.
 */
UCLASS()
class UNREALGAMEJAM_API ATDSpacePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATDSpacePlayerController();

	/** Swap between the Space and Atmosphere layers (ENG-20 calls this on atmospheric entry / exit). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Input")
	void SetFlightContext(ETDFlightContext NewContext);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	ETDFlightContext GetFlightContext() const { return FlightContext; }

	/** Enables / disables all space flight input (cutscenes, menus, ground sections). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Input")
	void SetSpaceInputEnabled(bool bEnabled);

	/** Mouse virtual-joystick position, -1..1 on each axis (HUD draws the reticle from this). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	FVector2D GetVirtualStick() const { return VirtualStick; }
	float GetMouseSensitivity() const { return MouseSensitivity; }
	bool IsMousePitchInverted() const { return bInvertMousePitch; }

	/** The aim the ship is currently steering with (-1..1), after deadzone, from mouse or gamepad. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	FVector2D GetEffectiveAim() const { return CurrentFlightInput.Aim; }

	/** True when the last aim input came from a gamepad stick (HUD hides the mouse reticle). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	bool IsUsingGamepad() const { return bUsingGamepad; }

	/** Switch radial (hold D-pad up): open flag and the ship currently highlighted by the right stick. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	bool IsSwitchRadialOpen() const { return bRadialOpen; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	bool GetRadialSelection(ETDShipId& OutShip) const;

	/** World-space aim direction the weapons use: through the virtual-stick reticle on screen. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	FVector GetAimWorldDirection() const;

	/** As GetAimWorldDirection for an explicit viewport size (tests, offscreen). Uses the ship's own camera. */
	FVector GetAimWorldDirectionForViewport(int32 SizeX, int32 SizeY) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Input")
	const FTDFlightInput& GetCurrentFlightInput() const { return CurrentFlightInput; }

	/** Radial deadzone + response curve: |v| in [Deadzone, 1] remapped to [0, 1], then ^Exponent. */
	static FVector2D ShapeStick(const FVector2D& Raw, float Deadzone, float Exponent);

	/** The runtime-built objects (for tests, debug UI and rebinding later). */
	UInputMappingContext* GetCommonContext() const { return IMC_SpaceCommon; }
	UInputMappingContext* GetSpaceContext() const { return IMC_Space; }
	UInputMappingContext* GetAtmosphereContext() const { return IMC_Atmosphere; }
	UInputAction* FindActionByName(FName ActionName) const;

	// ---- Routed-input events -----------------------------------------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnWeaponInput OnWeaponInput;

	/** Q tap = Signature1, Q hold = Signature2 (Bastion), E = Ability. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnPowerInput OnPowerInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnSwitchStepInput OnSwitchStepInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnSwitchDirectInput OnSwitchDirectInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnHoldInput OnSwitchRadialInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnSimpleInput OnTargetCycleInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnHoldInput OnHyperdriveInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnSimpleInput OnSquadCommandInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnSimpleInput OnLandInput;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnSimpleInput OnPauseInput;

	// ---- Pause (Escape / Start, in space and on foot) ------------------------------------------------
	/** The player paused or resumed the game (a pause menu opens / closes on this). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Pause")
	FTDOnPauseChanged OnPauseChanged;

	/** Pauses or resumes. Refused (false) during level travel, a pending CUT retake, or the victory sequence. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Pause")
	bool SetPlayerPaused(bool bPause);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Pause")
	bool TogglePlayerPause() { return SetPlayerPaused(!bPausedByPlayer); }

	/** The game is paused because the player paused it (not a loading or engine pause). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Pause")
	bool IsGamePausedByPlayer() const { return bPausedByPlayer; }

	/** The galaxy map key (M / Select) and OnGalaxyMapInput. Off until the galaxy map screen exists. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TriadDrift|Input")
	bool bGalaxyMapEnabled = false;

	/** Two presses of A / D (or two flicks of the left stick) within this window dodge to that side (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input")
	float DodgeDoubleTapWindow = 0.3f;

	/** Whether a pause is allowed right now, and why not. */
	bool CanPauseNow(FText* OutReason = nullptr) const;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Input")
	FTDOnSimpleInput OnGalaxyMapInput;

	// ---- APlayerController -------------------------------------------------------------------------
	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// ---- Tuning ---------------------------------------------------------------------------------
	/** Mouse counts -> virtual stick units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Mouse")
	float MouseSensitivity = 0.0035f;

	/** Virtual stick radius below which aim is zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Mouse", meta = (ClampMin = "0", ClampMax = "0.9"))
	float MouseDeadzone = 0.06f;

	/**
	 * Virtual stick re-centres at this rate (1/s) when the mouse is still. 0 = stays put.
	 * 3 brings a released stick to ~5% (inside the deadzone) in about a second, so a mouse player stops turning
	 * and the hull settles wings-level instead of circling banked forever (Lead playtest, Nov 12).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Mouse", meta = (ClampMin = "0"))
	float MouseRecenterRate = 3.f;

	/** Response exponent on the virtual stick (1 = linear, >1 = finer control near centre). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Mouse", meta = (ClampMin = "0.5", ClampMax = "4"))
	float MouseResponseExponent = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Mouse")
	bool bInvertMousePitch = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Gamepad", meta = (ClampMin = "0", ClampMax = "0.9"))
	float StickDeadzone = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Gamepad", meta = (ClampMin = "0.5", ClampMax = "4"))
	float StickResponseExponent = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Gamepad")
	bool bInvertStickPitch = false;

	/** W / RT above neutral raise the ship's top speed by up to this fraction of cruise (0 = W does nothing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input", meta = (ClampMin = "0", ClampMax = "1"))
	float ThrottleUpBonus = 0.2f;

	/** A mouse movement must be at least this many counts to take aim back from the gamepad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Mouse", meta = (ClampMin = "0"))
	float MouseTakeOverThreshold = 3.f;

	/** The reticle reaches this fraction of half the short screen side at full virtual-stick deflection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Mouse", meta = (ClampMin = "0", ClampMax = "1"))
	float ReticleScreenScale = 0.5f;

	/** Shots converge on the reticle ray this far ahead (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input", meta = (ClampMin = "100"))
	float AimConvergenceDistance = 30000.f;

	/** Left trigger past this (throttle <= -value) also brakes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input|Gamepad", meta = (ClampMin = "0", ClampMax = "1"))
	float TriggerBrakeThreshold = 0.92f;

	/** Holding the power key longer than this triggers Signature2 instead of Signature1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input", meta = (ClampMin = "0.05"))
	float PowerHoldSeconds = 0.5f;

	/** Holding D-pad up longer than this opens the switch radial. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Input", meta = (ClampMin = "0.05"))
	float RadialHoldSeconds = 0.25f;

private:
	void BuildInputObjects();
	UInputAction* MakeAction(FName Name, EInputActionValueType ValueType);
	void ApplyMappingContexts();
	ATDSpacecraft* GetSpacecraft() const;
	void PushFlightInput();
	void ResetHeldInputs();

	// Handlers
	void OnThrottle(const FInputActionValue& Value);
	void OnThrottleReleased(const FInputActionValue& Value);
	void OnMouseAim(const FInputActionValue& Value);
	void OnStickAim(const FInputActionValue& Value);
	void OnStickAimReleased(const FInputActionValue& Value);
	void OnRoll(const FInputActionValue& Value);
	void OnDodgePressed(const FInputActionValue& Value);
	void OnDodgeTap(const FInputActionValue& Value);
	/** Routes a dodge to the flown ship (-1 left / +1 right). */
	bool RouteDodge(float Direction);
	double LastDodgeTapTime[2] = { -1.0, -1.0 };
	void OnRollReleased(const FInputActionValue& Value);
	void OnStrafe(const FInputActionValue& Value);
	void OnStrafeReleased(const FInputActionValue& Value);
	void OnBoostPressed(const FInputActionValue& Value);
	void OnBoostReleased(const FInputActionValue& Value);
	void OnDriftPressed(const FInputActionValue& Value);
	void OnDriftReleased(const FInputActionValue& Value);
	void OnBrakePressed(const FInputActionValue& Value);
	void OnBrakeReleased(const FInputActionValue& Value);
	void OnFirePrimaryPressed(const FInputActionValue& Value);
	void OnFirePrimaryReleased(const FInputActionValue& Value);
	void OnFireSecondaryPressed(const FInputActionValue& Value);
	void OnFireSecondaryReleased(const FInputActionValue& Value);
	void OnPowerPressed(const FInputActionValue& Value);
	void OnPowerReleased(const FInputActionValue& Value);
	void OnAbility(const FInputActionValue& Value);
	void OnAbilityReleased(const FInputActionValue& Value);
	void ApplyThrottleUp();
	void OnSwitchNext(const FInputActionValue& Value);
	void OnSwitchPrev(const FInputActionValue& Value);
	void OnSwitch1(const FInputActionValue& Value);
	void OnSwitch2(const FInputActionValue& Value);
	void OnSwitch3(const FInputActionValue& Value);
	void OnSwitchRadialPressed(const FInputActionValue& Value);
	void OnSwitchRadialReleased(const FInputActionValue& Value);
	void OnTargetCycle(const FInputActionValue& Value);
	void OnHyperdrivePressed(const FInputActionValue& Value);
	void OnHyperdriveReleased(const FInputActionValue& Value);
	void OnSquadCommand(const FInputActionValue& Value);
	void OnLand(const FInputActionValue& Value);
	void OnLandReleased(const FInputActionValue& Value);
	void OnCameraToggle(const FInputActionValue& Value);
	void RouteHyperdrive(bool bPressed);
	/** Hold F / A: land on a pad in reach, or take off again from a landed ship. True if it acted. */
	bool RouteLand();
	void OnPause(const FInputActionValue& Value);
	/** Space (IA_Pause) and foot (T5's IA_Ground_Pause) both broadcast OnPauseInput; this is the one toggle. */
	UFUNCTION()
	void HandlePauseInput();
	bool bPausedByPlayer = false;
	void OnGalaxyMap(const FInputActionValue& Value);

	// Routing to other areas (no-ops until their classes are merged)
	void RouteTrigger(ETDWeaponSlot Slot, bool bHeld);
	void RoutePower(ETDPowerInput Power, bool bPressed = true);
	void RouteSwitchStep(int32 Direction);
	void RouteSwitchDirect(ETDShipId Ship);
	void RouteTargetCycle();
	void RouteSquadCommand();

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_SpaceCommon;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_Space;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC_Atmosphere;
	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> Actions;

	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Throttle;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_AimMouse;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_AimStick;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Roll;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Strafe;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Boost;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Drift;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Brake;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_FirePrimary;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_FireSecondary;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Power;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Ability;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_SwitchNext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_SwitchPrev;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Switch1;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Switch2;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Switch3;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_SwitchRadial;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_TargetCycle;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Hyperdrive;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_SquadCommand;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Land;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Pause;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_CameraToggleHold;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_TargetCycleTap;
	/** Dodge: LeftAlt (dedicated) and a double-tap of A / D or a double flick of the left stick. */
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Dodge;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_DodgeTap;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_GalaxyMap;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_CameraToggle;

	ETDFlightContext FlightContext = ETDFlightContext::Space;
	bool bSpaceInputEnabled = true;

	FTDFlightInput CurrentFlightInput;
	FVector2D VirtualStick = FVector2D::ZeroVector;
	FVector2D StickAim = FVector2D::ZeroVector;
	float ThrottleAxis = 0.f;
	bool bBrakeKey = false;
	bool bUsingGamepad = false;
	bool bMouseMovedThisFrame = false;

	double PowerPressTime = -1.0;
	bool bPowerHoldFired = false;
	/** This press went straight to Signature1 (the ship has no Signature2 power). */
	bool bPowerImmediate = false;
	bool bThrottleUpApplied = false;
	bool bAbilityHeld = false;
	void ReleaseHeldPowers();

	friend struct FTDSpaceControllerTestAccess;
	double RadialPressTime = -1.0;
	double LandPressTime = -1.0;
	bool bLandFired = false;
	bool bRadialOpen = false;
	bool bFirePrimaryHeld = false;
	bool bFireSecondaryHeld = false;
};
