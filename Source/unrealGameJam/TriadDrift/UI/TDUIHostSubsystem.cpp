// Triad Drift - engineering host for Nick's native space HUD (Teammate 2).

#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/UI/TDUIHostSettings.h"
#include "TriadDrift/UI/TDGroundHUDWidget.h"
#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "TriadDrift/UI/TDTacticalDisplayWidget.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "InputCoreTypes.h"
#include "TriadDrift/UI/TDScanMonitorWidget.h"
#include "TriadDrift/UI/TDRewardAnnouncementWidget.h"
#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "Components/MeshComponent.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TriadDrift/UI/TDPauseMenuWidget.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Powers/TDAbilityRewards.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDUIHost, Log, All);

namespace TDUIHost
{
	const TCHAR* const NativeWidgetClassPath = TEXT("/Script/TriadDriftHUD.TDSpaceHUDWidget");
	const float CheckInterval = 0.2f;

	float ReadUIShipNumber(const UStruct* Type, const void* Memory, const TCHAR* Name)
	{
		const FNumericProperty* P = FindFProperty<FNumericProperty>(Type, Name);
		if (!P || !Memory)
		{
			return 0.f;
		}
		const void* Value = P->ContainerPtrToValuePtr<void>(Memory);
		return P->IsFloatingPoint() ? float(P->GetFloatingPointPropertyValue(Value)) : float(P->GetSignedIntPropertyValue(Value));
	}

	bool ReadUIShipFlag(const UStruct* Type, const void* Memory, const TCHAR* Name)
	{
		const FBoolProperty* P = FindFProperty<FBoolProperty>(Type, Name);
		return P && Memory && P->GetPropertyValue_InContainer(Memory);
	}
}

UTDUIHostSubsystem* UTDUIHostSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDUIHostSubsystem>() : nullptr;
}

bool UTDUIHostSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

double UTDUIHostSubsystem::LastGalaxyReveal = -1.0;

void UTDUIHostSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// The descent cover clears on the galaxy before this world's flight HUD exists: remember the reveal here.
	if (UTDDescentSubsystem* Descent = UTDDescentSubsystem::Get(GetWorld()))
	{
		RevealHandle = Descent->OnRevealed.AddLambda([](ETDDestination Destination)
		{
			if (Destination == ETDDestination::Galaxy)
			{
				LastGalaxyReveal = FPlatformTime::Seconds();
			}
		});
	}
}

void UTDUIHostSubsystem::Deinitialize()
{
	if (UTDDescentSubsystem* Descent = UTDDescentSubsystem::Get(GetWorld()))
	{
		Descent->OnRevealed.Remove(RevealHandle);
	}
	Hide();
	HideGround();
	HidePause();
	if (ScanWidget)
	{
		ScanWidget->RemoveFromParent();
		ScanWidget = nullptr;
	}
	if (RewardWidget)
	{
		RewardWidget->RemoveFromParent(); // teardown without acknowledgment: the award stays pending, re-presented later
		RewardWidget = nullptr;
		PresentedReward = ETDAbilityId::None;
	}
	if (TacticalWidget)
	{
		TacticalWidget->RemoveFromParent();
		TacticalWidget = nullptr;
	}
	BindPause(nullptr);
	BindHitSounds(false);
	SetDevHUDSuppressed(false);
	Super::Deinitialize();
}

TStatId UTDUIHostSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDUIHostSubsystem, STATGROUP_Tickables);
}

void UTDUIHostSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// The Aftermath tactical panel belongs to the phase it was shown in: any phase change closes it (live: it stayed
	// over the reticle into the next wave after a stage jump).
	if (TacticalWidget)
	{
		const ITDGameFlow* Flow = UTDStatics::GetGameFlow(this);
		const int32 Phase = Flow ? static_cast<int32>(Flow->GetProgress().Phase) : -1;
		if (TacticalPhase < 0)
		{
			TacticalPhase = Phase;
		}
		else if (Phase != TacticalPhase)
		{
			TacticalWidget->RemoveFromParent();
			TacticalWidget = nullptr;
		}
	}
	if (!TacticalWidget)
	{
		TacticalPhase = -1;
	}
	UpdateTacticalDismiss();
	if (const UTDTacticalDisplayWidget* Display = Cast<UTDTacticalDisplayWidget>(TacticalWidget); Display && Display->IsFinished())
	{
		TacticalWidget->RemoveFromParent();
		TacticalWidget = nullptr;
	}
	if (const UTDScanMonitorWidget* Monitor = Cast<UTDScanMonitorWidget>(ScanWidget); Monitor && Monitor->IsFinished())
	{
		ScanWidget->RemoveFromParent();
		ScanWidget = nullptr;
	}
	ApplyFocus();
	TimeUntilCheck -= DeltaTime;
	if (TimeUntilCheck > 0.f)
	{
		return;
	}
	TimeUntilCheck = TDUIHost::CheckInterval;
	Evaluate();
	UpdateAbilityReward();
}

void UTDUIHostSubsystem::ApplyFocus()
{
	float Opacity = 1.f;
	bool bHideCombat = false;
	for (const TPair<FName, FTDHUDFocus>& It : Focus)
	{
		Opacity = FMath::Min(Opacity, It.Value.Opacity);
		bHideCombat |= It.Value.bHideCombat;
	}
	if (UTDFlightHUDWidget* Flight = Cast<UTDFlightHUDWidget>(Widget))
	{
		Flight->FocusOpacity = Opacity;
		Flight->bFocusHideCombat = bHideCombat;
	}
}

void UTDUIHostSubsystem::PushHUDFocus(const UObject* WorldContext, FName Reason, float Opacity, bool bHideCombat)
{
	if (UTDUIHostSubsystem* Host = Get(WorldContext))
	{
		Host->Focus.Add(Reason, { FMath::Clamp(Opacity, 0.f, 1.f), bHideCombat });
		Host->ApplyFocus();
	}
}

void UTDUIHostSubsystem::PopHUDFocus(const UObject* WorldContext, FName Reason)
{
	if (UTDUIHostSubsystem* Host = Get(WorldContext))
	{
		Host->Focus.Remove(Reason);
		Host->ApplyFocus();
	}
}

bool UTDUIHostSubsystem::IsCombatPresentationHeld(const UObject* WorldContext)
{
	const UTDUIHostSubsystem* Host = Get(WorldContext);
	if (!Host)
	{
		return false;
	}
	for (const TPair<FName, FTDHUDFocus>& It : Host->Focus)
	{
		if (It.Value.bHideCombat)
		{
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------ decide

APlayerController* UTDUIHostSubsystem::FindLocalController() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController() && PC->GetLocalPlayer())
		{
			return PC;
		}
	}
	return nullptr;
}

bool UTDUIHostSubsystem::WantsSpaceHUD(APlayerController*& OutController, UObject*& OutSource) const
{
	OutController = nullptr;
	OutSource = nullptr;
	if (UTDUIHostSettings::GetSpaceHUDChoice() == ETDSpaceHUDChoice::Dev)
	{
		return false;
	}
	ATDSquadManager* Squad = ATDSquadManager::Get(this);
	APlayerController* PC = FindLocalController();
	// On foot (planets, the Dominion interior) the pawn is the first-person character: the widget has
	// no on-foot half yet, so the dev HUD stays there.
	if (bCinematicHidden || !Squad || !PC || !Cast<ATDShip>(PC->GetPawn()))
	{
		return false;
	}
	OutController = PC;
	OutSource = Squad;
	return true;
}

void UTDUIHostSubsystem::Evaluate()
{
	// On foot and pause first: independent of the space HUD's switch.
	APlayerController* LocalPC = FindLocalController();
	BindPause(LocalPC);
	BindHitSounds(true);
	if (WantsGroundHUD(LocalPC))
	{
		if (!GroundWidget || GroundOwner.Get() != LocalPC)
		{
			HideGround();
			ShowGround(LocalPC);
		}
	}
	else
	{
		HideGround();
	}
	if (PauseWidget && !(GetWorld() && GetWorld()->IsPaused()))
	{
		HidePause(); // resumed by something that didn't broadcast (a retake, travel)
	}
	UpdateDevHUDSuppression();

	APlayerController* PC = nullptr;
	UObject* Source = nullptr;
	if (!WantsSpaceHUD(PC, Source))
	{
		Hide();
		return;
	}
	if (Widget && Owner.Get() == PC && ShownChoice == UTDUIHostSettings::GetSpaceHUDChoice())
	{
		if (DataSource.Get() != Source)
		{
			DataSource = Source;
			CallSetDataSource(Widget, Source);
		}
		return;
	}
	Hide();
	ShowFor(PC, Source);
}

