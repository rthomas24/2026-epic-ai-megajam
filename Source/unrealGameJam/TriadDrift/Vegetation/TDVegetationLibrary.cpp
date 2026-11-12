// Triad Drift - procedural trees: archetype matcher. See TDVegetationLibrary.h.

#include "TriadDrift/Vegetation/TDVegetationLibrary.h"

#include "Math/RandomStream.h"

namespace TDVegetation
{
	/** One row of the archetype table. Kept as plain data so the whole table reads at a glance. */
	struct FArchetypeRow
	{
		ETDTreeArchetype Archetype;
		float HeightM;
		float CrownWidthRatio;
		float TrunkDiameterRatio;
		float CrownBaseFraction;
		float ApicalDominance;
		float BranchAngleDeg;
		float BranchDroop;
		float BranchDensity;
		int32 BranchOrders;
		float CrownFlatness;
		int32 StemCount;
		float LeafLengthM;
		float LeafDensity;
		int32 CrownLeafCount;
		ETDLeafType LeafType;
		bool bEvergreen;
	};

	// Height  Crown/H  Trunk/H  CrownBase  Apical  Angle  Droop  Density  Orders  Flat  Stems  Leaf   LeafDens  Fronds
	static const FArchetypeRow GArchetypeTable[] =
	{
		{ ETDTreeArchetype::ConicalConifer, 25.f, 0.30f, 0.025f, 0.15f, 0.95f, 80.f, 0.25f, 0.90f, 3, 0.00f, 1, 0.20f, 0.90f, 0,  ETDLeafType::Needle,    true  },
		{ ETDTreeArchetype::Columnar,       18.f, 0.18f, 0.025f, 0.10f, 0.85f, 20.f, 0.00f, 0.80f, 3, 0.00f, 1, 0.10f, 0.80f, 0,  ETDLeafType::Broadleaf, false },
		{ ETDTreeArchetype::BroadSpreading, 22.f, 1.00f, 0.045f, 0.45f, 0.25f, 65.f, 0.20f, 0.60f, 4, 0.70f, 1, 0.15f, 0.70f, 0,  ETDLeafType::Broadleaf, false },
		{ ETDTreeArchetype::RoundOval,      18.f, 0.65f, 0.035f, 0.35f, 0.45f, 50.f, 0.15f, 0.75f, 4, 0.20f, 1, 0.12f, 0.80f, 0,  ETDLeafType::Broadleaf, false },
		{ ETDTreeArchetype::Weeping,        12.f, 0.80f, 0.040f, 0.30f, 0.35f, 60.f, 0.90f, 0.80f, 4, 0.30f, 1, 0.10f, 0.80f, 0,  ETDLeafType::Broadleaf, false },
		{ ETDTreeArchetype::Palm,           12.f, 0.35f, 0.020f, 0.90f, 1.00f, 60.f, 0.50f, 0.00f, 1, 0.00f, 1, 2.50f, 0.50f, 12, ETDLeafType::Frond,     true  },
		{ ETDTreeArchetype::MultiStemShrub,  3.f, 1.00f, 0.020f, 0.05f, 0.10f, 40.f, 0.10f, 0.90f, 3, 0.30f, 5, 0.08f, 0.90f, 0,  ETDLeafType::Broadleaf, false },
	};

	static const FArchetypeRow& FindRow(ETDTreeArchetype Archetype)
	{
		for (const FArchetypeRow& Row : GArchetypeTable)
		{
			if (Row.Archetype == Archetype)
			{
				return Row;
			}
		}
		return GArchetypeTable[3]; // RoundOval: the fallback form
	}

	/** A shrub is at most this tall when nothing else says what the plant is. */
	static constexpr float ShrubMaxHeightM = 4.f;
	/** Crown narrower than this fraction of the height reads as columnar. */
	static constexpr float ColumnarMaxAspect = 0.25f;
	/** Crown at least this wide relative to the height reads as spreading. */
	static constexpr float SpreadingMinAspect = 0.8f;

	static bool IsUsable(float Value)
	{
		return FMath::IsFinite(Value) && Value > 0.f;
	}
}

