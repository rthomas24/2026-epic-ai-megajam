// Triad Drift - space player controller and runtime Enhanced Input setup (ENG-02). Owner: Teammate 1.

#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "InputKeyEventArgs.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"

// Other areas' classes: routing switches on automatically when their headers are merged in.
#if __has_include("TriadDrift/Combat/TDWeaponComponent.h")
#include "TriadDrift/Combat/TDWeaponComponent.h"
#define TD_HAS_WEAPONS 1
#else
#define TD_HAS_WEAPONS 0
#endif

#if TD_HAS_WEAPONS
template <typename TWeapons>
concept TDSpacePC_HasCancelCharge = requires(TWeapons* W) { W->CancelCharge(ETDWeaponSlot::Secondary); };

/** Calls T2's UTDWeaponComponent::CancelCharge once it exists (compiles to nothing before). */
template <typename TWeapons>
void TDSpacePC_CallCancelCharge(TWeapons* Weapons, ETDWeaponSlot Slot)
{
	if constexpr (TDSpacePC_HasCancelCharge<TWeapons>)
	{
		if (Weapons)
		{
			Weapons->CancelCharge(Slot);
		}
	}
}
#endif

#if __has_include("TriadDrift/Combat/TDTargetingComponent.h")
#include "TriadDrift/Combat/TDTargetingComponent.h"
#define TD_HAS_TARGETING 1
#else
#define TD_HAS_TARGETING 0
#endif

#if __has_include("TriadDrift/Powers/TDPowerComponent.h")
#include "TriadDrift/Powers/TDPowerComponent.h"
#define TD_HAS_POWERS 1
#else
#define TD_HAS_POWERS 0
#endif

#if __has_include("TriadDrift/Squad/TDSquadManager.h")
#include "TriadDrift/Squad/TDSquadManager.h"
#define TD_HAS_SQUAD 1
#else
#define TD_HAS_SQUAD 0
#endif

namespace
{
	FVector2D TDSpacePC_ShapeStickImpl(const FVector2D& Raw, float Deadzone, float Exponent)
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
	T* TDSpacePC_AddModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer)
	{
		T* Modifier = NewObject<T>(Outer);
		Mapping.Modifiers.Add(Modifier);
		return Modifier;
	}

	template <typename T>
	T* TDSpacePC_AddTrigger(FEnhancedActionKeyMapping& Mapping, UObject* Outer)
	{
		T* Trigger = NewObject<T>(Outer);
		Mapping.Triggers.Add(Trigger);
		return Trigger;
	}

	/** Maps a key onto the Y axis of a 2D action (optionally negated). */
	void TDSpacePC_MapAxis2D(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key, bool bYAxis, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bYAxis)
		{
			TDSpacePC_AddModifier<UInputModifierSwizzleAxis>(Mapping, Context)->Order = EInputAxisSwizzle::YXZ;
		}
		if (bNegate)
		{
			TDSpacePC_AddModifier<UInputModifierNegate>(Mapping, Context);
		}
	}

	void TDSpacePC_MapNegated(UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		TDSpacePC_AddModifier<UInputModifierNegate>(Mapping, Context);
	}
}

FVector2D ATDSpacePlayerController::ShapeStick(const FVector2D& Raw, float Deadzone, float Exponent)
{
	return TDSpacePC_ShapeStickImpl(Raw, Deadzone, Exponent);
}

