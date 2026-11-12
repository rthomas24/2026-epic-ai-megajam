// Live game state for audio and music (features.md ART-10 / ART-11, MESSAGEBOARD.md R-010 / R-013).
//
// One Blueprint-readable place that says "what is the game doing right now", so an audio director
// (Blueprint or C++, owned by presentation) can drive music layers, sound mixes and ambience without
// knowing any gameplay class. Gameplay never calls into audio; audio reads this and the event bus.
//
//   Blueprint:  Get TD Audio State Subsystem -> Get State  (or the single-value getters)
//   C++:        if (UTDAudioStateSubsystem* A = UTDAudioStateSubsystem::Get(this)) { A->GetState(); }
//
// Everything here is derived from Core contracts only (ITDHUDData, ITDGameFlow, UTDEventBus,
// UTDFeelSubsystem), so it works in every map and costs nothing when no audio reads it.
// Smoothed values move in REAL time: slow motion does not slow the music's reaction.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDAudioState.generated.h"

class AActor;

/** Where the player is, as far as a mix is concerned. */
UENUM(BlueprintType)
enum class ETDAudioScene : uint8
{
	/** Menu, transition, or nothing playable yet. */
	FrontEnd,
	/** Flying a ship with the squad. */
	Space,
	/** On foot. */
	Ground
};

/** A snapshot. Every 0..1 value is clamped; every "SecondsSince" value is real seconds, large when never. */
USTRUCT(BlueprintType)
struct FTDAudioState
{
	GENERATED_BODY()

	// ------------------------------------------------------------------ where
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	ETDAudioScene Scene = ETDAudioScene::FrontEnd;

	/** Story phase (Intro, Waves1to2, Verdance ... Victory). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	ETDGamePhase Phase = ETDGamePhase::Intro;

	// ------------------------------------------------------------------ the player
	/** Ship the player flies (last one flown while on foot). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	ETDShipId ActiveShip = ETDShipId::Rift;

	/** 0 stopped .. 1 top speed of the player's ship (0 on foot). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float Speed01 = 0.f;

	/** Boost energy left on the player's ship. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float Boost01 = 1.f;

	/** Weapon heat of the player's ship. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float Heat01 = 0.f;

	/** Hull of the player's ship, or the character's health on foot. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float Health01 = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float Shield01 = 1.f;

	/** 0 while the hull is above 35%, rising to 1 at 0%: drive a low-health filter or heartbeat. Smoothed. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float Danger01 = 0.f;

	/** Squad ships alive, 0..3 (3 on foot). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	int32 SquadAlive = 3;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	bool bOutOfBounds = false;

	// ------------------------------------------------------------------ the fight
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	int32 WaveNumber = 0;

	/** The wave director's own 0..1 escalation for the current wave. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float WaveIntensity01 = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	int32 EnemiesAlive = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	int32 EnemiesTotal = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	bool bWaveActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	bool bInBreather = false;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	ETDMissileWarning MissileWarning = ETDMissileWarning::None;

	/** Real seconds since the player's side last landed a hit / got a kill / a highlight moment fired. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float SecondsSinceHit = 1000.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float SecondsSinceKill = 1000.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float SecondsSinceHighlight = 1000.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	ETDHighlightKind LastHighlight = ETDHighlightKind::RamKill;

	/** How hot the fight is right now: enemies present and recent hits and kills. Smoothed. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float Combat01 = 0.f;

	// ------------------------------------------------------------------ travel and time
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	ETDHyperdriveState Hyperdrive = ETDHyperdriveState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float HyperdriveCharge01 = 0.f;

	/** Global time dilation as the player experiences it (1 = normal). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float TimeDilation = 1.f;

	/** A director slow-motion beat is running (not a 40 ms hit-stop): push the slow-motion sound mix. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	bool bSlowMotion = false;

	/** A take just ended with "CUT!" and the scene is about to restart. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	bool bSceneCut = false;

	// ------------------------------------------------------------------ music
	/**
	 * One smoothed 0..1 value to drive layered music. A suggestion, not a rule: it is built from the
	 * raw values above, which stay available for a different mapping.
	 *   front end 0.0 · cruising with no wave 0.15 · breather 0.25 · wave 0.4 .. 0.9 by the wave's
	 *   escalation and Combat01 · on foot 0.3 .. 0.7 · Dominion assault never below 0.8 · "CUT!" 0.1 ·
	 *   victory 0.3 · low hull adds up to 0.1. Rises at 0.5 per real second, falls at 0.2.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Audio")
	float MusicIntensity01 = 0.f;
};

UCLASS()
class UNREALGAMEJAM_API UTDAudioStateSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Null outside game worlds. */
	static UTDAudioStateSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	FTDAudioState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	float GetMusicIntensity01() const { return State.MusicIntensity01; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	float GetCombat01() const { return State.Combat01; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	float GetDanger01() const { return State.Danger01; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	float GetSpeed01() const { return State.Speed01; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	bool IsSlowMotion() const { return State.bSlowMotion; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	ETDAudioScene GetScene() const { return State.Scene; }

	/** The unsmoothed music target for the current raw state (what MusicIntensity01 is moving toward). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Audio")
	float GetMusicIntensityTarget() const;

	/** Recompute from the game right now. Called every frame; public so tests can step it. */
	void Refresh(float RealDeltaSeconds);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	UFUNCTION()
	void HandlePhaseChanged(ETDGamePhase Phase);
	UFUNCTION()
	void HandleShipSwitched(ETDShipId NewShip, ETDShipId OldShip);
	UFUNCTION()
	void HandleHitConfirmed(AActor* Target, bool bKill);
	UFUNCTION()
	void HandleHyperdriveState(ETDHyperdriveState NewState);
	UFUNCTION()
	void HandleHighlight(ETDHighlightKind Kind, AActor* Subject, float Importance01);
	UFUNCTION()
	void HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds);
	UFUNCTION()
	void HandleTakeStarted(FName SceneId, int32 TakeNumber);

	void BindBus();
	AActor* FindSquadData();
	float ComputeCombatTarget() const;

	FTDAudioState State;

	TWeakObjectPtr<AActor> CachedSquadData;
	double NextSearchRealTime = 0.0;
	double LastRefreshRealTime = -1.0;
	bool bBusBound = false;
};
