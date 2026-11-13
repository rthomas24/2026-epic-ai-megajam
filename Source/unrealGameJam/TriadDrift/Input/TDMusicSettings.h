// Triad Drift - combat music tuning (Project Settings > Game > Triad Drift Music, DefaultGame.ini). Owner: Teammate 1.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TDMusicSettings.generated.h"

/**
 * N-037 (Nick's Audio, approved by the Lead): the combat music's volume follows the fight. The music director
 * multiplies its fade envelope by a gain from UTDAudioStateSubsystem's smoothed MusicIntensity01: CalmGain at
 * intensity 0, FightGain at 1. 1.0 / 1.0 turns it off (the authored volume throughout).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Triad Drift Music"))
class UNREALGAMEJAM_API UTDMusicSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Music gain at intensity 0 (a lull between packs). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Intensity", meta = (ClampMin = "0", ClampMax = "1"))
	float CalmGain = 0.55f;

	/** Music gain at intensity 1 (a full fight): the authored volume. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Intensity", meta = (ClampMin = "0", ClampMax = "1"))
	float FightGain = 1.f;

	/** The gain never moves faster than this (per second), on top of the intensity's own smoothing: no clicks. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Intensity", meta = (ClampMin = "0.01"))
	float MaxGainRatePerSecond = 0.5f;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	static const UTDMusicSettings* Get() { return GetDefault<UTDMusicSettings>(); }
};
