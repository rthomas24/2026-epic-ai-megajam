// Triad Drift - tutorial music (N-039, GitHub issue #24). Owner: Teammate 1.
//
// One voice at game-instance level (it outlives level travel and world teardown) for the opening of a New Game:
//  - Prelude: UTDUIStyle::TutorialPrelude starts during the intro at (intro length - prelude length), so it ends on
//    the frame UTDIntroDirector::Finish returns control. A skip (or a run with no intro) cuts it.
//  - Single voice (issue #24 v2, preferred): UTDUIStyle::TutorialMusic is one continuous source (prelude + loop) in a
//    MetaSound whose Wave Player loops from the downbeat. Watched intro: it enters at intro time (length - downbeat)
//    so the source downbeat (0:38) lands on the control frame, and nothing at all happens at control. Skip / no intro:
//    it enters at the downbeat on the control frame with a 150 ms fade-in.
//  - Two voices (fallback when TutorialMusic is empty; the 35f37d6 timing): the prelude plays as above, the loop starts
//    at 0:00 on the control frame (150 ms fade-in on a skip).
//  - The music plays across the opening fights and the gaps between them; the combat-music gate and the intensity gain
//    don't touch it; a CUT retake doesn't restart it.
//  - Fade (Nicholas, issue #24): the tutorial battle ends when the player crosses the escape boundary during Command's
//    ordered withdrawal (T4's ATDWaveDirector::OnTutorialEscaped -> NotifyTutorialEscaped), gunship alive or destroyed;
//    a wave clear or a kill alone never starts it. Until T4's event exists the fallback is the run leaving Waves1to2,
//    which follows the escape. TutorialFadeOutSeconds (default 20) on the same voice, carrying on across a level
//    travel. The combat music waits until the fade has finished (finding 3).
//  - While it owns the opening, the intro director does not start the legacy UTDUIStyle::IntroMusic (finding 1).
//  - A Continue never plays it (only UTDGameInstance::StartNewRun arms it); the menu stops it.
// Stepped by the music director (each game world's), so tests drive it with a test world.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TDTutorialMusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class UTDIntroDirector;

UENUM(BlueprintType)
enum class ETDTutorialMusicState : uint8
{
	Off,
	Armed,      // New Game: waiting for the intro's prelude point or the control frame
	Prelude,    // during the intro, ending on the control frame
	Loop,       // the tutorial
	FadingOut   // wave TutorialEndsAfterWave cleared
};

