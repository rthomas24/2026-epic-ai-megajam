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

/** One area the world scatter (T4, WORLD-DENSITY-PLAN) must leave clear, in world space. */
USTRUCT(BlueprintType)
struct FTDCourseKeepOut
{
	GENERATED_BODY()
	/** What it protects: "Pad", "Route", "Site", "Encounter", "Spawn", "Structure". */
	UPROPERTY(BlueprintReadOnly, Category = "Planet") FName Kind;
	/** A box (Box valid) or a polyline with a half width (Points, HalfWidth). 2D: Z is ignored by the query. */
	UPROPERTY(BlueprintReadOnly, Category = "Planet") FBox Box = FBox(ForceInit);
	UPROPERTY(BlueprintReadOnly, Category = "Planet") TArray<FVector> Points;
	UPROPERTY(BlueprintReadOnly, Category = "Planet") float HalfWidth = 0.f;
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

	/**
	 * T4's world scatter: may something of Radius (cm) stand at this world point? False on the pad and the ships'
	 * parking ring, within the walking width of a route, on an objective site, inside an encounter volume, near a crew
	 * spawn point or on the course's own colliding structures. The shapes behind it: GetKeepOuts (world space).
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	bool IsScatterAllowed(const FVector& WorldPoint, float Radius) const;
	/** Every keep-out shape, in world space (built once, after the course is built). */
	const TArray<FTDCourseKeepOut>& GetKeepOuts() const;
	/** Static helper: the first course in the world (or null) answers; no course = allowed. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet", meta = (WorldContext = "WorldContextObject"))
	static bool IsScatterAllowedAt(const UObject* WorldContextObject, const FVector& WorldPoint, float Radius);

	/** Clear width either side of a route's centre line (cm). */
	static constexpr float RouteHalfWidth = 700.f;
	/** The pad and the parked squad (cm, from the course origin). */
	static constexpr float PadClearRadius = 3500.f;

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
	struct FCoreRing { TWeakObjectPtr<UStaticMeshComponent> Ring; float YawSpeed = 0.f; float Tilt = 0.f; };
	TArray<FCoreRing> CoreRings;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> CoreShield;
	UPROPERTY(Transient) TObjectPtr<AActor> InteriorExposure;
	/** The final record (story 07-09): the reactor door opens only once it has played (or was skipped). */
	UPROPERTY(Transient) TObjectPtr<class ATDDoor> ReactorDoor;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ParkedShip;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> ParkedParts;
	int32 ConduitsDown = 0;
	UPROPERTY(Transient) TObjectPtr<AActor> FinalRecordConsole;
	float FinalRecordElapsed = -1.f;
	float FinalRecordQuiet = 0.f;
	bool bFinalRecordDone = false;
	void TickFinalRecord(float DeltaSeconds);
	void TickCoreChamber();
	FBox CoreChamberBox = FBox(ForceInit);
	bool bPlayerInCoreChamber = false;
	UFUNCTION() void HandleStorySequenceFinished(FName SequenceId, bool bSkipped);
	int32 ShownPlaybackSecond = -1;
	bool bFinalRecordSequence = false;
	UFUNCTION() void HandleConduitDown(class UTDHealthComponent* Health, const struct FTDDamageInfo& Info);
	/** N-052: the reactor room's defenders overcome: T3's Face to Face "overcome" sequence for the boarding pilot. */
	UFUNCTION() void HandleReactorDefendersCleared(AActor* Encounter);
	UFUNCTION() void HandleHeartHit(class UTDHealthComponent* Health, const struct FTDDamageInfo& Info, float ShieldDamage, float HullDamage);
	/** DA_ReactorCore_FX (UTDReactorCoreFX): play a slot (logged by name, even when empty: tests, the effects builder). */
	void PlayReactorFX(FName Slot, const FVector& At, USceneComponent* AttachTo = nullptr, bool bLoop = false);
	void StopReactorLoop(FName Slot);
	UPROPERTY(Transient) TMap<FName, TObjectPtr<class UNiagaraComponent>> ReactorLoopFX;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<class UAudioComponent>> ReactorLoopAudio;
	UPROPERTY(Transient) TObjectPtr<AActor> ReactorHeart;
	UPROPERTY(Transient) TObjectPtr<class ATDCollapseRun> EscapeCollapse;
	TArray<FName> ReactorFXLog;
	void OpenReactorDoor();
public:
	/** The final record started (T3 drives the F_* lines from here; the reactor door waits for the queue to end). */
	DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnFinalRecord, AActor*);
	static FTDOnFinalRecord OnFinalRecordStarted;
	/** The interior's music moments (T1's bridge for Nick's score, N-089), each once when it happens: CoreChamberEntered
	 *  (the on-foot pilot crosses into the core chamber; again after a retake that restarts outside it),
	 *  ReactorFightStarted (the conduit phase begins), ReactorFightEnded (the heart is destroyed), EscapeStarted (the
	 *  escape run's hand-off). Source = the course. */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FTDOnInteriorMoment, FName /*Moment*/, AActor* /*Source*/);
	static FTDOnInteriorMoment OnInteriorMoment;
	/** The core chamber's floor box (world), for the CoreChamberEntered crossing (tests). */
	FBox GetCoreChamberBox() const { return CoreChamberBox; }
	/** Every dressing piece placed, as "<Site>:<Name>" (tests, captures). */
	const TArray<FName>& GetDressLog() const { return DressLog; }
	/** The one actor holding the course's static blocks and dressing meshes (tests). */
	AActor* GetDressingActor() const { return Dressing; }
	/** The interior's escape run: seconds left while it runs, else -1 (the environment builder's last-30-seconds state). */
	float GetEscapeSecondsLeft() const;
	/** The interior's room lights: point lights on the dressing actor tagged TD.InteriorLight. */
	static constexpr const TCHAR* InteriorLightTag = TEXT("TD.InteriorLight");
	/** Component tag on the interior kit pieces that carry the red strip glow (their strip slot has its own instance). */
	static constexpr const TCHAR* EmergencyStripTag = TEXT("TD.EmergencyStrip");

	/** Cinder's magma (T7's play, Nov 14): the magma volume that just killed the pilot (ATDMagmaVolume calls it). */
	void NoteMagmaDeath(const AActor* Magma);
	/** The director's respawn spot for a CUT on foot: after MagmaRescueAfterDeaths magma deaths in a row in the same
	 *  magma, the far side of it instead. True when it moved the spot. */
	bool ResolveRespawn(FTransform& InOutWhere);
	static constexpr int32 MagmaRescueAfterDeaths = 2;
	/** Where a CUT in the interior picks up: the scene's checkpoint, else the next room's door (a death between rooms,
	 *  scene None: T7 came back at the hangar, 104 m from the reactor). */
	FTransform GetInteriorRetakeSpot(FName SceneId) const;
	/** The reactor room (N-052 / N-060): the boarder's own future self (staged, never fights), and the fight of the other
	 *  two future pilots (or the two drones when td.Interior.Defenders is 0). */
	class ATDFutureSelf* GetFutureSelf() const { return FutureSelf; }
	class ATDEncounterVolume* GetReactorDefenders() const { return ReactorDefenders; }
	UPROPERTY(Transient) TObjectPtr<class ATDFutureSelf> FutureSelf;
	UPROPERTY(Transient) TObjectPtr<class ATDEncounterVolume> ReactorDefenders;
	/** The interior's doorways, in order along +X: the marker leads through the next one square-on (T7, Nov 14: the
	 *  walker wedged on the arena's far door frame heading straight for the Warden's trailhead). */
	TArray<TWeakObjectPtr<AActor>> InteriorDoorways;
	/** Each doorway's approach points on its centre line, 2.5 m before and after it (same order as InteriorDoorways). */
	TArray<TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>>> DoorApproaches;
	/** Target, or the centre of the first doorway between the pilot and it (interior, along +X). */
	AActor* RouteThroughDoors(const FVector& Pilot, AActor* Target) const;
	/** The course's objectives in story order (the order they were made). */
	const TArray<TObjectPtr<ATDObjective>>& GetObjectives() const { return Objectives; }
	/** Where a magma rescue puts the pilot, per magma volume (tests). */
	bool GetMagmaRescue(const AActor* Magma, FTransform& Out) const;
	/** Every DA_ReactorCore_FX slot played, in order (CoreIdle, ConduitBreak, ShieldDrop, HeartHit, CoreDestroyed,
	 *  EscapeRunLoop), for tests and the effects builder. */
	const TArray<FName>& GetReactorFXLog() const { return ReactorFXLog; }
	bool IsFinalRecordDone() const { return bFinalRecordDone; }
	/** The interior: the boarding pilot's ship on the hangar pad (null when its art is missing). */
	UStaticMeshComponent* GetParkedShip() const { return ParkedShip; }
	/** The parked ship's visual parts (DA_<Ship>_Visuals Parts: the Rift's portal ring), as the flying ship has them. */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetParkedParts() const { return ParkedParts; }
	AActor* GetFinalRecordConsole() const { return FinalRecordConsole; }
	/** Seconds the reactor door waits at most for the record (the queue can be long; a skip ends it early). */
	float FinalRecordMaxSeconds = 90.f;
