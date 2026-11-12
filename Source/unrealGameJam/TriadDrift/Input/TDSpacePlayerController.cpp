// Triad Drift - space player controller and runtime Enhanced Input setup (ENG-02). Owner: Teammate 1.

#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"

// Other areas' classes: routing switches on automatically when their headers are merged in.
#if __has_include("TriadDrift/Combat/TDWeaponComponent.h")
#include "TriadDrift/Combat/TDWeaponComponent.h"
#define TD_HAS_WEAPONS 1
#else
#define TD_HAS_WEAPONS 0
#endif

#if __has_include("TriadDrift/Combat/TDTargetingComponent.h")
#include "TriadDrift/Combat/TDTargetingComponent.h"
#define TD_HAS_TARGETING 1
#else
#define TD_HAS_TARGETING 0
#endif

#if __has_include("TriadDrift/Squad/TDSquadManager.h")
#include "TriadDrift/Squad/TDSquadManager.h"
#define TD_HAS_SQUAD 1
#else
#define TD_HAS_SQUAD 0
#endif

namespace
{
	/** Radial deadzone + response curve: |v| in [Deadzone, 1] remapped to [0, 1], then ^Exponent. */
	FVector2D ShapeStick(const FVector2D& Raw, float Deadzone, float Exponent)
	{
		const float Len = Raw.Size();
		if (Len <= Deadzone || Len <= KINDA_SMALL_NUMBER)
		{
			return FVector2D::ZeroVector;
		}
		const float Clamped = FMath::Min(Len, 1.f);
		const float Remapped = (Clamped - Deadzone) / FMath::Max(1.f - Deadzone, KINDA_SMALL_NUMBER);
		return (Raw / Len) * FMath::Pow(Remapped, Exponent);
	}

	template <typename T>
	T* AddModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer)
	{
		T* Modifier = NewObject<T>(Outer);
		Mapping.Modifiers.Add(Modifier);
		return Modifier;
	}

	/** Maps a key onto the Y axis of a 2D action (optionally negated). */
	void MapAxis2D(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key, bool bYAxis, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bYAxis)
		{
			AddModifier<UInputModifierSwizzleAxis>(Mapping, Context)->Order = EInputAxisSwizzle::YXZ;
		}
		if (bNegate)
		{
			AddModifier<UInputModifierNegate>(Mapping, Context);
		}
	}

	void MapNegated(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		AddModifier<UInputModifierNegate>(Mapping, Context);
	}
}

ATDSpacePlayerController::ATDSpacePlayerController()
{
	bShowMouseCursor = false;
}

// ------------------------------------------------------------------------------------------------ setup

UInputAction* ATDSpacePlayerController::MakeAction(FName Name, EInputActionValueType ValueType)
{
	UInputAction* Action = NewObject<UInputAction>(this, Name);
	Action->ValueType = ValueType;
	Actions.Add(Action);
	return Action;
}

UInputAction* ATDSpacePlayerController::FindActionByName(FName ActionName) const
{
	for (const TObjectPtr<UInputAction>& Action : Actions)
	{
		if (Action && Action->GetFName() == ActionName)
		{
			return Action;
		}
	}
	return nullptr;
}

