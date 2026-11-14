// Triad Drift - game modes for the non-gameplay maps. Owner: Teammate 5.

#include "TriadDrift/Flow/TDFrontEndGameModes.h"
#include "TimerManager.h"
#include "TriadDrift/Profile/TDProfileSubsystem.h"
#include "EngineUtils.h"
#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Director/Menu/TDCredits.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
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
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "TriadDrift/Flow/TDFrontEndStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
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
	/** Restrained, texture-free screen graphics; no scene capture, timer or input. */
	class SVictoryLinework : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SVictoryLinework) {} SLATE_END_ARGS()
		void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920,1080); }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
		{
			const FLinearColor Cyan(.35f,.76f,.83f,.12f);
			auto Line=[&](TArray<FVector2f> Points)
			{
				FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Cyan,true,1.f);
			};
			for(const float Radius:{450.f,600.f,860.f})
			{
				TArray<FVector2f> Arc;
				for(int32 I=0;I<=96;++I)
				{
					const float A=2.f*PI*I/96.f;
					Arc.Add(FVector2f(960.f+Radius*FMath::Cos(A),540.f+Radius*FMath::Sin(A)));
				}
				Line(MoveTemp(Arc));
			}
			Line({FVector2f(520,110),FVector2f(1400,110),FVector2f(1400,970),FVector2f(520,970),FVector2f(520,110)});
			Line({FVector2f(280,0),FVector2f(1380,1080)});
			Line({FVector2f(1640,0),FVector2f(540,1080)});
			for(const FVector2f P:{FVector2f(960,52),FVector2f(310,540),FVector2f(1610,540)})
			{
				Line({P-FVector2f(12,0),P+FVector2f(12,0)});
				Line({P-FVector2f(0,12),P+FVector2f(0,12)});
			}
			return Layer+1;
		}
	};
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
	TickMusicYield();
}

