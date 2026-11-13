// Triad Drift - engineering space HUD (ENG-36, Teammate 2). Spec: docs/eng/hud-redesign.md.

#include "TriadDrift/UI/TDFlightHUDWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Core/TDEventBus.h"
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
		/** The one backing style for text over a bright sky (N-037, Lead): #0A0F1C at this opacity, tight around the text. */
		static constexpr float BackingOpacity = .62f;
		/** Words on a tight dark plate (small readouts that cross bright planet rings: the range under a bracket). */
		void BackedWords(float X, float Y, const FString& S, int32 Size, FLinearColor C, float Align = 0.f, int32 Tracking = 20) const
		{
			float W = Measure(S, Font("Bold", Size, Tracking));
			if (W <= 0.f)
			{
				W = S.Len() * Size * .62f; // headless: no font measure; an estimate keeps the plate sane
			}
			const float Left = X - W * Align;
			Plate(Left - 5.f, Y - 1.f, W + 10.f, Size + 6.f, 4.f, T::Plate().CopyWithNewOpacity(BackingOpacity));
			Words(Left, Y, S, Size, C, "Bold", 0.f, Tracking);
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

static TAutoConsoleVariable<int32> CVarTDUIForceGlyphs(
	TEXT("TD.UI.ForceGlyphs"), -1,
	TEXT("Engineering space HUD glyphs (dev, captures): -1 = follow the controller's device detection, 0 = keyboard, 1 = gamepad."),
	ECVF_Default);

bool UTDFlightHUDWidget::IsUsingGamepad() const
{
	if (ForceGamepad >= 0)
	{
		return ForceGamepad > 0;
	}
	if (const int32 Forced = CVarTDUIForceGlyphs.GetValueOnGameThread(); Forced >= 0)
	{
		return Forced > 0;
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
	if (UTDEventBus* Bus = BoundBus.Get())
	{
		Bus->OnHitConfirmed.RemoveDynamic(this, &UTDFlightHUDWidget::HandleHitConfirmed);
		Bus->OnTakeStarted.RemoveDynamic(this, &UTDFlightHUDWidget::HandleTakeStarted);
		Bus->OnSceneCut.RemoveDynamic(this, &UTDFlightHUDWidget::HandleSceneCut);
		Bus->OnBarkRequested.RemoveDynamic(this, &UTDFlightHUDWidget::HandleBarkRequested);
	}
	BoundBus.Reset();
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

void UTDFlightHUDWidget::BindBus()
{
	UTDEventBus* Bus = UTDEventBus::Get(this);
	if (Bus == BoundBus.Get())
	{
		return;
	}
	if (UTDEventBus* Old = BoundBus.Get())
	{
		Old->OnHitConfirmed.RemoveDynamic(this, &UTDFlightHUDWidget::HandleHitConfirmed);
		Old->OnTakeStarted.RemoveDynamic(this, &UTDFlightHUDWidget::HandleTakeStarted);
		Old->OnSceneCut.RemoveDynamic(this, &UTDFlightHUDWidget::HandleSceneCut);
		Old->OnBarkRequested.RemoveDynamic(this, &UTDFlightHUDWidget::HandleBarkRequested);
	}
	BoundBus = Bus;
	if (Bus)
	{
		Bus->OnHitConfirmed.AddUniqueDynamic(this, &UTDFlightHUDWidget::HandleHitConfirmed);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDFlightHUDWidget::HandleTakeStarted);
		Bus->OnSceneCut.AddUniqueDynamic(this, &UTDFlightHUDWidget::HandleSceneCut);
		Bus->OnBarkRequested.AddUniqueDynamic(this, &UTDFlightHUDWidget::HandleBarkRequested);
	}
}

FString UTDFlightHUDWidget::BarkText(FName BarkId)
{
	FString Id = BarkId.ToString();
	Id.RemoveFromStart(TEXT("Bark_"));
	Id.RemoveFromStart(TEXT("Squad"));
	FString Out;
	for (int32 I = 0; I < Id.Len(); ++I)
	{
		const TCHAR Ch = Id[I];
		if (Ch == TEXT('_'))
		{
			Out += TEXT(' ');
			continue;
		}
		if (I > 0 && FChar::IsUpper(Ch) && !Out.EndsWith(TEXT(" ")))
		{
			Out += TEXT(' ');
		}
		Out += FChar::ToLower(Ch);
	}
	return Out.TrimStartAndEnd();
}

void UTDFlightHUDWidget::ShowTransmission(FName Speaker, const FText& Line, ETransmissionStyle Style, float Seconds)
{
	View.Subtitle.Speaker = Speaker.IsNone() ? FString() : Speaker.ToString().ToUpper();
	View.Subtitle.Text = Line.ToString();
	View.Subtitle.Style = Style;
	View.Subtitle.Age = 0.f;
	View.Subtitle.Seconds = FMath::Max(.5f, Seconds);
}

void UTDFlightHUDWidget::HandleBarkRequested(FName BarkId, FName Speaker)
{
	// A story transmission on screen outranks a squad bark (it is the line the player must not miss).
	if (!View.Subtitle.Text.IsEmpty() && View.Subtitle.Style != ETransmissionStyle::Clean)
	{
		return;
	}
	ShowTransmission(Speaker, FText::FromString(BarkText(BarkId)), ETransmissionStyle::Clean, BarkSeconds);
}

void UTDFlightHUDWidget::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	CutLeft = 0.f; // the take starts: the slate card is up, the HUD sits at 40% behind it
	SlateLeft = SlateSeconds;
}

void UTDFlightHUDWidget::HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds)
{
	CutLeft = FMath::Max(.1f, DelaySeconds); // "CUT!": every HUD element off until the next take starts
	SlateLeft = 0.f;
}