ATDSpacePlayerController::ATDSpacePlayerController()
{
	Hints = CreateDefaultSubobject<UTDHintComponent>(TEXT("Hints")); // first-minute control hints
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
	IA_Dodge = MakeAction(TEXT("IA_Dodge"), V::Boolean);
	IA_DodgeTap = MakeAction(TEXT("IA_DodgeTap"), V::Axis1D);
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
	IA_CameraToggle = MakeAction(TEXT("IA_CameraToggle"), V::Boolean);
	IA_CameraToggleHold = MakeAction(TEXT("IA_CameraToggleHold"), V::Boolean);
	IA_TargetCycleTap = MakeAction(TEXT("IA_TargetCycleTap"), V::Boolean);
	// Menus must work while the game is paused (Esc / Start unpause).
	IA_Pause->bTriggerWhenPaused = true;
	IA_GalaxyMap->bTriggerWhenPaused = true;
	IA_OnFootCameraToggle = MakeAction(TEXT("IA_OnFootCameraToggle"), V::Boolean);
	IA_OpenSettings = MakeAction(TEXT("IA_OpenSettings"), V::Boolean);
	IA_OpenSettings->bTriggerWhenPaused = true;
	IA_OpenSettings->bConsumeInput = false; // pad Y is also the ability: never swallow it (it only acts while paused)

	IMC_SpaceCommon = NewObject<UInputMappingContext>(this, TEXT("IMC_SpaceCommon"));
	IMC_Space = NewObject<UInputMappingContext>(this, TEXT("IMC_Space"));
	IMC_Atmosphere = NewObject<UInputMappingContext>(this, TEXT("IMC_Atmosphere"));
	// ENG-32: on foot, V / View switch first / third person (space keeps V for the cockpit; the galaxy map key is hidden).
	IMC_OnFoot = NewObject<UInputMappingContext>(this, TEXT("IMC_OnFoot"));
	IMC_OnFoot->MapKey(IA_OnFootCameraToggle, EKeys::V);
	IMC_OnFoot->MapKey(IA_OnFootCameraToggle, EKeys::Gamepad_Special_Left);
	// Always: O / Y open the Settings page from the pause screen (ignored while not paused).
	IMC_Global = NewObject<UInputMappingContext>(this, TEXT("IMC_Global"));
	IMC_Global->MapKey(IA_OpenSettings, EKeys::O);
	IMC_Global->MapKey(IA_OpenSettings, EKeys::Gamepad_FaceButton_Top);
	UInputMappingContext* C = IMC_SpaceCommon;

	// Throttle: W / S, RT / LT.
	C->MapKey(IA_Throttle, EKeys::W);
	TDSpacePC_MapNegated(C, IA_Throttle, EKeys::S);
	C->MapKey(IA_Throttle, EKeys::Gamepad_RightTriggerAxis);
	TDSpacePC_MapNegated(C, IA_Throttle, EKeys::Gamepad_LeftTriggerAxis);

	// Aim: mouse delta (virtual joystick, shaped in code) and right stick (shaped in code).
	C->MapKey(IA_AimMouse, EKeys::Mouse2D);
	C->MapKey(IA_AimStick, EKeys::Gamepad_Right2D);

	// Roll: A / D, left stick X.
	C->MapKey(IA_Roll, EKeys::D);
	TDSpacePC_MapNegated(C, IA_Roll, EKeys::A);
	{
		FEnhancedActionKeyMapping& M = C->MapKey(IA_Roll, EKeys::Gamepad_LeftX);
		UInputModifierDeadZone* DZ = TDSpacePC_AddModifier<UInputModifierDeadZone>(M, C);
		DZ->LowerThreshold = 0.2f;
		DZ->Type = EDeadZoneType::Axial;
	}

	// Strafe (optional thrusters): arrow keys, left stick Y = vertical.
	TDSpacePC_MapAxis2D(C, IA_Strafe, EKeys::Right, false, false);
	TDSpacePC_MapAxis2D(C, IA_Strafe, EKeys::Left, false, true);
	TDSpacePC_MapAxis2D(C, IA_Strafe, EKeys::Up, true, false);
	TDSpacePC_MapAxis2D(C, IA_Strafe, EKeys::Down, true, true);
	{
		FEnhancedActionKeyMapping& M = C->MapKey(IA_Strafe, EKeys::Gamepad_LeftY);
		UInputModifierDeadZone* DZ = TDSpacePC_AddModifier<UInputModifierDeadZone>(M, C);
		DZ->LowerThreshold = 0.35f;
		DZ->Type = EDeadZoneType::Axial;
		TDSpacePC_AddModifier<UInputModifierSwizzleAxis>(M, C)->Order = EInputAxisSwizzle::YXZ;
	}

	// Boost / Drift / Brake.
	// Dodge: LeftAlt dodges toward the held roll (A / D) or steering side; A / D double-tap, left stick double flick.
	C->MapKey(IA_Dodge, EKeys::LeftAlt);
	C->MapKey(IA_DodgeTap, EKeys::D);
	TDSpacePC_MapNegated(C, IA_DodgeTap, EKeys::A);
	{
		FEnhancedActionKeyMapping& M = C->MapKey(IA_DodgeTap, EKeys::Gamepad_LeftX);
		UInputTriggerPressed* Flick = TDSpacePC_AddTrigger<UInputTriggerPressed>(M, C);
		Flick->ActuationThreshold = 0.85f; // a full flick, not roll steering
	}

	C->MapKey(IA_Boost, EKeys::LeftShift);
	C->MapKey(IA_Boost, EKeys::Gamepad_LeftThumbstick);
	// No LeftAlt: Alt + Tab (switch) would alt-tab out of the game on Windows.
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
	// R3: a quick tap cycles the target (on release); holding it 0.5 s toggles the cockpit camera (Lead, Nov 12).
	{
		FEnhancedActionKeyMapping& Tap = C->MapKey(IA_TargetCycleTap, EKeys::Gamepad_RightThumbstick);
		TDSpacePC_AddTrigger<UInputTriggerTap>(Tap, C)->TapReleaseTimeThreshold = 0.35f;
		FEnhancedActionKeyMapping& Hold = C->MapKey(IA_CameraToggleHold, EKeys::Gamepad_RightThumbstick);
		UInputTriggerHold* HoldTrigger = TDSpacePC_AddTrigger<UInputTriggerHold>(Hold, C);
		HoldTrigger->HoldTimeThreshold = 0.5f;
		HoldTrigger->bIsOneShot = true;
	}

	// Menus.
	C->MapKey(IA_Pause, EKeys::Escape);
	C->MapKey(IA_Pause, EKeys::Gamepad_Special_Right);
	if (bGalaxyMapEnabled) // hidden until Nick's galaxy map screen exists (Lead, Nov 12): no key, no broadcast
	{
		C->MapKey(IA_GalaxyMap, EKeys::M);
		C->MapKey(IA_GalaxyMap, EKeys::Gamepad_Special_Left);
	}

	// Camera: chase / cockpit (not in the ENG-02 table; V is free).
	C->MapKey(IA_CameraToggle, EKeys::V);

	// IMC_Space: hyperdrive + squad command.
	IMC_Space->MapKey(IA_Hyperdrive, EKeys::H);
	IMC_Space->MapKey(IA_Hyperdrive, EKeys::Gamepad_DPad_Down);
	IMC_Space->MapKey(IA_SquadCommand, EKeys::F);
	IMC_Space->MapKey(IA_SquadCommand, EKeys::Gamepad_FaceButton_Bottom);

	// IMC_Atmosphere: Land replaces Hyperdrive; F / A lands.
	IMC_Atmosphere->MapKey(IA_Land, EKeys::F);
	IMC_Atmosphere->MapKey(IA_Land, EKeys::Gamepad_FaceButton_Bottom);
}

