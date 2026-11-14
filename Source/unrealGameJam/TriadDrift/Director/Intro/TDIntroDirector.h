// Triad Drift - ENG-33 the cinematic intro (Director's Cut). Owner: Teammate 3.
//
// About 48 s of real-time shots in L_Galaxy with the game's own assets, played once on New Game (T5's flow calls Play
// and starts the waves from OnFinished). Seven beats: the nebula from black, Verdance in eclipse, the squad passing
// close with its pilot name cards, threading an asteroid field, Cinder beyond a portal gate, the Null Armada warping
// in with the Dominion's silhouette, then the TRIAD DRIFT title and a 1 s blend into the chase camera.
//
// Look: 2.39:1 bars, filmback-true focal lengths (Super 35 width), depth of field on the subject, motion blur,
// bloom, lens flare and light grain on the shot camera's post-process; handheld noise on close shots only.
// Narration: short subtitle lines in the director's voice, with an optional voice cue per line
// (UTDUIStyle::IntroNarrationVoice) and the intro music (UTDUIStyle::IntroMusic).
//
// Rules: skippable after 1 s by holding any key, mouse button or pad button for 0.5 s ("HOLD TO SKIP"); every
// other input is blocked while it plays and only while it plays; the HUD is hidden; the squad is frozen (AI, flight,
// collision) and put back exactly where it was, so the run is identical watched or skipped. Nothing plays when
// td.Intro is 0 or there is no local player with a viewport.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "InputCoreTypes.h"
#include "TDIntroDirector.generated.h"

class ACameraActor;
class APlayerController;
class AActor;
class ATDShip;
class UInputComponent;
class SWidget;
class UAudioComponent;
class ULevelSequencePlayer;
class ULevelSequence;
class ALevelSequenceActor;

/** One beat of the intro: a camera move, a lens, what is in focus, and what happens in it. */
USTRUCT()
struct FTDIntroShot
{
	GENERATED_BODY()

	FName Id;
	float Duration = 6.f;
	FVector CamFrom = FVector::ZeroVector;
	FVector CamTo = FVector::ZeroVector;
	/** Where the lens looks (start / end) when there is no tracked subject. */
	FVector LookFrom = FVector::ForwardVector;
	FVector LookTo = FVector::ForwardVector;
	/** Focal length (mm), start / end. */
	float FocalFrom = 35.f;
	float FocalTo = 35.f;
	float FStop = 4.f;
	/** Seconds of fade from / to black at the start and end. */
	float FadeIn = 0.6f;
	float FadeOut = 0.4f;
	/** Handheld noise amplitude (cm), close shots only. */
	float Handheld = 0.f;
	/** Narration line index shown during this shot, or -1. */
	int32 Narration = -1;
	/** The squad flies from SquadFrom to SquadTo (formation centre) facing SquadFacing. */
	bool bSquad = false;
	FVector SquadFrom = FVector::ZeroVector;
	FVector SquadTo = FVector::ZeroVector;
	/** Track the squad (look at it, focus on it) instead of LookFrom / LookTo. */
	bool bTrackSquad = false;
	/** The shot's landmark (the gate the squad flies through, the cargo cluster it passes). */
	FVector Anchor = FVector::ZeroVector;
	/** Distance the focus is held at when nothing is tracked (cm); 0 = infinity-ish. */
	float FocusDistance = 0.f;
};

/** Where the intro's overlay puts the pilot card, and the regions it must stay clear of (issue #49). Pixels in the
 *  viewport's local space; computed from the measured text, so a long trait line or a narrow or ultrawide screen
 *  scales the card down instead of clipping it. */
