// Triad Drift - game instance: progress, checkpoints, save slot, travel, scenes & takes (ENG-19, C-03 #3).
// Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "Templates/Function.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Flow/TDObjectiveText.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TriadDrift/Flow/TDSaveGame.h"
#include "TDGameInstance.generated.h"

class UTDEventBus;
class FTDDebugMenu;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTDOnSceneRestart, FName /*SceneId*/, int32 /*TakeNumber*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnCheckpointRestored, const FTDCheckpoint& /*Checkpoint*/);

/**
 * The run's single source of truth.
 *
 * Scenes and takes (C-03 #3): a scene is one wave or one objective. Whoever starts one calls
 * BeginScene(Id) (the wave director, objectives). ReportPlayerDefeated() is "CUT!":
 * - it fires OnSceneCut(Scene, NextTake, Delay) at once
 * - after RetakeDelaySeconds of REAL time (slow motion and pause don't stretch it), it restores the
 *   last checkpoint, bumps the scene's take and fires OnTakeStarted(Scene, Take)
 * - C++ listeners that rebuild the scene (game modes, the wave director, objectives) bind OnSceneRestart
 * A death outside any scene still cuts and restarts, with SceneId None and take 0.
 *
 * Takes, run stats and run time survive a restore: they are the record of what happened. Everything
 * else rolls back to the checkpoint.
 *
 * Save safety:
 * - The main menu and victory never checkpoint.
 * - A pristine run (nothing done yet) is never written.
 * - A fresh boot doesn't overwrite an existing save until the player starts a new run, continues,
 *   or makes real progress (a wave cleared or an objective completed).
 */
UCLASS(Config = Game)
class UNREALGAMEJAM_API UTDGameInstance : public UGameInstance, public ITDGameFlow
{
	GENERATED_BODY()

public:
	UTDGameInstance();
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
	/** How many objective-complete stings have played (one per objective, never on a repeat): tests. */
	int32 GetObjectiveCompleteSounds() const { return ObjectiveCompleteSounds; }
	virtual void SetDominionState(bool bRevealed, bool bDefensesDown) override;
	virtual void ReportPlayerDefeated() override;
	virtual void AddStat(FName StatName, int32 Amount) override;
	virtual void RequestBark(FName BarkId, FName Speaker) override;
	virtual void ShowTip(FName TipId) override;
	virtual void SaveCheckpoint() override;

	// ---------------------------------------------------------------- Run lifecycle
	/** Main menu "New Game": fresh progress; the run may now overwrite the save slot. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void StartNewRun();

	/** True when the save slot holds a compatible run (cached; refreshed on every write/delete). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	bool HasContinue() const;

	/**
	 * ENG-33: true from New Game until the galaxy's first StartPlay takes it (it plays the cinematic intro once).
	 * Continue, a loaded save and a reset clear it; retakes never set it. Returns the flag and clears it.
	 */
	bool ConsumeIntroPending();
	bool IsIntroPending() const { return bIntroPending; }

	/** Story transmissions (T3): played once per run, kept across retakes and in the checkpoint a Continue loads. */
	bool HasPlayedTransmission(FName Id) const { return Progress.StoryTransmissions.Contains(Id); }
	void MarkTransmissionPlayed(FName Id);
	void ForgetPlayedTransmissions();
	/** T3: an ambient line claimed when queued but cut off by a map travel before it aired: unclaim it (run and checkpoint). */
	void ForgetTransmission(FName Id);
	/** This run was resumed with Continue (onboarding's control lessons stay off). A New Game clears it. */
	bool IsRunFromContinue() const { return bRunFromContinue; }
	/** A bark played once per run (the first view on a planet), kept across retakes, reloads and staged starts in
	 *  this session; a new run clears it. True the first time it is claimed. */
	bool ClaimRunOnceBark(FName BarkId) { bool bAlready = false; RunOnceBarks.Add(BarkId, &bAlready); return !bAlready; }
	bool HasRunOnceBark(FName BarkId) const { return RunOnceBarks.Contains(BarkId); }
	TSet<FName> RunOnceBarks;
#if WITH_DEV_AUTOMATION_TESTS
	void SetRunFromContinueForTests(bool b) { bRunFromContinue = b; }
#endif