// ------------------------------------------------------------------------------------------ show / hide

void UTDUIHostSubsystem::ShowFor(APlayerController* Controller, UObject* Source)
{
	const ETDSpaceHUDChoice Choice = UTDUIHostSettings::GetSpaceHUDChoice();
	UClass* Class = Choice == ETDSpaceHUDChoice::Engineering ? UTDFlightHUDWidget::StaticClass() : ResolveWidgetClass();
	if (!Class)
	{
		if (!bClassWarningLogged)
		{
			bClassWarningLogged = true;
			UE_LOG(LogTDUIHost, Warning, TEXT("Space HUD is switched on but no widget class loads (%s, %s). Is the TriadDriftHUD plugin enabled? The dev HUD stays."),
				*UTDUIHostSettings::Get()->SpaceHUDClass.ToString(), TDUIHost::NativeWidgetClassPath);
		}
		return;
	}
	UUserWidget* NewWidget = CreateWidget<UUserWidget>(Controller, Class);
	if (!NewWidget)
	{
		return;
	}
	if (!CallSetDataSource(NewWidget, Source))
	{
		if (!bClassWarningLogged)
		{
			bClassWarningLogged = true;
			UE_LOG(LogTDUIHost, Warning, TEXT("%s has no compatible SetDataSource(UObject*); the space HUD stays off."), *Class->GetPathName());
		}
		return;
	}
	Widget = NewWidget;
	Owner = Controller;
	DataSource = Source;
	ShownChoice = Choice;
	const UWorld* World = GetWorld();
	if (bAddToViewport && World && World->GetGameViewport())
	{
		Widget->AddToViewport(UTDUIHostSettings::Get()->ZOrder);
		// SetDataSource ran before NativeConstruct bound the event bus; sample again now it's live.
		CallRefreshData(Widget);
	}
	UpdateDevHUDSuppression();
	UE_LOG(LogTDUIHost, Log, TEXT("Space HUD shown (%s) for %s."), *Class->GetName(), *Controller->GetName());
}

void UTDUIHostSubsystem::Hide()
{
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	Owner.Reset();
	DataSource.Reset();
	UpdateDevHUDSuppression();
}

void UTDUIHostSubsystem::SetCinematicHidden(bool bHidden)
{
	if (bCinematicHidden == bHidden)
	{
		return;
	}
	bCinematicHidden = bHidden;
	APlayerController* PC = FindLocalController();
	if (bHidden)
	{
		HidePause();
		if (AHUD* HUD = PC ? PC->GetHUD() : nullptr)
		{
			CinematicHUD = HUD;
			bCinematicHUDWasShown = HUD->bShowHUD;
			HUD->bShowHUD = false;
		}
	}
	else if (AHUD* HUD = CinematicHUD.Get())
	{
		HUD->bShowHUD = bCinematicHUDWasShown;
		CinematicHUD.Reset();
	}
	UE_LOG(LogTDUIHost, Log, TEXT("All HUDs %s for a cinematic."), bHidden ? TEXT("hidden") : TEXT("restored"));
	Evaluate();
}

FBox2D UTDUIHostSubsystem::LastSubtitlePlate01(ForceInit);
double UTDUIHostSubsystem::LastSubtitlePlateTime = -1.0;

float UTDUIHostSubsystem::GetSubtitleCeiling01(const UObject* WorldContext)
{
	const UTDUIHostSubsystem* Host = Get(WorldContext);
	const UTDGroundHUDWidget* Ground = Host ? Cast<UTDGroundHUDWidget>(Host->GroundWidget) : nullptr;
	return Ground && Ground->IsInViewport() && Ground->IsVisible() ? Ground->SubtitleCeiling01 : 1.f;
}

void UTDUIHostSubsystem::UpdateTacticalDismiss()
{
	UTDTacticalDisplayWidget* Display = Cast<UTDTacticalDisplayWidget>(TacticalWidget);
	if (!Display || Display->IsDismissing() || Display->IsFinished())
	{
		return;
	}
	APlayerController* PC = FindLocalController();
	const ATDSpacePlayerController* SpacePC = Cast<ATDSpacePlayerController>(PC);
	const bool bPad = SpacePC && SpacePC->IsUsingGamepad();
	Display->DismissKey = bPad ? TEXT("B") : TEXT("X");
	// Ryan (Nov 14): fire, boost, power, ability, switch, hyperdrive, or the skip / back key close it at once. It never
	// takes input: the keys still do their own job (polled, not consumed).
	static const FKey Keys[] = {
		EKeys::X, EKeys::BackSpace, EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::LeftShift, EKeys::Q, EKeys::E,
		EKeys::Tab, EKeys::One, EKeys::Two, EKeys::Three, EKeys::H, EKeys::Escape,
		EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_RightShoulder, EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_LeftThumbstick,
		EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right,
		EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_Special_Right };
	bool bClose = false;
	if (PC)
	{
		for (const FKey& Key : Keys)
		{
			if (PC->WasInputKeyJustPressed(Key))
			{
				bClose = true;
				break;
			}
		}
	}
	// ... and by itself the moment a hyperdrive spool starts or the objective changes.
	if (const ITDHUDData* Data = Cast<ITDHUDData>(DataSource.Get()))
	{
		const FString Objective = Data->GetObjectiveText().ToString();
		if (!bTacticalObjectiveKnown)
		{
			TacticalObjective = Objective;
			bTacticalObjectiveKnown = true;
		}
		else if (Objective != TacticalObjective)
		{
			bClose = true;
		}
		if (Data->GetHyperdriveCharge01() > .01f)
		{
			bClose = true;
		}
	}
	if (bClose)
	{
		Display->Dismiss();
	}
}

bool UTDUIHostSubsystem::ShowTacticalDisplay(const UObject* WorldContext, float Seconds)
{
	UTDUIHostSubsystem* Host = Get(WorldContext);
	APlayerController* PC = Host ? Host->FindLocalController() : nullptr;
	if (!PC)
	{
		return false;
	}
	UTDTacticalDisplayWidget* Display = Cast<UTDTacticalDisplayWidget>(Host->TacticalWidget);
	if (!Display)
	{
		Display = CreateWidget<UTDTacticalDisplayWidget>(PC, UTDTacticalDisplayWidget::StaticClass());
		if (!Display)
		{
			return false;
		}
		Host->TacticalWidget = Display;
		const UWorld* World = Host->GetWorld();
		if (Host->bAddToViewport && World && World->GetGameViewport())
		{
			Display->AddToViewport(UTDUIHostSettings::Get()->ZOrder + 50); // over the HUD and a cinematic, under menus
		}
	}
	// Never blocks input or takes focus (Ryan: "I couldn't remove it").
	Display->SetVisibility(ESlateVisibility::HitTestInvisible);
	Display->SetIsFocusable(false);
	Host->bTacticalObjectiveKnown = false;
	Display->Play(FMath::Min(Seconds, UTDTacticalDisplayWidget::MaxSeconds));
	return true;
}

bool UTDUIHostSubsystem::ShowScanMonitor(const UObject* WorldContext, int32 Stage, float Seconds)
{
	UTDUIHostSubsystem* Host = Get(WorldContext);
	APlayerController* PC = Host ? Host->FindLocalController() : nullptr;
	if (!PC)
	{
		return false;
	}
	UTDScanMonitorWidget* Monitor = Cast<UTDScanMonitorWidget>(Host->ScanWidget);
	if (!Monitor)
	{
		Monitor = CreateWidget<UTDScanMonitorWidget>(PC, UTDScanMonitorWidget::StaticClass());
		if (!Monitor)
		{
			return false;
		}
		Host->ScanWidget = Monitor;
		const UWorld* World = Host->GetWorld();
		if (Host->bAddToViewport && World && World->GetGameViewport())
		{
			Monitor->AddToViewport(UTDUIHostSettings::Get()->ZOrder + 50); // over the HUD and a cinematic, under menus
		}
	}
	Monitor->Show(Stage, Seconds);
	return true;
}

