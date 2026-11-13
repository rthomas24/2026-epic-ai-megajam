// Triad Drift - Director (Teammate 3). C-03 fallback presentation (see header).

#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Core/TDDebugHUD.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

namespace TDCutPresentationPrivate
{
	/** Real text size (the font measure service), with a rough fallback when Slate isn't up. */
	static FVector2D MeasureText(const FText& T, const FSlateFontInfo& Font)
	{
		if (FSlateApplication::IsInitialized())
		{
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			return Measure->Measure(T, Font);
		}
		return FVector2D(Font.Size * 0.62f * T.ToString().Len(), Font.Size * 1.3f);
	}
}
using TDCutPresentationPrivate::MeasureText;
#include "TriadDrift/UI/TDUIHostSettings.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/SlateFontInfo.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/Intro/TDIntroDirector.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "TDCutPresentation"

namespace TDCutPresentationPrivate
{
	/** Paints the model. No layout, no input: a hit-test-invisible leaf over the whole viewport. */
	class STDCutOverlay : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(STDCutOverlay) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, UTDCutPresentation* InModel)
		{
			Model = InModel;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Culling,
			FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			const UTDCutPresentation* M = Model.Get();
			if (!M || M->IsHandledExternally() || M->IsSuppressedByIntro())
			{
				return Layer;
			}
			const FTDCutPresentationState& S = M->GetState();
			if (S.IsEmpty())
			{
				return Layer;
			}
			const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
			const FVector2D Size = Geo.GetLocalSize();

			auto Box = [&](const FVector2D& Pos, const FVector2D& Extent, const FLinearColor& Color)
			{
				FSlateDrawElement::MakeBox(Out, Layer, Geo.ToPaintGeometry(Extent, FSlateLayoutTransform(Pos)), White,
					ESlateDrawEffect::None, Color);
			};
			auto Text = [&](const FText& T, int32 FontSize, const FVector2D& Center, const FLinearColor& Color, bool bCentered)
			{
				const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FontSize);
				const FVector2D Measure = MeasureText(T, Font);
				const FVector2D Pos = bCentered ? Center - Measure * 0.5f : Center;
				// Drop shadow for readability over bright space.
				FSlateDrawElement::MakeText(Out, Layer + 1, Geo.ToPaintGeometry(Measure, FSlateLayoutTransform(Pos + FVector2D(3.f, 3.f))),
					T, Font, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f));
				FSlateDrawElement::MakeText(Out, Layer + 2, Geo.ToPaintGeometry(Measure, FSlateLayoutTransform(Pos)),
					T, Font, ESlateDrawEffect::None, Color);
			};

			// Letterbox bars.
			if (S.Letterbox > 0.f)
			{
				const float H = Size.Y * S.Letterbox;
				Box(FVector2D::ZeroVector, FVector2D(Size.X, H), FLinearColor::Black);
				Box(FVector2D(0.f, Size.Y - H), FVector2D(Size.X, H), FLinearColor::Black);
			}
			// Pilot name card, lower centre on the bottom bar: clear of the dev HUD's left-hand squad rows
			// and flight line, and of the crosshair. (Hidden while the real HUD draws its own card.)
			if (S.NameCardAlpha > 0.f)
			{
				FLinearColor C = S.NameCardColor;
				C.A = S.NameCardAlpha;
				const float BarH = Size.Y * FMath::Max(S.Letterbox, 0.08f);
				const float Y = Size.Y - BarH - 30.f;
				Text(S.NameCardText, 30, FVector2D(Size.X * 0.5f, Y - 8.f), FLinearColor(1.f, 1.f, 1.f, S.NameCardAlpha), true);
				Box(FVector2D(Size.X * 0.5f - 90.f, Y + 18.f), FVector2D(180.f, 5.f), C); // squad-colour underline
			}
			// Slate, top centre under the bar.
			if (S.SlateAlpha > 0.f)
			{
				const FVector2D TextSize = MeasureText(S.SlateText, FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 28));
				const FBox2D R = UTDCutPresentation::SlateRect(Size, S.Letterbox, TextSize.X);
				Box(R.Min, R.GetSize(), FLinearColor(0.f, 0.f, 0.f, 0.65f * S.SlateAlpha));
				Text(S.SlateText, 28, R.GetCenter(), FLinearColor(1.f, 1.f, 1.f, S.SlateAlpha), true);
			}
			// "CUT!"
			if (S.CutCardAlpha > 0.f)
			{
				Text(S.CutCardText, 120, Size * 0.5f, FLinearColor(1.f, 0.92f, 0.25f, S.CutCardAlpha), true);
			}
			// The cut flash last, over everything.
			if (S.FlashAlpha > 0.f)
			{
				Box(FVector2D::ZeroVector, Size, FLinearColor(1.f, 1.f, 1.f, S.FlashAlpha));
			}
			return Layer + 3;
		}

	private:
		TWeakObjectPtr<UTDCutPresentation> Model;
	};

	/** 0..1 for a card visible over [Start, Start+Length] with Fade in and out. */
	float CardAlpha(double Now, double Start, float Length, float Fade)
	{
		const float T = static_cast<float>(Now - Start);
		if (T < 0.f || T > Length)
		{
			return 0.f;
		}
		const float In = Fade > 0.f ? FMath::Clamp(T / Fade, 0.f, 1.f) : 1.f;
		const float OutA = Fade > 0.f ? FMath::Clamp((Length - T) / Fade, 0.f, 1.f) : 1.f;
		return FMath::Min(In, OutA);
	}
}