void ATDSpacePlayerController::BuildInputObjects()
{
	if (IMC_SpaceCommon)
	{
		return;
	}

	using V = EInputActionValueType;
	IA_Throttle = MakeAction(TEXT("IA_Throttle"), V::Axis1D);
	IA_AimMouse = MakeAction(TEXT("IA_AimMouse"), V::Axis2D);
	IA_AimStick = MakeAction(TEXT("IA_AimStick"), V::Axis2D);
	IA_Roll = MakeAction(TEXT("IA_Roll"), V::Axis1D);
	IA_Strafe = MakeAction(TEXT("IA_Strafe"), V::Axis2D);
	IA_Boost = MakeAction(TEXT("IA_Boost"), V::Boolean);
	IA_Drift = MakeAction(TEXT("IA_Drift"), V::Boolean);
	IA_Brake = MakeAction(TEXT("IA_Brake"), V::Boolean);
	IA_FirePrimary = MakeAction(TEXT("IA_FirePrimary"), V::Boolean);
	IA_FireSecondary = MakeAction(TEXT("IA_FireSecondary"), V::Boolean);
	IA_Power = MakeAction(TEXT("IA_Power"), V::Boolean);
	IA_Ability = MakeAction(TEXT("IA_Ability"), V::Boolean);
	IA_SwitchNext = MakeAction(TEXT("IA_SwitchNext"), V::Boolean);
	IA_SwitchPrev = MakeAction(TEXT("IA_SwitchPrev"), V::Boolean);
	IA_Switch1 = MakeAction(TEXT("IA_Switch1"), V::Boolean);
	IA_Switch2 = MakeAction(TEXT("IA_Switch2"), V::Boolean);
	IA_Switch3 = MakeAction(TEXT("IA_Switch3"), V::Boolean);
	IA_SwitchRadial = MakeAction(TEXT("IA_SwitchRadial"), V::Boolean);
	IA_TargetCycle = MakeAction(TEXT("IA_TargetCycle"), V::Boolean);
	IA_Hyperdrive = MakeAction(TEXT("IA_Hyperdrive"), V::Boolean);
	IA_SquadCommand = MakeAction(TEXT("IA_SquadCommand"), V::Boolean);
	IA_Land = MakeAction(TEXT("IA_Land"), V::Boolean);
	IA_Pause = MakeAction(TEXT("IA_Pause"), V::Boolean);
	IA_GalaxyMap = MakeAction(TEXT("IA_GalaxyMap"), V::Boolean);

	IMC_SpaceCommon = NewObject<UInputMappingContext>(this, TEXT("IMC_SpaceCommon"));
	IMC_Space = NewObject<UInputMappingContext>(this, TEXT("IMC_Space"));
	IMC_Atmosphere = NewObject<UInputMappingContext>(this, TEXT("IMC_Atmosphere"));
	UInputMappingContext* C = IMC_SpaceCommon;

	// Throttle: W / S, RT / LT.
	C->MapKey(IA_Throttle, EKeys::W);
	MapNegated(C, IA_Throttle, EKeys::S);
	C->MapKey(IA_Throttle, EKeys::Gamepad_RightTriggerAxis);
	MapNegated(C, IA_Throttle, EKeys::Gamepad_LeftTriggerAxis);

	// Aim: mouse delta (virtual joystick, shaped in code) and right stick (shaped in code).
	C->MapKey(IA_AimMouse, EKeys::Mouse2D);
	C->MapKey(IA_AimStick, EKeys::Gamepad_Right2D);

	// Roll: A / D, left stick X.
	C->MapKey(IA_Roll, EKeys::D);
	MapNegated(C, IA_Roll, EKeys::A);
	{
		FEnhancedActionKeyMapping& M = C->MapKey(IA_Roll, EKeys::Gamepad_LeftX);
		UInputModifierDeadZone* DZ = AddModifier<UInputModifierDeadZone>(M, C);
		DZ->LowerThreshold = 0.2f;
		DZ->Type = EDeadZoneType::Axial;
	}

	// Strafe (optional thrusters): arrow keys, left stick Y = vertical.
	MapAxis2D(C, IA_Strafe, EKeys::Right, false, false);
	MapAxis2D(C, IA_Strafe, EKeys::Left, false, true);
	MapAxis2D(C, IA_Strafe, EKeys::Up, true, false);
	MapAxis2D(C, IA_Strafe, EKeys::Down, true, true);
	{
		FEnhancedActionKeyMapping& M = C->MapKey(IA_Strafe, EKeys::Gamepad_LeftY);
		UInputModifierDeadZone* DZ = AddModifier<UInputModifierDeadZone>(M, C);
		DZ->LowerThreshold = 0.35f;
		DZ->Type = EDeadZoneType::Axial;
		AddModifier<UInputModifierSwizzleAxis>(M, C)->Order = EInputAxisSwizzle::YXZ;
	}

	// Boost / Drift / Brake.
	C->MapKey(IA_Boost, EKeys::LeftShift);
	C->MapKey(IA_Boost, EKeys::Gamepad_LeftThumbstick);
	C->MapKey(IA_Drift, EKeys::LeftAlt);
	C->MapKey(IA_Drift, EKeys::C);
	C->MapKey(IA_Drift, EKeys::Gamepad_FaceButton_Right);
	C->MapKey(IA_Brake, EKeys::SpaceBar);

	// Weapons.
	C->MapKey(IA_FirePrimary, EKeys::LeftMouseButton);
	C->MapKey(IA_FirePrimary, EKeys::Gamepad_RightShoulder);
	C->MapKey(IA_FireSecondary, EKeys::RightMouseButton);
	C->MapKey(IA_FireSecondary, EKeys::Gamepad_LeftShoulder);

	// Powers.
	C->MapKey(IA_Power, EKeys::Q);
	C->MapKey(IA_Power, EKeys::Gamepad_FaceButton_Left);
	C->MapKey(IA_Ability, EKeys::E);
	C->MapKey(IA_Ability, EKeys::Gamepad_FaceButton_Top);

	// Switching. Shift+Tab is resolved in OnSwitchNext (Shift is also Boost).
	C->MapKey(IA_SwitchNext, EKeys::Tab);
	C->MapKey(IA_SwitchNext, EKeys::Gamepad_DPad_Right);
	C->MapKey(IA_SwitchPrev, EKeys::Gamepad_DPad_Left);
	C->MapKey(IA_Switch1, EKeys::One);
	C->MapKey(IA_Switch2, EKeys::Two);
	C->MapKey(IA_Switch3, EKeys::Three);
	C->MapKey(IA_SwitchRadial, EKeys::Gamepad_DPad_Up);

	// Targeting.
	C->MapKey(IA_TargetCycle, EKeys::T);
	C->MapKey(IA_TargetCycle, EKeys::MiddleMouseButton);
	C->MapKey(IA_TargetCycle, EKeys::Gamepad_RightThumbstick);

	// Menus.
	C->MapKey(IA_Pause, EKeys::Escape);
	C->MapKey(IA_Pause, EKeys::Gamepad_Special_Right);
	C->MapKey(IA_GalaxyMap, EKeys::M);
	C->MapKey(IA_GalaxyMap, EKeys::Gamepad_Special_Left);

	// IMC_Space: hyperdrive + squad command.
	IMC_Space->MapKey(IA_Hyperdrive, EKeys::H);
	IMC_Space->MapKey(IA_Hyperdrive, EKeys::Gamepad_DPad_Down);
	IMC_Space->MapKey(IA_SquadCommand, EKeys::F);
	IMC_Space->MapKey(IA_SquadCommand, EKeys::Gamepad_FaceButton_Bottom);

	// IMC_Atmosphere: Land replaces Hyperdrive; F / A lands.
	IMC_Atmosphere->MapKey(IA_Land, EKeys::F);
	IMC_Atmosphere->MapKey(IA_Land, EKeys::Gamepad_FaceButton_Bottom);
}

