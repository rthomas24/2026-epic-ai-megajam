// Triad Drift - zero-asset playable space sandbox (ENG-19). Owner: Teammate 5.
//
// UnrealEditor.exe <uproject> /Engine/Maps/Entry?game=/Script/unrealGameJam.TDSpaceGameMode -game -windowed

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDSpaceGameMode.generated.h"

/**
 * Makes any map playable with no content: lighting, a star backdrop and asteroids from engine basic
 * shapes, plus the squad manager (T3) and wave director (T4), which are resolved by class path at
 * runtime. If one isn't built yet the sandbox logs it and keeps going (a free-fly pawn stands in for
 * the squad). Also rebuilds the scene when the game instance calls for a new take (C-03 #3).
 */
UCLASS(Config = Game)
class ATDSpaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDSpaceGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Where the squad spawns and where retakes put it back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	FTransform SquadOrigin;

	/** Spawn lights / post process / stars if the map has none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	bool bSpawnEnvironment = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (ClampMin = "0"))
	int32 StarCount = 1200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (ClampMin = "1000.0", Units = "cm"))
	float StarSphereRadius = 400000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (ClampMin = "0"))
	int32 AsteroidCount = 24;

	/** Asteroids are scattered in a shell between these radii around the squad origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (Units = "cm"))
	FVector2D AsteroidRadiusRange = FVector2D(6000.f, 30000.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (Units = "cm"))
	FVector2D AsteroidScaleRange = FVector2D(4.f, 22.f);

	/** Deterministic layout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	int32 LayoutSeed = 1312;

	/** Delay between the squad spawning and the first wave, so the player gets their bearings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (ClampMin = "0.0", Units = "s"))
	float FirstWaveDelaySeconds = 3.f;

	/** Which destination this mode represents for checkpoints and travel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	ETDDestination Destination = ETDDestination::Galaxy;

	UFUNCTION(BlueprintPure, Category = "Sandbox")
	AActor* GetSquadManager() const { return SquadManager; }

	UFUNCTION(BlueprintPure, Category = "Sandbox")
	AActor* GetWaveDirector() const { return WaveDirector; }

	/** Rebuilds the scene for a new take: respawns the squad at the origin and restarts the wave. */
	UFUNCTION(BlueprintCallable, Category = "Sandbox")
	void RestartScene();

protected:
	void SpawnEnvironment();
	void SpawnStarBackdrop();
	void SpawnAsteroids();
	void SpawnSquadManager();
	void SpawnWaveDirector();
	void StartWaves();
	void PossessActiveShip(APlayerController* PC);
	void HandleSceneRestart(FName SceneId, int32 TakeNumber);

	UPROPERTY(Transient)
	TObjectPtr<AActor> SquadManager;

	UPROPERTY(Transient)
	TObjectPtr<AActor> WaveDirector;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedEnvironment;

	FTimerHandle FirstWaveTimer;
	FDelegateHandle SceneRestartHandle;
};
