// Triad Drift - World (Teammate 4). ENG-16 permanent portal gates (galaxy shortcuts).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Powers/TDPortalTransit.h"
#include "TDPortalGate.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UTDTravelFX;
class ATDPortalGate;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDGateTransitSignature, ATDPortalGate*, Gate, AActor*, Traveller);

/**
 * One end of a permanent two-way gate pair. Anything that crosses the gate's disc (spacecraft from
 * the combat registry, projectiles from UTDProjectileSubsystem) comes out of the partner gate via
 * T2's shared UTDPortalTransit (ENG-09 rules: keep speed, 0.3 s invulnerability, enemies lose
 * tracking, enemy shots are returned). Detection is a swept plane crossing each tick, so it works
 * at hyperdrive speed and for collision-less projectiles.
 *
 * Facing: entering along this gate's forward (+X) exits along the partner's forward; entering from
 * the back exits out of the partner's back.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDPortalGate : public AActor
{
	GENERATED_BODY()

public:
	ATDPortalGate();

	/** Link two gates both ways. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Gate")
	static void LinkGates(ATDPortalGate* A, ATDPortalGate* B);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Gate")
	ATDPortalGate* GetPartner() const { return Partner; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Gate")
	void SetGateActive(bool bInActive);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Gate")
	bool IsGateActive() const { return bActive && Partner != nullptr; }

	/** Total transits through this gate (tests, stats). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Gate")
	int32 GetTransitCount() const { return TransitCount; }

	/**
	 * Pure maths: does the segment From -> To cross a disc of Radius centred at GateTransform's origin,
	 * facing its +X? bOutForward = crossed travelling along +X.
	 */
	static bool SegmentCrossesGate(const FTransform& GateTransform, float Radius, const FVector& From, const FVector& To, bool& bOutForward);

	/** Gate id for layout, return points and debugging ("Gate_Verdance_A"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate")
	FName GateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate")
	TObjectPtr<ATDPortalGate> Partner;

	/** Disc radius travellers must pass within (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate", meta = (ClampMin = "100"))
	float Radius = 2500.f;

	/** Inactive gates are visible but closed (the flank pair opens for waves 3-5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate")
	bool bActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate")
	bool bTransitProjectiles = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate")
	FTDPortalTransitParams TransitParams;

	/** Optional art for the frame (replaces the placeholder bars, keeps collision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TSoftObjectPtr<UStaticMesh> FrameMeshOverride;

	/** Nick's travel FX: PortalIdle (attached loop; hides the placeholder disc), PortalTransit at both ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Presentation")
	TObjectPtr<UTDTravelFX> TravelFX;

	/** Ships teleported to within this depth of the gate plane (and inside 1.5x the radius) ignore it for ArrivalGuardSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate", meta = (ClampMin = "0"))
	float ArrivalGuardDepth = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Gate", meta = (ClampMin = "0"))
	float ArrivalGuardSeconds = 1.5f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Gate")
	FTDGateTransitSignature OnTransit;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void LayoutParts();

	UFUNCTION()
	void HandleSpacecraftTeleported(AActor* Spacecraft, FVector From, FVector To);

	bool TryTransit(AActor* Traveller, float DeltaSeconds);
	void UpdateLook();

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Gate")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Gate")
	TArray<TObjectPtr<UStaticMeshComponent>> Frame;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Gate")
	TObjectPtr<UStaticMeshComponent> Surface;

	int32 TransitCount = 0;
	double LastTransitTime = -1000.0;
	TArray<AActor*> Scratch;
	TMap<TWeakObjectPtr<AActor>, double> ArrivalBlockedUntil;
};
