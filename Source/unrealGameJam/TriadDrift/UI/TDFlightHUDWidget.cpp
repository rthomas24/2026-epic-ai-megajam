// Triad Drift - engineering space HUD (ENG-36, Teammate 2). Spec: docs/eng/hud-redesign.md.

#include "TriadDrift/UI/TDFlightHUDWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Input/TDHintComponent.h"
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
		/** An arrowhead pointing along Angle (screen radians). Filled = solid wedge (stacked strokes), else an outline. */
		void Chevron(const FVector2D& At, float Angle, float Size, FLinearColor Color, bool bFilled) const
		{
			const FVector2D F(FMath::Cos(Angle), FMath::Sin(Angle));
			const FVector2D S(-F.Y, F.X);
			const FVector2D Tip = At + F * Size * .5f;
			const FVector2D L = At - F * Size * .5f + S * Size * .45f;
			const FVector2D R = At - F * Size * .5f - S * Size * .45f;
			const FVector2D Notch = At - F * Size * .2f;
			if (bFilled)
			{
				for (int32 I = 0; I <= 4; ++I)
				{
					const float T = I / 4.f;
					Line(Tip, FMath::Lerp(L, R, T), Color, 2.5f);
				}
			}
			TArray<FVector2D> Points;
			for (const FVector2D& Q : { Tip, L, Notch, R, Tip })
			{
				Points.Add(Q * Scale + Origin);
			}
			FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, 2.f * Scale);
		}
		/** A ready / cooling ring pip with a glyph (signature, ability, second power). */
		void Pip(const FVector2D& At, float Radius, float Value, bool bReady, FLinearColor Accent, const FString& Glyph, int32 GlyphSize) const
		{
			Plate(At.X - Radius, At.Y - Radius, 2 * Radius, 2 * Radius, Radius, T::Plate().CopyWithNewOpacity(.55f));
			if (bReady)
			{
				Arc(At, Radius, 1.f, Accent, 3.f);
			}
			else
			{
				Arc(At, Radius, 1.f, T::Dim().CopyWithNewOpacity(.25f), 3.f);
				Arc(At, Radius, Value, T::Dim(), 3.f, -PI * .5f);
			}
			Words(At.X, At.Y - GlyphSize * .62f, Glyph, GlyphSize, T::Neutral().CopyWithNewOpacity(bReady ? 1.f : .45f), "Bold", .5f, 0);
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
	if (MyGeometry.GetLocalSize().X > 1.f && MyGeometry.GetLocalSize().Y > 1.f)
	{
		ViewportSize = MyGeometry.GetLocalSize();
	}
	Sample(InDeltaTime);
}

void UTDFlightHUDWidget::NativeDestruct()
{
	if (ATDSpacePlayerController* PC = BoundPC.Get())
	{
		PC->OnHyperdriveInput.RemoveDynamic(this, &UTDFlightHUDWidget::HandleHyperdriveInput);
	}
	BoundPC.Reset();
	Super::NativeDestruct();
}

void UTDFlightHUDWidget::BindController()
{
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(GetOwningPlayer());
	if (PC == BoundPC.Get())
	{
		return;
	}
	if (ATDSpacePlayerController* Old = BoundPC.Get())
	{
		Old->OnHyperdriveInput.RemoveDynamic(this, &UTDFlightHUDWidget::HandleHyperdriveInput);
	}
	BoundPC = PC;
	if (PC)
	{
		PC->OnHyperdriveInput.AddUniqueDynamic(this, &UTDFlightHUDWidget::HandleHyperdriveInput);
	}
}

void UTDFlightHUDWidget::HandleHyperdriveInput(bool bHeld)
{
	const ITDHUDData* Data = Cast<ITDHUDData>(Source.Get());
	if (bHeld && Data && !Data->GetHyperdriveBlockText().IsEmpty())
	{
		BlockedLeft = BlockedSeconds;
	}
}

FVector2D UTDFlightHUDWidget::ToDesign(const FVector2D& Local) const
{
	const float Scale = FMath::Min(ViewportSize.X / 1920.f, ViewportSize.Y / 1080.f);
	if (Scale <= 0.f)
	{
		return Local;
	}
	const FVector2D Origin = (ViewportSize - FVector2D(1920, 1080) * Scale) * .5;
	return (Local - Origin) / Scale;
}