void ATDSpacePlayerController::BeginPlay()
{
	Super::BeginPlay();
	BuildInputObjects();
	ApplyMappingContexts();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

void ATDSpacePlayerController::ApplyMappingContexts()
{
	BuildInputObjects();
	const ULocalPlayer* LP = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}
	Subsystem->RemoveMappingContext(IMC_SpaceCommon);
	Subsystem->RemoveMappingContext(IMC_Space);
	Subsystem->RemoveMappingContext(IMC_Atmosphere);
	if (!bSpaceInputEnabled)
	{
		return;
	}
	Subsystem->AddMappingContext(IMC_SpaceCommon, 0);
	Subsystem->AddMappingContext(FlightContext == ETDFlightContext::Space ? IMC_Space.Get() : IMC_Atmosphere.Get(), 1);
}

void ATDSpacePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	BuildInputObjects();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTriadDrift, Error, TEXT("TDSpacePlayerController: InputComponent is not an EnhancedInputComponent; space input disabled."));
		return;
	}

	using E = ETriggerEvent;
	EIC->BindAction(IA_Throttle, E::Triggered, this, &ThisClass::OnThrottle);
	EIC->BindAction(IA_Throttle, E::Completed, this, &ThisClass::OnThrottleReleased);
	EIC->BindAction(IA_AimMouse, E::Triggered, this, &ThisClass::OnMouseAim);
	EIC->BindAction(IA_AimStick, E::Triggered, this, &ThisClass::OnStickAim);
	EIC->BindAction(IA_AimStick, E::Completed, this, &ThisClass::OnStickAimReleased);
	EIC->BindAction(IA_Roll, E::Triggered, this, &ThisClass::OnRoll);
	EIC->BindAction(IA_Roll, E::Completed, this, &ThisClass::OnRollReleased);
	EIC->BindAction(IA_Strafe, E::Triggered, this, &ThisClass::OnStrafe);
	EIC->BindAction(IA_Strafe, E::Completed, this, &ThisClass::OnStrafeReleased);
	EIC->BindAction(IA_Boost, E::Started, this, &ThisClass::OnBoostPressed);
	EIC->BindAction(IA_Boost, E::Completed, this, &ThisClass::OnBoostReleased);
	EIC->BindAction(IA_Drift, E::Started, this, &ThisClass::OnDriftPressed);
	EIC->BindAction(IA_Drift, E::Completed, this, &ThisClass::OnDriftReleased);
	EIC->BindAction(IA_Brake, E::Started, this, &ThisClass::OnBrakePressed);
	EIC->BindAction(IA_Brake, E::Completed, this, &ThisClass::OnBrakeReleased);
	EIC->BindAction(IA_FirePrimary, E::Started, this, &ThisClass::OnFirePrimaryPressed);
	EIC->BindAction(IA_FirePrimary, E::Completed, this, &ThisClass::OnFirePrimaryReleased);
	EIC->BindAction(IA_FireSecondary, E::Started, this, &ThisClass::OnFireSecondaryPressed);
	EIC->BindAction(IA_FireSecondary, E::Completed, this, &ThisClass::OnFireSecondaryReleased);
	EIC->BindAction(IA_Power, E::Started, this, &ThisClass::OnPowerPressed);
	EIC->BindAction(IA_Power, E::Completed, this, &ThisClass::OnPowerReleased);
	EIC->BindAction(IA_Ability, E::Started, this, &ThisClass::OnAbility);
	EIC->BindAction(IA_SwitchNext, E::Started, this, &ThisClass::OnSwitchNext);
	EIC->BindAction(IA_SwitchPrev, E::Started, this, &ThisClass::OnSwitchPrev);
	EIC->BindAction(IA_Switch1, E::Started, this, &ThisClass::OnSwitch1);
	EIC->BindAction(IA_Switch2, E::Started, this, &ThisClass::OnSwitch2);
	EIC->BindAction(IA_Switch3, E::Started, this, &ThisClass::OnSwitch3);
	EIC->BindAction(IA_SwitchRadial, E::Started, this, &ThisClass::OnSwitchRadialPressed);
	EIC->BindAction(IA_SwitchRadial, E::Completed, this, &ThisClass::OnSwitchRadialReleased);
	EIC->BindAction(IA_TargetCycle, E::Started, this, &ThisClass::OnTargetCycle);
	EIC->BindAction(IA_Hyperdrive, E::Started, this, &ThisClass::OnHyperdrivePressed);
	EIC->BindAction(IA_Hyperdrive, E::Completed, this, &ThisClass::OnHyperdriveReleased);
	EIC->BindAction(IA_SquadCommand, E::Started, this, &ThisClass::OnSquadCommand);
	EIC->BindAction(IA_Land, E::Started, this, &ThisClass::OnLand);
	EIC->BindAction(IA_Pause, E::Started, this, &ThisClass::OnPause);
	EIC->BindAction(IA_GalaxyMap, E::Started, this, &ThisClass::OnGalaxyMap);
}

