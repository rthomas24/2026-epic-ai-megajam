// Triad Drift - engineering space HUD (ENG-36, Teammate 2). Spec: docs/eng/hud-redesign.md.

#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "TriadDrift/Combat/TDCombatPerf.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Camera/PlayerCameraManager.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Input/TDHintComponent.h"
#include "TriadDrift/Squad/TDBarkLines.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Story/TDStoryTypes.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/UI/TDUIPaint.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDVerdanceApproach.h"
#include "TriadDrift/World/TDDockingBay.h"
#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Input/TDSettingsSubsystem.h"
#if __has_include("TriadDrift/UI/TDHUDRadarPaint.h")
#include "TriadDrift/UI/TDHUDRadarPaint.h"
#endif
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "EngineUtils.h"
#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"

static_assert(TDPickupTypeCount == 5, "UTDFlightHUDWidget::PickupSeen holds one count per pickup type");

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
			FSlateDrawElement::MakeBox(List, Layer, Geo({ X, Y }, { W, H }), RoundedBrush(Radius * Scale, 0.f), ESlateDrawEffect::None, Faded(C));
		}
		/** A rounded-rectangle outline as a polyline (the rounded brush with a transparent fill still filled, live). */
		void Outline(float X, float Y, float W, float H, float Radius, float Width, FLinearColor C) const
		{
			const float R = FMath::Min(Radius, FMath::Min(W, H) * .5f);
			TArray<FVector2D>& Points = ScratchPoints();
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
			FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Faded(C), true, Width * Scale);
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
			FSlateFontInfo Shown = F;
			Shown.OutlineSettings.OutlineColor.A *= Alpha; // the dark outline fades with the HUD too
			FSlateDrawElement::MakeText(List, Layer + 2, Geo({ X - W * Align, Y }, { 1, 1 }), S, Shown, ESlateDrawEffect::None, Faded(C));
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
			TArray<FVector2D>& Points = ScratchPoints();
			for (const FVector2D& Q : Local)
			{
				Points.Add((At + Q * Size) * Scale + Origin);
			}
			FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Faded(C), true, Width * Scale);
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
				TArray<FVector2D>& Hex = ScratchPoints();
				for (int32 I = 0; I <= 6; ++I)
				{
					const float A = PI / 6.f + I * PI / 3.f;
					Hex.Add((C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * H) * Scale + Origin);
				}
				FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Hex, ESlateDrawEffect::None, Faded(Color), true, 2.2f * Scale);
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
			TArray<FVector2D>& Points = ScratchPoints();
			for (const FVector2D& Q : { Tip, L, Notch, R, Tip })
			{
				Points.Add(Q * Scale + Origin);
			}
			FSlateDrawElement::MakeLines(List, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Faded(Color), true, 2.f * Scale);
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

static TAutoConsoleVariable<int32> CVarTDUILogBarks(
	TEXT("TD.UI.LogBarks"), 0,
	TEXT("1: the engineering space HUD logs each bark request and what it did with it (shown / no line / story)."),
	ECVF_Default);

DEFINE_LOG_CATEGORY_STATIC(LogTDFlightHUD, Log, All);

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
		PixelScale = FMath::Max(.01f, MyGeometry.GetAccumulatedLayoutTransform().GetScale());
	}
	Sample(InDeltaTime);
}

void UTDFlightHUDWidget::NativeDestruct()
{
	FTDFieldDamage::OnPlayerFieldHit.Remove(FieldHitHandle);
	FieldHitHandle.Reset();
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
	if (UTDStorySubsystem* Story = BoundStory.Get())
	{
		Story->bDrawnExternally = false; // the HUD went away: T3's fallback overlay draws transmissions again
	}
	BoundStory.Reset();
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
	View.Subtitle.bStory = Style != ETransmissionStyle::Clean;
	View.Subtitle.TagColor = Style == ETransmissionStyle::Echo ? FTDFlightHUDTokens::Shield() : FTDFlightHUDTokens::Dim();
}

void UTDFlightHUDWidget::HandleBarkRequested(FName BarkId, FName Speaker)
{
	// A transmission also fires OnBarkRequested with its own id: T3's subsystem draws that one (no double line). And a story
	// transmission on screen outranks a squad bark (it is the line the player must not miss).
	const bool bLog = CVarTDUILogBarks.GetValueOnGameThread() != 0;
	FTDTransmission Entry;
	if (UTDStorySubsystem::FindEntry(BarkId, Entry) || (!View.Subtitle.Text.IsEmpty() && View.Subtitle.bStory))
	{
		UE_CLOG(bLog, LogTDFlightHUD, Display, TEXT("TD UI bark %s by %s: skipped (story transmission)"), *BarkId.ToString(), *Speaker.ToString());
		return;
	}
	// The real line from dialogs.md (T3's FTDBarkLines: Nicholas's DT_Barks wins row by row; the same line every caller
	// sees for 3 s, so this HUD and the dev HUD agree). No line, no subtitle: no more text made up from the id.
	FText Line;
	if (!FTDBarkLines::PickLine(this, BarkId, Speaker, Line) || Line.IsEmpty())
	{
		UE_CLOG(bLog, LogTDFlightHUD, Display, TEXT("TD UI bark %s by %s: no line"), *BarkId.ToString(), *Speaker.ToString());
		return;
	}
	UE_CLOG(bLog, LogTDFlightHUD, Display, TEXT("TD UI bark %s by %s: shown \"%s\" (%s, in viewport %d)"), *BarkId.ToString(), *Speaker.ToString(),
		*Line.ToString(), *GetName(), IsInViewport() ? 1 : 0);
	ShowTransmission(Speaker, Line, ETransmissionStyle::Clean, BarkSeconds);
	const FString& Who = View.Subtitle.Speaker;
	View.Subtitle.TagColor = Who == TEXT("WREN") ? FTDFlightHUDTokens::Rift() : Who == TEXT("KATO") ? FTDFlightHUDTokens::Lancer()
		: Who == TEXT("MOSS") ? FTDFlightHUDTokens::Bastion() : FTDFlightHUDTokens::Dim();
}

void UTDFlightHUDWidget::SampleStory()
{
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(this);
	if (!Story)
	{
		return;
	}
	Story->bDrawnExternally = true; // T3's fallback overlay steps aside while this HUD draws the line
	BoundStory = Story;
	const FTDTransmissionView T = Story->GetCurrentTransmission();
	if (!T.bActive)
	{
		return;
	}
	const ETransmissionStyle Style = T.Style == ETDTransmissionStyle::Static ? ETransmissionStyle::Static
		: T.Style == ETDTransmissionStyle::Echo ? ETransmissionStyle::Echo : ETransmissionStyle::Clean;
	FSubtitle& Sub = View.Subtitle;
	const FString Text = T.Text.ToString();
	if (!Sub.bStory || Sub.Text != Text)
	{
		Sub.Speaker = T.SpeakerTag.ToString().ToUpper();
		Sub.Text = Text;
		Sub.Style = Style;
	}
	Sub.bStory = true;
	Sub.TagColor = T.Color;
	// ENG-45 (T3): FTDTransmissionView::bShowSubtitle, read by name (on when the field is not there yet).
	static const FBoolProperty* ShowProp = FindFProperty<FBoolProperty>(FTDTransmissionView::StaticStruct(), TEXT("bShowSubtitle"));
	Sub.bStoryHidden = ShowProp && !ShowProp->GetPropertyValue_InContainer(&T);
	// Drive the fade from the subsystem's own clock (one line at a time, never overlapping).
	const float Remaining01 = FMath::Max(1.f - T.Progress01, .01f);
	Sub.Seconds = FMath::Max(.5f, T.TimeLeft / Remaining01);
	Sub.Age = Sub.Seconds * T.Progress01;
}

uint8 UTDFlightHUDWidget::KindOf(const AActor* Actor)
{
	const ATDEnemyShip* Ship = Cast<ATDEnemyShip>(Actor);
	if (!Ship)
	{
		return KindOther;
	}
	const FName Id = Ship->GetEnemyId();
	return Id == TDEnemyIds::ShardFighter ? KindFighter : Id == TDEnemyIds::HookInterceptor ? KindInterceptor
		: Id == TDEnemyIds::MaulBomber ? KindBomber : Id == TDEnemyIds::WallGunship ? KindGunship
		: Id == TDEnemyIds::HiveCarrier ? KindCarrier : KindOther;
}

const TCHAR* UTDFlightHUDWidget::KindName(uint8 Kind)
{
	switch (Kind)
	{
	case KindFighter: return TEXT("fighter");
	case KindInterceptor: return TEXT("interceptor");
	case KindBomber: return TEXT("bomber");
	case KindGunship: return TEXT("gunship");
	case KindCarrier: return TEXT("carrier");
	default: return TEXT("other");
	}
}

namespace TDFlightHUDShapes
{
	/** The outline for a kind, pointing down (+Y): the end that points at the ship under it, or outward off screen. */
	void Points(uint8 Kind, TArray<FVector2D>& Out, float& Width)
	{
		Width = 2.f;
		switch (Kind)
		{
		case UTDFlightHUDWidget::KindFighter: Out = { { -7, -5 }, { 0, 5 }, { 7, -5 } }; break;
		case UTDFlightHUDWidget::KindInterceptor: Out = { { -3, -9 }, { 0, 8 }, { 3, -9 }, { -3, -9 } }; break;
		case UTDFlightHUDWidget::KindBomber: Out = { { -12, -4 }, { 0, 5 }, { 12, -4 }, { -12, -4 } }; break;
		case UTDFlightHUDWidget::KindGunship: Out = { { 0, -8 }, { 8, 0 }, { 0, 8 }, { -8, 0 }, { 0, -8 } }; Width = 3.f; break;
		case UTDFlightHUDWidget::KindCarrier:
			for (int32 I = 0; I <= 6; ++I)
			{
				const float A = PI / 2.f + I * PI / 3.f;
				Out.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * 11.f);
			}
			Width = 2.5f;
			break;
		default: Out = { { -7, -6 }, { 7, -6 }, { 0, 6 }, { -7, -6 } }; break;
		}
	}
}

