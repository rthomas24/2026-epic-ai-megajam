// Triad Drift - World (Teammate 4; scatter v2 by the environment builder, Nov 14). Planet scatter driven by the terrain
// field: the zone layer built once, plus mid / near rings streamed around the camera (see TDPlanetScatter.h).
#include "TriadDrift/World/TDPlanetScatter.h"

#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Core/TDTypes.h"

#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

static TAutoConsoleVariable<int32> CVarTDScatterCourseFill(TEXT("td.ScatterCourseFill"), 1,
	TEXT("#194: 1 = the course fill (forest, boulder fields, groves, landmarks inside the course box) and the denser course ground; 0 = as before (before/after captures). Read at level start."));

static TAutoConsoleVariable<int32> CVarTDScatter(
	TEXT("td.Scatter"), 1,
	TEXT("World density: 1 = planets get their trees, rocks, groves and ground cover across the play zone; 0 = bare terrain. Read when the planet loads."));

namespace
{
	// Tree species in Gameplay/Props/Trees (PVE, real size): name and the three growth-stage ids (S100, S070, S030).
	struct FTDScatterSpecies { const TCHAR* Name; int32 Id100; };
	const FTDScatterSpecies GTDScatterTrees[] = {
		{ TEXT("BrazilNut"), 21 }, { TEXT("Cacao"), 61 }, { TEXT("Kapok"), 11 },
		{ TEXT("StiltPalm"), 41 }, { TEXT("TreeFern"), 51 }, { TEXT("TrumpetTree"), 31 } };
	enum ETDScatterTree : int32 { BrazilNut, Cacao, Kapok, StiltPalm, TreeFern, TrumpetTree };

	// The layers (Pick modes).
	enum ETDScatterMode : int32 { ModeFar = 0, ModeDense = 1, ModeCanopy = 2, ModeRock = 3, ModeGrove = 4, ModeGround = 5, ModeCount = 6 };

	/** Cheap deterministic hash -> 0..1 (seed, cell, salt). */
	float TDScatter_Hash01(int32 Seed, int32 X, int32 Y, int32 Salt)
	{
		uint32 H = uint32(Seed) * 0x9E3779B1u ^ uint32(X) * 0x85EBCA77u ^ uint32(Y) * 0xC2B2AE3Du ^ uint32(Salt) * 0x27D4EB2Fu;
		H ^= H >> 15; H *= 0x2C1B3C6Du; H ^= H >> 12; H *= 0x297A2D39u; H ^= H >> 15;
		return (H & 0xFFFFFF) / float(0x1000000);
	}

	const TCHAR* GProps = TEXT("/Game/TriadDrift/Gameplay/Props");

	FString TDScatter_Path(const TCHAR* Folder, const FString& Name)
	{
		return FString::Printf(TEXT("%s/%s/%s.%s"), GProps, Folder, *Name, *Name);
	}

	// Kind ranges per planet plan (indices into Kinds), filled by ForPlanet. Kept beside the plan by planet.
	struct FTDScatterRanges
	{
		int32 Trees = INDEX_NONE;	// 18 = 6 species x 3 stages (Verdance)
		TArray<int32> Canopy, CanopyAccent, Rocks, CliffRocks, Grove, GroveGlow, Ground, GroundGlow, GroundForest, Columns, CharTrees, Spires, Crystals, Rubble;
		TArray<int32> CourseCover, CourseAccent, CoursePebbles;	// s20: ground detail inside the course (never collides)
		TArray<int32> FillStumps, FillOutcrops, FillPlants, FillFerns, FillLow, FillLogs, FillCliffs;	// CJ intake: ferns, clover / gravel, logs, cliffs	// FillPlants: Fab low-poly plants, the near-path cover	// #194: Fab VerdanceStump (no collision) / Fab CinderRock mounds (collide)
	};
	FTDScatterRanges GTDRanges[2];	// [0] Verdance, [1] Cinder (filled once, read-only after)
	FCriticalSection GTDRangesLock;

	const FTDScatterRanges& TDScatter_Ranges(ETDDestination P) { return GTDRanges[P == ETDDestination::Cinder ? 1 : 0]; }

	/** Pick one of a list by a 0..1 roll. */
	int32 TDScatter_OneOf(const TArray<int32>& L, float Roll)
	{
		return L.Num() ? L[FMath::Min(L.Num() - 1, int32(Roll * L.Num()))] : INDEX_NONE;
	}
}

int32 FTDScatterResult::Total() const
{
	int32 N = 0;
	for (const TArray<FTransform>& A : Near) { N += A.Num(); }
	for (const TArray<FTransform>& A : Far) { N += A.Num(); }
	return N;
}