ETDTreeArchetype UTDVegetationLibrary::MatchTreeArchetype(const FTDTreeSpeciesData& Species, FString& OutReason)
{
	using namespace TDVegetation;

	const bool bHasHeight = IsUsable(Species.HeightM) && Species.HeightM <= MaxPlausibleHeightM;
	const bool bHasAspect = bHasHeight && IsUsable(Species.CrownWidthM) && Species.CrownWidthM <= Species.HeightM * MaxCrownWidthToHeight;
	const float Aspect = bHasAspect ? Species.CrownWidthM / Species.HeightM : 0.f;

	// 1. Rosette forms are unmistakable and nothing else can build them.
	if (Species.LeafType == ETDLeafType::Frond || Species.CrownShape == ETDCrownShape::Rosette || Species.Branching == ETDBranchingHabit::Unbranched)
	{
		OutReason = TEXT("frond leaves, a rosette crown or an unbranched stem");
		return ETDTreeArchetype::Palm;
	}
	// 2. Several stems from the ground.
	if (Species.Branching == ETDBranchingHabit::MultiStem)
	{
		OutReason = TEXT("multi-stem branching habit");
		return ETDTreeArchetype::MultiStemShrub;
	}
	// 3. Hanging branches.
	if (Species.CrownShape == ETDCrownShape::Weeping || Species.Branching == ETDBranchingHabit::Pendulous)
	{
		OutReason = TEXT("weeping crown or pendulous branches");
		return ETDTreeArchetype::Weeping;
	}
	// 4. Stated crown outlines.
	if (Species.CrownShape == ETDCrownShape::Columnar)
	{
		OutReason = TEXT("columnar crown shape");
		return ETDTreeArchetype::Columnar;
	}
	if (Species.CrownShape == ETDCrownShape::Conical)
	{
		OutReason = TEXT("conical crown shape");
		return ETDTreeArchetype::ConicalConifer;
	}
	if (Species.CrownShape == ETDCrownShape::Spreading)
	{
		OutReason = TEXT("spreading crown shape");
		return ETDTreeArchetype::BroadSpreading;
	}
	if (Species.CrownShape == ETDCrownShape::Round || Species.CrownShape == ETDCrownShape::Oval)
	{
		OutReason = TEXT("round or oval crown shape");
		return ETDTreeArchetype::RoundOval;
	}
	// 5. No outline given (Unknown or Irregular): infer from leaves and proportions.
	if ((Species.LeafType == ETDLeafType::Needle || Species.LeafType == ETDLeafType::Scale) && Species.Branching != ETDBranchingHabit::Decurrent)
	{
		OutReason = TEXT("needle or scale leaves without a decurrent habit");
		return ETDTreeArchetype::ConicalConifer;
	}
	if (bHasAspect && Aspect < ColumnarMaxAspect)
	{
		OutReason = FString::Printf(TEXT("crown width is %.2f of the height (narrower than %.2f)"), Aspect, ColumnarMaxAspect);
		return ETDTreeArchetype::Columnar;
	}
	if (bHasHeight && Species.HeightM <= ShrubMaxHeightM && Species.Branching == ETDBranchingHabit::Unknown)
	{
		OutReason = FString::Printf(TEXT("no habit given and only %.1f m tall"), Species.HeightM);
		return ETDTreeArchetype::MultiStemShrub;
	}
	if (bHasAspect && Aspect >= SpreadingMinAspect)
	{
		OutReason = FString::Printf(TEXT("crown width is %.2f of the height (at least %.2f)"), Aspect, SpreadingMinAspect);
		return ETDTreeArchetype::BroadSpreading;
	}
	if (bHasAspect)
	{
		OutReason = FString::Printf(TEXT("crown width is %.2f of the height"), Aspect);
		return ETDTreeArchetype::RoundOval;
	}
	OutReason = TEXT("no crown shape, habit or proportions given: fallback form");
	return ETDTreeArchetype::RoundOval;
}

FTDTreeArchetypeDefaults UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype Archetype)
{
	const TDVegetation::FArchetypeRow& Row = TDVegetation::FindRow(Archetype);
	FTDTreeArchetypeDefaults Out;
	Out.Archetype = Row.Archetype;
	Out.HeightM = Row.HeightM;
	Out.CrownWidthRatio = Row.CrownWidthRatio;
	Out.TrunkDiameterRatio = Row.TrunkDiameterRatio;
	Out.CrownBaseFraction = Row.CrownBaseFraction;
	Out.ApicalDominance = Row.ApicalDominance;
	Out.BranchAngleDeg = Row.BranchAngleDeg;
	Out.BranchDroop = Row.BranchDroop;
	Out.BranchDensity = Row.BranchDensity;
	Out.BranchOrders = Row.BranchOrders;
	Out.CrownFlatness = Row.CrownFlatness;
	Out.StemCount = Row.StemCount;
	Out.LeafLengthM = Row.LeafLengthM;
	Out.LeafDensity = Row.LeafDensity;
	Out.CrownLeafCount = Row.CrownLeafCount;
	Out.LeafType = Row.LeafType;
	Out.bEvergreen = Row.bEvergreen;
	return Out;
}

