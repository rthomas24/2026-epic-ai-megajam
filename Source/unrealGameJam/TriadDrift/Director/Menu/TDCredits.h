// Triad Drift - the credits roll on the victory screen. Owner: Teammate 3.
//
// After THAT'S A WRAP and the run's stats, the credits roll slowly up inside T5's victory panel (the code-built
// victory screen; Nick's victory widget, when assigned, replaces the whole screen and this with it). The words come from
// Nicholas's DA_Credits (Presentation folder, a UTDCreditsData) when it has sections, else from the fallback below,
// which names only what CREDITS.md credits. The roll lives in the panel's widget tree: it goes when the panel does.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TDCredits.generated.h"

class SWidget;

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDCreditsSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits")
	FText Heading;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits")
	TArray<FText> Lines;
};

/** Nicholas's words for the credits (DA_Credits). Sections in order; the fallback is used when this has none. */
UCLASS(BlueprintType)
class UNREALGAMEJAM_API UTDCreditsData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits", meta = (TitleProperty = "Heading"))
	TArray<FTDCreditsSection> Sections;

	/** Scroll speed in pixels per second at 1080p. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits", meta = (ClampMin = "5"))
	float PixelsPerSecond = 30.f;
};

struct UNREALGAMEJAM_API FTDCredits
{
	/** The sections to show: DA_Credits' when it has any, else the fallback. */
	static TArray<FTDCreditsSection> GetSections();
	/** The engineering fallback (from CREDITS.md). */
	static const TArray<FTDCreditsSection>& Fallback();
	/** The roll: a clipped box the victory panel adds above RETURN TO MENU. */
	static TSharedRef<SWidget> MakeRoll(float Width = 620.f, float Height = 210.f);

#if WITH_DEV_AUTOMATION_TESTS
	static TWeakObjectPtr<const UTDCreditsData> OverrideForTests;
#endif
};