struct FTDIntroCardLayout
{
	FBox2D Name = FBox2D(ForceInit);
	FBox2D Role = FBox2D(ForceInit);
	FBox2D Trait = FBox2D(ForceInit);
	FBox2D Insignia = FBox2D(ForceInit);
	FBox2D Bar = FBox2D(ForceInit);          // the thin accent between the insignia and the text
	FBox2D SubtitleBand = FBox2D(ForceInit); // reserved for the narration line (full width)
	FBox2D Skip = FBox2D(ForceInit);         // HOLD TO SKIP and its fill bar
	FBox2D Picture = FBox2D(ForceInit);      // the frame inside the 2.39:1 bars
	int32 NameFontSize = 0;
	int32 RoleFontSize = 0;
	int32 TraitFontSize = 0;
};

/** What the overlay draws this frame (tests read it). */
USTRUCT()
struct FTDIntroOverlayState
{
	GENERATED_BODY()

	float Black = 1.f;
	/** 0..1: how far the 2.39:1 bars are in (1 = full bars). */
	float Bars = 1.f;
	FText Subtitle;
	float SubtitleAlpha = 0.f;
	float TitleAlpha = 0.f;
	FText NameCard;
	FText NameCardRole;   // "RIFT  ·  TRICKSTER"
	FText NameCardTrait;  // "BOLD · INSTINCTIVE" (Trait_*)
	int32 NameCardShip = -1; // ETDShipId: which insignia
	FLinearColor NameCardColor = FLinearColor::White;
	float NameCardAlpha = 0.f;
	float SkipHintAlpha = 0.f;
	float SkipProgress = 0.f;
};

