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
class UNREALGAMEJAM_API ATDSpaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDSpaceGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	/** Keeps the star backdrop centred on the camera, like a skybox (the galaxy is bigger than the shell). */
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Where the squad spawns and where retakes put it back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	FTransform SquadOrigin;

	/** Build the galaxy with T4's ATDGalaxyLayout (asteroids, planets, gates, Dominion, return points). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	bool bUseGalaxyLayout = true;

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

	/** The placeholder star backdrop (null once a real sky has taken over, or if none was needed). */
	AActor* GetStarBackdrop() const { return StarBackdrop; }
	UMaterialInterface* GetStarMaterial() const { return StarGlowMaterial; }

protected:
	/** A sky tagged TD.Presentation.Sky that appears late (streamed / spawned) removes our stars too. */
	void CheckStarBackdrop();
	void SpawnEnvironment();
	void SpawnStarBackdrop();
	void SpawnAsteroids();
	void SpawnSquadManager();
	void SpawnWaveDirector();
	void StartWaves();
	/** After StartPlay hands the squad over: the intro on New Game, the phase, NotifyLevelEntered, the first waves. */
	void EnterLevel();
	/** ENG-33: Intro -> Waves1to2 and the first-wave timer (after the intro, or at once without it). */
	void BeginFirstWaves();
	void ArmFirstWaveTimer();
	void PossessActiveShip(APlayerController* PC);
	/** Install cheats, give every player the squad's active ship, destroy stray login pawns. */
	void HandOverPlayers();
	void ApplyStartingShip();
	/** Where the squad starts: the Dominion approach / trench during the assault, else the progress return point. */
	FTransform ResolveSquadOrigin() const;
	void HandleSceneRestart(FName SceneId, int32 TakeNumber);

	UPROPERTY(Transient)
	TObjectPtr<AActor> SquadManager;

	UPROPERTY(Transient)
	TObjectPtr<AActor> WaveDirector;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedEnvironment;

	UPROPERTY(Transient)
	TObjectPtr<AActor> FallbackStart;

	UPROPERTY(Transient)
	TObjectPtr<class ATDGalaxyLayout> GalaxyLayout;

	UPROPERTY(Transient)
	TObjectPtr<AActor> StarBackdrop;

	/** Unlit glow material for the stars (Core's placeholder glow: instanced-mesh usage, always cooked). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> StarGlowMaterial;

	FTimerHandle FirstWaveTimer;
	/** ENG-33: the run timer's state when the intro paused it (restored at the first waves). */
	bool bRunTimerPausedBeforeIntro = false;
public:
	/** Tests: enter the level again (what StartPlay does after the squad is handed over: intro or first waves). */
	void EnterLevelForTests() { EnterLevel(); }
	/** Tests: the first-wave timer is armed. */
	bool IsFirstWaveTimerArmed() const { return GetWorldTimerManager().IsTimerActive(FirstWaveTimer); }
protected:
	FTimerHandle StarWatchTimer;
	FDelegateHandle SceneRestartHandle;
};
