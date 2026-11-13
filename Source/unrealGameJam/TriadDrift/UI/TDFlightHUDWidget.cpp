// Triad Drift - engineering space HUD (ENG-36, Teammate 2). Spec: docs/eng/hud-redesign.md.

#include "TriadDrift/UI/TDFlightHUDWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/UI/TDUIPaint.h"

namespace TDFlightHUD
{
	using T = FTDFlightHUDTokens;

	const TCHAR* PilotName(ETDShipId Ship)
	{
		return Ship == ETDShipId::Rift ? TEXT("WREN") : Ship == ETDShipId::Lancer ? TEXT("KATO") : TEXT("MOSS");
	}
	const TCHAR* ShipName(ETDShipId Ship)
	{
		return Ship == ETDShipId::Rift ? TEXT("RIFT") : Ship == ETDShipId::Lancer ? TEXT("LANCER") : TEXT("BASTION");
	}

	/** Rounded plates, cached per pixel radius and outline (Slate keeps the brush pointer until the frame renders). */
	const FSlateBrush* RoundedBrush(float RadiusPx, float OutlinePx)
	{
		static TMap<uint32, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
		const int32 R = FMath::Clamp(FMath::RoundToInt(RadiusPx), 1, 255);
		const int32 O = FMath::Clamp(FMath::RoundToInt(OutlinePx * 2.f), 0, 255);
		TSharedPtr<FSlateRoundedBoxBrush>& Brush = Cache.FindOrAdd((uint32(R) << 8) | uint32(O));
		if (!Brush.IsValid())
		{
			Brush = O == 0 ? MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, float(R))
						   : MakeShared<FSlateRoundedBoxBrush>(FLinearColor::Transparent, float(R), FLinearColor::White, O * .5f);
		}
		return Brush.Get();
	}

	/** FTDUIPainter plus the ENG-36 language: rounded plates, outlined text, polylines and icons. */
	struct FPaint : FTDUIPainter
	{
		using FTDUIPainter::FTDUIPainter;

		void Plate(float X, float Y, float W, float H, float Radius, FLinearColor C) const
		{
			FSlateDrawElement::MakeBox(List, Layer, Geo({ X, Y }, { W, H }), RoundedBrush(Radius * Scale, 0.f), ESlateDrawEffect::None, C);
		}
		/** A rounded-rectangle outline as a polyline (the rounded brush with a transparent fill still filled, live). */
		void Outline(float X, float Y, float W, float H, float Radius, float Width, FLinearColor C) const
		{
			const float R = FMath::Min(Radius, FMath::Min(W, H) * .5f);
			TArray<FVector2D> Points;
			const FVector2D Corners[4] = { { X + W - R, Y + R }, { X + W - R, Y + H - R }, { X + R, Y + H - R }, { X + R, Y + R } };
			for (int32 Corner = 0; Corner < 4; ++Corner)
			{
				const float A0 = -PI * .5f + Corner * PI * .5f; // top-right corner starts pointing up, clockwise
				for (int32 I = 0; I <= 5; ++I)
				{
					const float A = A0 + (PI * .5f) * I / 5.f;
					Points.Add((Corners[Corner] + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R) * Scale + Origin);
				}
			}
			const FVector2D First = Points[0]; // a copy: adding an element of the array to itself asserts
			Points.Add(First);
			FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, C, true, Width * Scale);
		}
		FSlateFontInfo Font(const ANSICHAR* Face, int32 Size, int32 Tracking = 0) const
		{
			FSlateFontInfo F = MakeFont(Face, Size, Scale, Tracking);
			F.OutlineSettings.OutlineSize = FMath::Max(1, FMath::RoundToInt(Scale));
			F.OutlineSettings.OutlineColor = T::Plate().CopyWithNewOpacity(.85f);
			return F;
		}
		/** Caps text with a 1 px dark outline (reads over a bright nebula). Align: 0 left, .5 centre, 1 right. */
		void Words(float X, float Y, const FString& S, int32 Size, FLinearColor C, const ANSICHAR* Face = "Bold", float Align = 0.f, int32 Tracking = 60) const
		{
			const FSlateFontInfo F = Font(Face, Size, Tracking);
			const float W = Align > 0.f ? Measure(S, Font(Face, Size, Tracking)) : 0.f;
			FSlateDrawElement::MakeText(List, Layer + 2, Geo({ X - W * Align, Y }, { 1, 1 }), S, F, ESlateDrawEffect::None, C);
		}
		void Poly(const FVector2D& At, std::initializer_list<FVector2D> Local, FLinearColor C, float Width, float Size = 1.f) const
		{
			TArray<FVector2D> Points;
			for (const FVector2D& Q : Local)
			{
				Points.Add((At + Q * Size) * Scale + Origin);
			}
			FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, C, true, Width * Scale);
		}
		/** An arc from angle A0 sweeping Sweep radians (screen angles: 0 = right, +PI/2 = down). */
		void ArcSpan(FVector2D C, float R, float A0, float Sweep, FLinearColor Color, float Width) const
		{
			if (FMath::Abs(Sweep) < 1e-3f)
			{
				return;
			}
			Arc(C, R, Sweep / (2.f * PI), Color, Width, A0);
		}
		/** Original ship marks: Rift crescent, Lancer dart, Bastion hexagon (spec section 5). Size = full height. */
		void ShipIcon(ETDShipId Ship, const FVector2D& C, float Size, FLinearColor Color) const
		{
			const float H = Size * .5f;
			if (Ship == ETDShipId::Rift)
			{
				ArcSpan(C, H, PI * .25f, PI * 1.5f, Color, 2.4f);
				ArcSpan(C + FVector2D(H * .45f, 0), H * .72f, PI * .38f, PI * 1.24f, Color, 1.6f);
			}
			else if (Ship == ETDShipId::Lancer)
			{
				Poly(C, { { -H, H * .8f }, { 0, -H }, { H, H * .8f }, { 0, H * .35f }, { -H, H * .8f } }, Color, 2.2f);
			}
			else
			{
				TArray<FVector2D> Hex;
				for (int32 I = 0; I <= 6; ++I)
				{
					const float A = PI / 6.f + I * PI / 3.f;
					Hex.Add((C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * H) * Scale + Origin);
				}
				FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Hex, ESlateDrawEffect::None, Color, true, 2.2f * Scale);
			}
		}
		/** A plain bar with a dim track. */
		void Bar(float X, float Y, float W, float H, float Value, FLinearColor C, float Opacity) const
		{
			const float V = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0.f;
			Plate(X, Y, W, H, H * .5f, T::Dim().CopyWithNewOpacity(.22f * Opacity));
			if (V > 0.f)
			{
				Plate(X, Y, FMath::Max(H, W * V), H, H * .5f, C.CopyWithNewOpacity(Opacity));
			}
		}
	};

	float Clamp01(float V) { return FMath::IsFinite(V) ? FMath::Clamp(V, 0.f, 1.f) : 0.f; }
}

