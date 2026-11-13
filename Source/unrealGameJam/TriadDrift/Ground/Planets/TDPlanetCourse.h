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
	/** "THE RESEARCH SITE", "THE RELAY": used in the idle objective line. */
	UPROPERTY() FText Name;
	/** Ordered courses (the Dominion interior): this trailhead only starts once After is complete. */
	UPROPERTY() TObjectPtr<ATDObjective> After;
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

	/** "Verdance", "Cinder" or "DominionInterior" (ENG-29, TDDominionInteriorCourse.cpp). Set before BeginPlay (SpawnActorDeferred). */
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

	/** Between objectives on foot: "HEAD TO A SIGNAL · THE RESEARCH SITE OR THE RELAY" and the trailheads still open. */
	bool GetIdleLine(FText& OutText, TArray<AActor*>& OutMarkers) const;

	/** True if a point (course-local) is within Clearance of a route (tests: nothing collides on a route). */
	bool IsRouteBlockedAt(const FVector& Local, float Clearance) const { return IsOnRoute(Local, Clearance); }

	/** Everything the course spawned (tests). */
	const TArray<TObjectPtr<AActor>>& GetSpawned() const { return Spawned; }

	/** Start an objective now (the trailhead does this; tests and the debug menu too). */
	bool StartObjectiveAt(FName ObjectiveId);

	// --- Dominion interior (ENG-29/30/31) ---
	bool IsInterior() const { return PlanetId == TEXT("DominionInterior"); }
	/** Where the pilot starts: the hangar, or the entry of the first room not yet done (a Continue mid-interior). */
	FTransform GetInteriorStart() const;
	/** The interior's objectives, in play order (hangar corridor, arena, Warden, core, escape). */
	static const TArray<FName>& InteriorObjectiveIds();
	/** Story consoles (one per room): StoryId per console; using one plays its recording (T3's transmissions). */
	const TMap<TObjectPtr<AActor>, FName>& GetStoryConsoles() const { return StoryConsoles; }
	/** Tests: the last story beat a console asked for. */
	FName GetLastStoryBeat() const { return LastStoryBeat; }
	/** Seconds the escape run gives (features.md ENG-31: 90). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior", meta = (Units = "s"))
	float EscapeSeconds = 90.f;
	/** Tests: no travel at the end of the escape run (the hand-off is still recorded in progress). */
	bool bInteriorTravel = true;
	bool HasHandedOff() const { return bHandedOff; }

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

	/**
	 * Engineering's planet props and trees (Gameplay/Props, Gameplay/Vegetation) by short name. Soft references
	 * on the CDO (the cooker follows them; loading the PVE trees during module start-up asserts in PCG), loaded
	 * when the course is built.
	 */
	UPROPERTY() TMap<FName, TSoftObjectPtr<UStaticMesh>> PropRefs;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UStaticMesh>> PropMeshes;