/** The player has control (N-039): bPlayed = the intro ran (watched or skipped); false = the run started without it. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnIntroFinished, bool, bPlayed, bool, bSkipped);

UCLASS()
class UNREALGAMEJAM_API UTDIntroDirector : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDIntroDirector* Get(const UObject* WorldContext);

	/**
	 * Play the intro now (T5's New Game flow). False, and OnFinished never fires, when there is nothing to play
	 * (td.Intro 0, no local player or viewport, already playing). OnFinished fires exactly once: at the end of the
	 * hand-off blend, or right after a skip.
	 */
	bool Play(FSimpleDelegate OnFinished);

	/** Tests: play even without a game viewport (no overlay widget). */
	bool PlayForTests(FSimpleDelegate OnFinished)
	{
		bAllowWithoutViewport = true;
		const bool bPlayed = Play(MoveTemp(OnFinished));
		bAllowWithoutViewport = false;
		return bPlayed;
	}

	bool IsPlaying() const { return bPlaying; }
	/** The last second: the lens eases into the chase camera (the scene slate shows now). */
	bool IsHandingOff() const { return bPlaying && bHandingOff; }
	/** A button that may skip (not an axis, a virtual stick button or the mouse wheel). */
	static bool IsSkipKey(const FKey& Key);
	/** True while presentation's Level Sequence plays instead of the code shots. */
	bool IsPlayingSequence() const { return bPlaying && bSequenceMode; }
	/** Tests: play this sequence as if UTDUIStyle::IntroSequence pointed at it. */
	void SetSequenceOverrideForTests(ULevelSequence* Sequence);
	/** Tests: let Play run in a test world with no game viewport (other systems' tests that go through Play). */
	void SetAllowWithoutViewportForTests(bool bAllow) { bAllowWithoutViewportAlways = bAllow; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Intro", meta = (WorldContext = "WorldContext"))
	static bool IsIntroPlaying(const UObject* WorldContext);

	/**
	 * N-039 (the music director): fired on the frame the player gains control. After the hand-off when the intro is
	 * watched, at once on a skip (bSkipped), and from NotifyRunStartedWithoutIntro when the galaxy is entered without
	 * it (Continue, a retake level, td.Intro 0): bPlayed false. Fires once per entry.
	 */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Intro")
	FTDOnIntroFinished OnIntroFinished;

	/** Seconds from the intro's first frame to control: the shots plus the 1 s hand-off (49.0 today). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Intro")
	/** T1, issue #24: this intro did not start IntroMusic because the tutorial music owns the opening. */
	bool WasIntroMusicSuppressed() const { return bIntroMusicSuppressed; }
	float GetIntroLength() const { return GetTotalDuration() + HandOffSeconds; }

	/** Seconds since the intro's first frame, through the hand-off (0 when not playing). A music prelude of L seconds
	 *  starts at GetIntroLength() - L to land its end on the control frame. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Intro")
	float GetIntroTime() const { return bPlaying ? (bHandingOff ? GetTotalDuration() + HandOffElapsed : Elapsed) : 0.f; }

	/** The space game mode calls this when it enters the galaxy without playing the intro: OnIntroFinished(false, false). */
	void NotifyRunStartedWithoutIntro();

	/** Skip now (the hold-to-skip input calls this; also the debug command). Ignored in the first second. */
	bool Skip();

	// ---- State (tests, tools) ----
	float GetElapsed() const { return Elapsed; }
	float GetTotalDuration() const;
	int32 GetShotIndex() const { return ShotIndex; }
	const TArray<FTDIntroShot>& GetShots() const { return Shots; }
	const FTDIntroOverlayState& GetOverlay() const { return Overlay; }
	ACameraActor* GetShotCamera() const { return Camera; }
	bool WasSkipped() const { return bSkipped; }
	/** The narration lines, in order (localisable). */
	static const TArray<FText>& NarrationLines();

	/** Lays out the pilot card for a viewport (issue #49). Measure returns a text's size in a font (the Slate font
	 *  service in the game; tests may pass an estimate when Slate has no renderer). */
	static FTDIntroCardLayout LayoutNameCard(const FVector2D& ViewportSize, const FText& Name, const FText& Role, const FText& Trait,
		TFunctionRef<FVector2D(const FText&, const struct FSlateFontInfo&)> Measure);
	/** Concept A's squad colours (the HUD palette, sRGB): Rift #F18CFA, Lancer #79E5FF, Bastion #FFCE77. */
	static FLinearColor CardColor(int32 Ship);
	/** The opening's Squad shot is three steady frames, one per ship (issue #49): which one at T, and its window. */
	static int32 SquadSegment(float T, float ShotDuration);
	/** This play's narration (story table Intro_N*, else NarrationLines()). */
	TArray<FText> Narration;
	/** Jump to a shot (captures, debug): restarts that shot from its first frame. */
	void DebugJumpToShot(int32 Index);
	/** Captures: replay the intro's shot up to time T (s from the start) in 1/30 s steps; bFreeze holds it there. */
	void DebugSetTime(float T, bool bFreeze);
	bool IsDebugFrozen() const { return bDebugFrozen; }

	/** Seconds after start before a skip is accepted, and how long the skip key must be held. */
	/** Seconds before the intro can be skipped (Nicholas, N-060: ten for every cutscene). td.Intro.SkipLockSeconds;
	 *  automation runs use 1 s unless a test sets the cvar (other suites skip early). */
	static float GetSkipLockSeconds();
	static constexpr float SkipHoldSeconds = 0.5f;
	static constexpr float HandOffSeconds = 1.f;

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDIntroDirector, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return bPlaying; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual void Deinitialize() override;

private:
	void BuildShots();
	/** The code-driven shots: the shot camera, the view, shot 1. */
	void StartCodeShots();
	void BeginShot(int32 Index);
	void UpdateShot(float ShotTime);
	void UpdateOverlay(float ShotTime);
	void UpdateSkip(float RealDelta);
	void BeginHandOff();
	void Finish();
	void HoldSquad(bool bHold);
	void PlaceSquad(const FVector& Centre, const FRotator& Facing, float Time);
	void SpawnShowcase();
	void ClearShowcase();
	void SetKeyLight(const FVector& At, float Radius, float Intensity);
	void SpawnDominionStandIn(const FTransform& At);
public:
	/** The Dominion's art (DA_Dominion_Visuals: hull, bridge, trench walls) on a transient visual-only actor. */
	static AActor* SpawnDominionVisual(UWorld* World, const FTransform& At);
private:
public:
	/** The Dominion hull's radius (cm) from its art, for framing. */
	static float DominionRadius();
private:
	void RestoreGateSurface();
	/** Put back what one shot hid (the eclipse's fill light, the gates beside Verdance). */
	void RestoreShotHidden();
	static float EclipseExposureEV100();
	void ApplyLens(float FocalMm, float FStop, float FocusDistance);
	void PlayNarrationVoice(int32 Line);
	void EnsureWidget();
	void RemoveWidget();
	void HandleSkipPressed(FKey Key);
	void HandleSkipReleased(FKey Key);
	APlayerController* GetPlayer() const;
	FVector SunDirection() const;

	FSimpleDelegate OnFinishedDelegate;
	TArray<FTDIntroShot> Shots;
	FTDIntroOverlayState Overlay;

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TObjectPtr<UInputComponent> SkipInput;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Showcase;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
	bool bIntroMusicSuppressed = false; // T1, issue #24: the tutorial music owns the opening
	/** Shot 6: a red-orange key light on the fleet (it would read black against space otherwise). */
	UPROPERTY(Transient) TObjectPtr<AActor> KeyLight;
	/** Shot 6 until T4's ATDDominion::SetShowcase: the Dominion's art (DA_Dominion_Visuals) on a transient actor. */
	UPROPERTY(Transient) TObjectPtr<AActor> DominionStandIn;
	/** Shot 5: the gate's membrane, hidden so the ring frames Cinder instead of covering it. */
	TWeakObjectPtr<class UPrimitiveComponent> HiddenSurface;
	TArray<TWeakObjectPtr<class UDirectionalLightComponent>> ShotHiddenLights;
	TArray<TWeakObjectPtr<AActor>> ShotHiddenActors;
	UPROPERTY(Transient) TObjectPtr<ULevelSequencePlayer> SequencePlayer;
	UPROPERTY(Transient) TObjectPtr<ALevelSequenceActor> SequenceActor;
	TWeakObjectPtr<APlayerController> Player;
	TSharedPtr<SWidget> Widget;

	/** The squad as it was before the intro: put back exactly. */
	struct FHeldShip
	{
		TWeakObjectPtr<ATDShip> Ship;
		FTransform Transform;
		FVector Velocity = FVector::ZeroVector;
		bool bCollision = true;
		bool bMovementTick = true;
		bool bControllerTick = true;
		bool bHidden = false;
	};
	TArray<FHeldShip> Held;

	bool bPlaying = false;
	bool bAllowWithoutViewport = false;
	bool bAllowWithoutViewportAlways = false;
	bool bHandingOff = false;
	bool bSkipped = false;
	bool bSkipHeld = false;
	TSet<FKey> SkipKeysHeld;
	bool bSkipConsumed = false;
	float SkipHeldFor = 0.f;
	float Elapsed = 0.f;
	float HandOffElapsed = 0.f;
	int32 ShotIndex = -1;
	float ShotStart = 0.f;
	int32 LastNarrationVoice = -1;
	FVector LastLook = FVector::ForwardVector;
	float LastDt = 1.f / 60.f;
	/** Shot 6: fighters spawned yet. */
	bool bFleetSpawned = false;
	bool bGateFlared = false;
	bool bDominionShown = false;
	bool bDebugFrozen = false;
	/** Playing presentation's Level Sequence instead of the code shots (it owns framing: no bars). */
	bool bSequenceMode = false;
	bool bScreenMessagesHidden = false;
	bool bWasScreenMessages = true;
	bool bWasMapWarnings = false;
	bool bSequenceStarted = false;
	TWeakObjectPtr<ULevelSequence> SequenceOverrideForTests;
	/** Shot 6b: close fighters spawned so far (one every 0.45 s). */
	int32 CloseSpawned = 0;
};
