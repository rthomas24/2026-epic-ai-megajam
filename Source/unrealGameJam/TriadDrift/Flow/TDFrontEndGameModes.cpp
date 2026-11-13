// Triad Drift - game modes for the non-gameplay maps. Owner: Teammate 5.

#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
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
#include "TriadDrift/Flow/TDFrontEndStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"

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
	static UUserWidget* ShowWidget(UWorld* World, UClass* Class, bool bCursor)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			return nullptr;
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
				return W;
			}
		}
		return nullptr;
	}
}

ATDMenuGameMode::ATDMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = ATDFrontEndHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ATDMenuGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	KeepMenuFocus();
}

void ATDMenuGameMode::KeepMenuFocus()
{
	if (UTDSettingsSubsystem::IsSettingsPageOpen(this))
	{
		return; // the Settings page owns focus while it is up
	}
	const TSharedPtr<SWidget> Button = FocusButton.Pin();
	if (!Placeholder.IsValid() || !Button.IsValid() || !FSlateApplication::IsInitialized())
	{
		return;
	}
	FSlateApplication& App = FSlateApplication::Get();
	const TSharedPtr<SWidget> Focused = App.GetUserFocusedWidget(0);
	for (TSharedPtr<SWidget> W = Focused; W.IsValid(); W = W->GetParentWidget())
	{
		if (W == Placeholder)
		{
			LastMenuFocus = Focused; // remembered: closing Settings comes back here (T1's pad pass)
			return; // already on one of the menu's buttons (the d-pad moves between them)
		}
	}
	App.SetAllUserFocus(ChooseRefocus(LastMenuFocus.Pin(), Button), EFocusCause::SetDirectly);
}

TSharedPtr<SWidget> ATDMenuGameMode::ChooseRefocus(const TSharedPtr<SWidget>& LastFocused, const TSharedPtr<SWidget>& First)
{
	return LastFocused.IsValid() && LastFocused->IsEnabled() && LastFocused->SupportsKeyboardFocus() ? LastFocused : First;
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
	// T3: the scene behind the menu or the victory screen (sky, Verdance, the squad; after a win the Dominion breaking
	// up); off if the map has its own scene.
	ATDMenuBackdrop::SpawnFor(this, bShowingVictory ? ETDBackdropScene::Victory : ETDBackdropScene::Menu);
	ShowMenuUI();
}

void ATDMenuGameMode::ShowMenuUI()
{
	// Whatever is up goes first (a return to the menu from the victory screen comes through here too, N-033).
	if (ShownWidget)
	{
		ShownWidget->RemoveFromParent();
		ShownWidget = nullptr;
	}
	HidePlaceholder();
	const UTDUIStyle* MenuStyle = StyleOverride ? StyleOverride.Get() : FTDFrontEndWidgets::LoadDefaultStyle();
	UClass* Widget = nullptr;
	if (bShowingVictory)
	{
		// N-023: a main-menu widget is never asked to show the victory statistics. A styled victory widget if one
		// is assigned, otherwise the code-built victory screen (the placeholder path).
		Widget = (MenuStyle && !MenuStyle->VictoryWidgetClass.IsNull()) ? MenuStyle->VictoryWidgetClass.LoadSynchronous() : nullptr;
	}
	else
	{
		Widget = FTDFrontEndWidgets::Resolve(MenuWidgetClass.Get(), MenuWidgetClass ? nullptr : MenuStyle, FTDFrontEndWidgets::EKind::MainMenu);
	}
	bPlaceholderWanted = !Widget;
	ShownWidgetClass = Widget;
	ShownWidget = TDFrontEnd::ShowWidget(GetWorld(), Widget, true);
	if (!Widget)
	{
		ShowPlaceholder(); // no menu widget assigned yet: a plain working menu, so the game is always playable
	}
}

void ATDMenuGameMode::StartMusic()
{
	// DA_UI_Style.MenuMusic (N-015): Nick's asset carries its own volume and class; no overrides here.
	const UTDUIStyle* Style = StyleOverride ? StyleOverride.Get() : FTDFrontEndWidgets::LoadDefaultStyle();
	USoundBase* Sound = (Style && !Style->MenuMusic.IsNull()) ? Style->MenuMusic.LoadSynchronous() : nullptr;
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
	// Nick's track is not a looping asset (MUS_Menu_EtherialPulse_01): loop it here, without touching his asset.
	Music->OnAudioFinished.AddUniqueDynamic(this, &ATDMenuGameMode::HandleMusicFinished);
	Music->Play();
	bMusicStarted = true;
}

