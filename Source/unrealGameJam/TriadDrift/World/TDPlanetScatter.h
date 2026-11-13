// Triad Drift - World (Teammate 4). World density v1: the planet's dressed play zone - trees and boulders across the
// terrain, from the same field the ground is built from.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDPlanetScatter.generated.h"

class FTDTerrainField;
class UHierarchicalInstancedStaticMeshComponent;

/** One kind of scattered mesh. */
struct FTDScatterKind
{
	FString MeshPath;			// /Game/... (Gameplay/Props is always cooked)
	float MinScale = 0.85f;
	float MaxScale = 1.15f;
	float SinkCm = 120.f;		// pushed into the ground (the terrain mesh is coarser than the field)
	float CullStartCm = 150000.f;
	float CullEndCm = 200000.f;
	bool bBlocksOnFoot = false;	// trunks and boulders near the course block the pilot (never ships)
};

/** The scatter's result: per kind, the transforms (course-local), split into "near the course" and the rest. */
struct FTDScatterResult
{
	TArray<TArray<FTransform>> Near;	// within CollisionRadius of the course: these collide on foot
	TArray<TArray<FTransform>> Far;
	int32 Total() const;
};

/**
 * The plan for a planet: the kinds and the rules that place them. Pure and thread-safe, deterministic from the seed.
 * Verdance v1: rainforest / mixed / open by the biome mask, trees thinning with height and slope, none in the sea or
 * on the peaks; boulders on slopes; nothing inside the course's ground box (+ margin); full density to ZoneFullRadius
 * from the pad, falling to 0 at ZoneRadius.
 */
struct UNREALGAMEJAM_API FTDScatterPlan
{
	TArray<FTDScatterKind> Kinds;
	float ZoneFullRadius = 300000.f;	// full density to 3 km (the world-density plan)
	float ZoneRadius = 400000.f;		// tapering to ZoneEdgeDensity at 4 km, nothing beyond (the far ring is v2)
	float ZoneEdgeDensity = 0.25f;
	float TreeSpacing = 2400.f;		// jittered grid (cm): ~22k instances, inside the plan's 25k budget
	float RockSpacing = 4000.f;
	float KeepOutMargin = 3000.f;	// around the course's ground box
	float CollisionRadius = 40000.f;	// instances this close to the ground box collide on foot
	int32 FirstRock = 0;			// Kinds[FirstRock..] are boulders

	static FTDScatterPlan ForPlanet(ETDDestination Planet);

	/** Places every instance (ParallelFor over rows). */
	void Generate(const FTDTerrainField& Field, FTDScatterResult& Out) const;

	/** True where nothing may be placed (the course's ground box + margin). */
	bool IsKeptOut(const FTDTerrainField& Field, float X, float Y) const;
};

/**
 * Spawned by the planet terrain. Generates the plan off the game thread, then fills one HISM per kind (two when some
 * instances collide on foot), with per-kind cull distances. `td.Scatter 0` = no scatter.
 */
UCLASS()
class UNREALGAMEJAM_API ATDPlanetScatter : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetScatter();

	static ATDPlanetScatter* SpawnFor(class ATDPlanetTerrain* Terrain, ETDDestination Planet);

	/** Tests / dev: generate and apply now, on the game thread. */
	void BuildNow();

	bool IsBuilt() const { return bBuilt; }
	int32 GetInstanceCount() const { return InstanceCount; }
	double GetGenerateMs() const { return GenerateMs; }

protected:
	void StartAsync();
	void Apply(FTDScatterResult&& Result, double Ms);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Scatter")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Hisms;

	ETDDestination Planet = ETDDestination::Verdance;
	FTDScatterPlan Plan;
	bool bBuilt = false;
	bool bBuilding = false;
	int32 InstanceCount = 0;
	double GenerateMs = 0.0;
};
