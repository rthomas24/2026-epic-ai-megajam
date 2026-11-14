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
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

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
		P.ScarA = FVector2D(13000.f, 8200.f);
		P.ScarB = FVector2D(57000.f, 41000.f);
		P.ScarHalfA = 1500.f;
		P.ScarHalfB = 6000.f;
		for (int32 i = 1; i <= 2; ++i)
		{
			R.CharTrees.Add(Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Cind_CharTree_%02d"), i)), 1.0f, 1.6f, 30.f, 180000.f, 240000.f, 300.f, true, false, true));
		}
		P.ScorchKind = Add(TDScatter_Path(TEXT("Kit/Story"), TEXT("SM_Kit_ScorchLines")), 4.f, 6.5f, 0.f, 180000.f, 240000.f, 1200.f, false, false, false);
		for (int32 i = 1; i <= 3; ++i)
		{
			R.Rocks.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_Rock_%02d"), i)), 1.5f, 4.5f, 60.f, 100000.f, 140000.f, 500.f, true, true, true));
		}
		for (int32 i = 1; i <= 4; ++i)
		{
			R.CliffRocks.Add(Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Any_Boulder_%02d"), i)), 0.8f, 1.6f, 80.f, 140000.f, 180000.f, 900.f, true, true, true));
		}
		// Glowing groves: giant mushrooms (3-8x) and big glow plants where the ground is painted as a grove.
		for (int32 i = 1; i <= 3; ++i)
		{
			R.Grove.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_Mushroom_%02d"), i)), i == 3 ? 3.f : 4.f, i == 3 ? 6.f : 8.f, 40.f, 120000.f, 160000.f, 1200.f, true, false, true));
		}
		for (int32 i = 1; i <= 2; ++i)
		{
			R.GroveGlow.Add(Add(TDScatter_Path(TEXT("Verdance"), FString::Printf(TEXT("SM_Verd_GlowPlant_%02d"), i)), 2.f, 3.5f, 10.f, 50000.f, 70000.f, 400.f, false, false, false));
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
			R.Columns.Add(Add(TDScatter_Path(TEXT("Cinder"), FString::Printf(TEXT("SM_Cind_BasaltColumn_%02d"), i)), 3.f, 7.f, 80.f, 260000.f, 320000.f, 900.f, true, false, true, ColumnMat));
		}
		for (int32 i = 1; i <= 2; ++i)
		{
			R.CharTrees.Add(Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Cind_CharTree_%02d"), i)), 0.9f, 1.6f, 30.f, 100000.f, 140000.f, 300.f, true, false, true));
		}
		// Crystal spires: the big landmarks on the obsidian patches, read from the air.
		R.Spires.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_04")), 4.f, 6.5f, 80.f, 260000.f, 320000.f, 1500.f, true, false, true));
		R.Spires.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_03")), 4.f, 7.f, 60.f, 220000.f, 280000.f, 1200.f, true, false, true));
		for (int32 i = 1; i <= 4; ++i)
		{
			R.CliffRocks.Add(Add(TDScatter_Path(TEXT("Scatter"), FString::Printf(TEXT("SM_Scatter_Any_Boulder_%02d"), i)), 0.8f, 1.8f, 80.f, 140000.f, 180000.f, 900.f, true, true, true, RockMat));
		}
		R.Rocks.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_03")), 1.5f, 3.5f, 30.f, 100000.f, 140000.f, 600.f, true, true, true));
		R.Rocks.Add(Add(TDScatter_Path(TEXT("Planet"), TEXT("SM_Cind_ChokeRock_A")), 0.8f, 1.3f, 60.f, 180000.f, 240000.f, 1000.f, true, false, true));
		R.Rocks.Add(Add(TDScatter_Path(TEXT("Planet"), TEXT("SM_Cind_ChokeRock_B")), 0.8f, 1.3f, 60.f, 180000.f, 240000.f, 1000.f, true, false, true));
		for (int32 i = 1; i <= 4; ++i)
		{
			R.Crystals.Add(Add(TDScatter_Path(TEXT("Cinder"), FString::Printf(TEXT("SM_Cind_Crystal_%02d"), i)), 1.f, 2.5f, 20.f, 90000.f, 130000.f, 400.f, i >= 3, false, true));
		}
		R.Rubble.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_01")), 0.6f, 1.6f, 10.f, 11000.f, 15000.f, 100.f, false, true, false));
		R.Rubble.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Basalt_02")), 0.5f, 1.2f, 15.f, 11000.f, 15000.f, 150.f, false, true, false));
		R.Rubble.Add(Add(TDScatter_Path(TEXT("Planet"), TEXT("SM_Cind_DebrisChunk")), 0.6f, 1.2f, 30.f, 11000.f, 15000.f, 120.f, false, true, false));
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_01")), 0.5f, 1.2f, 10.f, 11000.f, 15000.f, 100.f, false, false, false));
		R.GroundGlow.Add(Add(TDScatter_Path(TEXT("Cinder"), TEXT("SM_Cind_Crystal_02")), 0.4f, 0.9f, 10.f, 11000.f, 15000.f, 100.f, false, false, false));
	}
	{
		FScopeLock Lock(&GTDRangesLock);
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
		return (bPatch && Sm.Slope < 0.45f && Rnd < 0.55f) ? TDScatter_OneOf(R.Crystals, Rnd / 0.55f) : INDEX_NONE;
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
				// a canopy clump is kept out by its whole footprint (its centre can sit 90 m from the scar and still cover it)
				const float Scar = ScarWeight(X, Y, Mode == ModeCanopy ? 9000.f : 0.f);
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
						FVector(X, Y, Field.Height(X, Y) - K.SinkCm * Sc), FVector(Sc));
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

void ATDPlanetScatter::StartAsync()
{
	if (bBuilt || bBuilding)
	{
		return;
	}
	bBuilding = true;
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
	Apply(MoveTemp(R), (FPlatformTime::Seconds() - Start) * 1000.0);
}

UStaticMesh* ATDPlanetScatter::LoadKindMesh(int32 Kind) const
{
	const FSoftObjectPath Path(Plan.Kinds[Kind].MeshPath);
	return FPackageName::DoesPackageExist(Path.GetLongPackageName()) ? Cast<UStaticMesh>(Path.TryLoad()) : nullptr;
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
	if (!K.MaterialPath.IsEmpty())
	{
		const FSoftObjectPath MP(K.MaterialPath);
		if (FPackageName::DoesPackageExist(MP.GetLongPackageName()))
		{
			if (UMaterialInterface* M = Cast<UMaterialInterface>(MP.TryLoad())) { H->SetMaterial(0, M); }
		}
	}
	if (bCollide)
	{
		// On foot near the course: trunks and boulders block the pilot. Ships and shots fly through.
		H->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		H->SetCollisionResponseToAllChannels(ECR_Ignore);
		H->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		H->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	else
	{
		H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ATDPlanetScatter::Apply(FTDScatterResult&& Result, double Ms)
{
	bBuilding = false;
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;
	GenerateMs = Ms;
	const double ApplyStart = FPlatformTime::Seconds();
	for (int32 k = 0; k < Plan.Kinds.Num(); ++k)
	{
		UStaticMesh* Mesh = LoadKindMesh(k);
		if (!Mesh)
		{
			continue;
		}
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			const TArray<FTransform>& Xfs = Pass == 0 ? Result.Near[k] : Result.Far[k];
			if (Xfs.Num() == 0) { continue; }
			UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			H->SetStaticMesh(Mesh);
			SetupComponent(H, k, Pass == 0 && Plan.Kinds[k].bBlocksOnFoot);
			if (Pass == 1) { H->SetCastShadow(false); }	// beyond the course's surroundings nothing casts dynamic shadow (render thread)
			H->RegisterComponent();
			H->AddInstances(Xfs, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);
			AddInstanceComponent(H);
			Hisms.Add(H);
			InstanceCount += Xfs.Num();
			UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetScatter: %s x%d (%s, cull %d-%d, mat %s)"), *Mesh->GetName(), Xfs.Num(), Pass == 0 ? TEXT("near") : TEXT("far"),
				H->InstanceStartCullDistance, H->InstanceEndCullDistance, H->GetMaterial(0) ? *H->GetMaterial(0)->GetName() : TEXT("none"));
		}
	}
	UE_LOG(LogTriadDrift, Log, TEXT("ATDPlanetScatter: zone layer %d instances in %d HISMs (generate %.0f ms off-thread, apply %.0f ms)"),
		InstanceCount, Hisms.Num(), GenerateMs, (FPlatformTime::Seconds() - ApplyStart) * 1000.0);
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
	if (!bBuilt)
	{
		return;
	}
	FVector Local;
	if (!GetViewLocal(Local))
	{
		return;
	}
	UpdateRing(ETDScatterRing::Mid, Local, false);
	// The near ring (ground cover, culled at ~150 m) only while the camera is low over the ground.
	const ATDPlanetTerrain* Terrain = Cast<ATDPlanetTerrain>(GetOwner());
	const float Over = Terrain ? GetActorTransform().TransformPosition(Local).Z - Terrain->GetHeightAt(GetActorTransform().TransformPosition(Local)) : 0.f;
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