bool UTDUIHostSubsystem::ShowScanOnScreen(UMeshComponent* Mesh, FName ScreenSlot, bool bCinder, float Power)
{
	UTexture2D* Scan = UTDScanMonitorWidget::ResolveScanTexture(bCinder);
	if (!Mesh || !Scan)
	{
		return false;
	}
	const int32 Index = ScreenSlot.IsNone() ? 0 : Mesh->GetMaterialIndex(ScreenSlot);
	if (Index == INDEX_NONE || Index >= Mesh->GetNumMaterials())
	{
		UE_LOG(LogTDUIHost, Warning, TEXT("ShowScanOnScreen: %s has no material slot '%s'"), *GetNameSafe(Mesh), *ScreenSlot.ToString());
		return false;
	}
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Index));
	if (!MID)
	{
		MID = Mesh->CreateDynamicMaterialInstance(Index);
	}
	if (!MID)
	{
		return false;
	}
	MID->SetTextureParameterValue(TEXT("ScreenTexture"), Scan);
	MID->SetScalarParameterValue(TEXT("ScreenPower"), Power);
	return true;
}

void UTDUIHostSubsystem::HideScanMonitor(const UObject* WorldContext)
{
	const UTDUIHostSubsystem* Host = Get(WorldContext);
	if (UTDScanMonitorWidget* Monitor = Host ? Cast<UTDScanMonitorWidget>(Host->ScanWidget) : nullptr)
	{
		Monitor->Hide();
	}
}

bool UTDUIHostSubsystem::ShowNavigationPulse(const UObject* WorldContext, FVector From, FVector To, float Seconds)
{
	const UTDUIHostSubsystem* Host = Get(WorldContext);
	UTDFlightHUDWidget* Flight = Host ? Cast<UTDFlightHUDWidget>(Host->GetSpaceHUD()) : nullptr;
	if (!Flight)
	{
		return false;
	}
	Flight->ShowNavigationPulse(From, To, Seconds);
	return true;
}

float UTDUIHostSubsystem::GetNavigationPulse01(const UObject* WorldContext)
{
	const UTDUIHostSubsystem* Host = Get(WorldContext);
	const UTDFlightHUDWidget* Flight = Host ? Cast<UTDFlightHUDWidget>(Host->GetSpaceHUD()) : nullptr;
	return Flight ? Flight->GetNavigationPulseEnvelope() : 0.f;
}

bool UTDUIHostSubsystem::ShowTransmission(const UObject* WorldContext, FName Speaker, FText Line, uint8 Style, float Seconds)
{
	const UTDUIHostSubsystem* Host = Get(WorldContext);
	UTDFlightHUDWidget* Flight = Host ? Cast<UTDFlightHUDWidget>(Host->GetSpaceHUD()) : nullptr;
	if (!Flight)
	{
		return false;
	}
	const UTDFlightHUDWidget::ETransmissionStyle S = Style == 1 ? UTDFlightHUDWidget::ETransmissionStyle::Static
		: Style == 2 ? UTDFlightHUDWidget::ETransmissionStyle::Echo : UTDFlightHUDWidget::ETransmissionStyle::Clean;
	Flight->ShowTransmission(Speaker, Line, S, Seconds);
	return true;
}

void UTDUIHostSubsystem::SetCinematicHiddenIn(const UObject* WorldContext, bool bHidden)
{
	if (UTDUIHostSubsystem* Host = Get(WorldContext))
	{
		Host->SetCinematicHidden(bHidden);
	}
}

void UTDUIHostSubsystem::UpdateDevHUDSuppression()
{
	// While a squad exists and a real space HUD is chosen, a frame where the pawn is briefly not a ship (a switch, a
	// respawn) must not flash the dev HUD's full read-out for up to 0.2 s, packaged builds included (ENG-36 review m10).
	const bool bSpaceHUDChosen = UTDUIHostSettings::GetSpaceHUDChoice() != ETDSpaceHUDChoice::Dev && !bCinematicHidden;
	const APlayerController* PC = FindLocalController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const bool bOnFoot = Pawn && !Pawn->IsA<ATDSpacecraft>() && Pawn->GetClass()->ImplementsInterface(UTDHUDData::StaticClass());
	const bool bHoldForSquad = bSpaceHUDChosen && !bOnFoot && ATDSquadManager::Get(this) != nullptr;
	SetDevHUDSuppressed(Widget != nullptr || GroundWidget != nullptr || PauseWidget != nullptr || bHoldForSquad);
}

// ------------------------------------------------------------------------------------------ on foot

bool UTDUIHostSubsystem::WantsGroundHUD(APlayerController* PC) const
{
	if (bCinematicHidden || !PC || !UTDUIHostSettings::IsNativeGroundHUDEnabled())
	{
		return false;
	}
	// The first-person character (planets, the Dominion interior): a HUD-data pawn that is not a ship.
	const APawn* Pawn = PC->GetPawn();
	return Pawn && !Pawn->IsA<ATDSpacecraft>() && Pawn->GetClass()->ImplementsInterface(UTDHUDData::StaticClass());
}

namespace TDUIHost
{
	/** A soft class slot on DA_UI_Style by name (reflection: GroundHUDWidgetClass may not be on this branch's Core yet). */
	UClass* LoadStyleClass(FName Property)
	{
		const UTDUIStyle* Style = UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
		const FSoftClassProperty* Slot = Style ? FindFProperty<FSoftClassProperty>(UTDUIStyle::StaticClass(), Property) : nullptr;
		if (!Slot)
		{
			return nullptr;
		}
		const FSoftObjectPtr& Ptr = Slot->GetPropertyValue_InContainer(Style);
		UClass* Class = Cast<UClass>(Ptr.IsNull() ? nullptr : Ptr.LoadSynchronous());
		return Class && Class->IsChildOf(UUserWidget::StaticClass()) ? Class : nullptr;
	}
}

// ---- The ability reward screen (board N-070 / N-072). Nick's UTDAbilityRewardWidget lives in the TriadDriftHUD plugin,
// which this module does not link: its contract is called by name (README, IndustrialTerminal01).
namespace TDRewardMount
{
	/** Calls a no-argument function; OutBool gets a bool return value when there is one. */
	bool CallNoArgs(UObject* Target, FName Name, bool* OutBool = nullptr)
	{
		UFunction* F = Target ? Target->FindFunction(Name) : nullptr;
		if (!F)
		{
			return false;
		}
		const int32 Size = FMath::Max<int32>(F->ParmsSize, 1);
		uint8* Buf = static_cast<uint8*>(FMemory_Alloca(Size));
		FMemory::Memzero(Buf, Size);
		Target->ProcessEvent(F, Buf);
		if (const FBoolProperty* Ret = CastField<FBoolProperty>(F->GetReturnProperty()); Ret && OutBool)
		{
			*OutBool = Ret->GetPropertyValue_InContainer(Buf);
		}
		return true;
	}

	template <typename TProp, typename TValue>
	void SetField(const UScriptStruct* S, void* Data, const TCHAR* Name, const TValue& Value)
	{
		if (const TProp* P = FindFProperty<TProp>(S, Name))
		{
			P->SetPropertyValue_InContainer(Data, Value);
		}
	}

	struct FDisplay
	{
		FName AbilityId;
		FText AbilityName, ShipName, AvailabilityText, ActivationKey;
		FLinearColor Accent = FLinearColor::White;
		bool bCanUseNow = false;
	};