FTDScatterPlan FTDScatterPlan::ForPlanet(ETDDestination Planet)
{
	FTDScatterPlan P;
	P.Planet = Planet;
	if (Planet != ETDDestination::Verdance && Planet != ETDDestination::Cinder)
	{
		return P;
	}
	FTDScatterRanges R;
	auto Add = [&P](const FString& Path, float MinS, float MaxS, float Sink, float CullS, float CullE, float Clear, bool bBlock, bool bTumble, bool bShadow, const FString& Mat = FString()) -> int32
	{
		FTDScatterKind K;
		K.MeshPath = Path; K.MaterialPath = Mat;
		K.MinScale = MinS; K.MaxScale = MaxS; K.SinkCm = Sink; K.CullStartCm = CullS; K.CullEndCm = CullE;
		K.ClearRadiusCm = Clear; K.bBlocksOnFoot = bBlock; K.bTumble = bTumble; K.bShadow = bShadow;
		return P.Kinds.Add(K);
	};
	if (Planet == ETDDestination::Verdance)
	{
		P.CanopySpacing = 7000.f;	// s6: larger clumps on a wider grid (same instance count), overlapping inside a stand
		R.Trees = P.Kinds.Num();
		for (const FTDScatterSpecies& S : GTDScatterTrees)
		{
			for (int32 Stage = 0; Stage < 3; ++Stage)
			{
				const int32 Id = S.Id100 + Stage;
				const int32 Size = Stage == 0 ? 100 : (Stage == 1 ? 70 : 30);
				const FString Name = FString::Printf(TEXT("SM_TDVeg_%s_S%03d_%d"), S.Name, Size, Id);
				// Saplings are detail; grown trees read to ~2.4 km (the far tier); trunks block the pilot near the course.
				Add(TDScatter_Path(TEXT("Trees"), Name), 0.85f, 1.15f, 150.f, Stage == 2 ? 60000.f : 180000.f, Stage == 2 ? 90000.f : 240000.f, 400.f, Stage != 2, false, true);
			}
		}
		// Far-tier canopy clumps (their material dithers them out nearer than ~0.8 km, where the trees take over): green
		// forest stands (MI_Scatter_Canopy_Forest), with the bioluminescent pink / cyan clumps kept as accents (~12 %).
		// pass 3: the forest stands are SM_Scatter_Verd_CanopyStand_01..03 (environment builder, Blender: lumpy crown masses,
		// dark concave underside in vertex colour, 60-80 tris, 30-42 m; M_TD_CanopyStand dithers them out nearer ~0.75 km).
		// pass 4: the stands are SM_Scatter_Verd_TreeClump_01..04 instead: 5-9 of the real PVE trees (Kapok, Brazil nut)
		// merged per clump, trunks dropped, each crown reduced to its lumpy hull (558-578 tris, 35-65 m wide);
		// MI_TreeClump_Verdance fades them in from ~0.4 km, where the mid ring's real trees end. Sunk so the crown undersides
		// (the trunks are gone) meet the forest floor: 18 m per unit of scale (s5: at 9 m they floated over the ridges seen
		// from the pad); 1.3-2.0x so neighbouring clumps close into one canopy at 300-900 m with the same instance count.
		for (int32 i = 1; i <= 4; ++i)
		{
			// s6: 1.9-2.7x across but 0.6 of that in height (a clump spans ~70-170 m, crowns ~25-60 m high), sunk 18 m per
			// unit of height scale and seated on the lowest ground under its footprint (25 m x scale)
			const int32 Ci = Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Verd_TreeClump_%02d"), i)), 2.6f, 3.6f, 1800.f, 420000.f, 460000.f, 2200.f, false, false, false);
			P.Kinds[Ci].ZScale = 0.45f;	// s8: 2.6-3.6x across (150-250 m), crowns still 30-60 m high
			P.Kinds[Ci].ContactRadiusCm = 2000.f;
			R.Canopy.Add(Ci);
		}
		R.CanopyAccent.Add(Add(TDScatter_Path(TEXT("Scatter"), TEXT("SM_Scatter_Verd_Canopy_02")), 1.0f, 1.5f, 400.f, 420000.f, 460000.f, 4000.f, false, false, false));
		// s6 story: the wreck scar. Something came down on the research site from the north-east: a 550 m strip of burned
		// forest, narrow at the site and widening outward, readable from the landing approach (charred trunks, scorch).
		// s17: bearing 150 from the research site, 200-700 m out (61 % gentle ground; the old line ran down the Rift canyon's
		// walls); same numbers as the terrain material's Scar*
		P.ScarA = FVector2D(-5800.f, 17200.f);
		P.ScarB = FVector2D(-49100.f, 42200.f);
		P.ScarHalfA = 7000.f;	// s19: 70-160 m half width (s15's 50-140 m was weak from the approach)
		P.ScarHalfB = 16000.f;
		for (int32 i = 1; i <= 2; ++i)
		{
			// s11: snapped charred stumps, not whole glowing trunks (from above their ember material read as a grid of
			// yellow dots): a quarter of the height, with the dark basalt-column material (no glow, ash on top)
			const int32 Ck = Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Cind_CharTree_%02d"), i)), 1.5f, 2.4f, 30.f, 180000.f, 240000.f, 300.f, true, false, true,
				FString::Printf(TEXT("%s/Scatter/MI_Scatter_Column_Cinder.MI_Scatter_Column_Cinder"), GProps));
			P.Kinds[Ck].ZScale = 0.28f;
			R.CharTrees.Add(Ck);
		}
		P.ScorchKind = Add(TDScatter_Path(TEXT("Kit/Story"), TEXT("SM_Kit_ScorchLines")), 4.f, 6.5f, 0.f, 180000.f, 240000.f, 1200.f, false, false, false);
		// Fab intake (Nov 14): ForestNordicMoss on Verdance rocks and cliff boulders
		const FString VerdMoss = FString::Printf(TEXT("%s/Fab/Textures/ForestNordicMoss/MI_Fab_ForestNordicMoss.MI_Fab_ForestNordicMoss"), GProps);
		for (int32 i = 1; i <= 3; ++i)
		{
			R.Rocks.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_Rock_%02d"), i)), 1.5f, 4.5f, 60.f, 60000.f, 80000.f, 500.f, true, true, false, VerdMoss));	// T6/VSM: no dynamic shadow
		}
		for (int32 i = 1; i <= 4; ++i)
		{
			R.CliffRocks.Add(Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Any_Boulder_%02d"), i)), 0.8f, 1.6f, 80.f, 90000.f, 120000.f, 900.f, true, true, true, VerdMoss));
		}
		// Glowing groves: giant mushrooms (3-8x) and big glow plants where the ground is painted as a grove.
		for (int32 i = 1; i <= 3; ++i)
		{
			R.Grove.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_Mushroom_%02d"), i)), i == 3 ? 3.f : 4.f, i == 3 ? 6.f : 8.f, 40.f, 80000.f, 100000.f, 1200.f, true, false, false));	// T6/VSM: no dynamic shadow
		}
		for (int32 i = 1; i <= 2; ++i)
		{
			R.GroveGlow.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_GlowPlant_%02d"), i)), 2.f, 3.5f, 10.f, 30000.f, 40000.f, 400.f, false, false, false));
		}
		// Ground cover (near ring): no shadows, gone by ~150 m.
		R.Ground.Add(Add(TDScatter_Path(TEXT("Scatter"), TEXT("SM_Scatter_Verd_Grass_01")), 0.8f, 1.4f, 5.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.Ground.Add(Add(TDScatter_Path(TEXT("Scatter"), TEXT("SM_Scatter_Verd_Grass_02")), 0.8f, 1.4f, 5.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.Ground.Add(Add(TDScatter_Path(TEXT("Scatter"), TEXT("SM_Scatter_Verd_Fern_01")), 0.8f, 1.5f, 5.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.GroundForest.Add(Add(TDScatter_Path(TEXT("Verdance"), TEXT("SM_Verd_Fern_01")), 0.9f, 1.6f, 10.f, 11000.f, 15000.f, 150.f, false, false, false));
		R.GroundForest.Add(Add(TDScatter_Path(TEXT("Verdance"), TEXT("SM_Verd_Fern_02")), 0.9f, 1.6f, 10.f, 11000.f, 15000.f, 150.f, false, false, false));
		R.GroundForest.Add(R.Ground[2]);
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Scatter"), TEXT("SM_Scatter_Verd_GlowFlora_01")), 0.8f, 1.5f, 5.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Scatter"), TEXT("SM_Scatter_Verd_GlowFlora_02")), 0.8f, 1.5f, 5.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Verdance"), TEXT("SM_Verd_GlowPlant_01")), 0.8f, 1.6f, 10.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Verdance"), TEXT("SM_Verd_GlowPlant_02")), 0.8f, 1.6f, 10.f, 11000.f, 15000.f, 100.f, false, false, false));
		for (int32 i = 1; i <= 3; ++i)
		{
			R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_Mushroom_%02d"), i)), 0.5f, 1.1f, 10.f, 11000.f, 15000.f, 150.f, false, false, false));
		}
	}
	else
	{
		P.GroveSpacing = 3500.f;	// crystal clusters: the obsidian patches cover a larger share of Cinder than groves do of Verdance
		P.CanopySpacing = 6000.f;	// spires
		const FString RockMat = FString::Printf(TEXT("%s/Scatter/MI_Scatter_Rock_Cinder.MI_Scatter_Rock_Cinder"), GProps);
		const FString ColumnMat = FString::Printf(TEXT("%s/Scatter/MI_Scatter_Column_Cinder.MI_Scatter_Column_Cinder"), GProps);
		for (int32 i = 1; i <= 2; ++i)
		{
			// pass 4: M_TD_ScatterColumn (near-black base, weathered grey up the shaft, pale ash tops) so they read from the air
			R.Columns.Add(Add(TDScatter_Path(TEXT("Cinder"), FString::Printf(TEXT("SM_Cind_BasaltColumn_%02d"), i)), 3.f, 7.f, 80.f, 100000.f, 120000.f, 900.f, true, false, true, ColumnMat));
		}
		for (int32 i = 1; i <= 2; ++i)
		{
			R.CharTrees.Add(Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Cind_CharTree_%02d"), i)), 0.9f, 1.6f, 30.f, 70000.f, 90000.f, 300.f, true, false, true));
		}
		// Crystal spires: the big landmarks on the obsidian patches, read from the air.
		R.Spires.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_04")), 4.f, 6.5f, 80.f, 100000.f, 120000.f, 1500.f, true, false, false));	// T6: cull 1.2 km, no dynamic shadow
		R.Spires.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_03")), 4.f, 7.f, 60.f, 100000.f, 120000.f, 1200.f, true, false, false));	// T6: cull 1.2 km, no dynamic shadow
		for (int32 i = 1; i <= 4; ++i)
		{
			R.CliffRocks.Add(Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Any_Boulder_%02d"), i)), 0.8f, 1.8f, 80.f, 90000.f, 120000.f, 900.f, true, true, true, RockMat));
		}
		R.Rocks.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_03")), 1.5f, 3.5f, 30.f, 60000.f, 80000.f, 600.f, true, true, false));	// T6/VSM: no dynamic shadow
		R.Rocks.Add(Add(TDScatter_Path(TEXT("Planet"), TEXT("SM_Cind_ChokeRock_A")), 0.8f, 1.3f, 60.f, 180000.f, 240000.f, 1000.f, true, false, true));
		R.Rocks.Add(Add(TDScatter_Path(TEXT("Planet"), TEXT("SM_Cind_ChokeRock_B")), 0.8f, 1.3f, 60.f, 180000.f, 240000.f, 1000.f, true, false, true));
		for (int32 i = 1; i <= 4; ++i)
		{
			// s9: 2-4.5x and kept to ~2 km so the obsidian patches read from the air as crystal fields, not specks
			R.Crystals.Add(Add(TDScatter_Path(TEXT("Cinder"), FString::Printf(TEXT("SM_Cind_Crystal_%02d"), i)), 2.f, 4.5f, 20.f, 90000.f, 110000.f, 400.f, i >= 3, false, false));	// T6/VSM: no dynamic shadow
		}
		const FString TundraV = FString::Printf(TEXT("%s/Fab/Textures/TundraVolcanic/MI_Fab_TundraVolcanic.MI_Fab_TundraVolcanic"), GProps);	// #194: Cinder ground debris in the Fab tundra basalt
		R.Rubble.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_01")), 0.6f, 1.6f, 10.f, 11000.f, 15000.f, 100.f, false, true, false, TundraV));
		R.Rubble.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_02")), 0.5f, 1.2f, 15.f, 11000.f, 15000.f, 150.f, false, true, false, TundraV));
		R.Rubble.Add(Add(TDScatter_Path(TEXT("Planet"), TEXT("SM_Cind_DebrisChunk")), 0.6f, 1.2f, 30.f, 11000.f, 15000.f, 120.f, false, true, false, TundraV));
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_01")), 0.5f, 1.2f, 10.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_02")), 0.4f, 0.9f, 10.f, 11000.f, 15000.f, 100.f, false, false, false));
	}
	{
		FScopeLock Lock(&GTDRangesLock);
		// s20: ground detail inside the course (walking distance): cover, a few accents, small stones. No collision at all:
		// nothing may block or snag the pilot (bBlocksOnFoot false, and the course layer never sets collision).
		if (Planet == ETDDestination::Verdance)
		{
			R.CourseCover = { R.Ground[0], R.Ground[1], R.Ground[2], R.GroundForest[0], R.GroundForest[1] };
			R.CourseAccent = { R.GroundGlow[0], R.GroundGlow[1] };
			// #194: stumps (140 cm patch, sunk 15 cm so the tan rim hides), never colliding
			R.FillStumps.Add(Add(TDScatter_Path(TEXT("Fab/VerdanceStump"), TEXT("SM_Fab_VerdanceStump")), 0.8f, 1.3f, 15.f, 9000.f, 12000.f, 120.f, false, false, false));
			// #194: the Fab low-poly plants (24-32 tris, ~0.9 m, no collision), teal and a third violet, as the near-path cover;
			// the mossy rock clusters (1.9 m, colliding) join the outcrops
			{
				const FString Violet = FString::Printf(TEXT("%s/Fab/VerdancePlants/MI_Fab_VerdancePlants_Violet.MI_Fab_VerdancePlants_Violet"), GProps);
				for (const TCHAR* Pl : { TEXT("SM_Fab_VerdancePlant_Fern"), TEXT("SM_Fab_VerdancePlant_Broad"), TEXT("SM_Fab_VerdancePlant_Long") })
				{
					R.FillPlants.Add(Add(TDScatter_Path(TEXT("Fab/VerdancePlants"), Pl), 0.8f, 1.5f, 3.f, 9000.f, 12000.f, 60.f, false, false, false));
					R.FillPlants.Add(Add(TDScatter_Path(TEXT("Fab/VerdancePlants"), Pl), 0.8f, 1.5f, 3.f, 9000.f, 12000.f, 60.f, false, false, false));
					R.FillPlants.Add(Add(TDScatter_Path(TEXT("Fab/VerdancePlants"), Pl), 0.8f, 1.5f, 3.f, 9000.f, 12000.f, 60.f, false, false, false, Violet));
				}
				R.FillOutcrops.Add(Add(TDScatter_Path(TEXT("Fab/VerdanceRockCluster"), TEXT("SM_Fab_VerdanceRockCluster")), 0.6f, 2.5f, 20.f, 60000.f, 80000.f, 250.f, true, true, false));
				// CJ intake (no dynamic shadow on anything small): ferns and leather ferns as the near-path cover, clover as low
				// patches anywhere (fight areas too: non-colliding), a fallen log beside routes
				for (const TCHAR* F : { TEXT("SM_Fab_VerdanceFerns_A"), TEXT("SM_Fab_VerdanceFerns_B"), TEXT("SM_Fab_VerdanceFerns_C"), TEXT("SM_Fab_VerdanceLeatherFern_A"), TEXT("SM_Fab_VerdanceLeatherFern_B"), TEXT("SM_Fab_VerdanceLeatherFern_C") })
				{
					R.FillFerns.Add(Add(TDScatter_Path(TEXT("Fab/VerdanceFerns"), F), 0.8f, 1.4f, 3.f, 9000.f, 12000.f, 60.f, false, false, false));
				}
				for (const TCHAR* F : { TEXT("SM_Fab_VerdanceClover_A"), TEXT("SM_Fab_VerdanceClover_B") })
				{
					R.FillLow.Add(Add(TDScatter_Path(TEXT("Fab/VerdanceClover"), F), 0.8f, 1.6f, 2.f, 7000.f, 9000.f, 40.f, false, false, false));
				}
				R.FillLogs.Add(Add(TDScatter_Path(TEXT("Fab/VerdanceLog"), TEXT("SM_Fab_VerdanceLog_A")), 0.8f, 1.2f, 15.f, 40000.f, 60000.f, 250.f, true, true, true));
			}
			for (int32 i = 1; i <= 3; ++i)
			{
				R.CoursePebbles.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_Rock_%02d"), i)), 0.12f, 0.35f, 5.f, 9000.f, 12000.f, 50.f, false, true, false));
			}
		}
		else
		{
			R.CourseCover = R.Rubble;
			R.CourseAccent = { R.GroundGlow[0], R.GroundGlow[1] };
			R.CoursePebbles.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_01")), 0.25f, 0.6f, 5.f, 9000.f, 12000.f, 50.f, false, true, false));
			// #194: Fab CinderRock mounds (455 x 417 x 174 cm) for outcrops and clusters, colliding
			R.FillOutcrops.Add(Add(TDScatter_Path(TEXT("Fab/CinderRock"), TEXT("SM_Fab_CinderRock")), 0.5f, 2.0f, 20.f, 60000.f, 80000.f, 250.f, true, true, true));
			// CJ intake: dark cliff outcrops (also the backdrop behind the camps) and low gravel everywhere on the open ground
			R.FillCliffs.Add(Add(TDScatter_Path(TEXT("Fab/CinderCliff"), TEXT("SM_Fab_CinderCliff_A")), 0.7f, 1.4f, 40.f, 120000.f, 160000.f, 500.f, true, true, true));
			R.FillCliffs.Add(Add(TDScatter_Path(TEXT("Fab/CinderCliff"), TEXT("SM_Fab_CinderCliff_B")), 0.7f, 1.4f, 40.f, 120000.f, 160000.f, 500.f, true, true, true));
			R.FillLow.Add(Add(TDScatter_Path(TEXT("Fab/CinderRockScatter"), TEXT("SM_Fab_CinderRockScatter_A")), 0.8f, 1.5f, 3.f, 7000.f, 9000.f, 60.f, false, false, false));
			R.FillLow.Add(Add(TDScatter_Path(TEXT("Fab/CinderRockScatter"), TEXT("SM_Fab_CinderRockScatter_B")), 0.8f, 1.5f, 3.f, 7000.f, 9000.f, 60.f, false, false, false));
			R.CoursePebbles.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_02")), 0.25f, 0.55f, 5.f, 9000.f, 12000.f, 50.f, false, true, false));
		}
		GTDRanges[Planet == ETDDestination::Cinder ? 1 : 0] = R;
	}
	return P;
}

float FTDScatterPlan::ZoneWeight(float X, float Y) const
{
	const float D = FMath::Sqrt(X * X + Y * Y);
	if (D > ZoneRadius) { return 0.f; }
	return FMath::Lerp(1.f, ZoneEdgeDensity, FMath::SmoothStep(ZoneFullRadius, ZoneRadius, D));
}

bool FTDScatterPlan::IsKeptOut(const FTDTerrainField& Field, float X, float Y) const
{
	const FTDTerrainStyle& S = Field.GetStyle();
	return FMath::Abs(X - S.BasinCentre.X) < S.BasinHalfExtent.X + KeepOutMargin
		&& FMath::Abs(Y - S.BasinCentre.Y) < S.BasinHalfExtent.Y + KeepOutMargin;
}

bool FTDScatterPlan::IsNearCourse(const FTDTerrainField& Field, float X, float Y) const
{
	const FTDTerrainStyle& S = Field.GetStyle();
	return FMath::Abs(X - S.BasinCentre.X) < S.BasinHalfExtent.X + CollisionRadius
		&& FMath::Abs(Y - S.BasinCentre.Y) < S.BasinHalfExtent.Y + CollisionRadius;
}

