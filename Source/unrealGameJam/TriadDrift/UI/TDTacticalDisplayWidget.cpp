// Triad Drift - Aftermath tactical display (Teammate 2).

#include "TriadDrift/UI/TDTacticalDisplayWidget.h"

#include "TriadDrift/UI/TDFlightHUDWidget.h"
#include "TriadDrift/UI/TDUIPaint.h"

namespace TDTactical
{
	using T = FTDFlightHUDTokens;

	/** A point on lane L at progress P (0 = the Dominion, 1 = Verdance): gentle parallel arcs. */
	FVector2D LanePoint(int32 L, float P)
	{
		const FBox2D Box = UTDTacticalDisplayWidget::PanelBox();
		const FVector2D From(Box.Min.X + 130.f, Box.GetCenter().Y);
		const FVector2D To(Box.Max.X - 120.f, Box.GetCenter().Y - 20.f);
		const float Spread = (L - (UTDTacticalDisplayWidget::Lanes - 1) * .5f) * 34.f;
		const FVector2D Straight = FMath::Lerp(From, To, P);
		// Lanes fan out from the ship and converge on the planet: a sine bulge, 0 at both ends.
		return Straight + FVector2D(0.f, Spread * FMath::Sin(P * PI) * 1.6f);
	}
}

void UTDTacticalDisplayWidget::Play(float Seconds)
{
	Age = 0.f;
	Duration = FMath::Max(1.f, Seconds);
}

float UTDTacticalDisplayWidget::GetVisibility01() const
{
	if (Duration <= 0.f || Age >= Duration)
	{
		return 0.f;
	}
	return FMath::Min(FMath::Clamp(Age / FadeSeconds, 0.f, 1.f), FMath::Clamp((Duration - Age) / FadeSeconds, 0.f, 1.f));
}

float UTDTacticalDisplayWidget::LaneProgress(int32 Lane, int32 Index, float T)
{
	// Evenly spaced on each lane, lanes slightly staggered: disciplined, not random.
	const float Phase = (float)Index / ContactsPerLane + Lane * .07f;
	return FMath::Fmod(T / LaneSeconds + Phase, 1.f);
}

void UTDTacticalDisplayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
}

