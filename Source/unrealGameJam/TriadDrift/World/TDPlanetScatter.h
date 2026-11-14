// Triad Drift - World (Teammate 4; scatter v2 by the environment builder, Nov 14). The planet's dressed play zone:
// trees, boulders, columns, crystals, groves and ground cover, from the same field the ground is built from.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TDPlanetScatter.generated.h"

class FTDTerrainField;
struct FTDTerrainSample;
class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;

/** One kind of scattered mesh. */
struct FTDScatterKind
{
	FString MeshPath;			// /Game/... (Gameplay/Props is always cooked)
	FString MaterialPath;		// optional override for slot 0 (e.g. the Cinder rock instance on the shared boulders)
	float MinScale = 0.85f;
	float MaxScale = 1.15f;
	float SinkCm = 120.f;		// pushed into the ground (the terrain mesh is coarser than the field)
	float CullStartCm = 150000.f;
	float CullEndCm = 200000.f;
	float ClearRadiusCm = 300.f;	// its footprint for the course keep-outs
	bool bBlocksOnFoot = false;	// trunks and boulders near the course block the pilot (never ships)
	bool bTumble = false;		// rocks: random tilt; everything else stands up (a slight lean with the slope)
	bool bShadow = true;
	float ZScale = 1.f;			// height relative to the scale (the merged canopy clumps spread wide but stay tree height)
	float ContactRadiusCm = 0.f;	// > 0: seated on the lowest ground within this radius x scale (no crown floats off a slope)
};

/** The scatter's result: per kind, the transforms (course-local), split into "near the course" and the rest. */
struct FTDScatterResult
{
	TArray<TArray<FTransform>> Near;	// within CollisionRadius of the course: these collide on foot
	TArray<TArray<FTransform>> Far;
	int32 Total() const;
};

/** The streamed rings around the camera. */
enum class ETDScatterRing : uint8 { Mid, Near };

/**
 * The plan for a planet: the kinds and the rules that place them. Pure and thread-safe, deterministic from the seed.
 * Three tiers (v2):
 * - the zone layer, built once at load: the far tier (sparse trees and canopy clumps on Verdance; columns, boulders,
 *   crystals and charred trees on Cinder), boulders at cliff feet, giant groves, and the full-density forest within
 *   CollisionRadius of the course (it collides on foot);
 * - the mid ring, streamed in cells around the camera (MidRadius): the dense forest outside the course's surroundings;
 * - the near ring, streamed (NearRadius): ground cover and rubble, no shadows, culled at ~150 m.
 * Nothing on the course's ground box (+ margin) or inside any of the course's keep-outs (routes, sites, the pad, ...),
 * full density to ZoneFullRadius from the pad, falling to 0 at ZoneRadius. The zone layer plus both rings stay under
 * InstanceBudget. Rules use the same masks as the terrain material (broad / fine), so groves and crystals stand on the
 * ground painted for them.
 */
struct UNREALGAMEJAM_API FTDScatterPlan
{
	ETDDestination Planet = ETDDestination::Verdance;
	TArray<FTDScatterKind> Kinds;
	float ZoneFullRadius = 300000.f;	// full density to 3 km (the world-density plan)
	float ZoneRadius = 400000.f;		// tapering to ZoneEdgeDensity at 4 km, nothing beyond
	float ZoneEdgeDensity = 0.25f;
	float FarTreeSpacing = 4000.f;	// zone layer, everywhere (cm): the far tier, ~6/ha on the rainforest floor
	float DenseTreeSpacing = 1450.f;	// the full forest: around the course (zone layer) and in the mid ring
	float CanopySpacing = 7000.f;	// Verdance far-tier canopy clumps (they dither out nearer than ~0.8 km)
	float RockSpacing = 4000.f;
	float GroveSpacing = 1600.f;
	float GroundSpacing = 650.f;		// near ring
	float KeepOutMargin = 3000.f;	// around the course's ground box
	float CollisionRadius = 40000.f;	// instances this close to the ground box collide on foot (zone layer: full forest)
	float MidCell = 20000.f, MidRadius = 80000.f;
	float NearCell = 5000.f, NearRadius = 15000.f;
	float NearMaxHeightCm = 25000.f;	// the near ring only while the camera is this low over the ground
	int32 InstanceBudget = 25000;
	/** Story (environment builder, s6): Verdance's wreck scar, a burned strip through the forest from the research site
	 *  outward (course-local cm), ScarHalfA wide at A widening to ScarHalfB at B; 0 = none. Inside it the forest is gone:
	 *  charred trunks and scorch marks only. */
	FVector2D ScarA = FVector2D::ZeroVector, ScarB = FVector2D::ZeroVector;
	float ScarHalfA = 0.f, ScarHalfB = 0.f;
	int32 ScorchKind = INDEX_NONE;
	/** 0 outside the scar, 1 on its centre line. */
	float ScarWeight(float X, float Y) const;
	/** The course's keep-outs in course-local space (copied on the game thread before any generation). */
	TArray<FTDCourseKeepOut> KeepOuts;