using namespace TDCutPresentationPrivate;

// ---------------------------------------------------------------------------------------------

void UTDCutPresentation::Init(UWorld* InWorld)
{
	OwnerWorld = InWorld;
	EnsureViewportWidget();
	BindSquad();
}

void UTDCutPresentation::Shutdown()
{
	if (ATDSquadManager* Squad = BoundSquad.Get())
	{
		Squad->OnSwitchCut.RemoveAll(this);
	}
	BoundSquad.Reset();
	ClearAll();
	RemoveViewportWidget();
}

double UTDCutPresentation::Now() const
{
	const UWorld* World = OwnerWorld.Get();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

bool UTDCutPresentation::IsSuppressedByIntro() const
{
	return UTDIntroDirector::IsIntroPlaying(OwnerWorld.Get());
}

void UTDCutPresentation::EnsureViewportWidget()
{
	UWorld* World = OwnerWorld.Get();
	if (Widget.IsValid() || !World || !World->IsGameWorld() || !GEngine || !GEngine->GameViewport)
	{
		return; // headless / editor worlds: model only
	}
	if (GEngine->GameViewport->GetWorld() != World)
	{
		return;
	}
	Widget = SNew(STDCutOverlay, this);
	WidgetZOrder = OverlayZOrder(ATDDebugHUD::IsRealHUDActive(World));
	GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), WidgetZOrder);
}

int32 UTDCutPresentation::OverlayZOrder(bool bRealHUDActive)
{
	// Live capture with the real HUD: our letterbox bars covered Nick's top row and the squad cards right
	// when a switch needs them. Under his HUD the bars frame the picture and leave the panels readable.
	// Without it, stay above gameplay widgets and under T5's F10 menu (1000).
	return bRealHUDActive ? UTDUIHostSettings::Get()->ZOrder - 1 : 900;
}

void UTDCutPresentation::RemoveViewportWidget()
{
	if (Widget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
	}
	Widget.Reset();
}

void UTDCutPresentation::BindSquad()
{
	ATDSquadManager* Squad = ATDSquadManager::Get(OwnerWorld.Get());
	if (Squad == BoundSquad.Get())
	{
		return;
	}
	if (ATDSquadManager* Old = BoundSquad.Get())
	{
		Old->OnSwitchCut.RemoveAll(this);
	}
	BoundSquad = Squad;
	if (Squad)
	{
		Squad->OnSwitchCut.AddUniqueDynamic(this, &UTDCutPresentation::HandleSwitchCut);
	}
}

void UTDCutPresentation::SetHandledExternally(bool bHandled)
{
	bHandledExternally = bHandled;
	if (bHandled)
	{
		ClearAll();
	}
}

void UTDCutPresentation::ClearAll()
{
	SwitchStart = CutStart = SlateStart = ShotStart = -1000.0;
	FlashFramesLeft = 0;
	State = FTDCutPresentationState();
}