private:
	UFUNCTION() void HandleStepStarted(ATDObjective* Objective, int32 StepIndex);
	UFUNCTION() void HandleObjectiveCompleted(ATDObjective* Objective);

	void BuildVerdance();
	void BuildCinder();
	void BuildInterior();
	UFUNCTION() void HandleStoryConsole(AActor* Interactable, AActor* Interactor);
	void HandInteriorOver();
	void HandleInteriorObjectiveCompleted(ATDObjective* Objective);
	void StartEscape();
	UPROPERTY(Transient) TMap<TObjectPtr<AActor>, FName> StoryConsoles;
	/** Doors opened when an objective completes (interior room exits); kept open when loaded complete. */
	UPROPERTY(Transient) TArray<FTDStepDoor> CompletionDoors;
	/** Interior: where each objective's room is entered (Continue starts at the first one not done). */
	UPROPERTY(Transient) TMap<FName, FTransform> InteriorEntries;
	FName LastStoryBeat;
	bool bHandedOff = false;
	FTimerHandle EscapeTimer;
	void MarkCompletedFromProgress();

	// --- Building helpers (all positions relative to the course actor = the pad) ---
	enum class EShape : uint8 { Cube, Cylinder, Sphere, Cone };
	UStaticMeshComponent* Block(EShape Shape, const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, float Glow = 0.f, const FRotator& Rot = FRotator::ZeroRotator, bool bCollide = true, FName Art = NAME_None);
	/** Art for a code-built block: PropRefs[Name] when set, else Gameplay/Props/Planet/<Name> (shown fitted to the block). */
	UStaticMesh* ArtFor(FName Name);

	/** The ground slab's material per planet (world-aligned tiling); the slab keeps its placeholder look without it. */
	UPROPERTY(EditAnywhere, Category = "Art") TSoftObjectPtr<UMaterialInterface> GroundMaterialVerdance;
	UPROPERTY(EditAnywhere, Category = "Art") TSoftObjectPtr<UMaterialInterface> GroundMaterialCinder;
	/** A walkable box whose TOP is at Top (cm). */
	UStaticMeshComponent* Platform(const FVector& XY, float Top, const FVector& SizeCm, const FLinearColor& Color, float Glow = 0.f, FName Art = NAME_None);
	void Route(const TArray<FVector>& Points, const FLinearColor& Color, float Spacing = 700.f);
	void ScatterFlora(int32 Count, float Radius, uint32 Seed);

	/** One kind of dressing mesh: how many, how big, and whether it collides (only when well off the routes). */
	struct FScatter
	{
		FName Mesh;
		int32 Count = 0;
		FVector2D Scale = FVector2D(1.f, 1.f);
		bool bCollide = false;
		float Clearance = 600.f;
		FVector Center = FVector::ZeroVector;
		FVector2D Radius = FVector2D(4000.f, 16000.f);
		float Sink = 10.f;
	};
	/** Instanced, seeded scatter of real meshes (one component per mesh). False if a mesh is missing. */
	bool ScatterMeshes(const TArray<FScatter>& Kinds, uint32 Seed);
	/** Tall markers along each route, alternating sides. */
	void MarkRoutes(FName Mesh, float Spacing, float Offset, const FVector2D& Scale);
	bool IsKeptClear(const FVector& Local, float Clearance) const;
	struct FKeepOut { FVector Center; FVector Extent; };
	TArray<FKeepOut> KeepOuts;
	UStaticMesh* Prop(FName Name) const { const TObjectPtr<UStaticMesh>* M = PropMeshes.Find(Name); return M ? M->Get() : nullptr; }
	ATDObjective* MakeObjective(FName Id, const FText& Title, ETDAbilityId Reward, ETDShipId Ship, int32 StepCount);
	void FinishObjective(ATDObjective* Obj, const TArray<FText>& StepTexts, const TArray<AActor*>& Waypoints);
	ATDEncounterVolume* Encounter(ATDObjective* Obj, int32 LinkedStep, const FVector& Center, const FVector& Extent, const TArray<ETDCrewType>& Mix, bool bOnOverlap, float RingRadius = 600.f, const TArray<FVector>& Points = TArray<FVector>());
	template <class T> T* SpawnProp(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	void AddTrailhead(ATDObjective* Obj, const FVector& At, const FLinearColor& Color, const FText& Name);
	/** Mark a step as a checkpoint: a retake restarts the scene there and the pilot comes back at At. */
	void Checkpoint(ATDObjective* Obj, int32 Step, const FVector& At, float Yaw = 0.f);
	AActor* Marker(const FVector& At, float Yaw = 0.f);
	FVector W(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }

	bool IsOnRoute(const FVector& Local, float Clearance) const;

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Spawned;

	/**
	 * The walkable ground slab (top z = 0): the gameplay box plus ~600 m (ENG-34: T4's terrain basin matches it and
	 * its hills start at its edge). Course-local. Dressing stays inside it, so nothing floats over or sinks into a hill.
	 */
	FBox2D GroundBox = FBox2D(FVector2D(-300000.f, -300000.f), FVector2D(300000.f, 300000.f));
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> GroundSlab;
	/** Hide the slab (collision kept) under the procedural terrain, whose flat basin draws the floor; retries while it spawns. */
	void AdoptTerrainMaterial();
	int32 TerrainMaterialTries = 0;
	FTimerHandle TerrainMaterialTimer;
	bool IsOnGround(const FVector& Local, float Margin) const
	{
		return GroundBox.ExpandBy(-Margin).IsInside(FVector2D(Local.X, Local.Y));
	}
public:
	const FBox2D& GetGroundBox() const { return GroundBox; }
	UStaticMeshComponent* GetGroundSlab() const { return GroundSlab; }
private:
	UPROPERTY(Transient) TArray<TObjectPtr<ATDObjective>> Objectives;
	UPROPERTY(Transient) TArray<FTDTrailhead> Trailheads;
	UPROPERTY(Transient) TArray<FTDStepEncounter> StepEncounters;
	UPROPERTY(Transient) TArray<FTDStepDoor> StepDoors;
	UPROPERTY(Transient) TObjectPtr<AActor> Dressing; // one actor holds every static block (cheap)
	TArray<TArray<FVector>> Routes;
	float TrailheadCheck = 0.f;
};
