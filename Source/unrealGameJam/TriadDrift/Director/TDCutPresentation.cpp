// Triad Drift - Director (Teammate 3). C-03 fallback presentation (see header).

#include "TriadDrift/Director/TDCutPresentation.h"
#include "TriadDrift/Director/TDDirectorSounds.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "EngineUtils.h"
#include "TriadDrift/Ground/TDObjective.h"
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
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "Blueprint/UserWidget.h"

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
				// Left of the ship at reticle height: clear of T2's ship zone (790-1130 x 612-800 at 1080p), the tutorial card
				// (830+) and the squad strip (Lead, after a capture with each ship: the Bastion is the widest).
				FVector2D At = UTDCutPresentation::NameCardCentre(Size);
				// Issue #49 (same check as the intro card): the centred lines never leave the title-safe frame (5%), so a
				// long strike card or trait line on a narrow window is pulled in instead of clipped.
				const float NameW = MeasureText(S.NameCardText, FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 30)).X;
				const float SubW = S.NameCardSubText.IsEmpty() ? 0.f : MeasureText(S.NameCardSubText, FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16)).X;
				const float Half = FMath::Max3(NameW, SubW, 180.f) * 0.5f;
				At.X = FMath::Clamp(At.X, Size.X * 0.05f + Half, FMath::Max(Size.X * 0.05f + Half, Size.X * 0.95f - Half));
				const float Y = At.Y;
				Text(S.NameCardText, 30, FVector2D(At.X, Y - 8.f), FLinearColor(1.f, 1.f, 1.f, S.NameCardAlpha), true);
				const float UnderW = FMath::Max(180.f, NameW);
				Box(FVector2D(At.X - UnderW * 0.5f, Y + 18.f), FVector2D(UnderW, 5.f), C); // squad-colour underline, as wide as the name
				if (!S.NameCardSubText.IsEmpty())
				{
					Text(S.NameCardSubText, 16, FVector2D(At.X, Y + 40.f), C, true); // "HEAVY · STEADY · GOOD-HUMOURED"
				}
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
	// The intro owns the screen, except its last second (the hand-off), when the slate comes up.
	const UTDIntroDirector* Intro = UTDIntroDirector::Get(OwnerWorld.Get());
	return Intro && Intro->IsPlaying() && !Intro->IsHandingOff();
}

