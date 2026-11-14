// Triad Drift - Enemies & Waves (Teammate 4). ENG-14 wave director.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Waves/TDWaveTypes.h"
#include "TDWaveDirector.generated.h"

class UDataTable;

namespace TDHyperdriveLocks
{
	inline const FName Wave(TEXT("Wave"));
	inline const FName Finale(TEXT("Finale"));
}
class UTDHealthComponent;
class UTDTravelFX;

UENUM(BlueprintType)
enum class ETDWaveState : uint8
{
	Idle,			// nothing running (before StartFromProgress, or the current phase has no waves)
	WarpingIn,		// red warp-in telegraphs are playing; the wave's first groups are not spawned yet
	Active,			// fighting
	Breather,		// 6 s between waves of the same phase; hulls repair
	PhaseComplete,	// last wave of the current phase cleared; the flow takes over (planet / Dominion)
	AllComplete		// every wave cleared
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDWarpInSignature, FVector, Location, FName, EnemyId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDWaveStateSignature, ETDWaveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDEnemySpawnedSignature, AActor*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDTutorialSignature);

/** The tutorial battle's ending (storyline 13 phase 05). */
UENUM(BlueprintType)
enum class ETDTutorialPhase : uint8
{
	Battle,		// fighting (phases 01-04)
	Withdrawal,	// Command ordered the pull-back: escape boundary and marker up, pursuers stop at the perimeter
	Escaped		// the flown ship crossed the boundary: the tutorial is complete
};

/** Whether the tutorial wave ends with the withdrawal (Auto = real levels only, so simulations and other lanes'
 *  test worlds keep clearing wave 2 by kills). */
UENUM(BlueprintType)
enum class ETDTutorialMode : uint8
{
	Auto,
	ForceOn,
	ForceOff
};

/** Spawns one enemy. Return null to skip. Tests install stand-ins through SetSpawnFactory. */
using FTDEnemySpawnFactory = TFunction<AActor*(UWorld* World, FName EnemyId, const FTransform& Transform)>;

/**
 * Runs the five galaxy waves (features.md ENG-14). Each wave warps its groups in behind / flanking
 * the player's ship after a red warp-in telegraph, counts every enemy (carrier launches included,
 * via RegisterEnemy), clears when they're all dead, runs a 6 s breather with hull repair, then starts
 * the next wave of the same phase. The last wave of a phase hands over to the flow (planet travel).
 * Talks to the rest of the game only through UTDEventBus and ITDGameFlow.
 *
 * Numbering: StartWave takes a 0-based table index; every event and FTDWaveInfo use 1-based
 * wave numbers (Core rule).
 */
class ATDEnemyShip;

UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDWaveDirector : public AActor
{
	GENERATED_BODY()

public:
	/** Every Cinder objective is complete (UTDFlowSettings::CinderObjectives): half of the field-opening rule. */
	static bool IsCinderFinished(const FTDProgress& Progress);

	ATDWaveDirector();

	/** The director in this world, or null. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves", meta = (WorldContext = "WorldContext"))
	static ATDWaveDirector* Get(const UObject* WorldContext);

	/** Start the wave at table Index (0-based) now. Clears whatever is running. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void StartWave(int32 Index);

	/** Read ITDGameFlow::GetProgress() and start the right wave for the current phase (or stay idle). No-op while a wave runs. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void StartFromProgress();

	/** Debug: remove the current wave's enemies and treat it as cleared. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void SkipWave();

	/** Despawn everything and go idle (travel, debug). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void StopAndClear();

	/** Restart the current wave as the next take (Director's Cut #3). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void RestartCurrentWave();

	/** Count an enemy that joined mid-wave (carrier launches). Safe to call twice. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void RegisterEnemy(AActor* Enemy);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	FTDWaveInfo GetWaveInfo() const;

	/**
	 * Second review MAJOR B (ENG-14/16/17): waves 3-5 are fought in the combat zone, not at a planet's orbit return
	 * point. StartFromProgress holds such a wave (Idle) until the player's ship is inside the layout's CombatZoneRadius;
	 * the flow shows "RETURN TO THE COMBAT ZONE" with a marker on GetCombatZoneMarker() meanwhile.
	 */
	bool IsAwaitingCombatZone() const { return bAwaitingCombatZone; }
	AActor* GetCombatZoneMarker() const { return bAwaitingCombatZone ? CombatZoneMarker.Get() : nullptr; }
	/** A wave is warping in or being fought (the planet orbit gates close meanwhile). */
	bool IsWaveRunning() const { return State == ETDWaveState::WarpingIn || State == ETDWaveState::Active; }
	/** True when there is no layout or no player ship to check (sandboxes and tests never wait). */
	bool IsPlayerInCombatZone() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	ETDWaveState GetState() const { return State; }

	/** 0-based table index of the current (or last) wave; INDEX_NONE before the first. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetCurrentWaveIndex() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetWaveCount() const { return Waves.Num(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetEnemiesAlive() const;

	/** Enemies of the current wave so far (table count + launches registered). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetEnemiesTotal() const { return EnemiesTotal; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	float GetBreatherRemaining() const { return State == ETDWaveState::Breather ? StateRemaining : 0.f; }

	/** Living enemies of the current wave (HUD, debug, tests). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	TArray<AActor*> GetLivingEnemies() const;

	/** Groups still waiting to warp in for the current wave. */
	int32 GetPendingGroupCount() const { return Pending.Num(); }

	const FTDWaveDef* GetWaveDef(int32 Index) const;
	const TArray<FTDWaveDef>& GetWaveTable() const { return Waves; }

	/** The features.md ENG-14 table. */
	static TArray<FTDWaveDef> MakeDefaultWaves();

	/** Replace the wave table (tests / debug). */
	void SetWaveTable(const TArray<FTDWaveDef>& InWaves);

	/** Override how enemies are spawned (tests). Null restores the default (EnemyClass). */
	void SetSpawnFactory(FTDEnemySpawnFactory InFactory) { SpawnFactory = MoveTemp(InFactory); }

	// --- Tuning ---------------------------------------------------------------------------------

	/** Optional DT_Waves override (rows of FTDWaveDef, in row order). Empty = C++ defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Waves")
	TObjectPtr<UDataTable> WaveTableOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float BreatherSeconds = 6.f;	// clamped to >= 2 s at runtime (longer than the 1.75 s CUT delay)

	/** ENG-27: attack slots per wave (index 0 = wave 1): how many pilots may press the same squad ship at once.
	 *  0 = unlimited. Tuned with the Evader balance pilot as the reference player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	TArray<int32> AttackSlotsByWave = { 0, 1, 1, 1, 2 };

	/**
	 * Wave 1 is where a new player learns the controls (Lead, Nov 12). For its first OpeningGraceSeconds, until the
	 * player fires or boosts, wave-1 enemies deal OpeningGraceDamage x their damage and no squad ship is let below
	 * OpeningGraceHullFloor01 of its hull by enemy fire. Then normal pressure. Waves 2+ are unchanged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float OpeningGraceSeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0", ClampMax = "1"))
	float OpeningGraceDamage = 0.5f;

	/** The reduced damage lasts at least this long into wave 1 even after the player acts (T1's control hints run
	 *  in the first ~15-25 s: fire, boost and dodge are tried against reduced fire). The hull floor still ends
	 *  as soon as the player fires or boosts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float OpeningGraceMinDamageSeconds = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0", ClampMax = "1"))
	float OpeningGraceHullFloor01 = 0.5f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	bool IsOpeningGraceActive() const { return bOpeningGrace; }

	/** Red warp-in telegraph before a group appears. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float WarpInSeconds = 1.5f;

	/** Distance from the player at which groups warp in (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float SpawnDistance = 16000.f;

	/** Random jitter per group around the spawn point (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float SpawnJitter = 2500.f;

	/** Spacing between ships in a pack formation (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float FormationSpacing = 900.f;

	/** Carriers and gunships warp in this much farther out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "1"))
	float LargeShipSpawnDistanceScale = 1.6f;

	/** Actor class spawned for enemies by default (must implement ITDWaveEnemy). Null = ATDEnemyShip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	TSubclassOf<AActor> EnemyClass;

	/** Start automatically on BeginPlay. Off by default: the space game mode calls StartFromProgress. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	bool bAutoStartFromProgress = false;

	/** Seed for spawn placement (tests set it for determinism). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	int32 RandomSeed = 1337;

	/** Nick's travel FX asset; EnemyWarpIn plays at each warp-in point. Empty = OnWarpInStarted only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDTravelFX> TravelFX;

	/** Optional explicit anchor for spawns. Null = the player's pawn (or this actor if none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	TObjectPtr<AActor> SpawnAnchorOverride;

	// --- Events (local; global ones go through UTDEventBus) -------------------------------------

	/** A red warp-in telegraph started at Location (Nick: red flash VFX + sound, UTDTravelFX::EnemyWarpIn). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDWarpInSignature OnWarpInStarted;

	/** Placeholder warp-in telegraph (a growing red ring) while DA_Travel_FX has no EnemyWarpIn VFX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	bool bWarpInPlaceholder = true;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDWaveStateSignature OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDEnemySpawnedSignature OnEnemySpawned;

	// --- Tutorial withdrawal (storyline 13 phase 05; Nicholas's decision) ------------------------------------

	/** Command orders the withdrawal: the escape boundary and its marker appear (T1, T2, T3 hang on this). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Tutorial")
	FTDTutorialSignature OnTutorialWithdrawalOrdered;

	/** The flown ship crossed the escape boundary: the tutorial is complete (T1's music fade, T3's Aftermath). Once. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Tutorial")
	FTDTutorialSignature OnTutorialEscaped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial")
	ETDTutorialMode TutorialMode = ETDTutorialMode::Auto;

	/** The escape boundary sits this far (cm) ahead of the flown ship when Command orders the withdrawal, on the bearing to Verdance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float EscapeBoundaryDistance = 250000.f;

	/** ...but never closer than this (cm) to Verdance's atmosphere, which the squad cannot enter yet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float EscapeAtmosphereMargin = 30000.f;

	/** ...and never nearer the ship than this (cm), so the escape is always a short flight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float EscapeMinDistance = 80000.f;

	/** Pursuers stop this far (cm) short of the boundary and hold there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float PursuerStopMargin = 20000.f;

	/** Heavy contact engaged: this long (s) after it arrives, if it took damage or the flown ship came within range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float HeavyEngagedSeconds = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float HeavyEngageRange = 150000.f;

	/** ...or at this fraction of its hull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0", ClampMax = "1"))
	float HeavyHullTrigger = 0.5f;

	ETDTutorialPhase GetTutorialPhase() const { return TutorialPhase; }
	/** The boundary plane (a point on it and its outward normal). False before the withdrawal. */
	bool GetEscapeBoundary(FVector& OutPoint, FVector& OutNormal) const;
	/** The corridor marker (tags TD.Objective.Marker and TD.Tutorial.Escape) while the withdrawal is on. */
	AActor* GetEscapeMarker() const { return EscapeMarker.Get(); }
	AActor* GetHeavyContact() const { return HeavyContact.Get(); }
	AActor* GetJumpPoint() const { return JumpPointMarker.Get(); }
	/** The JUMP POINT sits this far (cm) past the escape line, on its bearing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float JumpPointPastLine = 30000.f;
	/** Spawn points re-picked because they fell inside an obstacle, and those pushed out as a last resort (tests). */
	int32 GetSpawnPointRetries() const { return SpawnPointRetries; }
	int32 GetSpawnPointsPushedOut() const { return SpawnPointsPushedOut; }
	/** The warp-in points picked this wave (tests). */
	const TArray<FVector>& GetPickedSpawnPoints() const { return PickedSpawnPoints; }
	bool IsTutorialWithdrawalWave() const;
	/** Pursuers holding at the perimeter right now (tests). */
	int32 GetHoldingPursuers() const { return HeldPursuers.Num(); }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	struct FPendingGroup
	{
		FTDWaveGroup Group;
		FVector Location = FVector::ZeroVector;
		FRotator Facing = FRotator::ZeroRotator;
		float Remaining = 0.f;		// seconds until spawn (delay + warp-in)
		bool bWarpAnnounced = false;
	};

	static TArray<TWeakObjectPtr<ATDWaveDirector>>& Instances();
	void SetWaveHyperdriveLock(bool bLocked);
	void LoadWaveTable();
	void SetState(ETDWaveState NewState, float Duration = 0.f);
	void BeginWave(int32 Index);
	void SpawnGroup(const FPendingGroup& Group);
	AActor* SpawnEnemy(FName EnemyId, const FTransform& Transform);
	void CheckWaveCleared();
	void FinishWave();
	void AdvanceAfterBreather();
	void ClearEnemies();
	void PickSpawnPoint(ETDSpawnZone Zone, bool bLarge, FVector& OutLocation, FRotator& OutFacing);
	AActor* GetSpawnAnchor() const;
	void AnnounceScene(const FTDWaveDef& Wave);
	void FireTips(const FTDWaveDef& Wave);
	bool PhaseMatches(ETDGamePhase WavePhase, ETDGamePhase ProgressPhase) const;
	FName GetSceneId(int32 Index) const;

	/** T5's game instance announces retakes on the bus: a higher take of our scene restarts the wave. */
	UFUNCTION()
	void HandleTakeStarted(FName SceneId, int32 TakeNumber);

	UFUNCTION()
	void HandleGamePhaseChanged(ETDGamePhase Phase);

	UFUNCTION()
	void HandleHyperdriveState(ETDHyperdriveState NewState);
	void ClearJumpPoint();
	/** The tutorial's JUMP POINT waypoint (TD.Objective.Marker, TD.Tutorial.JumpPoint) after the crossing. */
	TWeakObjectPtr<AActor> JumpPointMarker;

public:
	// ENG-40 with T3's UTDTutorialPhases (agreed Nov 14): the lessons gate the pressure.
	/** Wave-1 enemies' outgoing damage while the player is taught to evade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0", ClampMax = "1"))
	float EvadeDamageScale = 0.3f;
	/** Gate the heavy contact and Command's withdrawal on T3's squad lesson (tests of the withdrawal itself turn it off). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial")
	bool bWaitForLessons = true;
	/** Wave 2's heavy contact waits for the squad lesson (Done), at most this long after the wave starts (never locks). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float GunshipHoldCapSeconds = 60.f;
	/** Command's withdrawal waits for the squad lesson (Done), at most this long after it was due (never locks). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Tutorial", meta = (ClampMin = "0"))
	float WithdrawalWaitCapSeconds = 45.f;
protected:
	void RequestWithdrawal();
	void UpdateTeachPressure();
	bool IsTeachingSquad() const;
	void SetSquadBoundaryRelease(bool bRelease);
	float TutorialClock = 0.f;
	float WithdrawalWait = 0.f;
	bool bWithdrawalRequested = false;
	bool bBoundaryReleased = false;
	// ENG-43 barks (TDWorldBarks).
	TSet<FName> SeenTypes;
	bool bIncomingSaid = false;
	float MineCheckTimer = 0.f;
	float WaveStoryClock = 0.f;
	bool bWaveStoryFired = false;
	int32 SpawnPointRetries = 0;
	int32 SpawnPointsPushedOut = 0;
	TArray<FVector> PickedSpawnPoints;
	void CheckMinesAhead();

	UFUNCTION()
	void HandleEnemyDeath(UTDHealthComponent* Health, const FTDDamageInfo& KillingBlow);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* Actor);

	UPROPERTY(Transient)
	TArray<FTDWaveDef> Waves;

	TArray<TWeakObjectPtr<AActor>> Living;
	TArray<FPendingGroup> Pending;
	FTDEnemySpawnFactory SpawnFactory;
	TArray<AActor*> ScratchActors;

	ETDWaveState State = ETDWaveState::Idle;
	float StateRemaining = 0.f;
	int32 CurrentWave = INDEX_NONE;
	float LockSyncTimer = 0.f;
	// MAJOR B: a wave 3-5 held until the player is back in the combat zone.
	bool bAwaitingCombatZone = false;
	float CombatZoneCheckTimer = 0.f;
	UPROPERTY(Transient) TObjectPtr<AActor> CombatZoneMarker;
	void AwaitCombatZone(int32 Index);
	void StopAwaitingCombatZone();
	bool WaveNeedsCombatZone(int32 Index) const;
	bool bCutPending = false;
	bool bOpeningGrace = false;
	bool bOpeningFloor = false;
	float OpeningGraceLeft = 0.f;
	int32 OpeningGraceShotBaseline = 0;
	void TickOpeningGrace(float DeltaSeconds);
	void TickTutorial(float DeltaSeconds);
	void BeginWithdrawal();
	void CompleteEscape();
	void ResetTutorial();
	ETDTutorialPhase TutorialPhase = ETDTutorialPhase::Battle;
	TWeakObjectPtr<AActor> HeavyContact;
	bool bHeavySeen = false;
	bool bHeavyEngaged = false;
	float HeavyTime = 0.f;
	FVector EscapePoint = FVector::ZeroVector;
	FVector EscapeNormal = FVector::ForwardVector;
	TWeakObjectPtr<AActor> EscapeMarker;
	TSet<TWeakObjectPtr<AActor>> HeldPursuers;
	void EndOpeningGrace();
	int32 PlayerShotsFired() const;
	UFUNCTION()
	void HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds);
	void SpawnWarpInMarker(const FVector& Location, float EnemyRadius);

public:
	/**
	 * ENG-33 (T3's showcase shots): an enemy ship for the camera only. No AI, never registered with a wave, invulnerable,
	 * not targetable, weapons blocked, no collision, no movement (place it yourself), tagged "TD.Showcase". Its carrier
	 * parts get the same treatment. Destroy it when the shot ends.
	 */
	static ATDEnemyShip* SpawnShowcaseShip(UWorld* World, FName EnemyId, const FTransform& Where);

	/** ENG-33: the enemy warp-in telegraph alone (Nick's EnemyWarpIn cue, or the placeholder marker). Size in cm. */
	static void PlayShowcaseWarpIn(UWorld* World, const FVector& Location, float Size);

	static const FName ShowcaseTag;	// "TD.Showcase"
protected:
	int32 EnemiesTotal = 0;
	int32 CurrentTake = 1;
	bool bTakeAnnouncedByFlow = false;
	bool bWaveTableLoaded = false;
	bool bSwitchTipShown = false;
	FRandomStream Rng;
};