// Input/TDInputDebug.cpp: activates Unreal's InputToolset for agent playtests (no-op in Shipping).
void TDActivateInputToolset(ULocalPlayer* LocalPlayer);

void ATDSpacePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (UTDSettingsSubsystem* Options = UTDSettingsSubsystem::GetTDSettings(this))
	{
		Options->ApplyToController(this); // the player's saved options (sensitivity, invert, dead zone, hints)
	}
	if (IsLocalController())
	{
		TDActivateInputToolset(GetLocalPlayer());
	}
	BuildInputObjects();
	ApplyMappingContexts();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
	// Bound for every controller: BeginPlay can run before the local player is attached (IsLocalController false).
	OnPauseInput.AddUniqueDynamic(this, &ATDSpacePlayerController::HandlePauseInput);
}

void ATDSpacePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bPausedByPlayer)
	{
		SetPlayerPaused(false); // never leave a world paused behind us
	}
	UTDSettingsSubsystem::CloseSettingsPage(this); // never carry the page into the next map
	Super::EndPlay(EndPlayReason);
}

void ATDSpacePlayerController::HandlePauseInput()
{
	if (UTDSettingsSubsystem::IsSettingsPageOpen(this))
	{
		UTDSettingsSubsystem::CloseSettingsPage(this); // Esc / Start back out of Settings first, to the pause screen
		return;
	}
	TogglePlayerPause();
}

namespace TDPauseBlockers
{
	static TMap<FName, ATDSpacePlayerController::FTDPauseBlocker>& Map()
	{
		static TMap<FName, ATDSpacePlayerController::FTDPauseBlocker> Blockers;
		return Blockers;
	}
}

void ATDSpacePlayerController::RegisterPauseBlocker(FName Id, FTDPauseBlocker Blocker)
{
	TDPauseBlockers::Map().Add(Id, MoveTemp(Blocker));
}

void ATDSpacePlayerController::UnregisterPauseBlocker(FName Id)
{
	TDPauseBlockers::Map().Remove(Id);
}

bool ATDSpacePlayerController::CanPauseNow(FText* OutReason) const
{
	auto No = [OutReason](const FText& Why) { if (OutReason) { *OutReason = Why; } return false; };
	for (const TPair<FName, FTDPauseBlocker>& Blocker : TDPauseBlockers::Map())
	{
		FText Why;
		if (Blocker.Value && Blocker.Value(this, Why))
		{
			return No(Why.IsEmpty() ? FText::FromName(Blocker.Key) : Why);
		}
	}
	const UWorld* World = GetWorld();
	if (const UTDGameInstance* GI = World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr)
	{
		if (GI->IsTravelPending())
		{
			return No(NSLOCTEXT("TDPause", "Travel", "Travelling"));
		}
		if (GI->IsRetakePending())
		{
			return No(NSLOCTEXT("TDPause", "Retake", "CUT! - new take starting"));
		}
		if (GI->GetProgressRef().Phase == ETDGamePhase::Victory)
		{
			return No(NSLOCTEXT("TDPause", "Victory", "Victory"));
		}
	}
	return true;
}