private:
	UPROPERTY(Transient) TObjectPtr<class ATDArchiveTransfer> ArchiveTransfer;
	UPROPERTY(Transient) TObjectPtr<class ATDSurveyTerminal> SurveyTerminal;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> DormantDrone;
	/**
	 * Grok Bot's survey outpost (BP_Outpost_Site_Verdance / _Cinder; ArtSource/EngProps/Outpost/export/
	 * Outpost_Site.manifest.json): spawned at Site (course-local, on the ground), +X (its bay and ramp) turned to Yaw.
	 * Returns null when the asset is missing (the caller keeps its placeholder pieces). OutConsole: its console mesh.
	 */
	/** Places the kit outpost; Site.Z is raised onto the ground under it (T4's terrain, 3 cm over: #42, the room showed
	 *  the terrain through its floor). */
	AActor* PlaceOutpost(FVector& Site, float Yaw, bool bCinder, UStaticMeshComponent*& OutConsole);
	/** The ground's height under a course-local point: the highest world-static surface that is not the course's own
	 *  (T4's terrain), else 0 (the course slab). */
	float GroundZAt(const FVector& Local) const;
	/** One dressing piece in a site's frame: a static mesh ("SM_...", on the course's dressing actor) or a blueprint
	 *  ("BP_..."). Folder is under /Game/TriadDrift/Gameplay/Props/. */
	struct FDressItem { const TCHAR* Folder; const TCHAR* Name; FVector Local; float Yaw; float Scale; bool bCollide; };
	/** Places Items in the frame (Origin, Yaw) on the ground, clear of flora (a site-clear circle each); returns how many
	 *  were placed (missing assets are skipped and logged). */
	int32 Dress(const TArray<FDressItem>& Items, const FVector& Origin, float Yaw, FName Site);
	AActor* SpawnKitBP(const TCHAR* Folder, const TCHAR* Name, const FVector& Local, float Yaw, float Scale = 1.f);
	/** The outpost's exterior props in its frame (+X = the door side), clear of the door apron and the room. */
	void DressOutpost(const FVector& Site, float Yaw, FName Tag);
	/** The pad's ring of bollard lights and its service corner, outside the 14 m take-off circle (+X = take-off). */
	void DressPad(FName Tag);
	TArray<FName> DressLog;
	/** An invisible interact box over the outpost console, at its 87 x 260 x 150 cm, facing Yaw. */
	template <class T> T* OutpostTerminal(const FVector& Site, float Yaw);
	/** Terminal -> its outpost console screen and planet (the scan on the screen when used, T2's ShowScanOnScreen). */
	UPROPERTY(Transient) TMap<TObjectPtr<AActor>, TObjectPtr<UStaticMeshComponent>> TerminalScreens;
	UFUNCTION() void HandleTerminalScreen(AActor* Interactable, AActor* Interactor);