void ATDSpacePlayerController::SetFlightContext(ETDFlightContext NewContext)
{
	if (FlightContext != NewContext)
	{
		FlightContext = NewContext;
		if (FlightContext == ETDFlightContext::Atmosphere)
		{
			OnHyperdriveReleased(FInputActionValue()); // don't leave a hyperdrive spool latched
		}
		ApplyMappingContexts();
	}
}

void ATDSpacePlayerController::SetSpaceInputEnabled(bool bEnabled)
{
	if (bSpaceInputEnabled != bEnabled)
	{
		bSpaceInputEnabled = bEnabled;
		if (!bEnabled)
		{
			ResetHeldInputs();
		}
		ApplyMappingContexts();
	}
}

void ATDSpacePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// Held buttons carry over a switch (keep firing / boosting into the new ship), but the trigger
	// state must be re-sent to the new ship's weapons.
	if (bFirePrimaryHeld) { RouteTrigger(ETDWeaponSlot::Primary, true); }
	if (bFireSecondaryHeld) { RouteTrigger(ETDWeaponSlot::Secondary, true); }
	PushFlightInput();
}

void ATDSpacePlayerController::OnUnPossess()
{
	// Release the old ship's triggers so its wingman AI starts clean. Never touch its velocity.
	if (bFirePrimaryHeld) { RouteTrigger(ETDWeaponSlot::Primary, false); }
	if (bFireSecondaryHeld) { RouteTrigger(ETDWeaponSlot::Secondary, false); }
	Super::OnUnPossess();
}