	/** Loads the save slot and travels to the saved destination. False if nothing valid or travel fails. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	bool ContinueRun();

	/** Changes at every run boundary (New Game, Continue, a run reset): anything kept across a map travel is keyed to
	 *  it, so one run's leftovers never reach another (T3's story carry, #139). */
	uint32 GetRunSerial() const { return RunSerial; }

	/** The run's difficulty: applied to the balance tables now and kept in the save slot (Continue restores it). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void SetDifficulty(ETDDifficulty Difficulty);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	ETDDifficulty GetDifficulty() const;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void DeleteSave();

	/** Called by game modes from StartPlay: records the destination, applies story transitions, checkpoints. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	void NotifyLevelEntered(ETDDestination Destination);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	ETDDestination GetCurrentDestination() const { return CurrentDestination; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	bool IsTravelPending() const { return TravelStage != ETravelStage::None; }
	/** T1, ENG-38: the pending travel goes under the white cover (no fade to black, no L_Transition). */
	bool IsCoveredTravel() const { return IsTravelPending() && bCoveredTravel; }

	/** May this run write the save slot yet? (see the class comment) */
	bool IsRunCommitted() const { return bRunCommitted; }

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
	bool IsRetakePending() const { return RetakeTimeLeft >= 0.f; }

	/** Victory wins over a CUT already counting down (the reactor died from a shot in flight after the wipe). */
	void CancelPendingRetake();

	/** The finale's scene id: the whole Dominion assault is one scene with takes. */
	static const FName DominionScene;

	/** Restore the checkpoint and start the next take immediately (skips RetakeDelaySeconds). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Scenes")
	void RestartSceneNow();

	FTDOnSceneRestart OnSceneRestart;
	FTDOnCheckpointRestored OnCheckpointRestored;

	// ---------------------------------------------------------------- Checkpoints & save slot
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	const FTDCheckpoint& GetLastCheckpoint() const { return LastCheckpoint; }

	/** Takes a checkpoint with a reason tag; writes the slot when allowed. */
	void TakeCheckpoint(FName Reason);

	/** Writes the current checkpoint to the slot (respects the save-safety rules unless bForce). */
	bool WriteSaveSlot(bool bForce = false);

	/** Reads the slot into OutSave. False if missing, foreign, corrupt or a different version. */
	bool ReadSaveSlot(UTDSaveGame*& OutSave) const;

	/** Applies a save to this instance (used by ContinueRun and tests). */
	bool ApplySave(const UTDSaveGame& Save);

	/** Slot override for tests so they never touch the player's real save. */
	void SetSaveSlotOverride(const FString& SlotName) { SaveSlotOverride = SlotName; bContinueCacheValid = false; }
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
	bool IsRunTimerPaused() const { return bRunTimerPaused; }

	/** Read-only view without a copy (C++ callers on hot paths). */
	const FTDProgress& GetProgressRef() const { return Progress; }

	/** Mutable access for cheats and tests only. */
	FTDProgress& GetMutableProgress() { return Progress; }

	/**
	 * Advances the real-time flow clock: CUT delay, travel fade, transition hold, travel watchdog.
	 * Driven by the core ticker (ignores pause and time dilation); tests call it directly.
	 */
	void TickFlow(float RealDeltaSeconds);

	/** Travel arrival hook (PostLoadMapWithWorld calls this with the loaded package name). */
	void NotifyMapLoaded(const FString& LoadedPackageName);

	/** Test hook: replaces UGameplayStatics::OpenLevel so travel can be exercised without loading maps. */
	TFunction<void(const FString& /*PackageName*/)> OpenLevelOverride;

