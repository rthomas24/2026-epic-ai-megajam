// Triad Drift - game modes for the non-gameplay maps. Owner: Teammate 5.

#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "Blueprint/UserWidget.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

UTDUIStyle* FTDFrontEndWidgets::LoadDefaultStyle()
{
	return UTDStatics::LoadPresentation<UTDUIStyle>(TEXT("DA_UI_Style"));
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
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	// Arriving from a won run: the victory screen first (the menu after "Return").
	bShowingVictory = GI && GI->GetProgressRef().Phase == ETDGamePhase::Victory;
	if (GI)
	{
		GI->NotifyLevelEntered(bShowingVictory ? ETDDestination::Victory : ETDDestination::MainMenu); // no checkpoint, clock stopped
	}
	StartMusic();
	UClass* Widget = FTDFrontEndWidgets::Resolve(MenuWidgetClass.Get(), MenuWidgetClass ? nullptr : FTDFrontEndWidgets::LoadDefaultStyle(), FTDFrontEndWidgets::EKind::MainMenu);
	TDFrontEnd::ShowWidget(GetWorld(), Widget, true);
	if (!Widget)
	{
		ShowPlaceholder(); // no menu widget assigned yet: a plain working menu, so the game is always playable
	}
}

void ATDMenuGameMode::StartMusic()
{
	// DA_UI_Style.MenuMusic (N-015): Nick's asset carries its own volume and class; no overrides here.
	const UTDUIStyle* Style = StyleOverride ? StyleOverride.Get() : FTDFrontEndWidgets::LoadDefaultStyle();
	const FSoftObjectProperty* Prop = Style ? CastField<FSoftObjectProperty>(UTDUIStyle::StaticClass()->FindPropertyByName(TEXT("MenuMusic"))) : nullptr;
	if (!Prop)
	{
		return;
	}
	const FSoftObjectPtr& Soft = *Prop->ContainerPtrToValuePtr<FSoftObjectPtr>(Style);
	USoundBase* Sound = Cast<USoundBase>(Soft.LoadSynchronous());
	if (!Sound)
	{
		return; // no music assigned: silence, nothing created
	}
	// A 2D UI sound on the menu mode itself: it goes when the map goes (no persistence across travel).
	Music = NewObject<UAudioComponent>(this, TEXT("MenuMusic"));
	Music->SetSound(Sound);
	Music->bAllowSpatialization = false;
	Music->bIsUISound = true;
	Music->bAutoDestroy = false;
	Music->RegisterComponentWithWorld(GetWorld());
	Music->Play();
	bMusicStarted = true;
}

void ATDMenuGameMode::FadeOutMusic()
{
	if (Music)
	{
		Music->FadeOut(1.f, 0.f); // a short fade as the travel starts
		bMusicFading = true;
	}
}

void ATDMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Music)
	{
		Music->Stop();
	}
	HidePlaceholder();
	Super::EndPlay(EndPlayReason);
}

bool ATDMenuGameMode::NewGame()
{
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (!GI)
	{
		return false;
	}
	GI->StartNewRun();
	FadeOutMusic();
	return GI->TravelTo(ETDDestination::Galaxy, NAME_None);
}

bool ATDMenuGameMode::ContinueGame()
{
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (!GI || !GI->HasContinue())
	{
		return false;
	}
	FadeOutMusic();
	return GI->ContinueRun();
}

bool ATDMenuGameMode::CanContinue() const
{
	const UTDGameInstance* GI = UTDGameInstance::Get(this);
	return GI && GI->HasContinue();
}

void ATDMenuGameMode::QuitGame()
{
	bQuitRequested = true;
	UKismetSystemLibrary::QuitGame(this, GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr, EQuitPreference::Quit, false);
}

void ATDMenuGameMode::ReturnToMenu()
{
	bShowingVictory = false;
	if (Placeholder.IsValid())
	{
		HidePlaceholder();
		ShowPlaceholder();
	}
}

void ATDMenuGameMode::HidePlaceholder()
{
	if (Placeholder.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Placeholder.ToSharedRef());
	}
	Placeholder.Reset();
}

void ATDMenuGameMode::ShowPlaceholder()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return; // headless (tests): the actions above are the menu
	}
	TWeakObjectPtr<ATDMenuGameMode> Weak(this);
	auto Button = [Weak](const FString& Label, TFunction<void(ATDMenuGameMode*)> Action, bool bEnabled) -> TSharedRef<SWidget>
	{
		return SNew(SBox).WidthOverride(320.f).Padding(FMargin(0.f, 6.f))
			[
				SNew(SButton).IsEnabled(bEnabled).HAlign(HAlign_Center)
				.OnClicked_Lambda([Weak, Action]() { if (ATDMenuGameMode* M = Weak.Get()) { Action(M); } return FReply::Handled(); })
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
			];
	};
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	const UTDGameInstance* GI = UTDGameInstance::Get(this);
	if (bShowingVictory && GI)
	{
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TDMenu", "Wrap", "THAT'S A WRAP")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 54)).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.4f))
		];
		const FTDProgress& P = GI->GetProgressRef();
		const int32 Seconds = FMath::RoundToInt(P.RunTimeSeconds);
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f)
		[
			SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("RUN TIME %d:%02d   KILLS %d   CUTS %d"), Seconds / 60, Seconds % 60, P.Stats.Kills, P.Stats.Deaths)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
		];
		for (const FTDSceneTakes& Row : GI->GetSceneTakes())
		{
			Box->AddSlot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("SCENE %d  %s  -  %d %s"), Row.SceneNumber, *Row.SceneId.ToString(), Row.Takes, Row.Takes == 1 ? TEXT("TAKE") : TEXT("TAKES"))))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).ColorAndOpacity(FLinearColor(0.8f, 0.85f, 0.95f))
			];
		}
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 18.f, 0.f, 0.f)[Button(TEXT("RETURN TO MENU"), [](ATDMenuGameMode* M) { M->ReturnToMenu(); }, true)];
	}
	else
	{
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 24.f)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TDMenu", "Title", "TRIAD DRIFT")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 64)).ColorAndOpacity(FLinearColor(0.4f, 0.9f, 1.f))
		];
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("NEW GAME"), [](ATDMenuGameMode* M) { M->NewGame(); }, true)];
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("CONTINUE"), [](ATDMenuGameMode* M) { M->ContinueGame(); }, CanContinue())];
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("QUIT"), [](ATDMenuGameMode* M) { M->QuitGame(); }, true)];
	}
	Placeholder = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.03f, 0.92f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			Box
		];
	GEngine->GameViewport->AddViewportWidgetContent(Placeholder.ToSharedRef(), 10);
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