void UTDCutPresentation::PreviewFirstSlate(FName SceneId)
{
	if (bHandledExternally || SceneId.IsNone())
	{
		return;
	}
	Slate = MakeSlateText(SceneId, 1, 1);
	SlateScene = SceneId;
	SlateTake = 1;
	SlateStart = Now();
	PreviewSlateAt = SlateStart;
	FTDDirectorSounds::Play(OwnerWorld.Get(), ETDDirectorSound::SlateClap);
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

FVector2D UTDCutPresentation::NameCardCentre(const FVector2D& Size)
{
	// 90 px further left and a little higher at 1080p than before (T1's switching-under-fire play: the card ran into the
	// reticle and the lock box when the ship sat left of centre). T2, with the Lead's leave for this one value.
	return FVector2D(Size.X * 0.198f, Size.Y * 0.45f);
}

#if WITH_DEV_AUTOMATION_TESTS
TOptional<bool> UTDCutPresentation::NameCardClaimOverrideForTests;
#endif

bool UTDCutPresentation::IsNameCardClaimedByHUD(const UWorld* World)
{
#if WITH_DEV_AUTOMATION_TESTS
	if (NameCardClaimOverrideForTests.IsSet())
	{
		return NameCardClaimOverrideForTests.GetValue();
	}
#endif
	const UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(World);
	const UUserWidget* HUD = Host ? Host->GetSpaceHUD() : nullptr;
	if (!HUD)
	{
		return false; // the dev HUD draws no pilot card
	}
	if (const FBoolProperty* Claim = FindFProperty<FBoolProperty>(HUD->GetClass(), TEXT("bCoversNameCard")))
	{
		return Claim->GetPropertyValue_InContainer(HUD);
	}
	return UTDUIHostSettings::GetSpaceHUDChoice() == ETDSpaceHUDChoice::Nick; // his widget has its own "CUT TO" card
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
	Cards = FTDDirectorCards();
	Cards.SwitchCount = SwitchCount;
}

uint32 FTDDirectorCards::Signature() const
{
	uint32 H = 0;
	for (const FTDDirectorCard* C : { &SwitchCut, &NameCard, &Slate, &Cut, &MoneyShot })
	{
		H = HashCombine(H, C->bActive ? 1u : 0u);
		H = HashCombine(H, GetTypeHash(C->bActive ? C->Text.ToString() : FString()));
	}
	H = HashCombine(H, bNameCardClaimedByHUD ? 1u : 0u); // N-038: a renderer hears when the HUD takes or gives back the card
	return HashCombine(H, HashCombine(GetTypeHash(SwitchCount), GetTypeHash(SlateTake)));
}

FBox2D UTDCutPresentation::SlateRect(const FVector2D& Size, float Letterbox, float TextWidth)
{
	// Fits its text (the numbered titles run long: "SCENE 6 · CINDER · OUTPOST · TAKE 1"), never off screen.
	const FVector2D Half(FMath::Min(FMath::Max(300.f, TextWidth * 0.5f + 40.f), Size.X * 0.45f), 32.f);
	const float CentreY = FMath::Max(Size.Y * 0.27f, Size.Y * Letterbox + Half.Y + 16.f);
	const FVector2D C(Size.X * 0.5f, CentreY);
	return FBox2D(C - Half, C + Half);
}

FText UTDCutPresentation::FindSceneTitle(const UWorld* World, FName SceneId)
{
	if (!World || SceneId.IsNone())
	{
		return FText::GetEmpty();
	}
	for (TActorIterator<ATDObjective> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->ObjectiveId == SceneId && !It->Title.IsEmpty())
		{
			return It->Title;
		}
	}
	return FText::GetEmpty();
}

FText UTDCutPresentation::MakeSlateText(FName SceneId, int32 TakeNumber, int32 SceneNumber, const FString& Place, const FText& SceneTitle)
{
	if (!SceneTitle.IsEmpty())
	{
		// The story name (T5's objective titles, storyline.md): "SCENE 4 · VERDANCE · THE RESEARCH SITE · TAKE 2".
		FString Title = SceneTitle.ToString().ToUpper();
		if (!Place.IsEmpty() && !Title.StartsWith(Place.ToUpper()))
		{
			Title = Place.ToUpper() + TEXT(" · ") + Title;
		}
		const FText Take = FText::AsNumber(FMath::Max(1, TakeNumber));
		return SceneNumber > 0
			? FText::Format(LOCTEXT("SlateNumberedTitle", "SCENE {0} · {1} · TAKE {2}"), FText::AsNumber(SceneNumber), FText::FromString(Title), Take)
			: FText::Format(LOCTEXT("SlateTitle", "{0} · TAKE {1}"), FText::FromString(Title), Take);
	}
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
	const FText Trait = Cut.NewShipActor ? UTDStorySubsystem::PilotTrait(Cut.NewShipActor->GetShipId()) : FText::GetEmpty();
	// The card says "MOSS · BASTION"; the line under it is the role and traits, never the ship again (Lead).
	NameCardSub = Trait.IsEmpty() ? FText::Format(NSLOCTEXT("TDDirector", "PilotSub", "{0}"), Cut.RoleName) : Trait;
	++SwitchCount;
	State.FlashAlpha = FlashFramesLeft > 0 ? 1.f : 0.f; // visible on the very next paint
}