FBox2D UTDCutPresentation::SlateRect(const FVector2D& Size, float Letterbox, float TextWidth)
{
	// Fits its text (the numbered titles run long: "SCENE 6 · CINDER · OUTPOST · TAKE 1"), never off screen.
	const FVector2D Half(FMath::Min(FMath::Max(300.f, TextWidth * 0.5f + 40.f), Size.X * 0.45f), 32.f);
	const float CentreY = FMath::Max(Size.Y * 0.27f, Size.Y * Letterbox + Half.Y + 16.f);
	const FVector2D C(Size.X * 0.5f, CentreY);
	return FBox2D(C - Half, C + Half);
}

FText UTDCutPresentation::MakeSlateText(FName SceneId, int32 TakeNumber, int32 SceneNumber, const FString& Place)
{
	FString Id = SceneId.ToString();
	Id.RemoveFromStart(TEXT("Obj_")); // planet objectives: "Obj_Outpost" -> "OUTPOST"
	FString Pretty;
	for (int32 i = 0; i < Id.Len(); ++i)
	{
		const TCHAR C = Id[i];
		const bool bBoundary = i > 0 && ((FChar::IsDigit(C) && !FChar::IsDigit(Id[i - 1])) || (FChar::IsUpper(C) && FChar::IsLower(Id[i - 1])));
		if (bBoundary)
		{
			Pretty.AppendChar(TEXT(' '));
		}
		Pretty.AppendChar(C == TEXT('_') ? TEXT(' ') : C);
	}
	FString Title = Pretty.ToUpper();
	if (!Place.IsEmpty() && !Title.StartsWith(Place.ToUpper()))
	{
		Title = Place.ToUpper() + TEXT(" · ") + Title;
	}
	const FText Take = FText::AsNumber(FMath::Max(1, TakeNumber));
	return SceneNumber > 0
		? FText::Format(LOCTEXT("SlateNumbered", "SCENE {0} · {1} · TAKE {2}"), FText::AsNumber(SceneNumber), FText::FromString(Title), Take)
		: FText::Format(LOCTEXT("Slate", "{0} · TAKE {1}"), FText::FromString(Title), Take);
}

// ---------------------------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------------------------

void UTDCutPresentation::HandleSwitchCut(const FTDSwitchCut& Cut)
{
	if (bHandledExternally)
	{
		return;
	}
	SwitchStart = Now();
	SwitchTiming = Cut.Timing;
	const float DebugHold = UTDDirectorSettings::GetDebugHoldSeconds();
	SwitchTiming.LetterboxHoldSeconds += DebugHold;
	SwitchTiming.NameCardSeconds += DebugHold;
	bQuickCut = Cut.bQuickCut;
	FlashFramesLeft = FMath::Max(0, Cut.Timing.FlashFrames);
	NameCard = Cut.bSwitchStrike
		? FText::Format(NSLOCTEXT("TDDirector", "StrikeCard", "{0}  ·  STRIKE +25%"), Cut.NameCardText) : Cut.NameCardText;
	NameColor = Cut.SquadColor;
	State.FlashAlpha = FlashFramesLeft > 0 ? 1.f : 0.f; // visible on the very next paint
}

void UTDCutPresentation::HandleSceneCut(float DelaySeconds)
{
	if (bHandledExternally)
	{
		return;
	}
	CutStart = Now();
	CutSeconds = FMath::Max(Tuning.CutCardMinSeconds, DelaySeconds) + UTDDirectorSettings::GetDebugHoldSeconds();
	SlateStart = -1000.0;
}

void UTDCutPresentation::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	if (bHandledExternally || SceneId.IsNone())
	{
		return; // a death outside any scene has no slate
	}
	CutStart = -1000.0; // the take replaces the CUT! card
	SlateStart = Now();
	// The scene's number in this run and, for a planet objective, the planet: from the game flow.
	int32 SceneNumber = 0;
	FString Place;
	if (const UTDGameInstance* GI = UTDGameInstance::Get(OwnerWorld.Get()))
	{
		SceneNumber = GI->GetSceneNumber(SceneId);
		const ETDDestination Where = GI->GetCurrentDestination();
		Place = Where == ETDDestination::Verdance ? TEXT("Verdance") : (Where == ETDDestination::Cinder ? TEXT("Cinder") : FString());
	}
	Slate = MakeSlateText(SceneId, TakeNumber, SceneNumber, Place);
}

