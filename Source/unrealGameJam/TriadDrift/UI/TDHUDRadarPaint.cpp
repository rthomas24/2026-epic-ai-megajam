// Triad Drift - the flight HUD's radar (issue #160; Teammate 6). See TDHUDRadarPaint.h.

#include "TriadDrift/UI/TDHUDRadarPaint.h"
#include "TriadDrift/UI/TDHUDModel.h"
#include "TriadDrift/UI/TDUIPaint.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "HAL/IConsoleManager.h"

namespace TDHUDRadar
{
	using Pal = FTDUIPalette;

	// Design frame (HUD-CONCEPT.md section 4): centre (1684, 886), radius 140; the wave header 280x34 above it.
	static constexpr float CX = 1684.f, CY = 886.f, Radius = 140.f, HeaderH = 34.f, HeaderGap = 12.f;
	// Enemy kinds as UTDFlightHUDWidget::KindOf numbers them.
	enum : uint8 { KindOther = 0, KindFighter = 1, KindInterceptor = 2, KindBomber = 3, KindGunship = 4, KindCarrier = 5 };

	/** One scratch point list, reused every draw call (no allocation once it has grown). */
	static TArray<FVector2D>& Scratch()
	{
		static TArray<FVector2D> Points;
		Points.Reset();
		return Points;
	}

	/** T2's small-screen rule (the squad panel's, 3f6b57c3), from the painter's own geometry: real pixels per design unit =
	 *  the frame fit times UMG's DPI scale. Under 900 / 1080 the radar draws at 85% of its 1080 pixel size and its text at
	 *  the 1080 pixel size; otherwise 1. */
	static void Fit(const FTDUIPainter& P, float& OutK, float& OutFont)
	{
		const float Real = P.Scale * FMath::Max(.01f, P.G.GetAccumulatedLayoutTransform().GetScale());
		const bool bSmall = Real > 0.f && Real < 900.f / 1080.f;
		OutK = bSmall ? .85f / Real : 1.f;
		OutFont = bSmall ? 1.f / Real : 1.f;
	}

	struct FRadarPaint
	{
		const FTDUIPainter& P;
		FVector2D C;
		float R;
		/** Design-unit multiplier for sizes (T2's rule, see Fit) and for font sizes. */
		float Shrink;
		float FontK = 1.f;

		FVector2D Px(const FVector2D& D) const { return D * P.Scale + P.Origin; }
		void Lines(const TArray<FVector2D>& Pts, FLinearColor Col, float Width, int32 LayerAdd = 1) const
		{
			FSlateDrawElement::MakeLines(P.List, P.Layer + LayerAdd, P.G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, P.Faded(Col), true,
				FMath::Max(1.f, Width * P.Scale));
		}
		void Seg(const FVector2D& A, const FVector2D& B, FLinearColor Col, float Width) const
		{
			TArray<FVector2D>& Pts = Scratch();
			Pts.Add(Px(A));
			Pts.Add(Px(B));
			Lines(Pts, Col, Width);
		}
		/** A closed or open shape from local points (design units, around At). */
		void Shape(const FVector2D& At, std::initializer_list<FVector2D> Local, FLinearColor Col, float Width, bool bClose) const
		{
			TArray<FVector2D>& Pts = Scratch();
			for (const FVector2D& Q : Local) { Pts.Add(Px(At + Q)); }
			if (bClose && Pts.Num() > 0) { const FVector2D First = Pts[0]; Pts.Add(First); } // never Add an element of the array itself by reference (asserts, or reads freed memory in Shipping)
			Lines(Pts, Col, Width);
		}
		void Circle(const FVector2D& At, float Rad, FLinearColor Col, float Width, int32 Segments = 40) const
		{
			ArcAt(At, Rad, 0.f, 2.f * PI, Col, Width, Segments);
		}
		/** Angles in radians, 0 = up (forward), clockwise. */
		void ArcAt(const FVector2D& At, float Rad, float A0, float Sweep, FLinearColor Col, float Width, int32 Segments = 24) const
		{
			TArray<FVector2D>& Pts = Scratch();
			for (int32 I = 0; I <= Segments; ++I)
			{
				const float A = A0 + Sweep * I / Segments;
				Pts.Add(Px(At + FVector2D(FMath::Sin(A), -FMath::Cos(A)) * Rad));
			}
			Lines(Pts, Col, Width);
		}
		void Ellipse(const FVector2D& At, float Rx, float Ry, FLinearColor Col, float Width) const
		{
			TArray<FVector2D>& Pts = Scratch();
			for (int32 I = 0; I <= 20; ++I)
			{
				const float A = 2.f * PI * I / 20;
				Pts.Add(Px(At + FVector2D(FMath::Cos(A) * Rx, FMath::Sin(A) * Ry)));
			}
			Lines(Pts, Col, Width);
		}
		void Dot(const FVector2D& At, float Size, FLinearColor Col) const
		{
			P.Box(At.X - Size * .5f, At.Y - Size * .5f, Size, Size, Col);
		}
		/** Text at design (X, Y), Align 0 left / .5 centre / 1 right; at least 12 px on screen. */
		void Text(float X, float Y, const FString& S, int32 Size, FLinearColor Col, const ANSICHAR* Face, float Align) const
		{
			const int32 Design = FMath::Max(1, FMath::RoundToInt(Size * FontK));
			const FSlateFontInfo Font = FTDUIPainter::MakeFont(Face, Design, P.Scale, 20);
			const float W = Align > 0.f ? P.Measure(S, Font) : 0.f;
			FSlateDrawElement::MakeText(P.List, P.Layer + 2, P.Geo({ X - W * Align, Y }, { 1, 1 }), S, Font, ESlateDrawEffect::None, P.Faded(Col));
		}
	};

