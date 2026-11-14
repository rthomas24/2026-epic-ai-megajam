// Triad Drift - the credits roll on the victory screen (see header). Owner: Teammate 3.

#include "TriadDrift/Director/Menu/TDCredits.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "TDCredits"

#if WITH_DEV_AUTOMATION_TESTS
TWeakObjectPtr<const UTDCreditsData> FTDCredits::OverrideForTests;
#endif

namespace TDCreditsPrivate
{
	static FTDCreditsSection S(const FText& Heading, TArray<FText> Lines)
	{
		FTDCreditsSection Out;
		Out.Heading = Heading;
		Out.Lines = MoveTemp(Lines);
		return Out;
	}

	static const UTDCreditsData* Data()
	{
#if WITH_DEV_AUTOMATION_TESTS
		if (const UTDCreditsData* D = FTDCredits::OverrideForTests.Get())
		{
			return D;
		}
#endif
		return UTDStatics::LoadPresentation<UTDCreditsData>(TEXT("DA_Credits"));
	}

	/** Paints the sections scrolling upward through its box, then again from the bottom. Never takes input. */
	class STDCreditsRoll : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(STDCreditsRoll) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, TArray<FTDCreditsSection> InSections, float InSpeed)
		{
			Sections = MoveTemp(InSections);
			Speed = InSpeed;
			Start = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : 0.0;
			SetVisibility(EVisibility::HitTestInvisible);
			// Repaint every frame while it rolls (the timer lives with the widget: it goes when the panel goes).
			RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda([](double, float) { return EActiveTimerReturnType::Continue; }));
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(100.f, 100.f); }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geo, const FSlateRect& Culling, FSlateWindowElementList& Out, int32 Layer,
			const FWidgetStyle&, bool) const override
		{
			const FVector2D Size = Geo.GetLocalSize();
			const FSlateFontInfo HeadFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 15);
			const FSlateFontInfo LineFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14);
			const float HeadH = 24.f, LineH = 21.f, Gap = 18.f;
			float Total = 0.f;
			for (const FTDCreditsSection& Sec : Sections)
			{
				Total += (Sec.Heading.IsEmpty() ? 0.f : HeadH) + LineH * Sec.Lines.Num() + Gap;
			}
			const double Now = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : Start;
			const float Cycle = Total + Size.Y;
			// Starts just below the box after a 1.5 s pause, rolls up, loops.
			const float T = FMath::Max(0.f, static_cast<float>(Now - Start) - 1.5f);
			float Y = Size.Y - FMath::Fmod(T * Speed, FMath::Max(Cycle, 1.f));
			const FSlateRect Clip = Geo.GetLayoutBoundingRect();
			Out.PushClip(FSlateClippingZone(Clip));
			auto Line = [&](const FText& Text, const FSlateFontInfo& Font, const FLinearColor& Color, float H)
			{
				if (Y > -H && Y < Size.Y)
				{
					FVector2D M(Size.X, H);
					if (FSlateApplication::IsInitialized())
					{
						M = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font);
					}
					// Fade at the top and bottom edges of the box.
					const float Edge = FMath::Min(FMath::Clamp(Y / 30.f, 0.f, 1.f), FMath::Clamp((Size.Y - Y - H) / 30.f + 1.f, 0.f, 1.f));
					FLinearColor C = Color;
					C.A *= Edge;
					FSlateDrawElement::MakeText(Out, Layer, Geo.ToPaintGeometry(M, FSlateLayoutTransform(FVector2D((Size.X - M.X) * 0.5f, Y))),
						Text, Font, ESlateDrawEffect::None, C);
				}
				Y += H;
			};
			for (const FTDCreditsSection& Sec : Sections)
			{
				if (!Sec.Heading.IsEmpty())
				{
					Line(Sec.Heading, HeadFont, FLinearColor(1.f, 0.85f, 0.4f), HeadH);
				}
				for (const FText& L : Sec.Lines)
				{
					Line(L, LineFont, FLinearColor(0.88f, 0.92f, 1.f), LineH);
				}
				Y += Gap;
			}
			Out.PopClip();
			return Layer + 1;
		}

	private:
		TArray<FTDCreditsSection> Sections;
		float Speed = 30.f;
		double Start = 0.0;
	};
}

const TArray<FTDCreditsSection>& FTDCredits::Fallback()
{
	using TDCreditsPrivate::S;
	// Only what CREDITS.md credits; Nicholas's DA_Credits replaces all of it.
	static const TArray<FTDCreditsSection> Sections = {
		S(LOCTEXT("H_Title", "TRIAD DRIFT"), { LOCTEXT("L_Made", "Made in four days for the 2026 Epic AI MegaJam") }),
		S(LOCTEXT("H_Team", "TEAM"), {
			LOCTEXT("L_Ryan", "Ryan Thomas  ·  Gameplay & Engineering Lead"),
			LOCTEXT("L_RyanDid", "game design, flight and three-ship switching, combat and enemy AI, worlds, VFX and animation direction"),
			LOCTEXT("L_Nick", "Nicholas Tosti  ·  Player Experience Lead") }),
		S(LOCTEXT("H_Story", "STORY"), { LOCTEXT("L_Story", "Story by Nicholas Tosti") /* StoryFidelity01: the subtitle reveals the twist; credits are reachable before it */ }),
		S(LOCTEXT("H_AI", "BUILT WITH AI AGENTS"), {
			LOCTEXT("L_Claude", "Claude Code (Anthropic)  ·  engineering lead and teammate agents"),
			LOCTEXT("L_Codex", "OpenAI Codex  ·  art, UI, audio and polish agents"),
			LOCTEXT("L_Rodin", "Hyper3D Rodin  ·  3D models"),
			LOCTEXT("L_Grok", "Grok  ·  concept art and terrain textures"),
			LOCTEXT("L_Meshy", "Meshy AI  ·  the Sentinel armour") }),
		S(LOCTEXT("H_Tools", "TOOLS"), {
			LOCTEXT("L_UE", "Unreal Engine 5.9 and its MCP server"),
			LOCTEXT("L_Blender", "Blender 5.2"),
			LOCTEXT("L_ffmpeg", "ffmpeg  ·  Python  ·  NumPy") }),
		S(LOCTEXT("H_Thanks", "THANKS"), {
			LOCTEXT("L_Epic", "Epic Games, for the jam and the template content"),
			LOCTEXT("L_Assets", "Full asset and licence list on the game's page"),
			LOCTEXT("L_Play", "Thank you for playing.") }),
	};
	return Sections;
}

TArray<FTDCreditsSection> FTDCredits::GetSections()
{
	if (const UTDCreditsData* D = TDCreditsPrivate::Data(); D && D->Sections.Num() > 0)
	{
		return D->Sections;
	}
	return Fallback();
}

TSharedRef<SWidget> FTDCredits::MakeRoll(float Width, float Height)
{
	const UTDCreditsData* D = TDCreditsPrivate::Data();
	const float Speed = D ? D->PixelsPerSecond : 30.f;
	return SNew(SBox).WidthOverride(Width).HeightOverride(Height).Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(TDCreditsPrivate::STDCreditsRoll, GetSections(), Speed)
		];
}

#undef LOCTEXT_NAMESPACE