	/** PresentReward(const FTDAbilityRewardDisplay&) -> bool, the struct filled field by field by name. */
	bool Present(UObject* Widget, const FDisplay& D, bool& bAccepted)
	{
		bAccepted = false;
		UFunction* F = Widget ? Widget->FindFunction(TEXT("PresentReward")) : nullptr;
		if (!F)
		{
			return false;
		}
		const int32 Size = FMath::Max<int32>(F->ParmsSize, 1);
		uint8* Buf = static_cast<uint8*>(FMemory_Alloca(Size));
		FMemory::Memzero(Buf, Size);
		const FStructProperty* Arg = nullptr;
		for (TFieldIterator<FProperty> It(F); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->InitializeValue_InContainer(Buf);
			if (!Arg && !It->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				Arg = CastField<FStructProperty>(*It);
			}
		}
		if (Arg)
		{
			const UScriptStruct* S = Arg->Struct;
			void* Data = Arg->ContainerPtrToValuePtr<void>(Buf);
			SetField<FNameProperty>(S, Data, TEXT("AbilityId"), D.AbilityId);
			SetField<FTextProperty>(S, Data, TEXT("AbilityName"), D.AbilityName);
			SetField<FTextProperty>(S, Data, TEXT("ShipName"), D.ShipName);
			SetField<FTextProperty>(S, Data, TEXT("AvailabilityText"), D.AvailabilityText);
			SetField<FTextProperty>(S, Data, TEXT("ActivationKey"), D.ActivationKey);
			SetField<FBoolProperty>(S, Data, TEXT("bCanUseNow"), D.bCanUseNow);
			if (const FStructProperty* Accent = FindFProperty<FStructProperty>(S, TEXT("Accent")); Accent && Accent->Struct == TBaseStructure<FLinearColor>::Get())
			{
				*Accent->ContainerPtrToValuePtr<FLinearColor>(Data) = D.Accent;
			}
			Widget->ProcessEvent(F, Buf);
			if (const FBoolProperty* Ret = CastField<FBoolProperty>(F->GetReturnProperty()))
			{
				bAccepted = Ret->GetPropertyValue_InContainer(Buf);
			}
		}
		for (TFieldIterator<FProperty> It(F); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->DestroyValue_InContainer(Buf);
		}
		return Arg != nullptr;
	}

	FText UpperName(const UEnum* E, int64 Value)
	{
		// "EMPPulse" -> "EMP PULSE", "GravityWell" -> "GRAVITY WELL" (the enum's display name ran the acronym in: live).
		// Words split at a lower->Upper step and before the last capital of an acronym ("EMPPulse" -> "EMP PULSE").
		const FString N = E->GetNameStringByValue(Value);
		FString Out;
		for (int32 I = 0; I < N.Len(); ++I)
		{
			const bool bUpper = FChar::IsUpper(N[I]);
			const bool bAfterLower = I > 0 && FChar::IsLower(N[I - 1]);
			const bool bAcronymEnd = I > 0 && FChar::IsUpper(N[I - 1]) && I + 1 < N.Len() && FChar::IsLower(N[I + 1]);
			if (bUpper && (bAfterLower || bAcronymEnd))
			{
				Out.AppendChar(TEXT(' '));
			}
			Out.AppendChar(N[I]);
		}
		return FText::FromString(Out.ToUpper());
	}
}

bool UTDUIHostSubsystem::ShowSensorFlicker(const UObject* WorldContext, FVector At, float Seconds)
{
	UTDUIHostSubsystem* Host = Get(WorldContext);
	const UWorld* World = Host ? Host->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	Host->SensorAt = At;
	Host->SensorStart = World->GetTimeSeconds();
	Host->SensorSeconds = FMath::Max(Seconds, SensorVerifiedAt + 1.f);
	return true;
}

bool UTDUIHostSubsystem::GetSensorFlicker(FVector& OutAt, float& OutAge) const
{
	const UWorld* World = GetWorld();
	const float Age = World ? static_cast<float>(World->GetTimeSeconds() - SensorStart) : -1.f;
	if (Age < 0.f || Age > SensorSeconds)
	{
		return false;
	}
	OutAt = SensorAt;
	OutAge = Age;
	return true;
}

UClass* UTDUIHostSubsystem::ResolveRewardWidgetClass() const
{
	if (RewardWidgetClassOverride && RewardWidgetClassOverride->IsChildOf(UUserWidget::StaticClass()))
	{
		return RewardWidgetClassOverride;
	}
	if (UClass* Class = TDUIHost::LoadStyleClass(TEXT("AbilityRewardWidgetClass")))
	{
		return Class;
	}
	UClass* Nick = FindObject<UClass>(nullptr, TEXT("/Script/TriadDriftHUD.TDAbilityRewardWidget"));
	if (Nick && Nick->IsChildOf(UUserWidget::StaticClass()))
	{
		return Nick;
	}
	// No reward screen anywhere: engineering's native announcement, so an award is never silent (T7's Verdance playtest).
	static bool bSaid = false;
	if (!bSaid)
	{
		bSaid = true;
		UE_LOG(LogTemp, Display, TEXT("TD reward: no AbilityRewardWidgetClass and no TriadDriftHUD reward widget: the native announcement shows (UTDRewardAnnouncementWidget)."));
	}
	return UTDRewardAnnouncementWidget::StaticClass();
}

void UTDUIHostSubsystem::UpdateAbilityReward()
{
	UTDAbilityRewardSubsystem* Rewards = UTDAbilityRewardSubsystem::Get(this);
	APlayerController* PC = FindLocalController();
	if (!Rewards || !PC || bCinematicHidden)
	{
		return;
	}
	ETDAbilityId Ability = ETDAbilityId::None;
	ETDShipId Ship = ETDShipId::Rift;
	bool bCanUseNow = false;
	if (!Rewards->GetPendingAbilityReward(Ability, Ship, bCanUseNow, this))
	{
		return;
	}
	const ATDSpacePlayerController* SpacePC = Cast<ATDSpacePlayerController>(PC);
	const bool bGamepad = SpacePC && SpacePC->IsUsingGamepad();
	if (PresentedReward == Ability && bRewardShownGamepad == bGamepad && bRewardShownUsable == bCanUseNow)
	{
		return; // on screen and current
	}
	if (!RewardWidget)
	{
		UClass* Class = ResolveRewardWidgetClass();
		RewardWidget = Class ? CreateWidget<UUserWidget>(PC, Class) : nullptr;
		if (!RewardWidget)
		{
			return;
		}
		// Bound before mounting (the contract): the widget broadcasts exactly once per dismissal.
		if (const FMulticastDelegateProperty* Dismissed = FindFProperty<FMulticastDelegateProperty>(RewardWidget->GetClass(), TEXT("OnRewardDismissed")))
		{
			FScriptDelegate D;
			D.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UTDUIHostSubsystem, HandleRewardDismissed));
			Dismissed->AddDelegate(MoveTemp(D), RewardWidget);
		}
	}
	TDRewardMount::FDisplay D;
	D.AbilityId = FName(*StaticEnum<ETDAbilityId>()->GetNameStringByValue(static_cast<int64>(Ability)));
	D.AbilityName = TDRewardMount::UpperName(StaticEnum<ETDAbilityId>(), static_cast<int64>(Ability));
	D.ShipName = TDRewardMount::UpperName(StaticEnum<ETDShipId>(), static_cast<int64>(Ship));
	D.AvailabilityText = bCanUseNow ? NSLOCTEXT("TDUIHost", "RewardReady", "READY NOW")
		: FText::Format(NSLOCTEXT("TDUIHost", "RewardLater", "READY WHEN YOU FLY {0}"), D.ShipName);
	D.ActivationKey = UTDHintComponent::KeyFor(ETDHint::Ability, bGamepad);
	D.Accent = UTDStatics::ShipColor(Ship);
	D.bCanUseNow = bCanUseNow;
	bool bAccepted = false;
	if (!TDRewardMount::Present(RewardWidget, D, bAccepted) || !bAccepted)
	{
		return; // refused (another award still up, or bad data): tried again on the next check
	}
	const bool bFirstShow = PresentedReward != Ability;
	PresentedReward = Ability;
	bRewardShownGamepad = bGamepad;
	bRewardShownUsable = bCanUseNow;
	if (bAddToViewport && !RewardWidget->IsInViewport())
	{
		RewardWidget->AddToViewport(60);
	}
	if (bFirstShow)
	{
		TDRewardMount::CallNoArgs(RewardWidget, TEXT("FocusDismissButton"));
	}
	UE_LOG(LogTemp, Log, TEXT("TD reward: presented %s (ship %s, usable now %d, key %s, %s)"), *D.AbilityId.ToString(), *D.ShipName.ToString(),
		bCanUseNow, *D.ActivationKey.ToString(), *RewardWidget->GetClass()->GetName());
}