	/** Horizontal metres -> radius on the scope: half the range takes 2/3 of the radius (close fights get the space), the
	 *  rest is compressed (log) into the outer third. */
	static float RadiusFor(float Metres, float Range, float R)
	{
		const float T = FMath::Clamp(Metres / FMath::Max(Range, 1.f), 0.f, 1.f);
		if (T <= .5f)
		{
			return R * (2.f / 3.f) * (T / .5f);
		}
		const float U = FMath::Loge(1.f + 9.f * (T - .5f) / .5f) / FMath::Loge(10.f);	// 0..1
		return R * (2.f / 3.f + .27f * U);
	}

	/** An enemy glyph by kind (the on-screen marker shapes at radar scale). */
	static void Enemy(const FRadarPaint& D, const FVector2D& At, uint8 Kind, FLinearColor Col, float S)
	{
		switch (Kind)
		{
		case KindFighter:		D.Shape(At, { { -5 * S, 3 * S }, { 0, -3 * S }, { 5 * S, 3 * S } }, Col, 1.8f, false); break;	// chevron
		case KindInterceptor:	D.Shape(At, { { 0, -6 * S }, { 2.5f * S, 5 * S }, { 0, 2.5f * S }, { -2.5f * S, 5 * S } }, Col, 1.5f, true); break;	// narrow dart
		case KindBomber:		D.Shape(At, { { -8 * S, 4 * S }, { 0, -4 * S }, { 8 * S, 4 * S } }, Col, 1.8f, true); break;	// wide wedge
		case KindGunship:		D.Shape(At, { { 0, -7 * S }, { 7 * S, 0 }, { 0, 7 * S }, { -7 * S, 0 } }, Col, 1.6f, true); break;	// diamond
		case KindCarrier:
		{
			const float H = 8 * S;
			D.Shape(At, { { H, 0 }, { H * .5f, H * .866f }, { -H * .5f, H * .866f }, { -H, 0 }, { -H * .5f, -H * .866f }, { H * .5f, -H * .866f } }, Col, 1.6f, true);	// hexagon
			D.Dot(At, 2.5f * S, Col);
			break;
		}
		default:				D.Shape(At, { { 0, -4 * S }, { 4 * S, 3 * S }, { -4 * S, 3 * S } }, Col, 1.5f, true); break;	// small triangle
		}
	}

