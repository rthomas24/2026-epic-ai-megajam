// Triad Drift - World (Teammate 4). ENG-17 galaxy gameplay layout, data-driven and spawned at runtime.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDGalaxyLayout.generated.h"

class UInstancedStaticMeshComponent;
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
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDGalaxyLayout : public AActor
{
	GENERATED_BODY()

public:
	ATDGalaxyLayout();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Galaxy", meta = (WorldContext = "WorldContext"))
	static ATDGalaxyLayout* Get(const UObject* WorldContext);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy")
	bool bSpawnDominion = true;

	/** Re-apply the play bounds to spacecraft this often (catches respawns and new enemies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Galaxy", meta = (ClampMin = "0.1"))
	float BoundsRefreshSeconds = 2.f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	void BuildAsteroids();
	void BuildRuins();
	void BuildPlanets();
	void BuildGates();
	void ApplyBoundsToShips();
	void UpdateGatePhases();

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TObjectPtr<UInstancedStaticMeshComponent> Asteroids;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Galaxy")
	TObjectPtr<UInstancedStaticMeshComponent> RuinBlocks;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDPlanetAtmosphere>> Planets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATDPortalGate>> GateActors;

	/** Parallel to GateActors: index into Layout.Gates. */
	TArray<int32> GatePairIndex;

	UPROPERTY(Transient)
	TObjectPtr<ATDDominion> Dominion;

	float BoundsTimer = 0.f;
	float PhaseTimer = 0.f;
	bool bBuilt = false;
};
