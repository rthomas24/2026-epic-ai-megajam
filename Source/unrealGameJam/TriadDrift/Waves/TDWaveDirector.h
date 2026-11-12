// Triad Drift - Enemies & Waves (Teammate 4). ENG-14 wave director.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Waves/TDWaveTypes.h"
#include "TDWaveDirector.generated.h"

class UDataTable;
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
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDWaveDirector : public AActor
{
	GENERATED_BODY()

public:
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
	float BreatherSeconds = 6.f;

	/** Red warp-in telegraph before a group appears. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float WarpInSeconds = 1.5f;

	/** Fraction of max hull repaired per second during the breather, on every living squad ship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float BreatherHullRepairPerSecond01 = 0.08f;

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

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDWaveStateSignature OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDEnemySpawnedSignature OnEnemySpawned;

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

	void LoadWaveTable();
	void SetState(ETDWaveState NewState, float Duration = 0.f);
	void BeginWave(int32 Index);
	void SpawnGroup(const FPendingGroup& Group);
	AActor* SpawnEnemy(FName EnemyId, const FTransform& Transform);
	void CheckWaveCleared();
	void FinishWave();
	void AdvanceAfterBreather();
	void ClearEnemies();
	void RepairSquadHulls(float DeltaSeconds);
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
	int32 EnemiesTotal = 0;
	int32 CurrentTake = 1;
	bool bTakeAnnouncedByFlow = false;
	bool bWaveTableLoaded = false;
	bool bSwitchTipShown = false;
	FRandomStream Rng;
};
