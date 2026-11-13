// Triad Drift - engineering host for Nick's native space HUD (Teammate 2).

#include "TriadDrift/UI/TDUIHostSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/UI/TDUIHostSettings.h"

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
	Super::Deinitialize();
}

TStatId UTDUIHostSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDUIHostSubsystem, STATGROUP_Tickables);
}

void UTDUIHostSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	if (!UTDUIHostSettings::IsNativeSpaceHUDEnabled())
	{
		return false;
	}
	ATDSquadManager* Squad = ATDSquadManager::Get(this);
	APlayerController* PC = FindLocalController();
	// On foot (planets, the Dominion interior) the pawn is the first-person character: the widget has
	// no on-foot half yet, so the dev HUD stays there.
	if (!Squad || !PC || !Cast<ATDShip>(PC->GetPawn()))
	{
		return false;
	}
	OutController = PC;
	OutSource = Squad;
	return true;
}

void UTDUIHostSubsystem::Evaluate()
{
	APlayerController* PC = nullptr;
	UObject* Source = nullptr;
	if (!WantsSpaceHUD(PC, Source))
	{
		Hide();
		return;
	}
	if (Widget && Owner.Get() == PC)
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
	UClass* Class = ResolveWidgetClass();
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
	const UWorld* World = GetWorld();
	if (bAddToViewport && World && World->GetGameViewport())
	{
		Widget->AddToViewport(UTDUIHostSettings::Get()->ZOrder);
		// SetDataSource ran before NativeConstruct bound the event bus; sample again now it's live.
		CallRefreshData(Widget);
	}
	SetDevHUDSuppressed(true);
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
	SetDevHUDSuppressed(false);
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
		UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: enabled=%d shown=%d class=%s source=%s"),
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
				UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: nearest=%s dist=%.0f onscreen=%d at (%.0f, %.0f) yaw=%.1f pitch=%.1f"),
					*Hostile->GetName(), M.Distance, bOn ? 1 : 0, At.X, At.Y,
					FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X)), FMath::RadiansToDegrees(FMath::Atan2(Local.Z, Local.Size2D())));
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
			for (int32 I = 0; I < Cards.Num(); ++I)
			{
				const FTDHostShipView& C = Cards[I];
				UE_LOG(LogTDUIHost, Display, TEXT("TD.UI.Status: card %d active=%d alive=%d unlocked=%d hull=%.2f shield=%.2f power=%.2f ability=%.2f"),
					I, C.bActive, C.bAlive, C.bUnlocked, C.Hull, C.Shield, C.Power, C.Ability);
			}
		}
	}

	FAutoConsoleCommandWithWorld CmdTDUIStatus(TEXT("TD.UI.Status"),
		TEXT("TD.UI.Status: log the space HUD host state, its data source's current target and the widget's decoded squad cards."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&LogHostStatus));
}
#endif
