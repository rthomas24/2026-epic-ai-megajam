// Triad Drift - UI (Teammate 2). The space HUD's drawing language for engineering-built widgets (on-foot HUD, pause).
//
// A copy of the painter in Nick's Plugins/TriadDriftHUD (TDSpaceHUDWidget.cpp, FPainter, "refined" look) so the on-foot
// HUD and the pause screen read as the same family: same colours, fonts, glass panels, thin rules and meters. The game
// module has no compile-time dependency on the plugin, so this is a copy, not a link. When the plugin's look changes,
// mirror it here (the plugin is the source of truth).

#pragma once

#include "CoreMinimal.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

/** FTDHUDStyle's colours (Plugins/TriadDriftHUD/Source/TriadDriftHUD/Public/TDSpaceHUDWidget.h). */
struct FTDUIPalette
{
	static FLinearColor Rift() { return FLinearColor::FromSRGBColor(FColor(241, 140, 250)); }
	static FLinearColor Lancer() { return FLinearColor::FromSRGBColor(FColor(121, 229, 255)); }
	static FLinearColor Bastion() { return FLinearColor::FromSRGBColor(FColor(255, 206, 119)); }
	static FLinearColor Ink() { return FLinearColor::FromSRGBColor(FColor(240, 248, 255)); }
	static FLinearColor Muted() { return FLinearColor::FromSRGBColor(FColor(162, 182, 201)); }
	static FLinearColor Panel() { return FLinearColor::FromSRGBColor(FColor(8, 19, 31, 230)); }
	static FLinearColor Hostile() { return FLinearColor::FromSRGBColor(FColor(255, 148, 124)); }
	/** The player's accent on foot: squad cyan (the space HUD's Lancer). */
	static FLinearColor Accent() { return Lancer(); }
	/** Text on a filled accent plate (the space HUD's key-glyph ink). */
	static FLinearColor PlateInk() { return FLinearColor(.008f, .02f, .03f); }
};

/** Draws in a 1920x1080 design space, letterboxed into the widget like the space HUD. */
struct FTDUIPainter
{
	const FGeometry& G;
	FSlateWindowElementList& List;
	int32 Layer;
	float Scale = 1.f;
	FVector2D Origin = FVector2D::ZeroVector;

	FTDUIPainter(const FGeometry& InG, FSlateWindowElementList& InList, int32 InLayer, FVector2D Design = FVector2D(1920, 1080))
		: G(InG), List(InList), Layer(InLayer)
	{
		Scale = FMath::Min(G.GetLocalSize().X / Design.X, G.GetLocalSize().Y / Design.Y);
		Origin = (G.GetLocalSize() - Design * Scale) * .5;
	}

	/** Local widget position -> design space (mouse hit tests). */
	FVector2D ToDesign(const FVector2D& Local) const { return Scale > 0.f ? (Local - Origin) / Scale : FVector2D::ZeroVector; }

	FPaintGeometry Geo(FVector2D Pos, FVector2D Size) const { return G.ToPaintGeometry(Size * Scale, FSlateLayoutTransform(Pos * Scale + Origin)); }

