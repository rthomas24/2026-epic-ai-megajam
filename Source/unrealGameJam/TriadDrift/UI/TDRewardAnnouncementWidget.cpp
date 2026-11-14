// Triad Drift - the native ability-unlock announcement (Teammate 2). See the header.

#include "TriadDrift/UI/TDRewardAnnouncementWidget.h"

#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "TriadDrift/UI/TDUIPaint.h"

namespace TDRewardAnnouncement
{
	using T = FTDFlightHUDTokens;
}

bool UTDRewardAnnouncementWidget::PresentReward(const FTDRewardAnnouncementDisplay& Display)
{
	if (Display.AbilityId.IsNone() || (bActive && Display.AbilityId != Data.AbilityId))
	{
		return false;
	}
	if (!bActive)
	{
		Age = 0.f; // a refresh of the same award keeps its clock
	}
	Data = Display;
	bActive = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

void UTDRewardAnnouncementWidget::RequestDismiss()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	SetVisibility(ESlateVisibility::Collapsed);
	OnRewardDismissed.Broadcast(Data.AbilityId); // the owner may present the next award from here
}

void UTDRewardAnnouncementWidget::ResetPresentation()
{
	bActive = false;
	Age = 0.f;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDRewardAnnouncementWidget::Advance(float Dt)
{
	if (!bActive)
	{
		return;
	}
	Age += Dt;
	if (Age >= ShowSeconds)
	{
		RequestDismiss();
	}
}

void UTDRewardAnnouncementWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

int32 UTDRewardAnnouncementWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace TDRewardAnnouncement;
	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	LastBoxes.Reset();
	if (!bActive)
	{
		return Base;
	}
	const float In = FMath::Clamp(Age / FadeSeconds, 0.f, 1.f);
	const float Out = FMath::Clamp((ShowSeconds - Age) / FadeSeconds, 0.f, 1.f);
	FTDUIPainter P(AllottedGeometry, OutDrawElements, Base + 1);
	P.Alpha = FMath::Min(In, Out);
	const FLinearColor Accent = Data.Accent;

	// The plate: top centre, under the objective lane, wide enough for the longest name.
	const FString Name = Data.AbilityName.ToString();
	const float NameW = P.MeasureTitle(Name, 46);
	const float W = FMath::Clamp(NameW + 140.f, 620.f, 1100.f), H = Data.bCanUseNow ? 186.f : 164.f;
	const float X = 960.f - W * .5f, Y = 150.f - (1.f - In) * 14.f;
	LastBoxes.Emplace(TEXT("plate"), FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
	P.Glass(X, Y, W, H, T::Plate().CopyWithNewOpacity(.92f), true);
	P.Box(X, Y, W, 3.f, Accent);
	P.Frame(X, Y, W, H, Accent, .45f);

	P.Label(X + 28.f, Y + 18.f, TEXT("NEW ABILITY UNLOCKED"), 14, Accent, 140);
	P.Title(X + 28.f, Y + 40.f, Name, 46, T::Neutral());
	LastBoxes.Emplace(TEXT("name"), FBox2D(FVector2D(X + 28.f, Y + 40.f), FVector2D(X + 28.f + NameW, Y + 92.f)));
	const FString Ship = Data.ShipName.ToString();
	P.Label(X + 28.f, Y + 102.f, Ship, 16, Accent, 120);
	const float ShipW = P.MeasureText(Ship, 16, true) + 40.f;
	P.Text(X + 28.f + ShipW, Y + 102.f, Data.AvailabilityText.ToString(), 16, T::Dim());

	// The key, only where it works: flying the owning ship. On foot the line says which ship to fly.
	if (Data.bCanUseNow && !Data.ActivationKey.IsEmpty())
	{
		const FString Key = Data.ActivationKey.ToString();
		const float KeyW = FMath::Max(34.f, P.MeasureText(Key, 16, true) + 18.f);
		P.Box(X + 28.f, Y + 134.f, KeyW, 28.f, Accent);
		P.CenteredText(X + 28.f + KeyW * .5f, Y + 137.f, Key, 16, FTDUIPalette::PlateInk(), true);
		P.Text(X + 40.f + KeyW, Y + 138.f, TEXT("TO USE"), 15, T::Neutral());
		LastBoxes.Emplace(TEXT("key"), FBox2D(FVector2D(X + 28.f, Y + 134.f), FVector2D(X + 28.f + KeyW, Y + 162.f)));
	}

	// Time left, a thin rail along the bottom.
	FTDUIPainter Rail = P;
	Rail.Layer = P.Layer + 3;
	Rail.Box(X, Y + H - 3.f, W * FMath::Clamp(1.f - Age / ShowSeconds, 0.f, 1.f), 3.f, Accent.CopyWithNewOpacity(.7f));
	return P.Layer + 4;
}