float UTDVegetationLibrary::GetHeightFractionAtGrowthStage(float GrowthStage)
{
	const float Stage = FMath::IsFinite(GrowthStage) ? FMath::Clamp(GrowthStage, 0.f, 1.f) : 1.f;
	// Fast early height growth that levels off, as in real trees.
	return FMath::Lerp(SaplingHeightFraction, 1.f, FMath::Pow(Stage, 0.75f));
}

FTDTreeParams UTDVegetationLibrary::BuildTreeParams(const FTDTreeSpeciesData& Species, float GrowthStage, int32 Seed)
{
	using namespace TDVegetation;

	FTDTreeParams Out;
	Out.CommonName = Species.CommonName;
	Out.LatinName = Species.LatinName;
	Out.Archetype = MatchTreeArchetype(Species, Out.ArchetypeReason);
	const FTDTreeArchetypeDefaults Defaults = GetTreeArchetypeDefaults(Out.Archetype);

	if (Species.CrownShape == ETDCrownShape::Unknown && Species.Branching == ETDBranchingHabit::Unknown)
	{
		Out.DefaultedFields.Add(TEXT("CrownShape"));
		Out.DefaultedFields.Add(TEXT("Branching"));
	}

	// ---- Mature size: species data where it is usable and consistent, archetype defaults otherwise ----
	float MatureHeight = Defaults.HeightM;
	if (!IsUsable(Species.HeightM))
	{
		Out.DefaultedFields.Add(TEXT("HeightM"));
	}
	else if (Species.HeightM > MaxPlausibleHeightM)
	{
		Out.DefaultedFields.Add(TEXT("HeightM"));
		Out.Notes.Add(FString::Printf(TEXT("HeightM %.1f is taller than any known tree (%.0f m): used the archetype default %.1f"), Species.HeightM, MaxPlausibleHeightM, Defaults.HeightM));
	}
	else
	{
		MatureHeight = Species.HeightM;
	}

	float MatureCrownWidth = MatureHeight * Defaults.CrownWidthRatio;
	if (!IsUsable(Species.CrownWidthM))
	{
		Out.DefaultedFields.Add(TEXT("CrownWidthM"));
	}
	else if (Species.CrownWidthM > MatureHeight * MaxCrownWidthToHeight)
	{
		Out.DefaultedFields.Add(TEXT("CrownWidthM"));
		Out.Notes.Add(FString::Printf(TEXT("CrownWidthM %.1f is more than %.0f times the height %.1f: used the archetype ratio"), Species.CrownWidthM, MaxCrownWidthToHeight, MatureHeight));
	}
	else
	{
		MatureCrownWidth = Species.CrownWidthM;
	}

	float MatureTrunkDiameter = MatureHeight * Defaults.TrunkDiameterRatio;
	if (!IsUsable(Species.TrunkDiameterM))
	{
		Out.DefaultedFields.Add(TEXT("TrunkDiameterM"));
	}
	else if (Species.TrunkDiameterM > MatureHeight * MaxTrunkDiameterToHeight)
	{
		Out.DefaultedFields.Add(TEXT("TrunkDiameterM"));
		Out.Notes.Add(FString::Printf(TEXT("TrunkDiameterM %.2f is more than %.0f%% of the height %.1f: used the archetype ratio"), Species.TrunkDiameterM, MaxTrunkDiameterToHeight * 100.f, MatureHeight));
	}
	else
	{
		MatureTrunkDiameter = Species.TrunkDiameterM;
	}

	float CrownBase = Defaults.CrownBaseFraction;
	if (!FMath::IsFinite(Species.ClearBoleFraction) || Species.ClearBoleFraction < 0.f)
	{
		Out.DefaultedFields.Add(TEXT("ClearBoleFraction"));
	}
	else if (Species.ClearBoleFraction > 1.f)
	{
		Out.DefaultedFields.Add(TEXT("ClearBoleFraction"));
		Out.Notes.Add(FString::Printf(TEXT("ClearBoleFraction %.2f is above 1: used the archetype default %.2f"), Species.ClearBoleFraction, Defaults.CrownBaseFraction));
	}
	else
	{
		// A crown needs room: never let the clear trunk take more than 95% of the height.
		CrownBase = FMath::Min(Species.ClearBoleFraction, 0.95f);
	}

	float LeafLength = Defaults.LeafLengthM;
	if (!IsUsable(Species.LeafLengthM))
	{
		Out.DefaultedFields.Add(TEXT("LeafLengthM"));
	}
	else if (Species.LeafLengthM > MatureHeight)
	{
		Out.DefaultedFields.Add(TEXT("LeafLengthM"));
		Out.Notes.Add(FString::Printf(TEXT("LeafLengthM %.2f is longer than the tree is tall (%.1f): used the archetype default"), Species.LeafLengthM, MatureHeight));
	}
	else
	{
		LeafLength = Species.LeafLengthM;
	}

	int32 CrownLeafCount = Defaults.CrownLeafCount;
	if (Out.Archetype == ETDTreeArchetype::Palm)
	{
		if (Species.CrownLeafCount > 0)
		{
			CrownLeafCount = Species.CrownLeafCount;
		}
		else
		{
			Out.DefaultedFields.Add(TEXT("CrownLeafCount"));
		}
	}

	Out.LeafType = Species.LeafType;
	if (Species.LeafType == ETDLeafType::Unknown)
	{
		Out.LeafType = Defaults.LeafType;
		Out.DefaultedFields.Add(TEXT("LeafType"));
	}

	Out.bEvergreen = Species.Persistence == ETDLeafPersistence::Evergreen;
	if (Species.Persistence == ETDLeafPersistence::Unknown)
	{
		Out.bEvergreen = Defaults.bEvergreen;
		Out.DefaultedFields.Add(TEXT("Persistence"));
	}

	// ---- Growth stage: sapling (0) to mature (1) ----
	const float Stage = FMath::IsFinite(GrowthStage) ? FMath::Clamp(GrowthStage, 0.f, 1.f) : 1.f;
	const float HeightFraction = GetHeightFractionAtGrowthStage(Stage);

	Out.GrowthStage = Stage;
	Out.Seed = Seed;
	Out.MatureHeightM = MatureHeight;
	Out.HeightM = MatureHeight * HeightFraction;
	// Young crowns are narrower for their height; young trunks are more slender.
	Out.CrownWidthM = MatureCrownWidth * HeightFraction * FMath::Lerp(0.7f, 1.f, Stage);
	Out.TrunkDiameterM = MatureTrunkDiameter * FMath::Pow(HeightFraction, 1.5f);
	// Young trees still carry their low branches.
	Out.CrownBaseFraction = CrownBase * FMath::Lerp(0.35f, 1.f, Stage);
	// A sapling always has a leader; the adult habit takes over as it matures.
	Out.ApicalDominance = FMath::Lerp(FMath::Max(Defaults.ApicalDominance, 0.7f), Defaults.ApicalDominance, Stage);
	Out.BranchAngleDeg = Defaults.BranchAngleDeg;
	Out.BranchDroop = Defaults.BranchDroop * FMath::Lerp(0.5f, 1.f, Stage);
	Out.BranchDensity = Defaults.BranchDensity;
	const int32 OrdersLost = Stage < 0.25f ? 2 : (Stage < 0.6f ? 1 : 0);
	Out.BranchOrders = Defaults.BranchOrders <= 1 ? 1 : FMath::Max(2, Defaults.BranchOrders - OrdersLost);
	Out.CrownFlatness = Defaults.CrownFlatness * Stage;
	Out.StemCount = Defaults.StemCount;
	Out.LeafLengthM = LeafLength * FMath::Lerp(0.6f, 1.f, Stage);
	Out.LeafDensity = Defaults.LeafDensity;
	Out.CrownLeafCount = CrownLeafCount > 0 ? FMath::Max(4, FMath::RoundToInt32(CrownLeafCount * FMath::Lerp(0.5f, 1.f, Stage))) : 0;

	// ---- Seed: small, deterministic differences between individuals of one species ----
	FRandomStream Stream(Seed);
	Out.HeightM *= 1.f + Stream.FRandRange(-0.07f, 0.07f);
	Out.CrownWidthM *= 1.f + Stream.FRandRange(-0.10f, 0.10f);
	Out.TrunkDiameterM *= 1.f + Stream.FRandRange(-0.08f, 0.08f);
	Out.BranchAngleDeg = FMath::Clamp(Out.BranchAngleDeg + Stream.FRandRange(-5.f, 5.f), 5.f, 120.f);
	const float DensityJitter = Stream.FRandRange(-0.08f, 0.08f);
	Out.BranchDensity = Out.BranchOrders <= 1 ? 0.f : FMath::Clamp(Out.BranchDensity + DensityJitter, 0.f, 1.f);
	Out.BranchDroop = FMath::Clamp(Out.BranchDroop + Stream.FRandRange(-0.05f, 0.05f), 0.f, 1.f);
	Out.CrownBaseFraction = FMath::Clamp(Out.CrownBaseFraction + Stream.FRandRange(-0.04f, 0.04f), 0.f, 0.95f);

	return Out;
}