void ATDSpacePlayerController::ResetHeldInputs()
{
	if (bFirePrimaryHeld) { RouteTrigger(ETDWeaponSlot::Primary, false); }
	if (bFireSecondaryHeld) { RouteTrigger(ETDWeaponSlot::Secondary, false); }
	bFirePrimaryHeld = bFireSecondaryHeld = false;
	CurrentFlightInput = FTDFlightInput();
	CurrentFlightInput.Throttle = 1.f;
	ThrottleAxis = 0.f;
	StickAim = FVector2D::ZeroVector;
	VirtualStick = FVector2D::ZeroVector;
	bBrakeKey = false;
	PowerPressTime = -1.0;
	bPowerHoldFired = false;
	RadialPressTime = -1.0;
	if (bRadialOpen)
	{
		bRadialOpen = false;
		OnSwitchRadialInput.Broadcast(false);
	}
	PushFlightInput();
}

ATDSpacecraft* ATDSpacePlayerController::GetSpacecraft() const
{
	return Cast<ATDSpacecraft>(GetPawn());
}

// --------------------------------------------------------------------------------------------- per tick

void ATDSpacePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;

	// Virtual stick re-centring (optional).
	if (!bMouseMovedThisFrame && MouseRecenterRate > 0.f)
	{
		VirtualStick = FMath::Vector2DInterpTo(VirtualStick, FVector2D::ZeroVector, DeltaTime, MouseRecenterRate);
	}
	bMouseMovedThisFrame = false;

	// Hold-Q: Signature2 fires the moment the hold threshold is crossed.
	if (PowerPressTime >= 0.0 && !bPowerHoldFired && Now - PowerPressTime >= PowerHoldSeconds)
	{
		bPowerHoldFired = true;
		RoutePower(ETDPowerInput::Signature2);
	}

	// Hold D-pad up: open the switch radial.
	if (RadialPressTime >= 0.0 && !bRadialOpen && Now - RadialPressTime >= RadialHoldSeconds)
	{
		bRadialOpen = true;
		OnSwitchRadialInput.Broadcast(true);
	}

	// Build this frame's flight input.
	FTDFlightInput In;
	// Core contract: Throttle -1 = min speed, +1 = cruise. Neutral flies at cruise; back throttle slows.
	In.Throttle = FMath::Clamp(1.f + FMath::Min(ThrottleAxis, 0.f) * 2.f, -1.f, 1.f);
	In.bBrake = bBrakeKey || (ThrottleAxis <= -TriggerBrakeThreshold && bUsingGamepad);

	FVector2D Aim;
	if (bRadialOpen)
	{
		Aim = FVector2D::ZeroVector; // right stick picks a ship while the radial is open
	}
	else if (bUsingGamepad)
	{
		Aim = ShapeStick(StickAim, StickDeadzone, StickResponseExponent);
		if (bInvertStickPitch) { Aim.Y = -Aim.Y; }
	}
	else
	{
		Aim = ShapeStick(VirtualStick, MouseDeadzone, MouseResponseExponent);
	}
	In.Aim = Aim;
	In.Roll = CurrentFlightInput.Roll;
	In.Strafe = CurrentFlightInput.Strafe;
	In.bBoost = CurrentFlightInput.bBoost;
	In.bDrift = CurrentFlightInput.bDrift;
	CurrentFlightInput = In;

	PushFlightInput();

