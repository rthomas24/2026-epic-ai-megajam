// Triad Drift - game modes for the non-gameplay maps. Owner: Teammate 5.

#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace TDFrontEnd
{
	static void ShowWidget(UWorld* World, TSubclassOf<UUserWidget> Class, bool bCursor)
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
			if (UUserWidget* W = CreateWidget<UUserWidget>(PC, Class))
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
	TDFrontEnd::ShowWidget(GetWorld(), MenuWidgetClass, true);
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
	TDFrontEnd::ShowWidget(GetWorld(), LoadingWidgetClass, false);
}
