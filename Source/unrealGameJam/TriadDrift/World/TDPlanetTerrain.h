// Triad Drift - World (Teammate 4). ENG-34: the planet world around the objective course, built at runtime in chunks.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/World/TDTerrainField.h"
#include "TDPlanetTerrain.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * Chunked terrain (ProceduralMeshComponent sections) from FTDTerrainField, centred on the course (this actor sits at the
 * course's origin, z = 0 = the course floor). Core chunks (1 km) carry collision and refine by distance to the viewer;
 * a far ring of big low-detail chunks closes the horizon. Tagged "TD.Engineering.Terrain"; Nick's TD.Presentation.Sky
 * environment removes it (the placeholder sky owns it).
 * Vertex data for the terrain material: UV0 = world XY / 1000 cm, UV1 = (height m, slope 0..1), vertex colour
 * R height01, G slope, B broad mask, A fine mask.
 */
UCLASS(NotPlaceable)
class UNREALGAMEJAM_API ATDPlanetTerrain : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetTerrain();

	static const FName TerrainTag;	// "TD.Engineering.Terrain"

	/**
	 * M1 (review): the planet world exists once per planet map, independent of any sky. Spawns (once) a terrain that waits
	 * for the course and builds itself at the course origin; a later call returns the same actor. Nick's sky replaces
	 * the placeholder lights, never the world. td.PlanetTerrain 0 = none.
	 */
	static ATDPlanetTerrain* EnsureForWorld(UWorld* World, ETDDestination Planet);

	/** Spawns the terrain for a planet at Origin (the course origin); builds the whole coarse world at once. Material null =
	 *  MI_Terrain_<Planet> when it exists, else MI_Ground_<Planet>. */
	static ATDPlanetTerrain* SpawnForPlanet(UWorld* World, ETDDestination Planet, const FTransform& Origin, UMaterialInterface* Material);

	/** Core chunk size (cm) and quads per chunk per LOD (0 near .. 2 far). */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") float ChunkSize = 100000.f;
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") int32 QuadsLod0 = 64;
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") int32 QuadsLod1 = 24;
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") int32 QuadsLod2 = 8;
	/** LOD switch distances from the viewer (cm). */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") float Lod0Distance = 180000.f;
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") float Lod1Distance = 450000.f;
	/** Rebuild budget per frame (ms). */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") float BudgetMs = 1.5f;

	const FTDTerrainField* GetField() const { return Field.Get(); }
	/** The terrain's material (a MID with this planet's HeightRangeM): T5's course slab uses it so the two read as one ground. */
	UMaterialInterface* GetTerrainMaterial() const { return TerrainMaterial; }

	/**
	 * Half size (cm) of the walkable, collidable core: a square centred on this actor (which sits at the course
	 * origin), in course-local XY. Beyond it only the far mesh draws, with no collision (T5's on-foot turn-back).
	 */
	float GetWalkableHalfExtent() const { return Field.IsValid() ? Field->GetStyle().CoreHalfSize : 0.f; }
	/** Terrain height under a world point (world z). */
	float GetHeightAt(const FVector& WorldPoint) const;
	int32 GetChunkCount() const { return Chunks.Num(); }
	int32 GetChunkLod(FIntPoint Cell) const { for (const FChunk& C : Chunks) { if (C.Cell == Cell) { return C.Lod; } } return -1; }
	int32 GetPendingCount() const { return Pending.Num() + InFlight; }
	/** Finish every pending chunk now (tests, first view). */
	void FlushAll();
	/** Tests: the point the LOD follows (otherwise the camera / pawn). */
	void SetViewOverride(const FVector& WorldPoint) { bViewOverride = true; ViewOverride = WorldPoint; ReplanTimer = 0.f; }
	bool IsBuilt() const { return Field.IsValid(); }
	int32 GetChunkLodAt(const FVector& WorldPoint) const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	struct FChunk
	{
		FIntPoint Cell;
		int32 Section = INDEX_NONE;
		int32 Lod = -1;
		int32 Building = -1;	// LOD being generated on a worker, -1 = idle
	};

	/** One chunk's mesh data, generated off the game thread. */
	struct FChunkData
	{
		int32 Index = INDEX_NONE;
		int32 Lod = 0;
		TArray<FVector> V;
		TArray<int32> Tri;
		TArray<FVector> Nrm;
		TArray<FVector2D> UV0;
		TArray<FVector2D> UV1;
		TArray<FColor> Col;
	};

	void Setup(ETDDestination Planet, UMaterialInterface* Material);
	/** Thread-safe: the field is pure and const. */
	static void GenerateChunk(const FTDTerrainField& InField, FIntPoint Cell, float InChunkSize, int32 Quads, FChunkData& Out);
	void ApplyChunk(FChunkData& Data);
	int32 QuadsFor(int32 Lod) const { return Lod == 0 ? QuadsLod0 : (Lod == 1 ? QuadsLod1 : QuadsLod2); }
	int32 WantedLod(const FChunk& C, const FVector2D& ViewLocal) const;
	FVector2D ViewLocal() const;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Terrain")
	TObjectPtr<USceneComponent> Root;

	/** One component per chunk: each cooks its own small collision (one big trimesh hit a cooking limit and re-cooked the
	 *  whole world on every refresh). Parallel to Chunks. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> ChunkMeshes;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Terrain")
	TObjectPtr<UProceduralMeshComponent> FarMesh;

	/** The region sea: water on Verdance, glowing lava on Cinder (placeholder look; a flat disc at the sea level). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Terrain")
	TObjectPtr<UStaticMeshComponent> Sea;

	/** The volcano's glowing crater floor (Cinder). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Terrain")
	TObjectPtr<UStaticMeshComponent> CraterGlow;

public:
	UStaticMeshComponent* GetSea() const { return Sea; }
	UStaticMeshComponent* GetCraterGlow() const { return CraterGlow; }
protected:

	TSharedPtr<const FTDTerrainField, ESPMode::ThreadSafe> Field;
	TArray<FChunk> Chunks;
	TArray<int32> Pending;
	/** Finished on workers, handed to the game thread (weak: a destroyed terrain drops them), applied in the budget. */
	TArray<TSharedPtr<FChunkData, ESPMode::ThreadSafe>> Ready;
	int32 InFlight = 0;
	/** Max chunks generating at once. */
	int32 MaxInFlight = 6;
	void OnChunkReady(const TSharedPtr<FChunkData, ESPMode::ThreadSafe>& Data);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> TerrainMaterial;
	float ReplanTimer = 0.f;
	bool bViewOverride = false;
	FVector ViewOverride = FVector::ZeroVector;
	/** EnsureForWorld: set up once the course exists (or after a timeout at the spawn point). */
	bool bAwaitingCourse = false;
	ETDDestination PendingPlanet = ETDDestination::Verdance;
	float AwaitSeconds = 0.f;
	/** Apply budget per frame (m1: one LOD0 upload can take ~4 ms; at most one LOD0 per frame and a small budget). */
	int32 MaxLod0PerFrame = 1;
};