void UTDUIHostSubsystem::HandleRewardDismissed(FName AbilityId)
{
	const int64 Value = StaticEnum<ETDAbilityId>()->GetValueByNameString(AbilityId.ToString());
	const ETDAbilityId Ability = Value == INDEX_NONE ? ETDAbilityId::None : static_cast<ETDAbilityId>(Value);
	if (Ability == ETDAbilityId::None || Ability != PresentedReward)
	{
		return; // only the award on screen is acknowledged
	}
	if (UTDAbilityRewardSubsystem* Rewards = UTDAbilityRewardSubsystem::Get(this))
	{
		Rewards->MarkAbilityRewardPresented(Ability);
	}
	PresentedReward = ETDAbilityId::None;
	UpdateAbilityReward(); // the next pending award, at once
	if (PresentedReward == ETDAbilityId::None && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport(); // back to play (the previous focus may be gone)
	}
}

UClass* UTDUIHostSubsystem::ResolveGroundWidgetClass()
{
	UClass* Class = TDUIHost::LoadStyleClass(TEXT("GroundHUDWidgetClass"));
	return Class ? Class : UTDGroundHUDWidget::StaticClass();
}

UClass* UTDUIHostSubsystem::ResolvePauseWidgetClass()
{
	UClass* Class = TDUIHost::LoadStyleClass(TEXT("PauseWidgetClass"));
	return Class ? Class : UTDPauseMenuWidget::StaticClass();
}

void UTDUIHostSubsystem::ShowGround(APlayerController* PC)
{
	UUserWidget* NewWidget = PC ? CreateWidget<UUserWidget>(PC, ResolveGroundWidgetClass()) : nullptr;
	if (!NewWidget)
	{
		return;
	}
	GroundWidget = NewWidget;
	GroundOwner = PC;
	const UWorld* World = GetWorld();
	if (bAddToViewport && World && World->GetGameViewport())
	{
		GroundWidget->AddToViewport(UTDUIHostSettings::Get()->ZOrder);
	}
	UpdateDevHUDSuppression();
	UE_LOG(LogTDUIHost, Log, TEXT("Ground HUD shown (%s) for %s."), *NewWidget->GetClass()->GetName(), *PC->GetName());
}

void UTDUIHostSubsystem::HideGround()
{
	if (GroundWidget)
	{
		GroundWidget->RemoveFromParent();
		GroundWidget = nullptr;
	}
	GroundOwner.Reset();
	UpdateDevHUDSuppression();
}

// ------------------------------------------------------------------------------------------ pause

void UTDUIHostSubsystem::BindPause(APlayerController* PC)
{
	if (PauseBoundTo.Get() == PC)
	{
		return;
	}
	if (ATDSpacePlayerController* Old = Cast<ATDSpacePlayerController>(PauseBoundTo.Get()))
	{
		Old->OnPauseChanged.RemoveDynamic(this, &UTDUIHostSubsystem::HandlePauseChanged);
	}
	PauseBoundTo = PC;
	if (ATDSpacePlayerController* New = Cast<ATDSpacePlayerController>(PC))
	{
		New->OnPauseChanged.AddUniqueDynamic(this, &UTDUIHostSubsystem::HandlePauseChanged);
	}
}

void UTDUIHostSubsystem::BindHitSounds(bool bBind)
{
	UTDEventBus* Bus = bBind ? UTDEventBus::Get(this) : nullptr;
	if (HitBus.Get() == Bus)
	{
		return;
	}
	if (UTDEventBus* Old = HitBus.Get())
	{
		Old->OnHitConfirmed.RemoveDynamic(this, &UTDUIHostSubsystem::HandleHitConfirmed);
	}
	HitBus = Bus;
	if (Bus)
	{
		Bus->OnHitConfirmed.AddUniqueDynamic(this, &UTDUIHostSubsystem::HandleHitConfirmed);
	}
}

void UTDUIHostSubsystem::HandleHitConfirmed(AActor* Target, bool bKill)
{
	const UTDUIStyle* Style = StyleOverride ? StyleOverride.Get() : UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
	if (!Style)
	{
		return;
	}
	// Rapid fire would play the hit tick every frame: at most one per HitSoundMinInterval (real time). Kills always play.
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetRealTimeSeconds() : 0.0;
	if (!bKill && Now - LastHitSoundTime < HitSoundMinInterval)
	{
		++HitSoundsThrottled;
		return;
	}
	const FTDFXCue& Cue = bKill ? Style->KillConfirm : Style->HitConfirm;
	USoundBase* Sound = Cue.Sound.IsNull() ? nullptr : Cue.Sound.LoadSynchronous();
	if (!Sound)
	{
		return; // silent until Nick assigns the sound
	}
	UGameplayStatics::PlaySound2D(this, Sound); // 2D, no overrides: the asset's class, volume and concurrency apply
	++(bKill ? KillSoundsPlayed : HitSoundsPlayed);
	if (!bKill)
	{
		LastHitSoundTime = Now;
	}
}

void UTDUIHostSubsystem::HandlePauseChanged(bool bPaused)
{
	if (bPaused && !bCinematicHidden && UTDUIHostSettings::IsNativePauseScreenEnabled())
	{
		ShowPause(PauseBoundTo.Get() ? PauseBoundTo.Get() : FindLocalController());
	}
	else
	{
		HidePause();
	}
}

void UTDUIHostSubsystem::ShowPause(APlayerController* PC)
{
	if (PauseWidget || !PC)
	{
		return;
	}
	UUserWidget* NewWidget = CreateWidget<UUserWidget>(PC, ResolvePauseWidgetClass());
	if (!NewWidget)
	{
		return;
	}
	PauseWidget = NewWidget;
	const UWorld* World = GetWorld();
	if (bAddToViewport && World && World->GetGameViewport())
	{
		PauseWidget->AddToViewport(UTDUIHostSettings::Get()->ZOrder + 100);
		PauseWidget->SetUserFocus(PC);
		PauseWidget->SetKeyboardFocus();
	}
	UpdateDevHUDSuppression();
}

void UTDUIHostSubsystem::HidePause()
{
	if (PauseWidget)
	{
		PauseWidget->RemoveFromParent();
		PauseWidget = nullptr;
	}
	UpdateDevHUDSuppression();
}

void UTDUIHostSubsystem::SetDevHUDSuppressed(bool bSuppressed)
{
	if (bDevHUDSuppressed == bSuppressed)
	{
		return;
	}
	bDevHUDSuppressed = bSuppressed;
	// Cleared on every removal (Hide, which Deinitialize calls) so a stale world key never hides the dev HUD.
	ATDDebugHUD::SetRealHUDActive(GetWorld(), bSuppressed);
}

// ------------------------------------------------------------------------------------------ reflection

FName UTDUIHostSubsystem::GetCoverPropertyName(ETDHUDSurface Surface)
{
	const UEnum* Enum = StaticEnum<ETDHUDSurface>();
	return FName(*(TEXT("bCovers") + Enum->GetNameStringByValue(static_cast<int64>(Surface))));
}

namespace TDUIHost
{
	/** Dev override per surface: -1 = read the widget, 0 = not covered, 1 = covered. */
	TMap<ETDHUDSurface, int32>& CoverOverrides()
	{
		static TMap<ETDHUDSurface, int32> Map;
		return Map;
	}
}

bool UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD(const UObject* WorldContext, ETDHUDSurface Surface)
{
	const UTDUIHostSubsystem* Host = Get(WorldContext);
	if (!Host || (!Host->GetSpaceHUD() && !Host->GetGroundHUD() && !Host->GetPauseScreen()))
	{
		return false; // no hosted widget up: the fallback HUD draws everything
	}
	if (const int32* Forced = TDUIHost::CoverOverrides().Find(Surface))
	{
		if (*Forced >= 0)
		{
			return *Forced > 0;
		}
	}
	const FName Name = GetCoverPropertyName(Surface);
	for (const UUserWidget* HUD : { Host->GetSpaceHUD(), Host->GetGroundHUD(), Host->GetPauseScreen() })
	{
		const FBoolProperty* Claim = HUD ? FindFProperty<FBoolProperty>(HUD->GetClass(), Name) : nullptr;
		if (Claim && Claim->GetPropertyValue_InContainer(HUD))
		{
			return true;
		}
	}
	return false;
}

UClass* UTDUIHostSubsystem::FindNativeWidgetClass()
{
	return FindObject<UClass>(nullptr, TDUIHost::NativeWidgetClassPath);
}

