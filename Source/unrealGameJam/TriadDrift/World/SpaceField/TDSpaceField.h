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
	/** Taken over from the galaxy layout (its belt, flank and approach fields, debris and lane rocks), not placed by the field.
	 *  Every piece is solid (a hidden collision sphere); bCollides marks the big ones the stricter keep-outs apply to.
	 *  Note: hiding the field at runtime (SetFieldEnabled false) shows the layout's rocks again, but obstacles of rocks
	 *  already broken stay removed; td.SpaceField 0 at load is the exact fallback. */
	UPROPERTY() bool bAdopted = false;
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
	// ---- The Shepherd Belt (region 2): one thin ring round the zone ----
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float BeltInner = 200000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float BeltOuter = 260000.f;
	/** Standard deviation across the plane (cm): about +-60 m. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float BeltThickness = 3000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float BeltTiltDeg = 8.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") int32 ShepherdRocks = 1800;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") int32 BeltLandmarks = 40;
	/** Half width of the gap where a route crosses (cm); the Verdance gap is wider (T4: the tutorial escape crosses it). */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float BeltGapHalf = 25000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float BeltVerdanceGapHalf = 50000.f;
	// ---- The Watchline halo (region 1) and the Quiet Graveyard (region 4) ----
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 HaloWrecks = 3;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 HaloDebris = 220;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 GraveyardHulls = 5;
	/** Region 7, the Outer Dark: lone rocks with gravel from this distance (cm) to the play bounds. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 OuterDarkRocks = 500;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 IceDriftRocks = 900;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 ScarRocks = 500;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 ScarRubble = 400;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") int32 MoonShards = 14;
	/** The Shattered Moons sit closer than the 3.5 km Dominion sphere, beside the route, with T4's finer keep-outs:
	 *  this far from the Dominion (its 720 m field, the generators, the turrets), clear of the bow tube and the stage start. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") float MoonDominionClear = 180000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") float GraveyardFragments = 220;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Regions") float OuterDarkInner = 600000.f;

	/** Shepherd moonlets at the Verdance and Dominion gap edges (bounding diameter, cm). */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float MoonletMinSize = 25000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Belt") float MoonletMaxSize = 40000.f;

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
	/** The effects builder's NS_TD_Asteroid_Break_S / _M / _L / _Crystal (hard-referenced defaults). */
	UPROPERTY(VisibleAnywhere, Category = "SpaceField|Break") TArray<TObjectPtr<class UNiagaraSystem>> DefaultBreakVFX;

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

	/** Take over the galaxy layout's rocks (same meshes, transforms and collision), so they break too (Lead, Nov 14). */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") bool bAdoptLayoutRocks = true;
	/** Cover in the combat zone: adopted rocks this big (cm) within CoverRadius of the zone take CoverHealthScale x health. */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float CoverMinSize = 2000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float CoverRadius = 100000.f;
	UPROPERTY(EditAnywhere, Category = "SpaceField|Break") float CoverHealthScale = 5.f;

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

	/** Hide or show the field (and its collision); the layout's rocks come back when hidden. Tests and internal use: the
	 *  td.SpaceField console variable is read at level load only. */
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
	void BuildClusters(FRandomStream& Rng, int32 FirstShell = 0);
	// Composed regions (TDSpaceFieldCompose.cpp; docs/eng/SPACE-DESIGN.md).
	bool UseComposedRegions() const;
	void BuildShepherdBelt(FRandomStream& Rng);
	void BuildWatchlineHalo(FRandomStream& Rng);
	void BuildQuietGraveyard(FRandomStream& Rng);
	void BuildOuterDark(FRandomStream& Rng);
	void BuildIceDrift(FRandomStream& Rng);
	void BuildSurveyScar(FRandomStream& Rng);
	void BuildShatteredMoons(FRandomStream& Rng);
	void PlaceRegionCentrepieces(FRandomStream& Rng);
	bool AddStructure(UStaticMesh* Mesh, const FTransform& T, int32 BeaconRegion = 0);
	// Region dressing (TDSpaceFieldDressing.cpp): the effects builder's beacons and haze, soft and null-safe.
	struct FBeaconPoint { FVector Location; int32 Region; };
	TArray<FBeaconPoint> BeaconPoints;
	UPROPERTY(Transient) TArray<TObjectPtr<class UNiagaraComponent>> RegionBeacons;
	UPROPERTY(Transient) TArray<TObjectPtr<class UStaticMeshComponent>> RegionHazes;
	bool bDressingShown = false;
	void BuildRegionDressing();
	static bool IsDressingWanted();