void ATDMenuGameMode::TickMusicYield()
{
	// Another score owns the music (Nick's victory score claims it on VictoryBegan, N-053): the menu bed steps aside
	// rather than playing underneath, and comes back when the claim is released (T1, approved by T5).
	if (!Music || bMusicFading) { return; }
	const UTDMusicDirectorSubsystem* Director = UTDMusicDirectorSubsystem::GetMusicDirector(this);
	const bool bHeld = Director && Director->IsExclusiveScoreHeld();
	if (bHeld && !bMusicYielded)
	{
		bMusicYielded = true;
		Music->FadeOut(MusicYieldFadeOutSeconds, 0.f);
	}
	else if (!bHeld && bMusicYielded)
	{
		bMusicYielded = false;
		Music->FadeIn(MusicYieldFadeInSeconds, 1.f);
	}
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
	// Cache in the presentation lifecycle, never in Paint/Tick or a text attribute (N-064).
	if (bShowingVictory && VictoryHeadline.IsEmpty())
	{
		const TArray<FText>& Choices = VictoryHeadlineChoices();
		VictoryHeadline = Choices[FMath::RandHelper(Choices.Num())];
	}
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
	if (Music && !bMusicFading && !bMusicYielded && !IsActorBeingDestroyed())
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

bool ATDMenuGameMode::NewGameSkipTutorial()
{
	UTDGameInstance* GI = UTDGameInstance::Get(this);
	UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(this);
	if (!GI || !Profile || !Profile->StartNewRunSkippingTutorial())
	{
		return false;
	}
	FadeOutMusic();
	return GI->TravelTo(ETDDestination::Galaxy, NAME_None);
}

bool ATDMenuGameMode::CanSkipTutorial() const
{
	const UTDProfileSubsystem* Profile = UTDProfileSubsystem::Get(this);
	return Profile && Profile->CanSkipTutorial();
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
	// Leaving the victory screen before the ending stinger: it plays first, then the menu (Lead, Nov 13: the player
	// always receives the signal). Once.
	if (bShowingVictory && !bReturnPending)
	{
		for (TActorIterator<ATDMenuBackdrop> It(GetWorld()); It; ++It)
		{
			if (It->Scene == ETDBackdropScene::Victory && !It->HasPlayedStinger())
			{
				const float Wait = It->PlayStinger();
				bReturnPending = true;
				FTimerHandle H;
				GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]() { ReturnToMenu(); }), FMath::Max(Wait, 0.1f), false);
				return;
			}
		}
	}
	bReturnPending = false;
	bShowingVictory = false;
	VictoryHeadline = FText::GetEmpty();
	for (TActorIterator<ATDMenuBackdrop> It(GetWorld()); It; ++It)
	{
		It->FadeStingerCaption(); // the menu comes up clean
	}
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
		// N-064 (Nick's victory revision): eyebrow, a random large headline (cached for this presentation), stats in
		// cyan, a thin rule, the scene list (T5's bounded scroll, N-047), the native credits at 1.4x, RETURN bottom-left,
		// over a veil and fine linework instead of the opaque card.
		const FLinearColor Ink(.88f, .95f, 1.f), Cyan(.40f, .75f, .81f);
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TDMenu", "VictoryEyebrow", "MISSION COMPLETE // OPERATION PARADOX"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 24)).ColorAndOpacity(Cyan)
		];
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(1320).HeightOverride(112)
			[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[SNew(STextBlock).Text(VictoryHeadline).Font(FCoreStyle::GetDefaultFontStyle("Bold", 72)).ColorAndOpacity(Ink)]]
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
			SNew(SBox).WidthOverride(1320).HeightOverride(48)
			[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[SNew(STextBlock).Text(FText::Format(NSLOCTEXT("TDMenu", "VictoryStats", "RUN TIME {0}   |   KILLS {1}   |   SHIPS LOST {2}   |   CUTS {3}   |   DEBRIS CLEARED {4}"),
					FText::FromString(FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60)), FText::AsNumber(P.Stats.Kills), FText::AsNumber(P.Stats.Deaths), FText::AsNumber(Cuts),
					FText::AsNumber(P.Stats.DebrisCleared)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 28)).ColorAndOpacity(Cyan)]]
		];
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 24, 0, 18)
		[SNew(SBox).WidthOverride(1040).HeightOverride(1)
			[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.4f, .75f, .81f, .5f)).Padding(0)]];
		// The scene list is bounded (N-047, T5: a long run pushed RETURN TO MENU off a 720p screen): a scroll box at most
		// SceneListMaxHeight tall, so the credits roll and the button always fit.
		const TArray<FTDSceneTakes> Scenes = GI->GetSceneTakes();
		TSharedRef<SScrollBox> SceneList = SNew(SScrollBox).ScrollBarAlwaysVisible(Scenes.Num() > 9);
		for (const FTDSceneTakes& Row : Scenes)
		{
			SceneList->AddSlot().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("SCENE %d  %s  -  %d %s"), Row.SceneNumber, *Row.SceneId.ToString(), Row.Takes, Row.Takes == 1 ? TEXT("TAKE") : TEXT("TAKES"))))
				.WrapTextAt(1160).Justification(ETextJustify::Center)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).ColorAndOpacity(Ink)
			];
		}
		if (Scenes.Num() > 0)
		{
			Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 2.f)
			[
				SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("SCENES (%d)"), Scenes.Num()))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				.ColorAndOpacity(Cyan)
			];
			Box->AddSlot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox).MaxDesiredHeight(SceneListMaxHeight).WidthOverride(1200.f)[SceneList]
			];
		}
		// T3: the credits roll (DA_Credits, else the CREDITS.md fallback), at 1.4x of the native roll.
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 24, 0, 0)
		[SNew(SBox).WidthOverride(1008).HeightOverride(280)
			[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[FTDCredits::MakeRoll(720, 200)]]];
		// Reference-like open composition; the real return action and focus, no invented shortcut hints.
		Placeholder = SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.005f, .012f, .025f, .38f))]
			+ SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[SNew(SBox).WidthOverride(1920).HeightOverride(1080)
					[SNew(SOverlay)
						+ SOverlay::Slot()[SNew(TDFrontEnd::SVictoryLinework)]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, 188, 0, 0)[Box]
						+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(56, 0, 0, 44)
							[Button(TEXT("RETURN TO MENU"), [](ATDMenuGameMode* M) { M->ReturnToMenu(); }, true)]
					]]];
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->AddViewportWidgetContent(Placeholder.ToSharedRef(), 10);
		}
		FocusButton = FirstButton;
		KeepMenuFocus();
		return; // the normal menu's card, focus, Settings and music path stay untouched
	}
	else
	{
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 24.f)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TDMenu", "Title", "TRIAD DRIFT")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 64)).ColorAndOpacity(FLinearColor(0.4f, 0.9f, 1.f))
		];
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("NEW GAME"), [](ATDMenuGameMode* M) { M->NewGame(); }, true)];
		if (CanSkipTutorial()) // T7: only after the tutorial was completed once on this profile
		{
			Box->AddSlot().AutoHeight().HAlign(HAlign_Center)[Button(TEXT("SKIP THE TUTORIAL"), [](ATDMenuGameMode* M) { M->NewGameSkipTutorial(); }, true)];
		}
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

const TArray<FText>& ATDMenuGameMode::VictoryHeadlineChoices()
{
	static const TArray<FText> Choices = {
		NSLOCTEXT("TDMenu", "VictoryFuture", "A FUTURE RECLAIMED"),
		NSLOCTEXT("TDMenu", "VictoryBalance", "BALANCE RESTORED"),
		NSLOCTEXT("TDMenu", "VictoryTomorrow", "TOMORROW IS OURS"),
		NSLOCTEXT("TDMenu", "VictoryWorlds", "THE WORLDS ENDURE"),
		NSLOCTEXT("TDMenu", "VictoryDawn", "A NEW DAWN"),
		NSLOCTEXT("TDMenu", "VictoryOdds", "AGAINST ALL ODDS")
	};
	return Choices;
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