void UTDFlightHUDWidget::SampleDirector(float Dt)
{
	if (!View.Subtitle.Text.IsEmpty())
	{
		View.Subtitle.Age += Dt;
		if (View.Subtitle.Age >= View.Subtitle.Seconds)
		{
			View.Subtitle = FSubtitle();
		}
	}
	CutLeft = FMath::Max(0.f, CutLeft - Dt);
	SlateLeft = FMath::Max(0.f, SlateLeft - Dt);
	const UWorld* World = GetWorld();
	const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this);
	const bool bMoney = ForceMoneyShot >= 0 ? ForceMoneyShot > 0 : (Feel && Feel->IsSlowMoActive());
	View.bPaused = ForcePaused >= 0 ? ForcePaused > 0 : (World && World->IsPaused());
	View.bInCut = CutLeft > 0.f;
	View.bInSlate = !View.bInCut && SlateLeft > 0.f;
	// Money shot eases in and out over 150 ms (spec: "returns over 150 ms when time dilation ends"). Real time: the
	// slow-mo itself scales world delta, so ease on the undilated step.
	const float RealDt = World && World->GetWorldSettings() ? Dt / FMath::Max(World->GetWorldSettings()->GetEffectiveTimeDilation(), .01f) : Dt;
	View.MoneyShot = FMath::FInterpConstantTo(View.MoneyShot, bMoney ? 1.f : 0.f, RealDt, 1.f / MoneyShotEaseSeconds);
	const float Target = View.bInCut ? 0.f : View.bInSlate ? SlateOpacity : View.bPaused ? PauseOpacity : 1.f;
	// Off at once on a cut; everything else eases (the fade back in after CUT is the 250 ms the spec asks for).
	View.HUDOpacity = View.bInCut ? 0.f : FMath::FInterpConstantTo(View.HUDOpacity, Target, RealDt, 1.f / CutFadeInSeconds);
	SetRenderOpacity(View.HUDOpacity);
}