	static void Pickup(const FRadarPaint& D, const FVector2D& At, uint8 Kind, float S)
	{
		const ETDPickupType Type = static_cast<ETDPickupType>(Kind);
		const FLinearColor Col = ATDPickup::ColorOf(Type);
		switch (Type)
		{
		case ETDPickupType::Hull:		D.Seg(At + FVector2D(-4 * S, 0), At + FVector2D(4 * S, 0), Col, 2.f); D.Seg(At + FVector2D(0, -4 * S), At + FVector2D(0, 4 * S), Col, 2.f); break;	// plus
		case ETDPickupType::Shield:		D.Circle(At, 4 * S, Col, 1.6f, 12); break;	// disc
		case ETDPickupType::Power:		D.Shape(At, { { -4 * S, 3 * S }, { -1.5f * S, -4 * S }, { 1 * S, 3 * S } }, Col, 1.6f, false); D.Shape(At, { { 0.5f * S, 3 * S }, { 3 * S, -4 * S }, { 5.5f * S, 3 * S } }, Col, 1.6f, false); break;	// double point
		case ETDPickupType::Overcharge:	D.Shape(At, { { -4 * S, -4 * S }, { 4 * S, -4 * S }, { 4 * S, 4 * S }, { -4 * S, 4 * S } }, Col, 1.6f, true); break;	// square
		default:						D.Shape(At, { { -4 * S, 3 * S }, { 0, -4 * S }, { 4 * S, 3 * S }, { 0, 1 * S } }, Col, 1.6f, true); break;	// arrowhead (Boost)
		}
	}

	/** A pulsing arc on the rim at a bearing, 30 degrees wide, with an inward arrow (warp-in solid, incoming dashed). */
	static void RimArc(const FRadarPaint& D, float Bearing, float Pulse, bool bDashed)
	{
		const FLinearColor Col = Pal::Hostile().CopyWithNewOpacity(.45f + .55f * Pulse);
		const float Half = FMath::DegreesToRadians(15.f);
		if (bDashed)
		{
			for (int32 I = 0; I < 3; ++I) { D.ArcAt(D.C, D.R - 1.f, Bearing - Half + I * (2.f * Half / 3.f), 2.f * Half / 3.f * .6f, Col, 4.f, 4); }
		}
		else
		{
			D.ArcAt(D.C, D.R - 1.f, Bearing - Half, 2.f * Half, Col, 4.f, 10);
		}
		const FVector2D Dir(FMath::Sin(Bearing), -FMath::Cos(Bearing));
		const FVector2D Side(-Dir.Y, Dir.X);
		const FVector2D Tip = D.C + Dir * (D.R - 16.f);
		D.Shape(Tip, { Dir * 7.f + Side * 5.f, FVector2D::ZeroVector, Dir * 7.f - Side * 5.f }, Col, 1.8f, false);
	}

	/** What the last paint drew, by contact type (TD.Radar.Stats; dev captures: "was a missile on the scope in that frame"). */
	static int32 GDrawn[8] = { 0 };
	static int32 GDrawnFrame[8] = { 0 };
#if !UE_BUILD_SHIPPING
	static FAutoConsoleCommand CmdRadarStats(TEXT("TD.Radar.Stats"),
		TEXT("Log what the radar drew on its last paint, by type: enemies, missiles, wingmen, objectives, pickups, gates, warp-ins, incoming groups"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UE_LOG(LogTemp, Display, TEXT("TD.Radar.Stats: enemies %d, missiles %d, wingmen %d, objectives %d, pickups %d, gates %d, warp-ins %d, incoming %d"),
				GDrawn[0], GDrawn[1], GDrawn[2], GDrawn[3], GDrawn[4], GDrawn[5], GDrawn[6], GDrawn[7]);
		}));
#endif

	/** Strings rebuilt only when what they show changes. */
	struct FCache
	{
		bool bNextWave = false;
		int32 Wave = -1, Count = -1, Next = -1, NextIn = -1, Breather = -1;
		float Range = -1.f;
		FString WaveText, NextText, BreatherText, RangeText, GenText;
		int32 GenDown = -1, GenTotal = -1;
		bool bGenReactor = false;
		int32 GenPhase = -1;
	};
	static FCache& Cache()
	{
		static FCache C;
		return C;
	}
}