UClass* UTDUIHostSubsystem::ResolveWidgetClass()
{
	UClass* Native = FindNativeWidgetClass();
	if (!Native)
	{
		return nullptr; // plugin not loaded: a Blueprint child of it can't be valid either
	}
	const UTDUIHostSettings* Settings = UTDUIHostSettings::Get();
	for (const FSoftClassPath& Path : { Settings->SpaceHUDClass, Settings->NativeFallbackClass })
	{
		if (Path.IsNull())
		{
			continue;
		}
		UClass* Class = Path.TryLoadClass<UUserWidget>();
		if (Class && Class->IsChildOf(Native) && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			return Class;
		}
	}
	return Native;
}

bool UTDUIHostSubsystem::CallSetDataSource(UUserWidget* HUD, UObject* Source)
{
	UFunction* Function = HUD ? HUD->FindFunction(TEXT("SetDataSource")) : nullptr;
	if (!Function || Function->NumParms != 1)
	{
		return false;
	}
	FObjectPropertyBase* Param = CastField<FObjectPropertyBase>(Function->PropertyLink);
	if (!Param || !Param->HasAnyPropertyFlags(CPF_Parm) || !UObject::StaticClass()->IsChildOf(Param->PropertyClass))
	{
		return false;
	}
	FStructOnScope Params(Function);
	Param->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Source);
	HUD->ProcessEvent(Function, Params.GetStructMemory());
	return true;
}

bool UTDUIHostSubsystem::CallRefreshData(UUserWidget* HUD)
{
	UFunction* Function = HUD ? HUD->FindFunction(TEXT("RefreshData")) : nullptr;
	if (!Function || Function->NumParms != 0)
	{
		return false;
	}
	HUD->ProcessEvent(Function, nullptr);
	return true;
}

bool UTDUIHostSubsystem::ReadSnapshot(const UUserWidget* HUD, TArray<FTDHostShipView>& Out)
{
	Out.Reset();
	const FArrayProperty* Array = HUD ? FindFProperty<FArrayProperty>(HUD->GetClass(), TEXT("Ships")) : nullptr;
	const FStructProperty* Item = Array ? CastField<FStructProperty>(Array->Inner) : nullptr;
	if (!Item)
	{
		return false;
	}
	FScriptArrayHelper Values(Array, Array->ContainerPtrToValuePtr<void>(HUD));
	for (int32 Index = 0; Index < Values.Num(); ++Index)
	{
		const void* V = Values.GetRawPtr(Index);
		const UStruct* S = Item->Struct;
		FTDHostShipView& View = Out.AddDefaulted_GetRef();
		View.Hull = TDUIHost::ReadUIShipNumber(S, V, TEXT("Hull"));
		View.Shield = TDUIHost::ReadUIShipNumber(S, V, TEXT("Shield"));
		View.Power = TDUIHost::ReadUIShipNumber(S, V, TEXT("Power"));
		View.Power2 = TDUIHost::ReadUIShipNumber(S, V, TEXT("Power2"));
		View.Ability = TDUIHost::ReadUIShipNumber(S, V, TEXT("Ability"));
		View.RespawnSeconds = TDUIHost::ReadUIShipNumber(S, V, TEXT("RespawnSeconds"));
		View.Boost = TDUIHost::ReadUIShipNumber(S, V, TEXT("Boost"));
		View.Heat = TDUIHost::ReadUIShipNumber(S, V, TEXT("Heat"));
		View.Ammo = FMath::RoundToInt(TDUIHost::ReadUIShipNumber(S, V, TEXT("Ammo")));
		View.bActive = TDUIHost::ReadUIShipFlag(S, V, TEXT("bActive"));
		View.bAlive = TDUIHost::ReadUIShipFlag(S, V, TEXT("bAlive"));
		View.bUnlocked = TDUIHost::ReadUIShipFlag(S, V, TEXT("bUnlocked"));
		View.bAvailable = TDUIHost::ReadUIShipFlag(S, V, TEXT("bAvailable"));
	}
	return true;
}

// ------------------------------------------------------------------------------------------ dev status