// ------------------------------------------------------------------------------------------ data

void UTDFlightHUDWidget::SetDataSource(UObject* InSource)
{
	Source = InSource;
	RefreshData();
}

void UTDFlightHUDWidget::RefreshData()
{
	Sample(0.f);
}

bool UTDFlightHUDWidget::IsUsingGamepad() const
{
	if (ForceGamepad >= 0)
	{
		return ForceGamepad > 0;
	}
	const ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(GetOwningPlayer());
	return PC && PC->IsUsingGamepad();
}

void UTDFlightHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Sample(InDeltaTime);
}

void UTDFlightHUDWidget::Sample(float DeltaSeconds)
{
	const float Dt = FMath::Max(0.f, DeltaSeconds);
	Time += Dt;
	const ITDHUDData* Data = Cast<ITDHUDData>(Source.Get());
	View.bHasData = Data != nullptr;
	View.bUsingGamepad = IsUsingGamepad();
	if (!Data)
	{
		View.Chips.Reset();
		return;
	}

	// Squad: fixed order 1, 2, 3 whatever order the source lists them in.
	const TArray<FTDShipHUD> Cards = Data->GetSquadHUD();
	View.Chips.Reset();
	const FTDShipHUD* Flown = nullptr;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FTDShipHUD* Card = Cards.FindByPredicate([Index](const FTDShipHUD& C) { return static_cast<int32>(C.Ship) == Index; });
		if (!Card)
		{
			continue;
		}
		FChip& Chip = View.Chips.AddDefaulted_GetRef();
		Chip.Ship = Card->Ship;
		Chip.bAlive = Card->bAlive;
		Chip.bActive = Card->bPlayerControlled;
		Chip.Hull = Card->bAlive ? TDFlightHUD::Clamp01(Card->Health01) : 0.f;
		Chip.Shield = Card->bAlive ? TDFlightHUD::Clamp01(Card->Shield01) : 0.f;
		Chip.RespawnSeconds = Card->bAlive ? 0.f : FMath::Max(0.f, Card->RespawnSeconds);
		Chip.bTrouble = Card->bAlive && !Card->bPlayerControlled && Chip.Hull < TroubleHull;
		Chip.bPowerReady = Card->bAlive && Card->PowerCooldown01 >= .999f;
		Chip.Key = View.bUsingGamepad ? FString() : FString::FromInt(Index + 1);
		if (Card->bPlayerControlled)
		{
			Flown = Card;
		}
	}
	LayoutChips();

	// Reticle cluster: the flown ship.
	if (Flown)
	{
		const bool bSwitched = Flown->Ship != LastFlown;
		View.Flown = Flown->Ship;
		View.Hull = TDFlightHUD::Clamp01(Flown->Health01);
		View.Shield = TDFlightHUD::Clamp01(Flown->Shield01);
		View.Power = TDFlightHUD::Clamp01(Flown->PowerCooldown01);
		const bool bReady = View.Power >= .999f;
		// The ready pulse marks a power coming off cooldown, not a switch onto a ship whose power was already ready.
		if (bReady && !bWasPowerReady && !bSwitched)
		{
			PulseLeft = PulseSeconds;
		}
		bWasPowerReady = bReady;
		View.bPowerReady = bReady;
		if (FMath::Abs(View.Hull - LastHull) > 1e-3f || FMath::Abs(View.Shield - LastShield) > 1e-3f)
		{
			ArcAwake = LastHull < 0.f ? 0.f : ArcAwakeSeconds; // the first sample is not a change
		}
		LastHull = View.Hull;
		LastShield = View.Shield;
		LastFlown = Flown->Ship;
	}
	ArcAwake = FMath::Max(0.f, ArcAwake - Dt);
	View.ArcOpacity = (View.Hull < .999f || View.Shield < .999f || ArcAwake > 0.f) ? 1.f : ArcRestOpacity;
	PulseLeft = FMath::Max(0.f, PulseLeft - Dt);
	View.PowerPulse = PulseLeft > 0.f ? 1.f - PulseLeft / PulseSeconds : 0.f;
	View.PowerGlyph = View.bUsingGamepad ? TEXT("X") : TEXT("Q");

	// Objective line: the text in caps, the wave's enemies left as the counter; expanded for 3 s when the text changes.
	const FString Objective = Data->GetObjectiveText().ToString().ToUpper();
	if (Objective != LastObjective)
	{
		LastObjective = Objective;
		ObjectiveAge = Objective.IsEmpty() ? 1e6f : 0.f;
	}
	else
	{
		ObjectiveAge += Dt;
	}
	// "WAVE 1 · CLEAR THE NULL ARMADA": the wave goes to the detail line (and the counter), the line keeps the instruction.
	View.Objective = Objective;
	int32 Dot = INDEX_NONE;
	if (Objective.StartsWith(TEXT("WAVE ")) && Objective.FindChar(TCHAR(0x00B7), Dot))
	{
		View.Objective = Objective.Mid(Dot + 1).TrimStartAndEnd();
	}
	const FTDWaveInfo Wave = Data->GetWaveInfo();
	View.Counter = Wave.bWaveActive ? FString::Printf(TEXT("%d LEFT"), FMath::Max(0, Wave.EnemiesAlive)) : FString();
	View.ObjectiveDetail = Wave.WaveNumber <= 0 ? FString()
		: Wave.WaveCount > 0 ? FString::Printf(TEXT("WAVE %d OF %d"), Wave.WaveNumber, Wave.WaveCount)
							 : FString::Printf(TEXT("WAVE %d"), Wave.WaveNumber);
	const float Ease = ObjectiveEaseSeconds;
	const float A = ObjectiveAge;
	View.ObjectiveExpand = Objective.IsEmpty() ? 0.f
		: A < Ease ? A / Ease
		: A < ObjectiveExpandSeconds ? 1.f
		: A < ObjectiveExpandSeconds + Ease ? 1.f - (A - ObjectiveExpandSeconds) / Ease
		: 0.f;
}

