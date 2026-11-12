// Triad Drift - tests for the tree archetype matcher (TriadDrift.Vegetation.*).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/PlatformTime.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Vegetation/TDVegetationLibrary.h"

#include <limits>

namespace TDVegetationTests
{
	inline FTDTreeSpeciesData MakeSpecies(float HeightM, float CrownWidthM, float TrunkDiameterM, ETDCrownShape Shape,
		ETDBranchingHabit Habit, ETDLeafType Leaf, ETDLeafPersistence Persistence = ETDLeafPersistence::Unknown)
	{
		FTDTreeSpeciesData Species;
		Species.HeightM = HeightM;
		Species.CrownWidthM = CrownWidthM;
		Species.TrunkDiameterM = TrunkDiameterM;
		Species.CrownShape = Shape;
		Species.Branching = Habit;
		Species.LeafType = Leaf;
		Species.Persistence = Persistence;
		return Species;
	}

	inline ETDTreeArchetype MatchOnly(const FTDTreeSpeciesData& Species)
	{
		FString Reason;
		return UTDVegetationLibrary::MatchTreeArchetype(Species, Reason);
	}

	/** The six Verdance analogue species with the values recorded in .claude/skills/procedural-trees/biomes/verdance.json. */
	inline TArray<TPair<FTDTreeSpeciesData, ETDTreeArchetype>> VerdanceSpecies()
	{
		TArray<TPair<FTDTreeSpeciesData, ETDTreeArchetype>> Out;
		// Ceiba pentandra: emergent, "pagoda-shaped" crown, palmately compound leaves, deciduous.
		FTDTreeSpeciesData Ceiba = MakeSpecies(60.f, 60.f, 2.f, ETDCrownShape::Spreading, ETDBranchingHabit::Decurrent, ETDLeafType::CompoundBroadleaf, ETDLeafPersistence::Deciduous);
		Ceiba.LeafLengthM = 0.13f;
		Out.Emplace(Ceiba, ETDTreeArchetype::BroadSpreading);
		// Bertholletia excelsa: globose crown, unbranched until near the top.
		FTDTreeSpeciesData Brazil = MakeSpecies(40.f, 0.f, 1.5f, ETDCrownShape::Round, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf, ETDLeafPersistence::Evergreen);
		Brazil.ClearBoleFraction = 0.6f;
		Brazil.LeafLengthM = 0.275f;
		Out.Emplace(Brazil, ETDTreeArchetype::RoundOval);
		// Cecropia peltata: open umbrella-shaped crown of a few stout branches.
		FTDTreeSpeciesData Cecropia = MakeSpecies(15.f, 0.f, 0.2f, ETDCrownShape::Spreading, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf, ETDLeafPersistence::Evergreen);
		Cecropia.ClearBoleFraction = 0.6f;
		Cecropia.LeafLengthM = 0.2f;
		Out.Emplace(Cecropia, ETDTreeArchetype::BroadSpreading);
		// Socratea exorrhiza: palm, about 7 leaves.
		FTDTreeSpeciesData Socratea = MakeSpecies(16.5f, 0.f, 0.15f, ETDCrownShape::Unknown, ETDBranchingHabit::Unbranched, ETDLeafType::Frond, ETDLeafPersistence::Evergreen);
		Socratea.LeafLengthM = 1.75f;
		Socratea.CrownLeafCount = 7;
		Out.Emplace(Socratea, ETDTreeArchetype::Palm);
		// Cyathea arborea: tree fern.
		FTDTreeSpeciesData Cyathea = MakeSpecies(9.f, 0.f, 0.1f, ETDCrownShape::Unknown, ETDBranchingHabit::Unbranched, ETDLeafType::Frond, ETDLeafPersistence::Evergreen);
		Cyathea.LeafLengthM = 3.f;
		Out.Emplace(Cyathea, ETDTreeArchetype::Palm);
		// Theobroma cacao: small understorey tree with a globose crown on a short bole.
		FTDTreeSpeciesData Cacao = MakeSpecies(8.f, 0.f, 0.25f, ETDCrownShape::Round, ETDBranchingHabit::Unknown, ETDLeafType::Broadleaf, ETDLeafPersistence::Evergreen);
		Cacao.ClearBoleFraction = 0.15f;
		Cacao.LeafLengthM = 0.3f;
		Out.Emplace(Cacao, ETDTreeArchetype::RoundOval);
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVegetationArchetypeTable, "TriadDrift.Vegetation.ArchetypeTableIsSane", TD_TEST_FLAGS)
bool FTDVegetationArchetypeTable::RunTest(const FString& Parameters)
{
	for (int32 Index = 0; Index < static_cast<int32>(ETDTreeArchetype::Count); ++Index)
	{
		const ETDTreeArchetype Archetype = static_cast<ETDTreeArchetype>(Index);
		const FTDTreeArchetypeDefaults Defaults = UTDVegetationLibrary::GetTreeArchetypeDefaults(Archetype);
		const FString Name = FString::Printf(TEXT("archetype %d"), Index);
		TestEqual(Name + TEXT(" returns its own row"), Defaults.Archetype, Archetype);
		TestTrue(Name + TEXT(" height"), Defaults.HeightM > 0.f && Defaults.HeightM <= UTDVegetationLibrary::MaxPlausibleHeightM);
		TestTrue(Name + TEXT(" crown ratio"), Defaults.CrownWidthRatio > 0.f && Defaults.CrownWidthRatio <= UTDVegetationLibrary::MaxCrownWidthToHeight);
		TestTrue(Name + TEXT(" trunk ratio"), Defaults.TrunkDiameterRatio > 0.f && Defaults.TrunkDiameterRatio <= UTDVegetationLibrary::MaxTrunkDiameterToHeight);
		TestTrue(Name + TEXT(" crown base"), Defaults.CrownBaseFraction >= 0.f && Defaults.CrownBaseFraction <= 0.95f);
		TestTrue(Name + TEXT(" unit ranges"), Defaults.ApicalDominance >= 0.f && Defaults.ApicalDominance <= 1.f && Defaults.BranchDroop >= 0.f && Defaults.BranchDroop <= 1.f
			&& Defaults.BranchDensity >= 0.f && Defaults.BranchDensity <= 1.f && Defaults.CrownFlatness >= 0.f && Defaults.CrownFlatness <= 1.f && Defaults.LeafDensity >= 0.f && Defaults.LeafDensity <= 1.f);
		TestTrue(Name + TEXT(" orders and stems"), Defaults.BranchOrders >= 1 && Defaults.BranchOrders <= 5 && Defaults.StemCount >= 1);
		TestTrue(Name + TEXT(" leaf"), Defaults.LeafLengthM > 0.f && Defaults.LeafType != ETDLeafType::Unknown);
	}

	const FTDTreeArchetypeDefaults Palm = UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype::Palm);
	TestEqual(TEXT("palm is trunk only"), Palm.BranchOrders, 1);
	TestTrue(TEXT("palm has a rosette"), Palm.CrownLeafCount > 0 && Palm.LeafType == ETDLeafType::Frond);
	TestTrue(TEXT("conifer keeps a leader"), UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype::ConicalConifer).ApicalDominance > 0.9f);
	TestTrue(TEXT("weeping droops most"), UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype::Weeping).BranchDroop > 0.8f);
	TestTrue(TEXT("shrub has several stems"), UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype::MultiStemShrub).StemCount > 1);
	TestEqual(TEXT("out-of-range archetype falls back to RoundOval"), UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype::Count).Archetype, ETDTreeArchetype::RoundOval);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVegetationMatchRules, "TriadDrift.Vegetation.MatchRulesPickTheExpectedArchetype", TD_TEST_FLAGS)
