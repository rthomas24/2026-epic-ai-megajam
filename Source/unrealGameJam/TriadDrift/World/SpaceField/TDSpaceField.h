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
#include "TriadDrift/Combat/TDFieldDamageable.h"
#include "TriadDrift/Flight/TDShipImpactReceiver.h"
#include "TDSpaceField.generated.h"

class ATDGalaxyLayout;
class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UPrimitiveComponent;
class UTDSpaceFieldFX;

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
class UNREALGAMEJAM_API ATDSpaceField : public AActor, public ITDFieldDamageable, public ITDShipImpactReceiver
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
	/** Clusters per shell; three shells (ClusterShells: inner and outer distance pairs, cm) so every bearing has one near and one far. */
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 Clusters = 30;
	UPROPERTY(EditAnywhere, Category = "SpaceField") TArray<FVector2D> ClusterShells = { FVector2D(220000.f, 420000.f), FVector2D(420000.f, 620000.f), FVector2D(620000.f, 820000.f) };
	/** Big rocks per cluster (60-150 m, they collide) so a cluster reads from 2 km away, not only as specks. */
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 LandmarksPerCluster = 3;
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 RocksPerCluster = 32;
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
	/** The Dominion, its distortion field (720 m), generators, trench, docking cone and escape path (T4: 3.5 km). */
	UPROPERTY(EditAnywhere, Category = "SpaceField") float DominionKeepOut = 350000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float AtmosphereKeepOutScale = 1.3f;
	/** The Verdance convoys (Dominion -> Verdance, offsets up to 900 m): nothing hittable in this tube. */
	UPROPERTY(EditAnywhere, Category = "SpaceField") float ConvoyKeepOut = 120000.f;
	/** Return points (orbit take-off, the transponder 300 m beside Verdance's): nothing hittable within this. */
	UPROPERTY(EditAnywhere, Category = "SpaceField") float ReturnPointKeepOut = 50000.f;

	/** Slowly tumbling big rocks (rotation only: their collision sphere never moves). */
	UPROPERTY(EditAnywhere, Category = "SpaceField") int32 MaxTumblers = 60;
	UPROPERTY(EditAnywhere, Category = "SpaceField") float TumbleFromSize = 3000.f;

	// ---- Destructible (Ryan, Nov 13: shoot it and it breaks, ram it, hit it and it hurts, never fly through it) ----
	/** Size classes by bounding diameter (cm): small below MediumFromSize, large from LargeFromSize. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float MediumFromSize = 1000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float LargeFromSize = 4000.f;
	/** Health: small (1-2 cannon shots), medium (several), large (base + per metre of diameter), wrecks. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float SmallHealth = 12.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float MediumHealth = 60.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float LargeHealthBase = 120.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float LargeHealthPerMetre = 3.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float WreckHealth = 900.f;
	/** A broken medium / large splits into this many pieces of the next class down. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") int32 SplitMin = 2;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") int32 SplitMax = 3;
	/** About one asteroid in this many (belts and clusters, medium and up) is special: crystal-veined, drops a reward. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") int32 SpecialOneIn = 15;
	/** Damage to the rock from a ship hitting it: Scale x (normal speed in m/s)^2 x head-on. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float ShipImpactDamageScale = 0.02f;
	/** Break cues (effects builder): /Game/TriadDrift/Presentation/DA_SpaceField_FX. Empty = engineering placeholder burst. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") TObjectPtr<UTDSpaceFieldFX> FX;

	/** The piece behind a hit (the hidden collision component and the hit's Item), or INDEX_NONE. */
	int32 PieceForHit(const UPrimitiveComponent* Component, int32 Item) const;
	/** Damage one piece; breaks it at zero (split, cue, bus event). Returns true if it broke. */
	bool DamagePiece(int32 Piece, float Damage, AActor* ByWhom);
	/** Lancer's ram dash segment: breaks small and medium pieces along it; a large piece stops it (true, OutBlock). */
	bool RamSweep(const FVector& From, const FVector& To, float Radius, AActor* ByWhom, FVector& OutBlock);
	/** An explosion: every piece inside Radius takes Damage with a linear falloff (at least a quarter). */
	void Splash(const FVector& Center, float Radius, float Damage, AActor* ByWhom);
	/** A ship hit a piece at NormalSpeed (cm/s). Returns true if the piece broke (its collision is gone already). */
	bool ShipImpact(const UPrimitiveComponent* Component, int32 Item, float NormalSpeed, float HeadOn01, AActor* Ship, bool bRamming);

	bool IsPieceAlive(int32 Piece) const { return States.IsValidIndex(Piece) && States[Piece].bAlive; }
	float GetPieceHealth(int32 Piece) const { return States.IsValidIndex(Piece) ? States[Piece].Health : 0.f; }
	uint8 GetPieceSizeClass(int32 Piece) const { return States.IsValidIndex(Piece) ? States[Piece].SizeClass : 0; }
	bool IsPieceSpecial(int32 Piece) const { return States.IsValidIndex(Piece) && States[Piece].bSpecial; }
	int32 GetBrokenCount() const { return Broken; }
	int32 GetAliveCount() const;
	const UInstancedStaticMeshComponent* GetCollision() const { return Collision; }
	/** The world transform of a piece's hidden collision sphere (tests). */
	bool GetColliderTransform(int32 Piece, FTransform& Out) const;

	/** Pickup caches: spawned when the flown ship comes this close, once per run each (collected = gone). */
	UPROPERTY(EditAnywhere, Category = "SpaceField") float CacheSpawnDistance = 80000.f;

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

	// ITDFieldDamageable (T2): shots, Lancer's ram and explosions reach the right piece.
	virtual void ReceiveProjectileHit(const FHitResult& Hit, float Damage, AActor* InInstigator, ETDDamageKind Kind) override;
	virtual bool ReceiveRamSweep(const FVector& From, const FVector& To, float Radius, AActor* InInstigator, FVector& OutBlock) override;
	virtual void ReceiveSplash(const FVector& Center, float Radius, float Damage, AActor* InInstigator) override;

	// ITDShipImpactReceiver (T1): a ship hit a piece. True = it broke and the ship flies on.
	virtual bool ReceiveShipImpact(UPrimitiveComponent* HitComponent, int32 Item, const FVector& ImpactPoint, float NormalSpeed, float HeadOn01, ATDShip* Ship) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Build();
	void BuildBelts(FRandomStream& Rng);
	void BuildClusters(FRandomStream& Rng);
	void BuildDebrisFields(FRandomStream& Rng);
	void BuildBuoys();

	/** Add one visual instance of a rock family mesh, sized (bounding diameter, cm). Returns false if not allowed. */
	bool AddRock(const FVector& At, float Size, ETDSpaceFieldKind Kind, FRandomStream& Rng, int32 Family);
	bool AddPiece(UStaticMesh* Mesh, const FVector& At, const FRotator& Rot, float Size, ETDSpaceFieldKind Kind, bool bCollides, bool bTumble = false, bool bSpecial = false);
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
	/** The first BuoyLanes lanes carry buoys (zone to planets, Dominion, gate ends); the rest are keep-outs only. */
	int32 BuoyLanes = 0;
	TArray<TArray<FVector>> ConvoyLanes;

	// Tumblers: movable instanced components (one per mesh), every instance rotates about its own axis.
	UPROPERTY(Transient) TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> TumbleComponents;
	struct FTumbler { TObjectPtr<UInstancedStaticMeshComponent> Comp; FTransform Home; FVector Axis; float RadPerSec; };
	TArray<FTumbler> Tumblers;
	FRandomStream TumbleRng;
	TArray<FTransform> TumbleScratch;

	// Caches.
	struct FCache { FVector Spot; uint8 Type = 0; bool bCollected = false; TWeakObjectPtr<AActor> Pickup; FVector LastPickupLocation = FVector::ZeroVector; };
	TArray<FCache> Caches;
	float CacheTimer = 0.f;
	FVector LastShipLocation = FVector::ZeroVector;
	void UpdateCaches();
	void UpdateTumblers();