void UTDFlightHUDWidget::LayoutChips()
{
	float X = SafeX;
	const float Y = 1080.f - SafeY - ChipHeight;
	for (FChip& Chip : View.Chips)
	{
		Chip.Pos = FVector2D(X, Y);
		Chip.Size = FVector2D(Chip.bActive ? ActiveChipWidth : WingChipWidth, ChipHeight);
		X += Chip.Size.X + ChipGap;
	}
}

// ------------------------------------------------------------------------------------------ paint

int32 UTDFlightHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace TDFlightHUD;
	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!View.bHasData)
	{
		return Base;
	}
	const FPaint P(AllottedGeometry, OutDrawElements, Base + 1);
	const FVector2D C(CenterX, CenterY);
	const FLinearColor Accent = T::Ship(View.Flown);

	// ---- reticle: 2 px dot, four 8 px ticks starting 8 px out
	const FLinearColor Ink = T::Neutral().CopyWithNewOpacity(.9f);
	P.Box(C.X - 1, C.Y - 1, 2, 2, Ink);
	for (const FVector2D D : { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) })
	{
		P.Line(C + D * 8.f, C + D * 16.f, Ink, 2.f);
	}

	// ---- hull (thick inner) and shield (thin outer) arcs, mirrored left and right, draining from the top down
	const float Half = FMath::DegreesToRadians(ArcHalfDeg);
	auto Pair = [&](float R, float Value, FLinearColor Color, float Width)
	{
		const FLinearColor Track = T::Dim().CopyWithNewOpacity(.16f * View.ArcOpacity + .06f);
		P.ArcSpan(C, R, PI - Half, 2.f * Half, Track, Width);	// left track, bottom to top
		P.ArcSpan(C, R, -Half, 2.f * Half, Track, Width);		// right track, top to bottom
		const float Sweep = 2.f * Half * Value;
		const FLinearColor Fill = Color.CopyWithNewOpacity(View.ArcOpacity);
		P.ArcSpan(C, R, PI - Half, Sweep, Fill, Width);			// left: grows up from the bottom end
		P.ArcSpan(C, R, Half - Sweep, Sweep, Fill, Width);		// right: ends at the bottom end
	};
	Pair(HullRadius, View.Hull, T::Hull(View.Hull), 5.f);
	Pair(ShieldRadius, View.Shield, T::Shield(), 3.f);

	// ---- signature power pip: ship-colour ring + glyph when ready, grey ring refilling clockwise when cooling
	{
		const FVector2D Pip = C + FVector2D(PipOffsetX, PipOffsetY);
		const float Pulse = View.PowerPulse > 0.f ? FMath::Sin(View.PowerPulse * PI) * .25f : 0.f;
		const float R = PipRadius * (1.f + Pulse);
		P.Plate(Pip.X - R, Pip.Y - R, 2 * R, 2 * R, R, T::Plate().CopyWithNewOpacity(.55f));
		if (View.bPowerReady)
		{
			if (Pulse > 0.f)
			{
				P.Arc(Pip, R + 4.f, 1.f, Accent.CopyWithNewOpacity(.35f * (Pulse / .25f)), 6.f); // the soft ready glow
			}
			P.Arc(Pip, R, 1.f, Accent, 3.f);
		}
		else
		{
			P.Arc(Pip, R, 1.f, T::Dim().CopyWithNewOpacity(.25f), 3.f);
			P.Arc(Pip, R, View.Power, T::Dim(), 3.f, -PI * .5f);
		}
		P.Words(Pip.X, Pip.Y - 10.f, View.PowerGlyph, 15, T::Neutral().CopyWithNewOpacity(View.bPowerReady ? 1.f : .45f), "Bold", .5f, 0);
	}

	// ---- squad strip, bottom-left of the safe frame, order 1 2 3
	const float Pulse1Hz = .75f + .25f * FMath::Sin(Time * 2.f * PI);
	for (const FChip& Chip : View.Chips)
	{
		const FLinearColor Ship = T::Ship(Chip.Ship);
		const float O = Chip.bActive ? 1.f : .72f;
		const float X = Chip.Pos.X, Y = Chip.Pos.Y, W = Chip.Size.X, H = Chip.Size.Y;
		const float S = StripScale;
		P.Plate(X, Y, W, H, 6.f * S, T::Plate().CopyWithNewOpacity(.62f));
		if (Chip.bActive)
		{
			P.Outline(X, Y, W, H, 6.f * S, 1.5f, Ship.CopyWithNewOpacity(.9f));
			P.Plate(X + 8 * S, Y, W - 16 * S, 3.f, 1.5f, Ship);
		}
		else if (Chip.bTrouble)
		{
			P.Outline(X, Y, W, H, 6.f * S, 2.f, T::Warning().CopyWithNewOpacity(Pulse1Hz));
		}
		// Keycap with the switch key.
		float PenX = X + 8 * S;
		if (!Chip.Key.IsEmpty())
		{
			const float K = 18.f * S;
			P.Outline(PenX, Y + (H - K) * .5f, K, K, 4.f * S, 1.2f, T::Neutral().CopyWithNewOpacity(.6f * O));
			P.Words(PenX + K * .5f, Y + (H - K) * .5f + 2.f, Chip.Key, 14, T::Neutral().CopyWithNewOpacity(O), "Bold", .5f, 0);
			PenX += K + 6 * S;
		}
		// Ship mark, crossed out while down.
		const float Icon = 20.f * S;
		const FVector2D IconC(PenX + Icon * .5f, Y + H * .5f);
		P.ShipIcon(Chip.Ship, IconC, Icon, (Chip.bAlive ? Ship : T::Dim()).CopyWithNewOpacity(O));
		if (!Chip.bAlive)
		{
			P.Line(IconC + FVector2D(-Icon, -Icon) * .45f, IconC + FVector2D(Icon, Icon) * .45f, T::Danger(), 2.f);
			P.Line(IconC + FVector2D(-Icon, Icon) * .45f, IconC + FVector2D(Icon, -Icon) * .45f, T::Danger(), 2.f);
		}
		PenX += Icon + 7 * S;
		const float Right = X + W - 16 * S;
		const float BarW = FMath::Max(10.f, Right - PenX);
		if (Chip.bAlive)
		{
			if (Chip.bActive)
			{
				P.Words(PenX, Y + 7 * S, FString(TDFlightHUD::ShipName(Chip.Ship)), 14, T::Neutral(), "Bold", 0.f, 80);
			}
			const float BarsY = Y + H - 16 * S;
			P.Bar(PenX, BarsY, BarW, 2.f * S, Chip.Shield, T::Shield(), O);
			P.Bar(PenX, BarsY + 4 * S, BarW, 5.f * S, Chip.Hull, T::Hull(Chip.Hull), O);
		}
		else
		{
			P.Words(PenX, Y + H * .5f - 10.f, FString::Printf(TEXT("%.0fs"), FMath::CeilToFloat(Chip.RespawnSeconds)), 15, T::Dim(), "Bold", 0.f, 0);
		}
		// Power-ready dot (top-right): filled in the ship's colour when ready, a dim ring while cooling.
		const FVector2D Dot(X + W - 9 * S, Y + 9 * S);
		if (Chip.bPowerReady)
		{
			P.Plate(Dot.X - 4 * S, Dot.Y - 4 * S, 8 * S, 8 * S, 4 * S, Ship.CopyWithNewOpacity(O));
		}
		else
		{
			P.Arc(Dot, 3.5f * S, 1.f, T::Dim().CopyWithNewOpacity(.6f * O), 1.2f);
		}
		// Trouble badge: a "!" triangle on the chip's top edge.
		if (Chip.bTrouble)
		{
			const FVector2D B(X + W - 26 * S, Y - 2 * S);
			P.Poly(B, { { 0, -9 }, { 9, 7 }, { -9, 7 }, { 0, -9 } }, T::Warning(), 2.f, S);
			P.Words(B.X, B.Y - 8 * S, TEXT("!"), 13, T::Warning(), "Bold", .5f, 0);
		}
	}

	// ---- objective line: top-centre pill, diamond, caps, counter right; expands for 3 s on change
	if (!View.Objective.IsEmpty())
	{
		const float E = FMath::SmoothStep(0.f, 1.f, View.ObjectiveExpand);
		// Grow to fit long text: diamond + text + gap + counter + padding (headless measures 0: the design sizes stand).
		const float TextW = P.Measure(View.Objective, P.Font("Bold", 17, 60));
		const float CountW = View.Counter.IsEmpty() ? 0.f : P.Measure(View.Counter, P.Font("Bold", 15, 40)) + 24.f;
		const float FitW = 34.f + TextW + CountW + 20.f;
		const float BigW = 34.f + P.Measure(View.Objective, P.Font("Bold", 28, 40)) + CountW + 20.f;
		const float W = FMath::Lerp(FMath::Max(ObjectiveWidth, FitW), FMath::Max(ObjectiveExpandedWidth, BigW), E);
		const float H = FMath::Lerp(ObjectiveHeight, ObjectiveExpandedHeight, E);
		const float X = CenterX - W * .5f, Y = SafeY;
		P.Plate(X, Y, W, H, 14.f, T::Plate().CopyWithNewOpacity(FMath::Lerp(.45f, .7f, E)));
		const float TextO = FMath::Lerp(.7f, 1.f, E);
		const FVector2D Dia(X + 18.f, Y + FMath::Lerp(14.f, 26.f, E)); // centred on the 17 px line, then on the 28 px title
		P.Poly(Dia, { { 0, -6 }, { 6, 0 }, { 0, 6 }, { -6, 0 }, { 0, -6 } }, T::Neutral().CopyWithNewOpacity(TextO), 2.f);
		if (E < .5f)
		{
			P.Words(X + 34.f, Y + 4.f, View.Objective, 17, T::Neutral().CopyWithNewOpacity(TextO), "Bold", 0.f, 60);
		}
		else
		{
			P.Words(X + 34.f, Y + 6.f, View.Objective, 28, T::Neutral(), "Bold", 0.f, 40);
			if (!View.ObjectiveDetail.IsEmpty())
			{
				P.Words(X + 34.f, Y + 44.f, View.ObjectiveDetail, 16, T::Dim(), "Bold", 0.f, 80);
			}
		}
		if (!View.Counter.IsEmpty())
		{
			P.Words(X + W - 16.f, Y + 5.f, View.Counter, 15, T::Neutral().CopyWithNewOpacity(TextO), "Bold", 1.f, 40);
		}
	}
	return Base + 4;
}