bool FTDScatterPlan::IsCourseClear(float X, float Y, float Radius) const
{
	// Same shapes and maths as ATDPlanetCourse::IsScatterAllowed (copied course-local, so workers never touch the actor).
	const FVector2D Pt(X, Y);
	for (const FTDCourseKeepOut& K : KeepOuts)
	{
		if (K.Points.Num() > 1)
		{
			for (int32 i = 0; i + 1 < K.Points.Num(); ++i)
			{
				const FVector C = FMath::ClosestPointOnSegment(FVector(Pt, 0.f), FVector(K.Points[i].X, K.Points[i].Y, 0.f), FVector(K.Points[i + 1].X, K.Points[i + 1].Y, 0.f));
				if (FVector2D::Distance(FVector2D(C.X, C.Y), Pt) < K.HalfWidth + Radius) { return false; }
			}
		}
		else if (K.Box.IsValid)
		{
			if (Pt.X > K.Box.Min.X - Radius && Pt.X < K.Box.Max.X + Radius && Pt.Y > K.Box.Min.Y - Radius && Pt.Y < K.Box.Max.Y + Radius) { return false; }
		}
	}
	return true;
}

void FTDScatterPlan::RunCourseGround(const FTDTerrainField& Field, FTDScatterResult& Out) const
{
	// s20: inside the course's ground box the zone layer placed nothing; the walked areas read bare at walking distance.
	// A fine jittered grid of cover, accents and small stones, kept off every course keep-out (T5's routes, sites,
	// encounters, spawn rings, structures, the pad), thinning toward the routes so the walked lines read as trodden.
	const FTDScatterRanges& R = TDScatter_Ranges(Planet);
	if (R.CourseCover.Num() == 0 || KeepOuts.Num() == 0)
	{
		return;	// no course (tests and the galaxy): nothing to dress
	}
	const FTDTerrainStyle& S = Field.GetStyle();
	const int32 Seed = S.Seed * 3571 + 29;
	const bool bFill = CVarTDScatterCourseFill.GetValueOnAnyThread() != 0;
	const float Spacing = bFill ? CourseGroundSpacing : 380.f;
	const FVector2D Lo = FVector2D(S.BasinCentre) - FVector2D(S.BasinHalfExtent), Hi = FVector2D(S.BasinCentre) + FVector2D(S.BasinHalfExtent);
	auto RouteDistance = [this](float X, float Y)
	{
		float Best = 1e9f;
		for (const FTDCourseKeepOut& K : KeepOuts)
		{
			if (K.Kind != TEXT("Route")) { continue; }
			for (int32 i = 0; i + 1 < K.Points.Num(); ++i)
			{
				const FVector C = FMath::ClosestPointOnSegment(FVector(X, Y, 0.f), FVector(K.Points[i].X, K.Points[i].Y, 0.f), FVector(K.Points[i + 1].X, K.Points[i + 1].Y, 0.f));
				Best = FMath::Min(Best, FVector2D::Distance(FVector2D(C.X, C.Y), FVector2D(X, Y)) - K.HalfWidth);
			}
		}
		return Best;	// beyond the route's walking half width
	};
	const float Base = Planet == ETDDestination::Verdance ? (bFill ? 1.f : 0.85f) : (bFill ? 0.75f : 0.5f);	// #194: fuller
	for (int32 iy = FMath::FloorToInt(Lo.Y / Spacing); iy <= FMath::CeilToInt(Hi.Y / Spacing); ++iy)
	{
		for (int32 ix = FMath::FloorToInt(Lo.X / Spacing); ix <= FMath::CeilToInt(Hi.X / Spacing); ++ix)
		{
			const float X = (ix + TDScatter_Hash01(Seed, ix, iy, 1)) * Spacing;
			const float Y = (iy + TDScatter_Hash01(Seed, ix, iy, 2)) * Spacing;
			if (X < Lo.X || X > Hi.X || Y < Lo.Y || Y > Hi.Y) { continue; }
			// every keep-out except the routes (sites, encounters, spawn rings, triggers, structures, the bridge, the pad);
			// the routes themselves get sparse non-colliding cover so the walked line reads trodden, not bare
			bool bClear = true;
			for (const FTDCourseKeepOut& K : KeepOuts)
			{
				if (K.Kind == TEXT("Route") || !K.Box.IsValid) { continue; }
				// #194 (Lead): non-colliding cover goes inside encounter, spawn-ring and trigger boxes too; the pad only keeps its deck
				if (bFill && (K.Kind == TEXT("Encounter") || K.Kind == TEXT("Spawn"))) { continue; }
				if (bFill && K.Kind == TEXT("Pad")) { if (FVector2D::Distance(FVector2D(X, Y), FVector2D(K.Box.GetCenter())) < 1700.f) { bClear = false; break; } continue; }
				if (X > K.Box.Min.X - 150.f && X < K.Box.Max.X + 150.f && Y > K.Box.Min.Y - 150.f && Y < K.Box.Max.Y + 150.f) { bClear = false; break; }
			}
			if (!bClear) { continue; }
			// trodden: 15 % on the walked centre line (distance -700), rising to full ~4 m beyond the route's edge
			// #194: the walked 4 m strip (2 m either side of the centre line) bare, full cover from 3 m out
			const float Trodden = bFill ? FMath::SmoothStep(-500.f, -400.f, RouteDistance(X, Y)) : FMath::Lerp(0.15f, 1.f, FMath::SmoothStep(-500.f, 400.f, RouteDistance(X, Y)));
			// clumps, not an even carpet: the 30 m noise gathers the cover into patches with bare ground between
			const float Patch = FMath::Clamp(0.55f + 0.8f * FTDTerrainField::Noise2D(X / 3000.f, Y / 3000.f, S.Seed + 777), 0.f, 1.f);
			const float Roll = TDScatter_Hash01(Seed, ix, iy, 3);
			if (Roll > Base * Trodden * (bFill ? FMath::Max(Patch, 0.55f) : Patch) + (bFill && Trodden <= 0.f ? 0.f : 0.06f)) { continue; }	// a few stones even on the trodden edges
			const float Pk = TDScatter_Hash01(Seed, ix, iy, 4);
			const int32 Kind = (bFill && R.FillStumps.Num() && Pk > 0.97f) ? TDScatter_OneOf(R.FillStumps, (Pk - 0.97f) / 0.03f)
				: (bFill && R.FillLow.Num() && Pk > 0.75f) ? TDScatter_OneOf(R.FillLow, (Pk - 0.75f) / 0.22f)	// CJ: clover / gravel patches everywhere
				: (bFill && R.FillPlants.Num() && Pk > 0.45f) ? TDScatter_OneOf(R.FillPlants, (Pk - 0.45f) / 0.3f)	// #194 (T7): knee-high card plants everywhere
				: Pk < 0.14f ? TDScatter_OneOf(R.CoursePebbles, Pk / 0.14f)
				: (Pk < 0.2f ? TDScatter_OneOf(R.CourseAccent, (Pk - 0.14f) / 0.06f) : TDScatter_OneOf(R.CourseCover, (Pk - 0.2f) / 0.8f));
			if (!Kinds.IsValidIndex(Kind)) { continue; }
			const FTDScatterKind& K = Kinds[Kind];
			const float Sc = FMath::Lerp(K.MinScale, K.MaxScale, TDScatter_Hash01(Seed, ix, iy, 5));
			const float Yaw = TDScatter_Hash01(Seed, ix, iy, 6) * 360.f;
			const FRotator Rot = K.bTumble ? FRotator(TDScatter_Hash01(Seed, ix, iy, 7) * 30.f - 15.f, Yaw, TDScatter_Hash01(Seed, ix, iy, 8) * 30.f - 15.f) : FRotator(0.f, Yaw, 0.f);
			Out.Near[Kind].Add(FTransform(Rot, FVector(X, Y, Field.Height(X, Y) - K.SinkCm * Sc), FVector(Sc)));
			// #194 (Lead): knee-to-waist cover at about double the count in the first 10 m either side of a path (outside the
			// bare 4 m strip): a second piece in the same cell, offset half a cell, a little larger
			if (bFill)
			{
				const float Rd = RouteDistance(X, Y);	// beyond the route keep-out's 7 m half width
				if (Rd > -500.f && Rd < 300.f)
				{
					const float X2 = X + (TDScatter_Hash01(Seed, ix, iy, 11) - 0.5f) * Spacing, Y2 = Y + (TDScatter_Hash01(Seed, ix, iy, 12) - 0.5f) * Spacing;
					const float Rd2 = RouteDistance(X2, Y2);
					const float Pk2 = TDScatter_Hash01(Seed, ix, iy, 13);
					const int32 Kind2 = (R.FillFerns.Num() && Pk2 < 0.4f) ? TDScatter_OneOf(R.FillFerns, Pk2 / 0.4f) : (R.FillPlants.Num() && Pk2 < 0.6f) ? TDScatter_OneOf(R.FillPlants, (Pk2 - 0.4f) / 0.2f) : (R.FillStumps.Num() && Pk2 > 0.94f) ? TDScatter_OneOf(R.FillStumps, (Pk2 - 0.94f) / 0.06f)
						: (Pk2 < 0.12f ? TDScatter_OneOf(R.CoursePebbles, Pk2 / 0.12f) : (Pk2 < 0.3f ? TDScatter_OneOf(R.CourseAccent, (Pk2 - 0.12f) / 0.18f) : TDScatter_OneOf(R.CourseCover, (Pk2 - 0.3f) / 0.64f)));
					if (R.FillPlants.Num() && TDScatter_Hash01(Seed, ix, iy, 21) < 0.35f)	// CJ frame times (+90 %): the third near-path piece only in a third of the cells
					{
						const float X3 = X - (TDScatter_Hash01(Seed, ix, iy, 16) - 0.5f) * Spacing, Y3 = Y - (TDScatter_Hash01(Seed, ix, iy, 17) - 0.5f) * Spacing;
						const float Pk3 = TDScatter_Hash01(Seed, ix, iy, 18);
						const int32 Kind3 = (R.FillFerns.Num() && Pk3 < 0.5f) ? TDScatter_OneOf(R.FillFerns, Pk3 / 0.5f) : TDScatter_OneOf(R.FillPlants, (Pk3 - 0.5f) / 0.5f);
						if (RouteDistance(X3, Y3) > -500.f && Kinds.IsValidIndex(Kind3))
						{
							const FTDScatterKind& K3 = Kinds[Kind3];
							const float Sc3 = FMath::Lerp(K3.MinScale, K3.MaxScale, TDScatter_Hash01(Seed, ix, iy, 19));
							Out.Near[Kind3].Add(FTransform(FRotator(0.f, TDScatter_Hash01(Seed, ix, iy, 20) * 360.f, 0.f), FVector(X3, Y3, Field.Height(X3, Y3) - K3.SinkCm * Sc3), FVector(Sc3)));
						}
					}
					if (Rd2 > -500.f && Kinds.IsValidIndex(Kind2))
					{
						const FTDScatterKind& K2 = Kinds[Kind2];
						const float Sc2 = FMath::Lerp(K2.MinScale, K2.MaxScale, TDScatter_Hash01(Seed, ix, iy, 14)) * 1.15f;
						Out.Near[Kind2].Add(FTransform(FRotator(0.f, TDScatter_Hash01(Seed, ix, iy, 15) * 360.f, 0.f), FVector(X2, Y2, Field.Height(X2, Y2) - K2.SinkCm * Sc2), FVector(Sc2)));
					}
				}
			}
		}
	}
}

