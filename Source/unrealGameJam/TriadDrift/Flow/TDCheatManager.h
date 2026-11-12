// Triad Drift - debug cheats (ENG-19 debug menu). Owner: Teammate 5. Not compiled into Shipping.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDCheatManager.generated.h"

/**
 * Console commands (also registered as "TD.*" console variables so they work without a cheat
 * manager on the controller):
 *   TD.Phase <ETDGamePhase>        jump to a phase
 *   TD.Unlock <ability> [ship]     unlock an ability (ship defaults to the ability's owner)
 *   TD.God                         toggle invulnerability for the player's team
 *   TD.KillAll                     kill every enemy-team actor
 *   TD.SkipWave                    skip the current wave
 *   TD.Travel <ETDDestination>     travel to a destination
 *   TD.Take                        force "CUT!" and a retake of the current scene
 *   TD.Stage <ETDJourneyStage>     jump to a journey stage (Waves1to2 ... Victory): progress and world
 *   TD.Unlock All                  every ability on its owner ship
 * The static Do* functions hold the logic so tests and the F10 debug menu can call them directly.
 */
UCLASS()
class UNREALGAMEJAM_API UTDCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec) void TDPhase(const FString& PhaseName);
	UFUNCTION(Exec) void TDUnlock(const FString& AbilityName, const FString& ShipName);
	UFUNCTION(Exec) void TDGod();
	UFUNCTION(Exec) void TDKillAll();
	UFUNCTION(Exec) void TDSkipWave();
	UFUNCTION(Exec) void TDTravel(const FString& DestinationName);
	UFUNCTION(Exec) void TDTake();
	UFUNCTION(Exec) void TDStage(const FString& StageName);

	static bool DoPhase(UWorld* World, const FString& PhaseName);
	static bool DoUnlock(UWorld* World, const FString& AbilityName, const FString& ShipName);
	static bool DoGod(UWorld* World);
	static int32 DoKillAll(UWorld* World);
	static bool DoSkipWave(UWorld* World);
	static bool DoTravel(UWorld* World, const FString& DestinationName);
	static bool DoTake(UWorld* World);
	/** Jump to a journey stage: progress AND world (FTDJourney). */
	static bool DoStage(UWorld* World, const FString& StageName);

	/**
	 * Makes UTDCheatManager the controller's cheat manager (replacing the stock one the engine creates
	 * in standalone). Dev builds only; no-op in Shipping or if already installed.
	 */
	static void InstallOn(APlayerController* PC);

	/** Current god-mode state (shared between the console command and the cheat manager). */
	static bool IsGodModeOn() { return bGodMode; }

private:
	static bool bGodMode;
};