void ATDMenuGameMode::HandleMusicFinished()
{
	if (Music && !bMusicFading && !IsActorBeingDestroyed())
	{
		++MusicLoops;
		Music->Play(0.f);
	}
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
	// Re-resolve, as at start-up: Nick's main-menu widget comes back after the victory screen (N-033), or the
	// code-built menu when none is assigned.
	ShowMenuUI();
}

void ATDMenuGameMode::HidePlaceholder()
{
	LastMenuFocus.Reset(); // a new screen starts on its first button
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
	TSharedPtr<SButton> FirstButton; // keyboard / gamepad focus starts here (Enter or pad A presses it)
	auto Button = [Weak, &FirstButton](const FString& Label, TFunction<void(ATDMenuGameMode*)> Action, bool bEnabled) -> TSharedRef<SWidget>
	{
		TSharedPtr<SButton> Made;
		SAssignNew(Made, SButton).IsEnabled(bEnabled).HAlign(HAlign_Center)
			.OnClicked_Lambda([Weak, Action]() { if (ATDMenuGameMode* M = Weak.Get()) { Action(M); } return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
			];
		// The focused button lights up in the squad colour and grows a little: a pad player sees what A presses.
		TSharedRef<SWidget> Out = SNew(SBox).WidthOverride(320.f).Padding(FMargin(0.f, 6.f))
			[
				TDFrontEndStyle::FocusFrame(Made.ToSharedRef())
			];
		if (bEnabled && !FirstButton.IsValid())
		{
			FirstButton = Made;
		}
		return Out;
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
		int32 Cuts = 0; // a CUT is a scene restarted: every take after the first (Stats.Deaths counts ships lost, which a squad survives)
		for (const FTDSceneTakes& Row : GI->GetSceneTakes())
		{
			Cuts += FMath::Max(0, Row.Takes - 1);
		}
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f)
		[
			SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("RUN TIME %d:%02d   KILLS %d   SHIPS LOST %d   CUTS %d"), Seconds / 60, Seconds % 60, P.Stats.Kills, P.Stats.Deaths, Cuts)))
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
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("SETTINGS"), [](ATDMenuGameMode* M) { UTDSettingsSubsystem::OpenSettingsPage(M); }, true)];
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("QUIT"), [](ATDMenuGameMode* M) { M->QuitGame(); }, true)];
	}
	// A light veil over the whole screen and a darker panel behind the buttons only: the menu backdrop scene (T3)
	// shows through while the text stays readable.
	Placeholder = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.03f, 0.3f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.03f, 0.6f))
			.Padding(FMargin(48.f, 32.f))
			[
				Box
			]
		];
	GEngine->GameViewport->AddViewportWidgetContent(Placeholder.ToSharedRef(), 10);
	FocusButton = FirstButton;
	KeepMenuFocus();
}

void ATDMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UTDCheatManager::InstallOn(NewPlayer);
}

ATDTransitionGameMode::ATDTransitionGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = ATDFrontEndHUD::StaticClass();
}

void ATDTransitionGameMode::StartPlay()
{
	Super::StartPlay();
	TDFrontEnd::ShowWidget(GetWorld(), FTDFrontEndWidgets::Resolve(LoadingWidgetClass.Get(), LoadingWidgetClass ? nullptr : FTDFrontEndWidgets::LoadDefaultStyle(), FTDFrontEndWidgets::EKind::Loading), false);
}

#if !UE_BUILD_SHIPPING
// Dev: a real Slate key press (down and up) as a keyboard or pad would send it - drives the menus over MCP.
//   TD.UI.Key Enter | Down | Up | Gamepad_FaceButton_Bottom
static FAutoConsoleCommandWithWorldAndArgs GTDUISlateKey(TEXT("TD.UI.Key"), TEXT("TD.UI.Key <Key>: press a key through Slate (menus)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
	{
		if (Args.Num() < 1 || !FSlateApplication::IsInitialized())
		{
			return;
		}
		const FKey Key(*Args[0]);
		if (!Key.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("TD.UI.Key: unknown key %s"), *Args[0]);
			return;
		}
		FSlateApplication& App = FSlateApplication::Get();
		const FKeyEvent Event(Key, App.GetModifierKeys(), 0, false, 0, 0);
		App.ProcessKeyDownEvent(Event);
		App.ProcessKeyUpEvent(Event);
		const TSharedPtr<SWidget> Focus = App.GetUserFocusedWidget(0);
		UE_LOG(LogTemp, Log, TEXT("TD.UI.Key %s (focus now %s)"), *Key.ToString(), Focus.IsValid() ? *Focus->GetTypeAsString() : TEXT("none"));
	}));
#endif
