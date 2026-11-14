// Triad Drift - the player profile: first-play tutorial flag and "skip the tutorial" (Teammate 7, from T3's queue).
//
// Nicholas (storyline.md, reconciliation of Nov 13): the tutorial battle is required on a first playthrough; once it has
// been completed, a later New Game may skip it and start at *Aftermath*.
//
// - The flag lives in its own save slot ("TD_Profile"), separate from the run save (one slot per run) and from the hint
//   profile (TD_Hints). Deleting the run save never forgets it.
// - Completion: NotifyTutorialCompleted(), which T4's ATDWaveDirector::OnTutorialEscaped should call (the escape boundary
//   crossed during Command's ordered withdrawal). Until that event exists, the run leaving Waves1to2 for a later phase
//   (the same fallback T1's tutorial music uses) counts. Cheat stages (TD.Stage) start a new run first and never count.
// - Skip: StartNewRunSkippingTutorial() = a New Game whose opening cinematic plays (or is skipped) as today, then lands
//   in the state a finished tutorial leaves: in the galaxy, waves 1-2 cleared, phase Verdance, nothing unlocked (the
//   abilities are earned on the planets), the squad at the player start. OnAftermathBegins fires for the director.
// - The game mode leaves the intro through LeaveIntro(), which applies a pending skip or sets Waves1to2 as before.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDProfileSubsystem.generated.h"

class UTDGameInstance;

/** The profile slot. Versioned like the run save: another version is ignored, never half-loaded. */
UCLASS()
class UNREALGAMEJAM_API UTDProfileSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 Version = CurrentVersion;

	/** The tutorial battle was finished at least once on this profile. */
	UPROPERTY(SaveGame)
	bool bTutorialComplete = false;

	/** How many times it was finished (stats; the menu only needs the flag). */
	UPROPERTY(SaveGame)
	int32 TutorialCompletions = 0;

	UPROPERTY(SaveGame)
	FDateTime TutorialFirstCompletedUtc;
};

/** Fired when the run reaches *Aftermath*: after the escape (bSkippedTutorial false) or a skip (true). */
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnAftermathBegins, bool /*bSkippedTutorial*/);

UCLASS()
class UNREALGAMEJAM_API UTDProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UTDProfileSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------------------------------------------------------------- The flag
	/** The tutorial was finished on this profile. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Profile")
	bool IsTutorialComplete() const { return bTutorialComplete; }

	/** The menu may offer "Skip the tutorial" (the tutorial was finished before). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Profile")
	bool CanSkipTutorial() const { return bTutorialComplete; }

	/**
	 * The tutorial battle is complete (T4's escape event calls this). Sets and writes the flag (idempotent: the first
	 * call of a run counts, later ones do nothing) and fires OnAftermathBegins(false). Why is logged.
	 */
	void NotifyTutorialCompleted(const TCHAR* Why = TEXT("escape boundary"));

	/** Forget the flag (dev: TD.Profile reset; tests). Writes the slot. */
	void ResetProfile();

	// ---------------------------------------------------------------- Skip
	/**
	 * A New Game that skips the tutorial: UTDGameInstance::StartNewRun(), then the skip is armed. The caller travels to
	 * the galaxy (ATDMenuGameMode::NewGameSkipTutorial does). False, and nothing changes, when the tutorial was never
	 * completed or there is no Triad Drift game instance.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Profile")
	bool StartNewRunSkippingTutorial();

	/** A skip is armed for this run and has not landed yet. */
	bool IsSkipTutorialPending() const { return bSkipPending; }

	/** This run started with a skip (until the next New Game). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Profile")
	bool DidRunSkipTutorial() const { return bRunSkipped; }

	/**
	 * The galaxy game mode leaves the intro (watched, skipped or not played) through this. With a skip armed: the run
	 * lands at *Aftermath* (see the file comment) and true is returned; otherwise SetPhase(Waves1to2), false.
	 */
	static bool LeaveIntro(UTDGameInstance* GI);

	/** UTDGameInstance::StartNewRun: a fresh run, nothing armed. */
	void HandleNewRun();

	// ---------------------------------------------------------------- Aftermath hook (T3's director)
	FTDOnAftermathBegins OnAftermathBegins;

	/** *Aftermath* is due and nobody has taken it yet; returns it and clears it (a director created late). */
	bool ConsumeAftermathPending();
	bool IsAftermathPending() const { return bAftermathPending; }

	// ---------------------------------------------------------------- Persistence
	FString GetSlotName() const;
	/** Tests: use this slot (and persist to it even under automation), then reload from it. Empty = default. */
	void SetSlotOverrideForTests(const FString& Slot);
	/** Reads the slot into the flag (missing, foreign, corrupt or another version = a fresh profile). */
	void Load();
	bool Save() const;

	/** Phase changes (the event bus). Public for tests. */
	UFUNCTION()
	void HandlePhaseChanged(ETDGamePhase Phase);

private:
	void ApplySkip(UTDGameInstance* GI);
	bool ShouldPersist() const;

	bool bTutorialComplete = false;
	int32 TutorialCompletions = 0;
	FDateTime FirstCompletedUtc;

	bool bSkipPending = false;
	bool bRunSkipped = false;
	bool bCompletedThisRun = false;
	bool bAftermathPending = false;
	ETDGamePhase LastPhase = ETDGamePhase::Intro;
	FString SlotOverride;
	bool bBusBound = false;
};
