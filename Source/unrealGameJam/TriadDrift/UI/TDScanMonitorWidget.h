// Triad Drift - the scan monitor graphic for Cinematic 03 and Cinematic 05 (Teammate 2). storyline.md: Kato projects
// layered subsurface scans of Verdance (a large, geometrically precise buried structure; Dominion scan sites aligned on
// its outline; a correlated coordinate at Cinder); on Cinder the same display overlays a partial Cinder pattern that
// agrees at several angles, then marks the crystal cache and the field anchor.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDScanMonitorWidget.generated.h"

/**
 * Stages, each adding to the one before (call ShowScanMonitor again with a higher stage to advance; new layers draw in
 * over ~1.5 s):
 *   1 Verdance: strata and the buried structure, traced in.
 *   2 + the Dominion scan sites on its outline.
 *   3 + "CORRELATED  /  CINDER" (Cinematic 03's last beat).
 *   4 Cinder overlay: the partial Cinder pattern over the Verdance outline, matching angles marked (Cinematic 05).
 *   5 + the local map markers: CRYSTAL CACHE and FIELD ANCHOR.
 * Labels are instrument words only (no story lines: those are Nicholas's, spoken over it). Nothing in the shape reads as
 * the twist. Painted in the flight HUD's tokens, no art; a 1920x1080 design frame; its own widget, so it shows while a
 * cinematic hides the HUD.
 */
UCLASS()
class UNREALGAMEJAM_API UTDScanMonitorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxStage = 5;
	static constexpr float RevealSeconds = 1.5f;
	static constexpr float FadeSeconds = .4f;

	/** Show at Stage (1..5) for Seconds (0 = until Hide). A higher stage than the current one reveals the new layers. */
	void Show(int32 Stage, float Seconds);
	/** Fade out now. */
	void Hide();
	bool IsFinished() const { return bHiding && HideAge >= FadeSeconds; }
	int32 GetStage() const { return Stage; }
	float GetVisibility01() const;
	/** 0..1 how far layer L (1..5) has drawn in. */
	float GetLayer01(int32 Layer) const;

	/** The monitor panel in the design frame (tests). */
	static FBox2D PanelBox() { return FBox2D(FVector2D(420.f, 170.f), FVector2D(1500.f, 830.f)); }
	/** The buried structure's outline vertices in the design frame (tests: markers sit on it). */
	static TArray<FVector2D> StructureOutline();

	/** Tests: advance without a tick. */
	void Advance(float Dt);

	/** Named boxes painted last frame (tests). */
	mutable TArray<TPair<FName, FBox2D>> LastBoxes;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	int32 Stage = 0;
	float Age = 0.f;
	float Duration = 0.f;
	bool bHiding = false;
	float HideAge = 0.f;
	/** When each layer started drawing in (Age). */
	float LayerStart[MaxStage + 1] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
};
