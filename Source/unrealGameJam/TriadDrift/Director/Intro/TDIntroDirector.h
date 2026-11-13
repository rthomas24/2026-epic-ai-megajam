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
#include "TDIntroDirector.generated.h"

class ACameraActor;
class APlayerController;
class AActor;
class ATDShip;
class UInputComponent;
class SWidget;
class UAudioComponent;
class ULevelSequencePlayer;
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
	/** Distance the focus is held at when nothing is tracked (cm); 0 = infinity-ish. */
	float FocusDistance = 0.f;
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
	FText NameCardRole;
	FLinearColor NameCardColor = FLinearColor::White;
	float NameCardAlpha = 0.f;
	float SkipHintAlpha = 0.f;
	float SkipProgress = 0.f;
};

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

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Intro", meta = (WorldContext = "WorldContext"))
	static bool IsIntroPlaying(const UObject* WorldContext);

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
	/** Jump to a shot (captures, debug): restarts that shot from its first frame. */
	void DebugJumpToShot(int32 Index);
	/** Captures: replay the intro's shot up to time T (s from the start) in 1/30 s steps; bFreeze holds it there. */
	void DebugSetTime(float T, bool bFreeze);
	bool IsDebugFrozen() const { return bDebugFrozen; }

	/** Seconds after start before a skip is accepted, and how long the skip key must be held. */
	static constexpr float SkipLockSeconds = 1.f;
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
	void ApplyLens(float FocalMm, float FStop, float FocusDistance);
	void PlayNarrationVoice(int32 Line);
	void EnsureWidget();
	void RemoveWidget();
	void HandleSkipPressed();
	void HandleSkipReleased();
	APlayerController* GetPlayer() const;
	FVector SunDirection() const;

	FSimpleDelegate OnFinishedDelegate;
	TArray<FTDIntroShot> Shots;
	FTDIntroOverlayState Overlay;

	UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
	UPROPERTY(Transient) TObjectPtr<UInputComponent> SkipInput;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Showcase;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
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
	bool bHUDWasShown = true;

	bool bPlaying = false;
	bool bAllowWithoutViewport = false;
	bool bHandingOff = false;
	bool bSkipped = false;
	bool bSkipHeld = false;
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
};