void FTDScatterPlan::RunCourseFill(const FTDTerrainField& Field, FTDScatterResult& Out) const
{
	// #194 (Ryan, Nov 14: "way too empty... it should be almost completely filled up"): inside the course's ground box the
	// zone layer places nothing, so the pad, the sites and the routes stood in open ground. Fill it from the pilot's eye:
	// forest stands (Verdance) or charred groves, crystal stands and column groups (Cinder) up to the routes' clearance
	// (IsCourseClear: the route half width plus each kind's clear radius, ~6-8 m from a path's edge for trees), boulder
	// fields and outcrops, and a landmark rock every ~40 m. Clustered by two noises, not an even sprinkle. Near bucket:
	// trunks and rocks block on foot like the zone layer's.
	const FTDScatterRanges& R = TDScatter_Ranges(Planet);
	if (KeepOuts.Num() == 0) { return; }	// no course: nothing to fill
	const FTDTerrainStyle& S = Field.GetStyle();
	const FVector2D Lo = FVector2D(S.BasinCentre) - FVector2D(S.BasinHalfExtent) - FVector2D(KeepOutMargin, KeepOutMargin);
	const FVector2D Hi = FVector2D(S.BasinCentre) + FVector2D(S.BasinHalfExtent) + FVector2D(KeepOutMargin, KeepOutMargin);
	const bool bV = Planet == ETDDestination::Verdance;
	const float Floor = S.BasinFloor;
	auto Place = [&](int32 Kind, float X, float Y, int32 Seed, int32 ix, int32 iy, float ScaleMul)
	{
		// Lead (Cinder pad views were a wall of tall yellow crystals): within 60 m of the pad a third of the tall crystals and
		// spires become dark shapes (basalt columns, CinderRock outcrops, charred trees) so the glow has gaps and the camp
		// and landmarks stay visible
		if (!bV && FVector2D(X, Y).Size() < 6000.f && (R.Spires.Contains(Kind) || (R.Crystals.Contains(Kind) && R.Crystals.IndexOfByKey(Kind) >= 2))
			&& TDScatter_Hash01(Seed, ix, iy, 31) < 0.7f)	// Lead: pad_W still a wall: two thirds
		{
			const float Sw = TDScatter_Hash01(Seed, ix, iy, 32);
			Kind = Sw < 0.2f && R.FillCliffs.Num() ? TDScatter_OneOf(R.FillCliffs, Sw / 0.2f) : Sw < 0.4f && R.FillOutcrops.Num() ? TDScatter_OneOf(R.FillOutcrops, Sw / 0.4f) : (Sw < 0.75f ? TDScatter_OneOf(R.Columns, (Sw - 0.4f) / 0.35f) : TDScatter_OneOf(R.CharTrees, (Sw - 0.75f) / 0.25f));
			ScaleMul = FMath::Min(ScaleMul, 0.7f);
		}
		if (!Kinds.IsValidIndex(Kind)) { return; }
		const FTDScatterKind& K = Kinds[Kind];
		// Fill's own clearance (Lead, #194: "keep the 4 m path corridors clear"; trees within 6-8 m of a path): a route
		// keeps a 2 m half width (not the zone layer's 7 m), every other keep-out (sites, encounters, spawn points, the
		// pad, structures) as before; the kind's clear radius is capped at 6 m so big rocks still reach the routes' sides.
		// #194 (Lead): colliding pieces up to 3 m from a path's centre line (the trunk or rock body, not its canopy), every
		// structure / site / pad keep-out as before, and inside an encounter box only in its outer 15 % (fights stay open);
		// spawn points stay clear.
		const float Rc = FMath::Min(K.ClearRadiusCm * ScaleMul, 600.f);
		const float Body = K.bBlocksOnFoot ? FMath::Min(Rc, 100.f) : 0.f;
		// T7 (relief1 walk: the pilot jammed on a landmark walking straight at the marker): colliding pieces also stay
		// 4 m + body off the straight walk lines, from the pad to each objective (each route's end) and from each objective
		// to the next
		if (K.bBlocksOnFoot)
		{
			FVector2D Prev(0.f, 0.f);
			for (const FTDCourseKeepOut& KO : KeepOuts)
			{
				if (KO.Kind != TEXT("Route") || KO.Points.Num() < 2) { continue; }
				const FVector2D End(KO.Points.Last());
				for (const FVector2D& From : { FVector2D(0.f, 0.f), Prev })
				{
					const FVector C = FMath::ClosestPointOnSegment(FVector(X, Y, 0.f), FVector(From, 0.f), FVector(End, 0.f));
					if (FVector2D::Distance(FVector2D(C.X, C.Y), FVector2D(X, Y)) < 400.f + Body) { return; }
				}
				Prev = End;
			}
		}
		for (const FTDCourseKeepOut& KO : KeepOuts)
		{
			if (KO.Points.Num() > 1)
			{
				for (int32 i = 0; i + 1 < KO.Points.Num(); ++i)
				{
					const FVector C = FMath::ClosestPointOnSegment(FVector(X, Y, 0.f), FVector(KO.Points[i].X, KO.Points[i].Y, 0.f), FVector(KO.Points[i + 1].X, KO.Points[i + 1].Y, 0.f));
					if (FVector2D::Distance(FVector2D(C.X, C.Y), FVector2D(X, Y)) < 200.f + FMath::Max(Body, 100.f)) { return; }
				}
				continue;
			}
			if (!KO.Box.IsValid) { continue; }
			if (KO.Kind == TEXT("Encounter"))
			{
				const FVector2D Ce(KO.Box.GetCenter()), Ex(KO.Box.GetExtent());
				const float Ux = FMath::Abs(X - Ce.X) / FMath::Max(Ex.X, 1.f), Uy = FMath::Abs(Y - Ce.Y) / FMath::Max(Ex.Y, 1.f);
				if (Ux < 0.7f && Uy < 0.7f) { return; }	// the open middle of a fight (Lead: colliding pieces in the outer 30 %)
				continue;	// the outer ring, or outside: fine
			}
			if (X > KO.Box.Min.X - Rc && X < KO.Box.Max.X + Rc && Y > KO.Box.Min.Y - Rc && Y < KO.Box.Max.Y + Rc) { return; }
		}
		const FTDTerrainSample Sm = Field.Sample(X, Y, 300.f);
		if (Sm.Slope > 0.6f) { return; }
		const float Sc = FMath::Lerp(K.MinScale, K.MaxScale, TDScatter_Hash01(Seed, ix, iy, 5)) * ScaleMul;
		const float Yaw = TDScatter_Hash01(Seed, ix, iy, 6) * 360.f;
		const FRotator Rot = K.bTumble ? FRotator(TDScatter_Hash01(Seed, ix, iy, 7) * 30.f - 15.f, Yaw, TDScatter_Hash01(Seed, ix, iy, 8) * 30.f - 15.f)
			: FRotator(Sm.Normal.Y * 6.f, Yaw, -Sm.Normal.X * 6.f);
		Out.Near[Kind].Add(FTransform(Rot, FVector(X, Y, Sm.Height - K.SinkCm * Sc * K.ZScale), FVector(Sc, Sc, Sc * K.ZScale)));
	};
	auto Grid = [&](float Spacing, int32 Salt, TFunctionRef<void(float, float, int32, int32, int32)> Fn)
	{
		const int32 Seed = S.Seed * 6007 + Salt;
		for (int32 iy = FMath::FloorToInt(Lo.Y / Spacing); iy <= FMath::CeilToInt(Hi.Y / Spacing); ++iy)
		{
			for (int32 ix = FMath::FloorToInt(Lo.X / Spacing); ix <= FMath::CeilToInt(Hi.X / Spacing); ++ix)
			{
				const float X = (ix + TDScatter_Hash01(Seed, ix, iy, 1)) * Spacing;
				const float Y = (iy + TDScatter_Hash01(Seed, ix, iy, 2)) * Spacing;
				if (X < Lo.X || X > Hi.X || Y < Lo.Y || Y > Hi.Y) { continue; }
				Fn(X, Y, ix, iy, Seed);
			}
		}
	};
	// stands: a 60 m noise (where the forest / groves are) and a 20 m one (their ragged edges)
	auto Stand = [&](float X, float Y) { return FMath::Clamp(0.5f + 0.75f * FTDTerrainField::Noise2D(X / 6000.f, Y / 6000.f, S.Seed + 1931) + 0.35f * FTDTerrainField::Noise2D(X / 2000.f, Y / 2000.f, S.Seed + 1937), 0.f, 1.f); };
	// 1. trees / charred groves, crystal stands and columns: 6.5 m grid, dense in the stands, a thinner scatter between
	Grid(650.f, 11, [&](float X, float Y, int32 ix, int32 iy, int32 Seed)
	{
		const float St = Stand(X, Y);
		const float Roll = TDScatter_Hash01(Seed, ix, iy, 3);
		if (Roll > (St > 0.55f ? 0.85f : 0.22f)) { return; }
		const float Pk = TDScatter_Hash01(Seed, ix, iy, 4);
		if (bV)
		{
			if (R.Trees == INDEX_NONE) { return; }
			const float Moist = Field.Sample(X, Y, 600.f).BroadMask;
			const int32 Species = Moist > 0.55f ? (Pk < 0.2f ? Kapok : (Pk < 0.45f ? BrazilNut : (Pk < 0.7f ? TrumpetTree : (Pk < 0.85f ? Cacao : TreeFern))))
				: (Pk < 0.3f ? StiltPalm : (Pk < 0.6f ? TreeFern : (Pk < 0.8f ? Cacao : TrumpetTree)));
			const float Stage = TDScatter_Hash01(Seed, ix, iy, 9);
			Place(R.Trees + Species * 3 + (Stage < 0.45f ? 0 : (Stage < 0.85f ? 1 : 2)), X, Y, Seed, ix, iy, FMath::Lerp(0.8f, 1.25f, TDScatter_Hash01(Seed, ix, iy, 10)));
		}
		else
		{
			// Cinder: charred trees and crystal stands in the stands, column groups where the second noise peaks
			const float Col = FTDTerrainField::Noise2D(X / 3000.f, Y / 3000.f, S.Seed + 1949);
			const int32 Kind = Col > 0.35f ? TDScatter_OneOf(R.Columns, Pk) : (Pk < 0.55f ? TDScatter_OneOf(R.CharTrees, Pk / 0.55f) : TDScatter_OneOf(R.Crystals, (Pk - 0.55f) / 0.45f));
			Place(Kind, X, Y, Seed, ix, iy, Col > 0.35f ? 0.6f : 1.f);
		}
	});
	// 1b. #194: frame every path: walking each route every ~6 m, a clump of trees (Verdance) or charred trees, columns and
	// CinderRock mounds (Cinder) 3-6 m either side, so from any route point something stands within 10 m on both sides.
	{
		const int32 Seed = S.Seed * 6007 + 71;
		int32 n = 0;
		for (const FTDCourseKeepOut& KO : KeepOuts)
		{
			if (KO.Kind != TEXT("Route") || KO.Points.Num() < 2) { continue; }
			for (int32 i = 0; i + 1 < KO.Points.Num(); ++i)
			{
				const FVector2D A(KO.Points[i]), B(KO.Points[i + 1]);
				const float Len = FVector2D::Distance(A, B);
				const FVector2D D = (B - A) / FMath::Max(Len, 1.f), Side(-D.Y, D.X);
				for (float T = 0.f; T < Len; T += 600.f, ++n)
				{
					for (int32 sgn = -1; sgn <= 1; sgn += 2)
					{
						if (TDScatter_Hash01(Seed, n, sgn, 3) > 0.7f) { continue; }
						const float Off = 300.f + TDScatter_Hash01(Seed, n, sgn, 4) * 300.f;
						const FVector2D P = A + D * (T + (TDScatter_Hash01(Seed, n, sgn, 9) - 0.5f) * 300.f) + Side * (Off * sgn);
						const float Pk = TDScatter_Hash01(Seed, n, sgn, 10);
						int32 Kind = INDEX_NONE;
						if (bV)
						{
							if (R.Trees == INDEX_NONE) { continue; }
							const int32 Species = Pk < 0.25f ? Kapok : (Pk < 0.5f ? BrazilNut : (Pk < 0.7f ? TrumpetTree : (Pk < 0.85f ? TreeFern : StiltPalm)));
							Kind = (Pk > 0.86f && R.FillLogs.Num()) ? TDScatter_OneOf(R.FillLogs, (Pk - 0.86f) / 0.06f) : Pk > 0.92f ? TDScatter_OneOf(R.Rocks, (Pk - 0.92f) / 0.08f) : R.Trees + Species * 3 + (TDScatter_Hash01(Seed, n, sgn, 11) < 0.5f ? 1 : 2);
						}
						else
						{
							Kind = Pk < 0.35f ? TDScatter_OneOf(R.CharTrees, Pk / 0.35f) : (Pk < 0.7f && R.FillOutcrops.Num() ? TDScatter_OneOf(R.FillOutcrops, (Pk - 0.35f) / 0.35f) : TDScatter_OneOf(R.Crystals, Pk));
						}
						Place(Kind, P.X, P.Y, Seed, n, sgn + 7, 1.f);
					}
				}
			}
		}
	}
	// 1c. #194 (T7: the player walks straight to the marker, not only along the route lines): a clump on a 12 m jittered grid
	// over the whole course box: one tree or rock with one or two smaller pieces round it, wherever Place allows (outside
	// fight centres, pads, spawn points and structures).
	Grid(1200.f, 53, [&](float X, float Y, int32 ix, int32 iy, int32 Seed)
	{
		if (TDScatter_Hash01(Seed, ix, iy, 3) > 0.85f) { return; }
		const float Pk = TDScatter_Hash01(Seed, ix, iy, 4);
		int32 Main = INDEX_NONE;
		if (bV)
		{
			const int32 Species = Pk < 0.2f ? Kapok : (Pk < 0.4f ? BrazilNut : (Pk < 0.55f ? TrumpetTree : (Pk < 0.7f ? Cacao : (Pk < 0.85f ? TreeFern : StiltPalm))));
			Main = (Pk > 0.78f && R.FillOutcrops.Num()) ? TDScatter_OneOf(R.FillOutcrops, TDScatter_Hash01(Seed, ix, iy, 9)) : (R.Trees != INDEX_NONE ? R.Trees + Species * 3 + (TDScatter_Hash01(Seed, ix, iy, 9) < 0.5f ? 1 : 2) : INDEX_NONE);
		}
		else
		{
			Main = Pk < 0.35f ? TDScatter_OneOf(R.CharTrees, Pk / 0.35f) : (Pk < 0.75f && R.FillOutcrops.Num() ? TDScatter_OneOf(R.FillOutcrops, (Pk - 0.35f) / 0.4f) : TDScatter_OneOf(R.Crystals, Pk));
		}
		Place(Main, X, Y, Seed, ix, iy, 1.f);
		for (int32 j = 0; j < 2; ++j)
		{
			const float A = TDScatter_Hash01(Seed, ix, iy, 20 + j) * 2.f * PI, D = 250.f + 250.f * TDScatter_Hash01(Seed, ix, iy, 22 + j);
			const float Pj = TDScatter_Hash01(Seed, ix, iy, 24 + j);
			const int32 Small = bV ? (Pj < 0.5f ? TDScatter_OneOf(R.Rocks, Pj / 0.5f) : TDScatter_OneOf(R.Grove, (Pj - 0.5f) / 0.5f)) : (Pj < 0.5f ? TDScatter_OneOf(R.Rocks, Pj / 0.5f) : TDScatter_OneOf(R.Crystals, (Pj - 0.5f) / 0.5f));
			Place(Small, X + FMath::Cos(A) * D, Y + FMath::Sin(A) * D, Seed, ix * 3 + j + 1, iy * 3 + 1, bV ? 0.55f : 0.7f);
		}
	});
	// 2. boulder fields and outcrops: 4.5 m grid, only where a 30 m noise is up (fields, not a sprinkle)
	Grid(450.f, 23, [&](float X, float Y, int32 ix, int32 iy, int32 Seed)
	{
		const float F = FTDTerrainField::Noise2D(X / 3000.f, Y / 3000.f, S.Seed + 1951);
		if (F < 0.25f || TDScatter_Hash01(Seed, ix, iy, 3) > 0.55f) { return; }
		const float Pk = TDScatter_Hash01(Seed, ix, iy, 4);
		const int32 RockKind = (R.FillOutcrops.Num() && Pk > (bV ? 0.65f : 0.5f)) ? TDScatter_OneOf(R.FillOutcrops, (Pk - (bV ? 0.65f : 0.5f)) / (bV ? 0.35f : 0.5f))
			: (Pk < 0.6f ? TDScatter_OneOf(R.Rocks, Pk / 0.6f) : TDScatter_OneOf(R.CliffRocks, (Pk - 0.6f) / 0.4f));
		Place(RockKind, X, Y, Seed, ix, iy, 0.7f);
	});
	// 3. groves (Verdance giant mushrooms and glow plants) / crystal clusters (Cinder): 9 m grid in the grove noise
	Grid(900.f, 37, [&](float X, float Y, int32 ix, int32 iy, int32 Seed)
	{
		const float G = FTDTerrainField::Noise2D(X / 2500.f, Y / 2500.f, S.Seed + 1973);
		if (G < 0.2f || TDScatter_Hash01(Seed, ix, iy, 3) > 0.7f) { return; }
		const float Pk = TDScatter_Hash01(Seed, ix, iy, 4);
		const int32 Kind = bV ? (Pk < 0.5f ? TDScatter_OneOf(R.Grove, Pk / 0.5f) : TDScatter_OneOf(R.GroveGlow, (Pk - 0.5f) / 0.5f)) : TDScatter_OneOf(R.Crystals, Pk);
		Place(Kind, X, Y, Seed, ix, iy, bV ? 0.8f : 1.f);
	});
	// 4. landmarks: one big rock or spire per ~40 m cell
	Grid(4000.f, 41, [&](float X, float Y, int32 ix, int32 iy, int32 Seed)
	{
		const float Pk = TDScatter_Hash01(Seed, ix, iy, 4);
		const int32 Kind = bV ? TDScatter_OneOf(R.Rocks, Pk) : (Pk < 0.5f ? TDScatter_OneOf(R.Spires, Pk / 0.5f) : TDScatter_OneOf(R.CliffRocks, (Pk - 0.5f) / 0.5f));
		Place(Kind, X, Y, Seed, ix, iy, bV ? 1.6f : 1.f);
	});
	(void)Floor;
}

