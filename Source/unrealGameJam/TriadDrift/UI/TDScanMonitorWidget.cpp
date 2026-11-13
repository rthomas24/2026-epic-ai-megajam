// Triad Drift - the scan monitor graphic (Teammate 2). See header.

#include "TriadDrift/UI/TDScanMonitorWidget.h"

#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "TriadDrift/UI/TDUIPaint.h"

namespace TDScanMonitor
{
	using T = FTDFlightHUDTokens;

	/** Where the strata end and the subsurface begins (design frame). */
	constexpr float SurfaceY = 330.f;

	/** Points along a closed polyline, first Fraction (0..1) of its perimeter. */
	void Trace(const FTDUIPainter& P, const TArray<FVector2D>& Pts, float Fraction, FLinearColor C, float Width)
	{
		if (Pts.Num() < 2 || Fraction <= 0.f)
		{
			return;
		}
		float Total = 0.f;
		for (int32 I = 0; I < Pts.Num(); ++I)
		{
			Total += FVector2D::Distance(Pts[I], Pts[(I + 1) % Pts.Num()]);
		}
		float Left = Total * FMath::Clamp(Fraction, 0.f, 1.f);
		for (int32 I = 0; I < Pts.Num() && Left > 0.f; ++I)
		{
			const FVector2D A = Pts[I], B = Pts[(I + 1) % Pts.Num()];
			const float Len = FVector2D::Distance(A, B);
			P.Line(A, Len <= Left ? B : FMath::Lerp(A, B, Left / FMath::Max(Len, 1.f)), C, Width);
			Left -= Len;
		}
	}

	/** The outline scaled about its centre (inner rings). */
	TArray<FVector2D> Scaled(const TArray<FVector2D>& Pts, float S)
	{
		FVector2D C = FVector2D::ZeroVector;
		for (const FVector2D& V : Pts)
		{
			C += V;
		}
		C /= FMath::Max(1, Pts.Num());
		TArray<FVector2D> Out;
		for (const FVector2D& V : Pts)
		{
			Out.Add(C + (V - C) * S);
		}
		return Out;
	}
}

TArray<FVector2D> UTDScanMonitorWidget::StructureOutline()
{
	// A long, precise octagon with chamfered ends: artificial, unreadable, nothing that looks like a ship or a face.
	const FVector2D C(930.f, 560.f);
	const float W = 330.f, H = 125.f, Ch = 90.f;
	return {
		C + FVector2D(-W + Ch, -H), C + FVector2D(W - Ch, -H), C + FVector2D(W, -H + Ch * .6f), C + FVector2D(W, H - Ch * .6f),
		C + FVector2D(W - Ch, H), C + FVector2D(-W + Ch, H), C + FVector2D(-W, H - Ch * .6f), C + FVector2D(-W, -H + Ch * .6f) };
}

void UTDScanMonitorWidget::Show(int32 InStage, float Seconds)
{
	InStage = FMath::Clamp(InStage, 1, MaxStage);
	if (Stage == 0 || bHiding || IsFinished())
	{
		Stage = 0;
		Age = 0.f;
		bHiding = false;
		HideAge = 0.f;
	}
	// New layers draw in one after another, 0.6 s apart.
	for (int32 L = Stage + 1; L <= InStage; ++L)
	{
		LayerStart[L] = Age + (L - Stage - 1) * .6f;
	}
	Stage = FMath::Max(Stage, InStage);
	Duration = Seconds > 0.f ? Age + Seconds : 0.f;
}

void UTDScanMonitorWidget::Hide()
{
	if (!bHiding && Stage > 0)
	{
		bHiding = true;
		HideAge = 0.f;
	}
}

void UTDScanMonitorWidget::Advance(float Dt)
{
	Age += Dt;
	if (bHiding)
	{
		HideAge += Dt;
	}
	else if (Duration > 0.f && Age >= Duration)
	{
		Hide();
	}
}

void UTDScanMonitorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

float UTDScanMonitorWidget::GetVisibility01() const
{
	if (Stage == 0)
	{
		return 0.f;
	}
	const float In = FMath::Clamp(Age / FadeSeconds, 0.f, 1.f);
	return bHiding ? In * FMath::Clamp(1.f - HideAge / FadeSeconds, 0.f, 1.f) : In;
}

