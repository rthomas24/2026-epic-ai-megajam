// Triad Drift - World (Teammate 4). ENG-17 planet + atmosphere trigger (hand-off to ENG-20 landing).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDPlanetAtmosphere.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ATDSpacecraft;
class ATDPlanetAtmosphere;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDAtmosphereEnteredSignature, ATDPlanetAtmosphere*, Atmosphere, ATDSpacecraft*, Ship);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDAtmosphereOpenSignature, ATDPlanetAtmosphere*, Atmosphere, bool, bOpen);

/**
 * A landable planet: a solid placeholder sphere and a larger atmosphere trigger. The atmosphere is
 * only open while its planet marker is active (progress Phase == the planet's phase, or the bus
 * OnMarkerRevealed for its destination). When the player-controlled ship enters an open atmosphere it
 * fires OnShipEnteredAtmosphere; T1's ENG-20 entry/landing binds that. Until ENG-20 takes over
 * (SetAutoTravel(false)), it calls Flow->TravelTo(Destination, ReturnPoint) after EntryDelaySeconds.
 */
/** A planet's look: surface, atmosphere shell and optional ring, each with an optional material. Every mesh is
 *  authored around a planet of radius 100 cm at scale 1 and scaled from the planet's radius. Collision stays the
 *  gameplay sphere. Empty or missing assets fall back to the tinted placeholder sphere. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDPlanetLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TSoftObjectPtr<UStaticMesh> Surface;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TSoftObjectPtr<UMaterialInterface> SurfaceMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TSoftObjectPtr<UStaticMesh> Atmosphere;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TSoftObjectPtr<UMaterialInterface> AtmosphereMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TSoftObjectPtr<UStaticMesh> Ring;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") TSoftObjectPtr<UMaterialInterface> RingMaterial;
	/** Ring tilt (degrees about the planet's forward axis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") float RingTiltDeg = 20.f;
	/** Self-rotation (degrees per second); 0.3 = one turn per 20 minutes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet") float SpinDegPerSecond = 0.3f;
};

UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDPlanetAtmosphere : public AActor
{
	GENERATED_BODY()

public:
	ATDPlanetAtmosphere();

	void Configure(ETDDestination InDestination, FName InReturnPoint, float InPlanetRadius, float InAtmosphereRadius, const FLinearColor& InColor);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	ETDDestination GetDestination() const { return Destination; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	FName GetReturnPoint() const { return ReturnPoint; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	FVector GetPlanetCenter() const { return GetActorLocation(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	float GetPlanetRadius() const { return PlanetRadius; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	float GetAtmosphereRadius() const { return AtmosphereRadius; }

	/** Open = the marker is up and ships can enter. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Planet")
	void SetOpen(bool bInOpen);

	/** ENG-20 calls SetAutoTravel(false) once it handles entry itself. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Planet")
	void SetAutoTravel(bool bInAutoTravel) { bAutoTravel = bInAutoTravel; }

	/** Re-read progress to decide whether the atmosphere is open. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Planet")
	void SyncWithProgress();

	/** Phase in which this planet is the destination (Verdance -> Verdance, Cinder -> Cinder). */
	static ETDGamePhase PhaseFor(ETDDestination Destination);

	/** Fallback when the entering ship has no ENG-20 entry component: travel directly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	bool bAutoTravel = true;

	/** Gravity well radius = AtmosphereRadius x this (orbit return points sit at ~1.3x). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "1"))
	float GravityWellRadiusScale = 1.8f;

	/** "Planet_Verdance" etc. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Planet")
	FName GetWellId() const;

	/** Hide the placeholder sphere (keeps its collision). */
	void SetPlaceholderVisible(bool bVisible);

	/** Optional art (surface / atmosphere / ring). Set before BeginPlay (the galaxy layout passes its spec's Look). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	FTDPlanetLook Look;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "0"))
	float EntryDelaySeconds = 1.f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Planet")
	FTDAtmosphereEnteredSignature OnShipEnteredAtmosphere;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Planet")
	FTDAtmosphereOpenSignature OnOpenChanged;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleMarkerRevealed(ETDDestination InDestination);

	void TryEnter(ATDSpacecraft* Ship);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Planet")
	TObjectPtr<USphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Planet")
	TObjectPtr<UStaticMeshComponent> PlanetMesh;	// gameplay collision (and the placeholder look)

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Planet")
	TObjectPtr<USceneComponent> SpinPivot;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Planet")
	TObjectPtr<UStaticMeshComponent> SurfaceLook;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Planet")
	TObjectPtr<UStaticMeshComponent> AtmosphereLook;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Planet")
	TObjectPtr<USceneComponent> RingPivot;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Planet")
	TObjectPtr<UStaticMeshComponent> RingLook;

	bool bArtLook = false;
	bool bSunSynced = false;
	float SunSyncTimer = 0.f;
	void ApplyLook();
	/** Sets the surface material's SunDirection to the space sun; false until a sun light exists. */
	bool SyncSunDirection();