float FTDScatterPlan::ScarWeight(float X, float Y, float Margin) const
{
	if (ScarHalfB <= 0.f) { return 0.f; }
	const FVector2D P(X, Y), D = ScarB - ScarA;
	const float Len2 = FMath::Max(D.SizeSquared(), 1.f);
	const float T = FVector2D::DotProduct(P - ScarA, D) / Len2;
	if (T < 0.f || T > 1.f) { return 0.f; }
	const float Half = FMath::Lerp(ScarHalfA, ScarHalfB, T);
	// a ragged edge: the half width breathes with a slow noise along the strip
	const float Edge = Half * (0.8f + 0.4f * (FTDTerrainField::Noise2D(T * 18.f, 3.1f, 4411) * 0.5f + 0.5f)) + Margin;
	const float Dist = FVector2D::Distance(P, ScarA + D * T);
	return FMath::Clamp(1.f - Dist / FMath::Max(Edge, 1.f), 0.f, 1.f) * FMath::Clamp(T * 20.f, 0.f, 1.f);
}

bool FTDScatterPlan::IsPatch(const FTDTerrainSample& Sm) const
{
	return Planet == ETDDestination::Cinder ? Sm.FineMask > ObsidianFineMin
		: (Sm.BroadMask < GroveBroadMax && Sm.FineMask > GroveFineMin);
}

int32 FTDScatterPlan::Pick(int32 Mode, const FTDTerrainSample& Sm, float HeightM, float Rnd, float Stand) const
{
	const FTDScatterRanges& R = TDScatter_Ranges(Planet);
	const bool bPatch = IsPatch(Sm);
	if (Planet == ETDDestination::Verdance)
	{
		const float Moist = Sm.BroadMask;
		switch (Mode)
		{
		case ModeFar:
		case ModeDense:
		{
			// Trees: rainforest / mixed / open by the biome mask; thinning up the slopes and above ~800 m over the pad (most of
			// the zone is 400-1000 m up), none above 1.5 km; none in a grove (the giant mushrooms stand there).
			if (Sm.Slope > 0.55f || HeightM > 1500.f || bPatch || R.Trees == INDEX_NONE) { return INDEX_NONE; }
			const float Base = Mode == ModeDense ? (Moist > RainforestBroad ? 0.92f : (Moist > MixedBroad ? 0.45f : 0.08f))
				: (Moist > RainforestBroad ? 0.9f : (Moist > MixedBroad ? 0.5f : 0.2f));
			// The far tier follows the canopy stands: thicker along a stand's edge (the trees that frame it), thinner in the gaps.
			const float StandMul = Mode == ModeFar ? (FMath::Abs(Stand - 0.1f) < 0.15f ? 1.6f : (Stand > 0.1f ? 1.f : 0.6f)) : 1.f;
			const float Density = FMath::Min(1.f, Base * StandMul) * FMath::Clamp(1.f - (Sm.Slope - 0.2f) * 2.5f, 0.f, 1.f) * FMath::Clamp(1.f - (HeightM - 800.f) / 700.f, 0.f, 1.f);
			if (Rnd > Density) { return INDEX_NONE; }
			const float Pk = Rnd / FMath::Max(Density, 0.01f);
			int32 Species;
			if (Moist > RainforestBroad) { Species = Pk < 0.15f ? Kapok : (Pk < 0.45f ? BrazilNut : (Pk < 0.75f ? TrumpetTree : Cacao)); }
			else if (Moist > MixedBroad) { Species = Pk < 0.35f ? StiltPalm : (Pk < 0.7f ? TreeFern : Cacao); }
			else { Species = Pk < 0.6f ? TreeFern : StiltPalm; }
			const float StageRoll = FMath::Frac(Pk * 7.31f);
			return R.Trees + Species * 3 + (StageRoll < 0.6f ? 0 : (StageRoll < 0.9f ? 1 : 2));
		}
		case ModeCanopy:
		{
			// s6: continuous stands over ~35-45 % of the vegetated lowland, with clearings: the stand noise against a threshold
			// that drops with moisture (rainforest ~50 %, open meadow ~30 %) and in the low ground (valleys and drainage, the
			// bottom 300 m over the pad), full density inside a stand so crowns overlap; none on slopes over ~25 degrees
			// (s6 sheet: that left the hills bare; now ~34 degrees, Slope 0.17) nor on the heights; RunLayer's contact check
			// drops any clump whose footprint spans a ridge edge, so no crown overhangs a cliff top.
			const float Valley = FMath::Clamp(1.f - HeightM / 300.f, 0.f, 1.f);
			const float Threshold = 0.24f - 0.3f * Moist - 0.15f * Valley;	// s8: coverage test 58 % at 0.04, 54 % at 0.09, 51 % at 0.13, 40 % at 0.24 (1475 clumps)
			if (Stand < Threshold) { return INDEX_NONE; }
			const float Density = FMath::Clamp((Stand - Threshold) * 12.f, 0.f, 1.f) * FMath::Clamp((0.17f - Sm.Slope) / 0.04f, 0.f, 1.f)
				* FMath::Clamp(1.f - (HeightM - 900.f) / 500.f, 0.f, 1.f);
			if (Rnd >= Density) { return INDEX_NONE; }
			const float Pk = Rnd / FMath::Max(Density, 0.01f);
			return Pk < 0.12f ? TDScatter_OneOf(R.CanopyAccent, Pk / 0.12f) : TDScatter_OneOf(R.Canopy, (Pk - 0.12f) / 0.88f);
		}
		case ModeRock:
		{
			// Boulders sparse everywhere; big boulders at cliff feet (moderate slopes under steep ones) and on the heights.
			const float Cliff = FMath::Clamp((Sm.Slope - 0.28f) * 4.f, 0.f, 1.f) * FMath::Clamp((0.65f - Sm.Slope) * 6.f, 0.f, 1.f);
			// s6: a scree skirt where the meadow meets a cliff (the 15-35 degree band under the walls), so the cliff no
			// longer meets the grass at a hard seam
			const float Foot = FMath::Clamp((Sm.Slope - 0.035f) * 20.f, 0.f, 1.f) * FMath::Clamp((0.2f - Sm.Slope) * 10.f, 0.f, 1.f);
			const float Density = 0.05f + 0.45f * Cliff + 0.22f * Foot + 0.12f * FMath::Clamp((HeightM - 700.f) / 600.f, 0.f, 1.f);
			if (Rnd > Density) { return INDEX_NONE; }
			const float Pk = Rnd / FMath::Max(Density, 0.01f);
			return (Cliff > 0.3f || Foot > 0.5f) ? TDScatter_OneOf(R.CliffRocks, Pk) : TDScatter_OneOf(R.Rocks, Pk);
		}
		case ModeGrove:
		{
			if (!bPatch || Sm.Slope > 0.35f || HeightM > 700.f) { return INDEX_NONE; }
			if (Rnd > 0.6f) { return INDEX_NONE; }
			const float Pk = Rnd / 0.6f;
			return Pk < 0.55f ? TDScatter_OneOf(R.Grove, Pk / 0.55f) : TDScatter_OneOf(R.GroveGlow, (Pk - 0.55f) / 0.45f);
		}
		case ModeGround:
		{
			if (Sm.Slope > 0.5f || HeightM > 1300.f) { return INDEX_NONE; }
			const float Density = bPatch ? 0.8f : 0.55f;
			if (Rnd > Density) { return INDEX_NONE; }
			const float Pk = Rnd / Density;
			if (bPatch) { return TDScatter_OneOf(R.GroundGlow, Pk); }
			if (Pk < 0.12f) { return TDScatter_OneOf(R.GroundGlow, Pk / 0.12f); }	// a few glowing accents everywhere
			return Moist > MixedBroad ? TDScatter_OneOf(R.GroundForest, (Pk - 0.12f) / 0.88f) : TDScatter_OneOf(R.Ground, (Pk - 0.12f) / 0.88f);
		}
		default:
			return INDEX_NONE;
		}
	}
	// Cinder: never in the lava rivers (the broad mask is the river noise there: 0.5 on a river's centre line).
	if (FMath::Abs(Sm.BroadMask - 0.5f) < 0.05f) { return INDEX_NONE; }
	switch (Mode)
	{
	case ModeFar:
	{
		// Basalt fields: columns on the flats at mid height, a few charred trees.
		if (Sm.Slope > 0.3f || HeightM > 1100.f) { return INDEX_NONE; }
		// pass 3: clusters of large columns (they read at 300-900 m): dense where the stand noise is up, none elsewhere.
		const float Col = (Stand > 0.15f ? 0.55f : 0.04f) * FMath::Clamp(1.f - Sm.Slope * 3.f, 0.f, 1.f);
		const float Char = 0.06f * FMath::Clamp((HeightM - 50.f) / 200.f, 0.f, 1.f);
		if (Rnd < Col) { return TDScatter_OneOf(R.Columns, Rnd / FMath::Max(Col, 0.01f)); }
		if (Rnd < Col + Char) { return TDScatter_OneOf(R.CharTrees, (Rnd - Col) / FMath::Max(Char, 0.01f)); }
		return INDEX_NONE;
	}
	case ModeCanopy:	// crystal spires on the obsidian patches, and a few on the flats as landmarks
		return (Sm.Slope < 0.4f && Rnd < (bPatch ? 0.7f : 0.04f)) ? TDScatter_OneOf(R.Spires, Rnd / (bPatch ? 0.7f : 0.04f)) : INDEX_NONE;
	case ModeRock:
	{
		const float Cliff = FMath::Clamp((Sm.Slope - 0.25f) * 4.f, 0.f, 1.f) * FMath::Clamp((0.65f - Sm.Slope) * 6.f, 0.f, 1.f);
		const float Density = 0.06f + 0.5f * Cliff;
		if (Rnd > Density) { return INDEX_NONE; }
		const float Pk = Rnd / FMath::Max(Density, 0.01f);
		if (Cliff > 0.3f) { return Pk < 0.08f ? TDScatter_OneOf({ R.Rocks[1], R.Rocks[2] }, Pk / 0.08f) : TDScatter_OneOf(R.CliffRocks, Pk); }
		return R.Rocks.Num() ? R.Rocks[0] : INDEX_NONE;
	}
	case ModeGrove:	// crystal clusters on the obsidian patches
	{
		// s9: denser in a patch's core (fine mask well over the threshold), so the fields cluster instead of sprinkling
		const float Core = FMath::Clamp((Sm.FineMask - ObsidianFineMin) / 0.12f, 0.f, 1.f);
		const float D = 0.3f + 0.55f * Core;
		return (bPatch && Sm.Slope < 0.45f && Rnd < D) ? TDScatter_OneOf(R.Crystals, Rnd / D) : INDEX_NONE;
	}
	case ModeGround:
	{
		if (Sm.Slope > 0.5f) { return INDEX_NONE; }
		const float Density = bPatch ? 0.6f : 0.3f;
		if (Rnd > Density) { return INDEX_NONE; }
		const float Pk = Rnd / Density;
		return bPatch && Pk < 0.6f ? TDScatter_OneOf(R.GroundGlow, Pk / 0.6f) : TDScatter_OneOf(R.Rubble, Pk);
	}
	default:
		return INDEX_NONE;
	}
}