#if TD_HAS_WEAPONS
	if (ATDSpacecraft* Craft = GetSpacecraft())
	{
		if (UTDWeaponComponent* Weapons = Craft->FindComponentByClass<UTDWeaponComponent>())
		{
			Weapons->SetAimDirection(GetAimWorldDirection());
		}
	}
#endif
}

void ATDSpacePlayerController::PushFlightInput()
{
	if (ATDSpacecraft* Craft = GetSpacecraft())
	{
		Craft->SetFlightInput(bSpaceInputEnabled ? CurrentFlightInput : FTDFlightInput());
	}
}

FVector ATDSpacePlayerController::GetAimWorldDirection() const
{
	const ATDSpacecraft* Craft = GetSpacecraft();
	const FVector Fallback = Craft ? Craft->GetActorForwardVector() : FVector::ForwardVector;
	if (!PlayerCameraManager || !Craft)
	{
		return Fallback;
	}

	// The reticle sits at VirtualStick on screen; on gamepad it is the screen centre.
	int32 SizeX = 0, SizeY = 0;
	GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0)
	{
		return Fallback;
	}
	const FVector2D Stick = bUsingGamepad ? FVector2D::ZeroVector : VirtualStick;
	const float HalfMin = 0.5f * FMath::Min(SizeX, SizeY);
	const FVector2D ScreenPos(0.5f * SizeX + Stick.X * HalfMin * 0.5f, 0.5f * SizeY - Stick.Y * HalfMin * 0.5f);

	FVector WorldPos, WorldDir;
	if (!DeprojectScreenPositionToWorld(ScreenPos.X, ScreenPos.Y, WorldPos, WorldDir))
	{
		return Fallback;
	}
	// Converge on a point far ahead so shots from the ship's muzzles meet the reticle line.
	const FVector AimPoint = WorldPos + WorldDir * 30000.f;
	return (AimPoint - Craft->GetActorLocation()).GetSafeNormal(UE_SMALL_NUMBER, Fallback);
}

bool ATDSpacePlayerController::GetRadialSelection(ETDShipId& OutShip) const
{
	if (!bRadialOpen || StickAim.Size() < 0.5f)
	{
		return false;
	}
	// Three sectors: left = Rift, up = Lancer, right = Bastion.
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(StickAim.Y, StickAim.X)); // 0 = right, 90 = up
	if (AngleDeg > 150.f || AngleDeg < -120.f)
	{
		OutShip = ETDShipId::Rift;
	}
	else if (AngleDeg > 30.f)
	{
		OutShip = ETDShipId::Lancer;
	}
	else
	{
		OutShip = ETDShipId::Bastion;
	}
	return true;
}

// ------------------------------------------------------------------------------------------- handlers

void ATDSpacePlayerController::OnThrottle(const FInputActionValue& Value)
{
	ThrottleAxis = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
}

void ATDSpacePlayerController::OnThrottleReleased(const FInputActionValue&)
{
	ThrottleAxis = 0.f;
}