int32 UTDTacticalDisplayWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace TDTactical;
	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const float Vis = GetVisibility01();
	if (Vis <= 0.f)
	{
		return Base;
	}
	FTDUIPainter P(AllottedGeometry, OutDrawElements, Base + 1);
	P.Alpha = Vis;
	const FBox2D Box = PanelBox();
	const FVector2D Size = Box.GetSize();

	// Panel: the HUD's plate, a faint grid, a thin frame.
	P.Box(Box.Min.X, Box.Min.Y, Size.X, Size.Y, T::Plate().CopyWithNewOpacity(.82f));
	for (float X = Box.Min.X + 40.f; X < Box.Max.X; X += 40.f)
	{
		P.Line({ X, Box.Min.Y + 34.f }, { X, Box.Max.Y - 8.f }, T::Dim().CopyWithNewOpacity(.07f), 1.f);
	}
	for (float Y = Box.Min.Y + 54.f; Y < Box.Max.Y; Y += 40.f)
	{
		P.Line({ Box.Min.X + 8.f, Y }, { Box.Max.X - 8.f, Y }, T::Dim().CopyWithNewOpacity(.07f), 1.f);
	}
	P.Frame(Box.Min.X, Box.Min.Y, Size.X, Size.Y, T::Neutral(), .25f);
	P.Label(Box.Min.X + 18.f, Box.Min.Y + 10.f, TEXT("TACTICAL  ·  CONTACT MOVEMENT"), 14, T::Neutral().CopyWithNewOpacity(.85f), 120);
	// A scan line sweeping left to right (sensor display, not a cutscene card).
	const float Sweep = FMath::Fmod(Age * .35f, 1.f);
	const float SweepX = FMath::Lerp(Box.Min.X + 8.f, Box.Max.X - 8.f, Sweep);
	P.Line({ SweepX, Box.Min.Y + 34.f }, { SweepX, Box.Max.Y - 8.f }, T::Shield().CopyWithNewOpacity(.18f), 2.f);

	// The Dominion (left): a large hexagon in Armada red with a thin ring (the distortion field).
	const FVector2D Dom = LanePoint(0, 0.f) + FVector2D(0.f, 0.f);
	{
		TArray<FVector2D> Hex;
		for (int32 I = 0; I <= 6; ++I)
		{
			const float A = I * PI / 3.f;
			Hex.Add(Dom + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 34.f);
		}
		for (int32 I = 0; I < 6; ++I)
		{
			P.Line(Hex[I], Hex[I + 1], T::Danger(), 2.5f);
		}
		P.Arc(Dom, 52.f, 1.f, T::Danger().CopyWithNewOpacity(.3f), 1.f);
		P.Label(Dom.X - 40.f, Dom.Y + 60.f, TEXT("DOMINION"), 13, T::Danger(), 100);
	}
	// Verdance (right): a teal disc outline with a ring.
	const FVector2D Ver = LanePoint(0, 1.f);
	P.Arc(Ver, 30.f, 1.f, T::Lancer(), 2.5f);
	P.Arc(Ver, 44.f, 1.f, T::Lancer().CopyWithNewOpacity(.35f), 1.f);
	P.Label(Ver.X - 38.f, Ver.Y + 52.f, TEXT("VERDANCE"), 13, T::Lancer(), 100);

	// Lanes: thin dashed paths, and contacts on them, evenly spaced, all moving the same way.
	for (int32 L = 0; L < Lanes; ++L)
	{
		for (int32 S = 0; S < 40; S += 2)
		{
			P.Line(LanePoint(L, S / 40.f), LanePoint(L, (S + 1) / 40.f), T::Danger().CopyWithNewOpacity(.22f), 1.f);
		}
		for (int32 I = 0; I < ContactsPerLane; ++I)
		{
			const float Prog = LaneProgress(L, I, Age);
			const FVector2D At = LanePoint(L, Prog);
			const FVector2D Ahead = (LanePoint(L, FMath::Min(Prog + .01f, 1.f)) - At).GetSafeNormal();
			const FVector2D Side(-Ahead.Y, Ahead.X);
			// Small hostile arrowheads pointing along the lane; fading in near the ship and out near the planet.
			const float O = FMath::Min(FMath::Clamp(Prog / .08f, 0.f, 1.f), FMath::Clamp((1.f - Prog) / .08f, 0.f, 1.f));
			const FLinearColor C = T::Danger().CopyWithNewOpacity(O);
			P.Line(At + Ahead * 6.f, At - Ahead * 5.f + Side * 4.f, C, 2.f);
			P.Line(At + Ahead * 6.f, At - Ahead * 5.f - Side * 4.f, C, 2.f);
		}
	}

	// Triad (below the lanes), with a few contacts loitering nearby that never turn toward them.
	const FVector2D Triad(Box.GetCenter().X - 60.f, Box.Max.Y - 70.f);
	for (int32 I = 0; I < 3; ++I)
	{
		const FLinearColor Squad = I == 0 ? T::Rift() : I == 1 ? T::Lancer() : T::Bastion();
		P.Box(Triad.X + I * 14.f - 3.f, Triad.Y - 3.f, 6.f, 6.f, Squad);
	}
	P.Label(Triad.X - 6.f, Triad.Y + 10.f, TEXT("TRIAD"), 12, T::Neutral().CopyWithNewOpacity(.8f), 120);
	for (int32 I = 0; I < Idle; ++I)
	{
		const FVector2D Loiter = Triad + FVector2D(110.f + I * 46.f, -28.f + 14.f * FMath::Sin(Age * .8f + I * 1.7f));
		P.Arc(Loiter, 4.f, 1.f, T::Danger().CopyWithNewOpacity(.55f), 1.5f);
	}
	return Base + 3;
}