bool FTDVegetationMatchRules::RunTest(const FString& Parameters)
{
	using namespace TDVegetationTests;
	// Stated shapes and habits.
	TestEqual(TEXT("Norway spruce"), MatchOnly(MakeSpecies(35.f, 8.f, 1.f, ETDCrownShape::Conical, ETDBranchingHabit::Excurrent, ETDLeafType::Needle)), ETDTreeArchetype::ConicalConifer);
	TestEqual(TEXT("Lombardy poplar"), MatchOnly(MakeSpecies(25.f, 4.f, 0.8f, ETDCrownShape::Columnar, ETDBranchingHabit::Excurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::Columnar);
	TestEqual(TEXT("English oak"), MatchOnly(MakeSpecies(25.f, 25.f, 1.5f, ETDCrownShape::Spreading, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::BroadSpreading);
	TestEqual(TEXT("sugar maple"), MatchOnly(MakeSpecies(25.f, 15.f, 0.8f, ETDCrownShape::Oval, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::RoundOval);
	TestEqual(TEXT("weeping willow"), MatchOnly(MakeSpecies(12.f, 12.f, 0.8f, ETDCrownShape::Weeping, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::Weeping);
	TestEqual(TEXT("pendulous habit alone"), MatchOnly(MakeSpecies(10.f, 0.f, 0.f, ETDCrownShape::Unknown, ETDBranchingHabit::Pendulous, ETDLeafType::Unknown)), ETDTreeArchetype::Weeping);
	TestEqual(TEXT("coconut palm"), MatchOnly(MakeSpecies(25.f, 8.f, 0.3f, ETDCrownShape::Unknown, ETDBranchingHabit::Unknown, ETDLeafType::Frond)), ETDTreeArchetype::Palm);
	TestEqual(TEXT("rosette beats a stated round crown"), MatchOnly(MakeSpecies(5.f, 3.f, 0.2f, ETDCrownShape::Round, ETDBranchingHabit::Unbranched, ETDLeafType::Broadleaf)), ETDTreeArchetype::Palm);
	TestEqual(TEXT("hazel"), MatchOnly(MakeSpecies(6.f, 5.f, 0.1f, ETDCrownShape::Round, ETDBranchingHabit::MultiStem, ETDLeafType::Broadleaf)), ETDTreeArchetype::MultiStemShrub);

	// Inferred from leaves and proportions.
	TestEqual(TEXT("needle leaves, nothing else"), MatchOnly(MakeSpecies(0.f, 0.f, 0.f, ETDCrownShape::Unknown, ETDBranchingHabit::Unknown, ETDLeafType::Needle)), ETDTreeArchetype::ConicalConifer);
	TestEqual(TEXT("old spreading pine stays spreading"), MatchOnly(MakeSpecies(20.f, 18.f, 0.8f, ETDCrownShape::Spreading, ETDBranchingHabit::Decurrent, ETDLeafType::Needle)), ETDTreeArchetype::BroadSpreading);
	TestEqual(TEXT("narrow by proportions"), MatchOnly(MakeSpecies(20.f, 3.f, 0.f, ETDCrownShape::Unknown, ETDBranchingHabit::Unknown, ETDLeafType::Broadleaf)), ETDTreeArchetype::Columnar);
	TestEqual(TEXT("wide by proportions"), MatchOnly(MakeSpecies(15.f, 16.f, 0.f, ETDCrownShape::Irregular, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::BroadSpreading);
	TestEqual(TEXT("middling proportions"), MatchOnly(MakeSpecies(20.f, 10.f, 0.f, ETDCrownShape::Unknown, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::RoundOval);
	TestEqual(TEXT("short plant with no habit"), MatchOnly(MakeSpecies(2.f, 0.f, 0.f, ETDCrownShape::Unknown, ETDBranchingHabit::Unknown, ETDLeafType::Broadleaf)), ETDTreeArchetype::MultiStemShrub);
	TestEqual(TEXT("short tree with a stated habit is not a shrub"), MatchOnly(MakeSpecies(3.f, 0.f, 0.f, ETDCrownShape::Unknown, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::RoundOval);

	// Nothing usable at all.
	FString Reason;
	TestEqual(TEXT("empty data falls back"), UTDVegetationLibrary::MatchTreeArchetype(FTDTreeSpeciesData(), Reason), ETDTreeArchetype::RoundOval);
	TestTrue(TEXT("the fallback says so"), Reason.Contains(TEXT("fallback")));
	// An impossible crown width must not drive the match.
	TestEqual(TEXT("absurd crown width is ignored"), MatchOnly(MakeSpecies(10.f, 500.f, 0.f, ETDCrownShape::Unknown, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf)), ETDTreeArchetype::RoundOval);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVegetationVerdance, "TriadDrift.Vegetation.VerdanceSpeciesMatch", TD_TEST_FLAGS)
bool FTDVegetationVerdance::RunTest(const FString& Parameters)
{
	int32 Index = 0;
	for (const TPair<FTDTreeSpeciesData, ETDTreeArchetype>& Entry : TDVegetationTests::VerdanceSpecies())
	{
		const FTDTreeParams Params = UTDVegetationLibrary::BuildTreeParams(Entry.Key, 1.f, 0);
		const FString Name = FString::Printf(TEXT("Verdance species %d"), Index++);
		TestEqual(Name + TEXT(" archetype"), Params.Archetype, Entry.Value);
		TestEqual(Name + TEXT(" mature height is the species height"), Params.MatureHeightM, Entry.Key.HeightM);
		TestEqual(Name + TEXT(" crown width is defaulted exactly when no source gave it"), Params.DefaultedFields.Contains(TEXT("CrownWidthM")), Entry.Key.CrownWidthM <= 0.f);
		TestFalse(Name + TEXT(" height was not defaulted"), Params.DefaultedFields.Contains(TEXT("HeightM")));
		TestEqual(Name + TEXT(" no inconsistencies"), Params.Notes.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVegetationDefaultsReported, "TriadDrift.Vegetation.MissingDataFallsBackAndIsReported", TD_TEST_FLAGS)
bool FTDVegetationDefaultsReported::RunTest(const FString& Parameters)
{
	using namespace TDVegetationTests;
	// Nothing known: everything comes from the archetype and every field is listed.
	const FTDTreeParams Empty = UTDVegetationLibrary::BuildTreeParams(FTDTreeSpeciesData(), 1.f, 0);
	const FTDTreeArchetypeDefaults Round = UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype::RoundOval);
	for (const TCHAR* Field : { TEXT("HeightM"), TEXT("CrownWidthM"), TEXT("TrunkDiameterM"), TEXT("ClearBoleFraction"), TEXT("LeafLengthM"), TEXT("LeafType"), TEXT("Persistence"), TEXT("CrownShape"), TEXT("Branching") })
	{
		TestTrue(FString::Printf(TEXT("%s reported as defaulted"), Field), Empty.DefaultedFields.Contains(FName(Field)));
	}
	TestFalse(TEXT("frond count is not a field of a branching tree"), Empty.DefaultedFields.Contains(TEXT("CrownLeafCount")));
	TestEqual(TEXT("default mature height"), Empty.MatureHeightM, Round.HeightM);
	TestEqual(TEXT("default leaf type"), Empty.LeafType, Round.LeafType);
	TestEqual(TEXT("default branch orders"), Empty.BranchOrders, Round.BranchOrders);

	// Everything known: nothing defaulted, values pass through (within the seed's stated variation).
	FTDTreeSpeciesData Full = MakeSpecies(24.f, 12.f, 0.9f, ETDCrownShape::Oval, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf, ETDLeafPersistence::Deciduous);
	Full.ClearBoleFraction = 0.4f;
	Full.LeafLengthM = 0.1f;
	const FTDTreeParams Known = UTDVegetationLibrary::BuildTreeParams(Full, 1.f, 0);
	TestEqual(TEXT("nothing defaulted"), Known.DefaultedFields.Num(), 0);
	TestEqual(TEXT("no notes"), Known.Notes.Num(), 0);
	TestEqual(TEXT("mature height"), Known.MatureHeightM, 24.f);
	TestEqual(TEXT("height within 7%"), Known.HeightM, 24.f, 24.f * 0.0701f);
	TestEqual(TEXT("crown width within 10%"), Known.CrownWidthM, 12.f, 12.f * 0.1001f);
	TestEqual(TEXT("trunk within 8%"), Known.TrunkDiameterM, 0.9f, 0.9f * 0.0801f);
	TestEqual(TEXT("crown base within 0.04"), Known.CrownBaseFraction, 0.4f, 0.0401f);
	TestEqual(TEXT("leaf length"), Known.LeafLengthM, 0.1f, KINDA_SMALL_NUMBER);
	TestFalse(TEXT("deciduous"), Known.bEvergreen);

	// A palm with no frond count reports it; with one, uses it.
	FTDTreeSpeciesData PalmSpecies = MakeSpecies(15.f, 0.f, 0.2f, ETDCrownShape::Unknown, ETDBranchingHabit::Unbranched, ETDLeafType::Frond);
	TestTrue(TEXT("palm frond count defaulted"), UTDVegetationLibrary::BuildTreeParams(PalmSpecies, 1.f, 0).DefaultedFields.Contains(TEXT("CrownLeafCount")));
	PalmSpecies.CrownLeafCount = 7;
	const FTDTreeParams PalmParams = UTDVegetationLibrary::BuildTreeParams(PalmSpecies, 1.f, 0);
	TestEqual(TEXT("palm frond count used"), PalmParams.CrownLeafCount, 7);
	TestEqual(TEXT("palm has no branches"), PalmParams.BranchOrders, 1);
	TestEqual(TEXT("palm branch density is zero"), PalmParams.BranchDensity, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVegetationInconsistent, "TriadDrift.Vegetation.InconsistentDataIsRejected", TD_TEST_FLAGS)
bool FTDVegetationInconsistent::RunTest(const FString& Parameters)
{
	using namespace TDVegetationTests;
	const FTDTreeArchetypeDefaults Round = UTDVegetationLibrary::GetTreeArchetypeDefaults(ETDTreeArchetype::RoundOval);

	// A height in centimetres by mistake.
	const FTDTreeParams TooTall = UTDVegetationLibrary::BuildTreeParams(MakeSpecies(2500.f, 0.f, 0.f, ETDCrownShape::Round, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf), 1.f, 0);
	TestTrue(TEXT("impossible height defaulted"), TooTall.DefaultedFields.Contains(TEXT("HeightM")));
	TestEqual(TEXT("impossible height replaced"), TooTall.MatureHeightM, Round.HeightM);
	TestTrue(TEXT("impossible height explained"), TooTall.Notes.Num() > 0);

	// Crown and trunk that contradict the height.
	const FTDTreeParams Contradiction = UTDVegetationLibrary::BuildTreeParams(MakeSpecies(10.f, 100.f, 8.f, ETDCrownShape::Round, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf), 1.f, 0);
	TestTrue(TEXT("crown width rejected"), Contradiction.DefaultedFields.Contains(TEXT("CrownWidthM")));
	TestTrue(TEXT("trunk rejected"), Contradiction.DefaultedFields.Contains(TEXT("TrunkDiameterM")));
	TestEqual(TEXT("two notes"), Contradiction.Notes.Num(), 2);
	TestTrue(TEXT("crown stays plausible"), Contradiction.CrownWidthM <= 10.f * UTDVegetationLibrary::MaxCrownWidthToHeight);
	TestTrue(TEXT("trunk stays plausible"), Contradiction.TrunkDiameterM <= 10.f * UTDVegetationLibrary::MaxTrunkDiameterToHeight);

	// Garbage in: negative, NaN and infinite values count as missing, never as data.
	FTDTreeSpeciesData Garbage = MakeSpecies(-5.f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), ETDCrownShape::Round, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf);
	Garbage.ClearBoleFraction = 3.f;
	Garbage.LeafLengthM = 50.f;
	const FTDTreeParams FromGarbage = UTDVegetationLibrary::BuildTreeParams(Garbage, std::numeric_limits<float>::quiet_NaN(), 0);
	TestTrue(TEXT("finite height"), FMath::IsFinite(FromGarbage.HeightM) && FromGarbage.HeightM > 0.f);
	TestTrue(TEXT("finite crown"), FMath::IsFinite(FromGarbage.CrownWidthM) && FromGarbage.CrownWidthM > 0.f);
	TestTrue(TEXT("finite trunk"), FMath::IsFinite(FromGarbage.TrunkDiameterM) && FromGarbage.TrunkDiameterM > 0.f);
	TestTrue(TEXT("crown base in range"), FromGarbage.CrownBaseFraction >= 0.f && FromGarbage.CrownBaseFraction <= 0.95f);
	TestTrue(TEXT("bole fraction above 1 rejected"), FromGarbage.DefaultedFields.Contains(TEXT("ClearBoleFraction")));
	TestTrue(TEXT("leaf longer than the tree rejected"), FromGarbage.DefaultedFields.Contains(TEXT("LeafLengthM")));
	TestEqual(TEXT("NaN growth stage means mature"), FromGarbage.GrowthStage, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVegetationVariation, "TriadDrift.Vegetation.GrowthStageAndSeedVary", TD_TEST_FLAGS)
bool FTDVegetationVariation::RunTest(const FString& Parameters)
{
	using namespace TDVegetationTests;
	const FTDTreeSpeciesData Oak = MakeSpecies(25.f, 25.f, 1.5f, ETDCrownShape::Spreading, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf, ETDLeafPersistence::Deciduous);

	// Same input, same output: the function is deterministic.
	const FTDTreeParams A = UTDVegetationLibrary::BuildTreeParams(Oak, 0.6f, 42);
	const FTDTreeParams B = UTDVegetationLibrary::BuildTreeParams(Oak, 0.6f, 42);
	TestEqual(TEXT("same height"), A.HeightM, B.HeightM);
	TestEqual(TEXT("same crown"), A.CrownWidthM, B.CrownWidthM);
	TestEqual(TEXT("same angle"), A.BranchAngleDeg, B.BranchAngleDeg);
	TestEqual(TEXT("same density"), A.BranchDensity, B.BranchDensity);
	TestEqual(TEXT("same crown base"), A.CrownBaseFraction, B.CrownBaseFraction);

	// Different seeds give different individuals, all inside the stated variation.
	TSet<int32> DistinctHeights;
	for (int32 Seed = 1; Seed <= 50; ++Seed)
	{
		const FTDTreeParams Tree = UTDVegetationLibrary::BuildTreeParams(Oak, 1.f, Seed);
		DistinctHeights.Add(FMath::RoundToInt32(Tree.HeightM * 100.f));
		TestEqual(TEXT("seeded height within 7%"), Tree.HeightM, 25.f, 25.f * 0.0701f);
		TestEqual(TEXT("seeded crown within 10%"), Tree.CrownWidthM, 25.f, 25.f * 0.1001f);
		TestTrue(TEXT("seeded angle in range"), Tree.BranchAngleDeg >= 5.f && Tree.BranchAngleDeg <= 120.f);
		TestEqual(TEXT("seed never changes the archetype"), Tree.Archetype, ETDTreeArchetype::BroadSpreading);
		TestEqual(TEXT("seed never changes the mature height"), Tree.MatureHeightM, 25.f);
	}
	TestTrue(TEXT("50 seeds give many different heights"), DistinctHeights.Num() >= 40);

	// Growth: a sapling is small, slender, low-branched and simple; the tree grows monotonically.
	TestEqual(TEXT("sapling height fraction"), UTDVegetationLibrary::GetHeightFractionAtGrowthStage(0.f), UTDVegetationLibrary::SaplingHeightFraction);
	TestEqual(TEXT("mature height fraction"), UTDVegetationLibrary::GetHeightFractionAtGrowthStage(1.f), 1.f);
	TestEqual(TEXT("stage below 0 clamps"), UTDVegetationLibrary::GetHeightFractionAtGrowthStage(-3.f), UTDVegetationLibrary::SaplingHeightFraction);
	TestEqual(TEXT("stage above 1 clamps"), UTDVegetationLibrary::GetHeightFractionAtGrowthStage(7.f), 1.f);
	float PreviousFraction = 0.f;
	for (int32 Step = 0; Step <= 20; ++Step)
	{
		const float Fraction = UTDVegetationLibrary::GetHeightFractionAtGrowthStage(Step / 20.f);
		TestTrue(TEXT("height grows with the stage"), Fraction > PreviousFraction);
		PreviousFraction = Fraction;
	}
	const FTDTreeParams Sapling = UTDVegetationLibrary::BuildTreeParams(Oak, 0.f, 7);
	const FTDTreeParams Mature = UTDVegetationLibrary::BuildTreeParams(Oak, 1.f, 7);
	TestTrue(TEXT("sapling is much shorter"), Sapling.HeightM < Mature.HeightM * 0.2f);
	TestTrue(TEXT("sapling is more slender"), Sapling.TrunkDiameterM / Sapling.HeightM < Mature.TrunkDiameterM / Mature.HeightM);
	TestTrue(TEXT("sapling crown is narrower for its height"), Sapling.CrownWidthM / Sapling.HeightM < Mature.CrownWidthM / Mature.HeightM);
	TestTrue(TEXT("sapling branches lower"), Sapling.CrownBaseFraction < Mature.CrownBaseFraction);
	TestTrue(TEXT("sapling has a leader"), Sapling.ApicalDominance >= 0.7f && Sapling.ApicalDominance > Mature.ApicalDominance);
	TestTrue(TEXT("sapling has fewer branch orders"), Sapling.BranchOrders < Mature.BranchOrders && Sapling.BranchOrders >= 2);
	TestEqual(TEXT("same mature height at any stage"), Sapling.MatureHeightM, Mature.MatureHeightM);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDVegetationFast, "TriadDrift.Vegetation.MatcherIsFast", TD_TEST_FLAGS)
bool FTDVegetationFast::RunTest(const FString& Parameters)
{
	using namespace TDVegetationTests;
	const FTDTreeSpeciesData Oak = MakeSpecies(25.f, 25.f, 1.5f, ETDCrownShape::Spreading, ETDBranchingHabit::Decurrent, ETDLeafType::Broadleaf);
	const int32 Calls = 20000;
	double Checksum = 0.0;
	const double Start = FPlatformTime::Seconds();
	for (int32 Index = 0; Index < Calls; ++Index)
	{
		Checksum += UTDVegetationLibrary::BuildTreeParams(Oak, (Index % 11) / 10.f, Index).HeightM;
	}
	const double Seconds = FPlatformTime::Seconds() - Start;
	AddInfo(FString::Printf(TEXT("%d BuildTreeParams calls in %.1f ms (%.2f us each)"), Calls, Seconds * 1000.0, Seconds * 1e6 / Calls));
	TestTrue(TEXT("results were produced"), Checksum > 0.0);
	// Generous: this must hold on a machine that is also building and running other editors.
	TestTrue(TEXT("20000 calls take less than a second"), Seconds < 1.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