void ATDSpacePlayerController::OnMouseAim(const FInputActionValue& Value)
{
	FVector2D Delta = Value.Get<FVector2D>();
	if (Delta.IsNearlyZero())
	{
		return;
	}
	if (bInvertMousePitch)
	{
		Delta.Y = -Delta.Y;
	}
	bUsingGamepad = false;
	bMouseMovedThisFrame = true;
	VirtualStick += Delta * MouseSensitivity;
	if (VirtualStick.SizeSquared() > 1.f)
	{
		VirtualStick.Normalize();
	}
}

void ATDSpacePlayerController::OnStickAim(const FInputActionValue& Value)
{
	StickAim = Value.Get<FVector2D>();
	if (StickAim.Size() > StickDeadzone)
	{
		bUsingGamepad = true;
	}
}

void ATDSpacePlayerController::OnStickAimReleased(const FInputActionValue&)
{
	StickAim = FVector2D::ZeroVector;
}

void ATDSpacePlayerController::OnRoll(const FInputActionValue& Value)
{
	CurrentFlightInput.Roll = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
}

void ATDSpacePlayerController::OnRollReleased(const FInputActionValue&)
{
	CurrentFlightInput.Roll = 0.f;
}

void ATDSpacePlayerController::OnStrafe(const FInputActionValue& Value)
{
	CurrentFlightInput.Strafe = Value.Get<FVector2D>().ClampAxes(-1.f, 1.f);
}

void ATDSpacePlayerController::OnStrafeReleased(const FInputActionValue&)
{
	CurrentFlightInput.Strafe = FVector2D::ZeroVector;
}

void ATDSpacePlayerController::OnBoostPressed(const FInputActionValue&) { CurrentFlightInput.bBoost = true; }
void ATDSpacePlayerController::OnBoostReleased(const FInputActionValue&) { CurrentFlightInput.bBoost = false; }
void ATDSpacePlayerController::OnDriftPressed(const FInputActionValue&) { CurrentFlightInput.bDrift = true; }
void ATDSpacePlayerController::OnDriftReleased(const FInputActionValue&) { CurrentFlightInput.bDrift = false; }
void ATDSpacePlayerController::OnBrakePressed(const FInputActionValue&) { bBrakeKey = true; }
void ATDSpacePlayerController::OnBrakeReleased(const FInputActionValue&) { bBrakeKey = false; }

void ATDSpacePlayerController::OnFirePrimaryPressed(const FInputActionValue&) { bFirePrimaryHeld = true; RouteTrigger(ETDWeaponSlot::Primary, true); }
void ATDSpacePlayerController::OnFirePrimaryReleased(const FInputActionValue&) { bFirePrimaryHeld = false; RouteTrigger(ETDWeaponSlot::Primary, false); }
void ATDSpacePlayerController::OnFireSecondaryPressed(const FInputActionValue&) { bFireSecondaryHeld = true; RouteTrigger(ETDWeaponSlot::Secondary, true); }
void ATDSpacePlayerController::OnFireSecondaryReleased(const FInputActionValue&) { bFireSecondaryHeld = false; RouteTrigger(ETDWeaponSlot::Secondary, false); }

void ATDSpacePlayerController::OnPowerPressed(const FInputActionValue&)
{
	PowerPressTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	bPowerHoldFired = false;
}

void ATDSpacePlayerController::OnPowerReleased(const FInputActionValue&)
{
	if (PowerPressTime >= 0.0 && !bPowerHoldFired)
	{
		RoutePower(ETDPowerInput::Signature1);
	}
	PowerPressTime = -1.0;
	bPowerHoldFired = false;
}

void ATDSpacePlayerController::OnAbility(const FInputActionValue&) { RoutePower(ETDPowerInput::Ability); }

void ATDSpacePlayerController::OnSwitchNext(const FInputActionValue&)
{
	const bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
	RouteSwitchStep(bShift ? -1 : 1);
}

void ATDSpacePlayerController::OnSwitchPrev(const FInputActionValue&) { RouteSwitchStep(-1); }
void ATDSpacePlayerController::OnSwitch1(const FInputActionValue&) { RouteSwitchDirect(ETDShipId::Rift); }
void ATDSpacePlayerController::OnSwitch2(const FInputActionValue&) { RouteSwitchDirect(ETDShipId::Lancer); }
void ATDSpacePlayerController::OnSwitch3(const FInputActionValue&) { RouteSwitchDirect(ETDShipId::Bastion); }