bool ATDSpacePlayerController::SetPlayerPaused(bool bPause)
{
	if (bPause == bPausedByPlayer)
	{
		return true;
	}
	if (bPause && !CanPauseNow())
	{
		return false;
	}
	if (!UGameplayStatics::SetGamePaused(this, bPause))
	{
		return false;
	}
	bPausedByPlayer = bPause;
	if (IsLocalController())
	{
		if (bPause)
		{
			// The pause menu (Nick, ART-09) needs the cursor; Escape keeps working through bTriggerWhenPaused.
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			SetInputMode(Mode);
			bShowMouseCursor = true;
		}
		else
		{
			SetInputMode(FInputModeGameOnly());
			bShowMouseCursor = false;
		}
	}
	UE_LOG(LogTriadDrift, Log, TEXT("TDInput: game %s by the player"), bPause ? TEXT("paused") : TEXT("resumed"));
	OnPauseChanged.Broadcast(bPause);
	return true;
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
	// Only add / remove what actually changes, applied immediately, and never "ignore keys held until
	// released": a ship switch re-possesses, and the player's held fire / boost must carry straight over.
	// (Immediate also matters where no core ticker runs the deferred rebuild, e.g. automation worlds.)
	const bool bFlying = bSpaceInputEnabled && GetSpacecraft() != nullptr; // on foot: T5's character brings IMC_Ground
	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = false;
	Options.bForceImmediately = true;
	auto Want = [Subsystem, &Options](const UInputMappingContext* Context, int32 Priority, bool bWanted)
	{
		if (!Context)
		{
			return;
		}
		const bool bHas = Subsystem->HasMappingContext(Context);
		if (bWanted && !bHas)
		{
			Subsystem->AddMappingContext(Context, Priority, Options);
		}
		else if (!bWanted && bHas)
		{
			Subsystem->RemoveMappingContext(Context, Options);
		}
	};
	Want(IMC_SpaceCommon, 0, bFlying);
	Want(IMC_Space, 1, bFlying && FlightContext == ETDFlightContext::Space);
	Want(IMC_Atmosphere, 1, bFlying && FlightContext == ETDFlightContext::Atmosphere);
	Want(IMC_OnFoot, 2, GetPawn() != nullptr && GetSpacecraft() == nullptr);
	Want(IMC_Global, 0, true);
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
	EIC->BindAction(IA_Dodge, E::Started, this, &ThisClass::OnDodgePressed);
	EIC->BindAction(IA_DodgeTap, E::Started, this, &ThisClass::OnDodgeTap);
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
	EIC->BindAction(IA_Ability, E::Completed, this, &ThisClass::OnAbilityReleased);
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
	EIC->BindAction(IA_Land, E::Completed, this, &ThisClass::OnLandReleased);
	EIC->BindAction(IA_CameraToggle, E::Started, this, &ThisClass::OnCameraToggle);
	EIC->BindAction(IA_CameraToggleHold, E::Triggered, this, &ThisClass::OnCameraToggle);
	EIC->BindAction(IA_TargetCycleTap, E::Triggered, this, &ThisClass::OnTargetCycle);
	EIC->BindAction(IA_Pause, E::Started, this, &ThisClass::OnPause);
	EIC->BindAction(IA_OnFootCameraToggle, E::Started, this, &ThisClass::OnFootCameraToggle);
	EIC->BindAction(IA_OpenSettings, E::Started, this, &ThisClass::OnOpenSettings);
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
	ApplyMappingContexts(); // space contexts only while flying a spacecraft
	if (!GetSpacecraft())
	{
		ResetHeldInputs();
		return;
	}
	// A switch is a cut: the new ship starts flying straight. The mouse reticle's offset was the old ship's
	// turn command, so it doesn't carry over (T3 finding: the opening frame could start banked / turning).
	VirtualStick = FVector2D::ZeroVector;
	CurrentFlightInput.Aim = FVector2D::ZeroVector;
	// Held buttons carry over a switch (keep firing / boosting into the new ship), but the trigger
	// state must be re-sent to the new ship's weapons.
	if (bFirePrimaryHeld) { RouteTrigger(ETDWeaponSlot::Primary, true); }
	if (bFireSecondaryHeld) { RouteTrigger(ETDWeaponSlot::Secondary, true); }
	PushFlightInput();
}

void ATDSpacePlayerController::OnUnPossess()
{
	ReleaseHeldPowers();
	if (bThrottleUpApplied)
	{
		if (ATDSpacecraft* Craft = GetSpacecraft())
		{
			if (UTDShipMovementComponent* Move = Craft->GetShipMovement())
			{
				Move->ClearThrottleTopSpeed();
			}
		}
		bThrottleUpApplied = false;
	}
	// Release the old ship's triggers so its wingman AI starts clean. Never touch its velocity.
	// A charged secondary (Splitter) is cancelled first so the release doesn't fire it.
#if TD_HAS_WEAPONS
	if (bFireSecondaryHeld)
	{
		if (ATDSpacecraft* Craft = GetSpacecraft())
		{
			TDSpacePC_CallCancelCharge(Craft->GetWeapons(), ETDWeaponSlot::Secondary);
		}
	}
#endif
	if (bFirePrimaryHeld) { RouteTrigger(ETDWeaponSlot::Primary, false); }
	if (bFireSecondaryHeld) { RouteTrigger(ETDWeaponSlot::Secondary, false); }
	// The old ship keeps whatever input was latched last until its wingman AI writes its own: hand it back
	// neutral (cruise, no turn / roll / boost / drift / brake) so a held stick or boost doesn't fly the wingman.
	if (ATDSpacecraft* Craft = GetSpacecraft())
	{
		FTDFlightInput Neutral;
		Neutral.Throttle = 1.f;
		Craft->SetFlightInput(Neutral);
	}
	Super::OnUnPossess();
}