void FTDScatterPlan::RunLayer(const FTDTerrainField& Field, float Spacing, int32 Salt, const FBox2D& Rect, int32 Mode, bool bParallel,
	TArray<TArray<FTransform>>& OutNear, TArray<TArray<FTransform>>& OutFar) const
{
	const FTDTerrainStyle& S = Field.GetStyle();
	const int32 Seed = S.Seed * 7919 + 17;
	const float Floor = S.BasinFloor;
	// The grid is global (indices from the origin), so a cell of a ring and the zone layer line up without seams.
	const int32 X0 = FMath::FloorToInt(Rect.Min.X / Spacing), X1 = FMath::CeilToInt(Rect.Max.X / Spacing);
	const int32 Y0 = FMath::FloorToInt(Rect.Min.Y / Spacing), Y1 = FMath::CeilToInt(Rect.Max.Y / Spacing);
	const int32 Rows = FMath::Max(0, Y1 - Y0);
	struct FRow { TArray<TArray<FTransform>> Near, Far; };
	TArray<FRow> RowsOut;
	RowsOut.SetNum(Rows);
	auto DoRow = [&](int32 r)
	{
		FRow& Row = RowsOut[r];
		Row.Near.SetNum(Kinds.Num());
		Row.Far.SetNum(Kinds.Num());
		const int32 iy = Y0 + r;
		for (int32 ix = X0; ix < X1; ++ix)
		{
			const float X = (ix + TDScatter_Hash01(Seed, ix, iy, Salt)) * Spacing;
			const float Y = (iy + TDScatter_Hash01(Seed, ix, iy, Salt + 1)) * Spacing;
			if (X < Rect.Min.X || X >= Rect.Max.X || Y < Rect.Min.Y || Y >= Rect.Max.Y) { continue; }
			const float Zone = ZoneWeight(X, Y);
			if (Zone <= 0.f || TDScatter_Hash01(Seed, ix, iy, Salt + 2) > Zone || IsKeptOut(Field, X, Y)) { continue; }
			if (ScarHalfB > 0.f && (Mode == ModeFar || Mode == ModeDense || Mode == ModeCanopy))
			{
				// a canopy clump is kept out by its whole footprint (its centre can sit 120 m from the scar and still cover it)
				const float Scar = ScarWeight(X, Y, Mode == ModeCanopy ? 13000.f : 0.f);	// s14: 2.6-3.6x clumps reach ~120 m
				if (Scar > 0.f)
				{
					// Inside the scar: no living forest; on the far-tree grid, a charred trunk now and then
					const TArray<int32>& Char = TDScatter_Ranges(Planet).CharTrees;
					// (none in the course's surroundings: by the research site a glowing trunk read as a strange pillar, s8 walk)
					if (Mode == ModeDense || Char.Num() == 0 || TDScatter_Hash01(Seed, ix, iy, Salt + 9) > 0.9f * Scar || IsNearCourse(Field, X, Y)) { continue; }
					const int32 Ck = TDScatter_OneOf(Char, TDScatter_Hash01(Seed, ix, iy, Salt + 10));
					const FTDScatterKind& K = Kinds[Ck];
					if (!IsCourseClear(X, Y, K.ClearRadiusCm * K.MaxScale)) { continue; }
					const float Sc = FMath::Lerp(K.MinScale, K.MaxScale, TDScatter_Hash01(Seed, ix, iy, Salt + 4));
					const FTransform Xf(FRotator(TDScatter_Hash01(Seed, ix, iy, Salt + 6) * 16.f - 8.f, TDScatter_Hash01(Seed, ix, iy, Salt + 5) * 360.f, 0.f),
						FVector(X, Y, Field.Height(X, Y) - K.SinkCm * Sc), FVector(Sc, Sc, Sc * K.ZScale));
					Row.Far[Ck].Add(Xf);
					continue;
				}
			}
			const bool bNearCourse = IsNearCourse(Field, X, Y);
			// The dense forest: the zone layer carries it around the course, the mid ring everywhere else.
			if (Mode == ModeDense && (Salt == 301) != bNearCourse) { continue; }
			const FTDTerrainSample Sm = Field.Sample(X, Y, FMath::Max(Spacing * 0.25f, 200.f));
			if (Sm.Height < S.SeaLevel + 1000.f) { continue; }	// the sea and its shore
			const float Stand = FTDTerrainField::Noise2D(X / 45000.f, Y / 45000.f, S.Seed + 913) * 0.7f + FTDTerrainField::Noise2D(X / 17000.f, Y / 17000.f, S.Seed + 917) * 0.3f;
			const int32 Kind = Pick(Mode, Sm, (Sm.Height - Floor) / 100.f, TDScatter_Hash01(Seed, ix, iy, Salt + 3), Stand);
			if (!Kinds.IsValidIndex(Kind)) { continue; }
			const FTDScatterKind& K = Kinds[Kind];
			if (!IsCourseClear(X, Y, K.ClearRadiusCm * K.MaxScale)) { continue; }
			const float Scale = FMath::Lerp(K.MinScale, K.MaxScale, TDScatter_Hash01(Seed, ix, iy, Salt + 4));
			const float Yaw = TDScatter_Hash01(Seed, ix, iy, Salt + 5) * 360.f;
			const FRotator Rot = K.bTumble ? FRotator(TDScatter_Hash01(Seed, ix, iy, Salt + 6) * 40.f - 20.f, Yaw, TDScatter_Hash01(Seed, ix, iy, Salt + 7) * 40.f - 20.f)
				: FRotator(Sm.Normal.Y * 8.f, Yaw, -Sm.Normal.X * 8.f);
			float Ground = Sm.Height;
			if (K.ContactRadiusCm > 0.f)
			{
				// Seat it on the lowest ground under its footprint (four points around the centre)
				const float Rr = K.ContactRadiusCm * Scale;
				float High = Sm.Height;
				for (const FVector2D& D : { FVector2D(Rr, 0.f), FVector2D(-Rr, 0.f), FVector2D(0.f, Rr), FVector2D(0.f, -Rr) })
				{
					const float H = Field.Height(X + D.X, Y + D.Y);
					Ground = FMath::Min(Ground, H);
					High = FMath::Max(High, H);
				}
				if (High - Ground > 1.0f * Rr) { continue; }	// its footprint spans a ridge or a cliff edge: it would overhang
			}
			const FTransform Xf(Rot, FVector(X, Y, Ground - K.SinkCm * Scale * K.ZScale), FVector(Scale, Scale, Scale * K.ZScale));
			(bNearCourse ? Row.Near : Row.Far)[Kind].Add(Xf);
		}
	};
	if (bParallel) { ParallelFor(Rows, DoRow); }
	else { for (int32 r = 0; r < Rows; ++r) { DoRow(r); } }
	for (FRow& Row : RowsOut)
	{
		for (int32 k = 0; k < Kinds.Num(); ++k)
		{
			OutNear[k].Append(Row.Near[k]);
			OutFar[k].Append(Row.Far[k]);
		}
	}
}

void FTDScatterPlan::Generate(const FTDTerrainField& Field, FTDScatterResult& Out) const
{
	Out.Near.SetNum(Kinds.Num());
	Out.Far.SetNum(Kinds.Num());
	if (Kinds.Num() == 0)
	{
		return;
	}
	const FBox2D Zone(FVector2D(-ZoneRadius, -ZoneRadius), FVector2D(ZoneRadius, ZoneRadius));
	// Salts: 101 far trees, 301 the dense forest (zone layer: around the course; mid ring: the same grid elsewhere),
	// 401 canopy / spires, 211 rocks, 501 groves, 601 ground cover (near ring).
	RunLayer(Field, FarTreeSpacing, 101, Zone, ModeFar, true, Out.Near, Out.Far);
	if (Planet == ETDDestination::Verdance)
	{
		const FTDTerrainStyle& S = Field.GetStyle();
		const FBox2D Around(FVector2D(S.BasinCentre) - FVector2D(S.BasinHalfExtent) - FVector2D(CollisionRadius, CollisionRadius),
			FVector2D(S.BasinCentre) + FVector2D(S.BasinHalfExtent) + FVector2D(CollisionRadius, CollisionRadius));
		RunLayer(Field, DenseTreeSpacing, 301, Around, ModeDense, true, Out.Near, Out.Far);
	}
	RunLayer(Field, CanopySpacing, 401, Zone, ModeCanopy, true, Out.Near, Out.Far);
	RunCourseGround(Field, Out);
	if (CVarTDScatterCourseFill.GetValueOnAnyThread() != 0) { RunCourseFill(Field, Out); }
	if (ScorchKind != INDEX_NONE && ScarHalfB > 0.f)
	{
		// Scorch marks down the scar's centre line every ~22 m, jittered across it and laid on the slope.
		const FVector2D Dir = (ScarB - ScarA).GetSafeNormal();
		const FVector2D Side(-Dir.Y, Dir.X);
		const float Len = FVector2D::Distance(ScarA, ScarB);
		const FTDScatterKind& K = Kinds[ScorchKind];
		int32 i = 0;
		for (float T = 0.f; T < Len; T += 1400.f, ++i)
		{
			const float A = T / Len;
			const float Half = FMath::Lerp(ScarHalfA, ScarHalfB, A);
			const FVector2D P = ScarA + Dir * T + Side * ((TDScatter_Hash01(977, i, 0, 3) - 0.5f) * Half);
			if (IsKeptOut(Field, P.X, P.Y) || !IsCourseClear(P.X, P.Y, K.ClearRadiusCm)) { continue; }
			const FTDTerrainSample Sm = Field.Sample(P.X, P.Y, 800.f);
			if (Sm.Slope > 0.12f) { continue; }	// a flat decal on a wall stands up as a pillar
			const float Sc = FMath::Lerp(K.MinScale, K.MaxScale, TDScatter_Hash01(977, i, 0, 4)) * FMath::Lerp(0.8f, 1.4f, A);
			const FVector Fwd = FVector(FVector2D(Dir.X, Dir.Y).GetRotated((TDScatter_Hash01(977, i, 0, 5) - 0.5f) * 30.f), 0.f);
			const FRotator Rot = FRotationMatrix::MakeFromZX(Sm.Normal, Fwd).Rotator();
			Out.Far[ScorchKind].Add(FTransform(Rot, FVector(P.X, P.Y, Sm.Height + 15.f), FVector(Sc, Sc, 1.f)));
		}
	}
	RunLayer(Field, RockSpacing, 211, Zone, ModeRock, true, Out.Near, Out.Far);
	RunLayer(Field, GroveSpacing, 501, Zone, ModeGrove, true, Out.Near, Out.Far);
}