public:
	void SetRegionDressingVisible(bool bOn);
	int32 GetRegionBeaconCount() const { return RegionBeacons.Num(); }
	/** Beacon points placed (headless runs spawn no Niagara components, so tests count these). */
	int32 GetRegionBeaconPointCount() const { return BeaconPoints.Num(); }
	int32 GetRegionHazeCount() const { return RegionHazes.Num(); }
	/** Haze density (the effects builder: start at 0.15 and measure). */
	UPROPERTY(EditAnywhere, Category = "SpaceField|Dressing") float HazeDensity = 0.15f;
private:
	/** T4: the Dominion docking bay's tractor cone and near-field capture (no set piece inside). */
public:
	bool IsInDockingCone(const FVector& WorldPoint, float Radius) const;
private:
	void ScatterDebris(const FVector& Centre, float Radius, int32 Count, float MaxSize, FRandomStream& Rng);
	/** Indestructible set pieces placed (for tests and keep-out checks). */
	TArray<FTDSpaceFieldPiece> Structures;
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
	struct FTumbler { TObjectPtr<UInstancedStaticMeshComponent> Comp; FTransform Home; FVector Axis; float RadPerSec; int32 Instance = INDEX_NONE; };
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
	const TArray<FTDSpaceFieldPiece>& GetStructures() const { return Structures; }
	/** Each built region's centre (world), by its number in docs/eng/SPACE-DESIGN.md (captures, tests). */
	TMap<int32, FVector> RegionCentres;
	/** Dev: each region's nearest approach from the player's routes (TD.SpaceField.RouteReport). */
	void LogRouteReport() const;
	/** The routes the player flies (zone to each planet, the Dominion and each gate end), world space. */
	TArray<TArray<FVector>> GetRouteLanes() const { TArray<TArray<FVector>> Out; for (int32 i = 0; i < BuoyLanes && i < Lanes.Num(); ++i) { Out.Add(Lanes[i]); } return Out; }
	/** The Shepherd Belt's plane normal and centre (world), once built. */
	FVector BeltNormal = FVector::UpVector;
	FVector BeltCenter = FVector::ZeroVector;
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
		int32 Obstacle = INDEX_NONE;	// UTDWorldSubsystem handle (medium and up), removed when it breaks
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
	void BreakPieceInner(int32 Piece, AActor* ByWhom);
	void SpawnFragment(UStaticMesh* Mesh, const FVector& At, float Size);
	UInstancedStaticMeshComponent* FragmentComponentFor(UStaticMesh* Mesh);
	void PreloadBreakCues();
	UPROPERTY(Transient) TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> FragmentComponents;
	UPROPERTY(Transient) TArray<TObjectPtr<UObject>> PreloadedCueAssets;
	TSet<UInstancedStaticMeshComponent*> DirtyVisuals;
	void PlayBreakCue(const FVector& At, float Size, uint8 SizeClass, bool bSpecial);
	uint8 SizeClassFor(float Size) const { return Size < MediumFromSize ? 0 : (Size < LargeFromSize ? 1 : 2); }
	float HealthFor(float Size, bool bWreck) const;
	void GatherNear(const FVector& Min, const FVector& Max, TArray<int32>& Out) const;

	// Adopting the layout's rocks (TDSpaceFieldAdopt.cpp).
	void AdoptLayoutRocks();
	void ShowLayoutRocks(bool bLayoutOwnsThem);
	TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> AdoptedLayoutComps;
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> LayoutCollision;
	/** For the next RegisterPiece: an exact collision radius (> 0), and an obstacle handle to reuse (>= 0) or none (-2). */
	float PendingColliderRadius = -1.f;
	bool bMoonPlacement = false;
	/** Adopt during Build (false when td.SpaceField was 0 at load). */
	bool bAdoptAtLoad = true;
	bool bWarnedToggle = false;
	int32 PendingObstacle = INDEX_NONE;

	// Performance (Lead / Ryan, Nov 14: zero lag). TD.SpaceField.Perf prints them.
	double AnimAccum = 0.0;
	double PerfTickMsSum = 0.0, PerfTickMsMax = 0.0;
	int64 PerfTickFrames = 0;
	double PerfBreakMsSum = 0.0, PerfBreakMsMax = 0.0;
	int64 PerfBreaks = 0;
	double PerfFrameBreakMs = 0.0, PerfFrameBreakMsMax = 0.0;
	int32 PerfBreakDepth = 0;
	int32 PerfAnimated = 0;
public:
	void PerfReport(bool bReset);
	/** Dev: break the Count live pieces nearest Where in one call (one frame); returns the ms it took. */
	double BreakChain(const FVector& Where, int32 Count, AActor* ByWhom);
private:

	int32 ObstaclesAdded = 0;
	bool bEnabled = true;
	float Clock = 0.f;
};
