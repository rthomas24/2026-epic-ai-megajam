// Triad Drift - the playable planet layouts (ENG-25 Verdance, ENG-26 Cinder), built from C++ around the pad.
// Owner: Teammate 5. Nick dresses the look later; this is the readable, finishable gameplay layout.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Ground/Crew/TDCrewTypes.h"
#include "TDPlanetCourse.generated.h"

class ATDObjective;
class ATDEncounterVolume;
class ATDInteractableActor;
class ATDDoor;
class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;

/** An encounter started when an objective step starts (wave drops, the charge defence). */
USTRUCT()
struct FTDStepEncounter
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<ATDObjective> Objective;
	UPROPERTY() int32 Step = 0;
	UPROPERTY() TObjectPtr<ATDEncounterVolume> Encounter;
};

/** A door opened when an objective step starts (the outpost gate after the conduits). */
USTRUCT()
struct FTDStepDoor
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<ATDObjective> Objective;
	UPROPERTY() int32 Step = 0;
	UPROPERTY() TObjectPtr<ATDDoor> Door;
};

/** One objective's trailhead: walking up to it starts the objective (its own Director's Cut scene). */
USTRUCT()
struct FTDTrailhead
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<ATDObjective> Objective;
	UPROPERTY() TObjectPtr<AActor> Marker;
	/** "THE WRECK", "THE RELAY": used in the idle objective line. */
	UPROPERTY() FText Name;
};

/**
 * Builds a planet's whole on-foot course in the empty L_Planet_* map (nothing is saved to the .umap):
 * ground, routes with glowing posts, set dressing, the two objectives with their encounters, props and
 * hazards. Both objectives can be done in either order: each starts when the player reaches its
 * trailhead. Spawned by ATDPlanetDirector at StartVisit, relative to the pad (+X = the ship's nose).
 */
UCLASS()
class UNREALGAMEJAM_API ATDPlanetCourse : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetCourse();

	/** "Verdance" or "Cinder". Set before BeginPlay (SpawnActorDeferred). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FName PlanetId = TEXT("Verdance");

	/** Radius around a trailhead marker that starts its objective (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	float TrailheadRadius = 700.f;

	static ATDPlanetCourse* Get(const UObject* WorldContext);

	ATDObjective* FindObjective(FName ObjectiveId) const;
	const TArray<FTDTrailhead>& GetTrailheads() const { return Trailheads; }

	/** True while any of this planet's objectives is running. */
	bool IsAnyObjectiveRunning() const;

	/** Between objectives on foot: "HEAD TO A SIGNAL · THE WRECK OR THE RELAY" and the trailheads still open. */
	bool GetIdleLine(FText& OutText, TArray<AActor*>& OutMarkers) const;

	/** Everything the course spawned (tests). */
	const TArray<TObjectPtr<AActor>>& GetSpawned() const { return Spawned; }

	/** Start an objective now (the trailhead does this; tests and the debug menu too). */
	bool StartObjectiveAt(FName ObjectiveId);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// Engine shapes and materials held by the CDO so a packaged build cooks them.
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> GlowMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> LitMaterial;

private:
	UFUNCTION() void HandleStepStarted(ATDObjective* Objective, int32 StepIndex);
	UFUNCTION() void HandleObjectiveCompleted(ATDObjective* Objective);

	void BuildVerdance();
	void BuildCinder();
	void MarkCompletedFromProgress();

	// --- Building helpers (all positions relative to the course actor = the pad) ---
	enum class EShape : uint8 { Cube, Cylinder, Sphere, Cone };
	UStaticMeshComponent* Block(EShape Shape, const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, float Glow = 0.f, const FRotator& Rot = FRotator::ZeroRotator, bool bCollide = true);
	/** A walkable box whose TOP is at Top (cm). */
	UStaticMeshComponent* Platform(const FVector& XY, float Top, const FVector& SizeCm, const FLinearColor& Color, float Glow = 0.f);
	void Route(const TArray<FVector>& Points, const FLinearColor& Color, float Spacing = 700.f);
	void ScatterFlora(int32 Count, float Radius, uint32 Seed);
	ATDObjective* MakeObjective(FName Id, const FText& Title, ETDAbilityId Reward, ETDShipId Ship, int32 StepCount);
	void FinishObjective(ATDObjective* Obj, const TArray<FText>& StepTexts, const TArray<AActor*>& Waypoints);
	ATDEncounterVolume* Encounter(ATDObjective* Obj, int32 LinkedStep, const FVector& Center, const FVector& Extent, const TArray<ETDCrewType>& Mix, bool bOnOverlap, float RingRadius = 600.f, const TArray<FVector>& Points = TArray<FVector>());
	template <class T> T* SpawnProp(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	void AddTrailhead(ATDObjective* Obj, const FVector& At, const FLinearColor& Color, const FText& Name);
	FVector W(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }

	bool IsOnRoute(const FVector& Local, float Clearance) const;

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Spawned;
	UPROPERTY(Transient) TArray<TObjectPtr<ATDObjective>> Objectives;
	UPROPERTY(Transient) TArray<FTDTrailhead> Trailheads;
	UPROPERTY(Transient) TArray<FTDStepEncounter> StepEncounters;
	UPROPERTY(Transient) TArray<FTDStepDoor> StepDoors;
	UPROPERTY(Transient) TObjectPtr<AActor> Dressing; // one actor holds every static block (cheap)
	TArray<TArray<FVector>> Routes;
	float TrailheadCheck = 0.f;
};