	void Box(float X, float Y, float W, float H, FLinearColor C) const
	{
		FSlateDrawElement::MakeBox(List, Layer, Geo({ X, Y }, { W, H }), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, C);
	}
	void Gradient(float X, float Y, float W, float H, FLinearColor A, FLinearColor B, bool bVertical = true) const
	{
		TArray<FSlateGradientStop> Stops;
		Stops.Emplace(FVector2f(0, 0), A);
		Stops.Emplace(FVector2f(W * Scale, H * Scale), B);
		FSlateDrawElement::MakeGradient(List, Layer, Geo({ X, Y }, { W, H }), MoveTemp(Stops), bVertical ? Orient_Horizontal : Orient_Vertical);
	}
	void Frame(float X, float Y, float W, float H, FLinearColor C, float Opacity = 1) const
	{
		const float Cut = 8;
		TArray<FVector2D> Points;
		for (const FVector2D Q : { FVector2D(X + Cut, Y), FVector2D(X + W, Y), FVector2D(X + W, Y + H - Cut), FVector2D(X + W - Cut, Y + H), FVector2D(X, Y + H),
				 FVector2D(X, Y + Cut), FVector2D(X + Cut, Y) })
		{
			Points.Add(Q * Scale + Origin);
		}
		FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, C.CopyWithNewOpacity(Opacity), true, Scale);
	}
	/** The space HUD's glass panel: edge halo, dark gradient, top sheen, double cut-corner frame, corner ticks. */
	void Glass(float X, float Y, float W, float H, FLinearColor C, bool bEmphasis = false) const
	{
		Gradient(X - 3, Y - 3, W + 6, H + 6, C.CopyWithNewOpacity(bEmphasis ? .055f : .025f), FLinearColor(0, 0, 0, .08f));
		Gradient(X, Y, W, H, FLinearColor(.018f, .043f, .073f, .91f), FLinearColor(.0025f, .008f, .016f, .94f));
		Gradient(X + 1, Y + 1, W - 2, 32, C.CopyWithNewOpacity(bEmphasis ? .13f : .065f), C.CopyWithNewOpacity(0));
		Frame(X, Y, W, H, C, bEmphasis ? .55f : .30f);
		Frame(X + 4, Y + 4, W - 8, H - 8, C, .09f);
		Line({ X + 12, Y + 1 }, { X + W * .62f, Y + 1 }, FTDUIPalette::Ink().CopyWithNewOpacity(bEmphasis ? .65f : .30f));
		Line({ X + W - 1, Y + H - 37 }, { X + W - 1, Y + H - 12 }, C.CopyWithNewOpacity(.85f), 1.5f);
		Line({ X + 1, Y + 12 }, { X + 1, Y + 35 }, C.CopyWithNewOpacity(.75f), 1.5f);
		Line({ X + 12, Y + H - 1 }, { X + 40, Y + H - 1 }, C.CopyWithNewOpacity(.6f), 1.5f);
		for (int32 I = 0; I < 3; ++I)
		{
			Line({ X + W - 36 + I * 7, Y + 7 }, { X + W - 39 + I * 7, Y + 11 }, C.CopyWithNewOpacity(.28f));
		}
	}
	static FSlateFontInfo MakeFont(const ANSICHAR* Face, int32 Size, float InScale, int32 Tracking = 0)
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(Face, FMath::Max(8, FMath::RoundToInt(Size * InScale)));
		Font.LetterSpacing = Tracking;
		return Font;
	}
	void Text(float X, float Y, const FString& S, int32 Size, FLinearColor C, bool bBold = false) const
	{
		FSlateDrawElement::MakeText(List, Layer + 1, Geo({ X, Y }, { 1, 1 }), S, MakeFont(bBold ? "Bold" : "Regular", Size, Scale), ESlateDrawEffect::None, C);
	}
	/** Small tracked caps ("OBJECTIVE"). */
	void Label(float X, float Y, const FString& S, int32 Size, FLinearColor C, int32 Tracking = 70) const
	{
		FSlateDrawElement::MakeText(List, Layer + 1, Geo({ X, Y }, { 1, 1 }), S, MakeFont("Medium", Size, Scale, Tracking), ESlateDrawEffect::None, C);
	}
	/** Condensed bold headline (the objective line, power names). */
	void Title(float X, float Y, const FString& S, int32 Size, FLinearColor C) const
	{
		FSlateDrawElement::MakeText(List, Layer + 1, Geo({ X, Y }, { 1, 1 }), S, MakeFont("BoldCondensed", Size, Scale, 25), ESlateDrawEffect::None, C);
	}
	/** Width of S in design units (0 when Slate can't measure, e.g. headless). */
	float Measure(const FString& S, const FSlateFontInfo& DesignFont) const
	{
		if (Scale <= 0 || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
		{
			return 0.f; // headless (-nullrhi): no renderer, no font measure service
		}
		const float DrawScale = G.GetAccumulatedLayoutTransform().GetScale();
		if (DrawScale <= 0)
		{
			return 0.f;
		}
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, DesignFont, DrawScale).X / DrawScale / Scale;
	}
	float MeasureText(const FString& S, int32 Size, bool bBold = false) const { return Measure(S, MakeFont(bBold ? "Bold" : "Regular", Size, Scale)); }
	float MeasureTitle(const FString& S, int32 Size) const { return Measure(S, MakeFont("BoldCondensed", Size, Scale, 25)); }
	void CenteredText(float CenterX, float Y, const FString& S, int32 Size, FLinearColor C, bool bBold = false) const
	{
		Text(CenterX - MeasureText(S, Size, bBold) * .5f, Y, S, Size, C, bBold);
	}
	void CenteredTitle(float CenterX, float Y, const FString& S, int32 Size, FLinearColor C) const
	{
		Title(CenterX - MeasureTitle(S, Size) * .5f, Y, S, Size, C);
	}
	void Line(FVector2D A, FVector2D B, FLinearColor C, float Width = 1) const
	{
		TArray<FVector2D> Points;
		Points.Add(A * Scale + Origin);
		Points.Add(B * Scale + Origin);
		FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, C, true, Width * Scale);
	}
	void Arc(FVector2D C, float R, float Fraction, FLinearColor Color, float Width = 2, float Start = -PI * .5f) const
	{
		const int32 Count = FMath::Max(2, FMath::CeilToInt(48 * Fraction));
		TArray<FVector2D> Points;
		Points.Reserve(Count + 1);
		for (int32 I = 0; I <= Count; ++I)
		{
			const float T = Start + 2 * PI * Fraction * I / Count;
			Points.Add((C + FVector2D(FMath::Cos(T), FMath::Sin(T)) * R) * Scale + Origin);
		}
		FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Width * Scale);
	}
	/** The space HUD's refined meter: dark track, gradient fill, ink lip and end cap, ten dark segments when 7+ tall. */
	void Meter(float X, float Y, float W, float Value, FLinearColor Color, float Height = 4) const
	{
		const float V = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0.f;
		Box(X, Y, W, Height, FLinearColor(.05f, .10f, .14f, .95f));
		Gradient(X, Y, W, Height, FLinearColor(.008f, .021f, .033f), FLinearColor(.024f, .048f, .065f));
		Line({ X, Y + Height + 1 }, { X + W, Y + Height + 1 }, FTDUIPalette::Ink().CopyWithNewOpacity(.13f), .6f);
		const float Fill = W * V;
		if (Fill <= 0)
		{
			return;
		}
		Gradient(X, Y, Fill, Height, Color * .42f, Color, false);
		Gradient(X, Y - 2, Fill, 2, Color.CopyWithNewOpacity(0), Color.CopyWithNewOpacity(.16f));
		Line({ X, Y + .5f }, { X + Fill, Y + .5f }, FTDUIPalette::Ink().CopyWithNewOpacity(.6f), .65f);
		Box(X + FMath::Max(0.f, Fill - 1), Y, 1, Height, FTDUIPalette::Ink().CopyWithNewOpacity(.8f));
		if (Height >= 7)
		{
			for (int32 I = 1; I < 10; ++I)
			{
				if (W * I / 10.f < Fill - 2)
				{
					Box(X + W * I / 10.f, Y, 1, Height, FLinearColor(.003f, .012f, .022f, .35f));
				}
			}
		}
	}
	void Brackets(FVector2D C, float Radius, FLinearColor Color, float Arm = 14) const
	{
		for (int32 X : { -1, 1 })
		{
			for (int32 Y : { -1, 1 })
			{
				const FVector2D P = C + FVector2D(X, Y) * Radius;
				Line(P, P - FVector2D(X * Arm, 0), Color, 2);
				Line(P, P - FVector2D(0, Y * Arm), Color, 2);
			}
		}
	}
	void HealthIcon(float X, float Y, FLinearColor C) const
	{
		TArray<FVector2D> Points;
		for (const FVector2D Q : { FVector2D(-8, -4), FVector2D(-5, -8), FVector2D(-1, -8), FVector2D(0, -5), FVector2D(1, -8), FVector2D(5, -8), FVector2D(8, -4),
				 FVector2D(8, 0), FVector2D(0, 9), FVector2D(-8, 0), FVector2D(-8, -4) })
		{
			Points.Add((FVector2D(X, Y) + Q) * Scale + Origin);
		}
		FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, C, true, 1.8f * Scale);
	}
	void ShieldIcon(float X, float Y, FLinearColor C) const
	{
		TArray<FVector2D> Points;
		for (const FVector2D Q : { FVector2D(-8, -8), FVector2D(8, -8), FVector2D(7, 2), FVector2D(0, 10), FVector2D(-7, 2), FVector2D(-8, -8) })
		{
			Points.Add((FVector2D(X, Y) + Q) * Scale + Origin);
		}
		FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, C, true, 1.8f * Scale);
	}
	/** The space HUD's reticle: two arc halves, ink ticks and a centre dot. */
	void Reticle(FVector2D Center, FLinearColor Accent) const
	{
		Arc(Center, 26, .20f, Accent, 1.5, PI * .15f);
		Arc(Center, 26, .20f, Accent, 1.5, PI * 1.15f);
		const FLinearColor Ink = FTDUIPalette::Ink();
		Line(Center + FVector2D(-10, 0), Center + FVector2D(-4, 0), Ink, 1.5);
		Line(Center + FVector2D(4, 0), Center + FVector2D(10, 0), Ink, 1.5);
		Line(Center + FVector2D(0, 4), Center + FVector2D(0, 10), Ink, 1.5);
		Box(Center.X - 1, Center.Y - 1, 2, 2, Ink);
	}
};
