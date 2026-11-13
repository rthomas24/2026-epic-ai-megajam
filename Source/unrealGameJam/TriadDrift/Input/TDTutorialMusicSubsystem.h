// Triad Drift - tutorial music (N-039, GitHub issue #24). Owner: Teammate 1.
//
// One voice at game-instance level (it outlives level travel and world teardown) for the opening of a New Game:
//  - Prelude: UTDUIStyle::TutorialPrelude starts during the intro at (intro length - prelude length), so it ends on
//    the frame UTDIntroDirector::Finish returns control. A skip (or a run with no intro) cuts it.
//  - Loop: UTDUIStyle::TutorialLoop. Watched intro: scheduled on the audio clock (Quartz) when the prelude starts, to
//    begin on the prelude's last sample, so the hand-off has no gap and no game-frame stop / start (issue #24 finding 2).
//    Skipped or no intro: starts at 0:00 on the control frame. It plays across the opening fights and the gaps between
//    them; the combat-music gate and the intensity gain don't touch it; a CUT retake doesn't restart it.
//  - Fade: once wave UTDMusicSettings::TutorialEndsAfterWave is cleared, TutorialFadeOutSeconds (default 20) on the
//    same voice, carrying on across a level travel. The combat music waits until the fade has finished (finding 3).
//  - While it owns the opening, the intro director does not start the legacy UTDUIStyle::IntroMusic (finding 1).
//  - A Continue never plays it (only UTDGameInstance::StartNewRun arms it); the menu stops it.
// Stepped by the music director (each game world's), so tests drive it with a test world.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TDTutorialMusicSubsystem.generated.h"

class UAudioComponent;
class UQuartzClockHandle;
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
	/** Watched intro: the loop is queued at the prelude's start for the prelude's last sample (this clock's time). */
	float GetLoopScheduledClock() const { return LoopScheduledClock; }
	/** True when the loop was handed to the audio clock (Quartz), not started on a game frame. */
	bool IsLoopOnAudioClock() const { return bLoopOnAudioClock; }
	float GetControlClock() const { return ControlClock; }
	float GetFadeVolume() const { return Volume; }
	int32 GetVoicesStarted() const { return VoicesStarted; }
	/** Voices playing now: never more than one. */
	int32 GetLiveVoiceCount() const;
	UAudioComponent* GetVoice() const { return Voice; }

	/** Tests: these sounds and this prelude length instead of the style's (null loop = a silent but timed loop). */
	void SetSoundsForTests(USoundBase* Prelude, USoundBase* Loop, float PreludeSeconds, float FadeSeconds);

private:
	/** T3's UTDIntroDirector::OnIntroFinished (bound per world): the control frame, also with no intro. */
	UFUNCTION() void HandleIntroFinished(bool bPlayed, bool bSkipped);
	UFUNCTION() void HandleWaveCleared(int32 WaveNumber);
	void StartVoice(USoundBase* Sound, float StartSeconds, UWorld* World);
	/** Watched intro: the prelude now (at Offset) and the loop queued for the prelude's end, both on one audio clock. */
	void StartPreludeAndQueueLoop(UWorld* World, float Offset);
	void StopVoice();
	void BeginFade();
	USoundBase* PreludeSound() const;
	USoundBase* LoopSound() const;
	float PreludeSeconds() const;
	float FadeSeconds() const;

	ETDTutorialMusicState State = ETDTutorialMusicState::Off;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;         // the loop (or, before control, nothing)
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> PreludeVoice;  // the prelude; ends on its own as the loop begins
	UPROPERTY(Transient) TObjectPtr<UQuartzClockHandle> AudioClock;
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
	float LoopScheduledClock = -1.f;
	bool bLoopQueued = false;       // the loop voice exists and starts at the prelude's end (audio clock or logical)
	bool bLoopOnAudioClock = false;
	float ControlClock = -1.f;
	float FadeElapsed = 0.f;
	float Volume = 1.f;
	int32 VoicesStarted = 0;
	bool bPreludeStarted = false;
};