void UTDCutPresentation::HandleShotStarted(float RealSeconds)
{
	if (bHandledExternally)
	{
		return;
	}
	ShotStart = Now();
	ShotSeconds = FMath::Max(0.f, RealSeconds) + UTDDirectorSettings::GetDebugHoldSeconds();
}

// ---------------------------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------------------------

void UTDCutPresentation::Tick(double RealNow)
{
	// The real HUD can come and go (TD.UI.NativeHUD, travel): keep the overlay on the right layer.
	if (Widget.IsValid() && OverlayZOrder(ATDDebugHUD::IsRealHUDActive(OwnerWorld.Get())) != WidgetZOrder)
	{
		RemoveViewportWidget();
	}
	EnsureViewportWidget();
	BindSquad();
	if (bHandledExternally)
	{
		if (!State.IsEmpty())
		{
			ClearAll();
		}
		return;
	}

	FTDCutPresentationState S;

	// Switch: 1-frame flash, letterbox in/hold/out, name card.
	const float SwitchT = static_cast<float>(RealNow - SwitchStart);
	if (FlashFramesLeft > 0)
	{
		S.FlashAlpha = 1.f;
		--FlashFramesLeft;
	}
	if (!bQuickCut && SwitchT >= 0.f)
	{
		const FTDSwitchCutTiming& T = SwitchTiming;
		float A = 0.f;
		if (SwitchT < T.LetterboxInSeconds)
		{
			A = T.LetterboxInSeconds > 0.f ? SwitchT / T.LetterboxInSeconds : 1.f;
		}
		else if (SwitchT < T.LetterboxInSeconds + T.LetterboxHoldSeconds)
		{
			A = 1.f;
		}
		else if (SwitchT < T.LetterboxInSeconds + T.LetterboxHoldSeconds + T.LetterboxOutSeconds)
		{
			A = 1.f - (SwitchT - T.LetterboxInSeconds - T.LetterboxHoldSeconds) / FMath::Max(T.LetterboxOutSeconds, KINDA_SMALL_NUMBER);
		}
		S.Letterbox = FMath::Max(S.Letterbox, Tuning.SwitchLetterbox * FMath::Clamp(A, 0.f, 1.f));
	}
	const float CardA = CardAlpha(RealNow, SwitchStart, SwitchTiming.NameCardSeconds, Tuning.CardFadeSeconds * 0.6f);
	// N-007 (Lead): Nick's real HUD draws its own "CUT TO" card; ours would sit on top of it. Flash,
	// letterbox, CUT! and the slate stay ours until bCutPresentationHandledExternally.
	if (CardA > 0.f && !ATDDebugHUD::IsRealHUDActive(OwnerWorld.Get()))
	{
		S.NameCardText = NameCard;
		S.NameCardColor = NameColor;
		S.NameCardAlpha = CardA;
	}

	// Money shot: thin bars.
	const float ShotA = CardAlpha(RealNow, ShotStart, ShotSeconds + Tuning.ShotLetterboxEaseSeconds, Tuning.ShotLetterboxEaseSeconds);
	S.Letterbox = FMath::Max(S.Letterbox, Tuning.ShotLetterbox * ShotA);

	// "CUT!" with full bars.
	const float CutA = CardAlpha(RealNow, CutStart, CutSeconds, Tuning.CardFadeSeconds * 0.4f);
	if (CutA > 0.f)
	{
		S.CutCardText = LOCTEXT("Cut", "CUT!");
		S.CutCardAlpha = CutA;
		S.Letterbox = FMath::Max(S.Letterbox, Tuning.SwitchLetterbox * CutA);
	}

	// Slate.
	float SlateA = CardAlpha(RealNow, SlateStart, Tuning.SlateSeconds, Tuning.CardFadeSeconds);
	if (SlateA <= 0.f && UTDDirectorSettings::GetDebugHoldSeconds() > 0.f)
	{
		SlateA = CardAlpha(RealNow, SlateStart, Tuning.SlateSeconds + UTDDirectorSettings::GetDebugHoldSeconds(), Tuning.CardFadeSeconds);
	}
	if (SlateA > 0.f)
	{
		S.SlateText = Slate;
		S.SlateAlpha = SlateA;
	}

	State = S;
	if (Widget.IsValid())
	{
		Widget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

#undef LOCTEXT_NAMESPACE
