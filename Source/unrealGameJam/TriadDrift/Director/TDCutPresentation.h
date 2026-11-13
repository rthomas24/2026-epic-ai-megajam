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

	/** "SCENE 3 · WAVE 3 · TAKE 2" */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	FText SlateText;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Cut")
	float SlateAlpha = 0.f;

	bool IsEmpty() const
	{
		return FlashAlpha <= 0.f && Letterbox <= 0.f && NameCardAlpha <= 0.f && CutCardAlpha <= 0.f && SlateAlpha <= 0.f;
	}
};

/** One Director's Cut card for a renderer (N-038): what it says, how far through it is, and how long it has left. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDirectorCard
{
	GENERATED_BODY()

	/** Up this frame (its alpha may still be fading). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FText Text;

	/** Second line where there is one: the pilot card's "LANCER · INTERCEPTOR", the money shot's kind. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FText SubText;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FLinearColor Color = FLinearColor::White;

	/** The fallback's own fade (0..1): a renderer may use it or animate from Progress01. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float Alpha = 0.f;

	/** 0 at the start of the card, 1 at its end (real time: slow motion never stretches it). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float Progress01 = 0.f;

	/** Real seconds until it ends. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float TimeLeft = 0.f;
};

/** Everything the Director's Cut shows right now, for a renderer widget (N-038). Read-only; real time. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDirectorCards
{
	GENERATED_BODY()

	/** The ship switch as a camera cut: Text = the new pilot, Color = the squad colour; bQuickCut = no bars. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FTDDirectorCard SwitchCut;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	bool bQuickCut = false;

	/** Counts every switch (a renderer can restart its animation when it changes). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	int32 SwitchCount = 0;

	/** The pilot name card (with "STRIKE +25%" when the switch earned Switch Strike). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FTDDirectorCard NameCard;

	/** "SCENE 3 · WAVE 3 · TAKE 2". */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FTDDirectorCard Slate;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FName SlateScene;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	int32 SlateTake = 0;

	/** "CUT!" (the scene failed; the retake follows). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FTDDirectorCard Cut;

	/** A money shot's slow motion and thin letterbox; SubText = its kind ("RamKill"). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FTDDirectorCard MoneyShot;

	/** The fallback's letterbox height per bar (fraction of the screen) and its 1-frame white flash. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float Letterbox = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float FlashAlpha = 0.f;

	bool AnyActive() const { return SwitchCut.bActive || NameCard.bActive || Slate.bActive || Cut.bActive || MoneyShot.bActive; }
	/** Changes only when a card comes, goes or changes its text (what OnDirectorCardsChanged reports). */
	uint32 Signature() const;
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
	float SlateSeconds = 1.3f; // + fade: gone by the time a wave's warp-in (1.5 s) ends and enemies arrive

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

	/** N-038: a renderer widget draws the cards. Timing and GetCards() stay alive; only the fallback overlay yields. */
	void SetExternalRenderer(bool bRenderer) { bExternalRenderer = bRenderer; }
	bool HasExternalRenderer() const { return bExternalRenderer; }
	const FTDDirectorCards& GetCards() const { return Cards; }
	/** Fired from Tick when the cards' Signature() changes. */
	TMulticastDelegate<void(const FTDDirectorCards&)> OnCardsChanged;
	/** The running money shot's kind, for the card (set by the director). */
	void SetShotKindName(const FText& Kind) { ShotKindName = Kind; }

	/** True while a real HUD draws these; the fallback then draws nothing (and the cards are cleared too). */
	void SetHandledExternally(bool bHandled);
	bool IsHandledExternally() const { return bHandledExternally; }
	/** True while the ENG-33 intro plays: the intro owns the screen, nothing of ours draws over it. */
	bool IsSuppressedByIntro() const;
	/** ENG-33: scene one's slate at the intro's hand-off; the real first take then does not repeat it. */
	void PreviewFirstSlate(FName SceneId);

	const FTDCutPresentationState& GetState() const { return State; }
	bool HasViewportWidget() const { return Widget.IsValid(); }

	/** "Wave3" -> "Wave 3" (scene ids are CamelCase + number). */
	/** features.md C-03 #3: "SCENE 3 · CINDER · OUTPOST · TAKE 1". SceneNumber 0 (unknown) drops the number;
	 *  Place is the planet for an objective scene (empty in the galaxy). */
	static FText MakeSlateText(FName SceneId, int32 TakeNumber, int32 SceneNumber = 0, const FString& Place = FString());

	/** Where the scene slate is drawn on a viewport of Size (px). Centred, in the upper third, below
	 *  the top 15% (Nick's OBJECTIVE block, the real HUD's top row) and below the dev HUD's squad rows
	 *  (12-21% on the left); never under a letterbox bar. */
	static FBox2D SlateRect(const FVector2D& Size, float Letterbox, float TextWidth = 0.f);

	/** Viewport layer of the overlay: just under the real HUD while it is up, else 900. */
	static int32 OverlayZOrder(bool bRealHUDActive);

	/** True when the hosted space HUD draws its own pilot card: Nick's (N-007), or any widget whose bCoversNameCard is
	 *  true. Engineering's flight HUD (the default since ENG-36) doesn't, so the fallback card shows under it. */
	static bool IsNameCardClaimedByHUD(const UWorld* World);

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
	FTDDirectorCards Cards;
	uint32 CardsSignature = 0;
	bool bHandledExternally = false;
	bool bExternalRenderer = false;
	int32 SwitchCount = 0;
	FText NameCardSub;
	FText ShotKindName;

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
	/** A slate that came while the intro owned the screen: shown when it ends (never lost). */
	FName PendingSlateScene;
	int32 PendingSlateTake = 0;
	double PreviewSlateAt = -1000.0;
	FText Slate;
	FName SlateScene;
	int32 SlateTake = 0;
	// money shot
	double ShotStart = -1000.0;
	float ShotSeconds = 0.f;
};
