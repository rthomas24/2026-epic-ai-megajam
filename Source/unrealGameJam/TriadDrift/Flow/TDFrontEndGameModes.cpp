// Triad Drift - game modes for the non-gameplay maps. Owner: Teammate 5.

#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "Blueprint/UserWidget.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Misc/PackageName.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UTDUIStyle* FTDFrontEndWidgets::LoadDefaultStyle()
{
	static const TCHAR* Package = TEXT("/Game/TriadDrift/Presentation/DA_UI_Style");
	return FPackageName::DoesPackageExist(Package)
		? LoadObject<UTDUIStyle>(nullptr, TEXT("/Game/TriadDrift/Presentation/DA_UI_Style.DA_UI_Style"))
		: nullptr;
}

UClass* FTDFrontEndWidgets::Resolve(UClass* Override, const UTDUIStyle* Style, EKind Kind)
{
	if (Override)
	{
		return Override;
	}
	if (!Style)
	{
		return nullptr;
	}
	const TSoftClassPtr<UUserWidget>& Soft = Kind == EKind::MainMenu ? Style->MainMenuWidgetClass : Style->LoadingWidgetClass;
	return Soft.IsNull() ? nullptr : Soft.LoadSynchronous();
}

namespace TDFrontEnd
{
	static void ShowWidget(UWorld* World, UClass* Class, bool bCursor)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			return;
		}
		PC->SetShowMouseCursor(bCursor);
		if (bCursor)
		{
			PC->SetInputMode(FInputModeUIOnly());
		}
		if (Class)
		{
			if (UUserWidget* W = CreateWidget<UUserWidget>(PC, TSubclassOf<UUserWidget>(Class)))
			{
				W->AddToViewport();
			}
		}
	}
}

ATDMenuGameMode::ATDMenuGameMode()
{
	DefaultPawnClass = nullptr;
}

void ATDMenuGameMode::StartPlay()
{
	Super::StartPlay();
	if (UTDGameInstance* GI = UTDGameInstance::Get(this))
	{
		GI->NotifyLevelEntered(ETDDestination::MainMenu); // no checkpoint, clock stopped
	}
	TDFrontEnd::ShowWidget(GetWorld(), FTDFrontEndWidgets::Resolve(MenuWidgetClass.Get(), MenuWidgetClass ? nullptr : FTDFrontEndWidgets::LoadDefaultStyle(), FTDFrontEndWidgets::EKind::MainMenu), true);
}

void ATDMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UTDCheatManager::InstallOn(NewPlayer);
}

ATDTransitionGameMode::ATDTransitionGameMode()
{
	DefaultPawnClass = nullptr;
}

void ATDTransitionGameMode::StartPlay()
{
	Super::StartPlay();
	TDFrontEnd::ShowWidget(GetWorld(), FTDFrontEndWidgets::Resolve(LoadingWidgetClass.Get(), LoadingWidgetClass ? nullptr : FTDFrontEndWidgets::LoadDefaultStyle(), FTDFrontEndWidgets::EKind::Loading), false);
}
