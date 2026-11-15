// Triad Drift - Aftermath tactical display (Teammate 2). storyline.md section 12, 0:20-0:45: hostile contacts leaving
// the Dominion in disciplined lanes toward Verdance; some never targeted Triad.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDTacticalDisplayWidget.generated.h"

/**
 * A simple animated 2D schematic in the flight HUD's tokens, shown by the director for about 20 s during the Aftermath
 * sequence (UTDUIHostSubsystem::ShowTacticalDisplay). Lives outside the flight HUD so a cinematic that hides the HUD
 * still shows it. Painted in code like the flight HUD (no art needed); every element is in a 1920x1080 design frame.
 */
UCLASS()
class UNREALGAMEJAM_API UTDTacticalDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Contacts per lane and the number of lanes (disciplined: evenly spaced, same speed). */
	static constexpr int32 Lanes = 4;
	static constexpr int32 ContactsPerLane = 6;
	/** Contacts loitering near the squad without engaging ("some never targeted Triad"). */
	static constexpr int32 Idle = 3;
	/** Seconds for a contact to travel the lane. */
	static constexpr float LaneSeconds = 9.f;
	static constexpr float FadeSeconds = .5f;
	/** Ryan (Nov 14): it "stayed on the screen for too long, I couldn't remove it". At most 8 s however long a caller asks,
	 *  and Dismiss() closes it with a 0.2 s fade. */
	static constexpr float MaxSeconds = 8.f;
	static constexpr float DismissFadeSeconds = .2f;
	/** Docked upper-right under the objective line at 60% of its first size, clear of the reticle, radar and squad row. */
	static constexpr float DockScale = .6f;

	/** Starts (or restarts) the display for Seconds (clamped to MaxSeconds); it fades in and out by itself. */
	void Play(float Seconds);
	/** Closes it now with a short fade (any action key, a hyperdrive spool, a new objective). */
	void Dismiss();
	bool IsDismissing() const { return bDismissing; }
	float GetAge() const { return Age; }
	/** The keycap shown in the "[key] DISMISS" hint. */
	FString DismissKey = TEXT("X");
	bool IsFinished() const { return Age >= Duration; }
	/** 0..1 visibility (fade in / hold / fade out). */
	float GetVisibility01() const;
	/** Contact position along its lane at time T (0..1, wraps): what the paint and the tests use. */
	static float LaneProgress(int32 Lane, int32 Index, float T);
	/** The panel as drawn, in the 1920x1080 design frame: docked upper-right (TD.UI.Layout, tests). */
	static FBox2D PanelBox() { return FBox2D(FVector2D(1872.f - 800.f * DockScale, 112.f), FVector2D(1872.f, 112.f + 500.f * DockScale)); }
	/** The schematic's own drawing frame (800x500), scaled into PanelBox when painted. */
	static FBox2D DrawBox() { return FBox2D(FVector2D(560.f, 250.f), FVector2D(1360.f, 750.f)); }

	/** Tests: advance without a tick. */
	void Advance(float Dt) { Age += Dt; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float Age = 0.f;
	float Duration = 0.f;
	float OutFade = FadeSeconds;
	bool bDismissing = false;
};