void ATDSpacePlayerController::ResetHeldInputs()
{
	// A held hyperdrive key's release is swallowed while input is disabled: stop a spool now, never mid-jump.
	if (const ATDShip* Ship = Cast<ATDShip>(GetPawn()))
	{
		if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			if (Hyper->GetState() == ETDHyperdriveState::Spooling)
			{
				Hyper->StopSpool();
			}
		}
	}
	if (bFirePrimaryHeld) { RouteTrigger(ETDWeaponSlot::Primary, false); }
	if (bFireSecondaryHeld) { RouteTrigger(ETDWeaponSlot::Secondary, false); }
	bFirePrimaryHeld = bFireSecondaryHeld = false;
	// A held F must not stay armed through a hand-over (review minor 7): after re-boarding it would start the
	// approach assist and land the ship again by itself.
	LandPressTime = -1.0;
	bLandFired = false;
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

	if (!bSpaceInputEnabled || !GetSpacecraft())
	{
		// Disabled (cutscene, autopilot) or on foot: don't touch any ship's latched input at all.
		return;
	}

	// Hold-Q: Signature2 fires the moment the hold threshold is crossed.
	if (PowerPressTime >= 0.0 && !bPowerImmediate && !bPowerHoldFired && Now - PowerPressTime >= PowerHoldSeconds)
	{
		bPowerHoldFired = true;
		RoutePower(ETDPowerInput::Signature2);
	}

	// Hold F / A to land (the ship's landing component decides whether a pad is in reach).
	if (LandPressTime >= 0.0 && !bLandFired)
	{
		const ATDShip* Ship = Cast<ATDShip>(GetPawn());
		const float Hold = (Ship && Ship->GetLanding()) ? Ship->GetLanding()->Tuning.HoldToLandSeconds : 0.5f;
		if (Now - LandPressTime >= Hold)
		{
			if (RouteLand())
			{
				bLandFired = true; // one action per hold
			}
			else if (Ship && Ship->GetLanding() && !Ship->GetLanding()->IsBusy())
			{
				LandPressTime = Now; // flying, not possible yet (too far): keep holding to retry
			}
			else
			{
				bLandFired = true; // mid landing / take-off sequence: this hold is spent (never takes off at touchdown)
			}
		}
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
	ApplyThrottleUp();

	FVector2D Aim;
	if (bRadialOpen)
	{
		Aim = FVector2D::ZeroVector; // right stick picks a ship while the radial is open
	}
	else if (bUsingGamepad)
	{
		Aim = TDSpacePC_ShapeStickImpl(StickAim, StickDeadzone, StickResponseExponent) * StickSensitivity;
		Aim.X = FMath::Clamp(Aim.X, -1.f, 1.f);
		Aim.Y = FMath::Clamp(Aim.Y, -1.f, 1.f);
		if (bInvertStickPitch) { Aim.Y = -Aim.Y; }
	}
	else
	{
		Aim = TDSpacePC_ShapeStickImpl(VirtualStick, MouseDeadzone, MouseResponseExponent);
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
		if (UTDWeaponComponent* Weapons = Craft->GetWeapons())
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
		// When disabled, ResetHeldInputs has already neutralised this to "cruise, no buttons"; it's pushed
		// once there, and PlayerTick then leaves the ship alone for cutscenes and autopilots.
		Craft->SetFlightInput(CurrentFlightInput);
	}
}

FVector ATDSpacePlayerController::GetAimWorldDirection() const
{
	int32 SizeX = 0, SizeY = 0;
	GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0)
	{
		SizeX = 1920; // headless / not yet rendered: assume 16:9
		SizeY = 1080;
	}
	return GetAimWorldDirectionForViewport(SizeX, SizeY);
}