#if !UE_BUILD_SHIPPING
namespace TDUIHost
{
	void LogHostStatus(UWorld* World)
	{
		UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(World);
		if (!Host)
		{
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: no host in this world"));
			return;
		}
		UUserWidget* HUD = Host->GetSpaceHUD();
		// Without the native HUD (dev HUD on), read the squad manager directly.
		UObject* Source = Host->GetDataSource() ? Host->GetDataSource() : static_cast<UObject*>(ATDSquadManager::Get(World));
		APlayerController* ViewPC = HUD && HUD->GetOwningPlayer() ? HUD->GetOwningPlayer() : World->GetFirstPlayerController();
		if (const UTDFlightHUDWidget* Flight = Cast<UTDFlightHUDWidget>(HUD))
		{
			const UTDFlightHUDWidget::FView& V = Flight->GetView();
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: flight HUD coverage=%.2f%% marks=%d wings=%d hits=%d lane=%d hull=%.2f shield=%.2f gamepad=%d waypoints=%d card=%d step=%d/%d"),
				Flight->GetCoverage01() * 100.f, V.Marks.Num(), V.Wings.Num(), V.Hits.Num(), static_cast<int32>(V.Lane), V.Hull, V.Shield, V.bUsingGamepad ? 1 : 0,
				V.Waypoints.Num(), V.bHintCard ? 1 : 0, V.HintStep, V.HintCount);
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: flight HUD %s inviewport=%d opacity=%.2f subtitle=\"%s\" age=%.2f/%.2f story=%d"),
				*Flight->GetName(), Flight->IsInViewport() ? 1 : 0, V.HUDOpacity, *V.Subtitle.Text, V.Subtitle.Age, V.Subtitle.Seconds,
				V.Subtitle.bStory ? 1 : 0);
			if (const ITDHUDData* Src = Cast<ITDHUDData>(Flight->GetDataSource()))
			{
				for (const AActor* Marker : Src->GetObjectiveMarkers())
				{
					if (Marker)
					{
						UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: waypoint %s at (%.0f, %.0f, %.0f) m"), *Marker->GetName(),
							Marker->GetActorLocation().X / 100.f, Marker->GetActorLocation().Y / 100.f, Marker->GetActorLocation().Z / 100.f);
					}
				}
			}
		}
		UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: choice=%s enabled=%d shown=%d class=%s source=%s"),
			*StaticEnum<ETDSpaceHUDChoice>()->GetNameStringByValue(static_cast<int64>(UTDUIHostSettings::GetSpaceHUDChoice())),
			UTDUIHostSettings::IsNativeSpaceHUDEnabled() ? 1 : 0, HUD ? 1 : 0,
			HUD ? *HUD->GetClass()->GetName() : TEXT("-"), Source ? *Source->GetName() : TEXT("-"));
		if (const ITDHUDData* Data = Cast<ITDHUDData>(Source))
		{
			AActor* Target = Data->GetCurrentTarget();
			FVector2D Screen = FVector2D::ZeroVector;
			const bool bOnScreen = Target && ViewPC && ViewPC->ProjectWorldLocationToScreen(Target->GetActorLocation(), Screen, true);
			int32 ViewW = 0, ViewH = 0;
			if (ViewPC)
			{
				ViewPC->GetViewportSize(ViewW, ViewH);
			}
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: viewport=%dx%d"), ViewW, ViewH);
			if (Target && ViewPC && ViewPC->GetPawn())
			{
				const FVector ToTarget = Target->GetActorLocation() - ViewPC->GetPawn()->GetActorLocation();
				UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: target wyaw=%.1f wpitch=%.1f dist=%.0f"),
					FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X)), FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Z, ToTarget.Size2D())), ToTarget.Size());
			}
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: target=%s onscreen=%d at (%.0f, %.0f), locked=%d, markers=%d"),
				Target ? *Target->GetName() : TEXT("none"), bOnScreen ? 1 : 0, Screen.X, Screen.Y,
				Data->GetLockedTargets().Num(), Data->GetTargetMarkers().Num());
			// Nearest hostile ship (markers come nearest first), projected now: lets an agent pilot aim.
			for (const FTDTargetMarker& M : Data->GetTargetMarkers())
			{
				AActor* Hostile = M.Actor.Get();
				if (!Hostile || M.bIncomingMissile)
				{
					continue;
				}
				FVector2D At = FVector2D::ZeroVector;
				const bool bOn = ViewPC && ViewPC->ProjectWorldLocationToScreen(Hostile->GetActorLocation(), At, true);
				FVector ViewLoc; FRotator ViewRot;
				if (ViewPC) { ViewPC->GetPlayerViewPoint(ViewLoc, ViewRot); }
				const FVector Local = ViewRot.UnrotateVector(Hostile->GetActorLocation() - ViewLoc);
				// World bearing from the player's ship (for TD.Input.TurnToward <YawDeg> <PitchDeg>).
				const APawn* Ship = ViewPC ? ViewPC->GetPawn() : nullptr;
				const FVector ToHostile = Hostile->GetActorLocation() - (Ship ? Ship->GetActorLocation() : ViewLoc);
				UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: nearest=%s dist=%.0f onscreen=%d at (%.0f, %.0f) yaw=%.1f pitch=%.1f wyaw=%.1f wpitch=%.1f"),
					*Hostile->GetName(), M.Distance, bOn ? 1 : 0, At.X, At.Y,
					FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X)), FMath::RadiansToDegrees(FMath::Atan2(Local.Z, Local.Size2D())),
					FMath::RadiansToDegrees(FMath::Atan2(ToHostile.Y, ToHostile.X)), FMath::RadiansToDegrees(FMath::Atan2(ToHostile.Z, ToHostile.Size2D())));
				break;
			}
			const FTDWaveInfo Wave = Data->GetWaveInfo();
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: wave=%d alive=%d"), Wave.WaveNumber, Wave.EnemiesAlive);
			const TArray<FTDDamageIndicator> Hits = Data->GetDamageIndicators();
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: hits=%d newest=%.2fs"), Hits.Num(),
				Hits.Num() > 0 ? Hits[0].AgeSeconds : -1.f);
			if (!HUD)
			{
				const TArray<FTDShipHUD> Cards = Data->GetSquadHUD();
				for (int32 I = 0; I < Cards.Num(); ++I)
				{
					const FTDShipHUD& C = Cards[I];
					UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: card %d active=%d alive=%d unlocked=%d hull=%.2f shield=%.2f power=%.2f ability=%.2f"),
						I, C.bPlayerControlled, C.bAlive, C.Ability != ETDAbilityId::None, C.Health01, C.Shield01, C.PowerCooldown01, C.AbilityCooldown01);
				}
			}
		}
		TArray<FTDHostShipView> Cards;
		if (UTDUIHostSubsystem::ReadSnapshot(HUD, Cards))
		{
			// What the widget decoded vs what the data source says now: a MISMATCH line names every field that differs.
			const ITDHUDData* Src = Cast<ITDHUDData>(Source);
			const TArray<FTDShipHUD> Truth = Src ? Src->GetSquadHUD() : TArray<FTDShipHUD>();
			for (int32 I = 0; I < Cards.Num(); ++I)
			{
				const FTDHostShipView& C = Cards[I];
				UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: card %d active=%d alive=%d unlocked=%d hull=%.2f shield=%.2f power=%.2f ability=%.2f boost=%.2f heat=%.2f ammo=%d"),
					I, C.bActive, C.bAlive, C.bUnlocked, C.Hull, C.Shield, C.Power, C.Ability, C.Boost, C.Heat, C.Ammo);
				const FTDShipHUD* T = Truth.FindByPredicate([I](const FTDShipHUD& H) { return static_cast<int32>(H.Ship) == I; });
				if (!T)
				{
					continue;
				}
				FString Diff;
				auto Num = [&Diff](const TCHAR* Name, float Shown, float Real, float Tol)
				{
					if (FMath::Abs(Shown - Real) > Tol)
					{
						Diff += FString::Printf(TEXT(" %s %.2f vs %.2f"), Name, Shown, Real);
					}
				};
				// The widget samples every 0.1 s and eases meters: allow a little slack on continuous values.
				Num(TEXT("hull"), C.Hull, T->bAlive ? T->Health01 : 0.f, 0.06f);
				Num(TEXT("shield"), C.Shield, T->bAlive ? T->Shield01 : 0.f, 0.08f);
				Num(TEXT("power"), C.Power, T->PowerCooldown01, 0.06f);
				Num(TEXT("ability"), C.Ability, T->AbilityCooldown01, 0.06f);
				Num(TEXT("heat"), C.Heat, T->Heat01, 0.08f);
				Num(TEXT("ammo"), (float)C.Ammo, (float)T->SecondaryAmmo, 0.5f);
				if (C.bActive != T->bPlayerControlled) { Diff += TEXT(" active"); }
				if (C.bAlive != T->bAlive) { Diff += TEXT(" alive"); }
				if (C.bUnlocked != (T->Ability != ETDAbilityId::None)) { Diff += TEXT(" unlocked"); }
				if (!Diff.IsEmpty())
				{
					UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: MISMATCH card %d:%s"), I, *Diff);
				}
			}
		}
	}

	void SetCoverOverride(const TArray<FString>& Args)
	{
		const UEnum* Enum = StaticEnum<ETDHUDSurface>();
		const int64 Value = Args.Num() > 0 ? Enum->GetValueByNameString(Args[0]) : INDEX_NONE;
		if (Value == INDEX_NONE || Args.Num() < 2)
		{
			UE_LOG(LogTDUIHost, Warning, TEXT("TD.UI.Covers <DamageIndicators|SquadMarkers|InteractPrompt|HyperdriveBlockText|Altitude|Subtitles|Pause|BoundaryWarning|CombatStates> <-1|0|1>"));
			return;
		}
		CoverOverrides().Add(static_cast<ETDHUDSurface>(Value), FCString::Atoi(*Args[1]));
		UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Covers: %s = %s"), *Args[0], *Args[1]);
	}

	FAutoConsoleCommand CmdTDUICovers(TEXT("TD.UI.Covers"),
		TEXT("TD.UI.Covers <Surface> <-1|0|1>: dev override of which HUD surfaces the native space HUD covers (-1 = ask the widget)."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SetCoverOverride));

	/** TD.UI.DevHurt <amount> [fromYawDeg]: real damage to the player's ship from a side (HUD captures: hull bands, hit arcs). */
	void DevHurt(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || Args.Num() < 1)
		{
			UE_LOG(LogTDUIHost, Warning, TEXT("TD.UI.DevHurt <amount> [fromYawDeg, relative to the ship: 0 = ahead, 90 = right]"));
			return;
		}
		const float Amount = FCString::Atof(*Args[0]);
		const float Yaw = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f;
		const FVector From = Pawn->GetActorRotation().RotateVector(FRotator(0.f, Yaw, 0.f).Vector());
		const FVector HitAt = Pawn->GetActorLocation() + From * 300.f;
		UTDStatics::ApplyTDDamage(Pawn, Amount, ETDDamageKind::Energy, nullptr, nullptr, HitAt, -From, false);
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Pawn);
		UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.DevHurt: %.0f from %.0f deg -> hull %.2f shield %.2f"), Amount, Yaw,
			Health ? Health->GetHull01() : -1.f, Health ? Health->GetShield01() : -1.f);
	}

	FAutoConsoleCommandWithWorldAndArgs CmdTDUIDevHurt(TEXT("TD.UI.DevHurt"),
		TEXT("TD.UI.DevHurt <amount> [fromYawDeg]: damage the player's ship from a side (dev, HUD captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DevHurt));

	// TD.UI.Pause down | up | activate | back: drive the pause menu directly (an offscreen game's menu has no Slate focus,
	// so TD.UI.Key Down / Enter never reach it). "TD.UI.Pause settings" = select SETTINGS and activate it.
	FAutoConsoleCommandWithWorldAndArgs CmdTDUIPause(TEXT("TD.UI.Pause"),
		TEXT("TD.UI.Pause <down|up|activate|back|settings>: drive the pause menu (captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(World);
			UTDPauseMenuWidget* Menu = Host ? Cast<UTDPauseMenuWidget>(Host->GetPauseScreen()) : nullptr;
			if (!Menu || Args.Num() < 1)
			{
				UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Pause: %s"), Menu ? TEXT("down|up|activate|back|settings") : TEXT("no pause menu on screen"));
				return;
			}
			const FString& A = Args[0];
			if (A.Equals(TEXT("down"), ESearchCase::IgnoreCase)) { Menu->MoveSelection(1); }
			else if (A.Equals(TEXT("up"), ESearchCase::IgnoreCase)) { Menu->MoveSelection(-1); }
			else if (A.Equals(TEXT("activate"), ESearchCase::IgnoreCase)) { Menu->Activate(); }
			else if (A.Equals(TEXT("back"), ESearchCase::IgnoreCase)) { Menu->Back(); }
			else if (A.Equals(TEXT("settings"), ESearchCase::IgnoreCase))
			{
				Menu->MoveSelection(static_cast<int32>(ETDPauseItem::Settings) - Menu->GetSelected());
				Menu->Activate();
			}
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Pause %s: selected %d"), *A, Menu->GetSelected());
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdTDUITactical(TEXT("TD.UI.Tactical"),
		TEXT("TD.UI.Tactical [seconds=20]: show the Aftermath tactical display (dev, captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDUIHostSubsystem::ShowTacticalDisplay(World, Args.Num() > 0 ? FCString::Atof(*Args[0]) : 20.f);
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdTDUICaption(TEXT("TD.UI.Caption"),
		TEXT("TD.UI.Caption <seconds> <text...>: show any line as a Command caption through the story's own subtitle path (dev: layout captures with long lines)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() < 2)
			{
				return;
			}
			TArray<FString> Rest(Args.GetData() + 1, Args.Num() - 1);
			UTDStorySubsystem::ShowBarkCaption(World, TEXT("Command"), FString::Join(Rest, TEXT(" ")), FCString::Atof(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdTDUIScan(TEXT("TD.UI.Scan"),
		TEXT("TD.UI.Scan <stage 1..5> [seconds=0 until off] | TD.UI.Scan off: the Cinematic 03 / 05 scan monitor (dev, captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0 || Args[0] == TEXT("off"))
			{
				UTDUIHostSubsystem::HideScanMonitor(World);
				return;
			}
			UTDUIHostSubsystem::ShowScanMonitor(World, FCString::Atoi(*Args[0]), Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f);
		}));

	FAutoConsoleCommandWithWorld CmdTDUIGroundStatus(TEXT("TD.UI.GroundStatus"),
		TEXT("TD.UI.GroundStatus: log the on-foot HUD's boss and transfer state next to a direct GetActiveBoss call."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			const UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(World);
			const UTDGroundHUDWidget* Ground = Host ? Cast<UTDGroundHUDWidget>(Host->GetGroundHUD()) : nullptr;
			float Hull = -1.f;
			int32 Phase = -1;
			FText Name;
			const bool bBoss = ATDCrewCharacter::GetActiveBoss(World, Hull, Phase, Name);
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.GroundStatus: widget %s boss=%d hull=%.2f phase=%d ghost=%.2f | direct boss=%d hull=%.2f phase=%d"),
				*GetNameSafe(Ground), Ground && Ground->GetView().bBoss ? 1 : 0, Ground ? Ground->GetView().Boss01 : -1.f,
				Ground ? Ground->GetView().BossPhase : -1, Ground ? Ground->GetBossGhost01() : -1.f, bBoss ? 1 : 0, Hull, Phase);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdTDUIReward(TEXT("TD.UI.Reward"),
		TEXT("TD.UI.Reward <GravityWell|Cloak|EMPPulse> [Rift|Lancer|Bastion] | dismiss: STAGED - an award on the reward screen only (the bus's ")
		TEXT("OnAbilityUnlocked; no progression), or dismiss the one on screen. The real path is the objective's UnlockAbility."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(World);
			if (Args.Num() > 0 && Args[0] == TEXT("dismiss"))
			{
				if (Host && Host->GetRewardWidget())
				{
					if (UFunction* F = Host->GetRewardWidget()->FindFunction(TEXT("RequestDismiss"))) { Host->GetRewardWidget()->ProcessEvent(F, nullptr); }
				}
				return;
			}
			const int64 A = Args.Num() > 0 ? StaticEnum<ETDAbilityId>()->GetValueByNameString(Args[0]) : INDEX_NONE;
			const int64 S = Args.Num() > 1 ? StaticEnum<ETDShipId>()->GetValueByNameString(Args[1]) : static_cast<int64>(ETDShipId::Bastion);
			UTDEventBus* Bus = UTDEventBus::Get(World);
			if (A == INDEX_NONE || S == INDEX_NONE || !Bus)
			{
				UE_LOG(LogTDUIHost, Warning, TEXT("TD.UI.Reward <GravityWell|Cloak|EMPPulse> [Rift|Lancer|Bastion] | dismiss"));
				return;
			}
			Bus->BroadcastAbilityUnlocked(static_cast<ETDAbilityId>(A), static_cast<ETDShipId>(S));
			if (Host) { Host->UpdateAbilityReward(); }
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Reward: STAGED award %s for %s."), *Args[0], Args.Num() > 1 ? *Args[1] : TEXT("Bastion"));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdTDUIScanTexture(TEXT("TD.UI.ScanTexture"),
		TEXT("TD.UI.ScanTexture <texture object path> | off: a stand-in for both scan graphics (dev; DA_UI_Style's ScanVerdance / ScanCinder otherwise)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDScanMonitorWidget::SetTextureOverride(Args.Num() > 0 && Args[0] != TEXT("off")
				? LoadObject<UTexture2D>(nullptr, *Args[0], nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdTDUIScanOnNearest(TEXT("TD.UI.ScanOnNearest"),
		TEXT("TD.UI.ScanOnNearest [slot name|0] [cinder 0|1]: put the scan on the nearest mesh (not the player) within 8 m (dev, the issue #62 proof)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn)
			{
				return;
			}
			UMeshComponent* Best = nullptr;
			float BestDist = 800.f;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (*It == Pawn || It->IsA<APawn>())
				{
					continue;
				}
				TInlineComponentArray<UMeshComponent*> Meshes(*It);
				for (UMeshComponent* M : Meshes)
				{
					const float D = M ? FVector::Dist(M->GetComponentLocation(), Pawn->GetActorLocation()) : 1e9f;
					if (M && M->IsVisible() && D < BestDist)
					{
						Best = M;
						BestDist = D;
					}
				}
			}
			const FName Slot = Args.Num() > 0 && Args[0] != TEXT("0") ? FName(*Args[0]) : NAME_None;
			const bool bOk = UTDUIHostSubsystem::ShowScanOnScreen(Best, Slot, Args.Num() > 1 && Args[1] == TEXT("1"));
			UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.ScanOnNearest: %s on %s (%.0f cm)"), bOk ? TEXT("shown") : TEXT("not shown"), *GetPathNameSafe(Best), BestDist);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdTDUIFocus(TEXT("TD.UI.Focus"),
		TEXT("TD.UI.Focus <opacity 0..1> [hideCombat 0|1] | TD.UI.Focus off: the 'Dev' focus hold on the flight HUD (as T1's entry)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0 || Args[0] == TEXT("off"))
			{
				UTDUIHostSubsystem::PopHUDFocus(World, TEXT("Dev"));
				return;
			}
			UTDUIHostSubsystem::PushHUDFocus(World, TEXT("Dev"), FCString::Atof(*Args[0]), Args.Num() > 1 && FCString::Atoi(*Args[1]) != 0);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdTDUIBark(TEXT("TD.UI.Bark"),
		TEXT("TD.UI.Bark <BarkId> <Speaker>: broadcast a bark request on the event bus, as the squad does (dev, captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UTDEventBus* Bus = Args.Num() >= 2 ? UTDEventBus::Get(World) : nullptr)
			{
				Bus->BroadcastBarkRequested(FName(*Args[0]), FName(*Args[1]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdTDUINavPulse(TEXT("TD.UI.NavPulse"),
		TEXT("TD.UI.NavPulse [seconds=1.5]: the Aftermath navigation pulse, a corridor from 1 km to 9 km ahead and to the right (dev)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn)
			{
				return;
			}
			const FVector Fwd = Pawn->GetActorForwardVector(), Right = Pawn->GetActorRightVector();
			const FVector From = Pawn->GetActorLocation() + Fwd * 100000.f - Pawn->GetActorUpVector() * 8000.f;
			const FVector To = Pawn->GetActorLocation() + Fwd * 900000.f + Right * 250000.f;
			UTDUIHostSubsystem::ShowNavigationPulse(World, From, To, Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.5f);
		}));

	FAutoConsoleCommandWithWorld CmdTDUIStatus(TEXT("TD.UI.Status"),
		TEXT("TD.UI.Status: log the space HUD host state, its data source's current target and the widget's decoded squad cards."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&LogHostStatus));
}
#endif
