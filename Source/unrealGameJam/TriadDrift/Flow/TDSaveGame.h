// Triad Drift - save slot (ENG-19). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Waves/Balance/TDDifficulty.h"
#include "TDSaveGame.generated.h"

/** One checkpoint: everything needed to resume or to restart the current scene as a new take. */
USTRUCT(BlueprintType)
struct FTDCheckpoint
{
	GENERATED_BODY()

	/** Progress at the moment the checkpoint was taken. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Checkpoint")
	FTDProgress Progress;

	/** Where the player was. Continue travels here. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Checkpoint")
	ETDDestination Destination = ETDDestination::Galaxy;

	/** The scene (wave or objective) that was running. NAME_None between scenes. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Checkpoint")
	FName SceneId;

	/** Why it was taken: LevelEntry, WaveCleared, ObjectiveComplete, Manual. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Checkpoint")
	FName Reason;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Checkpoint")
	bool bValid = false;
};

/** Takes needed per scene, in the order scenes were first played (victory screen, C-03 #3). */
USTRUCT(BlueprintType)
struct FTDSceneTakes
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Scenes")
	FName SceneId;

	/** 1-based scene number as shown on the slate ("SCENE 3"). */
	UPROPERTY(BlueprintReadOnly, Category = "Scenes")
	int32 SceneNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Scenes")
	int32 Takes = 0;
};

/** The single run save. Versioned: a slot with a different version is rejected, never half-loaded. */
UCLASS(BlueprintType)
class UTDSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bump when the layout of FTDProgress / FTDCheckpoint changes incompatibly. */
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	int32 Version = CurrentVersion;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FTDCheckpoint Checkpoint;

	/** Scene play order, so the victory screen lists scenes chronologically. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	TArray<FName> SceneOrder;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	FDateTime SavedAtUtc;

	/** The run's difficulty (Story / Standard / Ace). Older slots without it load as Standard. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Save")
	ETDDifficulty Difficulty = ETDDifficulty::Standard;

	bool IsCompatible() const { return Version == CurrentVersion && Checkpoint.bValid; }
};