FVector2D UTDFlightHUDWidget::OnSafeFrame(const FVector2D& InDir)
{
	const FVector2D Dir = InDir.IsNearlyZero() ? FVector2D(0, 1) : InDir.GetSafeNormal();
	const FVector2D C(CenterX, CenterY);
	float T = TNumericLimits<float>::Max();
	if (Dir.X > 1e-4f) { T = FMath::Min(T, float((1920.f - SafeX - C.X) / Dir.X)); }
	if (Dir.X < -1e-4f) { T = FMath::Min(T, float((SafeX - C.X) / Dir.X)); }
	if (Dir.Y > 1e-4f) { T = FMath::Min(T, float((1080.f - SafeY - C.Y) / Dir.Y)); }
	if (Dir.Y < -1e-4f) { T = FMath::Min(T, float((SafeY - C.Y) / Dir.Y)); }
	return C + Dir * T;
}

void UTDFlightHUDWidget::SampleCombat(const ITDHUDData& Data, const FTDShipHUD* Flown, float Dt)
{
	APlayerController* PC = GetOwningPlayer();
	FVector ViewLoc = FVector::ZeroVector;
	FRotator ViewRot = FRotator::ZeroRotator;
	float FovDeg = 90.f;
	if (PC)
	{
		PC->GetPlayerViewPoint(ViewLoc, ViewRot);
		if (PC->PlayerCameraManager)
		{
			FovDeg = PC->PlayerCameraManager->GetFOVAngle();
		}
	}
	else if (const AActor* Ship = Cast<AActor>(Source.Get()))
	{
		ViewLoc = Ship->GetActorLocation();
	}
	int32 PxW = 0, PxH = 0;
	if (PC)
	{
		PC->GetViewportSize(PxW, PxH);
	}
	// World -> design space, or an off-screen direction (x right, y down) when it is behind or outside the view.
	auto Project = [&](const FVector& World, FVector2D& OutDesign, FVector2D& OutDir) -> bool
	{
		const FVector Local = ViewRot.UnrotateVector(World - ViewLoc);
		OutDir = FVector2D(Local.Y, -Local.Z);
		if (OutDir.IsNearlyZero())
		{
			OutDir = FVector2D(0, 1);
		}
		OutDir.Normalize();
		FVector2D Px;
		if (!PC || PxW <= 0 || Local.X <= 1.f || !PC->ProjectWorldLocationToScreen(World, Px, true))
		{
			return false;
		}
		OutDesign = ToDesign(Px / FVector2D(PxW, PxH) * ViewportSize);
		return OutDesign.X >= SafeX && OutDesign.X <= 1920.f - SafeX && OutDesign.Y >= SafeY && OutDesign.Y <= 1080.f - SafeY;
	};

	// Edge arrows never sit on the squad strip (first P2 capture: a chevron landed on the Rift chip): lift them above it.
	float StripRight = 0.f, StripTop = 1080.f;
	for (const FChip& Chip : View.Chips)
	{
		StripRight = FMath::Max(StripRight, float(Chip.Pos.X + Chip.Size.X));
		StripTop = FMath::Min(StripTop, float(Chip.Pos.Y));
	}
	auto AvoidStrip = [StripRight, StripTop](FVector2D Pos)
	{
		if (StripRight > 0.f && Pos.X <= StripRight + 14.f && Pos.Y >= StripTop - 14.f)
		{
			Pos.Y = StripTop - 16.f;
		}
		return Pos;
	};

	// Hostiles and missiles.
	View.Marks.Reset();
	const AActor* Current = Data.GetCurrentTarget();
	const AActor* Focus = Data.GetFocusRemaining01() > 0.f ? Data.GetFocusTarget() : nullptr;
	const float PxPerRadian = 540.f / FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FovDeg, 20.f, 170.f) * .5f));
	for (const FTDTargetMarker& M : Data.GetTargetMarkers())
	{
		FMark& Mark = View.Marks.AddDefaulted_GetRef();
		Mark.bCurrent = M.bIsCurrentTarget || (Current && M.Actor.Get() == Current);
		Mark.bLocked = M.bLocked;
		Mark.bMissile = M.bIncomingMissile;
		Mark.bFocus = Focus && M.Actor.Get() == Focus;
		Mark.Distance = M.Distance / 100.f;
		Mark.bOnScreen = M.bOnScreen;
		if (M.bOnScreen)
		{
			Mark.Pos = ToDesign(M.ScreenPosition01 * ViewportSize);
			Mark.bOnScreen = Mark.Pos.X >= SafeX && Mark.Pos.X <= 1920.f - SafeX && Mark.Pos.Y >= SafeY && Mark.Pos.Y <= 1080.f - SafeY;
		}
		if (!Mark.bOnScreen)
		{
			const FVector2D Dir = M.ScreenDirection.IsNearlyZero() ? FVector2D(0, 1) : M.ScreenDirection.GetSafeNormal();
			Mark.Pos = AvoidStrip(OnSafeFrame(Dir));
			Mark.Angle = FMath::Atan2(Dir.Y, Dir.X);
		}
		if (Mark.bCurrent)
		{
			if (const AActor* A = M.Actor.Get())
			{
				const UTDHealthComponent* Health = UTDStatics::GetHealth(A);
				Mark.Health = Health ? Health->GetHull01() : -1.f;
				const float Radius = A->GetSimpleCollisionRadius();
				Mark.Size = FMath::Clamp(2.f * Radius / FMath::Max(M.Distance, 1.f) * PxPerRadian, BracketMin, BracketMax);
			}
			else
			{
				Mark.Size = BracketMin;
			}
		}
	}

	// Lead indicator (only with a current target).
	FVector LeadWorld;
	FVector2D LeadDir;
	View.bHasLead = Current && Data.GetLeadIndicator(LeadWorld) && Project(LeadWorld, View.Lead, LeadDir);

	// Wingmen off screen: a hollow chevron in their colour, pulsing when that wingman is in trouble.
	View.Wings.Reset();
	for (const FTDSquadMarker& S : Data.GetSquadMarkers())
	{
		const AActor* A = S.Actor.Get();
		if (S.bPlayerControlled || !S.bAlive || !A)
		{
			continue;
		}
		FVector2D At, Dir;
		if (Project(A->GetActorLocation(), At, Dir))
		{
			continue; // on screen: the ship itself is visible
		}
		FWingArrow& W = View.Wings.AddDefaulted_GetRef();
		W.Ship = S.Ship;
		W.Pos = AvoidStrip(OnSafeFrame(Dir));
		W.Angle = FMath::Atan2(Dir.Y, Dir.X);
		const FChip* Chip = View.Chips.FindByPredicate([&S](const FChip& C) { return C.Ship == S.Ship; });
		W.bTrouble = Chip && Chip->bTrouble;
	}

	View.Missile = Data.GetMissileWarning();

	// Hit direction arcs around the reticle; the low-hull vignette.
	View.Hits.Reset();
	for (const FTDDamageIndicator& D : Data.GetDamageIndicators())
	{
		if (D.AgeSeconds >= HitSeconds)
		{
			continue;
		}
		const FVector Local = ViewRot.UnrotateVector(D.WorldDirection);
		FHitArc& Arc = View.Hits.AddDefaulted_GetRef();
		const FVector2D Flat(Local.Y, -Local.Z);
		Arc.Angle = Flat.SizeSquared() > .01f ? FMath::Atan2(Flat.Y, Flat.X) : (Local.X < 0.f ? PI * .5f : -PI * .5f); // behind = below
		Arc.Opacity = (.4f + .6f * FMath::Clamp(D.Strength01, 0.f, 1.f)) * (1.f - D.AgeSeconds / HitSeconds);
		Arc.bShieldOnly = D.bShieldOnly;
	}
	View.Vignette = (Flown && View.Hull < .25f) ? 1.f : 0.f;

	// The flown ship's contextual readouts.
	if (Flown)
	{
		View.bHasAbility = Flown->Ability != ETDAbilityId::None;
		View.Ability = TDFlightHUD::Clamp01(Flown->AbilityCooldown01);
		View.AbilityGlyph = View.bUsingGamepad ? TEXT("Y") : TEXT("E");
		View.bHasPower2 = Flown->Ship == ETDShipId::Bastion; // the dome: hold Q (only Bastion has a second signature power)
		View.Power2 = TDFlightHUD::Clamp01(Flown->Power2Cooldown01);
		View.Ammo = FMath::Max(0, Flown->SecondaryAmmo);
		View.Heat = TDFlightHUD::Clamp01(Flown->Heat01);
		View.Boost = TDFlightHUD::Clamp01(Flown->Boost01);
		View.bDodgeReady = Flown->Dodge01 >= .999f;
		BoostFullFor = View.Boost < .999f ? 0.f : BoostFullFor + Dt;
		View.bShowBoost = BoostFullFor < BoostHideSeconds;
	}
	View.Hyperdrive = TDFlightHUD::Clamp01(Data.GetHyperdriveCharge01());

	// The lower lane: one message at a time. Interact / landing prompt, then a blocked jump (3 s after the press), then a hint.
	BlockedLeft = FMath::Max(0.f, BlockedLeft - Dt);
	FString Prompt = Data.GetInteractPrompt().ToString();
	const FString Blocked = Data.GetHyperdriveBlockText().ToString();
	const FString Hint = UTDHintComponent::GetHintText(this).ToString();
	if (!Hint.IsEmpty() && Prompt == Hint)
	{
		Prompt.Reset(); // today the control hints also arrive as the interact prompt: show them as a hint (T3, Nov 13)
	}
	if (!Prompt.IsEmpty())
	{
		View.Lane = ELane::Prompt;
		View.LaneText = Prompt.ToUpper();
	}
	else if (BlockedLeft > 0.f && !Blocked.IsEmpty())
	{
		View.Lane = ELane::HyperdriveBlocked;
		View.LaneText = Blocked.ToUpper();
	}
	else if (!Hint.IsEmpty())
	{
		View.Lane = ELane::Hint;
		View.LaneText = Hint.ToUpper();
	}
	else
	{
		View.Lane = ELane::None;
		View.LaneText.Reset();
	}
	View.Boundary = Data.GetBoundaryWarningText().ToString().ToUpper();
	View.SwitchStrike = TDFlightHUD::Clamp01(Data.GetSwitchStrike01());
	View.Altitude = Data.GetAltitude();
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
	BindController();
	SampleCombat(*Data, Flown, Dt);
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
	float Area = 224.f * 116.f; // the reticle cluster's box (spec 3.1); the rest is added as it is drawn
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

	// ======================================================================== P2: contextual
	const float Now = Time;
	// ---- low-hull vignette: a 10% red edge glow breathing at 1 Hz
	if (View.Vignette > 0.f)
	{
		const FLinearColor Edge = T::Danger().CopyWithNewOpacity(.10f * (.6f + .4f * FMath::Sin(Now * 2.f * PI)));
		const FLinearColor Clear = T::Danger().CopyWithNewOpacity(0.f);
		P.Gradient(-200, 0, 320, 1080, Edge, Clear, false);
		P.Gradient(1800, 0, 320, 1080, Clear, Edge, false);
		P.Gradient(0, -120, 1920, 220, Edge, Clear, true);
		P.Gradient(0, 980, 1920, 220, Clear, Edge, true);
	}

	// ---- hostiles: tick above each, the bracket + health + distance on the current target, chevrons off screen
	const FLinearColor Enemy = T::Danger();
	const float Pulse25 = .55f + .45f * FMath::Sin(Now * 2.5f * 2.f * PI); // under 3 Hz
	for (const FMark& M : View.Marks)
	{
		if (!M.bOnScreen)
		{
			P.Chevron(M.Pos, M.Angle, 18.f, Enemy.CopyWithNewOpacity(M.bMissile ? Pulse25 : .9f), true);
			continue;
		}
		if (M.bMissile)
		{
			P.Arc(M.Pos, 9.f, 1.f, Enemy.CopyWithNewOpacity(Pulse25), 2.f);
			P.Box(M.Pos.X - 2, M.Pos.Y - 2, 4, 4, Enemy);
			continue;
		}
		if (M.bCurrent)
		{
			const float H = M.Size * .5f;
			P.Brackets(M.Pos, H, Enemy, FMath::Clamp(M.Size * .22f, 8.f, 18.f));
			if (M.Health >= 0.f)
			{
				P.Bar(M.Pos.X - H, M.Pos.Y - H - 10.f, M.Size, 4.f, M.Health, Enemy, 1.f);
			}
			P.Words(M.Pos.X, M.Pos.Y + H + 4.f, FString::Printf(TEXT("%.0f m"), M.Distance), 14, T::Neutral(), "Bold", .5f, 20);
			if (M.bLocked)
			{
				P.Poly(M.Pos, { { 0, -9 }, { 9, 0 }, { 0, 9 }, { -9, 0 }, { 0, -9 } }, T::Warning(), 2.f);
				P.Words(M.Pos.X + H + 6.f, M.Pos.Y - 9.f, TEXT("LOCK"), 14, T::Warning(), "Bold", 0.f, 40);
			}
			if (M.bFocus)
			{
				for (const float Up : { 0.f, 7.f })
				{
					const FVector2D A(M.Pos.X, M.Pos.Y - H - 18.f - Up);
					P.Line(A + FVector2D(-7, -4), A, T::Warning(), 2.f);
					P.Line(A, A + FVector2D(7, -4), T::Warning(), 2.f);
				}
			}
			continue;
		}
		// Other hostiles: a 14 px hollow triangle, point down, above the ship; a locked one gets a 20 px amber diamond.
		const FVector2D Tick(M.Pos.X, M.Pos.Y - 18.f);
		P.Poly(Tick, { { -7, -6 }, { 7, -6 }, { 0, 6 }, { -7, -6 } }, Enemy.CopyWithNewOpacity(.9f), 2.f);
		if (M.bLocked)
		{
			P.Poly(M.Pos, { { 0, -10 }, { 10, 0 }, { 0, 10 }, { -10, 0 }, { 0, -10 } }, T::Warning(), 2.f);
		}
		if (M.bFocus)
		{
			P.Line(Tick + FVector2D(-6, -14), Tick + FVector2D(0, -10), T::Warning(), 2.f);
			P.Line(Tick + FVector2D(0, -10), Tick + FVector2D(6, -14), T::Warning(), 2.f);
		}
	}
	if (View.bHasLead)
	{
		P.Arc(View.Lead, 7.f, 1.f, T::Neutral().CopyWithNewOpacity(.9f), 2.f);
	}
	// ---- wingmen off screen: hollow chevron in squad colour, pulsing at 1 Hz in trouble
	for (const FWingArrow& Wing : View.Wings)
	{
		const float O = Wing.bTrouble ? .5f + .5f * FMath::Sin(Now * 2.f * PI) : .85f;
		P.Chevron(Wing.Pos, Wing.Angle, 18.f, T::Ship(Wing.Ship).CopyWithNewOpacity(O), false);
	}

	// ---- hit direction arcs (radius 150, 50 degrees, 8 px)
	for (const FHitArc& Hit : View.Hits)
	{
		const float HalfWidth = FMath::DegreesToRadians(25.f);
		P.ArcSpan(C, HitRadius, Hit.Angle - HalfWidth, 2.f * HalfWidth, (Hit.bShieldOnly ? T::Shield() : T::Danger()).CopyWithNewOpacity(Hit.Opacity), 8.f);
	}

	// ---- around the reticle: Switch Strike ring, heat arc, ability pip, second power pip, ammo, altitude
	if (View.SwitchStrike > 0.f)
	{
		P.Arc(C, 62.f, View.SwitchStrike, Accent.CopyWithNewOpacity(.85f), 2.f, -PI * .5f);
	}
	if (View.Heat >= .6f)
	{
		const FLinearColor HeatC = View.Heat >= .9f ? T::Danger() : T::Bastion();
		const float Half60 = FMath::DegreesToRadians(30.f);
		P.ArcSpan(C, 30.f, PI * .5f - Half60, 2.f * Half60 * View.Heat, HeatC, 3.f);
		if (View.Heat >= .9f)
		{
			P.Words(C.X, C.Y + 34.f, TEXT("HOT"), 14, T::Danger(), "Bold", .5f, 60);
		}
	}
	if (View.bHasAbility)
	{
		P.Pip(C + FVector2D(-PipOffsetX, PipOffsetY), 14.f, View.Ability, View.Ability >= .999f, Accent, View.AbilityGlyph, 13);
	}
	float AmmoX = C.X + PipOffsetX + PipRadius + 8.f;
	if (View.bHasPower2)
	{
		const FVector2D P2(C.X + PipOffsetX + 40.f, C.Y + PipOffsetY);
		P.Pip(P2, 12.f, View.Power2, View.Power2 >= .999f, Accent, View.PowerGlyph, 12);
		P.Words(P2.X, P2.Y + 13.f, TEXT("HOLD"), 10, T::Dim(), "Bold", .5f, 40);
		AmmoX = P2.X + 12.f + 8.f;
	}
	P.Words(AmmoX, C.Y + PipOffsetY - 10.f, FString::FromInt(View.Ammo), 15, T::Neutral().CopyWithNewOpacity(View.Ammo > 0 ? .9f : .4f), "Bold", 0.f, 0);
	if (View.Altitude >= 0.f)
	{
		P.Words(C.X - 110.f, C.Y - 5.f, FString::Printf(TEXT("ALT %.0f m"), View.Altitude), 15, View.Altitude < 60.f ? T::Bastion() : T::Neutral(),
			"Bold", 1.f, 20);
	}

	// ---- missile warning, centred above the reticle
	if (View.Missile != ETDMissileWarning::None)
	{
		const float O = View.Missile == ETDMissileWarning::Imminent ? Pulse25 : 1.f;
		P.Plate(CenterX - 75.f, 444.f, 150.f, 28.f, 6.f, T::Plate().CopyWithNewOpacity(.6f * O));
		P.Poly(FVector2D(CenterX - 56.f, 458.f), { { 0, -8 }, { 8, 6 }, { -8, 6 }, { 0, -8 } }, Enemy.CopyWithNewOpacity(O), 2.f);
		P.Words(CenterX + 10.f, 449.f, TEXT("MISSILE"), 15, Enemy.CopyWithNewOpacity(O), "Bold", .5f, 80);
	}

	// ---- boundary warning
	if (!View.Boundary.IsEmpty())
	{
		P.Plate(CenterX - 260.f, 132.f, 520.f, 28.f, 14.f, T::Plate().CopyWithNewOpacity(.6f));
		P.Poly(FVector2D(CenterX - 240.f, 146.f), { { 0, -8 }, { 8, 6 }, { -8, 6 }, { 0, -8 } }, T::Warning(), 2.f);
		P.Words(CenterX + 8.f, 136.f, View.Boundary, 15, T::Warning(), "Bold", .5f, 40);
	}

	// ---- boost (with the dodge notch) and hyperdrive bars, under the ship
	if (View.bShowBoost)
	{
		P.Bar(CenterX - 80.f, 792.f, 160.f, 6.f, View.Boost, Accent, .9f);
		P.Box(CenterX + 84.f, 791.f, 2.f, 8.f, (View.bDodgeReady ? T::Neutral() : T::Dim()).CopyWithNewOpacity(View.bDodgeReady ? .95f : .4f));
	}
	if (View.Hyperdrive > 0.f)
	{
		P.Bar(CenterX - 120.f, 806.f, 240.f, 6.f, View.Hyperdrive, T::Shield(), .95f);
	}

	// ---- the lower lane: one message
	if (View.Lane != ELane::None && !View.LaneText.IsEmpty())
	{
		const FLinearColor LaneC = View.Lane == ELane::HyperdriveBlocked ? T::Warning() : T::Neutral();
		const float LaneW = FMath::Clamp(P.Measure(View.LaneText, P.Font("Bold", 17, 50)) + 48.f, 240.f, 760.f);
		Area += LaneW * 36.f;
		P.Plate(CenterX - LaneW * .5f, 838.f, LaneW, 36.f, 8.f, T::Plate().CopyWithNewOpacity(View.Lane == ELane::Hint ? .5f : .65f));
		if (View.Lane == ELane::HyperdriveBlocked)
		{
			// A padlock: shackle arc + body.
			const FVector2D L(CenterX - LaneW * .5f + 20.f, 858.f);
			P.ArcSpan(L + FVector2D(0, -4), 5.f, PI, PI, LaneC, 2.f);
			P.Outline(L.X - 7.f, L.Y - 4.f, 14.f, 10.f, 2.f, 1.5f, LaneC);
		}
		P.Words(CenterX + (View.Lane == ELane::HyperdriveBlocked ? 10.f : 0.f), 845.f, View.LaneText, 17, LaneC, "Bold", .5f, 50);
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
		Area += W * H;
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
	// ---- coverage (spec 3.3: bounding boxes of what was drawn this frame)
	for (const FChip& Chip : View.Chips)
	{
		Area += Chip.Size.X * Chip.Size.Y;
	}
	for (const FMark& M : View.Marks)
	{
		Area += !M.bOnScreen ? 18.f * 18.f : M.bMissile ? 18.f * 18.f : M.bCurrent ? M.Size * (M.Size + 34.f) : 14.f * 12.f;
	}
	Area += View.Wings.Num() * 18.f * 18.f + (View.bHasLead ? 14.f * 14.f : 0.f) + View.Hits.Num() * 131.f * 16.f;
	Area += (View.Missile != ETDMissileWarning::None ? 150.f * 28.f : 0.f) + (View.Boundary.IsEmpty() ? 0.f : 520.f * 28.f);
	Area += (View.bShowBoost ? 168.f * 8.f : 0.f) + (View.Hyperdrive > 0.f ? 240.f * 6.f : 0.f);
	LastCoverage01 = Area / (1920.f * 1080.f);
	return Base + 4;
}