FVector ATDSpacePlayerController::GetAimWorldDirectionForViewport(int32 SizeX, int32 SizeY) const
{
	const ATDSpacecraft* Craft = GetSpacecraft();
	const FVector Fallback = Craft ? Craft->GetActorForwardVector() : FVector::ForwardVector;
	if (!Craft || SizeX <= 0 || SizeY <= 0)
	{
		return Fallback;
	}

	// The reticle ray comes from the ship's OWN camera (chase or cockpit), never from the current view
	// target: a director shot (T3) can take over the screen without moving the player's aim.
	FTransform CameraXf;
	float FOV = 90.f;
	if (const ATDShip* Ship = Cast<ATDShip>(Craft))
	{
		if (Ship->IsCockpitView() && Ship->GetCockpitCamera())
		{
			CameraXf = Ship->GetCockpitCamera()->GetComponentTransform();
			FOV = Ship->GetCockpitCamera()->FieldOfView;
		}
		else
		{
			CameraXf = Ship->GetChaseCameraTransformNow();
			FOV = (Ship->IsPlayerFlown() && Ship->GetChaseCamera()) ? Ship->GetChaseCamera()->FieldOfView : Ship->GetChaseCameraFOVNow();
		}
	}
	else if (PlayerCameraManager)
	{
		CameraXf = FTransform(PlayerCameraManager->GetCameraRotation(), PlayerCameraManager->GetCameraLocation());
		FOV = PlayerCameraManager->GetFOVAngle();
	}
	else
	{
		return Fallback;
	}

	// The reticle sits at VirtualStick on screen; on gamepad it is the screen centre.
	const FVector2D Stick = bUsingGamepad ? FVector2D::ZeroVector : VirtualStick;
	const float HalfMin = 0.5f * FMath::Min(SizeX, SizeY);
	const FVector2D ScreenPos(0.5f * SizeX + Stick.X * HalfMin * ReticleScreenScale, 0.5f * SizeY - Stick.Y * HalfMin * ReticleScreenScale);

	// Perspective deprojection with a horizontal FOV (Unreal's convention).
	const float NdcX = (ScreenPos.X / SizeX) * 2.f - 1.f;
	const float NdcY = 1.f - (ScreenPos.Y / SizeY) * 2.f;
	const float TanHalf = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FOV, 5.f, 170.f) * 0.5f));
	const float Aspect = static_cast<float>(SizeX) / static_cast<float>(SizeY);
	const FQuat CamQ = CameraXf.GetRotation();
	const FVector WorldDir = (CamQ.GetForwardVector() + CamQ.GetRightVector() * (NdcX * TanHalf) + CamQ.GetUpVector() * (NdcY * TanHalf / Aspect)).GetSafeNormal();

	// Converge on a point far ahead so shots from the ship's muzzles meet the reticle line.
	const FVector AimPoint = CameraXf.GetLocation() + WorldDir * AimConvergenceDistance;
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

bool ATDSpacePlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	// On foot there's no stick-aim action to tell the devices apart, so the raw keys do it: any pad button, or a pad
	// axis past the stick deadzone, means "using gamepad"; any key or mouse button press, or a real mouse move,
	// means mouse and keyboard. On-foot prompts read IsUsingGamepad() to switch their wording. In flight the
	// stick/mouse aim hysteresis above stays in charge.
	if (GetPawn() != nullptr && GetSpacecraft() == nullptr)
	{
		const FKey& K = Params.Key;
		if (K.IsGamepadKey())
		{
			if (K.IsAxis1D() || K.IsAxis2D() ? FMath::Abs(Params.AmountDepressed) > StickDeadzone : Params.Event == IE_Pressed)
			{
				bUsingGamepad = true;
			}
		}
		else if (!K.IsTouch())
		{
			const bool bMouseMove = K == EKeys::MouseX || K == EKeys::MouseY;
			if (bMouseMove ? FMath::Abs(Params.AmountDepressed) >= MouseTakeOverThreshold
				: (!K.IsAxis1D() && !K.IsAxis2D() && Params.Event == IE_Pressed))
			{
				bUsingGamepad = false;
			}
		}
	}
	return Super::InputKey(Params);
}