void UTDFlightHUDWidget::HandleHitConfirmed(AActor* Target, bool bKill)
{
	if (bKill)
	{
		KillLeft = KillMarkSeconds;
		HitLeft = 0.f;
		if (Target)
		{
			KillWorld = Target->GetActorLocation();
			KillAtLeft = KillAtTargetSeconds;
		}
	}
	else if (KillLeft <= 0.f)
	{
		HitLeft = HitMarkSeconds; // a hit never cuts a kill X short
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

bool UTDFlightHUDWidget::ReadHintCard()
{
	// T3's getters on UTDHintComponent (direct calls; review M4: the reflection walk read the step and count in the wrong
	// order). Key and action are upper-cased only when they change (review m8).
	const ETDHintState State = UTDHintComponent::GetHintState(this);
	View.bHintDone = State == ETDHintState::Done;
	if (State == ETDHintState::None)
	{
		return false;
	}
	const FString Key = UTDHintComponent::GetHintKey(this).ToString();
	const FString Action = UTDHintComponent::GetHintAction(this).ToString();
	if (Key != LastHintKeyRaw)
	{
		LastHintKeyRaw = Key;
		View.HintKey = Key.ToUpper();
	}
	if (Action != LastHintActionRaw)
	{
		LastHintActionRaw = Action;
		View.HintAction = Action.ToUpper();
	}
	int32 Step = 0, Count = 0;
	const bool bStep = UTDHintComponent::GetHintStep(this, Step, Count);
	View.HintStep = bStep ? Step : 0;
	View.HintCount = bStep ? Count : 0;
	return !View.HintAction.IsEmpty();
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
	// Inset so the whole edge marker (up to 18 px) stays inside the safe frame, not just its centre.
	const float In = 12.f;
	float T = TNumericLimits<float>::Max();
	if (Dir.X > 1e-4f) { T = FMath::Min(T, float((1920.f - SafeX - In - C.X) / Dir.X)); }
	if (Dir.X < -1e-4f) { T = FMath::Min(T, float((SafeX + In - C.X) / Dir.X)); }
	if (Dir.Y > 1e-4f) { T = FMath::Min(T, float((1080.f - SafeY - In - C.Y) / Dir.Y)); }
	if (Dir.Y < -1e-4f) { T = FMath::Min(T, float((SafeY + In - C.Y) / Dir.Y)); }
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
	// ...nor inside the objective pill (P2 capture: a wing chevron on the top edge sat in the pill).
	const float PillHalf = ObjectiveExpandedWidth * .5f + 14.f;
	const float PillBottom = SafeY + FMath::Lerp(ObjectiveHeight, ObjectiveExpandedHeight, View.ObjectiveExpand) + 14.f;
	const bool bPill = !View.Objective.IsEmpty();
	auto AvoidStrip = [StripRight, StripTop, PillHalf, PillBottom, bPill](FVector2D Pos)
	{
		if (StripRight > 0.f && Pos.X <= StripRight + 14.f && Pos.Y >= StripTop - 14.f)
		{
			Pos.Y = StripTop - 16.f;
		}
		if (bPill && FMath::Abs(Pos.X - CenterX) <= PillHalf && Pos.Y <= PillBottom)
		{
			Pos.Y = PillBottom + 2.f;
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

	// Objective waypoints (review M3): planets, gates, Dominion generators, the landing pad on a 16 km planet.
	View.Waypoints.Reset();
	const APawn* Me = PC ? PC->GetPawn() : nullptr;
	const FVector From = Me ? Me->GetActorLocation() : ViewLoc;
	for (const AActor* Marker : Data.GetObjectiveMarkers())
	{
		if (!IsValid(Marker) || Marker == Me)
		{
			continue;
		}
		FWaypoint& Wp = View.Waypoints.AddDefaulted_GetRef();
		FVector2D Dir;
		Wp.bOnScreen = Project(Marker->GetActorLocation(), Wp.Pos, Dir);
		if (!Wp.bOnScreen)
		{
			Wp.Pos = AvoidStrip(OnSafeFrame(Dir));
			Wp.Angle = FMath::Atan2(Dir.Y, Dir.X);
		}
		Wp.Distance = FVector::Dist(From, Marker->GetActorLocation()) / 100.f;
		Wp.DrawPos = Wp.Pos;
		Wp.bLifted = Wp.bOnScreen && LiftWaypoint(Wp.Pos, Wp.DrawPos);
		const UTDHealthComponent* Health = UTDStatics::GetHealth(Marker);
		Wp.Health = Health && Health->IsAlive() ? Health->GetHull01() : -1.f;
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
		if (View.Ammo > 0)
		{
			ShipsWithAmmo.Add(Flown->Ship);
		}
		View.bUsesAmmo = ShipsWithAmmo.Contains(Flown->Ship); // a ship that ran dry still shows "0"; one with none shows nothing
		View.Heat = TDFlightHUD::Clamp01(Flown->Heat01);
		View.Boost = TDFlightHUD::Clamp01(Flown->Boost01);
		View.bDodgeReady = Flown->Dodge01 >= .999f;
		BoostFullFor = View.Boost < .999f ? 0.f : BoostFullFor + Dt;
		View.bShowBoost = BoostFullFor < BoostHideSeconds;
	}
	View.Hyperdrive = TDFlightHUD::Clamp01(Data.GetHyperdriveCharge01());

	// The lower lane: one message at a time. Interact / landing prompt, then a blocked jump (3 s after the press), then a hint.
	BlockedLeft = FMath::Max(0.f, BlockedLeft - Dt);
	const bool bWasDone = View.bHintDone;
	View.bHintCard = ReadHintCard();
	HintDoneAge = View.bHintDone ? (bWasDone ? HintDoneAge + Dt : 0.f) : 0.f;
	View.HintDone01 = View.bHintDone ? FMath::Clamp(HintDoneAge / .5f, 0.f, 1.f) : 0.f;
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
	else if (!Hint.IsEmpty() || View.bHintCard)
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
	const float Strike = TDFlightHUD::Clamp01(Data.GetSwitchStrike01());
	if (Strike > View.SwitchStrike + .05f)
	{
		StrikePulseLeft = StrikePulseSeconds; // a window just opened (or re-armed): the reward pulse
	}
	View.SwitchStrike = Strike;
	StrikePulseLeft = FMath::Max(0.f, StrikePulseLeft - Dt);
	View.SwitchStrikePulse = StrikePulseLeft > 0.f ? 1.f - StrikePulseLeft / StrikePulseSeconds : 0.f;
	View.Focus = TDFlightHUD::Clamp01(Data.GetFocusRemaining01());

	// Hit feel: the reticle tick / kill X, and the kill X on the victim's last spot (readable at range).
	HitLeft = FMath::Max(0.f, HitLeft - Dt);
	KillLeft = FMath::Max(0.f, KillLeft - Dt);
	KillAtLeft = FMath::Max(0.f, KillAtLeft - Dt);
	View.HitMark = HitLeft > 0.f ? 1.f - HitLeft / HitMarkSeconds : 0.f;
	View.KillMark = KillLeft > 0.f ? 1.f - KillLeft / KillMarkSeconds : 0.f;
	View.KillAtTarget = KillAtLeft > 0.f ? 1.f - KillAtLeft / KillAtTargetSeconds : 0.f;
	FVector2D KillDir;
	View.bKillAtTarget = KillAtLeft > 0.f && Project(KillWorld, View.KillPos, KillDir);
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
	View.bHasFlown = Flown != nullptr;
	if (!Flown)
	{
		// Nobody flown (the active ship is down before the retake): no stale arcs, pips or bars (review m9).
		View.bHasAbility = View.bHasPower2 = View.bUsesAmmo = View.bShowBoost = false;
		View.Heat = 0.f;
	}
	BindController();
	BindBus();
	SampleDirector(Dt);
	SampleCombat(*Data, Flown, Dt);
	ArcAwake = FMath::Max(0.f, ArcAwake - Dt);
	View.ArcOpacity = (View.Hull < .999f || View.Shield < .999f || ArcAwake > 0.f) ? 1.f : ArcRestOpacity;
	PulseLeft = FMath::Max(0.f, PulseLeft - Dt);
	View.PowerPulse = PulseLeft > 0.f ? 1.f - PulseLeft / PulseSeconds : 0.f;
	View.PowerGlyph = View.bUsingGamepad ? TEXT("X") : TEXT("Q");

	// Objective line: the text in caps, the wave's enemies left as the counter; expanded for 3 s when the text changes.
	// Upper-cased and split only when the source text or wave changes (review m8), not every frame.
	const FTDWaveInfo Wave = Data->GetWaveInfo();
	const FString RawObjective = Data->GetObjectiveText().ToString();
	const bool bWaveShown = Wave.WaveNumber > 0 && (Wave.bWaveActive || Wave.bInBreather);
	if (RawObjective != LastObjective || Wave.WaveNumber != LastWaveNumber)
	{
		if (RawObjective != LastObjective)
		{
			ObjectiveAge = RawObjective.IsEmpty() ? 1e6f : 0.f;
		}
		LastObjective = RawObjective;
		LastWaveNumber = Wave.WaveNumber;
		View.Objective = RawObjective.ToUpper();
		// The squad manager writes "WAVE n · CLEAR ...": the wave belongs to the detail line. Strip exactly that prefix,
		// built from FTDWaveInfo, not a parse of the text (review m11).
		const FString Prefix = FString::Printf(TEXT("WAVE %d · "), Wave.WaveNumber);
		if (Wave.WaveNumber > 0 && View.Objective.StartsWith(Prefix))
		{
			View.Objective = View.Objective.Mid(Prefix.Len());
		}
	}
	else
	{
		ObjectiveAge += Dt;
	}
	const FString& Objective = View.Objective;
	View.Counter = Wave.bWaveActive ? FString::Printf(TEXT("%d LEFT"), FMath::Max(0, Wave.EnemiesAlive)) : FString();
	// The wave line only while a wave is on or about to start (review m11).
	View.ObjectiveDetail = !bWaveShown ? FString()
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
	LastBoxes.Reset();
	LastVignetteRect = FBox2D(ForceInit);
	auto Rec = [this](const TCHAR* Name, float X, float Y, float W, float H) { LastBoxes.Emplace(FName(Name), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H))); };
	Rec(TEXT("reticle"), C.X - 16.f, C.Y - 16.f, 32.f, 32.f);

	// ---- reticle: 2 px dot, four 8 px ticks starting 8 px out
	const FLinearColor Ink = T::Neutral().CopyWithNewOpacity(FMath::Lerp(.9f, .4f, View.MoneyShot)); // 40% in a money shot
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
	if (View.bHasFlown) // nobody flown: no stale arcs or pip (review m9)
	{
		Pair(HullRadius, View.Hull, T::Hull(View.Hull), 5.f);
		Pair(ShieldRadius, View.Shield, T::Shield(), 3.f);
		Rec(TEXT("arcs"), C.X - ShieldRadius - 2.f, C.Y - 40.f, 2.f * ShieldRadius + 4.f, 80.f);
	}

	// ---- money shot (director slow-mo): everything but the reticle and the hull / shield arcs gets out of the shot
	if (View.MoneyShot >= .5f)
	{
		LastCoverage01 = Area / (1920.f * 1080.f);
		return Base + 4;
	}

	// ---- signature power pip: ship-colour ring + glyph when ready, grey ring refilling clockwise when cooling
	if (View.bHasFlown)
	{
		const FVector2D Pip = C + FVector2D(PipOffsetX, PipOffsetY);
		Rec(TEXT("pip"), Pip.X - PipRadius, Pip.Y - PipRadius, 2.f * PipRadius, 2.f * PipRadius);
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
		// Over the whole widget, not the centred 16:9 design frame: at 3440x1440 the frame-space version showed as two
		// vertical bands about 170 px in from the sides (review M6). Band widths scale with the screen height.
		const FLinearColor Edge = T::Danger().CopyWithNewOpacity(.10f * (.6f + .4f * FMath::Sin(Now * 2.f * PI)));
		const FLinearColor Clear = T::Danger().CopyWithNewOpacity(0.f);
		const FVector2D Full = AllottedGeometry.GetLocalSize();
		const float Band = Full.Y * .2f;
		auto Ramp = [&](const FVector2D& At, const FVector2D& Size, const FLinearColor& A, const FLinearColor& B, EOrientation Orient)
		{
			TArray<FSlateGradientStop> Stops;
			Stops.Emplace(FVector2f(0, 0), A);
			Stops.Emplace(FVector2f(Size), B);
			FSlateDrawElement::MakeGradient(OutDrawElements, Base + 1, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(At)), MoveTemp(Stops), Orient);
		};
		Ramp(FVector2D(0, 0), FVector2D(Band, Full.Y), Edge, Clear, Orient_Vertical);
		Ramp(FVector2D(Full.X - Band, 0), FVector2D(Band, Full.Y), Clear, Edge, Orient_Vertical);
		Ramp(FVector2D(0, 0), FVector2D(Full.X, Band), Edge, Clear, Orient_Horizontal);
		Ramp(FVector2D(0, Full.Y - Band), FVector2D(Full.X, Band), Clear, Edge, Orient_Horizontal);
		LastVignetteRect = FBox2D(FVector2D::ZeroVector, Full);
	}

	// ---- hostiles: tick above each, the bracket + health + distance on the current target, chevrons off screen
	const FLinearColor Enemy = T::Danger();
	const float Pulse25 = .55f + .45f * FMath::Sin(Now * 2.5f * 2.f * PI); // under 3 Hz
	for (const FMark& M : View.Marks)
	{
		if (!M.bOnScreen)
		{
			P.Chevron(M.Pos, M.Angle, 18.f, Enemy.CopyWithNewOpacity(M.bMissile ? Pulse25 : .9f), true);
			Rec(M.bMissile ? TEXT("missile_edge") : TEXT("edge"), M.Pos.X - 9.f, M.Pos.Y - 9.f, 18.f, 18.f);
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
			Rec(TEXT("bracket"), M.Pos.X - H, M.Pos.Y - H - 10.f, M.Size, M.Size + 34.f);
			if (M.Health >= 0.f)
			{
				P.Bar(M.Pos.X - H, M.Pos.Y - H - 10.f, M.Size, 4.f, M.Health, Enemy, 1.f);
			}
			// The range on a tight dark plate: it crossed bright planet rings with a weak edge (N-037).
			P.BackedWords(M.Pos.X, M.Pos.Y + H + 5.f, FString::Printf(TEXT("%.0f m"), M.Distance), 14, T::Neutral(), .5f, 20);
			if (M.bLocked)
			{
				// The selected lock reads as the selected one (N-037): diamond and LOCK in the flown ship's colour, LOCK on
				// a plate; other locks are small half-alpha amber diamonds.
				P.Poly(M.Pos, { { 0, -9 }, { 9, 0 }, { 0, 9 }, { -9, 0 }, { 0, -9 } }, Accent, 2.5f);
				P.BackedWords(M.Pos.X + H + 8.f, M.Pos.Y - H - 2.f, TEXT("LOCK"), 14, Accent, 0.f, 40);
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
		Rec(TEXT("tick"), Tick.X - 7.f, Tick.Y - 6.f, 14.f, 12.f);
		P.Poly(Tick, { { -7, -6 }, { 7, -6 }, { 0, 6 }, { -7, -6 } }, Enemy.CopyWithNewOpacity(.9f), 2.f);
		if (M.bLocked)
		{
			P.Poly(M.Pos, { { 0, -6 }, { 6, 0 }, { 0, 6 }, { -6, 0 }, { 0, -6 } }, T::Warning().CopyWithNewOpacity(.5f), 1.5f);
		}
		if (M.bFocus)
		{
			P.Line(Tick + FVector2D(-6, -14), Tick + FVector2D(0, -10), T::Warning(), 2.f);
			P.Line(Tick + FVector2D(0, -10), Tick + FVector2D(6, -14), T::Warning(), 2.f);
		}
	}
	// ---- objective waypoints: a white diamond + range on screen (health bar on a generator), an edge diamond off screen
	auto Range = [](float Metres) { return Metres >= 1000.f ? FString::Printf(TEXT("%.1f km"), Metres / 1000.f) : FString::Printf(TEXT("%.0f m"), Metres); };
	for (const FWaypoint& Wp : View.Waypoints)
	{
		const FLinearColor Way = T::Neutral();
		if (Wp.bOnScreen && Wp.bLifted)
		{
			// Dead ahead on a planet the pad sits behind the player's own ship: draw it just above the ship zone, range
			// above the diamond, with a thin tether down to the true point (Lead, Nov 13).
			const FVector2D At = Wp.DrawPos;
			P.Line(At + FVector2D(0, 11), Wp.Pos, Way.CopyWithNewOpacity(.45f), 1.f);
			P.Poly(At, { { 0, -11 }, { 11, 0 }, { 0, 11 }, { -11, 0 }, { 0, -11 } }, Way, 2.5f);
			Rec(TEXT("waypoint"), At.X - 11.f, At.Y - 11.f, 22.f, 22.f);
			P.Box(At.X - 1.5f, At.Y - 1.5f, 3.f, 3.f, Way);
			// Range to the left (above it would sit on the arcs' lower ends, to the right on the power pip).
			P.BackedWords(At.X - 18.f, At.Y - 9.f, Range(Wp.Distance), 14, Way, 1.f, 20);
			if (Wp.Health >= 0.f)
			{
				P.Bar(At.X - 24.f, At.Y + 16.f, 48.f, 4.f, Wp.Health, T::Danger(), 1.f);
			}
			Area += 60.f * 60.f;
		}
		else if (Wp.bOnScreen)
		{
			P.Poly(Wp.Pos, { { 0, -11 }, { 11, 0 }, { 0, 11 }, { -11, 0 }, { 0, -11 } }, Way, 2.5f);
			Rec(TEXT("waypoint"), Wp.Pos.X - 11.f, Wp.Pos.Y - 11.f, 22.f, 22.f);
			P.Box(Wp.Pos.X - 1.5f, Wp.Pos.Y - 1.5f, 3.f, 3.f, Way);
			P.BackedWords(Wp.Pos.X, Wp.Pos.Y + 16.f, Range(Wp.Distance), 14, Way, .5f, 20);
			if (Wp.Health >= 0.f)
			{
				P.Bar(Wp.Pos.X - 24.f, Wp.Pos.Y - 22.f, 48.f, 4.f, Wp.Health, T::Danger(), 1.f);
			}
			Area += 60.f * 60.f;
		}
		else
		{
			// An edge diamond with a pointer notch toward the waypoint, and the range beside it.
			const FVector2D F(FMath::Cos(Wp.Angle), FMath::Sin(Wp.Angle));
			P.Poly(Wp.Pos, { { 0, -9 }, { 9, 0 }, { 0, 9 }, { -9, 0 }, { 0, -9 } }, Way, 2.5f);
			Rec(TEXT("waypoint_edge"), Wp.Pos.X - 9.f, Wp.Pos.Y - 9.f, 18.f, 18.f);
			P.Line(Wp.Pos + F * 11.f, Wp.Pos + F * 17.f, Way, 2.5f);
			// The range sits inward of the diamond, clear of it (the planet capture had "5.1 km" over the diamond).
			const FVector2D Label = Wp.Pos - F * 52.f;
			P.BackedWords(Label.X, Label.Y - 8.f, Range(Wp.Distance), 14, Way, .5f, 20);
			Area += 70.f * 40.f;
		}
	}

	// ---- hit feel: a white tick around the reticle per hit; a bigger red X on a kill, and one on the victim's spot
	auto Cross = [&P](const FVector2D& At, float Inner, float Outer, FLinearColor Color, float Width)
	{
		for (const FVector2D D : { FVector2D(1, 1), FVector2D(-1, 1), FVector2D(1, -1), FVector2D(-1, -1) })
		{
			const FVector2D N = D.GetSafeNormal();
			P.Line(At + N * Inner, At + N * Outer, Color, Width);
		}
	};
	if (View.KillMark > 0.f)
	{
		const float Pop = 1.f + .35f * FMath::Sin(FMath::Min(View.KillMark * 3.f, 1.f) * PI); // pops out, settles
		Cross(C, 10.f * Pop, 24.f * Pop, T::Danger().CopyWithNewOpacity(1.f - View.KillMark * View.KillMark), 3.f);
		Rec(TEXT("killmark"), C.X - 24.f, C.Y - 24.f, 48.f, 48.f);
	}
	else if (View.HitMark > 0.f)
	{
		Cross(C, 9.f, 17.f, T::Neutral().CopyWithNewOpacity(1.f - View.HitMark), 2.f);
		Rec(TEXT("hitmark"), C.X - 17.f, C.Y - 17.f, 34.f, 34.f);
	}
	if (View.bKillAtTarget)
	{
		Cross(View.KillPos, 0.f, 16.f + 10.f * View.KillAtTarget, T::Danger().CopyWithNewOpacity(1.f - View.KillAtTarget), 3.f);
		Rec(TEXT("killtarget"), View.KillPos.X - 18.f, View.KillPos.Y - 18.f, 36.f, 36.f);
	}
	if (View.bHasLead)
	{
		P.Arc(View.Lead, 7.f, 1.f, T::Neutral().CopyWithNewOpacity(.9f), 2.f);
		Rec(TEXT("lead"), View.Lead.X - 7.f, View.Lead.Y - 7.f, 14.f, 14.f);
	}
	// ---- wingmen off screen: hollow chevron in squad colour, pulsing at 1 Hz in trouble
	for (const FWingArrow& Wing : View.Wings)
	{
		const float O = Wing.bTrouble ? .5f + .5f * FMath::Sin(Now * 2.f * PI) : .85f;
		P.Chevron(Wing.Pos, Wing.Angle, 18.f, T::Ship(Wing.Ship).CopyWithNewOpacity(O), false);
		Rec(TEXT("wing"), Wing.Pos.X - 9.f, Wing.Pos.Y - 9.f, 18.f, 18.f);
	}

	// ---- hit direction arcs (radius 150, 50 degrees, 8 px)
	for (const FHitArc& Hit : View.Hits)
	{
		const float HalfWidth = FMath::DegreesToRadians(25.f);
		P.ArcSpan(C, HitRadius, Hit.Angle - HalfWidth, 2.f * HalfWidth, (Hit.bShieldOnly ? T::Shield() : T::Danger()).CopyWithNewOpacity(Hit.Opacity), 8.f);
		Rec(TEXT("hitarc"), C.X - HitRadius, C.Y - HitRadius, 2.f * HitRadius, 2.f * HitRadius);
	}

	// ---- around the reticle: Switch Strike ring, heat arc, ability pip, second power pip, ammo, altitude
	if (View.SwitchStrike > 0.f)
	{
		// The reward moment: on opening, a squad-colour ring bursts out from the reticle and fades (0.35 s); then the
		// window drains clockwise, with "+25%" beside it.
		if (View.SwitchStrikePulse > 0.f)
		{
			const float T01 = View.SwitchStrikePulse;
			P.Arc(C, FMath::Lerp(62.f, 96.f, T01), 1.f, Accent.CopyWithNewOpacity(.8f * (1.f - T01)), FMath::Lerp(6.f, 1.5f, T01));
		}
		P.Arc(C, 62.f, View.SwitchStrike, Accent.CopyWithNewOpacity(.9f), 2.5f, -PI * .5f);
		const float Pop = View.SwitchStrikePulse > 0.f ? 1.f + .25f * FMath::Sin(View.SwitchStrikePulse * PI) : 1.f;
		P.Words(C.X + 70.f, C.Y - 74.f, TEXT("+25%"), FMath::RoundToInt(17 * Pop), Accent, "Bold", 0.f, 20);
		Rec(TEXT("switchstrike"), C.X - 62.f, C.Y - 80.f, 140.f, 142.f);
	}
	Area += View.SwitchStrike > 0.f ? 60.f * 22.f : 0.f;
	if (View.Focus > 0.f)
	{
		// A focus-fire order: a small amber line under the reticle cluster with its own drain bar.
		P.Words(C.X, C.Y + 84.f, TEXT("FOCUS FIRE"), 14, T::Warning(), "Bold", .5f, 80);
		Rec(TEXT("focus"), C.X - 55.f, C.Y + 84.f, 110.f, 24.f);
		P.Bar(C.X - 40.f, C.Y + 104.f, 80.f, 3.f, View.Focus, T::Warning(), .9f);
		Area += 110.f * 26.f;
	}
	if (View.Heat >= .6f)
	{
		const FLinearColor HeatC = View.Heat >= .9f ? T::Danger() : T::Bastion();
		const float Half60 = FMath::DegreesToRadians(30.f);
		P.ArcSpan(C, 30.f, PI * .5f - Half60, 2.f * Half60 * View.Heat, HeatC, 3.f);
		if (View.Heat >= .9f)
		{
			P.Words(C.X, C.Y + 34.f, TEXT("HOT"), 14, T::Danger(), "Bold", .5f, 60);
			Rec(TEXT("heat"), C.X - 30.f, C.Y + 20.f, 60.f, 32.f);
		}
	}
	if (View.bHasAbility)
	{
		P.Pip(C + FVector2D(-PipOffsetX, PipOffsetY), 14.f, View.Ability, View.Ability >= .999f, Accent, View.AbilityGlyph, 13);
		Rec(TEXT("ability"), C.X - PipOffsetX - 14.f, C.Y + PipOffsetY - 14.f, 28.f, 28.f);
	}
	float AmmoX = C.X + PipOffsetX + PipRadius + 8.f;
	if (View.bHasPower2)
	{
		const FVector2D P2(C.X + PipOffsetX + 40.f, C.Y + PipOffsetY);
		P.Pip(P2, 12.f, View.Power2, View.Power2 >= .999f, Accent, View.PowerGlyph, 12);
		Rec(TEXT("power2"), P2.X - 12.f, P2.Y - 12.f, 24.f, 24.f);
		P.Words(P2.X, P2.Y + 13.f, TEXT("HOLD"), 10, T::Dim(), "Bold", .5f, 40);
		AmmoX = P2.X + 12.f + 8.f;
	}
	if (View.bUsesAmmo)
	{
		P.Words(AmmoX, C.Y + PipOffsetY - 10.f, FString::FromInt(View.Ammo), 15, T::Neutral().CopyWithNewOpacity(View.Ammo > 0 ? .9f : .4f), "Bold", 0.f, 0);
		Rec(TEXT("ammo"), AmmoX, C.Y + PipOffsetY - 10.f, 24.f, 18.f);
	}
	if (View.Altitude >= 0.f)
	{
		P.Words(C.X - 110.f, C.Y - 5.f, FString::Printf(TEXT("ALT %.0f m"), View.Altitude), 15, View.Altitude < 60.f ? T::Bastion() : T::Neutral(),
			"Bold", 1.f, 20);
		Rec(TEXT("altitude"), C.X - 200.f, C.Y - 5.f, 90.f, 18.f);
	}

	// ---- missile warning, centred above the reticle
	if (View.Missile != ETDMissileWarning::None)
	{
		const float O = View.Missile == ETDMissileWarning::Imminent ? Pulse25 : 1.f;
		P.Plate(CenterX - 75.f, 444.f, 150.f, 28.f, 6.f, T::Plate().CopyWithNewOpacity(.6f * O));
		Rec(TEXT("missile"), CenterX - 75.f, 444.f, 150.f, 28.f);
		P.Poly(FVector2D(CenterX - 56.f, 458.f), { { 0, -8 }, { 8, 6 }, { -8, 6 }, { 0, -8 } }, Enemy.CopyWithNewOpacity(O), 2.f);
		P.Words(CenterX + 10.f, 449.f, TEXT("MISSILE"), 15, Enemy.CopyWithNewOpacity(O), "Bold", .5f, 80);
	}

	// ---- boundary warning
	if (!View.Boundary.IsEmpty())
	{
		P.Plate(CenterX - 260.f, 132.f, 520.f, 28.f, 14.f, T::Plate().CopyWithNewOpacity(.6f));
		Rec(TEXT("boundary"), CenterX - 260.f, 132.f, 520.f, 28.f);
		P.Poly(FVector2D(CenterX - 240.f, 146.f), { { 0, -8 }, { 8, 6 }, { -8, 6 }, { 0, -8 } }, T::Warning(), 2.f);
		P.Words(CenterX + 8.f, 136.f, View.Boundary, 15, T::Warning(), "Bold", .5f, 40);
	}

	// ---- boost (with the dodge notch) and hyperdrive bars, under the ship
	if (View.bShowBoost)
	{
		P.Bar(CenterX - 80.f, 792.f, 160.f, 6.f, View.Boost, Accent, .9f);
		Rec(TEXT("boost"), CenterX - 80.f, 791.f, 168.f, 8.f);
		P.Box(CenterX + 84.f, 791.f, 2.f, 8.f, (View.bDodgeReady ? T::Neutral() : T::Dim()).CopyWithNewOpacity(View.bDodgeReady ? .95f : .4f));
	}
	if (View.Hyperdrive > 0.f)
	{
		P.Bar(CenterX - 120.f, 806.f, 240.f, 6.f, View.Hyperdrive, T::Shield(), .95f);
		Rec(TEXT("hyperdrive"), CenterX - 120.f, 806.f, 240.f, 6.f);
	}

	// ---- the onboarding card (T3): highlighted key + action at 24 px, "2 / 4", a tick when done, then a fade
	if (View.Lane == ELane::Hint && View.bHintCard)
	{
		const float Fade = View.bHintDone ? 1.f - FMath::Clamp((View.HintDone01 - .5f) * 2.f, 0.f, 1.f) : 1.f;
		const FSlateFontInfo KeyFont = P.Font("Bold", 20, 30);
		const FSlateFontInfo ActFont = P.Font("Bold", 24, 60);
		const float KeyW = FMath::Max(40.f, P.Measure(View.HintKey, KeyFont) + 22.f);
		const float ActW = FMath::Max(120.f, P.Measure(View.HintAction, ActFont));
		const bool bCounter = View.HintCount > 0;
		const float CardW = FMath::Clamp(28.f + KeyW + 16.f + ActW + (bCounter ? 70.f : 0.f) + 28.f, 300.f, 900.f);
		const float CardH = 60.f;
		const float X = CenterX - CardW * .5f, Y = 830.f;
		Area += CardW * CardH;
		P.Plate(X, Y, CardW, CardH, 12.f, T::Plate().CopyWithNewOpacity(.72f * Fade));
		Rec(TEXT("card"), X, Y, CardW, CardH);
		P.Outline(X, Y, CardW, CardH, 12.f, 1.5f, (View.bHintDone ? Accent : T::Neutral()).CopyWithNewOpacity(.35f * Fade));
		// The key: a filled plate in the flown ship's colour with dark text, the one thing to look at.
		const float KX = X + 28.f, KY = Y + 13.f;
		P.Plate(KX, KY, KeyW, 34.f, 7.f, Accent.CopyWithNewOpacity(Fade));
		P.Words(KX + KeyW * .5f, KY + 5.f, View.HintKey, 20, T::Plate().CopyWithNewOpacity(Fade), "Bold", .5f, 30);
		P.Words(KX + KeyW + 16.f, Y + 13.f, View.HintAction, 24, T::Neutral().CopyWithNewOpacity(Fade), "Bold", 0.f, 60);
		if (bCounter)
		{
			P.Words(X + CardW - 24.f, Y + 19.f, FString::Printf(TEXT("%d / %d"), View.HintStep, View.HintCount), 15, T::Dim().CopyWithNewOpacity(Fade), "Bold", 1.f, 40);
		}
		if (View.bHintDone)
		{
			// The tick: a check mark over the key, popping in over the first half of the beat.
			const float Pop = FMath::Clamp(View.HintDone01 * 4.f, 0.f, 1.f);
			const FVector2D Tk(KX + KeyW * .5f, KY + 17.f);
			P.Poly(Tk, { { -9, 0 }, { -3, 7 }, { 10, -8 } }, T::Neutral().CopyWithNewOpacity(Fade * Pop), 3.f, 1.f + .3f * (1.f - Pop));
		}
	}
	// ---- the lower lane: one message
	else if (View.Lane != ELane::None && !View.LaneText.IsEmpty())
	{
		const FLinearColor LaneC = View.Lane == ELane::HyperdriveBlocked ? T::Warning() : T::Neutral();
		const float LaneW = FMath::Clamp(P.Measure(View.LaneText, P.Font("Bold", 17, 50)) + 48.f, 240.f, 760.f);
		Area += LaneW * 36.f;
		P.Plate(CenterX - LaneW * .5f, 838.f, LaneW, 36.f, 8.f, T::Plate().CopyWithNewOpacity(View.Lane == ELane::Hint ? .5f : .65f));
		Rec(TEXT("lane"), CenterX - LaneW * .5f, 838.f, LaneW, 36.f);
		if (View.Lane == ELane::HyperdriveBlocked)
		{
			// A padlock: shackle arc + body.
			const FVector2D L(CenterX - LaneW * .5f + 20.f, 858.f);
			P.ArcSpan(L + FVector2D(0, -4), 5.f, PI, PI, LaneC, 2.f);
			P.Outline(L.X - 7.f, L.Y - 4.f, 14.f, 10.f, 2.f, 1.5f, LaneC);
		}
		P.Words(CenterX + (View.Lane == ELane::HyperdriveBlocked ? 10.f : 0.f), 845.f, View.LaneText, 17, LaneC, "Bold", .5f, 50);
	}

	// ---- subtitles / story transmissions: centred under the lower lane, speaker tag in the pilot's colour, 2 lines max
	if (!View.Subtitle.Text.IsEmpty())
	{
		const FSubtitle& Sub = View.Subtitle;
		const FString& Who = Sub.Speaker;
		const FLinearColor Tag = Who == TEXT("WREN") ? T::Rift() : Who == TEXT("KATO") ? T::Lancer() : Who == TEXT("MOSS") ? T::Bastion()
			: Who == TEXT("ARMADA") ? T::Danger() : Who == TEXT("OLDER") ? T::Shield() : T::Dim();
		// Fade in 0.15 s, out over the last 0.3 s.
		const float Fade = FMath::Min(FMath::Clamp(Sub.Age / .15f, 0.f, 1.f), FMath::Clamp((Sub.Seconds - Sub.Age) / .3f, 0.f, 1.f));
		// Static: the line flickers and jitters like a bad channel; Echo: a faint late copy trails it.
		const bool bStatic = Sub.Style == ETransmissionStyle::Static;
		const bool bEcho = Sub.Style == ETransmissionStyle::Echo;
		const float Flicker = bStatic ? .65f + .35f * FMath::Abs(FMath::Sin(Now * 37.f) * FMath::Cos(Now * 23.f)) : 1.f;
		const float Jitter = bStatic ? FMath::Sin(Now * 61.f) * 2.f : 0.f;
		// Two lines at most: split at the space nearest the middle when the line is long (78 chars fit at 22 px).
		FString Line1 = Sub.Text, Line2;
		const float FullW = P.Measure(Sub.Text, P.Font("Medium", 22, 10));
		if ((FullW > SubtitleMaxWidth - 160.f) || (FullW <= 0.f && Sub.Text.Len() > 64))
		{
			int32 Split = INDEX_NONE;
			for (int32 D = 0; D < Sub.Text.Len() / 2 && Split == INDEX_NONE; ++D)
			{
				for (const int32 I : { Sub.Text.Len() / 2 + D, Sub.Text.Len() / 2 - D })
				{
					if (Sub.Text.IsValidIndex(I) && Sub.Text[I] == TEXT(' '))
					{
						Split = I;
						break;
					}
				}
			}
			if (Split != INDEX_NONE)
			{
				Line1 = Sub.Text.Left(Split);
				Line2 = Sub.Text.Mid(Split + 1);
			}
		}
		const float H = Line2.IsEmpty() ? 38.f : 64.f;
		const float X = CenterX - SubtitleMaxWidth * .5f;
		P.Plate(X, SubtitleY, SubtitleMaxWidth, H, 8.f, T::Plate().CopyWithNewOpacity(.6f * Fade));
		Rec(TEXT("subtitle"), X, SubtitleY, SubtitleMaxWidth, H);
		Area += SubtitleMaxWidth * H;
		if (bStatic)
		{
			for (int32 S = 0; S < 3; ++S) // scan lines drifting through the plate
			{
				const float Y = SubtitleY + FMath::Fmod(Now * 40.f + S * 21.f, H);
				P.Box(X + 6.f, Y, SubtitleMaxWidth - 12.f, 1.f, T::Neutral().CopyWithNewOpacity(.08f * Fade));
			}
		}
		const float TextX = X + 22.f + Jitter;
		if (!Who.IsEmpty())
		{
			P.Words(TextX, SubtitleY + 8.f, Who, 16, Tag.CopyWithNewOpacity(Fade * Flicker), "Bold", 0.f, 80);
		}
		const float BodyX = TextX + (Who.IsEmpty() ? 0.f : 120.f);
		const FLinearColor Body = T::Neutral().CopyWithNewOpacity(Fade * Flicker);
		if (bEcho)
		{
			P.Words(BodyX + 4.f, SubtitleY + 9.f, Line1, 22, T::Shield().CopyWithNewOpacity(.28f * Fade), "Medium", 0.f, 10);
		}
		P.Words(BodyX, SubtitleY + 6.f, Line1, 22, Body, "Medium", 0.f, 10);
		if (!Line2.IsEmpty())
		{
			if (bEcho)
			{
				P.Words(BodyX + 4.f, SubtitleY + 35.f, Line2, 22, T::Shield().CopyWithNewOpacity(.28f * Fade), "Medium", 0.f, 10);
			}
			P.Words(BodyX, SubtitleY + 32.f, Line2, 22, Body, "Medium", 0.f, 10);
		}
	}

	// ---- squad strip, bottom-left of the safe frame, order 1 2 3
	const float Pulse1Hz = .75f + .25f * FMath::Sin(Time * 2.f * PI);
	for (const FChip& Chip : View.Chips)
	{
		const FLinearColor Ship = T::Ship(Chip.Ship);
		const float O = Chip.bActive ? 1.f : .72f;
		const float X = Chip.Pos.X, Y = Chip.Pos.Y, W = Chip.Size.X, H = Chip.Size.Y;
		const float S = StripScale;
		// The shared backing, a step darker on wingman chips (their contents sit at 72%; they faded into bright nebula).
		P.Plate(X, Y, W, H, 6.f * S, T::Plate().CopyWithNewOpacity(Chip.bActive ? FPaint::BackingOpacity : FPaint::BackingOpacity + .12f));
		Rec(TEXT("chip"), X, Y, W, H);
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
		// The shared backing (was 45%: low contrast on bright nebula, Lead + N-037); text up to 85% collapsed.
		P.Plate(X, Y, W, H, 14.f, T::Plate().CopyWithNewOpacity(FMath::Lerp(FPaint::BackingOpacity, .75f, E)));
		Rec(TEXT("objective"), X, Y, W, H);
		const float TextO = FMath::Lerp(.85f, 1.f, E);
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
