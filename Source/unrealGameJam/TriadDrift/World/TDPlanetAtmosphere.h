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
	void ApplyLook();
	/** Sets the surface material's SunDirection to the space sun; false until a sun light exists. */
	bool SyncSunDirection();

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
};
