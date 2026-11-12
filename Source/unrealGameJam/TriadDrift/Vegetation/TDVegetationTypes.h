// Triad Drift - procedural trees: botanical input data, crown-structure archetypes and the
// normalised tree parameters every generator backend consumes.
//
// Used by the `procedural-trees` skill (.claude/skills/procedural-trees): species data is looked up
// from botanical sources, UTDVegetationLibrary turns it into FTDTreeParams, and an editor Python
// script writes those into a Procedural Vegetation Editor (PVE) grower preset. Nothing here touches
// an editor module: the matcher is plain runtime-safe maths.

#pragma once

#include "CoreMinimal.h"
#include "TDVegetationTypes.generated.h"

/**
 * Crown-structure archetypes. Seven forms that between them cover the crown classes used in
 * horticulture and forestry field guides (pyramidal, columnar, spreading / vase, round, oval,
 * weeping, palm, shrub). Each one has hard-coded grower behaviour in UTDVegetationLibrary.
 */
UENUM(BlueprintType)
enum class ETDTreeArchetype : uint8
{
	/** Single straight leader, whorled near-horizontal branches, widest at the base (spruce, fir, young pine). */
	ConicalConifer,
	/** Single leader, steeply ascending branches, crown much taller than wide (Lombardy poplar, Italian cypress). */
	Columnar,
	/** Leader dissolves into a few heavy limbs; crown as wide as or wider than it is deep (oak, kapok, acacia, umbrella forms). */
	BroadSpreading,
	/** Rounded or egg-shaped crown on a clear trunk (maple, beech, most broadleaf forest trees). */
	RoundOval,
	/** Limbs arch out and hang down (weeping willow, weeping birch). */
	Weeping,
	/** Unbranched stem with one terminal rosette of large leaves (palms, tree ferns, cycads). */
	Palm,
	/** Several stems from the base, no dominant trunk (hazel, elder, most shrubs). */
	MultiStemShrub,

	Count UMETA(Hidden)
};

/** Crown outline as botanical descriptions give it. Unknown = the source did not say. */
UENUM(BlueprintType)
enum class ETDCrownShape : uint8
{
	Unknown,
	/** Conical / pyramidal. */
	Conical,
	/** Columnar / fastigiate / narrow. */
	Columnar,
	/** Spreading, vase, umbrella, flat-topped, pagoda. */
	Spreading,
	/** Round / globose. */
	Round,
	/** Oval / ovoid / egg-shaped. */
	Oval,
	/** Weeping / pendulous. */
	Weeping,
	/** One terminal tuft of leaves on an unbranched stem. */
	Rosette,
	/** Irregular / open: no outline to rely on. */
	Irregular
};

/** How the stem divides. Unknown = the source did not say. */
UENUM(BlueprintType)
enum class ETDBranchingHabit : uint8
{
	Unknown,
	/** One leader runs to the top (most conifers). */
	Excurrent,
	/** The leader divides into several large limbs (most broadleaf trees). */
	Decurrent,
	/** No branches at all (palms, tree ferns). */
	Unbranched,
	/** Several stems from the ground. */
	MultiStem,
	/** Branches hang. */
	Pendulous
};

UENUM(BlueprintType)
enum class ETDLeafType : uint8
{
	Unknown,
	/** Simple broad leaf. */
	Broadleaf,
	/** Compound broad leaf (palmate or pinnate leaflets on an ordinary branching tree). */
	CompoundBroadleaf,
	Needle,
	Scale,
	/** Palm or fern frond: one very large divided leaf growing from the stem tip. */
	Frond
};

UENUM(BlueprintType)
enum class ETDLeafPersistence : uint8
{
	Unknown,
	Evergreen,
	Deciduous
};