public:
	AActor* GetOutpost() const { return SiteOutpost; }
private:
	UPROPERTY(Transient) TObjectPtr<AActor> SiteOutpost;
	/** The kit relay (BP_Relay) when placed: woken (RelayPower) when Obj_Relay completes. */
	UPROPERTY(Transient) TObjectPtr<AActor> RelayWake;
public:
	class ATDArchiveTransfer* GetArchiveTransfer() const { return ArchiveTransfer; }
	class ATDSurveyTerminal* GetSurveyTerminal() const { return SurveyTerminal; }
	/** Cinder: the field anchor is disabled (T2's pending approach marker toward the Dominion). */
	DECLARE_MULTICAST_DELEGATE(FTDOnFieldAnchorDisabled);
	static FTDOnFieldAnchorDisabled OnFieldAnchorDisabled;
	/** T2's pending approach marker (ENG-26 step 5: "approach prepared; passage still blocked"): the Cinder field anchor
	 *  is down this run (saved, so true after a reload or Continue). The passage itself opens elsewhere (T4: Cinder done
	 *  and wave 5 cleared); this never says it is open. */
	static bool IsFieldAnchorDisabled(const UObject* WorldContextObject);
	UStaticMeshComponent* GetDormantDrone() const { return DormantDrone; }
	/** Cinematic 03's drone (features.md row "drone wakes blue, three pulses"): its optic, dark until C3_Wren_Answered,
	 *  then three blue blinks in step with T3's light and steady blue; the drone lifts off the deck as it wakes. */
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> DroneOptic;
	FVector DroneHome = FVector::ZeroVector;
	double DroneWakeStart = -1.0;
	UFUNCTION() void HandleStoryLine(const struct FTDTransmissionView& Line);
	void TickDroneWake();
