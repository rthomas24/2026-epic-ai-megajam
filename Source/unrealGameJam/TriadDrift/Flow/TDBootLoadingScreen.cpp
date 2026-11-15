// Triad Drift - the boot loading screen (T6). See TDBootLoadingScreen.h.

#include "TriadDrift/Flow/TDBootLoadingScreen.h"
#include "TriadDrift/UI/TDUIPaint.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MoviePlayer.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace TDBootLoadingScreen
{
	/** The splash picture as a Slate texture made from the loose file (null when it is missing or unreadable). */
	static TSharedPtr<FSlateDynamicImageBrush> GSplashBrush;
	static const TCHAR* GSplashResult = TEXT("not tried");

	/** Content/Splash/Splash.bmp (staged as a loose file, read before the asset system is up) decoded by hand: 24 or 32
	 *  bit, uncompressed, either row order. The packaged 36bee71d drew a path brush of it as a plain white quad, so the
	 *  picture is handed to the renderer as pixels instead. */
	static TSharedPtr<FSlateDynamicImageBrush> LoadSplash()
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Splash/Splash.bmp"));
		TArray<uint8> File;
		if (!FFileHelper::LoadFileToArray(File, *Path, FILEREAD_Silent)) { GSplashResult = TEXT("file missing"); return nullptr; }
		auto U16 = [&File](int32 At) { return int32(File[At]) | (int32(File[At + 1]) << 8); };
		auto U32 = [&File](int32 At) { return int32(uint32(File[At]) | (uint32(File[At + 1]) << 8) | (uint32(File[At + 2]) << 16) | (uint32(File[At + 3]) << 24)); };
		if (File.Num() < 54 || File[0] != 'B' || File[1] != 'M') { GSplashResult = TEXT("not a BMP"); return nullptr; }
		const int32 Offset = U32(10), W = U32(18), RawH = U32(22), Bpp = U16(28), Compression = U32(30);
		const int32 H = FMath::Abs(RawH);
		if (W <= 0 || H <= 0 || W > 8192 || H > 8192 || (Bpp != 24 && Bpp != 32) || (Compression != 0 && Compression != 3))
		{
			GSplashResult = TEXT("unsupported BMP"); return nullptr;
		}
		const int32 Row = ((Bpp * W + 31) / 32) * 4;
		if (Offset < 54 || int64(Offset) + int64(Row) * H > File.Num()) { GSplashResult = TEXT("BMP too short"); return nullptr; }
		TArray<uint8> Bgra;
		Bgra.SetNumUninitialized(W * H * 4);
		const int32 Step = Bpp / 8;
		for (int32 y = 0; y < H; ++y)
		{
			const uint8* Src = File.GetData() + Offset + int64(Row) * (RawH > 0 ? H - 1 - y : y);	// positive height = bottom-up
			uint8* Dst = Bgra.GetData() + int64(y) * W * 4;
			for (int32 x = 0; x < W; ++x, Src += Step, Dst += 4)
			{
				Dst[0] = Src[0]; Dst[1] = Src[1]; Dst[2] = Src[2]; Dst[3] = 255;
			}
		}
		const FName Name(TEXT("TDBootSplash"));
		if (!FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(Name, uint32(W), uint32(H), Bgra))
		{
			GSplashResult = TEXT("renderer refused it"); return nullptr;
		}
		GSplashResult = TEXT("drawn from pixels");
		return MakeShared<FSlateDynamicImageBrush>(Name, FVector2D(W, H));
	}

	static TSharedRef<SWidget> MakeWidget()
	{
		GSplashBrush = LoadSplash();
		const FSlateBrush* Solid = FCoreStyle::Get().GetBrush("WhiteBrush");
		// Near-black first, so a missing picture is a dark card with white text, never a white one.
		TSharedRef<SOverlay> Root = SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(Solid).ColorAndOpacity(FLinearColor(.02f, .02f, .025f, 1.f))
			];
		if (GSplashBrush.IsValid())
		{
			// The splash picture filling the window (cropped to fit, never letterboxed).
			Root->AddSlot()
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFill)
				[
					SNew(SImage).Image(GSplashBrush.Get())
				]
			];
		}
		// A dark band along the bottom so the line reads on any picture, then LOADING in the HUD's ink with the squad cyan
		// throbber beside it.
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 20);
		Font.LetterSpacing = 420;
		Root->AddSlot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(96.f)
			[
				SNew(SImage).Image(Solid).ColorAndOpacity(FLinearColor(.008f, .02f, .035f, .78f))
			]
		];
		Root->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0.f, 0.f, 56.f, 32.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 18.f, 0.f)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0.f).ColorAndOpacity(FTDUIPalette::Lancer())
				[
					SNew(SThrobber).NumPieces(3)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(NSLOCTEXT("TDBoot", "Loading", "LOADING")).Font(Font).ColorAndOpacity(FTDUIPalette::Ink())
				.ShadowOffset(FVector2D(0.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .8f))
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
#if PLATFORM_WINDOWS
		// The packaged 36bee71d froze the game thread 7.8 s in the first load; after 5 s without messages Windows swaps the
		// window for a whitened "Not Responding" ghost over the card. The card keeps painting on its own thread, so the
		// ghost is only in the way.
		::DisableProcessWindowsGhosting();
#endif
		FLoadingScreenAttributes Attributes;
		Attributes.bAutoCompleteWhenLoadingCompletes = true;	// the engine's WaitForMovieToFinish after the first map load ends it
		Attributes.bMoviesAreSkippable = false;
		Attributes.MinimumLoadingScreenDisplayTime = -1.f;
		Attributes.WidgetLoadingScreen = MakeWidget();
		Player->SetupLoadingScreen(Attributes);
		const bool bPlaying = Player->PlayMovie();
		UE_LOG(LogTemp, Log, TEXT("TD boot loading screen: %s (splash picture: %s)"), bPlaying ? TEXT("playing") : TEXT("not started"), GSplashResult);
#endif
	}

	void Shutdown()
	{
		GSplashBrush.Reset();	// before Slate goes away (the brush releases its texture through the renderer)
	}
}
