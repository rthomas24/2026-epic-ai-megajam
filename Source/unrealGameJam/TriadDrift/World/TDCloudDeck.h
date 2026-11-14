// Triad Drift - World (environment builder). ENG-38 / ENG-39: the planet's cloud deck.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDCloudDeck.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class ATDPlanetTerrain;

/**
 * One layer of cloud over a planet map: a flat disc of radius DeckRadius at DeckAltitude above the course floor (the
 * pad), centred on the planet terrain, bent down beyond the collidable core exactly like the terrain's far ring
 * (ATDPlanetTerrain::CurvatureDrop), so its edge always lies below the horizon. The look is all material
 * (MI_CloudDeck_<Planet>: world-projected drifting sheet, sparse near, solid towards the horizon, faded around a camera
 * inside it, soft where it meets a mountain). The descent glides down through it; from the 1.6 km ceiling the player
 * looks across its top to the rim. Visual only: no collision, no shadow. td.CloudDeck 0 = none.
 */
UCLASS(NotPlaceable)
class UNREALGAMEJAM_API ATDCloudDeck : public AActor
{
	GENERATED_BODY()

public:
	ATDCloudDeck();

	static const FName CloudDeckTag;	// "TD.Engineering.CloudDeck"

	/** Spawns the deck once per planet map (a later call returns the same actor); null when td.CloudDeck is 0. */
	static ATDCloudDeck* EnsureForWorld(UWorld* World, ETDDestination Planet);

	/** Height of the deck above the course floor (cm), per planet. */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|CloudDeck") float VerdanceAltitude = 115000.f;
	UPROPERTY(EditAnywhere, Category = "TriadDrift|CloudDeck") float CinderAltitude = 125000.f;
	/** Radius of the disc (cm); the terrain's far ring reaches 40 km. */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|CloudDeck") float DeckRadius = 6000000.f;
	/** Seconds to wait for the planet terrain before building at the world origin with no bend. */
	UPROPERTY(EditAnywhere, Category = "TriadDrift|CloudDeck") float TerrainWaitSeconds = 12.f;

	ETDDestination GetPlanet() const { return Planet; }
	bool IsBuilt() const { return bBuilt; }
	float GetAltitude() const { return Planet == ETDDestination::Cinder ? CinderAltitude : VerdanceAltitude; }
	UProceduralMeshComponent* GetMesh() const { return Mesh; }

	/** Builds the disc now at the terrain (if any) and stops waiting. */
	void Build();

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|CloudDeck") TObjectPtr<UProceduralMeshComponent> Mesh;

private:
	ETDDestination Planet = ETDDestination::Verdance;
	bool bBuilt = false;
	float WaitSeconds = 0.f;
};