float UTDScanMonitorWidget::GetLayer01(int32 Layer) const
{
	if (Layer < 1 || Layer > Stage)
	{
		return 0.f;
	}
	return FMath::Clamp((Age - LayerStart[Layer]) / RevealSeconds, 0.f, 1.f);
}

int32 UTDScanMonitorWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace TDScanMonitor;
	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	LastBoxes.Reset();
	const float Vis = GetVisibility01();
	if (Vis <= 0.f)
	{
		return Base;
	}
	FTDUIPainter P(AllottedGeometry, OutDrawElements, Base + 1);
	P.Alpha = Vis;
	const FBox2D Box = PanelBox();
	const FVector2D Size = Box.GetSize();
	auto Rec = [this](const TCHAR* Name, const FBox2D& B) { LastBoxes.Emplace(FName(Name), B); };
	Rec(TEXT("panel"), Box);

	// Panel: the HUD's plate, a fine grid, a frame, a header that names the instrument.
	P.Box(Box.Min.X, Box.Min.Y, Size.X, Size.Y, T::Plate().CopyWithNewOpacity(.94f));
	for (float X = Box.Min.X + 36.f; X < Box.Max.X; X += 36.f)
	{
		P.Line({ X, Box.Min.Y + 40.f }, { X, Box.Max.Y - 8.f }, T::Dim().CopyWithNewOpacity(.05f), 1.f);
	}
	P.Frame(Box.Min.X, Box.Min.Y, Size.X, Size.Y, T::Neutral(), .25f);
	const bool bOverlay = Stage >= 4;
	P.Label(Box.Min.X + 20.f, Box.Min.Y + 12.f, bOverlay ? TEXT("SURVEY OVERLAY  ·  VERDANCE  +  CINDER") : TEXT("SUBSURFACE SCAN  ·  VERDANCE"),
		14, T::Neutral().CopyWithNewOpacity(.85f), 120);
	// Depth scale on the left edge.
	for (int32 I = 0; I < 6; ++I)
	{
		const float Y = SurfaceY + 30.f + I * 80.f;
		P.Line({ Box.Min.X + 10.f, Y }, { Box.Min.X + 22.f, Y }, T::Dim().CopyWithNewOpacity(.6f), 1.f);
		P.Label(Box.Min.X + 26.f, Y - 7.f, FString::Printf(TEXT("%d"), (I + 1) * 400), 10, T::Dim().CopyWithNewOpacity(.7f), 40);
	}

	// Layer 1: strata (gentle bands, the jungle surface on top) and the buried structure traced in, two inner rings.
	const float L1 = GetLayer01(1);
	const float Now = Age;
	for (int32 S = 0; S < 7; ++S)
	{
		const float BaseY = SurfaceY + S * 70.f;
		TArray<FVector2D> Band;
		for (int32 I = 0; I <= 36; ++I)
		{
			const float X = Box.Min.X + 40.f + I * (Size.X - 60.f) / 36.f;
			Band.Add({ X, BaseY + 10.f * FMath::Sin(I * .45f + S * 1.3f) + 4.f * FMath::Sin(I * 1.7f + S) });
		}
		const FLinearColor C = S == 0 ? T::Lancer().CopyWithNewOpacity(.55f) : T::Dim().CopyWithNewOpacity(.14f + .02f * S);
		for (int32 I = 0; I < Band.Num() - 1 && (float)I / (Band.Num() - 1) <= FMath::Max(L1, .02f) * 1.2f; ++I)
		{
			P.Line(Band[I], Band[I + 1], C, S == 0 ? 2.f : 1.f);
		}
	}
	const TArray<FVector2D> Outline = StructureOutline();
	const FLinearColor Struct = T::Shield();
	Trace(P, Outline, L1, Struct, 2.5f);
	Trace(P, Scaled(Outline, .72f), FMath::Clamp(L1 * 1.4f - .4f, 0.f, 1.f), Struct.CopyWithNewOpacity(.55f), 1.5f);
	Trace(P, Scaled(Outline, .44f), FMath::Clamp(L1 * 1.8f - .8f, 0.f, 1.f), Struct.CopyWithNewOpacity(.35f), 1.f);
	if (L1 >= 1.f)
	{
		// Precise: cross-members between the rings at every vertex, and a slow return pulse along the outline.
		const TArray<FVector2D> Inner = Scaled(Outline, .44f);
		for (int32 I = 0; I < Outline.Num(); ++I)
		{
			P.Line(Outline[I], Inner[I], Struct.CopyWithNewOpacity(.3f), 1.f);
		}
		const float Pulse = FMath::Fmod(Now * .25f, 1.f);
		const FVector2D Head = FMath::Lerp(Outline[FMath::FloorToInt(Pulse * 8.f) % 8], Outline[(FMath::FloorToInt(Pulse * 8.f) + 1) % 8], FMath::Frac(Pulse * 8.f));
		P.Arc(Head, 5.f, 1.f, Struct, 2.f);
		Rec(TEXT("structure"), FBox2D(Outline));
	}

	// Layer 2: the Dominion's scan sites, on the outline, each with a scan line up to the surface.
	const float L2 = GetLayer01(2);
	if (L2 > 0.f)
	{
		const int32 Sites[5] = { 0, 1, 3, 4, 6 };
		for (int32 K = 0; K < 5; ++K)
		{
			const float Show = FMath::Clamp(L2 * 5.f - K, 0.f, 1.f);
			if (Show <= 0.f)
			{
				continue;
			}
			const FVector2D At = Outline[Sites[K]];
			const FLinearColor C = T::Danger().CopyWithNewOpacity(Show);
			for (float Y = SurfaceY; Y < At.Y - 8.f; Y += 14.f)
			{
				P.Line({ At.X, Y }, { At.X, FMath::Min(Y + 7.f, At.Y - 8.f) }, T::Danger().CopyWithNewOpacity(.35f * Show), 1.f);
			}
			P.Box(At.X - 4.f, At.Y - 4.f, 8.f, 8.f, C);
			P.Arc(At, 9.f + 3.f * FMath::Sin(Now * 3.f + K), 1.f, C.CopyWithNewOpacity(.5f * Show), 1.f);
			Rec(TEXT("site"), FBox2D(At - FVector2D(9, 9), At + FVector2D(9, 9)));
		}
		P.Label(Box.Min.X + 40.f, Box.Max.Y - 30.f, TEXT("SCAN SITES  ·  5"), 12, T::Danger().CopyWithNewOpacity(.85f * L2), 100);
	}

	// Layer 3: the correlated coordinate at Cinder, top right, linked from the structure.
	const float L3 = GetLayer01(3);
	if (L3 > 0.f && !bOverlay)
	{
		const FVector2D Tag(Box.Max.X - 300.f, Box.Min.Y + 60.f);
		const FLinearColor C = T::Warning().CopyWithNewOpacity(L3);
		const FVector2D From = Outline[2];
		const FVector2D To(Tag.X + 10.f, Tag.Y + 30.f);
		for (int32 S = 0; S < 20; S += 2)
		{
			P.Line(FMath::Lerp(From, To, S / 20.f * L3), FMath::Lerp(From, To, (S + 1) / 20.f * L3), C.CopyWithNewOpacity(.5f * L3), 1.5f);
		}
		P.Frame(Tag.X, Tag.Y, 270.f, 46.f, T::Warning(), .6f * L3);
		P.Label(Tag.X + 14.f, Tag.Y + 6.f, TEXT("CORRELATED"), 11, C, 140);
		P.Label(Tag.X + 14.f, Tag.Y + 22.f, TEXT("CINDER"), 16, C, 140);
		Rec(TEXT("cinder_tag"), FBox2D(Tag, Tag + FVector2D(270.f, 46.f)));
	}

	// Layer 4: the partial Cinder pattern laid over the Verdance outline, in amber, with the matching angles marked.
	const float L4 = GetLayer01(4);
	if (L4 > 0.f)
	{
		const FLinearColor Amber = T::Warning();
		// Cinder's section: the right half of the outline, drawn in from a small offset that settles onto it.
		const FVector2D Off = FVector2D(36.f, -22.f) * (1.f - FMath::SmoothStep(0.f, 1.f, L4));
		const int32 Part[5] = { 1, 2, 3, 4, 5 };
		for (int32 K = 0; K < 4; ++K)
		{
			P.Line(Outline[Part[K]] + Off, Outline[Part[K + 1]] + Off, Amber.CopyWithNewOpacity(.9f * L4), 2.5f);
		}
		P.Line(Outline[Part[0]] + Off, Outline[0] + Off * 1.4f + FVector2D(-40.f, 0.f), Amber.CopyWithNewOpacity(.4f * L4), 1.5f); // breaks off
		if (L4 >= 1.f)
		{
			for (const int32 V : { 2, 3, 4 })
			{
				P.Arc(Outline[V], 16.f, .3f, T::Neutral(), 2.f, -PI * .5f + V);
				P.Box(Outline[V].X - 2.f, Outline[V].Y - 2.f, 4.f, 4.f, T::Neutral());
			}
			P.Label(Outline[3].X + 24.f, Outline[3].Y - 8.f, TEXT("MATCH  ·  3"), 12, T::Neutral(), 100);
			Rec(TEXT("cinder_layer"), FBox2D(TArray<FVector2D>{ Outline[1], Outline[2], Outline[3], Outline[4], Outline[5] }));
		}
	}

	// Layer 5: the local Cinder map, bottom right: contours, the crystal cache and the field anchor.
	const float L5 = GetLayer01(5);
	if (L5 > 0.f)
	{
		const FBox2D Map(FVector2D(Box.Max.X - 330.f, Box.Max.Y - 230.f), FVector2D(Box.Max.X - 20.f, Box.Max.Y - 20.f));
		const FVector2D MapC = Map.GetCenter();
		P.Box(Map.Min.X, Map.Min.Y, Map.GetSize().X, Map.GetSize().Y, T::Plate().CopyWithNewOpacity(.9f * L5));
		P.Frame(Map.Min.X, Map.Min.Y, Map.GetSize().X, Map.GetSize().Y, T::Neutral(), .3f * L5);
		P.Label(Map.Min.X + 12.f, Map.Min.Y + 8.f, TEXT("CINDER  ·  LOCAL"), 11, T::Neutral().CopyWithNewOpacity(.8f * L5), 120);
		for (int32 R = 1; R <= 4; ++R)
		{
			P.Arc(MapC + FVector2D(-20.f, 14.f), 22.f * R, 1.f, T::Dim().CopyWithNewOpacity(.18f * L5), 1.f);
		}
		auto Marker = [&](const FVector2D& At, const TCHAR* Name, FLinearColor C, float Show, const TCHAR* Key)
		{
			if (Show <= 0.f)
			{
				return;
			}
			const FLinearColor K = C.CopyWithNewOpacity(Show);
			P.Line(At + FVector2D(0, -9), At + FVector2D(9, 0), K, 2.f);
			P.Line(At + FVector2D(9, 0), At + FVector2D(0, 9), K, 2.f);
			P.Line(At + FVector2D(0, 9), At + FVector2D(-9, 0), K, 2.f);
			P.Line(At + FVector2D(-9, 0), At + FVector2D(0, -9), K, 2.f);
			P.Arc(At, 14.f + 4.f * FMath::Sin(Now * 4.f), 1.f, K.CopyWithNewOpacity(.4f * Show), 1.f);
			P.Label(At.X + 16.f, At.Y - 7.f, Name, 11, K, 100);
			Rec(Key, FBox2D(At - FVector2D(9, 9), At + FVector2D(9, 9)));
		};
		Marker(MapC + FVector2D(-90.f, -10.f), TEXT("CRYSTAL CACHE"), T::Shield(), FMath::Clamp(L5 * 2.f - .5f, 0.f, 1.f), TEXT("cache"));
		Marker(MapC + FVector2D(40.f, 50.f), TEXT("FIELD ANCHOR"), T::Warning(), FMath::Clamp(L5 * 2.f - 1.f, 0.f, 1.f), TEXT("anchor"));
		Rec(TEXT("local_map"), Map);
	}
	return Base + 3;
}
