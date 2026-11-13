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

	/** Tests: as if a level travel (any destination, the menu included) had been requested. */
	void SetTravellingForTests() { bTravelling = true; }

	static constexpr float FadeInSeconds = 2.f;
	static constexpr float HoldSeconds = 6.f;
	static constexpr float FadeOutSeconds = 4.f;
	static constexpr float StopSeconds = 1.f;
	static constexpr float AmbienceFadeSeconds = 2.f;

private:
	USoundBase* ResolveMusic() const;
	void StartPlayback();
	void BeginStop(const TCHAR* Why);
	void ApplyEnvelope();
	void HandleSceneCut();
	void HandleTakeStarted();

	UFUNCTION() void OnSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds);
	UFUNCTION() void OnTakeStarted(FName SceneId, int32 TakeNumber);

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Audio;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> AmbienceAudio;
	UPROPERTY(Transient) TObjectPtr<USoundBase> AmbienceOverride;
	UPROPERTY(Transient) TObjectPtr<USoundBase> AmbienceBed;
	bool bUseAmbienceOverride = false;
	float AmbienceEnvelope = 0.f;
	float AmbienceRetry = 0.f;
	USoundBase* ResolveAmbience() const;
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