void UTDFlightHUDWidget::ShowNavigationPulse(const FVector& From, const FVector& To, float Seconds)
{
	NavWorldFrom = From;
	NavWorldTo = To;
	NavSeconds = FMath::Max(.3f, Seconds);
	NavLeft = NavSeconds;
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
	const float Target = FMath::Min(View.bInCut ? 0.f : View.bInSlate ? SlateOpacity : View.bPaused ? PauseOpacity : 1.f,
		FMath::Clamp(bPlanetClosed ? 1.f : FocusOpacity, 0.f, 1.f)); // #161: the closed-planet warning is never dimmed
	View.bCombatHeld = bFocusHideCombat;
	// A director shot (the highlight money shots with their letterbox bars, T3's frame: the bars cut across the panel
	// and radar): they leave for the whole shot, whether or not the feel subsystem reports slow-mo, and come back after.
	const UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(this);
	const UTDCutPresentation* Cut = Director ? Director->GetCutPresentation() : nullptr;
	const bool bShot = bMoney || (Director && Director->IsShotActive()) || (Cut && Cut->IsShotLetterboxHeld());
	View.ShotHide = FMath::FInterpConstantTo(View.ShotHide, bShot ? 1.f : 0.f, RealDt, 1.f / MoneyShotEaseSeconds);
	// Off at once on a cut; everything else eases (the fade back in after CUT is the 250 ms the spec asks for).
	View.HUDOpacity = View.bInCut ? 0.f : FMath::FInterpConstantTo(View.HUDOpacity, Target, RealDt, 1.f / CutFadeInSeconds);
	// Applied in NativePaint through the painter's Alpha (render opacity does not reach custom-painted elements).
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
	for (const FTDTargetMarker& M : FrameMarkers)
	{
		FMark& Mark = View.Marks.AddDefaulted_GetRef();
		Mark.bCurrent = M.bIsCurrentTarget || (Current && M.Actor.Get() == Current);
		Mark.bLocked = M.bLocked;
		Mark.bMissile = M.bIncomingMissile;
		Mark.bFocus = Focus && M.Actor.Get() == Focus;
		Mark.Distance = M.Distance / 100.f;
		Mark.bOnScreen = M.bOnScreen;
		if (const AActor* A = M.Actor.Get(); A && !M.bIncomingMissile)
		{
			Mark.Kind = KindOf(A);
			// Toward / away / crossing, from its nose against the line to the player (the view).
			const FVector Fwd = A->GetActorForwardVector();
			const FVector ToMe = (ViewLoc - A->GetActorLocation()).GetSafeNormal();
			const float Dot = FVector::DotProduct(Fwd, ToMe);
			Mark.Heading = Dot > .6f ? 1 : (Dot < -.6f ? 2 : 0);
			FVector2D From2, Ahead2, Dir2;
			if (Project(A->GetActorLocation(), From2, Dir2) && Project(A->GetActorLocation() + Fwd * 3000.f, Ahead2, Dir2) && !(Ahead2 - From2).IsNearlyZero())
			{
				Mark.HeadingAngle = FMath::Atan2(Ahead2.Y - From2.Y, Ahead2.X - From2.X);
			}
			const APawn* Mine = PC ? PC->GetPawn() : nullptr;
			const APawn* EnemyPawn = Cast<APawn>(A);
			const ATDEnemyAIController* AI = EnemyPawn ? Cast<ATDEnemyAIController>(EnemyPawn->GetController()) : nullptr;
			Mark.bThreat = Mine && AI && AI->GetTarget() == Mine;
			Mark.ScreenRadius = FMath::Clamp(A->GetSimpleCollisionRadius() / FMath::Max(M.Distance, 1.f) * PxPerRadian, 0.f, 220.f);
		}
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
	for (const AActor* Marker : FrameObjectives)
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

	// Dominion approach marker (alignment a13): pending once Cinder's anchor objective is in the run's progress and the
	// field is still closed; on the reveal (wave 5 cleared) it reads open for 8 s, then the assault's own markers take over.
	{
		bool bAnchorDown = false, bRevealed = false;
		if (const ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
		{
			const FTDProgress Progress = Flow->GetProgress();
			bAnchorDown = Progress.CompletedObjectives.Contains(AnchorObjectiveId);
			bRevealed = Progress.bDominionRevealed;
		}
		if (bApproachSeeded && bRevealed && !bApproachWasRevealed)
		{
			ApproachOpenLeft = ApproachOpenSeconds;
		}
		bApproachSeeded = true;
		bApproachWasRevealed = bRevealed;
		ApproachOpenLeft = FMath::Max(0.f, ApproachOpenLeft - Dt);
		View.Approach = ForceApproach >= 0 ? ForceApproach : (ApproachOpenLeft > 0.f ? 2 : (bAnchorDown && !bRevealed ? 1 : 0));
		const ATDDominion* Dominion = ATDDominion::Get(this);
		const TOptional<FVector> Target = Dominion ? TOptional<FVector>(Dominion->GetActorLocation()) : ApproachTargetOverride;
		if (View.Approach > 0 && Target.IsSet())
		{
			FWaypoint& Wp = View.ApproachWp;
			FVector2D Dir;
			Wp.bOnScreen = Project(Target.GetValue(), Wp.Pos, Dir);
			if (!Wp.bOnScreen)
			{
				Wp.Pos = AvoidStrip(OnSafeFrame(Dir));
				Wp.Angle = FMath::Atan2(Dir.Y, Dir.X);
			}
			Wp.Distance = FVector::Dist(From, Target.GetValue()) / 100.f;
			Wp.DrawPos = Wp.Pos;
			Wp.bLifted = Wp.bOnScreen && LiftWaypoint(Wp.Pos, Wp.DrawPos);
		}
		else
		{
			View.Approach = 0;
		}
	}

	// T6's field: the quiet hit tick (a player shot into a rock).
	if (!FieldHitHandle.IsValid())
	{
		FieldHitHandle = FTDFieldDamage::OnPlayerFieldHit.AddUObject(this, &UTDFlightHUDWidget::HandleFieldHit);
	}
	FieldHitLeft = FMath::Max(0.f, FieldHitLeft - Dt);
	View.FieldHit01 = FieldHitLeft / FieldHitSeconds;

	// Space pickups: a marker for each within 800 m, a toast when one is taken, the overcharge timer.
	View.Pickups.Reset();
	View.Overcharge01 = View.OverchargeSeconds = 0.f;
	PickupToastLeft = FMath::Max(0.f, PickupToastLeft - Dt);
	if (const UTDPickupSubsystem* Pickups = UTDPickupSubsystem::Get(this))
	{
		for (const ATDPickup* Pk : Pickups->GetPickups())
		{
			if (!IsValid(Pk) || FVector::DistSquared(From, Pk->GetActorLocation()) > FMath::Square(PickupMarkRange))
			{
				continue;
			}
			FView::FPickupMark& M = View.Pickups.AddDefaulted_GetRef();
			M.Type = static_cast<uint8>(Pk->GetType());
			M.Color = ATDPickup::ColorOf(Pk->GetType());
			FVector2D Dir;
			M.bOnScreen = Project(Pk->GetActorLocation(), M.Pos, Dir);
			if (!M.bOnScreen)
			{
				M.Pos = AvoidStrip(OnSafeFrame(Dir));
				M.Angle = FMath::Atan2(Dir.Y, Dir.X);
			}
			M.Distance = FVector::Dist(From, Pk->GetActorLocation()) / 100.f;
		}
		for (int32 I = 0; I < TDPickupTypeCount; ++I)
		{
			const int32 N = Pickups->GetCollected(static_cast<ETDPickupType>(I));
			if (N > PickupSeen[I])
			{
				const ETDPickupType Type = static_cast<ETDPickupType>(I);
				View.PickupToast = ATDPickup::WordOf(Type);
				View.PickupToastColor = ATDPickup::ColorOf(Type);
				PickupToastLeft = PickupToastSeconds;
			}
			PickupSeen[I] = N;
		}
		View.DebrisCount = Pickups->GetDebrisThisScene();
		View.DebrisAge = Pickups->GetSecondsSinceDebris();
		View.BreakBoostAge = Pickups->GetSecondsSinceBreakBoost();
		const float Total = Pickups->GetOverchargeSeconds();
		View.OverchargeSeconds = Pickups->GetOverchargeRemaining();
		View.Overcharge01 = Total > 0.f ? FMath::Clamp(View.OverchargeSeconds / Total, 0.f, 1.f) : 0.f;
	}
	View.PickupToast01 = PickupToastLeft > 0.f ? 1.f - PickupToastLeft / PickupToastSeconds : 0.f;
	if (PickupToastLeft <= 0.f)
	{
		View.PickupToast.Reset();
	}

	// Navigation pulse: project the corridor; off screen or no view, a generic corridor from under the ship toward the
	// screen's right third (it only has to read as a sensor flicker).
	NavLeft = FMath::Max(0.f, NavLeft - Dt);
	View.NavPulse = NavLeft > 0.f && NavSeconds > 0.f ? 1.f - NavLeft / NavSeconds : 0.f;
	if (View.NavPulse > 0.f)
	{
		FVector2D A, B, DirA, DirB;
		const bool bA = Project(NavWorldFrom, A, DirA);
		const bool bB = Project(NavWorldTo, B, DirB);
		View.NavFrom = bA ? A : FVector2D(CenterX, 760.f);
		View.NavTo = bB ? B : OnSafeFrame(bA ? DirB : FVector2D(.8f, -.35f));
	}

	// ENG-38 descent readings (T1): by name, so this builds with or without T1's statics; GetHeat() before them.
	{
		auto ReadStatic = [this](const TCHAR* Name, float Fallback) -> float
		{
			UFunction* F = UTDDescentSubsystem::StaticClass()->FindFunctionByName(FName(Name));
			if (!F || F->ParmsSize <= 0)
			{
				return Fallback;
			}
			uint8* Buf = static_cast<uint8*>(FMemory_Alloca(F->ParmsSize));
			FMemory::Memzero(Buf, F->ParmsSize);
			if (const FObjectPropertyBase* Ctx = CastField<FObjectPropertyBase>(F->PropertyLink); Ctx && !Ctx->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				Ctx->SetObjectPropertyValue_InContainer(Buf, const_cast<UTDFlightHUDWidget*>(this));
			}
			UTDDescentSubsystem::StaticClass()->GetDefaultObject()->ProcessEvent(F, Buf);
			const FFloatProperty* Ret = CastField<FFloatProperty>(F->GetReturnProperty());
			return Ret ? Ret->GetPropertyValue_InContainer(Buf) : Fallback;
		};
		const UTDDescentSubsystem* Descent = UTDDescentSubsystem::Get(this);
		View.Heat01 = ForceHeat01 >= 0.f ? ForceHeat01 : FMath::Clamp(ReadStatic(TEXT("GetEntryHeat01"), Descent ? Descent->GetHeat() : 0.f), 0.f, 1.f);
		View.Descent01 = ForceDescent01 > -2.f ? ForceDescent01 : ReadStatic(TEXT("GetDescent01"), -1.f);
		const APawn* Ship = PC ? PC->GetPawn() : nullptr;
		const ATDShip* AsShip = Cast<ATDShip>(Ship);
		const bool bEntering = AsShip && AsShip->GetAtmosphereEntry() && AsShip->GetAtmosphereEntry()->IsEntering();
		// #161 (T1): a closed planet burns the hull through the entry heat, but it is not a descent: no ATMOSPHERE panel,
		// and its warning line shows in the prompt lane for as long as the entry component reports it.
		const bool bClosed = AsShip && AsShip->GetAtmosphereEntry() && AsShip->GetAtmosphereEntry()->IsPlanetClosedWarning();
		View.bDescent = !bClosed && (View.Heat01 > .01f || View.Descent01 >= 0.f || bEntering);
		if (bClosed)
		{
			const FString Raw = AsShip->GetAtmosphereEntry()->GetPlanetClosedText().ToString();
			if (Raw != PlanetClosedRaw)
			{
				PlanetClosedRaw = Raw;
				PlanetClosedLine = Raw.ToUpper();
			}
		}
		bPlanetClosed = bClosed && !PlanetClosedLine.IsEmpty();
		// ORBIT: the descent cover ending on the galaxy map is the climb-out's reveal (the HUD is rebuilt after the travel,
		// so the edge is watched here rather than only through OnRevealed).
		const bool bCovered = Descent && Descent->GetCoverPhase() != ETDDescentCover::Off;
		const UWorld* World = GetWorld();
		if (bCoverSeen && !bCovered && World && World->GetMapName().Contains(TEXT("Galaxy")))
		{
			StartOrbitCue();
		}
		bCoverSeen = bCovered;
		// The host's record of the reveal (this HUD may be built after it): ORBIT for what is left of its seconds.
		const double Reveal = UTDUIHostSubsystem::GetLastGalaxyReveal();
		if (Reveal > 0.0 && Reveal != OrbitRevealSeen)
		{
			OrbitRevealSeen = Reveal;
			const float Since = static_cast<float>(FPlatformTime::Seconds() - Reveal);
			if (Since < OrbitCueSeconds + 1.f)
			{
				OrbitCueLeft = FMath::Max(OrbitCueLeft, FMath::Max(OrbitCueSeconds - Since, 1.5f));
			}
		}
		OrbitCueLeft = FMath::Max(0.f, OrbitCueLeft - Dt);
		View.OrbitCue01 = OrbitCueLeft / OrbitCueSeconds;
	}

	// ENG-29 docking (T4): the tractor's hold on the flown ship, then the glide in (controls locked). Read every frame:
	// the approach is short (10 s from the cone's edge to the commit).
	if (ForceDocking >= 0)
	{
		View.Docking = static_cast<uint8>(ForceDocking);
		View.TractorPull01 = ForceTractorPull01;
	}
	else
	{
		DockingScanLeft -= Dt;
		if (DockingScanLeft <= 0.f)
		{
			DockingScanLeft = .5f;
			DockingBay = nullptr;
			if (UWorld* World = GetWorld())
			{
				for (TActorIterator<ATDDockingBay> It(World); It; ++It) { if (It->IsOpen()) { DockingBay = *It; break; } }
			}
		}
		const ATDDockingBay* Bay = Cast<ATDDockingBay>(DockingBay.Get());
		const ETDDockingState State = Bay ? Bay->GetDockingState() : ETDDockingState::Closed;
		View.TractorPull01 = Bay ? FMath::Clamp(Bay->GetCurrentPull(), 0.f, 1.f) : 0.f;
		const ATDDominion* HoldDom = ATDDominion::Get(this);
		View.bReactorHold = HoldDom && HoldDom->IsReactorHoldActive();
		View.Docking = State == ETDDockingState::Gliding || State == ETDDockingState::Docked ? 2 : (State == ETDDockingState::Open && View.TractorPull01 > .01f ? 1 : 0);
	}

	// The escape flight (ENG-31, T4's timing): the clock, and the way out straight away from the hull at the exit radius.
	{
		const ATDDominion* Dom = ATDDominion::Get(this);
		const bool bForced = ForceEscapeLeft >= 0.f;
		View.bEscapeFlight = bForced || (Dom && Dom->GetState() == ETDDominionState::EscapeFlight && !Dom->IsEscapeClear());
		if (View.bEscapeFlight)
		{
			View.EscapeLeft = bForced ? ForceEscapeLeft : Dom->GetEscapeTimeLeft();
			View.EscapeTotal = FMath::Max(bForced ? ForceEscapeTotal : Dom->EscapeSeconds, 1.f);
			const FVector Centre = bForced ? ForceEscapeCentre : Dom->GetActorLocation();
			const float Radius = bForced ? ForceEscapeRadius : Dom->GetEscapeExitRadius();
			const APawn* Ship = PC ? PC->GetPawn() : nullptr;
			const FVector ShipAt = Ship ? Ship->GetActorLocation() : ViewLoc;
			const FVector Out = (ShipAt - Centre).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
			const FVector Clear = Centre + Out * Radius;
			View.ClearMetres = FMath::Max(0.f, Radius - FVector::Dist(ShipAt, Centre)) * .01f;
			FVector2D Dir;
			View.bClearOnScreen = Project(Clear, View.ClearPos, Dir);
			if (!View.bClearOnScreen)
			{
				View.ClearPos = OnSafeFrame(Dir);
			}
		}
	}

	// The transponder off Verdance (storyline.md §18): a marker on T4's beacon once it is sighted, "UNKNOWN", that reads
	// "VERIFIED" for a moment and resets. Found by iteration every 2 s (one beacon in the galaxy, only in Phase Verdance).
	{
		TransponderScanLeft -= Dt;
		if (TransponderScanLeft <= 0.f && !ForceTransponderAt.IsSet())
		{
			TransponderScanLeft = 2.f;
			TransponderBeacon = nullptr;
			if (UWorld* World = GetWorld())
			{
				for (TActorIterator<ATDTransponderBeacon> It(World); It; ++It)
				{
					if (It->IsActive()) { TransponderBeacon = *It; break; }
				}
			}
		}
		const ATDTransponderBeacon* Beacon = Cast<ATDTransponderBeacon>(TransponderBeacon.Get());
		const bool bSighted = ForceTransponderAt.IsSet() || (Beacon && Beacon->IsActive() && Beacon->WasSighted());
		const FVector At = ForceTransponderAt.IsSet() ? ForceTransponderAt.GetValue() : (Beacon ? Beacon->GetActorLocation() : FVector::ZeroVector);
		const float Metres = FVector::Dist(ViewLoc, At) * .01f;
		View.bTransponder = bSighted && (ForceTransponderAt.IsSet() || (Beacon && Metres * 100.f <= Beacon->NoticeRadius));
		if (View.bTransponder)
		{
			TransponderSeenFor = ForceTransponderAge >= 0.f ? ForceTransponderAge : FMath::Max(0.f, TransponderSeenFor) + Dt;
			FVector2D Dir;
			View.bTransponderOnScreen = Project(At, View.TransponderPos, Dir);
			if (!View.bTransponderOnScreen)
			{
				View.TransponderPos = OnSafeFrame(Dir);
			}
			View.TransponderMetres = Metres;
			const float Since = TransponderSeenFor - TransponderVerifiedAt;
			// Twice, briefly: on for .45 s, a stutter off, on for .12 s, then UNKNOWN for good.
			View.bTransponderVerified = (Since >= 0.f && Since < TransponderVerifiedFor) || (Since >= TransponderVerifiedFor + .1f && Since < TransponderVerifiedFor + .22f);
		}
		else
		{
			View.bTransponderVerified = false;
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
	SwitchNoticeLeft = FMath::Max(0.f, SwitchNoticeLeft - Dt);
	if (SwitchNoticeLeft > 0.f)
	{
		View.Lane = ELane::Prompt; // the auto-switch notice outranks a prompt for its 3 s
		View.LaneText = SwitchNotice;
	}
	else if (!Prompt.IsEmpty())
	{
		View.Lane = ELane::Prompt;
		View.LaneText = Prompt.ToUpper();
	}
	else if (bPlanetClosed)
	{
		View.Lane = ELane::HyperdriveBlocked; // the same warning look: amber words with the padlock
		View.LaneText = PlanetClosedLine;
	}
	else if (BlockedLeft > 0.f && !Blocked.IsEmpty())
	{
		View.Lane = ELane::HyperdriveBlocked;
		View.LaneText = Blocked.ToUpper();
	}
	else if ((!Hint.IsEmpty() || View.bHintCard) && !(View.Subtitle.bStory && !View.Subtitle.Text.IsEmpty()))
	// (a story transmission on air holds the tutorial card back: the two crowded each other, Lead's note on #17's sheet)
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
	FTDHUDCostScope CostScope(SampleCost);
	TD_COMBAT_PERF(FlightHUD);
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
	FrameMarkers = Data->GetTargetMarkers();
	FrameObjectives = Data->GetObjectiveMarkers();
	HUDModel.Update(GetWorld(), Cards, FrameMarkers, FrameObjectives, Dt);
	UpdateSquadCards(Dt);
	LayoutChips();

	// Reticle cluster: the flown ship.
	if (Flown)
	{
		const bool bSwitched = Flown->Ship != LastFlown;
		// The view changed because the flown ship went down (auto-switch on death, respawn rescue): say so for 3 s
		// ("KATO DOWN  ·  SWITCHED TO MOSS"), so the player knows why. A manual switch says nothing.
		if (bSwitched && LastHull >= 0.f)
		{
			const FChip* Old = View.Chips.FindByPredicate([this](const FChip& C) { return C.Ship == LastFlown; });
			if (Old && !Old->bAlive)
			{
				SwitchNotice = FString::Printf(TEXT("%s DOWN  ·  SWITCHED TO %s"), TDFlightHUD::PilotName(LastFlown), TDFlightHUD::PilotName(Flown->Ship));
				SwitchNoticeLeft = 3.f;
			}
		}
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
	SampleStory();
	// ENG-45 (T1's setting): UTDSettingsSubsystem::AreSubtitlesOn(World), by name (on without it).
	if (ForceSubtitlesOn >= 0)
	{
		bSubtitlesOn = ForceSubtitlesOn > 0;
	}
	else
	{
		static UClass* Settings = FindObject<UClass>(nullptr, TEXT("/Script/unrealGameJam.TDSettingsSubsystem"));
		UFunction* F = Settings ? Settings->FindFunctionByName(TEXT("AreSubtitlesOn")) : nullptr;
		bSubtitlesOn = true;
		if (F && F->ParmsSize > 0)
		{
			uint8* Buf = static_cast<uint8*>(FMemory_Alloca(F->ParmsSize));
			FMemory::Memzero(Buf, F->ParmsSize);
			if (const FObjectPropertyBase* Ctx = CastField<FObjectPropertyBase>(F->PropertyLink); Ctx && !Ctx->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				Ctx->SetObjectPropertyValue_InContainer(Buf, this);
			}
			Settings->GetDefaultObject()->ProcessEvent(F, Buf);
			if (const FBoolProperty* Ret = CastField<FBoolProperty>(F->GetReturnProperty()))
			{
				bSubtitlesOn = Ret->GetPropertyValue_InContainer(Buf);
			}
		}
	}
	SampleCombat(*Data, Flown, Dt);
	ArcAwake = FMath::Max(0.f, ArcAwake - Dt);
	View.ArcOpacity = (View.Hull < .999f || View.Shield < .999f || ArcAwake > 0.f) ? 1.f : ArcRestOpacity;
	PulseLeft = FMath::Max(0.f, PulseLeft - Dt);
	View.PowerPulse = PulseLeft > 0.f ? 1.f - PulseLeft / PulseSeconds : 0.f;
	View.PowerGlyph = View.bUsingGamepad ? TEXT("X") : TEXT("Q");

	// Objective line: the text in caps, the wave's enemies left as the counter; expanded for 3 s when the text changes.
	// Upper-cased and split only when the source text or wave changes (review m8), not every frame.
	const FTDWaveInfo Wave = Data->GetWaveInfo();
	FrameWave = Wave;
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
	// The Dominion assault: the generator pips are the count (the objective line names it); no "N LEFT" beside them.
	{
		int32 Total = 0, Down = 0;
		if (ForceGeneratorsTotal >= 0)
		{
			Total = ForceGeneratorsTotal;
			Down = FMath::Clamp(ForceGeneratorsDown, 0, Total);
		}
		else if (const ATDDominion* Dom = ATDDominion::Get(this); Dom && Dom->GetState() == ETDDominionState::ShieldsUp)
		{
			Total = FMath::Max(Dom->GetGenerators().Num(), 4);
			Down = FMath::Clamp(Total - Dom->GetGeneratorsRemaining(), 0, Total);
		}
		if (Total > 0 && GeneratorsDownSeen >= 0 && Down > GeneratorsDownSeen)
		{
			GeneratorFlashLeft = 1.2f;
		}
		GeneratorsDownSeen = Total > 0 ? Down : -1;
		GeneratorFlashLeft = FMath::Max(0.f, GeneratorFlashLeft - DeltaSeconds);
		View.GeneratorsTotal = Total;
		View.GeneratorsDown = Down;
		// The radar header's assault state (T6): the reactor exposed (the space ending) and its phase.
		const ATDDominion* HeaderDom = ForceGeneratorsTotal >= 0 ? nullptr : ATDDominion::Get(this);
		const ATDDominionReactor* Reactor = HeaderDom ? HeaderDom->GetReactor() : nullptr;
		View.bReactorExposed = HeaderDom && HeaderDom->GetState() == ETDDominionState::ReactorExposed;
		View.ReactorPhase = Reactor ? Reactor->GetPhase() : 0;
		View.ReactorPhases = Reactor ? Reactor->PhaseCount : 0;
		View.GeneratorFlash01 = GeneratorFlashLeft / 1.2f;
		if (Total > 0)
		{
			View.Counter.Reset();
		}
	}
	// One count, once: the generator pips and the escape clock carry the number, so the objective line's own trailing
	// "n / 4" (generators) or "· 54" (escape seconds) is cut. Only when the text ends in a digit (no work otherwise).
	if ((View.GeneratorsTotal > 0 || View.bEscapeFlight) && View.Objective.Len() > 0 && FChar::IsDigit(View.Objective[View.Objective.Len() - 1]))
	{
		int32 End = View.Objective.Len();
		auto Skip = [&](TFunctionRef<bool(TCHAR)> Is) { while (End > 0 && Is(View.Objective[End - 1])) { --End; } };
		Skip([](TCHAR C) { return FChar::IsDigit(C); });
		Skip([](TCHAR C) { return C == TEXT(' ') || C == TEXT('/') || C == TEXT('·') || C == TEXT(':'); });
		if (View.GeneratorsTotal > 0)
		{
			Skip([](TCHAR C) { return FChar::IsDigit(C); });
			Skip([](TCHAR C) { return C == TEXT(' '); });
		}
		View.Objective.LeftInline(End, EAllowShrinking::No);
	}
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
	if (View.bSquadPanel)
	{
		// #160: cards in the fixed order 1-2-3, bottom-aligned on the safe frame; the flown one is larger (eased on a switch).
		const float K = View.PanelK, Bottom = 1080.f - SafeY;
		for (FChip& Chip : View.Chips)
		{
			FSquadCard& Card = View.Cards[static_cast<int32>(Chip.Ship)];
			const float G = FMath::InterpEaseOut(0.f, 1.f, Card.Grow, 2.f);
			Card.Size = FVector2D(FMath::Lerp(WingCardW, FlownCardW, G), FMath::Lerp(WingCardH, FlownCardH, G)) * K;
			Card.Pos = FVector2D(X, Bottom - Card.Size.Y);
			Chip.Pos = Card.Pos;
			Chip.Size = Card.Size;
			X += Card.Size.X + CardGap * K;
		}
		return;
	}
	for (FChip& Chip : View.Chips)
	{
		Chip.Pos = FVector2D(X, Y);
		Chip.Size = FVector2D(Chip.bActive ? ActiveChipWidth : WingChipWidth, ChipHeight);
		X += Chip.Size.X + ChipGap;
	}
}

// ------------------------------------------------------------------------------------------ squad panel (#160)

#if !UE_BUILD_SHIPPING
// TD.UI.Cost: the flight HUD's game-thread cost since the last call (sample = reading the game, paint = drawing), in
// ms per frame: average and worst. Budget (#160): under 0.5 ms for both together.
static FAutoConsoleCommandWithWorld CmdTDUICost(TEXT("TD.UI.Cost"), TEXT("TD.UI.Cost: log the flight HUD's sample + paint cost (ms per frame, average and worst) since the last call."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		const UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(World);
		UTDFlightHUDWidget* HUD = Host ? Cast<UTDFlightHUDWidget>(Host->GetSpaceHUD()) : nullptr;
		if (!HUD)
		{
			UE_LOG(LogTemp, Display, TEXT("TD.UI.Cost: no flight HUD"));
			return;
		}
		const FTDHUDCost S = HUD->SampleCost, P = HUD->PaintCost;
		UE_LOG(LogTemp, Display, TEXT("TD.UI.Cost: %d frames; sample %.3f ms avg, %.3f worst; paint %.3f ms avg, %.3f worst; together %.3f ms avg (budget 0.5)"),
			P.Frames, S.AverageMs(), S.WorstMs(), P.AverageMs(), P.WorstMs(), S.AverageMs() + P.AverageMs());
		HUD->SampleCost = FTDHUDCost();
		HUD->PaintCost = FTDHUDCost();
	}));
#endif

static TAutoConsoleVariable<int32> CVarTDUIMinimalHUD(
	TEXT("TD.UI.MinimalHUD"), 0, TEXT("1 = the ENG-36 flight HUD (small squad chips, no squad panel or radar); 0 = the #160 HUD."));

static TAutoConsoleVariable<float> CVarTDUISubtitleScale(
	TEXT("TD.UI.SubtitleScale"), 0.f, TEXT("Dev, captures: force the flight subtitle scale (0.8 Small, 1 Medium, 1.3 Large); 0 = the player's setting."));

void UTDFlightHUDWidget::UpdateSquadCards(float Dt)
{
	// Minimal HUD: the player's setting (T1's UTDSettingsSubsystem::IsMinimalHUD, read by name once a second: off until
	// it exists), or the dev override TD.UI.MinimalHUD 1.
	MinimalReadLeft -= Dt;
	if (MinimalReadLeft <= 0.f)
	{
		MinimalReadLeft = 1.f;
		bMinimalSetting = false;
		static UClass* Settings = FindObject<UClass>(nullptr, TEXT("/Script/unrealGameJam.TDSettingsSubsystem"));
		UFunction* F = Settings ? Settings->FindFunctionByName(TEXT("IsMinimalHUD")) : nullptr;
		if (F && F->ParmsSize > 0)
		{
			uint8* Buf = static_cast<uint8*>(FMemory_Alloca(F->ParmsSize));
			FMemory::Memzero(Buf, F->ParmsSize);
			if (const FObjectPropertyBase* Ctx = CastField<FObjectPropertyBase>(F->PropertyLink); Ctx && !Ctx->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				Ctx->SetObjectPropertyValue_InContainer(Buf, this);
			}
			Settings->GetDefaultObject()->ProcessEvent(F, Buf);
			if (const FBoolProperty* Ret = CastField<FBoolProperty>(F->GetReturnProperty()))
			{
				bMinimalSetting = Ret->GetPropertyValue_InContainer(Buf);
			}
		}
	}
	View.bSquadPanel = CVarTDUIMinimalHUD.GetValueOnGameThread() == 0 && !bMinimalSetting;
	const float ForcedScale = CVarTDUISubtitleScale.GetValueOnGameThread();
	View.SubtitleScale = FMath::Clamp(ForcedScale > 0.f ? ForcedScale : UTDSettingsSubsystem::GetSubtitleScale(this), .5f, 2.f);
	// Below 900 px of height the design frame would shrink the text under its 1080 size: the cards go to 85% and the
	// text keeps its 1080 pixels instead (Lead's rule for #160).
	// Real pixels per design unit: the design frame's fit times UMG's DPI scale (T7's 720p frame: the DPI scale, not the
	// frame, was shrinking the cards and their text to 67%).
	const float Scale = FMath::Min(ViewportSize.X / 1920.f, ViewportSize.Y / 1080.f) * PixelScale;
	const bool bSmall = Scale > 0.f && Scale < 900.f / 1080.f;
	View.PanelK = bSmall ? .85f / Scale : 1.f;
	View.PanelFont = bSmall ? 1.f / Scale : 1.f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FTDSquadStatus& S = HUDModel.Squad[Index];
		FSquadCard& Card = View.Cards[Index];
		Card.bValid = S.bValid;
		if (!S.bValid)
		{
			LastCardHull[Index] = -1.f;
			continue;
		}
		Card.Ship = S.Ship;
		Card.bFlown = S.bFlown;
		Card.bAlive = S.bAlive;
		Card.Hull = S.Hull;
		Card.Shield = S.Shield;
		Card.Power01 = S.Power01;
		Card.bPowerActive = S.bPowerActive;
		Card.Ability = S.Ability;
		Card.Ability01 = S.Ability01;
		Card.bAbilityActive = S.bAbilityActive;
		Card.bTrouble = S.bTrouble && !S.bFlown;
		// Under 25% hull, any card (flown included): a red-orange pulsing edge and hull bar, so the player sees which
		// ship not to switch into.
		Card.bCritical = S.bAlive && S.Hull < .25f;
		Card.UnderFire01 = S.UnderFire01;
		// Switch: the old flown card shrinks and the new one grows over 120 ms (the first sample snaps).
		const float Target = S.bFlown ? 1.f : 0.f;
		Card.Grow = LastCardHull[Index] < 0.f ? Target : FMath::FInterpConstantTo(Card.Grow, Target, Dt, 1.f / CardGrowSeconds);
		// Damage ghost: the hull just lost stays white and drains over 400 ms.
		if (LastCardHull[Index] >= 0.f && S.Hull < LastCardHull[Index] - 1e-3f)
		{
			Card.GhostHull = FMath::Max(Card.GhostHull, LastCardHull[Index]);
			GhostRate[Index] = (Card.GhostHull - S.Hull) / .4f;
		}
		Card.GhostHull = FMath::Max(S.Hull, Card.GhostHull - GhostRate[Index] * Dt);
		LastCardHull[Index] = S.Hull;
		// Texts only when their number changes.
		const int32 HullN = FMath::RoundToInt(S.Hull * 100.f), ShieldN = FMath::RoundToInt(S.Shield * 100.f);
		if (HullN != CardHullShown[Index]) { CardHullShown[Index] = HullN; Card.HullText = FString::FromInt(HullN); }
		if (ShieldN != CardShieldShown[Index]) { CardShieldShown[Index] = ShieldN; Card.ShieldText = FString::FromInt(ShieldN); }
		const int32 DownN = S.bAlive ? -1 : FMath::CeilToInt(S.RespawnSeconds);
		if (DownN != CardDownShown[Index])
		{
			CardDownShown[Index] = DownN;
			Card.DownText = DownN < 0 ? FString() : FString::Printf(TEXT("DOWN  ·  BACK IN 0:%02d"), FMath::Min(DownN, 59));
		}
		const int32 AbSec = S.Ability01 >= .999f ? 0 : FMath::Max(1, FMath::CeilToInt(S.AbilitySeconds));
		if (AbSec != CardAbilitySecShown[Index])
		{
			CardAbilitySecShown[Index] = AbSec;
			Card.AbilitySecondsText = AbSec > 0 ? FString::FromInt(AbSec) : FString();
		}
		// Status flags. Cloak is Rift's ability, the dome Bastion's hold-Q, the afterburner Lancer's ram.
		Card.bCloaked = S.bAlive && S.bAbilityActive && S.Ability == ETDAbilityId::Cloak;
		Card.bDomeUp = S.bAlive && S.bPower2Active;
		Card.bAfterburn = S.bAlive && S.Ship == ETDShipId::Lancer && S.bPowerActive;
		Card.bOvercharge = S.bAlive && S.bFlown && S.OverchargeSeconds >= .05f; // never "OVERCHARGE 0.0" in its last frames
		const int32 OverN = Card.bOvercharge ? FMath::RoundToInt(S.OverchargeSeconds * 10.f) : -1;
		if (OverN != CardOverShown[Index])
		{
			CardOverShown[Index] = OverN;
			Card.OverchargeText = OverN < 0 ? FString() : FString::Printf(TEXT("OVERCHARGE %d.%d"), OverN / 10, OverN % 10);
		}
		const int32 KeyN = View.bUsingGamepad ? 0 : Index + 1;
		if ((KeyN == 0) != Card.Key.IsEmpty() || (KeyN > 0 && Card.Key.IsEmpty()))
		{
			Card.Key = KeyN > 0 ? FString::FromInt(KeyN) : FString();
		}
	}
	// The subtitle slot ends above the panel's flag row (Concept A: y 858..890 at 1080p).
	View.SubtitleBottom = View.bSquadPanel ? 1080.f - SafeY - FlownCardH * View.PanelK - 28.f * View.PanelK - 8.f : SubtitleY + 64.f;
	// A prompt, a hint card or the hyperdrive block is in the lower lane (y 830..890): the subtitle block (up to three
	// Large lines) sits above it, never over it, the squad panel or the radar.
	if (View.bSquadPanel && View.Lane != ELane::None)
	{
		View.SubtitleBottom = FMath::Min(View.SubtitleBottom, 822.f);
	}
}

namespace TDFlightHUD
{
	static const TCHAR* PowerShortName(ETDShipId Ship)
	{
		return Ship == ETDShipId::Rift ? TEXT("PORTAL") : Ship == ETDShipId::Lancer ? TEXT("RAM") : TEXT("SWARM");
	}
	/** Nick's FTDHUDStyle squad colours (board R-049: the new panel uses the plugin's values, copied in FTDUIPalette). */
	static FLinearColor PanelShip(ETDShipId Ship)
	{
		return Ship == ETDShipId::Rift ? FTDUIPalette::Rift() : Ship == ETDShipId::Lancer ? FTDUIPalette::Lancer() : FTDUIPalette::Bastion();
	}

	/** Ten segments with 2 px gaps: a dim track, the value, and (hull) the white ghost of what was just lost. */
	static void SegmentBar(const FPaint& P, float X, float Y, float W, float H, float Value, float Ghost, FLinearColor C)
	{
		const float Gap = 2.f, SegW = (W - Gap * 9.f) / 10.f;
		for (int32 I = 0; I < 10; ++I)
		{
			const float SX = X + I * (SegW + Gap);
			const float Fill = FMath::Clamp(Value * 10.f - I, 0.f, 1.f);
			const float GhostFill = FMath::Clamp(Ghost * 10.f - I, 0.f, 1.f);
			P.Box(SX, Y, SegW, H, FTDUIPalette::Muted().CopyWithNewOpacity(.22f));
			if (GhostFill > Fill)
			{
				P.Box(SX + SegW * Fill, Y, SegW * (GhostFill - Fill), H, FLinearColor::White.CopyWithNewOpacity(.9f));
			}
			if (Fill > 0.f)
			{
				P.Box(SX, Y, SegW * Fill, H, C);
			}
		}
	}

	/** Signature power: ready = a full ring in the ship colour with a soft fill; cooling = a grey track refilling clockwise. */
	static void PowerRing(const FPaint& P, FVector2D At, float R, float Value, bool bActive, FLinearColor Ship, const FString& Glyph, int32 GlyphSize, float O)
	{
		const bool bReady = Value >= .999f;
		P.Plate(At.X - R, At.Y - R, 2 * R, 2 * R, R, bReady ? Ship.CopyWithNewOpacity(.18f * O) : FTDUIPalette::Panel().CopyWithNewOpacity(.6f * O));
		if (bReady || bActive)
		{
			P.Arc(At, R + 2.f, 1.f, Ship.CopyWithNewOpacity(.25f * O), 4.f); // the glow: a wider, faint stroke
			P.Arc(At, R, 1.f, Ship.CopyWithNewOpacity(O), 2.5f);
		}
		else
		{
			P.Arc(At, R, 1.f, FTDUIPalette::Muted().CopyWithNewOpacity(.3f * O), 2.5f);
			P.Arc(At, R, Value, Ship.CopyWithNewOpacity(O), 2.5f, -PI * .5f);
		}
		if (!Glyph.IsEmpty())
		{
			P.Words(At.X, At.Y - GlyphSize * .68f, Glyph, GlyphSize, FTDUIPalette::Ink().CopyWithNewOpacity((bReady ? 1.f : .5f) * O), "Bold", .5f, 0);
		}
	}

	/** Unlocked ability: locked = dashed dim ring + padlock; ready = white ring with its key (or a dot); cooling = an arc. */
	static void AbilityRing(const FPaint& P, FVector2D At, float R, bool bLocked, float Value, bool bActive, const FString& Glyph, int32 GlyphSize, float O, float Time)
	{
		if (bLocked)
		{
			for (int32 I = 0; I < 8; ++I)
			{
				P.ArcSpan(At, R, I * PI * .25f, PI * .14f, FTDUIPalette::Muted().CopyWithNewOpacity(.7f * O), 1.5f);
			}
			// Padlock: a body and a shackle.
			const float B = R * .55f;
			P.Box(At.X - B, At.Y - B * .15f, 2 * B, B * 1.25f, FTDUIPalette::Muted().CopyWithNewOpacity(.95f * O));
			P.ArcSpan(At + FVector2D(0, -B * .15f), B * .68f, PI, PI, FTDUIPalette::Muted().CopyWithNewOpacity(.95f * O), 2.f);
			return;
		}
		// ACTIVE: a filled disc with a second ring pulsing outward. READY: a filled, lit disc with its key in dark ink.
		// COOLING: an open ring refilling clockwise with the seconds inside (Glyph is then the seconds).
		const bool bReady = Value >= .999f;
		if (bActive)
		{
			const float Pulse = FMath::Fmod(Time * 2.f, 1.f);
			P.Plate(At.X - R, At.Y - R, 2 * R, 2 * R, R, FTDUIPalette::Ink().CopyWithNewOpacity(O));
			P.Arc(At, R + 2.f + 5.f * Pulse, 1.f, FTDUIPalette::Ink().CopyWithNewOpacity((1.f - Pulse) * O), 2.f);
			// Four ticks radiating from the disc: ACTIVE differs from READY by shape, not only by the pulse.
			for (int32 I = 0; I < 4; ++I)
			{
				const float A = PI * .25f + I * PI * .5f;
				const FVector2D D(FMath::Cos(A), FMath::Sin(A));
				P.Line(At + D * (R + 3.f), At + D * (R + 8.f), FTDUIPalette::Ink().CopyWithNewOpacity(O), 2.5f);
			}
		}
		else if (bReady)
		{
			P.Plate(At.X - R - 2.f, At.Y - R - 2.f, 2 * R + 4.f, 2 * R + 4.f, R + 2.f, FTDUIPalette::Ink().CopyWithNewOpacity(.25f * O)); // glow
			P.Plate(At.X - R, At.Y - R, 2 * R, 2 * R, R, FTDUIPalette::Ink().CopyWithNewOpacity(O));
		}
		else
		{
			P.Arc(At, R, 1.f, FTDUIPalette::Muted().CopyWithNewOpacity(.35f * O), 2.f);
			P.Arc(At, R, Value, FTDUIPalette::Ink().CopyWithNewOpacity(.9f * O), 2.f, -PI * .5f);
		}
		if (!Glyph.IsEmpty())
		{
			const FLinearColor GlyphC = (bReady || bActive) ? FTDUIPalette::PlateInk() : FTDUIPalette::Ink();
			P.Words(At.X, At.Y - GlyphSize * .68f, Glyph, GlyphSize, GlyphC.CopyWithNewOpacity(O), (bReady || bActive) ? "Bold" : "Mono", .5f, 0);
		}
	}

	/** One card. All sizes are the 1080p design values times K (cards) or F (text). */
	static void SquadCard(const FPaint& P, const UTDFlightHUDWidget::FSquadCard& Card, float K, float F, float Time, const FString& PowerGlyph, const FString& AbilityGlyph)
	{
		const float X = Card.Pos.X, Y = Card.Pos.Y, W = Card.Size.X, H = Card.Size.Y;
		const bool bBig = Card.Grow > .5f;
		const FLinearColor Ship = PanelShip(Card.Ship);
		const FLinearColor Ink = FTDUIPalette::Ink(), Muted = FTDUIPalette::Muted();
		// Cloaked: the card's contents drop to 60% (the concept), with the CLOAKED flag above it.
		const float O = (Card.bAlive ? 1.f : .8f) * (Card.bCloaked ? .6f : 1.f);
		auto Sz = [F](float Px) { return FMath::RoundToInt(Px * F); };

		// Plate (dark red when down), the cut top-right corner, the border and the ship-colour top bar.
		const float Cut = 14.f * K;
		FLinearColor Plate = FTDUIPalette::Panel();
		if (!Card.bAlive)
		{
			Plate = FMath::Lerp(Plate, FLinearColor(.25f, .02f, .02f, Plate.A), .6f);
		}
		P.Box(X, Y, W, H, Plate);
		const float Pulse1Hz = .65f + .35f * FMath::Sin(Time * 2.f * PI);
		const FLinearColor Border = !Card.bAlive ? T::Danger().CopyWithNewOpacity(.7f)
			: Card.bCritical ? FTDUIPalette::Hostile().CopyWithNewOpacity(Pulse1Hz)
			: Card.bTrouble ? T::Warning().CopyWithNewOpacity(Pulse1Hz)
			: bBig ? Ship.CopyWithNewOpacity(.95f) : Ink.CopyWithNewOpacity(.22f);
		P.Poly(FVector2D(X, Y), { { 0, 0 }, { W - Cut, 0 }, { W, Cut }, { W, H }, { 0, H }, { 0, 0 } }, Border, Card.bCritical ? 2.5f : bBig || Card.bTrouble ? 1.5f : 1.f);
		if (bBig)
		{
			P.Box(X, Y - 2.f, W - Cut, 7.f, Ship.CopyWithNewOpacity(.22f)); // glow
		}
		P.Box(X, Y, W - Cut, bBig ? 3.f : 2.f, Ship.CopyWithNewOpacity(bBig ? 1.f : .7f));

		// Keycap, ship glyph, names.
		const float Pad = (bBig ? 12.f : 10.f) * K;
		const float Key = (bBig ? 24.f : 20.f) * K;
		float PenX = X + Pad;
		const float RowY = Y + Pad;
		if (!Card.Key.IsEmpty())
		{
			P.Outline(PenX, RowY, Key, Key, 4.f * K, 1.2f, Ink.CopyWithNewOpacity(.7f * O));
			P.Words(PenX + Key * .5f, RowY + Key * .5f - Sz(bBig ? 15 : 13) * .68f, Card.Key, Sz(bBig ? 15 : 13), Ink.CopyWithNewOpacity(O), "Bold", .5f, 0);
			PenX += Key + 8.f * K;
		}
		const float Icon = (bBig ? 22.f : 18.f) * K;
		const FVector2D IconC(PenX + Icon * .5f, RowY + Key * .5f);
		P.ShipIcon(Card.Ship, IconC, Icon, (Card.bAlive ? Ship : Muted).CopyWithNewOpacity(O));
		if (!Card.bAlive)
		{
			P.Line(IconC + FVector2D(-Icon, -Icon) * .5f, IconC + FVector2D(Icon, Icon) * .5f, T::Danger(), 2.f);
			P.Line(IconC + FVector2D(-Icon, Icon) * .5f, IconC + FVector2D(Icon, -Icon) * .5f, T::Danger(), 2.f);
		}
		PenX += Icon + 10.f * K;
		static const FString Pilots[3] = { PilotName(ETDShipId::Rift), PilotName(ETDShipId::Lancer), PilotName(ETDShipId::Bastion) };
		static const FString Powers[3] = { PowerShortName(ETDShipId::Rift), PowerShortName(ETDShipId::Lancer), PowerShortName(ETDShipId::Bastion) };
		const int32 ShipIndex = FMath::Clamp(static_cast<int32>(Card.Ship), 0, 2);
		float NameEndX = PenX;
		if (bBig)
		{
			P.Words(PenX, Y + 8.f * K, Pilots[ShipIndex], Sz(19), Ink.CopyWithNewOpacity(O), "Bold", 0.f, 80);
			// The ship name alone (ship + role ran under the Q ring at 720p, T7's frame).
			static const FString BigShips[3] = { ShipName(ETDShipId::Rift), ShipName(ETDShipId::Lancer), ShipName(ETDShipId::Bastion) };
			P.Words(PenX, Y + 8.f * K + Sz(19) * 1.3f, BigShips[ShipIndex], Sz(12), Ship.CopyWithNewOpacity(.9f * O), "Bold", 0.f, 80);
			NameEndX = PenX + Sz(19) * 3.6f;
		}
		else
		{
			// Pilot and, under it, the ship (the player switches ships: Nick's Lead, #160).
			static const FString Ships[3] = { ShipName(ETDShipId::Rift), ShipName(ETDShipId::Lancer), ShipName(ETDShipId::Bastion) };
			P.Words(PenX, RowY - 3.f * K, Pilots[ShipIndex], Sz(14), Ink.CopyWithNewOpacity(O), "Bold", 0.f, 80);
			P.Words(PenX, RowY - 3.f * K + Sz(14) * 1.22f, Ships[ShipIndex], Sz(10), Ship.CopyWithNewOpacity(.9f * O), "Bold", 0.f, 80);
			NameEndX = PenX + Sz(14) * 3.6f;
		}
		// Trouble badge after the name: a "!" triangle (the border pulses amber too).
		if (Card.bTrouble)
		{
			const FVector2D B(NameEndX + 12.f * K, RowY + Key * .5f);
			P.Poly(B, { { 0, -8 }, { 8, 6 }, { -8, 6 }, { 0, -8 } }, T::Warning(), 2.f, K);
			P.Words(B.X, B.Y - Sz(11) * .55f, TEXT("!"), Sz(11), T::Warning(), "Bold", .5f, 0);
		}

		// Signature power and ability, right side.
		if (bBig)
		{
			const FVector2D PowerAt(X + W - 34.f * K, Y + 28.f * K);
			PowerRing(P, PowerAt, 16.f * K, Card.Power01, Card.bPowerActive, Ship, PowerGlyph, Sz(13), O);
			P.Words(PowerAt.X, PowerAt.Y + 18.f * K, Powers[ShipIndex], Sz(10), Ship.CopyWithNewOpacity(O), "Bold", .5f, 60);
			AbilityRing(P, FVector2D(X + W - 34.f * K, Y + H - 19.f * K), 12.f * K, Card.Ability == ETDAbilityId::None, Card.Ability01, Card.bAbilityActive,
				Card.AbilitySecondsText.IsEmpty() || Card.bAbilityActive ? AbilityGlyph : Card.AbilitySecondsText, Sz(11), O, Time);
		}
		else
		{
			const FVector2D PowerAt(X + W - 50.f * K, RowY + Key * .5f);
			if (Card.Power01 >= .999f)
			{
				P.Plate(PowerAt.X - 10.f * K, PowerAt.Y - 10.f * K, 20.f * K, 20.f * K, 10.f * K, Ship.CopyWithNewOpacity(.25f * O)); // glow
				P.Plate(PowerAt.X - 8.f * K, PowerAt.Y - 8.f * K, 16.f * K, 16.f * K, 8.f * K, Ship.CopyWithNewOpacity(O));
			}
			else
			{
				P.Arc(PowerAt, 7.f * K, 1.f, Muted.CopyWithNewOpacity(.3f * O), 2.f);
				P.Arc(PowerAt, 7.f * K, Card.Power01, Ship.CopyWithNewOpacity(O), 2.f, -PI * .5f);
			}
			// The ability on a wingman card: the same four shapes at 9 px radius (padlock, filled, ring + seconds, pulse).
			AbilityRing(P, FVector2D(X + W - 20.f * K, RowY + Key * .5f), 9.f * K, Card.Ability == ETDAbilityId::None, Card.Ability01, Card.bAbilityActive,
				Card.bAbilityActive ? AbilityGlyph : Card.AbilitySecondsText.IsEmpty() ? AbilityGlyph : Card.AbilitySecondsText, Sz(10), O, Time);
		}

		// Status flags above the card: icon + word, left to right by priority (3 on the flown card, 2 on a wingman).
		{
			static const FString UnderFire(TEXT("UNDER FIRE")), Cloaked(TEXT("CLOAKED")), Dome(TEXT("DOME UP")), Afterburn(TEXT("AFTERBURN"));
			const float TagH = 20.f * K, TagY = Y - TagH - 6.f * K;
			float TagX = X;
			int32 Left = bBig ? 3 : 2;
			// Icon: 0 triangle, 1 eye with a slash, 2 half circle, 3 turned square, 4 double chevron.
			auto Flag = [&](const FString& Word, FLinearColor C, int32 IconKind, bool bFilled)
			{
				if (Left-- <= 0)
				{
					return;
				}
				const int32 Px = Sz(11);
				float TextW = P.Measure(Word, P.Font("Bold", Px, 60));
				if (TextW <= 0.f)
				{
					TextW = Word.Len() * Px * .72f;
				}
				const float TW = TextW + 30.f * K + 8.f;
				P.Box(TagX, TagY, TW, TagH, bFilled ? FMath::Lerp(FTDUIPalette::Panel(), C, .28f).CopyWithNewOpacity(.92f) : FTDUIPalette::Panel());
				P.Poly(FVector2D(TagX, TagY), { { 0, 0 }, { TW, 0 }, { TW, TagH }, { 0, TagH }, { 0, 0 } }, C.CopyWithNewOpacity(.9f), 1.2f);
				const FVector2D I(TagX + 13.f * K, TagY + TagH * .5f);
				switch (IconKind)
				{
				case 0: P.Poly(I, { { 0, -6 }, { 6, 5 }, { -6, 5 }, { 0, -6 } }, C, 1.6f, K); break;
				case 1: P.ArcSpan(I, 6.f * K, PI * 1.15f, PI * .7f, C, 1.5f); P.ArcSpan(I, 6.f * K, PI * .15f, PI * .7f, C, 1.5f);
					P.Line(I + FVector2D(-6, 6) * K, I + FVector2D(6, -6) * K, C, 1.5f); break;
				case 2: P.ArcSpan(I + FVector2D(0, 4.f * K), 7.f * K, PI, PI, C, 1.8f); P.Line(I + FVector2D(-7, 4) * K, I + FVector2D(7, 4) * K, C, 1.5f); break;
				case 3: P.Poly(I, { { 0, -6 }, { 6, 0 }, { 0, 6 }, { -6, 0 }, { 0, -6 } }, C, 1.6f, K); break;
				default: P.Poly(I + FVector2D(-3.f * K, 0), { { -3, -5 }, { 2, 0 }, { -3, 5 } }, C, 1.6f, K);
					P.Poly(I + FVector2D(3.f * K, 0), { { -3, -5 }, { 2, 0 }, { -3, 5 } }, C, 1.6f, K); break;
				}
				P.Words(TagX + 26.f * K, TagY + TagH * .5f - Px * .68f, Word, Px, C, "Bold", 0.f, 60);
				TagX += TW + 6.f * K;
			};
			if (Card.bAlive && Card.UnderFire01 > 0.f) { Flag(UnderFire, FTDUIPalette::Hostile(), 0, true); }
			if (Card.bCloaked) { Flag(Cloaked, FTDUIPalette::Rift(), 1, false); }
			if (Card.bDomeUp) { Flag(Dome, FTDUIPalette::Bastion(), 2, false); }
			if (Card.bOvercharge) { Flag(Card.OverchargeText, FLinearColor::FromSRGBColor(FColor(0xFF, 0xE7, 0xA0)), 3, false); }
			if (Card.bAfterburn) { Flag(Afterburn, Ship, 4, false); }
		}

		// Bars with numbers, or the DOWN line.
		if (!Card.bAlive)
		{
			P.Words(X + Pad, Y + H - (bBig ? 36.f : 28.f) * K, Card.DownText, Sz(bBig ? 15 : 13), T::Danger(), "Bold", 0.f, 60);
			return;
		}
		static const FString Shd(TEXT("SHD")), Hul(TEXT("HUL"));
		const float TagW = bBig ? Sz(11) * 3.2f : 0.f;
		const float NumW = bBig ? Sz(14) * 3.f : 0.f;
		const float BarX = X + Pad + TagW;
		const float BarRight = (bBig ? X + W - 60.f * K : X + W - Pad) - NumW;
		const float BarW = FMath::Max(20.f, BarRight - BarX);
		const float ShH = (bBig ? 6.f : 4.f) * K, HuH = (bBig ? 12.f : 10.f) * K;
		const float HuY = Y + H - Pad - HuH, ShY = HuY - (bBig ? 12.f : 6.f) * K - ShH;
		const FLinearColor HullC = Card.bCritical ? FTDUIPalette::Hostile().CopyWithNewOpacity(Pulse1Hz) : T::Hull(Card.Hull);
		if (bBig)
		{
			P.Words(X + Pad, ShY + ShH * .5f - Sz(11) * .68f, Shd, Sz(11), Muted, "Medium", 0.f, 80);
			P.Words(X + Pad, HuY + HuH * .5f - Sz(11) * .68f, Hul, Sz(11), Muted, "Medium", 0.f, 80);
		}
		SegmentBar(P, BarX, ShY, BarW, ShH, Card.Shield, Card.Shield, T::Shield());
		SegmentBar(P, BarX, HuY, BarW, HuH, Card.Hull, Card.GhostHull, HullC);
		// Numbers on the flown card only, beside their SHD / HUL tags; wingman cards keep the two bars (thin = shield).
		if (bBig)
		{
			const float NumX = BarRight + NumW;
			P.Words(NumX, ShY + ShH * .5f - Sz(14) * .68f, Card.ShieldText, Sz(14), T::Shield(), "Mono", 1.f, 0);
			P.Words(NumX, HuY + HuH * .5f - Sz(14) * .68f, Card.HullText, Sz(14), HullC, "Mono", 1.f, 0);
		}
	}
}

// ------------------------------------------------------------------------------------------ paint

int32 UTDFlightHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace TDFlightHUD;
	FTDHUDCostScope CostScope(PaintCost);
	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!View.bHasData)
	{
		return Base;
	}
	FPaint P(AllottedGeometry, OutDrawElements, Base + 1);
	P.Alpha = View.HUDOpacity; // CUT (0), slate (0.4), pause (0.3), the 0.25 s fade back in: every element follows
	if (P.Alpha <= .001f)
	{
		LastBoxes.Reset();
		LastCoverage01 = 0.f;
		return Base; // a CUT: nothing at all on screen
	}
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
		const FLinearColor Edge = T::Danger().CopyWithNewOpacity(.10f * (.6f + .4f * FMath::Sin(Now * 2.f * PI)) * P.Alpha);
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
	// The docking plate (drawn later) keeps its words clear: no hostile tick or bracket inside it (live: a turret's tick
	// sat on "LOCKED" in the bay).
	const FBox2D DockPlate(FVector2D(CenterX - 200.f, C.Y - 180.f), FVector2D(CenterX + 200.f, C.Y - 116.f));
	for (const FMark& M : View.Marks)
	{
		if (View.Docking > 0 && M.bOnScreen && DockPlate.IsInside(M.Pos))
		{
			continue;
		}
		// A kind's outline at At, rotated (0 = pointing down), brighter and heavier on a threat to the flown ship.
		auto Shape = [&P, &Enemy](const FMark& Mk, FVector2D At, float Rot, float Alpha)
		{
			static TArray<FVector2D> Pts; // its own buffer: P.Line below reuses the shared one
			Pts.Reset();
			float Width = 2.f;
			TDFlightHUDShapes::Points(Mk.Kind, Pts, Width);
			const FLinearColor C = (Mk.bThreat ? FMath::Lerp(Enemy, FLinearColor::White, .35f) : Enemy).CopyWithNewOpacity(Alpha);
			const float W = Width + (Mk.bThreat ? 1.f : 0.f);
			// 1.4x the first size: live at 1080p the 12 px outlines were hard to tell apart.
			const float Cs = FMath::Cos(Rot) * 1.4f, Sn = FMath::Sin(Rot) * 1.4f;
			for (int32 I = 0; I + 1 < Pts.Num(); ++I)
			{
				const FVector2D A(Pts[I].X * Cs - Pts[I].Y * Sn, Pts[I].X * Sn + Pts[I].Y * Cs);
				const FVector2D B(Pts[I + 1].X * Cs - Pts[I + 1].Y * Sn, Pts[I + 1].X * Sn + Pts[I + 1].Y * Cs);
				P.Line(At + A, At + B, C, W);
			}
			if (Mk.Kind == UTDFlightHUDWidget::KindCarrier)
			{
				P.Brackets(At, 21.f, C, 6.f);
			}
		};
		if (!M.bOnScreen)
		{
			if (M.bMissile || M.Kind == KindOther)
			{
				P.Chevron(M.Pos, M.Angle, 18.f, Enemy.CopyWithNewOpacity(M.bMissile ? Pulse25 : .9f), true);
			}
			else
			{
				Shape(M, M.Pos, M.Angle - PI * .5f, .9f); // its point outward, toward where it is
			}
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
		// Just above the ship's outline (live: a fixed 18 px sat on the hull of a close ship).
		const FVector2D Tick(M.Pos.X, M.Pos.Y - FMath::Max(18.f, M.ScreenRadius + 16.f));
		Rec(TEXT("tick"), Tick.X - 7.f, Tick.Y - 6.f, 14.f, 12.f);
		Shape(M, Tick, 0.f, .95f);
		Rec(*FString::Printf(TEXT("kind_%s"), KindName(M.Kind)), Tick.X - 12.f, Tick.Y - 12.f, 24.f, 24.f);
		// Heading: toward = a filled dot in the outline, otherwise a short tick on the ship along its nose (crossing or away).
		if (M.Heading == 1)
		{
			P.Box(Tick.X - 2.f, Tick.Y - 2.f, 4.f, 4.f, Enemy);
		}
		else
		{
			const FVector2D Nose(FMath::Cos(M.HeadingAngle), FMath::Sin(M.HeadingAngle));
			P.Line(M.Pos + Nose * 6.f, M.Pos + Nose * (M.Heading == 2 ? 11.f : 15.f), Enemy.CopyWithNewOpacity(.75f), 2.f);
		}
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
	// ---- Aftermath navigation pulse: a faint corridor that flickers in and clears in 1.5 s, like sensor noise
	if (View.NavPulse > 0.f)
	{
		const float T01 = View.NavPulse;
		// Up fast (0.15 of the time), hold, then gone; with a high-frequency flicker so it reads as interference.
		const float Env = FMath::Min(FMath::Clamp(T01 / .1f, 0.f, 1.f), FMath::Clamp((1.f - T01) / .35f, 0.f, 1.f));
		const float Flicker = .55f + .45f * FMath::Abs(FMath::Sin(Now * 53.f) * FMath::Sin(Now * 31.f));
		const FLinearColor Edge = T::Shield().CopyWithNewOpacity(.32f * Env * Flicker);
		const FVector2D A = View.NavFrom, B = View.NavTo;
		const FVector2D Along = (B - A).GetSafeNormal();
		const FVector2D Side(-Along.Y, Along.X);
		const float Jit = FMath::Sin(Now * 71.f) * 1.5f;
		for (int32 S = 0; S < 24; S += 2) // two dashed rails, narrowing toward the far end
		{
			const float P0 = S / 24.f, P1 = (S + 1) / 24.f;
			for (const float Sgn : { -1.f, 1.f })
			{
				const float W0 = FMath::Lerp(26.f, 8.f, P0) * Sgn + Jit, W1 = FMath::Lerp(26.f, 8.f, P1) * Sgn + Jit;
				P.Line(FMath::Lerp(A, B, P0) + Side * W0, FMath::Lerp(A, B, P1) + Side * W1, Edge, 1.5f);
			}
		}
		// A bright blip running down the corridor once.
		const FVector2D Blip = FMath::Lerp(A, B, FMath::Clamp(T01 * 1.4f, 0.f, 1.f));
		P.Box(Blip.X - 2.f, Blip.Y - 2.f, 4.f, 4.f, T::Neutral().CopyWithNewOpacity(.6f * Env));
		Rec(TEXT("navpulse"), FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), FMath::Abs(B.X - A.X), FMath::Abs(B.Y - A.Y));
	}

	// ---- the transponder: a thin sensor bracket and "TRANSPONDER  UNKNOWN" (or, for a moment, "VERIFIED" in the
	// shield colour with a stutter), the range under it. On the frame edge when off screen.
	if (View.bTransponder)
	{
		// B2-04 (Nick's playtest, 720p): the label block never leaves the safe frame. It sits right of the bracket, or
		// left of it (right-aligned) when there is no room, and the whole block is held inside the frame vertically.
		const float LabelW = 120.f, BlockTop = 16.f, BlockBottom = 30.f;
		FVector2D TC = View.TransponderPos;
		TC.Y = FMath::Clamp(TC.Y, SafeY + BlockTop, 1080.f - SafeY - BlockBottom);
		const bool bLeft = TC.X + 24.f + LabelW > 1920.f - SafeX;
		const float LX = bLeft ? TC.X - 24.f : TC.X + 24.f, LA = bLeft ? 1.f : 0.f;
		const float Stutter = View.bTransponderVerified ? .75f + .25f * FMath::Abs(FMath::Sin(Now * 47.f)) : 1.f;
		const FLinearColor Col = (View.bTransponderVerified ? T::Shield() : T::Dim()).CopyWithNewOpacity(.85f * Stutter);
		P.Brackets(TC, 16.f, Col, 6.f);
		const FString Word = View.bTransponderVerified ? TEXT("VERIFIED") : TEXT("UNKNOWN");
		P.Words(LX, TC.Y - 16.f, TEXT("TRANSPONDER"), 11, T::Dim().CopyWithNewOpacity(.8f), "Bold", LA, 120);
		P.Words(LX, TC.Y - 2.f, Word, 13, Col, "Bold", LA, 100);
		P.Words(LX, TC.Y + 14.f, View.TransponderMetres >= 1000.f ? FString::Printf(TEXT("%.1f km"), View.TransponderMetres / 1000.f)
			: FString::Printf(TEXT("%.0f m"), View.TransponderMetres), 11, T::Dim().CopyWithNewOpacity(.7f), "Bold", LA, 40);
		const float BoxX = bLeft ? TC.X - 24.f - LabelW : TC.X - 16.f;
		Rec(View.bTransponderVerified ? TEXT("transponder_verified") : TEXT("transponder"), BoxX, TC.Y - 16.f, LabelW + 40.f, 46.f);
	}

	// ---- objective waypoints: a white diamond + range on screen (health bar on a generator), an edge diamond off screen
	auto Range = [](float Metres) { return Metres >= 1000.f ? FString::Printf(TEXT("%.1f km"), Metres / 1000.f) : FString::Printf(TEXT("%.0f m"), Metres); };
	// ---- the Dominion approach: pending = a dashed amber diamond (the route is prepared, the passage still closed), open =
	// a solid one for its first 8 s; "APPROACH  PENDING" / "APPROACH  OPEN" with the range. A marker only: no corridor.
	if (View.Approach > 0)
	{
		const FWaypoint& Wp = View.ApproachWp;
		const bool bOpen = View.Approach == 2;
		const FLinearColor AppC = bOpen ? T::Neutral() : T::Warning().CopyWithNewOpacity(.8f);
		const FVector2D At = Wp.bOnScreen ? Wp.DrawPos : Wp.Pos;
		const float R = Wp.bOnScreen ? 13.f : 10.f;
		const FVector2D Pts[4] = { { 0, -R }, { R, 0 }, { 0, R }, { -R, 0 } };
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector2D A = At + Pts[i], B = At + Pts[(i + 1) % 4];
			if (bOpen)
			{
				P.Line(A, B, AppC, 2.5f);
			}
			else
			{
				// Two dashes per side: a gap at each corner and one in the middle.
				P.Line(FMath::Lerp(A, B, .1f), FMath::Lerp(A, B, .4f), AppC, 2.f);
				P.Line(FMath::Lerp(A, B, .6f), FMath::Lerp(A, B, .9f), AppC, 2.f);
			}
		}
		if (Wp.bOnScreen && Wp.bLifted)
		{
			P.Line(At + FVector2D(0, R), Wp.Pos, AppC.CopyWithNewOpacity(.4f), 1.f);
		}
		const FString Word = bOpen ? TEXT("APPROACH  OPEN") : TEXT("APPROACH  PENDING");
		if (Wp.bOnScreen)
		{
			P.BackedWords(At.X, At.Y + R + 6.f, Word, 13, AppC, .5f, 60);
			P.BackedWords(At.X, At.Y + R + 26.f, Range(Wp.Distance), 13, AppC, .5f, 20);
		}
		else
		{
			const FVector2D F(FMath::Cos(Wp.Angle), FMath::Sin(Wp.Angle));
			P.Line(At + F * (R + 2.f), At + F * (R + 8.f), AppC, 2.5f);
			const FVector2D Label = At - F * 70.f;
			P.BackedWords(Label.X, Label.Y - 18.f, Word, 13, AppC, .5f, 60);
			P.BackedWords(Label.X, Label.Y + 2.f, Range(Wp.Distance), 13, AppC, .5f, 20);
		}
		Rec(TEXT("approach"), At.X - R, At.Y - R, 2.f * R, 2.f * R);
		Area += 160.f * 60.f;
	}

	// ---- space pickups within 800 m: a small diamond in the pickup's colour with a dot (on screen: the range under it),
	// an edge diamond with a pointer off screen. Smaller than an objective waypoint: a reward, not an order.
	if (!View.bCombatHeld)
	{
		for (const FView::FPickupMark& M : View.Pickups)
		{
			// No marker on the reticle cluster or over the ship (live: three pickups dead ahead piled their labels onto the
			// reticle); no range text either: the diamond in its colour is enough, the pickup itself is visible.
			if (M.bOnScreen && (FVector2D::Distance(M.Pos, C) < 72.f || ShipZone().IsInside(M.Pos)))
			{
				continue;
			}
			const FLinearColor PkC = M.Color;
			const float R = M.bOnScreen ? 8.f : 7.f;
			P.Poly(M.Pos, { { 0, -R }, { R, 0 }, { 0, R }, { -R, 0 }, { 0, -R } }, PkC, 2.f);
			P.Box(M.Pos.X - 2.f, M.Pos.Y - 2.f, 4.f, 4.f, PkC);
			if (!M.bOnScreen)
			{
				const FVector2D F(FMath::Cos(M.Angle), FMath::Sin(M.Angle));
				P.Line(M.Pos + F * (R + 2.f), M.Pos + F * (R + 7.f), PkC, 2.f);
			}
			Rec(M.bOnScreen ? TEXT("pickup") : TEXT("pickup_edge"), M.Pos.X - R, M.Pos.Y - R, 2.f * R, 2.f * R);
			Area += 30.f * 30.f;
		}
	}

	// ---- overcharge: a thin gold-white ring outside the shield arc running down clockwise, the seconds at its top
	if (View.Overcharge01 > 0.f)
	{
		const FLinearColor Gold = ATDPickup::ColorOf(ETDPickupType::Overcharge);
		const float RingR = ShieldRadius + 11.f;
		P.ArcSpan(C, RingR, -PI * .5f, 2.f * PI, Gold.CopyWithNewOpacity(.18f), 2.f);
		P.ArcSpan(C, RingR, -PI * .5f, 2.f * PI * View.Overcharge01, Gold, 2.5f);
		P.BackedWords(C.X, C.Y - RingR - 22.f, FString::FromInt(FMath::CeilToInt(View.OverchargeSeconds)), 13, Gold, .5f, 20);
		Rec(TEXT("overcharge"), C.X - RingR, C.Y - RingR, 2.f * RingR, 2.f * RingR);
	}

	// ---- T6's field, kept quiet: a faint, small tick at the reticle per rock hit (no sound; ships get the bold one), and
	// "DEBRIS n" above the squad strip for a moment after each break, brighter for its first 0.3 s.
	if (View.FieldHit01 > 0.f)
	{
		const FLinearColor Tick = T::Neutral().CopyWithNewOpacity(.45f * View.FieldHit01);
		for (const FVector2D D : { FVector2D(.707f, .707f), FVector2D(-.707f, .707f), FVector2D(.707f, -.707f), FVector2D(-.707f, -.707f) })
		{
			P.Line(C + D * 6.f, C + D * 10.f, Tick, 1.5f);
		}
		Rec(TEXT("field_hit"), C.X - 10.f, C.Y - 10.f, 20.f, 20.f);
	}
	if (View.DebrisCount > 0 && View.DebrisAge < DebrisShowSeconds && View.Chips.Num() > 0)
	{
		const float Fresh = FMath::Clamp(1.f - View.DebrisAge / .3f, 0.f, 1.f);
		const float Fade = FMath::Clamp((DebrisShowSeconds - View.DebrisAge) / .5f, 0.f, 1.f);
		const FVector2D Chip = View.Chips[0].Pos;
		const FLinearColor DebrisInk = FMath::Lerp(T::Dim(), T::Neutral(), Fresh).CopyWithNewOpacity(.85f * Fade);
		P.BackedWords(Chip.X + 6.f, Chip.Y - 74.f, FString::Printf(TEXT("DEBRIS  %d"), View.DebrisCount), 13, DebrisInk, 0.f, 80);
		Rec(TEXT("debris"), Chip.X, Chip.Y - 78.f, 140.f, 22.f);
	}

	// ---- the collect toast, just above the squad strip: the pickup's word in its colour, rising a little, then fading
	if (!View.PickupToast.IsEmpty() && View.bSquadPanel)
	{
		// #160 step 3: a plate right of the reticle cluster: a bar and a turned square in the pickup's colour, its word
		// (and the seconds for Overcharge); after 70% of its time it slides toward the squad panel and fades, where a timed
		// one lands as a flag on the flown card.
		const float A = View.PickupToast01;
		const float Out = FMath::Clamp((A - .7f) / .3f, 0.f, 1.f);
		const float Fade = 1.f - Out;
		const float K = View.PanelK, F = View.PanelFont;
		const bool bOver = View.PickupToast == TEXT("OVERCHARGE");
		const float W = 214.f * K, H = (bOver ? 44.f : 32.f) * K;
		const float X = C.X + 102.f - 28.f * Out, Y = C.Y - H * .5f + 28.f * Out;
		const FLinearColor Col = View.PickupToastColor;
		P.Box(X, Y, W, H, FTDUIPalette::Panel().CopyWithNewOpacity(.9f * Fade));
		P.Box(X, Y, 3.f, H, Col.CopyWithNewOpacity(Fade));
		P.Poly(FVector2D(X + 20.f * K, Y + H * .5f), { { 0, -8 }, { 8, 0 }, { 0, 8 }, { -8, 0 }, { 0, -8 } }, Col.CopyWithNewOpacity(Fade), 2.f, K);
		const int32 Px = FMath::RoundToInt(16 * F);
		P.Words(X + 38.f * K, Y + (bOver ? 5.f * K : H * .5f - Px * .68f), View.PickupToast, Px, Col.CopyWithNewOpacity(Fade), "Bold", 0.f, 80);
		if (bOver)
		{
			static FString Detail;
			static int32 DetailSeconds = -1;
			const int32 Seconds = FMath::CeilToInt(View.OverchargeSeconds);
			if (Seconds != DetailSeconds)
			{
				DetailSeconds = Seconds;
				Detail = FString::Printf(TEXT("Damage up  ·  %d s"), Seconds);
			}
			P.Words(X + 38.f * K, Y + 5.f * K + Px * 1.25f, Detail, FMath::RoundToInt(12 * F), FTDUIPalette::Ink().CopyWithNewOpacity(.9f * Fade), "Medium", 0.f, 20);
		}
		Rec(TEXT("pickup_toast"), X, Y, W, H);
		Area += W * H;
	}
	else if (!View.PickupToast.IsEmpty() && View.Chips.Num() > 0)
	{
		const float A = View.PickupToast01;
		const float Fade = FMath::Clamp((1.f - A) / .3f, 0.f, 1.f);
		const FVector2D Chip = View.Chips[0].Pos;
		const float Y = Chip.Y - 40.f - 10.f * A;
		const FString Text = View.PickupToast == TEXT("OVERCHARGE")
			? FString::Printf(TEXT("OVERCHARGE  %d s"), FMath::CeilToInt(View.OverchargeSeconds)) : View.PickupToast;
		P.BackedWords(Chip.X + 6.f, Y, Text, 20, View.PickupToastColor.CopyWithNewOpacity(Fade), 0.f, 60);
		Rec(TEXT("pickup_toast"), Chip.X, Y - 4.f, 260.f, 32.f);
	}

	static const TArray<FWaypoint> NoWaypoints;
	const TArray<FWaypoint>& ShownWaypoints = View.bCombatHeld ? NoWaypoints : View.Waypoints; // a focus hold: none
	// A "follow the marker" objective (one waypoint, no wave count, no generators: the withdrawal, a planet, the bay):
	// the marker is the thing to look at (T1's play: the small edge icon was easy to miss). Off screen it is twice the
	// size with a slow pulse and an arrow, and it is named from the objective's first word ("WITHDRAW 2.7 KM"); on
	// screen it gets a bracket.
	const bool bFollow = ShownWaypoints.Num() >= 1 && ShownWaypoints.Num() <= 2 && View.Counter.IsEmpty() && View.GeneratorsTotal == 0 && !View.Objective.IsEmpty();
	static FString FollowObjective, FollowWord;
	if (bFollow && FollowObjective != View.Objective)
	{
		FollowObjective = View.Objective;
		int32 Space = INDEX_NONE;
		FollowWord = View.Objective.FindChar(TEXT(' '), Space) ? View.Objective.Left(Space) : View.Objective;
	}
	const float FollowPulse = .7f + .3f * FMath::Sin(Now * 2.f * PI * .8f);
	for (const FWaypoint& Wp : ShownWaypoints)
	{
		const FLinearColor Way = T::Neutral();
		if (bFollow && !Wp.bOnScreen)
		{
			const FVector2D F(FMath::Cos(Wp.Angle), FMath::Sin(Wp.Angle));
			const FLinearColor Lit = Way.CopyWithNewOpacity(FollowPulse);
			P.Poly(Wp.Pos, { { 0, -9 }, { 9, 0 }, { 0, 9 }, { -9, 0 }, { 0, -9 } }, Lit, 3.f, 2.f);
			P.Box(Wp.Pos.X - 2.5f, Wp.Pos.Y - 2.5f, 5.f, 5.f, Lit);
			P.Chevron(Wp.Pos + F * 30.f, Wp.Angle, 22.f, Lit, true);
			Rec(TEXT("waypoint_edge"), Wp.Pos.X - 18.f, Wp.Pos.Y - 18.f, 36.f, 36.f);
			const FVector2D Label = Wp.Pos - F * 110.f;
			P.BackedWords(Label.X, Label.Y - 10.f, FollowWord + TEXT("  ") + Range(Wp.Distance).ToUpper(), 17, Way, .5f, 40);
			Area += 220.f * 44.f;
			continue;
		}
		if (bFollow && Wp.bOnScreen && !Wp.bLifted)
		{
			P.Brackets(Wp.Pos, 26.f, Way.CopyWithNewOpacity(FollowPulse), 9.f);
		}
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
	// ---- ENG-38 descent: an altimeter tape right of the reticle (100 m ticks, a number every 500 m, the reading in a
	// box), a HULL TEMP bar under it (amber, red above 70%), "ATMOSPHERE" over it; squad-neutral tokens.
	if (View.bDescent)
	{
		// Readable over a bright sky (the Lead, live at 720p): its own dark plate, full-white text, and never below 80%
		// while the HUD is up (the director's exit shot fades the rest; a CUT still hides it).
		FPaint D = P;
		if (View.HUDOpacity > .05f && !View.bInCut)
		{
			D.Alpha = FMath::Max(D.Alpha, .8f);
		}
		const float TX = C.X + 190.f, TY = C.Y - 100.f, TH = 200.f;
		// Wide enough for the reading box on the left (the Lead: it hung half off the plate's edge).
		D.Plate(TX - 116.f, TY - 34.f, 216.f, TH + 84.f, 12.f, T::Plate().CopyWithNewOpacity(.78f));
		D.Layer = P.Layer + 2; // everything else above its plate (same-layer boxes batch under rounded brushes)
		const float Alt = FMath::Max(0.f, View.Altitude);
		D.Words(TX - 4.f, TY - 26.f, TEXT("ATMOSPHERE"), 13, T::Shield(), "Bold", 0.f, 160);
		if (View.Altitude >= 0.f)
		{
			// 200 px = 1000 m: 0.2 px per metre, centred on the current altitude.
			D.Line({ TX, TY }, { TX, TY + TH }, T::Neutral().CopyWithNewOpacity(.7f), 1.5f);
			const int32 First = FMath::FloorToInt((Alt - 500.f) / 100.f) * 100;
			for (int32 M = First; M <= Alt + 500.f; M += 100)
			{
				if (M < 0) { continue; }
				const float Y = TY + TH * .5f - (M - Alt) * .2f;
				if (Y < TY || Y > TY + TH) { continue; }
				const bool bMajor = M % 500 == 0;
				D.Line({ TX, Y }, { TX + (bMajor ? 12.f : 6.f), Y }, T::Neutral().CopyWithNewOpacity(bMajor ? .8f : .45f), 1.5f);
				if (bMajor)
				{
					D.Words(TX + 16.f, Y - 7.f, FString::Printf(TEXT("%d"), M), 12, T::Neutral(), "Bold", 0.f, 20);
				}
			}
			const float BY = TY + TH * .5f;
			D.Plate(TX - 100.f, BY - 13.f, 92.f, 26.f, 6.f, FLinearColor(.02f, .03f, .05f, .95f));
			D.Words(TX - 14.f, BY - 9.f, FString::Printf(TEXT("%.0f m"), Alt), 15, Alt < 60.f ? T::Bastion() : T::Neutral(), "Bold", 1.f, 20);
			Rec(TEXT("altimeter"), TX - 84.f, TY, 130.f, TH);
		}
		const float HY = TY + TH + 16.f;
		D.Words(TX - 4.f, HY, TEXT("HULL TEMP"), 12, T::Neutral(), "Bold", 0.f, 140);
		D.Box(TX - 4.f, HY + 18.f, 120.f, 6.f, T::Plate().CopyWithNewOpacity(.85f));
		const FLinearColor HeatC = View.Heat01 > .7f ? T::Danger() : T::Bastion();
		D.Box(TX - 4.f, HY + 18.f, 120.f * View.Heat01, 6.f, HeatC);
		Rec(TEXT("hull_temp"), TX - 4.f, HY, 120.f, 24.f);
	}
	// ---- ENG-29 docking: the tractor's hold, above the reticle on a plate (calm: no flashing), the pull as a bar
	if (View.Docking > 0)
	{
		const bool bGlide = View.Docking == 2;
		const FString Line = bGlide ? TEXT("DOCKING  ·  CONTROLS LOCKED") : TEXT("TRACTOR BEAM  ·  CONTROLS LIMITED");
		const float W = 380.f, H = bGlide ? 34.f : 44.f, X = CenterX - W * .5f, Y = C.Y - 170.f;
		P.Plate(X, Y, W, H, 12.f, T::Plate().CopyWithNewOpacity(.8f));
		P.Words(CenterX, Y + 8.f, Line, 15, T::Shield(), "Bold", .5f, 120);
		if (!bGlide)
		{
			P.Box(X + 20.f, Y + 32.f, W - 40.f, 4.f, T::Dim().CopyWithNewOpacity(.4f));
			P.Box(X + 20.f, Y + 32.f, (W - 40.f) * View.TractorPull01, 4.f, T::Shield());
		}
		Rec(bGlide ? TEXT("docking_locked") : TEXT("tractor"), X, Y, W, H);
	}
	// ---- the reactor hold (T4's ATDDominion::IsReactorHoldActive: the ship is slowed in front of the exposed core):
	// one calm amber line in the same place and style as the tractor plate.
	if (View.bReactorHold && View.Docking == 0)
	{
		static const FString Hold(TEXT("REACTOR IN RANGE  ·  HOLD FIRE ON THE CORE"));
		const float W = 520.f, H = 34.f, X = CenterX - W * .5f, Y = C.Y - 170.f;
		P.Plate(X, Y, W, H, 12.f, T::Plate().CopyWithNewOpacity(.8f));
		P.Words(CenterX, Y + 8.f, Hold, 15, T::Warning(), "Bold", .5f, 120);
		Rec(TEXT("reactor_hold"), X, Y, W, H);
	}
	if (View.OrbitCue01 > 0.f)
	{
		const float Fade = FMath::Min(1.f, View.OrbitCue01 * 3.f);
		P.Words(C.X, C.Y - 170.f, TEXT("ORBIT"), 22, T::Shield().CopyWithNewOpacity(.9f * Fade), "Bold", .5f, 300);
		Rec(TEXT("orbit"), C.X - 60.f, C.Y - 170.f, 120.f, 26.f);
	}
	if (View.Altitude >= 0.f && !View.bDescent)
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
	if (View.bShowBoost && View.bSquadPanel)
	{
		// #160 step 3: the boost meter is an arc under the reticle (r 70, the bottom 56 degrees), read where the eye is;
		// it fills left to right, the dodge notch is a tick at its right end, "+BOOST" ticks beside it on a top-up.
		const float ArcHalf = FMath::DegreesToRadians(28.f), R = 70.f;
		const float ArcLeft = PI * .5f + ArcHalf;
		P.ArcSpan(C, R, ArcLeft, -2.f * ArcHalf, T::Dim().CopyWithNewOpacity(.35f), 5.f);
		if (View.Boost > .01f)
		{
			P.ArcSpan(C, R, ArcLeft, -2.f * ArcHalf * View.Boost, Accent.CopyWithNewOpacity(.95f), 5.f);
		}
		const float NotchA = PI * .5f - ArcHalf - .07f;
		const FVector2D N(FMath::Cos(NotchA), FMath::Sin(NotchA));
		P.Line(C + N * (R - 5.f), C + N * (R + 5.f), (View.bDodgeReady ? T::Neutral() : T::Dim()).CopyWithNewOpacity(View.bDodgeReady ? .95f : .4f), 2.5f);
		Rec(TEXT("boost"), C.X - 40.f, C.Y + 56.f, 84.f, 20.f);
		if (View.BreakBoostAge < .9f)
		{
			const float Fade = FMath::Clamp((.9f - View.BreakBoostAge) / .4f, 0.f, 1.f);
			static const FString Plus(TEXT("+BOOST"));
			P.ArcSpan(C, R, ArcLeft, -2.f * ArcHalf * View.Boost, T::Neutral().CopyWithNewOpacity(.5f * Fade), 5.f);
			P.Words(C.X + 46.f, C.Y + 58.f, Plus, FMath::RoundToInt(12 * View.PanelFont), Accent.CopyWithNewOpacity(.9f * Fade), "Bold", 0.f, 60);
			Rec(TEXT("break_boost"), C.X + 46.f, C.Y + 58.f, 60.f, 16.f);
		}
	}
	else if (View.bShowBoost)
	{
		P.Bar(CenterX - 80.f, 792.f, 160.f, 6.f, View.Boost, Accent, .9f);
		Rec(TEXT("boost"), CenterX - 80.f, 791.f, 168.f, 8.f);
		// A break's top-up: "+BOOST" beside the bar and a short brightening of the bar (0.9 s).
		if (View.BreakBoostAge < .9f)
		{
			const float Fade = FMath::Clamp((.9f - View.BreakBoostAge) / .4f, 0.f, 1.f);
			auto Tick = P;
			Tick.Layer = P.Layer + 2;
			Tick.Box(CenterX - 80.f, 792.f, 160.f * View.Boost, 6.f, T::Neutral().CopyWithNewOpacity(.35f * Fade));
			Tick.Words(CenterX + 92.f, 784.f, TEXT("+BOOST"), 12, Accent.CopyWithNewOpacity(.9f * Fade), "Bold", 0.f, 60);
			Rec(TEXT("break_boost"), CenterX + 92.f, 784.f, 60.f, 16.f);
		}
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
		// #161: the closed-planet warning is the one large line (24 px on a taller plate, at full strength).
		const bool bBigLane = bPlanetClosed && View.Lane == ELane::HyperdriveBlocked;
		const int32 LanePx = bBigLane ? 24 : 17;
		const float LaneH = bBigLane ? 50.f : 36.f, LaneY = bBigLane ? 826.f : 838.f;
		const float LaneW = FMath::Clamp(P.Measure(View.LaneText, P.Font("Bold", LanePx, 50)) + 56.f, 240.f, bBigLane ? 1100.f : 760.f);
		Area += LaneW * LaneH;
		P.Plate(CenterX - LaneW * .5f, LaneY, LaneW, LaneH, 8.f, T::Plate().CopyWithNewOpacity(View.Lane == ELane::Hint ? .5f : bBigLane ? .85f : .65f));
		Rec(TEXT("lane"), CenterX - LaneW * .5f, LaneY, LaneW, LaneH);
		if (View.Lane == ELane::HyperdriveBlocked)
		{
			// A padlock: shackle arc + body.
			const FVector2D L(CenterX - LaneW * .5f + 20.f, LaneY + LaneH * .5f + 2.f);
			P.ArcSpan(L + FVector2D(0, -4), 5.f, PI, PI, LaneC, 2.f);
			P.Outline(L.X - 7.f, L.Y - 4.f, 14.f, 10.f, 2.f, 1.5f, LaneC);
		}
		P.Words(CenterX + (View.Lane == ELane::HyperdriveBlocked ? 10.f : 0.f), LaneY + LaneH * .5f - LanePx * .66f, View.LaneText, LanePx, LaneC, "Bold", .5f, 50);
	}

	// ---- subtitles / story transmissions: centred under the lower lane, speaker tag in the pilot's colour, 2 lines max.
	// ENG-45: nothing when subtitles are off (barks) or the transmission says so (story lines); the voices still play.
	if (!View.Subtitle.Text.IsEmpty() && bSubtitlesOn && !(View.Subtitle.bStory && View.Subtitle.bStoryHidden))
	{
		const FSubtitle& Sub = View.Subtitle;
		const FString& Who = Sub.Speaker;
		const FLinearColor Tag = Sub.TagColor;
		// Fade in 0.15 s, out over the last 0.3 s.
		const float Fade = FMath::Min(FMath::Clamp(Sub.Age / .15f, 0.f, 1.f), FMath::Clamp((Sub.Seconds - Sub.Age) / .3f, 0.f, 1.f));
		// Static: the line flickers and jitters like a bad channel; Echo: a faint late copy trails it.
		const bool bStatic = Sub.Style == ETransmissionStyle::Static;
		const bool bEcho = Sub.Style == ETransmissionStyle::Echo;
		const float Flicker = bStatic ? .65f + .35f * FMath::Abs(FMath::Sin(Now * 37.f) * FMath::Cos(Now * 23.f)) : 1.f;
		const float Jitter = bStatic ? FMath::Sin(Now * 61.f) * 2.f : 0.f;
		// #155: the size follows UTDSettingsSubsystem::GetSubtitleScale (Small / Medium / Large), read every tick.
		const float Sc = View.SubtitleScale;
		const int32 BodyPx = FMath::RoundToInt(22.f * Sc), TagPx = FMath::RoundToInt(16.f * Sc);
		// #154: wrapped by the measured width of each line at word breaks, up to three lines; the plate fits the text and
		// grows upward, at most 80% of the frame wide and clear of the radar when the #160 HUD is on. The wrap is cached
		// per line / size (no per-frame measuring of the whole text).
		const float TagW = Who.IsEmpty() ? 0.f : FMath::Max(P.Measure(Who, P.Font("Bold", TagPx, 80)), Who.Len() * 11.5f * Sc) + 20.f;
		// With the #160 HUD on, the plate stops short of the radar, which is 280 * PanelK design units wide (wider in
		// design units at 720p: the three-line Large block ran under it in the first capture).
		const float MaxPlateW = View.bSquadPanel ? 2.f * (1920.f - SafeX - 280.f * View.PanelK - CenterX - 24.f) : 1536.f;
		const float MaxTextW = MaxPlateW - 44.f - TagW;
		auto LineW = [&P, BodyPx, Sc](const FString& S)
		{
			if (S.IsEmpty()) { return 0.f; }
			const float M = P.Measure(S, P.Font("Medium", BodyPx, 10));
			return M > 0.f ? M + 4.f : S.Len() * 10.5f * Sc; // headless: no font measure, an estimate
		};
		if (SubWrapText != Sub.Text || SubWrapPx != BodyPx || !FMath::IsNearlyEqual(SubWrapMax, MaxTextW, 1.f))
		{
			SubWrapText = Sub.Text;
			SubWrapPx = BodyPx;
			SubWrapMax = MaxTextW;
			SubWrapLines.Reset();
			SubWrapWidth = 0.f;
			FString Line;
			TArray<FString> WordList;
			Sub.Text.ParseIntoArray(WordList, TEXT(" "), true);
			for (const FString& Word : WordList)
			{
				const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
				if (!Line.IsEmpty() && SubWrapLines.Num() < 2 && LineW(Try) > MaxTextW)
				{
					SubWrapWidth = FMath::Max(SubWrapWidth, LineW(Line));
					SubWrapLines.Add(MoveTemp(Line));
					Line = Word;
				}
				else
				{
					Line = Try;
				}
			}
			if (!Line.IsEmpty())
			{
				SubWrapWidth = FMath::Max(SubWrapWidth, LineW(Line));
				SubWrapLines.Add(MoveTemp(Line));
			}
		}
		const int32 Lines = FMath::Max(1, SubWrapLines.Num());
		// Slate font sizes are points (a 29 pt Large line is about 39 px tall): the line step follows the font, not a fixed 26.
		const float LineH = BodyPx * 1.45f;
		const float H = 12.f * Sc + Lines * LineH;
		// Above the squad panel and its flag row (#160), growing upward.
		const float SubY = View.SubtitleBottom - H;
		// The plate fits the text, centred, inside the safe frame (a third line that is still too long widens it to 80%).
		const float PlateW = FMath::Clamp(44.f + TagW + SubWrapWidth, 240.f, FMath::Min(1536.f, 1920.f - 2.f * SafeX));
		const float X = CenterX - PlateW * .5f;
		P.Plate(X, SubY, PlateW, H, 8.f, T::Plate().CopyWithNewOpacity(.6f * Fade));
		Rec(TEXT("subtitle"), X, SubY, PlateW, H);
		Area += PlateW * H;
		if (bStatic)
		{
			for (int32 S = 0; S < 3; ++S) // scan lines drifting through the plate
			{
				const float Y = SubY + FMath::Fmod(Now * 40.f + S * 21.f, H);
				P.Box(X + 6.f, Y, PlateW - 12.f, 1.f, T::Neutral().CopyWithNewOpacity(.08f * Fade));
			}
		}
		const float TextX = X + 22.f + Jitter;
		if (!Who.IsEmpty())
		{
			P.Words(TextX, SubY + 8.f * Sc, Who, TagPx, Tag.CopyWithNewOpacity(Fade * Flicker), "Bold", 0.f, 80);
		}
		const float BodyX = TextX + TagW;
		const FLinearColor Body = T::Neutral().CopyWithNewOpacity(Fade * Flicker);
		for (int32 L = 0; L < SubWrapLines.Num(); ++L)
		{
			const float LY = SubY + 6.f * Sc + L * LineH;
			if (bEcho)
			{
				P.Words(BodyX + 4.f, LY + 3.f * Sc, SubWrapLines[L], BodyPx, T::Shield().CopyWithNewOpacity(.28f * Fade), "Medium", 0.f, 10);
			}
			P.Words(BodyX, LY, SubWrapLines[L], BodyPx, Body, "Medium", 0.f, 10);
		}
	}

	// ---- squad strip, bottom-left of the safe frame, order 1 2 3
	const float Pulse1Hz = .75f + .25f * FMath::Sin(Time * 2.f * PI);
	// The squad panel and the radar leave with the combat HUD: while docking (controls locked), under a focus hold that
	// hides combat (director shots, letterboxed sequences, T4's breakup camera), and they fade out with a money shot.
	const float PanelAlpha = (View.Docking > 0 || View.bCombatHeld) ? 0.f : 1.f - FMath::Clamp(FMath::Max(View.MoneyShot, View.ShotHide) * 2.f, 0.f, 1.f);
	FPaint PanelP = P;
	PanelP.Alpha *= PanelAlpha;
	const bool bPanelShown = View.bSquadPanel && PanelP.Alpha > .01f;
	if (bPanelShown)
	{
		float Left = 1e9f, Top = 1e9f, Right = 0.f;
		static const FString PadAbilityKey(TEXT("Y")), KeyAbilityKey(TEXT("E"));
		const FString& AbilityKey = View.bUsingGamepad ? PadAbilityKey : KeyAbilityKey;
		for (const FChip& Chip : View.Chips)
		{
			const FSquadCard& Card = View.Cards[static_cast<int32>(Chip.Ship)];
			if (Card.bValid)
			{
				SquadCard(PanelP, Card, View.PanelK, View.PanelFont, Time, View.PowerGlyph, AbilityKey);
				Rec(TEXT("chip"), Card.Pos.X, Card.Pos.Y, Card.Size.X, Card.Size.Y);
				Area += Card.Size.X * Card.Size.Y;
				Left = FMath::Min(Left, float(Card.Pos.X));
				Top = FMath::Min(Top, float(Card.Pos.Y));
				Right = FMath::Max(Right, float(Card.Pos.X + Card.Size.X));
			}
		}
		if (Right > Left)
		{
			Rec(TEXT("squad"), Left, Top, Right - Left, 1080.f - SafeY - Top);
		}
	}
#if __has_include("TriadDrift/UI/TDHUDRadarPaint.h")
	// T6's radar (#160): bottom-right, from the HUD model.
	if (bPanelShown)
	{
		TDPaintHUDRadar(PanelP, HUDModel, FTDHUDRadarInput{ View.Flown, View.bHasFlown, FrameWave, Time,
			View.GeneratorsTotal, View.GeneratorsDown, View.bReactorExposed, View.ReactorPhase, View.ReactorPhases });
		const FBox2D RadarBox = TDHUDRadarBox(PanelP);
		Rec(TEXT("radar"), RadarBox.Min.X, RadarBox.Min.Y, RadarBox.GetSize().X, RadarBox.GetSize().Y);
	}
#endif
	static const TArray<FChip> NoChips;
	for (const FChip& Chip : View.bSquadPanel ? NoChips : View.Chips)
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

	// ---- objective line: top-centre pill, diamond, caps, counter right; expands for 3 s on change (held off by a focus hold)
	if (!View.Objective.IsEmpty() && !View.bCombatHeld)
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
	// ---- the escape flight: "ESCAPE 0:42" and a draining bar under the objective (red in the last 20 s, pulsing in the
	// last 10), and an amber "CLEAR" diamond with the distance still to fly where the way out is.
	if (View.bEscapeFlight)
	{
		const float W = 300.f, H = 64.f, X = CenterX - W * .5f, Y = SafeY + ObjectiveHeight + (View.GeneratorsTotal > 0 ? 40.f : 12.f);
		const bool bLate = View.EscapeLeft <= 20.f;
		const float Pulse = View.EscapeLeft <= 10.f ? .6f + .4f * FMath::Abs(FMath::Sin(Now * PI * 2.f)) : 1.f;
		const FLinearColor Col = (bLate ? T::Danger() : T::Bastion()).CopyWithNewOpacity(Pulse);
		P.Plate(X, Y, W, H, 14.f, T::Plate().CopyWithNewOpacity(.8f));
		const int32 Secs = FMath::CeilToInt(View.EscapeLeft - KINDA_SMALL_NUMBER);
		P.Words(X + 18.f, Y + 8.f, TEXT("ESCAPE"), 12, Col, "Bold", 0.f, 160);
		P.Words(X + 18.f, Y + 24.f, FString::Printf(TEXT("%d:%02d"), Secs / 60, Secs % 60), 28, T::Neutral(), "Bold", 0.f, 40);
		P.Box(X + 120.f, Y + 38.f, (W - 138.f) * FMath::Clamp(View.EscapeLeft / View.EscapeTotal, 0.f, 1.f), 6.f, Col);
		Rec(TEXT("escape"), X, Y, W, H);
		Area += W * H;
		const FVector2D CP = View.ClearPos;
		P.Poly(CP, { { 0, -12 }, { 12, 0 }, { 0, 12 }, { -12, 0 }, { 0, -12 } }, T::Bastion(), 2.5f);
		P.Words(CP.X + 18.f, CP.Y - 16.f, TEXT("CLEAR"), 12, T::Bastion(), "Bold", 0.f, 150);
		P.Words(CP.X + 18.f, CP.Y, View.ClearMetres >= 1000.f ? FString::Printf(TEXT("%.1f km"), View.ClearMetres / 1000.f)
			: FString::Printf(TEXT("%.0f m"), View.ClearMetres), 12, T::Neutral().CopyWithNewOpacity(.85f), "Bold", 0.f, 40);
		Rec(TEXT("clear"), CP.X - 12.f, CP.Y - 12.f, 24.f, 24.f);
	}

	// ---- the Dominion's shield generators: one pip each under the objective line, filled amber when down; the newest
	// down flashes white and the row reads "GENERATOR DOWN" for a moment (storyline.md Gameplay 06: distinct feedback).
	if (View.GeneratorsTotal > 0)
	{
		const float Pip = 14.f, Gap = 10.f;
		const float RowW = View.GeneratorsTotal * Pip + (View.GeneratorsTotal - 1) * Gap;
		const float PX = CenterX - RowW * .5f, PY = SafeY + ObjectiveHeight + 12.f;
		for (int32 G = 0; G < View.GeneratorsTotal; ++G)
		{
			const FVector2D PC(PX + G * (Pip + Gap) + Pip * .5f, PY + Pip * .5f);
			const bool bDown = G < View.GeneratorsDown;
			const bool bNewest = bDown && G == View.GeneratorsDown - 1 && View.GeneratorFlash01 > 0.f;
			const FLinearColor Col = bNewest ? FMath::Lerp(T::Warning(), T::Neutral(), View.GeneratorFlash01) : bDown ? T::Bastion() : T::Dim();
			if (bDown)
			{
				P.Box(PC.X - 4.f, PC.Y - 4.f, 8.f, 8.f, Col);
			}
			P.Poly(PC, { { 0, -7 }, { 6, -3.5f }, { 6, 3.5f }, { 0, 7 }, { -6, 3.5f }, { -6, -3.5f }, { 0, -7 } }, Col.CopyWithNewOpacity(bDown ? 1.f : .7f), 2.f);
		}
		// The pips are the count (the objective line's own "n / 4" is stripped in Sample): only the flash has words.
		if (View.GeneratorFlash01 > 0.f)
		{
			static const FString Down(TEXT("GENERATOR DOWN"));
			P.Words(PX + RowW + 14.f, PY - 1.f, Down, 13, T::Warning().CopyWithNewOpacity(.9f), "Bold", 0.f, 80);
		}
		Rec(TEXT("generators"), PX, PY, RowW + 120.f, Pip);
		Area += (RowW + 120.f) * Pip;
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