UCLASS()
class UNREALGAMEJAM_API UTDTutorialMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UTDTutorialMusicSubsystem* Get(const UObject* WorldContext);

	/** UTDGameInstance::StartNewRun: this run opens with the tutorial music. */
	void ArmForNewRun();
	/** The player crossed the escape boundary in the ordered withdrawal: the 20 s fade starts (once). */
	void NotifyTutorialEscaped(const TCHAR* Why = TEXT("escape boundary"));
	/** Stop at once (no fade). */
	void Stop(const TCHAR* Why);

	/** Called by the music director every frame of every game world (real seconds). */
	void Step(UWorld* World, float RealDeltaSeconds, bool bMenuMap);

	ETDTutorialMusicState GetState() const { return State; }
	/** True while the prelude, the loop or its fade plays: the combat music waits until the fade has finished. */
	bool IsHoldingCombatMusic() const
	{
		return State == ETDTutorialMusicState::Prelude || State == ETDTutorialMusicState::Loop || State == ETDTutorialMusicState::FadingOut;
	}

	/** This New Game opens with the tutorial music (armed or playing, and a prelude or loop is set): the intro director
	 *  then does not start the legacy UTDUIStyle::IntroMusic (issue #24 finding 1). */
	bool OwnsOpeningMusic() const;
	/** UTDIntroDirector asks this before starting IntroMusic. */
	static bool ShouldIntroPlayIntroMusic(const UObject* WorldContext);

	// ---- What happened, on this subsystem's clock (real seconds since ArmForNewRun) - tests, tools.
	float GetClock() const { return Clock; }
	float GetPreludeStartClock() const { return PreludeStartClock; }
	float GetPreludeStartOffset() const { return PreludeStartOffset; }
	float GetPreludeIntroTime() const { return PreludeIntroTime; }
	float GetLoopStartClock() const { return LoopStartClock; }
	/** Single-voice mode: the source time the voice entered at, and its fade-in (0 = none). */
	bool IsSingleVoice() const;
	float GetEnteredAtSourceTime() const { return EnteredAtSource; }
	float GetEnteredFadeIn() const { return EnteredFadeIn; }
	/** Source time now (single voice), from the entry and this clock: tests, TD.Tutorial.Status. */
	float GetSourceTimeNow() const { return EnterClock >= 0.f ? EnteredAtSource + (Clock - EnterClock) : -1.f; }
	/** Source time on the control frame (single voice): the downbeat, for a correct hand-off. */
	float GetSourceTimeAtControl() const { return EnterClock >= 0.f && ControlClock >= 0.f ? EnteredAtSource + (ControlClock - EnterClock) : -1.f; }
	int32 GetFadesBegun() const { return FadesBegun; }
	float GetControlClock() const { return ControlClock; }
	float GetFadeVolume() const { return Volume; }
	int32 GetVoicesStarted() const { return VoicesStarted; }
	/** Voices playing now: never more than one. */
	int32 GetLiveVoiceCount() const;
	UAudioComponent* GetVoice() const { return Voice; }

	/** Tests: these sounds and this prelude length instead of the style's (null loop = a silent but timed loop). */
	void SetSoundsForTests(USoundBase* Prelude, USoundBase* Loop, float PreludeSeconds, float FadeSeconds);
	/** Tests: single-voice mode with this (possibly null) source, downbeat and fades. */
	void SetSingleVoiceForTests(USoundBase* Music, float DownbeatSeconds, float FadeSeconds, float SkipFadeIn);

private:
	/** T3's UTDIntroDirector::OnIntroFinished (bound per world): the control frame, also with no intro. */
	UFUNCTION() void HandleIntroFinished(bool bPlayed, bool bSkipped);
	UFUNCTION() void HandleWaveCleared(int32 WaveNumber);
	void StartVoice(USoundBase* Sound, float StartSeconds, UWorld* World);
	/** Single voice: the one source, entered at SourceTime (seconds), with an optional fade-in. */
	void StartSingle(float SourceTime, float FadeIn, UWorld* World);
	USoundBase* SingleSound() const;
	float SkipFadeIn() const;
	void StopVoice();
	void BeginFade();
	USoundBase* PreludeSound() const;
	USoundBase* LoopSound() const;
	float PreludeSeconds() const;
	float FadeSeconds() const;

	ETDTutorialMusicState State = ETDTutorialMusicState::Off;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
	UPROPERTY(Transient) TObjectPtr<USoundBase> TestPrelude;
	UPROPERTY(Transient) TObjectPtr<USoundBase> TestLoop;
	bool bTestSounds = false;
	float TestPreludeSeconds = 0.f;
	float TestFadeSeconds = 0.f;

	TWeakObjectPtr<UTDIntroDirector> Intro;
	TWeakObjectPtr<UWorld> BoundWorld;
	float Clock = 0.f;
	float PreludeStartClock = -1.f;
	float PreludeStartOffset = 0.f;
	float PreludeIntroTime = -1.f;
	float LoopStartClock = -1.f;
	UPROPERTY(Transient) TObjectPtr<USoundBase> TestSingle;
	bool bTestSingle = false;
	float TestDownbeat = 38.f;
	float TestSkipFade = 0.15f;
	float EnteredAtSource = -1.f;
	float EnteredFadeIn = 0.f;
	float EnterClock = -1.f;
	int32 FadesBegun = 0;
	float ControlClock = -1.f;
	float FadeElapsed = 0.f;
	float Volume = 1.f;
	int32 VoicesStarted = 0;
	bool bPreludeStarted = false;
};