FBox2D TDHUDRadarBox(const FTDUIPainter& P)
{
	using namespace TDHUDRadar;
	float K = 1.f, Font = 1.f;
	Fit(P, K, Font);
	const float R = Radius * K;
	const FVector2D C(CX + Radius - R, CY + Radius - R);	// anchored to the bottom-right corner of the safe frame
	return FBox2D(FVector2D(C.X - R, C.Y - R - HeaderGap - HeaderH * Font), FVector2D(C.X + R, C.Y + R));
}

void TDPaintHUDRadar(const FTDUIPainter& P, const FTDHUDModel& Model, const FTDHUDRadarInput& In)
{
	using namespace TDHUDRadar;
	float Shrink = 1.f, FontK = 1.f;
	Fit(P, Shrink, FontK);
	const float R = Radius * Shrink;
	const FRadarPaint D{ P, FVector2D(CX + Radius - R, CY + Radius - R), R, Shrink, FontK };	// anchored to the bottom-right of the safe frame
	const FVector2D& C = D.C;
	const float S = Shrink;	// glyph scale
	const float Range = FMath::Max(Model.RadarRange, 1.f);

	// ---- plate: a dark disc (Panel), a soft rim, rings at 1/3 and 2/3, cardinal ticks, the camera's 70 degree view cone
	// Built once and never written again: the render thread reads this brush after the paint returns.
	static const FSlateRoundedBoxBrush Disc = []()
	{
		FSlateRoundedBoxBrush B(FLinearColor::White, 0.f);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		return B;
	}();
	FSlateDrawElement::MakeBox(P.List, P.Layer, P.Geo({ C.X - R, C.Y - R }, { 2 * R, 2 * R }), &Disc, ESlateDrawEffect::None,
		P.Faded(Pal::Panel().CopyWithNewOpacity(.88f)));	// readable over bright nebula (T7, Lead)
	const float ConeHalf = FMath::DegreesToRadians(35.f);
	for (int32 I = 0; I < 6; ++I)	// the cone: a faint fan
	{
		D.ArcAt(C, R * (I + 1) / 6.f, -ConeHalf, 2.f * ConeHalf, Pal::Ink().CopyWithNewOpacity(.035f), R / 6.f + 1.f, 8);
	}
	D.Seg(C, C + FVector2D(FMath::Sin(-ConeHalf), -FMath::Cos(-ConeHalf)) * R, Pal::Muted().CopyWithNewOpacity(.16f), 1.f);
	D.Seg(C, C + FVector2D(FMath::Sin(ConeHalf), -FMath::Cos(ConeHalf)) * R, Pal::Muted().CopyWithNewOpacity(.16f), 1.f);
	D.Circle(C, R * (1.f / 3.f), Pal::Muted().CopyWithNewOpacity(.30f), 1.f);
	D.Circle(C, R * (2.f / 3.f), Pal::Muted().CopyWithNewOpacity(.34f), 1.f);
	D.Circle(C, R, Pal::Ink().CopyWithNewOpacity(.55f), 1.5f, 56);	// a thin light rim
	for (int32 I = 0; I < 4; ++I)
	{
		const float A = I * PI * .5f;
		const FVector2D Dir(FMath::Sin(A), -FMath::Cos(A));
		D.Seg(C + Dir * (R - 7.f), C + Dir * R, Pal::Muted().CopyWithNewOpacity(I == 0 ? .7f : .4f), I == 0 ? 2.f : 1.2f);
	}
	FCache& K = Cache();
	if (K.Range != Range)
	{
		K.Range = Range;
		K.RangeText = Range >= 1000.f ? FString::Printf(TEXT("%.1f KM"), Range / 1000.f) : FString::Printf(TEXT("%.0f M"), Range);
	}

	// ---- contacts (the model holds at most MaxContacts, nearest first after the must-shows)
	const float Flash = FMath::Fmod(In.Time * 2.5f, 1.f) < .55f ? 1.f : .25f;	// missiles: 2.5 Hz
	const float Pulse2 = .5f + .5f * FMath::Sin(In.Time * 2.f * 2.f * PI);		// warp-ins: 2 Hz
	const float Pulse1 = .5f + .5f * FMath::Sin(In.Time * 2.f * PI);			// trouble ring: 1 Hz
	const float Level = 50.f;	// within 50 m up or down: no stem
	for (int32& N : GDrawnFrame) { N = 0; }
	// Two passes: everything else first, missiles last and on the top layer, so one is never lost under enemy blips.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	for (const FTDRadarContact& Ct : Model.Contacts)
	{
		if ((Ct.Type == ETDRadarType::Missile) != (Pass == 1)) { continue; }
		const float Flat = (float)Ct.Plane.Size();
		const float Bearing = FMath::Atan2((float)Ct.Plane.X, (float)Ct.Plane.Y);	// 0 = forward (up), clockwise
		const FVector2D Dir(FMath::Sin(Bearing), -FMath::Cos(Bearing));
		const bool bBig = Ct.Type == ETDRadarType::Objective || Ct.Type == ETDRadarType::Gate || Ct.Type == ETDRadarType::Incoming
			|| Ct.Type == ETDRadarType::WarpIn || (Ct.Type == ETDRadarType::Enemy && Ct.Kind == KindCarrier);
		if (Ct.bBeyond && !bBig)
		{
			continue;	// small things beyond range are not pinned (the edge chevrons on screen cover them)
		}
		if (Ct.Type == ETDRadarType::Incoming || Ct.Type == ETDRadarType::WarpIn)
		{
			RimArc(D, Bearing, Pulse2, Ct.Type == ETDRadarType::Incoming);	// announced arrivals only: the model lists nothing else
			if (Ct.bBeyond || Ct.Type == ETDRadarType::Incoming) { continue; }
		}
		// Never on the player's dart: anything closer than about 150 m sits on a small ring round it, at its bearing.
		const float Rad = Ct.bBeyond ? R - 8.f : FMath::Max(RadiusFor(Flat, Range, R), 20.f * S);
		const FVector2D At = C + Dir * Rad;
		// Height: above = full brightness, stem hangs down to the foot; below = dimmed, stem rises; level: none.
		const bool bAbove = Ct.Up > Level, bBelow = Ct.Up < -Level;
		const float Stem = FMath::Min(18.f * S, FMath::Abs(Ct.Up) / Range * R);
		const float Bright = bBelow ? .55f : 1.f;
		const FVector2D Blip = At + FVector2D(0.f, bAbove ? -Stem : bBelow ? Stem : 0.f);
		if (!Ct.bBeyond && (bAbove || bBelow) && Stem >= 2.f)
		{
			D.Seg(At, Blip, Pal::Muted().CopyWithNewOpacity(.5f * Bright), 1.f);
			D.Dot(At, 1.6f, Pal::Muted().CopyWithNewOpacity(.7f * Bright));	// the foot on your plane
		}
		++GDrawnFrame[FMath::Clamp((int32)Ct.Type, 0, 7)];
		const float Hollow = Ct.bBeyond ? .6f : 1.f;	// pinned to the rim: drawn fainter, hollow shapes
		switch (Ct.Type)
		{
		case ETDRadarType::Enemy:
			Enemy(D, Blip, Ct.Kind, Pal::Hostile().CopyWithNewOpacity(Bright * Hollow), S);
			// No ring on attackers: in a close fight the rings merged into a blob over the player's dart (played, Nov 14).
			break;
		case ETDRadarType::Missile:
		{
			// A white core (nested squares, the top layer), a red box, a flashing ring and a short trail: bigger than any
			// fighter blip (played, Nov 14: the 4 px dot was lost in a close fight).
			auto Square = [&D, &Blip](float H, FLinearColor Col, float Width)
			{
				TArray<FVector2D>& Pts = Scratch();
				for (const FVector2D Q : { FVector2D(-H, -H), FVector2D(H, -H), FVector2D(H, H), FVector2D(-H, H), FVector2D(-H, -H) }) { Pts.Add(D.Px(Blip + Q)); }
				D.Lines(Pts, Col, Width, 2);
			};
			Square(1.2f * S, Pal::Ink().CopyWithNewOpacity(Flash), 2.4f);
			Square(2.6f * S, Pal::Ink().CopyWithNewOpacity(Flash), 1.6f);
			Square(5.f * S, Pal::Hostile().CopyWithNewOpacity(Flash), 1.6f);
			D.Circle(Blip, 10.f * S, Pal::Hostile().CopyWithNewOpacity(.75f * Flash), 1.4f, 16);
			D.Seg(Blip, Blip + (Blip - C).GetSafeNormal() * 12.f * S, Pal::Hostile().CopyWithNewOpacity(.5f * Flash), 1.4f);	// a short trail
		}
			break;
		case ETDRadarType::Wingman:
		{
			const ETDShipId Ship = static_cast<ETDShipId>(Ct.Kind);
			const FLinearColor Col = (Ship == ETDShipId::Rift ? Pal::Rift() : Ship == ETDShipId::Lancer ? Pal::Lancer() : Pal::Bastion()).CopyWithNewOpacity(Bright);
			D.Shape(Blip, { { 0, -6 * S }, { 4.5f * S, 5 * S }, { 0, 2.5f * S }, { -4.5f * S, 5 * S } }, Col, 1.5f, true);	// hollow dart
			const int32 Index = (int32)Ship;
			if (Index >= 0 && Index < 3 && Model.Squad[Index].bTrouble)
			{
				D.Circle(Blip, 9.f * S, Pal::Bastion().CopyWithNewOpacity(.35f + .5f * Pulse1), 1.2f, 16);	// in trouble: an amber ring at 1 Hz
			}
			break;
		}
		case ETDRadarType::Objective:
			D.Shape(Blip, { { 0, -7 * S }, { 7 * S, 0 }, { 0, 7 * S }, { -7 * S, 0 } }, Pal::Ink().CopyWithNewOpacity(Hollow), 1.6f, true);
			D.Dot(Blip, 2.5f * S, Pal::Ink().CopyWithNewOpacity(Hollow));
			break;
		case ETDRadarType::Pickup:
			Pickup(D, Blip, Ct.Kind, S);
			break;
		case ETDRadarType::Gate:
			D.Ellipse(Blip, 8.f * S, 5.f * S, Pal::Muted().CopyWithNewOpacity(Hollow), 1.5f);
			D.Ellipse(Blip, 4.f * S, 2.5f * S, Pal::Muted().CopyWithNewOpacity(.7f * Hollow), 1.2f);
			break;
		case ETDRadarType::WarpIn:
			D.Circle(Blip, 6.f * S * (1.f + .3f * Pulse2), Pal::Hostile().CopyWithNewOpacity(.4f + .5f * Pulse2), 1.5f, 16);
			break;
		default:
			break;
		}
	}

	for (int32 I = 0; I < 8; ++I) { GDrawn[I] = GDrawnFrame[I]; }

	// ---- the range label, after the contacts and on its own small plate so a rim blip never covers it
	{
		const float LW = 66.f * D.FontK, LH = 16.f * D.FontK, LY = C.Y + R - 22.f * D.FontK;
		// On the top layer (the blips' lines are one layer above the plate's): nothing draws over the label.
		FSlateDrawElement::MakeBox(P.List, P.Layer + 2, P.Geo({ C.X - LW * .5f, LY }, { LW, LH }), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None,
			P.Faded(Pal::Panel().CopyWithNewOpacity(.95f)));
		D.Text(C.X, LY + 1.f * D.FontK, K.RangeText, 12, Pal::Muted().CopyWithNewOpacity(.9f), "Mono", .5f);
	}

	// ---- you: a filled dart at the centre in the flown ship's colour, with a soft glow
	if (In.bHasFlown)
	{
		const FLinearColor You = In.Flown == ETDShipId::Rift ? Pal::Rift() : In.Flown == ETDShipId::Lancer ? Pal::Lancer() : Pal::Bastion();
		D.Shape(C, { { 0, -8 * S }, { 6 * S, 7 * S }, { 0, 3.5f * S }, { -6 * S, 7 * S } }, You.CopyWithNewOpacity(.25f), 5.f, true);	// glow
		for (float W = 1.f; W <= 4.f; W += 1.5f)	// filled: nested darts
		{
			const float F = 1.f - (W - 1.f) / 5.f;
			D.Shape(C, { { 0, -8 * S * F }, { 6 * S * F, 7 * S * F }, { 0, 3.5f * S * F }, { -6 * S * F, 7 * S * F } }, You, 2.f, true);
		}
	}

	// ---- wave header above the radar: WAVE n / N, a pip per wave, the next group (+count, seconds) or NEXT WAVE
	// Only during a wave or its breather (after wave 5 the director reports wave 0: it read "WAVE 1 / 5", #103).
	const bool bWaves = In.Wave.WaveCount > 0 && In.Wave.WaveNumber > 0 && (In.Wave.bWaveActive || In.Wave.bInBreather);
	if (!bWaves && (In.GeneratorsTotal > 0 || In.bReactorExposed))
	{
		// The Dominion assault: GENERATORS n / N with a pip each (down = filled), then REACTOR.
		const float HH = HeaderH * D.FontK;
		const float HX = C.X - R, HY = C.Y - R - HeaderGap - HH, HW = 2.f * R;
		P.Box(HX, HY, HW, HH, Pal::Panel().CopyWithNewOpacity(.88f));
		D.Seg(FVector2D(HX, HY + HH), FVector2D(HX + HW * .4f, HY + HH), Pal::Hostile().CopyWithNewOpacity(.55f), 1.5f);
		const bool bReactor = In.bReactorExposed || In.GeneratorsDown >= In.GeneratorsTotal;
		if (K.GenDown != In.GeneratorsDown || K.GenTotal != In.GeneratorsTotal || K.bGenReactor != bReactor || K.GenPhase != In.ReactorPhase)
		{
			K.bGenReactor = bReactor;
			K.GenDown = In.GeneratorsDown;
			K.GenTotal = In.GeneratorsTotal;
			K.GenPhase = In.ReactorPhase;
			K.GenText = !bReactor ? FString::Printf(TEXT("GENERATORS %d / %d"), K.GenDown, K.GenTotal)
				: In.ReactorPhases > 0 && In.ReactorPhase > 0 && In.ReactorPhase <= In.ReactorPhases ? FString::Printf(TEXT("REACTOR  %d / %d"), In.ReactorPhase, In.ReactorPhases)
				: FString(TEXT("REACTOR"));
		}
		D.Text(HX + 10.f, HY + 7.f * D.FontK, K.GenText, 14, bReactor ? Pal::Hostile() : Pal::Ink(), "Bold", 0.f);
		if (!bReactor)
		{
			const int32 Pips = FMath::Min(In.GeneratorsTotal, 8);
			for (int32 I = 0; I < Pips; ++I)
			{
				const FVector2D Pip(HX + HW - 14.f * S - (Pips - 1 - I) * 13.f * S, HY + HH * .5f);
				if (I < In.GeneratorsDown) { D.Dot(Pip, 7.f * S, Pal::Ink().CopyWithNewOpacity(.85f)); }
				else { D.Shape(Pip, { { 0, -4.5f * S }, { 4.5f * S, 0 }, { 0, 4.5f * S }, { -4.5f * S, 0 } }, Pal::Hostile(), 1.4f, true); }	// still up: a hollow diamond
			}
		}
	}
	if (bWaves)
	{
		const float HH = HeaderH * D.FontK;	// the header grows with its text
		const float HX = C.X - R, HY = C.Y - R - HeaderGap - HH, HW = 2.f * R;
		P.Box(HX, HY, HW, HH, Pal::Panel().CopyWithNewOpacity(.88f));
		D.Seg(FVector2D(HX, HY + HH), FVector2D(HX + HW * .4f, HY + HH), Pal::Hostile().CopyWithNewOpacity(.55f), 1.5f);
		// In a breather the header names the wave that is coming, as the objective line does (Shipping check, Nov 14: it
		// kept "WAVE 1 / 5" while the objective read "Wave 2 incoming").
		const bool bNext = In.Wave.bInBreather && In.Wave.WaveNumber < In.Wave.WaveCount;
		const int32 Shown = bNext ? In.Wave.WaveNumber + 1 : In.Wave.WaveNumber;
		if (K.Wave != Shown || K.Count != In.Wave.WaveCount || K.bNextWave != bNext)
		{
			K.Wave = Shown;
			K.Count = In.Wave.WaveCount;
			K.bNextWave = bNext;
			K.WaveText = bNext ? FString::Printf(TEXT("NEXT WAVE %d / %d"), Shown, K.Count) : FString::Printf(TEXT("WAVE %d / %d"), Shown, K.Count);
		}
		D.Text(HX + 10.f, HY + 7.f * D.FontK, K.WaveText, 14, Pal::Ink(), "Bold", 0.f);
		const FString* TimerText = nullptr;
		// The next group from T4's GetPendingGroups (Incoming contacts), soonest first.
		const FTDRadarContact* Next = nullptr;
		for (const FTDRadarContact& Ct : Model.Contacts)
		{
			if (Ct.Type == ETDRadarType::Incoming && (!Next || Ct.Seconds < Next->Seconds)) { Next = &Ct; }
		}
		if (In.Wave.bInBreather && !Next)
		{
			const int32 Secs = FMath::CeilToInt(FMath::Max(0.f, In.Wave.BreatherRemaining));
			if (K.Breather != Secs) { K.Breather = Secs; K.BreatherText = FString::Printf(TEXT("%d:%02d"), Secs / 60, Secs % 60); }
			D.Text(HX + HW - 10.f, HY + 8.f * D.FontK, K.BreatherText, 12, Pal::Muted(), "Mono", 1.f);
			TimerText = &K.BreatherText;
		}
		else if (Next)
		{
			const int32 Secs = FMath::CeilToInt(FMath::Max(0.f, Next->Seconds));
			if (K.Next != Next->Count || K.NextIn != Secs) { K.Next = Next->Count; K.NextIn = Secs; K.NextText = FString::Printf(TEXT("+%d %d:%02d"), Next->Count, Secs / 60, Secs % 60); }
			D.Text(HX + HW - 10.f, HY + 8.f * D.FontK, K.NextText, 12, Pal::Hostile(), "Mono", 1.f);
			TimerText = &K.NextText;
		}
		const float TimerW = TimerText ? P.Measure(*TimerText, FTDUIPainter::MakeFont("Mono", FMath::Max(1, FMath::RoundToInt(12 * D.FontK)), P.Scale, 20)) + 18.f : 0.f;
		// Pips: cleared = filled ink, current = hostile, to come = hollow.
		const int32 Pips = FMath::Min(In.Wave.WaveCount, 8);
		// After the wave text, wherever it ends (it is larger on small screens).
		const float PipX0 = HX + 10.f + FMath::Max(HW * .34f, P.Measure(K.WaveText, FTDUIPainter::MakeFont("Bold", FMath::Max(1, FMath::RoundToInt(14 * D.FontK)), P.Scale, 20))) + 14.f * S;
		// The pips give way to the timer when the header is too narrow for both (720p, "NEXT WAVE", a long timer).
		const bool bPipsFit = PipX0 + Pips * 11.f * S <= HX + HW - 10.f - TimerW;
		for (int32 I = 0; bPipsFit && I < Pips; ++I)
		{
			const FVector2D Pip(PipX0 + I * 11.f * S, HY + HH * .5f);
			const int32 Number = I + 1;
			if (Number < Shown) { D.Dot(Pip, 6.f * S, Pal::Ink().CopyWithNewOpacity(.85f)); }
			else if (Number == Shown && !bNext) { D.Dot(Pip, 7.f * S, Pal::Hostile()); }
			else if (Number == Shown) { D.Shape(Pip, { { -3.5f * S, -3.5f * S }, { 3.5f * S, -3.5f * S }, { 3.5f * S, 3.5f * S }, { -3.5f * S, 3.5f * S } }, Pal::Hostile(), 1.4f, true); }	// coming: hollow, hostile
			else { D.Shape(Pip, { { -3 * S, -3 * S }, { 3 * S, -3 * S }, { 3 * S, 3 * S }, { -3 * S, 3 * S } }, Pal::Muted().CopyWithNewOpacity(.7f), 1.f, true); }
		}
	}
}
