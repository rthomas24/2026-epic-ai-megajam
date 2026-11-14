// Triad Drift - combat music director (N-029). Owner: Teammate 1 (audio settings and the sound mix live with Input).
//
// Plays UTDUIStyle::CombatMusic (Nick's slot) while a fight is on, through ONE 2D UI-sound audio component per map,
// with the sound's own class and volume (the options' Music slider applies through that class).
//
// Rules (each has a test, Input/Tests/TDMusicDirectorTests.cpp):
//  - A fight is on when FTDAudioState::bWaveActive, during the DominionAssault phase, or while any ATDEncounterVolume
//    IsActive() (on foot).
//  - Start: 2 s fade in. End of the fight: hold 6 s at full volume, then a 4 s fade out. A new fight during the hold
//    just carries on; during the fade out it swells back up (no restart).
//  - 1 s fade-stop on level travel (incl. quit to menu), on the Victory phase and on a CUT (the retake's fight
//    starts it again, from the top).
//  - Silent when the slot is empty. The menu map never plays it (the menu's own music stays separate, and the
//    component belongs to the map, so the two never overlap).
//
// A world subsystem (not a game-instance one) so it ticks with the world, its one component dies with the map,
// and tests drive it with a test world.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDMusicDirectorSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ETDCombatMusicState : uint8
{
	Silent,
	FadingIn,
	Playing,
	Holding,    // the fight ended: full volume for HoldSeconds
	FadingOut,  // after the hold
	Stopping    // 1 s stop: travel, victory, CUT
};

UCLASS()
class UNREALGAMEJAM_API UTDMusicDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

	static UTDMusicDirectorSubsystem* Get(const UObject* WorldContextObject);
	/** N-082: the music director of a world, for callers that use reflection (Nick's audio plugin). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music", meta = (WorldContext = "WorldContext"))
	static UTDMusicDirectorSubsystem* GetMusicDirector(const UObject* WorldContext) { return Get(WorldContext); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music")
	ETDCombatMusicState GetMusicState() const { return MusicState; }

	/** 0..1 volume the director wants right now (the fade envelope; the sound's own volume multiplies it). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music")
	float GetEnvelope() const { return Envelope; }

	/** Is a fight on right now (the rule above)? */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music")
	bool IsCombatNow() const;

	/** How many times playback started from the top (tests: a hold or fade-out swell never restarts it). */
	int32 GetStartCount() const { return StartCount; }
	UAudioComponent* GetAudioComponent() const { return Audio; }

	/** Tests: the track instead of the style slot (null = the slot is empty). */
	void SetMusicForTests(USoundBase* InMusic) { MusicOverride = InMusic; bUseOverride = true; }
	/** Tests: force the fight signal (otherwise the audio state, phase and encounter volumes decide). */
	void SetCombatOverrideForTests(TOptional<bool> bInCombat) { CombatOverride = bInCombat; }
	/**
	 * Ambience bed (ENG-35): the map's own loop under the music (space on the galaxy, Verdance / Cinder on the
	 * planet maps), 2D, faded in over AmbienceFadeSeconds and out on travel, victory and the menu.
	 */
	static FName AmbienceSlotForMap(const FString& MapName);
	float GetAmbienceEnvelope() const { return AmbienceEnvelope; }
	UAudioComponent* GetAmbienceComponent() const { return AmbienceAudio; }
	USoundBase* GetAmbienceBed() const { return AmbienceBed; }
	/** Tests: the bed instead of the style slot (null = no bed). */
	void SetAmbienceForTests(USoundBase* InBed) { AmbienceOverride = InBed; bUseAmbienceOverride = true; }

	/**
	 * N-037: the gain the fight's intensity puts on the music (UTDMusicSettings: CalmGain at intensity 0 .. FightGain
	 * at 1, rate-limited). The component's volume = GetEnvelope() x this.
	 */
	float GetIntensityGain() const { return IntensityGain; }
	float GetAppliedVolume() const { return Envelope * IntensityGain; }
	/** Tests (review M10): how many times the style slot was looked up (never per frame while silent). */
	int32 GetMusicLookupsForTests() const { return MusicLookups; }
	/** T7 perf: score slots requested in the background or already held (none is ever loaded on the game thread). */
	int32 GetPrimedScoreCountForTests() const { return PendingScores.Num() + HeldScores.Num(); }
	/** Tests: force the music intensity (otherwise UTDAudioStateSubsystem's smoothed MusicIntensity01). */
	void SetIntensityForTests(TOptional<float> InIntensity) { IntensityOverride = InIntensity; }

	/** Tests: as if a level travel (any destination, the menu included) had been requested. */
	void SetTravellingForTests() { bTravelling = true; }

	static constexpr float FadeInSeconds = 2.f;
	static constexpr float HoldSeconds = 6.f;
	static constexpr float FadeOutSeconds = 4.f;
	static constexpr float StopSeconds = 1.f;
	static constexpr float AmbienceFadeSeconds = 2.f;

	/** True while the Dominion assault's own score plays (UTDUIStyle::AssaultMusic; combat music held). */
	bool IsAssaultMusicPlaying() const { return AssaultVoice != nullptr || bAssaultLogical; }
	/**
	 * N-074: any score owner (Nick's players: Verdance, boarding, credits) claims the music; while any claim stands the
	 * generic combat bed and the idle score hold. Release with a fade: the hold lasts through it, then combat may return.
	 * Claiming again (same owner) cancels a pending release. Owners are independent: the hold lasts until the last one.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Music")
	void RequestExclusiveScore(FName Owner);
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Music")
	void ReleaseExclusiveScore(FName Owner, float FadeSeconds = 0.f);
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music")
	bool IsExclusiveScoreHeld() const { return ExclusiveClaims.Num() > 0; }

	/** True while the quiet-flight score plays (UTDUIStyle::IdleSpaceMusic). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music")
	bool IsIdleMusicPlaying() const { return IdleVoice != nullptr || bIdleLogical; }
	/** N-063: quiet flight in the galaxy right now (out of the tutorial, no fight, not the assault, no travel, no menu). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Music")
	bool IsQuietFlight() const { return bQuietFlight; }
	void SetIdleMusicForTests(USoundBase* Sound, float DelaySeconds) { IdleOverride = Sound; bIdleOverride = true; IdleDelayOverride = DelaySeconds; }
	/** Tests: this sound as AssaultMusic (null = the style's). */
	void SetAssaultMusicForTests(USoundBase* Sound) { AssaultOverride = Sound; bAssaultOverride = true; }

