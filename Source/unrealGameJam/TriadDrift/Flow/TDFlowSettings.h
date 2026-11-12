// Triad Drift - game flow settings (ENG-19). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDFlowSettings.generated.h"

/**
 * Project Settings > Game > Triad Drift Flow.
 * Destination -> map table, transition/fade timing, save slot, scene/take timing and the class
 * paths the zero-asset sandbox looks up at runtime. Maps that don't exist yet are fine: travel logs a
 * warning and stays put.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Triad Drift Flow"))
class UTDFlowSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTDFlowSettings();

	static const UTDFlowSettings* Get() { return GetDefault<UTDFlowSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Map loaded for each destination. A missing entry or a map that doesn't exist fails safely. */
	UPROPERTY(Config, EditAnywhere, Category = "Travel", meta = (AllowedClasses = "/Script/Engine.World"))
	TMap<ETDDestination, FSoftObjectPath> DestinationMaps;

	/** Loading-screen level shown between destinations (Nick's widget lives there). Optional. */
	UPROPERTY(Config, EditAnywhere, Category = "Travel", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath TransitionMap;

	/** Fade to black before the level change. */
	UPROPERTY(Config, EditAnywhere, Category = "Travel", meta = (ClampMin = "0.0", Units = "s"))
	float TravelFadeSeconds = 0.6f;

	/** Minimum time on the transition map so the loading screen can breathe. */
	UPROPERTY(Config, EditAnywhere, Category = "Travel", meta = (ClampMin = "0.0", Units = "s"))
	float MinTransitionSeconds = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Save")
	FString SaveSlotName = TEXT("TriadDrift_Run");

	UPROPERTY(Config, EditAnywhere, Category = "Save")
	int32 SaveUserIndex = 0;

	/** Write the save slot every time a checkpoint is taken (level entry, wave clear, objective). */
	UPROPERTY(Config, EditAnywhere, Category = "Save")
	bool bWriteSlotOnCheckpoint = true;

	/** C-03 #3: time between "CUT!" (player defeated) and the scene restarting as the next take. */
	UPROPERTY(Config, EditAnywhere, Category = "Scenes and takes", meta = (ClampMin = "0.0", Units = "s"))
	float RetakeDelaySeconds = 1.75f;

	/** Which ship owns each unlockable ability (features.md §0 player journey). */
	UPROPERTY(Config, EditAnywhere, Category = "Progression")
	TMap<ETDAbilityId, ETDShipId> AbilityOwners;

	// Sandbox class lookups (resolved at runtime so Flow compiles before the other areas merge).
	UPROPERTY(Config, EditAnywhere, Category = "Sandbox", meta = (MetaClass = "/Script/Engine.PlayerController"))
	FSoftClassPath SpacePlayerControllerClass;

	UPROPERTY(Config, EditAnywhere, Category = "Sandbox", meta = (MetaClass = "/Script/Engine.Actor"))
	FSoftClassPath SquadManagerClass;

	UPROPERTY(Config, EditAnywhere, Category = "Sandbox", meta = (MetaClass = "/Script/Engine.Actor"))
	FSoftClassPath WaveDirectorClass;

	/** Returns the map for a destination if it is configured AND exists on disk. */
	bool ResolveDestinationMap(ETDDestination Destination, FString& OutPackageName) const;
};
