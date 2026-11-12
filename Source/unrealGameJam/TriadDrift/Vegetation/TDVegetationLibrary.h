// Triad Drift - procedural trees: species data -> crown archetype -> normalised tree parameters.
// Deterministic, allocation-light and safe at runtime (no editor module, no assets, no world).
// See .claude/skills/procedural-trees/SKILL.md for the pipeline this belongs to.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TriadDrift/Vegetation/TDVegetationTypes.h"
#include "TDVegetationLibrary.generated.h"

UCLASS()
class UNREALGAMEJAM_API UTDVegetationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Picks the crown archetype for a species. Rules are tried in a fixed order and the first that
	 * fires wins; OutReason says which one. With no usable data at all the answer is RoundOval.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Vegetation")
	static ETDTreeArchetype MatchTreeArchetype(const FTDTreeSpeciesData& Species, FString& OutReason);

	/** The hard-coded behaviour of one archetype. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Vegetation")
	static FTDTreeArchetypeDefaults GetTreeArchetypeDefaults(ETDTreeArchetype Archetype);

	/**
	 * The whole step: match the archetype, take its behaviour, adjust it with whatever the species
	 * data provides, reject values that contradict each other, then apply the growth stage
	 * (0 = sapling, 1 = mature) and the seed's small deterministic variation.
	 * The result lists every field that fell back to the archetype default.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Vegetation")
	static FTDTreeParams BuildTreeParams(const FTDTreeSpeciesData& Species, float GrowthStage = 1.f, int32 Seed = 0);

	/** Fraction of the mature height a tree has reached at a growth stage (0.12 for a sapling, 1 when mature). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Vegetation")
	static float GetHeightFractionAtGrowthStage(float GrowthStage);

	// ---- Limits used by the consistency checks (also asserted by the tests) ----
	/** Taller than any tree on record: a height above this is treated as a unit error. */
	static constexpr float MaxPlausibleHeightM = 130.f;
	/** A crown wider than this many times the height is rejected. */
	static constexpr float MaxCrownWidthToHeight = 3.f;
	/** A trunk thicker than this fraction of the height is rejected. */
	static constexpr float MaxTrunkDiameterToHeight = 0.5f;
	/** Height of a sapling as a fraction of the mature height. */
	static constexpr float SaplingHeightFraction = 0.12f;
};
