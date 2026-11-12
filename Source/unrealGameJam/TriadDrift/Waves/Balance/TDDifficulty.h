// Triad Drift - Enemies & Waves (Teammate 4). ENG-27 difficulty presets.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TDDifficulty.generated.h"

UENUM(BlueprintType)
enum class ETDDifficulty : uint8
{
	Story,
	Standard,
	Ace
};

/** How one difficulty scales the Armada. 1.0 everywhere = the features.md numbers. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDifficultyScale
{
	GENERATED_BODY()

	/** Enemy hull, shields and the gunship frontal shield. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty", meta = (ClampMin = "0.1"))
	float EnemyHull = 1.f;

	/** Damage of every enemy weapon (ships and turrets). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty", meta = (ClampMin = "0"))
	float EnemyDamage = 1.f;

	/** Enemy aim error (higher = misses more). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty", meta = (ClampMin = "0"))
	float EnemyAimError = 1.f;

	/** Pack sizes / small-ship pack counts in every wave (rounded, never below 1). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty", meta = (ClampMin = "0.1"))
	float WaveSize = 1.f;
};

/**
 * Difficulty presets (Project Settings > Game > Triad Drift Difficulty, DefaultGame.ini). The current
 * difficulty is runtime state: the front end calls SetCurrent; enemy specs and the wave director read
 * GetScale() when enemies / waves are created.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Triad Drift Difficulty"))
class UNREALGAMEJAM_API UTDDifficultySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTDDifficultySettings();

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty")
	FTDDifficultyScale Story;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty")
	FTDDifficultyScale Standard;

	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty")
	FTDDifficultyScale Ace;

	/** Difficulty a new run starts on. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Difficulty")
	ETDDifficulty DefaultDifficulty = ETDDifficulty::Standard;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Difficulty")
	static ETDDifficulty GetCurrent();

	/** Takes effect for enemies and waves spawned from now on. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Difficulty")
	static void SetCurrent(ETDDifficulty Difficulty);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Difficulty")
	static FTDDifficultyScale GetScale();

	static const FTDDifficultyScale& GetScaleFor(ETDDifficulty Difficulty);

	/** Scale a group size (wave pack) by the current WaveSize; never below 1. */
	static int32 ScaleCount(int32 Count, float Scale) { return FMath::Max(1, FMath::RoundToInt(Count * Scale)); }

private:
	static ETDDifficulty& CurrentRef();
};