void FTDScatterPlan::GenerateCell(const FTDTerrainField& Field, ETDScatterRing Ring, FIntPoint Cell, TArray<TArray<FTransform>>& Out) const
{
	Out.SetNum(Kinds.Num());
	const float Size = Ring == ETDScatterRing::Mid ? MidCell : NearCell;
	const FBox2D Rect(FVector2D(Cell.X * Size, Cell.Y * Size), FVector2D((Cell.X + 1) * Size, (Cell.Y + 1) * Size));
	TArray<TArray<FTransform>> Unused;
	Unused.SetNum(Kinds.Num());
	if (Ring == ETDScatterRing::Mid)
	{
		if (Planet == ETDDestination::Verdance)
		{
			RunLayer(Field, DenseTreeSpacing, 302, Rect, ModeDense, false, Unused, Out);	// salt 302 != 301: the ring's half (outside the course)
		}
		return;
	}
	TArray<TArray<FTransform>> Near;
	Near.SetNum(Kinds.Num());
	RunLayer(Field, GroundSpacing, 601, Rect, ModeGround, false, Near, Out);
	for (int32 k = 0; k < Kinds.Num(); ++k) { Out[k].Append(Near[k]); }	// ground cover never collides: one bucket
}

TArray<int32> FTDScatterPlan::RingKinds(ETDScatterRing Ring) const
{
	const FTDScatterRanges& R = TDScatter_Ranges(Planet);
	TArray<int32> Out;
	if (Ring == ETDScatterRing::Mid)
	{
		if (R.Trees != INDEX_NONE) { for (int32 i = 0; i < 18; ++i) { Out.Add(R.Trees + i); } }
		return Out;
	}
	for (const TArray<int32>* L : { &R.Ground, &R.GroundForest, &R.GroundGlow, &R.Rubble }) { for (int32 k : *L) { Out.AddUnique(k); } }
	return Out;
}

// ------------------------------------------------------------------------------------------------ actor

ATDPlanetScatter::ATDPlanetScatter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

ATDPlanetScatter* ATDPlanetScatter::SpawnFor(ATDPlanetTerrain* Terrain, ETDDestination InPlanet)
{
	UWorld* World = Terrain ? Terrain->GetWorld() : nullptr;
	if (!World || CVarTDScatter.GetValueOnGameThread() == 0 || FTDScatterPlan::ForPlanet(InPlanet).Kinds.Num() == 0)
	{
		return nullptr;
	}
	FActorSpawnParameters P;
	P.Owner = Terrain;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPlanetScatter* S = World->SpawnActor<ATDPlanetScatter>(ATDPlanetScatter::StaticClass(), Terrain->GetActorTransform(), P);
	if (S)
	{
		S->AttachToActor(Terrain, FAttachmentTransformRules::KeepWorldTransform);
		S->Planet = InPlanet;
		S->Plan = FTDScatterPlan::ForPlanet(InPlanet);
		S->CaptureKeepOuts();
		S->StartAsync();
	}
	return S;
}

void ATDPlanetScatter::CaptureKeepOuts()
{
	// The course's keep-outs (routes, sites and their clear circles, the pad, encounters), world -> course-local.
	Plan.KeepOuts.Reset();
	const ATDPlanetCourse* Course = ATDPlanetCourse::Get(this);
	if (!Course)
	{
		return;
	}
	const FTransform& Xf = GetActorTransform();
	for (const FTDCourseKeepOut& K : Course->GetKeepOuts())
	{
		FTDCourseKeepOut L = K;
		for (FVector& Pt : L.Points) { Pt = Xf.InverseTransformPosition(Pt); }
		if (K.Box.IsValid)
		{
			L.Box = FBox(Xf.InverseTransformPosition(K.Box.Min), Xf.InverseTransformPosition(K.Box.Max));
			L.Box = FBox(L.Box.Min.ComponentMin(L.Box.Max), L.Box.Min.ComponentMax(L.Box.Max));
		}
		Plan.KeepOuts.Add(L);
	}
}

void ATDPlanetScatter::RequestKindLoads()
{
	// Every kind's mesh and material, loaded off the game thread while Generate runs (T7, issue #89: the old apply did
	// DoesPackageExist + TryLoad per kind on the game thread, ~330 ms on the first entry).
	TArray<FSoftObjectPath> Paths;
	for (const FTDScatterKind& K : Plan.Kinds)
	{
		for (const FString& S : { K.MeshPath, K.MaterialPath })
		{
			if (S.IsEmpty()) { continue; }
			const FSoftObjectPath P(S);
			if (FPackageName::DoesPackageExist(P.GetLongPackageName())) { Paths.AddUnique(P); }
		}
	}
	if (Paths.Num() == 0) { bKindsLoaded = true; return; }
	TWeakObjectPtr<ATDPlanetScatter> Weak(this);
	KindLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate::CreateLambda([Weak]()
	{
		if (ATDPlanetScatter* Self = Weak.Get()) { Self->bKindsLoaded = true; }
	}));
	if (!KindLoadHandle.IsValid() || KindLoadHandle->HasLoadCompleted()) { bKindsLoaded = true; }
}

bool ATDPlanetScatter::AreKindPathsResolved() const
{
	// Every existing kind mesh / material is in memory (the async request finished, or something else loaded them).
	for (const FTDScatterKind& K : Plan.Kinds)
	{
		for (const FString& S : { K.MeshPath, K.MaterialPath })
		{
			if (S.IsEmpty()) { continue; }
			const FSoftObjectPath P(S);
			if (!P.ResolveObject() && FPackageName::DoesPackageExist(P.GetLongPackageName())) { return false; }
		}
	}
	return true;
}

void ATDPlanetScatter::StartAsync()
{
	if (bBuilt || bBuilding)
	{
		return;
	}
	bBuilding = true;
	RequestKindLoads();
	TWeakObjectPtr<ATDPlanetScatter> Weak(this);
	const ETDDestination P = Planet;
	const FTDScatterPlan PlanCopy = Plan;
	Async(EAsyncExecution::ThreadPool, [Weak, P, PlanCopy]()
	{
		const double Start = FPlatformTime::Seconds();
		const FTDTerrainField Field(FTDTerrainStyle::ForPlanet(P));
		FTDScatterResult R;
		PlanCopy.Generate(Field, R);
		const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
		AsyncTask(ENamedThreads::GameThread, [Weak, R = MoveTemp(R), Ms]() mutable
		{
			if (ATDPlanetScatter* Self = Weak.Get()) { Self->Apply(MoveTemp(R), Ms); }
		});
	});
}

void ATDPlanetScatter::BuildNow()
{
	if (bBuilt)
	{
		return;
	}
	const double Start = FPlatformTime::Seconds();
	const FTDTerrainField Field(FTDTerrainStyle::ForPlanet(Planet));
	FTDScatterResult R;
	Plan.Generate(Field, R);
	bKindsLoaded = true;	// tests and dev: loads happen synchronously here, by design
	Apply(MoveTemp(R), (FPlatformTime::Seconds() - Start) * 1000.0);
	ApplyQueued(0.0);	// tests and dev: everything now
}

UStaticMesh* ATDPlanetScatter::LoadKindMesh(int32 Kind) const
{
	const FSoftObjectPath Path(Plan.Kinds[Kind].MeshPath);
	if (UStaticMesh* Loaded = Cast<UStaticMesh>(Path.ResolveObject())) { return Loaded; }	// the async load's result
	if (!FPackageName::DoesPackageExist(Path.GetLongPackageName())) { return nullptr; }
	++SyncKindLoads;	// BuildNow (tests, dev) or a kind the async request missed
	return Cast<UStaticMesh>(Path.TryLoad());
}

UMaterialInterface* ATDPlanetScatter::ResolveKindMaterial(int32 Kind) const
{
	const FString& S = Plan.Kinds[Kind].MaterialPath;
	if (S.IsEmpty()) { return nullptr; }
	const FSoftObjectPath MP(S);
	if (UMaterialInterface* Loaded = Cast<UMaterialInterface>(MP.ResolveObject())) { return Loaded; }
	if (!FPackageName::DoesPackageExist(MP.GetLongPackageName())) { return nullptr; }
	++SyncKindLoads;
	return Cast<UMaterialInterface>(MP.TryLoad());
}

void ATDPlanetScatter::SetupComponent(UInstancedStaticMeshComponent* H, int32 Kind, bool bCollide) const
{
	const FTDScatterKind& K = Plan.Kinds[Kind];
	H->SetupAttachment(Root);
	// Movable, explicitly: (H)ISMs default to Static, and a Static component cannot attach to the actor's movable root -
	// Unreal aborts the attach and the forest lands around the world origin instead of the course (live, T4).
	H->SetMobility(EComponentMobility::Movable);
	H->SetCullDistances(int32(K.CullStartCm), int32(K.CullEndCm));
	H->SetCastShadow(K.bShadow);
	if (UMaterialInterface* M = ResolveKindMaterial(Kind)) { H->SetMaterial(0, M); }
	if (bCollide)
	{
		// On foot near the course: trunks and boulders block the pilot. Ships and shots fly through.
		H->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		H->SetCollisionResponseToAllChannels(ECR_Ignore);
		H->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		H->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		H->SetCollisionResponseToChannel(ECC_Camera, ECR_Block); // the chase camera stops at trunks and branches (T7: it sat inside trees)
	}
	else
	{
		H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ATDPlanetScatter::Apply(FTDScatterResult&& Result, double Ms)
{
	bBuilding = false;
	if (bBuilt || ApplyJobs.Num() > 0)
	{
		return;
	}
	GenerateMs = Ms;
	// Queue, near tier first, big kinds split: the actor's tick adds a few components per frame (issue #89).
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (int32 k = 0; k < Plan.Kinds.Num(); ++k)
		{
			TArray<FTransform>& Xfs = Pass == 0 ? Result.Near[k] : Result.Far[k];
			for (int32 From = 0; From < Xfs.Num(); From += FMath::Max(1, ApplyChunk))
			{
				FApplyJob J;
				J.Kind = k;
				J.bNear = Pass == 0;
				J.Xfs.Append(Xfs.GetData() + From, FMath::Min(FMath::Max(1, ApplyChunk), Xfs.Num() - From));
				ApplyJobs.Add(MoveTemp(J));
			}
		}
	}
	ApplyJobHead = 0;
	bNearApplied = ApplyJobs.Num() == 0 || !ApplyJobs[0].bNear;
	if (ApplyJobs.Num() == 0) { bBuilt = true; }
}

void ATDPlanetScatter::ApplyQueued(double BudgetMs)
{
	if (!bKindsLoaded && ((KindLoadHandle.IsValid() && KindLoadHandle->HasLoadCompleted()) || AreKindPathsResolved())) { bKindsLoaded = true; }
	if (ApplyJobHead >= ApplyJobs.Num() || !bKindsLoaded)
	{
		return;	// nothing queued, or the kinds are still loading off the game thread
	}
	const double Start = FPlatformTime::Seconds();
	int32 Done = 0;
	while (ApplyJobHead < ApplyJobs.Num())
	{
		if (Done > 0 && BudgetMs > 0.0 && (FPlatformTime::Seconds() - Start) * 1000.0 >= BudgetMs) { break; }
		FApplyJob& J = ApplyJobs[ApplyJobHead++];
		++Done;
		UStaticMesh* Mesh = LoadKindMesh(J.Kind);
		if (Mesh && J.Xfs.Num() > 0)
		{
			UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			H->SetStaticMesh(Mesh);
			SetupComponent(H, J.Kind, J.bNear && Plan.Kinds[J.Kind].bBlocksOnFoot);
			if (!J.bNear) { H->SetCastShadow(false); }	// beyond the course's surroundings nothing casts dynamic shadow (render thread)
			H->RegisterComponent();
			H->AddInstances(J.Xfs, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);
			AddInstanceComponent(H);
			Hisms.Add(H);
			InstanceCount += J.Xfs.Num();
		}
		J.Xfs.Empty();
		if (!bNearApplied && (ApplyJobHead >= ApplyJobs.Num() || !ApplyJobs[ApplyJobHead].bNear))
		{
			bNearApplied = true;
			UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetScatter: near tier in (%d instances)"), InstanceCount);
		}
	}
	const double FrameMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	if (BudgetMs > 0.0) { MaxApplyFrameMs = FMath::Max(MaxApplyFrameMs, FrameMs); }
	ApplyTotalMs += FrameMs;
	if (ApplyJobHead >= ApplyJobs.Num())
	{
		ApplyJobs.Reset();
		ApplyJobHead = 0;
		bBuilt = true;
		UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetScatter: zone layer %d instances in %d HISMs (generate %.0f ms off-thread, apply %.0f ms spread, worst frame %.1f ms)"),
			InstanceCount, Hisms.Num(), GenerateMs, ApplyTotalMs, MaxApplyFrameMs);
	}
}