void ATDSpacePlayerController::OnMouseAim(const FInputActionValue& Value)
{
	FVector2D Delta = Value.Get<FVector2D>();
	if (Delta.IsNearlyZero())
	{
		return;
	}
	if (bUsingGamepad && Delta.Size() < MouseTakeOverThreshold)
	{
		return; // a bumped desk doesn't steal aim from the gamepad
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
	if (StickAim.Size() > StickDeadzone && !bUsingGamepad)
	{
		bUsingGamepad = true;
		VirtualStick = FVector2D::ZeroVector; // the mouse reticle doesn't linger off-centre under gamepad play
	}
}

void ATDSpacePlayerController::OnStickAimReleased(const FInputActionValue&)
{
	StickAim = FVector2D::ZeroVector;
}

bool ATDSpacePlayerController::RouteDodge(float Direction)
{
	ATDShip* Ship = Cast<ATDShip>(GetPawn());
	return Ship && bSpaceInputEnabled && Ship->TryDodge(Direction);
}

void ATDSpacePlayerController::OnDodgePressed(const FInputActionValue&)
{
	// Dedicated key: toward the held roll, else the steering side, else right.
	float Dir = CurrentFlightInput.Roll;
	if (FMath::Abs(Dir) < 0.2f)
	{
		Dir = CurrentFlightInput.Aim.X;
	}
	RouteDodge(FMath::Abs(Dir) < 0.05f ? 1.f : FMath::Sign(Dir));
}

void ATDSpacePlayerController::OnDodgeTap(const FInputActionValue& Value)
{
	const float V = Value.Get<float>();
	if (FMath::IsNearlyZero(V))
	{
		return;
	}
	const int32 Side = V > 0.f ? 1 : 0;
	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	if (LastDodgeTapTime[Side] >= 0.0 && Now - LastDodgeTapTime[Side] <= DodgeDoubleTapWindow)
	{
		LastDodgeTapTime[Side] = -1.0;
		RouteDodge(Side == 1 ? 1.f : -1.f);
	}
	else
	{
		LastDodgeTapTime[Side] = Now;
		LastDodgeTapTime[1 - Side] = -1.0;
	}
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
	// Ships without a second signature power (Rift, Lancer) fire on press: no hold delay to wait out.
	bool bHasSignature2 = false;
#if TD_HAS_POWERS
	bHasSignature2 = UTDPowerComponent::FindPower(GetPawn(), ETDPowerInput::Signature2) != nullptr;
#endif
	bPowerImmediate = !bHasSignature2;
	if (bPowerImmediate)
	{
		RoutePower(ETDPowerInput::Signature1, true);
	}
}

void ATDSpacePlayerController::OnPowerReleased(const FInputActionValue&)
{
	if (PowerPressTime >= 0.0)
	{
		if (bPowerImmediate)
		{
			RoutePower(ETDPowerInput::Signature1, false);
		}
		else if (!bPowerHoldFired)
		{
			// Tap on a two-power ship (Bastion): Signature1 press + release.
			RoutePower(ETDPowerInput::Signature1, true);
			RoutePower(ETDPowerInput::Signature1, false);
		}
		else
		{
			RoutePower(ETDPowerInput::Signature2, false); // release edge after a hold (Bastion's dome)
		}
	}
	PowerPressTime = -1.0;
	bPowerHoldFired = false;
	bPowerImmediate = false;
}

void ATDSpacePlayerController::OnAbility(const FInputActionValue&) { bAbilityHeld = true; RoutePower(ETDPowerInput::Ability, true); }
void ATDSpacePlayerController::OnAbilityReleased(const FInputActionValue&)
{
	if (bAbilityHeld) // a key-up arriving after a ship switch was already released on the old ship
	{
		bAbilityHeld = false;
		RoutePower(ETDPowerInput::Ability, false);
	}
}

void ATDSpacePlayerController::ReleaseHeldPowers()
{
	// The ship being left must see the release edge of anything still held (Bastion's dome hold).
	if (PowerPressTime >= 0.0)
	{
		if (bPowerImmediate)
		{
			RoutePower(ETDPowerInput::Signature1, false);
		}
		else if (bPowerHoldFired)
		{
			RoutePower(ETDPowerInput::Signature2, false);
		}
	}
	if (bAbilityHeld)
	{
		RoutePower(ETDPowerInput::Ability, false);
	}
	PowerPressTime = -1.0;
	bPowerHoldFired = false;
	bPowerImmediate = false;
	bAbilityHeld = false;
}

void ATDSpacePlayerController::ApplyThrottleUp()
{
	ATDSpacecraft* Craft = GetSpacecraft();
	UTDShipMovementComponent* Move = Craft ? Craft->GetShipMovement() : nullptr;
	if (!Move)
	{
		return;
	}
	if (ThrottleAxis > 0.f && ThrottleUpBonus > 0.f)
	{
		Move->SetThrottleTopSpeed(Move->GetFlightTuning().CruiseSpeed * (1.f + ThrottleUpBonus * ThrottleAxis));
		bThrottleUpApplied = true;
	}
	else if (bThrottleUpApplied)
	{
		Move->ClearThrottleTopSpeed();
		bThrottleUpApplied = false;
	}
}

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
void ATDSpacePlayerController::OnHyperdrivePressed(const FInputActionValue&) { OnHyperdriveInput.Broadcast(true); RouteHyperdrive(true); }
void ATDSpacePlayerController::OnHyperdriveReleased(const FInputActionValue&) { OnHyperdriveInput.Broadcast(false); RouteHyperdrive(false); }
void ATDSpacePlayerController::OnSquadCommand(const FInputActionValue&) { RouteSquadCommand(); }
void ATDSpacePlayerController::OnLand(const FInputActionValue&)
{
	OnLandInput.Broadcast();
	LandPressTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	bLandFired = false;
	// F again while the approach assist flies the ship: the pilot takes back control (this press is spent).
	if (const ATDShip* Ship = Cast<ATDShip>(GetPawn()))
	{
		if (UTDLandingComponent* Landing = Ship->GetLanding())
		{
			if (Landing->IsApproachAssisting())
			{
				Landing->StopApproachAssist();
				bLandFired = true;
			}
		}
	}
}

void ATDSpacePlayerController::OnLandReleased(const FInputActionValue&)
{
	LandPressTime = -1.0;
	bLandFired = false;
	// Letting go of F keeps an engaged approach assist flying: a first-time player who holds F for a moment on
	// arrival (throttle still on) must not sail past the pad (T5's journey). Tap F again or boost to cancel.
}

void ATDSpacePlayerController::OnCameraToggle(const FInputActionValue&)
{
	if (ATDShip* Ship = Cast<ATDShip>(GetPawn()))
	{
		Ship->ToggleCockpitView();
	}
}
void ATDSpacePlayerController::OnPause(const FInputActionValue&) { OnPauseInput.Broadcast(); }
void ATDSpacePlayerController::OnGalaxyMap(const FInputActionValue&)
{
	if (bGalaxyMapEnabled)
	{
		OnGalaxyMapInput.Broadcast();
	}
}

// -------------------------------------------------------------------------------------------- routing

void ATDSpacePlayerController::RouteTrigger(ETDWeaponSlot Slot, bool bHeld)
{
	OnWeaponInput.Broadcast(Slot, bHeld);
#if TD_HAS_WEAPONS
	if (ATDSpacecraft* Craft = GetSpacecraft())
	{
		if (UTDWeaponComponent* Weapons = Craft->GetWeapons())
		{
			Weapons->SetTriggerHeld(Slot, bHeld && bSpaceInputEnabled);
		}
	}
#endif
}

void ATDSpacePlayerController::RoutePower(ETDPowerInput Power, bool bPressed)
{
	if (!bSpaceInputEnabled && bPressed)
	{
		return; // releases still go through so nothing stays held
	}
	if (bPressed)
	{
		OnPowerInput.Broadcast(Power);
	}
#if TD_HAS_POWERS
	UTDPowerComponent::RoutePowerInput(GetPawn(), Power, bPressed);
#endif
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
		const ATDShip* Ship = Cast<ATDShip>(P);
		UTDTargetingComponent* Targeting = Ship ? Ship->GetTargeting() : P->FindComponentByClass<UTDTargetingComponent>();
		if (Targeting)
		{
			Targeting->CycleTarget();
		}
	}
#endif
}

