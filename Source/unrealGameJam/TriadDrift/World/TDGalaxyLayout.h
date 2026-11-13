// Triad Drift - World (Teammate 4). ENG-17 galaxy gameplay layout, data-driven and spawned at runtime.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TDGalaxyLayout.generated.h"

class UInstancedStaticMeshComponent;
class UTDTravelFX;
class UStaticMesh;
class ATDPortalGate;
class ATDPlanetAtmosphere;
class ATDDominion;

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDAsteroidFieldSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FVector Center = FVector::ZeroVector;
	/** Asteroids are scattered inside this radius (cm), flattened by Flatten on Z. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float Radius = 30000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") int32 Count = 40;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float MinSize = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float MaxSize = 3500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float Flatten = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") int32 Seed = 1;
	/** Keep this radius around the centre clear (a dogfighting pocket). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float ClearRadius = 0.f;
	/** Rock families (space dressing kit), as shares of the field; the rest are the classic rocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0", ClampMax = "1")) float IceShare = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0", ClampMax = "1")) float MetalShare = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0", ClampMax = "1")) float CrystalShare = 0.f;
	/** Crystal rocks are teal (Verdance side) instead of violet (Cinder side). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") bool bTealCrystal = false;
};

/** A box of station ruin used as cover. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDRuinSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FVector Location = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FRotator Rotation = FRotator::ZeroRotator;
	/** Half extents (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FVector Extent = FVector(3000.f, 600.f, 1500.f);
	/** Module (true) or girder/truss (false): which art slot dresses it (RuinModuleMesh vs RuinMesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") bool bModule = false;
};

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDPlanetSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") ETDDestination Destination = ETDDestination::Verdance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FVector Location = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float PlanetRadius = 60000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float AtmosphereRadius = 72000.f;
	/** Return point used when coming back from the planet (also registered as a named return point). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FName ReturnPoint = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FLinearColor Color = FLinearColor(0.1f, 0.9f, 0.5f);
	/** The planet's look (surface / atmosphere / ring); defaults point at the engineering planet props. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FTDPlanetLook Look;
};

USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDGatePairSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FName IdA = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FTransform A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FName IdB = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FTransform B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float Radius = 2500.f;
	/** Phases in which the pair is open. Empty = always open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") TArray<ETDGamePhase> OpenPhases;
};

/** Everything the galaxy is made of, in world units (cm). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDGalaxyLayoutData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FVector CombatZoneCenter = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float CombatZoneRadius = 45000.f;

	/** Soft play-space boundary handed to flight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FVector BoundaryCenter = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") float BoundaryRadius = 900000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FTransform PlayerStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") TArray<FTDAsteroidFieldSpec> AsteroidFields;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") TArray<FTDRuinSpec> Ruins;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") TArray<FTDPlanetSpec> Planets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") TArray<FTDGatePairSpec> Gates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") FTransform Dominion;

	/** Named return points (Verdance_Orbit, Cinder_Orbit, Dominion_Hangar_Exit, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout") TMap<FName, FTransform> ReturnPoints;
};

namespace TDGalaxyWells
{
	inline const FName Dominion(TEXT("Dominion_Approach"));
}

namespace TDReturnPoints
{
	inline const FName VerdanceOrbit(TEXT("Verdance_Orbit"));
	inline const FName CinderOrbit(TEXT("Cinder_Orbit"));
	inline const FName DominionHangarExit(TEXT("Dominion_Hangar_Exit"));
}

/**
 * Builds the galaxy from FTDGalaxyLayoutData at BeginPlay: asteroid fields (instanced, with collision),
 * station-ruin cover, planets with atmosphere triggers, portal gate pairs, the Dominion, the
 * play-space bounds and the obstacle registry for AI avoidance. ATDSpaceGameMode spawns one (or the
 * L_Galaxy map places one); both run the same code. Layout data, return points and the player start
 * are valid right after spawn, before BeginPlay.
 */
UCLASS(Blueprintable, Config = Game)
class UNREALGAMEJAM_API ATDGalaxyLayout : public AActor
{
	GENERATED_BODY()

public:
	ATDGalaxyLayout();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Galaxy", meta = (WorldContext = "WorldContext"))
	static ATDGalaxyLayout* Get(const UObject* WorldContext);
	/** Re-applies each gate pair's open phases now (also every second); planet orbit gates close while a wave runs. */
	void UpdateGatePhases();

