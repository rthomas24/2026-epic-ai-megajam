// Triad Drift - game instance: progress, checkpoints, save slot, travel, scenes & takes (ENG-19, C-03 #3).
// Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Flow/TDSaveGame.h"
#include "TDGameInstance.generated.h"

class UTDEventBus;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTDOnSceneRestart, FName /*SceneId*/, int32 /*TakeNumber*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnCheckpointRestored, const FTDCheckpoint& /*Checkpoint*/);

/**
 * The run's single source of truth.
 *
 * Scenes and takes (C-03 #3): a scene is one wave or one objective. Whoever starts one calls
 * BeginScene(Id) (wave director, objective). ReportPlayerDefeated() is "CUT!": deaths++, the take
 * counter for the current scene goes up, and after RetakeDelaySeconds the last checkpoint is restored
 * and OnTakeStarted(SceneId, Take) fires on the event bus. C++ listeners that rebuild the scene
 * (game mode, wave director, objectives) bind OnSceneRestart.
 *
 * Takes and run stats survive a restore (they are the record of what happened); everything else
 * rolls back to the checkpoint.
 */
UCLASS(Config = Game)
class UTDGameInstance : public UGameInstance, public ITDGameFlow
{
	GENERATED_BODY()

public:
	static UTDGameInstance* Get(const UObject* WorldContext);

	// UGameInstance
	virtual void Init() override;
	virtual void Shutdown() override;

	// ---------------------------------------------------------------- ITDGameFlow
	virtual bool TravelTo(ETDDestination Destination, FName ReturnPoint) override;
	virtual FTDProgress GetProgress() const override;
	virtual void SetPhase(ETDGamePhase NewPhase) override;
	virtual void ReportWaveCleared(int32 WaveNumber) override;
	virtual void CompleteObjective(FName ObjectiveId, ETDAbilityId Reward, ETDShipId ForShip) override;
	virtual void SetDominionState(bool bRevealed, bool bDefensesDown) override;
	virtual void ReportPlayerDefeated() override;
	virtual void AddStat(FName StatName, int32 Amount) override;
	virtual void RequestBark(FName BarkId, FName Speaker) override;
	virtual void ShowTip(FName TipId) override;
	virtual void SaveCheckpoint() override;

	// ---------------------------------------------------------------- Run lifecycle
	/** Wipes progress and the in-memory checkpoint (does not touch the save slot). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void StartNewRun();

	/** True when the save slot holds a compatible run. Drives the main menu's Continue button. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	bool HasContinue() const;

	/** Loads the save slot and travels to the saved destination. False if there is nothing valid. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	bool ContinueRun();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void DeleteSave();

	/** Called by game modes from BeginPlay: records the destination and takes the level-entry checkpoint. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void NotifyLevelEntered(ETDDestination Destination);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	ETDDestination GetCurrentDestination() const { return CurrentDestination; }

	// ---------------------------------------------------------------- Scenes and takes (C-03 #3)
	/** Opens a scene. First time: take 1. Re-opening the current scene after a retake keeps its take. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Scenes")
	void BeginScene(FName SceneId);

	/** Closes the current scene (wave cleared / objective done). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Scenes")
	void EndScene();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Scenes")
	FName GetCurrentScene() const { return CurrentSceneId; }

	/** Current take of a scene (0 if never played). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Scenes")
	int32 GetTakeNumber(FName SceneId) const;

	/** 1-based slate number of a scene (0 if never played). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Scenes")
	int32 GetSceneNumber(FName SceneId) const;

	/** Victory screen: takes per scene, in play order. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Scenes")
	TArray<FTDSceneTakes> GetSceneTakes() const;

	/** True between "CUT!" and the restart. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Scenes")
	bool IsRetakePending() const { return bRetakePending; }

	/** Restore the checkpoint and start the next take immediately (skips RetakeDelaySeconds). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Scenes")
	void RestartSceneNow();

	FTDOnSceneRestart OnSceneRestart;
	FTDOnCheckpointRestored OnCheckpointRestored;

	// ---------------------------------------------------------------- Checkpoints & save slot
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	const FTDCheckpoint& GetLastCheckpoint() const { return LastCheckpoint; }

	/** Takes a checkpoint with a reason tag; writes the slot if configured. */
	void TakeCheckpoint(FName Reason);

	/** Writes the current checkpoint to the slot. */
	bool WriteSaveSlot() const;

	/** Reads the slot into OutSave. False if missing or a different version. */
	bool ReadSaveSlot(UTDSaveGame*& OutSave) const;

	/** Applies a save to this instance (used by ContinueRun and tests). */
	bool ApplySave(const UTDSaveGame& Save);

	/** Slot override for tests so they never touch the player's real save. */
	void SetSaveSlotOverride(const FString& SlotName) { SaveSlotOverride = SlotName; }
	FString GetSaveSlotName() const;

	// ---------------------------------------------------------------- Progress helpers
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	bool IsAbilityUnlocked(ETDAbilityId Ability) const;

	/** Unlocks an ability for a ship without an objective (cheats, tests). Fires OnAbilityUnlocked. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void UnlockAbility(ETDAbilityId Ability, ETDShipId ForShip);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void SetLastShip(ETDShipId Ship);

	/** Advances the run timer. Normally driven by the core ticker; public for deterministic tests. */
	void AdvanceRunTime(float DeltaSeconds);

	/** Pauses the run timer (menus, transitions, victory). */
	void SetRunTimerPaused(bool bPaused) { bRunTimerPaused = bPaused; }

	/** Read-only view without a copy (C++ callers on hot paths). */
	const FTDProgress& GetProgressRef() const { return Progress; }

	/** Mutable access for cheats and tests only. */
	FTDProgress& GetMutableProgress() { return Progress; }

	// Event bus listeners (public so tests can drive them directly).
	UFUNCTION()
	void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber, float Intensity01);

	UFUNCTION()
	void HandleWaveCleared(int32 WaveNumber);

	/** Scene id used for a wave (matches FTDWaveInfo::SceneId: "Wave3"). */
	static FName WaveSceneId(int32 WaveNumber);

private:
	void BindEventBus();
	bool TickRunTimer(float DeltaSeconds);
	void ExecuteRetake();
	void OpenDestination(ETDDestination Destination, const FString& PackageName);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	UTDEventBus* GetBus() const;

	UPROPERTY(Transient)
	FTDProgress Progress;

	UPROPERTY(Transient)
	FTDCheckpoint LastCheckpoint;

	/** Scene ids in first-played order. */
	UPROPERTY(Transient)
	TArray<FName> SceneOrder;

	FName CurrentSceneId;
	FName LastAnnouncedScene;
	int32 LastAnnouncedTake = 0;
	ETDDestination CurrentDestination = ETDDestination::MainMenu;
	ETDDestination PendingDestination = ETDDestination::MainMenu;
	bool bTravelPending = false;
	bool bRetakePending = false;
	bool bRunTimerPaused = false;
	FString SaveSlotOverride;

	FTimerHandle RetakeTimer;
	FTSTicker::FDelegateHandle RunTimerHandle;
	FDelegateHandle PostLoadMapHandle;
};