/**
 * What the botanical sources say about one species. A metric that is zero or negative, or an enum
 * that is Unknown, means "no usable data": the matcher then uses the archetype's default and
 * reports the field as defaulted. Never fill a gap with a guess here.
 */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDTreeSpeciesData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FString CommonName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FString LatinName;

	/** Typical mature height in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = 0))
	float HeightM = 0.f;

	/** Mature crown width (spread) in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = 0))
	float CrownWidthM = 0.f;

	/** Mature trunk diameter at breast height in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = 0))
	float TrunkDiameterM = 0.f;

	/** Fraction of the height that is branch-free trunk, 0..1. Negative = unknown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	float ClearBoleFraction = -1.f;

	/** Length of one leaf (or one frond for palms and tree ferns) in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = 0))
	float LeafLengthM = 0.f;

	/** Rosette forms only: how many fronds the crown carries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = 0))
	int32 CrownLeafCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	ETDCrownShape CrownShape = ETDCrownShape::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	ETDBranchingHabit Branching = ETDBranchingHabit::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	ETDLeafType LeafType = ETDLeafType::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	ETDLeafPersistence Persistence = ETDLeafPersistence::Unknown;
};

/** The hard-coded behaviour of one archetype: every value a species can fall back to. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDTreeArchetypeDefaults
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	ETDTreeArchetype Archetype = ETDTreeArchetype::RoundOval;

	/** Mature height used when the species has none, metres. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float HeightM = 18.f;

	/** Crown width as a fraction of height. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float CrownWidthRatio = 0.65f;

	/** Trunk diameter as a fraction of height. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float TrunkDiameterRatio = 0.035f;

	/** Height fraction where the crown starts (clear trunk below it). */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float CrownBaseFraction = 0.35f;

	/** 1 = one straight leader to the top, 0 = the leader dissolves into limbs. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float ApicalDominance = 0.45f;

	/** Angle between a main branch and the trunk axis: 0 = straight up, 90 = horizontal. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float BranchAngleDeg = 50.f;

	/** How far branches sag or hang under their own weight, 0..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float BranchDroop = 0.15f;

	/** How many of the possible side branches grow, 0..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float BranchDensity = 0.75f;

	/** Branching depth: 1 = trunk only, 2 = trunk and limbs, and so on. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	int32 BranchOrders = 4;

	/** 0 = pointed or domed top, 1 = flat-topped (lower limbs reach as far out as upper ones). */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float CrownFlatness = 0.2f;

	/** Stems rising from the ground. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	int32 StemCount = 1;

	/** Leaf (or frond) length, metres. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float LeafLengthM = 0.12f;

	/** Foliage density, 0..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float LeafDensity = 0.8f;

	/** Rosette forms: fronds in the crown. 0 for branching forms. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	int32 CrownLeafCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	ETDLeafType LeafType = ETDLeafType::Broadleaf;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	bool bEvergreen = false;
};

/**
 * Normalised parameters for one tree: the archetype's behaviour, adjusted by the species' data,
 * at one growth stage and seed. Generator backends (the PVE preset writer, or any other) read only
 * this struct, so the botanical side never depends on which generator is in use.
 */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDTreeParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	FString CommonName;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	FString LatinName;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	ETDTreeArchetype Archetype = ETDTreeArchetype::RoundOval;

	/** Why this archetype was chosen (the rule that fired). */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	FString ArchetypeReason;

	// ---- Size at this growth stage (metres) ----
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float HeightM = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float CrownWidthM = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float TrunkDiameterM = 0.f;

	/** Height of the same species when mature (GrowthStage 1, no seed jitter), metres. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float MatureHeightM = 0.f;

	// ---- Structure ----
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float CrownBaseFraction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float ApicalDominance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float BranchAngleDeg = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float BranchDroop = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float BranchDensity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	int32 BranchOrders = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float CrownFlatness = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	int32 StemCount = 1;

	// ---- Foliage ----
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	ETDLeafType LeafType = ETDLeafType::Broadleaf;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float LeafLengthM = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float LeafDensity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	int32 CrownLeafCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	bool bEvergreen = false;

	// ---- Variation ----
	/** 0 = sapling, 1 = mature. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	float GrowthStage = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	int32 Seed = 0;

	// ---- Report ----
	/** Names of the species fields that had no usable data and were taken from the archetype. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	TArray<FName> DefaultedFields;

	/** Values that were given but rejected as inconsistent, and why. */
	UPROPERTY(BlueprintReadOnly, Category = "Vegetation")
	TArray<FString> Notes;
};