void UTDCutPresentation::HandleSceneCut(float DelaySeconds)
{
	FTDDirectorSounds::Play(OwnerWorld.Get(), ETDDirectorSound::CutSting); // ENG-35: with the card, drawn by us or by Nick
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
	if (SceneId.IsNone())
	{
		return; // a death outside any scene has no slate
	}
	if (bHandledExternally)
	{
		FTDDirectorSounds::Play(OwnerWorld.Get(), ETDDirectorSound::SlateClap); // Nick's widget draws the slate; the clap is ours
		return;
	}
	// The intro owns the screen; T1's atmosphere entry (or any HUD focus hold) keeps combat presentation away (T2's
	// UTDUIHostSubsystem::IsCombatPresentationHeld). The slate waits and comes up when either ends. "CUT!" never waits.
	if (IsSuppressedByIntro() || UTDUIHostSubsystem::IsCombatPresentationHeld(OwnerWorld.Get()))
	{
		PendingSlateScene = SceneId; // shown when the intro or the hold ends
		PendingSlateTake = TakeNumber;
		return;
	}
	// Scene one's first take right after the intro already had its slate at the hand-off.
	if (TakeNumber == 1 && Now() - PreviewSlateAt < 8.0)
	{
		const UTDGameInstance* GIForScene = UTDGameInstance::Get(OwnerWorld.Get());
		if (!GIForScene || GIForScene->GetSceneNumber(SceneId) <= 1)
		{
			PreviewSlateAt = -1000.0;
			return;
		}
	}
	CutStart = -1000.0; // the take replaces the CUT! card
	SlateStart = Now();
	FTDDirectorSounds::Play(OwnerWorld.Get(), ETDDirectorSound::SlateClap); // ENG-35: the clap as the slate comes up
	// The scene's number in this run and, for a planet objective, the planet: from the game flow.
	int32 SceneNumber = 0;
	FString Place;
	if (const UTDGameInstance* GI = UTDGameInstance::Get(OwnerWorld.Get()))
	{
		SceneNumber = GI->GetSceneNumber(SceneId);
		const ETDDestination Where = GI->GetCurrentDestination();
		Place = Where == ETDDestination::Verdance ? TEXT("Verdance") : (Where == ETDDestination::Cinder ? TEXT("Cinder") : FString());
	}
	Slate = MakeSlateText(SceneId, TakeNumber, SceneNumber, Place, FindSceneTitle(OwnerWorld.Get(), SceneId));
	SlateScene = SceneId;
	SlateTake = TakeNumber;
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
	if (!PendingSlateScene.IsNone() && !IsSuppressedByIntro() && !UTDUIHostSubsystem::IsCombatPresentationHeld(OwnerWorld.Get()))
	{
		const FName Scene = PendingSlateScene;
		PendingSlateScene = NAME_None;
		HandleTakeStarted(Scene, PendingSlateTake);
	}
	// The real HUD can come and go (TD.UI.NativeHUD, travel): keep the overlay on the right layer.
	if (Widget.IsValid() && OverlayZOrder(ATDDebugHUD::IsRealHUDActive(OwnerWorld.Get())) != WidgetZOrder)
	{
		RemoveViewportWidget();
	}
	EnsureViewportWidget();
	BindSquad();
	if (bHandledExternally)
	{
		if (!State.IsEmpty() || Cards.AnyActive())
		{
			ClearAll();
		}
		return;
	}

	FTDCutPresentationState S;
	FTDDirectorCards C;
	C.SwitchCount = SwitchCount;
	C.bQuickCut = bQuickCut;
	auto Card = [RealNow](FTDDirectorCard& Out, double Start, float Seconds, float Alpha, const FText& Text)
	{
		const float T = static_cast<float>(RealNow - Start);
		if (Alpha <= 0.f || T < 0.f || Seconds <= 0.f)
		{
			return;
		}
		Out.bActive = true;
		Out.Alpha = Alpha;
		Out.Text = Text;
		Out.Progress01 = FMath::Clamp(T / Seconds, 0.f, 1.f);
		Out.TimeLeft = FMath::Max(0.f, Seconds - T);
	};

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
		const float CutLen = T.LetterboxInSeconds + T.LetterboxHoldSeconds + T.LetterboxOutSeconds;
		Card(C.SwitchCut, SwitchStart, CutLen, FMath::Clamp(A, 0.f, 1.f), NameCard);
		C.SwitchCut.Color = NameColor;
	}
	else if (bQuickCut && SwitchT >= 0.f && SwitchT < 0.1f)
	{
		Card(C.SwitchCut, SwitchStart, 0.1f, 1.f, NameCard); // a quick cut: one beat, no bars
		C.SwitchCut.Color = NameColor;
	}
	const float CardA = CardAlpha(RealNow, SwitchStart, SwitchTiming.NameCardSeconds, Tuning.CardFadeSeconds * 0.6f);
	Card(C.NameCard, SwitchStart, SwitchTiming.NameCardSeconds, CardA, NameCard);
	C.NameCard.SubText = NameCardSub;
	C.NameCard.Color = NameColor;
	C.bNameCardClaimedByHUD = IsNameCardClaimedByHUD(OwnerWorld.Get()); // N-038: for external renderers; NameCard stays intact
	// N-007 (Lead): Nick's real HUD draws its own "CUT TO" card; ours would sit on top of it. It steps aside only for a
	// HUD that really draws one (Nick's, or one that claims bCoversNameCard); under engineering's default flight HUD
	// (ENG-36) the card is ours. Flash, letterbox, CUT! and the slate stay ours until bCutPresentationHandledExternally.
	if (CardA > 0.f && !IsNameCardClaimedByHUD(OwnerWorld.Get()))
	{
		S.NameCardText = NameCard;
		S.NameCardSubText = NameCardSub;
		S.NameCardColor = NameColor;
		S.NameCardAlpha = CardA;
	}

	// Money shot: thin bars.
	const float ShotA = CardAlpha(RealNow, ShotStart, ShotSeconds + Tuning.ShotLetterboxEaseSeconds, Tuning.ShotLetterboxEaseSeconds);
	S.Letterbox = FMath::Max(S.Letterbox, Tuning.ShotLetterbox * ShotA);
	Card(C.MoneyShot, ShotStart, ShotSeconds + Tuning.ShotLetterboxEaseSeconds, ShotA, FText::GetEmpty());
	// Held bars (HoldShotLetterbox): the same height and ease, no card.
	{
		const float Dt = LastHoldTick > 0.0 ? FMath::Clamp(static_cast<float>(RealNow - LastHoldTick), 0.f, 0.1f) : 0.f;
		LastHoldTick = RealNow;
		const float Step = Dt / FMath::Max(Tuning.ShotLetterboxEaseSeconds, 0.01f);
		HeldLetterboxA = FMath::Clamp(HeldLetterboxA + (LetterboxHolds.Num() > 0 ? Step : -Step), 0.f, 1.f);
		S.Letterbox = FMath::Max(S.Letterbox, Tuning.ShotLetterbox * HeldLetterboxA);
	}
	C.MoneyShot.SubText = ShotKindName;

	// "CUT!" with full bars.
	const float CutA = CardAlpha(RealNow, CutStart, CutSeconds, Tuning.CardFadeSeconds * 0.4f);
	if (CutA > 0.f)
	{
		S.CutCardText = LOCTEXT("Cut", "CUT!");
		S.CutCardAlpha = CutA;
		Card(C.Cut, CutStart, CutSeconds, CutA, S.CutCardText);
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
		Card(C.Slate, SlateStart, Tuning.SlateSeconds + Tuning.CardFadeSeconds, SlateA, Slate);
		C.SlateScene = SlateScene;
		C.SlateTake = SlateTake;
	}

	C.Letterbox = S.Letterbox;
	C.FlashAlpha = S.FlashAlpha;
	// The intro owns the screen (its hand-off excepted): no cards for a renderer either; timing runs on.
	if (IsSuppressedByIntro())
	{
		const int32 Count = C.SwitchCount;
		C = FTDDirectorCards();
		C.SwitchCount = Count;
	}
	Cards = C;
	const uint32 Sig = Cards.Signature();
	if (Sig != CardsSignature)
	{
		CardsSignature = Sig;
		OnCardsChanged.Broadcast(Cards);
	}
	// N-038: a renderer widget draws them; the fallback overlay draws nothing.
	State = bExternalRenderer ? FTDCutPresentationState() : S;
	if (Widget.IsValid())
	{
		Widget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

#undef LOCTEXT_NAMESPACE
