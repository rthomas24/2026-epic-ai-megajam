// Triad Drift - Enemies & Waves (Teammate 4). ENG-14 wave director.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Waves/TDWaveTypes.h"
#include "TDWaveDirector.generated.h"

class ATDEnemyShip;
class UDataTable;

UENUM(BlueprintType)
enum class ETDWaveState : uint8
{
	Idle,			// nothing running (before StartFromProgress, or waiting for the flow to change phase)
	WarpingIn,		// red warp-in telegraphs are playing; enemies not yet spawned
	Active,			// fighting
	Breather,		// 6 s between waves; hulls repair
	PhaseComplete,	// last wave of the current phase cleared; the flow takes over (planet / Dominion)
	AllComplete		// every wave cleared
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDWarpInSignature, FVector, Location, FName, EnemyId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDWaveStateSignature, ETDWaveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDEnemySpawnedSignature, ATDEnemyShip*, Enemy);

/**
 * Runs the five galaxy waves (features.md ENG-14). Spawns each wave's groups behind / flanking the
 * player's active ship after a red warp-in telegraph, counts every enemy (carrier launches included),
 * clears the wave when they're all dead, runs a 6 s breather with hull repair, then starts the next
 * wave of the same phase. Talks to the rest of the game only through UTDEventBus and ITDGameFlow.
 *
 * One per world; the space game mode spawns it. Get() finds it.
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

	/** Start wave Index (0-based) now: kills nothing, spawns its warp-ins. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void StartWave(int32 Index);

	/** Read ITDGameFlow::GetProgress() and start the right wave for the current phase (or stay idle). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void StartFromProgress();

	/** Debug: kill the current wave's enemies and treat it as cleared. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void SkipWave();

	/** Despawn everything and go idle (scene restart, travel). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void StopAndClear();

	/** Restart the current wave as the next take (Director's Cut #3). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Waves")
	void RestartCurrentWave();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	FTDWaveInfo GetWaveInfo() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	ETDWaveState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetCurrentWaveIndex() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetWaveCount() const { return Waves.Num(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetEnemiesAlive() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	int32 GetEnemiesTotal() const { return EnemiesTotal; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	float GetBreatherRemaining() const { return State == ETDWaveState::Breather ? StateRemaining : 0.f; }

	/** Living enemies of the current wave (HUD, debug, tests). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Waves")
	TArray<ATDEnemyShip*> GetLivingEnemies() const;

	const FTDWaveDef* GetWaveDef(int32 Index) const;
	const TArray<FTDWaveDef>& GetWaveTable() const { return Waves; }

	/** The features.md ENG-14 table. */
	static TArray<FTDWaveDef> MakeDefaultWaves();

	// --- Tuning ---------------------------------------------------------------------------------

	/** Optional DT_Waves override (rows of FTDWaveDef, in row order). Empty = C++ defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Waves")
	TObjectPtr<UDataTable> WaveTableOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "0"))
	float BreatherSeconds = 6.f;

	/** Red warp-in telegraph before enemies appear. */
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

	/** Big ships (carriers, gunships) spawn this much farther out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves", meta = (ClampMin = "1"))
	float LargeShipSpawnDistanceScale = 1.6f;

	/** Pawn class spawned for enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	TSubclassOf<ATDEnemyShip> EnemyClass;

	/** If true, StartFromProgress runs automatically on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	bool bAutoStartFromProgress = true;

	/** Seed for spawn placement (tests set it for determinism). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	int32 RandomSeed = 1337;

	/** Optional explicit anchor for spawns. Null = the player's pawn (or this actor if no pawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Waves")
	TObjectPtr<AActor> SpawnAnchorOverride;

	// --- Events (local; the global ones go through UTDEventBus) --------------------------------

	/** A red warp-in telegraph started at Location (Nick: red flash VFX + sound). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDWarpInSignature OnWarpInStarted;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDWaveStateSignature OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Waves")
	FTDEnemySpawnedSignature OnEnemySpawned;

	virtual void Tick(float DeltaSeconds) override;

	/** Replace the wave table (tests / debug). */
	void SetWaveTable(const TArray<FTDWaveDef>& InWaves) { Waves = InWaves; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	struct FPendingGroup
	{
		FTDWaveGroup Group;
		FVector Location = FVector::ZeroVector;
		FRotator Facing = FRotator::ZeroRotator;
		float Remaining = 0.f;	// seconds until spawn (delay + warp-in)
		bool bWarpAnnounced = false;
	};

	void LoadWaveTable();
	void SetState(ETDWaveState NewState, float Duration = 0.f);
	void BeginWave(int32 Index, bool bIsRetake);
	void SpawnGroup(const FPendingGroup& Pending);
	ATDEnemyShip* SpawnEnemy(FName EnemyId, const FVector& Location, const FRotator& Facing);
	void RegisterEnemy(ATDEnemyShip* Enemy);
	void CheckWaveCleared();
	void OnWaveCleared();
	void RepairSquadHulls(float DeltaSeconds);
	void PickSpawnPoint(ETDSpawnZone Zone, bool bLarge, FVector& OutLocation, FRotator& OutFacing);
	AActor* GetSpawnAnchor() const;
	int32 ResolveTakeNumber(FName SceneId) const;
	void FireTips(const FTDWaveDef& Wave);

	UFUNCTION()
	void HandleEnemyDied(ATDEnemyShip* Ship, AActor* Killer);

	UFUNCTION()
	void HandleEnemyLaunched(ATDEnemyShip* Parent, ATDEnemyShip* Launched);

	UPROPERTY(Transient)
	TArray<FTDWaveDef> Waves;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATDEnemyShip>> Living;

	TArray<FPendingGroup> Pending;

	ETDWaveState State = ETDWaveState::Idle;
	float StateRemaining = 0.f;
	int32 CurrentWave = INDEX_NONE;
	int32 EnemiesTotal = 0;
	int32 CurrentTake = 1;
	bool bSwitchTipShown = false;
	FRandomStream Rng;
};
