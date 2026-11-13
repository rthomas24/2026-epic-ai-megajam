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
#include "TriadDrift/UI/TDPauseMenuWidget.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"

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

void UTDUIHostSubsystem::Deinitialize()
{
	Hide();
	HideGround();
	HidePause();
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
	if (const UTDTacticalDisplayWidget* Display = Cast<UTDTacticalDisplayWidget>(TacticalWidget); Display && Display->IsFinished())
	{
		TacticalWidget->RemoveFromParent();
		TacticalWidget = nullptr;
	}
	TimeUntilCheck -= DeltaTime;
	if (TimeUntilCheck > 0.f)
	{
		return;
	}
	TimeUntilCheck = TDUIHost::CheckInterval;
	Evaluate();
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
	Display->Play(Seconds);
	return true;
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

	FAutoConsoleCommandWithWorldAndArgs CmdTDUITactical(TEXT("TD.UI.Tactical"),
		TEXT("TD.UI.Tactical [seconds=20]: show the Aftermath tactical display (dev, captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDUIHostSubsystem::ShowTacticalDisplay(World, Args.Num() > 0 ? FCString::Atof(*Args[0]) : 20.f);
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
