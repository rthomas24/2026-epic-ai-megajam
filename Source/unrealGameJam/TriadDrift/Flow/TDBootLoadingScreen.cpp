// Triad Drift - the boot loading screen (T6). See TDBootLoadingScreen.h.

#include "TriadDrift/Flow/TDBootLoadingScreen.h"
#include "Brushes/SlateImageBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace TDBootLoadingScreen
{
	/** The splash picture as a Slate brush, loaded from the loose file the game ships with (null when it is missing). */
	static TSharedPtr<FSlateImageBrush> GSplashBrush;

	static TSharedRef<SWidget> MakeWidget()
	{
		const FString SplashPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Splash/Splash.bmp"));
		if (IFileManager::Get().FileExists(*SplashPath))
		{
			GSplashBrush = MakeShared<FSlateImageBrush>(FName(*SplashPath), FVector2D(640.f, 360.f));
		}
		TSharedRef<SOverlay> Root = SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.01f, .015f, .03f, 1.f))
			];
		if (GSplashBrush.IsValid())
		{
			// The same picture as the splash, filling the window (cropped to fit, never letterboxed).
			Root->AddSlot()
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFill)
				[
					SNew(SImage).Image(GSplashBrush.Get())
				]
			];
		}
		Root->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0.f, 0.f, 48.f, 40.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SThrobber)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(NSLOCTEXT("TDBoot", "Loading", "LOADING"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18)).ColorAndOpacity(FLinearColor(.75f, .9f, 1.f))
			]
		];
		return Root;
	}

	void Start()
	{
#if !UE_SERVER
		if (GIsEditor || IsRunningCommandlet() || IsRunningDedicatedServer() || FParse::Param(FCommandLine::Get(), TEXT("NoBootLoadingScreen")))
		{
			return;
		}
		if (!IsMoviePlayerEnabled() || !FSlateApplication::IsInitialized())
		{
			return;
		}
		IGameMoviePlayer* Player = GetMoviePlayer();
		if (!Player || Player->IsMovieCurrentlyPlaying())
		{
			return;	// a startup movie from the ini is already up
		}
		FLoadingScreenAttributes Attributes;
		Attributes.bAutoCompleteWhenLoadingCompletes = true;	// the engine's WaitForMovieToFinish after the first map load ends it
		Attributes.bMoviesAreSkippable = false;
		Attributes.MinimumLoadingScreenDisplayTime = -1.f;
		Attributes.WidgetLoadingScreen = MakeWidget();
		Player->SetupLoadingScreen(Attributes);
		const bool bPlaying = Player->PlayMovie();
		UE_LOG(LogTemp, Log, TEXT("TD boot loading screen: %s (splash picture %s)"), bPlaying ? TEXT("playing") : TEXT("not started"),
			GSplashBrush.IsValid() ? TEXT("found") : TEXT("missing"));
#endif
	}
}
