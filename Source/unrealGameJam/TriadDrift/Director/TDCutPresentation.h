// Triad Drift - Director (Teammate 3). C-03 #1 / #2 / #3 fallback presentation.
//
// The theme must read in every build, even before Nick's HUD exists. This is a gameplay-side
// fallback that draws ONLY when no real HUD claims it (UTDDirectorSubsystem::
// bCutPresentationHandledExternally): on a ship switch a 1-frame white flash, letterbox bars and the
// pilot name card in the squad colour; on "CUT!" a big CUT! card; on a new take a slate; during a
// money shot thin letterbox bars. A Slate overlay on the game viewport (works with any HUD class and
// in Shipping), engine fonts only, real-time animation (slow-mo never stretches it), hit-test
// invisible (never blocks input), removed with the world (travel).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriadDrift/Squad/TDSwitchCut.h"
#include "TDCutPresentation.generated.h"

class SWidget;
class ATDSquadManager;

/** What the overlay draws this frame (0 alpha / amount = nothing). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDCutPresentationState
{
	GENERATED_BODY()

	/** Full-screen white flash. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	float FlashAlpha = 0.f;

	/** Letterbox bar height as a fraction of the screen height (each bar). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	float Letterbox = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText NameCardText;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FLinearColor NameCardColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	float NameCardAlpha = 0.f;

	/** "CUT!" */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText CutCardText;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	float CutCardAlpha = 0.f;

	/** "SCENE Wave 3 · TAKE 2" */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText SlateText;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	float SlateAlpha = 0.f;

	bool IsEmpty() const
	{
		return FlashAlpha <= 0.f && Letterbox <= 0.f && NameCardAlpha <= 0.f && CutCardAlpha <= 0.f && SlateAlpha <= 0.f;
	}
};

/** Timing for the fallback cards (switch timing comes from FTDSwitchCut::Timing). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDCutPresentationTuning
{
	GENERATED_BODY()

	/** Letterbox bar height on a switch and on "CUT!" (fraction of screen height per bar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut")
	float SwitchLetterbox = 0.11f;

	/** Thin bars during a money shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut")
	float ShotLetterbox = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (Units = "s"))
	float ShotLetterboxEaseSeconds = 0.15f;

	/** "CUT!" stays at least this long (or the flow's retake delay if longer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (Units = "s"))
	float CutCardMinSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (Units = "s"))
	float SlateSeconds = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut", meta = (Units = "s"))
	float CardFadeSeconds = 0.25f;
};

/**
 * The presentation model. Owned and ticked (real time) by UTDDirectorSubsystem; the Slate overlay
 * only reads GetState(). All public entry points are safe to call in any order.
 */
UCLASS()
class UNREALGAMEJAM_API UTDCutPresentation : public UObject
{
	GENERATED_BODY()

public:
	void Init(UWorld* InWorld);
	void Shutdown();

	/** Real-time update: recomputes the state, rebinds to the squad manager if it changed. */
	void Tick(double RealNow);

	UFUNCTION()
	void HandleSwitchCut(const FTDSwitchCut& Cut);

	void HandleSceneCut(float DelaySeconds);
	void HandleTakeStarted(FName SceneId, int32 TakeNumber);
	void HandleShotStarted(float RealSeconds);

	/** Clear everything at once (travel, external HUD taking over). */
	void ClearAll();

	/** True while a real HUD draws these; the fallback then draws nothing. */
	void SetHandledExternally(bool bHandled);
	bool IsHandledExternally() const { return bHandledExternally; }

	const FTDCutPresentationState& GetState() const { return State; }
	bool HasViewportWidget() const { return Widget.IsValid(); }

	/** "Wave3" -> "Wave 3" (scene ids are CamelCase + number). */
	static FText MakeSlateText(FName SceneId, int32 TakeNumber);

	/** Where the scene slate is drawn on a viewport of Size (px). Centred, in the upper third, below
	 *  the top 15% (Nick's OBJECTIVE block, the real HUD's top row) and below the dev HUD's squad rows
	 *  (12-21% on the left); never under a letterbox bar. */
	static FBox2D SlateRect(const FVector2D& Size, float Letterbox);

	/** Viewport layer of the overlay: just under the real HUD while it is up, else 900. */
	static int32 OverlayZOrder(bool bRealHUDActive);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Cut")
	FTDCutPresentationTuning Tuning;

private:
	double Now() const;
	void EnsureViewportWidget();
	void RemoveViewportWidget();
	void BindSquad();

	TWeakObjectPtr<UWorld> OwnerWorld;
	TWeakObjectPtr<ATDSquadManager> BoundSquad;
	TSharedPtr<SWidget> Widget;
	int32 WidgetZOrder = 900;

	FTDCutPresentationState State;
	bool bHandledExternally = false;

	// switch
	double SwitchStart = -1000.0;
	int32 FlashFramesLeft = 0;
	FTDSwitchCutTiming SwitchTiming;
	bool bQuickCut = false;
	FText NameCard;
	FLinearColor NameColor = FLinearColor::White;
	// CUT!
	double CutStart = -1000.0;
	float CutSeconds = 0.f;
	// slate
	double SlateStart = -1000.0;
	FText Slate;
	// money shot
	double ShotStart = -1000.0;
	float ShotSeconds = 0.f;
};