	/** The one call for the game mode: the existing layout, or a new one with the default galaxy. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Galaxy", meta = (WorldContext = "WorldContext"))
	static ATDGalaxyLayout* SpawnDefaultGalaxy(const UObject* WorldContext);

	/** The ENG-17 galaxy. */
	static FTDGalaxyLayoutData MakeDefaultLayout();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Galaxy")
	bool FindReturnPoint(FName Name, FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Galaxy")
	FTransform GetPlayerStart() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Galaxy")
	ATDDominion* GetDominion() const { return Dominion; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Galaxy")
	TArray<ATDPlanetAtmosphere*> GetPlanets() const;

	/** Open planets, for HUD objective markers. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Galaxy")
	TArray<AActor*> GetActiveMarkers() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Galaxy")
	ATDPortalGate* FindGate(FName GateId) const;

	const FTDGalaxyLayoutData& GetLayout() const { return Layout; }

	/** Build now (normally BeginPlay). Safe to call twice. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Galaxy")
	void Build();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Galaxy")
	bool IsBuilt() const { return bBuilt; }

	int32 GetAsteroidCount() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy")
	FTDGalaxyLayoutData Layout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy")
	bool bBuildOnBeginPlay = true;

	/** Spawn the engineering nebula sky (SM_TD_SkySphere) when its art exists; Nick's TD.Presentation.Sky wins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy")
	bool bSpawnSky = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy")
	bool bSpawnDominion = true;

	// --- Art hooks: BP subclasses / data assets Nick can fill without touching this code ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSubclassOf<ATDPortalGate> GateClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSubclassOf<ATDPlanetAtmosphere> PlanetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSubclassOf<ATDDominion> DominionClass;

	/** Portal FX for every gate. Empty = convention asset /Game/TriadDrift/Presentation/DA_Travel_FX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDTravelFX> TravelFX;

	/** Optional art meshes for asteroids / ruins (replace the placeholder sphere / cube, keep collision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSoftObjectPtr<UStaticMesh> AsteroidMesh;

	/** Girders / trusses (ruins with bModule false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSoftObjectPtr<UStaticMesh> RuinMesh;

	/** Station modules (ruins with bModule true). Empty = RuinMesh for both. Like RuinMesh, authored in a 100 cm
	 *  box and scaled by each ruin's extent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSoftObjectPtr<UStaticMesh> RuinModuleMesh;

	/** Optional art mesh for each gate frame: one ring (inner diameter ATDPortalGate::FrameMeshInnerDiameter at
	 *  scale 1). Defaults to the engineering ring; missing = the four placeholder bars. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSoftObjectPtr<UStaticMesh> GateFrameMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/TriadDrift/Gameplay/Props/SpaceDressing/SM_GateRing_01.SM_GateRing_01")));

	/** No asteroid within this distance of a gate, return point or the player start (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy", meta = (ClampMin = "0"))
	float AsteroidKeepOut = 7000.f;

	/** True if any keep-out zone (gates, return points, player start) contains Point within Margin. */
	bool IsInKeepOut(const FVector& LocalPoint, float Margin) const;

	/** Editor-only: draw the gameplay layout (planets, gates, Dominion, return points) in the viewport. */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|Galaxy")
	bool bEditorPreview = true;

	/** Hyperdrive drop-out radius around the Dominion (T1's gravity wells). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy", meta = (ClampMin = "0"))
	float DominionWellRadius = 60000.f;

	/**
	 * Hide our placeholder dressing (asteroid/ruin/planet meshes) when Nick's environment art is in
	 * the map. Collision, obstacles and gameplay are unchanged. Config: [/Script/unrealGameJam.TDGalaxyLayout]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "TriadDrift|Galaxy")
	bool bHidePlaceholderDressing = false;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Galaxy")
	void SetHidePlaceholderDressing(bool bHide);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void BuildInstances(bool bRegisterObstacles);
	void DrawEditorPreview() const;

	void BuildPlanets();
	void BuildGates();

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TObjectPtr<UInstancedStaticMeshComponent> Asteroids;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TObjectPtr<UInstancedStaticMeshComponent> RuinBlocks;

	/** Module ruins when RuinModuleMesh is set (otherwise all ruins are in RuinBlocks). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TObjectPtr<UInstancedStaticMeshComponent> RuinModules;

	/** The rocks' look: one no-collision instanced component per engineering asteroid mesh (SM_Asteroid_01..04),
	 *  each asteroid picks one from the layout seed. Gameplay collision stays on Asteroids (spheres, hidden). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> AsteroidLooks;

	/** Sparse debris between the asteroids (SM_SpaceDebris_01..03): visual only, no collision. */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> DebrisLooks;

	/** Parallel to AsteroidLooks: the family (0 classic, 1 ice, 2 metal, 3 violet crystal, 4 teal crystal) and the
	 *  scale that makes the mesh's bounding sphere 100 cm across (the dressing kit is real-scale). */
	TArray<uint8> AsteroidLookFamily;
	TArray<float> AsteroidLookNorm;

	/** Space dressing kit (visual only, no collision): ruin art over the hidden ruin collision boxes (girder truss,
	 *  hub, hull panel), debris clusters around the ruins, and sparse landmarks along the routes (satellites in high
	 *  orbit, cargo pod clusters on the lanes, beacon buoys lining the gate approaches). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> RuinArtLooks;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> DebrisClusterLooks;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> SatelliteLooks;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> CargoLooks;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> BuoyLooks;

	/** Small visual-only rocks lining the travel lanes (a hollow tube: the lane itself stays open). */
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> LaneRockLooks;

	void BuildDressing();
	int32 PickRockLook(const FTDAsteroidFieldSpec& F, FRandomStream& Rng) const;
	bool UsesRuinArt() const;

public:
	/** Debris pieces per asteroid field, as a fraction of its asteroid count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy", meta = (ClampMin = "0"))
	float DebrisPerAsteroid = 0.8f;

	/** Visual-only rocks per lane (zone to each planet, gate and the Dominion). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy", meta = (ClampMin = "0"))
	int32 LaneRocksPerLane = 70;

	/** The lane itself stays open within this distance of its centre line (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy", meta = (ClampMin = "0"))
	float LaneClearRadius = 9000.f;

	/** Space dressing on/off (landmarks, ruin art, debris clusters, rock families). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy")
	bool bSpaceDressing = true;

	/** Turn the landmarks, lane rocks and ruin art on or off at runtime (dev captures: TD.World.Dressing). */
	void SetSpaceDressing(bool bOn) { bSpaceDressing = bOn; BuildDressing(); }

	/** Every space-dressing component (tests: visual only, no collision). */
	TArray<UInstancedStaticMeshComponent*> GetDressingComponents() const;
	/** The family of each asteroid look (see AsteroidLookFamily) and its normalising scale. */
	const TArray<uint8>& GetAsteroidLookFamilies() const { return AsteroidLookFamily; }
	const TArray<float>& GetAsteroidLookNorms() const { return AsteroidLookNorm; }

	/** Instanced components carrying the asteroid look (tests: valid mesh and material on each). */
	const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& GetAsteroidLooks() const { return AsteroidLooks; }
	const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& GetDebrisLooks() const { return DebrisLooks; }
	/** Total visual asteroid instances (equals GetAsteroidCount when the engineering meshes are in use). */
	int32 GetAsteroidLookCount() const;
	/** The collision spheres (hidden when the rocks carry the look). */
	const UInstancedStaticMeshComponent* GetAsteroidCollision() const { return Asteroids; }
	/** Every gate this layout spawned (both ends of every pair). */
	const TArray<TObjectPtr<ATDPortalGate>>& GetGateActors() const { return GateActors; }
	/** Every place a ship appears: the player start and all return points. */
	TArray<FTransform> GetSpawnPoints() const;
protected:

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDPlanetAtmosphere>> Planets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDPortalGate>> GateActors;

	/** Parallel to GateActors: index into Layout.Gates. */
	TArray<int32> GatePairIndex;

	UPROPERTY(Transient)
	TObjectPtr<ATDDominion> Dominion;

	/** The engineering nebula sky (camera-centred), when its art exists. */
	UPROPERTY(Transient)
	TObjectPtr<class ATDSkySphere> SkySphere;

	float PhaseTimer = 0.f;
	bool bBuilt = false;
};