private:
	float InteriorTime = 0.f;
	/** Story beats on objective moments (T3's transmissions, storyline.md §5): planets and interior. */
	void FireStepBeats(ATDObjective* Objective, int32 StepIndex);
	void FireCompletionBeats(ATDObjective* Objective);
public:
	UStaticMeshComponent* GetCoreShield() const { return CoreShield; }
private:
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
	/** A walkable kit site's scatter-clear circle (course-local): no flora or rock within Radius plus its own clearance. */
	struct FSiteClear { FVector Center; float Radius; };
	TArray<FSiteClear> SiteClears;
	void ClearSite(const FVector& Local, float Radius) { SiteClears.Add({ Local, Radius }); }
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
	struct FMagmaRescue { TWeakObjectPtr<const AActor> Magma; FTransform Far; };
	TArray<FMagmaRescue> MagmaRescues;
	TWeakObjectPtr<const AActor> PendingMagmaDeath;
	TWeakObjectPtr<const AActor> LastMagmaDeath;
	int32 MagmaDeathsInRow = 0;
	/** The reactor's four conduits and their step counter: the marker points at the nearest one still standing. */
	/** A step whose targets are several things to break (the reactor's conduits, Cinder's gate conduits): its marker is
	 *  the nearest one still standing, the step's own waypoint (the counter) once none is. */
	struct FMarkerGroup
	{
		TWeakObjectPtr<ATDObjective> Objective;
		int32 Step = 0;
		TWeakObjectPtr<AActor> Counter;
		TArray<TWeakObjectPtr<AActor>> Targets;
	};
	TArray<FMarkerGroup> MarkerGroups;
	void UpdateConduitMarker();
	/** A magma the marker routes around (chain of local points, safe way across) and its rescue spot (local, yaw). */
	void AddMagmaCrossing(const AActor* Magma, ATDObjective* Objective, int32 Step, const TArray<FVector>& Chain, const FVector& RescueAt, float RescueYaw);

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
	UPROPERTY(Transient) mutable TArray<FTDCourseKeepOut> KeepOutCache;
	mutable bool bKeepOutBuilt = false;
	void BuildKeepOuts() const;
	UPROPERTY(Transient) TArray<FTDStepDoor> StepDoors;
	UPROPERTY(Transient) TObjectPtr<AActor> Dressing; // one actor holds every static block (cheap)
	TArray<TArray<FVector>> Routes;
	float TrailheadCheck = 0.f;
};
