// Triad Drift - the space field, galaxy layer (docs/eng/WORLD-DENSITY-PLAN.md section 3.3, Ryan Nov 13: "space should be
// filled ... continuously going ... not a massive amount of stuff in one area and the rest empty"). Owner: Teammate 5;
// T4 reviews (World/), T1 advises on flight.
//
// One deterministic layer built at load from the galaxy seed, filling the 9 km flyable sphere with structure:
//   - two tilted asteroid belts around the combat zone (gaps wherever a lane crosses),
//   - clusters in every direction, above and below,
//   - debris fields of old battle wreckage, each around a large drifting wreck (landmarks),
//   - buoy chains along the lanes.
// Hierarchical instancing per mesh and size tier (cull distance by size). Small pieces are visual only; rocks over
// 25 m and the wrecks collide and are registered as avoidance obstacles. Nothing at all within 1.2 km of the combat
// zone centre, nothing hittable within 1.5 km of it, in a lane's core, near a gate, return point or the player start,
// inside 1.3 x a planet's atmosphere, or within 1.2 km of the Dominion. td.SpaceField 0 hides it (and its collision).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDSpaceField.generated.h"

class ATDGalaxyLayout;
class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/** What a placed piece is (tests and the cache spots). */
UENUM()
enum class ETDSpaceFieldKind : uint8
{
	Belt,
	Cluster,
	Debris,
	Wreck,
	Buoy
};

/** One placed piece, in world space (kept for tests, cache spots and the keep-out check). */
USTRUCT()
struct FTDSpaceFieldPiece
{
	GENERATED_BODY()
	UPROPERTY() FVector Location = FVector::ZeroVector;
	/** Bounding diameter (cm). */
	UPROPERTY() float Size = 0.f;
	UPROPERTY() bool bCollides = false;
	UPROPERTY() ETDSpaceFieldKind Kind = ETDSpaceFieldKind::Belt;
};

UCLASS()
class UNREALGAMEJAM_API ATDSpaceField : public AActor
{
	GENERATED_BODY()

public:
	ATDSpaceField();

	/** Build the field for this galaxy (deterministic for the layout's seed). Spawned by the space game mode. */
	static ATDSpaceField* SpawnFor(ATDGalaxyLayout* Layout);
	static ATDSpaceField* Get(const UObject* WorldContext);

	// ---- Tuning (cm) ----
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 Seed = 7;
	/** Two belts: radii from the combat zone centre, radial half width, vertical half thickness, tilt (degrees). */
	UPROPERTY(EditAnywhere, Category = "SpaceField") TArray<float> BeltRadii = { 160000.f, 340000.f };
	UPROPERTY(EditAnywhere, Category = "SpaceField") float BeltHalfWidth = 22000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float BeltHalfThickness = 15000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") TArray<float> BeltTilts = { 12.f, -18.f };
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 BeltRocks = 900;
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 Clusters = 30;
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 RocksPerCluster = 40;
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 DebrisFields = 5;
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 PiecesPerDebrisField = 150;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float BuoySpacing = 50000.f;
	/** Rocks at least this big collide (cm). */
	UPROPERTY(EditAnywhere, Category = "SpaceField") float CollideFromSize = 3500.f;

	// ---- Keep-outs (cm) ----
	UPROPERTY(EditAnywhere, Category = "SpaceField") float CombatClearAll = 120000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float CombatClearHittable = 150000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float LaneCoreRadius = 9000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float LaneHittableRadius = 15000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float PointKeepOut = 20000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float DominionKeepOut = 120000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float AtmosphereKeepOutScale = 1.3f;

	/** May something of Size (cm) stand at this world point (bCollides: the stricter hittable rules)? */
	bool IsPlacementAllowed(const FVector& WorldPoint, float Size, bool bCollides) const;

	/** Every placed piece (world space). */
	const TArray<FTDSpaceFieldPiece>& GetPieces() const { return Pieces; }
	int32 GetInstanceCount() const { return Pieces.Num(); }
	int32 GetObstacleCount() const { return ObstaclesAdded; }

	/**
	 * T2's pickup caches: points inside the debris fields where a cache may sit (deterministic, clear of every
	 * collidable piece by 15 m and of the keep-outs).
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|SpaceField", meta = (WorldContext = "WorldContextObject"))
	static TArray<FVector> GetCacheSpots(const UObject* WorldContextObject);

	/** td.SpaceField 0 / 1: hide or show (and switch the collision off / on). */
	void SetFieldEnabled(bool bOn);
	bool IsFieldEnabled() const { return bEnabled; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void Build();
	void BuildBelts(FRandomStream& Rng);
	void BuildClusters(FRandomStream& Rng);
	void BuildDebrisFields(FRandomStream& Rng);
	void BuildBuoys();

	/** Add one visual instance of a rock family mesh, sized (bounding diameter, cm). Returns false if not allowed. */
	bool AddRock(const FVector& At, float Size, ETDSpaceFieldKind Kind, FRandomStream& Rng, int32 Family);
	bool AddPiece(UStaticMesh* Mesh, const FVector& At, const FRotator& Rot, float Size, ETDSpaceFieldKind Kind, bool bCollides);
	UHierarchicalInstancedStaticMeshComponent* ComponentFor(UStaticMesh* Mesh, float Size);
	float NormFor(UStaticMesh* Mesh) const;

	UPROPERTY(Transient) TObjectPtr<ATDGalaxyLayout> Layout;
	/** Mesh pools by family: 0 classic rocks, 1 ice, 2 metal, 3 crystal, 4 debris (small), 5 wreckage (truss, satellite, cargo). */
	TArray<TArray<TObjectPtr<UStaticMesh>>> Families;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> BuoyMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> SphereMesh;
	/** Per mesh and size tier. */
	UPROPERTY(Transient) TMap<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Components;
	/** The hidden collision spheres (one instanced component; visuals never collide). */
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Collision;
	/** The drifting wrecks (movable, updated each frame on a small closed loop: they never wander into a keep-out). */
	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> WreckComponents;
	TArray<FTransform> WreckHomes;
	TArray<FVector> DebrisFieldCenters;
	TArray<FTDSpaceFieldPiece> Pieces;
	TArray<TArray<FVector>> Lanes; // world space, each a segment [A, B]
	int32 ObstaclesAdded = 0;
	bool bEnabled = true;
	float Clock = 0.f;
};