	// Event bus listeners (public so tests can drive them directly).
	UFUNCTION()
	void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);

	UFUNCTION()
	void HandleShipSwitched(ETDShipId NewShip, ETDShipId OldShip);

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber, float Intensity01);

	UFUNCTION()
	void HandleWaveCleared(int32 WaveNumber);

	UFUNCTION()
	void HandleObjectiveUpdated(FName ObjectiveId, int32 Step, FText Text);

	/** The last objective line anyone broadcast (a HUD created late can read it). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flow")
	FText GetCurrentObjectiveText() const { return LastObjective.Text; }
	const FTDObjectiveLine& GetCurrentObjective() const { return LastObjective; }

	/**
	 * Broadcasts the flow's objective line (FTDObjectiveText) if it changed, or always when bForce (level entry,
	 * a new take) so a freshly created HUD is never empty. Called every 0.25 s from TickFlow.
	 */
	void RefreshObjective(bool bForce);

	/** Actors the flow currently tags TDTags::ObjectiveMarker (see FTDObjectiveText::ComputeMarkers). */
	TArray<AActor*> GetFlowMarkers() const;

	/** Fallback scene id for a wave ("Wave3") when the wave director gives none. The director owns the real id. */
	static FName WaveSceneId(int32 WaveNumber);

	/** Main menu and victory are not gameplay: no checkpoints, clock stopped. */
	static bool IsMenuDestination(ETDDestination Destination)
	{
		return Destination == ETDDestination::MainMenu || Destination == ETDDestination::Victory;
	}

private:
	enum class ETravelStage : uint8 { None, FadingOut, OnTransitionMap, Loading };

	void ResetRunState();
	uint32 RunSerial = 0;
	void BindEventBus();
	/** ENG-33: New Game plays the intro once (transient, never saved). */
	bool bIntroPending = false;
	bool bRunFromContinue = false;
	bool TickRunTimer(float DeltaSeconds);
	void ExecuteRetake();
	void BeginLoad(const FString& PackageName);
	void OpenLevelByName(const FString& PackageName);
	void FinishTravel(bool bArrived);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void ApplyStoryTransitionsOnEntry(ETDDestination Destination);
	bool IsPristine() const;
	void BroadcastPhase();
	UTDEventBus* GetBus() const;
	void TickDebugMenu();
public:
	/** In the galaxy during the assault: open the "Dominion" scene (once) and put it in the checkpoint. */
	void BeginDominionSceneIfAssault();
private:

	UPROPERTY(Transient)
	FTDProgress Progress;

	UPROPERTY(Transient)
	FTDCheckpoint LastCheckpoint;

	/** Scene ids in first-played order. */
	UPROPERTY(Transient)
	TArray<FName> SceneOrder;

	FTDObjectiveLine LastObjective;
	TArray<TWeakObjectPtr<AActor>> FlowMarkers;
	void SyncMarkers(bool bClearAll);
	float ObjectiveCheckLeft = 0.f;
	float VictoryTravelLeft = -1.f; // >= 0: travel to the victory screen when it runs out
	int32 VictoryTravelAttempts = 0;
	float ObjectiveForceLeft = -1.f; // >= 0: force a re-broadcast when it runs out (a HUD made on BeginPlay misses the first)

	FName CurrentSceneId;
	FName LastAnnouncedScene;
	int32 LastAnnouncedTake = 0;
	ETDDestination CurrentDestination = ETDDestination::MainMenu;
	bool bRunTimerPaused = false;
	bool bRunCommitted = false;
	FString SaveSlotOverride;

	// Real-time scheduling (FTSTicker).
	float RetakeTimeLeft = -1.f;
	FName RetakeScene;
	ETravelStage TravelStage = ETravelStage::None;
	float TravelStageTimeLeft = 0.f;
	bool bCoveredTravel = false; // T1, ENG-38: galaxy <-> planet under the white cover, no L_Transition
	float TravelElapsed = 0.f;
	ETDDestination PendingDestination = ETDDestination::MainMenu;
	FString PendingPackage;

	mutable bool bContinueCacheValid = false;
	mutable bool bContinueCache = false;

	FTSTicker::FDelegateHandle RunTimerHandle;
	FDelegateHandle PostLoadMapHandle;

	TSharedPtr<FTDDebugMenu> DebugMenu;
private:
	int32 ObjectiveCompleteSounds = 0;
};