public:
	int32 GetTumblerCount() const { return Tumblers.Num(); }
	TArray<UInstancedStaticMeshComponent*> GetTumbleComponents() const
	{
		TArray<UInstancedStaticMeshComponent*> Out;
		for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& T : TumbleComponents) { if (T.Value) { Out.Add(T.Value); } }
		return Out;
	}
	int32 GetCachesCollected() const;
	int32 GetCachesAlive() const;
	/** Tests / dev: run the cache logic now as if the flown ship were at ShipLocation. */
	void UpdateCachesAt(const FVector& ShipLocation);

private:
	// Per piece (parallel to Pieces).
	struct FPieceState
	{
		UInstancedStaticMeshComponent* Visual = nullptr;	// owned by Components / TumbleComponents / WreckComponents
		int32 Instance = INDEX_NONE;
		int32 Collider = INDEX_NONE;
		int32 Tumbler = INDEX_NONE;
		int32 Wreck = INDEX_NONE;
		UStaticMesh* Mesh = nullptr;
		float Health = 1.f;
		uint8 SizeClass = 0;
		bool bSpecial = false;
		bool bAlive = true;
	};
	TArray<FPieceState> States;
	TArray<int32> ColliderToPiece;
	/** Recycled slots: dead collision instances and dead visual instances per component. */
	TArray<int32> FreeColliders;
	TMap<UInstancedStaticMeshComponent*, TArray<int32>> FreeVisuals;
	/** A coarse grid of piece indices (ram and splash queries). */
	TMap<FIntVector, TArray<int32>> Grid;
	static FIntVector CellOf(const FVector& P) { return FIntVector(FMath::FloorToInt(P.X / 20000.f), FMath::FloorToInt(P.Y / 20000.f), FMath::FloorToInt(P.Z / 20000.f)); }
	FRandomStream SpecialRng;
	FRandomStream BreakRng;
	int32 Broken = 0;
	bool bFXLoaded = false;

	/** Register the piece just added to Pieces (collider, state, grid). */
	int32 RegisterPiece(UInstancedStaticMeshComponent* Visual, int32 Instance, UStaticMesh* Mesh, float Size, bool bSpecial, int32 Tumbler, int32 Wreck);
	int32 AddCollider(const FVector& At, float Size);
	void BreakPiece(int32 Piece, AActor* ByWhom);
	void SpawnFragment(UStaticMesh* Mesh, const FVector& At, float Size);
	void PlayBreakCue(const FVector& At, float Size, uint8 SizeClass, bool bSpecial);
	uint8 SizeClassFor(float Size) const { return Size < MediumFromSize ? 0 : (Size < LargeFromSize ? 1 : 2); }
	float HealthFor(float Size, bool bWreck) const;
	void GatherNear(const FVector& Min, const FVector& Max, TArray<int32>& Out) const;

	int32 ObstaclesAdded = 0;
	bool bEnabled = true;
	float Clock = 0.f;
};