void ATDSpacePlayerController::OnSwitchRadialPressed(const FInputActionValue&)
{
	RadialPressTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
}

void ATDSpacePlayerController::OnSwitchRadialReleased(const FInputActionValue&)
{
	if (bRadialOpen)
	{
		ETDShipId Picked;
		if (GetRadialSelection(Picked))
		{
			RouteSwitchDirect(Picked);
		}
		bRadialOpen = false;
		OnSwitchRadialInput.Broadcast(false);
	}
	else if (RadialPressTime >= 0.0)
	{
		// A quick tap of D-pad up: there is no "up" target in a 3-ship cycle, treat it as next.
		RouteSwitchStep(1);
	}
	RadialPressTime = -1.0;
}

void ATDSpacePlayerController::OnTargetCycle(const FInputActionValue&) { RouteTargetCycle(); }
void ATDSpacePlayerController::OnHyperdrivePressed(const FInputActionValue&) { OnHyperdriveInput.Broadcast(true); }
void ATDSpacePlayerController::OnHyperdriveReleased(const FInputActionValue&) { OnHyperdriveInput.Broadcast(false); }
void ATDSpacePlayerController::OnSquadCommand(const FInputActionValue&) { RouteSquadCommand(); }
void ATDSpacePlayerController::OnLand(const FInputActionValue&) { OnLandInput.Broadcast(); }
void ATDSpacePlayerController::OnPause(const FInputActionValue&) { OnPauseInput.Broadcast(); }
void ATDSpacePlayerController::OnGalaxyMap(const FInputActionValue&) { OnGalaxyMapInput.Broadcast(); }

// -------------------------------------------------------------------------------------------- routing

void ATDSpacePlayerController::RouteTrigger(ETDWeaponSlot Slot, bool bHeld)
{
	OnWeaponInput.Broadcast(Slot, bHeld);
#if TD_HAS_WEAPONS
	if (ATDSpacecraft* Craft = GetSpacecraft())
	{
		if (UTDWeaponComponent* Weapons = Craft->FindComponentByClass<UTDWeaponComponent>())
		{
			Weapons->SetTriggerHeld(Slot, bHeld && bSpaceInputEnabled);
		}
	}
#endif
}

void ATDSpacePlayerController::RoutePower(ETDPowerInput Power)
{
	OnPowerInput.Broadcast(Power);
}

void ATDSpacePlayerController::RouteSwitchStep(int32 Direction)
{
	OnSwitchStepInput.Broadcast(Direction);
#if TD_HAS_SQUAD
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		Direction >= 0 ? Squad->RequestSwitchNext() : Squad->RequestSwitchPrev();
	}
#endif
}

void ATDSpacePlayerController::RouteSwitchDirect(ETDShipId Ship)
{
	OnSwitchDirectInput.Broadcast(Ship);
#if TD_HAS_SQUAD
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		Squad->RequestSwitchTo(Ship);
	}
#endif
}

void ATDSpacePlayerController::RouteTargetCycle()
{
	OnTargetCycleInput.Broadcast();
#if TD_HAS_TARGETING
	if (APawn* P = GetPawn())
	{
		if (UTDTargetingComponent* Targeting = P->FindComponentByClass<UTDTargetingComponent>())
		{
			Targeting->CycleTarget();
		}
	}
#endif
}

void ATDSpacePlayerController::RouteSquadCommand()
{
	OnSquadCommandInput.Broadcast();
#if TD_HAS_SQUAD
	if (ATDSquadManager* Squad = ATDSquadManager::Get(this))
	{
		AActor* Target = nullptr;
#if TD_HAS_TARGETING
		if (APawn* P = GetPawn())
		{
			if (const UTDTargetingComponent* Targeting = P->FindComponentByClass<UTDTargetingComponent>())
			{
				Target = Targeting->GetCurrentTarget();
			}
		}
#endif
		Squad->IssueFocusCommand(Target);
	}
#endif
}