public:
	/** The sun planets light from: the atmosphere sun light, else the brightest visible directional light. */
	static class UDirectionalLightComponent* FindActiveSun(UWorld* World);
protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Planet")
	ETDDestination Destination = ETDDestination::Verdance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Planet")
	FName ReturnPoint = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Planet")
	float PlanetRadius = 60000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Planet")
	float AtmosphereRadius = 75000.f;

	FLinearColor Color = FLinearColor(0.1f, 0.9f, 0.5f);
	float PendingTravel = -1.f;
	float PollTimer = 0.f;
	bool bOpen = false;
	bool bEntered = false;
	/** The squad ship's entry this planet started (T1, review M1): when it goes back to idle without travelling
	 *  (Pause > Restart Scene, death, a failed or timed-out travel) the planet re-arms once the ship is outside. */
	TWeakObjectPtr<class UTDAtmosphereEntryComponent> EntryInFlight;
	bool bRearmOnExit = false;

public:
	/**
	 * ENG-38 stage 2: the globe shows the same world as the ground. A runtime bake (FTDGlobeBake, off the game thread)
	 * of the planet's terrain field replaces the surface material's BaseColorTex / EmissiveTex (the art's normal,
	 * ORM and clouds stay), with a beacon at the landing zone (the actor's +Z). The surface stops spinning so the
	 * landing site stays put under the descent mapping. `td.PlanetGlobeBake 0` keeps the old art look.
	 */
	bool IsGlobeBaked() const { return bGlobeBaked; }
	/** True while any planet in the world is still baking its globe off-thread (issue #89: hold the cover on it). */
	static bool AreGlobeBakesRunning(const UObject* WorldContext);
	class UTexture2D* GetGlobeBaseTexture() const { return GlobeBase; }

	/** Calibration of the sphere mesh's UVs against the globe frame: yaw (deg) of the surface, and mirrored U. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	float GlobeYawOffsetDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	bool bGlobeMirrorU = true;	// measured: SM_Planet_Sphere's U runs westward (lon 90 showed on the -Y side)

	/**
	 * The globe frame (FTDPlanetEntry): its +Z is the landing zone, +X longitude 0. With the bake on, the landing
	 * zone faces GlobeFacePoint (the combat zone: the side the squad arrives from), so the pad's beacon is in view;
	 * otherwise the actor's own rotation.
	 */
	FQuat GetGlobeQuat() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet")
	FVector GlobeFacePoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "64"))
	int32 GlobeBakeWidth = 2048;

	/**
	 * The landing site's marker on the globe: a small soft light at the landing zone that reads from orbit as "something
	 * is down there", holds about BeaconAngularDeg of the view, and fades out as the ship closes: gone inside
	 * BeaconHideRadii planet radii from the centre (well before the dive's white cover).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "0"))
	float BeaconAngularDeg = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "1"))
	float BeaconHideRadii = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Planet", meta = (ClampMin = "1"))
	float BeaconFullRadii = 6.f;

	bool IsBeaconVisible() const;
	/** Scale the beacon had last frame (tests). */
	float GetBeaconWorldSize() const;
	/** Updates the beacon for a viewer at ViewLocation (Tick does this with the camera; tests call it directly). */
	void UpdateBeacon(const FVector& ViewLocation);

protected:
	void StartGlobeBake();
	void ApplyGlobeBake(TArray<FColor>&& Base, TArray<FColor>&& Emissive, int32 W, int32 H);

	UPROPERTY(Transient)
	TObjectPtr<class UTexture2D> GlobeBase;

	UPROPERTY(Transient)
	TObjectPtr<class UTexture2D> GlobeEmissive;

	bool bGlobeBaked = false;
	bool bGlobeBaking = false;
	bool bGlobeFacing = false;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PadBeacon;
	FQuat GlobeQuat = FQuat::Identity;
};