// ------------------------------------------------------------------------------------------------ rings

bool ATDPlanetScatter::GetViewLocal(FVector& OutLocal) const
{
	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Cam)
	{
		return false;
	}
	OutLocal = GetActorTransform().InverseTransformPosition(Cam->GetCameraLocation());
	return true;
}

void ATDPlanetScatter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ApplyJobs.Num() > 0)
	{
		if (!bKindsLoaded)
		{
			KindWaitSeconds += DeltaSeconds;
			if (KindWaitSeconds >= KindLoadTimeoutSeconds)
			{
				// A kind that never finishes loading must not hold the zone layer (or the arrival's reveal) forever.
				bKindsLoaded = true;
				UE_LOG(LogTriadDrift, Warning, TEXT("ATDPlanetScatter: kind loads not done after %.0f s - applying anyway (missing kinds load on the game thread)"), KindLoadTimeoutSeconds);
			}
		}
		ApplyQueued(ApplyBudgetMs);
	}
	if (!bBuilt)
	{
		return;
	}
	FVector Local;
	const bool bView = GetViewLocal(Local);
	// ENG-38: the arrival's glide end is ready before the camera gets there; hand back once a real camera is within 1 km.
	if (bFocusOverride && bView && FVector2D::Distance(FVector2D(Local), FVector2D(FocusLocal)) < 100000.f) { bFocusOverride = false; }
	if (bFocusOverride) { Local = FocusLocal; }
	else if (!bView) { return; }
	// The near ring (ground cover, culled at ~150 m) only while the camera is low over the ground.
	const ATDPlanetTerrain* Terrain = Cast<ATDPlanetTerrain>(GetOwner());
	const float Over = Terrain ? GetActorTransform().TransformPosition(Local).Z - Terrain->GetHeightAt(GetActorTransform().TransformPosition(Local)) : 0.f;
	// T6's take-off profile (Nov 14): re-centring the mid ring every 200 m cleared and re-added thousands of instances while
	// the ship climbed (render thread 31 ms). Flying (camera over RingFreezeHeightCm), the rings stay where they are; on
	// foot and for the arrival's focus point they follow as before.
	if (bFocusOverride || Over < RingFreezeHeightCm)
	{
		UpdateRing(ETDScatterRing::Mid, Local, false);
	}
	FRingState& NearRing = Rings[int32(ETDScatterRing::Near)];
	const bool bWantNear = Over < Plan.NearMaxHeightCm;
	if (bWantNear != NearRing.bVisible)
	{
		NearRing.bVisible = bWantNear;
		for (int32 k : Plan.RingKinds(ETDScatterRing::Near)) { if (TObjectPtr<UInstancedStaticMeshComponent>* C = RingComps.Find(k)) { (*C)->SetVisibility(bWantNear); } }
	}
	if (bWantNear)
	{
		UpdateRing(ETDScatterRing::Near, Local, false);
	}
}

void ATDPlanetScatter::SetFocusOverride(const FVector& WorldPoint)
{
	bFocusOverride = true;
	FocusLocal = GetActorTransform().InverseTransformPosition(WorldPoint);
}

bool ATDPlanetScatter::IsReadyAround(const FVector& WorldPoint) const
{
	if (!bBuilt) { return false; }
	const FRingState& St = Rings[int32(ETDScatterRing::Mid)];
	if (Plan.RingKinds(ETDScatterRing::Mid).Num() == 0) { return true; }
	const FVector L = GetActorTransform().InverseTransformPosition(WorldPoint);
	const FIntPoint Want(FMath::FloorToInt(L.X / Plan.MidCell), FMath::FloorToInt(L.Y / Plan.MidCell));
	return St.Centre == Want && St.Pending.Num() == 0 && !St.bDirty;
}

void ATDPlanetScatter::BuildRingsNowAt(const FVector& CourseLocal)
{
	UpdateRing(ETDScatterRing::Mid, CourseLocal, true);
	UpdateRing(ETDScatterRing::Near, CourseLocal, true);
}

void ATDPlanetScatter::UpdateRing(ETDScatterRing Ring, const FVector& Local, bool bForceSync)
{
	FRingState& St = Rings[int32(Ring)];
	if (Plan.RingKinds(Ring).Num() == 0)
	{
		return;
	}
	const float Size = Ring == ETDScatterRing::Mid ? Plan.MidCell : Plan.NearCell;
	const float Radius = Ring == ETDScatterRing::Mid ? Plan.MidRadius : Plan.NearRadius;
	const FIntPoint Centre(FMath::FloorToInt(Local.X / Size), FMath::FloorToInt(Local.Y / Size));
	if (Centre == St.Centre && !St.bDirty)
	{
		return;
	}
	// The cells whose square touches the ring's circle, nearest first.
	const int32 N = FMath::CeilToInt(Radius / Size) + 1;
	TArray<FIntPoint> Need;
	for (int32 dy = -N; dy <= N; ++dy)
	{
		for (int32 dx = -N; dx <= N; ++dx)
		{
			const FIntPoint C(Centre.X + dx, Centre.Y + dy);
			const FVector2D Mid((C.X + 0.5f) * Size, (C.Y + 0.5f) * Size);
			if (FVector2D::Distance(Mid, FVector2D(Local)) <= Radius + Size * 0.71f) { Need.Add(C); }
		}
	}
	TArray<FIntPoint> Missing;
	for (const FIntPoint& C : Need) { if (!St.Cells.Contains(C) && !St.Pending.Contains(C)) { Missing.Add(C); } }
	if (Centre != St.Centre)
	{
		St.Centre = Centre;
		St.bDirty = true;
		// Forget cells well outside the ring (they regenerate identically if the player comes back).
		for (auto It = St.Cells.CreateIterator(); It; ++It)
		{
			const FVector2D Mid((It.Key().X + 0.5f) * Size, (It.Key().Y + 0.5f) * Size);
			if (FVector2D::Distance(Mid, FVector2D(Local)) > Radius + Size * 2.5f) { It.RemoveCurrent(); }
		}
	}
	if (!RingField.IsValid())
	{
		RingField = MakeShared<FTDTerrainField, ESPMode::ThreadSafe>(FTDTerrainStyle::ForPlanet(Planet));
	}
	if (Missing.Num() > 0)
	{
		if (bForceSync)
		{
			for (const FIntPoint& C : Missing) { Plan.GenerateCell(*RingField, Ring, C, St.Cells.Add(C)); }
		}
		else
		{
			for (const FIntPoint& C : Missing) { St.Pending.Add(C); }
			TWeakObjectPtr<ATDPlanetScatter> Weak(this);
			const FTDScatterPlan PlanCopy = Plan;
			TSharedPtr<FTDTerrainField, ESPMode::ThreadSafe> Field = RingField;
			Async(EAsyncExecution::ThreadPool, [Weak, PlanCopy, Field, Ring, Missing]()
			{
				TArray<TArray<TArray<FTransform>>> Results;
				Results.SetNum(Missing.Num());
				ParallelFor(Missing.Num(), [&](int32 i) { PlanCopy.GenerateCell(*Field, Ring, Missing[i], Results[i]); });
				AsyncTask(ENamedThreads::GameThread, [Weak, Ring, Missing, Results = MoveTemp(Results)]() mutable
				{
					ATDPlanetScatter* Self = Weak.Get();
					if (!Self) { return; }
					FRingState& S = Self->Rings[int32(Ring)];
					for (int32 i = 0; i < Missing.Num(); ++i)
					{
						S.Pending.Remove(Missing[i]);
						S.Cells.Add(Missing[i], MoveTemp(Results[i]));
					}
					S.bDirty = true;
				});
			});
			return;	// rebuild once the cells are in (keeps the old instances on screen meanwhile)
		}
	}
	if (St.Pending.Num() == 0 && St.bDirty)
	{
		RebuildRing(Ring, Local);
	}
}

void ATDPlanetScatter::RebuildRing(ETDScatterRing Ring, const FVector& Local)
{
	FRingState& St = Rings[int32(Ring)];
	St.bDirty = false;
	const double Start = FPlatformTime::Seconds();
	const float Size = Ring == ETDScatterRing::Mid ? Plan.MidCell : Plan.NearCell;
	const float Radius = Ring == ETDScatterRing::Mid ? Plan.MidRadius : Plan.NearRadius;
	// Cells in the ring, nearest first, kept within the budget: zone layer + the other ring + this ring <= InstanceBudget.
	TArray<TPair<float, const TArray<TArray<FTransform>>*>> Cells;
	for (const TPair<FIntPoint, TArray<TArray<FTransform>>>& It : St.Cells)
	{
		const FVector2D Mid((It.Key.X + 0.5f) * Size, (It.Key.Y + 0.5f) * Size);
		const float D = FVector2D::Distance(Mid, FVector2D(Local));
		if (D <= Radius + Size * 0.71f) { Cells.Add({ D, &It.Value }); }
	}
	Cells.Sort([](const auto& A, const auto& B) { return A.Key < B.Key; });
	const int32 Other = RingInstances[Ring == ETDScatterRing::Mid ? 1 : 0];
	const int32 Allowed = FMath::Max(0, Plan.InstanceBudget - InstanceCount - Other - (Ring == ETDScatterRing::Mid ? 1500 : 0));	// room kept for the near ring
	TMap<int32, TArray<FTransform>> PerKind;
	int32 Count = 0;
	for (const auto& C : Cells)
	{
		int32 CellCount = 0;
		for (const TArray<FTransform>& A : *C.Value) { CellCount += A.Num(); }
		if (Count + CellCount > Allowed) { break; }
		Count += CellCount;
		for (int32 k = 0; k < C.Value->Num(); ++k)
		{
			if ((*C.Value)[k].Num()) { PerKind.FindOrAdd(k).Append((*C.Value)[k]); }
		}
	}
	for (int32 k : Plan.RingKinds(Ring))
	{
		TObjectPtr<UInstancedStaticMeshComponent>* Found = RingComps.Find(k);
		UInstancedStaticMeshComponent* C = Found ? Found->Get() : nullptr;
		const TArray<FTransform>* Xfs = PerKind.Find(k);
		if (!C)
		{
			if (!Xfs || Xfs->Num() == 0) { continue; }
			UStaticMesh* Mesh = LoadKindMesh(k);
			if (!Mesh) { continue; }
			C = NewObject<UInstancedStaticMeshComponent>(this);
			C->SetStaticMesh(Mesh);
			SetupComponent(C, k, false);	// rings never collide: the course's surroundings carry the colliding forest
			if (Ring == ETDScatterRing::Mid)
			{
				C->SetCullDistances(int32(Plan.MidRadius * 0.9f), int32(Plan.MidRadius + 10000.f));
				C->SetCastShadow(false);
			}
			C->RegisterComponent();
			AddInstanceComponent(C);
			RingComps.Add(k, C);
		}
		C->ClearInstances();
		if (Xfs && Xfs->Num()) { C->AddInstances(*Xfs, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false); }
	}
	RingInstances[int32(Ring)] = Count;
	LastRingApplyMs = (FPlatformTime::Seconds() - Start) * 1000.0;
	UE_LOG(LogTriadDrift, Verbose, TEXT("ATDPlanetScatter: %s ring %d instances in %d cells (%.2f ms)"), Ring == ETDScatterRing::Mid ? TEXT("mid") : TEXT("near"), Count, Cells.Num(), LastRingApplyMs);
}