private:
	USoundBase* ResolveMusic() const;
	USoundBase* ResolveAssaultMusic() const;
	void TickAssault(UWorld* World, bool bWanted);
	void TickIdle(UWorld* World, bool bQuiet, float Dt);
	USoundBase* ResolveIdleMusic(float& OutDelay) const;
	void StartPlayback();
	void BeginStop(const TCHAR* Why);
	void ApplyEnvelope();
	void HandleSceneCut();
	void HandleTakeStarted();

	UFUNCTION() void OnSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds);
	UFUNCTION() void OnTakeStarted(FName SceneId, int32 TakeNumber);

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Audio;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> AmbienceAudio;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> AssaultVoice;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> IdleVoice;
	UPROPERTY(Transient) TObjectPtr<USoundBase> IdleOverride;
	bool bIdleOverride = false;
	float IdleDelayOverride = 6.f;
	bool bIdleLogical = false;
	bool bQuietFlight = false;
	/** Owner -> seconds of release fade left (< 0: held until released). */
	TMap<FName, float> ExclusiveClaims;
	float QuietTime = 0.f;
	UPROPERTY(Transient) TObjectPtr<USoundBase> AssaultOverride;
	bool bAssaultOverride = false;
	bool bAssaultLogical = false; // tests / no audio device: the assault score "plays"
	UPROPERTY(Transient) TObjectPtr<USoundBase> AmbienceOverride;
	UPROPERTY(Transient) TObjectPtr<USoundBase> AmbienceBed;
	/** The combat track, held once found (a packaged build would otherwise reload it after every GC). */
	UPROPERTY(Transient) mutable TObjectPtr<USoundBase> CachedMusic;
	mutable float MusicRetry = 0.f;
	mutable int32 MusicLookups = 0;
	bool bUseAmbienceOverride = false;
	float AmbienceEnvelope = 0.f;
	float IntensityGain = 1.f;
	TOptional<float> IntensityOverride;
	float TargetIntensityGain() const;
	float AmbienceRetry = 0.f;
	USoundBase* ResolveAmbience() const;
	/** The loaded sound, or null while it streams in (requested once, in the background; never a game-thread load). */
	USoundBase* AsyncSound(const FSoftObjectPtr& Soft) const;
	void PrimeScoreSlots();
	UPROPERTY(Transient) mutable TArray<TObjectPtr<USoundBase>> HeldScores;
	mutable TMap<FSoftObjectPath, TSharedPtr<struct FStreamableHandle>> PendingScores;
	void TickAmbience(float Dt, bool bStop);
	UPROPERTY(Transient) TObjectPtr<USoundBase> MusicOverride;
	bool bUseOverride = false;
	TOptional<bool> CombatOverride;

	ETDCombatMusicState MusicState = ETDCombatMusicState::Silent;
	float Envelope = 0.f;
	float StateTime = 0.f;
	float StopFrom = 1.f;
	bool bWaitForTake = false;   // after a CUT: no music until the retake starts (or the old fight signal drops)
	bool bTravelling = false;    // a level travel was requested: stay silent on this map
	bool bMenuMap = false;
	int32 StartCount = 0;
};
