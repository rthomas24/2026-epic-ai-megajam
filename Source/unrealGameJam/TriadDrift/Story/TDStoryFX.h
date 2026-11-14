// Triad Drift - story presentation effects (engineering-owned). Owner: Teammate 3.
// The asset lives at /Game/TriadDrift/Gameplay/Characters/FX/DA_StoryFX (UTDStoryFX::AssetPath); the effects builder
// sets its cues. Every field is optional and null-safe: no asset, no cue, nothing plays.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TDStoryFX.generated.h"

UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDStoryFX : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Played once, attached to the player camera (no rotation), when a line with a static delivery starts:
	 *  A2_Voices, G6_Clue1, G6_Clue2 (NS_TD_Glitch_Tear, 0.4 s). Skipped with Reduce motion. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Story")
	FTDFXCue TransmissionStatic;

	static constexpr const TCHAR* AssetPath = TEXT("/Game/TriadDrift/Gameplay/Characters/FX/DA_StoryFX.DA_StoryFX");

	/** The asset, loaded once; null when it does not exist yet. */
	static const UTDStoryFX* Get();
};
