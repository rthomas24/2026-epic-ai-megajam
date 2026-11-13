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

	/** Spawns the terrain for a planet at Origin (the course origin); builds the whole coarse world at once. */
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
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Terrain") float BudgetMs = 3.f;

	const FTDTerrainField* GetField() const { return Field.Get(); }
	/** Terrain height under a world point (world z). */
	float GetHeightAt(const FVector& WorldPoint) const;
	int32 GetChunkCount() const { return Chunks.Num(); }
	int32 GetChunkLod(FIntPoint Cell) const { for (const FChunk& C : Chunks) { if (C.Cell == Cell) { return C.Lod; } } return -1; }
	int32 GetPendingCount() const { return Pending.Num() + InFlight; }
	/** Finish every pending chunk now (tests, first view). */
	void FlushAll();

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
};