	/** Shared thresholds with M_TD_TerrainR2 (ue_terrain_r2_graph.py: GroveBroadMax, GroveFineMin). */
	static constexpr float GroveBroadMax = 0.3f;
	static constexpr float GroveFineMin = 0.62f;
	static constexpr float ObsidianFineMin = 0.7f;
	// Verdance's broad mask spans ~0.2..0.8 in the play zone (measured: 65 % in 0.2-0.4, 33 % in 0.4-0.6): rainforest is
	// its upper quarter, mixed forest the next band. Same number in M_TD_TerrainR2's JungleBroad.
	static constexpr float RainforestBroad = 0.475f;
	static constexpr float MixedBroad = 0.35f;

	static FTDScatterPlan ForPlanet(ETDDestination Planet);

	/** The zone layer (ParallelFor over rows). */
	void Generate(const FTDTerrainField& Field, FTDScatterResult& Out) const;
	/** One streamed cell (course-local cell index at the ring's cell size): transforms per kind. */
	void GenerateCell(const FTDTerrainField& Field, ETDScatterRing Ring, FIntPoint Cell, TArray<TArray<FTransform>>& Out) const;

	/** True where nothing may be placed (the course's ground box + margin). */
	bool IsKeptOut(const FTDTerrainField& Field, float X, float Y) const;
	/** The course's own keep-outs (routes, sites, the pad, encounters), course-local. */
	bool IsCourseClear(float X, float Y, float Radius) const;
	/** Inside the course's surroundings, where the zone layer carries the full forest (and the mid ring does not). */
	bool IsNearCourse(const FTDTerrainField& Field, float X, float Y) const;
	/** Verdance grove / Cinder obsidian patch at a sample (the material's patch rule). */
	bool IsPatch(const FTDTerrainSample& Sm) const;

	/** Dev / tests: the kinds of a ring (indices into Kinds). */
	TArray<int32> RingKinds(ETDScatterRing Ring) const;

private:
	void RunLayer(const FTDTerrainField& Field, float Spacing, int32 Salt, const FBox2D& Rect, int32 Mode, bool bParallel,
		TArray<TArray<FTransform>>& OutNear, TArray<TArray<FTransform>>& OutFar) const;
	int32 Pick(int32 Mode, const FTDTerrainSample& Sm, float HeightM, float Rnd, float Stand) const;
	float ZoneWeight(float X, float Y) const;
};

/**
 * Spawned by the planet terrain. Generates the zone layer off the game thread, then fills one HISM per kind (two when
 * some instances collide on foot); streams the mid and near rings around the camera into pooled instanced components.
 * Every component is Movable (a Static one cannot attach to the moving root: the forest landed at the world origin).
 * `td.Scatter 0` = no scatter (the frame-time A/B switch).
 */
UCLASS()
class UNREALGAMEJAM_API ATDPlanetScatter : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetScatter();

	static ATDPlanetScatter* SpawnFor(class ATDPlanetTerrain* Terrain, ETDDestination Planet);

	/** Tests / dev: generate and apply the zone layer now, on the game thread. */
	void BuildNow();
	/** Tests / dev: fill both rings around a course-local point now, on the game thread. */
	void BuildRingsNowAt(const FVector& CourseLocal);

	bool IsBuilt() const { return bBuilt; }
	int32 GetInstanceCount() const { return InstanceCount; }
	int32 GetRingInstanceCount() const { return RingInstances[0] + RingInstances[1]; }
	int32 GetTotalInstanceCount() const { return InstanceCount + GetRingInstanceCount(); }
	double GetGenerateMs() const { return GenerateMs; }
	double GetLastRingApplyMs() const { return LastRingApplyMs; }
	const FTDScatterPlan& GetPlan() const { return Plan; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	void StartAsync();
	void Apply(FTDScatterResult&& Result, double Ms);
	void CaptureKeepOuts();
	UStaticMesh* LoadKindMesh(int32 Kind) const;
	void SetupComponent(UInstancedStaticMeshComponent* C, int32 Kind, bool bCollide) const;

	// --- rings ---
	struct FRingState
	{
		FIntPoint Centre = FIntPoint(MAX_int32, MAX_int32);
		TMap<FIntPoint, TArray<TArray<FTransform>>> Cells;
		TSet<FIntPoint> Pending;
		bool bDirty = false;
		bool bVisible = true;
	};
	void UpdateRing(ETDScatterRing Ring, const FVector& Local, bool bForceSync);
	void RebuildRing(ETDScatterRing Ring, const FVector& Local);
	bool GetViewLocal(FVector& OutLocal) const;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Scatter")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Hisms;

	/** Pooled ring components, one per ring kind (index = kind). */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UInstancedStaticMeshComponent>> RingComps;

	ETDDestination Planet = ETDDestination::Verdance;
	FTDScatterPlan Plan;
	FRingState Rings[2];
	TSharedPtr<FTDTerrainField, ESPMode::ThreadSafe> RingField;
	bool bBuilt = false;
	bool bBuilding = false;
	int32 InstanceCount = 0;
	int32 RingInstances[2] = { 0, 0 };
	double GenerateMs = 0.0;
	double LastRingApplyMs = 0.0;
};