void ATDSpacePlayerController::RouteHyperdrive(bool bPressed)
{
	const ATDShip* Ship = Cast<ATDShip>(GetPawn());
	UTDHyperdriveComponent* Hyper = Ship ? Ship->GetHyperdrive() : nullptr;
	if (!Hyper || !bSpaceInputEnabled)
	{
		return;
	}
	if (bPressed)
	{
		// Press again during a jump = drop out; otherwise start spooling (hold).
		if (Hyper->IsJumping())
		{
			Hyper->RequestDropOut();
		}
		else
		{
			Hyper->RequestSpoolFromPilot(); // refused: HUD reason + refused cue
		}
	}
	else
	{
		Hyper->StopSpool();
	}
}

bool ATDSpacePlayerController::RouteLand()
{
	const ATDShip* Ship = Cast<ATDShip>(GetPawn());
	UTDLandingComponent* Landing = Ship ? Ship->GetLanding() : nullptr;
	if (!Landing || !bSpaceInputEnabled)
	{
		return false;
	}
	// Still sitting in a landed ship (no on-foot hand-off took the pilot out): hold F takes off again.
	// This keeps landing from ever being a dead end, in every build.
	if (Landing->IsLanded())
	{
		return Landing->RequestTakeOff();
	}
	// Slow and close: land now. Otherwise, near a pad, the approach assist flies the ship in while F is held.
	return Landing->RequestLand() || Landing->StartApproachAssist();
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

void ATDSpacePlayerController::ApplyPlayerSettings(const FTDPlayerSettings& Settings)
{
	const ATDSpacePlayerController* Defaults = GetClass()->GetDefaultObject<ATDSpacePlayerController>();
	MouseSensitivity = Defaults->MouseSensitivity * Settings.MouseSensitivity;
	bInvertMousePitch = Settings.bInvertMouseY;
	StickSensitivity = Settings.GamepadSensitivity;
	StickDeadzone = Settings.GamepadDeadzone;
	bInvertStickPitch = Settings.bInvertGamepadY;
}

void ATDSpacePlayerController::OnFootCameraToggle(const FInputActionValue&)
{
	if (GetPawn() == nullptr || GetSpacecraft() != nullptr || IsGamePausedByPlayer())
	{
		return; // on foot only (the context is only added on foot; this guards a stale binding)
	}
	if (UTDSettingsSubsystem* Options = UTDSettingsSubsystem::GetTDSettings(this))
	{
		Options->ToggleOnFootCameraMode(); // saved; the ground character applies it (after a director shot / death cam)
	}
}

void ATDSpacePlayerController::OnOpenSettings(const FInputActionValue&)
{
	if (IsGamePausedByPlayer())
	{
		UTDSettingsSubsystem::OpenSettingsPage(this);
	}
}
